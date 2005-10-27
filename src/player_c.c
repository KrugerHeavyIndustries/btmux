
/*
 * player_c.c -- Player cache routines 
 */

/*
 * $Id: player_c.c,v 1.3 2005/08/08 09:43:07 murrayma Exp $ 
 */

#include "copyright.h"
#include "config.h"

#include "mudconf.h"
#include "htab.h"
#include "externs.h"
#include "alloc.h"
#include "attrs.h"
#include "db.h"
#include "rbtree.h"

#ifndef STANDALONE

typedef struct player_cache {
    dbref player;
    int money;
    int queue;
    int qmax;
    int cflags;
    struct player_cache *next;
} PCACHE;

NHSHTAB pcache_htab;

PCACHE *pcache_head;

rbtree *pct;

#define	PF_DEAD		0x0001
#define	PF_REF		0x0002
#define	PF_MONEY_CH	0x0004
#define	PF_QMAX_CH	0x0008

static int pcache_compare(void *vleft, void *vright, void *arg) {
    int left = (int)vleft;
    int right = (int)vright;
    return (right-left);
}

void pcache_init(void)
{
#if 0
    pool_init(POOL_PCACHE, sizeof(PCACHE));
#endif 
    pct = rb_init(pcache_compare, NULL);
//    nhashinit((&pcache_htab), 15 * HASH_FACTOR);
    pcache_head = NULL;
}

static void pcache_reload1(player, pp)
dbref player;
PCACHE *pp;
{
    char *cp;

    cp = atr_get_raw(player, A_MONEY);
    if (cp && *cp)
	pp->money = atoi(cp);
    else
	pp->money = 0;

    cp = atr_get_raw(player, A_QUEUEMAX);
    if (cp && *cp)
	pp->qmax = atoi(cp);
    else if (!Wizard(player))
	pp->qmax = mudconf.queuemax;
    else
	pp->qmax = -1;
}


PCACHE *pcache_find(dbref player) {
    PCACHE *pp;

    if (!Good_obj(player) || !OwnsOthers(player))
        return NULL;
#if 0
    pp = (PCACHE *) nhashfind(player, &pcache_htab);
#endif
    pp = (PCACHE *) rb_find(pct, (void *)player);
    if (pp) {
        pp->cflags |= PF_REF;
        return pp;
    }
    // pp = alloc_pcache("pcache_find");
    pp = malloc(sizeof(PCACHE));
    pp->queue = 0;
    pp->cflags = PF_REF;
    pp->player = player;
    pcache_reload1(player, pp);
    pp->next = pcache_head;
    pcache_head = pp;
#if 0
    nuashadd(player, (int *) pp, &pcache_htab);
#endif
    rb_insert(pct, (void *)player, (void *)pp);
    return pp;
}

void pcache_reload(player)
dbref player;
{
    PCACHE *pp;

    pp = pcache_find(player);
    if (!pp)
	return;
    pcache_reload1(player, pp);
}

static void pcache_save(pp)
PCACHE *pp;
{
    IBUF tbuf;

    if (pp->cflags & PF_DEAD)
	return;
    if (pp->cflags & PF_MONEY_CH) {
	sprintf(tbuf, "%d", pp->money);
	atr_add_raw(pp->player, A_MONEY, tbuf);
    }
    if (pp->cflags & PF_QMAX_CH) {
	sprintf(tbuf, "%d", pp->qmax);
	atr_add_raw(pp->player, A_QUEUEMAX, tbuf);
    }
    pp->cflags &= ~(PF_MONEY_CH | PF_QMAX_CH);
}

void pcache_trim(void)
{
    PCACHE *pp, *pplast, *ppnext;

    pp = pcache_head;
    pplast = NULL;
    while (pp) {
	if (!(pp->cflags & PF_DEAD) && (pp->queue ||
		(pp->cflags & PF_REF))) {
	    pp->cflags &= ~PF_REF;
	    pplast = pp;
	    pp = pp->next;
	} else {
	    ppnext = pp->next;
	    if (pplast)
		pplast->next = ppnext;
	    else
		pcache_head = ppnext;
	    if (!(pp->cflags & PF_DEAD)) {
		pcache_save(pp);
#if 0
        nhashdelete(pp->player, &pcache_htab);
#endif
        rb_delete(pct, (void *)pp->player);
	    }
	    free_pcache(pp);
	    pp = ppnext;
	}
    }
}

void pcache_sync(void)
{
    PCACHE *pp;

    pp = pcache_head;
    while (pp) {
	pcache_save(pp);
	pp = pp->next;
    }
}

void pcache_purge(player)
dbref player;
{
    PCACHE *pp;

#if 0
    pp = (PCACHE *) nhashfind(player, &pcache_htab);
#endif
    pp = (PCACHE *) rb_find(pct, (void *)player);
    if (!pp)
	return;
    pp->cflags = PF_DEAD;
#if 0
    nhashdelete(pp->player, &pcache_htab);
#endif 
    rb_delete(pct, (void *)pp->player);
}

int a_Queue(player, adj)
dbref player;
int adj;
{
    PCACHE *pp;

    if (OwnsOthers(player)) {
	pp = pcache_find(player);
	if (pp)
	    pp->queue += adj;
	return pp->queue;
    }
    return 0;
}

void s_Queue(player, val)
dbref player;
int val;
{
    PCACHE *pp;

    if (OwnsOthers(player)) {
	pp = pcache_find(player);
	if (pp)
	    pp->queue = val;
    }
}

int QueueMax(player)
dbref player;
{
    PCACHE *pp;
    int m;

    m = 0;
    if (OwnsOthers(player)) {
	pp = pcache_find(player);
	if (pp) {
	    if (pp->qmax >= 0) {
		m = pp->qmax;
	    } else {
		m = mudstate.db_top + 1;
		if (m < mudconf.queuemax)
		    m = mudconf.queuemax;
	    }
	}
    }
    return m;
}

#endif

int Pennies(obj)
dbref obj;
{
    char *cp;

#ifndef STANDALONE
    PCACHE *pp;

    if (OwnsOthers(obj)) {
	pp = pcache_find(obj);
	if (pp)
	    return pp->money;
    }
#endif
    cp = atr_get_raw(obj, A_MONEY);
    return (safe_atoi(cp));
}

void s_Pennies(obj, howfew)
dbref obj;
int howfew;
{
    IBUF tbuf;

#ifndef STANDALONE
    PCACHE *pp;

    if (OwnsOthers(obj)) {
	pp = pcache_find(obj);
	if (pp) {
	    pp->money = howfew;
	    pp->cflags |= PF_MONEY_CH;
	}
    }
#endif
    sprintf(tbuf, "%d", howfew);
    atr_add_raw(obj, A_MONEY, tbuf);
}
