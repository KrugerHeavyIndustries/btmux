/*
 * version.c - version information 
 */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "mudconf.h"
#include "alloc.h"
#include "externs.h"
#include "patchlevel.h"

/*
 * 7 years of btech patches.
 */

/*
 * 1.0.0 TinyMUX 
 */

/*
 * 2.0
 * All known bugs fixed with disk-based.  Played with gdbm, it
 * sucked.  Now using bsd 4.4 hash stuff.
 */

/*
 * 1.12
 * * All known bugs fixed after several days of debugging 1.10/1.11.
 * * Much string-handling braindeath patched, but needs a big overhaul,
 * * really.   GAC 2/10/91
 */

/*
 * 1.11
 * * Fixes for 1.10.  (@name didn't call do_name, etc.)
 * * Added dexamine (debugging examine, dumps the struct, lots of info.)
 */

/*
 * 1.10
 * * Finally got db2newdb working well enough to run from the big (30000
 * * object) db with ATR_KEY and ATR_NAME defined.   GAC 2/3/91
 */

/*
 * TinyCWRU version.c file.  Add a comment here any time you've made a
 * * big enough revision to increment the TinyCWRU version #.
 */

void do_version(dbref player, dbref cause, int extra)
{
	notify(player, mudstate.version);
}

char *mux_version = PACKAGE_STRING "." MINOR_REVNUM
#ifdef HUDINFO_SUPPORT
    "+HUD"
#endif
#ifdef HAG_WAS_HERE
    "+HAG"
#endif
#ifdef SQL_SUPPORT
    "+SQL"
#endif
#ifdef ARBITRARY_LOGFILES
    "+ALG"
#endif
#ifdef DEBUG
    " DEBUG svn revision " SVN_REVISION
#else
    " RELEASE svn revision " SVN_REVISION
#endif
    " '" RELEASE_NAME "' build #" MUX_BUILD_NUM " on " MUX_BUILD_DATE
#ifdef DEBUG
    " by " MUX_BUILD_USER "@" MUX_BUILD_HOST
#endif
    ;


void init_version(void)
{
    strlcpy(mudstate.version, mux_version, sizeof(mudstate.version));

	STARTLOG(LOG_ALWAYS, "INI", "START") {
		log_text((char *) "Starting: ");
		log_text(mudstate.version);
		ENDLOG;
	} STARTLOG(LOG_ALWAYS, "INI", "START") {
		log_text((char *) "Build date: ");

		log_text((char *) MUX_BUILD_DATE);
		ENDLOG;
	}
}
