#include <platform.h>
#include <interrupts.h>

struct platform platform = {

    .cpu_num = 1,

    .region_num = 1,
    .regions =  (struct mem_region[]) {
        {
            .base = 0x82200000,
            .size = 0x7E000000 - 0x2200000
        }
    },

    .arch = {
        .irqc.plic.base = 0x1c000000,
    }
};
