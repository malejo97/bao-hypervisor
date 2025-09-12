/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <emul.h>
#include <vm.h>
#include <arch/barr.h>

static void emulate_barr_init_access(struct emul_access* acc, size_t chann, unsigned long write_val) {
    struct vcpu* vcpu = cpu()->vcpu;
    struct vm* vm = vcpu->vm;
    
    if (acc->arch.op != NO_OP) {
        /* Reads to BRnINIT always return zero */
        unsigned long psw = get_gmpsw();
        set_gmpsw(psw | PSW_Z);

        if (acc->arch.op == SET1 && acc->arch.byte_mask == 0x1) {
            vm->arch.vbarr[chann].chk_msk = 0;
            vm->arch.vbarr[chann].sync_msk = 0;
        }
    }
    else if (acc->write) {
        if ((write_val & 0x1U) != 0) {
            vm->arch.vbarr[chann].chk_msk = 0;
            vm->arch.vbarr[chann].sync_msk = 0;
        }
    }
    else {
        vcpu_writereg(vcpu, acc->reg, 0);
    }
}

static void emulate_barr_en_access(struct emul_access* acc, size_t chann, unsigned long write_val) {
    struct vcpu* vcpu = cpu()->vcpu;
    struct vm* vm = vcpu->vm;
    
    if (acc->arch.op != NO_OP) {

        acc->arch.byte_mask &= 0xFU;

        unsigned long psw = get_gmpsw();
        if (vm->arch.vbarr[chann].en_msk & acc->arch.byte_mask)
            set_gmpsw(psw & ~PSW_Z);
        else
            set_gmpsw(psw | PSW_Z);

        switch (acc->arch.op)
        {
            case SET1:
                vm->arch.vbarr[chann].en_msk |= (uint8_t)acc->arch.byte_mask;
            break;
            case NOT1:                
                vm->arch.vbarr[chann].en_msk = 
                    (uint8_t)((vm->arch.vbarr[chann].en_msk & acc->arch.byte_mask) ?
                                (vm->arch.vbarr[chann].en_msk & ~acc->arch.byte_mask) :
                                (vm->arch.vbarr[chann].en_msk | acc->arch.byte_mask));
            break;
            case CLR1:
                vm->arch.vbarr[chann].en_msk &= (uint8_t)(~acc->arch.byte_mask);
            break;
            /* TST1 only modifies the PSW.Z flag */
            default:
            break;
        }
    }
    else if (acc->write) {
        vm->arch.vbarr[chann].en_msk = write_val & 0xFU;
    }
    else {
        vcpu_writereg(vcpu, acc->reg, vm->arch.vbarr[chann].en_msk);
    }

    /* Set BRSYNC mask if all the participating vCPUs arrived to the barrier */
    if ((vm->arch.vbarr[chann].en_msk & vm->arch.vbarr[chann].chk_msk) 
                                        == vm->arch.vbarr[chann].en_msk) {
        vm->arch.vbarr[chann].sync_msk = vm->arch.vbarr[chann].en_msk;
    }
}

static void emulate_barr_chk_access(struct emul_access* acc, size_t chann, 
                                size_t vcpu_idx, unsigned long write_val) {
    UNUSED_ARG(write_val);
    struct vcpu* vcpu = cpu()->vcpu;
    struct vm* vm = vcpu->vm;
    
    if (acc->arch.op != NO_OP) {

        acc->arch.byte_mask &= 0x1U;

        unsigned long psw = get_gmpsw();
        if (vm->arch.vbarr[chann].chk_msk & (acc->arch.byte_mask << vcpu_idx))
            set_gmpsw(psw & ~PSW_Z);
        else
            set_gmpsw(psw | PSW_Z);

        /* BRCHK bit is set to 1 regardless of the written value */
        if (acc->arch.op != TST1) {
            vm->arch.vbarr[chann].chk_msk |= (uint8_t)(acc->arch.byte_mask << vcpu_idx);
        }
    }
    else if (acc->write) {
        vm->arch.vbarr[chann].chk_msk |= (uint8_t)(0x1U << vcpu_idx);
    }
    else {
        unsigned long read_val = (uint8_t)(vm->arch.vbarr[chann].chk_msk >> vcpu_idx) & 0x1U;
        vcpu_writereg(vcpu, acc->reg, read_val);
    }

    /* Set BRSYNC mask if all the participating vCPUs arrived to the barrier */
    if ((vm->arch.vbarr[chann].en_msk & vm->arch.vbarr[chann].chk_msk) 
                                        == vm->arch.vbarr[chann].en_msk) {
        vm->arch.vbarr[chann].sync_msk = vm->arch.vbarr[chann].en_msk;
    }
}

static void emulate_barr_sync_access(struct emul_access* acc, size_t chann, 
                                size_t vcpu_idx, unsigned long write_val) {
    struct vcpu* vcpu = cpu()->vcpu;
    struct vm* vm = vcpu->vm;
    
    if (acc->arch.op != NO_OP) {

        acc->arch.byte_mask &= 0x1U;
        uint8_t msk_idx = (uint8_t)(acc->arch.byte_mask << vcpu_idx);

        unsigned long psw = get_gmpsw();
        if (vm->arch.vbarr[chann].sync_msk & msk_idx)
            set_gmpsw(psw & ~PSW_Z);
        else
            set_gmpsw(psw | PSW_Z);

        switch (acc->arch.op) {
            case SET1:
                vm->arch.vbarr[chann].sync_msk |= msk_idx;
            break;
            case NOT1:                
                vm->arch.vbarr[chann].sync_msk = 
                    (uint8_t)((vm->arch.vbarr[chann].sync_msk & msk_idx) ?
                                (vm->arch.vbarr[chann].sync_msk & ~msk_idx) :
                                (vm->arch.vbarr[chann].sync_msk | msk_idx));
            break;
            case CLR1:
                vm->arch.vbarr[chann].en_msk &= (uint8_t)(~msk_idx);
            break;
            /* TST1 only modifies the PSW.Z flag */
            default:
            break;
        }
    }
    else if (acc->write) {
        vm->arch.vbarr[chann].sync_msk |= (uint8_t)((write_val & 0x1U) << vcpu_idx);
    }
    else {
        unsigned long read_val = (uint8_t)(vm->arch.vbarr[chann].sync_msk >> vcpu_idx) & 0x1U;
        vcpu_writereg(vcpu, acc->reg, read_val);
    }
}

bool vbarr_emul_handler(struct emul_access* acc) {
    struct vcpu* vcpu = cpu()->vcpu;

    size_t acc_offset = acc->addr - platform.arch.barr_addr;
    size_t reg_offset = acc_offset % 0x10;
    size_t chann = (acc_offset & 0xFFUL) / 0x10;
    unsigned long val = vcpu_readreg(vcpu, acc->reg);

    /* cfg region */
    size_t barr_cfg_bot = offsetof(struct barr_hw, cfg);
    size_t barr_cfg_top = sizeof(((struct barr_hw*)NULL)->cfg) + barr_cfg_bot;
    if (acc_offset >= barr_cfg_bot && acc_offset < barr_cfg_top) {
        
        /* BRnINIT */
        if (reg_offset == 0) {
            emulate_barr_init_access(acc, chann, val);
        }
        /* BRnEN */
        else if (reg_offset == 4) {
            emulate_barr_en_access(acc, chann, val);
        }
        else {
            if (!acc->write)
            vcpu_writereg(cpu()->vcpu, acc->reg, 0);
        }
        
        return true;
    }
    
    /* self region */
    size_t barr_self_bot = offsetof(struct barr_hw, self);
    size_t barr_self_top = sizeof(((struct barr_hw*)NULL)->self) + barr_self_bot;
    if (acc_offset >= barr_self_bot && acc_offset < barr_self_top) {
        
        /* BRnCHKS */
        if (reg_offset == 0) {
            emulate_barr_chk_access(acc, chann, vcpu->id, val);
        }
        /* BRnSYNCS */
        else if (reg_offset == 4) {
            emulate_barr_sync_access(acc, chann, vcpu->id, val);
        }
        else {
            if (!acc->write)
            vcpu_writereg(cpu()->vcpu, acc->reg, 0);
        }
        
        return true;
    }
    
    /* PEm region */
    size_t barr_pe_bot = offsetof(struct barr_hw, pe);
    size_t barr_pe_top = sizeof(((struct barr_hw*)NULL)->pe) + barr_pe_bot;
    if (acc_offset >= barr_pe_bot && acc_offset < barr_pe_top) {

        size_t pe_idx = (acc_offset / 100) - 8;

        /* BRnCHKm */
        if (reg_offset == 0) {
            emulate_barr_chk_access(acc, chann, pe_idx, val);
        }
        /* BRnSYNCm */
        else if (reg_offset == 4) {
            emulate_barr_sync_access(acc, chann, pe_idx, val);
        }
        else {
            if (!acc->write)
                vcpu_writereg(cpu()->vcpu, acc->reg, 0);
        }

        return true;
    }

    /* Ignore access */
    if (!acc->write && acc->arch.op == NO_OP)
        vcpu_writereg(cpu()->vcpu, acc->reg, 0);

    return true;
}

void vbarr_init(struct vm* vm) {

    if (cpu()->id == vm->master) {

        for (size_t i = 0; i < BARR_NUM_CHANNELS; i++) {
            vm->arch.vbarr[i].en_msk = 0;
            vm->arch.vbarr[i].chk_msk = 0;
            vm->arch.vbarr[i].sync_msk = 0;
        }

        vm->arch.barr_emul = (struct emul_mem){
            .va_base = platform.arch.barr_addr,
            .size = ALIGN(sizeof(struct barr_hw), PAGE_SIZE),
            .handler = vbarr_emul_handler,
        };
        vm_emul_add_mem(vm, &vm->arch.barr_emul);
    }
}
