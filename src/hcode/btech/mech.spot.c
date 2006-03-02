/*
 * Author: Cord Awtry <kipsta@mediaone.net>
 *  Copyright (c) 2001-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 *
 * Based on work that was:
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2001 Thomas Wouters
 */

#include <stdio.h>
#include <stdlib.h>

#include "mech.h"
#include "create.h"
#include "btmacros.h"
#include "mech.events.h"
#include "p.btechstats.h"
#include "p.mech.bth.h"
#include "p.mech.combat.h"
#include "p.mech.combat.misc.h"
#include "p.mech.los.h"
#include "p.mech.utils.h"

int IsArtyMech(MECH * mech)
{
	int weapnum, section, critical, weaptype = -2;

	for(weapnum = 0; weaptype != -1; weapnum++) {
		weaptype = FindWeaponNumberOnMech(mech, weapnum, &section, &critical);
		if(IsArtillery(weaptype))
			return 1;
	}
	return 0;
}

static void mech_check_range(MUXEVENT * e)
{
	MECH *spotter = (MECH *) e->data2, *mech = (MECH *) e->data;
	float range;

	if(!mech)
		return;

	if(MechSpotter(mech) == -1)
		return;

	if(!spotter) {
		mech_notify(mech, MECHALL, "You have lost link with your spotter!");
		MechSpotter(mech) = -1;
		return;
	}
	range = FlMechRange(fl, mech, spotter);
	if(range > 2 * MechRadioRange(spotter) || MechSpotter(spotter) == -1
	   || spotter->mapindex != mech->mapindex) {
		mech_notify(mech, MECHALL, "You have lost link with your spotter!");
		MechSpotter(mech) = -1;
		return;
	}
	MECHEVENT(mech, EVENT_SPOT_CHECK, mech_check_range, SPOT_TICK, spotter);
}

static void mech_spot_event(MUXEVENT * e)
{
	MECH *target, *mech = (MECH *) e->data;
	struct spot_data *sd = (struct spot_data *) e->data2;

	target = (MECH *) sd->target;

	if(MechFX(mech) != sd->mechFX && MechFY(mech) != sd->mechFY &&
	   MechFX(target) != sd->tarFX && MechFY(target) != sd->tarFY) {
		mech_notify(target, MECHALL,
					"The data link was not established due to movement!");
		mech_notify(mech, MECHALL,
					"The data link was not established due to movement!");
		free((void *) e->data2);
		return;
	}
	mech_printf(target, MECHALL, "Data link established with %s.",
				GetMechToMechID(target, mech));
	mech_printf(mech, MECHALL,
				"Data link established with %s, you now have a forward observer.",
				GetMechToMechID(target, mech));
	MechSpotter(mech) = target->mynum;
	MECHEVENT(mech, EVENT_SPOT_CHECK, mech_check_range, SPOT_TICK, target);
	free((void *) e->data2);
}

void ClearFireAdjustments(MAP * map, dbref mech)
{
    MECH *m;
    dllist_node *node;

    for (node = dllist_head(map->mechs); node; node = dllist_next(node)) {

        if (!(m = getMech((int) dllist_data(node))))
            continue;

        if (m->mynum == mech)
            continue;

        if (MechSpotter(m) == mech)
            MechFireAdjustment(m) = 0;
    }
}

void mech_spot(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data, *target;
	char *args[5];
	char targetID[3];
	int argc;
	int LOS = 1;
	dbref targetref;
	float range;
	struct spot_data *dat;
	MAP *mech_map;

	cch(MECH_USUALO);
	mech_map = getMap(mech->mapindex);
	argc = mech_parseattributes(buffer, args, 5);
#ifdef BT_MOVEMENT_MODES
	DOCHECK(MoveModeLock(mech),
			"You cannot spot while using a special movement mode.");
#endif
	DOCHECK(argc != 1, "You may only use mech ID's to set spotter!");
	DOCHECK(MechType(mech) == CLASS_MW,
			"Spot ? You ? What with, your pretty blue eyes ? Hah!");
	targetID[0] = args[0][0];
	targetID[1] = args[0][1];
	targetID[2] = 0;
	targetref = FindTargetDBREFFromMapNumber(mech, targetID);
	if(!strcmp(args[0], "-")) {
		if(MechSpotter(mech) == mech->mynum) {
			mech_notify(mech, MECHALL, "You spot no longer.");
			ClearFireAdjustments(mech_map, mech->mynum);
		} else
			mech_notify(mech, MECHALL,
						"You disable the datalink to spotter.");
		MechSpotter(mech) = -1;
		return;
	}
	if(!strcasecmp(targetID, MechIDS(mech, 0))) {
		MechSpotter(mech) = mech->mynum;
		mech_notify(mech, MECHALL, "You are now set as a spotter.");
		return;
	}
	target = getMech(targetref);
	if(target)
		LOS =
			InLineOfSight(mech, target, MechX(target), MechY(target),
						  FlMechRange(mech_map, mech, target));
	DOCHECK((targetref == -1) ||
			MechTeam(target) != MechTeam(mech),
			"That target does not exist!");

	DOCHECK(MechType(target) == CLASS_MW,
			"Spot ? That puny being ?! What with, those clear brown eyes ? Hah!");
	DOCHECK(MechSpotter(target) != target->mynum,
			"That 'mech is not set up as spotter!");

	if(IsArtyMech(mech) && !LOS) {
		mech_notify(target, MECHALL,
					"Someone is trying to establish a data link with you!");
		mech_notify(mech, MECHALL,
					"You attempt to establish a data link..... please stand by.");
		range = FlMechRange(mech_map, mech, target);
		if(range > 2 * MechRadioRange(target)) {
			mech_notify(mech, MECHALL,
						"That target is our of data link range!");
			return;
		}
		Create(dat, struct spot_data, 1);
		dat->mechFY = MechFY(mech);
		dat->mechFX = MechFX(mech);
		dat->tarFX = MechFX(target);
		dat->tarFY = MechFY(target);
		dat->target = (MECH *) target;
		MECHEVENT(mech, EVENT_SPOT_LOCK, mech_spot_event,
				  WEAPON_TICK * ((int) range / 10 + 5), dat);
		return;
	} else
		DOCHECK(!LOS, "You do not have LOS to that target!")
			MechSpotter(mech) = targetref;
	MechFireAdjustment(mech) = 0;
	mech_printf(mech, MECHALL, "%s set as spotter.",
				GetMechToMechID(mech, target));
}

int FireSpot(dbref player,
			 MECH * mech,
			 MAP * mech_map,
			 int weaponnum, int weapontype, int sight, int section,
			 int critical)
{
	/* Nim 9/11/96 */

	float spot_range, range;
	float enemyX, enemyY, enemyZ = 0;
	int LOS, mapx = 0, mapy = 0;
	MECH *target = NULL, *spotter;
	int spotTerrain;
	int found_target = 0;

	/* No spotter or not IDF weapon lets get outta here */
	if(MechSpotter(mech) == -1 || !(MechWeapons[weapontype].special & IDF))
		return 0;

	spotter = getMech(MechSpotter(mech));
	DOCHECKMP1(!spotter, "There is no spotter avilable to IDF with!");

	if(!(MechSpotter(spotter) == spotter->mynum)) {
		mech_notify(mech, MECH_PILOT, "You do not have a spotter!");
		MechSpotter(mech) = -1;
		return 1;
	}
	DOCHECKMP1(Uncon(spotter), "Your spotter is unconscious!");
	DOCHECKMP1(Blinded(spotter), "Your spotter can't see a thing!");

	/* Is the spotter set to a Mech or to a Hex? */
	if(MechTarget(spotter) != -1) {
		target = getMech(MechTarget(spotter));
		DOCHECKMP1(!target, "Your spotter has invalid target!");
		mapx = MechX(target);
		mapy = MechY(target);
		spot_range = FaMechRange(spotter, target);
		LOS = InLineOfSight(spotter, target, mapx, mapy, spot_range);
		DOCHECKMP1(!LOS, "You spotter does not have a target in LOS!");
		range = FaMechRange(mech, target);
		spotTerrain =
			IsArtillery(weapontype) ? 2 : 1 + AddTerrainMod(spotter,
															target,
															mech_map,
															spot_range,
															0) +
			AttackMovementMods(spotter) + (Locking(spotter)
										   && MechTargComp(spotter) !=
										   TARGCOMP_MULTI)
			? 2 : 0;
		DOCHECK1(IsArtillery(weapontype) &&
				 target,
				 "You can only target hexes with this kind of artillery.");
		if(!sight) {
			AccumulateSpotXP(MechPilot(spotter), spotter, target);
			AccumulateArtyXP(MechPilot(mech), mech, target);
		}
		FireWeapon(mech, mech_map, target, 0, weapontype, weaponnum,
				   section, critical, MechFX(target), MechFY(target), mapx,
				   mapy, range, spotTerrain, sight, 2);
		return 1;
	}
	if(!(MechTargX(spotter) >= 0 && MechTargY(spotter) >= 0)) {
		notify(player, "Your spotter has no target set!");
		return 1;
	}
	if(!IsArtillery(weapontype))
		if((target =
			find_mech_in_hex(mech, mech_map, MechTargX(spotter),
							 MechTargY(spotter), 0))) {
			enemyX = MechFX(target);
			enemyY = MechFY(target);
			enemyZ = MechFZ(target);
			mapx = MechX(target);
			mapy = MechY(target);
			found_target = 1;
		}
	if(!found_target) {
		target = (MECH *) NULL;
		mapx = MechTargX(spotter);
		mapy = MechTargY(spotter);
		enemyZ = ZSCALE * MechTargZ(spotter);
		MapCoordToRealCoord(mapx, mapy, &enemyX, &enemyY);
	}
	spot_range =
		FindRange(MechFX(spotter), MechFY(spotter), MechFZ(spotter),
				  enemyX, enemyY, enemyZ);
	LOS = InLineOfSight(spotter, target, mapx, mapy, spot_range);
	DOCHECK0(!LOS, "That target is not in your spotters line of sight!");
	range =
		FindRange(MechFX(mech), MechFY(mech), MechFZ(mech), enemyX, enemyY,
				  enemyZ);
	spotTerrain =
		IsArtillery(weapontype) ? 2 : 1 + AttackMovementMods(spotter) +
		(Locking(spotter) && MechTargComp(spotter) != TARGCOMP_MULTI)
		? 2 : 0;
	FireWeapon(mech, mech_map, target, 0, weapontype, weaponnum, section,
			   critical, enemyX, enemyY, mapx, mapy, range, spotTerrain,
			   sight, 2);
	return 1;
}
