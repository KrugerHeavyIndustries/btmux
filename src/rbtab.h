
/* htab.h - Structures and declarations needed for table hashing */

/* $Id: htab.h,v 1.3 2005/08/08 09:43:07 murrayma Exp $ */

#include "copyright.h"

#ifndef __HTAB_H
#define __HTAB_H

#include "config.h"
#include "db.h"
#include "rbtree.h"
#include "nametab.h"

struct rbtable {
    long long checks, scans, max_scan, hits, entries, deletes, nulls;
    rbtree tree;
    void *last;
};
typedef struct rbtable HASHTAB;
typedef struct rbtable RBTAB;
typedef struct rbtable NHSHTAB;

void hashinit(RBTAB *, int);

void hashreset(RBTAB *);

int hashval(char *, int);
int get_hashmask(int *);
int *hashfind(char *, RBTAB *);
int hashadd(char *, int *, RBTAB *);
void hashdelete(char *, RBTAB *);
void hashflush(RBTAB *, int);
int hashrepl(char *, int *, RBTAB *);
void hashreplall(int *, int *, RBTAB *);
char *hashinfo(const char *, RBTAB *);
int search_nametab(dbref, NAMETAB *, char *);
NAMETAB *find_nametab_ent(dbref, NAMETAB *, char *);
void display_nametab(dbref, NAMETAB *, char *, int);
void interp_nametab(dbref, NAMETAB *, int, char *, char *, char *);
void listset_nametab(dbref, NAMETAB *, int, char *, int);
int *hash_nextentry(RBTAB * htab);
int *hash_firstentry(RBTAB * htab);
char *hash_firstkey(RBTAB * htab);
char *hash_nextkey(RBTAB * htab);

void nhashinit(RBTAB *, int);
void nhashreset(RBTAB *);
int *nhash_nextentry(RBTAB * htab);
int *nhash_firstentry(RBTAB * htab);
char *nhashinfo(const char *, RBTAB *);
int *nhashfind(int, RBTAB *);
int nhashadd(int, int *, RBTAB *);
void nhashdelete(int, RBTAB *);
void nhashflush(RBTAB *, int);
int nhashrepl(int, int *, RBTAB *);
extern NAMETAB powers_nametab[];
#endif
