
/*
 * $Id: events.c,v 1.2 2005/06/22 22:07:17 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1998 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters 
 *  Copyright (c) 2000-2002 Cord Awtry 
 *       All rights reserved
 *
 * Created: Wed Apr 29 20:17:02 1998 fingon
 * Last modified: Tue Jul 28 10:20:35 1998 fingon
 *
 */


#include "mech.h"
#include "mech.events.h"

#define MAX_EVENTS 100

int event_exec_count[MAX_EVENTS];

void event_count_initialize()
{
    int i;

    for (i = 0; i < MAX_EVENTS; i++)
	event_exec_count[i] = 0;
}
static int event_mech_event[] = {
    0, 1, 0, 1, 1, 1, 1, 1, 1, 0,	/* 0-9 */
    1, 0, 1, 1, 0, 1, 1, 0, 0, 1,	/*10-19 */
    1, 1, 1, 0, 0, 0, 0, 0, 0, 1,	/*20-29 */
    1, 0, 1, 1, 1, 0, 1, 1, 0, 1,	/*30-39 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/*40-49 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static char *event_names[] = {
    "NONAME",			/* 0 - */
    "Move",			/* 1 */
    "DHIT",			/* 2 */
    "Startup",			/* 3 */
    "Lock",			/* 4 */
    "Stand",			/* 5 */
    "Jump",			/* 6 */
    "Recycle",			/* 7 */
    "JumpSt",			/* 8 */
    "PRecov",			/* 9 */
    "SChange",			/* 10 */
    "DecRemv",			/* 11 */
    "SpotLck",			/* 12 */
    "PLos",			/* 13 */
    "ChkRng",			/* 14 */
    "Takeoff",			/* 15 */

    "Fall",			/* 16 */
    "BRegen",			/* 17 */
    "BRebuild",			/* 18 */
    "Dump",			/* 19 */

    "MASCF",			/* 20 */
    "MASCR",			/* 21 */
    "AmmoWarn",			/* 22 */

    "AutoGo",			/* 23 */
    "AutoLe",			/* 24 */
    "AutoCo",			/* 25 */
    "AutoGu",			/* 26 */
    "AutoGS",			/* 27 */
    "AutoFo",			/* 28 */
    "MRec",			/* 29 */

    "BlindR",			/* 30 */
    "Burn",			/* 31 */
    "SixthS",			/* 32 */

    "Hidin",			/* 33 */
    "OOD",			/* 34 */

    "Misc",			/* 35 */
    "Lateral",			/* 36 */
    "SelfExp",			/* 37 */

    "AutoRep",			/* 38 */
    "DigIn",			/* 39 */

    "TRepl",			/* 40 */
    "TReplG",			/* 41 */
    "TReat",			/* 42 */
    "TRelo",			/* 43 */
    "TFix",			/* 44 */
    "TFixI",			/* 45 */
    "TScrL",			/* 46 */
    "TScrP",			/* 47 */
    "TScrG",			/* 48 */
    "TRepaG",			/* 49 */
    "TRepaP",			/* 50 */
    "TMoB",			/* 51 */
    "TUMoB",			/* 52 */
    "StandF",			/* 53 */
    "SliteC",			/* 54 */
    "HeatCutOff",		/* 55 */
    "56",
    "57",
    "58",
    "59",
    "60",
    "61",
    "62",
    "63",
    "64",
    "65",
    "66",
    NULL
};

void debug_EventTypes(dbref player, void *data, char *buffer)
{
    int i, j, k, tot = 0;

    if (buffer && *buffer) {
	int t[MAX_EVENTS];
	int tot_ev = 0;

	for (i = 0; i < MAX_EVENTS; i++) {
	    t[i] = i;
	    tot_ev += event_exec_count[i];
	}
	for (i = 0; i < (MAX_EVENTS - 1); i++)
	    for (j = i + 1; j < MAX_EVENTS; j++)
		if (event_exec_count[t[i]] > event_exec_count[t[j]]) {
		    int s = t[i];

		    t[i] = t[j];
		    t[j] = s;
		}
	/* Then, display */
	notify(player, "Event history (by use)");
	for (i = 0; i < MAX_EVENTS; i++)
	    if (event_exec_count[t[i]])
		notify(player, tprintf("%-3d%-20s%10d %.3f%%", t[i],
			event_names[t[i]], event_exec_count[t[i]],
			((float) 100.0 * event_exec_count[t[i]] /
			    (tot_ev ? tot_ev : 1))));

	return;
    }
    notify(player, "Events by type: ");
    notify(player, "-------------------------------");
    k = muxevent_last_type();
    for (i = 0; i <= k; i++) {
	j = muxevent_count_type(i);
	if (!j)
	    continue;
	tot += j;
	notify(player, tprintf("%-20s%d", event_names[i], j));
    }
    if (tot)
	notify(player, "-------------------------------");
    notify(player, tprintf("%d total", tot));
    notify(player, tprintf("%d scheduled - %d executed / %d zombies.",
	    events_scheduled, events_executed, events_zombies));
    if ((i = abs(events_scheduled - (j =
		    (tot + events_executed + events_zombies)))))
	notify(player, tprintf("ERROR: %d events %s!", i,
		events_scheduled > j ? "missing" : "too many"));
    else
	notify(player, "Events seem to have been executed perfectly.");
}


void prerun_event(EVENT * e)
{
#if 1
    static char buf[LBUF_SIZE];
    MECH *mech = (MECH *) e->data;

    /* Magic 2-hour uptime means that we are 'supposedly' stable
       [ read: crashy as hell, but.. :> you never know ] */
    if (muxevent_tick <= 7200) {
	if (event_mech_event[(int) e->type])
	    sprintf(buf, "< %s event for #%d[%s] driven by #%d[%s] >",
		event_names[(int) e->type], mech->mynum, GetMechID(mech),
		MechPilot(mech),
		Good_obj(MechPilot(mech)) ? Name(MechPilot(mech)) :
		"Nobody");
	else
	    sprintf(buf, "< %s event >", event_names[(int) e->type]);
	mudstate.debug_cmd = buf;
    }
#endif
}

void postrun_event(EVENT * e)
{
    event_exec_count[(int) e->type]++;
}
