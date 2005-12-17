/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 * Last modified: Fri Dec 11 00:59:48 1998 fingon
 *
 * Original authors: 
 *   4.8.93- rdm created
 *   6.16.93- jb modified, added hex_struct
 * Since that modified by:
 *   '95 - '97: Markus Stenberg <fingon@iki.fi>
 *   '98 - '02: Thomas Wouters <thomas@xs4all.net>
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>
#include <time.h>

#include "mech.h"
#include "mech.events.h"
#include "create.h"
#include "glue.h"
#include "spath.h"
#include "autopilot.h"
#include "p.mech.maps.h"
#include "p.mechfile.h"
#include "p.mech.utils.h"
#include "p.mech.build.h"
#include "p.map.obj.h"
#include "p.mech.sensor.h"
#include "p.spath.h"
#include "p.debug.h"


void debug_fixmap(dbref player, void *data, char *buffer)
{
    MAP *m = (MAP *) data;
    int i, k;
    MECH *mek;

    if (!m)
	return;
    notify_printf(player, "Checking %d entries..", m->first_free);
    DOLIST(k, Contents(m->mynum)) {
	if (Hardcode(k)) {
	    if (WhichSpecial(k) == GTYPE_MECH) {
		MECH *mek;

		/* Check if it's on the map */
		for (i = 0; i < m->first_free; i++)
		    if (m->mechsOnMap[i] == k)
			break;
		if (i != m->first_free)
		    continue;
		mek = getMech(k);
		mek->mapindex = -1;	/* Eep. */
		mek->mapnumber = 0;
	    }
	}
    }
    for (i = 0; i < m->first_free; i++)
	if ((k = m->mechsOnMap[i]) >= 0) {
	    if (!IsMech(k)) {
		notify_printf(player,
		    "Error: #%d isn't mech yet is in mapindex. Fixing..",
			k);
		m->mechsOnMap[i] = -1;
	    } else if (!(mek = getMech(k))) {
		notify_printf(player,
		    "Error: #%d has no mech data. Removing..", k);
		m->mechsOnMap[i] = -1;
	    } else if (mek->mapindex != m->mynum) {
		notify_printf(player,
		    "Error: #%d isn't really here! Removing..",
			k);
		m->mechsOnMap[i] = -1;
	    } else if (mek->mapnumber != i) {
		notify_printf(player,
		    "Error: #%d has invalid mapnumber (mn:%d <-> real:%d)..",
			k, mek->mapnumber, i);
	    }
	}
    notify(player, "Done.");
}




/* Selectors */
#define SPECIAL_FREE 0
#define SPECIAL_ALLOC 1

/* Displays a map to player when they use the VIEW <X> <Y> command
 * with a Map Object */
void map_view(dbref player, void *data, char *buffer)
{
    MAP *mech_map = (MAP *) data;
    int argc, i;
    int x, y;
    char *args[2];
    int displayHeight = MAP_DISPLAY_HEIGHT, displayWidth = MAP_DISPLAY_WIDTH;
    char *str;
    char **maptext;

    /* Check if its a valid map */
    if (!mech_map)
	    return;

    /* Make sure the proper number of arguments '<X> <Y>' were passed */
    argc = mech_parseattributes(buffer, args, 2);
    switch (argc) {
        case 2:
	        x = BOUNDED(0, atoi(args[0]), mech_map->map_width - 1);
	        y = BOUNDED(0, atoi(args[1]), mech_map->map_height - 1);
	        break;
        default:
	        notify(player, "Invalid number of parameters!");
	        return;
    }

    /* Get the Tacsize attribute from
     * the player, if doesn't exist set the height and width to
     * default params. If it does exist, check the values and
     * make sure they are legit. */
    str = silly_atr_get(player, A_TACSIZE);
    if (!*str) {
	    displayHeight = MAP_DISPLAY_HEIGHT;
        displayWidth = MAP_DISPLAY_WIDTH;
    } else if (sscanf(str, "%d %d", &displayHeight, &displayWidth) != 2 || 
            displayHeight > 24 || displayHeight < 5 || displayWidth < 5 ||
            displayWidth > 40) {

	    notify(player, 
                "Illegal Tacsize attribute. Must be in format "
                "'Height Width' . Height : 5-24 Width : 5-40");
	    displayHeight = MAP_DISPLAY_HEIGHT;
        displayWidth = MAP_DISPLAY_WIDTH;

    }

    /* Everything worked but lets check the map size */
    displayHeight = (displayHeight <= mech_map->map_height)
	    ? displayHeight : mech_map->map_height;
    displayWidth = (displayWidth <= mech_map->map_width)
        ? displayWidth : mech_map->map_width;

    /* Get the map data */
    maptext = MakeMapText(player, NULL, mech_map, x, y, displayWidth,
	    displayHeight, 3, 0);

    /* Display the map to the player */
    for (i = 0; maptext[i]; i++)
	    notify(player, maptext[i]);
}

void map_addhex(dbref player, void *data, char *buffer)
{
    MAP *map;
    int x, y, argc;
    char *args[4], elev;

    map = (MAP *) data;
    if (!CheckData(player, map))
	return;
    argc = mech_parseattributes(buffer, args, 4);
    DOCHECK(argc != 4, "Invalid number of arguments!");
    x = atoi(args[0]);
    y = atoi(args[1]);
    elev = abs(atoi(args[3]));
    DOCHECK(!((x >= 0) && (x < map->map_width) && (y >= 0) &&
	    (y < map->map_height)), "X,Y out of range!");
    if (args[2][0] == '.')
	SetTerrain(map, x, y, ' ');
    else
	SetTerrain(map, x, y, args[2][0]);
    SetElevation(map, x, y, (elev <= MAX_ELEV) ? elev : MAX_ELEV);
    notify(player, "Hex set!");
}

void map_mapemit(dbref player, void *data, char *buffer)
{
    MAP *map;

    map = (MAP *) data;
    if (!CheckData(player, map))
	return;
    while (*buffer == ' ')
	buffer++;
    DOCHECK(!buffer || !*buffer, "What do you want to @mapemit?");
    MapBroadcast(map, buffer);
    notify(player, "Message sent!");
}

/* Logic: OPPOSITE sides must have water, within r<=3 of each other */

extern int dirs[6][2];

int water_distance(MAP * map, int x, int y, int dir, int max)
{
    int i;
    int x2, y2;

    for (i = 1; i < max; i++) {
	x = x + dirs[dir][0];
	y = y + dirs[dir][1];
	if (!x && dirs[dir][0])
	    y--;
	x2 = BOUNDED(0, x, map->map_width - 1);
	y2 = BOUNDED(0, y, map->map_height - 1);
	if (x != x2 || y != y2)
	    return max;
	if (GetTerrain(map, x, y) == WATER || GetTerrain(map, x, y) == ICE)
	    return i;
	if (GetTerrain(map, x, y) != BRIDGE &&
	    GetTerrain(map, x, y) != ROAD)
	    return max;
    }
    return max;
}

static int eligible_bridge_hex(MAP * map, int x, int y)
{
    int i, j, k;

    for (k = 0; k < 3; k++)
	if ((i = water_distance(map, x, y, k, 4)) < 4)
	    if ((j = water_distance(map, x, y, k + 3, 4)) < 4) {
		if ((i - j) > 3)
		    continue;
		return 1;
	    }
    return 0;
}

/* Convert some of the roads to bridges */

static void make_bridges(MAP * map)
{
    int x, y;

    for (x = 0; x < map->map_width; x++)
	for (y = 0; y < map->map_height; y++)
	    if (GetTerrain(map, x, y) == ROAD)
		if (eligible_bridge_hex(map, x, y))
		    SetTerrainBase(map, x, y, BRIDGE);
}

int map_load(MAP * map, char * mapname)
{
    char openfile[50];
    char terr, elev;
    int i1, i2, i3;
    FILE *fp;
    char row[MAPX * 2 + 3];
    int i, j = 0, height, width, filemode;

    if (strlen(mapname) >= MAP_NAME_SIZE)
	mapname[MAP_NAME_SIZE] = 0;
    sprintf(openfile, "%s/%s", MAP_PATH, mapname);
    fp = my_open_file(openfile, "r", &filemode);
    if (!fp) {
	return -1;
    }
    del_mapobjs(map);		/* Just in case */
    if (map->map) {
	for (i = 0; i < map->map_height; i++)
	    free((char *) (map->map[i]));
	free((char *) (map->map));
    }
    if (fscanf(fp, "%d %d\n", &height, &width) != 2 || height < 1 ||
	height > MAPY || width < 1 || width > MAPX) {
	SendError(tprintf("Map #%d: Invalid height and/or width",
			  map->mynum));
	width = DEFAULT_MAP_WIDTH;
	height = DEFAULT_MAP_HEIGHT;
    }
    Create(map->map, unsigned char *, height);

    for (i = 0; i < height; i++)
	Create(map->map[i], unsigned char, width);

    for (i = 0; i < height; i++) {
	if (feof(fp)
	    || fgets(row, 2 * MAPX + 2, fp) == NULL ||
	    strlen(row) < (2 * width)) {
	    break;
	}
	for (j = 0; j < width; j++) {
	    terr = row[2 * j];
	    elev = row[2 * j + 1] - '0';
	    switch (terr) {
	    case FIRE:
		map->flags |= MAPFLAG_FIRES;
		break;
	    case TFIRE:
	    case SMOKE:
	    case '.':
		terr = GRASSLAND;
		break;
	    case '\'':
		terr = LIGHT_FOREST;
		break;
	    }
	    if (!strcmp(GetTerrainName_base(terr), "Unknown")) {
		SendError(tprintf("Map #%d: Invalid terrain at %d,%d: '%c'",
				  map->mynum, j, i, terr));
		terr = GRASSLAND;
	    }
	    SetMap(map, j, i, terr, elev);
	}
    }
    if (i != height) {
	SendError(tprintf("Error: EOF reached prematurely. "
			  "(x%d != %d || y%d != %d)", j, width, i, height));
	my_close_file(fp, &filemode);
	return -2;
    }
    map->grav = 100;
    map->temp = 20;
    if (!feof(fp)) {
	if (fscanf(fp, "%d: %d %d\n", &i1, &i2, &i3) == 3) {
	    map->flags = i1;
	    map->grav = i2;
	    map->temp = i3;
	}
    }
    map->map_height = height;
    map->map_width = width;
    if (!MapNoBridgify(map))
        make_bridges(map);
    sprintf(map->mapname, mapname);
    my_close_file(fp, &filemode);
    return 0;
}

void map_loadmap(dbref player, void *data, char *buffer)
{
    MAP *map;
    char *args[1];

    map = (MAP *) data;

    if (!CheckData(player, map))
	return;

    DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
	"Invalid number of arguments!");
    notify_printf(player, "Loading %s", args[0]);
    switch (map_load(map, args[0])) {
    case -1:
	notify(player, "Map not found.");
	return;
    case -2:
	notify(player, "Map invalid.");
	return;
    case 0:
	notify(player, "Map loaded.");
	break;
    default:
	notify(player, "Unknown error while loading map!");
	return;
    }
	
    if (player != 1) {
	notify(player, "Clearing Mechs off Newly Loaded Map");
	map_clearmechs(player, data, "");
    }
}

mapobj *find_mapobj(MAP * map, int x, int y, int type);

void map_savemap(dbref player, void *data, char *buffer)
{
    MAP *map;
    char *args[1];
    FILE *fp;
    char openfile[50];
    int i, j;
    char row[MAPX * 2 + 1];
    char terrain;
    int filemode;

    map = (MAP *) data;

    if (!CheckData(player, map))
	return;

    DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
	"Invalid number of arguments!");
    if (strlen(args[0]) >= MAP_NAME_SIZE)
	args[MAP_NAME_SIZE] = 0;
    notify_printf(player, "Saving %s", args[0]);
    sprintf(openfile, "%s/", MAP_PATH);
    strcat(openfile, args[0]);
    DOCHECK(!(fp =
	    my_open_file(openfile, "w", &filemode)),
	"Unable to open the map file!");
    fprintf(fp, "%d %d\n", map->map_height, map->map_width);
    for (i = 0; i < map->map_height; i++) {
	mapobj *mo;

	row[0] = 0;
	for (j = 0; j < map->map_width; j++) {
	    terrain = GetTerrain(map, j, i);
	    switch (terrain) {
	    case ' ':
		terrain = '.';
		break;
	    case FIRE:
		/* check if we're burnin', if so, alter terrain type */
		if ((mo = find_mapobj(map, j, i, TYPE_FIRE)))
		    terrain = TFIRE;
		else if (!(map->flags & MAPFLAG_FIRES)) {
		    SetTerrain(map, j, i, ' ');
		    SendEvent(tprintf
			("[lost?] fire event noticed on map #%d (%s) at %d,%d",
			    map->mynum, map->mapname, j, i));
		    terrain = '.';
		}
		break;
	    case SMOKE:
		terrain = GetRTerrain(map, j, i);
		if (terrain == ' ')
		    terrain = '.';
		if (terrain == SMOKE) {
		    SetTerrain(map, j, i, ' ');
		    SendEvent(tprintf
			("[lost?] smoke event noticed on map #%d (%s) at %d,%d",
			    map->mynum, map->mapname, j, i));
		    terrain = '.';
		}
		break;
	    }
	    row[j * 2] = terrain;
	    row[j * 2 + 1] = GetElevation(map, j, i) + '0';
	}
	row[j * 2] = 0;
	fprintf(fp, "%s\n", row);
    }
    if ((i = (map->flags & ~(MAPFLAG_MAPO))))
	fprintf(fp, "%d: %d %d\n", i, map->grav, map->temp);
    notify(player, "Saving complete!");
    my_close_file(fp, &filemode);
}

void map_setmapsize(dbref player, void *data, char *buffer)
{
    MAP *oldmap;
    unsigned char **map;
    int x, y, i, j, failed = 0, argc, x1, y1;
    char *args[4];

    oldmap = (MAP *) data;
    if (!CheckData(player, oldmap))
	return;
    DOCHECK(oldmap->mapobj[TYPE_BITS],
	"Invalid map for size change, sorry.");
    DOCHECK((argc =
	    mech_parseattributes(buffer, args, 4)) != 2,
	"Invalid number of arguments (X/Y expected)");
    x = atoi(args[0]);
    y = atoi(args[1]);
    DOCHECK(!((x >= 0) && (x <= MAPX) && (y >= 0) &&
	    (y <= MAPY)), "X,Y out of range!");
    /* allocate new map space */
    Create(map, unsigned char *, y);
    for (i = 0; i < y; i++)
	Create(map[i], unsigned char, x);

    if (failed)
	SendError("Memory allocation failed in setmapsize!");
    else {
	/* Initialize the hexes in the new map to blank */
	for (i = 0; i < y; i++)
	    for (j = 0; j < x; j++)
		SetMapB(map, j, i, ' ', 0);
	/* Copy old map into new map */
	x1 = (oldmap->map_width < x) ? oldmap->map_width : x;
	y1 = (oldmap->map_height < y) ? oldmap->map_height : y;
	for (i = 0; i < y1; i++)
	    for (j = 0; j < x1; j++)
		SetMapB(map, j, i, GetTerrain(oldmap, j, i),
		    GetElevation(oldmap, j, i));
	/* Now free the old map */
	for (i = oldmap->map_height - 1; i >= 0; i--)
	    free((char *) (oldmap->map[i]));
	del_mapobjs(oldmap);
	/* set new map size and pointer to new map space */
	oldmap->map_height = y;
	oldmap->map_width = x;
	oldmap->map = map;
	notify(player, "Size set.");
    }
}

void map_clearmechs(dbref player, void *data, char *buffer)
{
    MAP *map;

    map = (MAP *) data;
    if (CheckData(player, map))
	ShutDownMap(player, map->mynum);
}

extern void update_LOSinfo(dbref, MAP *);

void map_update(dbref obj, void *data)
{
    MAP *map = ((MAP *) data);
    MECH *mech;
    char *tmps, changemsg[LBUF_SIZE] = "";
    int ma, ml, wind, wspeed, cloudbase = 200;
    int oldl, oldv, i, j;
    AUTO *au;
   
/* Changed from % 25 to % 60. %60 never hit when muxevent_tick came here and was odd.
   % 25 should hit when its odd or even (25 75 125... when odd 50 100 150... when even */

    if (!(muxevent_tick % 25)) {
	oldl = map->maplight;
	oldv = map->mapvis;
	if (!(tmps = silly_atr_get(obj, A_MAPVIS)) ||
	    sscanf(tmps, "%d %d %d %d %d %[^\n]", &ma, &ml, &wind,
	        &wspeed, &cloudbase, changemsg) < 4) {
	    ma = 30;
	    ml = 2;
	    wind = 0;
	    wspeed = 0;
	    cloudbase = 200;
	}
	map->winddir = wind;
	map->windspeed = wspeed;
	map->mapvis = BOUNDED(0, ma, 60);
	map->maxvis = BOUNDED(24, ma * 3, 60);
	map->maplight = BOUNDED(0, ml, 2);
	map->cloudbase = cloudbase;
	if (ml != oldl || ma != oldv)
	    for (i = 0; i < map->first_free; i++) {
		if ((j = map->mechsOnMap[i]) < 0)
		    continue;
		if (!(mech = getMech(j)))
		    continue;
		if (ml != oldl)
		    sensor_light_availability_check(mech);
		if (strlen(changemsg) > 5)
		    mech_notify(mech, MECHALL, changemsg);
		if (MechAuto(mech) > 0)
		    if ((au = FindObjectsData(MechAuto(mech))))
			if (Gunning(au))
			    UpdateAutoSensor(au, 0);
	    }
    }
    if (map->moves) {
	update_LOSinfo(obj, map);
	map->moves = 0;
    }
    /* Fire/Smoke are event-driven -> nothing related to them done here */
}

void initialize_map_empty(MAP * new, dbref key)
{
    int i, j;

    new->mynum = key;
    new->map_width = DEFAULT_MAP_WIDTH;
    new->map_height = DEFAULT_MAP_HEIGHT;
    Create(new->map, unsigned char *, new->map_height);

    for (i = 0; i < new->map_height; i++)
	Create(new->map[i], unsigned char, new->map_width);

    for (i = 0; i < new->map_height; i++)
	for (j = 0; j < new->map_width; j++)
	    SetMap(new, j, i, ' ', 0);
}

/* Mem alloc/free routines */
void newfreemap(dbref key, void **data, int selector)
{
    MAP *new = *data;
    int i;

    switch (selector) {
    case SPECIAL_ALLOC:
	initialize_map_empty(new, key);
	/* allocate default map space */
	for (i = 0; i < NUM_MAPOBJTYPES; i++)
	    new->mapobj[i] = NULL;
	sprintf(new->mapname, "%s", "Default Map");
	break;
    case SPECIAL_FREE:
	del_mapobjs(new);
	if (new->map) {
	    for (i = new->map_height - 1; i >= 0; i--)
		if (new->map[i])
		    free((char *) (new->map[i]));
	    free((char *) (new->map));
	}
	break;
    }
}

int map_sizefun(void *data, int flag)
{
    MAP *map = (MAP *) data;
    int size = 0;

    if (!map)
	return 0;
    size = sizeof(*map);
    if (map->map)
	size += sizeof(*map->map);
    return size;
}

void map_listmechs(dbref player, void *data, char *buffer)
{
    MAP *map;
    MECH *tempMech;
    int i;
    int count = 0;
    char valid[50];
    char *ID;
    char *args[2];
    char *cmds[] = { "MECHS", "OBJS", NULL };
    enum {
	MECHS, OBJS
    };

    map = (MAP *) data;

    if (!CheckData(player, map))
	return;
    DOCHECK(mech_parseattributes(buffer, args, 1) == 0,
	"Supply target type too!");
    switch (listmatch(cmds, args[0])) {
    case MECHS:
	notify(player, "--- Mechs on Map ---");
	for (i = 0; i < map->first_free; i++) {
	    if (map->mechsOnMap[i] != -1) {
		tempMech = getMech(map->mechsOnMap[i]);
		ID = MechIDS(tempMech, 0);
		if (tempMech)
		    strcpy(valid, "Valid Data");
		else
		    strcpy(valid,
			"Invalid Object Data!  Remove this Mech!");
		notify_printf(player, "Mech DB Number: %d : [%s]\t%s",
			map->mechsOnMap[i], ID, valid);
		count++;
	    }
	}
	notify_printf(player, "%d Mechs On Map", count);
	notify_printf(player, "%d positions open",
		MAX_MECHS_PER_MAP - count);
	if (count != map->first_free)
	    notify_printf(player,
		"%d is first free slot, according to db.",
		    map->first_free);
	return;
	break;
    case OBJS:
	list_mapobjs(player, map);
	return;
	break;
    }
    notify_printf(player, "Invalid argument (%s)!", args[0]);
    return;
}

void clear_hex(MECH * mech, int x, int y, int meant)
{
    MAP *map;

    if (!(map = getMap(mech->mapindex)))
	return;
    switch (GetTerrain(map, x, y)) {
    case HEAVY_FOREST:
	SetTerrain(map, x, y, LIGHT_FOREST);
	break;
    case LIGHT_FOREST:
	if (Number(1, 2) == 1)
	    SetTerrain(map, x, y, ROUGH);
	else
	    SetTerrain(map, x, y, GRASSLAND);
	break;
    default:
	return;
    }
    if (meant) {
	MechLOSBroadcast(mech, tprintf("'s shot clears %d,%d!", x, y));
	mech_printf(mech, MECHALL, "You clear %d,%d.", x, y);
    } else {
	MechLOSBroadcast(mech, tprintf("'s stray shot clears %d,%d!", x,
		y));
	mech_printf(mech, MECHALL,
	    "You accidentally clear the %d,%d!", x, y);
    }
}

MAP *spath_map;

#define readval(f,t)  DOCHECK(readint(f, t), "Invalid argument!")

void map_pathfind(dbref player, void *data, char *buffer)
{
    MAP *map = (MAP *) data;
    char *args[6];
    int argc;
    int x1, y1, x2, y2;
    time_t start_t;
    int errper = -1;

    if (!map)
	return;
    argc = mech_parseattributes(buffer, args, 6);
    DOCHECK((argc < 4 || argc > 5), "Invalid arguments!");
    readval(x1, args[0]);
    readval(y1, args[1]);
    readval(x2, args[2]);
    readval(y2, args[3]);
    if (argc > 4) {
	readval(errper, args[4]);
	errper = BOUNDED(0, errper, 1000);
    }
    spath_map = map;
    start_t = time(NULL);
}

#undef readval

void UpdateMechsTerrain(MAP * map, int x, int y, int t)
{
    MECH *mech;
    int i;

    for (i = 0; i < map->first_free; i++) {
	if (!(mech = FindObjectsData(map->mechsOnMap[i])))
	    continue;
	if (MechX(mech) != x || MechY(mech) != y)
	    continue;
	if (MechTerrain(mech) != t) {
	    MechTerrain(mech) = t;
	    MarkForLOSUpdate(mech);
	}
    }
}
