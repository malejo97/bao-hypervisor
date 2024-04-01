/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __ARCH_CPU_H__
#define __ARCH_CPU_H__

#include <bao.h>
#include <arch/csrs.h>
#include <list.h>
#include <bitmap.h>
#include <arch/spmp.h>

#define CPU_HAS_EXTENSION(EXT) (DEFINED(EXT))

extern cpuid_t CPU_MASTER;
struct cpu_arch {
    unsigned long extra_scratch;
    unsigned hart_id;
    unsigned plic_cntxt;
#ifdef MEM_PROT_MPU
    struct spmp spmp_hyp;
#endif
};

static inline struct cpu* cpu()
{
    return (struct cpu*)csrs_sscratch_read();
}

#endif /* __ARCH_CPU_H__ */
