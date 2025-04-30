/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <bao.h>
#include <cpu.h>
#include <vm.h>
#include <platform.h>

void cpu_defines() __attribute__((used));
void cpu_defines()
{
    DEFINE_SIZE(CPU_SIZE, struct cpu);

    DEFINE_OFFSET(CPU_STACK_OFF, struct cpu, stack);
    DEFINE_SIZE(CPU_STACK_SIZE, ((struct cpu*)NULL)->stack);

    DEFINE_OFFSET(CPU_VCPU_OFF, struct cpu, vcpu);
    DEFINE_OFFSET(CPU_NEXT_VCPU_OFF, struct cpu, next_vcpu);

    DEFINE_OFFSET(CPU_ARCH_EXTRA_SCRATCH_OFF, struct cpu, arch.extra_scratch);

#ifdef MEM_PROT_MPU
    DEFINE_OFFSET(CPU_ARCH_SPMP_LOCKED_OFF, struct cpu, arch.spmp_hyp.locked);
#endif
}

void vcpu_defines() __attribute__((used));
void vcpu_defines()
{
    DEFINE_OFFSET(VCPU_REGS_X_OFF, struct vcpu, regs.x);
    DEFINE_OFFSET(VCPU_REGS_HSTATUS_OFF, struct vcpu, regs.hstatus);
    DEFINE_OFFSET(VCPU_REGS_SSTATUS_OFF, struct vcpu, regs.sstatus);
    DEFINE_OFFSET(VCPU_REGS_SEPC_OFF, struct vcpu, regs.sepc);
    DEFINE_OFFSET(VCPU_BLOCKED_COUNT, struct vcpu, blocked_count);
}
