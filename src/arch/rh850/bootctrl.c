/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <emul.h>
#include <vm.h>
#include <arch/bootctrl.h>

bool vbootctrl_emul_handler(struct emul_access* acc) {
    struct vcpu* vcpu = cpu()->vcpu;
    struct vm* vm = vcpu->vm;
    unsigned long notify = 0;

    /* Translate access */
    if (acc->arch.op != NO_OP) {
        size_t virt_id = INVALID_CPUID;

        for (size_t i = 0; i < vcpu->vm->cpu_num; i++) {
            if ((1U << i) & acc->arch.byte_mask) {
                virt_id = vm->vcpus[i].id;
                if (!vm->vcpus[i].arch.started) {
                    notify |= (1UL << vm->vcpus[i].phys_id);
                }
                break;
            }
        }

        if (virt_id == INVALID_CPUID)
            return true;

        unsigned long psw = get_gmpsw();
        if (vm->vcpus[virt_id].arch.started)
            set_gmpsw(psw & ~PSW_Z);
        else
            set_gmpsw(psw | PSW_Z);

        switch (acc->arch.op)
        {
            case SET1:
                vm->vcpus[virt_id].arch.started = true;
            break;
            case NOT1:                
                vm->vcpus[virt_id].arch.started = true;
            break;
            /* CLR1 accesses are ignored */
            /* TST1 only modifies the PSW.Z flag */
            default:
            break;
        }
    }
    else if (acc->write) {
        unsigned long val = vcpu_readreg(vcpu, acc->reg);
        for (size_t i = 0; i < vcpu->vm->cpu_num; i++) {
            if ((1U << i) & val) {
                if (!vm->vcpus[i].arch.started) {
                    notify |= 1UL << vm->vcpus[i].phys_id;
                }
                vm->vcpus[i].arch.started = true;
            }
        }
    }
    else {
        unsigned long val = 0;
        for (size_t i = 0; i < vcpu->vm->cpu_num; i++) {
            if (vm->vcpus[i].arch.started) {
                val |= 1UL << i;
            }
        }
        vcpu_writereg(vcpu, acc->reg, val);
    }

    /* Notify physical CPUs, if any */
    for (cpuid_t c = 0; c < platform.cpu_num; c++)
        if (notify & (1UL << c))
            interrupts_cpu_sendipi(c);

    return true;
}

void vbootctrl_init(struct vm* vm) {
    if (cpu()->id == vm->master) {
        vm->arch.bootctrl_emul = (struct emul_mem){
            .va_base = platform.arch.bootctrl_addr,
            .size = ALIGN(0x10, PAGE_SIZE),
            .handler = vbootctrl_emul_handler,
        };
        vm_emul_add_mem(vm, &vm->arch.bootctrl_emul);
    }
}
