/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#include <mem.h>
#include <cpu.h>

void as_arch_init(struct addr_space* as)
{
    if (as->type != AS_HYP) {
        return;
    }

    /**
     * Add hypervisor mpu entries set up during boot to the vmpu:
     *  - hypervisor image (loadable and non-loadable)
     *  - private cpu region
     */

    extern uint8_t _image_start, _image_load_end, _image_noload_start, _image_end;
    vaddr_t image_start = (vaddr_t)&_image_start;
    vaddr_t image_load_end = (vaddr_t)&_image_load_end;
    vaddr_t image_noload_start = (vaddr_t)&_image_noload_start;
    vaddr_t image_end = (vaddr_t)&_image_end;

    struct mp_region mpr;
    mpid_t mpid = 0;

    bool separate_noload_region = image_load_end != image_noload_start;
    vaddr_t first_region_end = separate_noload_region ? image_load_end : image_end;

    mpr = (struct mp_region){
        .base = image_start,
        .size = (size_t)(first_region_end - image_start),
        .mem_flags = PTE_HYP_FLAGS,
        .as_sec = SEC_HYP_IMAGE,
    };
    mem_vmpu_set_entry(&cpu()->as, mpid, &mpr);
    mpid++;

    if (separate_noload_region) {
        mpr = (struct mp_region){
            .base = image_noload_start,
            .size = (size_t)image_end - image_noload_start,
            .mem_flags = PTE_HYP_FLAGS,
            .as_sec = SEC_HYP_IMAGE,
        };
        mem_vmpu_set_entry(&cpu()->as, mpid, &mpr);
        mpid++;
    }

    mpr = (struct mp_region){
        .base = (vaddr_t)cpu(),
        .size = ALIGN(sizeof(struct cpu), PAGE_SIZE),
        .mem_flags = PTE_HYP_FLAGS,
        .as_sec = SEC_HYP_PRIVATE,
    };
    mem_vmpu_set_entry(&cpu()->as, mpid, &mpr);
}
