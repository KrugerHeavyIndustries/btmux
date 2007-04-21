#include "config.h"
#include "alloc.h"
#include "flags.h"
#include "externs.h"

extern char *colorize(dbref, char *);

/* #define NDEBUG */
#include <assert.h>

/**
 * The full notify_checked() function is quite long and involved, although Penn
 * likely has something similar.  However, BT only uses notify_checked() in two
 * situations:
 *
 * glue.c:mecha_notify_except()
 *   key: MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A | MSG_COLORIZE
 *   key: MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | MSG_COLORIZE
 *
 * externs.h:notify() (only in btech/mech.stats.c)
 *   key: MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN
 *
 * MSG_ME_ALL = MSG_ME | MSG_INV_EXITS | MSG_FWDLIST
 * MSG_F_UP = MSG_NBR_A | MSG_LOC_A
 * MSG_F_DOWN = MSG_INV_L
 */

#define CASE_MECHA_1 (MSG_ME | MSG_LOC_A | MSG_S_INSIDE  | MSG_COLORIZE | MSG_INV_EXITS | MSG_FWDLIST | MSG_NBR_A | MSG_NBR_EXITS_A)
#define CASE_MECHA_2 (MSG_ME | MSG_INV_L | MSG_S_OUTSIDE | MSG_COLORIZE)
#define CASE_NOTIFY (MSG_ME | MSG_INV_L | MSG_INV_EXITS | MSG_FWDLIST | MSG_PUP_ALWAYS)

void
notify_checked(dbref target, dbref sender, const char *msg, int key)
{
	char *colbuf = NULL;

	/* TODO: Turn off assertions in production builds.  */
	assert(key == CASE_MECHA_1
	       || key == CASE_MECHA_2
	       || key == CASE_NOTIFY);

	/* Validate input arguments.  TODO: Target must be a GoodObject.  */
	if (!msg || !*msg)
		return;

	/* Enforce recursion limit on notify().  We probably don't need to do
	 * this for hard code-generated notify()s.  Maybe.
	 */

	/* Generate NOSPOOF strings. (Always MSG_ME.) */

#if 0
	switch (key) {
	case CASE_MECHA_1:
		/* If target is a PLAYER, colorize output.  */
		if (isPlayer(target)) {
			/* FIXME: Make sure colorize() doesn't modify msg.  */
			colbuf = colorize(target, (char *)msg);
		}

		/* Something about 'inpipe'.  Objects get raw_notify() unless
		 * they're a connected player.
		 */
		/* Notify puppet owners.  Colorize output.  */
		/* Perform @listen matching and processing.  */
		/* Forward list processing.  */
		/* Audible (room) exit processing.  Include PREFIX.  */
		/* Audible (neighbor) processing.  */
		/* Notify neighbors.  */
		/* Notify container.  */

		/* FIXME */
		//fprintf(stderr, "broken: game.c: notify_checked(%d) > %s\r\n",
		//        target, msg);

		if (colbuf) {
			raw_notify(target, colbuf);
			free_lbuf(colbuf);
		} else {
			raw_notify(target, msg);
		}
		break;

	case CASE_MECHA_2:
		/* If target is a PLAYER, colorize output.  */
		if (isPlayer(target)) {
			/* FIXME: Make sure colorize() doesn't modify msg.  */
			colbuf = colorize(target, (char *)msg);
		}

		/* Something about 'inpipe'.  Objects get raw_notify() unless
		 * they're a connected player.
		 */
		/* Notify puppet owners.  Colorize output.  */
		/* Perform @listen matching and processing.  */
		/* Notify inventory (from outside) if listening.  */

		/* FIXME */
		//fprintf(stderr, "broken: game.c: notify_checked(%d) > %s\r\n",
		//        target, msg);

		if (colbuf) {
			raw_notify(target, colbuf);
			free_lbuf(colbuf);
		} else {
			raw_notify(target, msg);
		}
		break;

	case CASE_NOTIFY:
		/* Something about 'inpipe'.  Objects get raw_notify() unless
		 * they're a connected player.
		 */
		/* Always notify puppet owners.  Colorize output.  */
		/* Perform @listen matching and processing.  */
		/* Forward list processing.  */
		/* Audible (room) exit processing.  */
		/* Notify inventory if listening.  */

		/* FIXME */
		//fprintf(stderr, "broken: game.c: notify_checked(%d) > %s\r\n",
		//        target, msg);
		raw_notify(target, msg);
		break;

	default:
		/* FIXME: Replace with logging routine in production code.  */
		fputs("broken: game.c: notify_checked()\n", stderr);
		break;
	}
#else /* !0 */
	/* PennMUSH doesn't care if you colorize everything.  */
	colbuf = colorize(target, (char *)msg);
	raw_notify(target, colbuf);
	free_lbuf(colbuf);
#endif /* !0 */
}
