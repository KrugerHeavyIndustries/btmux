
/*
 * $Id: mine.c,v 1.1.1.1 2005/01/11 21:18:29 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry 
 *       All rights reserved
 *
 * Created: Tue Oct 22 18:25:30 1996 fingon
 * Last modified: Sun Jun 14 22:29:54 1998 fingon
 *
 */

/*
   Different types of mines:

   1 = Standard round (infinite explosions, same damage, to everyone in hex)
   2 = Inferno        (single explosion, adds heat instead) 
   3 = Command-detonated (single explosion, goes off when hears transmission
   on predefined freq - damages neighbor hexes 1/2)
   4 = Vibra          (single explosion, triggered by weight (setting):
   target<=tons<(target+10) = when stepped on
   (target+10*n)<=tons      = when stepped on n hexes away
   tons<target              = no explosion
 */

#include "copyright.h"
#include "config.h"
#include <math.h>
#include "mech.h"
#include "mine.h"
#include "p.artillery.h"
#include "p.map.obj.h"
#include "p.mine.h"
#include "p.mech.utils.h"
#include "p.btechstats.h"
#include "p.template.h"

/* Different types of mines
 *
 * The Trigger and ScenTrigger mines are used to let the MUX
 * know if a unit has moved to a certain spot
 * 
 * The others are the explosive do damage kind */
char *mine_type_names[] = {
    "Standard",
    "Inferno",
    "CD",
    "Vibra",
    "Trigger",
    "ScenTrigger",
    NULL
};

extern int compare_array(char *[], char *);

void add_mine(MAP * map, int x, int y, int dam)
{
    mapobj *o, foo;

    if (is_mine_hex(map, x, y)) {
        for (o = map->mapobj[TYPE_MINE]; o; o = o->next)
            if (o->x == x && o->y == y)
                break;
        if (o)
            return;
    }
    bzero(&foo, sizeof(foo));
    foo.x = x;
    foo.y = y;
    foo.datas = dam;
    foo.datac = MINE_STANDARD;
    add_mapobj(map, &map->mapobj[TYPE_MINE], &foo, 1);
}



static void mine_damage_mechs(MAP * map, int tx, int ty, char *tomsg,
        char *otmsg, char *tomsg1, char *otmsg1, int dam, int heat, int nb)
{
    blast_hit_hexes(map, dam, 5, heat, tx, ty, tomsg, otmsg, tomsg1,
            otmsg1, MINE_TABLE, 2, 1, 1, 1);
}

static void update_mine(MAP * map, mapobj * mine)
{
    int i;

    i = mine->datas;
    i = i * MINE_NEXT_MODIFIER;
    if (i >= MINE_MIN)
        mine->datas = i;
}

void make_mine_explode(MECH * mech, MAP * map, mapobj * o, int x, int y,
    int reason)
{
    int cool = (o->datas >= MINE_MIN);

    if ((o->datac == MINE_TRIGGER)
            && reason != MINE_STEP && reason != MINE_LAND)
        return;
    if (o->datac != MINE_TRIGGER) {
        if (o->datac != MINE_COMMAND) {
            switch (reason) {
                case MINE_STEP:
                    MechLOSBroadcast(mech,
                            tprintf("moves to %d,%d, and triggers a mine!", x, y));
                    mech_notify(mech, MECHALL,
                            tprintf("As you move to %d,%d, you trigger a mine!", x, y));
                    break;
                case MINE_LAND:
                    MechLOSBroadcast(mech, tprintf("triggers a mine!", x, y));
                    mech_notify(mech, MECHALL, "You trigger a mine!");
                    break;
                case MINE_DROP:
                case MINE_FALL:
                    MechLOSBroadcast(mech, tprintf("triggers a mine!", x, y));
                    mech_notify(mech, MECHALL, "You trigger a mine!");
                    break;
            }
        } else
            HexLOSBroadcast(map, o->x, o->y, "A mine explodes in $H!");
    }

    switch (o->datac) {
        case MINE_STANDARD:
            update_mine(map, o);
            mine_damage_mechs(map, o->x, o->y, "A blast of shrapnel hits you!",
                    "is hit by shrapnel!", NULL, NULL, o->datas, 0, 0);
            if (!cool)
                mapobj_del(map, o->x, o->y, TYPE_MINE);
            break;
        case MINE_INFERNO:
            update_mine(map, o);
            mine_damage_mechs(map, o->x, o->y, "Globs of flaming gel hit you!",
                    "is hit by globs of flaming gel!", NULL, NULL, o->datas / 3,
                    o->datas, 0);
            if (!cool)
                mapobj_del(map, o->x, o->y, TYPE_MINE);
            break;
        case MINE_COMMAND:
            unset_hex_mine(map, o->x, o->y);
            mine_damage_mechs(map, o->x, o->y, "A blast of shrapnel hits you!",
                    "is hit by shrapnel!", "A little blast of shrapnel hits you!",
                    "is hit by some of the shrapnel!", o->datas, 0, 1);
            mapobj_del(map, o->x, o->y, TYPE_MINE);
            break;
        case MINE_TRIGGER:
            SendTrigger(tprintf("#%d %s activated trigger at %d,%d.",
                        mech->mynum, GetMechID(mech), o->x, o->y));
            return;
        case MINE_VIBRA:
            unset_hex_mine(map, o->x, o->y);
            if (o->x != x || o->y != y)
                HexLOSBroadcast(map, o->x, o->y, "A mine explodes in $H!");
            mine_damage_mechs(map, o->x, o->y, "A blast of shrapnel hits you!",
                    "is hit by shrapnel!", "A little blast of shrapnel hits you!",
                    "is hit by some of the shrapnel!", o->datas, 0, 1);
            mapobj_del(map, o->x, o->y, TYPE_MINE);
            break;
    }
    recalculate_minefields(map);
}

/* we find the mine(s) that cause this (vibras can do it long-distance),
   and eliminate it */

static void possible_mine_explosion(MECH * mech, MAP * map, int x, int y, int reason) 
{
    mapobj *o, *o2;
    int mdis = (MechRealTons(mech) - 20) / 10;
    float x1, y1, x2, y2, range;

    MapCoordToRealCoord(x, y, &x1, &y1);
    for (o = map->mapobj[TYPE_MINE]; o; o = o2) {

        int real = 1;

        o2 = o->next;
        if (o->x == x && o->y == y) {
            
            switch (o->datac) {

                case MINE_TRIGGER:
                    if (o->datas > MechRealTons(mech))
                        continue;
                    break;
                case MINE_VIBRA:
                    if (o->datai > MechRealTons(mech))
                        continue;   /* No message, just boom */
                    break;
                case MINE_COMMAND:
                    mech_notify(mech, MECHALL,
                            "You spot small bomblets lying on the ground here..");
                    real = 0;
                    continue;
            }

            if (!real)
                return;

            make_mine_explode(mech, map, o, x, y, reason);

        } else if (VIBRO(o->datac)) {

            if (o->datac == MINE_TRIGGER) {

                /* To small let it go */
                if (o->datas > (MechRealTons(mech)))
                    continue;

                MapCoordToRealCoord(o->x, o->y, &x2, &y2);

                /* Out side of range */
                /* Using round here because we get some funky ranges like
                 * 0.999987 and 1.00000072 */
                if (nearbyintf(FindHexRange(x1, y1, x2, y2)) > ((float) o->datai))
                    continue;

                make_mine_explode(mech, map, o, x, y, reason);

            } else if (o->datai < MechRealTons(mech)) {

                if (abs(o->x - x) <= mdis && abs(o->y - y) <= mdis) {

                    /* Possible remote explosion */
                    MapCoordToRealCoord(o->x, o->y, &x2, &y2);
                    if ((range = FindHexRange(x1, y1, x2, y2)) >
                            (MechRealTons(mech) - o->datai) / 10)
                        continue;

                    make_mine_explode(mech, map, o, x, y, reason);
                }
            }
        }
    }
}

void possible_mine_poof(MECH * mech, int reason)
{
    MAP *map = getMap(mech->mapindex);
    int x = MechX(mech);
    int y = MechY(mech);

    if (!is_mine_hex(map, x, y))
        return;

    if (MechZ(mech) > (MechRTerrain(mech) == ICE ? 0 : Elevation(map, x, y)))
        return;

    possible_mine_explosion(mech, map, x, y, reason);
}

void possibly_remove_mines(MECH * mech, int x, int y)
{
    MAP *map = FindObjectsData(mech->mapindex);

    if (!map)
        return;
    if (!is_mine_hex(map, x, y))
        return;

    /* Do the cleaning stuff here */

    /* Ok, we're lazy and just decide that roll of <= 4 removes
       all traces of mines in the hex */
    if (Roll() <= 4) {
        if (mapobj_del(map, x, y, TYPE_MINE)) {
            /* There _was_ something to clear.. no message, we're evil */
            recalculate_minefields(map);
        }
    }
}


/* for now, just put the hexes themselves ; vibras should have larger radius */
/* Added Exile's MINE_TRIGGER changes.  Can set a distance for the
   mine and it will add mines to the hexes within that range - Dany */
static void add_mine_on_map(MAP * map, int x, int y, char type, int data)
{
    int x1, y1;
    int mdis = (100 - data) / 10;
    int t = mdis * 3 / 2;

    if (type == MINE_TRIGGER) {

        float fx, fy, fx1, fy1;

        /* Get the main hex's location in floating values */
        MapCoordToRealCoord(x, y, &fx, &fy);
        
        /* Loop through all the possible hexes within range
         * and add mines to those hexes if they are within
         * range */
        for (x1 = x - data; x1 <= x + data; x1++)
            for (y1 = y - data; y1 <= y + data; y1++) {

                /* Check the range, if in range add a mine */
                /* We round because of weirdness with FindHexRange returning
                 * values like 1.00215 */
                MapCoordToRealCoord(x1, y1, &fx1, &fy1);
                if (nearbyintf(FindHexRange(fx, fy, fx1, fy1)) <= ((float) data))
                    set_hex_mine(map, x1, y1);
            }

    } else if (type >= MINE_LOW && type <= MINE_HIGH) {

        if (VIBRO(type) && mdis) {
            for (x1 = x - mdis; x1 <= (x + mdis); x1++)
                for (y1 = y - mdis; y1 <= (y + mdis); y1++)
                    if ((abs(x1 - x) + abs(y1 - y)) <= t)
                        if (!(x1 < 0 || y1 < 0 || x1 >= map->map_width ||
                                    y1 >= map->map_height))
                            set_hex_mine(map, x1, y1);
        } else {
            set_hex_mine(map, x, y);
        }
    }
}


/* Re-set all the minefield bits on a map */
void recalculate_minefields(MAP * map)
{
    mapobj *o;

    clear_hex_bits(map, 1);
    for (o = map->mapobj[TYPE_MINE]; o; o = o->next)
        add_mine_on_map(map, o->x, o->y, o->datac, o->datai);
}

/* x y type strength <optvalue> */
void map_add_mine(dbref player, void *data, char *buffer) {
    
    char *args[6];
    int argc;
    int x, y, str, type, extra = 0;
    MAP *map = (MAP *) data;
    mapobj foo;

    if (!map)
        return;

#define READINT(from,to) \
    DOCHECK(Readnum(to,from), "Invalid number!")
    
    argc = mech_parseattributes(buffer, args, 6);
    DOCHECK(argc < 4 || argc > 5, "Invalid arguments!");
    READINT(args[0], x);
    READINT(args[1], y);
    READINT(args[3], str);
    
    if (argc == 5)
        READINT(args[4], extra);

    DOCHECK((type = compare_array(mine_type_names, args[2])) < 0,
            "Invalid mine type!");
    DOCHECK(!((x >= 0) && (x < map->map_width) && (y >= 0) &&
	    (y < map->map_height)), "X,Y out of range!");

    bzero(&foo, sizeof(foo));
    foo.x = x;
    foo.y = y;
    foo.datai = extra;
    foo.datas = str;
    foo.datac = type + 1;
    foo.obj = player;
    add_mapobj(map, &map->mapobj[TYPE_MINE], &foo, 1);

    notify(player, tprintf("%s mine added to (%d,%d) (strength: %d / extra: %d)",
	    mine_type_names[type], x, y, str, extra));
    recalculate_minefields(map);
}



void explode_mines(MECH * mech, int chn)
{
    MAP *map = getMap(mech->mapindex);
    mapobj *o, *o2;
    int count = 0;

    if (!map)
        return;
    for (o = map->mapobj[TYPE_MINE]; o; o = o2) {
        o2 = o->next;
        if (o->datac == MINE_COMMAND)
            if (o->datai == chn) {
                make_mine_explode(mech, map, o, 0, 0, 0);
                count++;
            }
    }
    if (count)
        recalculate_minefields(map);
}

void show_mines_in_hex(dbref player, MECH * mech, float range, int x, int y)
{
    MAP *map = getMap(mech->mapindex);
    mapobj *o;

    DOCHECK(!is_mine_hex(map, x, y),
            "You see nothing else of interest in the hex, either.");
    
    for (o = map->mapobj[TYPE_MINE]; o; o = o->next)
        if (o->x == x && o->y == y)
            break;

    DOCHECK(!o, "You see nothing else of interest in the hex, either.");
    DOCHECK(Number(2, 9) < ((int) range),
            "You see nothing else of interest in the hex, either.");
    DOCHECK(!MadePerceptionRoll(mech, 0),
            "You see nothing else of interest in the hex, either.");
    mech_notify(mech, MECHALL,
            "Small bomblets litter the hex, interesting... You vaguely "
            "recall them from some class or other.");
}
