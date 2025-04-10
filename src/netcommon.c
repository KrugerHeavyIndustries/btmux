/*
 * netcommon.c
 */

/*
 * This file contains routines used by the networking code that do not
 * depend on the implementation of the networking code.  The network-specific
 * portions of the descriptor data structure are not used.
 */

#include "copyright.h"
#include "config.h"

#include <time.h>

#include "db.h"
#include "mudconf.h"
#include "file_c.h"
#include "interface.h"
#include "command.h"
#include "externs.h"
#include "alloc.h"
#include "attrs.h"
#include "mguests.h"
#include "ansi.h"
#include "mail.h"
#include "powers.h"
#include "alloc.h"
#include "config.h"
#include "p.comsys.h"

#ifdef DEBUG_NETCOMMON
#ifndef DEBUG
#define DEBUG
#endif
#endif
#include "debug.h"

extern int process_output(DESC * d);
extern void handle_prog(DESC *, char *);
extern void fcache_dump_conn(DESC *, int);
extern void do_comconnect(dbref, DESC *);
extern void do_comdisconnect(dbref);
void set_lastsite(DESC *, char *);

/*
 * ---------------------------------------------------------------------------
 * * make_portlist: Make a list of ports for PORTS().
 */

void make_portlist(dbref player, dbref target, char *buff, char **bufc)
{
	DESC *d;
	int i = 0;

	DESC_ITER_CONN(d) {
		if(d->player == target) {
			safe_str(tprintf("%d ", d->descriptor), buff, bufc);
			i = 1;
		}
	}
	if(i) {
		(*bufc)--;
	}
	**bufc = '\0';
}

/*
 * ---------------------------------------------------------------------------
 * * timeval_sub: return difference between two times as a timeval
 */

struct timeval timeval_sub(struct timeval now, struct timeval then)
{
	now.tv_sec -= then.tv_sec;
	now.tv_usec -= then.tv_usec;
	if(now.tv_usec < 0) {
		now.tv_usec += 1000000;
		now.tv_sec--;
	}
	return now;
}

/*
 * ---------------------------------------------------------------------------
 * * msec_diff: return difference between two times in msec
 */

int msec_diff(struct timeval now, struct timeval then)
{
	return ((now.tv_sec - then.tv_sec) * 1000 + (now.tv_usec -
												 then.tv_usec) / 1000);
}

/*
 * ---------------------------------------------------------------------------
 * * msec_add: add milliseconds to a timeval
 */

struct timeval msec_add(struct timeval t, int x)
{
	t.tv_sec += x / 1000;
	t.tv_usec += (x % 1000) * 1000;
	if(t.tv_usec >= 1000000) {
		t.tv_sec += t.tv_usec / 1000000;
		t.tv_usec = t.tv_usec % 1000000;
	}
	return t;
}

/*
 * ---------------------------------------------------------------------------
 * * update_quotas: Update timeslice quotas
 */

struct timeval update_quotas(struct timeval last, struct timeval current)
{
	int nslices;
	DESC *d;

	nslices =
		msec_diff(current,
				  last) / (mudconf.timeslice > 0 ? mudconf.timeslice : 1);

	if(nslices > 0) {
		DESC_ITER_ALL(d) {
			d->quota += mudconf.cmd_quota_incr * nslices;
			if(d->quota > mudconf.cmd_quota_max)
				d->quota = mudconf.cmd_quota_max;
		}
	}
	return msec_add(last, nslices * mudconf.timeslice);
}

#ifdef TCP_CORK					// Linux 2.4, 2.6
/* choke_player: cork the player's sockets, must have a matching release_socket */
void choke_player(dbref player)
{
	DESC *d;
	int eins = 1, null = 0;

	DESC_ITER_PLAYER(player, d) {
		if(d->chokes == 0) {
			if(setsockopt
			   (d->descriptor, IPPROTO_TCP, TCP_CORK, &eins,
				sizeof(eins)) < 0) {
				// XXX: currently we ignore the error, because if its not supported,
				// then we'd spam the logs; other errors will be caught in the event loop.
			}
		}
		d->chokes++;
	}
}

void release_player(dbref player)
{
	DESC *d;
	int eins = 1, null = 0;

	DESC_ITER_PLAYER(player, d) {
		d->chokes--;
		if(d->chokes == 0) {
			if(setsockopt
			   (d->descriptor, IPPROTO_TCP, TCP_CORK, &null,
				sizeof(null)) < 0) {
				// XXX: current we ignore any error, ebcause if its not supported,
				// then we'd spam the logs; other errors will be caught in the event loop.
			}
		}
		if(d->chokes < 0)
			d->chokes = 0;
	}
}
#else
#ifdef TCP_NOPUSH				// *BSD, Mac OSX
/* choke_player: cork the player's sockets, must have a matching release_socket */
void choke_player(dbref player)
{
	DESC *d;
	int eins = 1, null = 0;

	DESC_ITER_PLAYER(player, d) {
		if(setsockopt
		   (d->descriptor, IPPROTO_TCP, TCP_NOPUSH, &eins,
			sizeof(eins)) < 0) {
			log_perror("NET", "FAIL", "choke_player", "setsockopt");
		}
	}
}

void release_player(dbref player)
{
	DESC *d;
	int eins = 1, null = 0;

	DESC_ITER_PLAYER(player, d) {
		if(setsockopt
		   (d->descriptor, IPPROTO_TCP, TCP_NOPUSH, &null,
			sizeof(null)) < 0) {
			log_perror("NET", "FAIL", "release_player", "setsockopt");
		}
	}
}
#else /* no OS support for network block coalescing. */
void choke_player(dbref player)
{
	// Do nothing!
}
void release_player(dbref player)
{
	// Do nothing!
}
#endif
#endif

/* raw_notify_raw: write a message to a player without the newline */

void raw_notify_raw(dbref player, const char *msg, char *append)
{
	DESC *d;

	if(!msg || !*msg)
		return;

	if(mudstate.inpipe && (player == mudstate.poutobj)) {
		safe_str((char *) msg, mudstate.poutnew, &mudstate.poutbufc);
		if(append != NULL)
			safe_str(append, mudstate.poutnew, &mudstate.poutbufc);
		return;
	}

	if(!Connected(player))
		return;

	DESC_ITER_PLAYER(player, d) {
		queue_string(d, msg);
		if(append != NULL)
			queue_write(d, append, strlen(append));
	}
}

/* raw_notify: write a message to a player */
void raw_notify(dbref player, const char *msg)
{
	raw_notify_raw(player, msg, "\r\n");
}

void notify_printf(dbref player, const char *format, ...)
{
	DESC *d;
	char buffer[LBUF_SIZE];
	va_list ap;
    memset(buffer, 0, LBUF_SIZE);

	va_start(ap, format);

	vsnprintf(buffer, LBUF_SIZE-1, format, ap);
	va_end(ap);

	strncat(buffer, "\r\n", LBUF_SIZE-1);
	buffer[LBUF_SIZE-1] = '\0';

	DESC_ITER_PLAYER(player, d) {
		queue_string(d, buffer);
	}
}

void raw_notify_newline(dbref player)
{
	DESC *d;

	if(mudstate.inpipe && (player == mudstate.poutobj)) {
		safe_str("\r\n", mudstate.poutnew, &mudstate.poutbufc);
		return;
	}
	if(!Connected(player))
		return;

	DESC_ITER_PLAYER(player, d) {
		queue_write(d, "\r\n", 2);
	}
}

#ifdef HUDINFO_SUPPORT
void hudinfo_notify(DESC * d, const char *msgclass, const char *msgtype,
					const char *msg)
{
	char buf[LBUF_SIZE];

    memset(buf, 0, LBUF_SIZE);

	if(!msgclass || !msgtype) {
		queue_write(d, msg, strnlen(msg, LBUF_SIZE-1));
		queue_write(d, "\r\n", 2);
		return;
	}

	snprintf(buf, LBUF_SIZE-1, "#HUD:%s:%s:%s# %s\r\n",
			 d->hudkey[0] ? d->hudkey : "???", msgclass, msgtype, msg);
	buf[LBUF_SIZE - 1] = '\0';
	queue_write(d, buf, strnlen(buf, LBUF_SIZE-1));
}
#endif

/*
 * ---------------------------------------------------------------------------
 * * raw_broadcast: Send message to players who have indicated flags
 */

void raw_broadcast(int inflags, char *template, ...)
{
	char buff[LBUF_SIZE];
	DESC *d;
	va_list ap;

	va_start(ap, template);
	if(!template || !*template)
		return;

	vsnprintf(buff, LBUF_SIZE, template, ap);
	buff[LBUF_SIZE-1] = '\0';

	DESC_ITER_CONN(d) {
		if((Flags(d->player) & inflags) == inflags) {
			queue_write(d, buff, strnlen(buff, LBUF_SIZE-1));
			queue_write(d, "\r\n", 2);
		}
	}
	flush_sockets();
	va_end(ap);
}

/*
 * ---------------------------------------------------------------------------
 * * clearstrings: clear out prefix and suffix strings
 */

void clearstrings(DESC * d)
{
	if(d->output_prefix) {
		free_lbuf(d->output_prefix);
		d->output_prefix = NULL;
	}
	if(d->output_suffix) {
		free_lbuf(d->output_suffix);
		d->output_suffix = NULL;
	}
}

/*
 * ---------------------------------------------------------------------------
 * * queue_write: Add text to the output queue for the indicated descriptor.
 */

void queue_write(DESC * d, const char *b, int n)
{
	int retval;
	if(n <= 0)
		return;

	bufferevent_write(d->sock_buff, b, n);
	d->output_tot += n;
	return;
}

void queue_string(DESC * d, const char *s)
{
	char new[LBUF_SIZE];

	strncpy(new, s, LBUF_SIZE-1);
	new[LBUF_SIZE-1] = '\0';

	if(!Ansi(d->player) && index(s, ESC_CHAR))
	        strip_ansi_r(new, s, strlen(s));
    queue_write(d, new, strlen(new));
}

void freeqs(DESC * d)
{
	CBLK *cb, *cnext;

    d->input_tail = 0;
    memset(d->input, 0, sizeof(d->input));
}
extern int fcache_conn_c;

void welcome_user(DESC * d)
{
	if(d->host_info & H_REGISTRATION)
		fcache_dump(d, FC_CONN_REG);
	else {
		if(fcache_conn_c) {
			fcache_dump_conn(d, rand() % fcache_conn_c);
			return;
		}
		fcache_dump(d, FC_CONN);
	}
}

void set_lastsite(DESC * d, char *lastsite)
{
    long i, j;
    char buf[LBUF_SIZE];

    if(d->player) {
        if(lastsite) {
            strncpy(buf, lastsite, LBUF_SIZE-1);
			buf[LBUF_SIZE-1] = '\0';
        } else {
            atr_get_str(buf, d->player, A_LASTSITE, &i, &j);
        }
        atr_add_raw(d->player, A_LASTSITE, buf);
    }
}

static void set_userstring(char **userstring, const char *command)
{
	while (*command && isascii(*command) && isspace(*command))
		command++;
	if(!*command) {
		if(*userstring != NULL) {
			free_lbuf(*userstring);
			*userstring = NULL;
		}
	} else {
		if(*userstring == NULL) {
			*userstring = alloc_lbuf("set_userstring");
		}
	    snprintf(*userstring, LBUF_SIZE-1, "%s\r\n", command);
    }
}

static void parse_connect(const char *msg, char *command, char *user,
  char *pass) {
	char *p;

	if(strlen(msg) > (MBUF_SIZE-1)) {
		*command = '\0';
		*user = '\0';
		*pass = '\0';
		return;
	}
	while (*msg && isascii(*msg) && isspace(*msg))
		msg++;
	p = command;
	while (*msg && isascii(*msg) && !isspace(*msg))
		*p++ = *msg++;
	*p = '\0';
	while (*msg && isascii(*msg) && isspace(*msg))
		msg++;
	p = user;
	if(mudconf.name_spaces && (*msg == '\"')) {
		for(; *msg && (*msg == '\"' || isspace(*msg)); msg++);
		while (*msg && *msg != '\"') {
			while (*msg && !isspace(*msg) && (*msg != '\"'))
				*p++ = *msg++;
			if(*msg == '\"')
				break;
			while (*msg && isspace(*msg))
				msg++;
			if(*msg && (*msg != '\"'))
				*p++ = ' ';
		}
		for(; *msg && *msg == '\"'; msg++);
	} else
		while (*msg && isascii(*msg) && !isspace(*msg))
			*p++ = *msg++;
	*p = '\0';
	while (*msg && isascii(*msg) && isspace(*msg))
		msg++;
	p = pass;
	while (*msg && isascii(*msg) && !isspace(*msg))
		*p++ = *msg++;
	*p = '\0';
}

static const char *time_format_1(time_t dt)
{
	register struct tm *delta;
	static char buf[64];

	if(dt < 0)
		dt = 0;

	delta = gmtime(&dt);
	if(delta->tm_yday > 0) {
		snprintf(buf, sizeof(buf), "%dd %02d:%02d", delta->tm_yday, delta->tm_hour,
				delta->tm_min);
	} else {
		snprintf(buf, sizeof(buf), "%02d:%02d", delta->tm_hour, delta->tm_min);
	}
	return buf;
}

static const char *time_format_2(time_t dt)
{
	register struct tm *delta;
	static char buf[64];

	if(dt < 0)
		dt = 0;

	delta = gmtime(&dt);
	if(delta->tm_yday > 0) {
		snprintf(buf, sizeof(buf), "%dd", delta->tm_yday);
	} else if(delta->tm_hour > 0) {
		snprintf(buf, sizeof(buf), "%dh", delta->tm_hour);
	} else if(delta->tm_min > 0) {
		snprintf(buf, sizeof(buf), "%dm", delta->tm_min);
	} else {
		snprintf(buf, sizeof(buf), "%ds", delta->tm_sec);
	}
	return buf;
}

extern char *mux_version;
void desc_addhash(DESC *);

static void announce_connect(dbref player, DESC * d)
{
	dbref loc, aowner, temp;
	dbref zone, obj;

	long aflags; int num, key, count;
	char *buf, *time_str;
	DESC *dtemp;

    queue_string(d, "Connected.\n");
    queue_string(d, mux_version);
    queue_string(d, "\n\n");

	desc_addhash(d);

	choke_player(player);

	count = 0;
	DESC_ITER_CONN(dtemp)
		count++;

	if(mudstate.record_players < count)
		mudstate.record_players = count;

	buf = atr_pget(player, A_TIMEOUT, &aowner, &aflags);
	if(buf) {
		d->timeout = atoi(buf);
		if(d->timeout <= 0)
			d->timeout = mudconf.idle_timeout;
	}
	free_lbuf(buf);

	loc = Location(player);
	s_Connected(player);


	queue_string(d, tprintf("\n%sMOTD:%s %s\n", ANSI_HILITE,
							   ANSI_NORMAL, mudconf.motd_msg));
	if(Wizard(player)) {
		raw_notify(player, tprintf("%sWIZMOTD:%s %s\n", ANSI_HILITE,
								   ANSI_NORMAL, mudconf.wizmotd_msg));
		if(!(mudconf.control_flags & CF_LOGIN)) {
			raw_notify(player, "*** Logins are disabled.");
		}
		if(mudconf.registeredonly) {
			raw_notify(player, "*** Only players with the Registered FLAG are allowed to login.");
		}
	}
	buf = atr_get(player, A_LPAGE, &aowner, &aflags);
	if(buf && *buf) {
		raw_notify(player,
				   "Your PAGE LOCK is set. You may be unable to receive some pages.");
	}
	num = 0;
	DESC_ITER_PLAYER(player, dtemp) num++;

	/*
	 * Reset vacation flag
	 */
	s_Flags2(player, Flags2(player) & ~VACATION);

	if(num < 2) {
		snprintf(buf, LBUF_SIZE, "%s has connected.", Name(player));

		if(mudconf.have_comsys)
			do_comconnect(player, d);

		if(Dark(player)) {
			raw_broadcast(MONITOR, (char *) "GAME: %s has DARK-connected.",
						  Name(player), 0, 0, 0, 0, 0);
		} else {
			raw_broadcast(MONITOR, (char *) "GAME: %s has connected.",
						  Name(player), 0, 0, 0, 0, 0);
		}
	} else {
		snprintf(buf, LBUF_SIZE, "%s has reconnected.", Name(player));
		raw_broadcast(MONITOR, (char *) "GAME: %s has reconnected.",
					  Name(player), 0, 0, 0, 0, 0);
	}

	key = MSG_INV;
	if((loc != NOTHING) && !(Dark(player) && Wizard(player)))
		key |= (MSG_NBR | MSG_NBR_EXITS | MSG_LOC | MSG_FWDLIST);

	temp = mudstate.curr_enactor;
	mudstate.curr_enactor = player;
	notify_checked(player, player, buf, key);
	free_lbuf(buf);
	if(Suspect(player)) {
		send_channel("Suspect", "%s has connected.", Name(player));

	}
	if(d->host_info & H_SUSPECT)
		send_channel("Suspect", "[Suspect site: %s] %s has connected.",
					 d->addr, Name(player));
	buf = atr_pget(player, A_ACONNECT, &aowner, &aflags);
	if(buf)
		wait_que(player, player, 0, NOTHING, 0, buf, (char **) NULL, 0, NULL);
	free_lbuf(buf);
	if(mudconf.master_room != NOTHING) {
		buf = atr_pget(mudconf.master_room, A_ACONNECT, &aowner, &aflags);
		if(buf)
			wait_que(mudconf.master_room, player, 0, NOTHING, 0, buf,
					 (char **) NULL, 0, NULL);
		free_lbuf(buf);
		DOLIST(obj, Contents(mudconf.master_room)) {
			buf = atr_pget(obj, A_ACONNECT, &aowner, &aflags);
			if(buf) {
				wait_que(obj, player, 0, NOTHING, 0, buf, (char **) NULL,
						 0, NULL);
			}
			free_lbuf(buf);
		}
	}
	/*
	 * do the zone of the player's location's possible aconnect
	 */
	if(mudconf.have_zones && ((zone = Zone(loc)) != NOTHING)) {
		switch (Typeof(zone)) {
		case TYPE_THING:
			buf = atr_pget(zone, A_ACONNECT, &aowner, &aflags);
			if(buf) {
				wait_que(zone, player, 0, NOTHING, 0, buf, (char **) NULL,
						 0, NULL);
			}
			free_lbuf(buf);
			break;
		case TYPE_ROOM:
			/*
			 * check every object in the room for a connect * * *
			 * action
			 */
			DOLIST(obj, Contents(zone)) {
				buf = atr_pget(obj, A_ACONNECT, &aowner, &aflags);
				if(buf) {
					wait_que(obj, player, 0, NOTHING, 0, buf,
							 (char **) NULL, 0, NULL);
				}
				free_lbuf(buf);
			}
			break;
		default:
			log_text(tprintf
					 ("Invalid zone #%d for %s(#%d) has bad type %d", zone,
					  Name(player), player, Typeof(zone)));
		}
	}
	time_str = ctime(&mudstate.now);
	time_str[strlen(time_str) - 1] = '\0';
	record_login(player, 1, time_str, d->addr, d->username);
	look_in(player, Location(player),
			(LK_SHOWEXIT | LK_OBEYTERSE));
	mudstate.curr_enactor = temp;
	release_player(player);
}

void announce_disconnect(dbref player, DESC * d, const char *reason)
{
	dbref loc, aowner, temp, zone, obj;
	int num, key; long aflags;
	char *buf, *atr_temp;
	DESC *dtemp;
	char *argv[1];

	if(Suspect(player)) {
		send_channel("Suspect", "%s has disconnected.", Name(player));
	}
	if(d->host_info & H_SUSPECT) {
		send_channel("Suspect", "[Suspect site: %s] %s has disconnected.",
					 d->addr, Name(d->player));
	}
	loc = Location(player);
	num = 0;
	DESC_ITER_PLAYER(player, dtemp) num++;

	temp = mudstate.curr_enactor;
	mudstate.curr_enactor = player;

	if(num < 2) {
		buf = alloc_mbuf("announce_disconnect.only");

		snprintf(buf, MBUF_SIZE, "%s has disconnected.", Name(player));
		key = MSG_INV;
		if((loc != NOTHING) && !(Dark(player) && Wizard(player)))
			key |= (MSG_NBR | MSG_NBR_EXITS | MSG_LOC | MSG_FWDLIST);
		notify_checked(player, player, buf, key);
		free_mbuf(buf);

		if(mudconf.have_comsys)
			do_comdisconnect(player);

		if(mudconf.have_mailer)
			do_mail_purge(player);

		raw_broadcast(MONITOR, (char *) "GAME: %s has disconnected.",
					  Name(player), 0, 0, 0, 0, 0);

		if(Guest(player) && mudconf.have_comsys)
			toast_player(player);

		argv[0] = (char *) reason;
		c_Connected(player);

		atr_temp = atr_pget(player, A_ADISCONNECT, &aowner, &aflags);
		if(atr_temp && *atr_temp)
			wait_que(player, player, 0, NOTHING, 0, atr_temp, argv, 1, NULL);
		free_lbuf(atr_temp);
		if(mudconf.master_room != NOTHING) {
			atr_temp =
				atr_pget(mudconf.master_room, A_ADISCONNECT, &aowner,
						 &aflags);
			if(atr_temp)
				wait_que(mudconf.master_room, player, 0, NOTHING, 0,
						 atr_temp, (char **) NULL, 0, NULL);
			free_lbuf(atr_temp);
			DOLIST(obj, Contents(mudconf.master_room)) {
				atr_temp = atr_pget(obj, A_ADISCONNECT, &aowner, &aflags);
				if(atr_temp) {
					wait_que(obj, player, 0, NOTHING, 0, atr_temp,
							 (char **) NULL, 0, NULL);
				}
				free_lbuf(atr_temp);
			}
		}
		/*
		 * do the zone of the player's location's possible * * *
		 * adisconnect
		 */
		if(mudconf.have_zones && ((zone = Zone(loc)) != NOTHING)) {
			switch (Typeof(zone)) {
			case TYPE_THING:
				atr_temp = atr_pget(zone, A_ADISCONNECT, &aowner, &aflags);
				if(atr_temp) {
					wait_que(zone, player, 0, NOTHING, 0, atr_temp,
							 (char **) NULL, 0, NULL);
				}
				free_lbuf(atr_temp);
				break;
			case TYPE_ROOM:
				/*
				 * check every object in the room for a * * *
				 * connect action
				 */
				DOLIST(obj, Contents(zone)) {
					atr_temp = atr_pget(obj, A_ADISCONNECT, &aowner, &aflags);
					if(atr_temp) {
						wait_que(obj, player, 0, NOTHING, 0, atr_temp,
								 (char **) NULL, 0, NULL);
					}
					free_lbuf(atr_temp);
				}
				break;
			default:
				log_text(tprintf
						 ("Invalid zone #%d for %s(#%d) has bad type %d",
						  zone, Name(player), player, Typeof(zone)));
			}
		}
		if(d->flags & DS_AUTODARK) {
			s_Flags(d->player, Flags(d->player) & ~DARK);
			d->flags &= ~DS_AUTODARK;
		}

		if(Guest(player))
			s_Flags(player, Flags(player) | DARK);
	} else {
		buf = alloc_mbuf("announce_disconnect.partial");
		snprintf(buf, MBUF_SIZE, "%s has partially disconnected.", Name(player));
		key = MSG_INV;
		if((loc != NOTHING) && !(Dark(player) && Wizard(player)))
			key |= (MSG_NBR | MSG_NBR_EXITS | MSG_LOC | MSG_FWDLIST);
		notify_checked(player, player, buf, key);
		raw_broadcast(MONITOR,
					  (char *) "GAME: %s has partially disconnected.",
					  Name(player), 0, 0, 0, 0, 0);
		free_mbuf(buf);
	}

	mudstate.curr_enactor = temp;
	release_player(player);
}

int boot_off(dbref player, char *message)
{
	DESC *d, *dnext;
	int count;

	count = 0;
	DESC_SAFEITER_PLAYER(player, d, dnext) {
		if(message && *message) {
			queue_string(d, message);
			queue_string(d, "\r\n");
		}
		shutdownsock(d, R_BOOT);
		count++;
	}
	return count;
}

int boot_by_port(int port, int no_god, char *message)
{
	DESC *d, *dnext;
	int count;

	count = 0;
	DESC_SAFEITER_ALL(d, dnext) {
		if((d->descriptor == port) && (!no_god || !God(d->player))) {
			if(message && *message) {
				queue_string(d, message);
				queue_string(d, "\r\n");
			}
			shutdownsock(d, R_BOOT);
			count++;
		}
	}
	return count;
}

/*
 * ---------------------------------------------------------------------------
 * * desc_reload: Reload parts of net descriptor that are based on db info.
 */

void desc_reload(dbref player)
{
	DESC *d;
	char *buf;
	dbref aowner;
	FLAG aflags;

	DESC_ITER_PLAYER(player, d) {
		buf = atr_pget(player, A_TIMEOUT, &aowner, &aflags);
		if(buf) {
			d->timeout = atoi(buf);
			if(d->timeout <= 0)
				d->timeout = mudconf.idle_timeout;
		}
		free_lbuf(buf);
	}
}

/*
 * ---------------------------------------------------------------------------
 * * fetch_idle, fetch_connect: Return smallest idle time/largest connec time
 * * for a player (or -1 if not logged in)
 */

int fetch_idle(dbref target)
{
	DESC *d;
	int result, idletime;

	result = -1;
	DESC_ITER_PLAYER(target, d) {
		idletime = (mudstate.now - d->last_time);
		if((result == -1) || (idletime < result))
			result = idletime;
	}
	return result;
}

int fetch_connect(dbref target)
{
	DESC *d;
	int result, conntime;

	result = -1;
	DESC_ITER_PLAYER(target, d) {
		conntime = (mudstate.now - d->connected_at);
		if(conntime > result)
			result = conntime;
	}
	return result;
}

static char *trimmed_name(dbref player)
{
	static char cbuff[18];

	if(strlen(Name(player)) <= 16)
		return Name(player);
	StringCopyTrunc(cbuff, Name(player), 16);
	cbuff[16] = '\0';
	return cbuff;
}

static char *trimmed_site(char *name)
{
	static char buff[MBUF_SIZE];

	if((strlen(name) <= mudconf.site_chars) || (mudconf.site_chars == 0))
		return name;
	StringCopyTrunc(buff, name, mudconf.site_chars);
	buff[mudconf.site_chars + 1] = '\0';
	return buff;
}

static void dump_users(DESC * e, char *match, int key)
{
	DESC *d;
	int count, rcount, ucount;
	char *buf, *fp, *sp, flist[4], slist[4];
	dbref room_it;

	while (match && *match && isspace(*match))
		match++;
	if(!match || !*match)
		match = NULL;



	buf = alloc_mbuf("dump_users");
	if(key == CMD_SESSION) {
		queue_string(e, "                               ");
		queue_string(e,
					 "     Characters Input----  Characters Output---\r\n");
	}
	queue_string(e, "Player Name         On For  Idle ");
	if(key == CMD_SESSION) {
		queue_string(e,
					 "Port Pend  Lost     Total  Pend  Lost     Total\r\n");
	} else if((e->flags & DS_CONNECTED) && (Wizard_Who(e->player)) &&
			  (key == CMD_WHO)) {
		queue_string(e, "     Room    Cmds Host\r\n");
	} else {
		if(Wizard_Who(e->player))
			queue_string(e, "  ");
		else
			queue_string(e, " ");
		queue_string(e, mudstate.doing_hdr);
		queue_string(e, "\r\n");
	}
//	queue_string(e,"------------------------------------------------------------------------------\r\n");
	count = 0;
	rcount = 0;
	ucount = 0;
	DESC_ITER_CONN(d) {
		if((!mudconf.show_unfindable_who || !Hidden(d->player)) ||
		   (e->flags & DS_CONNECTED) & Wizard_Who(e->player)) {
			count++;
			if(match && !(string_prefix(Name(d->player), match)))
				continue;
#if 0
			if((!((Wizard_Who(e->player)) && (e->flags & DS_CONNECTED)) &&
				(d->player != e->player)))
				if(In_Character(Location(d->player)) &&
				   In_Character(Location(Location(d->player))))
					continue;
#endif
			rcount++;
			if((key == CMD_SESSION) && !(Wizard_Who(e->player) && (e->flags & DS_CONNECTED)) && (d->player != e->player))
				continue;

			/*
			 * Get choice flags for wizards
			 */

			fp = flist;
			sp = slist;
			if((e->flags & DS_CONNECTED) && Wizard_Who(e->player)) {
				if(Hidden(d->player)) {
					if(d->flags & DS_AUTODARK)
						*fp++ = 'd';
					else if(Dark(d->player))
						*fp++ = 'D';
				}
				if(!Findable(d->player)) {
					*fp++ = 'U';
				} else {
					room_it = where_room(d->player);
					if(Good_obj(room_it)) {
						if(Hideout(room_it))
							*fp++ = 'u';
					} else {
						*fp++ = 'u';
					}
				}

				if(Suspect(d->player))
					*fp++ = '+';
				if(d->host_info & H_FORBIDDEN)
					*sp++ = 'F';
				if(d->host_info & H_REGISTRATION)
					*sp++ = 'R';
				if(d->host_info & H_SUSPECT)
					*sp++ = '+';
			}
			*fp = '\0';
			*sp = '\0';

			if((e->flags & DS_CONNECTED) && Wizard_Who(e->player) &&
			   (key == CMD_WHO)) {
				snprintf(buf, MBUF_SIZE, "%-16s%10s %5s%-3s#%6ld %7d %-25s\r\n",
						trimmed_name(d->player),
						time_format_1(mudstate.now - d->connected_at),
						time_format_2(mudstate.now - d->last_time), flist,
						Location(d->player), d->command_count,
						trimmed_site(((d->username[0] !=
									   '\0') ? tprintf("%s@%s", d->username,
													   d->addr) : d->addr)));
			} else if(key == CMD_SESSION) {
				snprintf(buf, MBUF_SIZE, "%-16s%10s %5s%5d%5d%6d%10d%6d%6d%10d\r\n",
						trimmed_name(d->player),
						time_format_1(mudstate.now - d->connected_at),
						time_format_2((mudstate.now - d->last_time) >
									  HIDDEN_IDLESECS ? (mudstate.now -
														 d->last_time) : 0),
						d->descriptor, d->input_size, d->input_lost,
						d->input_tot, d->output_size, d->output_lost,
						d->output_tot);
			} else if(Wizard_Who(e->player)) {
				snprintf(buf, MBUF_SIZE, "%-16s%10s %5s%-3s%s%s\r\n",
						trimmed_name(d->player),
						time_format_1(mudstate.now - d->connected_at),
						time_format_2((mudstate.now - d->last_time) >
									  HIDDEN_IDLESECS ? (mudstate.now -
														 d->last_time) : 0),
						flist, d->doing, ANSI_NORMAL);
			} else {
				snprintf(buf, MBUF_SIZE, "%-16s%10s %5s  %s%s\r\n",
						trimmed_name(d->player),
						time_format_1(mudstate.now - d->connected_at),
						time_format_2((mudstate.now - d->last_time) >
									  HIDDEN_IDLESECS ? (mudstate.now -
														 d->last_time) : 0),
						d->doing, ANSI_NORMAL);
			}
			queue_string(e, buf);
		} else {
		ucount++;
		}
	}
	count = rcount;				/* previous mode was .. disgusting. */
	/*
	 * sometimes I like the ternary operator....
	 */
	if (ucount)
		snprintf(buf, MBUF_SIZE, "%d Visible Player%slogged in, (%d %s hidden), %d record, %s maximum.\r\n", count,
			(count == 1) ? " " : "s ", ucount, (ucount == 1) ? "is" : "are", mudstate.record_players,
			(mudconf.max_players == -1) ? "no" : tprintf("%d",
														 mudconf.
														 max_players));
	else
		snprintf(buf, MBUF_SIZE, "%d Player%slogged in, %d record, %s maximum.\r\n", count,
			(count == 1) ? " " : "s ", mudstate.record_players,
			(mudconf.max_players == -1) ? "no" : tprintf("%d", mudconf.max_players));

	queue_string(e, buf);


	free_mbuf(buf);
}

/*
 * ---------------------------------------------------------------------------
 * * do_doing: Set the doing string that appears in the WHO report.
 * * Idea from R'nice@TinyTIM.
 */

void do_doing(dbref player, dbref cause, int key, char *arg)
{
	DESC *d;
	char *c, *e;
	int foundany, over;

	if(key == DOING_MESSAGE) {
		foundany = 0;
		over = 0;
		DESC_ITER_PLAYER(player, d) {
			c = d->doing;

			over =
				safe_copy_str(arg, d->doing, &c,
							  DOINGLEN - 2 - strlen(ANSI_NORMAL));
			/* See if there's <esc>[<numbers> as the last remaining stuff */
			if(over) {
				e = c;
				c--;
				if(isdigit(*c)) {
					while (isdigit(*c) && c > e)
						c--;
					if(*c == '[') {
						c--;
						if(c > e && *c == '\033') {
							*c = 0;
							e = c;
						}
					}
				}
				StringCopy(e, ANSI_NORMAL);
			} else
				StringCopy(c, ANSI_NORMAL);
			foundany = 1;
		}
		if(foundany) {
			if(over) {
				notify_printf(player, "Warning: %d characters lost.", over);
			}
			if(!Quiet(player))
				notify(player, "Set.");
		} else {
			notify(player, "Not connected.");
		}
	} else if(key == DOING_HEADER) {
		if(!(Can_Poll(player))) {
			notify(player, "Permission denied.");
			return;
		}
		if(!arg || !*arg) {
			StringCopy(mudstate.doing_hdr, "Doing");
			over = 0;
		} else {
			c = mudstate.doing_hdr;
			over = safe_copy_str(arg, mudstate.doing_hdr, &c, DOINGLEN - 1);
			*c = '\0';
		}
		if(over) {
			notify_printf(player, "Warning: %d characters lost.", over);
		}
		if(!Quiet(player))
			notify(player, "Set.");
	} else {
		notify_printf(player, "Poll: %s", mudstate.doing_hdr);
	}
}

NAMETAB logout_cmdtable[] = {
	{(char *) "DOING", 5, CA_PUBLIC, CMD_DOING},
	{(char *) "LOGOUT", 6, CA_PUBLIC, CMD_LOGOUT},
	{(char *) "OUTPUTPREFIX", 12, CA_PUBLIC, CMD_PREFIX | CMD_NOxFIX},
	{(char *) "OUTPUTSUFFIX", 12, CA_PUBLIC, CMD_SUFFIX | CMD_NOxFIX},
	{(char *) "QUIT", 4, CA_PUBLIC, CMD_QUIT},
	{(char *) "SESSION", 7, CA_PUBLIC, CMD_SESSION},
	{(char *) "WHO", 3, CA_PUBLIC, CMD_WHO},
	{NULL, 0, 0, 0}
};

void init_logout_cmdtab(void)
{
	NAMETAB *cp;

	/*
	 * Make the htab bigger than the number of entries so that we find
	 * things on the first check.  Remember that the admin can add
	 * aliases.
	 */

	hashinit(&mudstate.logout_cmd_htab, 3 * HASH_FACTOR);
	for(cp = logout_cmdtable; cp->flag; cp++)
		hashadd(cp->name, (int *) cp, &mudstate.logout_cmd_htab);
}

static void failconn(const char *logcode, const char *logtype,
					 const char *logreason, DESC * d, int disconnect_reason,
					 dbref player, int filecache, char *motd_msg,
					 char *command, char *user, char *password, char *cmdsave)
{
	char *buff;

	STARTLOG(LOG_LOGIN | LOG_SECURITY, logcode, "RJCT") {
		buff = alloc_mbuf("failconn.LOG");
		snprintf(buff, MBUF_SIZE, "[%d/%s] %s rejected to ", d->descriptor, d->addr,
				logtype);
		log_text(buff);
		free_mbuf(buff);
		if(player != NOTHING)
			log_name(player);
		else
			log_text(user);
		log_text((char *) " (");
		log_text((char *) logreason);
		log_text((char *) ")");
		ENDLOG;
	} fcache_dump(d, filecache);

	if(*motd_msg) {
		queue_string(d, motd_msg);
		queue_write(d, "\r\n", 2);
	}
	free_lbuf(command);
	free_lbuf(user);
	free_lbuf(password);
	shutdownsock(d, disconnect_reason);
	mudstate.debug_cmd = cmdsave;
	return;
}

static const char *connect_fail =
	"Either that player does not exist, or has a different password.\r\n";
static const char *create_fail =
	"Either there is already a player with that name, or that name is illegal.\r\n";

static int check_connect(DESC * d, char *msg)
{
	char *command, *user, *password, *buff, *cmdsave;
	dbref player, aowner;
	long aflags; int nplayers;
	DESC *d2;
	char *p;

	cmdsave = mudstate.debug_cmd;
	mudstate.debug_cmd = (char *) "< check_connect >";

	/*
	 * Hide the password length from SESSION
	 */

	d->input_tot -= (strlen(msg) + 1);

	/*
	 * Crack the command apart
	 */

	command = alloc_lbuf("check_conn.cmd");
	user = alloc_lbuf("check_conn.user");
	password = alloc_lbuf("check_conn.pass");
	parse_connect(msg, command, user, password);

	if(!strncmp(command, "co", 2) || !strncmp(command, "cd", 2)) {
		if((string_prefix(user, mudconf.guest_prefix)) &&
		   (mudconf.guest_char != NOTHING) &&
		   (mudconf.control_flags & CF_LOGIN)) {
			if((p = make_guest(d)) == NULL) {
				queue_string(d,
							 "All guests are tied up, please try again later.\n");
				free_lbuf(command);
				free_lbuf(user);
				free_lbuf(password);
				return 0;
			}
			StringCopy(user, p);
			StringCopy(password, mudconf.guest_prefix);
		}
		/*
		 * See if this connection would exceed the max #players
		 */

		if(mudconf.max_players < 0) {
			nplayers = mudconf.max_players - 1;
		} else {
			nplayers = 0;
			DESC_ITER_CONN(d2)
				nplayers++;
		}

		player = connect_player(user, password, d->addr, d->username);
		if(player == NOTHING) {

			/*
			 * Not a player, or wrong password
			 */

			queue_string(d, connect_fail);
			STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "BAD") {
				buff = alloc_lbuf("check_conn.LOG.bad");
				user[3800] = '\0';
				snprintf(buff, LBUF_SIZE, "[%d/%s] Failed connect to '%s'",
						d->descriptor, d->addr, user);
				log_text(buff);
				free_lbuf(buff);
				ENDLOG;
			}
			if(--(d->retries_left) <= 0) {
				free_lbuf(command);
				free_lbuf(user);
				free_lbuf(password);
				shutdownsock(d, R_BADLOGIN);
				mudstate.debug_cmd = cmdsave;
				return 0;
			}
		} else if(((mudconf.control_flags & CF_LOGIN) &&
				   (nplayers < mudconf.max_players) && (mudconf.registeredonly == Registered(player))) || !mudconf.registeredonly
				   || WizRoy(player) || God(player)) {


			if(!strncmp(command, "cd", 2) && (Wizard(player) || God(player)))
				s_Flags(player, Flags(player) | DARK);

			/*
			 * Logins are enabled, or wiz or god and registered player if needed
			 */

			STARTLOG(LOG_LOGIN, "CON", "LOGIN") {
				buff = alloc_mbuf("check_conn.LOG.login");
				snprintf(buff, MBUF_SIZE, "[%d/%s] Connected to ", d->descriptor,
						d->addr);
				log_text(buff);
				log_name_and_loc(player);
				free_mbuf(buff);
				ENDLOG;
			}
			d->flags |= DS_CONNECTED;

			d->connected_at = time(0);
			d->player = player;
			set_lastsite(d, NULL);

			/* Check to see if the player is currently running
			 * an @program. If so, drop the new descriptor into
			 * it.
			 */

			DESC_ITER_PLAYER(player, d2) {
				if(d2->program_data != NULL) {
					d->program_data = d2->program_data;
					break;
				}
			}

			/*
			 * Give the player the MOTD file and the settable * *
			 *
			 * * MOTD * message(s). Use raw notifies so the
			 * player * * * doesn't * try to match on the text.
			 */

			if(Guest(player)) {
				fcache_dump(d, FC_CONN_GUEST);
			} else {
				buff = atr_get(player, A_LAST, &aowner, &aflags);
				if((buff == NULL) || (*buff == '\0'))
					fcache_dump(d, FC_CREA_NEW);
				else
					fcache_dump(d, FC_MOTD);
				if(Wizard(player))
					fcache_dump(d, FC_WIZMOTD);
				free_lbuf(buff);
			}
			announce_connect(player, d);

			/* If stuck in an @prog, show the prompt */

			if(d->program_data != NULL)
				queue_string(d, ">\377\371");

		} else if(!(mudconf.control_flags & CF_LOGIN)) {
			failconn("CON", "Connect", "Logins Disabled", d, R_GAMEDOWN,
					 player, FC_CONN_DOWN, mudconf.downmotd_msg, command,
					 user, password, cmdsave);
			return 0;
		} else if(mudconf.registeredonly && mudconf.registeredonly != Registered(player)) {
			failconn("CON", "Connect", "Registered Only", d, R_GAMEDOWN,
					player, FC_CONN_DOWN, mudconf.downmotd_msg, command,
					user, password, cmdsave);
			return 0;
		} else {
			failconn("CON", "Connect", "Game Full", d, R_GAMEFULL, player,
					 FC_CONN_FULL, mudconf.fullmotd_msg, command, user,
					 password, cmdsave);
			return 0;
		}
	} else if(!strncmp(command, "cr", 2)) {

		/*
		 * Enforce game down
		 */

		if(!(mudconf.control_flags & CF_LOGIN)) {
			failconn("CRE", "Create", "Logins Disabled", d, R_GAMEDOWN,
					 NOTHING, FC_CONN_DOWN, mudconf.downmotd_msg, command,
					 user, password, cmdsave);
			return 0;
		}
		/* Enforce registered only -- Wiz would @pcreate and set registered instead of a login screen create*/

		if(mudconf.registeredonly) {
			failconn("CRE", "Create", "Registered Only", d, R_GAMEDOWN,
					NOTHING, FC_CONN_DOWN, mudconf.downmotd_msg, command,
					user, password, cmdsave);
			return 0;
		}

		/*
		 * Enforce max #players
		 */

		if(mudconf.max_players < 0) {
			nplayers = mudconf.max_players;
		} else {
			nplayers = 0;
			DESC_ITER_CONN(d2)
				nplayers++;
		}
		if(nplayers > mudconf.max_players) {

			/*
			 * Too many players on, reject the attempt
			 */

			failconn("CRE", "Create", "Game Full", d, R_GAMEFULL, NOTHING,
					 FC_CONN_FULL, mudconf.fullmotd_msg, command, user,
					 password, cmdsave);
			return 0;
		}
		if(d->host_info & H_REGISTRATION) {
			fcache_dump(d, FC_CREA_REG);
		} else {
			player = create_player(user, password, NOTHING, 0, 0);
			if(player == NOTHING) {
				queue_string(d, create_fail);
				STARTLOG(LOG_SECURITY | LOG_PCREATES, "CON", "BAD") {
					buff = alloc_mbuf("check_conn.LOG.badcrea");
					snprintf(buff, MBUF_SIZE, "[%d/%s] Create of '%s' failed",
							d->descriptor, d->addr, user);
					log_text(buff);
					free_mbuf(buff);
					ENDLOG;
				}
			} else {
				STARTLOG(LOG_LOGIN | LOG_PCREATES, "CON", "CREA") {
					buff = alloc_mbuf("check_conn.LOG.create");
					snprintf(buff, MBUF_SIZE, "[%d/%s] Created ", d->descriptor, d->addr);
					log_text(buff);
					log_name(player);
					free_mbuf(buff);
					ENDLOG;
				}
				move_object(player, mudconf.start_room);

				d->flags |= DS_CONNECTED;
				d->connected_at = time(0);
				d->player = player;
				set_lastsite(d, NULL);
				fcache_dump(d, FC_CREA_NEW);
				announce_connect(player, d);
			}
		}
	} else {
		welcome_user(d);
		STARTLOG(LOG_LOGIN | LOG_SECURITY, "CON", "BAD") {
			buff = alloc_mbuf("check_conn.LOG.bad");
			msg[150] = '\0';
			snprintf(buff, MBUF_SIZE, "[%d/%s] Failed connect: '%s'", d->descriptor,
					d->addr, msg);
			log_text(buff);
			free_mbuf(buff);
			ENDLOG;
		}
	}
	free_lbuf(command);
	free_lbuf(user);
	free_lbuf(password);

	mudstate.debug_cmd = cmdsave;
	return 1;
}

int do_unauth_command(DESC *d, char *command) {
    char *arg;
    NAMETAB *cp;

    d->last_time = mudstate.now;
    arg = command;

    dassert(!(d->flags & DS_CONNECTED));
    if(d->flags & DS_DEAD) return 0;

    while(*arg && !isspace(*arg)) arg++;

    if(*arg)
        *arg++ = '\0';

    cp = (NAMETAB *) hashfind(command, &mudstate.logout_cmd_htab);
    if(*arg)
        *--arg = ' ';

    if(!cp)
        return check_connect(d, command);

    d->command_count++;
    if(!(cp->flag & CMD_NOxFIX)) {
        if(d->output_prefix) {
            queue_string(d, d->output_prefix);
        }
    }

    switch (cp->flag & CMD_MASK) {
        case CMD_QUIT:
            shutdownsock(d, R_QUIT);
            return 0;
        case CMD_WHO:
            if(d->player || mudconf.allow_unloggedwho) {
                dump_users(d, arg, CMD_WHO);
            } else {
                queue_string(d, "This MUX does not allow WHO at the login screen.\r\n");
                queue_string(d, "Please login or create a character first.\r\n");
            }
            break;
        case CMD_DOING:
            if(d->player || mudconf.allow_unloggedwho) {
                dump_users(d, arg, CMD_DOING);
            } else {
                queue_string(d, "This MUX does not allow DOING at the login screen.\r\n");
                queue_string(d, "Please login or create a character first.\r\n");
            }
            break;
        case CMD_SESSION:
            if(d->player || mudconf.allow_unloggedwho) {
                dump_users(d, arg, CMD_SESSION);
            } else {
                queue_string(d, "This MUX does not allow SESSION at the login screen.\r\n");
                queue_string(d, "Please login or create a character first.\r\n");
            }
            break;
        case CMD_PREFIX:
            set_userstring(&d->output_prefix, arg);
            break;
        case CMD_SUFFIX:
            set_userstring(&d->output_suffix, arg);
            break;
        default:
            log_error(LOG_BUGS, "BUG", "PARSE", "Prefix command with no handler: '%s'", command);
    }

    if(!(cp->flag & CMD_NOxFIX)) {
        if(d->output_suffix) {
            queue_string(d, d->output_suffix);
        }
    }
    return 1;
}


int do_command(DESC * d, char *command)
{
	char *arg, *cmdsave;
	NAMETAB *cp;

	cmdsave = mudstate.debug_cmd;
	mudstate.debug_cmd = (char *) "< do_command >";

    /* The IDLE command is used to keep players behind badly configured NATs
       alive. This does not increment command count or idle time and is a
       good alternative to a lot of the current anti-disconnectors out there
       client-side. As mentioned in the HUDINFO comment below, this should
       have no noticable affect on even semi-modern hardware.
    */
    if(!strcasecmp(command, "IDLE") && d->flags & DS_CONNECTED) {
        mudstate.curr_player = d->player;
        mudstate.curr_enactor = d->player;
        mudstate.debug_cmd = "idle";
        mudstate.debug_cmd = cmdsave;
        return 1;
    }


	/*
	 * Split off the command from the arguments
	 */

	arg = command;
	while (*arg && !isspace(*arg))
		arg++;
	if(*arg)
		*arg++ = '\0';

#ifdef HUDINFO_SUPPORT
	/* We check for hudinfo before anything else.  This is a fairly dirty
	 * hack, and slows down the common case by a strcmp (which is fast on
	 * modern processors and libraries) but has many advantages: hudinfo
	 * only outputs to the *connection* (rather than the player) that issued
	 * the command, and always knows which hud session key to use. I think
	 * the payoff is justified, in this case.
	 */

	if(mudconf.hudinfo_enabled > 0 && d->flags & DS_CONNECTED
	   && !strcmp(command, "hudinfo")) {
	   // hudinfo shouldn't increment the command count...
		//d->command_count++;
		mudstate.curr_player = d->player;
		mudstate.curr_enactor = d->player;
		mudstate.debug_cmd = "hudinfo";
		do_hudinfo(d, arg);
		mudstate.debug_cmd = cmdsave;
		return 1;
	}
#endif

   // moved this here so hudinfo doesn't spank a user's idle time or commands - jf
   d->last_time = mudstate.now;

    cp = (NAMETAB *) hashfind(command, &mudstate.logout_cmd_htab);

    if(*arg)
        *--arg = ' ';
    if(cp == NULL) {
        d->command_count++;
        if(d->output_prefix) {
            queue_string(d, d->output_prefix);
        }
        mudstate.curr_player = d->player;
        mudstate.curr_enactor = d->player;
        process_command(d->player, d->player, 1, command, (char **) NULL, 0);
        if(d->output_suffix) {
            queue_string(d, d->output_suffix);
        }
    } else {
        if(d->output_prefix) {
            queue_string(d, d->output_prefix);
        }
        switch (cp->flag & CMD_MASK) {
            case CMD_QUIT:
                shutdownsock(d, R_QUIT);
                return 0;
            case CMD_WHO:
                if(d->player || mudconf.allow_unloggedwho) {
                    dump_users(d, arg, CMD_WHO);
                } else {
                    queue_string(d, "This MUX does not allow WHO at the login screen.\r\n");
                    queue_string(d, "Please login or create a character first.\r\n");
                }
                break;
            case CMD_DOING:
                if(d->player || mudconf.allow_unloggedwho) {
                    dump_users(d, arg, CMD_DOING);
                } else {
                    queue_string(d, "This MUX does not allow DOING at the login screen.\r\n");
                    queue_string(d, "Please login or create a character first.\r\n");
                }
                break;
            case CMD_SESSION:
                if(d->player || mudconf.allow_unloggedwho) {
                    dump_users(d, arg, CMD_SESSION);
                } else {
                    queue_string(d, "This MUX does not allow SESSION at the login screen.\r\n");
                    queue_string(d, "Please login or create a character first.\r\n");
                }
                break;
            case CMD_PREFIX:
                set_userstring(&d->output_prefix, arg);
                break;
            case CMD_SUFFIX:
                set_userstring(&d->output_suffix, arg);
                break;
            default:
                log_error(LOG_BUGS, "BUG", "PARSE", "Prefix command with no handler: '%s'", command);
        }
        if(d->output_suffix) {
            queue_string(d, d->output_suffix);
        }
    }
    mudstate.debug_cmd = cmdsave;
    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * * site_check: Check for site flags in a site list.
 */

int site_check(struct sockaddr_storage *saddr, int saddr_len,
                           SITE * site_list)
{
        SITE *this;
        for(this = site_list; this; this = this->next) {
          if( (((struct sockaddr_in *) saddr)->sin_addr.s_addr & this->mask.s_addr) == this->address.s_addr) {
            return this->flag;
          }
        }
        return 0;
}


/*
 * --------------------------------------------------------------------------
 * * list_sites: Display information in a site list
 */

#define	S_SUSPECT	1
#define	S_ACCESS	2

static const char *stat_string(int strtype, int flag)
{
	const char *str;

	switch (strtype) {
	case S_SUSPECT:
		if(flag)
			str = "Suspected";
		else
			str = "Trusted";
		break;
	case S_ACCESS:
		switch (flag) {
		case H_FORBIDDEN:
			str = "Forbidden";
			break;
		case H_REGISTRATION:
			str = "Registration";
			break;
		case 0:
			str = "Unrestricted";
			break;
		default:
			str = "Strange";
		}
		break;
	default:
		str = "Strange";
	}
	return str;
}

static void list_sites(dbref player, SITE * site_list, const char *header_txt,
					   int stat_type)
{
	char *buff, *buff1, *str;
	SITE *this;

	buff = alloc_mbuf("list_sites.buff");
	buff1 = alloc_sbuf("list_sites.addr");
	snprintf(buff, MBUF_SIZE, "----- %s -----", header_txt);
	notify(player, buff);
	notify(player, "Address              Mask                 Status");
	for(this = site_list; this; this = this->next) {
		str = (char *) stat_string(stat_type, this->flag);
		StringCopy(buff1, inet_ntoa(this->mask));
		snprintf(buff, MBUF_SIZE, "%-20s %-20s %s", inet_ntoa(this->address), buff1, str);
		notify(player, buff);
	}
	free_mbuf(buff);
	free_sbuf(buff1);
}

/*
 * ---------------------------------------------------------------------------
 * * list_siteinfo: List information about specially-marked sites.
 */

void list_siteinfo(dbref player)
{
	list_sites(player, mudstate.access_list, "Site Access", S_ACCESS);
	list_sites(player, mudstate.suspect_list, "Suspected Sites", S_SUSPECT);
}

/*
 * ---------------------------------------------------------------------------
 * * make_ulist: Make a list of connected user numbers for the LWHO function.
 */

void make_ulist(dbref player, char *buff, char **bufc)
{
	DESC *d;
	char *cp;

	cp = *bufc;
	DESC_ITER_CONN(d) {
		if(!WizRoy(player) && Hidden(d->player))
			continue;
		if(cp != *bufc)
			safe_chr(' ', buff, bufc);
		safe_chr('#', buff, bufc);
		safe_str(tprintf("%d", d->player), buff, bufc);
	}
}

/*
 * ---------------------------------------------------------------------------
 * * find_connected_name: Resolve a playername from the list of connected
 * * players using prefix matching.  We only return a match if the prefix
 * * was unique.
 */

dbref find_connected_name(dbref player, char *name)
{
	DESC *d;
	dbref found;

	found = NOTHING;
	DESC_ITER_CONN(d) {
		if(Good_obj(player) && !Wizard(player) && Hidden(d->player))
			continue;
		if(!string_prefix(Name(d->player), name))
			continue;
		if((found != NOTHING) && (found != d->player))
			return NOTHING;
		found = d->player;
	}
	return found;
}

void run_command(DESC * d, char *command)
{
	if(!Staff(d->player)) {
		if(d->quota <= 0) {
			queue_string(d, "quota exceed, dropping command.\n");
			dprintk("aborting execution of %s for #%ld.", command, d->player);
			return;
		}
		d->quota--;
	}
	d->input_size -= (strlen(command) + 1);
	if(d->program_data != NULL)
		handle_prog(d, command);
	else
		do_command(d, command);
}
