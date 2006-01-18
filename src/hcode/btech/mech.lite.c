
/*
 * $Id: mech.lite.c,v 1.1.1.1 2005/01/11 21:18:17 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1998 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Wed Mar 18 22:33:16 1998 fingon
 * Last modified: Thu Dec 10 21:47:06 1998 fingon
 *
 */

#include "mech.h"
#include "p.mech.utils.h"

/* If the target is in the front arc, and Line of Sight is not blocked
 * (by terrain, water hexes or more than 2 'points' of wood) and in
 * range, the target is lit.
 */
static int mech_lites_target(MECH * mech, MECH * target)
{
	MAP *map = getMap(mech->mapindex);
	int losflag = MechToMech_LOSFlag(map, mech, target);

	if(!MechLites(mech))
		return 0;
	if(FaMechRange(mech, target) > LITE_RANGE)
		return 0;
	if(!(InWeaponArc(mech, MechFX(target), MechFY(target)) & FORWARDARC))
		return 0;
	if((losflag & MECHLOSFLAG_BLOCK) ||
	   MechLOSFlag_WoodCount(losflag) > 2 ||
	   MechLOSFlag_WaterCount(losflag) != 0)
		return 0;
	return 1;
}

void cause_lite(MECH * mech, MECH * tempMech)
{
	if(MechLit(tempMech))
		return;
	if(mech_lites_target(mech, tempMech)) {
		MechCritStatus(tempMech) |= SLITE_LIT;
		if(MechSLWarn(tempMech))
			mech_notify(tempMech, MECHALL, "You are being illuminated!");
	}
}

void end_lite_check(MECH * mech)
{
	MAP *map = getMap(mech->mapindex);
	MECH *t;
	int i;

	if(!MechLit(mech))
		return;
	if(!map)
		return;
	for(i = 0; i < map->first_free; i++) {
		if(i == mech->mapnumber)
			continue;
		if(!(t = FindObjectsData(map->mechsOnMap[i])))
			continue;
		if(mech_lites_target(t, mech))
			return;
	}
	MechCritStatus(mech) &= ~SLITE_LIT;
	if(MechSLWarn(mech))
		mech_notify(mech, MECHALL, "You are no longer being illuminated.");
}
