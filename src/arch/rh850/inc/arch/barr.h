/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved.
 */

#ifndef __ARCH_BARR_H__
#define __ARCH_BARR_H__

#include <bao.h>

#define BARR_NUM_CHANNELS   (16)

struct barr_hw {
    struct {
        uint8_t BRINIT;
        uint8_t pad0[0x4 - 0x1];
        uint8_t BREN;
        uint8_t pad1[0x10 - 0x5];
    } cfg[BARR_NUM_CHANNELS];

    struct {
        uint8_t BRCHK;
        uint8_t pad0[0x4 - 0x1];
        uint8_t BRSYNC;
        uint8_t pad1[0x10 - 0x5];
    } self[BARR_NUM_CHANNELS];

    uint8_t pad0[0x800 - 0x200];

    struct {
        struct {
            uint8_t BRCHK;
            uint8_t pad0[0x4 - 0x1];
            uint8_t BRSYNC;
            uint8_t pad1[0x10 - 0x5];
        } chann[BARR_NUM_CHANNELS];
    } pe[PLAT_CPU_NUM];
};

struct vbarr_ctx {
    uint8_t en_msk;
    uint8_t chk_msk;
    uint8_t sync_msk;
};

bool vbarr_emul_handler(struct emul_access* acc);
void vbarr_init(struct vm* vm);

#endif /* __ARCH_BARR_H__ */