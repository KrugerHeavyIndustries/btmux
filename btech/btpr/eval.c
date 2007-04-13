#include "mudconf.h"
#include "externs.h"

#include <assert.h>

/* Eval flags used by mech.status.c:get_statustemplate_attr().  */

#define MS_EVAL \
	(EV_STRIP_AROUND | EV_NO_COMPRESS | EV_NO_LOCATION | EV_NOFCHECK \
	 | EV_NOTRACE | EV_FIGNORE)

extern void btshim_exec(char *, char **, const char **, dbref, dbref);

void
exec(char *buff, char **bufc, int tflags, dbref player, dbref cause,
     int eval, char **dstr, char *cargs[], int ncargs)
{
	const char *const_dstr;

	assert(tflags == 0 && eval == MS_EVAL && cargs == NULL && ncargs == 0);

	/* This is the "correct" way to handle the const-ness, but a cast would
	 * have worked just as well.  Still, it's not a big hit.
	 */
	const_dstr = *dstr;
	btshim_exec(buff, bufc, &const_dstr, player, cause);

	/* TODO: We're fairly sure const_dstr isn't going to point into some
	 * random new bit of unwritable memory.  Fairly.
	 */
	*dstr = (char *)const_dstr;
}
