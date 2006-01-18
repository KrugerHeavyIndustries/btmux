
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
int get_hashmask(void *);
void *hashfind(char *, RBTAB *);
int hashadd(char *, void *, RBTAB *);
void hashdelete(char *, RBTAB *);
void hashflush(RBTAB *, int);
int hashrepl(char *, void *, RBTAB *);
void hashreplall(void *, void *, RBTAB *);
char *hashinfo(const char *, RBTAB *);
int search_nametab(dbref, NAMETAB *, char *);
NAMETAB *find_nametab_ent(dbref, NAMETAB *, char *);
void display_nametab(dbref, NAMETAB *, char *, int);
void interp_nametab(dbref, NAMETAB *, int, char *, char *, char *);
void listset_nametab(dbref, NAMETAB *, int, char *, int);
void *hash_nextentry(RBTAB * htab);
void *hash_firstentry(RBTAB * htab);
char *hash_firstkey(RBTAB * htab);
char *hash_nextkey(RBTAB * htab);

void nhashinit(RBTAB *, int);
void nhashreset(RBTAB *);
void *nhash_nextentry(RBTAB * htab);
void *nhash_firstentry(RBTAB * htab);
char *nhashinfo(const char *, RBTAB *);
void *nhashfind(int, RBTAB *);
int nhashadd(int, void *, RBTAB *);
void nhashdelete(int, RBTAB *);
void nhashflush(RBTAB *, int);
int nhashrepl(int, void *, RBTAB *);
extern NAMETAB powers_nametab[];
#endif
