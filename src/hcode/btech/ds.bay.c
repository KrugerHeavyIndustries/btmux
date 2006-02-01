/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 */

#include "config.h"

#include <math.h>

#include "mech.h"
#include "mech.events.h"
#include "p.mech.utils.h"
#include "p.bsuit.h"
#include "p.eject.h"
#include "p.mech.restrict.h"
#include "p.mech.startup.h"

dbref match_thing(dbref player, char *name);

void mech_createbays(dbref player, void *data, char *buffer)
{
	char *args[NUM_BAYS + 1];
	int argc;
	dbref it;
	int i;
	MECH *ds = (MECH *) data;
	MAP *map;

	DOCHECK((argc =
			 mech_parseattributes(buffer, args,
								  NUM_BAYS + 1)) == (NUM_BAYS + 1),
			"Invalid number of arguments!");
	for(i = 0; i < argc; i++) {
		it = match_thing(player, args[i]);
		DOCHECK(it == NOTHING, tprintf("Argument %d is invalid.", i + 1));
		DOCHECK(!IsMap(it), tprintf("Argument %d is not a map.", i + 1));
		map = FindObjectsData(it);
		AeroBay(ds, i) = it;
		map->onmap = ds->mynum;
	}
	for(i = argc; i < NUM_BAYS; i++)
		AeroBay(ds, i) = -1;
	notify_printf(player, "%d bay(s) set up!", argc);
}

extern int dirs[6][2];

static int dir2loc[6] =
	{ DS_NOSE, DS_RWING, DS_RRWING, DS_AFT, DS_LRWING, DS_LWING };

int Find_DS_Bay_Number(MECH * ds, int dir)
{
	int bayn = 0;
	int i, j;

	for(i = 0; i <= dir; i++) {
		for(j = 0; j < NUM_CRITICALS; j++)
			if(GetPartType(ds, dir2loc[i % 6],
						   j) == I2Special(DS_MECHDOOR) ||
			   GetPartType(ds, dir2loc[i % 6], j) == I2Special(DS_AERODOOR))
				break;
		if(j != NUM_CRITICALS) {
			if(i == dir)
				return bayn;
			bayn++;
		}
	}
	return -1;
}

int Find_DS_Bay_Dir(MECH * ds, int num)
{
	int i;

	for(i = 0; i < 6; i++)
		if(Find_DS_Bay_Number(ds, i) == num)
			return i;
	return -1;
}

#define KLUDGE(fx,tx) ((((fx)%2)&&!((tx)%2)) ? -1 : 0)

int Find_DS_Bay_In_MechHex(MECH * seer, MECH * ds, int *bayn)
{
	int i;
	int t = DSBearMod(ds);

	for(i = t; i < (t + 6); i++) {

		if(((MechX(ds) + dirs[i % 6][0]) == MechX(seer)) &&
		   ((MechY(ds) + dirs[i % 6][1] + KLUDGE(MechX(ds),
												 MechX(ds) +
												 dirs[i % 6][0])) ==
			MechY(seer))) {
			if((*bayn = Find_DS_Bay_Number(ds, ((i - t + 6) % 6))) >= 0)
				return 1;
			return 0;
		}
	}
	return 0;
}

static int Find_Single_DS_In_MechHex(MECH * mech, int *ref, int *bayn)
{
	MAP *map = FindObjectsData(mech->mapindex);
	int loop;
	MECH *tempMech;
	int count = 0;

	*ref = 0;
	if(!map)
		return 0;
	for(loop = 0; loop < map->first_free; loop++)
		if(map->mechsOnMap[loop] >= 0) {
			if(!(tempMech = getMech(map->mechsOnMap[loop])))
				continue;
			if(!IsDS(tempMech))
				continue;
			if(!Landed(tempMech))
				continue;		/* This might break midflight-aero-DS-docking. But aeros are broken anyway. */
			if(Find_DS_Bay_In_MechHex(mech, tempMech, bayn)) {
				if(count++)
					*ref = -1;
				else
					*ref = tempMech->mynum;
			}
		}
	return count;
}

static void mech_enterbay_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data, *ds, *tmpm = NULL;
	int ref = (int) e->data2;
	int bayn;
	int x = 5, y = 5;
	MAP *tmpmap;

	if(!Started(mech) || Uncon(mech) || Jumping(mech) ||
	   (MechType(mech) == CLASS_MECH && (Fallen(mech) || Standing(mech))) ||
	   OODing(mech) || (fabs(MechSpeed(mech)) * 5 >= MMaxSpeed(mech) &&
						fabs(MMaxSpeed(mech)) >= MP1)
	   || (MechType(mech) == CLASS_VTOL && AeroFuel(mech) <= 0))
		return;
	tmpmap = getMap(ref);
	if(!(ds = getMech(tmpmap->onmap)))
		return;
	if(!Find_DS_Bay_In_MechHex(mech, ds, &bayn))
		return;
	/* whee */
	ref = AeroBay(ds, bayn);
	StopBSuitSwarmers(FindObjectsData(mech->mapindex), mech, 1);
	mech_notify(mech, MECHALL, "You enter bay.");
	MechLOSBroadcast(mech, tprintf("has entered %s at %d,%d.",
								   GetMechID(ds), MechX(mech), MechY(mech)));
	MarkForLOSUpdate(mech);
	if(MechType(mech) == CLASS_MW && !In_Character(ref)) {
		enter_mw_bay(mech, ref);
		return;
	}
	if(MechCarrying(mech) > 0)
		tmpm = getMech(MechCarrying(mech));
	mech_Rsetmapindex(GOD, (void *) mech, tprintf("%d", ref));
	mech_Rsetxy(GOD, (void *) mech, tprintf("%d %d", x, y));
	MechLOSBroadcast(mech, "has entered the bay.");
	loud_teleport(mech->mynum, ref);
	if(tmpm) {
		mech_Rsetmapindex(GOD, (void *) tmpm, tprintf("%d", ref));
		mech_Rsetxy(GOD, (void *) tmpm, tprintf("%d %d", x, y));
		loud_teleport(tmpm->mynum, ref);
	}
}

static int DS_Bay_Is_Open(MECH * mech, MECH * ds, dbref bayref)
{
	int i, j;

	for(i = 0; i < NUM_BAYS; i++)
		if(AeroBay(ds, i) > 0)
			if(AeroBay(ds, i) == bayref) {
				j = Find_DS_Bay_Dir(ds, i);
				for(i = 0; i < NUM_CRITICALS; i++) {
					if(((is_aero(mech) &&
						 GetPartType(ds, dir2loc[j],
									 i) == I2Special(DS_AERODOOR)) ||
						(!is_aero(mech)
						 && GetPartType(ds, dir2loc[j],
										i) == I2Special(DS_MECHDOOR))) &&
					   !PartIsDestroyed(ds, dir2loc[j], i))
						return 1;
				}
				return 0;
			}
	return 0;
}

static int DS_Bay_Is_EnterOK(MECH * mech, MECH * ds, dbref bayref)
{
	int i;

	for(i = 0; i < NUM_BAYS; i++)
		if(AeroBay(ds, i) > 0)
			if(AeroBay(ds, i) == bayref)
				return muxevent_count_type_data2(EVENT_ENTER_HANGAR,
												 (void *) bayref) > 0 ? 0 : 1;
	return 0;
}

/* ID / Number, both optional (this _will_ be painful) */

void mech_enterbay(dbref player, void *data, char *buffer)
{
	char *args[3];
	int argc;
	dbref ref = -1, bayn = -1;
	MECH *mech = data, *ds;
	MAP *map;

	cch(MECH_USUAL);
	DOCHECK(MechType(mech) == CLASS_VTOL &&
			AeroFuel(mech) <= 0, "You lack fuel to maneuver in!");
	DOCHECK(Jumping(mech), "While in mid-jump? No way.");
	DOCHECK(MechType(mech) == CLASS_MECH && (Fallen(mech) ||
											 Standing(mech)),
			"Crawl inside? I think not. Stand first.");
	DOCHECK(OODing(mech), "While in mid-flight? No way.");
	DOCHECK((argc =
			 mech_parseattributes(buffer, args, 2)) == 2,
			"Hmm, invalid number of arguments?");
	if(argc > 0)
		DOCHECK((ref =
				 FindTargetDBREFFromMapNumber(mech, args[0])) <= 0,
				"Invalid target!");
	if(ref < 0) {
		DOCHECK(!Find_Single_DS_In_MechHex(mech, &ref, &bayn),
				"No DS bay found in your hex!");
		DOCHECK(ref < 0,
				"Multiple enterable things found ; use the id for specifying which you want.");
		DOCHECK(!(ds =
				  getMech(ref)), "You sense wrongness in fabric of space.");
	} else {
		DOCHECK(!(ds =
				  getMech(ref)), "You sense wrongness in fabric of space.");
		DOCHECK(!Find_DS_Bay_In_MechHex(mech, ds, &bayn),
				"You see no bays in your hex.");
	}
	DOCHECK(IsDS(mech)
			&& !(MechSpecials2(mech) & CARRIER_TECH),
			"Your craft can't enter bays.");
	DOCHECK(!DS_Bay_Is_Open(mech, ds, AeroBay(ds, bayn)),
			"The door has been jammed!");
	DOCHECK(IsDS(mech), "Your unit is a bit too large to fit in there.");
	DOCHECK((fabs((float) (MechSpeed(mech) - MechSpeed(ds)))) > MP1,
			"Speed difference's too large to enter!");
	DOCHECK(MechZ(ds) != MechZ(mech),
			"Get to same elevation before thinking about entering!");
	DOCHECK(abs(MechVerticalSpeed(mech) - MechVerticalSpeed(ds)) > 10,
			"Vertical speed difference is too great to enter safely!");
	DOCHECK(MechType(mech) == CLASS_MECH && !MechIsQuad(mech) &&
			(IsMechLegLess(mech)), "Without legs? Are you kidding?");
	ref = AeroBay(ds, bayn);
	map = getMap(ref);

	DOCHECK(!map, "You sense wrongness in fabric of space.");

	DOCHECK(EnteringHangar(mech), "You are already entering the hangar!");
	if(!can_pass_lock(mech->mynum, ref, A_LENTER)) {
		char *msg = silly_atr_get(ref, A_FAIL);
		if(!msg || !*msg)
			msg = "You are unable to enter the bay!";
		notify(player, msg);
		return;
	}
	DOCHECK(!DS_Bay_Is_EnterOK(mech, ds, AeroBay(ds, bayn)),
			"Someone else is using the door at the moment.");
	DOCHECK(!(map =
			  getMap(mech->mapindex)),
			"You sense a wrongness in fabric of space.");
	HexLOSBroadcast(map, MechX(mech), MechY(mech),
					"The bay doors at $h start to open..");
	MECHEVENT(mech, EVENT_ENTER_HANGAR, mech_enterbay_event, 12, ref);
}

static void DS_Place(MECH * ds, MECH * mech, int frombay)
{
	int i;
	int nx, ny;
	MAP *mech_map;

	for(i = 0; i < NUM_BAYS; i++)
		if(AeroBay(ds, i) == frombay)
			break;
	if(i == NUM_BAYS || !(mech_map = getMap(mech->mapindex))) {
		/* i _should_ be set, otherwise things are deeply disturbing */
		mech_notify(mech, MECHALL, "Reality collapse imminent.");
		return;
	}
	i = Find_DS_Bay_Dir(ds, i);
	nx = dirs[(DSBearMod(ds) + i) % 6][0] + MechX(ds);
	ny = dirs[(DSBearMod(ds) + i) % 6][1] + MechY(ds) + KLUDGE(MechX(ds), nx);
	nx = BOUNDED(0, nx, mech_map->width - 1);
	ny = BOUNDED(0, ny, mech_map->height - 1);

	/* snippage from mech_Rsetxy */
	MechX(mech) = nx;
	MechLastX(mech) = nx;
	MechY(mech) = ny;
	MechLastY(mech) = ny;
	MechZ(mech) = MechZ(ds);
	MechElev(mech) = MechElev(ds);
	MapCoordToRealCoord(MechX(mech), MechY(mech), &MechFX(mech),
						&MechFY(mech));
	MechTerrain(mech) = GetTerrain(mech_map, MechX(mech), MechY(mech));
}

static int Leave_DS_Bay(MAP * map, MECH * ds, MECH * mech, dbref frombay)
{
	MECH *car = NULL;

	StopBSuitSwarmers(FindObjectsData(mech->mapindex), mech, 1);
	MechLOSBroadcast(mech, "has left the bay.");
	/* We escape confines of the bay to open air/land! */
	mech_Rsetmapindex(GOD, (void *) mech, tprintf("%d", ds->mapindex));
	if(MechCarrying(mech) > 0)
		car = getMech(MechCarrying(mech));
	if(car)
		mech_Rsetmapindex(GOD, (void *) car, tprintf("%d", ds->mapindex));
	DOCHECKMA0(mech->mapindex == map->mynum,
			   "Fatal error: Unable to find the map 'ship is on.");
	loud_teleport(mech->mynum, mech->mapindex);
	if(car)
		loud_teleport(car->mynum, mech->mapindex);
	mech_notify(mech, MECHALL, "You have left the bay.");
	DS_Place(ds, mech, frombay);
	if(car)
		MirrorPosition(mech, car, 0);
	MechLOSBroadcasti(mech, ds, "has left %s's bay.");
	mech_notify(ds, MECHALL, tprintf("%s has left the bay.",
									 GetMechID(mech)));
	ContinueFlying(mech);
	if(In_Character(mech->mynum) && Location(MechPilot(mech)) != mech->mynum) {
		mech_notify(mech, MECHALL,
					"%ch%cr%cf%ciINTRUDER ALERT! INTRUDER ALERT!%c");
		mech_notify(mech, MECHALL,
					"%ch%cr%cfAutomatic self-destruct sequence initiated.%c");
		mech_shutdown(GOD, (void *) mech, "");
	}
	return 1;
}

int Leave_DS(MAP * map, MECH * mech)
{
	MECH *car;

	DOCHECKMA0(!(car = getMech(map->onmap)), "Invalid : No parent object?");
	DOCHECKMA0(!DS_Bay_Is_Open(mech, car, map->mynum),
			   "The door has been jammed!");
	DOCHECKMA0(!Landed(car) &&
			   !FlyingT(mech), "The 'ship is still airborne!");
	DOCHECKMA0(Zombie(car->mynum),
			   "You don't feel leaving right now would be prudent..");
	return Leave_DS_Bay(map, car, mech, map->mynum);
}
