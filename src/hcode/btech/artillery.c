/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 */

/* 
   Artillery code for
   - standard rounds (damage to target hex, damage/2 to neighbor hexes)
   - smoke rounds (to be implemented)
   - fascam rounds (to be implemented) 
 */

#include <math.h>

#include "mech.h"
#include "artillery.h"
#include "mech.events.h"
#include "create.h"
#include "p.artillery.h"
#include "p.mech.fire.h"
#include "p.mech.combat.h"
#include "p.mech.combat.misc.h"
#include "p.mech.damage.h"
#include "p.mech.utils.h"
#include "p.map.obj.h"
#include "p.mech.hitloc.h"
#include "p.mine.h"
#include "spath.h"

struct artillery_shot_type *free_shot_list = NULL;
static void artillery_hit(artillery_shot * s);

static const char *artillery_type(artillery_shot * s)
{
	if(s->type == CL_ARROW || s->type == IS_ARROW)
		return "a missile";
	return "a round";
}

static struct {
	int dir;
	char *desc;
} arty_dirs[] = {
	{
	0, "north"}, {
	60, "northeast"}, {
	90, "east"}, {
	120, "southeast"}, {
	180, "south"}, {
	240, "southwest"}, {
	270, "west"}, {
	300, "northwest"}, {
	0, NULL}
};

static const char *artillery_direction(artillery_shot * s)
{
	float fx, fy, tx, ty;
	int b, d, i, best = -1, bestd = 0;

	MapCoordToRealCoord(s->from_x, s->from_y, &fx, &fy);
	MapCoordToRealCoord(s->to_x, s->to_y, &tx, &ty);
	b = FindBearing(fx, fy, tx, ty);
	for(i = 0; arty_dirs[i].desc; i++) {
		d = abs(b - arty_dirs[i].dir);
		if(best < 0 || d < bestd) {
			best = i;
			bestd = d;
		}
	}
	if(best < 0)
		return "Invalid";
	return arty_dirs[best].desc;
}

int artillery_round_flight_time(float fx, float fy, float tx, float ty)
{
	int delay = MAX(ARTILLERY_MINIMUM_FLIGHT,
					(FindHexRange(fx, fy, tx, ty) / ARTY_SPEED));

	/* XXX Different weapons, diff. speed? */
	return delay;
}

static void artillery_hit_event(MUXEVENT * e)
{
	artillery_shot *s = (artillery_shot *) e->data;

	artillery_hit(s);
	ADD_TO_LIST_HEAD(free_shot_list, next, s);
}

void artillery_shoot(MECH * mech, int targx, int targy, int windex,
					 int wmode, int ishit)
{
	struct artillery_shot_type *s;
	float fx, fy, tx, ty;

	if(free_shot_list) {
		s = free_shot_list;
		free_shot_list = free_shot_list->next;
	} else
		Create(s, artillery_shot, 1);
	s->from_x = MechX(mech);
	s->from_y = MechY(mech);
	s->to_x = targx;
	s->to_y = targy;
	s->type = windex;
	s->mode = wmode;
	s->ishit = ishit;
	s->shooter = mech->mynum;
	s->map = mech->mapindex;
	MechLOSBroadcast(mech, tprintf("shoots %s towards %s!",
								   artillery_type(s),
								   artillery_direction(s)));
	MapCoordToRealCoord(s->from_x, s->from_y, &fx, &fy);
	MapCoordToRealCoord(s->to_x, s->to_y, &tx, &ty);
	muxevent_add(artillery_round_flight_time(fx, fy, tx, ty), 0, EVENT_DHIT,
				 artillery_hit_event, (void *) s, NULL);
}

static int blast_arcf(float fx, float fy, MECH * mech)
{
	int b, dir;

	b = FindBearing(MechFX(mech), MechFY(mech), fx, fy);
	dir = AcceptableDegree(b - MechFacing(mech));
	if(dir > 120 && dir < 240)
		return BACK;
	if(dir > 300 || dir < 60)
		return FRONT;
	if(dir > 180)
		return LEFTSIDE;
	return RIGHTSIDE;
}

#define TABLE_GEN   0
#define TABLE_PUNCH 1
#define TABLE_KICK  2

void blast_hit_hexf(MAP * map, int dam, int singlehitsize, int heatdam,
        float fx, float fy, float tfx, float tfy, char *tomsg,
        char *otmsg, int table, int safeup, int safedown,
        int isunderwater)
{
    MECH *tempMech;
    dllist_node *node;
    int isrear = 0, iscritical = 0, hitloc;
    int damleft, arc, ndam;
    int ground_zero;
    int tx, ty;

    /* Not on a map so just return */
    if (!map)
        return;

    RealCoordToMapCoord(&tx, &ty, fx, fy);
    if (tx < 0 || ty < 0 || tx >= map->width || ty >= map->height)
        return;
    if (!tomsg || !otmsg)
        return;
    if (isunderwater)
        ground_zero = Elevation(map, tx, ty);
    else
        ground_zero = MAX(0, Elevation(map, tx, ty));

    for (node = dllist_head(map->mechs); node; node = dllist_next(node)) {

        if (!(tempMech = getMech((int) dllist_data(node))))
            continue;

        if (MechX(tempMech) != tx || MechY(tempMech) != ty)
            continue;

        /* Far too high.. */
        if (MechZ(tempMech) >= (safeup + ground_zero))
            continue;

        /* Far too below (underwater, mostly) */
        if (MechZ(tempMech) <= (ground_zero - safedown))
            continue;

        MechLOSBroadcast(tempMech, otmsg);
        mech_notify(tempMech, MECHALL, tomsg);
        arc = blast_arcf(tfx, tfy, tempMech);

        if(arc == BACK)
            isrear = 1;

        damleft = dam;

        while (damleft > 0) {
            if(singlehitsize <= damleft)
                ndam = singlehitsize;
            else
                ndam = damleft;

            damleft -= ndam;

            switch (table) {
                case TABLE_PUNCH:
                    if (MechType(tempMech) != CLASS_MECH) {
                        hitloc = FindHitLocation(tempMech, arc, &iscritical, &isrear);
                    } else {
                        hitloc = FindPunchLocation(tempMech, arc);
                    }
                    break;
                case TABLE_KICK:
                    if (MechType(tempMech) != CLASS_MECH) {
                        hitloc = FindHitLocation(tempMech, arc, &iscritical, &isrear);
                    } else {
                        hitloc = FindKickLocation(tempMech, arc);
                    }
                    break;
                default:
                    hitloc =
                        FindHitLocation(tempMech, arc, &iscritical, &isrear);
            }

            DamageMech(tempMech, tempMech, 0, -1, hitloc, isrear,
                    iscritical, ndam, 0, -1, 0, -1, 0, 0);
        }
        heat_effect(NULL, tempMech, heatdam, 0);
    }
}

void blast_hit_hex(MAP * map, int dam, int singlehitsize, int heatdam,
				   int fx, int fy, int tx, int ty, char *tomsg, char *otmsg,
				   int table, int safeup, int safedown, int isunderwater)
{
	float ftx, fty;
	float ffx, ffy;

	MapCoordToRealCoord(tx, ty, &ftx, &fty);
	MapCoordToRealCoord(fx, fy, &ffx, &ffy);
	blast_hit_hexf(map, dam, singlehitsize, heatdam, ffx, ffy, ftx, fty,
				   tomsg, otmsg, table, safeup, safedown, isunderwater);
}

void blast_hit_hexesf(MAP * map, int dam, int singlehitsize, int heatdam,
					  float fx, float fy, float ftx, float fty, char *tomsg,
					  char *otmsg, char *tomsg1, char *otmsg1, int table,
					  int safeup, int safedown, int isunderwater,
					  int doneighbors)
{
	int x1, y1, x2, y2;
	int dm;
	int tx, ty;
	float hx, hy;
	float t = FindXYRange(fx, fy, ftx, fty);

	dm = MAX(1, (int) t + 1);
	blast_hit_hexf(map, dam / dm, singlehitsize, heatdam / dm, fx, fy, ftx,
				   fty, tomsg, otmsg, table, safeup, safedown, isunderwater);
	if(!doneighbors)
		return;
	RealCoordToMapCoord(&tx, &ty, fx, fy);
	for(x1 = (tx - doneighbors); x1 <= (tx + doneighbors); x1++)
		for(y1 = (ty - doneighbors); y1 <= (ty + doneighbors); y1++) {
			int spot;

			if((dm = MyHexDist(tx, ty, x1, y1, 0)) > doneighbors)
				continue;
			if((tx == x1) && (ty == y1))
				continue;
			x2 = BOUNDED(0, x1, map->width - 1);
			y2 = BOUNDED(0, y1, map->height - 1);
			if(x1 != x2 || y1 != y2)
				continue;
			spot = (x1 == tx && y1 == ty);
			MapCoordToRealCoord(x1, y1, &hx, &hy);
			dm++;
			if(!(dam / dm))
				continue;
			blast_hit_hexf(map, dam / dm, singlehitsize, heatdam / dm, hx,
						   hy, ftx, fty, spot ? tomsg : tomsg1,
						   spot ? otmsg : otmsg1, table, safeup, safedown,
						   isunderwater);

			/*
			 * Added in burning woods when a mech's engine goes nova
			 *
			 * -Kipsta
			 * 8/4/99
			 */

			switch (GetRTerrain(map, x1, y1)) {
			case LIGHT_FOREST:
			case HEAVY_FOREST:
				if(!find_decorations(map, x1, y1)) {
					add_decoration(map, x1, y1, TYPE_FIRE, FIRE,
								   FIRE_DURATION);
				}

				break;
			}
		}
}

void blast_hit_hexes(MAP * map, int dam, int singlehitsize, int heatdam,
					 int tx, int ty, char *tomsg, char *otmsg, char *tomsg1,
					 char *otmsg1, int table, int safeup, int safedown,
					 int isunderwater, int doneighbors)
{
	float fx, fy;

	MapCoordToRealCoord(tx, ty, &fx, &fy);
	blast_hit_hexesf(map, dam, singlehitsize, heatdam, fx, fy, fx, fy,
					 tomsg, otmsg, tomsg1, otmsg1, table, safeup, safedown,
					 isunderwater, doneighbors);
}

static void artillery_hit_hex(MAP * map, artillery_shot * s, int type,
							  int mode, int dam, int tx, int ty, int isdirect)
{
	char buf[LBUF_SIZE];
	char buf1[LBUF_SIZE];
	char buf2[LBUF_SIZE];

	/* Safety check -- shouldn't happen */
	if(tx < 0 || tx >= map->width || ty < 0 || ty >= map->height)
		return;

	if((mode & SMOKE_MODE)) {
		/* Add smoke */
		add_decoration(map, tx, ty, TYPE_SMOKE, SMOKE, SMOKE_DURATION);
		return;
	}
	if(mode & MINE_MODE) {
		add_mine(map, tx, ty, dam);
		return;
	}
	if(!(mode & CLUSTER_MODE)) {
		if(isdirect)
			sprintf(buf1, "receives a direct hit!");
		else
			sprintf(buf1, "is hit by fragments!");
		if(isdirect)
			sprintf(buf2, "You receive a direct hit!");
		else
			sprintf(buf2, "You are hit by fragments!");
	} else {
		if(dam > 2)
			strcpy(buf, "bomblets");
		else
			strcpy(buf, "a bomblet");
		sprintf(buf1, "is hit by %s!", buf);
		sprintf(buf2, "You are hit by %s!", buf);
	}
	blast_hit_hex(map, dam, (mode & CLUSTER_MODE) ? 2 : 5, 0, tx, ty, tx,
				  ty, buf2, buf1,
				  (mode & CLUSTER_MODE) ? TABLE_PUNCH : TABLE_GEN, 10, 4, 0);
}

static artillery_shot *hit_neighbors_s;
static int hit_neighbors_type;
static int hit_neighbors_mode;
static int hit_neighbors_dam;

static void artillery_hit_neighbors_callback(MAP * map, int x, int y)
{
	artillery_hit_hex(map, hit_neighbors_s, hit_neighbors_type,
					  hit_neighbors_mode, hit_neighbors_dam, x, y, 0);
}

static void artillery_hit_neighbors(MAP * map, artillery_shot * s,
									int type, int mode, int dam, int tx,
									int ty)
{
	hit_neighbors_s = s;
	hit_neighbors_type = type;
	hit_neighbors_mode = mode;
	hit_neighbors_dam = dam;
	visit_neighbor_hexes(map, tx, ty, artillery_hit_neighbors_callback);
}

static void artillery_cluster_hit(MAP * map, artillery_shot * s, int type,
								  int mode, int dam, int tx, int ty)
{
	/* Main idea: Pick <dam/2> bombs of 2pts each, and scatter 'em
	   over 5x5 area with weighted numbers */
	int xd, yd, x, y;
	int i;

	int targets[5][5];
	int d;

	bzero(targets, sizeof(targets));
	for(i = 0; i < dam; i++) {
		do {
			xd = Number(-2, 0) + Number(0, 2);
			yd = Number(-2, 0) + Number(0, 2);
			x = tx + xd;
			y = ty + yd;
		}
		while (x < 0 || x >= map->width || y < 0 || y >= map->height);
		/* Whee.. it's time to drop a bomb to the hex */
		targets[xd + 2][yd + 2]++;
	}
	for(xd = 0; xd < 5; xd++)
		for(yd = 0; yd < 5; yd++)
			if((d = targets[xd][yd]))
				artillery_hit_hex(map, s, type, mode, d * 2, xd + tx - 2,
								  yd + ty - 2, 1);
}

void artillery_FriendlyAdjustment(dbref mechnum, MAP * map, int x, int y)
{
	MECH *mech;
	MECH *spotter;
	MECH *tempMech = NULL;

	if(!(mech = getMech(mechnum)))
		return;
	/* Ok.. we've a valid guy */
	spotter = getMech(MechSpotter(mech));
	if(!((MechTargX(mech) == x && MechTargY(mech) == y)
		 || (spotter && (MechTargX(spotter) == x &&
						 MechTargY(spotter) == y))))
		return;
	/* Ok.. we've a valid target to adjust fire on */
	/* Now, see if we've any friendlies in LOS.. NOTE: FRIENDLIES ;-) */
	if(spotter) {
		if(MechSeesHex(spotter, map, x, y))
			tempMech = spotter;
	} else
		tempMech = find_mech_in_hex(mech, map, x, y, 2);
	if(!tempMech)
		return;
	if(!Started(tempMech) || !Started(mech))
		return;
	if(spotter) {
		mech_printf(mech, MECHSTARTED,
					"%s sent you some trajectory-correction data.",
					GetMechToMechID(mech, tempMech));
		mech_printf(tempMech, MECHSTARTED,
					"You provide %s with information about the miss.",
					GetMechToMechID(tempMech, mech));
	}
	MechFireAdjustment(mech)++;
}

static void artillery_hit(artillery_shot * s)
{
	/* First, we figure where it exactly hits. Our first-hand information
	   is only whether it hits or not, not _where_ it hits */
	double dir;
	int di;
	int dist;
	int weight;
	MAP *map = getMap(s->map);
	int original_x = 0, original_y = 0;
	int dam = MechWeapons[s->type].damage;

	if(!map)
		return;
	if(!s->ishit) {
		/* Shit! We missed target ;-) */
		/* Time to calculate a new target hex */
		di = Number(0, 359);
		dir = di * TWOPIOVER360;
		dist = Number(2, 7);
		weight = 100 * (dist * 6) / ((dist * 6 + map->windspeed));
		di = (di * weight + map->winddir * (100 - weight)) / 100;
		dist = (dist * weight + (map->windspeed / 6) * (100 - weight)) / 100;
		original_x = s->to_x;
		original_y = s->to_y;
		s->to_x = s->to_x + dist * cos(dir);
		s->to_y = s->to_y + dist * sin(dir);
		s->to_x = BOUNDED(0, s->to_x, map->width - 1);
		s->to_y = BOUNDED(0, s->to_y, map->height - 1);
		/* Time to calculate if any friendlies have LOS to hex,
		   and if so, adjust fire adjustment unless you lack information /
		   have changed target */
	}
	/* It's time to run for your lives, lil' ones ;-) */
	if(!(s->mode & ARTILLERY_MODES))
		HexLOSBroadcast(map, s->to_x, s->to_y, tprintf("%s fire hits $H!",
													   &MechWeapons[s->type].
													   name[3]));
	else if(s->mode & CLUSTER_MODE)
		HexLOSBroadcast(map, s->to_x, s->to_y,
						"A rain of small bomblets hits $H's surroundings!");
	else if(s->mode & MINE_MODE)
		HexLOSBroadcast(map, s->to_x, s->to_y,
						"A rain of small bomblets hits $H!");
	else if(s->mode & SMOKE_MODE)
		HexLOSBroadcast(map, s->to_x, s->to_y,
						tprintf
						("A %s %s hits $h, and smoke starts to billow!",
						 &MechWeapons[s->type].name[3],
						 &(artillery_type(s)[2])));

	/* Basic theory:
	   - smoke / ordinary rounds are spread with the ordinary functions 
	   - mines are otherwise ordinary except no hitting of neighbor hexes
	   - cluster bombs are special 
	 */
	if(!(s->mode & CLUSTER_MODE)) {
		/* Enjoy ourselves in all neighbor hexes, too */
		artillery_hit_hex(map, s, s->type, s->mode, dam, s->to_x, s->to_y, 1);
		if(!(s->mode & MINE_MODE))
			artillery_hit_neighbors(map, s, s->type, s->mode, dam / 2,
									s->to_x, s->to_y);
	} else
		artillery_cluster_hit(map, s, s->type, s->mode, dam, s->to_x,
							  s->to_y);
	if(!s->ishit)
		artillery_FriendlyAdjustment(s->shooter, map, original_x, original_y);
}
