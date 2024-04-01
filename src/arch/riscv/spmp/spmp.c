#include <arch/spmp.h>

#include <mem.h>
#include <arch/csrs.h>

#define SPMPCFG_PERMS_MASK (SPMPCFG_W_BIT | SPMPCFG_X_BIT | SPMPCFG_R_BIT | SPMPCFG_S_BIT)

// This assumes all harts have the same number of SPMP entries
static size_t SPMP_NUM_ENTRIES;

static void spmp_set_icfg(mpid_t i, spmp_cfg_t cfg)
{
    size_t cfgi = i / 4;
    size_t cfgi_off = (i % 4) * 8;
    if (RV64 && ((cfgi & 1) != 0)) {
        cfgi -= 1;
        cfgi_off += 32;
    }
    unsigned long mask = SPMPCFG_ICFG_MSK << cfgi_off;
    unsigned long val = ((unsigned long)cfg.raw )<< cfgi_off;

    switch (cfgi) {
        case 0: csrs_spmpcfg0_clear(mask); csrs_spmpcfg0_set(val); break;
        case 1: csrs_spmpcfg1_clear(mask); csrs_spmpcfg1_set(val); break;
        case 2: csrs_spmpcfg2_clear(mask); csrs_spmpcfg2_set(val); break;
        case 3: csrs_spmpcfg3_clear(mask); csrs_spmpcfg3_set(val); break;
        case 4: csrs_spmpcfg4_clear(mask); csrs_spmpcfg4_set(val); break;
        case 5: csrs_spmpcfg5_clear(mask); csrs_spmpcfg5_set(val); break;
        case 6: csrs_spmpcfg6_clear(mask); csrs_spmpcfg6_set(val); break;
        case 7: csrs_spmpcfg7_clear(mask); csrs_spmpcfg7_set(val); break;
        case 8: csrs_spmpcfg8_clear(mask); csrs_spmpcfg8_set(val); break;
        case 9: csrs_spmpcfg9_clear(mask); csrs_spmpcfg9_set(val); break;
        case 10: csrs_spmpcfg10_clear(mask); csrs_spmpcfg10_set(val); break;
        case 11: csrs_spmpcfg11_clear(mask); csrs_spmpcfg11_set(val); break;
        case 12: csrs_spmpcfg12_clear(mask); csrs_spmpcfg12_set(val); break;
        case 13: csrs_spmpcfg13_clear(mask); csrs_spmpcfg13_set(val); break;
        case 14: csrs_spmpcfg14_clear(mask); csrs_spmpcfg14_set(val); break;
        case 15: csrs_spmpcfg15_clear(mask); csrs_spmpcfg15_set(val); break;            
    }
}

static void spmp_set_addr(mpid_t i, paddr_t addr)
{
    switch (i) {
        case 0:   csrs_spmpaddr0_write(addr); break;
        case 1:   csrs_spmpaddr1_write(addr); break;
        case 2:   csrs_spmpaddr2_write(addr); break;
        case 3:   csrs_spmpaddr3_write(addr); break;
        case 4:   csrs_spmpaddr4_write(addr); break;
        case 5:   csrs_spmpaddr5_write(addr); break;
        case 6:   csrs_spmpaddr6_write(addr); break;
        case 7:   csrs_spmpaddr7_write(addr); break;
        case 8:   csrs_spmpaddr8_write(addr); break;
        case 9:   csrs_spmpaddr9_write(addr); break;
        case 10: csrs_spmpaddr10_write(addr); break;
        case 11: csrs_spmpaddr11_write(addr); break;
        case 12: csrs_spmpaddr12_write(addr); break;
        case 13: csrs_spmpaddr13_write(addr); break;
        case 14: csrs_spmpaddr14_write(addr); break;
        case 15: csrs_spmpaddr15_write(addr); break;
        case 16: csrs_spmpaddr16_write(addr); break;
        case 17: csrs_spmpaddr17_write(addr); break;
        case 18: csrs_spmpaddr18_write(addr); break;
        case 19: csrs_spmpaddr19_write(addr); break;
        case 20: csrs_spmpaddr20_write(addr); break;
        case 21: csrs_spmpaddr21_write(addr); break;
        case 22: csrs_spmpaddr22_write(addr); break;
        case 23: csrs_spmpaddr23_write(addr); break;
        case 24: csrs_spmpaddr24_write(addr); break;
        case 25: csrs_spmpaddr25_write(addr); break;
        case 26: csrs_spmpaddr26_write(addr); break;
        case 27: csrs_spmpaddr27_write(addr); break;
        case 28: csrs_spmpaddr28_write(addr); break;
        case 29: csrs_spmpaddr29_write(addr); break;
        case 30: csrs_spmpaddr30_write(addr); break;
        case 31: csrs_spmpaddr31_write(addr); break;
        case 32: csrs_spmpaddr32_write(addr); break;
        case 33: csrs_spmpaddr33_write(addr); break;
        case 34: csrs_spmpaddr34_write(addr); break;
        case 35: csrs_spmpaddr35_write(addr); break;
        case 36: csrs_spmpaddr36_write(addr); break;
        case 37: csrs_spmpaddr37_write(addr); break;
        case 38: csrs_spmpaddr38_write(addr); break;
        case 39: csrs_spmpaddr39_write(addr); break;
        case 40: csrs_spmpaddr40_write(addr); break;
        case 41: csrs_spmpaddr41_write(addr); break;
        case 42: csrs_spmpaddr42_write(addr); break;
        case 43: csrs_spmpaddr43_write(addr); break;
        case 44: csrs_spmpaddr44_write(addr); break;
        case 45: csrs_spmpaddr45_write(addr); break;
        case 46: csrs_spmpaddr46_write(addr); break;
        case 47: csrs_spmpaddr47_write(addr); break;
        case 48: csrs_spmpaddr48_write(addr); break;
        case 49: csrs_spmpaddr49_write(addr); break;
        case 50: csrs_spmpaddr50_write(addr); break;
        case 51: csrs_spmpaddr51_write(addr); break;
        case 52: csrs_spmpaddr52_write(addr); break;
        case 53: csrs_spmpaddr53_write(addr); break;
        case 54: csrs_spmpaddr54_write(addr); break;
        case 55: csrs_spmpaddr55_write(addr); break;
        case 56: csrs_spmpaddr56_write(addr); break;
        case 57: csrs_spmpaddr57_write(addr); break;
        case 58: csrs_spmpaddr58_write(addr); break;
        case 59: csrs_spmpaddr59_write(addr); break;
        case 60: csrs_spmpaddr60_write(addr); break;
        case 61: csrs_spmpaddr61_write(addr); break;
        case 62: csrs_spmpaddr62_write(addr); break;
        case 63: csrs_spmpaddr63_write(addr); break;
    }
}
