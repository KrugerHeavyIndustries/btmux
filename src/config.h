/* config.h */

#ifndef CONFIG_H
#define CONFIG_H

#include "copyright.h"
#include "autoconf.h"

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#ifdef STDC_HEADERS
#include <stdarg.h>
#else
#include <varargs.h>
#endif


#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "debug.h"

typedef int	dbref;
typedef int	FLAG;
typedef int     POWER;
typedef char	boolexp_type;
typedef char	IBUF[16];

#ifdef HAVE_SYS_RUSAGE_H
#include <sys/rusage.h>
#endif
#if defined(HAVE_SETRLIMIT) || defined(HAVE_GETRUSAGE)
#include <sys/resource.h>
#endif

#include <event.h>

/* TEST_MALLOC:	Defining this makes a malloc that keeps track of the number
 *		of blocks allocated.  Good for testing for Memory leaks.
 * ATR_NAME:	Define if you want name to be stored as an attribute on the
 *		object rather than in the object structure.
 */

/* Compile time options */

#define CONF_FILE "netmux.conf"	/* Default config file */
#define FILEDIR "files/"	/* Source for @cat */

/* #define TEST_MALLOC *//* Keep track of block allocs */
#define SIDE_EFFECT_FUNCTIONS	/* Those neat funcs that should be
				 * commands */
#define ENTERLEAVE_PARANOID	/* Enter/leave commands
				   require opposite locks succeeding
				   as well */
#define PLAYER_NAME_LIMIT	22	/* Max length for player names */
#define NUM_ENV_VARS		10	/* Number of env vars (%0 et al) */
#define MAX_ARG			100	/* max # args from command processor */
#define MAX_GLOBAL_REGS		10	/* r() registers */

#define HASH_FACTOR		16	/* How much hashing you want. */

#define PLUSHELP_COMMAND	"+help"	/* What you type to see the +help file */
#define OUTPUT_BLOCK_SIZE	16384
#define StringCopy		strcpy
#define StringCopyTrunc		strncpy

/* define DO_PARSE_WIZNEWS if wiznews.txt should be parsed like news.txt */
/* #define DO_PARSE_WIZNEWS */

/* define ARBITRARY_LOGFILES if you want (wiz-only) access to arbitrary
   logfiles in game/logs/, through @log and logf(). */

/* Define EXTENDED_DEFAULT_PARENTS to have room_parent and exit_parent mudconf
 * value (0 for none, default) to set a default exit and room parent. Usefull for some.
 */
#define EXTENDED_DEFAULT_PARENTS

#define CHANNEL_HISTORY
#define CHANNEL_HISTORY_LEN     20	/* at max 20 last msgs */
#define COMMAND_HISTORY_LEN     10	/* at max 10 last msgs */


/* ---------------------------------------------------------------------------
 * Database R/W flags.
 */

#define MANDFLAGS       (V_LINK|V_PARENT|V_XFLAGS|V_ZONE|V_POWERS|V_3FLAGS|V_QUOTED)

#define OFLAGS1		(V_GDBM|V_ATRKEY)	/* GDBM has these */

#define OFLAGS2		(V_ATRNAME|V_ATRMONEY)

#define OUTPUT_VERSION	1	/* Version 1 */
#define OUTPUT_FLAGS	(MANDFLAGS)

#define UNLOAD_VERSION	1	/* verison for export */
#define UNLOAD_OUTFLAGS	(MANDFLAGS)	/* format for export */

/* magic lock cookies */
#define NOT_TOKEN	'!'
#define AND_TOKEN	'&'
#define OR_TOKEN	'|'
#define LOOKUP_TOKEN	'*'
#define NUMBER_TOKEN	'#'
#define INDIR_TOKEN	'@'	/* One of these two should go. */
#define CARRY_TOKEN	'+'	/* One of these two should go. */
#define IS_TOKEN	'='
#define OWNER_TOKEN	'$'

/* matching attribute tokens */
#define AMATCH_CMD	'$'
#define AMATCH_LISTEN	'^'

/* delimiters for various things */
#define EXIT_DELIMITER	';'
#define ARG_DELIMITER	'='
#define ARG_LIST_DELIM	','

/* These chars get replaced by the current item from a list in commands and
 * functions that do iterative replacement, such as @apply_marked, dolist,
 * the eval= operator for @search, and iter().
 */

#define BOUND_VAR	"##"
#define LISTPLACE_VAR	"#@"

/* amount of object endowment, based on cost */
#define OBJECT_ENDOWMENT(cost) (((cost)/mudconf.sacfactor) +mudconf.sacadjust)

/* !!! added for recycling, return value of object */
#define OBJECT_DEPOSIT(pennies) \
    (((pennies)-mudconf.sacadjust)*mudconf.sacfactor)


#define DEV_NULL "/dev/null"
#define READ read
#define WRITE write

#ifdef BRAIN_DAMAGE		/* a kludge to get it to work on a mutant
				 * DENIX system */
#undef toupper
#endif

#ifdef TEST_MALLOC
extern int malloc_count;

#define XMALLOC(x,y) (fprintf(stderr,"Malloc: %s\n", (y)), malloc_count++, \
                    (char *)malloc((x)))
#define XFREE(x,y) (fprintf(stderr, "Free: %s\n", (y)), \
                    ((x) ? malloc_count--, free((x)), (x)=NULL : (x)))
#else
#define XMALLOC(x,y) (char *)malloc((x))
#define XFREE(x,y) (free((x)), (x) = NULL)
#endif				/* TEST_MALLOC */

#ifdef ENTERLEAVE_PARANOID
#define ENTER_REQUIRES_LEAVESUCC	/* Enter checks leaveloc of player's
					   origin */
#define LEAVE_REQUIRES_ENTERSUCC	/* Leave checks enterlock of player's
					   origin */
#endif

#define EVAL_ALL_NEWS 1
#include <sys/socket.h>
#ifndef HAVE_SRANDOM
#define random rand
#define srandom srand
#endif /* HAVE_SRANDOM */

#ifndef HAVE_STRNLEN
size_t strnlen(const char *s, size_t maxlen);
#endif
#ifndef HAVE_STRNDUP
char *strndup(const char *s, size_t n);
#endif
#ifndef HAVE_POSIX_MEMALIGN
int posix_memalign(void **memptr, size_t alignment, size_t size);
#endif
#ifndef HAVE_STRLCAT
size_t strlcat(char *, const char *, size_t);
#endif
#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif

#endif				/* CONFIG_H */
