/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 *
 */

/* This is the place for
   - loadcargo
   - unloadcargo
   - manifest
   - stores
 */

#include <stdio.h>
#include "mech.h"
#include "coolmenu.h"
#include "math.h"
#include "mech.partnames.h"
#include "p.aero.bomb.h"
#include "p.econ.h"
#include "p.mech.partnames.h"
#include "p.crit.h"
#include "p.mech.status.h"
#include "p.mech.utils.h"

#ifdef BT_PART_WEIGHTS
/* From template.c */

extern int internalsweight[];
extern int cargoweight[];
#endif				/* BT_PART_WEIGHTS */

/* Also sets the fuel we have ; but I digress */

void SetCargoWeight(MECH * mech)
{
    int pile[NUM_ITEMS];
    int sw, weight = 0;		/* in 1/10 tons */
    int i, j, k;
    char *t;
    int i1, i2, i3;

    t = silly_atr_get(mech->mynum, A_ECONPARTS);
    bzero(pile, sizeof(pile));
    while (*t) {
	if (*t == '[')
	    if ((sscanf(t, "[%d,%d,%d]", &i1, &i2, &i3)) == 3)
		pile[i1] += ((IsBomb(i1)) ? 4 : 1) * i3;
	t++;
    }
    if (FlyingT(mech))
	for (i = 0; i < NUM_SECTIONS; i++)
	    for (j = 0; j < NUM_CRITICALS; j++) {
		if (IsBomb((k = GetPartType(mech, i, j))))
		    pile[k]++;
		else if (IsSpecial(k))
		    if (Special2I(k) == FUELTANK)
			pile[I2Special(FUELTANK)]++;
	    }
    /* We've 'so-called' pile now */
    for (i = 0; i < NUM_ITEMS; i++)
	if (pile[i]) {
	    sw = GetPartWeight(i);
	    weight += sw * pile[i];
	}
    if (FlyingT(mech)) {
	AeroFuelMax(mech) =
	    AeroFuelOrig(mech) + 2000 * pile[I2Special(FUELTANK)];
	if (AeroFuel(mech) > AeroFuelOrig(mech))
	    weight += AeroFuel(mech) - AeroFuelOrig(mech);
    }
    SetCarriedCargo(mech, weight);
}

/* Returns 1 if calling function should return */

int loading_bay_whine(dbref player, dbref cargobay, MECH * mech)
{
    char *c;
    int i1, i2, i3 = 0;

    c = silly_atr_get(cargobay, A_MECHSKILLS);
    if (c && *c)
	if (sscanf(c, "%d %d %d", &i1, &i2, &i3) >= 2)
	    if (MechX(mech) != i1 || MechY(mech) != i2) {
		notify(player, "You're not where the cargo is!");
		if (i3)
		    notify_printf(player,
			"Try looking around %d,%d instead.", i1,
			    i2);
		return 1;
	    }
    return 0;
}

void mech_Rfixstuff(dbref player, void *data, char *buffer)
{
    int loc = Location(player);
    int pile[BRANDCOUNT + 1][NUM_ITEMS];
    char *t;
    int ol, nl, items = 0, kinds = 0;
    int i1, i2, i3, id, brand;

    if (!data)
	loc = atoi(buffer);
    bzero(pile, sizeof(pile));
    t = silly_atr_get(loc, A_ECONPARTS);
    ol = strlen(t);
    while (*t) {
	if (*t == '[')
	    if ((sscanf(t, "[%d,%d,%d]", &i1, &i2, &i3)) == 3)
		if (!IsCrap(i1))
		    pile[i2][i1] += i3;
	t++;
    }
    silly_atr_set(loc, A_ECONPARTS, "");
    for (id = 0; id < NUM_ITEMS; id++)
	for (brand = 0; brand <= BRANDCOUNT; brand++)
	    if (pile[brand][id] > 0 && get_parts_long_name(id, brand)) {
		econ_change_items(loc, id, brand, pile[brand][id]);
		kinds++;
		items += pile[brand][id];
	    }
    t = silly_atr_get(loc, A_ECONPARTS);
    nl = strlen(t);
    notify_printf(player,
	"Fixing done. Original length: %d. New length: %d.", ol,
	    nl);
    notify_printf(player, "Items in new: %d. Unique items in new: %d.",
	    items, kinds);
}


void list_matching(dbref player, char *header, dbref loc, char *buf)
{
    int pile[BRANDCOUNT + 1][NUM_ITEMS];
    int pile2[BRANDCOUNT + 1][NUM_ITEMS];
    char *t, *ch;
    int i1, i2, i3, id, brand;
    int x, i;

#ifdef BT_PART_WEIGHTS
    char tmpstr[LBUF_SIZE];
    int sw = 0;
#endif				/* BT_PART_WEIGHTS */
    coolmenu *c = NULL;
    int found = 0;

    bzero(pile, sizeof(pile));
    bzero(pile2, sizeof(pile2));
    CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
    CreateMenuEntry_Simple(&c, header, CM_ONE | CM_CENTER);
    CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
    /* Then, we go on a mad rampage ;-) */
    t = silly_atr_get(loc, A_ECONPARTS);
    while (*t) {
	if (*t == '[')
	    if ((sscanf(t, "[%d,%d,%d]", &i1, &i2, &i3)) == 3)
		pile[i2][i1] += i3;
	t++;
    }
    i = 0;
    if (buf)
	while (find_matching_long_part(buf, &i, &id, &brand))
	    pile2[brand][id] = pile[brand][id];
    for (i = 0; i < object_count; i++) {
	UNPACK_PART(short_sorted[i]->index, id, brand);
	if ((buf && (x = pile2[brand][id])) || ((!buf &&
		    (x = pile[brand][id])))) {
#ifndef BT_PART_WEIGHTS
	    ch = part_name_long(id, brand);
#else
	    sw = GetPartWeight(id);
	    sprintf(tmpstr, "%s (%.1ft)", part_name_long(id, brand),
	    	    (sw * x) / 1024.0);
	    ch = tmpstr;
#endif				/* BT_PART_WEIGHTS */
	    if (!ch) {
		SendError(tprintf
		    ("#%d in %d encountered odd thing: %d %d/%d's.",
			player, loc, pile[brand][id], id, brand));
		continue;
	    }
	    /* x = amount of things */
	    CreateMenuEntry_Killer(&c, ch, CM_TWO | CM_NUMBER | CM_NOTOG,
		0, x, x);
	    found++;
	}
    }
    if (!found)
	CreateMenuEntry_Simple(&c, "None", CM_ONE);
    CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
    ShowCoolMenu(player, c);
    KillCoolMenu(c);
}

#define MY_DO_LIST(t) \
if (*buffer) \
  list_matching(player, tprintf("Part listing for %s matching %s", Name(t), buffer), t, buffer); \
else \
  list_matching(player, tprintf("Part listing for %s", Name(t)), t, NULL)

void mech_manifest(dbref player, void *data, char *buffer)
{
    while (isspace(*buffer))
	buffer++;
    MY_DO_LIST(Location(player));
}

void mech_stores(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUAL);
    DOCHECK(Location(mech->mynum) != mech->mapindex ||
	In_Character(Location(mech->mynum)),
	"You aren't inside a hangar!");
    if (loading_bay_whine(player, Location(mech->mynum), mech))
	return;
    while (isspace(*buffer))
	buffer++;
    MY_DO_LIST(Location(mech->mynum));
}

#ifdef ECON_ALLOW_MULTIPLE_LOAD_UNLOAD
#define silly_search(func) \
  if (!count) {  \
    i = -1 ; while (func(args[0], &i, &id, &brand)) \
      count++; \
    if (count > 0) sfun = func; }
#else
#define silly_search(func) \
  if (!count) {  \
    i = -1 ; while (func(args[0], &i, &id, &brand)) \
      count++; \
    DOCHECK(count > 1, "Too many matches!"); \
    if (count > 0) sfun = func; }
#endif

/* Handles adding or removing parts/commods from a map or unit's manifest.
 * btaddstores(), addstuff, and removestuff use this.
 */
static void stuff_change_sub(dbref player, char *buffer, dbref loc1,
    dbref loc2, int mod, int mort)
{
    int i = -1, id, brand;
    int count = 0;
    int argc;
    char *args[2];
    char *c;
    int num;
    int (*sfun) (char *, int *i, int *id, int *brand) = NULL;
    int foo = 0;

    argc = mech_parseattributes(buffer, args, 2);
    DOCHECK(argc < 2, "Invalid number of arguments!");
    
    /* 
     * If we hit the max amount of parts addable at once, set quantity
     * to add to max.
     */
    num = atoi(args[1]);
    if (num > ADDSTORES_MAX) {
        num = ADDSTORES_MAX;
    }
    
    DOCHECK(num <= 0, "Invalid amount!");
    silly_search(find_matching_short_part);
    silly_search(find_matching_vlong_part);
    silly_search(find_matching_long_part);
    DOCHECK(count == 0, tprintf("Nothing matches '%s'!", args[0]));
    DOCHECK(!mort && count > 20 &&
	player != GOD,
	tprintf
	("Wizards can't add more than 20 different objtypes at a time. ('%s' matches: %d)",
	    args[0], count));
    if (mort) {
	DOCHECK(Location(player) != loc1, "You ain't in your 'mech!");
	DOCHECK(Location(loc1) != loc2, "You ain't in hangar!");
    }
    i = -1;
#define MY_ECON_MODIFY(loc,num) \
      econ_change_items(loc, id, brand, num); \
      SendEcon(tprintf("#%d %s %d %s %s #%d.", \
			    player, num>0 ? "added": "removed",  \
			    abs(num), (c=get_parts_long_name(id,brand)), \
			    num>0 ? "to": "from", \
			    loc))
    while (sfun(args[0], &i, &id, &brand)) {
	if (mort) {
	    if (mod < 0)
		count = MIN(num, econ_find_items(loc1, id, brand));
	    else
		count = MIN(num, econ_find_items(loc2, id, brand));
	} else
	    count = num;
	foo += count;
	if (!count)
	    continue;
	MY_ECON_MODIFY(loc1, mod * count);
	if (count)
	    switch (mort) {
	    case 0:
		notify_printf(player, "You %s %d %s%s.",
			mod > 0 ? "add" : "remove", count, c,
			count > 1 ? "s" : "");
		break;
	    case 1:
		MY_ECON_MODIFY(loc2, (0 - mod) * count);
		notify_printf(player, "You %s %d %s%s.",
			mod > 0 ? "load" : "unload", count, c,
			count > 1 ? "s" : "");
		break;
	    }
    }
    DOCHECK(!foo, "Nothing matching that criteria was found!");
}

void mech_Raddstuff(dbref player, void *data, char *buffer)
{
    stuff_change_sub(player, buffer, Location(player), -1, 1, 0);
}

void mech_Rremovestuff(dbref player, void *data, char *buffer)
{
    stuff_change_sub(player, buffer, Location(player), -1, -1, 0);
}

void mech_loadcargo(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALO);
    DOCHECK(!(MechSpecials(mech) & CARGO_TECH),
	"This unit cannot haul cargo!");
    DOCHECK(fabs(MechSpeed(mech)) > 0.0, "You're moving too fast!");
    DOCHECK(Location(mech->mynum) != mech->mapindex ||
	In_Character(Location(mech->mynum)), "You aren't inside hangar!");
    if (loading_bay_whine(player, Location(mech->mynum), mech))
	return;
    stuff_change_sub(player, buffer, mech->mynum, mech->mapindex, 1, 1);
    correct_speed(mech);
}

void mech_unloadcargo(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALSO);
    DOCHECK(!(MechSpecials(mech) & CARGO_TECH),
	"This unit cannot haul cargo!");
    stuff_change_sub(player, buffer, mech->mynum, mech->mapindex, -1, 1);
    correct_speed(mech);
}

void mech_Rresetstuff(dbref player, void *data, char *buffer)
{
    notify(player, "Inventory cleaned!");
    silly_atr_set(Location(player), A_ECONPARTS, "");
    SendEcon(tprintf("#%d reset #%d's stuff.", player, Location(player)));
}
