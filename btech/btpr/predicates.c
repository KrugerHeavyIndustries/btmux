#include "mudconf.h"
#include "attrs.h"
#include "externs.h"

#include <assert.h>


/* MUX's could_doit() is essentially equivalent to PennMUSH's eval_lock().  To
 * further simplify things, BT only checks enter locks.
 */

extern int btshim_can_enter(dbref, dbref);

int
could_doit(dbref player, dbref thing, int locknum)
{
	assert(locknum == A_LENTER);

	return btshim_can_enter(player, thing);
}


extern void btshim_enter_fail(dbref, dbref, const char *);
extern void btshim_did_it(dbref, dbref, const char *, char *[], int);

void
did_it(dbref player, dbref thing,
       int what, const char *def, int owhat, const char *odef, int awhat,
       char *args[], int nargs)
{
	/* The calls to did_it really should be abstracted better, but we can
	 * use the following heuristic: If awhat == A_AFAIL, then we failed an
	 * enter lock.  Otherwise, we need to run A_AMECHDEST or
	 * A_AMINETRIGGER.
	 */
	switch (awhat) {
	case A_AFAIL:
		assert(args == NULL && nargs == 0);
		btshim_enter_fail(player, thing, def);
		break;
	
	case A_AMECHDEST:
		btshim_did_it(player, thing, "AMECHDEST", args, nargs);
		break;

	case A_AMINETRIGGER:
		assert(args == NULL && nargs == 0);
		btshim_did_it(player, thing, "AMINETRIGGER", NULL, 0);
		break;

	default:
		fprintf(stderr,
		        "warning: did_it(): attribute %d unsupported\n",
		        awhat);
		break;
	}
}


char *
#ifdef STDC_HEADERS
tprintf(const char *format, ...)
#else /* undef STDC_HEADERS */
tprintf(va_alist)
	va_dcl
#endif /* undef STDC_HEADERS */
{
	static char buff[LBUF_SIZE];
	va_list ap;

#ifdef STDC_HEADERS
	va_start(ap, format);
#else /* undef STDC_HEADERS */
	const char format;

	va_start(ap);
	format = va_arg(ap, char *);
#endif /* undef STDC_HEADERS */

	vsnprintf(buff, LBUF_SIZE, format, ap);
	va_end(ap);
	buff[LBUF_SIZE - 1] = '\0';

	return buff;
}

void
#ifdef STDC_HEADERS
safe_tprintf_str(char *str, char **bp, const char *format, ...)
#else /* undef STDC_HEADERS */
safe_tprintf_str(va_alist)
	va_dcl
#endif /* undef STDC_HEADERS */
{
	static char buff[LBUF_SIZE];
	va_list ap;

#ifdef STDC_HEADERS
	va_start(ap, format);
#else /* undef STDC_HEADERS */
	char *str;
	char **bp;
	const char *format;

	va_start(ap);
	str = va_arg(ap, char *);
	bp = va_arg(ap, char **);
	format = va_arg(ap, char *);
#endif /* undef STDC_HEADERS */

	/*
	 * Sigh, don't we wish _all_ vsprintf's returned int...
	 */
	vsnprintf(buff, LBUF_SIZE, format, ap);
	va_end(ap);
	buff[LBUF_SIZE - 1] = '\0';

	safe_str(buff, str, bp);
	**bp = '\0';
}
