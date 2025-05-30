
/*
 * $Id: mech.tech.repairs.c,v 1.1.1.1 2005/01/11 21:18:26 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Sat Aug 23 19:12:32 1997 fingon
 * Last modified: Sat Jun  6 20:45:48 1998 fingon
 *
 */

#include "mech.h"
#include "mech.events.h"
#include "mech.tech.h"
#include "mech.tech.damages.h"
#include "coolmenu.h"
#include "mycool.h"
#include "failures.h"
#include "p.mech.tech.h"
#include "p.mech.build.h"
#include "p.mech.status.h"

static coolmenu *c;

static void describe_repairs(MUXEVENT * e)
{
	int type = e->type;
	MECH *mech = (MECH *) e->data;
	long earg = ((long) e->data2) % PLAYERPOS;
	dbref player = ((long) e->data2) / PLAYERPOS;
	int loc, pos, extra;
	char buf[MBUF_SIZE] = { 0 };
	char buf2[LBUF_SIZE] = { 0 };
	int fail = (e->function == very_fake_func);

	UNPACK_LOCPOS_E(earg, loc, pos, extra);
	snprintf(buf, sizeof(buf), "%s%s", ShortArmorSectionString(MechType(mech),
												 MechMove(mech), loc % 8),
			loc >= 8 ? "(R)" : "");
	snprintf(buf2, sizeof(buf2), "%-5ld ", player);
	snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%-4d ",
			game_lag_time((e->tick - muxevent_tick) / 60));
	switch (type) {
	case EVENT_REPAIR_REPL:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s:%-2d Replacement of %s", buf,
				pos + 1, pos_part_name(mech, loc, pos));
		if(fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), " (Failure)");
		break;
	case EVENT_REPAIR_REPLG:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s:%-2d Replacement of %s", buf,
				pos + 1, pos_part_name(mech, loc, pos));
		if(fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), " (Failure)");
		break;
	case EVENT_REPAIR_REAT:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s Reattachment", buf);
		if(fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), " (Failure)");
		break;
	case EVENT_REPAIR_RELO:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s:%-2d %sload of %s", buf, pos + 1,
				extra ? "Un" : "Re", pos_part_name(mech, loc, pos));
		if(fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), " (Failure)");
		break;
	case EVENT_REPAIR_FIX:
		if(fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s:%-2d Failed armor repair",
					buf, 0);
		else
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2),
					"%5s:%-2d Repair of armor - possibly next point", buf,
					pos);
		break;
	case EVENT_REPAIR_FIXI:
		if(fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s:%-2d Failed internal repair", 
					buf, 0);
		else
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2),
					"%5s:%-2d Repair of internals - possibly next point", buf,
					pos);
		break;
	case EVENT_REPAIR_SCRL:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s Removal", buf);
		break;
	case EVENT_REPAIR_SCRP:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s:%-2d Scrapping of %s", buf,
				pos + 1, pos_part_name(mech, loc, pos));
		break;
	case EVENT_REPAIR_SCRG:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s:%-2d Scrapping of %s", buf,
				pos + 1, pos_part_name(mech, loc, pos));
		break;
	case EVENT_REPAIR_REPAG:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s:%-2d Repair of %s", buf, pos + 1,
				pos_part_name(mech, loc, pos));
		if(fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), " (Failure)");
		break;
	case EVENT_REPAIR_REPAP:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s:%-2d Repair of %s", buf, pos + 1,
				pos_part_name(mech, loc, pos));
		if(fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), " (Failure)");
		break;
	case EVENT_REPAIR_REPENHCRIT:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s:%-2d Repair of %s", buf, pos + 1,
				pos_part_name(mech, loc, pos));
		if(fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), " (Failure)");
		break;
	case EVENT_REPAIR_MOB:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s:%-2d Mounting of %s", buf,
				pos + 1, pos_part_name(mech, loc, pos));
		if(fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), " (Failure)");
		break;
	case EVENT_REPAIR_UMOB:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s:%-2d Removing of %s", buf,
				pos + 1, pos_part_name(mech, loc, pos));
		if(fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), " (Failure)");
		break;
	case EVENT_REPAIR_REPSUIT:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s Replacing suit", buf);
		if(fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), " (Failure)");
	// Added Reseal description
	case EVENT_REPAIR_RESE:
		snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), "%5s Reseal", buf);
		if (fail)
			snprintf(buf2 + strlen(buf2), sizeof(buf2) - strlen(buf2), " (Failure)");
		break;
	}

/*   snprintf(buf2+strlen(buf2), sizeof(buf2) - strlen(buf2), " - %s", */

/*        get_uptime_to_string(e->tick - muxevent_tick)); */
	vsi(buf2);
}

void tech_repairs(dbref player, MECH * mech, char *buffer)
{
	int i, isds = IsDS(mech);

	TECHCOMMANDD;

	DOCHECK(!figure_latest_tech_event(mech),
			"This 'mech has no repairs pending!");
	c = NULL;
	addline();
	cent(tprintf("Repairs/Scrapping in progress (%s)", GetMechID(mech)));
	vsi(tprintf("%-5s %-4s %s", "Plr", "Time", "Location + Description"));
	addline();
	for(i = FIRST_TECH_EVENT; i <= LAST_TECH_EVENT; i++)
		muxevent_gothru_type_data(i, (void *) mech, describe_repairs);
	addline();
	vsi("Note: Time = Time remaining in minutes. Plr = Tech's dbref");
	addline();
	ShowCoolMenu(player, c);
	KillCoolMenu(c);
}
