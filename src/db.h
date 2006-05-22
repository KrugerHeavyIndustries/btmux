
/* db.h */

#include "copyright.h"

#ifndef	__DB_H
#define	__DB_H

#include "config.h"
#include "mudconf.h"

#include <sys/file.h>


#define	ITER_PARENTS(t,p,l)	for ((l)=0, (p)=(t); \
				     (Good_obj(p) && \
				      ((l) < mudconf.parent_nest_lim)); \
				     (p)=Parent(p), (l)++)

#define Hasprivs(x)      (Royalty(x) || Wizard(x))
int get_atr();

typedef struct attr ATTR;
struct attr {
    const char *name;		/* This has to be first.  braindeath. */
    int number;			/* attr number */
    int flags;
    int (*check)(int, dbref, dbref, int, char *);
};

typedef struct atrlist ATRLIST;
struct atrlist {
    char *data;			/* Attribute text. */
    int size;			/* Length of attribute */
    int number;			/* Attribute number. */
};

typedef struct stack STACK;
struct stack {
    char *data;
    STACK *next;
};

extern ATTR *atr_num(int anum);
extern ATTR *atr_str(char *s);

extern ATTR attr[];

extern ATTR **anum_table;

#define anum_get(x)	(anum_table[(x)])
#define anum_set(x,v)	anum_table[(x)] = v
extern void anum_extend(int);

#define	ATR_INFO_CHAR	'\1'	/* Leadin char for attr control data */

/* Boolean expressions, for locks */
#define	BOOLEXP_AND	0
#define	BOOLEXP_OR	1
#define	BOOLEXP_NOT	2
#define	BOOLEXP_CONST	3
#define	BOOLEXP_ATR	4
#define	BOOLEXP_INDIR	5
#define	BOOLEXP_CARRY	6
#define	BOOLEXP_IS	7
#define	BOOLEXP_OWNER	8
#define	BOOLEXP_EVAL	9

typedef struct boolexp BOOLEXP;
struct boolexp {
    boolexp_type type;
    struct boolexp *sub1;
    struct boolexp *sub2;
    dbref thing;		/* thing refers to an object */
};

#define	TRUE_BOOLEXP ((BOOLEXP *) 0)

/* Database format information */

#define	F_UNKNOWN	0	/* Unknown database format */
#define	F_MUSH		1	/* MUSH format (many variants) */
#define	F_MUSE		2	/* MUSE format */
#define	F_MUD		3	/* Old TinyMUD format */
#define	F_MUCK		4	/* TinyMUCK format */
#define F_MUX		5	/* TinyMUX format */

#define	V_MASK		0x000000ff	/* Database version */
#define	V_ZONE		0x00000100	/* ZONE/DOMAIN field */
#define	V_LINK		0x00000200	/* LINK field (exits from objs) */
#define	V_GDBM		0x00000400	/* attrs are in a gdbm db, not here */
#define	V_ATRNAME	0x00000800	/* NAME is an attr, not in the hdr */
#define	V_ATRKEY	0x00001000	/* KEY is an attr, not in the hdr */
#define	V_PERNKEY	0x00001000	/* PERN: Extra locks in object hdr */
#define	V_PARENT	0x00002000	/* db has the PARENT field */
#define	V_COMM		0x00004000	/* PERN: Comm status in header */
#define	V_ATRMONEY	0x00008000	/* Money is kept in an attribute */
#define	V_XFLAGS	0x00010000	/* An extra word of flags */
#define V_POWERS        0x00020000	/* Powers? */
#define V_3FLAGS	0x00040000	/* Adding a 3rd flag word */
#define V_QUOTED	0x00080000	/* Quoted strings, ala PennMUSH */

/* Some defines for DarkZone's flavor of PennMUSH */
#define DB_CHANNELS    0x2	/*  Channel system */
#define DB_SLOCK       0x4	/*  Slock */
#define DB_MC          0x8	/*  Master Create Time + modifed */
#define DB_MPAR        0x10	/*  Multiple Parent Code */
#define DB_CLASS       0x20	/*  Class System */
#define DB_RANK        0x40	/*  Rank */
#define DB_DROPLOCK    0x80	/*  Drop/TelOut Lock */
#define DB_GIVELOCK    0x100	/*  Give/TelIn Lock */
#define DB_GETLOCK     0x200	/*  Get Lock */
#define DB_THREEPOW    0x400	/*  Powers have Three Long Words */

/* special dbref's */
#define	NOTHING		(-1)	/* null dbref */
#define	AMBIGUOUS	(-2)	/* multiple possibilities, for matchers */
#define	HOME		(-3)	/* virtual room, represents mover's home */
#define	NOPERM		(-4)	/* Error status, no permission */
#define NOSLAVE         (-5)	/* Don't send to slaves */

typedef struct object OBJ;
struct object {
    dbref location;		/* PLAYER, THING: where it is */
    /* ROOM: dropto: */
    /* EXIT: where it goes to */
    dbref contents;		/* PLAYER, THING, ROOM: head of contentslist */
    /* EXIT: unused */
    dbref exits;		/* PLAYER, THING, ROOM: head of exitslist */
    /* EXIT: where it is */
    dbref next;			/* PLAYER, THING: next in contentslist */
    /* EXIT: next in exitslist */
    /* ROOM: unused */
    dbref link;			/* PLAYER, THING: home location */
    /* ROOM, EXIT: unused */
    dbref parent;		/* ALL: defaults for attrs, exits, $cmds, */
    dbref owner;		/* PLAYER: domain number + class + moreflags */
    /* THING, ROOM, EXIT: owning player number */

    dbref zone;			/* Whatever the object is zoned to. */

    FLAG flags;			/* ALL: Flags set on the object */
    FLAG flags2;		/* ALL: even more flags */
    FLAG flags3;		/* ALL: yet _more_ flags */

    POWER powers;		/* ALL: Powers on object */
    POWER powers2;		/* ALL: even more powers */

    STACK *stackhead;		/* Every object has a stack. */

    ATRLIST *ahead;		/* The head of the attribute list. */
    int at_count;		/* How many attributes do we have? */
};

typedef char *NAME;

extern OBJ *db;
extern NAME *names;

#define	Location(t)		db[t].location

#define	Zone(t)			db[t].zone

#define	Contents(t)		db[t].contents
#define	Exits(t)		db[t].exits
#define	Next(t)			db[t].next
#define	Link(t)			db[t].link
#define	Owner(t)		db[t].owner
#define	Parent(t)		db[t].parent
#define	Flags(t)		db[t].flags
#define	Flags2(t)		db[t].flags2
#define Flags3(t)		db[t].flags3
#define Powers(t)		db[t].powers
#define Powers2(t)		db[t].powers2
#define Stack(t)		db[t].stackhead
#define	Home(t)			Link(t)
#define	Dropto(t)		Location(t)

#define	i_Name(t)		if (mudconf.cache_names) purenames[t] = NULL;

#define	s_Location(t,n)		db[t].location = (n)

#define	s_Zone(t,n)		db[t].zone = (n)

#define	s_Contents(t,n)		db[t].contents = (n)
#define	s_Exits(t,n)		db[t].exits = (n)
#define	s_Next(t,n)		db[t].next = (n)
#define	s_Link(t,n)		db[t].link = (n)
#define	s_Owner(t,n)		db[t].owner = (n)
#define	s_Parent(t,n)		db[t].parent = (n)
#define	s_Flags(t,n)		db[t].flags = (n)
#define	s_Flags2(t,n)		db[t].flags2 = (n)
#define s_Flags3(t,n)		db[t].flags3 = (n)
#define s_Powers(t,n)		db[t].powers = (n)
#define s_Powers2(t,n)		db[t].powers2 = (n)
#define s_Stack(t,n)		db[t].stackhead = (n)
#define	s_Home(t,n)		s_Link(t,n)
#define	s_Dropto(t,n)		s_Location(t,n)

extern int Pennies(dbref);
extern void s_Pennies(dbref, int);

extern dbref getref(FILE *);
extern void putref(FILE *, dbref);
extern BOOLEXP *dup_bool(BOOLEXP *);
extern void free_boolexp(BOOLEXP *);
extern dbref parse_dbref(const char *);
extern int mkattr(char *);
extern void al_add(dbref, int);
extern void al_delete(dbref, int);
extern void al_destroy(dbref);
extern void al_store(void);
extern void db_grow(dbref);
extern void db_free(void);
extern void db_make_minimal(void);
extern dbref db_read(FILE *, int *, int *, int *);
extern dbref db_write(FILE *, int, int);
extern void destroy_thing(dbref);
extern void destroy_exit(dbref);
extern void load_restart_db(void);
extern void dump_database_internal(int);

#define	DOLIST(thing,list) \
	for ((thing)=(list); \
	     ((thing)!=NOTHING) && (Next(thing)!=(thing)); \
	     (thing)=Next(thing))
#define	SAFE_DOLIST(thing,next,list) \
	for ((thing)=(list),(next)=((thing)==NOTHING ? NOTHING: Next(thing)); \
	     (thing)!=NOTHING && (Next(thing)!=(thing)); \
	     (thing)=(next), (next)=Next(next))
#define	DO_WHOLE_DB(thing) \
	for ((thing)=0; (thing)<mudstate.db_top; (thing)++)

#define DO_WHOLE_DB_REV(thing) \
	for ((thing)=mudstate.db_top-1; (thing)>0; (thing)--)

#define HAG_WAS_HERE
#define	Dropper(thing)	(Connected(Owner(thing)) && Hearer(thing))

#define DUMP_NORMAL  0
#define DUMP_CRASHED 1
#define DUMP_RESTART 2
#define DUMP_KILLED  4

#endif				/* __DB_H */
