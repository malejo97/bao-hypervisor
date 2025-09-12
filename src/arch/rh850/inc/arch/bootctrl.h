/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __ARCH_BOOTCTRL_H__
#define __ARCH_BOOTCTRL_H__

#include <bao.h>
#include <platform.h>

struct vm;
struct emul_access;
struct vbootctrl_dscrp;

static inline void bootctrl_start_pe(cpuid_t cpuid) {
    volatile unsigned long* bootctrl = (unsigned long*)platform.arch.bootctrl_addr;

    (*bootctrl) |= (1UL << cpuid);
}

void vbootctrl_init(struct vm* vm, const struct vbootctrl_dscrp* vbootctrl_dscrp);
bool vbootctrl_emul_handler(struct emul_access* acc);

#endif /* __ARCH_BOOTCTRL_H__ */
