
/* command.h - declarations used by the command processor */

/* $Id: command.h,v 1.4 2005/08/08 09:43:05 murrayma Exp $ */

#include "copyright.h"
#include "config.h"

#ifndef __COMMAND_H
#define __COMMAND_H

#include "db.h"

/* from comsys.c */

void do_cemit(dbref, dbref, int, char *, char *);		/* channel emit */
void do_chboot(dbref, dbref, int, char *, char *);		/* channel boot */
void do_editchannel(dbref, dbref, int, char *, char *);	/* edit a channel */
void do_checkchannel(dbref, dbref, int, char *);	/* check a channel */
void do_createchannel(dbref, dbref, int, char *);	/* create a channel */
void do_destroychannel(dbref, dbref, int, char *);	/* destroy a channel */
void do_edituser(dbref, dbref, int, char *, char *);	/* edit a channel user */
void do_chanlist(dbref, dbref, int);	/* gives a channel listing */
void do_chanstatus(dbref, dbref, int, char *);	/* gives channelstatus */
void do_chopen(dbref, dbref, int, char *, char *);		/* opens a channel */
void do_channelwho(dbref, dbref, int, char *);	/* who's on a channel */
void do_addcom(dbref, dbref, int, char *, char *);		/* adds a comalias */
void do_allcom(dbref, dbref, int, char *);		/* on, off, who, all aliases */
void do_comlist(dbref, dbref, int);		/* channel who by alias */
void do_comtitle(dbref, dbref, int, char *, char *);	/* sets a title on a channel */
void do_clearcom(dbref, dbref, int);	/* clears all comaliases */
void do_delcom(dbref, dbref, int, char *);		/* deletes a comalias */
void do_tapcom(dbref, dbref, int, char *, char *);		/* taps a channel */

/* from mail.c */

void do_mail(dbref, dbref, int, char *, char *);		/* mail command */
void do_malias(dbref, dbref, int, char *, char *);		/* mail alias command */
void do_prepend(dbref, dbref, int, char *);
void do_postpend(dbref, dbref, int, char *);

void do_admin(dbref, dbref, int, char *, char *);		/* Change config parameters */
void do_alias(dbref, dbref, int, char *, char *);		/* Change the alias of something */
void do_attribute(dbref, dbref, int, char *, char *);	/* Manage user-named attributes */
void do_boot(dbref, dbref, int, char *);		/* Force-disconnect a player */
void do_chown(dbref, dbref, int, char *, char *);		/* Change object or attribute owner */
void do_chownall(dbref, dbref, int, char *, char *);	/* Give away all of someone's objs */
void do_chzone(dbref, dbref, int, char *, char *);		/* Change an object's zone. */
void do_clone(dbref, dbref, int, char *, char *);		/* Create a copy of an object */
void do_comment(dbref, dbref, int);		/* Ignore argument and do nothing */
void do_cpattr(dbref, dbref, int, char *, char *[], int);	/* Copy attributes */
void do_create(dbref, dbref, int, char *, char *);		/* Create a new object */
void do_cut(dbref, dbref, int, char *);		/* Truncate contents or exits list */
void do_dbck(dbref, dbref, int);		/* Consistency check */
void do_decomp(dbref, dbref, int, char *, char *);		/* Reproduce commands to recrete obj */
void do_destroy(dbref, dbref, int, char *);	/* Destroy an object */
void do_dig(dbref, dbref, int, char *, char *[], int);	/* Dig a new room */
void do_doing(dbref, dbref, int, char *);		/* Set doing string in WHO report */
void do_dolist(dbref, dbref, int, char *, char *, char *[], int);	/* Iterate command on list members */
void do_drop(dbref, dbref, int, char *);		/* Drop an object */
void do_dump(dbref, dbref, int);		/* Dump the database */
void do_edit(dbref, dbref, int, char *, char *[], int);	/* Edit one or more attributes */
void do_enter(dbref, dbref, int, char *);		/* Enter an object */
void do_entrances(dbref, dbref, int, char *);	/* List exits and links to loc */
void do_examine(dbref, dbref, int, char *);	/* Examine an object */
void do_find(dbref, dbref, int, char *);		/* Search for name in database */
void do_fixdb(dbref, dbref, int, char *, char *);		/* Database repair functions */
void do_force(dbref, dbref, int, char *, char *, char *[], int);	/* Force someone to do something */
void do_force_prefixed(dbref, dbref, int, char *, char *[], int);	/* #<num> <cmd> variant of FORCE */
void do_function(dbref, dbref, int, char *, char *);	/* Make iser-def global function */
void do_get(dbref, dbref, int, char *);		/* Get an object */
void do_give(dbref, dbref, int, char *, char *);		/* Give something away */
void do_global(dbref, dbref, int, char *);		/* Enable/disable global flags */
void do_halt(dbref, dbref, int, char *);		/* Remove commands from the queue */
void do_help(dbref, dbref, int, char *);		/* Print info from help files */
void do_history(dbref, dbref, int, char *);	/* View various history info */
void do_multis(dbref, dbref, int);
void do_inventory(dbref, dbref, int);	/* Print what I am carrying */
void do_prog(dbref, dbref, int, char *, char *);		/* Interactive input */
void do_quitprog(dbref, dbref, int, char *);	/* Quits @prog */
void do_kill(dbref, dbref, int, char *, char *);		/* Kill something */
void do_last(dbref, dbref, int, char *);		/* Get recent login info */
void do_leave(dbref, dbref, int);		/* Leave the current object */
void do_link(dbref, dbref, int, char *, char *);		/* Set home, dropto, or dest */
void do_list(dbref, dbref, int, char *);		/* List contents of internal tables */
void do_list_file(dbref, dbref, int, char *);	/* List contents of message files */
void do_lock(dbref, dbref, int, char *, char *);		/* Set a lock on an object */
void do_pagelock(dbref, dbref, int, char *, char *); 		/* Sets a Pagelock */
void do_pageunlock(dbref, dbref, int, char *, char *);		/* Removes a Pagelock */
void do_look(dbref, dbref, int, char *);		/* Look here or at something */
void do_motd(dbref, dbref, int, char *);		/* Set/list MOTD messages */
void do_move(dbref, dbref, int, char *);		/* Move about using exits */
void do_mvattr(dbref, dbref, int, char *, char *[], int);	/* Move attributes on object */
void do_mudwho(dbref, dbref, int, char *, char *);		/* WHO for inter-mud page/who suppt */
void do_name(dbref, dbref, int, char *, char *);		/* Change the name of something */
void do_newpassword(dbref, dbref, int, char *, char *);	/* Change passwords */
void do_notify(dbref, dbref, int, char *, char *);		/* Notify or drain semaphore */
void do_open(dbref, dbref, int, char *, char *[], int);	/* Open an exit */
void do_page(dbref, dbref, int, char *, char *);		/* Send message to faraway player */
void do_parent(dbref, dbref, int, char *, char *);		/* Set parent field */
void do_password(dbref, dbref, int, char *, char *);	/* Change my password */
void do_pcreate(dbref, dbref, int, char *, char *);	/* Create new characters */
void do_pemit(dbref, dbref, int, char *, char *);		/* Messages to specific player */
void do_poor(dbref, dbref, int, char *);		/* Reduce wealth of all players */
void do_power(dbref, dbref, int, char *, char *);		/* Sets powers */
void do_ps(dbref, dbref, int, char *);		/* List contents of queue */
void do_queue(dbref, dbref, int, char *);		/* Force queue processing */
void do_quota(dbref, dbref, int, char *, char *);		/* Set or display quotas */
void do_readcache(dbref, dbref, int);	/* Reread text file cache */
void do_restart(dbref, dbref, int);		/* Restart the game. */
void do_say(dbref, dbref, int, char *);		/* Messages to all */
void do_score(dbref, dbref, int);		/* Display my wealth */
void do_search(dbref, dbref, int, char *);		/* Search for objs matching criteria */
void do_set(dbref, dbref, int, char *, char *);		/* Set flags or attributes */
void do_setattr(dbref, dbref, int, char *, char *);	/* Set object attribute */
void do_setvattr(dbref, dbref, int, char *, char *);	/* Set variable attribute */
void do_shutdown(dbref, dbref, int, char *);	/* Stop the game */
void do_stats(dbref, dbref, int, char *);		/* Display object type breakdown */
void do_sweep(dbref, dbref, int, char *);		/* Check for listeners */
void do_switch(dbref, dbref, int, char *, char *[], int, char *[], int);	/* Execute cmd based on match */
void do_teleport(dbref, dbref, int, char *, char *);	/* Teleport elsewhere */
void do_think(dbref, dbref, int, char *);		/* Think command */
void do_timewarp(dbref, dbref, int, char *);	/* Warp various timers */
void do_toad(dbref, dbref, int, char *, char *);		/* Turn a tinyjerk into a tinytoad */
void do_trigger(dbref, dbref, int, char *, char *[], int);	/* Trigger an attribute */
void do_unlock(dbref, dbref, int, char *);		/* Remove a lock from an object */
void do_unlink(dbref, dbref, int, char *);		/* Unlink exit or remove dropto */
void do_use(dbref, dbref, int, char *);		/* Use object */
void do_version(dbref, dbref, int);		/* List MUX version number */
void do_verb(dbref, dbref, int, char *, char *[], int);	/* Execute a user-created verb */
void do_wait(dbref, dbref, int, char *, char *, char *[], int);	/* Perform command after a wait */
void do_wipe(dbref, dbref, int, char *);		/* Mass-remove attrs from obj */
void do_dbclean(dbref, dbref, int);		/* Remove stale vattr entries */
void do_addcommand(dbref, dbref, int, char *, char *);	/* Add or replace a global command */
void do_delcommand(dbref, dbref, int, char *, char *);	/* Delete an added global command */
void do_listcommands(dbref, dbref, int, char *);	/* List added global commands */
#ifdef SQL_SUPPORT
void do_query(dbref, dbref, int, char *, char *);		/* Trigger an externalized query */
#endif
/* from log.c */
#ifdef ARBITRARY_LOGFILES
void do_log(dbref, dbref, int, char *, char *);		/* Log to arbitrary logfile in 'logs' */
#endif

/* Mecha stuff */
void do_show(dbref, dbref, int, char *, char *);
void do_charclear(dbref, dbref, int, char *);
void do_show_stat(dbref, dbref, int);

#ifdef HUDINFO_SUPPORT
void fake_hudinfo(dbref, dbref, int, char *);
#endif

#ifdef USE_PYTHON

/* From python.c */
void do_python(dbref, dbref, int, char *);
#endif

typedef struct cmdentry CMDENT;
struct cmdentry {
    char *cmdname;
    NAMETAB *switches;
    int perms;
    int extra;
    int callseq;
    void (*handler) ();
};

typedef struct addedentry ADDENT;
struct addedentry {
    dbref thing;
    int atr;
    char *name;
    struct addedentry *next;
};

/* Command handler call conventions */

#define CS_NO_ARGS	0x0000	/* No arguments */
#define CS_ONE_ARG	0x0001	/* One argument */
#define CS_TWO_ARG	0x0002	/* Two arguments */
#define CS_NARG_MASK	0x0003	/* Argument count mask */
#define CS_ARGV		0x0004	/* ARG2 is in ARGV form */
#define CS_INTERP	0x0010	/* Interpret ARG2 if 2 args, ARG1 if 1 */
#define CS_NOINTERP	0x0020	/* Never interp ARG2 if 2 or ARG1 if 1 */
#define CS_CAUSE	0x0040	/* Pass cause to old command handler */
#define CS_UNPARSE	0x0080	/* Pass unparsed cmd to old-style handler */
#define CS_CMDARG	0x0100	/* Pass in given command args */
#define CS_STRIP	0x0200	/* Strip braces even when not interpreting */
#define	CS_STRIP_AROUND	0x0400	/* Strip braces around entire string only */
#define CS_ADDED	0X0800	/* Command has been added by @addcommand */
#define CS_NO_MACRO     0x1000	/* Command can't be used inside macro */
#define CS_LEADIN	0x2000	/* Command is a single-letter lead-in */

/* Command permission flags */

#define CA_PUBLIC	0x00000000	/* No access restrictions */
#define CA_GOD		0x00000001	/* GOD only... */
#define CA_WIZARD	0x00000002	/* Wizards only */
#define CA_BUILDER	0x00000004	/* Builders only */
#define CA_IMMORTAL	0x00000008	/* Immortals only */
#define CA_ROBOT	0x00000010	/* Robots only */
#define CA_ANNOUNCE     0x00000020	/* Announce Power */
#define CA_ADMIN	0x00000800	/* Wizard or royal */
#define CA_NO_HAVEN	0x00001000	/* Not by HAVEN players */
#define CA_NO_ROBOT	0x00002000	/* Not by ROBOT players */
#define CA_NO_SLAVE	0x00004000	/* Not by SLAVE players */
#define CA_NO_SUSPECT	0x00008000	/* Not by SUSPECT players */
#define CA_NO_GUEST	0x00010000	/* Not by GUEST players */
#define CA_NO_IC        0x00020000	/* Not by IC players */


#define CA_GBL_BUILD	0x01000000	/* Requires the global BUILDING flag */
#define CA_GBL_INTERP	0x02000000	/* Requires the global INTERP flag */
#define CA_DISABLED	0x04000000	/* Command completely disabled */
#define	CA_NO_DECOMP	0x08000000	/* Don't include in @decompile */
#define CA_LOCATION	0x10000000	/* Invoker must have location */
#define CA_CONTENTS	0x20000000	/* Invoker must have contents */
#define CA_PLAYER	0x40000000	/* Invoker must be a player */
#define CF_DARK		0x80000000	/* Command doesn't show up in list */

int check_access(dbref, int);
void process_command(dbref, dbref, int, char *, char *[], int);

#endif
