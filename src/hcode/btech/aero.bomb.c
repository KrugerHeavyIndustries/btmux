/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Mon Jan  6 15:57:35 1997 fingon
 * Last modified: Mon Jun  8 19:49:25 1998 fingon
 *
 */

#define GMOD 1					/* Acceleration / second, in Z hexes */

#include "mech.h"
#include "aero.bomb.h"
#include "coolmenu.h"
#include "mycool.h"
#include "mech.events.h"
#include "math.h"
#include "create.h"
#include "p.mech.utils.h"
#include "p.artillery.h"
#include "p.econ_cmds.h"

BOMBINFO bombs[] = {
	{"10_Inferno", 10, 1, 30}
	,
	{"10_Cluster", 10, 2, 30}
	,
	{"10_Standard", 10, 0, 130}
	,
	{"50_Inferno", 50, 1, 130}
	,
	{"50_Cluster", 50, 2, 130}
	,
	{"50_Standard", 50, 0, 130}
	,
	{"100_Inferno", 100, 1, 250}
	,
	{"100_Cluster", 100, 2, 250}
	,
	{"100_Standard", 100, 0, 250}
	,
	{NULL, 0, 0, 0}
};

void DestroyBomb(MECH * mech, int loc)
{

}

int BombWeight(int i)
{
	return bombs[i].weight;
}

char *bomb_name(int i)
{
	return bombs[i].name;
}

void bomb_list(MECH * mech, int player)
{
	int bc = 0, fb;
	int i, j, k;
	char location[20];
	static char *types[] = {
		"Standard",
		"Inferno",
		"Cluster",
		NULL
	};
	coolmenu *c = NULL;

	addline();
	cent(tprintf("Bomb payload for %s:", GetMechID(mech)));
	addline();
	for(i = 0; i < NUM_SECTIONS; i++) {
		fb = 1;
		for(j = 0; j < NUM_CRITICALS; j++)
			if(IsBomb((k = GetPartType(mech, i, j)))) {
				k = Bomb2I(k);
				if(fb) {
					ArmorStringFromIndex(i, location, MechType(mech),
										 MechMove(mech));
					fb = 0;
				}
				if(!bc) {
					vsi(tprintf("#  %-20s %-5s %-5s %s", "Location",
								"Weight", "Power", "Type"));
				}
				vsi(tprintf("%-2d %-20s %5d %5d", bc + 1, location,
							bombs[k].weight / 10, bombs[k].aff,
							types[bombs[k].type]));
				bc++;
			}
	}
	if(!bc)
		cent("No bombs installed.");
	addline();
	ShowCoolMenu(player, c);
	KillCoolMenu(c);
}

float calc_dest(MECH * mech, short *x, short *y)
{
	/* Present location */
	float fx = MechFX(mech);
	float fy = MechFY(mech);
	float fz = MechFZ(mech) / ZSCALE;
	float zspd = MechStartFZ(mech) / ZSCALE;
	float t, ot;

	ot = t = (zspd + sqrt(zspd * zspd + 2 * GMOD * fz)) / GMOD;
	t = (float) t / MOVE_TICK;
	fx = fx + MechStartFX(mech) * t;
	fy = fy + MechStartFY(mech) * t;
	RealCoordToMapCoord(x, y, fx, fy);
	return ot;
}

void bomb_aim(MECH * mech, dbref player)
{
	float t;					/* The time of impact */
	char toi[LBUF_SIZE];
	short x, y;

	t = calc_dest(mech, &x, &y);
	sprintf(toi, "%.1f second%s", t, (t >= 2.0 || t < 1.0) ? "" : "s");
	mech_printf(mech, MECHALL,
				"Estimated bomb flight time %s, estimated landing hex %d,%d.",
				toi, x, y);
}

void bomb_hit_hexes(MAP * map, int x, int y, int hitnb, int iscluster,
					int aff_d, int aff_h, char *tomsg, char *otmsg,
					char *tomsg1, char *otmsg1)
{
	blast_hit_hexes(map, aff_d, iscluster ? 2 : 10, aff_h, x, y, tomsg,
					otmsg, tomsg1, otmsg1, 0, 4, 1, 1, hitnb);
}

static void bomb_hit(bomb_shot * s)
{
	switch (bombs[s->type].type) {
	case 0:
		HexLOSBroadcast(s->map, s->x, s->y,
						"A blast rocks the area around $H!");
		bomb_hit_hexes(s->map, s->x, s->y, 1, 0,
					   bombs[s->type].type ==
					   1 ? bombs[s->type].aff / 2 : bombs[s->type].aff,
					   bombs[s->type].type == 1 ? bombs[s->type].aff : 0,
					   "You receive a direct hit!", "receives a direct hit!",
					   "You are hit by shrapnel!", "is hit by shrapnel!");
		break;
	case 1:
		HexLOSBroadcast(s->map, s->x, s->y,
						"A fiery blast occurs in $H, spraying flaming gel everywhere!");
		bomb_hit_hexes(s->map, s->x, s->y, 1, 0,
					   bombs[s->type].type ==
					   1 ? bombs[s->type].aff / 2 : bombs[s->type].aff,
					   bombs[s->type].type == 1 ? bombs[s->type].aff : 0,
					   "You receive a direct hit!", "receives a direct hit!",
					   "You are hit by the globs of flaming gel!",
					   "is hit by the globs!");
		break;
	case 2:
		HexLOSBroadcast(s->map, s->x, s->y,
						"A bomb drops rain of small bomblets in $H's surroundings!");
		bomb_hit_hexes(s->map, s->x, s->y, 1, 1,
					   bombs[s->type].type ==
					   1 ? bombs[s->type].aff / 2 : bombs[s->type].aff,
					   bombs[s->type].type == 1 ? bombs[s->type].aff : 0,
					   "You are hit by ton of small munitions!",
					   "is hit by many small munitions!",
					   "You are hit by some of the small munitions!",
					   "is hit by some small munitions!");
		break;
	}
}

static void bomb_hit_event(MUXEVENT * e)
{
	bomb_shot *s = (bomb_shot *) e->data;

	bomb_hit(s);
	free((void *) s);
}

void simulate_flight(MECH * mech, MAP * map, short *x, short *y, float t)
{
	float fx = MechFX(mech);
	float fy = MechFY(mech);
	float fz = MechFZ(mech);

/*   float fs = MechStartFZ(mech); */
	float delx, dely;
	float dx, dy;
	int i;
	short tx, ty;

	if(t < 1.0)
		return;
	MapCoordToRealCoord(*x, *y, &dx, &dy);
	delx = (dx - fx) / t;
	dely = (dy - fy) / t;
	for(i = 1; i < t; i++) {
		fx = fx + delx;
		fy = fx + dely;
		fz = (float) fz - GMOD;
		RealCoordToMapCoord(&tx, &ty, fx, fy);
		if(tx < 0 || ty < 0 || tx >= map->map_width || ty >= map->map_height)
			continue;
		if(Elevation(map, tx, ty) > (fz / ZSCALE)) {
			*x = tx;
			*y = ty;
		}
	}
}

void bomb_drop(MECH * mech, int player, int bn)
{
	int bc = 0;
	int i, j, k;
	int lloc = 0, lpos = 0;
	float t;
	short x, y;
	int ob;
	int di;
	float dir;
	bomb_shot *s;
	MAP *map;

	DOCHECK(bn < 0, "Negative bomb number? Gimme a break.");
	bn--;
	for(i = 0; i < NUM_SECTIONS; i++)
		for(j = 0; j < NUM_CRITICALS; j++)
			if(IsBomb((k = GetPartType(mech, i, j))) &&
			   !PartIsDestroyed(mech, i, j)) {
				if(bc == bn) {
					lloc = i;
					lpos = j;
				}
				bc++;
			}
	DOCHECK(!bc, "No bombs installed.");
	DOCHECK(!(map = getMap(mech->mapindex)), "You're on invalid map!");
	DOCHECK(bn < 0 ||
			bn >= bc, "No bomb with such number installed! (See BOMB LIST)");
	MechLOSBroadcast(mech,
					 "detaches a small object that starts falling down..");
	k = Bomb2I(GetPartType(mech, lloc, lpos));
	mech_notify(mech, MECHALL, "The ship trembles as you detach a bomb..");
	t = calc_dest(mech, &x, &y);
	ob = (int) t / 10;
	if(MadePilotSkillRoll(mech, 4 + ob) || t < 2.0)
		mech_notify(mech, MECHALL,
					"Despite the slight problems, you keep the craft stable enough to drop the bomb right on target..");
	else {
		mech_notify(mech, MECHALL,
					"The ship's lurches slightly, dropping the bomb off target!");
		ob = 6 * (1 + ob);		/* Max distance missed  */
		ob = MAX(1, (Number(1, ob)) / 2);
		di = Number(0, 359);
		dir = di * TWOPIOVER360;
		x = x + ob * cos(dir);
		y = y + ob * sin(dir);
	}
	simulate_flight(mech, map, &x, &y, t);
	if(x < 0 || y < 0 || x >= map->map_width || y >= map->map_height)
		return;
	SetPartType(mech, lloc, lpos, 0);
	Create(s, bomb_shot, 1);
	s->x = x;
	s->y = y;
	s->type = k;
	s->map = map;
	SetCargoWeight(mech);
	muxevent_add(MAX(1, t), 0, EVENT_DHIT, bomb_hit_event, (void *) s, NULL);
}

void mech_bomb(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	char *args[3];
	int argc;
	int bn;

	cch(MECH_USUALSO);
	DOCHECK(!(argc =
			  mech_parseattributes(buffer, args, 3)),
			"(At least) one option required.");
	DOCHECK(argc > 2, "Too many arguments!");
	if(!strcasecmp(args[0], "list")) {
		bomb_list(mech, player);
		return;
	}
	DOCHECK(Landed(mech), "The craft is landed!");
	if(!strcasecmp(args[0], "aim")) {
		bomb_aim(mech, player);
		return;
	}
	DOCHECK(strcasecmp(args[0], "drop"), "Invalid argument to BOMB!");
	DOCHECK(argc < 2, "The BOMB commands needs to know WHICH bomb to drop!");
	DOCHECK(Readnum(bn, args[1]), "Invalid bomb number!");
	bomb_drop(mech, player, bn);
}
