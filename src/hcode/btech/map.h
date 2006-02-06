
/*
 * $Id: map.h,v 1.2 2005/06/22 15:08:04 av1-op Exp $
 *
 * Last modified: Sat Jun  6 20:30:40 1998 fingon
 *
 * Original authors: 
 *   4.8.93- rdm created
 *   6.16.93- jb modified, added hex_struct
 * Since that modified by:
 *   '95 - '97: Markus Stenberg <fingon@iki.fi>
 *   '98 - '02: Thomas Woutes <thomas@xs4all.net>
 */

#ifndef _MAP_H
#define _MAP_H

#include "rbtree.h"

/*
   map.h
   Structure definitions and what not for the maps for the mechs.

 */

#define MAX_MECHS_PER_MAP 250

/* map links */
#define MAP_UP 0
#define MAP_DOWN 1
#define MAP_RIGHT 2
#define MAP_LEFT 3

/* Map size */
#define MAPX 1000
#define MAPY 1000
#define MAP_NAME_SIZE 30
#define NUM_MAP_LINKS 4
#define DEFAULT_MAP_WIDTH 21
#define DEFAULT_MAP_HEIGHT 11
#define MAP_DISPLAY_WIDTH 21
#define MAP_DISPLAY_HEIGHT 14 
#define MAX_ELEV 9
#define GRASSLAND ' '
#define HEAVY_FOREST '"'
#define LIGHT_FOREST '`'
#define WATER '~'
#define HIGHWATER '?'
#define ROUGH '%'
#define MOUNTAINS '^'
#define ROAD '#'
#define BUILDING '@'
#define FIRE '&'
#define TFIRE '>'
#define SMOKE ':'
#define WALL '='

#define BRIDGE '/'
#define SNOW   '+'
#define ICE    '-'

#define UNKNOWN_TERRAIN '$'

/*
 * Various Map flags, for use for setting different affects on the map
 */
#define MAPFLAG_MAPO            1       /* (a) We got mapobjs */
#define MAPFLAG_SPEC            2       /* (b) We're using special rules - gravity/temp */
#define MAPFLAG_VACUUM          4       /* (c) We're in vacuum */
#define MAPFLAG_FIRES           8       /* (d) We have eternal fires */
#define MAPFLAG_UNDERGROUND     16      /* (e) We're underground. No ejecting, jumping, 
                                           VTOL taking off */
#define MAPFLAG_DARK            32      /* (f) We can't see map beyond sensor range */
#define MAPFLAG_BRIDGESCS       64      /* (g) We can't destroy bridges on this map */
#define MAPFLAG_NOBRIDGIFY      128     /* (h) We shouldn't convert roads into bridges */
#define MAPFLAG_NOFRIENDLYFIRE  256     /* (i) We can't shoot friendlies AT ALL on this map */

#define TYPE_FIRE  0		/* Fire - datas = counter until next spread, datac = stuff to burn */
#define TYPE_SMOKE 1		/* Smoke - datas = time until it gets lost */
#define TYPE_DEC   2		/* Decoration, like those 2 previous ones. obj = obj# of DS it is related to, datac = char it replaced */
#define TYPE_LAST_DEC 2
#define TYPE_MINE  3		/* datac = type, datas = damage it causes, datai = extra */
#define TYPE_BUILD 4		/* Building obj=# of the internal map */
#define TYPE_LEAVE 5		/* Reference to what happens when U leave ; obj=# of new map */
#define TYPE_ENTRANCE 6		/* datac = dir of entry (0=dontcare), x/y */

#define TYPE_LINKED  7		/* If this exists, we got a maplink propably */
#define TYPE_BITS    8		/* hangar / mine bit array, if any (in datai) */
#define TYPE_B_LZ    9		/* Land-block */
#define NUM_MAPOBJTYPES 10

#define BUILDFLAG_CS  1		/* Externally CS */
#define BUILDFLAG_CSI 2		/* Internally CS */
#define BUILDFLAG_DSS 4		/* DontShowStep when someone steps on the base */
#define BUILDFLAG_NOB 8		/* No way to break in */
#define BUILDFLAG_HID 16	/* Really hidden */

#define MapIsCS(map)       (map->buildflag & BUILDFLAG_CSI)
#define BuildIsCS(map)     (map->buildflag & BUILDFLAG_CS)
#define BuildIsHidden(map) (map->buildflag & (BUILDFLAG_DSS|BUILDFLAG_HID))
#define BuildIsSafe(map)   (map->buildflag & BUILDFLAG_NOB)
#define BuildIsInvis(map)  (map->buildflag & BUILDFLAG_HID)

#define MapUnderSpecialRules(map)   ((map)->flags & MAPFLAG_SPEC)
#define MapGravityMod(map)          (map)->grav
#define MapGravity                  MapGravityMod
#define MapIsVacuum(map)            ((map)->flags & MAPFLAG_VACUUM)
#define MapTemperature(map)         (map)->temp
#define MapCloudbase(map)           (map)->cloudbase
#define MapIsUnderground(map)       ((map)->flags & MAPFLAG_UNDERGROUND)
#define MapIsDark(map)              ((map)->flags & MAPFLAG_DARK)
#define MapBridgesCS(map)           ((map)->flags & MAPFLAG_BRIDGESCS)
#define MapNoBridgify(map)          ((map)->flags & MAPFLAG_NOBRIDGIFY)
#define MapNoFriendlyFire(map)      ((map)->flags & MAPFLAG_NOFRIENDLYFIRE)

#define MECHMAPFLAG_MOVED  1	/* mech has moved since last LOS update */

#define MECHLOSFLAG_SEEN   0x0001	/* x <mech> has seen <target> (before) */
#define MECHLOSFLAG_SEESP  0x0002	/* x <mech> sees <target> now w/ primary */
#define MECHLOSFLAG_SEESS  0x0004	/* x <mech> sees <target> now w/ secondary */
#define MECHLOSFLAG_SEEC2  0x0008	/* The terrain flag has been calculated */
#define MECHLOSFLAG_MNTN   0x0010	/* Intervening mountain hexes? */

#define MECHLOSFLAG_WOOD   0x0020	/* The number of intervening woods hexes */
#define MECHLOSFLAG_WOOD2  0x0040	/* Heavywood = 2, Light = 1 */
#define MECHLOSFLAG_WOOD3  0x0080
#define MECHLOSFLAG_WOOD4  0x0100
#define MECHLOSBYTES_WOOD  4
#define MECHLOSMAX_WOOD    16	/* No sensors penetrate this far */

#define MECHLOSFLAG_WATER  0x0200	/* The number of intervening water hexes */
#define MECHLOSFLAG_WATER2 0x0400
#define MECHLOSFLAG_WATER3 0x0800
#define MECHLOSBYTES_WATER 3
#define MECHLOSMAX_WATER   8	/* No sensors penetrate this far */

#define MECHLOSFLAG_PARTIAL 0x1000	/* The target's in partial cover for
					   da shooter */
#define MECHLOSFLAG_FIRE    0x2000	/* Fire between mech and target */
#define MECHLOSFLAG_SMOKE   0x4000	/* Smoke between mech and target */
#define MECHLOSFLAG_BLOCK   0x8000	/* Something blocks vision 
					   (elevation propably) */

typedef struct hex_t {
    int terrain;
    int elevation;
    int flags;
} hex;

typedef struct mapobj_struct {
    int x, y;
    dbref obj;
    int type;
    int datac;
    int datas;
    int datai;
    struct mapobj_struct *next;
} mapobj;

typedef struct {
    dbref mynum;                        /* My dbref */

    //unsigned char **map;                /* The map */
    /* The map using a single array that used to take a 2-D array */
    hex *hexes;

    char mapname[MAP_NAME_SIZE + 1];

    int width;                          /* Width of map <MAPX  */
    int height;                         /* Height of map */

    int temp;                           /* Temperature, in celsius degrees */
    unsigned int grav;                  /* Gravity, if any ; in 1/100 G's */
    int cloudbase;
    int mapvis;                         /* Visibility on the map, used as base 
                                           for most sensor types */
    int maxvis;                         /* maximum visibility (usually mapvis * n) */
    int maplight;
    int winddir, windspeed;

    /* Now, da wicked stuff */
    int flags;

    dbref onmap;
    int cf, cfmax;
    int buildflag;

    unsigned int first_free;            /* First free on da map */
    dbref *mechsOnMap;                  /* Mechs on the map */
    rbtree *mechs;
    unsigned int **LOSinfo;             /* Line of sight info */
    rbtree *LOSinfotree;

    /* 1 = mech has moved recently
       2 = mech has possible-LOS event ongoing */
    char *mechflags;
    int moves;                          /* Cheat to prevent idle CPU hoggage */
    int movemod;
    int sensorflags;

    /* Map objects like mines, links etc */
    mapobj *mapobj[NUM_MAPOBJTYPES];
    rbtree *mapobjs[NUM_MAPOBJTYPES];
} MAP;

/* Used by navigate_sketch_map */
#define NAVIGATE_LINES 13

#include "p.map.bits.h"
#include "p.map.h"
#include "p.map.obj.h"
#include "p.map.dynamic.h"

extern void newfreemap(dbref key, void **data, int selector);
extern void map_update(dbref obj, void *data);

#define set_buildflag(a,b) silly_atr_set((a), A_BUILDFLAG, tprintf("%d", (b)))
#define get_buildflag(a) atoi(silly_atr_get((a), A_BUILDFLAG))
#define set_buildcf(a,b) silly_atr_set((a), A_BUILDCF, tprintf("%d %d", (b), (b)))
#endif
