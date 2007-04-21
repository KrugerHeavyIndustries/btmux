#include "config.h"
#include "flags.h"
#include "externs.h"

#include <assert.h>

#define BROKEN fprintf(stderr, "broken: " __FILE__ ": %s()\n", __FUNCTION__)


extern int btshim_teleport(dbref, dbref, int);

int
move_via_teleport(dbref thing, dbref dest, dbref cause, int hush)
{
	assert(cause == GOD);

	switch (hush) {
	case 0: /* teleport loudly */
		return btshim_teleport(thing, dest, 0);

	case 7: /* teleport quietly (both leaving and entering) */
		return btshim_teleport(thing, dest, 1);

	default:
		BROKEN;
		return 0;
	}
}
