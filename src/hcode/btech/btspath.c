
/*
 * $Id: btspath.c,v 1.1.1.1 2005/01/11 21:18:04 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *       All rights reserved
 *
 * Created: Tue Sep 23 19:49:20 1997 fingon
 * Last modified: Thu Mar  5 18:50:58 1998 fingon
 *
 */

#include "mech.h"

extern MAP *spath_map;

#define XSIZE spath_map->width
#define YSIZE spath_map->height

extern int (*TileCost) (int fx, int fy, int tx, int ty);

int MechTileCost(int fx, int fy, int tx, int ty)
{
	int z1, z2;
	int bonus;
	char terr;

	if(tx < 0 || ty >= XSIZE || ty < 0 || ty >= YSIZE)
		return 0;
	z1 = Elevation(spath_map, fx, fy);
	z2 = Elevation(spath_map, tx, ty);
	if((bonus = abs(z1 - z2)) > 2)
		return 0;
	terr = GetRTerrain(spath_map, tx, ty);
	if(terr == WATER && z2)		/* No waterwalking, yet */
		return 0;
	if(terr == GRASSLAND)
		return 1 + bonus;
	switch (terr) {
	case LIGHT_FOREST:
	case ROUGH:
		bonus++;
		break;
	case HEAVY_FOREST:
	case MOUNTAINS:
		bonus += 2;
		break;
	}
	return bonus + 1;
}

int HoverTankTileCost(int fx, int fy, int tx, int ty)
{
	int z1, z2;
	int bonus;
	char terr;

	if(tx < 0 || ty >= XSIZE || ty < 0 || ty >= YSIZE)
		return 0;
	z1 = Elevation(spath_map, fx, fy);
	z2 = Elevation(spath_map, tx, ty);
	if((bonus = abs(z1 - z2)) > 1)
		return 0;
	terr = GetRTerrain(spath_map, tx, ty);
	if(terr == WATER)			/* Hovers LOVE water */
		return 1;
	if(terr == GRASSLAND)
		return 1 + bonus;
	if(terr == HEAVY_FOREST)
		return 0;
	switch (terr) {
	case LIGHT_FOREST:
	case ROUGH:
		bonus++;
		break;
	case MOUNTAINS:
		bonus += 2;
		break;
	}
	return bonus + 1;
}

int TrackedTankTileCost(int fx, int fy, int tx, int ty)
{
	int z1, z2;
	int bonus;
	char terr;

	if(tx < 0 || ty >= XSIZE || ty < 0 || ty >= YSIZE)
		return 0;
	z1 = Elevation(spath_map, fx, fy);
	z2 = Elevation(spath_map, tx, ty);
	if((bonus = abs(z1 - z2)) > 1)
		return 0;
	terr = GetRTerrain(spath_map, tx, ty);
	if(terr == GRASSLAND)
		return 1 + bonus;
	if(terr == WATER)
		return 0;
	if(terr == HEAVY_FOREST)
		return 0;
	switch (terr) {
	case LIGHT_FOREST:
	case ROUGH:
		bonus++;
		break;
	case HEAVY_FOREST:
	case MOUNTAINS:
		bonus += 2;
		break;
	}
	return bonus + 1;
}
