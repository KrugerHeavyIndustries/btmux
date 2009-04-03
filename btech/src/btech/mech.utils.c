/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "map.h"
#include "mech.events.h"
#include "p.mech.restrict.h"
#include "p.mech.consistency.h"
#include "p.mech.utils.h"
#include "p.mech.startup.h"
#include "p.ds.bay.h"
#include "p.btechstats.h"
#include "p.mechrep.h"
#include "p.crit.h"
#include "p.mech.combat.h"
#include "p.mech.damage.h"
#include "p.template.h"
#include "p.bsuit.h"
#include "p.mech.los.h"
#include "p.aero.bomb.h"
#include "autopilot.h"
#include "mt19937ar.h"

#ifdef BT_ADVANCED_ECON
#include "p.mech.tech.do.h"
#endif

#ifdef BT_PART_WEIGHTS
/* From template.c */
extern int internalsweight[];
extern int cargoweight[];
#endif

#ifdef BT_MOVEMENT_MODES
#include "failures.h"
#endif

/* TODO: We can use M_PI if exists, otherwise define something reasonable.  */
#define DEG2RAD(d) ((float)(d) * (3.14159265f / 180.f))
#define RAD2DEG(d) ((float)(d) * (180.f / 3.14159265f))

extern dbref pilot_override;

char *mechtypenames[CLASS_LAST + 1] = {
	"mech", "tank", "VTOL", "vessel", "aerofighter", "DropShip"
};

const char *mechtypename(MECH * foo)
{
	return mechtypenames[(int) MechType(foo)];
}

int MNumber(MECH * mech, int low, int high)
{
	if((muxevent_tick / RANDOM_TICK) != MechLastRndU(mech)) {
		MechRnd(mech) = (int)genrand_int31();
		MechLastRndU(mech) = muxevent_tick / RANDOM_TICK;
	}
	return (low + MechRnd(mech) % (high - low + 1));
}

char *MechIDS(MECH * mech, int islower)
{
	static char buf[3];

	if(mech) {
		buf[0] = MechID(mech)[0];
		buf[1] = MechID(mech)[1];
	} else {
		buf[0] = '*';
		buf[1] = '*';
	}
	buf[2] = 0;

	if(islower) {
		buf[0] = tolower(buf[0]);
		buf[1] = tolower(buf[1]);
	}
	return buf;
}

char *MyToUpper(char *string)
{
	if(*string)
		*string = toupper(*string);
	return string;
}

int CritsInLoc(MECH * mech, int index)
{
	if(MechType(mech) == CLASS_MECH)
		switch (index) {
		case HEAD:
		case RLEG:
		case LLEG:
			return 6;
		case RARM:
		case LARM:
			if(MechIsQuad(mech))
				return 6;
	} else if(MechType(mech) == CLASS_MW)
		return 2;
	return NUM_CRITICALS;
}

int SectHasBusyWeap(MECH * mech, int sect)
{
	int i = 0, count, critical[MAX_WEAPS_SECTION];
	unsigned char weaptype[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];

	count = FindWeapons(mech, sect, weaptype, weapdata, critical);
	for(i = 0; i < count; i++)
		if(WpnIsRecycling(mech, sect, critical[i]))
			return 1;
	return 0;
}

MAP *ValidMap(dbref player, dbref map)
{
	char *str;
	MAP *maps;

	DOCHECKN(!Good_obj(map), "Index out of range!");
	str = silly_atr_get(map, A_XTYPE);
	DOCHECKN(!str || !*str, "That is not a valid map! (no XTYPE!)");
	DOCHECKN(strcmp("MAP", str), "That is not a valid map!");
	DOCHECKN(!(maps = getMap(map)), "The map has not been allocated!!");
	return maps;
}

dbref FindMechOnMap(MAP * map, char *mechid)
{
	int loop;
	MECH *tempMech;

	for(loop = 0; loop < map->first_free; loop++)
		if(map->mechsOnMap[loop] != -1) {
			tempMech = getMech(map->mechsOnMap[loop]);
			if(tempMech && !strncasecmp(MechID(tempMech), mechid, 2))
				return tempMech->mynum;
		}
	return -1;
}

dbref FindTargetDBREFFromMapNumber(MECH * mech, char *mapnum)
{
	MAP *map;

	if(mech->mapindex == -1)
		return -1;
	map = getMap(mech->mapindex);
	if(!map) {
		SendError(tprintf("FTDBREFFMN:invalid map:Mech: %d  Index: %d",
						  mech->mynum, mech->mapindex));
		mech->mapindex = -1;
		return -1;
	}
	return FindMechOnMap(map, mapnum);
}

void FindComponents(float magnitude, int degrees, float *x, float *y)
{
	*x = magnitude * fcos((float) (TWOPIOVER360 * (degrees + 90)));
	*y = magnitude * fsin((float) (TWOPIOVER360 * (degrees + 90)));
	*x = -(*x);					/* because 90 is to the right */
	*y = -(*y);					/* because y increases downwards */
}

static int Leave_Hangar(MAP * map, MECH * mech)
{
	MECH *car = NULL;
	int mapob;
	mapobj *mapo;

	/* For now, leaving leads to finding yourself on the new map
	   at a predetermined position */
	mapob = mech->mapindex;
	if(MechCarrying(mech) > 0)
		car = getMech(MechCarrying(mech));
	DOCHECKMA0(!map->cf, "The entrance is still filled with rubble!");
	MechLOSBroadcast(mech, "has left the hangar.");
	mech_Rsetmapindex(GOD, (void *) mech, tprintf("%d",
												  (int) map->
												  mapobj[TYPE_LEAVE]->obj));
	if(car)
		mech_Rsetmapindex(GOD, (void *) car, tprintf("%d",
													 (int) map->
													 mapobj[TYPE_LEAVE]->
													 obj));
	map = getMap(mech->mapindex);
	if(mech->mapindex == mapob) {
		SendError(tprintf("#%d %s attempted to leave, but no target map?",
						  mech->mynum, GetMechID(mech)));
		mech_notify(mech, MECHALL,
					"Exit of this map is.. fubared. Please contact a wizard");
		return 0;
	}
	if(!(mapo = find_entrance_by_target(map, mapob))) {
		SendError(tprintf
				  ("#%d %s attempted to leave, but no target place was found? setting the mech at 0,0 at %d.",
				   mech->mynum, GetMechID(mech), mech->mapindex));
		mech_notify(mech, MECHALL,
					"Weird bug happened during leave. Please contact a wizard. ");
		return 1;
	}

	StopBSuitSwarmers(FindObjectsData(mech->mapindex), mech, 1);
	mech_printf(mech, MECHALL, "You have left %s.", structure_name(mapo));
	mech_Rsetxy(GOD, (void *) mech, tprintf("%d %d", mapo->x, mapo->y));
	ContinueFlying(mech);
	if(car)
		MirrorPosition(mech, car, 0);
	MechLOSBroadcast(mech, tprintf("has left %s at %d,%d.",
								   structure_name(mapo), MechX(mech),
								   MechY(mech)));
	loud_teleport(mech->mynum, mech->mapindex);
	if(car)
		loud_teleport(car->mynum, mech->mapindex);
	if(In_Character(mech->mynum) && Location(MechPilot(mech)) != mech->mynum) {
		mech_notify(mech, MECHALL,
					"%ch%cr%cf%ciINTRUDER ALERT! INTRUDER ALERT!%c");
		mech_notify(mech, MECHALL,
					"%ch%cr%cfAutomatic self-destruct sequence initiated...%c");
		mech_shutdown(GOD, (void *) mech, "");
	}
	auto_cal_mapindex(mech);
	if(MechSpeed(mech) > MMaxSpeed(mech))
		MechSpeed(mech) = MMaxSpeed(mech);
	return 1;
}

void CheckEdgeOfMap(MECH * mech)
{
	int pinned = 0;
	int linked;
	MAP *map;

	map = getMap(mech->mapindex);

	if(!map) {
		mech_notify(mech, MECHPILOT,
					"You are on an invalid map! Map index reset!");
		mech_shutdown(MechPilot(mech), (void *) mech, "");
		SendError(tprintf("CheckEdgeofMap:invalid map:Mech: %d  Index: %d",
						  mech->mynum, mech->mapindex));
		mech->mapindex = -1;
		return;
	}
	linked = map_linked(mech->mapindex);
	/* Prevents you from going off the map */
	/* Eventually this could wrap and all that.. */
	if(MechX(mech) < 0) {
		if(linked) {
			MechX(mech) += map->map_width;
			pinned = -1;
		} else {
			MechX(mech) = 0;
			pinned = 4;
		}
	} else if(MechX(mech) >= map->map_width) {
		if(linked) {
			MechX(mech) -= map->map_width;
			pinned = -1;
		} else {
			MechX(mech) = map->map_width - 1;
			pinned = 2;
		}
	}
	if(MechY(mech) < 0) {
		if(linked) {
			pinned = -1;
			MechY(mech) += map->map_height;
		} else {
			MechY(mech) = 0;
			pinned = 1;
		}
	} else if(MechY(mech) >= map->map_height) {
		if(linked) {
			pinned = -1;
			MechY(mech) -= map->map_height;
		} else {
			MechY(mech) = map->map_height - 1;
			pinned = 3;
		}
	}
	if(pinned > 0) {
		/* This is a DS bay. First, we need to check if the bay's doors are
		   blocked, one way or another.
		 */
		if(map->onmap && IsMech(map->onmap)) {
			if(Leave_DS(map, mech))
				return;
		} else if(map->flags & MAPFLAG_MAPO && map->mapobj[TYPE_LEAVE])
			if(Leave_Hangar(map, mech))
				return;
	}
	if(pinned) {
		MapCoordToRealCoord(MechX(mech), MechY(mech), &MechFX(mech),
							&MechFY(mech));
		if(pinned > 0) {
			mech_notify(mech, MECHALL, "You cannot move off this map!");
			if(Jumping(mech) && !is_aero(mech))
				LandMech(mech);
			MechCocoon(mech) = 0;
			MechSpeed(mech) = 0.0;
			MechDesiredSpeed(mech) = 0.0;
			if(is_aero(mech)) {
				MechStartFX(mech) = 0.0;
				MechStartFY(mech) = 0.0;
				MechStartFZ(mech) = 0.0;
				if(!Landed(mech))
					MaybeMove(mech);
			}
		}
	}
}
int FindZBearing(float x0, float y0, float z0, float x1, float y1, float z1)
{
	float adj, opp, deg;

	adj = FindXYRange(x0, y0, x1, y1);
	/*
	 * XXX: Why can't opp be negative?  If z1 < z0, shouldn't Z-bearing
	 * also be negative?  Also, why no range clamping on the value of deg?
	 */
	opp = (float)(1./SCALEMAP) * fabsf(z1 - z0);
	/* TODO: Use atan2f(), if we've got it.  */
	deg = RAD2DEG(atan2(opp, adj));
	return ceilf(deg);
}

int FindBearing(float x0, float y0, float x1, float y1)
{
	const float dx = x1 - x0;
	const float dy = y1 - y0;

	float rads;
	int degrees;

	/*
	 * atan2() doesn't need this check because we never actually divide by
	 * dx, but we handle it specially for consistency with existing code.
	 */
	if (dx == 0.f) {
		return (dy < 0.f) ? 0 : 180;
	}

	/* TODO: Use atan2f(), if we've got it.  */
	rads = (float)atan2(-dx, dy);

	/* Round off degrees.  */
	degrees = ((int)RAD2DEG(10.f * rads) + 5) / 10;

	return AcceptableDegree(degrees + 180);
}

int InWeaponArc(MECH * mech, float x, float y)
{
	int relat;
	int bearingToTarget;
	int res = NOARC;

	bearingToTarget = FindBearing(MechFX(mech), MechFY(mech), x, y);
	relat = MechFacing(mech) - bearingToTarget;
	if(MechType(mech) == CLASS_MECH || MechType(mech) == CLASS_MW ||
	   MechType(mech) == CLASS_BSUIT) {
		if(MechStatus(mech) & TORSO_RIGHT)
			relat += 59;
		else if(MechStatus(mech) & TORSO_LEFT)
			relat -= 59;
	}
	relat = AcceptableDegree(relat);
	if(relat >= 300 || relat <= 60)
		res |= FORWARDARC;
	if(relat > 120 && relat < 240)
		res |= REARARC;
	if(relat >= 240 && relat < 300)
		res |= RSIDEARC;
	if(relat > 60 && relat <= 120)
		res |= LSIDEARC;

	if(MechHasTurret(mech)) {
		relat = AcceptableDegree((MechFacing(mech) + MechTurretFacing(mech))
								 - bearingToTarget);
		if(relat >= 330 || relat <= 30)
			res |= TURRETARC;
	}
	if(res == NOARC)
		SendError(tprintf("NoArc: #%d: BearingToTarget:%d Facing:%d",
						  mech->mynum, bearingToTarget, MechFacing(mech)));
	return res;
}

char *FindGunnerySkillName(MECH * mech, int weapindx)
{
#ifndef BT_EXILE_MW3STATS
	switch (MechType(mech)) {
	case CLASS_BSUIT:
		return "Gunnery-BSuit";
	case CLASS_MECH:
		return "Gunnery-Battlemech";
	case CLASS_VEH_GROUND:
	case CLASS_VEH_NAVAL:
		return "Gunnery-Conventional";
	case CLASS_VTOL:
	case CLASS_AERO:
		return "Gunnery-Aerospace";
	case CLASS_SPHEROID_DS:
	case CLASS_DS:
		return "Gunnery-Spacecraft";
	case CLASS_MW:
		if(weapindx >= 0) {
			if(!strcmp(MechWeapons[weapindx].name, "PC.Sword"))
				return "Blade";
			if(!strcmp(MechWeapons[weapindx].name, "PC.Vibroblade"))
				return "Blade";
		}
		return "Small_Arms";
	}
#else
	if(weapindx < 0)
		return NULL;
	if(MechType(mech) == CLASS_MW) {
		if(weapindx >= 0) {
			if(!strcmp(MechWeapons[weapindx].name, "PC.Blade"))
				return "Blade";
			if(!strcmp(MechWeapons[weapindx].name, "PC.Vibroblade"))
				return "Blade";
			if(!strcmp(MechWeapons[weapindx].name, "PC.Blazer"))
				return "Support_Weapons";
			if(!strcmp(MechWeapons[weapindx].name, "PC.HeavyGyrojetGun"))
				return "Support_Weapons";
			return "Small_Arms";
		}
	} else if(IsArtillery(weapindx))
		return "Gunnery-Artillery";
	else if(IsMissile(weapindx))
		return "Gunnery-Missile";
	else if(IsBallistic(weapindx))
		return "Gunnery-Ballistic";
	else if(IsEnergy(weapindx))
		return "Gunnery-Laser";
	else if(IsFlamer(weapindx))
		return "Gunnery-Flamer";
#endif
	return NULL;
}

char *FindPilotingSkillName(MECH * mech)
{
#ifndef BT_EXILE_MW3STATS
	switch (MechType(mech)) {
	case CLASS_MW:
		return "Running";
	case CLASS_BSUIT:
		return "Piloting-BSuit";
	case CLASS_MECH:
		return "Piloting-Battlemech";
	case CLASS_VEH_GROUND:
	case CLASS_VEH_NAVAL:
		return "Drive";
	case CLASS_VTOL:
	case CLASS_AERO:
		return "Piloting-Aerospace";
	case CLASS_SPHEROID_DS:
	case CLASS_DS:
		return "Piloting-Spacecraft";
	}
#else
	if(MechType(mech) == CLASS_MW && MechRTerrain(mech) == WATER)
		return "Swimming";
	switch (MechType(mech)) {
	case CLASS_MW:
		return "Running";
	case CLASS_BSUIT:
		return "Piloting-Bsuit";
	case CLASS_VEH_NAVAL:
		return "Piloting-Naval";
	case CLASS_DS:
	case CLASS_SPHEROID_DS:
		return "Piloting-Spacecraft";
	case CLASS_VTOL:
	case CLASS_AERO:
		return "Piloting-Aerospace";
	}
	switch (MechMove(mech)) {
	case MOVE_BIPED:
		return "Piloting-Biped";
	case MOVE_QUAD:
		return "Piloting-Quad";
	case MOVE_TRACK:
		return "Piloting-Tracked";
	case MOVE_HOVER:
		return "Piloting-Hover";
	case MOVE_WHEEL:
		return "Piloting-Wheeled";
	}
#endif
	return NULL;
}

#define MECHSKILL_PILOTING  0
#define MECHSKILL_GUNNERY   1
#define MECHSKILL_SPOTTING  2
#define MECHSKILL_ARTILLERY 3
#define NUM_MECHSKILLS      4

// TODO: Replace this with a function.
#define GENERIC_FIND_MECHSKILL(num,n) \
    if (Quiet(mech->mynum)) \
        { str = silly_atr_get(mech->mynum, A_MECHSKILLS); \
    if (*str) if (sscanf (str, "%d %d %d %d", &i[0], &i[1], &i[2], &i[3]) > num) \
        return i[num] - n; }

int FindPilotPiloting(MECH * mech)
{
	char *str;
	int i[NUM_MECHSKILLS];

	GENERIC_FIND_MECHSKILL(MECHSKILL_PILOTING, 0);
	if(RGotPilot(mech))
		if((str = FindPilotingSkillName(mech)))
			return char_getskilltarget(MechPilot(mech), str, 0);
	return DEFAULT_PILOTING;
}

int FindSPilotPiloting(MECH * mech)
{
	return FindPilotPiloting(mech) + (MechMove(mech) == MOVE_QUAD ? -2 : 0);
}

int FindPilotSpotting(MECH * mech)
{
	char *str;
	int i[NUM_MECHSKILLS];

	GENERIC_FIND_MECHSKILL(MECHSKILL_SPOTTING, 0);
	if(RGotPilot(mech))
		return (char_getskilltarget(MechPilot(mech), "Gunnery-Spotting", 0));
	return DEFAULT_SPOTTING;
}

int FindPilotArtyGun(MECH * mech)
{
	char *str;
	int i[NUM_MECHSKILLS];

	GENERIC_FIND_MECHSKILL(MECHSKILL_ARTILLERY, 0);
	if(RGotGPilot(mech))
		return (char_getskilltarget(GunPilot(mech), "Gunnery-Artillery", 0));
	return DEFAULT_ARTILLERY;
}

int FindPilotGunnery(MECH * mech, int weapindx)
{
	char *str;
	int i[NUM_MECHSKILLS];

	GENERIC_FIND_MECHSKILL(MECHSKILL_GUNNERY, 0);
	if(RGotGPilot(mech))
		if((str = FindGunnerySkillName(mech, weapindx)))
			return char_getskilltarget(GunPilot(mech), str, 0);
	return DEFAULT_GUNNERY;
}

char *FindTechSkillName(MECH * mech)
{
	switch (MechType(mech)) {
	case CLASS_MECH:
	case CLASS_BSUIT:
		return "Technician-Battlemech";
	case CLASS_VEH_GROUND:
	case CLASS_VEH_NAVAL:
		return "Technician-Mechanic";
	case CLASS_AERO:
	case CLASS_VTOL:
	case CLASS_SPHEROID_DS:
	case CLASS_DS:
		return "Technician-Aerospace";
#if 0							/* Used to be DS tech */
		return (char_getskilltarget(player, "Technician-Spacecraft", 0));
#endif
	}
	return NULL;
}

int FindTechSkill(dbref player, MECH * mech)
{
	char *skname;

	if((skname = FindTechSkillName(mech)))
		return (char_getskilltarget(player, skname, 0));
	return 18;
}

int MadePilotSkillRoll(MECH * mech, int mods)
{
	return MadePilotSkillRoll_Advanced(mech, mods, 1);
}

int MechPilotSkillRoll_BTH(MECH * mech, int mods)
{
	mods += FindSPilotPiloting(mech) + MechPilotSkillBase(mech);
	if(In_Character(mech->mynum) && Location(MechPilot(mech)) != mech->mynum)
		mods += 5;
	return mods;
}

int MadePilotSkillRoll_NoXP(MECH * mech, int mods, int succeedWhenFallen)
{
	int roll, roll_needed;

	if(Fallen(mech) && succeedWhenFallen)
		return 1;
	if(Uncon(mech) || !Started(mech) || Blinded(mech))
		return 0;
	roll = Roll();
	roll_needed = MechPilotSkillRoll_BTH(mech, mods);

	SendDebug(tprintf("Attempting to make pilot skill roll. "
					  "SPilot: %d, mods: %d, MechPilot: %d, BTH: %d",
					  FindSPilotPiloting(mech), mods,
					  MechPilotSkillBase(mech), roll_needed));

	mech_notify(mech, MECHPILOT, "You make a piloting skill roll!");
	mech_printf(mech, MECHPILOT,
				"Modified Pilot Skill: BTH %d\tRoll: %d", roll_needed, roll);
	if(roll >= roll_needed) {
		return 1;
	}
	return 0;
}

int MadePilotSkillRoll_Advanced(MECH * mech, int mods, int succeedWhenFallen)
{
	int roll, roll_needed;

	if(Fallen(mech) && succeedWhenFallen)
		return 1;
	if(Uncon(mech) || !Started(mech) || Blinded(mech))
		return 0;
	roll = Roll();
	roll_needed = MechPilotSkillRoll_BTH(mech, mods);

	SendDebug(tprintf("Attempting to make pilot (noxp) skill roll. "
					  "SPilot: %d, mods: %d, MechPilot: %d, BTH: %d",
					  FindSPilotPiloting(mech), mods,
					  MechPilotSkillBase(mech), roll_needed));

	mech_notify(mech, MECHPILOT, "You make a piloting skill roll!");
	mech_printf(mech, MECHPILOT,
				"Modified Pilot Skill: BTH %d\tRoll: %d", roll_needed, roll);
	if(roll >= roll_needed) {
		if(roll_needed > 2)
			AccumulatePilXP(MechPilot(mech), mech, BOUNDED(1,
														   roll_needed - 7,
														   MAX(2, 1 + mods)),
							1);
		return 1;
	}
	return 0;
}

void FindXY(float x0, float y0, int bearing, float range, float *x1, float *y1)
{
	float xscale, correction;

	/* XXX: Something to do with ranges with actual number of hexes? */
	correction = (float) (bearing % 60) / 60.0;
	if(correction > 0.5)
		correction = 1.0 - correction;
	correction = -correction * 2.0;	/* 0 - 1 correction */
	xscale = (1.0 + XSCALE * correction) * SCALEMAP;

	/* TODO: Use sinf()/cosf(), if we've got them.  */
	*x1 = x0 + range * (float)sin(DEG2RAD(bearing)) * xscale;
	*y1 = y0 - range * (float)cos(DEG2RAD(bearing)) * SCALEMAP;
}

/* Computes hex range between Cartesian (x0, y0, z0) and (x1, y1, z1).  */
float FindRange(float x0, float y0, float z0, float x1, float y1, float z1)
{
	const float dx = x0 - x1;
	const float dy = y0 - y1;
	const float dz = z0 - z1;

	/* TODO: Use sqrtf(), if we've got it.  */
	return (float)(1./SCALEMAP) * (float)sqrt(dx * dx + dy * dy + dz * dz);
}

/* Computes hex range between Cartesian (x0, y0) and (x1, y1).  */
float FindXYRange(float x0, float y0, float x1, float y1)
{
	const float dx = x0 - x1;
	const float dy = y0 - y1;

	/* TODO: Use sqrtf(), if we've got it.  */
	return (float)(1./SCALEMAP) * (float)sqrt(dx * dx + dy * dy);
}

/* TODO: We could just make this a macro, right? Or substitute it away.  */
float FindHexRange(float x0, float y0, float x1, float y1)
{
	return FindXYRange(x0, y0, x1, y1);
}

/* CONVERSION ROUTINES courtesy Mike :) (Whoever that may be -focus) */

/* Picture blatantly ripped from the MUSH code by Dizzledorf and co. If only
   I had found it _before_ reverse-engineering the code :)
     - Focus, July 2002.
 */

/*
 * Convert floating-point cartesian coordinates into hex coordinates.
 *
 * To do this, split the hex map into a repeatable region, which itself has
 * 4 distinct regions, each part of a different hex. (See picture.) The hex
 * is normalized so that it is 1 unit high, and so is the repeatable region.
 * It works out that the repeatable region is exactly sqrt(3) wide, and can
 * be split up in six portions of each 1/6th sqrt(3), called 'alpha'. 
 * Section I is 2 alpha wide at the top and bottom, and 3 alpha in the
 * middle. Sections II and III are reversed, being 4 alpha at the top and
 * bottom of the region, and 2 alpha in the middle. Section IV is 1 alpha in
 * the middle and 0 at the top and bottom. The whole region encompasses
 * exactly two x-columns and one y-row. All calculations are now done in
 * 'real' scale, to avoid rounding errors (isn't floating point arithmatic
 * fun ?)
 *
 * Alpha also returns in the slope of the hexsides (2*alpha, flipped or rotated
 * as necessary).  ANGLE_ALPHA is alpha (unscaled) for use in angle
 * calculations.
 *
 *       ________________________
 *      |        \              /|
 *      |         \    III     / |
 *      |          \          /  |
 *      |           \________/ IV|
 *      |    I      /        \   |
 *      |          /   II     \  |
 *      |         /            \ |
 *      |________/______________\|
 *
 */

/* Doubles for added accuracy; most calculations are doubles internally
   anyway, so we suffer little to no performance hit. */

#define ROOT3 558.58638544096289	/* sqrt(3) * SCALEMAP */
#define ALPHA 93.097730906827152	/* ROOT3 / 6 */
#define ANGLE_ALPHA 0.28867513459481287	/* sqrt(3) / 6 */
#define FULL_Y (1 * SCALEMAP)
#define HALF_Y (0.5 * FULL_Y)

void RealCoordToMapCoord(short *hex_x, short *hex_y, float cart_x,
						 float cart_y)
{
	float x, y;
	int x_count, y_count;

	if(cart_x < ALPHA) {
		/* Special case: we are in section IV of x-column 0 or off the map */
		*hex_x = cart_x < 0 ? -1 : 0;
		*hex_y = floor(cart_y / SCALEMAP);
		return;
	}

	/* 'shift' the map to the left so the repeatable box starts at 0 */
	cart_x -= ALPHA;

	/* Figure out the x-coordinate of the 'repeatable box' we're in. */
	x_count = cart_x / ROOT3;
	/* And the offset inside the box, from the left edge. */
	x = cart_x - x_count * ROOT3;

	/* The repbox holds two x-columns, we want the real X coordinate. */
	x_count *= 2;

	/* Do the same for the y-coordinate; this is easy */
	y_count = floor(cart_y / FULL_Y);
	y = cart_y - y_count * FULL_Y;

	if(x < 2 * ALPHA) {

		/* Clean in area I. Nothing to do */

	} else if(x >= 3 * ALPHA && x < 5 * ALPHA) {
		/* Clean in either area II or III. Up x one, and y if in the lower
		   half of the box. */
		x_count++;
		if(y >= HALF_Y)
			/* Area II */
			y_count++;

	} else if(x >= 2 * ALPHA && x < 3 * ALPHA) {
		/* Any of areas I, II and III. */
		if(y >= HALF_Y) {
			/* Area I or II */
			if(2 * ANGLE_ALPHA * (FULL_Y - y) <= x - 2 * ALPHA) {
				/* Area II, up both */
				x_count++;
				y_count++;
			}
		} else {
			/* Area I or III */
			if(2 * ANGLE_ALPHA * y <= x - 2 * ALPHA)
				/* Area III, up only x */
				x_count++;
		}
	} else if(y >= HALF_Y) {
		/* Area II or IV. Up x at least one, maybe two, and y maybe one. */
		x_count++;
		if(2 * ANGLE_ALPHA * (y - HALF_Y) > (x - 5.0 * ALPHA))
			/* Area II */
			y_count++;
		else
			/* Area IV */
			x_count++;
	} else {
		/* Area III or IV, up x at least one, maybe two */
		x_count++;
		if(2 * ANGLE_ALPHA * y > ROOT3 - x)
			/* Area IV */
			x_count++;
	}

	*hex_x = x_count;
	*hex_y = y_count;
}

/*
 * Convert hex coordinates into centered floating-point cartesian coordinates.
 *
 * Properties of hex centers:
 * 1) Spaced 3 ALPHA apart horizontally, starting from 2 ALPHA.
 * 2) Spaced FULL_Y apart vertically.
 * 3a) Even column centers (counting from 0) are vertically offset HALF_Y.
 * 3b) Odd column centers (counting from 0) are not vertically offset.
 */
void MapCoordToRealCoord(int hex_x, int hex_y, float *cart_x, float *cart_y)
{
	/* TODO: Can use some integer math if we're careful about overflow.  */
	/* Use % 2 for theoretical portability to non-2's-complement archs.  */
	*cart_x = (2.f + 3.f * (float)hex_x) * ALPHA;
	*cart_y = ((hex_x % 2) ? 0 : HALF_Y) + ((float)hex_y * FULL_Y);
}

/*
   Sketch a 'mech on a Navigate map. Done here since it fiddles directly
   with cartesian coords.

   Navigate is 9 rows high, and a hex is exactly 1*SCALEMAP high, so each
   row is FULL_Y/9 cartesian y-coords high.
   
   Navigate is 21 hexes wide, at its widest point. This corresponds to the
   hex width, which is 4 * ALPHA, so each column is 4*ALPHA/21 cartesian
   x-coords wide.

   The actual navigate map starts two rows from the top and four columns
   from the left.

 */

#define NAV_ROW_HEIGHT (FULL_Y / 9.0)
#define NAV_COLUMN_WIDTH (4 * ALPHA / 21.0)
#define NAV_Y_OFFSET 2
#define NAV_X_OFFSET 4
#define NAV_MAX_HEIGHT 2+9+2
#define NAV_MAX_WIDTH 4+21+2

void navigate_sketch_mechs(MECH * mech, MAP * map, int x, int y,
						   char buff[NAVIGATE_LINES][MBUF_SIZE])
{
	float corner_fx, corner_fy, fx, fy;
	int i, row, column;
	MECH *other;

	MapCoordToRealCoord(x, y, &corner_fx, &corner_fy);
	corner_fx -= 2 * ALPHA;
	corner_fy -= HALF_Y;

	for(i = 0; i < map->first_free; i++) {
		if(map->mechsOnMap[i] < 0)
			continue;
		if(!(other = FindObjectsData(map->mechsOnMap[i])))
			continue;
		if(other == mech)
			continue;
		if(MechX(other) != x || MechY(other) != y)
			continue;
		if(!InLineOfSight(mech, other, x, y, 0.5))
			continue;

		fx = MechFX(other) - corner_fx;
		column = fx / NAV_COLUMN_WIDTH + NAV_X_OFFSET;

		fy = MechFY(other) - corner_fy;
		row = fy / NAV_ROW_HEIGHT + NAV_Y_OFFSET;

		if(column < 0 || column > NAV_MAX_WIDTH ||
		   row < 0 || row > NAV_MAX_HEIGHT)
			continue;

		buff[row][column] = MechSeemsFriend(mech, other) ? 'x' : 'X';
	}

	/* Draw 'mech last so we always see it. */

	fx = MechFX(mech) - corner_fx;
	column = fx / NAV_COLUMN_WIDTH + NAV_X_OFFSET;

	fy = MechFY(mech) - corner_fy;
	row = fy / NAV_ROW_HEIGHT + NAV_Y_OFFSET;

	if(column < 0 || column > NAV_MAX_WIDTH ||
	   row < 0 || row > NAV_MAX_HEIGHT)
		return;

	buff[row][column] = '*';
}

int FindTargetXY(MECH * mech, float *x, float *y, float *z)
{
	MECH *tempMech;

	if(MechTarget(mech) != -1) {
		tempMech = getMech(MechTarget(mech));
		if(tempMech) {
			*x = MechFX(tempMech);
			*y = MechFY(tempMech);
			*z = MechFZ(tempMech);
			return 1;
		}
	} else if(MechTargX(mech) != -1 && MechTargY(mech) != -1) {
		MapCoordToRealCoord(MechTargX(mech), MechTargY(mech), x, y);
		*z = (float) ZSCALE *(MechTargZ(mech));

		return 1;
	}
	return 0;
}

int global_silence = 0;

#define UGLYTEST \
	  if (num_crits) \
	    { \
	      if (num_crits != (i = GetWeaponCrits (mech, lastweap))) \
		{ \
		  if (whine && !global_silence) \
		    SendError (tprintf ("Error in the numcriticals for weapon on #%d! (Should be: %d, is: %d)", mech->mynum, i, num_crits)); \
		  return -1; \
		} \
	      num_crits = 0; \
	    }

/* ASSERTION: Weapons must be located next to each other in criticals */

/* This is a hacked function. Sorry. */

int FindWeapons_Advanced(MECH * mech, int index, unsigned char *weaparray,
						 unsigned char *weapdataarray, int *critical,
						 int whine)
{
	int loop;
	int weapcount = 0;
	int temp, data, lastweap = -1;
	int num_crits = 0, i;

	for(loop = 0; loop < MAX_WEAPS_SECTION; loop++) {
		temp = GetPartType(mech, index, loop);
		data = GetPartData(mech, index, loop);
		if(IsWeapon(temp)) {
			temp = Weapon2I(temp);
			if(weapcount == 0) {
				lastweap = temp;
				weapdataarray[weapcount] = data;
				weaparray[weapcount] = temp;
				critical[weapcount] = loop;
				weapcount++;
				num_crits = 1;
				continue;
			}
			if(!num_crits || temp != lastweap ||
			   (num_crits == GetWeaponCrits(mech, temp))) {
				UGLYTEST;
				weaparray[weapcount] = temp;
				weapdataarray[weapcount] = data;
				critical[weapcount] = loop;
				lastweap = temp;
				num_crits = 1;
				weapcount++;
			} else
				num_crits++;
		} else
			UGLYTEST;
	}
	UGLYTEST;
	return (weapcount);
}

int FindAmmunition(MECH * mech, unsigned char *weaparray,
				   unsigned short *ammoarray, unsigned short *ammomaxarray,
				   unsigned int *modearray, int returnall)
{
	int loop;
	int weapcount = 0;
	int temp, data, mode;
	int index, i, j, duplicate;

	for(index = 0; index < NUM_SECTIONS; index++)
		for(loop = 0; loop < MAX_WEAPS_SECTION; loop++) {
			temp = GetPartType(mech, index, loop);
			if(IsAmmo(temp)) {
				data = GetPartData(mech, index, loop);
				mode = (GetPartAmmoMode(mech, index, loop) & AMMO_MODES);
				temp = Ammo2Weapon(temp);
				duplicate = 0;

				for(i = 0; i < weapcount; i++) {
					if(temp == weaparray[i] && mode == modearray[i]) {
						if(!(PartIsNonfunctional(mech, index, loop)))
							ammoarray[i] += data;
						ammomaxarray[i] += FullAmmo(mech, index, loop);
						duplicate = 1;
					}
				}

				if(!duplicate) {
					weaparray[weapcount] = temp;

					if(!(PartIsNonfunctional(mech, index, loop)))
						ammoarray[weapcount] = data;
					else
						ammoarray[weapcount] = 0;

					ammomaxarray[weapcount] = FullAmmo(mech, index, loop);
					modearray[weapcount] = mode;

					weapcount++;
				}
			}
		}
	/* Then, prune entries with 0 ammo left */
	if (!returnall) {
	for(i = 0; i < weapcount; i++)
		if(!ammoarray[i]) {
			for(j = i + 1; j < weapcount; j++) {
				weaparray[j - 1] = weaparray[j];
				ammoarray[j - 1] = ammoarray[j];
				ammomaxarray[j - 1] = ammomaxarray[j];
				modearray[j - 1] = modearray[j];
			}
			i--;
			weapcount--;
		} 
	}
	return (weapcount);
}

int FindLegHeatSinks(MECH * mech)
{
	int loop;
	int heatsinks = 0;

	for(loop = 0; loop < NUM_CRITICALS; loop++) {
		if(GetPartType(mech, LLEG, loop) == I2Special((HEAT_SINK)) &&
		   !PartIsNonfunctional(mech, LLEG, loop))
			heatsinks++;
		if(GetPartType(mech, RLEG, loop) == I2Special((HEAT_SINK)) &&
		   !PartIsNonfunctional(mech, RLEG, loop))
			heatsinks++;
		/*
		 * Added by Kipsta on 8/5/99
		 * Quads can get 'arm' HS in the water too
		 */

		if(MechIsQuad(mech)) {
			if(GetPartType(mech, LARM, loop) == I2Special((HEAT_SINK)) &&
			   !PartIsNonfunctional(mech, LARM, loop))
				heatsinks++;
			if(GetPartType(mech, RARM, loop) == I2Special((HEAT_SINK)) &&
			   !PartIsNonfunctional(mech, RARM, loop))
				heatsinks++;
		}
	}
	return (heatsinks);
}

/* Added for tic support. */

/* returns the weapon index- -1 for not found, -2 for destroyed, -3, -4 */

/* for reloading/recycling */
int FindWeaponNumberOnMech_Advanced(MECH * mech, int number, int *section,
									int *crit, int sight)
{
	int loop;
	unsigned char weaparray[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	int critical[MAX_WEAPS_SECTION];
	int running_sum = 0;
	int num_weaps;
	int index;

	for(loop = 0; loop < NUM_SECTIONS; loop++) {
		num_weaps = FindWeapons(mech, loop, weaparray, weapdata, critical);

		if(num_weaps <= 0)
			continue;

		if(number < running_sum + num_weaps) {
			/* we found it... */
			index = number - running_sum;
			if(PartIsNonfunctional(mech, loop, critical[index])) {
				*section = loop;
				*crit = critical[index];
				return TIC_NUM_DESTROYED;
			} else if(weapdata[index] > 0 && !sight) {
				*section = loop;
				*crit = critical[index];
				return (MechWeapons[weaparray[index]].type ==
						TBEAM) ? TIC_NUM_RECYCLING : TIC_NUM_RELOADING;
			} else {

				if(MechSections(mech)[loop].recycle &&
				   (MechType(mech) == CLASS_MECH ||
					MechType(mech) == CLASS_VEH_GROUND ||
					MechType(mech) == CLASS_VTOL) && !sight) {

					*section = loop;
					*crit = critical[index];
					/* just did a physical attack */
					return TIC_NUM_PHYSICAL;
				}

				/* The recylce data for the weapon is clear- it is ready to fire! */
				*section = loop;
				*crit = critical[index];
				return weaparray[index];
			}
		} else
			running_sum += num_weaps;
	}
	return -1;
}

int FindWeaponNumberOnMech(MECH * mech, int number, int *section, int *crit)
{
	return FindWeaponNumberOnMech_Advanced(mech, number, section, crit, 0);
}

int FindWeaponFromIndex(MECH * mech, int weapindx, int *section, int *crit)
{
	int loop;
	unsigned char weaparray[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	int critical[MAX_WEAPS_SECTION];
	int num_weaps;
	int index;

	for(loop = 0; loop < NUM_SECTIONS; loop++) {
		num_weaps = FindWeapons(mech, loop, weaparray, weapdata, critical);
		for(index = 0; index < num_weaps; index++)
			if(weaparray[index] == weapindx) {
				*section = loop;
				*crit = critical[index];
				if(!PartIsNonfunctional(mech, loop, index) &&
				   !WpnIsRecycling(mech, loop, index))
					return 1;
				/* Return if not Recycling/Destroyed */
				/* Otherwise keep looking */
			}
	}
	return 0;
}

int FindWeaponIndex(MECH * mech, int number)
{
	int loop;
	unsigned char weaparray[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	int critical[MAX_WEAPS_SECTION];
	int running_sum = 0;
	int num_weaps;
	int index;

	if(number < 0)
		return -1;				/* Anti-crash */
	for(loop = 0; loop < NUM_SECTIONS; loop++) {
		num_weaps = FindWeapons(mech, loop, weaparray, weapdata, critical);
		if(num_weaps <= 0)
			continue;
		if(number < running_sum + num_weaps) {
			/* we found it... */
			index = number - running_sum;
			return weaparray[index];
		}
		running_sum += num_weaps;
	}
	return -1;
}


int FullAmmo(MECH * mech, int loc, int pos)
{
	int baseammo;

	baseammo = MechWeapons[Ammo2I(GetPartType(mech,loc,pos))].ammoperton;
	if((GetPartAmmoMode(mech, loc, pos) & AC_AP_MODE) 
			|| (GetPartAmmoMode(mech, loc, pos) & AC_PRECISION_MODE)
			|| (GetPartFireMode(mech, loc, pos) & HALFTON_MODE)) {
		return baseammo >> 1;
	}
	
	if((GetPartAmmoMode(mech, loc, pos) & AC_CASELESS_MODE)) {
		return baseammo << 1;
	}
	
	return baseammo;
}

int findAmmoInSection(MECH * mech, int section, int type, int nogof, int gof)
{
	int wIter;

	/* Can't use LBX ammo as normal, but can use Narc and Artemis as normal */
	for(wIter = 0; wIter < NUM_CRITICALS; wIter++) {
		if(GetPartType(mech, section, wIter) == type &&
		   !PartIsNonfunctional(mech, section, wIter) && (!nogof ||
														  !(GetPartAmmoMode
															(mech, section,
															 wIter) & nogof))
		   && (!gof || (GetPartAmmoMode(mech, section, wIter) & gof))) {

			if(!PartIsNonfunctional(mech, section, wIter) &&
			   GetPartData(mech, section, wIter) > 0)
				return wIter;
		}
	}

	return -1;
}

int FindAmmoForWeapon_sub(MECH * mech, int weapSection, int weapCritical,
						  int weapindx, int start, int *section,
						  int *critical, int nogof, int gof)
{
	int loop;
	int foundSlot;
	int desired;
	int wCritType = 0;
	int wWeapSize = 0;
	int wFirstCrit = 0;
	int wDesiredLoc = -1;

	desired = I2Ammo(weapindx);

	/* The data on the desired location */
	if((weapSection > -1) && (weapCritical > -1)) {
		wCritType = GetPartType(mech, weapSection, weapCritical);
		wWeapSize = GetWeaponCrits(mech, Weapon2I(wCritType));
		wFirstCrit =
			FindFirstWeaponCrit(mech, weapSection, weapCritical, 0,
								wCritType, wWeapSize);

		wDesiredLoc = GetPartDesiredAmmoLoc(mech, weapSection, wFirstCrit);

		if(wDesiredLoc >= 0) {
			foundSlot =
				findAmmoInSection(mech, wDesiredLoc, desired, nogof, gof);

			if(foundSlot >= 0) {
				*section = wDesiredLoc;
				*critical = foundSlot;

				return 1;
			}
		}
	}

	/* Now lets search the current section */
	foundSlot = findAmmoInSection(mech, start, desired, nogof, gof);

	if(foundSlot >= 0) {
		*section = start;
		*critical = foundSlot;

		return 1;
	}

	/* If all else fails, start hunting for ammo */
	for(loop = 0; loop < NUM_SECTIONS; loop++) {
		if((loop == start) || (loop == wDesiredLoc))
			continue;

		foundSlot = findAmmoInSection(mech, loop, desired, nogof, gof);

		if(foundSlot >= 0) {
			*section = loop;
			*critical = foundSlot;

			return 1;
		}
	}

	return 0;
}

int FindAmmoForWeapon(MECH * mech, int weapindx, int start, int *section,
					  int *critical)
{
	return FindAmmoForWeapon_sub(mech, -1, -1, weapindx, start, section,
								 critical, AMMO_MODES, 0);
}

int CountAmmoForWeapon(MECH * mech, int weapindx)
{
	int wSecIter;
	int wSlotIter;
	int wcAmmo = 0;
	int wAmmoIdx;

	wAmmoIdx = I2Ammo(weapindx);

	for(wSecIter = 0; wSecIter < NUM_SECTIONS; wSecIter++) {
		for(wSlotIter = 0; wSlotIter < NUM_CRITICALS; wSlotIter++) {
			if((GetPartType(mech, wSecIter, wSlotIter) == wAmmoIdx) &&
			   !PartIsNonfunctional(mech, wSecIter, wSlotIter) &&
			   (GetPartData(mech, wSecIter, wSlotIter) > 0))
				wcAmmo += GetPartData(mech, wSecIter, wSlotIter);
		}
	}

	return wcAmmo;
}

/* Function taken from 3065. Credit to RebelST) */
int FindArtemisForWeapon(MECH * mech, int section, int critical)
{
        int critloop;
        int desired;

	
        desired = I2Special(ARTEMIS_IV);
        for(critloop = 0; critloop < NUM_CRITICALS; critloop++) {
                if(GetPartType(mech, section, critloop) == desired && !PartIsNonfunctional(mech, section, critloop)) {
                        if(GetPartData(mech, section, critloop) == (critical+1))
                                return 1;
                }
        }
        if (MechType(mech) == CLASS_MECH && section == CTORSO) { // if it's mech, and torso missile, search in head
                for (critloop = 0; critloop < 6; critloop++) {
                        if (GetPartType(mech, HEAD, critloop) == desired && !PartIsNonfunctional(mech, HEAD, critloop)) {
                                if (GetPartData(mech, HEAD, critloop) == (critical+1))
                                        return 1;
                        }
                }
        } else if (MechType(mech) == CLASS_VEH_GROUND && section == TURRET) { // same thing for turret & aft
                for (critloop = 0; critloop < NUM_CRITICALS; critloop++) {
                        if (GetPartType(mech, BSIDE, critloop) == desired && !PartIsNonfunctional(mech, BSIDE, critloop)) {
                                if (GetPartData(mech, BSIDE, critloop) == (critical+1))
                                        return 1;
                        }
                }
        }
        return 0;
}


int FindDestructiveAmmo(MECH * mech, int *section, int *critical)
{
	int loop;
	int critloop;
	int maxdamage = 0;
	int damage;
	int weapindx;
	int i;
	int type, data;

	for(loop = 0; loop < NUM_SECTIONS; loop++)
		for(critloop = 0; critloop < NUM_CRITICALS; critloop++)
			if(IsAmmo(GetPartType(mech, loop, critloop)) &&
			   !PartIsDestroyed(mech, loop, critloop)) {
				data = GetPartData(mech, loop, critloop);
				type = GetPartType(mech, loop, critloop);
				weapindx = Ammo2WeaponI(type);
				damage = data * MechWeapons[weapindx].damage;
				if(MechWeapons[weapindx].special & GAUSS)
					continue;
				if(IsMissile(weapindx) || IsArtillery(weapindx)) {
					for(i = 0; MissileHitTable[i].key != -1; i++)
						if(MissileHitTable[i].key == weapindx)
							damage *= MissileHitTable[i].num_missiles[10];
				}
				if(damage > maxdamage) {
					*section = loop;
					*critical = critloop;
					maxdamage = damage;
				}
			}
	return (maxdamage);
}

int FindInfernoAmmo(MECH * mech, int *section, int *critical)
{
	int loop;
	int critloop;
	int maxdamage = 0;
	int damage;
	int weapindx;
	int i;
	int type, data;
	int mode;

	for(loop = 0; loop < NUM_SECTIONS; loop++)
		for(critloop = 0; critloop < NUM_CRITICALS; critloop++)
			if(IsAmmo(GetPartType(mech, loop, critloop)) &&
			   !PartIsDestroyed(mech, loop, critloop)) {
				data = GetPartData(mech, loop, critloop);
				type = GetPartType(mech, loop, critloop);
				mode = GetPartAmmoMode(mech, loop, critloop);
				if(!(mode & INFERNO_MODE))
					continue;
				weapindx = Ammo2WeaponI(type);
				damage = data * MechWeapons[weapindx].damage;
				if(MechWeapons[weapindx].special & GAUSS)
					continue;
				if(IsMissile(weapindx) || IsArtillery(weapindx)) {
					for(i = 0; MissileHitTable[i].key != -1; i++)
						if(MissileHitTable[i].key == weapindx)
							damage *= MissileHitTable[i].num_missiles[10];
				}
				if(damage > maxdamage) {
					*section = loop;
					*critical = critloop;
					maxdamage = damage;
				}
			}
	return (maxdamage);
}

int FindRoundsForWeapon(MECH * mech, int weapindx)
{
	int loop;
	int critloop;
	int desired;
	int found = 0;

	desired = I2Ammo(weapindx);
	for(loop = 0; loop < NUM_SECTIONS; loop++)
		for(critloop = 0; critloop < NUM_CRITICALS; critloop++)
			if(GetPartType(mech, loop, critloop) == desired &&
			   !PartIsNonfunctional(mech, loop, critloop))
				found += GetPartData(mech, loop, critloop);
	return found;
}

char *quad_locs[NUM_SECTIONS + 1] = {
	"Front Left Leg",
	"Front Right Leg",
	"Left Torso",
	"Right Torso",
	"Center Torso",
	"Rear Left Leg",
	"Rear Right Leg",
	"Head",
	NULL
};

char *mech_locs[NUM_SECTIONS + 1] = {
	"Left Arm",
	"Right Arm",
	"Left Torso",
	"Right Torso",
	"Center Torso",
	"Left Leg",
	"Right Leg",
	"Head",
	NULL
};

char *bsuit_locs[NUM_BSUIT_MEMBERS + 1] = {
	"Suit 1",
	"Suit 2",
	"Suit 3",
	"Suit 4",
	"Suit 5",
	"Suit 6",
	"Suit 7",
	"Suit 8",
	NULL
};

char *veh_locs[NUM_VEH_SECTIONS + 1] = {
	"Left Side",
	"Right Side",
	"Front Side",
	"Aft Side",
	"Turret",
	"Rotor",
	NULL
};

char *aero_locs[NUM_AERO_SECTIONS + 1] = {
	"Nose",
	"Left Wing",
	"Right Wing",
	"Aft Side",
	NULL
};

char *ds_locs[NUM_DS_SECTIONS + 1] = {
	"Right Wing",
	"Left Wing",
	"Left Rear Wing",
	"Right Rear Wing",
	"Aft",
	"Nose",
	NULL
};

char *ds_spher_locs[NUM_DS_SECTIONS + 1] = {
	"Front Right Side",
	"Front Left Side",
	"Rear Left Side",
	"Rear Right Side",
	"Aft",
	"Nose",
	NULL
};

char **ProperSectionStringFromType(int type, int mtype)
{
	switch (type) {
	case CLASS_BSUIT:
		return bsuit_locs;
	case CLASS_MECH:
	case CLASS_MW:
		if(mtype == MOVE_QUAD)
			return quad_locs;
		return mech_locs;
	case CLASS_VEH_GROUND:
	case CLASS_VEH_NAVAL:
	case CLASS_VTOL:
		return veh_locs;
	case CLASS_AERO:
		return aero_locs;
	case CLASS_SPHEROID_DS:
		return ds_spher_locs;
	case CLASS_DS:
		return ds_locs;
	}
	return NULL;
}

void ArmorStringFromIndex(int index, char *buffer, char type, char mtype)
{
	char **locs = ProperSectionStringFromType(type, mtype);
	int high = 0;

	switch (type) {
	case CLASS_MECH:
	case CLASS_MW:
		high = NUM_SECTIONS;
		break;
	case CLASS_VEH_GROUND:
	case CLASS_VEH_NAVAL:
		high = (NUM_VEH_SECTIONS - 1);
		break;
	case CLASS_VTOL:
		high = NUM_VEH_SECTIONS;
		break;
	case CLASS_AERO:
		high = NUM_AERO_SECTIONS;
		break;
	case CLASS_SPHEROID_DS:
		high = NUM_DS_SECTIONS;
	case CLASS_DS:
		high = NUM_DS_SECTIONS;
		break;
	case CLASS_BSUIT:
		high = NUM_BSUIT_MEMBERS;
		break;
	default:
		strcpy(buffer, "Invalid!!");
		return;
	}
	if(high > 0 && index < high && locs) {
		strcpy(buffer, locs[index]);
		return;
	}
	strcpy(buffer, "Invalid!!");
}

int IsInWeaponArc(MECH * mech, float x, float y, int section, int critical)
{
	int weaponarc, isrear;
	int wantarc = NOARC;

	if(MechType(mech) == CLASS_MECH && (section == LLEG || section == RLEG
										|| (MechIsQuad(mech)
											&& (section == LARM
												|| section == RARM)))) {
		int ts = MechStatus(mech) & (TORSO_LEFT | TORSO_RIGHT);
		MechStatus(mech) &= ~(ts);
		weaponarc = InWeaponArc(mech, x, y);
		MechStatus(mech) |= ts;
	} else
		weaponarc = InWeaponArc(mech, x, y);

	switch (MechType(mech)) {
	case CLASS_MECH:
	case CLASS_BSUIT:
	case CLASS_MW:
		if(GetPartFireMode(mech, section, critical) & REAR_MOUNT)
			wantarc = REARARC;
		else if(section == LARM && (MechStatus(mech) & FLIPPED_ARMS))
			wantarc = REARARC | LSIDEARC;
		else if(section == LARM)
			wantarc = FORWARDARC | LSIDEARC;
		else if(section == RARM && (MechStatus(mech) & FLIPPED_ARMS))
			wantarc = REARARC | RSIDEARC;
		else if(section == RARM)
			wantarc = FORWARDARC | RSIDEARC;
		else
			wantarc = FORWARDARC;
		break;
	case CLASS_VEH_GROUND:
	case CLASS_VEH_NAVAL:
	case CLASS_VTOL:
		switch (section) {
		case TURRET:
			wantarc = TURRETARC;
			break;
		case FSIDE:
			wantarc = FORWARDARC;
			break;
		case LSIDE:
			wantarc = LSIDEARC;
			break;
		case RSIDE:
			wantarc = RSIDEARC;
			break;
		case BSIDE:
			wantarc = REARARC;
			break;
		}
		break;
	case CLASS_DS:
		switch (section) {
		case DS_NOSE:
			wantarc = FORWARDARC;
			break;
		case DS_LWING:
		case DS_LRWING:
			wantarc = LSIDEARC;
			break;
		case DS_RWING:
		case DS_RRWING:
			wantarc = RSIDEARC;
			break;
		case DS_AFT:
			wantarc = REARARC;
			break;
		}
		break;
	case CLASS_SPHEROID_DS:
		switch (section) {
		case DS_NOSE:
			wantarc = FORWARDARC;
			break;
		case DS_LWING:
			wantarc = FORWARDARC | LSIDEARC;
			break;
		case DS_LRWING:
			wantarc = REARARC | LSIDEARC;
			break;
		case DS_RWING:
			wantarc = FORWARDARC | RSIDEARC;
			break;
		case DS_RRWING:
			wantarc = REARARC | RSIDEARC;
			break;
		case DS_AFT:
			wantarc = REARARC;
			break;
		}
		break;

	case CLASS_AERO:
		isrear = (GetPartFireMode(mech, section, critical) & REAR_MOUNT);
		switch (section) {
		case AERO_NOSE:
			wantarc = FORWARDARC | LSIDEARC | RSIDEARC;
			break;
		case AERO_LWING:
			wantarc = LSIDEARC | (isrear ? REARARC : FORWARDARC);
			break;
		case AERO_RWING:
			wantarc = RSIDEARC | (isrear ? REARARC : FORWARDARC);
			break;
		case AERO_AFT:
			wantarc = REARARC;
			break;
		}
		break;
	}
	return wantarc ? (wantarc & weaponarc) : 0;
}

int GetWeaponCrits(MECH * mech, int weapindx)
{
	return (MechType(mech) ==
			CLASS_MECH) ? (MechWeapons[weapindx].criticals) : 1;
}

int listmatch(char **foo, char *mat)
{
	int i;

	for(i = 0; foo[i]; i++)
		if(!strcasecmp(foo[i], mat))
			return i;
	return -1;
}

/* Takes care of :
   JumpSpeed
   Numsinks

   TODO: More support(?)
 */

void do_sub_magic(MECH * mech, int loud)
{
	int jjs = 0;
	int hses = 0;
	int wanths, wanths_f;
	int shs_size = HS_Size(mech);
	int hs_eff = HS_Efficiency(mech);
	int i, j;
	int inthses = MechEngineSize(mech) / 25;
	int dest_hses = 0;
	int maxjjs = (int) ((float) MechMaxSpeed(mech) * MP_PER_KPH * 2 / 3);

	if(MechSpecials(mech) & ICE_TECH)
		inthses = 0;
	for(i = 0; i < NUM_SECTIONS; i++)
		for(j = 0; j < CritsInLoc(mech, i); j++)
			switch (Special2I(GetPartType(mech, i, j))) {
			case HEAT_SINK:
				hses++;
				if(PartIsNonfunctional(mech, i, j))
					dest_hses++;
				break;
			case JUMP_JET:
				jjs++;
				break;
			}
	hses +=
		MIN(MechRealNumsinks(mech) * shs_size / hs_eff, inthses * shs_size);
	if(jjs > maxjjs) {
		if(loud)
			SendError(tprintf
					  ("Error in #%d (%s): %d JJs, yet %d maximum available (due to walk MPs)?",
					   mech->mynum, MechType_Ref(mech), jjs, maxjjs));

		jjs = maxjjs;
	}
	MechJumpSpeed(mech) = MP1 * jjs;
	wanths_f = (hses / shs_size) * hs_eff;
	wanths = wanths_f - (dest_hses * hs_eff / shs_size);
	if(loud)
		MechNumOsinks(mech) =
			wanths - MIN(MechRealNumsinks(mech), inthses * hs_eff);
	if(wanths != MechRealNumsinks(mech) && loud) {
		SendError(tprintf
				  ("Error in #%d (%s): Set HS: %d. Existing HS: %d. Difference: %d. Please %s.",
				   mech->mynum, MechType_Ref(mech), MechRealNumsinks(mech),
				   wanths, MechRealNumsinks(mech) - wanths,
				   wanths <
				   MechRealNumsinks(mech) ? "add the extra HS critical(s)" :
				   "fix the template"));
	} else
		MechRealNumsinks(mech) = wanths;
	MechNumOsinks(mech) = wanths_f;
}

#define CV(fun) fun(mech) = fun(&opp)

/* Values to take care of:
   - JumpSpeed
   - MaxSpeed
   - Numsinks
   - EngineHeat
   - PilotSkillBase
   - LRS/Tac/ScanRange
   - BTH

   Status:
   - Destroyed

   Critstatus:
   - kokonaan paitsi

   section(s) / basetohit
 */

void do_fixextra (MECH * mech)
{

	int i, j;

        for(i = 0; i < NUM_SECTIONS; i++) {
		if(SectIsFlooded(mech, i))
			UnSetSectFlooded(mech, i);
                for(j = 0; j < CritsInLoc(mech, i); j++) {
			if(!IsAmmo(GetPartType(mech, i, j))) {
				if(!PartIsBroken(mech,i,j) && !PartIsDestroyed(mech, i, j))	
					mech_RepairPart(mech, i, j);
				else {
					UnDisablePart(mech, i, j);
					mech_RepairPart(mech, i, j);
				}
			} else {
				UnDisablePart(mech, i, j);
				mech_FillPartAmmo(mech, i, j);
			}
		}
	}
}

void do_magic(MECH * mech)
{
	MECH opp;
	int i, j, t;
	int mask = 0;
	int tankCritMask = 0;

	if(MechType(mech) != CLASS_MECH)
		tankCritMask =
			(TURRET_LOCKED | TURRET_JAMMED | TAIL_ROTOR_DESTROYED |
			 CREW_STUNNED);

	/* stop the burning */
	StopBurning(mech);
	StopPerformingAction(mech);

	memcpy(&opp, mech, sizeof(MECH));
	mech_loadnew(GOD, &opp, MechType_Ref(mech));
	MechEngineSizeV(mech) = MechEngineSizeC(&opp);	/* From intact template */
	opp.mynum = -1;
	/* Ok.. It's at perfect condition. Start inflicting some serious crits.. */
	for(i = 0; i < NUM_SECTIONS; i++)
		for(j = 0; j < CritsInLoc(mech, i); j++) {
			SetPartType(&opp, i, j, GetPartType(mech, i, j));
			SetPartBrand(&opp, i, j, GetPartBrand(mech, i, j));
			SetPartData(&opp, i, j, 0);
			SetPartFireMode(&opp, i, j, 0);
			SetPartAmmoMode(&opp, i, j, 0);
		}
	if(MechType(mech) == CLASS_MECH)
		do_sub_magic(&opp, 0);
	MechNumOsinks(mech) = MechNumOsinks(&opp);
	for(i = 0; i < NUM_SECTIONS; i++) {
		
			for(j = 0; j < CritsInLoc(mech, i); j++) {
				if(PartIsDestroyed(mech, i, j)) {
					if(!PartIsDestroyed(&opp, i, j)) {
						if(!IsAmmo((t = GetPartType(mech, i, j)))) {
							if(!IsWeapon(t))
								if(MechType(mech) == CLASS_MECH) 
								HandleMechCrit(&opp, NULL, 0, i, j, t,
											   GetPartData(mech, i, j));
						}
					}
				} else {
					t = GetPartType(mech, i, j);
					if(IsAMS(Weapon2I(t))) {
						if(MechWeapons[Weapon2I(t)].special & CLAT)
							MechSpecials(mech) |= CL_ANTI_MISSILE_TECH;
						else
							MechSpecials(mech) |= IS_ANTI_MISSILE_TECH;
					}
					GetPartFireMode(mech, i, j) &=
						~(OS_USED | ROCKET_FIRED | IS_JETTISONED_MODE);
				}
			}
		
		
		MechSections(mech)[i].config &= ~STABILIZERS_DESTROYED;

		if(SectIsDestroyed(mech, i))
			DestroySection(&opp, NULL, 0, i);
		if(MechStall(mech) > 0)
			UnSetSectBreached(mech, i);	/* Just in case ; this leads to 'unbreachable' legs once you've 'done your time' once */
	}
	CV(MechJumpSpeed);
	CV(MechMaxSpeed);
	CV(MechRealNumsinks);
	CV(MechEngineHeat);
	CV(MechPilotSkillBase);
	CV(MechLRSRange);
	CV(MechTacRange);
	CV(MechScanRange);
	CV(MechBTH);
	MechCritStatus(mech) &= mask;
	MechCritStatus(mech) |= MechCritStatus(&opp) & (~mask);

	MechTankCritStatus(mech) &= tankCritMask;
	MechTankCritStatus(mech) |= MechTankCritStatus(&opp) & (~tankCritMask);

	for(i = 0; i < NUM_SECTIONS; i++) {
		MechSections(mech)[i].basetohit = MechSections(&opp)[i].basetohit;
		MechSections(mech)[i].specials = MechSections(&opp)[i].specials;
		MechSections(mech)[i].specials &=
			~(INARC_HOMING_ATTACHED | INARC_HAYWIRE_ATTACHED |
			  INARC_ECM_ATTACHED | INARC_NEMESIS_ATTACHED);
	}

	/* Case of undestroying */
	if(!Destroyed(&opp) && Destroyed(mech))
		MechStatus(mech) &= ~DESTROYED;
	else if(Destroyed(&opp) && !Destroyed(mech))
		MechStatus(mech) |= DESTROYED;
	if(!Destroyed(mech) && MechType(mech) != CLASS_MECH)
		EvalBit(MechStatus(mech), FALLEN, Fallen(&opp));
	update_specials(mech);
}

void mech_RepairPart(MECH * mech, int loc, int pos)
{
	int t = GetPartType(mech, loc, pos);

	UnDestroyPart(mech, loc, pos);
	if(IsWeapon(t) || IsAmmo(t)) {
		SetPartData(mech, loc, pos, 0);
		GetPartFireMode(mech, loc, pos) &=
			~(OS_USED | IS_JETTISONED_MODE | ROCKET_FIRED);
	} else if(IsSpecial(t)) {
		switch (Special2I(t)) {
		case TARGETING_COMPUTER:
		case HEAT_SINK:
		case LIFE_SUPPORT:
		case COCKPIT:
		case SENSORS:
		case JUMP_JET:
		case ENGINE:
		case GYRO:
		case SHOULDER_OR_HIP:
		case LOWER_ACTUATOR:
		case UPPER_ACTUATOR:
		case HAND_OR_FOOT_ACTUATOR:
		case C3_MASTER:
		case C3_SLAVE:
		case C3I:
		case ECM:
		case ANGELECM:
		case NULL_SIGNATURE_SYSTEM:
		case BEAGLE_PROBE:
		case ARTEMIS_IV:
		case TAG:
		case BLOODHOUND_PROBE:
			/* Magic stuff here :P */
			if(MechType(mech) == CLASS_MECH)
				do_magic(mech);
			break;
		}
	}
}

int no_locations_destroyed(MECH * mech)
{
	int i;

	for(i = 0; i < NUM_SECTIONS; i++)
		if(GetSectOInt(mech, i) && SectIsDestroyed(mech, i))
			return 0;
	return 1;
}

void mech_ReAttach(MECH * mech, int loc)
{
	if(!SectIsDestroyed(mech, loc))
		return;
	UnSetSectDestroyed(mech, loc);
	UnSetSectFlooded(mech, loc);
	SetSectInt(mech, loc, GetSectOInt(mech, loc));
	if(is_aero(mech))
		SetSectInt(mech, loc, 1);
	if(MechType(mech) != CLASS_MECH) {
		if(no_locations_destroyed(mech) && IsDS(mech))
			MechStatus(mech) &= ~DESTROYED;
		return;
	}
}

void mech_ReplaceSuit(MECH * mech, int loc)
{
	if(!SectIsDestroyed(mech, loc))
		return;

	UnSetSectDestroyed(mech, loc);
	SetSectInt(mech, loc, GetSectOInt(mech, loc));
}

/*
 * Added for new flood code by Kipsta
 * 8/4/99
 */

void mech_ReSeal(MECH * mech, int loc)
{
	int i;

	if(SectIsDestroyed(mech, loc))
		return;
	if(!SectIsFlooded(mech, loc))
		return;

	UnSetSectFlooded(mech, loc);

	for(i = 0; i < CritsInLoc(mech, loc); i++) {
		if(PartIsDisabled(mech, loc, i)) {
			if(!PartIsBroken(mech, loc, i) && !PartIsDamaged(mech, loc, i))
				mech_RepairPart(mech, loc, i);
			else
				UnDisablePart(mech, loc, i);
		}
	}
}

void mech_Detach(MECH * mech, int loc)
{
	if(SectIsDestroyed(mech, loc))
		return;
	DestroySection(mech, NULL, 0, loc);
}

/* Figures out how much ammo there is when we're 'fully loaded', and
   fills it */
void mech_FillPartAmmo(MECH * mech, int loc, int pos)
{
	int t, to;

	t = GetPartType(mech, loc, pos);

	if(!IsAmmo(t))
		return;
	if(!(to = MechWeapons[Ammo2Weapon(t)].ammoperton))
		return;
	SetPartData(mech, loc, pos, FullAmmo(mech, loc, pos));
}

int AcceptableDegree(int d)
{
	/*
	 * Silly billies, integer modulo (division) is still faster than loops.
	 * And probably slightly faster than branches, too, but let's not worry
	 * about that.
	 */
	if (d < 0) {
		return (d % 360) + 360;
	} else if (d >= 360) {
		return (d % 360);
	} else {
		return d;
	}
}

void MarkForLOSUpdate(MECH * mech)
{
	MAP *mech_map;

	if(!(mech_map = getMap(mech->mapindex)))
		return;
	mech_map->moves++;
	mech_map->mechflags[mech->mapnumber] = 1;
}

void multi_weap_sel(MECH * mech, dbref player, char *buffer, int bitbybit,
					int (*foo) (MECH *, dbref, int, int))
{
	/* Insight: buffer contains stuff in form:
	   <num>
	   <num>-<num>
	   <num>,..
	   <num>-<num>,..
	 */
	/* Ugly recursive piece of code :> */
	char *c;
	int i1, i2, i3;
	int section, critical;

	skipws(buffer);
	if((c = strstr(buffer, ","))) {
		*c = 0;
		c++;
	}
	if(sscanf(buffer, "%d-%d", &i1, &i2) == 2) {
		DOCHECK(i1 < 0 ||
				i1 >= MAX_WEAPONS_PER_MECH,
				tprintf("Invalid first number in range (%d)", i1));
		DOCHECK(i2 < 0 ||
				i2 >= MAX_WEAPONS_PER_MECH,
				tprintf("Invalid second number in range (%d)", i2));
		if(i1 > i2) {
			i3 = i1;
			i1 = i2;
			i2 = i3;
		}
	} else {
		DOCHECK(Readnum(i1, buffer), tprintf("Invalid value: %s", buffer));
		DOCHECK(i1 < 0 ||
				i1 >= MAX_WEAPONS_PER_MECH,
				tprintf("Invalid weapon number: %d", i1));
		i2 = i1;
	}
	if(bitbybit / 2) {
		DOCHECK(i2 >= NUM_TICS, tprintf("There are only %d tics!", i2));
	} else {
		DOCHECK(!(FindWeaponNumberOnMech(mech, i2, &section,
										 &critical) != -1),
				tprintf("Error: the mech doesn't HAVE %d weapons!", i2 + 1));
	}
	if(bitbybit % 2) {
		for(i3 = i1; i3 <= i2; i3++)
			if(foo(mech, player, i3, i3))
				return;
	} else if(foo(mech, player, i1, i2))
		return;
	if(c)
		multi_weap_sel(mech, player, c, bitbybit, foo);
}

int Roll()
{
	int i = Number(1, 6) + Number(1, 6);

	rollstat.rolls[i - 2]++;
	rollstat.totrolls++;
	return i;
}

int MyHexDist(int x1, int y1, int x2, int y2, int tc)
{
	int xd = abs(x2 - x1);
	int yd = abs(y2 - y1);
	int hm;

	/* _the_ base case */
	if(x1 == x2)
		return yd;
	/*
	   +
	   +
	   +
	   +
	 */
	if((hm = (xd / 2)) <= yd)
		return (yd - hm) + tc + xd;

	/*
	   +     +
	   +   +
	   + +
	   +
	 */
	if(!yd)
		return (xd + tc);
	/*
	   +
	   +
	   +   +
	   + +
	   +
	 */
	/* For now, same as above */
	return (xd + tc);
}

int CountDestroyedLegs(MECH * objMech)
{
	int wcDeadLegs = 0;

	if(MechType(objMech) != CLASS_MECH)
		return 0;

	if(MechIsQuad(objMech)) {
		if(IsLegDestroyed(objMech, LARM))
			wcDeadLegs++;

		if(IsLegDestroyed(objMech, RARM))
			wcDeadLegs++;
	}

	if(IsLegDestroyed(objMech, LLEG))
		wcDeadLegs++;

	if(IsLegDestroyed(objMech, RLEG))
		wcDeadLegs++;

	return wcDeadLegs;
}

int IsLegDestroyed(MECH * objMech, int wLoc)
{
	return (SectIsDestroyed(objMech, wLoc) || SectIsBreached(objMech, wLoc)
			|| SectIsFlooded(objMech, wLoc));
}

int IsMechLegLess(MECH * objMech)
{
	int wcMaxLegs = 0;

	if(MechType(objMech) != CLASS_MECH)
		return 0;

	if(MechIsQuad(objMech))
		wcMaxLegs = 4;
	else
		wcMaxLegs = 2;

	if(CountDestroyedLegs(objMech) >= wcMaxLegs)
		return 1;

	return 0;
}

int FindFirstWeaponCrit(MECH * objMech, int wLoc, int wSlot,
						int wStartSlot, int wCritType, int wMaxCrits)
{
	int wCritsInLoc = 0;
	int wCritIter, wFirstCrit;

	/*
	 * First let's count the number of crits in this loc, incase
	 * we have two of the same weapon
	 */

	wFirstCrit = -1;

	for(wCritIter = wStartSlot; wCritIter < NUM_CRITICALS; wCritIter++) {
		if(GetPartType(objMech, wLoc, wCritIter) == wCritType) {
			wCritsInLoc++;

			if(wFirstCrit == -1)
				wFirstCrit = wCritIter;
		}
	}

	if((wFirstCrit > -1) && (wSlot == -1))
		return wFirstCrit;

	/*
	 * Now, if there are more crits than our max crit, then we have
	 * two of the same weapon in this location. We need to figure
	 * out which weapon this crit actually belongs to.
	 */
	if(wCritsInLoc > wMaxCrits) {
		/*
		 * Well, we have thje first crit of the first instance, so
		 * let's see if our crit falls out of that range.. if so, then
		 * we need to figure out what range it actually falls into.
		 */
		if((wFirstCrit + wMaxCrits) <= wSlot)
			wFirstCrit =
				FindFirstWeaponCrit(objMech, wLoc, wSlot,
									(wFirstCrit + wMaxCrits), wCritType,
									wMaxCrits);
	}

	return wFirstCrit;
}

int checkAllSections(MECH * mech, int specialToFind)
{
	int i;

	for(i = 0; i < NUM_SECTIONS; i++) {
		if(checkSectionForSpecial(mech, specialToFind, i))
			return 1;
	}

	return 0;
}

int checkSectionForSpecial(MECH * mech, int specialToFind, int wSec)
{
	if(SectIsDestroyed(mech, wSec))
		return 0;

	if(MechSections(mech)[wSec].specials & specialToFind)
		return 1;

	return 0;
}

int getRemainingInternalPercent(MECH * mech)
{
	int i;
	float wMax = 0;
	float wRemaining = 0;

	for(i = 0; i < NUM_SECTIONS; i++) {
		wMax += GetSectOInt(mech, i);

		wRemaining += GetSectInt(mech, i);
	}

	if(wMax == 0)
		return 0;

	return ((wRemaining / wMax) * 100);
}

int getRemainingArmorPercent(MECH * mech)
{
	int i;
	float wMax = 0;
	float wRemaining = 0;

	for(i = 0; i < NUM_SECTIONS; i++) {
		wMax += GetSectOArmor(mech, i);
		wMax += GetSectORArmor(mech, i);

		wRemaining += GetSectArmor(mech, i);
		wRemaining += GetSectRArmor(mech, i);
	}

	if(wMax == 0)
		return 0;

	return ((wRemaining / wMax) * 100);
}

int FindObj(MECH * mech, int loc, int type)
{
	int count = 0, i;

	for(i = 0; i < NUM_CRITICALS; i++)
		if(GetPartType(mech, loc, i) == type)
			if(!PartIsNonfunctional(mech, loc, i))
				count++;
	return count;
}

int FindObjWithDest(MECH * mech, int loc, int type)
{
	int count = 0, i;

	for(i = 0; i < NUM_CRITICALS; i++)
		if(GetPartType(mech, loc, i) == type)
			count++;
	return count;
}

/* Usage:
   mech      = Mech who's looking for people
   mech_map  = Map mech's on
   x,y       = Target hex
   needlos   = Bitvector
   1 = Require LOS
   2 = We actually want a mech that is friendly and has LOS to hex
 */
MECH *find_mech_in_hex(MECH * mech, MAP * mech_map, int x, int y, int needlos)
{
	int loop;
	MECH *target;

	for(loop = 0; loop < mech_map->first_free; loop++)
		if(mech_map->mechsOnMap[loop] != mech->mynum &&
		   mech_map->mechsOnMap[loop] != -1) {
			target = (MECH *) FindObjectsData(mech_map->mechsOnMap[loop]);
			if(!target)
				continue;
			if(!(MechX(target) == x && MechY(target) == y) && !(needlos & 2))
				continue;
			if(needlos) {
				if(needlos & 1)
					if(!InLineOfSight(mech, target, x, y,
									  FlMechRange(mech_map, mech, target)))
						continue;
				if(needlos & 2) {
					if(MechTeam(mech) != MechTeam(target))
						continue;
					if(!(MechSeesHex(target, mech_map, x, y)))
						continue;
					if(mech == target)
						continue;
				}
			}
			return target;
		}
	return NULL;
}

int FindAndCheckAmmo(MECH * mech,
					 int weapindx,
					 int section,
					 int critical,
					 int *ammoLoc,
					 int *ammoCrit, int *ammoLoc1, int *ammoCrit1,
					 int *wGattlingShots)
{
	int mod, nmod = 0;
	int wMaxShots = 0;
	int wRoundsToCheck = 1;
	int wWeapMode = GetPartFireMode(mech, section, critical);
	int tResetMode = 0;
	dbref player = GunPilot(mech);

	/* Return if it's an energy or PC weapon */
	if(MechWeapons[weapindx].type == TBEAM ||
	   MechWeapons[weapindx].type == THAND)
		return 1;

	/* Check for rocket launchers */
	if(MechWeapons[weapindx].special == ROCKET) {
		DOCHECK0(wWeapMode & ROCKET_FIRED,
				 "That weapon has already been used!");
		return 1;
	}

	/* Check for One-Shots */
	if(wWeapMode & OS_MODE) {
		DOCHECK0(GetPartFireMode(mech, section, critical) & OS_USED,
				 "That weapon has already been used!");
		return 1;
	}
	/* Check RACs - No special ammo type possible */
	if(MechWeapons[weapindx].special & RAC) {
		wMaxShots = CountAmmoForWeapon(mech, weapindx);

		if((wWeapMode & RAC_TWOSHOT_MODE) && (wMaxShots < 2)) {
			GetPartFireMode(mech, section, critical) &= ~RAC_TWOSHOT_MODE;

			return 1;
		}

		if((wWeapMode & RAC_FOURSHOT_MODE) && (wMaxShots < 4)) {
			GetPartFireMode(mech, section, critical) &= ~RAC_FOURSHOT_MODE;

			return 1;
		}

		if((wWeapMode & RAC_SIXSHOT_MODE) && (wMaxShots < 6)) {
			GetPartFireMode(mech, section, critical) &= ~RAC_SIXSHOT_MODE;

			return 1;
		}
	}
	/* Check GMGs */
	if(wWeapMode & GATTLING_MODE) {
		wMaxShots = CountAmmoForWeapon(mech, weapindx);

		/*
		 * Gattling MGs suck up damage * 3 in ammo
		 */

		if((wMaxShots / 3) < *wGattlingShots)
			*wGattlingShots = MAX((wMaxShots / 3), 1);
	}
	/* If we're an ULTRA or RFAC, we need to check for multiple rounds */
	if((wWeapMode & ULTRA_MODE) || (wWeapMode & RFAC_MODE))
		wRoundsToCheck = 2;

	mod = GetPartAmmoMode(mech, section, critical) & AMMO_MODES;

	if(!mod) {
		DOCHECK0(!FindAmmoForWeapon_sub(mech, section, critical, weapindx,
										section, ammoLoc, ammoCrit,
										AMMO_MODES, 0),
				 "You don't have any ammo for that weapon stored on this mech!");

		DOCHECK0(!GetPartData(mech, *ammoLoc, *ammoCrit),
				 "You are out of ammo for that weapon!");

		if(wRoundsToCheck > 1) {
			GetPartData(mech, *ammoLoc, *ammoCrit)--;

			if(FindAmmoForWeapon_sub(mech, section, critical, weapindx,
									 section, ammoLoc1, ammoCrit1, AMMO_MODES,
									 0)) {
				if(!GetPartData(mech, *ammoLoc1, *ammoCrit1))
					tResetMode = 1;
			} else
				tResetMode = 1;

			if(tResetMode)
				GetPartFireMode(mech, section, critical) &= ~wWeapMode;

			GetPartData(mech, *ammoLoc, *ammoCrit)++;
		}
	} else {
		if(IsArtillery(weapindx))
			nmod = (~mod) & ARTILLERY_MODES;
		else
			nmod = (~mod) & AMMO_MODES;
		mod = (mod & AMMO_MODES);

		DOCHECK0(!FindAmmoForWeapon_sub(mech, section, critical, weapindx,
										section, ammoLoc, ammoCrit, nmod,
										mod),
				 "You don't have any ammo for that weapon stored on this mech!");

		DOCHECK0(!GetPartData(mech, *ammoLoc, *ammoCrit),
				 "You are out of the special ammo type for that weapon!");

		if(wRoundsToCheck > 1) {
			GetPartData(mech, *ammoLoc, *ammoCrit)--;

			if(FindAmmoForWeapon_sub(mech, section, critical, weapindx,
									 section, ammoLoc1, ammoCrit1, nmod,
									 mod)) {
				if(!GetPartData(mech, *ammoLoc1, *ammoCrit1))
					tResetMode = 1;
			} else
				tResetMode = 1;

			if(tResetMode)
				GetPartFireMode(mech, section, critical) &= ~wWeapMode;

			GetPartData(mech, *ammoLoc, *ammoCrit)++;
		}
	}

	return 1;
}

void ChannelEmitKill(MECH * mech, MECH * attacker, const char *reason)
{
	if (!attacker)
		attacker = mech;

	/* Very Rare Occassion where using btsetxcodevalue(mech,mechdamage,) triggers this, we'll just ignore */
	if ((mech->mynum == attacker->mynum) && !Good_obj(mech->mynum))
		return;


	if (reason) {
		SendDebug(tprintf("#%d [%s] has been killed by #%d [%s] (%s)",
		                  mech->mynum, MechType_Ref(mech), attacker->mynum, MechType_Ref(attacker), reason));
		SendDeath(tprintf("#%d [%s] has been killed by #%d [%s] (%s)",
				  mech->mynum, MechType_Ref(mech), attacker->mynum, MechType_Ref(attacker), reason));
	} else {
		SendDebug(tprintf("#%d [%s] has been killed by #%d [%s]",
		                  mech->mynum, MechType_Ref(mech), attacker->mynum, MechType_Ref(attacker)));
		SendDeath(tprintf("#%d [%s] has been killed by #%d [%s]",
				  mech->mynum, MechType_Ref(mech), attacker->mynum, MechType_Ref(attacker)));
	}

	if (IsDS(mech)) {
		if (reason) {
			SendDSInfo(tprintf("#%d has been killed by #%d (%s)",
			                   mech->mynum, attacker->mynum,
			                   reason));
		} else {
			SendDSInfo(tprintf("#%d has been killed by #%d",
			                   mech->mynum, attacker->mynum));
		}
	}

	/* Trigger AMECHDEST.  */
	if (Good_obj(mech->mynum) && Good_obj(attacker->mynum)) {
		char *reason_copy = NULL;

		char *args[1] = { NULL };
		int nargs = 0;

		if (reason) {
			reason_copy = alloc_lbuf("bt.reason");

			if (reason_copy) {
				/* Safe because reason is a KILL_TYPE_*. */
				strcpy(reason_copy, reason);

				args[0] = reason_copy;
				nargs = 1;
			}
		}

		did_it(attacker->mynum, mech->mynum,
		       0, NULL, 0, NULL, A_AMECHDEST, args, nargs);

		if (reason_copy) {
			free_lbuf(reason_copy);
		}
	}
}

#define NUM_NEIGHBORS	6
int dirs[6][2] = {
	{0, -1},
	{1, 0},
	{1, 1},
	{0, 1},
	{-1, 1},
	{-1, 0}
};

void visit_neighbor_hexes(MAP * map, int tx, int ty,
						  void (*callback) (MAP *, int, int))
{
	int x1, y1;
	int i;

	for(i = 0; i < NUM_NEIGHBORS; i++) {
		x1 = tx + dirs[i][0];
		y1 = ty + dirs[i][1];
		if(tx % 2 && !(x1 % 2))
			y1--;
		if(x1 < 0 || x1 >= map->map_width || y1 < 0 || y1 >= map->map_height)
			continue;
		callback(map, x1, y1);
	}
}

int GetPartWeight(int part)
{
	if(IsWeapon(part))
		return 10.24 * MechWeapons[Weapon2I(part)].weight;
	else if(IsAmmo(part))
		return 1024;
	else if(IsBomb(part))
		return 102 * BombWeight(Bomb2I(part));
#ifndef BT_PART_WEIGHTS
	else if(IsSpecial(part) && part <= I2Special(CLAW))
		return 1024;
#else
	else if(IsSpecial(part))	/* && i <= I2Special(LAMEQUIP) */
		return internalsweight[Special2I(part)];
	else if(IsCargo(part))
		return cargoweight[Cargo2I(part)];
#endif /* BT_PART_WEIGHTS */
	else
/* hmm.. tricky, suppose we'll make things light */
		return 102;
}

#ifdef BT_ADVANCED_ECON
unsigned long long int GetPartCost(int p)
{
	extern unsigned long long int specialcost[SPECIALCOST_SIZE];
	extern unsigned long long int ammocost[AMMOCOST_SIZE];
	extern unsigned long long int weapcost[WEAPCOST_SIZE];
	extern unsigned long long int cargocost[CARGOCOST_SIZE];
	extern unsigned long long int bombcost[BOMBCOST_SIZE];

	if(IsWeapon(p))
		return weapcost[Weapon2I(p)];
	else if(IsAmmo(p))
		return ammocost[Ammo2I(p)];
	else if(IsSpecial(p))
		return specialcost[Special2I(p)];
	else if(IsBomb(p))
		return bombcost[Bomb2I(p)];
	else if(IsCargo(p))
		return cargocost[Cargo2I(p)];
	else
		return 0;
}

void SetPartCost(int p, unsigned long long int cost)
{
	extern unsigned long long int specialcost[SPECIALCOST_SIZE];
	extern unsigned long long int ammocost[AMMOCOST_SIZE];
	extern unsigned long long int weapcost[WEAPCOST_SIZE];
	extern unsigned long long int cargocost[CARGOCOST_SIZE];
	extern unsigned long long int bombcost[BOMBCOST_SIZE];

	if(IsWeapon(p))
		weapcost[Weapon2I(p)] = cost;
	else if(IsAmmo(p))
		ammocost[Ammo2I(p)] = cost;
	else if(IsSpecial(p))
		specialcost[Special2I(p)] = cost;
	else if(IsBomb(p))
		bombcost[Bomb2I(p)] = cost;
	else if(IsCargo(p))
		cargocost[Cargo2I(p)] = cost;
}

void CalcFasaCost_AddPrice(float * total, char * desc, float value) {
   *total += value;
	if(mudconf.btech_cost_debug)
	      SendDebug(tprintf("Addprice - %25s %8.0f", desc, value));

}
    
int MechNumHeatsinksInEngine(MECH * mech) {
   // Heatsinks in Engine = Engine Rating / 25
   return (MechEngineSize(mech) / 25);
}

void CalcFasaCost_DoArmMath(MECH * mech, int loc, float * total) {
    int i = 0;
    for (i = 0; i < NUM_CRITICALS; i++) {
        int part = GetPartType(mech, loc, i);
        if (!IsActuator(part))
            continue;
        else if (Special2I(part) == SHOULDER_OR_HIP)
            continue;
            // BMR Says don't count this.
            //CalcFasaCost_AddPrice(total, "Shoulder Actuator", 0);
        else if (Special2I(part) == UPPER_ACTUATOR)
            CalcFasaCost_AddPrice(total, "ARM Upper Actuator", (MechTons(mech) * 100));
        else if (Special2I(part) == LOWER_ACTUATOR)
            CalcFasaCost_AddPrice(total, "ARM Lower Actuator", (MechTons(mech) * 50));
        else if (Special2I(part) == HAND_OR_FOOT_ACTUATOR)
            CalcFasaCost_AddPrice(total, "ARM Hand Actuator", (MechTons(mech) * 80));        
    }
}
    
void CalcFasaCost_DoLegMath(MECH * mech, int loc, float * total) {
    int i = 0;
    for (i = 0; i < NUM_CRITICALS; i++) {
        int part = GetPartType(mech, loc, i);
        if (!IsActuator(part))
            continue;
        else if (Special2I(part) == SHOULDER_OR_HIP)
		continue;
		// BMR Says don't count the Hip 
	else if (Special2I(part) == UPPER_ACTUATOR)
            CalcFasaCost_AddPrice(total, "LEG Upper Actuator", (MechTons(mech) * 150));
        else if (Special2I(part) == LOWER_ACTUATOR)
            CalcFasaCost_AddPrice(total, "LEG Lower Actuator", (MechTons(mech) * 80));
        else if (Special2I(part) == HAND_OR_FOOT_ACTUATOR)
            CalcFasaCost_AddPrice(total, "LEG Actuator", (MechTons(mech) * 120));        
    }
}

/* 
 * Calculate the FASA cost of a unit as per an approximation of Maxtech
 * construction/cost rules. 
 */
unsigned long long int CalcFasaCost(MECH * mech)
{
	int ii, i, part;
	float total = 0;
	float mod = 1.0;
	int count, ammoweapcount;
        unsigned char weaparray[MAX_WEAPS_SECTION];
        unsigned char weapdata[MAX_WEAPS_SECTION];
        int critical[MAX_WEAPS_SECTION];
        unsigned char ammoweap[8 * MAX_WEAPS_SECTION];
	unsigned short ammo[8 * MAX_WEAPS_SECTION];
	unsigned short ammomax[8 * MAX_WEAPS_SECTION];
	unsigned int modearray[8 * MAX_WEAPS_SECTION];
	int temp;
	int engine_size = 0;
	int has_sword = 0;
	
	if(!mech)
		return -1;

	if(!
	   (MechType(mech) == CLASS_MECH || MechType(mech) == CLASS_VEH_GROUND
		|| MechType(mech) == CLASS_VEH_NAVAL || MechType(mech) == CLASS_VTOL
		|| MechType(mech) == CLASS_BSUIT)
	   || is_aero(mech) || IsDS(mech))
		return 0;

	if(MechType(mech) == CLASS_MECH) {

/* Start MECH Internal Structure Skeleton ( Tech Manual (p278) MaxTech (p87) ) */
		if(MechSpecials(mech) & ES_TECH || MechSpecials(mech) & COMPI_TECH)
			CalcFasaCost_AddPrice(&total, "ES/Co Internals", (MechTons(mech) * 1600));
		else if(MechSpecials(mech) & REINFI_TECH)
			CalcFasaCost_AddPrice(&total, "RE Internals", (MechTons(mech) * 6400));
		else
			CalcFasaCost_AddPrice(&total, "Std Internals", (MechTons(mech) * 400));
/* End MECH Internal Structure Skeleton */

/* Cockpit */
#if 0
/* NULLTODO : Port any of these techs ASAP */
		if(MechSpecials2(mech) & SMALLCOCKPIT_TECH)
			ADDPRICE("SmallCockpit", 175000)
				else
		if(MechSpecials2(mech) & TORSOCOCKPIT_TECH)
			ADDPRICE("TorsoCockpit", 750000)
				else
				EI is 400000 (Clan Tech)
#endif
      CalcFasaCost_AddPrice(&total, "Cockpit", 200000);

/* Start MECH Life Support ( Tech Manual (p278) ) */
	      CalcFasaCost_AddPrice(&total, "LifeSupport", 50000);
/* End MECH Life Support */

/* Sensors */
/* TODO: Add variable range and multi-trac II */
	      CalcFasaCost_AddPrice(&total, "Sensors", (MechTons(mech) * 2000));

/* Start MECH Musculatre (Myomer) ( Tech Manual (p278) )*/
		if(MechSpecials(mech) & TRIPLE_MYOMER_TECH)
			CalcFasaCost_AddPrice(&total, "TS Myomer", (MechTons(mech) * 16000));
		else
			CalcFasaCost_AddPrice(&total, "Myomer", (MechTons(mech) * 2000));
/* End MECH Musculatre (Myomer) */

/* Actuators */
      CalcFasaCost_DoArmMath(mech, LARM, &total);
      CalcFasaCost_DoArmMath(mech, RARM, &total);
		
      CalcFasaCost_DoLegMath(mech, LLEG, &total);
      CalcFasaCost_DoLegMath(mech, RLEG, &total);
		
/* Gyro */
		i = MechEngineSize(mech);
		if(i % 100)
			i += (100 - (MechEngineSize(mech) % 100));
		i /= 100;

		if(MechSpecials2(mech) & XLGYRO_TECH)
			CalcFasaCost_AddPrice(&total, "XL Gyro", (i * 0.5 * 750000));
		else if(MechSpecials2(mech) & CGYRO_TECH)
			   CalcFasaCost_AddPrice(&total, "Compact Gyro", (i * 1.5 * 400000));
		else if(MechSpecials2(mech) & HDGYRO_TECH)
				CalcFasaCost_AddPrice(&total, "HD Gyro", (i * 2 * 500000));
		else
		   CalcFasaCost_AddPrice(&total, "Gyro", (i * 300000));

	} else if (MechType(mech) == CLASS_BSUIT) {
	/* ---------------------------------
	 * BSuit Costs
	 */
    	if (MechSpecials(mech) & CLAN_TECH) {
    	    CalcFasaCost_AddPrice(&total, "Clan Point", 3500000);
    	} else {
    	    CalcFasaCost_AddPrice(&total, "IS Squad", 2400000);
    	}
	
	} else {
	/* ---------------------------------
	 * Vehicle Costs
	 */
		int pamp = 0, turret = 0;

		for(i = 0; i < NUM_SECTIONS; i++)
			for(ii = 0; ii < NUM_CRITICALS; ii++) {
				if(!(part = GetPartType(mech, i, ii)))
					continue;
				if(!IsWeapon(part))
					continue;
				if(i == TURRET)
					turret += crit_weight(mech, part);
				if(IsEnergy(part)) {
					pamp += crit_weight(mech, part);
					SendDebug(tprintf("PAmp Weight: %d", crit_weight(mech, part)));
				}
			}
/* 
 * Internals 
 * 10,000 * Structure Tonnage
 */
		int internals = (float)MechTons(mech) * 1000;
		CalcFasaCost_AddPrice(&total, "Internals", internals);
/* 
 * Control Components
 * 10,000 * Control Tonnage 
 * Control Tonnage = .05 * Tons
 */
		int control_eq = 10000 * 0.05 * MechTons(mech);
		CalcFasaCost_AddPrice(&total, "Cockpit & Controls", control_eq);
/* 
 * Power Amp 
 * 20,000 * Amplifier Tonnage
 */
		if(MechSpecials(mech) & ICE_TECH) {
			int power_amp =  20000 * ( pamp / 1024 ) / 10;
			CalcFasaCost_AddPrice(&total, "Power Amplifiers", power_amp);
		}
		
/* 
 * Turret 
 * Standard: 5,000 * Turret Tonnage
 */
		int turret_price = 5000 * (turret / 10) / 1024;
		CalcFasaCost_AddPrice(&total, "Turret", turret_price);
/* 
 * Lift/Dive Equip (Hovercraft, Hydrofoils, Submarines)
 * 20,000 * Equipment Tonnage
 */
		if(MechMove(mech) == MOVE_HOVER
			|| MechMove(mech) == MOVE_FOIL
			|| MechMove(mech) == MOVE_SUB) {
			float lift_dive = 20000 * (0.1 * MechTons(mech));
			CalcFasaCost_AddPrice(&total, "Lift/Dive Equip", lift_dive);
		}

		if(MechMove(mech) == MOVE_VTOL) {
			float vtol_eq = 40000 * (0.1 * MechTons(mech));
			CalcFasaCost_AddPrice(&total, "Rotor", vtol_eq);
		}
	} // end if (Vehicle Calcs)

/* ----------------------------
 * General Calculations
 */
 
if (MechType(mech) != CLASS_BSUIT) {
    /* Engine Math 
     * (Engine Basecost * Engine Rating * Tonnage) / 75
     */
    int engine_basecost = (MechSpecials(mech) & CE_TECH ? 10000 :
        MechSpecials(mech) & LE_TECH ? 15000 :
    	MechSpecials(mech) & XL_TECH ? 20000 :
    	MechSpecials(mech) & XXL_TECH ? 100000 :
    	MechSpecials(mech) & ICE_TECH ? 1250 : 5000);
    	    
    engine_size = MechEngineSize(mech);   
        
    if (MechMove(mech) == MOVE_WHEEL ||
    	MechMove(mech) == MOVE_FOIL ||
    	MechMove(mech) == MOVE_HOVER ||
    	MechMove(mech) == MOVE_HULL || 
    	MechMove(mech) == MOVE_SUB ||
    	MechMove(mech) == MOVE_VTOL) {
    	engine_size = engine_size - susp_factor(mech);
    }
    	/* Don't forget to Round up! */ 
    	int engine_price = ceil((engine_basecost * engine_size * MechTons(mech)) / 75.0);
    	    
    	CalcFasaCost_AddPrice(&total, "Engine", engine_price);
    
    /* Jump Jets 
     * Standard: Tonnage * (number of JJs^2) * 200
     * Improved: Tonnage * (number of JJs^2) * 500
     * Mechanical: Tonnage * (Jumping MP) * 150
     */
    	int num_jjs = MechJumpSpeed(mech) * MP_PER_KPH;
    	int jj_price = MechTons(mech) * pow(num_jjs, 2) * 200.0;
    	if (num_jjs > 0)
    		CalcFasaCost_AddPrice(&total, "Jumpjets", jj_price);
    
    /* 
       Heat Sinks 
    */
    	int numsinks = MechRealNumsinks(mech);
    	
    	int sinkcost;
    	if(MechSpecials(mech) & DOUBLE_HEAT_TECH || MechSpecials(mech) & CLAN_TECH)
    	    sinkcost = 6000;
    	else if(MechSpecials2(mech) & COMPACT_HS_TECH)
    	    sinkcost = 3000;
    	else
    	    sinkcost = 2000;
    
    	if((MechSpecials(mech) & DOUBLE_HEAT_TECH || MechSpecials(mech) & CLAN_TECH)) {
    		/* We want to divide the heat dissipation by two if DHS */
    		numsinks = BOUNDED(0, numsinks/2, 500);
    	}
    
       // For single heatsinks, we only charge for every heatsink over 10.
       if(MechSpecials(mech) & DOUBLE_HEAT_TECH || MechSpecials(mech) & CLAN_TECH
          || MechSpecials2(mech) & COMPACT_HS_TECH || MechSpecials(mech) & ICE_TECH)
           CalcFasaCost_AddPrice(&total, "Heat Sinks", (numsinks * sinkcost));
       else {
           CalcFasaCost_AddPrice(&total, "Heat Sinks", 
             (BOUNDED(0, numsinks - 10, 500) * sinkcost));
       }
    
       
    #if COST_DEBUG
    	SendDebug(tprintf("Heat Sinks: %d, Cost Per Sink: %d", numsinks, sinkcost));
    #endif
    
    /* Armor */
    	int total_armor = 0;
	int orig_armor = 0;
    	int armor_section = 0;
    	for(armor_section = 0; armor_section < NUM_SECTIONS; ++armor_section) {
    		total_armor += GetSectOArmor(mech, armor_section);
    		total_armor += GetSectORArmor(mech, armor_section);
    	}

	orig_armor = total_armor;

	if(MechSpecials(mech) & FF_TECH)
	    total_armor = total_armor * 50 / ((MechSpecials(mech) & CLAN_TECH) ? 60 : 56);
	else if(MechSpecials2(mech) & HVY_FF_ARMOR_TECH)
	    total_armor = total_armor * 50 / 62;
	else if(MechSpecials2(mech) & LT_FF_ARMOR_TECH)
	    total_armor = total_armor * 50 / 53;

	/* Come on. Really. We don't do .1 of armor. Round this !!! */
	float armor_tons = round_to_halfton( total_armor * 1024 / 16 );
	armor_tons = armor_tons / 1024;

    	int armor_cost_point = (MechSpecials(mech) & FF_TECH ? 20000 : MechSpecials2(mech) & 
    		STEALTH_ARMOR_TECH ? 50830 : MechSpecials(mech) &
    		HARDA_TECH ? 15000 : MechSpecials2(mech) & LT_FF_ARMOR_TECH ?
    		 15000 : MechSpecials2(mech) & HVY_FF_ARMOR_TECH ? 25000 :
    		 10000);
    #if COST_DEBUG
    	SendDebug(tprintf("Armor Tons %.1f(%d pts) * Armor Cost Per Point %d", 
    		armor_tons, orig_armor, armor_cost_point));
    #endif
    	int armor_price = armor_tons * armor_cost_point;
    	CalcFasaCost_AddPrice(&total, "Armor", armor_price);
} // End Non-BSuit General Calculations


/* Weapons. */
/* While it might not make much sense to do this twice, we need to go through all the sections */
/* and handle weapons first, than we'll handle parts */

	for(i = 0; i < NUM_SECTIONS; i++) {
		count = FindWeapons(mech, i, weaparray, weapdata, critical);
		if(count <= 0)
			/* No weapons */
			continue;

		for (ii = 0; ii < count; ii++ ) {
			CalcFasaCost_AddPrice(&total, MechWeapons[weaparray[ii]].name, MechWeapons[weaparray[ii]].cost);
		}

	}

/* Ammo */
        
	ammoweapcount = FindAmmunition(mech, ammoweap, ammo, ammomax, modearray, 0);

	if(ammoweapcount > 0 ) {
		if(mudconf.btech_cost_debug)
			SendDebug("Ammo Costs");
		for (i = 0; i < ammoweapcount; i++) {
		/* ArtemisIV ammo is X2 */
		/* Interesting way to handle half_tons */
			if (ammomax[i] < MechWeapons[ammoweap[i]].ammoperton )
  			    CalcFasaCost_AddPrice(&total, MechWeapons[ammoweap[i]].name, MechWeapons[ammoweap[i]].ammo_cost / 
				      (MechWeapons[ammoweap[i]].ammoperton / ammomax[i]));
			else
		    	    CalcFasaCost_AddPrice(&total, MechWeapons[ammoweap[i]].name,
				MechWeapons[ammoweap[i]].ammo_cost * (ammomax[i] / MechWeapons[ammoweap[i]].ammoperton )
				* ((modearray[i] & ARTEMIS_MODE) ? 2 : 1));
		}
	}


/* Parts */

	int masc_count = 0;
	int bloodhound_count = 0;
	for(i = 0; i < NUM_SECTIONS; i++)
		for(ii = 0; ii < NUM_CRITICALS; ii++) {
			part = GetPartType(mech, i, ii);
			if(IsActuator(part) || part == EMPTY)
				continue;
			if(IsSpecial(part))
				/* These parts are handled above, don't count their crits */
				switch (Special2I(part)) {
					case LIFE_SUPPORT:
						continue;
					case SENSORS:
						continue;
					case COCKPIT:
						continue;
					case ENGINE:
						continue;
					case GYRO:
						continue;
					case HEAT_SINK:
						continue;
					case JUMP_JET:
						continue;
					case FERRO_FIBROUS:
						continue;
					case LT_FERRO_FIBROUS:
						continue;
					case ENDO_STEEL:
						continue;
					case TRIPLE_STRENGTH_MYOMER:
						continue;
					case STEALTH_ARMOR:
						continue;
					case MASC:
						masc_count++;
						continue;
					case SWORD:
						has_sword = 1;
						continue;
					case CASE:
						CalcFasaCost_AddPrice(&total, "Int Case", 50000);
						continue;
					case CASEII:
						CalcFasaCost_AddPrice(&total, "Int CaseII", 175000);
						continue;
					case AXE:
						CalcFasaCost_AddPrice(&total, "Int Axe", 5000);
						continue;
					case BEAGLE_PROBE:
						CalcFasaCost_AddPrice(&total, "BAP", 100000);
						continue;
					case BLOODHOUND_PROBE:
						bloodhound_count++;
						continue;
					case ARTEMIS_IV:
						CalcFasaCost_AddPrice(&total, "ArtemisIV FCS", 100000);
						continue;
					case ANGELECM:
						CalcFasaCost_AddPrice(&total, "Angel ECM", 375000);
						continue;
					case C3_MASTER:
						CalcFasaCost_AddPrice(&total, "C3M", 300000);
						continue;
					case C3_SLAVE:
						CalcFasaCost_AddPrice(&total, "C3S", 250000);
						continue;
					case C3I:
						CalcFasaCost_AddPrice(&total, "C3I", 375000);
						continue;
					case ECM:
						CalcFasaCost_AddPrice(&total, "ECM", 100000);
						continue;
					case TAG:
						CalcFasaCost_AddPrice(&total, "TAG", 50000);
						continue;
					case TARGETING_COMPUTER:
						CalcFasaCost_AddPrice(&total, "TargComp", 10000);
						continue;

#if 0	
/* NULLTODO : Port any of these techs ASAP */
						case HARDPOINT:
							continue;
#endif
					default:
						break;
				}
				if(IsAmmo(part)) 
					continue;
				if(IsWeapon(part))
					continue;

                                int indiv_part_cost = GetPartCost(part);
                                if (MechType(mech) != CLASS_MECH && IsWeapon(part)) {
                                    indiv_part_cost *= MechWeapons[part-1].criticals;
                                    //SendDebug(tprintf("Part#: %s(%d) Crits: %d", MechWeapons[part-1].name, part-1, MechWeapons[part-1].criticals));
                                }
                                    CalcFasaCost_AddPrice(&total, (char*)part_name(part, 0), indiv_part_cost);

		}
		/* We have to account for some other stuff that doesn't divide equally here */
		if(bloodhound_count / 3)
			CalcFasaCost_AddPrice(&total, "Bloodhound", 500000 * (bloodhound_count / 3));
		if(masc_count)
			CalcFasaCost_AddPrice(&total, "MASC", masc_count * engine_size * 1000);
		if(has_sword) {
		/* Sword Cost is Tonnage of sword * 10000. Sword Tonnage is 1/20th of Mech Tonnage, rounded up to nearest halfton */
	             float sword_tons = round_to_halfton( MechTons(mech) * 1024 / 20 );
                     sword_tons = sword_tons / 1024; 
		     CalcFasaCost_AddPrice(&total, "Sword", sword_tons * 10000);

		 }


	if(MechType(mech) != CLASS_MECH && MechType(mech) != CLASS_BSUIT) {
		switch (MechMove(mech)) {
			case MOVE_TRACK:
				mod = (float) 1 + (float) ((float) MechTons(mech) / (float) 100);
				break;
			case MOVE_WHEEL:
				mod = (float) 1 + (float) ((float) MechTons(mech) / (float) 200);
				break;
			case MOVE_HOVER:
				mod = (float) 1 + (float) ((float) MechTons(mech) / (float) 50);
				break;
			case MOVE_VTOL:
				mod = (float) 1 + (float) ((float) MechTons(mech) / (float) 30);
				break;
			case MOVE_HULL:
				mod = (float) 1 + (float) ((float) MechTons(mech) / (float) 200);
				break;
			case MOVE_FOIL:
				mod = (float) 1 + (float) ((float) MechTons(mech) / (float) 75);
				break;
			case MOVE_SUB:
				mod = (float) 1 + (float) ((float) MechTons(mech) / (float) 50);
				break;
		}
	} else if (MechType(mech) == CLASS_BSUIT) {
	    // There's nothing in Maxtech about this, but we're going to knock the prices
	    // down to be competitive with other unit types.
	    mod = 0.75;
	} else {
	    // The standard mech modifier.
		mod = (float) 1 + (float) ((float) MechTons(mech) / (float) 100);
	}

	if (MechIsOmniMech(mech)) {
		SendDebug(tprintf("Mech is Omni, multiplying %lld by .25", total));
		total *= .25;
	}

#if COST_DEBUG
	SendDebug(tprintf("Price Total %.0f * Mod - %f = %.0f", total, mod, total*mod));
#endif

	return (total * mod);
} /* End Function */

#endif

#ifdef BT_CALCULATE_BV
int FindAverageGunnery(MECH * mech) {
#if 1
/* NULLTODO : Get the multiple skills for gunnery and such ported or working here so this is usefull again. */
			return FindPilotGunnery(mech, 0);
#else
			int runtot = 0;
			int i;

			if(!mech)
				return 12;

			for(i = 0; i < 5; i++) {
				runtot +=
					FindPilotGunnery(mech,
									 (i == 0 ? 0 : i == 1 ? 4 : i ==
									  2 ? 5 : i == 3 ? 6 : i == 4 ? 103 : 0));
			}
			return (runtot / 5);
#endif
		}

#undef DEBUG_BV
#define HIGH_SKILL      8
#define LOW_SKILL       0
		float skillmul[HIGH_SKILL][HIGH_SKILL] = {
			{2.05, 2.00, 1.95, 1.90, 1.85, 1.80, 1.75, 1.70},
			{1.85, 1.80, 1.75, 1.70, 1.65, 1.60, 1.55, 1.50},
			{1.65, 1.60, 1.55, 1.50, 1.45, 1.40, 1.35, 1.30},
			{1.45, 1.40, 1.35, 1.30, 1.25, 1.20, 1.15, 1.10},
			{1.25, 1.20, 1.15, 1.10, 1.05, 1.00, 0.95, 0.90},
			{1.15, 1.10, 1.05, 1.00, 0.95, 0.90, 0.85, 0.80},
			{1.05, 1.00, 0.95, 0.90, 0.85, 0.80, 0.75, 0.70},
			{0.95, 0.90, 0.85, 0.80, 0.75, 0.70, 0.65, 0.60}
		};

#define LAZY_SKILLMUL(n)  (n < LOW_SKILL ? LOW_SKILL : n >= HIGH_SKILL - 1 ? HIGH_SKILL - 1 : n)

		int CalculateBV(MECH * mech, int gunstat, int pilstat) {
			int defbv = 0, offbv = 0, i, ii, temp, temp2, deduct =
				0, offweapbv = 0, defweapbv = 0, armor = 0, intern =
				0, weapindx, mostheat = 0, tempheat =
				0, mechspec, mechspec2, type, move, pilskl = pilstat, gunskl =
				gunstat;
			int debug1 = 0, debug2 = 0, debug3 = 0, debug4 = 0;
			float maxspeed, mul = 1.00;

			if(!mech)
				return 0;

			if(gunstat == 100 || pilstat == 100) {
				if(muxevent_tick - MechBVLast(mech) < 30)
					return MechBV(mech);
				else
					MechBVLast(mech) = muxevent_tick;
			}

			type = MechType(mech);
			move = MechMove(mech);
			mechspec = MechSpecials(mech);
			mechspec2 = MechSpecials2(mech);
			if(gunstat == 100)
				pilskl = FindPilotPiloting(mech);
			if(pilstat == 100)
				gunskl = FindAverageGunnery(mech);

			for(i = 0; i < NUM_SECTIONS; i++) {
				armor += (debug1 =
						  GetSectArmor(mech,
									   i) *
						  (mechspec & HARDA_TECH ? 200 : 100));
				if(type == CLASS_MECH
				   && (i == CTORSO || i == LTORSO || i == RTORSO)) {
					armor += (debug2 =
							  GetSectRArmor(mech,
											i) *
							  (mechspec & HARDA_TECH ? 200 : 100));
#if 0
/* NULLTODO : Port any of these techs ASAP */
					if(mechspec2 & TORSOCOCKPIT_TECH && i == CTORSO)
						armor += (debug4 =
								  (((GetSectArmor(mech, i) +
									 GetSectRArmor(mech,
												   i)) * 2) *
								   (mechspec & HARDA_TECH ? 200 : 100)));
#endif
				}
				if(!is_aero(mech))
					intern += (debug3 =
							   GetSectInt(mech,
										  i) *
							   (mechspec & COMPI_TECH ? 50 : mechspec &
								REINFI_TECH ? 200 : 100));
				else
					intern = (debug3 = AeroSI(mech));
#ifdef DEBUG_BV
				SendDebug(tprintf
						  ("Armoradd : %d ArmorRadd : %d Internadd : %d",
						   debug1 / 100, debug2 / 100, debug3 / 100));
				if(mechspec2 & TORSOCOCKPIT_TECH && i == CTORSO)
					SendDebug(tprintf("TorsoCockpit Armoradd : %d", debug4));
#endif

				debug1 = debug2 = debug3 = debug4 = 0;
				for(ii = 0; ii < CritsInLoc(mech, i); ii++) {
					if(IsWeapon(temp = GetPartType(mech, i, ii))) {
						weapindx = (Weapon2I(temp));
						if(PartIsNonfunctional(mech, i, ii)) {
							if(type == CLASS_MECH)
								ii += (MechWeapons[weapindx].criticals - 1);
							continue;
						}
						if(MechWeapons[weapindx].special & AMS) {
							defweapbv += (debug1 =
										  (MechWeapons[weapindx].battlevalue *
										   100) * (float) (3000 /
														   (MechWeapons
															[weapindx].vrt *
															100)));

#ifdef DEBUG_BV
							SendDebug(tprintf
									  ("DefWeapBVadd (%s) : %d - Total : %d",
									   MechWeapons[weapindx].name,
									   debug1 / 100, defweapbv / 100));
#endif

						} else {
							offweapbv += (debug1 =
										  (MechWeapons[weapindx].battlevalue *
										   (GetPartFireMode(mech, i, ii) &
											REAR_MOUNT ? 50 : 100)) *
										  (float) ((float) 3000 /
												   (float) (MechWeapons
															[weapindx].vrt *
															100)));
							if(MechWeapons[weapindx].type == TMISSILE)
								if(FindArtemisForWeapon(mech, i, ii))
									offweapbv +=
										(MechWeapons[weapindx].battlevalue *
										 20);
#ifdef DEBUG_BV
							SendDebug(tprintf
									  ("OffWeapBVadd (%s) : %d - Total : %d",
									   MechWeapons[weapindx].name,
									   debug1 / 100, offweapbv / 100));
#endif

						}
						if(type == CLASS_MECH) {
							if(!(GetPartFireMode(mech, i, ii) & REAR_MOUNT)) {
								tempheat =
									((MechWeapons[weapindx].heat * 100) *
									 (float) ((float) 3000 /
											  (float) (MechWeapons[weapindx].
													   vrt * 100)));
								if(MechWeapons[weapindx].special & ULTRA)
									tempheat = (tempheat * 2);
								if(MechWeapons[weapindx].special & STREAK)
									tempheat = (tempheat / 2);
								mostheat += tempheat;
#ifdef DEBUG_BV
								SendDebug(tprintf
										  ("Tempheatadded (%s) : %d - Total : %d",
										   MechWeapons[weapindx].name,
										   tempheat / 100, mostheat / 100));
#endif
								tempheat = 0;
							}
						}
						if(type == CLASS_MECH)
							ii += (MechWeapons[weapindx].criticals - 1);
					} else if(IsAmmo(temp)) {
						if(PartIsNonfunctional(mech, i, ii)
						   || !GetPartData(mech, i, ii))
							continue;
#if 0
/* NULLTODO : Port any of these techs ASAP */
						mul =
							((temp2 =
							  GetPartAmmoMode(mech, i,
											  ii)) & AC_AP_MODE ? 4 : temp2 &
							 AC_PRECISION_MODE ? 6 : temp2 & (TRACER_MODE |
															  STINGER_MODE |
															  SWARM_MODE |
															  SWARM1_MODE |
															  SGUIDED_MODE) ?
							 1.5 : 1);
#else
						mul =
							((temp2 =
							  GetPartAmmoMode(mech, i,
											  ii)) & AC_AP_MODE ? 4 : temp2 &
							 AC_PRECISION_MODE ? 6 : temp2 & (SWARM_MODE |
															  SWARM1_MODE |
															  STINGER_MODE) ?
							 1.5 : 1);
#endif
						mul =
							(mul *
							 ((float)
							  ((float) GetPartData(mech, i, ii) /
							   (float) MechWeapons[weapindx =
												   Ammo2WeaponI(temp)].
							   ammoperton)));

#ifdef DEBUG_BV
						SendDebug(tprintf
								  ("AmmoBVmul (%s) : %.2f",
								   MechWeapons[weapindx].name, mul));
#endif

						if(MechWeapons[weapindx].special & AMS) {
							defweapbv += (debug1 =
										  (((MechWeapons[weapindx].
											 battlevalue / 10) * 100) * mul) *
										  (float) ((float) 3000 /
												   (float) (MechWeapons
															[weapindx].vrt *
															100)));

#ifdef DEBUG_BV
							SendDebug(tprintf
									  ("AmmoDefWeapBVadd (%s) : %d - Total : %d",
									   MechWeapons[weapindx].name,
									   debug1 / 100, defweapbv / 100));
#endif

						} else {

#ifdef DEBUG_BV
							SendDebug(tprintf
									  ("Abattlebalue (%s) : %d",
									   MechWeapons[weapindx].name,
									   (MechWeapons[weapindx].battlevalue /
										10)));
#endif

							offweapbv += (debug1 =
										  (((MechWeapons[weapindx].
											 battlevalue / 10) * 100) * mul) *
										  (float) ((float) 3000 /
												   (float) (MechWeapons
															[weapindx].vrt *
															100)));

#ifdef DEBUG_BV
							SendDebug(tprintf
									  ("AmmoOffWeapBVadd (%s)  : %d - Total : %d",
									   MechWeapons[weapindx].name,
									   debug1 / 100, offweapbv / 100));
#endif

						}
					}
					if((IsAmmo(temp)
						|| (IsWeapon(temp)
							&& MechWeapons[(Weapon2I(temp))].special & GAUSS))
					   && type == CLASS_MECH) {
						if(mechspec & CLAN_TECH)
							if(i == CTORSO || i == HEAD || i == RLEG
							   || i == LLEG) {

#ifdef DEBUG_BV
								SendDebug("20 deduct added for ammo");
#endif
								deduct += 2000;
								continue;
							}
						if(mechspec &
						   (XL_TECH | XXL_TECH | ICE_TECH | LE_TECH)) {

#ifdef DEBUG_BV
							SendDebug("20/2000 deduct added for ammo");
#endif

							deduct += 2000;
							continue;
						}
						if((i == CTORSO || i == RLEG || i == LLEG
							|| i == HEAD)
						   && !(MechSections(mech)[i].config & CASE_TECH)) {

#ifdef DEBUG_BV
							SendDebug("20 deduct added for ammo");
#endif

							deduct += 2000;
							continue;
						}
						if((i == RARM || i == LARM)
						   && (!(MechSections(mech)[i].config & CASE_TECH)
							   &&
							   !(MechSections(mech)
								 [(i ==
								   RARM ? RTORSO : LTORSO)].
								 config & CASE_TECH))) {

#ifdef DEBUG_BV
							SendDebug("20 deduct added for ammo");
#endif

							deduct += 2000;
							continue;
						}
					}

				}
			}
			if(type == CLASS_MECH) {
				mostheat +=
					(MechJumpSpeed(mech) >
					 0 ? MAX((MechJumpSpeed(mech) / MP1) * 100, 300) : 200);
				if(mechspec2 & (NULLSIGSYS_TECH | STEALTH_ARMOR_TECH))
					mostheat += 1000;
				if((temp = (mostheat - (MechActiveNumsinks(mech) * 100))) > 0) {
					deduct += temp * 5;
#ifdef DEBUG_BV
					SendDebug(tprintf
							  ("Deduct add for heat : %d", (temp * 5) / 100));
#endif
				}
			}
#ifdef DEBUG_BV
			SendDebug(tprintf("DeductTotal : %d", deduct / 100));
#endif

			if(mechspec & ECM_TECH)
				defweapbv += 6100;

			if(mechspec & BEAGLE_PROBE_TECH) {
				if(mechspec & CLAN_TECH)
					offweapbv += 1200;
				else
					offweapbv += 1000;
			}
#if 0
/* NULLTODO : Port any of these techs ASAP */
			if(mechspec2 & HDGYRO_TECH)
				defweapbv += 3000;
#endif

			if(mechspec & (XL_TECH | XXL_TECH | LE_TECH)) {
				if(mechspec & (CLAN_TECH | LE_TECH))
					mul = 1.125;
				else
					mul = 0.75;
			} else if(mechspec & ICE_TECH
					  || MechType(mech) == CLASS_VEH_GROUND
					  || MechType(mech) == CLASS_VEH_NAVAL) {
				mul = 0.5;
			} else {
				mul = 1.5;
			}

#ifdef DEBUG_BV
			SendDebug(tprintf("InternMul : %.2f", mul));
#endif

			armor = (armor * (MechType(mech) == CLASS_MECH ? 2 : 1));
			intern = intern * mul;
			mul = 1.00;

#ifdef DEBUG_BV
			SendDebug(tprintf
					  ("ArmorEnd : %d IntEnd : %d", armor / 100,
					   intern / 100));
#endif

			maxspeed = MMaxSpeed(mech);
			if(mechspec & MASC_TECH || mechspec2 & SUPERCHARGER_TECH) {
				if(mechspec & MASC_TECH && mechspec2 & SUPERCHARGER_TECH)
					maxspeed = maxspeed * 2.5;
				else
					maxspeed = maxspeed * 1.5;
			}
			if(mechspec & TRIPLE_MYOMER_TECH)
				maxspeed = ((WalkingSpeed(maxspeed) + MP1) * 1.5);

			if(maxspeed <= MP2) {
				mul = 1.0;
			} else if(maxspeed <= MP4) {
				mul = 1.1;
			} else if(maxspeed <= MP6) {
				mul = 1.2;
			} else if(maxspeed <= MP9) {
				mul = 1.3;
			} else if(maxspeed <= MP1 * 13) {
				mul = 1.4;
			} else if(maxspeed <= MP1 * 18) {
				mul = 1.5;
			} else if(maxspeed <= MP1 * 24) {
				mul = 1.6;
			} else {
				mul = 1.7;
			}

			if(IsDS(mech))
				mul = 1.0;
			else if(is_aero(mech))
				mul = 1.1;

			if(mechspec2 & (NULLSIGSYS_TECH | STEALTH_ARMOR_TECH))
				mul += 1.5;
			if(MechInfantrySpecials(mech) & DC_KAGE_STEALTH_TECH)
				mul += .75;
			if(MechInfantrySpecials(mech) & FWL_ACHILEUS_STEALTH_TECH)
				mul += 1.5;
			if(MechInfantrySpecials(mech) & CS_PURIFIER_STEALTH_TECH)
				mul += 2.0;
			if(MechInfantrySpecials(mech) & FC_INFILTRATOR_STEALTH_TECH)
				mul += .75;
			if(MechInfantrySpecials(mech) & FC_INFILTRATOR_STEALTH_TECH)
				mul += 2.0;

#ifdef DEBUG_BV
			SendDebug(tprintf("DefBVMul : %.2f", mul));
#endif

			defbv = (armor + intern + (MechTons(mech) * 100) + defweapbv);

#ifdef DEBUG_BV
			SendDebug(tprintf("DefBV Tonnage added : %d", MechTons(mech)));
#endif

			if((defbv - deduct) < 1)
				defbv = 1;
			else
				defbv -= deduct;
			if(type != CLASS_MECH)
				defbv =
					((defbv *
					  (move == MOVE_TRACK ? 0.8 : move ==
					   MOVE_WHEEL ? 0.7 : move == MOVE_HOVER ? 0.6 : move ==
					   MOVE_VTOL ? 0.4 : move == MOVE_FOIL || move == MOVE_SUB
					   || move == MOVE_HULL ? 0.5 : 1.0)) - deduct);
			defbv = defbv * mul;

#ifdef DEBUG_BV
			SendDebug(tprintf("DefBV : %d", defbv / 100));
#endif

			if((type == CLASS_MECH || is_aero(mech))
			   && mostheat > (MechActiveNumsinks(mech) * 100)) {
#ifdef DEBUG_BV
				SendDebug(tprintf
						  ("Pre-Heat OffWeapBV : %d", offweapbv / 100));
#endif
				i = (((MechActiveNumsinks(mech) / 100) * offweapbv) /
					 mostheat);
				ii = ((offweapbv - i) / 2);
				offweapbv = i + ii;

#ifdef DEBUG_BV
				SendDebug(tprintf
						  ("Post-Heat OffWeapBV : %d", offweapbv / 100));
#endif
			}
/*
mul = pow(((((MMaxSpeed(mech) / MP1) + (type == CLASS_AERO || type == CLASS_DS ? 0 : (MechJumpSpeed(mech) / MP1)) + (mechspec & MASC_TECH ? 1 : 0) + (mechspec & TRIPLE_MYOMER_TECH ? 1 : 0)+ (mechspec2 & SUPERCHARGER_TECH ? 1 : 0) - 5) / 10) + 1), 1.2);
*/
			mul =
				pow((((((IsDS(mech) ? WalkingSpeed(MMaxSpeed(mech)) :
						 MMaxSpeed(mech)) / MP1) +
					   (mechspec & MASC_TECH ? 1 : 0) +
					   (mechspec & TRIPLE_MYOMER_TECH ? 1 : 0) +
					   (mechspec2 & SUPERCHARGER_TECH ? 1 : 0) - 5) / 10) +
					 1), 1.2);

#ifdef DEBUG_BV
			SendDebug(tprintf("DumbMul : %.2f", mul));
#endif

			if(mechspec2 & OMNIMECH_TECH)
				mul += .3;

			offweapbv = offweapbv * mul;
			if(type != CLASS_AERO && type != CLASS_DS
			   && MechJumpSpeed(mech) > 0)
				offweapbv +=
					((MechJumpSpeed(mech) / MP1) *
					 (100 * (MechTons(mech) / 5)));
			offbv = offweapbv;

#ifdef DEBUG_BV
			SendDebug(tprintf("OffWeapBVAfter : %d", offweapbv / 100));
			SendDebug(tprintf
					  ("DefBV : %d OffBV : %d TotalBV : %d", defbv / 100,
					   offbv / 100, (offbv + defbv) / 100));
#endif

			mul = (skillmul[LAZY_SKILLMUL(gunskl)][LAZY_SKILLMUL(pilskl)]);

#ifdef DEBUG_BV
			SendDebug(tprintf
					  ("SkillMul : %.2f (%d/%d)", mul, gunskl, pilskl));
#endif
			return ((offbv + defbv) / 100) * mul;
		}
#endif

		int MechFullNoRecycle(MECH * mech, int num) {
			int i;

			for(i = 0; i < NUM_SECTIONS; i++) {
				if(num & CHECK_WEAPS && SectHasBusyWeap(mech, i))
					return 1;
				if(num & CHECK_PHYS && MechSections(mech)[i].recycle > 0)
					return 2;
			}
			return 0;
		}

#ifdef BT_COMPLEXREPAIRS
		int GetPartMod(MECH * mech, int t) {
			int val, div, bound;

			div = (t && t == Special(GYRO) ? 100 : t
				   && t == Special(ENGINE) ? 20 : 10);
			bound = (t && t == Special(GYRO) ? 3 : t
					 && t == Special(ENGINE) ? 19 : 9);
			val = (t
				   && (t == Special(GYRO)
					   || t ==
					   Special(ENGINE)) ? MechEngineSize(mech) :
				   MechTons(mech));

			if(val % div != 0)
				val = val + (div - (val % div));

			return BOUNDED(0, (val / div) - 1, bound);
		}

		int ProperArmor(MECH * mech) {
/* For now they all use the same basic cargo parts. */
			return Cargo(MechSpecials(mech) & FF_TECH ? FF_ARMOR :
						 MechSpecials(mech) & HARDA_TECH ? HD_ARMOR :
						 MechSpecials2(mech) & HVY_FF_ARMOR_TECH ?
						 HVY_FF_ARMOR : MechSpecials2(mech) & LT_FF_ARMOR_TECH
						 ? LT_FF_ARMOR : MechSpecials2(mech) &
						 STEALTH_ARMOR_TECH ? STH_ARMOR : S_ARMOR);
		}

		int ProperInternal(MECH * mech) {
			int part = 0;

			if(mudconf.btech_complexrepair) {
				part = (MechSpecials(mech) & ES_TECH ? TON_ESINTERNAL_FIRST :
						MechSpecials(mech) & REINFI_TECH ?
						TON_REINTERNAL_FIRST : MechSpecials(mech) & COMPI_TECH
						? TON_COINTERNAL_FIRST : TON_INTERNAL_FIRST);
				part += GetPartMod(mech, 0);
			} else {
				part = (MechSpecials(mech) & ES_TECH ? ES_INTERNAL :
						MechSpecials(mech) & REINFI_TECH ? RE_INTERNAL :
						MechSpecials(mech) & COMPI_TECH ? CO_INTERNAL :
						S_INTERNAL);
			}
			return Cargo(part);
		}

		int alias_part(MECH * mech, int t, int loc) {
			int part = 0;

			if(!IsSpecial(t))
				return t;

			if(mudconf.btech_complexrepair) {
				int tonmod = GetPartMod(mech, t);
				int locmod;
				if(MechIsQuad(mech))
					locmod = (loc == RARM || loc == LARM || loc == RLEG
							  || loc == LLEG ? 2 : 0);
				else
					locmod = (loc == RARM || loc == LARM ? 1 : loc == LLEG
							  || loc == RLEG ? 2 : 0);

				part = (locmod
						&& (t == Special(SHOULDER_OR_HIP)
							|| t == Special(UPPER_ACTUATOR)) ? (locmod ==
																1 ?
																Cargo
																(TON_ARMUPPER_FIRST
																 +
																 tonmod) :
																Cargo
																(TON_LEGUPPER_FIRST
																 +
																 tonmod)) :
						locmod
						&& t == Special(LOWER_ACTUATOR) ? (locmod ==
														   1 ?
														   Cargo
														   (TON_ARMLOWER_FIRST
															+
															tonmod) :
														   Cargo
														   (TON_LEGLOWER_FIRST
															+
															tonmod)) : locmod
						&& t == Special(HAND_OR_FOOT_ACTUATOR) ? (locmod ==
																  1 ?
																  Cargo
																  (TON_ARMHAND_FIRST
																   +
																   tonmod) :
																  Cargo
																  (TON_LEGFOOT_FIRST
																   +
																   tonmod)) :
						t == Special(ENGINE)
						&& MechSpecials(mech) & XL_TECH ?
						Cargo(TON_ENGINE_XL_FIRST + tonmod) : t ==
						Special(ENGINE)
						&& MechSpecials(mech) & ICE_TECH ?
						Cargo(TON_ENGINE_ICE_FIRST + tonmod) : t ==
						Special(ENGINE)
						&& MechSpecials(mech) & CE_TECH ?
						Cargo(TON_ENGINE_COMP_FIRST + tonmod) : t ==
						Special(ENGINE)
						&& MechSpecials(mech) & XXL_TECH ?
						Cargo(TON_ENGINE_XXL_FIRST + tonmod) : t ==
						Special(ENGINE)
						&& MechSpecials(mech) & LE_TECH ?
						Cargo(TON_ENGINE_LIGHT_FIRST + tonmod) : t ==
						Special(ENGINE) ? Cargo(TON_ENGINE_FIRST +
												tonmod) : t ==
						Special(HEAT_SINK)
						&& MechSpecials(mech) & (DOUBLE_HEAT_TECH | CLAN_TECH)
						? Cargo(DOUBLE_HEAT_SINK) : t == Special(HEAT_SINK)
						&& MechSpecials2(mech) & COMPACT_HS_TECH ?
						Cargo(COMPACT_HEAT_SINK) : t == Special(GYRO)
						&& MechSpecials2(mech) & XLGYRO_TECH ?
						Cargo(TON_XLGYRO_FIRST + tonmod) : t == Special(GYRO)
						&& MechSpecials2(mech) & HDGYRO_TECH ?
						Cargo(TON_HDGYRO_FIRST + tonmod) : t == Special(GYRO)
						&& MechSpecials2(mech) & CGYRO_TECH ?
						Cargo(TON_CGYRO_FIRST + tonmod) : t ==
						Special(GYRO) ? Cargo(TON_GYRO_FIRST + tonmod) : t ==
						Special(SENSORS) ? Cargo(TON_SENSORS_FIRST +
												 tonmod) : t ==
						Special(JUMP_JET) ? Cargo(TON_JUMPJET_FIRST +
												  tonmod) : t);
			} else {
				part = (IsActuator(t) ? Cargo(S_ACTUATOR) :
						t == Special(ENGINE)
						&& MechSpecials(mech) & XL_TECH ? Cargo(XL_ENGINE) : t
						== Special(ENGINE)
						&& MechSpecials(mech) & ICE_TECH ? Cargo(IC_ENGINE) :
						t == Special(ENGINE)
						&& MechSpecials(mech) & CE_TECH ? Cargo(COMP_ENGINE) :
						t == Special(ENGINE)
						&& MechSpecials(mech) & XXL_TECH ? Cargo(XXL_ENGINE) :
						t == Special(ENGINE)
						&& MechSpecials(mech) & LE_TECH ? Cargo(LIGHT_ENGINE)
						: t == Special(HEAT_SINK)
						&& MechSpecials(mech) & (DOUBLE_HEAT_TECH | CLAN_TECH)
						? Cargo(DOUBLE_HEAT_SINK) : t == Special(HEAT_SINK)
						&& MechSpecials2(mech) & COMPACT_HS_TECH ?
						Cargo(COMPACT_HEAT_SINK) : t == Special(GYRO)
						&& MechSpecials2(mech) & XLGYRO_TECH ? Cargo(XL_GYRO)
						: t == Special(GYRO)
						&& MechSpecials2(mech) & HDGYRO_TECH ? Cargo(HD_GYRO)
						: t == Special(GYRO)
						&& MechSpecials2(mech) & CGYRO_TECH ? Cargo(COMP_GYRO)
						: t);
			}
			return part;
		}

		int ProperMyomer(MECH * mech) {
			int part;

			part =
				(MechSpecials(mech) & TRIPLE_MYOMER_TECH ?
				 TON_TRIPLEMYOMER_FIRST : TON_MYOMER_FIRST);
			part += GetPartMod(mech, 0);

			return Cargo(part);
		}
#endif

/* Function to return a value of how much heat a unit is putting out*/
/* TODO: Double check how Stealth Armor and Null Sig are coded */
		int HeatFactor(MECH * mech) {

			int factor = 0;
			char buf[LBUF_SIZE];

			if(MechType(mech) != CLASS_MECH) {
				factor = (((MechSpecials(mech) & ICE_TECH)) ? -1 : 21);
				return factor;
			} else {
				factor =
					(MechPlusHeat(mech) +
					 (2 * (MechPlusHeat(mech) - MechMinusHeat(mech))));
				return ((NullSigSysActive(mech) || HasWorkingECMSuite(mech)
						 || StealthArmorActive(mech)) ? -1 : factor);
			}
			snprintf(buf, LBUF_SIZE,
					 "HeatFactor : Invalid heat factor calculation on #%d.",
					 mech->mynum);
			SendDebug(buf);
		}

/* Function to determine if a weapon is functional or not
   Returns 0 if fully functional.
   Returns 1 if non functional.
   Returns 2 if fully damaged.
   Returns -(# of crits) if partially damaged.
   remember that values 3 means the weapon IS NOT destroyed.  */
		int WeaponIsNonfunctional(MECH * mech, int section, int crit,
								  int numcrits) {
			int sum = 0, disabled = 0, dested = 0;

			if(numcrits <= 0)
				numcrits =
					GetWeaponCrits(mech,
								   Weapon2I(GetPartType
											(mech, section, crit)));

			while (sum < numcrits) {
				if(PartIsDestroyed(mech, section, crit + sum))
					dested++;
				else if(PartIsDisabled(mech, section, crit + sum))
					disabled++;
				sum++;
			}

			if(disabled > 0)
				return 1;

			if((numcrits == 1 && (dested || disabled)) ||
			   (numcrits > 1 && (dested + disabled) >= numcrits / 2))
				return 2;

			if(dested)
				return 0 - (dested + disabled);

			return 0;
		}
