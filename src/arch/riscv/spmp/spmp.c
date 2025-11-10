#include <arch/spmp.h>

#include <mem.h>
#include <arch/csrs.h>
#include <arch/fences.h>
#include <cpu.h>
#include <vm.h>

#define SPMPCFG_PERMS_MASK (SPMPCFG_W_BIT | SPMPCFG_X_BIT | SPMPCFG_R_BIT | SPMPCFG_S_BIT)

// This assumes all harts have the same number of SPMP entries
static size_t SPMP_NUM_ENTRIES;

static void spmp_set_icfg(mpid_t i, spmp_cfg_t cfg)
{
    csrs_siselect_write(i + CSR_SPMP_SISELECT_BASE);
    csrs_sireg2_write((unsigned long)cfg.raw);
    fence_i();
}

static void spmp_set_addr(mpid_t i, paddr_t addr)
{
    csrs_siselect_write(i + CSR_SPMP_SISELECT_BASE);
    csrs_sireg_write(addr);
    fence_i();
}

static inline bool spmp_reg_is_napot(struct mp_region *mem)
{
    return ((mem->size & (mem->size - 1)) == 0) && (((mem->size - 1) & mem->base) == 0);
}

static struct spmp* spmp_get_local(struct addr_space *as)
{
    struct spmp *spmp = NULL;

    if (as == &cpu()->as) {
        spmp = &cpu()->arch.spmp_hyp;
    } else {
        list_foreach(cpu()->vcpu_list, struct vcpu, vcpu) {
            if (as == &vcpu->vm->as) {
                spmp = &vcpu->arch.spmp;
                break;
            }
        }
    }

    return spmp;
}

void spmp_lock_entry(struct spmp* spmp, mpid_t mpid)
{
    spmp->locked = bit64_set(spmp->locked, mpid);
}

bool spmp_entry_locked(struct spmp* spmp,mpid_t mpid)
{
    return !!bit64_get(spmp->locked, mpid);
}

void spmp_set_entry(struct spmp* spmp, mpid_t i, struct mp_region* mem)
{
    spmp_cfg_t cfg = mem->mem_flags;
    
    if (spmp_reg_is_napot(mem)) {
        unsigned long addr = (mem->base >> 2) | ((mem->size-1) >> 3);
        if (mem->size == 4) {
            cfg.a = SPMPCFG_A_NA4;
        } else {
            cfg.a = SPMPCFG_A_NAPOT;
        }
        spmp->entry[i].cfg = cfg;
        spmp->entry[i].addr = addr;
        if (spmp->active) {
            spmp_set_addr(i, addr);
            spmp_set_icfg(i, cfg);
        }
    } else {
        cfg.a = SPMPCFG_A_TOR;
        spmp->entry[i - 1].cfg = (spmp_cfg_t) { .a = SPMPCFG_A_OFF };
        spmp->entry[i - 1].addr = mem->base >> 2;
        spmp->entry[i].addr = (mem->base + mem->size) >> 2;
        spmp->entry[i].cfg = cfg;
        if (spmp->active) {
            spmp_set_icfg(i - 1, spmp->entry[i - 1].cfg);
            spmp_set_addr(i - 1, spmp->entry[i - 1].addr);
            spmp_set_addr(i, spmp->entry[i].addr);
            spmp_set_icfg(i, spmp->entry[i].cfg);
        }
    }
}

void spmp_clear_entry(struct spmp* spmp, mpid_t i)
{
    if (spmp->entry[i].cfg.a == SPMPCFG_A_TOR) {
        spmp->entry[i].cfg = (spmp_cfg_t) { .a = SPMPCFG_A_OFF };
        spmp->entry[i].addr = 0;
        spmp->entry[i-1].addr = 0;
        if (spmp->active) {
            spmp_set_icfg(i, spmp->entry[i].cfg);
            spmp_set_addr(i, spmp->entry[i].addr);
            spmp_set_addr(i-1, spmp->entry[i-1].addr);
        }
    }
    else {
        spmp->entry[i].cfg = (spmp_cfg_t) { .a = SPMPCFG_A_OFF };
        spmp->entry[i].addr = 0;
        if (spmp->active) {
            spmp_set_icfg(i, spmp->entry[i].cfg);
            spmp_set_addr(i, spmp->entry[i].addr);
        }
    }
}

static mpid_t spmp_alloc_entry(struct spmp *spmp, struct mp_region* mem)
{
    mpid_t mpid = INVALID_MPID;
    size_t num_entries;
    if (spmp_reg_is_napot(mem)) {
        num_entries = 1;
    } else {
        num_entries = 2;
    }
    uint64_t free_entries = cpu()->arch.spmp_hyp.alloc_entries | spmp->alloc_entries;
    ssize_t entry = bitmap_find_consec((bitmap_t*)&free_entries, SPMP_NUM_ENTRIES, 0, 
        num_entries, false);
    if (entry >= 0) {
        bitmap_set_consecutive((bitmap_t*)&spmp->alloc_entries, entry, num_entries);
        mpid = (mpid_t)entry + num_entries - 1;
    }
    return mpid;
}

static void spmp_free_entry(struct spmp* spmp, mpid_t mpid)
{

    spmp->alloc_entries = bit64_clear(spmp->alloc_entries, mpid);

    if (spmp->entry[mpid].cfg.a == SPMPCFG_A_TOR) {
        spmp->alloc_entries = bit64_clear(spmp->alloc_entries, mpid - 1);
    }
}

static void spmp_entry_get_region(struct spmp* spmp, mpid_t mpid, struct mp_region *reg)
{
    unsigned long addr = spmp->entry[mpid].addr;
    reg->mem_flags = spmp->entry[mpid].cfg;
    if (spmp_reg_is_napot(reg)) {
        if (reg->mem_flags.a == SPMPCFG_A_NA4) {
            reg->size = 4;
        } else {
            reg->size = (1UL << (bit_ffs(~addr) + 3));
        }
        reg->base = (addr << 2) & (reg->size - 1);
    } else {
        reg->base = spmp->entry[mpid - 1].addr << 2;
        reg->size = (addr << 2) - reg->base; 
    }
    reg->as_sec = SEC_UNKNOWN;
}

static int spmp_node_cmp(void* cookie, node_t* _n1, node_t* _n2)
{
    struct spmp* spmp = (struct spmp*)cookie;
    struct spmpe_node* n1 = (struct spmpe_node*)_n1;
    struct spmpe_node* n2 = (struct spmpe_node*)_n2;
    struct mp_region r1;
    struct mp_region r2;
    spmp_entry_get_region(spmp, n1->mpid, &r1);
    spmp_entry_get_region(spmp, n2->mpid, &r2);
    if (r1.base > r2.base) {
        return 1;
    } else if (r1.base < r2.base) {
        return -1;
    } else {
        return 0;
    }
}

bool spmp_add_region(struct spmp *spmp, struct mp_region* reg)
{
    bool failed = true;

    if (reg->size > 0) {
        mpid_t mpid = spmp_alloc_entry(spmp, reg);
        if (mpid != INVALID_MPID) {
            failed = false;

            spmp_set_entry(spmp, mpid, reg);

            if (spmp->priv == PRIV_HYP) {
                csrs_sspmpswitch_set(1ULL << mpid);
            }
            else if (spmp->priv == PRIV_VM) {
                csrs_hspmpswitch_set(1ULL << mpid);
            }

            struct node_cmp spmp_list_node_cmp = { 
                .cmp = spmp_node_cmp,
                .cookie = spmp,
            };

            list_insert_ordered(&spmp->order.list,
                (node_t*)&spmp->order.node[mpid], &spmp_list_node_cmp);
        }
    }

    return !failed;
}

void spmp_remove_entry(struct spmp *spmp, mpid_t mpid)
{
    if (bit64_set(spmp->alloc_entries, mpid)) {

        spmp_clear_entry(spmp, mpid);
        spmp_free_entry(spmp, mpid);

        if (spmp->priv == PRIV_HYP) {
            csrs_sspmpswitch_clear(1ULL << mpid);
        }
        else if (spmp->priv == PRIV_VM) {
            csrs_hspmpswitch_clear(1ULL << mpid);
        }

        list_rm(&spmp->order.list, (node_t*)&spmp->order.node[mpid]);
    }
}


static bool spmp_perms_comptible(uint16_t perms1, uint16_t perms2)
{
    uint16_t perms_mask = SPMPCFG_SHARED_BIT | SPMPCFG_U_BIT | SPMPCFG_R_BIT | SPMPCFG_W_BIT | SPMPCFG_X_BIT;
    return (perms1 & perms_mask) == (perms2 & perms_mask);
}

void spmp_coalesce_contiguous(struct spmp* spmp) {

    while (true) {
        bool merge = false;
        mpid_t cur_mpid = INVALID_MPID;
        mpid_t prev_mpid = INVALID_MPID;
        struct mp_region prev_reg;
        struct mp_region cur_reg;
        list_foreach_tail(spmp->order.list, struct spmpe_node, cur, prev) {

            if (prev == NULL) {
                continue;
            }
            
            if (spmp_entry_locked(spmp, cur->mpid) || spmp_entry_locked(spmp, prev->mpid)) {
                continue;
            }

            spmp_entry_get_region(spmp, cur->mpid, &cur_reg);
            spmp_entry_get_region(spmp, prev->mpid, &prev_reg);

            bool contigous = mp_region_top(&prev_reg) == cur_reg.base;
            bool perms_compatible = 
                spmp_perms_comptible(prev_reg.mem_flags.raw, cur_reg.mem_flags.raw);
            if (contigous && perms_compatible) {
                cur_mpid = cur->mpid;
                prev_mpid = prev->mpid;
                // merge = true;
                break;
            }
        }

        if (merge) {
            spmp_remove_entry(spmp, cur_mpid);
            spmp_remove_entry(spmp, prev_mpid);
            struct mp_region merged_reg = {
                .base = prev_reg.base,
                .size = prev_reg.size + cur_reg.size,
                .mem_flags = cur_reg.mem_flags,
            };
            spmp_add_region(spmp, &merged_reg);
        } else {
            break;
        }
    }
}

static bool spmp_perms_valid(spmp_cfg_t *cfg)
{
    bool valid = true;

    if (cfg->w && !cfg->r) {
        valid = false;
    }

    return valid;
}


bool mpu_map(struct addr_space* as, struct mp_region* mem, bool locked) {

    bool failed = true;

    struct spmp *spmp = spmp_get_local(as);

    // We need to force the s bit according to the privilege of the spmp instance we are managing
    // However, maybe this should be done a priori.
    mem->mem_flags.u = !(spmp->priv == PRIV_HYP);

    if(spmp_perms_valid(&mem->mem_flags)) {
        mpid_t mpid = spmp_add_region(spmp, mem);
        if (mpid != INVALID_MPID) {
            failed = false;
            if (locked) {
                spmp_lock_entry(spmp, mpid);
            }
            spmp_coalesce_contiguous(spmp);
        }
    }

    return !failed;
}

bool mpu_unmap(struct addr_space* as, struct mp_region* mem) {

    struct mp_region target_reg = *mem;
    struct mp_region overlapped_reg;
    mpid_t overlapped_mpid;
    bool failed = false;

    struct spmp *spmp = spmp_get_local(as);

    while(target_reg.size > 0 && !failed) {

        overlapped_mpid = INVALID_MPID;
        list_foreach (spmp->order.list, struct spmpe_node, entry) {

            if (!bit64_get(spmp->alloc_entries, entry->mpid)) {
                continue;
            }

            spmp_entry_get_region(spmp, entry->mpid, &overlapped_reg);

            if (mp_region_top(&target_reg) < overlapped_reg.base) {
                continue;
            }

            if (mem_regions_overlap(&target_reg, &overlapped_reg)) {
                overlapped_mpid = entry->mpid;
                break;
            }
        }

        if (overlapped_mpid == INVALID_MPID)  {
            break;
        }

        if (spmp_entry_locked(spmp, overlapped_mpid)) {
            failed = true;
            break;
        }

        vaddr_t target_top = mp_region_top(&target_reg);
        vaddr_t overlapped_top = mp_region_top(&overlapped_reg);
        size_t top_size =  target_top <= overlapped_top ? overlapped_top - target_top : 0;
        size_t bottom_size =  target_reg.base >= overlapped_top ? overlapped_top - target_top : 0;

        spmp_remove_entry(spmp, overlapped_mpid);
        struct mp_region tmp_reg;
        if (top_size > 0) {
            tmp_reg.base = mp_region_top(&target_reg);
            tmp_reg.size = top_size;
            tmp_reg.mem_flags.raw = overlapped_reg.mem_flags.raw;
            spmp_add_region(spmp, &tmp_reg);
        }

        if (bottom_size > 0) {
            tmp_reg.base = overlapped_reg.base;
            tmp_reg.size = bottom_size;
            tmp_reg.mem_flags.raw = overlapped_reg.mem_flags.raw;
            spmp_add_region(spmp, &tmp_reg);
        }

        target_reg.size = sat_ul_sub(target_top, overlapped_top);
        target_reg.base = overlapped_top;
    }

    if (!failed) {
        spmp_coalesce_contiguous(spmp);
    }

    return !failed && target_reg.size == 0;
}

void mpu_init() {

    /* Get number of entries delegated to HS-mode */
    SPMP_NUM_ENTRIES = csrs_hspmpdeleg_read();

    // TODO: Check if number of entries is enough?

    /* We asssume that the SBI delegates all PMP resources not used by M-mode to HS-mode.
        We also assume that the SBI sets the last hSPMP entry for initial operation of the hypervisor*/
    for (mpid_t i = 0; i < SPMP_NUM_ENTRIES - 1; i+= 1) {
        spmp_set_icfg(i, (spmp_cfg_t){ .a = SPMPCFG_A_OFF });
    }

    csrs_sspmpswitch_write(1ULL << (SPMP_NUM_ENTRIES-1));

    // TODO: check for granularity and expand it to the mapping functions
}

void spmp_disable_sbi_region()
{
    unsigned long idx = CSR_SPMP_SISELECT_BASE + SPMP_NUM_ENTRIES - 1;

    /* We asssume that the SBI delegates all PMP resources not used by M-mode to HS-mode.
        We also assume that the SBI sets the last hSPMP entry for initial operation of the hypervisor*/
    csrs_siselect_write(idx);
    csrs_sireg_write(0);
    csrs_sireg2_write(0);

    csrs_sspmpswitch_clear(1ULL << (SPMP_NUM_ENTRIES-1));
}


void spmp_init(struct spmp* spmp, priv_t priv)
{
    spmp->alloc_entries = 0;
    spmp->priv = priv;
    spmp->locked = 0;

    list_init(&spmp->order.list);

    for (mpid_t i = 0; i < SPMP_MAX_NUM_ENTRIES; i++) {
        spmp->order.node[i].mpid = i;
    }
}

void spmp_restore(struct spmp* spmp)
{
    list_foreach (spmp->order.list, struct spmpe_node, entry) {
        mpid_t i = entry->mpid;
        spmp_set_addr(i, spmp->entry[i].addr);
        spmp_set_icfg(i, spmp->entry[i].cfg);
        if (spmp->entry[i].cfg.a == SPMPCFG_A_TOR) {
            spmp_set_icfg(i - 1, spmp->entry[i - 1].cfg);
            spmp_set_addr(i - 1, spmp->entry[i - 1].addr);
        }
    }
}
