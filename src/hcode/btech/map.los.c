
/*
 * $Id: map.los.c,v 1.1.1.1 2005/01/11 21:18:08 kstevens Exp $
 * 
 * Author: Thomas Wouters <thomas@xs4all.net>
 *
 * Copyright (c) 2002 Thomas Wouters
 *     All rights reserved
 *
 */

#include "mech.h"
#include "btmacros.h"
#include "mech.sensor.h"
#include "map.los.h"
#include "p.mech.utils.h"

#define INDEX2X(i)		((i%(losmap.xsize))+(losmap.startx))
#define INDEX2Y(i)		((i/(losmap.xsize))+(losmap.starty))

extern int TraceLOS(MAP * map, int ax, int ay, int bx, int by,
					lostrace_info ** result);

static hexlosmap_info losmap;

int LOSMap_Hex2Index(hexlosmap_info * losmap, int x, int y)
{
	if(x < losmap->startx || x > losmap->startx + losmap->xsize ||
	   y < losmap->starty || y > losmap->starty + losmap->ysize) {
		SendError(tprintf("LOSMap request from out of bounds hex: %d,%d",
						  x, y));
		return 0;
	}
	return ((y - losmap->starty) * losmap->xsize) + (x - losmap->startx);
}

static float MechHeight(MECH * mech)
{
	switch (MechType(mech)) {
	case CLASS_MECH:
		return 0.2 + !Fallen(mech);
	case CLASS_SPHEROID_DS:
		return 4.2;
	case CLASS_DS:
		return 2.2;
	case CLASS_MW:
	case CLASS_VEH_NAVAL:
		return 0.01;
	}
	return 0.2;
}

static void set_hexlosinfo(int x, int y, int flag)
{
	if(x < (losmap.startx) || x >= (losmap.startx) + (losmap.xsize) ||
	   y < (losmap.starty) || y >= (losmap.starty) + (losmap.ysize)) {
		return;
	}
	losmap.map[LOSMap_Hex2Index(&losmap, x, y)] |= (flag | MAPLOSHEX_SEEN);
}

static int hexlit(int x, int y)
{
	if(x < (losmap.startx) || x >= (losmap.startx) + (losmap.xsize) ||
	   y < (losmap.starty) || y >= (losmap.starty) + (losmap.ysize)) {
		return 0;
	}

	return (losmap.map[LOSMap_Hex2Index(&losmap, x, y)] & MAPLOSHEX_LIT);
}

static void set_sliteinfo(int x, int y, int flag)
{
	if(x < (losmap.startx) || x >= (losmap.startx) + (losmap.xsize) ||
	   y < (losmap.starty) || y >= (losmap.starty) + (losmap.ysize)) {
		return;
	}
	losmap.flags |= MAPLOS_FLAG_SLITE;
	losmap.map[LOSMap_Hex2Index(&losmap, x, y)] |= flag;
}

/* To efficiently set all hexes NOLOS if neither sensor supports seeing
 * terrain, and (in the future) to set all hexes LOSALL if either sensor
 * sees all terrain (e.g. 'sattelite downlink' sensor)
 */

static void set_hexlosall(int flag)
{
	memset(&(losmap.map), flag | MAPLOSHEX_SEEN, losmap.xsize * losmap.ysize);
}

/* The following functions are, effectively, STUBS. They should be
   replaced with functions in the sensor struct, instead of their
   functionality being copied all over the tree. */

static int MechSeesThroughWoods(MECH * mech, MAP * map, int nwoods,
								int sensor)
{
	int sn = MechSensor(mech)[sensor];
	int fake_losflag = nwoods * MECHLOSFLAG_WOOD;
	int res = sensors[sn].cansee_func(mech, NULL, map, 1, fake_losflag);

	return res;
}

static int MechSeesOverMountain(MECH * mech, MAP * map, int sensor)
{
	int sn = MechSensor(mech)[sensor];
	int fake_losflag = MECHLOSFLAG_MNTN;

	return sensors[sn].cansee_func(mech, NULL, map, 1, fake_losflag);
}

static int MechSeesThroughWater(MECH * mech, MAP * map, int nwater,
								int sensor)
{
	int sn = MechSensor(mech)[sensor];
	int fake_losflag = nwater * MECHLOSFLAG_WATER;

	return sensors[sn].cansee_func(mech, NULL, map, 1, fake_losflag);
}

static int MechSeesRange(MECH * mech, MAP * map, int x, int y, int z,
						 int sensor)
{
	int sn = MechSensor(mech)[sensor];
	float fx, fy, range, maxvis = sensors[sn].maxvis;

	MapCoordToRealCoord(x, y, &fx, &fy);
	range = FindRange(MechFX(mech), MechFY(mech), MechFZ(mech),
					  fx, fy, ZSCALE * z);

	/* XXX HACK: code duplication. this should be replaced with sensor
	 * functions
	 */

	if(sn < 2)					/* V or L sensors */
		maxvis = map->mapvis;
	if(sn == 1 && map->maplight == 0)	/* L sensors in darkness */
		maxvis *= 2;

	if(!sensors[sn].fullvision) {
		int arc = InWeaponArc(mech, fx, fy);

		if(!(arc & (FORWARDARC | TURRETARC))) {
			if(MechSensor(mech)[0] == MechSensor(mech)[1])
				maxvis = (maxvis * 200 / 300);
			else
				maxvis = 0;
		}
	}

	if(sn == 0 && maxvis && range >= maxvis &&
	   (losmap.flags & MAPLOS_FLAG_SLITE))
		return -1;

	return range < maxvis;
}

static int MechSLitesRange(MECH * mech, int x, int y, int z)
{
	float fx, fy, range;
	int arc, maxvis = 60;

	MapCoordToRealCoord(x, y, &fx, &fy);
	arc = InWeaponArc(mech, fx, fy);
	if(!(arc & (FORWARDARC | TURRETARC))) {
		return 0;
	}

	range = FindRange(MechFX(mech), MechFY(mech), MechFZ(mech),
					  fx, fy, ZSCALE * z);
	return range < maxvis;
}

static int MechSeesTerrain(MECH * mech, int sn)
{
	return MechSensor(mech)[sn] < 4;
}

/* General idea: stateful LOS checking.

 * To minimize the number of lostracing we do, we calculate the losmap by
 * tracing los to all 'edge' hexes, and traversing that line of hexes
 * marking each hex as 'seen' and as 'los-or-not'. For each sensor on the
 * 'mech, we keep track of how steep the angle has to be in order for the
 * sensor to 'see' the terrain. The start angle is -20 (which should be low
 * enough for common purposes, even on jumping 'mechs) for sensors that can
 * see terrain, and 1000 for sensors that can't -- basically flagging the
 * whole line of sight as 'not seen' for that sensor.

 * In order to take wood-blockage into account, we also keep track of the
 * minimum 'block' angle. That is, if it is not equal to minangle,
 * blockangle is the angle below which 'woodcount' woods stand between the
 * current hex and the seeing 'mech. If we end up with a hex between
 * minangle and blockangle, we need to check if the sensor can see through
 * that many woods.
 
 * Blocking entirely, because of water- or EM-effects, is done by setting
 * the minangle and blockangle to 1000, a value high enough to block los to
 * all following hexes. To determine whether a sensors sees through a hex,
 * fake losflags are passed to the regular sensor functions... hacks, and
 * logic-duplication (the worst kind) but they work for now.
 
 * This is all proof-of-concept, based on Cord Awtry's ideas for
 * 'underground' maps. This should all be rewritten, together with the
 * sensor code, to have one general 'tracelos' function, which calls
 * callbacks defined on a state struct and stores its state info on that
 * same struct. That way, map-los, 'mech-los, searchlight-los and such can
 * all use the same routine, using different callbacks.

 * Known bugs / problems:
 * - It behaves awkwardly around water. It doesn't handle the transition as
 *   it should. This requires sufficient rewriting that I do not plan to do
 *   it before the whole sensor overhaul.

 * - It has too great a leniency in what terrain height you can see. You can
 *   sometimes see a level 1 hex behind a level 2 hex if you are in a 'mech,
 *   fallen on a level 1 hex. (you shouldn't.)

 */

static void trace_slitelos(MAP * map, MECH * mech, int index,
						   float start_height)
{
	float minangle = -20;
	lostrace_info *trace_coords;
	int trace_range = 0;
	int trace_x, trace_y, trace_height;
	float trace_a;
	int trace_coordnum = TraceLOS(map, MechX(mech), MechY(mech),
								  INDEX2X(index), INDEX2Y(index),
								  &trace_coords);

	for(; trace_range < trace_coordnum; trace_range++) {
		trace_x = trace_coords[trace_range].x;
		trace_y = trace_coords[trace_range].y;

		trace_height = MAX(0, Elevation(map, trace_x, trace_y));

		if(!MechSLitesRange(mech, trace_x, trace_y, trace_height))
			return;

		trace_a = (trace_height - start_height) / (trace_range + 1);
		switch (GetTerrain(map, trace_x, trace_y)) {
		case HEAVY_FOREST:
		case LIGHT_FOREST:
		case SMOKE:
			trace_a += 2;
		}

		if(trace_a < minangle)
			continue;

		set_sliteinfo(trace_x, trace_y, MAPLOSHEX_LIT);
		minangle = trace_a;
	}
}

static void litemark_callback(MAP * map, int x, int y)
{
	set_sliteinfo(x, y, MAPLOSHEX_LIT);
}

static void litemark_map(MAP * map)
{
    MECH *mech;
    dllist_node *node;
    int index;
    mapobj *fire;

    for (fire = first_mapobj(map, TYPE_FIRE); fire; fire = next_mapobj(fire)) {
        set_sliteinfo(fire->x, fire->y, MAPLOSHEX_LIT);
        visit_neighbor_hexes(map, fire->x, fire->y, litemark_callback);
    }

    for (node = dllist_head(map->mechs); node; node = dllist_next(node)) {

        if (!(mech = getMech((int) dllist_data(node))))
            continue;

        if (Jellied(mech)) {
            set_sliteinfo(MechX(mech), MechY(mech), MAPLOSHEX_LIT);
            visit_neighbor_hexes(map, MechX(mech), MechY(mech),
                    litemark_callback);
        }

        if (!MechLites(mech))
            continue;

        for (index = 0; index < losmap.xsize * losmap.ysize; index++) {
            trace_slitelos(map, mech, index, MechZ(mech) + MechHeight(mech));
        }
    }
}

#define DEF_MINA(mech, sn) (MechSeesTerrain(mech, sn) ? -20 : 1000)

static void trace_maphexlos(MAP * map, MECH * mech, int index, int tracew,
							float start_height)
{
	int trace_water[MAX_SENSORS] = { tracew, tracew };
	float minangle[MAX_SENSORS] = { DEF_MINA(mech, 0), DEF_MINA(mech, 1) };
	float blockangle[MAX_SENSORS] = { DEF_MINA(mech, 0), DEF_MINA(mech, 1) };
	int woodcount[MAX_SENSORS] = { 0, 0 };
	int watercount[MAX_SENSORS] = { 0, 0 };
	lostrace_info *trace_coords;
	int trace_range = 0;

	int trace_coordnum = TraceLOS(map, MechX(mech), MechY(mech),
								  INDEX2X(index), INDEX2Y(index),
								  &trace_coords);

	for(; trace_range < trace_coordnum; trace_range++) {
		int seestate;
		int trace_x = trace_coords[trace_range].x;
		int trace_y = trace_coords[trace_range].y;
		int trace_height = Elevation(map, trace_x, trace_y);

		float trace_a = (trace_height - start_height) / (trace_range + 1);
		float trace_ba =
			((trace_height + 2 - start_height)) / (trace_range + 1);
		int trace_terrain = GetRTerrain(map, trace_x, trace_y);
		int nsensor, newwoods;

		for(nsensor = 0; nsensor < NUMSENSORS(mech); nsensor++) {

			/* If the current hex and all its terrain ('blockangle') lies
			 * below our minimum angle of sight, we won't see it at all;
			 * jump straight ahead to the water/mountain checks. This check
			 * is made first, because it is the cheapest check and the
			 * general mechanism to signal 'no more visibility on this line
			 * of sight' is to set trace_ba to an impossible angle.
			 */

			if(trace_ba < minangle[nsensor]) {
				set_hexlosinfo(trace_x, trace_y, MAPLOSHEX_NOLOS);
				goto hexinfluence;

			}

			/* Then we check for range. */
			seestate = MechSeesRange(mech, map, trace_x, trace_y,
									 trace_height, nsensor);

			if(seestate == 0) {
				set_hexlosinfo(trace_x, trace_y, MAPLOSHEX_NOLOS);
				minangle[nsensor] = blockangle[nsensor] = 1000;
				goto hexinfluence;
			}

			/* Count the number of woods. */
			newwoods = 0;
			switch (trace_terrain) {
			case HEAVY_FOREST:
				newwoods++;
				/* FALLTHROUGH */
			case LIGHT_FOREST:
				newwoods++;
				/* Because we aren't in water, we stop tracing below water */
				trace_water[nsensor] = 0;
				break;
			}

			if(!newwoods) {

				if(trace_a < minangle[nsensor] || (seestate < 0 &&
												   !hexlit(trace_x,
														   trace_y))) {
					set_hexlosinfo(trace_x, trace_y, MAPLOSHEX_NOLOS);
				} else {
					set_hexlosinfo(trace_x, trace_y, MAPLOSHEX_SEE);
					blockangle[nsensor] = minangle[nsensor] = trace_a;
					woodcount[nsensor] = 0;
				}
				goto hexinfluence;
			}

			if(blockangle[nsensor] < trace_a) {
				minangle[nsensor] = trace_a;
				blockangle[nsensor] = trace_ba;
				woodcount[nsensor] = newwoods;
			} else if(!MechSeesThroughWoods(mech, map, woodcount[nsensor] +
											newwoods, nsensor)) {
				if(trace_ba >= blockangle[nsensor]) {
					minangle[nsensor] = blockangle[nsensor];
					blockangle[nsensor] = trace_ba;
					woodcount[nsensor] = newwoods;
				} else
					minangle[nsensor] = trace_ba;
			} else {
				minangle[nsensor] = trace_a;
				woodcount[nsensor] += newwoods;
			}

			if(trace_terrain == WATER) {
				if(trace_water[nsensor])
					watercount[nsensor]++;
				if(!trace_water[nsensor] ||
				   !MechSeesThroughWater(mech, map, watercount[nsensor],
										 nsensor)) {
					if(seestate < 0 && !hexlit(trace_x, trace_y))
						set_hexlosinfo(trace_x, trace_y, MAPLOSHEX_NOLOS);
					else
						set_hexlosinfo(trace_x, trace_y,
									   MAPLOSHEX_SEETERRAIN);
				}
			} else if(seestate < 0 && !hexlit(trace_x, trace_y))
				set_hexlosinfo(trace_x, trace_y, MAPLOSHEX_NOLOS);
			else
				set_hexlosinfo(trace_x, trace_y, MAPLOSHEX_SEE);

		  hexinfluence:
			if(trace_terrain == WATER &&
			   !MechSeesThroughWater(mech, map, 1, nsensor)) {
				set_hexlosinfo(trace_x, trace_y, MAPLOSHEX_NOLOS);
				minangle[nsensor] = blockangle[nsensor] = 1000;
				continue;
			}
			trace_water[nsensor] = 0;
			if(trace_terrain == MOUNTAINS &&
			   !MechSeesOverMountain(mech, map, nsensor)) {
				set_hexlosinfo(trace_x, trace_y, MAPLOSHEX_NOLOS);
				minangle[nsensor] = blockangle[nsensor] = 1000;
				continue;
			}
		}
	}
}

hexlosmap_info *CalculateLOSMap(MAP * map, MECH * mech, int sx,
								int sy, int xsz, int ysz)
{
	int index, underterrain, bothworlds;
	float start_height;

	/* Some safeguarding on size */

	if(xsz > MAPLOS_MAXX || ysz > MAPLOS_MAXY) {
		SendError(tprintf("xsize (%d vs %d) or ysize (%d vs %d) "
						  "to CalculateLOSMap too large, for mech #%d",
						  xsz, MAPLOS_MAXX, ysz, MAPLOS_MAXY, mech->mynum));
		return NULL;
	}

	losmap.startx = sx;
	losmap.starty = sy;
	losmap.xsize = xsz;
	losmap.ysize = ysz;
	losmap.flags = 0;
	memset(losmap.map, 0, xsz * ysz);

	underterrain = MechZ(mech) <= -1;
	if(IsWater(MechRTerrain(mech))
	   && ((MechType(mech) == CLASS_MECH && MechZ(mech) == -1)
		   || ((WaterBeast(mech) || MechMove(mech) == MOVE_HOVER)
			   && MechZ(mech) == 0))) {
		bothworlds = 1;
	} else
		bothworlds = 0;

	start_height = MechZ(mech) + MechHeight(mech);

	if(MechCritStatus(mech) & CLAIRVOYANT) {
		set_hexlosall(MAPLOSHEX_SEE);
		return &losmap;
	}

	if(!MechSeesTerrain(mech, 0) && !MechSeesTerrain(mech, 1)) {
		set_hexlosall(MAPLOSHEX_NOLOS);
		return &losmap;
	}

	/* In order for slites to properly light terrain, we have to mark the
	 * losmap with all lit hexes first. Which means going over all 'mechs on
	 * the map and tag all hexes that they light.
	 */

	litemark_map(map);

	/* In order to do the most efficient lostracing, we make losmaps by
	 * first tracing from the 'mech hex to the upper Y-row, the lower Y-row,
	 * the leftmost X-row, the rightmost X-row, and then all hexes starting
	 * at the upper left corner to make sure we have seen all hexes. (It is
	 * entirely possible for a hex not to be visited yet, even if we traced
	 * to every other hex.)
	 */

	for(index = 0; index < xsz; index++) {
		if(losmap.map[index] & MAPLOSHEX_SEEN)
			continue;
		trace_maphexlos(map, mech, index, underterrain || bothworlds,
						start_height);
	}
	for(index = (ysz - 1) * xsz; index < ysz * xsz; index++) {
		if(losmap.map[index] & MAPLOSHEX_SEEN)
			continue;
		trace_maphexlos(map, mech, index, underterrain || bothworlds,
						start_height);
	}
	for(index = xsz; index < ysz * xsz; index += xsz) {
		if(losmap.map[index] & MAPLOSHEX_SEEN)
			continue;
		trace_maphexlos(map, mech, index, underterrain || bothworlds,
						start_height);
	}
	for(index = 2 * xsz - 1; index < ysz * xsz; index += xsz) {
		if(losmap.map[index] & MAPLOSHEX_SEEN)
			continue;
		trace_maphexlos(map, mech, index, underterrain || bothworlds,
						start_height);
	}
	for(index = 0; index < xsz * ysz; index++) {
		if(losmap.map[index] & MAPLOSHEX_SEEN)
			continue;
		trace_maphexlos(map, mech, index, underterrain || bothworlds,
						start_height);
	}

	return &losmap;
}
