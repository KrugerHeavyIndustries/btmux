/*
 * log.c - logging routines 
 */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#include "db.h"
#include "mudconf.h"
#include "externs.h"
#include "flags.h"
#include "powers.h"
#include "alloc.h"
#include "htab.h"
#include "ansi.h"
#ifdef ARBITRARY_LOGFILES
#include "logcache.h"
#endif

NAMETAB logdata_nametab[] = {
	{(char *) "flags", 1, 0, LOGOPT_FLAGS},
	{(char *) "location", 1, 0, LOGOPT_LOC},
	{(char *) "owner", 1, 0, LOGOPT_OWNER},
	{(char *) "timestamp", 1, 0, LOGOPT_TIMESTAMP},
	{NULL, 0, 0, 0}
};

NAMETAB logoptions_nametab[] = {
	{(char *) "accounting", 2, 0, LOG_ACCOUNTING},
	{(char *) "all_commands", 2, 0, LOG_ALLCOMMANDS},
	{(char *) "suspect_commands", 2, 0, LOG_SUSPECTCMDS},
	{(char *) "bad_commands", 2, 0, LOG_BADCOMMANDS},
	{(char *) "buffer_alloc", 3, 0, LOG_ALLOCATE},
	{(char *) "bugs", 3, 0, LOG_BUGS},
	{(char *) "checkpoints", 2, 0, LOG_DBSAVES},
	{(char *) "config_changes", 2, 0, LOG_CONFIGMODS},
	{(char *) "create", 2, 0, LOG_PCREATES},
	{(char *) "killing", 1, 0, LOG_KILLS},
	{(char *) "logins", 1, 0, LOG_LOGIN},
	{(char *) "network", 1, 0, LOG_NET},
	{(char *) "problems", 1, 0, LOG_PROBLEMS},
	{(char *) "security", 2, 0, LOG_SECURITY},
	{(char *) "shouts", 2, 0, LOG_SHOUTS},
	{(char *) "startup", 2, 0, LOG_STARTUP},
	{(char *) "wizard", 1, 0, LOG_WIZARD},
	{NULL, 0, 0, 0}
};

char *strip_ansi_r(char *dest, char *raw, size_t n)
{
	char *p = (char *) raw;
	char *q = dest;

	while (p && *p && ((q - dest) < n)) {
		if(*p == ESC_CHAR) {
			/*
			 * Start of ANSI code. Skip to end. 
			 */
			while (*p && !isalpha(*p))
				p++;
			if(*p)
				p++;
		} else
			*q++ = *p++;
	}
	*q = '\0';
	return dest;
}

char *normal_to_white_r(char *dest, const char *raw, size_t n) {
    char *p = (char *) raw;
	char *q = dest;
    
	while (p && *p && ((q - dest) < n)) {
		if(*p == ESC_CHAR) {
			/*
			 * Start of ANSI code. 
			 */
			*q++ = *p++;		/*
								 * ESC CHAR 
								 */
			*q++ = *p++;		/*
								 * [ character. 
								 */
			if(*p == '0') {
                if((q - dest + 7) < n) {
                    memcpy(q, "0m\x1b[37m", 7);
                    q += 7;
                }
				p += 2;
			}
		} else
			*q++ = *p++;
	}
	*q = '\0';
	return buf;
}

/**
 * See if it's is OK to log something, and if so, start writing the
 * log entry.
 */
int start_log(const char *primary, const char *secondary)
{
	struct tm *tp;
	time_t now;

	mudstate.logging++;
	switch (mudstate.logging) {
	case 1:
	case 2:

		/*
		 * Format the timestamp 
		 */

		if((mudconf.log_info & LOGOPT_TIMESTAMP) != 0) {
			time((time_t *) (&now));
			tp = localtime((time_t *) (&now));
			sprintf(mudstate.buffer, "%d%02d%02d.%02d%02d%02d ",
					tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday,
					tp->tm_hour, tp->tm_min, tp->tm_sec);
		} else {
			mudstate.buffer[0] = '\0';
		}

		/*
		 * Write the header to the log 
		 */

		if(secondary && *secondary)
			fprintf(stderr, "%s%s %3s/%-5s: ", mudstate.buffer,
					mudconf.mud_name, primary, secondary);
		else
			fprintf(stderr, "%s%s %-9s: ", mudstate.buffer,
					mudconf.mud_name, primary);
		/*
		 * If a recursive call, log it and return indicating no log 
		 */

		if(mudstate.logging == 1)
			return 1;
		fprintf(stderr, "Recursive logging request.\r\n");
	default:
		mudstate.logging--;
	}
	return 0;
}

/**
 * Finish up writing a log entry
 */
void end_log(void)
{
	fprintf(stderr, "\n");
	fflush(stderr);
	mudstate.logging--;
}

/**
 * Write perror message to the log
 */
void log_perror(const char *primary, const char *secondary, const char *extra,
				const char *failing_object)
{
	start_log(primary, secondary);
	if(extra && *extra) {
		log_text((char *) "(");
		log_text((char *) extra);
		log_text((char *) ") ");
	}
	perror((char *) failing_object);
	fflush(stderr);
	mudstate.logging--;
}

/**
 * Write text to log file.
 */
void log_text(char *text)
{
	fprintf(stderr, "%s", strip_ansi(text));
}

void log_error(int key, char *primary, char *secondary, char *format, ...)
{
	char buffer[LBUF_SIZE];
	char stripped_buffer[LBUF_SIZE];
	va_list ap;

    if(!(key & mudconf.log_options))
		return;

	if(mudconf.log_info & LOGOPT_TIMESTAMP) {
		time_t now;
		struct tm tm;
		time(&now);
		localtime_r(&now, &tm);
		fprintf(stderr, "%d%02d%02d.%02d%02d%02d ",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec);
	}

	if(secondary && &secondary) {
		fprintf(stderr, "%s%s %3s/%-5s: ", mudstate.buffer,
				mudconf.mud_name, primary, secondary);
	} else {
		fprintf(stderr, "%s%s %-9s: ", mudstate.buffer,
				mudconf.mud_name, primary);
	}

	va_start(ap, format);
	vsnprintf(buffer, LBUF_SIZE, format, ap);
	va_end(ap);

	strip_ansi_r(stripped_buffer, buffer, LBUF_SIZE);
	fprintf(stderr, "%s\n", stripped_buffer);
}

void log_printf(char *format, ...)
{
	char buffer[LBUF_SIZE];
	char stripped_buffer[LBUF_SIZE];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buffer, LBUF_SIZE, format, ap);
	va_end(ap);

	strip_ansi_r(stripped_buffer, buffer, LBUF_SIZE);
	fprintf(stderr, "%s\n", stripped_buffer);
}

/*
 * Write a number to log file.
 */
void log_number(int num)
{
	fprintf(stderr, "%d", num);
}

/**
 * Writes the name, db number, and flags of an object to the log.
 * If the object does not own itself, append the name, db number, and flags
 * of the owner.
 */
void log_name(dbref target)
{
	char *tp;

	if((mudconf.log_info & LOGOPT_FLAGS) != 0)
		tp = unparse_object((dbref) GOD, target, 0);
	else
		tp = unparse_object_numonly(target);
	fprintf(stderr, "%s", strip_ansi(tp));
	free_lbuf(tp);
	if(((mudconf.log_info & LOGOPT_OWNER) != 0) && (target != Owner(target))) {
		if((mudconf.log_info & LOGOPT_FLAGS) != 0)
			tp = unparse_object((dbref) GOD, Owner(target), 0);
		else
			tp = unparse_object_numonly(Owner(target));
		fprintf(stderr, "[%s]", strip_ansi(tp));
		free_lbuf(tp);
	}
	return;
}

/**
 * Log both the name and location of an object
 */
void log_name_and_loc(dbref player)
{
	log_name(player);
	if((mudconf.log_info & LOGOPT_LOC) && Has_location(player)) {
		log_text((char *) " in ");
		log_name(Location(player));
	}
	return;
}

/*
 * Returns the object type of specified object.
 */
char *OBJTYP(dbref thing)
{
	if(!Good_obj(thing)) {
		return (char *) "??OUT-OF-RANGE??";
	}
	switch (Typeof(thing)) {
	case TYPE_PLAYER:
		return (char *) "PLAYER";
	case TYPE_THING:
		return (char *) "THING";
	case TYPE_ROOM:
		return (char *) "ROOM";
	case TYPE_EXIT:
		return (char *) "EXIT";
	case TYPE_GARBAGE:
		return (char *) "GARBAGE";
	default:
		return (char *) "??ILLEGAL??";
	}
}

void log_type_and_name(dbref thing)
{
	char nbuf[16];

	log_text(OBJTYP(thing));
	sprintf(nbuf, " #%d(", thing);
	log_text(nbuf);
	if(Good_obj(thing))
		log_text(Name(thing));
	log_text((char *) ")");
	return;
}

void log_type_and_num(dbref thing)
{
	char nbuf[16];

	log_text(OBJTYP(thing));
	sprintf(nbuf, " #%d", thing);
	log_text(nbuf);
	return;
}

#ifdef ARBITRARY_LOGFILES
int log_to_file(dbref thing, const char *logfile, const char *message)
{
	char pathname[210];			/* Arbitrary limit in logfile length */
	char message_buffer[4096];

	if(!message || !*message)
		return 1;				/* Nothing to do */

	if(!logfile || !*logfile || strlen(logfile) > 200)
		return 0;				/* invalid logfile name */

	if(strstr(pathname, "..") != NULL)
		return 0;
	if(strstr(pathname, "/") != NULL)
		return 0;
	snprintf(pathname, 210, "logs/%s", logfile);

	/* Hacking checks. */

	if(access(pathname, R_OK | W_OK) != 0)
		return 0;

	snprintf(message_buffer, 4096, "%s\n", message);

	if(!logcache_writelog(pathname, message_buffer)) {
		notify(thing, "Serious failure while trying to write to log.");
		return 0;
	}
	return 1;
}

void do_log(dbref player, dbref cause, int key, char *logfile, char *message)
{
	if(!message || !*message) {
		notify(player, "Nothing to log!");
		return;
	}

	if(!logfile || !*logfile) {
		notify(player, "Invalid logfile.");
		return;
	}

	if(!log_to_file(player, logfile, message)) {
		notify(player, "Request failed.");
		return;
	}

	notify(player, "Message logged.");
}
#endif
