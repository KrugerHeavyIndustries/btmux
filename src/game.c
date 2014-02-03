/*
 * game.c 
 */

#include "copyright.h"
#include "config.h"

#include <sys/stat.h>
#include <signal.h>
#include <event.h>
#include <regex.h>

#include "mudconf.h"
#include "config.h"
#include "file_c.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "flags.h"
#include "powers.h"
#include "attrs.h"
#include "alloc.h"
#include "vattr.h"
#include "commac.h"
#ifdef SQL_SUPPORT
#include "sqlchild.h"
#endif

#ifndef NEXT
#endif

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_SYS_UCONTEXT_H
#include <sys/ucontext.h>
#endif

#define NSUBEXP 10

extern void init_attrtab(void);
extern void init_cmdtab(void);
extern void init_mactab(void);
extern void init_chantab(void);
extern void cf_init(void);
extern void pcache_init(void);
extern int cf_read(char *fn);
extern void init_functab(void);
extern void close_sockets(int emergency, char *message);
extern void init_version(void);
extern void init_logout_cmdtab(void);
extern void init_timer(void);
extern void raw_notify(dbref, const char *);
extern void do_second(void);
extern void do_dbck(dbref, dbref, int);

#ifdef ARBITRARY_LOGFILES
void logcache_init();
void logcache_destruct();
#endif

#ifdef HUDINFO_SUPPORT
extern void init_hudinfo(void);
#endif

void fork_and_dump(int);
void dump_database(void);
void do_dump_optimize(dbref, dbref, int);
void pcache_sync(void);
void dump_database_internal(int);
static void init_rlimit(void);

int reserved;

extern int corrupt;

/*
 * used to allocate storage for temporary stuff, cleared before command
 * execution
 */

void do_dump(dbref player, dbref cause, int key)
{
	notify(player, "Dumping...");

	/*
	 * DUMP_OPTIMIZE takes advantage of a feature of GDBM to compress  
	 * unused space in the database, and will not be very useful
	 * except sparingly, perhaps done every month or so. 
	 */

	if(key & DUMP_OPTIMIZE)
		do_dump_optimize(player, cause, key);
	else
		fork_and_dump(key);
}

void do_dump_optimize(dbref player, dbref cause, int key)
{
	raw_notify(player, "Database is memory based.");
}

/**
 * print out stuff into error file 
 */
void report(void)
{
	STARTLOG(LOG_BUGS, "BUG", "INFO") {
		log_text((char *) "Command: '");
		log_text(mudstate.debug_cmd);
		log_text((char *) "'");
		ENDLOG;
	} if(Good_obj(mudstate.curr_player)) {
		STARTLOG(LOG_BUGS, "BUG", "INFO") {
			log_text((char *) "Player: ");
			log_name_and_loc(mudstate.curr_player);
			if((mudstate.curr_enactor != mudstate.curr_player) &&
			   Good_obj(mudstate.curr_enactor)) {
				log_text((char *) " Enactor: ");
				log_name_and_loc(mudstate.curr_enactor);
			}
			ENDLOG;
	}}
}

/*
 * Load a regular expression match and insert it into
 * registers.
 */
int regexp_match(char *pattern, char *str, char *args[], int nargs)
{
	regex_t re;
	int got_match;
	regmatch_t pmatch[NSUBEXP];
	int i, len;

	/*
	 * Load the regexp pattern. This allocates memory which must be
	 * later freed. A free() of the regexp does free all structures
	 * under it.
	 */

	if(regcomp(&re, pattern, REG_EXTENDED) != 0) {
		/*
		 * This is a matching error. We have an error message in
		 * regexp_errbuf that we can ignore, since we're doing
		 * command-matching.
		 */
		return 0;
	}

	/* 
	 * Now we try to match the pattern. The relevant fields will
	 * automatically be filled in by this.
	 */
	got_match = (regexec(&re, str, NSUBEXP, pmatch, 0) == 0);
	if(!got_match) {
		regfree(&re);
		return 0;
	}

	/*
	 * Now we fill in our args vector. Note that in regexp matching,
	 * 0 is the entire string matched, and the parenthesized strings
	 * go from 1 to 9. We DO PRESERVE THIS PARADIGM, for consistency
	 * with other languages.
	 */

	for(i = 0; i < nargs; i++) {
		args[i] = NULL;
	}

	/* Convenient: nargs and NSUBEXP are the same.
	 * We are also guaranteed that our buffer is going to be LBUF_SIZE
	 * so we can copy without fear.
	 */

	for(i = 0; (i < NSUBEXP) && (pmatch[i].rm_so != -1) && (pmatch[i].rm_eo != -1); i++) {
		len = pmatch[i].rm_eo - pmatch[i].rm_so;
		args[i] = alloc_lbuf("regexp_match");
        memset(args[i], 0, LBUF_SIZE);
		strncpy(args[i], str + pmatch[i].rm_so, len);
		args[i][len] = '\0';	/* strncpy() does not null-terminate */
	}

	regfree(&re);
	return 1;
}

/**
 * Check attribute list for wild card matches and queue them.
 */
static int atr_match1(dbref thing, dbref parent, dbref player, char type,
					  char *str, int check_exclude, int hash_insert)
{
	dbref aowner;
	int match, attr, i; long aflags;
	char buff[LBUF_SIZE], *s, *as;
	char *args[10];
	ATTR *ap;

    memset(args, 0, sizeof(args));
    
	/*
	 * See if we can do it.  Silently fail if we can't. 
	 */

	if(!could_doit(player, parent, A_LUSE))
		return -1;

	match = 0;
	for(attr = atr_head(parent, &as); attr; attr = atr_next(&as)) {
		ap = atr_num(attr);

		/*
		 * Never check NOPROG attributes. 
		 */

		if(!ap || (ap->flags & AF_NOPROG))
			continue;

		/*
		 * If we aren't the bottom level check if we saw this attr *
		 * * * * before.  Also exclude it if the attribute type is *
		 * * PRIVATE. 
		 */

		if(check_exclude && ((ap->flags & AF_PRIVATE) ||
							 nhashfind(ap->number, &mudstate.parent_htab))) {
			continue;
		}
		atr_get_str(buff, parent, attr, &aowner, &aflags);

		/*
		 * Skip if private and on a parent 
		 */

		if(check_exclude && (aflags & AF_PRIVATE)) {
			continue;
		}
		/*
		 * If we aren't the top level remember this attr so we * * *
		 * exclude * it from now on. 
		 */

		if(hash_insert)
			nhashadd(ap->number, (int *) &attr, &mudstate.parent_htab);

		/*
		 * Check for the leadin character after excluding the attrib
		 * * * * * This lets non-command attribs on the child block * 
		 * *  * commands * on the parent. 
		 */

		if((buff[0] != type) || (aflags & AF_NOPROG))
			continue;

		/*
		 * decode it: search for first un escaped : 
		 */

		for(s = buff + 1; *s && (*s != ':'); s++);
		if(!*s)
			continue;
		*s++ = 0;
		if(((aflags & AF_REGEXP) && regexp_match(buff + 1, str, args, 10))
		   || wild(buff + 1, str, args, 10)) {
			match = 1;
			wait_que(thing, player, 0, NOTHING, 0, s, args, 10,
					 mudstate.global_regs);
			for(i = 0; i < 10; i++) {
				if(args[i])
					free_lbuf(args[i]);
			}
		}
	}
	return (match);
}

int atr_match(dbref thing, dbref player, char type, char *str,
			  int check_parents)
{
	int match, lev, result, exclude, insert;
	dbref parent;

	/*
	 * If thing is halted, don't check anything 
	 */

	if(Halted(thing))
		return 0;

	/*
	 * If not checking parents, just check the thing 
	 */

	match = 0;
	if(!check_parents)
		return atr_match1(thing, thing, player, type, str, 0, 0);

	/*
	 * Check parents, ignoring halted objects 
	 */

	exclude = 0;
	insert = 1;
	nhashflush(&mudstate.parent_htab, 0);
	ITER_PARENTS(thing, parent, lev) {
		if(!Good_obj(Parent(parent)))
			insert = 0;
		result =
			atr_match1(thing, parent, player, type, str, exclude, insert);
		if(result > 0) {
			match = 1;
		} else if(result < 0) {
			return match;
		}
		exclude = 1;
	}

	return match;
}

/**
 * Notifies the object #target of the message msg, and
 * optionally notify the contents, neighbors, and location also.
 */
int check_filter(dbref object, dbref player, int filter, const char *msg)
{
	long aflags;
	dbref aowner;
	char *buf, *nbuf, *cp, *dp, *str;

	buf = atr_pget(object, filter, &aowner, &aflags);
	if(!*buf) {
		free_lbuf(buf);
		return (1);
	}
	nbuf = dp = alloc_lbuf("check_filter");
	str = buf;
	exec(nbuf, &dp, 0, object, player, EV_FIGNORE | EV_EVAL | EV_TOP, &str,
		 (char **) NULL, 0);
	*dp = '\0';
	dp = nbuf;
	free_lbuf(buf);
	do {
		cp = parse_to(&dp, ',', EV_STRIP);
		if(quick_wild(cp, (char *) msg)) {
			free_lbuf(nbuf);
			return (0);
		}
	} while (dp != NULL);
	free_lbuf(nbuf);
	return (1);
}

static char *add_prefix(dbref object, dbref player, int prefix,
						const char *msg, const char *dflt)
{
	long aflags;
	dbref aowner;
	char *buf, *nbuf, *cp, *bp, *str;

	buf = atr_pget(object, prefix, &aowner, &aflags);
	if(!*buf) {
		cp = buf;
		safe_str((char *) dflt, buf, &cp);
	} else {
		nbuf = bp = alloc_lbuf("add_prefix");
		str = buf;
		exec(nbuf, &bp, 0, object, player, EV_FIGNORE | EV_EVAL | EV_TOP,
			 &str, (char **) NULL, 0);
		*bp = '\0';
		free_lbuf(buf);
		buf = nbuf;
		cp = &buf[strlen(buf)];
	}
	if(cp != buf)
		safe_str((char *) " ", buf, &cp);
	safe_str((char *) msg, buf, &cp);
	*cp = '\0';
	return (buf);
}

static char *dflt_from_msg(dbref sender, dbref sendloc)
{
	char *tp, *tbuff;

	tp = tbuff = alloc_lbuf("notify_checked.fwdlist");
	safe_str((char *) "From ", tbuff, &tp);
	if(Good_obj(sendloc))
		safe_str(Name(sendloc), tbuff, &tp);
	else
		safe_str(Name(sender), tbuff, &tp);
	safe_chr(',', tbuff, &tp);
	*tp = '\0';
	return tbuff;
}

char *colorize(dbref player, char *from);

void notify_checked(dbref target, dbref sender, const char *msg, int key)
{
	char *msg_ns, *mp, *tbuff, *tp, *buff, *colbuf = NULL;
	char *args[10];
	dbref aowner, targetloc, recip, obj;
	int i, nargs, has_neighbors, pass_listen; long aflags;
	int check_listens, pass_uselock, is_audible;
	FWDLIST *fp;

	/*
	 * If speaker is invalid or message is empty, just exit 
	 */

	if(!Good_obj(target) || !msg || !*msg)
		return;

	/*
	 * Enforce a recursion limit 
	 */

	mudstate.ntfy_nest_lev++;
	if(mudstate.ntfy_nest_lev >= mudconf.ntfy_nest_lim) {
		mudstate.ntfy_nest_lev--;
		return;
	}
	/*
	 * If we want NOSPOOF output, generate it.  It is only needed if 
	 * we are sending the message to the target object 
	 */

	if(key & MSG_ME) {
		mp = msg_ns = alloc_lbuf("notify_checked");
		if(Nospoof(target) && (target != sender) &&
		   (target != mudstate.curr_enactor) &&
		   (target != mudstate.curr_player && Good_obj(sender))) {

			/*
			 * I'd really like to use tprintf here but I can't 
			 * because the caller may have.
			 * notify(target, tprintf(...)) is quite common 
			 * in the code. 
			 */

			tbuff = alloc_sbuf("notify_checked.nospoof");
			safe_chr('[', msg_ns, &mp);
			safe_str(Name(sender), msg_ns, &mp);
			sprintf(tbuff, "(#%ld)", sender);
			safe_str(tbuff, msg_ns, &mp);

			if(sender != Owner(sender)) {
				safe_chr('{', msg_ns, &mp);
				safe_str(Name(Owner(sender)), msg_ns, &mp);
				safe_chr('}', msg_ns, &mp);
			}
			if(sender != mudstate.curr_enactor) {
				sprintf(tbuff, "<-(#%ld)", mudstate.curr_enactor);
				safe_str(tbuff, msg_ns, &mp);
			}
			safe_str((char *) "] ", msg_ns, &mp);
			free_sbuf(tbuff);
		}
		safe_str((char *) msg, msg_ns, &mp);
		*mp = '\0';
	} else {
		msg_ns = NULL;
	}

	/*
	 * msg contains the raw message, msg_ns contains the NOSPOOFed msg 
	 */

	check_listens = Halted(target) ? 0 : 1;
	switch (Typeof(target)) {
	case TYPE_PLAYER:
		if(key & MSG_ME) {
			if(key & MSG_COLORIZE)
				colbuf = colorize(target, msg_ns);
			raw_notify(target, colbuf ? colbuf : msg_ns);


		}

		if(colbuf)
			free_lbuf(colbuf);
		if(!mudconf.player_listen)
			check_listens = 0;
	case TYPE_THING:
	case TYPE_ROOM:

		/* If we're in a pipe, objects can receive raw_notify
		 * if they're not a player and connected (if we didn't
		 * do this, they'd be notified twice! */

		if(mudstate.inpipe && (!isPlayer(target) || (isPlayer(target) &&
													 !Connected(target)))) {
			raw_notify(target, msg_ns);
		}

		/*
		 * Forward puppet message if it is for me 
		 */

		has_neighbors = Has_location(target);
		targetloc = where_is(target);
		is_audible = Audible(target);

		if((key & MSG_ME) && Puppet(target) && (target != Owner(target))
		   && ((key & MSG_PUP_ALWAYS) ||
			   ((targetloc != Location(Owner(target))) &&
				(targetloc != Owner(target))))) {
			tp = tbuff = alloc_lbuf("notify_checked.puppet");
			safe_str(Name(target), tbuff, &tp);
			safe_str((char *) "> ", tbuff, &tp);
			if(key & MSG_COLORIZE)
				colbuf = colorize(Owner(target), msg_ns);
			safe_str(colbuf ? colbuf : msg_ns, tbuff, &tp);
			*tp = '\0';
			raw_notify(Owner(target), tbuff);
			if(colbuf)
				free_lbuf(colbuf);
			free_lbuf(tbuff);
		}
		/*
		 * Check for @Listen match if it will be useful 
		 */

		pass_listen = 0;
		nargs = 0;
		if(check_listens && (key & (MSG_ME | MSG_INV_L)) && H_Listen(target)) {
			tp = atr_get(target, A_LISTEN, &aowner, &aflags);
			if(*tp && wild(tp, (char *) msg, args, 10)) {
				for(nargs = 10;
					nargs && (!args[nargs - 1] || !(*args[nargs - 1]));
					nargs--);
				pass_listen = 1;
			}
			free_lbuf(tp);
		}
		/*
		 * If we matched the @listen or are monitoring, check the * * 
		 * USE lock 
		 */

		if(sender < 0)
			sender = GOD;
		pass_uselock = 0;
		if((key & MSG_ME) && check_listens && (pass_listen ||
											   Monitor(target)))
			pass_uselock = could_doit(sender, target, A_LUSE);

		/*
		 * Process AxHEAR if we pass LISTEN, USElock and it's for me 
		 */

		if((key & MSG_ME) && pass_listen && pass_uselock) {
			if(sender != target)
				did_it(sender, target, 0, NULL, 0, NULL, A_AHEAR, args,
					   nargs);
			else
				did_it(sender, target, 0, NULL, 0, NULL, A_AMHEAR, args,
					   nargs);
			did_it(sender, target, 0, NULL, 0, NULL, A_AAHEAR, args, nargs);
		}
		/*
		 * Get rid of match arguments. We don't need them anymore 
		 */

		if(pass_listen) {
			for(i = 0; i < 10; i++)
				if(args[i] != NULL)
					free_lbuf(args[i]);
		}
		/*
		 * Process ^-listens if for me, MONITOR, and we pass USElock 
		 */
		/*
		 * \todo Eventually come up with a cleaner method for making sure
		 * the sender isn't the same as the target.
		 */
		if((key & MSG_ME) && (sender != target || Staff(target))
		   && pass_uselock && Monitor(target)) {
			(void) atr_match(target, sender, AMATCH_LISTEN, (char *) msg, 0);
		}
		/*
		 * Deliver message to forwardlist members 
		 */

		if((key & MSG_FWDLIST) && Audible(target) &&
		   check_filter(target, sender, A_FILTER, msg)) {
			tbuff = dflt_from_msg(sender, target);
			buff = add_prefix(target, sender, A_PREFIX, msg, tbuff);
			free_lbuf(tbuff);

			fp = fwdlist_get(target);
			if(fp) {
				for(i = 0; i < fp->count; i++) {
					recip = fp->data[i];
					if(!Good_obj(recip) || (recip == target))
						continue;
					notify_checked(recip, sender, buff,
								   (MSG_ME | MSG_F_UP | MSG_F_CONTENTS |
									MSG_S_INSIDE));
				}
			}
			free_lbuf(buff);
		}
		/*
		 * Deliver message through audible exits 
		 */

		if(key & MSG_INV_EXITS) {
			DOLIST(obj, Exits(target)) {
				recip = Location(obj);
				if(Audible(obj) && ((recip != target) &&
									check_filter(obj, sender, A_FILTER,
												 msg))) {
					buff =
						add_prefix(obj, target, A_PREFIX, msg,
								   "From a distance,");
					notify_checked(recip, sender, buff,
								   MSG_ME | MSG_F_UP | MSG_F_CONTENTS |
								   MSG_S_INSIDE);
					free_lbuf(buff);
				}
			}
		}
		/*
		 * Deliver message through neighboring audible exits 
		 */

		if(has_neighbors && ((key & MSG_NBR_EXITS) ||
							 ((key & MSG_NBR_EXITS_A) && is_audible))) {

			/*
			 * If from inside, we have to add the prefix string * 
			 * 
			 * *  * * of * the container. 
			 */

			if(key & MSG_S_INSIDE) {
				tbuff = dflt_from_msg(sender, target);
				buff = add_prefix(target, sender, A_PREFIX, msg, tbuff);
				free_lbuf(tbuff);
			} else {
				buff = (char *) msg;
			}

			DOLIST(obj, Exits(Location(target))) {
				recip = Location(obj);
				if(Good_obj(recip) && Audible(obj) && (recip != targetloc)
				   && (recip != target) &&
				   check_filter(obj, sender, A_FILTER, msg)) {
					tbuff =
						add_prefix(obj, target, A_PREFIX, buff,
								   "From a distance,");
					notify_checked(recip, sender, tbuff,
								   MSG_ME | MSG_F_UP | MSG_F_CONTENTS |
								   MSG_S_INSIDE);
					free_lbuf(tbuff);
				}
			}
			if(key & MSG_S_INSIDE) {
				free_lbuf(buff);
			}
		}
		/*
		 * Deliver message to contents 
		 */

		if(((key & MSG_INV) || ((key & MSG_INV_L) && pass_listen)) &&
		   (check_filter(target, sender, A_INFILTER, msg))) {

			/*
			 * Don't prefix the message if we were given the * *
			 * * * MSG_NOPREFIX key. 
			 */

			if(key & MSG_S_OUTSIDE) {
				buff = add_prefix(target, sender, A_INPREFIX, msg, "");
			} else {
				buff = (char *) msg;
			}
			DOLIST(obj, Contents(target)) {
				if(Slave(obj) && (key & MSG_NO_SLAVE))
					continue;
				if(obj != target) {
					notify_checked(obj, sender, buff,
								   MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE );
				}
			}
			if(key & MSG_S_OUTSIDE)
				free_lbuf(buff);
		}
		/*
		 * Deliver message to neighbors 
		 */

		if(has_neighbors && ((key & MSG_NBR) || ((key & MSG_NBR_A) &&
												 is_audible &&
												 check_filter(target, sender,
															  A_FILTER,
															  msg)))) {
			if(key & MSG_S_INSIDE) {
				tbuff = dflt_from_msg(sender, target);
				buff = add_prefix(target, sender, A_PREFIX, msg, "");
				free_lbuf(tbuff);
			} else {
				buff = (char *) msg;
			}
			DOLIST(obj, Contents(targetloc)) {
				if((obj != target) && (obj != targetloc)) {
					notify_checked(obj, sender, buff,
								   MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | (key
																		  &
																		  MSG_COLORIZE));
				}
			}
			if(key & MSG_S_INSIDE) {
				free_lbuf(buff);
			}
		}
		/*
		 * Deliver message to container 
		 */

		if(has_neighbors && ((key & MSG_LOC) || ((key & MSG_LOC_A) &&
												 is_audible &&
												 check_filter(target, sender,
															  A_FILTER,
															  msg)))) {
			if(key & MSG_S_INSIDE) {
				tbuff = dflt_from_msg(sender, target);
				buff = add_prefix(target, sender, A_PREFIX, msg, tbuff);
				free_lbuf(tbuff);
			} else {
				buff = (char *) msg;
			}
			notify_checked(targetloc, sender, buff,
						   MSG_ME | MSG_F_UP | MSG_S_INSIDE);
			if(key & MSG_S_INSIDE) {
				free_lbuf(buff);
			}
		}
	}
	if(msg_ns)
		free_lbuf(msg_ns);
	mudstate.ntfy_nest_lev--;
}

void notify_except(dbref loc, dbref player, dbref exception, const char *msg)
{
	dbref first;

	if(loc != exception)
		notify_checked(loc, player, msg,
					   (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE |
						MSG_NBR_EXITS_A));
	DOLIST(first, Contents(loc)) {
		if(exception == NOSLAVE)
			if(Slave(first))
				continue;
		if(first != exception)
			notify_checked(first, player, msg,
						   (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE |
							(exception == NOSLAVE ? MSG_NO_SLAVE : 0)));
	}
}

void notify_except2(dbref loc, dbref player, dbref exc1, dbref exc2,
					const char *msg)
{
	dbref first;

	if((loc != exc1) && (loc != exc2))
		notify_checked(loc, player, msg,
					   (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE |
						MSG_NBR_EXITS_A));
	DOLIST(first, Contents(loc)) {
		if(first != exc1 && first != exc2) {
			notify_checked(first, player, msg,
						   (MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE));
		}
	}
}

void do_shutdown(dbref player, dbref cause, int key, char *message)
{
	FILE *fs;

	ResetSpecialObjects();
	if(player != NOTHING) {
		raw_broadcast(0, "Game: Shutdown by %s", Name(Owner(player)));
		STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN") {
			log_text((char *) "Shutdown by ");
			log_name(player);
			ENDLOG;
		}
	} else {
		raw_broadcast(0, "Game: Fatal Error: %s", message);
		STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN") {
			log_text((char *) "Fatal error: ");
			log_text(message);
			ENDLOG;
		}
	}
	STARTLOG(LOG_ALWAYS, "WIZ", "SHTDN") {
		log_text((char *) "Shutdown status: ");
		log_text(message);
		ENDLOG;
	}

	fs = fopen(mudconf.status_file, "w");
	fprintf(fs, "%s\n", message);
	fclose(fs);

	/*
	 * Do we perform a normal or an emergency shutdown?  Normal shutdown
	 * * * * * is handled by exiting the main loop in shovechars,
	 * emergency  * * * * shutdown is done here. 
	 */

	if(key & SHUTDN_PANIC) {

		/*
		 * Close down the network interface 
		 */

		emergency_shutdown();

		/*
		 * Close the attribute text db and dump the header db 
		 */

		pcache_sync();
		STARTLOG(LOG_ALWAYS, "DMP", "PANIC") {
			log_text((char *) "Panic dump: ");
			log_text(mudconf.crashdb);
			ENDLOG;
		} dump_database_internal(DUMP_CRASHED);

		STARTLOG(LOG_ALWAYS, "DMP", "DONE") {
			log_text((char *) "Panic dump complete: ");
			log_text(mudconf.crashdb);
			ENDLOG;
		}
	}
	/*
	 * Set up for normal shutdown 
	 */

	mudstate.shutdown_flag = 1;
	event_loopexit(NULL);
	return;
}

void dump_database_internal(int dump_type)
{
	char tmpfile[256], outfn[256], prevfile[256];
	FILE *f;

#ifdef USE_PYTHON
	runPythonHook("save");
#endif

	if(dump_type == DUMP_CRASHED) {
		unlink(mudconf.crashdb);
		f = fopen(mudconf.crashdb, "w");
		if(f != NULL) {
			db_write(f, F_MUX, UNLOAD_VERSION | UNLOAD_OUTFLAGS);
			fclose(f);
		} else {
			log_perror("DMP", "FAIL", "Opening crash file", mudconf.crashdb);
		}
		if(mudconf.have_mailer)
			if((f = fopen(mudconf.mail_db, "w"))) {
				dump_mail(f);
				fclose(f);
			}
		if(mudconf.have_comsys || mudconf.have_macros)
			save_comsys_and_macros(mudconf.commac_db);
		SaveSpecialObjects(DUMP_CRASHED);
		return;
	}

	if(dump_type == DUMP_RESTART) {
		f = fopen(mudconf.indb, "w");
		if(f != NULL) {
			/* Write a flatfile */
			db_write(f, F_MUX, UNLOAD_VERSION | UNLOAD_OUTFLAGS);
			fclose(f);
		} else {
			log_perror("DMP", "FAIL", "Opening restart file", mudconf.indb);
		}
		if(mudconf.have_mailer)
			if((f = fopen(mudconf.mail_db, "w"))) {
				dump_mail(f);
				fclose(f);
			}
		if(mudconf.have_comsys || mudconf.have_macros)
			save_comsys_and_macros(mudconf.commac_db);
		if(mudconf.have_specials)
			SaveSpecialObjects(DUMP_RESTART);
		return;
	}
	if(dump_type == DUMP_KILLED) {
		sprintf(tmpfile, "%s.KILLED", mudconf.indb);
		f = fopen(tmpfile, "w");
		if(f != NULL) {
			/* Write a flatfile */
			db_write(f, F_MUX, UNLOAD_VERSION | UNLOAD_OUTFLAGS);
			fclose(f);
		} else {
			log_perror("DMP", "FAIL", "Opening killed file", mudconf.indb);
		}
		if(mudconf.have_mailer)
			if((f = fopen(mudconf.mail_db, "w"))) {
				dump_mail(f);
				fclose(f);
			}
		if(mudconf.have_comsys || mudconf.have_macros)
			save_comsys_and_macros(mudconf.commac_db);
		if(mudconf.have_specials)
			SaveSpecialObjects(DUMP_KILLED);
		return;
	}

	sprintf(prevfile, "%s.prev", mudconf.outdb);
	sprintf(tmpfile, "%s.#%d#", mudconf.outdb, mudstate.epoch - 1);
	unlink(tmpfile);			/*
								 * nuke our predecessor 
								 */
	sprintf(tmpfile, "%s.#%d#", mudconf.outdb, mudstate.epoch);

	if(mudconf.compress_db) {
		sprintf(tmpfile, "%s.#%d#.gz", mudconf.outdb, mudstate.epoch - 1);
		unlink(tmpfile);
		sprintf(tmpfile, "%s.#%d#.gz", mudconf.outdb, mudstate.epoch);
		StringCopy(outfn, mudconf.outdb);
		strcat(outfn, ".gz");
		f = popen(tprintf("%s > %s", mudconf.compress, tmpfile), "w");
		if(f) {
			db_write(f, F_MUX, OUTPUT_VERSION | OUTPUT_FLAGS);
			pclose(f);
			rename(mudconf.outdb, prevfile);
			if(rename(tmpfile, outfn) < 0)
				log_perror("SAV", "FAIL",
						   "Renaming output file to DB file", tmpfile);
		} else {
			log_perror("SAV", "FAIL", "Opening", tmpfile);
		}
	} else {
		f = fopen(tmpfile, "w");
		if(f) {
			db_write(f, F_MUX, OUTPUT_VERSION | OUTPUT_FLAGS);
			fclose(f);
			rename(mudconf.outdb, prevfile);
			if(rename(tmpfile, mudconf.outdb) < 0)
				log_perror("SAV", "FAIL",
						   "Renaming output file to DB file", tmpfile);
		} else {
			log_perror("SAV", "FAIL", "Opening", tmpfile);
		}
		rename(prevfile, mudconf.indb);
	}

	if(mudconf.have_mailer)
		if((f = fopen(mudconf.mail_db, "w"))) {
			dump_mail(f);
			fclose(f);
		}
	if(mudconf.have_comsys || mudconf.have_macros)
		save_comsys_and_macros(mudconf.commac_db);
	if(mudconf.have_specials)
		SaveSpecialObjects(DUMP_NORMAL);
}

void dump_database(void)
{
	char *buff;

	mudstate.epoch++;
	mudstate.dumping = 1;
	buff = alloc_mbuf("dump_database");
	sprintf(buff, "%s.#%d#", mudconf.outdb, mudstate.epoch);
	STARTLOG(LOG_DBSAVES, "DMP", "DUMP") {
		log_text((char *) "Dumping: ");
		log_text(buff);
		ENDLOG;
	} pcache_sync();

	dump_database_internal(DUMP_NORMAL);
	STARTLOG(LOG_DBSAVES, "DMP", "DONE") {
		log_text((char *) "Dump complete: ");
		log_text(buff);
		ENDLOG;
	} free_mbuf(buff);

	mudstate.dumping = 0;
}

void fork_and_dump(int key)
{
	char *buff;

	if(*mudconf.dump_msg)
		raw_broadcast(0, "%s", mudconf.dump_msg);

	check_mail_expiration();
	mudstate.epoch++;
	mudstate.dumping = 1;
	buff = alloc_mbuf("fork_and_dump");
	sprintf(buff, "%s.#%d#", mudconf.outdb, mudstate.epoch);
   
    log_error(LOG_DBSAVES, "DMP", "CHKPT", "Saving database: %s", buff);
    
    pcache_sync();
    
	if(!key || (key & DUMP_STRUCT)) {
		if (mudconf.fork_dump) {
			/* Fork and dump.  */
			switch (fork()) {
			case -1: /* fork() failed */
				/* FIXME: Make this error message conform.  */
				log_perror("DMP", "FAIL", NULL, "fork()");
				mudstate.dumping = 0;
				return;

			case 0: /* child */
				dprintk("child database write process starting.");
				unbind_signals();
				dump_database_internal(DUMP_NORMAL);
				dprintk("child database write process finished.");
				/* You generally don't want to run atexit()
				 * handlers and that sort of thing.  */
				_exit(0);
				break;

			default: /* parent */
				break;
			}
		} else {
			/* Just dump.  */
			dump_database_internal(DUMP_NORMAL);
		}
	}

	mudstate.dumping = 0;

	if(*mudconf.postdump_msg)
		raw_broadcast(0, "%s", mudconf.postdump_msg);
}

static int load_game(void)
{
	FILE *f;
	int compressed;
	char infile[256];
	struct stat statbuf;
	int db_format, db_version, db_flags;

	f = NULL;
	compressed = 0;
	if(mudconf.compress_db) {
		StringCopy(infile, mudconf.indb);
		strcat(infile, ".gz");
		if(stat(infile, &statbuf) == 0) {
			if((f = popen(tprintf(" %s < %s", mudconf.uncompress,
								  infile), "r")) != NULL)
				compressed = 1;
		}
	}
	if(compressed == 0) {
		StringCopy(infile, mudconf.indb);
		if((f = fopen(mudconf.indb, "r")) == NULL)
			return -1;
	}
	/*
	 * ok, read it in 
	 */

	STARTLOG(LOG_STARTUP, "INI", "LOAD") {
		log_text((char *) "Loading: ");
		log_text(infile);
		ENDLOG;
	};
	if(db_read(f, &db_format, &db_version, &db_flags) < 0) {
		STARTLOG(LOG_ALWAYS, "INI", "FATAL") {
			log_text((char *) "Error loading ");
			log_text(infile);
			ENDLOG;
		}
		if(compressed)
			pclose(f);
		else
			fclose(f);
		return -1;
	}
	if(compressed)
		pclose(f);
	else
		fclose(f);

	if(mudconf.have_comsys || mudconf.have_macros)
		load_comsys_and_macros(mudconf.commac_db);

	/* Load the mecha stuff.. */
	if(mudconf.have_specials)
		LoadSpecialObjects();

	if(mudconf.have_mailer)
		if((f = fopen(mudconf.mail_db, "r"))) {
			load_mail(f);
			fclose(f);
		}
	STARTLOG(LOG_STARTUP, "INI", "LOAD") {
		log_text((char *) "Load complete.");
		ENDLOG;
	}
	/*
	 * everything ok 
	 */
	return (0);
}

/**
 * match a list of things, using the no_command flag 
 */
int list_check(dbref thing, dbref player, char type, char *str,
			   int check_parent)
{
	int match, limit;

	match = 0;
	limit = mudstate.db_top;
	while (thing != NOTHING) {
		if((thing != player) && (!(No_Command(thing)))) {
			if(atr_match(thing, player, type, str, check_parent) > 0)
				match = 1;
		}
		thing = Next(thing);
		if(--limit < 0)
			return match;
	}
	return match;
}

int Hearer(dbref thing)
{
	char *as, *buff, *s;
	dbref aowner;
	int attr; long aflags;
	ATTR *ap;

	if(mudstate.inpipe && (thing == mudstate.poutobj))
		return 1;

	if(Connected(thing) || Puppet(thing))
		return 1;

	if(Monitor(thing))
		buff = alloc_lbuf("Hearer");
	else
		buff = NULL;
	for(attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
		if(attr == A_LISTEN) {
			if(buff)
				free_lbuf(buff);
			return 1;
		}
		if(Monitor(thing)) {
			ap = atr_num(attr);
			if(!ap || (ap->flags & AF_NOPROG))
				continue;

			atr_get_str(buff, thing, attr, &aowner, &aflags);

			/*
			 * Make sure we can execute it 
			 */

			if((buff[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
				continue;

			/*
			 * Make sure there's a : in it 
			 */

			for(s = buff + 1; *s && (*s != ':'); s++);
			if(s) {
				free_lbuf(buff);
				return 1;
			}
		}
	}
	if(buff)
		free_lbuf(buff);
	return 0;
}

void do_readcache(dbref player, dbref cause, int key)
{
	helpindex_load(player);
	fcache_load(player);
}

static void process_preload(void)
{
	dbref thing, parent, aowner;
	long aflags; int lev, i;
	char *tstr;
	FWDLIST *fp;

	fp = (FWDLIST *) alloc_lbuf("process_preload.fwdlist");
	tstr = alloc_lbuf("process_preload.string");
	i = 0;
	DO_WHOLE_DB(thing) {

		/*
		 * Ignore GOING objects 
		 */

		if(Going(thing))
			continue;

		do_top(10);

		/*
		 * Look for a STARTUP attribute in parents 
		 */

		ITER_PARENTS(thing, parent, lev) {
			if(Flags(thing) & HAS_STARTUP) {
				did_it(Owner(thing), thing, 0, NULL, 0, NULL, A_STARTUP,
					   (char **) NULL, 0);
				/*
				 * Process queue entries as we add them 
				 */

				do_second();
				do_top(10);
				break;
			}
		}

		/*
		 * Look for a FORWARDLIST attribute 
		 */

		if(H_Fwdlist(thing)) {
			(void) atr_get_str(tstr, thing, A_FORWARDLIST, &aowner, &aflags);
			if(*tstr) {
				fwdlist_load(fp, GOD, tstr);
				if(fp->count > 0)
					fwdlist_set(thing, fp);
			}
		}
	}
	free_lbuf(fp);
	free_lbuf(tstr);
}

int real_main(int argc, char *argv[])
{
	int mindb;

	if((argc > 2) && (!strcmp(argv[1], "-s") && (argc > 3))) {
		fprintf(stderr, "Usage: %s [-s] [config-file]\n", argv[0]);
		exit(1);
	}

	event_init();

#if defined(HAVE_IEEEFP_H) && defined(HAVE_SYS_UCONTEXT_H)
	/*
	 * Inhibit IEEE fp exception on overflow 
	 */

	fpsetmask(fpgetmask() & ~FP_X_OFL);
#endif

	mindb = 0;					/* Are we creating a new db? */
	corrupt = 0;				/* Database isn't corrupted. */
    memset(&mudstate, 0, sizeof(mudstate));
	time(&mudstate.start_time);
	time(&mudstate.restart_time);
	mudstate.executable_path = strdup(argv[0]);
    mudstate.db_top = -1;
	tcache_init();
	pcache_init();
	cf_init();
	init_rlimit();
	init_cmdtab();
	init_mactab();
	init_chantab();
	init_logout_cmdtab();
	init_flagtab();
	init_powertab();
	init_functab();
	init_attrtab();
	init_version();

#ifdef HUDINFO_SUPPORT
	init_hudinfo();
#endif

	hashinit(&mudstate.player_htab, 250 * HASH_FACTOR);
	nhashinit(&mudstate.mail_htab, 50 * HASH_FACTOR);
	nhashinit(&mudstate.fwdlist_htab, 25 * HASH_FACTOR);
	nhashinit(&mudstate.parent_htab, 5 * HASH_FACTOR);
	mudstate.desctree = rb_init(desc_cmp, NULL);
	vattr_init();

	if(argc > 1 && !strcmp(argv[1], "-s")) {
		mindb = 1;
		if(argc == 3)
			cf_read(argv[2]);
		else
			cf_read((char *) CONF_FILE);
	} else if(argc == 2) {
		cf_read(argv[1]);
	} else {
		cf_read((char *) CONF_FILE);
	}

	fcache_init();
	helpindex_init();
	db_free();

	mudstate.record_players = 0;

	if(mindb)
		db_make_minimal();
	else if(load_game() < 0) {
		STARTLOG(LOG_ALWAYS, "INI", "LOAD") {
			log_text((char *) "Couldn't load: ");
			log_text(mudconf.indb);
			ENDLOG;
		} exit(2);
	}
#ifdef USE_PYTHON
	MUXPy_Init();
	runPythonHook("load");
#endif

	/* initialize random.. */
	srandom(getpid());
	/* set singnals.. */
	bind_signals();

	/*
	 * Do a consistency check and set up the freelist 
	 */

	do_dbck(NOTHING, NOTHING, 0);

	/*
	 * Reset all the hash stats 
	 */

	hashreset(&mudstate.command_htab);
	hashreset(&mudstate.macro_htab);
	hashreset(&mudstate.channel_htab);
	nhashreset(&mudstate.mail_htab);
	hashreset(&mudstate.logout_cmd_htab);
	hashreset(&mudstate.func_htab);
	hashreset(&mudstate.flags_htab);
	hashreset(&mudstate.attr_name_htab);
	hashreset(&mudstate.player_htab);
	nhashreset(&mudstate.fwdlist_htab);
	hashreset(&mudstate.news_htab);
	hashreset(&mudstate.help_htab);
	hashreset(&mudstate.wizhelp_htab);
	hashreset(&mudstate.plushelp_htab);
	hashreset(&mudstate.wiznews_htab);

	for(mindb = 0; mindb < MAX_GLOBAL_REGS; mindb++) {
		mudstate.global_regs[mindb] = alloc_lbuf("main.global_reg");
        memset(mudstate.global_regs[mindb], 0, LBUF_SIZE);
	}

	mudstate.now = time(NULL);
	process_preload();

	dnschild_init();

    if(!load_restart_db_xdr()) {
        load_restart_db();
    }

#ifdef SQL_SUPPORT
	sqlchild_init();
#endif

#ifdef ARBITRARY_LOGFILES
	logcache_init();
#endif

#ifdef MCHECK
	mtrace();
#endif

	/*
	 * go do it 
	 */

	mudstate.now = time(NULL);
	init_timer();
	shovechars(mudconf.port);

#ifdef MCHECK
	muntrace();
#endif

	close_sockets(0, (char *) "Going down - Bye");
	dump_database();

#ifdef ARBITRARY_LOGFILES
	logcache_destruct();
#endif
#ifdef SQL_SUPPORT
	sqlchild_destruct();
#endif

	exit(0);
}

static void init_rlimit(void)
{
#if defined(HAVE_SETRLIMIT) && defined(RLIMIT_NOFILE)
	struct rlimit *rlp;

	rlp = (struct rlimit *) alloc_lbuf("rlimit");

	if(getrlimit(RLIMIT_NOFILE, rlp)) {
		log_perror("RLM", "FAIL", NULL, "getrlimit()");
		free_lbuf(rlp);
		return;
	}
	rlp->rlim_cur = rlp->rlim_max;
	if(setrlimit(RLIMIT_NOFILE, rlp))
		log_perror("RLM", "FAIL", NULL, "setrlimit()");
	free_lbuf(rlp);

#endif /*
	    * HAVE_SETRLIMIT 
	    */
}
