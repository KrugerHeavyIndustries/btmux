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

#include "macros.h"
#include "mt19937ar.h"
#include <time.h>
#include <assert.h>

stat_type rollstat;

void init_stat()
{
	/* This is not necessary -- globals are always initialized empty */
	/* bzero(&stat, sizeof(stat)); */

	/* Seed random generator with current time.  */
	init_genrand((unsigned long)time(NULL));
}

static int chances[11] = { 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1 };

void do_show_stat(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
	int i;
	float f1, f2;
	int hitstatstotal;
	float hitavg, missavg, glanceavg;
	int totalhitrolls[3] = {0,0,0};

	if(!rollstat.totrolls) {
		notify(player, "No rolls to show statistics for!");
		return;
	}
	for(i = 0; i < 11; i++) {
		if(i == 0) {
			notify(player, "#    Rolls Optimal% Present% Diff. in 1000");
		}
		f1 = (float) chances[i] * 100.0 / 36.0;
		f2 = (float) rollstat.rolls[i] * 100.0 / rollstat.totrolls;
		notify_printf(player, "%-3d %6d %8.3f %8.3f %13.3f", i + 2,
					  rollstat.rolls[i], f1, f2, 10.0 * f2 - 10.0 * f1);
	}
	notify_printf(player, "Total rolls: %d", rollstat.totrolls);

	if(Wizard(player)) {
	for(i = 0; i < 11; i++) {
		if(i == 0) {
			notify(player, "\nWeapon Fire Stats\nBTH   #Misses              #Hits           #Glances              Total");
		}
		hitavg = missavg = glanceavg = 0.0;
		hitstatstotal = rollstat.hitstats[i][0] + rollstat.hitstats[i][1] + rollstat.hitstats[i][2];
		totalhitrolls[3] += hitstatstotal;
		totalhitrolls[0] += rollstat.hitstats[i][0];
		totalhitrolls[1] += rollstat.hitstats[i][1];
		totalhitrolls[2] += rollstat.hitstats[i][2];
		if(hitstatstotal) {
			missavg   = ( (float) rollstat.hitstats[i][0] / (float) hitstatstotal) * 100.0;
			hitavg    = ( (float) rollstat.hitstats[i][1] / (float) hitstatstotal) * 100.0;
			glanceavg = ( (float) rollstat.hitstats[i][2] / (float) hitstatstotal) * 100.0;
		}
		notify_printf(player,"%3d  %8d (%5.1f%%)  %8d (%5.1f%%)  %8d (%5.1f%%)  %8d",
			i+2, rollstat.hitstats[i][0],missavg,rollstat.hitstats[i][1],hitavg,rollstat.hitstats[i][2],glanceavg,hitstatstotal);
	}
	hitavg = missavg = glanceavg = 0.0;
	if(totalhitrolls[3]) {
		missavg   = ( (float) totalhitrolls[0] / (float) totalhitrolls[3]) * 100.0;
		hitavg    = ( (float) totalhitrolls[1] / (float) totalhitrolls[3]) * 100.0;
		glanceavg = ( (float) totalhitrolls[2] / (float) totalhitrolls[3]) * 100.0;
	}
	notify_printf(player,"ALL  %8d (%5.1f%%)  %8d (%5.1f%%)  %8d (%5.1f%%)  %8d", 
		totalhitrolls[0], missavg, totalhitrolls[1], hitavg, totalhitrolls[2], glanceavg, totalhitrolls[3]);
	
	}
}

/*
 * Returns an integer chosen randomly from the interval [low,high].
 *
 * To eliminate bias from rounding error, this routine repeatedly takes some
 * number of high order bits from the Mersenne Twister, until it finds a value
 * <= (high - low).  If we take n bits, such that 2^n is the smallest power of
 * two greater than (high - low), then this procedure should only require
 * another iteration 50% or less of the time. (The actual value would be
 * (2^n - (high - low)) / (high - low).) It also always terminates due to the
 * statistical qualities of the Mersenne Twister, although possibly only after
 * several (but generally very few) iterations.
 *
 * For example, computing a D6 should require a second iteration 33% (1/3rd) of
 * the time, a third iteration 11% (1/9th) of the time, a fourth iteration 3.7%
 * (1/27th) of the time, a fifth iteration 1.2% of the time (1/81st) of the
 * time, a sixth iteration 0.4% (1/243rd) of the time, and so on.  Or in other
 * words, this will require fewer than six iterations 99.6% of the time, while
 * completely eliminating rounding bias.
 *
 * This code is on the critical path, but modern processors can compute this
 * stuff really fast.  There's really no need to have the compiler inline it to
 * perform further optimization.
 */
long int
Number(long int low, long int high)
{
	const unsigned long int range = (unsigned long int)(high - low);

	unsigned long value;
	unsigned int nn;

	assert(high >= low);

	/*
	 * Compute n, the shift value.  We're using the 32-bit version of the
	 * Mersenne Twister, so we only need shifts up to 32. (If we did need a
	 * larger value, we would also need to expand our random number size.)
	 *
	 * We can special case some of the common values (such as n = 8 for
	 * range = 5, for the D6) if this loop becomes a concern.
	 */
	for (nn = 0; nn < 32; nn++) {
		if ((range >> nn) == 0)
			break;
	}

	nn = 32 - nn;

	/* Shifts >= bit width are undefined in C.  At least on x86, they
	 * apparently do nothing, which causes the following do-while loop to
	 * run until genrand_int32() returns 0.  */
	if (nn == 32) {
		return 0;
	}

	assert(nn >= 0 && nn < 32);

	/* Repeatedly select random numbers until we get an acceptable one.  */
	do {
		value = genrand_int32() >> nn;
	} while (value > range);

	return low + value;
}
