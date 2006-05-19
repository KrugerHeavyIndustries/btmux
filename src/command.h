
/* command.h - declarations used by the command processor */

/* $Id: command.h,v 1.4 2005/08/08 09:43:05 murrayma Exp $ */

#include "copyright.h"
#include "config.h"

#ifndef __COMMAND_H
#define __COMMAND_H

#include "db.h"


#define CMD_NO_ARG(name) \
    extern void name(dbref, dbref, int)
#define CMD_ONE_ARG(name) \
    extern void name(dbref, dbref, int, char *)
#define CMD_ONE_ARG_CMDARG(name) \
    extern void name(dbref, dbref, int, char *, char *[], int)
#define CMD_TWO_ARG(name) \
    extern void name(dbref, dbref, int, char *, char *)
#define CMD_TWO_ARG_CMDARG(name) \
    extern void name(dbref, dbref, int, char *, char *, char*[], int)
#define CMD_TWO_ARG_ARGV(name) \
    extern void name(dbref, dbref, int, char *, char *[], int)
#define CMD_TWO_ARG_ARGV_CMDARG(name) \
    extern void name(dbref, dbref, int, char *, char *[], int, char*[], int)

/* Command function handlers */

/* from chat.c */
CMD_NO_ARG(do_chatlist);

/* from comsys.c */

CMD_TWO_ARG(do_cemit);		/* channel emit */
CMD_TWO_ARG(do_chboot);		/* channel boot */
CMD_TWO_ARG(do_editchannel);	/* edit a channel */
CMD_ONE_ARG(do_checkchannel);	/* check a channel */
CMD_ONE_ARG(do_createchannel);	/* create a channel */
CMD_ONE_ARG(do_destroychannel);	/* destroy a channel */
CMD_TWO_ARG(do_edituser);	/* edit a channel user */
CMD_NO_ARG(do_chanlist);	/* gives a channel listing */
CMD_ONE_ARG(do_chanstatus);	/* gives channelstatus */
CMD_TWO_ARG(do_chopen);		/* opens a channel */
CMD_ONE_ARG(do_channelwho);	/* who's on a channel */
CMD_TWO_ARG(do_addcom);		/* adds a comalias */
CMD_ONE_ARG(do_allcom);		/* on, off, who, all aliases */
CMD_NO_ARG(do_comlist);		/* channel who by alias */
CMD_TWO_ARG(do_comtitle);	/* sets a title on a channel */
CMD_NO_ARG(do_clearcom);	/* clears all comaliases */
CMD_ONE_ARG(do_delcom);		/* deletes a comalias */
CMD_TWO_ARG(do_tapcom);		/* taps a channel */

/* from mail.c */

CMD_TWO_ARG(do_mail);		/* mail command */
CMD_TWO_ARG(do_malias);		/* mail alias command */
CMD_ONE_ARG(do_prepend);
CMD_ONE_ARG(do_postpend);

CMD_TWO_ARG(do_admin);		/* Change config parameters */
CMD_TWO_ARG(do_alias);		/* Change the alias of something */
CMD_TWO_ARG(do_attribute);	/* Manage user-named attributes */
CMD_ONE_ARG(do_boot);		/* Force-disconnect a player */
CMD_TWO_ARG(do_chown);		/* Change object or attribute owner */
CMD_TWO_ARG(do_chownall);	/* Give away all of someone's objs */
CMD_TWO_ARG(do_chzone);		/* Change an object's zone. */
CMD_TWO_ARG(do_clone);		/* Create a copy of an object */
CMD_NO_ARG(do_comment);		/* Ignore argument and do nothing */
CMD_TWO_ARG_ARGV(do_cpattr);	/* Copy attributes */
CMD_TWO_ARG(do_create);		/* Create a new object */
CMD_ONE_ARG(do_cut);		/* Truncate contents or exits list */
CMD_NO_ARG(do_dbck);		/* Consistency check */
CMD_TWO_ARG(do_decomp);		/* Reproduce commands to recrete obj */
CMD_ONE_ARG(do_destroy);	/* Destroy an object */
CMD_TWO_ARG_ARGV(do_dig);	/* Dig a new room */
CMD_ONE_ARG(do_doing);		/* Set doing string in WHO report */
CMD_TWO_ARG_CMDARG(do_dolist);	/* Iterate command on list members */
CMD_ONE_ARG(do_drop);		/* Drop an object */
CMD_NO_ARG(do_dump);		/* Dump the database */
CMD_TWO_ARG_ARGV(do_edit);	/* Edit one or more attributes */
CMD_ONE_ARG(do_enter);		/* Enter an object */
CMD_ONE_ARG(do_entrances);	/* List exits and links to loc */
CMD_ONE_ARG(do_examine);	/* Examine an object */
CMD_ONE_ARG(do_find);		/* Search for name in database */
CMD_TWO_ARG(do_fixdb);		/* Database repair functions */
CMD_TWO_ARG_CMDARG(do_force);	/* Force someone to do something */
CMD_ONE_ARG_CMDARG(do_force_prefixed);	/* #<num> <cmd> variant of FORCE */
CMD_TWO_ARG(do_function);	/* Make iser-def global function */
CMD_ONE_ARG(do_get);		/* Get an object */
CMD_TWO_ARG(do_give);		/* Give something away */
CMD_ONE_ARG(do_global);		/* Enable/disable global flags */
CMD_ONE_ARG(do_halt);		/* Remove commands from the queue */
CMD_ONE_ARG(do_help);		/* Print info from help files */
CMD_ONE_ARG(do_history);	/* View various history info */
CMD_NO_ARG(do_multis);		/* View multiplayers (possibly) */
CMD_NO_ARG(do_inventory);	/* Print what I am carrying */
CMD_TWO_ARG(do_prog);		/* Interactive input */
CMD_ONE_ARG(do_quitprog);	/* Quits @prog */
CMD_TWO_ARG(do_kill);		/* Kill something */
CMD_ONE_ARG(do_last);		/* Get recent login info */
CMD_NO_ARG(do_leave);		/* Leave the current object */
CMD_TWO_ARG(do_link);		/* Set home, dropto, or dest */
CMD_ONE_ARG(do_list);		/* List contents of internal tables */
CMD_ONE_ARG(do_list_file);	/* List contents of message files */
CMD_TWO_ARG(do_lock);		/* Set a lock on an object */
CMD_ONE_ARG(do_look);		/* Look here or at something */
CMD_ONE_ARG(do_motd);		/* Set/list MOTD messages */
CMD_ONE_ARG(do_move);		/* Move about using exits */
CMD_TWO_ARG_ARGV(do_mvattr);	/* Move attributes on object */
CMD_TWO_ARG(do_mudwho);		/* WHO for inter-mud page/who suppt */
CMD_TWO_ARG(do_name);		/* Change the name of something */
CMD_TWO_ARG(do_newpassword);	/* Change passwords */
CMD_TWO_ARG(do_notify);		/* Notify or drain semaphore */
CMD_TWO_ARG_ARGV(do_open);	/* Open an exit */
CMD_TWO_ARG(do_page);		/* Send message to faraway player */
CMD_TWO_ARG(do_parent);		/* Set parent field */
CMD_TWO_ARG(do_password);	/* Change my password */
CMD_TWO_ARG(do_pcreate);	/* Create new characters */
CMD_TWO_ARG(do_pemit);		/* Messages to specific player */
CMD_ONE_ARG(do_poor);		/* Reduce wealth of all players */
CMD_TWO_ARG(do_power);		/* Sets powers */
CMD_ONE_ARG(do_ps);		/* List contents of queue */
CMD_ONE_ARG(do_queue);		/* Force queue processing */
CMD_TWO_ARG(do_quota);		/* Set or display quotas */
CMD_NO_ARG(do_readcache);	/* Reread text file cache */
CMD_NO_ARG(do_restart);		/* Restart the game. */
CMD_ONE_ARG(do_say);		/* Messages to all */
CMD_NO_ARG(do_score);		/* Display my wealth */
CMD_ONE_ARG(do_search);		/* Search for objs matching criteria */
CMD_TWO_ARG(do_set);		/* Set flags or attributes */
CMD_TWO_ARG(do_setattr);	/* Set object attribute */
CMD_TWO_ARG(do_setvattr);	/* Set variable attribute */
CMD_ONE_ARG(do_shutdown);	/* Stop the game */
CMD_ONE_ARG(do_stats);		/* Display object type breakdown */
CMD_ONE_ARG(do_sweep);		/* Check for listeners */
CMD_TWO_ARG_ARGV_CMDARG(do_switch);	/* Execute cmd based on match */
CMD_TWO_ARG(do_teleport);	/* Teleport elsewhere */
CMD_ONE_ARG(do_think);		/* Think command */
CMD_ONE_ARG(do_timewarp);	/* Warp various timers */
CMD_TWO_ARG(do_toad);		/* Turn a tinyjerk into a tinytoad */
CMD_TWO_ARG_ARGV(do_trigger);	/* Trigger an attribute */
CMD_ONE_ARG(do_unlock);		/* Remove a lock from an object */
CMD_ONE_ARG(do_unlink);		/* Unlink exit or remove dropto */
CMD_ONE_ARG(do_use);		/* Use object */
CMD_NO_ARG(do_version);		/* List MUX version number */
CMD_TWO_ARG_ARGV(do_verb);	/* Execute a user-created verb */
CMD_TWO_ARG_CMDARG(do_wait);	/* Perform command after a wait */
CMD_ONE_ARG(do_wipe);		/* Mass-remove attrs from obj */
CMD_NO_ARG(do_dbclean);		/* Remove stale vattr entries */
CMD_TWO_ARG(do_addcommand);	/* Add or replace a global command */
CMD_TWO_ARG(do_delcommand);	/* Delete an added global command */
CMD_ONE_ARG(do_listcommands);	/* List added global commands */
#ifdef SQL_SUPPORT
CMD_TWO_ARG(do_query);		/* Trigger an externalized query */
#endif
/* from log.c */
#ifdef ARBITRARY_LOGFILES
CMD_TWO_ARG(do_log);		/* Log to arbitrary logfile in 'logs' */
#endif

/* Mecha stuff */
CMD_TWO_ARG(do_show);
CMD_ONE_ARG(do_charclear);
CMD_NO_ARG(do_show_stat);

#ifdef HUDINFO_SUPPORT
CMD_ONE_ARG(fake_hudinfo);
#endif

#ifdef USE_PYTHON

/* From python.c */
CMD_ONE_ARG(do_python);
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

extern int check_access(dbref, int);
extern void process_command(dbref, dbref, int, char *, char *[], int);

#endif
