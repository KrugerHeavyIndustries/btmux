
/* misc.h - miscellaneous structures that are needed in more than one file */

/* $Id: misc.h,v 1.2 2005/06/23 02:59:58 murrayma Exp $ */

#include "copyright.h"

#ifndef _MISC_H
#define _MISC_H

#include "db.h"
#include "flags.h"
#include "powers.h"

/* Search structure, used by @search and search(). */

typedef struct search_type SEARCH;
struct search_type {
    int s_wizard;
    dbref s_owner;
    dbref s_rst_owner;
    long s_rst_type;
    FLAGSET s_fset;
    POWERSET s_pset;
    dbref s_parent;
    dbref s_zone;
    char *s_rst_name;
    char *s_rst_eval;
    long low_bound;
    long high_bound;
};

/* Stats structure, used by @stats and stats(). */

typedef struct stats_type STATS;
struct stats_type {
    int s_total;
    int s_rooms;
    int s_exits;
    int s_things;
    int s_players;
    int s_garbage;
};

extern int search_setup(dbref, char *, SEARCH *);
extern void search_perform(dbref, dbref, SEARCH *);
extern int get_stats(dbref, dbref, STATS *);

#endif
