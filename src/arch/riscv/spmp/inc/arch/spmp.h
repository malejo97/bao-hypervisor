#ifndef SPMP_H
#define SPMP_H

#include <bao.h>
#include <arch/csrs.h>
#include <bitmap.h>
#include <list.h>

#define SPMP_MAX_NUM_ENTRIES 64

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

struct spmp {
    bool active;

    priv_t priv;

    // The maximum number of spmp entries is 64, thus 1 bit per entry
    uint64_t alloc_entries;
    uint64_t switchmsk;
    uint64_t locked;

    struct {
        spmp_cfg_t cfg;
        unsigned long addr;
    } entry[SPMP_MAX_ENTRIES];

    struct {
        struct list list;
        struct spmpe_node {
            node_t node;
            mpid_t mpid;
        } node[SPMP_MAX_ENTRIES];
    } order;
};

void spmp_init(struct spmp* spmp, priv_t priv);
static inline void spmp_set_active(struct spmp *spmp, bool active)
{
    spmp->active = active;
}

void spmp_restore(struct spmp* spmp);
void spmp_enable_hyp_whitelist_mode();

#endif /* SPMP_H */
