#include "externs.h"

extern dbref btshim_lookup_player(const char *name);

dbref
lookup_player(dbref doer, char *name, int check_who)
{
	/* MUX supports "me", PennMUSH doesn't.  This only really matters to
	 * soft code (functions), as hard code only reads a dbref from
	 * PILOTNUM.
	 */
	return btshim_lookup_player(name);
}
