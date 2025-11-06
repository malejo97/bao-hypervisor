#include <vm.h>
#include <config.h>
#include <arch/spmp.h>
#include <arch/fences.h>


void vm_arch_mem_prot_init(struct vm* vm)
{   
    /* Determine the index of the last writable vSPMP entry */
    csrs_vspmpswitch_write((uint64_t)(-1));
    ssize_t nentries = bit64_ffs(~csrs_vspmpswitch_read());
    if (nentries < 0) {
        nentries = 64;
    }

    /* Map whole VM address space in the last vSPMP entry as RWX */
    spmp_cfg_t cfg = (spmp_cfg_t){ .a = SPMPCFG_A_NAPOT, .r = 1, .w = 1, .x = 1 };
    csrs_vsiselect_write(CSR_SPMP_SISELECT_BASE + (nentries - 1));
    csrs_vsireg_write(0x3FFFFFFFFFFFFFUL);
    csrs_vsireg2_write((unsigned long)cfg.raw);

    csrs_vspmpswitch_write(1ULL << (nentries-1));
}
