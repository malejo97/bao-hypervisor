/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <config.h>
#include <emul.h>
#include <string.h>
#include <vm.h>
#include <arch/bootctrl.h>
#include <arch/ipir.h>
#include <arch/vintc.h>
#include <arch/barr.h>

void vm_arch_init(struct vm* vm, const struct vm_config* vm_config)
{
    UNUSED_ARG(vm_config);

    /* All VMs use MPID1 for memory protection */
    set_mpid1(vm->id);

    vintc_init(vm, &vm_config->platform.arch.intc);
    vipir_init(vm, &vm_config->platform.arch.ibus);
    vbarr_init(vm, &vm_config->platform.arch.ibus);
    vbootctrl_init(vm, &vm_config->platform.arch.bootctrl);
}

void vcpu_arch_init(struct vcpu* vcpu, struct vm* vm)
{
    UNUSED_ARG(vm);
    UNUSED_ARG(vcpu);
}

void vcpu_arch_reset(struct vcpu* vcpu, vaddr_t entry)
{
    struct vm* vm = vcpu->vm;
    
    memset(&vcpu->regs, 0, sizeof(struct arch_regs));

    vcpu_writepc(vcpu, entry);
    set_eipc(entry);

    vcpu->arch.started = vcpu->id == 0 ? true : false;
    
    /* Bao fixes the VMID as SPID to isolate VM memory regions */
    vcpu->regs.spid = vm->id;
    set_gmspid(vm->id);
    set_gmspidlist(0x0);

    set_gmmpm(GMMPM_GMPE);

    unsigned long eipswh = get_eipswh() & ~EIPSWH_GPID_MASK;
    set_eipswh(eipswh | (vm->id << EIPSWH_GPID_OFF));

    unsigned long fepswh = get_fepswh() & ~FEPSWH_GPID_MASK;
    set_fepswh(fepswh | (vm->id << FEPSWH_GPID_OFF));

    set_gmpeid(vcpu->id);

    /* clear guest-context exception registers */
    set_gmeipc(0x0);
    set_gmfepc(0x0);
    set_gmmea(0x0);
    set_gmmei(0x0);
    set_gmeiic(0x0);
    set_gmfeic(0x0);

    vintc_vcpu_reset(vcpu);
}

bool vcpu_arch_is_on(struct vcpu* vcpu)
{
    return vcpu->arch.started;
}

unsigned long vcpu_readreg(struct vcpu* vcpu, unsigned long reg)
{
    if (reg > 32) {
        ERROR("reading register out of bounds");
    }

    return vcpu->regs.gp_regs.r[reg];
}

void vcpu_writereg(struct vcpu* vcpu, unsigned long reg, unsigned long val)
{
    if (reg > 32) {
        ERROR("writing register out of bounds");
    }

    vcpu->regs.gp_regs.r[reg] = val;
}

unsigned long vcpu_readpc(struct vcpu* vcpu)
{
    return vcpu->regs.pc;
}

void vcpu_writepc(struct vcpu* vcpu, unsigned long val)
{
    vcpu->regs.pc = val;
}

void vcpu_restore_state(struct vcpu* vcpu)
{
    UNUSED_ARG(vcpu);
    ERROR("%s not implemented", __func__);
}

void vcpu_save_state(struct vcpu* vcpu)
{
    UNUSED_ARG(vcpu);
    ERROR("%s not implemented", __func__);
}
