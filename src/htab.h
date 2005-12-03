
/* htab.h - Structures and declarations needed for table hashing */

/* $Id: htab.h,v 1.3 2005/08/08 09:43:07 murrayma Exp $ */

#include "copyright.h"

#ifndef __HTAB_H
#define __HTAB_H

#include "config.h"
#include "db.h"

typedef struct hashentry HASHENT;
struct hashentry {
    char *target;
    int *data;
    int checks;
    struct hashentry *next;
};

typedef struct num_hashentry NHSHENT;
struct num_hashentry {
    int target;
    int *data;
    int checks;
    struct num_hashentry *next;
};

typedef struct hasharray HASHARR;
struct hasharray {
    HASHENT *element[800];
};

typedef struct num_hasharray NHSHARR;
struct num_hasharray {
    NHSHENT *element[800];
};

typedef struct hashtable HASHTAB;
struct hashtable {
    int hashsize;
    int mask;
    int checks;
    int scans;
    int max_scan;
    int hits;
    int entries;
    int deletes;
    int nulls;
    HASHARR *entry;
    int last_hval;		/* Used for hashfirst & hashnext. */
    HASHENT *last_entry;	/* like last_hval */
};

typedef struct num_hashtable NHSHTAB;
struct num_hashtable {
    int hashsize;
    int mask;
    int checks;
    int scans;
    int max_scan;
    int hits;
    int entries;
    int deletes;
    int nulls;
    NHSHARR *entry;
    int last_hval;
    NHSHENT *last_entry;
};

typedef struct name_table NAMETAB;
struct name_table {
    char *name;
    int minlen;
    int perm;
    int flag;
};

extern void hashinit(HASHTAB *, int);
extern void hashreset(HASHTAB *);
extern int hashval(char *, int);
extern int get_hashmask(int *);
extern int *hashfind(char *, HASHTAB *);
extern int hashadd(char *, int *, HASHTAB *);
extern void hashdelete(char *, HASHTAB *);
extern void hashflush(HASHTAB *, int);
extern int hashrepl(char *, int *, HASHTAB *);
extern void hashreplall(int *, int *, HASHTAB *);
extern char *hashinfo(const char *, HASHTAB *);
extern int *nhashfind(int, NHSHTAB *);
extern int nhashadd(int, int *, NHSHTAB *);
extern void nhashdelete(int, NHSHTAB *);
extern void nhashflush(NHSHTAB *, int);
extern int nhashrepl(int, int *, NHSHTAB *);
extern int search_nametab(dbref, NAMETAB *, char *);
extern NAMETAB *find_nametab_ent(dbref, NAMETAB *, char *);
extern void display_nametab(dbref, NAMETAB *, char *, int);
extern void interp_nametab(dbref, NAMETAB *, int, char *, char *, char *);
extern void listset_nametab(dbref, NAMETAB *, int, char *, int);
extern int *hash_nextentry(HASHTAB * htab);
extern int *hash_firstentry(HASHTAB * htab);
extern char *hash_firstkey(HASHTAB * htab);
extern char *hash_nextkey(HASHTAB * htab);
extern int *nhash_nextentry(NHSHTAB * htab);
extern int *nhash_firstentry(NHSHTAB * htab);

extern NAMETAB powers_nametab[];

#define nhashinit(h,s) hashinit((HASHTAB *)h, s)
#define nhashreset(h) hashreset((HASHTAB *)h)
#define nhashinfo(t,h) hashinfo(t,(HASHTAB *)h)

#endif
