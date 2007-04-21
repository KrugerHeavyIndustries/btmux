
/* htab.h - Structures and declarations needed for table hashing */

/* $Id: htab.h,v 1.3 2005/08/08 09:43:07 murrayma Exp $ */

#include "copyright.h"

#ifndef __HTAB_H
#define __HTAB_H

#include "rbtree.h"

struct rbtable {
    long long checks, scans, max_scan, hits, entries, deletes, nulls;
    rbtree tree;
    void *last;
};
typedef struct rbtable HASHTAB;
typedef struct rbtable RBTAB;

void hashinit(RBTAB *, int);

void *hashfind(char *, RBTAB *);
int hashadd(char *, void *, RBTAB *);

#endif
