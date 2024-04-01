#ifndef SPMP_H
#define SPMP_H

#include <bao.h>
#include <arch/csrs.h>

typedef union {
    struct {
        uint8_t r: 1;
        uint8_t w: 1;
        uint8_t x: 1;
        uint8_t a: 2;
        uint8_t res: 2;
        uint8_t s: 1;
    };

    uint8_t raw;
} spmp_cfg_t;

#endif /* SPMP_H */
