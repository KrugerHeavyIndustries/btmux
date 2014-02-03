/* flags.h - object flags */

#include "copyright.h"

#ifndef __FLAGS_H
#define __FLAGS_H

#include "htab.h"
#include "db.h"

#define FLAG_WORD2   0x1  /* 2nd word of flags. */
#define FLAG_WORD3   0x2  /* 3rd word of flags. */

/* Object types */
#define TYPE_ROOM    0x0
#define TYPE_THING   0x1
#define TYPE_EXIT    0x2
#define TYPE_PLAYER  0x3

/* Empty */
#define TYPE_GARBAGE 0x5
#define NOTYPE       0x7
#define TYPE_MASK    0x7

/* First word of flags */
#define SEETHRU      0x00000008  /* Can see through to the other side */
#define WIZARD       0x00000010  /* gets automatic control */
#define LINK_OK      0x00000020  /* anybody can link to this room */
#define DARK         0x00000040  /* Don't show contents or presence */
#define JUMP_OK      0x00000080  /* Others may @tel here */
#define STICKY       0x00000100  /* Object goes home when dropped */
#define DESTROY_OK   0x00000200  /* Others may @destroy */
#define HAVEN        0x00000400  /* No killing here, or no pages */
#define QUIET        0x00000800  /* Prevent 'feelgood' messages */
#define HALT         0x00001000  /* object cannot perform actions */
#define TRACE        0x00002000  /* Generate evaluation trace output */
#define GOING        0x00004000  /* object is available for recycling */
#define MONITOR      0x00008000  /* Process ^x:action listens on obj? */
#define MYOPIC       0x00010000  /* See things as nonowner/nonwizard */
#define PUPPET       0x00020000  /* Relays ALL messages to owner */
#define CHOWN_OK     0x00040000  /* Object may be @chowned freely */
#define ENTER_OK     0x00080000  /* Object may be ENTERed */
#define VISUAL       0x00100000  /* Everyone can see properties */
#define IMMORTAL     0x00200000  /* Object can't be killed */
#define HAS_STARTUP  0x00400000  /* Load some attrs at startup */
#define OPAQUE       0x00800000  /* Can't see inside */
#define VERBOSE      0x01000000  /* Tells owner everything it does. */
#define INHERIT      0x02000000  /* Gets owner's privs. (i.e. Wiz) */
#define NOSPOOF      0x04000000  /* Report originator of all actions. */
#define ROBOT        0x08000000  /* Player is a ROBOT */
#define SAFE         0x10000000  /* Need /override to @destroy */
#define ROYALTY      0x20000000  /* Sees like a wiz, but ca't modify */
#define HEARTHRU     0x40000000  /* Can hear out of this obj or exit */
#define TERSE        0x80000000  /* Only show room name on look */

/* Second word of flags */
#define KEY          0x00000001  /* No puppets */
#define ABODE        0x00000002  /* May @set home here */
#define FLOATING     0x00000004  /* Inhibit Floating room.. msgs */
#define UNFINDABLE   0x00000008  /* Cant loc() from afar */
#define PARENT_OK    0x00000010  /* Others may @parent to me */
#define LIGHT        0x00000020  /* Visible in dark places */
#define HAS_LISTEN   0x00000040  /* Internal: LISTEN attr set */
#define HAS_FWDLIST  0x00000080  /* Internal: FORWARDLIST attr set */
#define AUDITORIUM   0x00000100  /* Should we check the SpeechLock? */
#define ANSI         0x00000200
#define REGISTERED_FLAG    0x00000400 /* Is player Registered? Useful to lockout non-registered players */
#define FIXED        0x00000800
#define UNINSPECTED  0x00001000
#define NO_COMMAND   0x00002000

#define NOBLEED      0x00008000
#define STAFF        0x00010000
#define HAS_DAILY    0x00020000
#define GAGGED       0x00040000
#define HARDCODE     0x00080000
#define IN_CHARACTER 0x00100000
#define ANSIMAP      0x00200000  /* Player uses ANSI maps */
#define HAS_HOURLY   0x00400000
#define MULTIOK      0x00800000

#define VACATION     0x01000000
#define PLAYER_MAILS 0x02000000
#define BLIND        0x04000000  /* Something to support blind players! */
#define ZOMBIE       0x08000000  /* Hardcode object is a zombie */

#define SUSPECT      0x10000000  /* Report some activities to wizards */
#define COMPRESS     0x20000000  /* Output is compressed */
#define CONNECTED    0x40000000  /* Player is connected */
#define SLAVE        0x80000000  /* Disallow most commands */

/* ---------------------------------------------------------------------------
* FLAGENT: Information about object flags.
*/
typedef struct flag_entry {
   const char *flagname;	/* Name of the flag */
   int flagvalue;		/* Which bit in the object is the flag */
   char flaglett;		/* Flag letter for listing */
   int flagflag;		/* Ctrl flags for this flag (recursive? :-) */
   int listperm;		/* Who sees this flag when set */
   int (*handler) ();		/* Handler for setting/clearing this flag */
} FLAGENT;

/* ---------------------------------------------------------------------------
* OBJENT: Fundamental object types
*/
typedef struct object_entry {
   const char *name;
   char lett;
   int perm;
   int flags;
} OBJENT;
extern OBJENT object_types[8];

#define OF_CONTENTS  0x0001   /* Object has contents: Contents() */
#define OF_LOCATION  0x0002   /* Object has a location: Location() */
#define OF_EXITS     0x0004   /* Object has exits: Exits() */
#define OF_HOME      0x0008   /* Object has a home: Home() */
#define OF_DROPTO    0x0010   /* Object has a dropto: Dropto() */
#define OF_OWNER     0x0020   /* Object can own other objects */
#define OF_SIBLINGS  0x0040   /* Object has siblings: Next() */

typedef struct flagset {
   FLAG word1;
   FLAG word2;
   FLAG word3;
} FLAGSET;

extern void init_flagtab(void);
extern void display_flagtab(dbref);
extern void flag_set(dbref, dbref, char *, int);
extern char *flag_description(dbref, dbref);
extern FLAGENT *find_flag(dbref, char *);
extern char *decode_flags(dbref, FLAG, long, long);
extern int has_flag(dbref, dbref, char *);
extern char *unparse_object(dbref, dbref, int);
extern char *unparse_object_numonly(dbref);
extern int convert_flags(dbref, char *, FLAGSET *, FLAG *);
extern void decompile_flags(dbref, dbref, char *);

#define unparse_flags(p,t) decode_flags(p,Flags(t),Flags2(t),Flags3(t))

#define GOD ((dbref) 1)

/* ---------------------- Object Permission/Attribute Macros */
/* IS(X,T,F)         - Is X of type T and have flag F set? */
/* Typeof(X)         - What object type is X */
/* God(X)            - Is X player #1 */
/* Robot(X)          - Is X a robot player */
/* Wizard(X)         - Does X have wizard privs */
/* Immortal(X)       - Is X unkillable */
/* Alive(X)          - Is X a player or a puppet */
/* Dark(X)           - Is X dark */
/* WHODark(X)        - Should X be hidden from the WHO report */
/* Floating(X)       - Prevent 'disconnected room' msgs for room X */
/* Quiet(X)          - Should 'Set.' messages et al from X be disabled */
/* Verbose(X)        - Should owner receive all commands executed? */
/* Trace(X)          - Should owner receive eval trace output? */
/* Player_haven(X)   - Is the owner of X no-page */
/* Haven(X)          - Is X no-kill(rooms) or no-page(players) */
/* Halted(X)         - Is X halted (not allowed to run commands)? */
/* Suspect(X)        - Is X someone the wizzes should keep an eye on */
/* Slave(X)          - Should X be prevented from db-changing commands */
/* Safe(X,P)         - Does P need the /OVERRIDE switch to @destroy X? */
/* Monitor(X)        - Should we check for ^xxx:xxx listens on player? */
/* Terse(X)          - Should we only show the room name on a look? */
/* Myopic(X)         - Should things as if we were nonowner/nonwiz */
/* Audible(X)        - Should X forward messages? */
/* Findroom(X)       - Can players in room X be found via @whereis? */
/* Unfindroom(X)     - Is @whereis blocked for players in room X? */
/* Findable(X)       - Can @whereis find X */
/* Unfindable(X)     - Is @whereis blocked for X */
/* No_robots(X)      - Does X disallow robot players from using */
/* Has_location(X)   - Is X something with a location (ie plyr or obj) */
/* Has_home(X)       - Is X something with a home (ie plyr or obj) */
/* Has_contents(X)   - Is X something with contents (ie plyr/obj/room) */
/* Good_obj(X)       - Is X inside the DB and have a valid type? */
/* Good_owner(X)     - Is X a good owner value? */
/* Going(X)          - Is X marked GOING? */
/* Inherits(X)       - Does X inherit the privs of its owner */
/* Examinable(P,X)   - Can P look at attribs of X */
/* MyopicExam(P,X)   - Can P look at attribs of X (obeys MYOPIC) */
/* Controls(P,X)     - Can P force X to do something */
/* Affects(P,X)      - (Controls in MUSH V1) Is P wiz or same owner as X */
/* Abode(X)          - Is X an ABODE room */
/* Link_exit(P,X)    - Can P link from exit X */
/* Linkable(P,X)     - Can P link to X */
/* Mark(x)           - Set marked flag on X */
/* Unmark(x)         - Clear marked flag on X */
/* Marked(x)         - Check marked flag on X */
/* Hardcode(x)       - Check hardcode flag on X */
/* In_Character(x)   - Whether or not mecha's IC */
/* See_attr(P,X.A,O,F)  - Can P see text attr A on X if attr has owner O */
/* Set_attr(P,X,A,F)    - Can P set/change text attr A (with flags F) on X */
/* Read_attr(P,X,A,O,F) - Can P see attr A on X if attr has owner O */
/* Write_attr(P,X,A,F)  - Can P set/change attr A (with flags F) on X */

#define IS(thing,type,flag) ((Typeof(thing)==(type)) && (Flags(thing) & (flag)))
#define Typeof(x)    (Flags(x) & TYPE_MASK)
#define God(x)       ((x) == GOD)
#define isRobot(x)   ((Flags(x) & ROBOT) != 0)
#define Robot(x)     (isPlayer(x) && isRobot(x))
#define Alive(x)     (isPlayer(x) || (Puppet(x) && Has_contents(x)))
#define OwnsOthers(x)   ((object_types[Typeof(x)].flags & OF_OWNER) != 0)
#define Has_location(x) ((object_types[Typeof(x)].flags & OF_LOCATION) != 0)
#define Has_contents(x) ((object_types[Typeof(x)].flags & OF_CONTENTS) != 0)
#define Has_exits(x)    ((object_types[Typeof(x)].flags & OF_EXITS) != 0)
#define Has_siblings(x) ((object_types[Typeof(x)].flags & OF_SIBLINGS) != 0)
#define Has_home(x)     ((object_types[Typeof(x)].flags & OF_HOME) != 0)
#define Has_dropto(x)   ((object_types[Typeof(x)].flags & OF_DROPTO) != 0)
#define Home_ok(x)   ((object_types[Typeof(x)].flags & OF_HOME) != 0)
#define isPlayer(x)  (Typeof(x) == TYPE_PLAYER)
#define isRoom(x)    (Typeof(x) == TYPE_ROOM)
#define isExit(x)    (Typeof(x) == TYPE_EXIT)
#define isThing(x)   (Typeof(x) == TYPE_THING)

#define Good_obj(x)     (((x) >= 0) && ((x) < mudstate.db_top) && \
                           (Typeof(x) < NOTYPE))
#define Good_owner(x)   (Good_obj(x) && OwnsOthers(x))

#define Royalty(x)      ((Flags(x) & ROYALTY) != 0)
#define WizRoy(x)       (Royalty(x) || Wizard(x))
#define Fixed(x)        ((Flags2(x) & FIXED) != 0)
#define Uninspected(x)  ((Flags2(x) & UNINSPECTED) != 0)
#define Ansi(x)         ((Flags2(x) & ANSI) != 0)
#define Ansimap(x)      ((Flags2(x) & ANSIMAP) != 0)
#define No_Command(x)   ((Flags2(x) & NO_COMMAND) != 0)
#define NoBleed(x)      ((Flags2(x) & NOBLEED) != 0)
#define Compress(x)     ((Flags2(x) & COMPRESS) != 0)

#define Transparent(x)  ((Flags(x) & SEETHRU) != 0)
#define Link_ok(x)      (((Flags(x) & LINK_OK) != 0) && Has_contents(x))
#define Wizard(x)       ((Flags(x) & WIZARD) || \
                           ((Flags(Owner(x)) & WIZARD) && Inherits(x)))
#define Dark(x)         (((Flags(x) & DARK) != 0) && (Wizard(x) || !Alive(x)))
#define Jump_ok(x)      (((Flags(x) & JUMP_OK) != 0) && Has_contents(x))
#define Sticky(x)       ((Flags(x) & STICKY) != 0)
#define Destroy_ok(x)   ((Flags(x) & DESTROY_OK) != 0)
#define Haven(x)        ((Flags(x) & HAVEN) != 0)
#define Player_haven(x) ((Flags(Owner(x)) & HAVEN) != 0)
#define Quiet(x)        ((Flags(x) & QUIET) != 0)
#define Halted(x)       ((Flags(x) & HALT) != 0)
#define Trace(x)        ((Flags(x) & TRACE) != 0)
#define Going(x)        ((Flags(x) & GOING) != 0)
#define Monitor(x)      ((Flags(x) & MONITOR) != 0)
#define Myopic(x)       ((Flags(x) & MYOPIC) != 0)
#define Puppet(x)       ((Flags(x) & PUPPET) != 0)
#define Chown_ok(x)     ((Flags(x) & CHOWN_OK) != 0)
#define Enter_ok(x)     (((Flags(x) & ENTER_OK) != 0) && \
                           Has_location(x) && Has_contents(x))
#define Visual(x)       ((Flags(x) & VISUAL) != 0)
#define Immortal(x)     ((Flags(x) & IMMORTAL) || \
                           ((Flags(Owner(x)) & IMMORTAL) && Inherits(x)))
#define Opaque(x)       ((Flags(x) & OPAQUE) != 0)
#define Verbose(x)      ((Flags(x) & VERBOSE) != 0)
#define Inherits(x)     (((Flags(x) & INHERIT) != 0) || \
                           ((Flags(Owner(x)) & INHERIT) != 0) || \
                           ((x) == Owner(x)))
#define Nospoof(x)      ((Flags(x) & NOSPOOF) != 0)
#define Safe(x,p)       (OwnsOthers(x) || \
                           (Flags(x) & SAFE) || \
                           (mudconf.safe_unowned && (Owner(x) != Owner(p))))
#define Audible(x)      ((Flags(x) & HEARTHRU) != 0)
#define Terse(x)        ((Flags(x) & TERSE) != 0)

#define Gagged(x)       ((Flags2(x) & GAGGED) != 0)
#define Vacation(x)     ((Flags2(x) & VACATION) != 0)
#define Key(x)          ((Flags2(x) & KEY) != 0)
#define Abode(x)        (((Flags2(x) & ABODE) != 0) && Home_ok(x))
#define Auditorium(x)   ((Flags2(x) & AUDITORIUM) != 0)
#define Floating(x)     ((Flags2(x) & FLOATING) != 0)
#define Findable(x)     ((Flags2(x) & UNFINDABLE) == 0)
#define Hideout(x)      ((Flags2(x) & UNFINDABLE) != 0)
#define Parent_ok(x)    ((Flags2(x) & PARENT_OK) != 0)
#define Light(x)        ((Flags2(x) & LIGHT) != 0)
#define Hardcode(x)     ((Flags2(x) & HARDCODE) != 0)
#define Zombie(x)       ((Flags2(x) & ZOMBIE) != 0)
#define MultiOK(x)      ((Flags2(x) & MULTIOK) != 0)
#define In_Character(x) ((Flags2(x) & IN_CHARACTER) != 0)
#define Suspect(x)      ((Flags2(Owner(x)) & SUSPECT) != 0)
#define Connected(x)    (((Flags2(x) & CONNECTED) != 0) && \
                           (Typeof(x) == TYPE_PLAYER))
#define Slave(x)        ((Flags2(Owner(x)) & SLAVE) != 0)
#define Hidden(x)       ((Flags(x) & DARK) || (Flags2(x) & UNFINDABLE))
#define Staff(x)        ((Flags2(x) & STAFF))
#define H_Startup(x)    ((Flags(x) & HAS_STARTUP) != 0)
#define H_Fwdlist(x)    ((Flags2(x) & HAS_FWDLIST) != 0)
#define H_Listen(x)     ((Flags2(x) & HAS_LISTEN) != 0)
#define Registered(x)	((Flags2(x) & REGISTERED_FLAG) !=0)

#define s_Opaque(x)	s_Flags((x), Flags(x) | OPAQUE)
#define s_Slave(x)      s_Flags2((x), Flags2(x) | SLAVE)
#define s_Fixed(x)      s_Flags2((x), Flags2(x) | FIXED)
#define s_Halted(x)     s_Flags((x), Flags(x) | HALT)
#define s_Going(x)      s_Flags((x), Flags(x) | GOING)
#define s_Connected(x)  s_Flags2((x), Flags2(x) | CONNECTED)
#define s_Hardcode(x)   s_Flags2((x), Flags2(x) | HARDCODE)
#define c_Hardcode(x)   s_Flags2((x), Flags2(x) & ~HARDCODE)
#define s_Zombie(x)     s_Flags2((x), Flags2(x) | ZOMBIE)
#define c_Zombie(x)     s_Flags2((x), Flags2(x) & ~ZOMBIE)
#define c_Connected(x)  s_Flags2((x), Flags2(x) & ~CONNECTED)
#define s_In_Character(x) s_Flags2((x), Flags2(x) | IN_CHARACTER)
#define c_In_Character(x) s_Flags2((x), Flags2(x) & ~IN_CHARACTER)

#define Parentable(p,x)    (Controls(p,x) || \
                              (Parent_ok(x) && could_doit(p,x,A_LPARENT)))

#define OnEnterLock(p,x)   (check_zone(p,x))

#define Examinable(p,x)    (((Flags(x) & VISUAL) != 0) || \
                              (See_All(p)) || \
                              (Owner(p) == Owner(x)) || \
                              OnEnterLock(p,x))

#define MyopicExam(p,x)    (((Flags(x) & VISUAL) != 0) || \
                              (!Myopic(p) && (See_All(p) || \
                              (Owner(p) == Owner(x)) || \
                              OnEnterLock(p,x))))

#define Controls(p,x)      (Good_obj(x) && \
                              (!(God(x) && !God(p))) && \
                              (Control_All(p) || \
                              ((Owner(p) == Owner(x)) && \
                              (Inherits(p) || !Inherits(x))) || \
                              OnEnterLock(p,x)))

#define Affects(p,x)       (Good_obj(x) && \
                              (!(God(x) && !God(p))) && \
                              (Wizard(p) || \
                              (Owner(p) == Owner(x))))
#define Mark(x)            (mudstate.markbits->chunk[(x)>>3] |= \
                              mudconf.markdata[(x)&7])
#define Unmark(x)          (mudstate.markbits->chunk[(x)>>3] &= \
                              ~mudconf.markdata[(x)&7])
#define Marked(x)          (mudstate.markbits->chunk[(x)>>3] & \
                              mudconf.markdata[(x)&7])
#define Mark_all(i)        for ((i)=0; (i)<((mudstate.db_top+7)>>3); (i)++) \
                              mudstate.markbits->chunk[i]=0xff
#define Unmark_all(i)      for ((i)=0; (i)<((mudstate.db_top+7)>>3); (i)++) \
                              mudstate.markbits->chunk[i]=0x0
#define Link_exit(p,x)     ((Typeof(x) == TYPE_EXIT) && \
                              ((Location(x) == NOTHING) || Controls(p,x)))
#define Linkable(p,x)      (Good_obj(x) && \
                              (Has_contents(x)) && \
                              (((Flags(x) & LINK_OK) != 0) || \
                              Controls(p,x)))
#define See_attr(p,x,a,o,f) \
                     (!((a)->flags & (AF_INTERNAL|AF_IS_LOCK)) && \
                     (God(p) || \
                     ((f) & AF_VISUAL) || \
                                 (((Owner(p) == (o)) || Examinable(p,x)) && \
                        !((a)->flags & (AF_DARK|AF_MDARK)) && \
                        !((f) & (AF_DARK|AF_MDARK)) && \
                        !((a)->name && strlen((a)->name) > 4 && \
                        !strcasecmp((a)->name + (strlen((a)->name)-5), \
                                       ".PRIV"))) || \
                     ((Wizard(p) || Royalty(p)) && !((a)->flags & AF_DARK)) || \
                     (!((a)->flags & (AF_DARK|AF_MDARK|AF_ODARK)) && \
                        !((a)->name && strlen((a)->name) > 4 && \
                           !strcasecmp((a)->name + (strlen((a)->name)-5), \
                                       ".PRIV")))))
#define See_attr_explicit(p,x,a,o,f) \
                     (!((a)->flags & (AF_INTERNAL|AF_IS_LOCK)) && \
                     (((f) & AF_VISUAL) || \
                     ((Owner(p) == (o)) && \
                        (!(((a)->flags & (AF_DARK|AF_MDARK))) && \
                        !((a)->name && strlen((a)->name) > 4 && \
                           !strcasecmp((a)->name + (strlen((a)->name)-5), \
                                       ".PRIV"))))))
#define  Set_attr(p,x,a,f) \
                     (!((a)->flags & (AF_INTERNAL|AF_IS_LOCK)) && \
                     (God(p) || \
                     (!God(x) && !(f & AF_LOCK) && \
                        ((Controls(p,x) && \
                        !((a)->flags & (AF_WIZARD|AF_GOD)) && \
                        !((f) & (AF_WIZARD|AF_GOD)) && \
                        !((a)->name && strlen((a)->name) > 4 && \
                           !strcasecmp((a)->name + (strlen((a)->name)-5), \
                                       ".PRIV"))) || \
                        (Wizard(p) && \
                        !((a)->flags & AF_GOD))))))
#define Read_attr(p,x,a,o,f) \
                     (!((a)->flags & AF_INTERNAL) && \
                     (God(p) || \
                     ((f) & AF_VISUAL) || \
                                 (((Owner(p) == o) || Examinable(p,x)) && \
                        !((a)->flags & (AF_DARK|AF_MDARK)) && \
                        !((f) & (AF_DARK|AF_MDARK)) && \
                        !((a)->name && strlen((a)->name) > 4 && \
                        !strcasecmp((a)->name + (strlen((a)->name)-5), \
                                       ".PRIV"))) || \
                     ((Wizard(p) || Royalty(p)) && !((a)->flags & AF_DARK)) || \
                     (!((a)->flags & (AF_DARK|AF_MDARK|AF_ODARK)) && \
                        !((a)->name && strlen((a)->name) > 4 && \
                        !strcasecmp((a)->name + (strlen((a)->name)-5), \
                                       ".PRIV")))))
#define Write_attr(p,x,a,f) \
                     (!((a)->flags & AF_INTERNAL) && \
                     (God(p) || \
                     (!God(x) && !(f & AF_LOCK) && \
                        ((Controls(p,x) && \
                        !((a)->flags & (AF_WIZARD|AF_GOD)) && \
                        !((f) & (AF_WIZARD|AF_GOD)) && \
                        !((a)->name && strlen((a)->name) > 4 && \
                           !strcasecmp((a)->name + (strlen((a)->name)-5), \
                                       ".PRIV"))) || \
                        (Wizard(p) && \
                        !((a)->flags & AF_GOD))))))

#define Has_power(p,x)  (check_access((p),powers_nametab[x].flag))
#define s_Dark(x)       s_Flags((x),Flags(x) | DARK)

#endif
