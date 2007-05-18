/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 */

#include "mech.h"
#include "mech.events.h"
#include "p.mech.update.h"
#include "p.bsuit.h"
#include "p.mech.utils.h"
#include "p.mech.pickup.h"
#include "p.mech.tag.h"

#define TMP_TERR '1'

static void swim_except(MAP * map, MECH * mech, int x, int y, char *msg,
						int isbridge)
{
	int i, j;
	MECH *t;

	if(!(Elevation(map, x, y)))
		return;
	for(i = 0; i < map->first_free; i++) {
		j = map->mechsOnMap[i];
		if(j < 0)
			continue;
		t = getMech(j);
		if(!t || t == mech)
			continue;
		if(MechX(t) != x || MechY(t) != y)
			continue;
		MechTerrain(t) = WATER;
		if((!isbridge && (MechZ(t) == 0) && (MechMove(t) != MOVE_HOVER)) ||
		   (isbridge && MechZ(t) == MechElev(t))) {
			MechLOSBroadcast(t, msg);
			MechFalls(t, MechElev(t) + isbridge, 0);
			if(MechType(t) == CLASS_VEH_GROUND && !Destroyed(t)) {
				mech_notify(t, MECHALL,
							"Water renders your vehicle inoperable.");
				MechLOSBroadcast(t,
								 "fizzles and pops as water renders it inoperable.");
				DestroyMech(t,t, 1, KILL_TYPE_FLOOD);
			}
		}
	}
}

static void break_sub(MAP * map, MECH * mech, int x, int y, char *msg)
{
	int isbridge = GetRTerrain(map, x, y) == BRIDGE;

	SetTerrain(map, x, y, WATER);
	if(isbridge)
		SetElevation(map, x, y, 1);
	swim_except(map, mech, x, y, msg, isbridge);
}

/* Up -> down */
void drop_thru_ice(MECH * mech)
{
	MAP *map = FindObjectsData(mech->mapindex);

	mech_notify(mech, MECHALL, "You break the ice!");
	MechLOSBroadcast(mech, "breaks the ice!");
	if(MechMove(mech) != MOVE_FOIL) {
		if(MechElev(mech) > 0)
			MechLOSBroadcast(mech, "vanishes into the waters!");
	}
	break_sub(map, mech, MechX(mech), MechY(mech), "goes swimming!");
	MechTerrain(mech) = WATER;
	if(MechMove(mech) != MOVE_FOIL) {
		if(MechElev(mech) > 0)
			MechFalls(mech, MechElev(mech), 0);
	}
	if(MechElev(mech) > 0 && MechType(mech) == CLASS_VEH_GROUND &&
	   !Destroyed(mech) && !(MechSpecials2(mech) & WATERPROOF_TECH)) {
		mech_notify(mech, MECHALL, "Water renders your vehicle inoperable.");
		MechLOSBroadcast(mech,
						 "fizzles and pops as water renders it inoperable.");
		DestroyMech(mech, mech, 1, KILL_TYPE_ICE);
	}
}

/* Down -> up */
void break_thru_ice(MECH * mech)
{
	MAP *map = FindObjectsData(mech->mapindex);

	MarkForLOSUpdate(mech);
	mech_notify(mech, MECHALL, "You break through the ice!");
	MechLOSBroadcast(mech, "breaks through the ice!");
	break_sub(map, mech, MechX(mech), MechY(mech), "goes swimming!");
	MechTerrain(mech) = WATER;
}

/* CHANCE of dropping thru the ice based on 'mech weight */
int possibly_drop_thru_ice(MECH * mech)
{
	if((MechMove(mech) == MOVE_HOVER) || (MechMove(mech) == MOVE_SUB) ||
	   (MechType(mech) == CLASS_BSUIT))
		return 0;
	if(Number(1, 6) != 1)
		return 0;
	drop_thru_ice(mech);
	return 1;
}

static int watercount;

static void growable_callback(MAP * map, int x, int y)
{
	int terrain = GetRTerrain(map, x, y);
	if((IsWater(terrain) && terrain != ICE) ||
	   GetRTerrain(map, x, y) == TMP_TERR)
		watercount++;
}

int growable(MAP * map, int x, int y)
{
	watercount = 0;
	visit_neighbor_hexes(map, x, y, growable_callback);

	if(watercount <= 4 && (watercount < 2 || (Number(1, 6) > watercount)))
		return 1;
	return 0;
}

static void meltable_callback(MAP * map, int x, int y)
{
	if(GetRTerrain(map, x, y) == ICE)
		watercount++;
}

int meltable(MAP * map, int x, int y)
{
	watercount = 0;
	visit_neighbor_hexes(map, x, y, meltable_callback);

	if(watercount > 4 && Number(1, 3) > 1)
		return 0;
	return 1;
}

void ice_growth(dbref player, MAP * map, int num)
{
	int x, y;
	int count = 0;

	for(x = 0; x < map->map_width; x++)
		for(y = 0; y < map->map_height; y++)
			if(GetRTerrain(map, x, y) == WATER)
				if(Number(1, 100) <= num && growable(map, x, y)) {
					SetTerrain(map, x, y, TMP_TERR);
					count++;
				}
	for(x = 0; x < map->map_width; x++)
		for(y = 0; y < map->map_height; y++)
			if(GetRTerrain(map, x, y) == TMP_TERR)
				SetTerrain(map, x, y, ICE);
	if(count)
		notify_printf(player, "%d hexes 'iced'.", count);
	else
		notify(player, "No hexes 'iced'.");
}

void ice_melt(dbref player, MAP * map, int num)
{
	int x, y;
	int count = 0;

	for(x = 0; x < map->map_width; x++)
		for(y = 0; y < map->map_height; y++)
			if(GetRTerrain(map, x, y) == ICE)
				if(Number(1, 100) <= num && meltable(map, x, y)) {
					break_sub(map, NULL, x, y,
							  "goes swimming as ice breaks!");
					SetTerrain(map, x, y, TMP_TERR);
					count++;
				}
	for(x = 0; x < map->map_width; x++)
		for(y = 0; y < map->map_height; y++)
			if(GetRTerrain(map, x, y) == TMP_TERR)
				SetTerrain(map, x, y, WATER);
	if(count)
		notify_printf(player, "%d hexes melted.", count);
	else
		notify(player, "No hexes melted.");
}

void map_addice(dbref player, MAP * map, char *buffer)
{
	char *args[2];
	int num;

	DOCHECK(mech_parseattributes(buffer, args, 2) != 1, "Invalid arguments!");
	DOCHECK(Readnum(num, args[0]), "Invalid number!");
	ice_growth(player, map, num);
}

void map_delice(dbref player, MAP * map, char *buffer)
{
	char *args[2];
	int num;

	DOCHECK(mech_parseattributes(buffer, args, 2) != 1, "Invalid arguments!");
	DOCHECK(Readnum(num, args[0]), "Invalid number!");
	ice_melt(player, map, num);
}

void possibly_blow_ice(MECH * mech, int weapindx, int x, int y)
{
	MAP *map = FindObjectsData(mech->mapindex);

	if(GetRTerrain(map, x, y) != ICE)
		return;
	if(Number(1, 15) > MechWeapons[weapindx].damage)
		return;
	HexLOSBroadcast(map, x, y, "The ice breaks from the blast!");
	break_sub(map, NULL, x, y, "goes swimming as ice breaks!");
}

void possibly_blow_bridge(MECH * mech, int weapindx, int x, int y)
{
	MAP *map = FindObjectsData(mech->mapindex);

	if(GetRTerrain(map, x, y) != BRIDGE)
		return;
	if(MapBridgesCS(map))
		return;
	if(Number(1, 10 * (1 + GetElev(map, x,
								   y))) > MechWeapons[weapindx].damage) {
		HexLOSBroadcast(map, x, y,
						"The bridge at $H shudders from direct hit!");
		return;
	}
	HexLOSBroadcast(map, x, y, "The bridge at $H is blown apart!");
	break_sub(map, NULL, x, y, "goes swimming as the bridge is blown apart!");
}
