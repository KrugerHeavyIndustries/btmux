/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Tue Aug 12 19:06:48 1997 fingon
 * Last modified: Tue Aug 12 20:04:59 1997 fingon
 */

/* Make statistics 'bout what we do.. whatever it is we _do_ */

#define MECH_STAT_C
#include "mech.stat.h"
#include "db.h"
#include "externs.h"

stat_type rollstat;

void init_stat()
{
	/* This is not necessary -- globals are always initialized empty */
	/* bzero(&stat, sizeof(stat)); */
}

static int chances[11] = { 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1 };

void do_show_stat(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
	int i;
	float f1, f2;

	if(!rollstat.totrolls) {
		notify(player, "No rolls to show statistics for!");
		return;
	}
	for(i = 0; i < 11; i++) {
		if(i == 0) {
			notify(player, "#   Rolls  Optimal% Present% Diff. in 1000");
		}
		f1 = (float) chances[i] * 100.0 / 36.0;
		f2 = (float) rollstat.rolls[i] * 100.0 / rollstat.totrolls;
		notify_printf(player, "%-3d %6d %8.3f %8.3f %.3f", i + 2,
					  rollstat.rolls[i], f1, f2, 10.0 * f2 - 10.0 * f1);
	}
	notify_printf(player, "Total rolls: %d", rollstat.totrolls);
}
