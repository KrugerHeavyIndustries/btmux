
/*
 * $Id: mech.los.c,v 1.1.1.1 2005/01/11 21:18:17 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Last modified: Sat Jul 18 13:47:27 1998 fingon
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "p.map.obj.h"
#include "p.mech.sensor.h"
#include "p.mech.los.h"
#include "p.mech.utils.h"

/* 'nice' sensor stuff's in the mech.sensor.c ; nasty brute code
   lies here */

/* from here onwards.. black magic happens. Enter if you're sure of
   your peace of mind :-P */

/* -------------------------------------------------------------------- */

float ActualElevation(MAP * map, int x, int y, MECH * mech)
{

	if(!map)
		return 0.0;
	if(!mech)
		return (float) (Elevation(map, x, y) + 0.1);
	if(MechType(mech) == CLASS_MECH && !Fallen(mech))
		return (float) MechZ(mech) + 1.5;
	else if(MechMove(mech) == MOVE_NONE)
		return (float) MechZ(mech) + 1.5;
	else if(IsDS(mech))
		return (float) MechZ(mech) + 2.5 + (MechType(mech) ==
											CLASS_DS ? 0 : 2);
	if(MechDugIn(mech))
		return (float) MechZ(mech) + 0.1;
	return (float) MechZ(mech) + 0.5;
}

extern int TraceLOS(MAP * map, int ax, int ay, int bx, int by,
					lostrace_info ** result);

/* from/mech: mech _mech_ seeing _target_ on map _map_, _ff_
   is the previous flag (or seeing _x_,_y_ if _target_ is NULL),
   hexRange is range in hexes */
int CalculateLOSFlag(MECH * mech, MECH * target, MAP * map, int x, int y,
					 int ff, float hexRange)
{
	int new_flag = (ff & (MECHLOSFLAG_SEEN)) + MECHLOSFLAG_SEEC2;
	int woods_count = 0;
	int water_count = 0;
	int height, i;
	int pos_x, pos_y;
	float pos_z, z_inc, end_z;
	int terrain;
	int dopartials = 0;
	int underwater, bothworlds, t_underwater, t_bothworlds;
	int uwatercount = 0, coordcount;
	lostrace_info *coords;

#ifndef BT_PARTIAL
	float partial_z, p_z_inc;
#endif

	/* A Hex target off the map? Don't bother */
	if(!target && ((x < 0 || y < 0 || x >= map->map_width ||
					y >= map->map_height)))
		return new_flag + MECHLOSFLAG_BLOCK;

	/* Outside max sensor range in the worst case? Don't bother. */
	if(hexRange > (((MechSpecials(mech) & AA_TECH) || (target &&
													   (MechSpecials(target) &
														AA_TECH))) ? 180 :
				   map->maxvis))
		return new_flag + MECHLOSFLAG_BLOCK;

	/* We start at (MechX(mech), MechY(mech)) and wind up at (x,y) */
	pos_x = MechX(mech);
	pos_y = MechY(mech);
	pos_z = ActualElevation(map, pos_x, pos_y, mech);
	end_z = ActualElevation(map, x, y, target);

/* Definition of 'both worlds': According to FASA, if a mech is half
   submerged, or a sub is surfaced, or any naval or hover is on top
   of the water, it can see in both the underwater and overwater 'worlds'.
   In other words, it'll never get a block from the water/air interface.
   Neither will anything get such a block against it. That's what the
   'both worlds' variables test for. */

	if(end_z > 10 && pos_z > 10)
		return new_flag;
	bothworlds = IsWater(MechRTerrain(mech)) &&	/* Can we be in both worlds? */
		(((MechType(mech) == CLASS_MECH) && (MechZ(mech) == -1)) ||
		 ((WaterBeast(mech)) && (MechZ(mech) == 0)) ||
		 ((MechMove(mech) == MOVE_HOVER) && (MechZ(mech) == 0)));
	underwater = InWater(mech) && (pos_z < 0.0);

	/* Ice hex targeting special case */
	if(!target && !underwater && GetRTerrain(map, x, y) == ICE)
		end_z = 0.0;

	if(target) {
		/* What about him? Both worlds? Or flat out underwater? */
		t_bothworlds = IsWater(MechRTerrain(target)) &&
			(((MechType(target) == CLASS_MECH) && (MechZ(target) == -1)) ||
			 ((WaterBeast(target)) && (MechZ(target) == 0)) ||
			 ((MechMove(target) == MOVE_HOVER) && (MechZ(target) == 0)));

		t_underwater = InWater(target) && (end_z < 0.0);
	} else {
		if(GetRTerrain(map, x, y) == ICE)
			t_bothworlds = 1;
		else
			t_bothworlds = 0;
		t_underwater = (end_z < 0.0);	/* Is the hex underwater? */
	}

	/* And now we look once more to make sure we aren't wasting our time */
	if(((underwater) && !(t_underwater)) ||
	   ((t_underwater) && !(underwater))) {
		return new_flag + MECHLOSFLAG_BLOCK;
	}

	/* Worth our time to mess with figuring partial cover? */
	if(target && mech)
		dopartials = (MechType(target) == CLASS_MECH) && (!Fallen(target));

	/*Same hex is always LoS */
	if((x == pos_x) && (y == pos_y))
		return new_flag;

	/* Special cases are out of the way, looks like we have to do actual work. */
	coordcount = TraceLOS(map, pos_x, pos_y, x, y, &coords);
	if(coordcount > 0) {
		z_inc = (float) (end_z - pos_z) / coordcount;
	} else {
		z_inc = 0;				/* In theory, this should never happen. */
	}

#ifndef BT_PARTIAL
	partial_z = 0;
	p_z_inc = (float) 1 / coordcount;
#endif

	if(coordcount > 0) {		/* not in same hex ; in same hex, you see always */
		for(i = 0; i < coordcount; i++) {
			pos_z += z_inc;
#ifndef BT_PARTIAL
			partial_z += p_z_inc;
#endif
			if(coords[i].x < 0 || coords[i].x >= map->map_width ||
			   coords[i].y < 0 || coords[i].y >= map->map_height)
				continue;
			/* Should be possible to see into water.. perhaps. But not
			   on vislight */
			terrain = GetRTerrain(map, coords[i].x, coords[i].y);
			/* get the current height */
			height = Elevation(map, coords[i].x, coords[i].y);

/* If you, persoanlly, are underwater, the only way you can see someone
   if if they are underwater or in both worlds AND your LoS passes thru
   nothing but water hexes AND your LoS doesn't go thru the sea floor */
			if(underwater) {

/* LoS hits sea floor */
				if(!(IsWater(terrain)) || (terrain != BRIDGE &&
										   height >= pos_z)) {
					new_flag |= MECHLOSFLAG_BLOCK;
					return new_flag;
				}

/* LoS pops out of water, AND we're not tracing to half-submerged mech's head */
				if(!t_bothworlds && pos_z > 0.0) {
					new_flag |= MECHLOSFLAG_BLOCK;
					return new_flag;
				}

/* uwatercount = # hexes LoS travel UNDERWATER */
				if(pos_z <= 0.0)
					uwatercount++;
				water_count++;
			} else {			/* Viewer is not underwater */
				/* keep track of how many wooded hexes we cross */
				if(pos_z < (height + 2)) {
					switch (GetTerrain(map, coords[i].x, coords[i].y)) {
					case SMOKE:
						if(i < coordcount - 1)
							new_flag |= MECHLOSFLAG_SMOKE;
						break;
					case FIRE:
						if(i < coordcount - 1)
							new_flag |= MECHLOSFLAG_FIRE;
						break;
					}
					switch (terrain) {
					case LIGHT_FOREST:
					case HEAVY_FOREST:
						if(i < coordcount - 1)
							woods_count += (terrain == LIGHT_FOREST) ? 1 : 2;
						break;
					case HIGHWATER:
						water_count++;
						break;
					case ICE:
						if(pos_z < -0.0) {
							new_flag |= MECHLOSFLAG_BLOCK;
							return new_flag;
						}
						water_count++;
						break;
					case WATER:

/* LoS goes INTO water and we're not tracing to a target in both worlds */
						if(!bothworlds && (pos_z < 0.0)) {
							new_flag |= MECHLOSFLAG_BLOCK;
							return new_flag;
						}

/* Hexes in LoS that are phsyically underwater */
						if(pos_z < 0.0)
							uwatercount++;
						water_count++;
						break;
					case MOUNTAINS:
						if(i < coordcount - 1)
							new_flag |= MECHLOSFLAG_MNTN;
						break;
					}
				}
				/* make this the new 'current hex' */
				if(height >= pos_z && terrain != BRIDGE) {
					new_flag |= MECHLOSFLAG_BLOCK;
					return new_flag;
				}
#ifndef BT_PARTIAL
				else if(dopartials && height >= (pos_z - partial_z))
					new_flag |= MECHLOSFLAG_PARTIAL;
#endif
			}
		}
	}
	/* Then, we check the hex before target hex */
#ifdef BT_PARTIAL
/* Sanity Check for Array Boundry */
	if((coordcount - 2) < 0) {
		if(coordcount >= 2)
			if(dopartials) {
				if(MechZ(target) >= MechZ(mech) &&
				   (Elevation(map, coords[coordcount - 2].x,
							  coords[coordcount - 2].y) == (MechZ(target) + 1)))
					new_flag |= MECHLOSFLAG_PARTIAL;
				if(MechZ(target) == -1 && MechRTerrain(target) == WATER)
					new_flag |= MECHLOSFLAG_PARTIAL;
			}
	}
#endif

	water_count = BOUNDED(0, water_count, MECHLOSMAX_WATER - 1);
	woods_count = BOUNDED(0, woods_count, MECHLOSMAX_WOOD - 1);
	new_flag += MECHLOSFLAG_WOOD * woods_count;
	new_flag += MECHLOSFLAG_WATER * water_count;

/* Block EM after 2, Vis/IR after 6 */
	if(uwatercount > 2)
		new_flag |= MECHLOSFLAG_MNTN;
	if(uwatercount > 6)
		new_flag |= MECHLOSFLAG_FIRE;
	return new_flag;
}

int AddTerrainMod(MECH * mech, MECH * target, MAP * map, float hexRange,
				  int wAmmoMode)
{
	/* Possibly do a quickie check only */
	if(mech && target) {
		if(MechToMech_LOSFlag(map, mech, target) & MECHLOSFLAG_PARTIAL)
			MechStatus(target) |= PARTIAL_COVER;
		else
			MechStatus(target) &= ~PARTIAL_COVER;

		return Sensor_ToHitBonus(mech, target,
								 MechToMech_LOSFlag(map, mech, target),
								 map->maplight, hexRange, wAmmoMode);
	}
	return 0;
}

int InLineOfSight_NB(MECH * mech, MECH * target, int x, int y, float hexRange)
{
	int i;

	if(mech == target)
		return 1;

	i = InLineOfSight(mech, target, x, y, hexRange);
	if(i & MECHLOSFLAG_BLOCK)
		return 0;
	return i;
}

int InLineOfSight(MECH * mech, MECH * target, int x, int y, float hexRange)
{
	MAP *map;
	float x1, y1;
	int arc;
	int losflag;

	map = getMap(mech->mapindex);
	if(!map) {
		mech_notify(mech, MECHPILOT,
					"You are on an invalid map! Map index reset!");
		SendError(tprintf("InLineOfSight:invalid map:Mech %d  Index %d",
						  mech->mynum, mech->mapindex));
		mech->mapindex = -1;
		return 0;
	}
	if(x < 0 || y < 0 || y >= map->map_height || x >= map->map_width) {
		SendError(tprintf("x:%d y:%d out of bounds for #%d (LOS check)", x,
						  y, mech ? mech->mynum : -1));
	}

	/* Possibly do a quickie check only */
	if(MechCritStatus(mech) & CLAIRVOYANT)
		return 1;

	if(target) {
		x1 = MechFX(target);
		y1 = MechFY(target);
	} else
		MapCoordToRealCoord(x, y, &x1, &y1);
	arc = InWeaponArc(mech, x1, y1);

	if(mech && target) {
#ifndef ADVANCED_LOS
		losflag = MechToMech_LOSFlag(map, mech, target);
		if(Sensor_CanSee(mech, target, &losflag, arc, hexRange, map->mapvis,
						 map->maplight, map->cloudbase)) {
			map->LOSinfo[mech->mapnumber][target->mapnumber] |=
				(MECHLOSFLAG_SEEN | MECHLOSFLAG_SEESP | MECHLOSFLAG_SEESS);
			return 1;
		} else {
			map->LOSinfo[mech->mapnumber][target->mapnumber] &=
				~(MECHLOSFLAG_SEEN | MECHLOSFLAG_SEESP | MECHLOSFLAG_SEESS);
			return 0;
		}
#else
		if(MechToMech_LOSFlag(map, mech, target) & (MECHLOSFLAG_SEESP |
													MECHLOSFLAG_SEESS))
			return MechToMech_LOSFlag(map, mech, target) &
				(MECHLOSFLAG_SEESP | MECHLOSFLAG_SEESS | MECHLOSFLAG_BLOCK);
#endif
		return 0;
	}
	losflag = CalculateLOSFlag(mech, NULL, map, x, y, 0, hexRange);
	return Sensor_CanSee(mech, NULL, &losflag, arc, hexRange, map->mapvis,
						 map->maplight, map->cloudbase);
}

void mech_losemit(dbref player, MECH * mech, char *buffer)
{
	cch(MECH_USUALSP);
	MechLOSBroadcast(mech, buffer);
	notify(player, "Broadcast done.");
}
