/*
 * pcache.h
 */

#ifndef __PCACHE_H__
#define __PCACHE_H__
#include "db.h"
#include "cque.h"
#include "rbtree.h"

typedef struct player_cache {
    dbref player;
    int money;
    int queue;
    int qmax;
    int cflags;
    struct player_cache *next;
} PCACHE;

/* Moved to the player_c.c file 
rbtree pcache_tree;
PCACHE *pcache_head;
*/

#define PF_DEAD     0x0001
#define PF_REF      0x0002
#define PF_MONEY_CH 0x0004
#define PF_QMAX_CH  0x0008

#endif
