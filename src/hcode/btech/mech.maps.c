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
#include "create.h"
#include "mech.events.h"
#include "map.los.h"
#include "p.mech.utils.h"
#include "p.mech.los.h"
#include "p.eject.h"
#include "p.mech.restrict.h"
#include "p.mech.maps.h"
#include "p.mech.notify.h"
#include "p.ds.bay.h"
#include "p.bsuit.h"
#include "p.mech.utils.h"
#include "autopilot.h"

void mech_findcenter(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    float fx, fy;
    int x, y;

    cch(MECH_USUAL);
    x = MechX(mech);
    y = MechY(mech);
    MapCoordToRealCoord(x, y, &fx, &fy);
    notify_printf(player, "Current hex: (%d,%d,%d)\tRange to center: %.2f\t"
			   "Bearing to center: %d", x, y, MechZ(mech),
			   FindHexRange(fx, fy, MechFX(mech), MechFY(mech)),
			   FindBearing(MechFX(mech), MechFY(mech), fx, fy));
}

static int parse_tacargs(dbref player, MECH *mech, char ** args, int argc,
			 int maxrange, short * x, short * y)
{
    int bearing;
    float range, fx, fy;
    MECH * tempMech;
    MAP * map;

    switch (argc) {
    case 2:
	bearing = atoi(args[0]);
	range = atof(args[1]);
	DOCHECK0(!MechIsObservator(mech) &&
	    abs((int) range) > maxrange,
	    "Those coordinates are out of sensor range!");
	FindXY(MechFX(mech), MechFY(mech), bearing, range, &fx, &fy);
	RealCoordToMapCoord(x, y, fx, fy);
	return 1;
    case 1:
	map = getMap(mech->mapindex);
	tempMech = getMech(FindMechOnMap(map, args[0]));
	DOCHECK0(!tempMech, "No such target.");
	range = FlMechRange(mech_map, mech, tempMech);
	DOCHECK0(!InLineOfSight(mech, tempMech, MechX(tempMech),
				MechY(tempMech), range), "No such target.");
	DOCHECK0(abs((int) range) > maxrange,
		 "Target is out of scanner range.");
	*x = MechX(tempMech);
	*y = MechY(tempMech);
	return 1;
    case 0:
	*x = MechX(mech);
	*y = MechY(mech);
	return 1;
    default:
	notify(player, "Invalid number of parameters!");
	return 0;
    }
}

const char *GetTerrainName_base(int t)
{
    switch (t) {
    case GRASSLAND:
    case '_':
	return "Grassland";
    case HEAVY_FOREST:
	return "Heavy Forest";
    case LIGHT_FOREST:
	return "Light Forest";
    case ICE:
	return "Ice";
    case BRIDGE:
	return "Bridge";
    case HIGHWATER:
    case WATER:
	return "Water";
    case ROUGH:
	return "Rough";
    case MOUNTAINS:
	return "Mountains";
    case ROAD:
	return "Road";
    case BUILDING:
	return "Building";
    case FIRE:
	return "Fire";
    case SMOKE:
	return "Smoke";
    case WALL:
	return "Wall";
    }
    return "Unknown";
}

const char *GetTerrainName(MAP * map, int x, int y)
{
    return GetTerrainName_base(GetTerrain(map, x, y));
}

/* Player-customizable colors */

enum { SWATER_IDX, DWATER_IDX, BUILDING_IDX, ROAD_IDX, ROUGH_IDX, MOUNTAIN_IDX,
       FIRE_IDX, ICE_IDX, WALL_IDX, SNOW_IDX, SMOKE_IDX, LWOOD_IDX, HWOOD_IDX,
       UNKNOWN_IDX, CLIFF_IDX, SELF_IDX, FRIEND_IDX, ENEMY_IDX, DS_IDX,
       GOODLZ_IDX, BADLZ_IDX, NUM_COLOR_IDX
};

/* Default colour string is "BbWnYyRWWWXGgbRHYRn" */
/* internal rep has H instead of h and \0 instead of n */

#define DEFAULT_COLOR_STRING "BbWXYyRWWWXGgbRhYRnGR"
#define DEFAULT_COLOR_SCHEME "BbWXYyRWWWXGgbRHYR\0GR"

static char custom_color_str[NUM_COLOR_IDX + 1] = DEFAULT_COLOR_SCHEME;

static void set_colorscheme(dbref player)
{
    char *str = silly_atr_get(player, A_MAPCOLOR);
    int i;
    
    if (*str && strlen(str) <= NUM_COLOR_IDX) {
	strncpy(custom_color_str, DEFAULT_COLOR_STRING, NUM_COLOR_IDX);
	strncpy(custom_color_str, str, strlen(str));
	for (i = 0; i < NUM_COLOR_IDX; i++) {
	    switch (custom_color_str[i]) {
	    case 'f':
	    case 'F':
	    case 'I':
	    case 'i':
	    case 'H':
	    case 'x':
	    case 'X':
	    case 'r':
	    case 'R':
	    case 'g':
	    case 'G':
	    case 'y':
	    case 'Y':
	    case 'b':
	    case 'B':
	    case 'm':
	    case 'M':
	    case 'c':
	    case 'C':
	    case 'w':
	    case 'W':
		break;
	    case 'h':
		custom_color_str[i] = 'H';
		break;
	    case 'n':
		custom_color_str[i] = '\0';
		break;
	    default:
		notify_printf(player, "Invalid character '%c' in MAPCOLOR "
				       "attribute!", custom_color_str[i]);
		notify(player, "Using default: " DEFAULT_COLOR_STRING);
		memcpy(custom_color_str, DEFAULT_COLOR_SCHEME, NUM_COLOR_IDX);
		return;
	    }
	}
	return;
    } else if (*str) {
	notify(player, "Invalid MAPCOLOR attribute!");
	notify(player, "Using default: " DEFAULT_COLOR_STRING);
    }
    memcpy(custom_color_str, DEFAULT_COLOR_SCHEME, NUM_COLOR_IDX);
}


void mech_navigate(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    char mybuff[NAVIGATE_LINES][MBUF_SIZE];
    MAP *mech_map;
    char **maptext, *args[3];
    int i, dolos, argc;
    short x, y;
    
    cch(MECH_USUAL);

    mech_map = getMap(mech->mapindex);

    dolos = MapIsDark(mech_map) || (MechType(mech) == CLASS_MW &&
    				    mudconf.btech_mw_losmap);

    DOCHECK(mech_map->map_width <= 0 || mech_map->map_height <= 0,
	    "Nothing to see on this map, move along.");

    argc = mech_parseattributes(buffer, args, 3);
    if (!parse_tacargs(player, mech, args, argc, MechTacRange(mech), &x, &y))
	return;

    set_colorscheme(player);
    maptext = MakeMapText(player, mech, mech_map, x, y, 5, 5, 4, dolos);

    sprintf(mybuff[0],
	"              0                                          %.150s",
	maptext[0]);
    sprintf(mybuff[1],
	"         ___________                                     %.150s",
	maptext[1]);
    sprintf(mybuff[2],
	"        /           \\          Location:%4d,%4d, %3d   %.150s",
	MechX(mech), MechY(mech), MechZ(mech), maptext[2]);
    sprintf(mybuff[3],
	"  300  /             \\  60     Terrain: %14s   %.150s",
	GetTerrainName(mech_map, MechX(mech), MechY(mech)), maptext[3]);
    sprintf(mybuff[4],
	"      /               \\                                  %.150s",
	maptext[4]);
    sprintf(mybuff[5],
	"     /                 \\                                 %.150s",
	maptext[5]);
    sprintf(mybuff[6],
	"270 (                   )  90  Speed:           %6.1f   %.150s",
	MechSpeed(mech), maptext[6]);
    sprintf(mybuff[7],
	"     \\                 /       Vertical Speed:  %6.1f   %.150s",
	MechVerticalSpeed(mech), maptext[7]);
    sprintf(mybuff[8],
	"      \\               /        Heading:           %4d   %.150s",
	MechFacing(mech), maptext[8]);
    sprintf(mybuff[9],
	"  240  \\             /  120                              %.150s",
	maptext[9]);
    sprintf(mybuff[10],
	"        \\___________/                                    %.150s",
	maptext[10]);
    sprintf(mybuff[11], "                      ");
    sprintf(mybuff[12], "             180");

    navigate_sketch_mechs(mech, mech_map, x, y, mybuff);
    for (i = 0; i < NAVIGATE_LINES; i++)
	notify(player, mybuff[i]);
}


/* INDENT OFF */

/* 
                0
            ___________                                     /``\][/""\][/""\
           /           \          HEX Location: 254, 122    \`1/``\""/``\""/
     300  /             \  60     Terrain: Light Forest     /``\``/""\`3/""\
         /               \        Elevation:  0             \`2/``\"1/``\""/
        /                 \                                 /""\``|**\`3/""\
   270 (                   )  90  Speed: 0.0                \"4/``\"4/``\""/
        \                 /       Vertical Speed: 0.0       /""\`3/""\`3/""\
         \               /        Heading: 0                \"4/``\"4/``\""/
     240  \             /  120                              /""\`3/""\`3/""\
           \____*______/                                    \"4/][\"4/][\"4/
                180
 */

/* INDENT ON */

char GetLRSMechChar(MECH * mech, MECH * other)
{
    char c = 'u';

    if (mech == other)
	return '*';
    if (IsDS(other))
	c = 'd';
    switch (MechMove(other)) {
    case MOVE_FLY:
	c = 'a';
    case MOVE_BIPED:
	c = 'b';
	break;
    case MOVE_QUAD:
	c = 'q';
	break;
    case MOVE_TRACK:
	c = 't';
	break;
    case MOVE_WHEEL:
	c = 'w';
	break;
    case MOVE_HOVER:
	c = 'h';
	break;
    case MOVE_VTOL:
	c = 'v';
	break;
    case MOVE_HULL:
	c = 'n';
	break;
    case MOVE_SUB:
	c = 's';
	break;
    case MOVE_FOIL:
	c = 'f';
	break;
    }
    if (!MechSeemsFriend(mech, other))
	c = toupper(c);
    return c;
}

static inline char TerrainColorChar(char terrain, int elev)
{
    switch (terrain) {
    case HIGHWATER:
	return custom_color_str[DWATER_IDX];
    case WATER:
	if (elev < 2 || elev == '0' || elev == '1' || elev == '~')
	    return custom_color_str[SWATER_IDX];
	return custom_color_str[DWATER_IDX];
    case BUILDING:
	return custom_color_str[BUILDING_IDX];
    case ROAD:
	return custom_color_str[ROAD_IDX];
    case ROUGH:
	return custom_color_str[ROUGH_IDX];
    case MOUNTAINS:
	return custom_color_str[MOUNTAIN_IDX];
    case FIRE:
	return custom_color_str[FIRE_IDX];
    case ICE:
	return custom_color_str[ICE_IDX];
    case WALL:
	return custom_color_str[WALL_IDX];
    case SNOW:
	return custom_color_str[SNOW_IDX];
    case SMOKE:
	return custom_color_str[SMOKE_IDX];
    case LIGHT_FOREST:
	return custom_color_str[LWOOD_IDX];
    case HEAVY_FOREST:
	return custom_color_str[HWOOD_IDX];
    case UNKNOWN_TERRAIN:
	return custom_color_str[UNKNOWN_IDX];
    }
    return '\0';
}

static char *add_color(char newc, char * prevc, char c)
{
    static char buf[10]; /* won't be filled with more than 7 characters */
    buf[0] = '\0';

    if (newc == *prevc) {
	buf[0] = c;
	buf[1] = '\0';
	return buf;
    }

    if (!newc || ((isupper(*prevc)) && !isupper(newc)) ||
    	(newc == 'H' && *prevc))
	strcpy(buf, "%cn");
    else if (isupper(newc) && !isupper(*prevc))
	strcpy(buf, "%ch");

    if (!newc)
	sprintf(buf + strlen(buf), "%c", c);
    else
	sprintf(buf + strlen(buf), "%%c%c%c", tolower(newc), c);
    *prevc = newc;
    return buf;
}

static char *GetLRSMech(MECH * mech, MECH * other, int docolor, char *prevc)
{
    static char buf[2]; /* Won't be filled with more than 1 character */
    char c = GetLRSMechChar(mech, other);
    char newc;

    if (!docolor) {
	sprintf(buf, "%c", c);
	return buf;
    }

    if (mech == other)
	newc = custom_color_str[SELF_IDX];
    else if (!MechSeemsFriend(mech, other))
	newc = custom_color_str[ENEMY_IDX];
    else
	newc = custom_color_str[FRIEND_IDX];

    return add_color(newc, prevc, c);
    
}

static char *LRSTerrain(MAP * map, int x, int y, int docolor, char *prevc)
{
    static char buf[2]; /* Won't be filled with more than 1 character */

    char c = GetTerrain(map, x, y);
    char newc;

    if (!c || !docolor || c == ' ') {
	buf[0] = c;
	buf[1] = '\0';
	return buf;
    } else
	newc = TerrainColorChar(c, GetElev(map, x, y));

    return add_color(newc, prevc, c);
}

static char *LRSElevation(MAP * map, int x, int y, int docolor, char *prevc)
{
    static char buf[2]; /* Won't be filled with more than 1 character */

    int e = GetElev(map, x, y);
    char c = (e || docolor) ? '0' + e : ' ';
    char newc;

    if (!docolor) {
	buf[0] = c;
	buf[1] = '\0';
	return buf;
    } else
	newc = TerrainColorChar(GetTerrain(map, x, y), e);

    return add_color(newc, prevc, c);
}

#define LRS_TERRAINMODE		1
#define LRS_ELEVMODE		2
#define LRS_MECHMODE		4
#define LRS_LOSMODE		8
#define LRS_COLORMODE		16
#define LRS_ELEVCOLORMODE	32

static char *get_lrshexstr(MECH * mech, MAP *map, int x, int y,
			   char * prevc, int mode, MECH ** mechs, int lm,
			   hexlosmap_info * losmap)
{
    int losflag = MAPLOSHEX_SEE | MAPLOSHEX_SEEN;

    if (mode & LRS_MECHMODE) {
	while (mechs[lm] && MechY(mechs[lm]) < y)
	    lm++;
	while (mechs[lm] && MechY(mechs[lm]) == y && MechX(mechs[lm]) < x)
	    lm++;
	if (mechs[lm] && MechY(mechs[lm]) == y && MechX(mechs[lm]) == x)
	    return GetLRSMech(mech, mechs[lm], mode & LRS_COLORMODE, prevc);
    }

    if (losmap)
	losflag = LOSMap_GetFlag(losmap, x, y);

    /* If the losmap doesn't contain this hex, we return X in bold red
     * in both terrain and elevation mode.
     */
    if (!(losflag & MAPLOSHEX_SEEN))
	return add_color('R', prevc, 'X');

    if (((mode & LRS_TERRAINMODE) && !(losflag & MAPLOSHEX_SEETERRAIN)) ||
	((mode & LRS_ELEVMODE) && !(losflag & MAPLOSHEX_SEEELEV)))
	return add_color(TerrainColorChar(UNKNOWN_TERRAIN, 0), prevc, '?');
    
    if (mode & LRS_ELEVMODE)
	return LRSElevation(map, x, y, mode & LRS_ELEVCOLORMODE, prevc);
    if (mode & LRS_TERRAINMODE)
	return LRSTerrain(map, x, y, mode & LRS_COLORMODE, prevc);
	    
    SendError(tprintf("Unknown LRS mode, mech #%d mode 0x%x.",
		      mech->mynum, mode));
    return add_color('R', prevc, 'Y');

}

static void show_lrs_map(dbref player, MECH * mech, MAP * map, int x,
			 int y, int displayHeight, int mode)
{
    int loop, b_width, e_width, b_height, e_height, i;
    MECH *oMech;

    /* topbuff and botbuff must be capable of holding enough
     * characters to colorize all hexes in the most inefficient
     * way. This means 15 characters per 2 hexes, or 8 per
     * hex. topbuff and botbuff both hold half the hexes on the row,
     * so that ends up 4 * LRS_DISPLAY_WIDTH (plus some padding thrown
     * in for good measure.)
     *
     * midbuff is only used for the lables, and only needs a few
     * characters more than the display width.
     */
    char topbuff[4 * LRS_DISPLAY_WIDTH + 30] = "    ";
    char botbuff[4 * LRS_DISPLAY_WIDTH + 30] = "    ";
    char midbuff[8 + LRS_DISPLAY_WIDTH] = "    ";
    char trash1[5]; /* temp var to hold the max-three-digit number of map Y */
    short oddcol = 0;
    MECH *mechs[MAX_MECHS_PER_MAP];
    int last_mech = 0;
    char prevct = 0, prevcb = 0;
    hexlosmap_info * losmap = NULL;


    /* x and y hold the viewing center of the map */
    b_width = x - LRS_DISPLAY_WIDTH / 2;
    b_width = MAX(b_width, 0);
    e_width = b_width + LRS_DISPLAY_WIDTH;
    if (e_width >= map->map_width) {
	e_width = map->map_width - 1;
	b_width = e_width - LRS_DISPLAY_WIDTH;
	b_width = MAX(b_width, 0);
    }

    if (b_width % 2)
	oddcol = 1;

    b_height = y - displayHeight / 2;
    b_height = MAX(b_height, 0);
    e_height = b_height + displayHeight;
    if (e_height > map->map_height) {
	e_height = map->map_height;
	b_height = e_height - displayHeight;
	b_height = MAX(b_height, 0);
    }

    /* Display the top labels */
    for (i = b_width; i <= e_width; i++) {
	sprintf(trash1, "%3d", i);
	sprintf(topbuff + strlen(topbuff), "%c", trash1[0]);
	sprintf(midbuff + strlen(midbuff), "%c", trash1[1]);
	sprintf(botbuff + strlen(botbuff), "%c", trash1[2]);
    }
    notify(player, topbuff);
    notify(player, midbuff);
    notify(player, botbuff);


    if (mode & LRS_MECHMODE) {
	for (i = 0; i < map->first_free; i++) {
	    if ((oMech = getMech(map->mechsOnMap[i]))) {
		if ((mech == oMech) ||
		    (MechY(oMech) >= b_height && MechY(oMech) <= e_height &&
		     MechX(oMech) >= b_width && MechX(oMech) <= e_width &&
		     InLineOfSight(mech, oMech, MechX(oMech), MechY(oMech),
				   FlMechRange(map, mech, oMech))))
		    mechs[last_mech++] = oMech;
	    }
	}
	for (i = 0; i < (last_mech - 1); i++)	/* Bubble-sort the list 
						 *  to y/x order */
	    for (loop = (i + 1); loop < last_mech; loop++) {
		if (MechY(mechs[i]) > MechY(mechs[loop])) {
		    oMech = mechs[i];
		    mechs[i] = mechs[loop];
		    mechs[loop] = oMech;
		} else if (MechY(mechs[i]) == MechY(mechs[loop]) &&
		    MechX(mechs[i]) > MechX(mechs[loop])) {
		    oMech = mechs[i];
		    mechs[i] = mechs[loop];
		    mechs[loop] = oMech;
		}
	    }
	mechs[last_mech] = NULL;
	last_mech = 0;
    }

    if (mode & LRS_LOSMODE)
	losmap = CalculateLOSMap(map, mech, b_width, b_height,
				 e_width - b_width, e_height - b_height);

    for (loop = b_height; loop < e_height; loop++) {
	sprintf(topbuff, "%3d ", loop);
	strcpy(botbuff, "    ");
	if (mode & LRS_MECHMODE)
	    while (mechs[last_mech] && MechY(mechs[last_mech]) < loop)
		last_mech++;

	for (i = b_width; i < e_width; i += 2) {
	    sprintf(topbuff + strlen(topbuff), oddcol ? "%s " : " %s",
		    get_lrshexstr(mech, map, i + !oddcol, loop, &prevct,
				  mode, mechs, last_mech, losmap));

	    sprintf(botbuff + strlen(botbuff), oddcol ? " %s" : "%s ",
		    get_lrshexstr(mech, map, i + oddcol, loop, &prevcb,
				  mode, mechs, last_mech, losmap));
	}
	if (i == e_width && !oddcol) {
	    sprintf(botbuff + strlen(botbuff), "%s",
		    get_lrshexstr(mech, map, i, loop, &prevcb, mode,
				  mechs, last_mech, losmap));
	} else if (i == e_width) {
	    sprintf(topbuff + strlen(topbuff), "%s",
		    get_lrshexstr(mech, map, i, loop, &prevct, mode,
				  mechs, last_mech, losmap));
	    strcat(botbuff, " ");
	}

	if (mode & (LRS_COLORMODE|LRS_ELEVCOLORMODE)) {
	    if (prevct) {
		strcat(topbuff, "%cn");
		prevct = 0;
	    }
	    if (prevcb) {
		strcat(botbuff, "%cn");
		prevcb = 0;
	    }
	}
	sprintf(botbuff + strlen(botbuff), " %-3d", loop);
	notify(player, topbuff);
	notify(player, botbuff);
    }
}

void mech_lrsmap(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    MAP *map;
    int argc, mode = 0;
    short x, y;
    char *args[5], *str;
    int displayHeight = LRS_DISPLAY_HEIGHT;

    cch(MECH_USUAL);

    if (Ansimap(player))
	mode |= LRS_COLORMODE;

    map = getMap(mech->mapindex);

    argc = mech_parseattributes(buffer, args, 4);
    DOCHECK(!MechLRSRange(mech), "Your system seems to be inoperational.");
    if (!parse_tacargs(player, mech, &args[1], argc - 1,
		       MechLRSRange(mech), &x, &y))
	return;
    switch(args[0][0]) {
    case 'M':
    case 'm':
	mode |= LRS_MECHMODE | LRS_TERRAINMODE;
	break;
    case 'E':
    case 'e':
	mode |= LRS_ELEVMODE;
	break;
    case 'C':
    case 'c':
	mode |= LRS_ELEVMODE | LRS_ELEVCOLORMODE;
	break;
    case 'T':
    case 't':
	mode |= LRS_TERRAINMODE;
	break;
    case 'L':
    case 'l':
	mode |= LRS_LOSMODE | LRS_TERRAINMODE;
	break;
    case 'H':
    case 'h':
	mode |= LRS_LOSMODE | LRS_ELEVMODE;
	break;
    case 'S':
    case 's':
	mode |= LRS_LOSMODE | LRS_MECHMODE | LRS_TERRAINMODE;
	break;
    default:
	notify_printf(player, "Unknown LRS sensor type '%s'!", args[0]);
	return;
    }
    
    if (MapIsDark(map) || (MechType(mech) == CLASS_MW &&
    				mudconf.btech_mw_losmap))
	mode |= LRS_LOSMODE;

    str = silly_atr_get(player, A_LRSHEIGHT);
    if (*str) {
	displayHeight = atoi(str);
	if (displayHeight < 10 || displayHeight > 40) {
	    notify(player,
		   "Illegal LRSHeight attribute.  Must be between 10 and 40");
	    displayHeight = LRS_DISPLAY_HEIGHT;
	}
    }

    displayHeight = MIN(displayHeight, 2 * MechLRSRange(mech));
    displayHeight = MIN(displayHeight, map->map_height);

    if (!(displayHeight % 2))
	displayHeight++;

    set_colorscheme(player);

    show_lrs_map(player, mech, map, x, y, displayHeight, mode);
}
	
static inline int is_oddcol(int col)
{
    /*
     * The only real trick here is to handle negative
     * numbers correctly.
     */
    return (unsigned) col & 1;
}

static inline int tac_dispcols(int hexcols)
{
    return hexcols * 3 + 1;
}

static inline int tac_hex_offset(int x, int y, int dispcols, int oddcol1)
{
    int oddcolx = is_oddcol(x + oddcol1);

    return (y * 2 + 1 - oddcolx) * dispcols + x * 3 + 1;
}

static inline void sketch_tac_row(char *pos, int left_offset,
    char const *src, int len)
{
    memset(pos, ' ', left_offset);
    memcpy(pos + left_offset, src, len);
    pos[left_offset + len] = '\0';
}

static void sketch_tac_map(char *buf, MAP * map, MECH * mech, int sx,
    int sy, int wx, int wy, int dispcols, int top_offset, int left_offset,
    int docolour, int dohexlos)
{
#if 0
    static char const hexrow[2][76] = {
	"\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/",
	"/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\"
    };
#else
    static char const hexrow[2][310] = {    
        "\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/]["
        "\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/]["
        "\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/]["
        "\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/]["
        "\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/",
        "/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\]["
        "/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\]["
        "/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\]["
        "/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\]["
        "/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\][/][\\"
    };
#endif
    int x, y;
    int oddcol1 = is_oddcol(sx);	/* One iff first hex col is odd */
    char *pos;
    int mapcols = tac_dispcols(wx);
    hexlosmap_info * losmap = NULL;

    /*
     * First create a blank hex map.
     */
    pos = buf;
    for (y = 0; y < top_offset; y++) {
	memset(pos, ' ', dispcols - 1);
	pos[dispcols - 1] = '\0';
	pos += dispcols;
    }
    for (y = 0; y < wy; y++) {
	sketch_tac_row(pos, left_offset, hexrow[oddcol1], mapcols);
	pos += dispcols;
	sketch_tac_row(pos, left_offset, hexrow[!oddcol1], mapcols);
	pos += dispcols;
    }
    sketch_tac_row(pos, left_offset, hexrow[oddcol1], mapcols);

    /*
     * Now draw the terrain and elevation. 
     */
    pos = buf + top_offset * dispcols + left_offset;
    wx = MIN(wx, map->map_width - sx);
    wy = MIN(wy, map->map_height - sy);

    if (dohexlos)
	losmap = CalculateLOSMap(map, mech, MAX(0, sx), MAX(0, sy),
				 wx, wy);

    for (y = MAX(0, -sy); y < wy; y++) {
	for (x = MAX(0, -sx); x < wx; x++) {
	    int terr, elev, losflag = MAPLOSHEX_SEE | MAPLOSHEX_SEEN;
	    char *base;
	    char topchar, botchar;
	    
	    if (losmap)
		losflag = LOSMap_GetFlag(losmap, sx+x, sy+y);

	    if (!(losflag & MAPLOSHEX_SEEN)) {
		terr = 'X';
		elev = 40; /* 'X' */
	    } else {
		
		if (losflag & MAPLOSHEX_SEETERRAIN)
		    terr = GetTerrain(map, sx + x, sy + y);
		else
		    terr = UNKNOWN_TERRAIN;
		
		if (losflag & MAPLOSHEX_SEEELEV)
		    elev = GetElev(map, sx + x, sy + y);
		else
		    elev = 15; /* Ugly hack: '0' + 15 == '?' */
	    }
	    base = pos + tac_hex_offset(x, y, dispcols, oddcol1);

	    switch (terr) {
	    case WATER:
		/*
		 * Colour hack:  Draw deep water with '\242'
		 * if using colour so colourize_tac_map()
		 * knows to use dark blue rather than light
		 * blue
		 */
		if (docolour && elev >= 2) {
		    topchar = '\242';
		    botchar = '\242';
		} else {
		    topchar = '~';
		    botchar = '~';
		}
		break;

	    case HIGHWATER:
		topchar = '~';
		botchar = '+';
		break;

	    case BRIDGE:
		topchar = '#';
		botchar = '+';
		break;

	    case ' ':		/* GRASSLAND */
		topchar = ' ';
		botchar = '_';
		break;

	    case UNKNOWN_TERRAIN:
		topchar = '?';
		botchar = '?';
		break;

	    default:
		topchar = terr;
		botchar = terr;
		break;
	    }

	    base[0] = topchar;
	    base[1] = topchar;
	    base[dispcols + 0] = botchar;
	    if (elev > 0) {
		botchar = '0' + elev;
	    }
	    base[dispcols + 1] = botchar;
	}
    }
}

/*
 * Draw one of the seven hexes that a Dropship takes up on a tac map.
 */
static void sketch_tac_ds(char *base, int dispcols, char terr)
{
    /*
     * Becareful not to overlay a 'mech id or terrain elevation.
     */
    if (!isalpha((unsigned char) base[0])) {
	base[0] = terr;
	base[1] = terr;
    }
    base[dispcols + 0] = terr;
    if (!isdigit((unsigned char) base[dispcols + 1])) {
	base[dispcols + 1] = terr;
    }
}

extern int dirs[6][2];

static void sketch_tac_ownmech(char *buf, MAP * map, MECH * mech, int sx,
    int sy, int wx, int wy, int dispcols, int top_offset, int left_offset)
{

    int oddcol1 = is_oddcol(sx);
    char *pos = buf + top_offset * dispcols + left_offset;
    char *base;
    int x = MechX(mech) - sx;
    int y = MechY(mech) - sy;

    if (x < 0 || x >= wx || y < 0 || y >= wy) {
	return;
    }
    base = pos + tac_hex_offset(x, y, dispcols, oddcol1);
    base[0] = '*';
    base[0] = '*';
}

static void sketch_tac_mechs(char *buf, MAP * map, MECH * player_mech,
    int sx, int sy, int wx, int wy, int dispcols, int top_offset,
    int left_offset, int docolour, int labels)
{
    int i;
    char *pos = buf + top_offset * dispcols + left_offset;
    int oddcol1 = is_oddcol(sx);

    /*
     * Draw all the 'mechs on the map.
     */
    for (i = 0; i < map->first_free; i++) {
	int x, y;
	char *base;
	MECH *mech;

	if (map->mechsOnMap[i] == -1) {
	    continue;
	}

	mech = getMech(map->mechsOnMap[i]);
	if (mech == NULL) {
	    continue;
	}

	/*
	 * Check to see if the 'mech is on the tac map and 
	 * that its in LOS of the player's 'mech.
	 */
	x = MechX(mech) - sx;
	y = MechY(mech) - sy;
	if (x < 0 || x >= wx || y < 0 || y >= wy) {
	    continue;
	}

	if (mech != player_mech &&
	    !InLineOfSight(player_mech, mech, MechX(mech), MechY(mech),
		FlMechRange(map, player_mech, mech))) {
	    continue;
	}

	base = pos + tac_hex_offset(x, y, dispcols, oddcol1);
	if (!(MechSpecials2(mech) & CARRIER_TECH) && IsDS(mech) &&
	    ((MechZ(mech) >= ORBIT_Z && mech != player_mech) || Landed(mech) || !Started(mech)))
	    {
	    int ts = DSBearMod(mech);
	    int dir;

	    /*
	     * Dropships are a special case.  They take up
	     * seven hexes on a tac map.  First draw the
	     * center hex and then the six surronding hexes.
	     */

	    if (docolour) {
		/*
		 * Colour hack: 'X' would be confused with
		 * any enemy con by colourize_tac_map()
		 */
		sketch_tac_ds(base, dispcols, '$');
	    } else {
		sketch_tac_ds(base, dispcols, 'X');
	    }

	    for (dir = 0; dir < 6; dir++) {
		int tx = x + dirs[dir][0];
		int ty = y + dirs[dir][1];

		if ((tx + oddcol1) % 2 == 0 && dirs[dir][0] != 0) {
		    ty--;
		}
		if (tx < 0 || tx >= wx || ty < 0 || ty >= wy) {
		    continue;
		}
		base = pos + tac_hex_offset(tx, ty, dispcols, oddcol1);
		if (Find_DS_Bay_Number(mech, (dir - ts + 6) % 6)
		    >= 0) {
		    sketch_tac_ds(base, dispcols, '@');
		} else {
		    sketch_tac_ds(base, dispcols, '=');
		}
	    }
	} else if (mech == player_mech) {
	    base[0] = '*';
	    base[1] = '*';
	} else {
	    char *id = MechIDS(mech, MechSeemsFriend(player_mech, mech));

	    base[0] = id[0];
	    base[1] = id[1];
	}
    }
}

static void sketch_tac_cliffs(char *buf, MAP * map, int sx, int sy, int wx,
    int wy, int dispcols, int top_offset, int left_offset, int cliff_size)
{
    char *pos = buf + top_offset * dispcols + left_offset;
    int y, x;
    int oddcol1 = is_oddcol(sx);

    wx = MIN(wx, map->map_width - sx);
    wy = MIN(wy, map->map_height - sy);
    for (y = MAX(0, -sy); y < wy; y++) {
	int ty = sy + y;

	for (x = MAX(0, -sx); x < wx; x++) {
	    int tx = sx + x;
	    int oddcolx = is_oddcol(tx);
	    int elev = Elevation(map, tx, ty);
	    char *base = pos + tac_hex_offset(x, y, dispcols,
		oddcol1);
	    char c;


	    /*
	     * Copy the elevation up to the top of the hex 
	     * so we can draw a bottom hex edge on every hex.
	     */
	    c = base[dispcols + 1];
	    if (base[0] == '*') {
		base[0] = '*';
		base[1] = '*';
	    } else if (isdigit((unsigned char) c)) {
		base[1] = c;
	    }


	    /*
	     * For each hex on the map check to see if each
	     * of it's 240, 180, and 120 hex sides is a cliff. 
	     * Don't check for cliffs between hexes that are on
	     * the tac map and those that are off of it.
	     */

	    if (x != 0 && (y < wy - 1 || oddcolx)
		&& abs(Elevation(map, tx - 1, ty + 1 - oddcolx)
		    - elev) >= cliff_size) {

		base[dispcols - 1] = '|';
	    }
	    if (y < wy - 1 && abs(Elevation(map, tx, ty + 1) - elev)
		>= cliff_size) {
		base[dispcols] = ',';
		base[dispcols + 1] = ',';
	    } else {
		base[dispcols] = '_';
		base[dispcols + 1] = '_';
	    }
	    if (x < wx - 1 && (y < wy - 1 || oddcolx)
		&& abs(Elevation(map, tx + 1, ty + 1 - oddcolx)
		    - elev) >= cliff_size) {
		base[dispcols + 2] = '!';
	    }
	}
    }
}
static void sketch_tac_dslz(char *buf, MAP * map, MECH * mech, int sx,
			    int sy, int wx, int wy, int dispcols,
			    int top_offset, int left_offset, int cliff_size,
			    int docolour)
{
    char *pos = buf + top_offset * dispcols + left_offset;
    int y, x;
    int oddcol1 = is_oddcol(sx);

    wx = MIN(wx, map->map_width - sx);
    wy = MIN(wy, map->map_height - sy);
    for (y = MAX(0, -sy); y < wy; y++) {
	int ty = sy + y;

	for (x = MAX(0, -sx); x < wx; x++) {
	    int tx = sx + x;
	    char *base = pos + tac_hex_offset(x, y, dispcols, oddcol1);

	    if (ImproperLZ(mech, tx, ty))
		base[dispcols] = docolour ? '\241' : 'X';
	    else
		base[dispcols] = docolour ? '\240' : 'O';
	}
    }
}

/*
 * Colourize a sketch tac map.  Uses dynmaically allocated buffers
 * which are overwritten on each call.
 */
static char **colourize_tac_map(char const *sketch, int dispcols,
    int disprows)
{
    static char *buf = NULL;
    static int buf_len = 5000;
    static char **lines = NULL;
    static int lines_len = 100;
    int pos = 0;
    int line = 0;
    unsigned char cur_colour = '\0';
    const char *line_start;
    char const *src = sketch;

    if (buf == NULL) {
	Create(buf, char, buf_len);
    }
    if (lines == NULL) {
	Create(lines, char *, lines_len);
    }

    line_start = (char *) src;
    lines[0] = buf;
    while (lines > 0) {
	unsigned char new_colour;
	unsigned char c = *src++;

	if (c == '\0') {
	    /*
	     * End of line.
	     */
	    if (cur_colour != '\0') {
		buf[pos++] = '%';
		buf[pos++] = 'c';
		buf[pos++] = 'n';
	    }
	    buf[pos++] = '\0';
	    line++;
	    if (line >= disprows) {
		break;		/* Done */
	    }
	    if (line + 1 >= lines_len) {
		lines_len *= 2;
		ReCreate(lines, char *, lines_len);
	    }
	    line_start += dispcols;
	    src = line_start;
	    lines[line] = buf + pos;
	    continue;
	}

	switch (c) {
	case (unsigned char) '\242':	/* Colour Hack: Deep Water */
	    c = '~';
	    new_colour = custom_color_str[DWATER_IDX];
	    break;

	case (unsigned char) '\241':	/* Colour Hack: improper LZ */
	    c = 'X';
	    new_colour = custom_color_str[BADLZ_IDX];
	    break;
	case (unsigned char) '\240':	/* Colour Hack: proper LZ */
	    c = 'O';
	    new_colour = custom_color_str[GOODLZ_IDX];
	    break;
	case '?':
	    c = '?';
	    new_colour = custom_color_str[UNKNOWN_IDX];
	    break;

	case '$':		/* Colour Hack: Drop Ship */
	    c = 'X';
	    new_colour = custom_color_str[DS_IDX];
	    break;

	case '!':		/* Cliff hex edge */
	    c = '/';
	    new_colour = custom_color_str[CLIFF_IDX];
	    break;

	case '|':		/* Cliff hex edge */
	    c = '\\';
	    new_colour = custom_color_str[CLIFF_IDX];
	    break;

	case ',':		/* Cliff hex edge */
	    c = '_';
	    new_colour = custom_color_str[CLIFF_IDX];
	    break;
	case '*':		/* mech itself. */
	    new_colour = custom_color_str[SELF_IDX];
	    break;

	default:
	    if (islower(c)) {	/* Friendly con */
		new_colour = custom_color_str[FRIEND_IDX];
	    } else if (isupper(c)) {	/* Enemy con */
		new_colour = custom_color_str[ENEMY_IDX];
	    } else if (isdigit(c)) {	/* Elevation */
		new_colour = cur_colour;
	    } else {
		new_colour = TerrainColorChar(c, 0);
	    }
	    break;
	}

	if (isupper(new_colour) != isupper(cur_colour)) {
	    if (isupper(new_colour)) {
		buf[pos++] = '%';
		buf[pos++] = 'c';
		buf[pos++] = 'h';
	    } else {
		buf[pos++] = '%';
		buf[pos++] = 'c';
		buf[pos++] = 'n';
		cur_colour = '\0';
	    }
	}
	if (tolower(new_colour) != tolower(cur_colour)) {
	    buf[pos++] = '%';
	    buf[pos++] = 'c';
	    if (new_colour == '\0') {
		buf[pos++] = 'n';
	    } else if (new_colour == 'H') {
		buf[pos++] = 'n';
		buf[pos++] = '%';
		buf[pos++] = 'c';
		buf[pos++] = tolower(new_colour);
	    } else {
		buf[pos++] = tolower(new_colour);
	    }
	    cur_colour = new_colour;
	}
	buf[pos++] = c;
	if (pos + 11 > buf_len) {
	    /*
	     * If we somehow run out of room then we don't
	     * bother to reallocate 'buf' and potentially have
	     * a bunch of invalid pointers in 'lines' to fix up.
	     * We just restart from scratch with a bigger 'buf'.
	     */
	    buf_len *= 2;
	    free(buf);
	    buf = NULL;
	    return colourize_tac_map(sketch, dispcols, disprows);
	}
    }
    lines[line] = NULL;
    return lines;
}

/*
 * Draw a tac map for the TACTICAL and NAVIGATE commands.
 *
 * This used to be "one MOFO of a function" but has been simplified
 * in a number of ways.  One is that it used to statically allocated
 * buffers which limit the map drawn to MAP_DISPLAY_WIDTH hexes across
 * and 24 hexes down in size.  The return value should no longer be
 * freed with KillText().
 *
 * player   = dbref of player wanting map (mostly irrelevant)
 * mech     = mech player's in (or NULL, if on map) 
 * map      = map obj itself
 * cx       = middle of the map (x)
 * cy       = middle of the map (y)
 * wx       = width in x
 * wy       = width in y
 * labels   = bit array
 *    1 = the 'top numbers'
 *    2 = the 'side numbers'
 *    4 = navigate mode
 *    8 = show mech cliffs
 *   16 = show tank cliffs
 *   32 = show DS LZ's
 *
 * If navigate mode, wx and wy should be equal and odd.  Navigate maps
 * cannot have top or side labels.
 *
 */

char **MakeMapText(dbref player, MECH * mech, MAP * map, int cx, int cy,
    int wx, int wy, int labels, int dohexlos)
{
    int docolour = Ansimap(player);
    int dispcols;
    int disprows;
    int mapcols;
    int left_offset = 0;
    int top_offset = 0;
    int navigate = 0;
    int sx, sy;
    int i;
    char *base;
    int oddcol1;
    enum {
	MAX_WIDTH = 40,
	MAX_HEIGHT = 24,
	TOP_LABEL = 3,
	LEFT_LABEL = 4,
	RIGHT_LABEL = 3
    };
    static char sketch_buf[((LEFT_LABEL + 1 + MAX_WIDTH * 3 + RIGHT_LABEL +
	    1) * (TOP_LABEL + 1 + MAX_HEIGHT * 2) + 2) * 5];
    static char *lines[(TOP_LABEL + 1 + MAX_HEIGHT * 2 + 1) * 5];

    if (labels & 4) {
	navigate = 1;
	labels = 0;
    }

    /*
     * Figure out the extent of the tac map to draw.  
     */
    wx = MIN(MAX_WIDTH, wx);
    wy = MIN(MAX_HEIGHT, wy);

    sx = cx - wx / 2;
    sy = cy - wy / 2;
    if (!navigate) {
	/*
	 * Only allow navigate maps to include off map hexes.
	 */
	sx = MAX(0, MIN(sx, map->map_width - wx));
	sy = MAX(0, MIN(sy, map->map_height - wy));
	wx = MIN(wx, map->map_width);
	wy = MIN(wy, map->map_height);
    }

    mapcols = tac_dispcols(wx);
    dispcols = mapcols + 1;
    disprows = wy * 2 + 1;
    oddcol1 = is_oddcol(sx);

    if (navigate) {
	if (oddcol1) {
	    /*
	     * Insert blank line at the top where we can put
	     * a "__" to make the navigate map look pretty.
	     */
	    top_offset = 1;
	    disprows++;
	}
    } else {
	/*
	 * Allow room for the labels.
	 */
	if (labels & 1) {
	    left_offset = LEFT_LABEL;
	    dispcols += LEFT_LABEL + RIGHT_LABEL;
	}
	if (labels & 2) {
	    top_offset = TOP_LABEL;
	    disprows += TOP_LABEL;
	}
    }

    /*
     * Create a sketch tac map including terrain and elevation.
     */
    sketch_tac_map(sketch_buf, map, mech, sx, sy, wx, wy, dispcols,
	top_offset, left_offset, docolour, dohexlos);

    /*
     * Draw the top and side labels.
     */
    if (labels & 1) {
	int x;

	for (x = 0; x < wx; x++) {
	    char scratch[4];
	    int label = sx + x;

	    if (label < 0 || label > 999) {
		continue;
	    }
	    sprintf(scratch, "%3d", label);
	    base = sketch_buf + left_offset + 1 + x * 3;
	    base[0] = scratch[0];
	    base[1 * dispcols] = scratch[1];
	    base[2 * dispcols] = scratch[2];
	}
    }

    if (labels & 2) {
	int y;

	for (y = 0; y < wy; y++) {
	    int label = sy + y;

	    base = sketch_buf + (top_offset + 1 + y * 2)
		* dispcols;
	    if (label < 0 || label > 999) {
		continue;
	    }

	    sprintf(base, "%3d", label);
	    base[3] = ' ';
	    sprintf(base + (dispcols - RIGHT_LABEL - 1), "%3d", label);
	}
    }

    if (labels & 8) {
	if (mech != NULL) {
	    sketch_tac_ownmech(sketch_buf, map, mech, sx, sy, wx, wy,
		dispcols, top_offset, left_offset);
	}
	sketch_tac_cliffs(sketch_buf, map, sx, sy, wx, wy, dispcols,
	    top_offset, left_offset, 3);
    } else if (labels & 16) {
	if (mech != NULL) {
	    sketch_tac_ownmech(sketch_buf, map, mech, sx, sy, wx, wy,
		dispcols, top_offset, left_offset);
	}
	sketch_tac_cliffs(sketch_buf, map, sx, sy, wx, wy, dispcols,
	    top_offset, left_offset, 2);
    } else if (labels & 32) {
	if (mech != NULL) {
	    sketch_tac_ownmech(sketch_buf, map, mech, sx, sy, wx, wy,
			       dispcols, top_offset, left_offset);
	}
	sketch_tac_dslz(sketch_buf, map, mech, sx, sy, wx, wy, dispcols,
			top_offset, left_offset, 2, docolour);
    } else if (mech != NULL) {
	sketch_tac_mechs(sketch_buf, map, mech, sx, sy, wx, wy, dispcols,
	    top_offset, left_offset, docolour, labels);
    }


    if (navigate) {
	int n = wx / 2;		/* Hexagon radius */

	/*
	 * Navigate hack: erase characters from the sketch map
	 * to turn it into a pretty hexagonal shaped map.
	 */
	if (oddcol1) {
	    /*
	     * Don't need the last line in this case. 
	     */
	    disprows--;
	}

	for (i = 0; i < n; i++) {
	    int len;

	    base = sketch_buf + (i + 1) * dispcols + left_offset;
	    len = (n - i - 1) * 3 + 1;
	    memset(base, ' ', len);
	    base[len] = '_';
	    base[len + 1] = '_';
	    base[mapcols - len - 2] = '_';
	    base[mapcols - len - 1] = '_';
	    base[mapcols - len] = '\0';

	    base =
		sketch_buf + (disprows - i - 1) * dispcols + left_offset;
	    len = (n - i) * 3;
	    memset(base, ' ', len);
	    base[mapcols - len] = '\0';
	}

	memset(sketch_buf + left_offset, ' ', n * 3 + 1);
	sketch_buf[left_offset + n * 3 + 1] = '_';
	sketch_buf[left_offset + n * 3 + 2] = '_';
	sketch_buf[left_offset + n * 3 + 3] = '\0';
    }

    if (docolour) {
	/*
	 * If using colour then colourize the sketch map and
	 * return the result.
	 */
	return colourize_tac_map(sketch_buf, dispcols, disprows);
    }

    /*
     * If not using colour, the sketch map can be used as is.
     */
    for (i = 0; i < disprows; i++) {
	lines[i] = sketch_buf + dispcols * i;
    }
    lines[i] = NULL;
    return lines;
}

/* Draws the map for the player when they use the 
 * TACTICAL [C | T | L] [<BEARING> <RANGE> | <TARGET-ID>]
 * command inside a unit */
void mech_tacmap(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    int argc, i;
    short x, y;
    int mapx, mapy;
    char *args_vec[4];
    char **args = args_vec;
    MAP *mech_map;
    int displayHeight = MAP_DISPLAY_HEIGHT, displayWidth = MAP_DISPLAY_WIDTH;
    char *str;
    char **maptext;
    int flags = 3, dohexlos = 0;

    /* Basic checks for pilot and mech */
    cch(MECH_USUAL);

    /* Get the map info */
    mech_map = getMap(mech->mapindex);
    mapx = MechX(mech);
    mapy = MechY(mech);

    /* Various checks for conditions and system of mech */
    argc = mech_parseattributes(buffer, args, 4);
    DOCHECK(!MechTacRange(mech), "Your system seems to be inoperational.");

    if (MapIsDark(mech_map) || (MechType(mech) == CLASS_MW &&
    				mudconf.btech_mw_losmap))
	    dohexlos = 1;

    /* Check to see which type of tactical to display 
     * if they specified a particular one */
    if (argc > 0 && isalpha((unsigned char) args[0][0])
	    && args[0][1] == '\0') {
	    
        switch (tolower((unsigned char) args[0][0])) {
	        case 'c':
	            flags |= 8;		/* Show cliffs */
	            break;

	        case 't':
	            flags |= 16;	/* Show tank cliffs */
	            break;

	        case 'l':
	            dohexlos = 1;
	            break;

	        case 'b':
	            flags |= 32;
	            break;

	        default:
	            notify(player, "Invalid tactical map flag.");
	            return;
	    }

	args++;
	argc--;
    }

    DOCHECK(dohexlos && (flags & (8|16|32)), "You can't see that much here!");

    if (!parse_tacargs(player, mech, args, argc, MechTacRange(mech), &x, &y))
	    return;

    /* Get the Tacsize attribute from
     * the player, if doesn't exist set the height and width to
     * default params. If it does exist, check the values and
     * make sure they are legit. */
    str = silly_atr_get(player, A_TACSIZE);
    if (!*str) {
	    displayHeight = MAP_DISPLAY_HEIGHT;
        displayWidth = MAP_DISPLAY_WIDTH;
    } else if (sscanf(str, "%d %d", &displayHeight, &displayWidth) != 2 || 
            displayHeight > 24 || displayHeight < 5 || displayWidth > 40 || 
            displayWidth < 5) {
	
        notify(player,
            "Illegal Tacsize attribute. Must be in format "
            "'Height Width' . Height : 5-24 Width : 5-40");
	    displayHeight = MAP_DISPLAY_HEIGHT;
        displayWidth = MAP_DISPLAY_WIDTH;
    }

    /* Everything worked but lets check the mech's tac range
     * and the map size */
    displayHeight = (displayHeight <= 2 * MechTacRange(mech)
	    ? displayHeight : 2 * MechTacRange(mech));
    displayWidth = (displayWidth <= 2 * MechTacRange(mech)
        ? displayWidth : 2 * MechTacRange(mech));

    displayHeight = (displayHeight <= mech_map->map_height)
	    ? displayHeight : mech_map->map_height;
    displayWidth = (displayWidth <= mech_map->map_width)
        ? displayWidth : mech_map->map_width;

    set_colorscheme(player);

    /* Get the data to draw the map */
    maptext =
	    MakeMapText(player, mech, mech_map, x, y, displayWidth,
	    displayHeight, flags, dohexlos);

    /* Draw the map for the player */
    for (i = 0; maptext[i]; i++)
	    notify(player, maptext[i]);
}


/* XXX Fix 'enterbase <dir>' */
static void mech_enter_event(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data, *tmpm = NULL;
    mapobj *mapo;
    MAP *map = getMap(mech->mapindex), *newmap;
    int target = (int) e->data2;
    int x, y;

    if (!(mapo = find_entrance_by_xy(map, MechX(mech), MechY(mech))))
	return;
    if (!Started(mech) || Uncon(mech) || Jumping(mech) ||
	(MechType(mech) == CLASS_MECH && (Fallen(mech) || Standing(mech)))
	|| OODing(mech) || (fabs(MechSpeed(mech)) * 5 >= MMaxSpeed(mech) &&
	    fabs(MMaxSpeed(mech)) >= MP1) || (MechType(mech) == CLASS_VTOL
	    && AeroFuel(mech) <= 0))
	return;
    if (!(newmap = getMap(mapo->obj)))
	return;
    if (!find_entrance(newmap, target, &x, &y))
	return;

    if (!can_pass_lock(mech->mynum, newmap->mynum, A_LENTER) &&
	    (BuildIsSafe(newmap) || newmap->cf >= (newmap->cfmax / 2))) {
        char *msg = silly_atr_get(newmap->mynum, A_FAIL);
        if (!msg || !*msg)
            msg = "The hangar is locked."; 
        mech_notify(mech, MECHALL, msg);
        return;
    }

    StopBSuitSwarmers(FindObjectsData(mech->mapindex), mech, 1);
    mech_printf(mech, MECHALL, "You enter %s.",
	    structure_name(mapo));
    MechLOSBroadcast(mech, tprintf("has entered %s at %d,%d.",
	    structure_name(mapo), MechX(mech), MechY(mech)));
    MarkForLOSUpdate(mech);
    if (MechType(mech) == CLASS_MW && !In_Character(mapo->obj)) {
	enter_mw_bay(mech, mapo->obj);
	return;
    }
    if (MechCarrying(mech) > 0)
	tmpm = getMech(MechCarrying(mech));
    mech_Rsetmapindex(GOD, (void *) mech, tprintf("%d", (int) mapo->obj));
    mech_Rsetxy(GOD, (void *) mech, tprintf("%d %d", x, y));
    MechLOSBroadcast(mech, tprintf("has entered %s at %d,%d.",
	    structure_name(mapo), MechX(mech), MechY(mech)));
    loud_teleport(mech->mynum, mapo->obj);
    if (tmpm) {
	mech_Rsetmapindex(GOD, (void *) tmpm, tprintf("%d",
		(int) mapo->obj));
	mech_Rsetxy(GOD, (void *) tmpm, tprintf("%d %d", x, y));
	loud_teleport(tmpm->mynum, mapo->obj);
    }
    auto_cal_mapindex(mech);
}


void mech_enterbase(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    MAP *map, *newmap;
    int x, y;
    mapobj *mapo;
    char target, *tmpc;
    char *args[2];
    int argc;

    char fail_mesg[SBUF_SIZE];

    argc = mech_parseattributes(buffer, args, 2);
    DOCHECK(argc > 1, "Invalid arguments to command!");
    tmpc = args[0];
    if (argc > 0 && *tmpc && !(*(tmpc + 1)))
	target = tolower(*tmpc);
    else
	target = 0;
    cch(MECH_USUAL);
    map = getMap(mech->mapindex);
    /* For now, no dir checks */
    DOCHECK(Jumping(mech), "While in mid-jump? No way.");
    DOCHECK(MechType(mech) == CLASS_MECH && (Fallen(mech) ||
	    Standing(mech)), "Crawl inside? I think not. Stand first.");
    DOCHECK(OODing(mech), "While in mid-flight? No way.");
    DOCHECK(MechType(mech) == CLASS_VTOL &&
	AeroFuel(mech) <= 0, "You lack fuel to maneuver in!");
    DOCHECK(FlyingT(mech) &&
	!Landed(mech),
	"You need to land before you can enter the hangar.");
    DOCHECK(IsDS(mech),
	"Heh, you're trying to be funny, right, a DropShip entering hangar?");
    DOCHECK(fabs(MechSpeed(mech)) * 5 >= MMaxSpeed(mech) &&
	fabs(MMaxSpeed(mech)) >= MP1,
	"You are moving too fast to enter the hangar!");
    DOCHECK(!(mapo =
	    find_entrance_by_xy(map, MechX(mech), MechY(mech))),
	"You see nothing to enter here!");
    /* Wow, *gasp*, we got something to enter */
    if (!(newmap = FindObjectsData(mapo->obj))) {
	mech_notify(mech, MECHALL,
	    "You sense wrongness in fabric of space..");
	SendError(tprintf
	    ("Error: No map existing for mapindex #%d (@ %d,%d of #%d)",
		(int) mapo->obj, mapo->x, mapo->y, mech->mapindex));
	return;
    }
    if (!find_entrance(newmap, target, &x, &y)) {
	mech_notify(mech, MECHALL,
	    "You sense wrongness in fabric of space..");
	SendError(tprintf
	    ("Error: No entrance existing for mapindex #%d (@ %d,%d of #%d)",
		(int) mapo->obj, mapo->x, mapo->y, mech->mapindex));
	return;
    }
    
    if (!can_pass_lock(mech->mynum, newmap->mynum, A_LENTER) &&
	    (BuildIsSafe(newmap) || newmap->cf >= (newmap->cfmax / 2))) {

        /* Trigger FAIL & AFAIL */
        memset(fail_mesg, 0, sizeof(fail_mesg));
        snprintf(fail_mesg, LBUF_SIZE, "The hangar is locked.");

        did_it(player, newmap->mynum, A_FAIL, fail_mesg, 0, NULL, A_AFAIL,
                (char **) NULL, 0);

        return;
    }

    DOCHECK(EnteringHangar(mech), "You are already entering the hangar!");
    /* XXX Check for other mechs in the hex possibly doing this as well (ick) */
    HexLOSBroadcast(map, MechX(mech), MechY(mech),
	"The doors at $h start to open..");
    MECHEVENT(mech, EVENT_ENTER_HANGAR, mech_enter_event, 18,
	(int) target);
}
