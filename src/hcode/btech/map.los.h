
/*
 * $Id: map.los.h,v 1.1.1.1 2005/01/11 21:18:08 kstevens Exp $
 *
 * Author: Thomas Wouters <thomas@xs4all.net>
 *
 *  Copyright (c) 2002 Thomas Wouters
 *      All rights reserved
 *
 */

#ifndef _MAP_LOS_H
#define _MAP_LOS_H

#define MAX_SENSORS		2
#define NUMSENSORS(mech)	2

#define MAPLOS_MAXX		70
#define MAPLOS_MAXY		45

#define MAPLOS_FLAG_SLITE	1

#define MAPLOSHEX_NOLOS		0
#define MAPLOSHEX_SEEN		1
#define MAPLOSHEX_SEETERRAIN	2
#define MAPLOSHEX_SEEELEV	4
#define MAPLOSHEX_LIT		8
#define MAPLOSHEX_SEE		(MAPLOSHEX_SEETERRAIN | MAPLOSHEX_SEEELEV)

#define LOSMap_GetFlag(losmap, x, y) \
		((losmap)->map[LOSMap_Hex2Index(losmap, x, y)])

typedef struct hexlosmap_info {
    int startx;
    int starty;
    int xsize;
    int ysize;
    int flags;
    unsigned char map[MAPLOS_MAXX * MAPLOS_MAXY];
} hexlosmap_info;


hexlosmap_info *CalculateLOSMap(MAP *, MECH *, int, int, int, int);
int LOSMap_Hex2Index(hexlosmap_info *, int, int);

#endif
