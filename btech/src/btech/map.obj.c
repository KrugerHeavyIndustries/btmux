/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "mech.events.h"
#include "create.h"
#include "p.mech.utils.h"
#include "p.mine.h"
#include "p.btechstats.h"

#define FIRESPEED(map) (MAX(20,60 - map->windspeed))

static char *map_types[] =
	{ "FIRE", "SMOKE", "DECO", "MINE", "BUILDING", "LEAVE", "ENTRA",
	"LINKED", "TBITS", "BLZ", NULL
};

mapobj *free_mapobjs = NULL;

#define MAPOBJSTART_MAGICNUM 27
#define MAPOBJEND_MAGICNUM 39

mapobj *next_mapobj(mapobj * m)
{
	return m->next;
}

mapobj *first_mapobj(MAP * map, int type)
{
	return map->mapobj[type];
}

void save_mapobjs(FILE * f, MAP * map)
{
#define outbyte(a) tmpb=(a);fwrite(&tmpb, 1, 1, f);
	int i;
	unsigned char tmpb;
	mapobj *tmp;

	outbyte(MAPOBJSTART_MAGICNUM);
	for(i = 0; i < NUM_MAPOBJTYPES; i++)
		if(map->mapobj[i]) {
			if(i == TYPE_BITS) {
				tmp = map->mapobj[i];
				map_save_bits(f, map, tmp);
			} else
				for(tmp = map->mapobj[i]; tmp; tmp = tmp->next) {
					outbyte(i + 1);
					fwrite(tmp, sizeof(mapobj), 1, f);
				}
		}
	outbyte(0);
	outbyte(MAPOBJEND_MAGICNUM);
}

int find_entrance(MAP * map, char dir, int *x, int *y)
{
	mapobj *tmp;

	for(tmp = first_mapobj(map, TYPE_ENTRANCE); tmp; tmp = next_mapobj(tmp))
		if(!dir || tmp->datac == dir) {
			*x = tmp->x;
			*y = tmp->y;
			return 1;
		}
	return 0;
}

char *structure_name(mapobj * mapo)
{
	static char buf[MBUF_SIZE];

	sprintf(buf, "the %s", Name(mapo->obj));
	return buf;
}

mapobj *find_entrance_by_target(MAP * map, dbref target)
{
	mapobj *tmp;

	for(tmp = first_mapobj(map, TYPE_BUILD); tmp; tmp = next_mapobj(tmp))
		if(tmp->obj == target)
			return tmp;
	return NULL;
}

mapobj *find_entrance_by_xy(MAP * map, int x, int y)
{
	mapobj *tmp;

	for(tmp = first_mapobj(map, TYPE_BUILD); tmp; tmp = next_mapobj(tmp))
		if(tmp->x == x && tmp->y == y)
			return tmp;
	return NULL;
}

mapobj *find_mapobj(MAP * map, int x, int y, int type)
{
	mapobj *tmp;
	int i;

	if(type >= 0) {
		for(tmp = first_mapobj(map, type); tmp; tmp = next_mapobj(tmp))
			if(tmp->x == x && tmp->y == y)
				return tmp;
	} else {
		for(i = 0; i < NUM_MAPOBJTYPES; i++)
			for(tmp = first_mapobj(map, i); tmp; tmp = next_mapobj(tmp))
				if(tmp->x == x && tmp->y == y)
					return tmp;
	}
	return NULL;
}

char find_decorations(MAP * map, int x, int y)
{
	int i;
	mapobj *m;

	for(i = 0; i <= TYPE_LAST_DEC; i++) {
		for(m = first_mapobj(map, i); m; m = next_mapobj(m))
			if(m->x == x && m->y == y)
				return m->datac;
	}
	return 0;
}

void del_mapobj(MAP * map, mapobj * mapob, int type, int zap)
{
	/* Delete the specified mapobj */
	struct mapobj_struct *tmp;

	MAP *tmap;
	if(!(map->flags & MAPFLAG_MAPO))
		return;
	if(map->mapobj[type] != mapob) {
		for(tmp = map->mapobj[type]; tmp->next && tmp->next != mapob;
			tmp = tmp->next);
		if(!tmp->next)
			return;
		tmp->next = mapob->next;
	} else
		map->mapobj[type] = mapob->next;
	/* Then, the silly thing. Decorations, they suck */
	if(type <= TYPE_LAST_DEC) {
		/* Need to alter terrain back to 'usual' */
		if(!(zap & 2))
			SetTerrain(map, mapob->x, mapob->y, mapob->datac);
		if(zap)
			StopDec(mapob);
	}
	if(type == TYPE_BUILD) {
		
		if(tmap = getMap(mapob->obj)) {
			del_mapobjst(tmap, TYPE_LEAVE);
			tmap->onmap = 0;
		}
	}
	mapob->next = free_mapobjs;
	free_mapobjs = mapob;
}

void del_mapobjst(MAP * map, int type)
{
	if(!(map->flags & MAPFLAG_MAPO))
		return;
	while (map->mapobj[type])
		del_mapobj(map, map->mapobj[type], type, 3);
}

void del_mapobjs(MAP * map)
{
	int i;

	for(i = 0; i < NUM_MAPOBJTYPES; i++)
		del_mapobjst(map, i);
	if(map->flags & MAPFLAG_MAPO)
		map->flags &= ~MAPFLAG_MAPO;
}

mapobj *add_mapobj(MAP * map, mapobj ** to, mapobj * from, int flag)
{
	mapobj *realto;

	map->flags |= MAPFLAG_MAPO;
	from->next = *to;
	if(!free_mapobjs) {
		Create(realto, mapobj, 1);
	} else {
		realto = free_mapobjs;
		free_mapobjs = realto->next;
	}
	bcopy(from, realto, sizeof(mapobj));
	*to = realto;
	return realto;
}

static void smoke_dissipation_event(MUXEVENT * e)
{
	MAP *map = (MAP *) e->data;
	mapobj *o = (mapobj *) e->data2;

	del_mapobj(map, o, TYPE_SMOKE, 0);
}

static void fire_dissipation_event(MUXEVENT * e)
{
	MAP *map = (MAP *) e->data;
	mapobj *o = (mapobj *) e->data2;
	int x, y;

	x = o->x;
	y = o->y;
	del_mapobj(map, o, TYPE_FIRE, 0);
	if(IsForestHex(map, x, y)) {
		if(Number(1, 6) < 3)
			SetTerrain(map, x, y, GRASSLAND);
		else
			SetTerrain(map, x, y, ROUGH);
	}
}

int FindXEven(wind, x)
	 int wind;
	 int x;

{
	switch (wind) {
	case 0:
		if(x == 0)
			return 0;
		if(x == 1)
			return -1;
		return 1;
	case 60:
		if(x == 0)
			return 1;
		if(x == 1)
			return 0;
		return 1;
	case 120:
		if(x == 0)
			return 1;
		if(x == 1)
			return 1;
		return 0;
	case 180:
		if(x == 0)
			return 0;
		if(x == 1)
			return 1;
		return -1;
	case 240:
		return x - 1;
	case 300:
		if(x == 0)
			return -1;
		if(x == 1)
			return 0;
		return -1;
	}
	return 0;
}

int FindYEven(wind, y)
	 int wind;
	 int y;

{
	switch (wind) {
	case 0:
		if(y == 0)
			return -1;
		if(y == 1)
			return 0;
		return 0;
	case 60:
		if(y == 0)
			return 0;
		if(y == 1)
			return -1;
		return 1;
	case 120:
		if(y == 0)
			return 1;
		if(y == 1)
			return 0;
		return 1;
	case 180:
		return 1;
	case 240:
		if(y == 0)
			return 1;
		if(y == 1)
			return 1;
		return 0;
	case 300:
		if(y == 0)
			return 0;
		if(y == 1)
			return -1;
		return 1;
	}
	return 0;
}

int FindXOdd(wind, x)
	 int wind;
	 int x;

{
	switch (wind) {
	case 0:
		if(x == 0)
			return 0;
		if(x == 1)
			return 1;
		return -1;
	case 60:
		if(x == 0)
			return 1;
		if(x == 1)
			return 0;
		return 1;
	case 120:
		if(x == 0)
			return 1;
		if(x == 1)
			return 1;
		return 0;
	case 180:
		if(x == 0)
			return 0;
		if(x == 1)
			return 1;
		return -1;
	case 240:
		return x - 1;
	case 300:
		if(x == 0)
			return -1;
		if(x == 1)
			return -1;
		return 0;
	}
	return 0;
}

int FindYOdd(wind, y)
	 int wind;
	 int y;

{
	switch (wind) {
	case 0:
		if(y == 0)
			return -1;
		if(y == 1)
			return -1;
		return -1;
	case 60:
		if(y == 0)
			return -1;
		if(y == 1)
			return -1;
		return 0;
	case 120:
		if(y == 0)
			return 0;
		if(y == 1)
			return -1;
		return 1;
	case 180:
		if(y == 0)
			return 1;
		if(y == 1)
			return 0;
		return 0;
	case 240:
		if(y == 0)
			return 0;
		if(y == 1)
			return 1;
		return -1;
	case 300:
		if(y == 0)
			return -1;
		if(y == 1)
			return 0;
		return -1;
	}
	return 0;
}

#define NUM_SPREAD_HEX 4

void CheckForFire(MAP * map, int x[], int y[])
{
	int i;

	for(i = 0; i < NUM_SPREAD_HEX; i++) {
		if(x[i] < 0 || y[i] < 0)
			continue;
		/* Cackle */
		if(IsForestHex(map, x[i], y[i]))
			add_decoration(map, x[i], y[i], TYPE_FIRE, FIRE, FIRE_DURATION);
	}
}

void CheckForSmoke(MAP * map, int x[], int y[])
{
	int i;

	for(i = 0; i < NUM_SPREAD_HEX; i++) {
		if(x[i] < 0 || y[i] < 0)
			continue;
		if(find_decorations(map, x[i], y[i]))
			continue;
		/* Cackle */
		switch (GetTerrain(map, x[i], y[i])) {
		case BUILDING:
		case WALL:
			continue;
		default:
			break;
		}
		add_decoration(map, x[i], y[i], TYPE_SMOKE, SMOKE, SMOKE_DURATION);
	}
}

static void FindMyCoord(MAP * map, int tx, int ty, int i, int wdir, int *x,
						int *y)
{
	int dx, dy;

	wdir = (((wdir + 30) / 60) * 60) % 360;
	if(tx % 2) {
		dx = tx + FindXOdd(wdir, i);
		dy = ty + FindYOdd(wdir, i);
	} else {
		dx = tx + FindXEven(wdir, i);
		dy = ty + FindYEven(wdir, i);
	}
	if(dx < 0 || dy < 0 || dx >= map->map_width || dy >= map->map_height) {
		*x = -1;
		*y = -1;
		return;
	}
	*x = dx;
	*y = dy;
}

static void fire_spreading_event(MUXEVENT * e)
{
	MAP *map = (MAP *) e->data;
	mapobj *o = (mapobj *) e->data2;
	int x, y, loop;
	int flaggo;
	int new_fire_hex_x[4];
	int new_fire_hex_y[4];
	int new_smoke_hex_x[4];
	int new_smoke_hex_y[4];

/*   if (Number(1, 10) == 3) */

/*     { */

/*       x = o->x; */

/*       y = o->y; */

/*       fire_dissipation_event(e); */

/*       add_decoration(map, x, y, TYPE_SMOKE, SMOKE, SMOKE_DURATION); */

/*       return; */

/*     } */
	x = o->x;
	y = o->y;
	for(loop = 0; loop < 3; loop++) {
		new_fire_hex_x[loop] = -1;
		new_fire_hex_y[loop] = -1;
		FindMyCoord(map, x, y, loop, map->winddir, &new_smoke_hex_x[loop],
					&new_smoke_hex_y[loop]);
	}
	new_fire_hex_x[3] = -1;
	new_fire_hex_y[3] = -1;
	FindMyCoord(map, new_smoke_hex_x[0], new_smoke_hex_y[0], 0,
				map->winddir, &new_smoke_hex_x[3], &new_smoke_hex_y[3]);
	
#define Spr(n,ch) \
  if(Roll() >= ch && Number(1,60) <= map->windspeed) \
    { \
      new_fire_hex_x[n] = new_smoke_hex_x[n]; \
      new_fire_hex_y[n] = new_smoke_hex_y[n]; \
    }
	Spr(0, 9);
	Spr(1, 11);
	Spr(2, 11);
	Spr(3, 12);					/* 2 hexes 'downwind' */
#undef Spr
	CheckForSmoke(map, new_smoke_hex_x, new_smoke_hex_y);
	CheckForFire(map, new_fire_hex_x, new_fire_hex_y);
	flaggo = (o->datas -= FIRESPEED(map));
	if(flaggo > FIRESPEED(map))
		MAPEVENT(map, EVENT_DECORATION, fire_spreading_event,
				 FIRESPEED(map), o);
	else
		MAPEVENT(map, EVENT_DECORATION, fire_dissipation_event, flaggo, o);
}

void add_decoration(MAP * map, int x, int y, int type, char data, int flaggo)
{
	mapobj foo;
	mapobj *tmpo;

	bzero(&foo, sizeof(mapobj));
	foo.x = x;
	foo.y = y;

	if (foo.x < 0 || foo.y < 0 || foo.x >= map->map_width || foo.y >= map->map_height)
		return;

	foo.datac = GetRTerrain(map, x, y);
	/* if (foo.datac) */
	{
		mapobj *m, *m2;
		int i;

		for(i = 0; i <= TYPE_LAST_DEC; i++) {
			for(m = first_mapobj(map, i); m; m = m2) {
				m2 = next_mapobj(m);
				if(m->x == x && m->y == y)
					del_mapobj(map, m, i, 1);
			}
		}
	}
	SetTerrain(map, x, y, data);
	foo.datas = (short) flaggo;
	tmpo = add_mapobj(map, &map->mapobj[type], &foo, 1);
	if(flaggo) {
		if(type == TYPE_SMOKE)
			MAPEVENT(map, EVENT_DECORATION, smoke_dissipation_event,
					 flaggo, tmpo);
		if(type == TYPE_FIRE) {
			foo.datas = foo.datas * FIRESPEED(map) * 4 / 3 / 60;
			foo.datas = MAX(foo.datas, FIRESPEED(map) * 2);
			MAPEVENT(map, EVENT_DECORATION, fire_spreading_event,
					 FIRESPEED(map), tmpo);
		}
	}
}

void load_mapobjs(FILE * f, MAP * map)
{
	unsigned char tmpb;
	int i;
	mapobj tmp;

	fread(&tmpb, 1, 1, f);
	if(tmpb != MAPOBJSTART_MAGICNUM) {
		fprintf(stderr, "Error: No mapobjstart found!");
		return;
	}
	/* Clean out */
	for(i = 0; i < NUM_MAPOBJTYPES; i++)
		map->mapobj[i] = NULL;
	fread(&tmpb, 1, 1, f);
	while (tmpb && !feof(f)) {
		if((tmpb - 1) == TYPE_BITS)
			map_load_bits(f, map);
		else {
			fread(&tmp, sizeof(mapobj), 1, f);
			add_mapobj(map, &map->mapobj[tmpb - 1], &tmp, 0);
			if((tmpb - 1) == TYPE_BUILD)
				possibly_start_building_regen(tmp.obj);
		}
		fread(&tmpb, 1, 1, f);
	}
	fread(&tmpb, 1, 1, f);
	if(tmpb != MAPOBJEND_MAGICNUM)
		fprintf(stderr, "Error: No mapobjend found!");
}

void list_mapobjs(dbref player, MAP * map)
{
	mapobj *tmp;
	int i;

	notify(player, "X   Y   Type  obj   dc   ds     di");
	notify(player, "--------------------------------------------");
	for(i = 0; i < NUM_MAPOBJTYPES; i++)
		for(tmp = first_mapobj(map, i); tmp; tmp = next_mapobj(tmp)) {
			if(i == TYPE_BITS)
				notify(player, "--- MAP/HANGAR INFORMATION OBJECT ---");
			else
				notify_printf(player, "%-3d %-3d %-5s %-5d %-4d %-6d %d",
							  tmp->x, tmp->y, map_types[i], (int) tmp->obj,
							  tmp->datac, tmp->datas, tmp->datai);
		}
	notify(player, "--------------------------------------------");
}

void map_addfire(dbref player, void *data, char *buffer)
{
	/* Entrance-checking code */
	MAP *map = (MAP *) data;
	char *args[4];
	int x, y, d;

	if(mech_parseattributes(buffer, args, 3) != 3) {
		notify(player,
			   "Error: Invalid number of attributes to addfire command.");
		return;
	}
	x = atoi(args[0]);
	y = atoi(args[1]);
	d = atoi(args[2]);
	add_decoration(map, x, y, TYPE_FIRE, FIRE, d);
	notify_printf(player, "Added: Fire at (%d,%d) with duration of %ds.",
				  x, y, d);
}

void map_addsmoke(dbref player, void *data, char *buffer)
{
	MAP *map = (MAP *) data;
	char *args[4];
	int x, y, d;

	if(mech_parseattributes(buffer, args, 3) != 3) {
		notify(player,
			   "Error: Invalid number of attributes to addsmoke command.");
		return;
	}
	x = atoi(args[0]);
	y = atoi(args[1]);
	d = atoi(args[2]);
	add_decoration(map, x, y, TYPE_SMOKE, SMOKE, d);
	notify_printf(player, "Added: Smoke at (%d,%d) with duration of %ds.",
				  x, y, d);
}

/* x y dist */
void map_add_block(dbref player, void *data, char *buffer)
{
	char *args[4];
	int argc;
	int x, y, str;
	MAP *map = (MAP *) data;
	mapobj foo;
	int team = 0;

	if(!map)
		return;
#define READINT(from,to) \
    DOCHECK(Readnum(to,from), "Invalid number!")
	argc = mech_parseattributes(buffer, args, 4);
	DOCHECK(argc < 3 || argc > 4, "Invalid arguments!");
	READINT(args[0], x);
	READINT(args[1], y);
	READINT(args[2], str);
	if(argc == 4)
		READINT(args[3], team);

        DOCHECK(!((x >= 0) && (x < map->map_width) && (y >= 0) &&
	                          (y < map->map_height)), "X,Y out of range!");


	bzero(&foo, sizeof(mapobj));
	foo.x = x;
	foo.y = y;
	foo.datai = str;
	foo.obj = player;
	foo.datac = team;
	add_mapobj(map, &map->mapobj[TYPE_B_LZ], &foo, 1);
	notify_printf(player,
				  "Landingzone-block added to %d,%d (distance: %d)", x, y,
				  str);
}

int is_blocked_lz(MECH * mech, MAP * map, int x, int y)
{
	mapobj *o;
	float fx, fy;
	float tx, ty;

	MapCoordToRealCoord(x, y, &fx, &fy);
	for(o = first_mapobj(map, TYPE_B_LZ); o; o = next_mapobj(o)) {
		if(abs(x - o->x) > o->datai || abs(y - o->y) > o->datai)
			continue;
		if(o->datac && o->datac == MechTeam(mech))
			continue;
		MapCoordToRealCoord(o->x, o->y, &tx, &ty);
		if(FindHexRange(fx, fy, tx, ty) <= o->datai)
			return 1;
	}
	return 0;
}

void map_setlinked(dbref player, void *data, char *buffer)
{
	MAP *map = (MAP *) data;
	mapobj foo;

	bzero(&foo, sizeof(mapobj));
	foo.datac = 1;
	add_mapobj(map, &map->mapobj[TYPE_LINKED], &foo, 1);
	notify_printf(player, "Map set to linked.");
}

int mapobj_del(MAP * map, int x, int y, int tt)
{
	int count = 0;
	mapobj *foo, *foo2;

	for(foo = first_mapobj(map, tt); foo; foo = foo2) {
		foo2 = next_mapobj(foo);
		if(foo->x == x && foo->y == y) {
			del_mapobj(map, foo, tt, 1);
			count++;
		}
	}
	return count;
}

void map_delobj(dbref player, void *data, char *buffer)
{
	MAP *map = (MAP *) data;
	char *args[5];
	mapobj *foo, *foo2;
	int tt, count = 0, mdel = 0;
	int x, y;

	switch (mech_parseattributes(buffer, args, 3)) {
	case 0:
		notify(player,
			   "Error: Invalid number of attributes to delobj command.");	
		return;
	case 1:
		DOCHECK((tt = listmatch(map_types, args[0])) < 0, "Invalid type!");
		for(foo = map->mapobj[tt]; foo; foo = foo2) {
			foo2 = next_mapobj(foo);
			del_mapobj(map, foo, tt, 1);
			count++;
		}
		notify_printf(player, "%d objects deleted!", count);
		if(tt == TYPE_MINE)
			mdel = 1;
		break;
	case 2:
		x = atoi(args[0]);
		y = atoi(args[1]);
		for(tt = 0; tt < NUM_MAPOBJTYPES; tt++)
			for(foo = first_mapobj(map, tt); foo; foo = foo2) {
				foo2 = next_mapobj(foo);
				if(foo->x == x && foo->y == y) {
					if(tt == TYPE_MINE)
						mdel = 1;
					del_mapobj(map, foo, tt, 1);
					count++;
				}
			}
		notify_printf(player, "%d objects at (%d,%d) deleted.", count, x, y);
		break;
	case 3:
		DOCHECK((tt = listmatch(map_types, args[0])) < 0, "Invalid type!");
		x = atoi(args[1]);
		y = atoi(args[2]);
		for(foo = first_mapobj(map, tt); foo; foo = foo2) {
			foo2 = next_mapobj(foo);
			if(foo->x == x && foo->y == y) {
				if(tt == TYPE_MINE)
					mdel = 1;
				del_mapobj(map, foo, tt, 1);
				count++;
			}
		}
		notify_printf(player, "%d %s at (%d,%d) deleted.", count,
					  map_types[tt], x, y);
		break;
	default:
		notify(player, "Invalid number of arguments!");
		return;
	}
	if(mdel)
		recalculate_minefields(map);
}

int update_stats[3];			/* Build / Leave / Entrance */

#define addstat(a) update_stats[(a)]++

struct {
	int x, y;
	char dir;
} dirtable[4] = {
	{
	1, 0, 'n'}, {
	2, 1, 'e'}, {
	1, 2, 's'}, {
	0, 1, 'w'}
};

void recursively_updatelinks(dbref from, dbref loc);

int parse_coord(MAP * map, int dir, char *data, int *x, int *y)
{
	int tx, ty, tox, toy;
	int doh;

	if(strchr(data, ',')) {
		if(sscanf(data, "%d,%d", x, y) == 2)
			return 1;
		return 0;
	}
	doh = atoi(data);
	if(doh < 0)
		return 0;
	tox = dirtable[dir].x;
	toy = dirtable[dir].y;
	tx = (map->map_width * tox) / 2;
	if(tx >= map->map_width)
		tx = map->map_width - 1;
	ty = (map->map_height * toy) / 2;
	if(ty >= map->map_height)
		ty = map->map_height - 1;
	if(tox == 1)
		ty += (toy > 1) ? (0 - doh) : (doh);
	if(toy == 1)
		tx += (tox > 1) ? (0 - doh) : (doh);
	if(tx < 0)
		tx = 0;
	if(ty < 0)
		ty = 0;
	if(tx >= map->map_width)
		tx = (map->map_width - 1);
	if(ty >= map->map_height)
		ty = (map->map_height - 1);
	*x = tx;
	*y = ty;
	return 1;
}

void add_entrances(dbref loc, MAP * map, char *data)
{
	char *buf;
	char *args[4];
	int x, y, i;
	mapobj foo;

	bzero(&foo, sizeof(mapobj));

	buf = alloc_mbuf("add_entrances");

	strcpy(buf, data);
	if(mech_parseattributes(buf, args, 4) == 4) {
		for(i = 0; i < 4; i++)
			if((parse_coord(map, i, args[i], &x, &y))) {
				foo.datac = dirtable[i].dir;
				foo.x = x;
				foo.y = y;
				add_mapobj(map, &map->mapobj[TYPE_ENTRANCE], &foo, 1);
				addstat(2);
			}
	}
	free_mbuf(buf);
}

void add_links(dbref loc, MAP * map, char *data)
{
	char *buf;
	char *args[500];
	int i, found, targ;
	char *tmps;
	int x, y;
	mapobj foo;

	bzero(&foo, sizeof(mapobj));

	buf = alloc_lbuf("add_links");

	strcpy(buf, data);
	if((found = mech_parseattributes(buf, args, 500)) > 0)
		for(i = 0; i < found; i++) {
			targ = atoi(args[i]);
			if(targ < 0 || !(FindObjectsData(targ)) || targ == loc)
				continue;
			tmps = silly_atr_get(targ, A_BUILDCOORD);
			if(!tmps)
				continue;
			if(sscanf(tmps, "%d,%d", &x, &y) != 2)
				continue;
			if(x < 0 || x >= map->map_width || y < 0 || y >= map->map_height)
				continue;
			set_hex_enterable(map, x, y);
			foo.x = x;
			foo.y = y;
			foo.obj = targ;
			add_mapobj(map, &map->mapobj[TYPE_BUILD], &foo, 1);
			addstat(0);
			recursively_updatelinks(loc, targ);
		}
	free_lbuf(buf);
}

void recursively_updatelinks(dbref from, dbref loc)
{
	MAP *map;
	mapobj foo;
	char *tmps;

	bzero(&foo, sizeof(mapobj));
	if(!(map = getMap(loc)))
		return;
	clear_hex_bits(map, 2);
	if(from >= 0) {
		map->onmap = from;
		/* Update leave exit */
		del_mapobjst(map, TYPE_LEAVE);
		addstat(1);
		foo.obj = from;
		add_mapobj(map, &map->mapobj[TYPE_LEAVE], &foo, 0);
		del_mapobjst(map, TYPE_ENTRANCE);
		/* Places you can enter this place from.. it's more or less
		   directly taken from BUILDENTRANCE */
		tmps = silly_atr_get(loc, A_BUILDENTRANCE);
		if(tmps) {
			/* number number number number
			   or
			   x,y x,y x,y x,y
			 */
			add_entrances(loc, map, tmps);
		}
	}
	del_mapobjst(map, TYPE_BUILD);
	tmps = silly_atr_get(loc, A_BUILDLINKS);
	if(tmps)
		add_links(loc, map, tmps);
}

void map_updatelinks(dbref player, void *data, char *buffer)
{
	dbref ourloc;

	ourloc = Location(player);
	bzero(update_stats, sizeof(update_stats));
	recursively_updatelinks(NOTHING, ourloc);
	notify_printf(player,
				  "Updated %d BUILD objs, %d LEAVE objs, %d ENTRANCE objs.",
				  update_stats[0], update_stats[1], update_stats[2]);
}

int map_linked(dbref mapobj)
{
	MAP *map = getMap(mapobj);

	if(!map)
		return 0;
	return (first_mapobj(map, TYPE_LINKED)) ? 1 : 0;
}

static int get_building_cf(dbref d, int *i1, int *i2)
{
	MAP *map;

	if(!(map = getMap(d)))
		return 0;
	*i1 = map->cf;
	*i2 = map->cfmax;
	return map->cf;
}

int get_cf(dbref d)
{
	int cf, max = 0;

	if(!(get_building_cf(d, &cf, &max)))
		if(max <= 0)
			return -1;
	return cf;
}

static void set_building_cf(dbref obj, int i1, int i2)
{
	MAP *map;

	if(!(map = getMap(obj)))
		return;
	map->cf = i1;
	map->cfmax = i2;
}

static void building_regen_event(MUXEVENT * e)
{
#ifdef BUILDINGS_REPAIR_THEMSELVES
	dbref d = (dbref) e->data;
	int cf, max;

	if(!get_building_cf(d, &cf, &max))
		return;
	cf = MIN(cf + BUILDING_REPAIR_AMOUNT, max);
	set_building_cf(d, cf, max);
	if(cf != max)
		OBJEVENT(d, EVENT_BREGEN, building_regen_event,
				 BUILDING_REPAIR_DELAY, NULL);
#endif
}

static void building_rebuild_event(MUXEVENT * e)
{
#ifdef BUILDINGS_REBUILD_FROM_DESTRUCTION
	dbref d = (dbref) e->data;
	int cf = 0, max = 0;

	if(get_building_cf(d, &cf, &max))
		return;
	if(max <= 0)
		return;
	set_building_cf(d, 1, max);
#endif
}

void possibly_start_building_regen(dbref obj)
{
	int f, t;

	if(!get_building_cf(obj, &f, &t))
		return;
	if(f == t)
		return;
	if(!f)
		OBJEVENT(obj, EVENT_BREBUILD, building_rebuild_event,
				 BUILDING_DREBUILD_DELAY, NULL);
	else
		OBJEVENT(obj, EVENT_BREGEN, building_regen_event,
				 BUILDING_REPAIR_DELAY, NULL);
}

static void damage_cf(MECH * mech, mapobj * o, int from, int to, int damage)
{
	int destroy = 0;
	int start_regen = 0;

	if(from == to)
		start_regen = 1;
	damage = MIN(from, damage);
	if(from == damage)
		destroy = 1;
	from -= damage;
	set_building_cf(o->obj, from, to);
	if(destroy) {
		mech_printf(mech, MECHALL,
					"You hit %s for %d points of damage, destroying it!",
					structure_name(o), damage);
		notify_except(o->obj, NOTHING, o->obj,
					  tprintf
					  ("%s is hit for %d more points of damage, destroying it!",
					   MyToUpper(structure_name(o)), damage));
		MechLOSBroadcast(mech, tprintf("hits %s, destroying it!",
									   structure_name(o), damage));
		start_regen = 2;
	} else {
		mech_printf(mech, MECHALL,
					"You hit %s for %d points of damage.",
					structure_name(o), damage);
		notify_except(o->obj, NOTHING, o->obj,
					  tprintf("%s is hit for %d points of damage.",
							  MyToUpper(structure_name(o)), damage));
	}
	if(start_regen)
		possibly_start_building_regen(o->obj);
}

void hit_building(MECH * mech, int x, int y, int weapindx, int damage)
{
	mapobj *o;
	MAP *map;
	MAP *nmap;
	int loop, num_missiles_hit, hit_roll;
	int i1, i2;

	if(!(map = getMap(mech->mapindex)))
		return;
	if(!(o = find_entrance_by_xy(map, x, y)))
		return;
	if(!(nmap = getMap(o->obj)))
		return;
	if(!damage) {
		if(!IsMissile(weapindx))
			damage = MechWeapons[weapindx].damage;
		else {
			/* Missile weapon.  Multiple Hit locations... */
			for(loop = 0; MissileHitTable[loop].key != -1; loop++)
				if(MissileHitTable[loop].key == weapindx)
					break;
			if(!(MissileHitTable[loop].key == weapindx))
				return;
			if((MechWeapons[weapindx].type == STREAK) &&
			   (!AngelECMDisturbed(mech)))
				num_missiles_hit = MissileHitTable[loop].num_missiles[10];
			else {
				hit_roll = Roll() - 2;
				num_missiles_hit =
					MissileHitTable[loop].num_missiles[hit_roll];
			}
			damage = num_missiles_hit * MechWeapons[weapindx].damage;
		}
	}
	if(!damage)
		return;
	if(MapIsCS(map) || BuildIsCS(nmap)) {
		mech_notify(mech, MECHALL, "Your shot only scratches the paint!");
		return;
	}
	if(!get_building_cf(o->obj, &i1, &i2))
		return;
	damage_cf(mech, o, i1, i2, damage);
}

void fire_hex(MECH * mech, int x, int y, int meant)
{
	MAP *map;

	if(!(map = getMap(mech->mapindex)))
		return;
	switch (GetTerrain(map, x, y)) {
	case HEAVY_FOREST:
		break;
	case LIGHT_FOREST:
		break;
	default:
		return;
	}
	if(meant) {
		MechLOSBroadcast(mech, tprintf("'s shot ignites %d,%d!", x, y));
		mech_printf(mech, MECHALL, "You ignite %d,%d.", x, y);
	} else {
		MechLOSBroadcast(mech, tprintf("'s stray shot ignites %d,%d!", x, y));
		mech_printf(mech, MECHALL, "You accidentally ignite %d,%d!", x, y);
	}
	add_decoration(map, x, y, TYPE_FIRE, FIRE, FIRE_DURATION);
}

void steppable_base_check(MECH * mech, int x, int y)
{
	mapobj *o;
	MAP *map;
	MAP *nmap;

	map = getMap(mech->mapindex);
	if(!map)
		return;
	if(MechZ(mech) != Elevation(map, x, y))
		return;
	if(!(is_hangar_hex(map, x, y)))
		return;
	if(!(o = find_entrance_by_xy(map, x, y)))
		return;
	if(!(nmap = getMap(o->obj)))
		return;
	if(BuildIsDSS(nmap))
		return;
	if(BuildIsHidden(nmap) && !MadePerceptionRoll(mech, 0))
		return;
	mech_printf(mech, MECHALL, "%s has CF of %d.",
				MyToUpper(structure_name(o)), nmap->cf);
}

void show_building_in_hex(MECH * mech, int x, int y)
{
	mapobj *o;
	MAP *map;
	MAP *nmap;

	if(!(map = getMap(mech->mapindex))) {
		mech_notify(mech, MECHALL,
					"The sensors detect no building in the hex!");
		return;
	}
	if(!(o = find_entrance_by_xy(map, x, y))) {
		mech_notify(mech, MECHALL,
					"The sensors detect no building in the hex!");
		return;
	}
	if(!(nmap = getMap(o->obj))) {
		mech_notify(mech, MECHALL,
					"The sensors detect no building in the hex!");
		return;
	}
	if(BuildIsInvis(nmap) || (BuildIsHidden(nmap) &&
							  !MadePerceptionRoll(mech,
												  (int) (FindRange
														 (MechX(mech),
														  MechY(mech),
														  MechZ(mech), x, y,
														  0) + 0.95)))) {
		mech_notify(mech, MECHALL,
					"The sensors detect no building in the hex!");
		return;
	}
	mech_printf(mech, MECHALL, "%s's CF is %d.",
				MyToUpper(structure_name(o)), nmap->cf);
}

int obj_size(MAP * map)
{
	int s = 0;
	mapobj *m;
	int i;

	for(i = 0; i < NUM_MAPOBJTYPES; i++)
		if(map->mapobj[i])
			for(m = first_mapobj(map, i); m; m = next_mapobj(m))
				s += sizeof(mapobj);
	return s;
}

int map_underlying_terrain(MAP * map, int x, int y)
{
	char c;

	if(!map)
		return 0;
	c = find_decorations(map, x, y);
	if(c)
		return c;
	return GetTerrain(map, x, y);
}

int mech_underlying_terrain(MECH * mech)
{
	char c;
	MAP *map = FindObjectsData(mech->mapindex);

	if(!map)
		return MechTerrain(mech);
	c = find_decorations(map, MechX(mech), MechY(mech));
	if(c)
		return c;
	return MechTerrain(mech);
}
