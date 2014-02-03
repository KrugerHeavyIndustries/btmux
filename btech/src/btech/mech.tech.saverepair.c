
/*
 * $Id: mech.tech.saverepair.c,v 1.1.1.1 2005/01/11 21:18:26 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Fri Mar 14 19:17:19 1997 fingon
 * Last modified: Thu Oct 30 18:42:58 1997 fingon
 *
 */

#include "mech.h"
#include "mech.events.h"
#include "mech.tech.h"

static FILE *cheat_file;
static int ev_type;

#define CHESA(var) fwrite(&var, sizeof(var), 1, cheat_file)
#define CHELO(var)  if (!fread(&var, sizeof(var), 1, f)) return

static void save_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;
	long data = (long) e->data2;
	int t;

	t = e->tick - muxevent_tick;
	t = MAX(1, t);
	if(e->function == very_fake_func)
		t = 0 - t;
	CHESA(mech->mynum);
	CHESA(ev_type);
	CHESA(t);
	CHESA(data);
}

void saverepairs(FILE * f)
{
	int i;
	dbref d = -1;

	cheat_file = f;
	for(i = FIRST_TECH_EVENT; i <= LAST_TECH_EVENT; i++) {
		ev_type = i;
		muxevent_gothru_type(i, save_event);
	}
	CHESA(d);
}

void loadrepairs(FILE * f)
{
	dbref d, player;
	int type;
	int data;
	int time;
	MECH *mech;
	int loaded = 0;
	int fake;

	if(feof(f))
		return;
	fread(&d, sizeof(d), 1, f);
	while (d > 0 && !feof(f)) {
		loaded++;
		CHELO(type);
		CHELO(time);
		CHELO(data);
		fake = (time < 0);
		time = abs(time);
		if(!(mech = FindObjectsData(d)))
			continue;
		player = data / PLAYERPOS;
		data = data % PLAYERPOS;
		if(fake)
			FIXEVENT(time, mech, data, very_fake_func, type);
		else
			switch (type) {
			case EVENT_REPAIR_MOB:
				FIXEVENT(time, mech, data, muxevent_tickmech_mountbomb, type);
				break;
			case EVENT_REPAIR_UMOB:
				FIXEVENT(time, mech, data, muxevent_tickmech_umountbomb,
						 type);
				break;
			case EVENT_REPAIR_REPL:
				FIXEVENT(time, mech, data, muxevent_tickmech_repairpart,
						 type);
				break;
			case EVENT_REPAIR_REPLG:
				FIXEVENT(time, mech, data, muxevent_tickmech_replacegun,
						 type);
				break;
			case EVENT_REPAIR_REPAP:
				FIXEVENT(time, mech, data, muxevent_tickmech_repairpart,
						 type);
				break;
			case EVENT_REPAIR_REPENHCRIT:
				FIXEVENT(time, mech, data, muxevent_tickmech_repairenhcrit,
						 type);
				break;
			case EVENT_REPAIR_REPAG:
				FIXEVENT(time, mech, data, muxevent_tickmech_repairgun, type);
				break;
			case EVENT_REPAIR_REAT:
				FIXEVENT(time, mech, data, muxevent_tickmech_reattach, type);
				break;
			case EVENT_REPAIR_RELO:
				FIXEVENT(time, mech, data, muxevent_tickmech_reload, type);
				break;
			case EVENT_REPAIR_FIX:
				FIXEVENT(time, mech, data, muxevent_tickmech_repairarmor,
						 type);
				break;
			case EVENT_REPAIR_FIXI:
				FIXEVENT(time, mech, data, muxevent_tickmech_repairinternal,
						 type);
				break;
			case EVENT_REPAIR_SCRL:
				FIXEVENT(time, mech, data, muxevent_tickmech_removesection,
						 type);
				break;
			case EVENT_REPAIR_SCRG:
				FIXEVENT(time, mech, data, muxevent_tickmech_removegun, type);
				break;
			case EVENT_REPAIR_SCRP:
				FIXEVENT(time, mech, data, muxevent_tickmech_removepart,
						 type);
				break;
			case EVENT_REPAIR_REPSUIT:
				FIXEVENT(time, mech, data, muxevent_tickmech_replacesuit,
						 type);
				break;
			}
		CHELO(d);
	}
	if(loaded)
		fprintf(stderr, "LOADED: %d tech events.\n", loaded);
}
