/*
 * Author: Cord Awtry <kipsta@mediaone.net>
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Based on work that was:
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2000 Thomas Wouters
 */

#include "mech.h"
#include "mech.events.h"
#include "p.mech.enhanced.criticals.h"
#include "p.mech.utils.h"
#include "p.mech.combat.h"
#include "p.mech.damage.h"
#include "p.mech.bth.h"
#include "failures.h"

void getWeapData(MECH * mech, int section, int critical, int *wWeapIndex,
				 int *wWeapSize, int *wFirstCrit)
{
	int wCritType = 0;

	/* Get the crit type */
	wCritType = GetPartType(mech, section, critical);

	/* Get the weapon index */
	*wWeapIndex = Weapon2I(wCritType);

	/* Get the max number of crits for this weapon */
	*wWeapSize = GetWeaponCrits(mech, *wWeapIndex);

	/* Find the first crit */
	*wFirstCrit =
		FindFirstWeaponCrit(mech, section, critical, 0, wCritType,
							*wWeapSize);
}

int getCritAddedBTH(MECH * mech, int section, int critical, int rangeBracket)
{
	int wWeapSize = 0;
	int wFirstCrit = 0;
	int wWeapIndex = 0;
	int i;
	int wRetMod = 0;
	int count = 0;
	int nloc, ncrit, stype;

	if(MechType(mech) != CLASS_MECH)
		return 0;

	getWeapData(mech, section, critical, &wWeapIndex, &wWeapSize, &wFirstCrit);


	/* Iterate over the crits and see if we have any enhanced damage */
	for (i = wFirstCrit; i < MIN(NUM_CRITICALS, wFirstCrit + wWeapSize); i++) {
		if (GetPartDamageFlags(mech, section, i) & WEAP_DAM_MODERATE)
			wRetMod++;

		if ((GetPartDamageFlags(mech, section, i) & (WEAP_DAM_EN_FOCUS|WEAP_DAM_MSL_RANGING))
		&& rangeBracket != RANGE_SHORT)
			wRetMod++;
		count++;
	}

	if (count < wWeapSize) { // got split crits
		if (GetSplitData(mech, section, wFirstCrit, &nloc, &ncrit, &stype)) {
			for (i = ncrit; i < (wWeapSize - count); i++) {
				if (GetPartDamageFlags(mech, nloc, i) & WEAP_DAM_MODERATE)
					wRetMod++;
				if ((GetPartDamageFlags(mech, nloc, i) & (WEAP_DAM_EN_FOCUS|WEAP_DAM_MSL_RANGING))
				&& rangeBracket != RANGE_SHORT)
					wRetMod++;
			}
		}
	}

	return wRetMod;
}

int getCritAddedHeat(MECH * mech, int section, int critical)
{
	int wWeapSize = 0;
	int wFirstCrit = 0;
	int wWeapIndex = 0;
	int i;
	int wRetMod = 0;
	int count = 0, nloc, ncrit, stype;

	getWeapData(mech, section, critical, &wWeapIndex, &wWeapSize, &wFirstCrit);


	if(!IsEnergy(wWeapIndex))
		return 0;

	/* Iterate over the crits and see if we have any enhanced damage */
	for(i = wFirstCrit; i < MIN(NUM_CRITICALS, wFirstCrit + wWeapSize); i++) {
		if(GetPartDamageFlags(mech, section, i) & WEAP_DAM_EN_CRYSTAL)
			wRetMod++;
		count++;
	}

	if (count < wWeapSize && MechType(mech) == CLASS_MECH) { // got split crits
		if (GetSplitData(mech, section, wFirstCrit, &nloc, &ncrit, &stype)) {
			for (i = ncrit; i < (wWeapSize - count); i++) {
				if(GetPartDamageFlags(mech, nloc, i) & WEAP_DAM_EN_CRYSTAL)
					wRetMod++;
			}
		}
	}

	return wRetMod;
}

int getCritSubDamage(MECH * mech, int section, int critical)
{
	int wWeapSize = 0;
	int wFirstCrit = 0;
	int wWeapIndex = 0;
	int i;
	int wRetMod = 0;
	int count = 0, nloc, ncrit, stype;

	if(MechType(mech) != CLASS_MECH)
		return 0;

	getWeapData(mech, section, critical, &wWeapIndex, &wWeapSize, &wFirstCrit);


	if(!IsEnergy(wWeapIndex))
		return 0;

	/* Iterate over the crits and see if we have any enhanced damage */
	for(i = wFirstCrit; i < MIN(NUM_CRITICALS, wFirstCrit + wWeapSize); i++) {
		if(GetPartDamageFlags(mech, section, i) & WEAP_DAM_EN_FOCUS)
			wRetMod++;
		count++;
	}

	if (count < wWeapSize) { // got split crits
		if (GetSplitData(mech, section, wFirstCrit, &nloc, &ncrit, &stype)) {
			for (i = ncrit; i < (wWeapSize - count); i++) {
				if(GetPartDamageFlags(mech, nloc, i) & WEAP_DAM_EN_FOCUS)
					wRetMod++;
			}
		}
	}

	return wRetMod;
}

int canWeapExplodeFromDamage(MECH * mech, int section, int critical, int roll)
{
	int wWeapSize = 0;
	int wFirstCrit = 0;
	int wWeapIndex = 0;
	int i;
	int wExplosionCheck = 0;
	int count = 0, nloc, ncrit, stype;

	if(MechType(mech) != CLASS_MECH)
		return 0;

	getWeapData(mech, section, critical, &wWeapIndex, &wWeapSize, &wFirstCrit);


	/* Iterate over the crits and see if we have any enhanced damage */
	for(i = wFirstCrit; i < MIN(NUM_CRITICALS, wFirstCrit + wWeapSize); i++) {
		if (GetPartDamageFlags(mech, section, i) & (WEAP_DAM_EN_CRYSTAL|WEAP_DAM_BALL_AMMO|WEAP_DAM_MSL_AMMO))
			wExplosionCheck++;
		count++;
	}

	if (count < wWeapSize) { // got split crits
		if (GetSplitData(mech, section, wFirstCrit, &nloc, &ncrit, &stype)) {
			for (i = ncrit; i < (wWeapSize - count); i++) {
				if (GetPartDamageFlags(mech, nloc, i) & (WEAP_DAM_EN_CRYSTAL|WEAP_DAM_BALL_AMMO|WEAP_DAM_MSL_AMMO))
					wExplosionCheck++;
			}
		}
	}

	if(wExplosionCheck > 0)
		wExplosionCheck += 1;

	return wExplosionCheck >= roll;
}

int canWeapJamFromDamage(MECH * mech, int section, int critical, int roll)
{
	int wWeapSize = 0;
	int wFirstCrit = 0;
	int wWeapIndex = 0;
	int i;
	int wJamCheck = 0;
	int count = 0, nloc, ncrit, stype;

	if(MechType(mech) != CLASS_MECH)
		return 0;

	getWeapData(mech, section, critical, &wWeapIndex, &wWeapSize, &wFirstCrit);


	/* Iterate over the crits and see if we have any enhanced damage */
	for(i = wFirstCrit; i < MIN(NUM_CRITICALS, wFirstCrit + wWeapSize); i++) {
		if(GetPartDamageFlags(mech, section, i) & WEAP_DAM_BALL_BARREL)
			wJamCheck++;
		count++;
	}

	if (count < wWeapSize) { // got split crits
		if (GetSplitData(mech, section, wFirstCrit, &nloc, &ncrit, &stype)) {
			for (i = ncrit; i < (wWeapSize - count); i++) {
				if(GetPartDamageFlags(mech, nloc, i) & WEAP_DAM_BALL_BARREL)
					wJamCheck++;
			}
		}
	}

	if(wJamCheck > 0)
		wJamCheck += 1;

	return wJamCheck >= roll;
}

int isWeapAmmoFeedLocked(MECH * mech, int section, int critical)
{
	int wWeapSize = 0;
	int wFirstCrit = 0;
	int wWeapIndex = 0;
	int i;
	int count = 0, nloc, ncrit, stype;

	if(MechType(mech) != CLASS_MECH)
		return 0;

	getWeapData(mech, section, critical, &wWeapIndex, &wWeapSize, &wFirstCrit);


	/* Iterate over the crits and see if we have any enhanced damage */
	for(i = wFirstCrit; i < MIN(NUM_CRITICALS, wFirstCrit + wWeapSize); i++) {
		if (GetPartDamageFlags(mech, section, i) & (WEAP_DAM_BALL_AMMO|WEAP_DAM_MSL_AMMO))
			return 1;
		count++;
	}

	if (count < wWeapSize) { // got split crits
		if (GetSplitData(mech, section, wFirstCrit, &nloc, &ncrit, &stype)) {
			for (i = ncrit; i < (wWeapSize - count); i++) {
				if (GetPartDamageFlags(mech, nloc, i) & (WEAP_DAM_BALL_AMMO|WEAP_DAM_MSL_AMMO))
					return 1;
			}
		}
	}

	return 0;
}

int countDamagedSlotsFromCrit(MECH * mech, int section, int critical)
{
	int wWeapSize = 0;
	int wFirstCrit = 0;
	int wWeapIndex = 0;

	getWeapData(mech, section, critical, &wWeapIndex, &wWeapSize,
				&wFirstCrit);

	return countDamagedSlots(mech, section, wFirstCrit, wWeapSize);
}

int countDamagedSlots(MECH * mech, int section, int wFirstCrit, int wWeapSize)
{
        int wCritsDamaged = 0;
        int i;
        int count = 0, nloc, ncrit, stype;

        for(i = wFirstCrit; i < MIN(NUM_CRITICALS, wFirstCrit + wWeapSize); i++) {
                if(PartIsDamaged(mech, section, i))
                        wCritsDamaged++;
                count++;
        }

        if (count < wWeapSize && MechType(mech) == CLASS_MECH) { // got split crits
                if (GetSplitData(mech, section, wFirstCrit, &nloc, &ncrit, &stype)) {
                        for (i = ncrit; i < (wWeapSize - count); i++) {
                                if(PartIsDamaged(mech, nloc, i))
                                        wCritsDamaged++;
                        }
                }
        }

        return wCritsDamaged;
}

int shouldDestroyWeapon(MECH * mech, int section, int critical,
						int incrementCount)
{
	int wCritsDamaged = 0;
	int wWeapSize = 0;
	int wFirstCrit = 0;
	int wWeapIndex = 0;

	if(MechType(mech) != CLASS_MECH)
		return 1;

	getWeapData(mech, section, critical, &wWeapIndex, &wWeapSize,
				&wFirstCrit);

	if(incrementCount)
		wCritsDamaged++;

	wCritsDamaged += countDamagedSlots(mech, section, wFirstCrit, wWeapSize);

	if((wCritsDamaged * 2) > wWeapSize)
		return 1;

	return 0;
}

void scoreEnhancedWeaponCriticalHit(MECH * mech, MECH * attacker, int LOS,
									int section, int critical)
{
	int wWeapSize = 0;
	int wFirstCrit = 0;
	int wWeapIndex = 0;
	int wCritRoll = Roll();
	int tDestroyWeapon = 0;
	int tNoCrit = 0;
	int tModerateCrit = 0;

	getWeapData(mech, section, critical, &wWeapIndex, &wWeapSize,
				&wFirstCrit);

	/* See if we should just destroy the sucker outright */
	if(shouldDestroyWeapon(mech, section, critical, 1))
		tDestroyWeapon = 1;
	else {
		/* Add the total number of damaged slots */
		wCritRoll += countDamagedSlots(mech, section, wFirstCrit, wWeapSize);
		wCritRoll++;
	}

	if(!tDestroyWeapon) {
		/* See what damage we do */
		if(IsEnergy(wWeapIndex)) {
			if(wCritRoll <= 3) {
				tNoCrit = 1;
			} else if(wCritRoll <= 5) {
				tModerateCrit = 1;
			} else if(wCritRoll <= 7) {
				mech_printf(mech, MECHALL,
							"Your %s's focusing mechanism gets knocked out of alignment!!",
							&MechWeapons[wWeapIndex].name[3]);
				GetPartDamageFlags(mech, section, critical) |=
					WEAP_DAM_EN_FOCUS;
			} else if(wCritRoll <= 9) {
				mech_printf(mech, MECHALL,
							"Your %s's charging crystal takes a direct hit!!",
							&MechWeapons[wWeapIndex].name[3]);
				GetPartDamageFlags(mech, section, critical) |=
					WEAP_DAM_EN_CRYSTAL;
			} else {
				tDestroyWeapon = 1;
			}
		} else if(IsMissile(wWeapIndex)) {
			if(wCritRoll <= 3) {
				tNoCrit = 1;
			} else if(wCritRoll <= 5) {
				tModerateCrit = 1;
			} else if(wCritRoll <= 7) {
				mech_printf(mech, MECHALL,
							"Your %s's ranging system takes a hit!!",
							&MechWeapons[wWeapIndex].name[3]);
				GetPartDamageFlags(mech, section, critical) |=
					WEAP_DAM_MSL_RANGING;
			} else if(wCritRoll <= 9) {
				mech_printf(mech, MECHALL,
							"Your %s's ammo feed is damaged!!",
							&MechWeapons[wWeapIndex].name[3]);
				GetPartDamageFlags(mech, section, critical) |=
					WEAP_DAM_MSL_AMMO;
			} else {
				tDestroyWeapon = 1;
			}
		} else if(IsBallistic(wWeapIndex) || IsArtillery(wWeapIndex)) {
			if(wCritRoll <= 3) {
				tNoCrit = 1;
			} else if(wCritRoll <= 5) {
				tModerateCrit = 1;
			} else if(wCritRoll <= 7) {
				mech_printf(mech, MECHALL,
							"Your %s's barrel warps from the damage!!",
							&MechWeapons[wWeapIndex].name[3]);
				GetPartDamageFlags(mech, section, critical) |=
					WEAP_DAM_BALL_BARREL;
			} else if(wCritRoll <= 9) {
				mech_printf(mech, MECHALL,
							"Your %s's ammo feed is damaged!!",
							&MechWeapons[wWeapIndex].name[3]);
				GetPartDamageFlags(mech, section, critical) |=
					WEAP_DAM_BALL_AMMO;
			} else {
				tDestroyWeapon = 1;
			}
		} else {
			tDestroyWeapon = 1;
		}
	}

	if(tDestroyWeapon) {
		mech_printf(mech, MECHALL, "Your %s has been destroyed!!",
					&MechWeapons[wWeapIndex].name[3]);
		DestroyWeapon(mech, section, GetPartType(mech, section, critical),
					  wFirstCrit, 1, wWeapSize);
	} else {
		DamagePart(mech, section, critical);

		if(tNoCrit)
			mech_printf(mech, MECHALL,
						"Your %s takes a hit but suffers no noticeable damage!!",
						&MechWeapons[wWeapIndex].name[3]);
		else if(tModerateCrit) {
			mech_printf(mech, MECHALL,
						"Your %s takes a hit but continues working!!",
						&MechWeapons[wWeapIndex].name[3]);
			GetPartDamageFlags(mech, section, critical) |= WEAP_DAM_MODERATE;
		}
	}
}

void mech_weaponstatus(dbref player, MECH * mech, char *buffer)
{
	int secIter = 0;
	int weapIter = 0;
	int wWeapsInSec = 0;
	int wcWeaps = 0;
	int wDamagedSlots = 0;
	unsigned char weaparray[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	int critical[MAX_WEAPS_SECTION];
	char tempbuff[160];
	char strLocation[80];
	char weapbuff[120];

	cch(MECH_USUALSP);

	notify(player,
		   "=========================WEAPON SYSTEMS STATUS=========================");
	notify(player,
		   "[##] -------- Weapon Name -------- || Location -------- || Status -----");

	for(secIter = 0; secIter < NUM_SECTIONS; secIter++) {
		wWeapsInSec =
			FindWeapons(mech, secIter, weaparray, weapdata, critical);

		if(wWeapsInSec <= 0)
			continue;

		ArmorStringFromIndex(secIter, tempbuff, MechType(mech),
							 MechMove(mech));
		sprintf(strLocation, "%-18.18s", tempbuff);

		for(weapIter = 0; weapIter < wWeapsInSec; weapIter++) {
			sprintf(weapbuff, "[%2d] %-29.29s || ", wcWeaps++,
					&MechWeapons[weaparray[weapIter]].name[3]);

			strcat(weapbuff, strLocation);
			wDamagedSlots = 0;

			if(PartIsBroken(mech, secIter, critical[weapIter]) ||
			   PartTempNuke(mech, secIter,
							critical[weapIter]) == FAIL_DESTROYED)
				strcat(weapbuff, "|| %ch%crDESTROYED%c");
			else {

				if(MechType(mech) == CLASS_MECH)
					wDamagedSlots =
						countDamagedSlotsFromCrit(mech, secIter,
												  critical[weapIter]);

				if(PartIsDisabled(mech, secIter, critical[weapIter]))
					strcat(weapbuff, "|| %cr%chDISABLED%c");
				else if(PartTempNuke(mech, secIter, critical[weapIter])) {
					switch (PartTempNuke(mech, secIter, critical[weapIter])) {
					case FAIL_JAMMED:
						strcat(weapbuff, "|| %cyJAMMED%c");
						break;
					case FAIL_SHORTED:
						strcat(weapbuff, "|| %cbSHORTED%c");
						break;
					case FAIL_EMPTY:
						strcat(weapbuff, "|| %ccEMPTY%c");
						break;
					case FAIL_DUD:
						strcat(weapbuff, "|| %cyDUD%c");
						break;
					case FAIL_AMMOJAMMED:
						strcat(weapbuff, "|| %cyAMMOJAM%c");
						break;
					}
				} else if(wDamagedSlots > 0)
					strcat(weapbuff, "|| %cy%chDAMAGED%c");
				else
					strcat(weapbuff, "|| %cg%chOPERATIONAL%cn");
			}

			notify(player, weapbuff);

			showWeaponDamageAndInfo(player, mech, secIter,
									critical[weapIter]);
		}
	}
}

void showWeaponDamageAndInfo(dbref player, MECH * mech, int section,
							 int critical)
{
	int wWeapSize = 0;
	int wFirstCrit = 0;
	int wWeapIndex = 0;
	int awDamage[3];
	int i;
	int tHasDamagedPart = 0;
	int tPrintSpace = 0;
	int wAmmoLoc = GetPartDesiredAmmoLoc(mech, section, critical);
	char strLocation[80];
	int awNonOpCrits[3];
	int damflag;
        int count = 0, nloc, ncrit, stype;

	getWeapData(mech, section, critical, &wWeapIndex, &wWeapSize,
				&wFirstCrit);

	for(i = 0; i < 3; i++) {
		awDamage[i] = 0;
		awNonOpCrits[i] = 0;
	}

        for(i = wFirstCrit; i < MIN(NUM_CRITICALS, wFirstCrit + wWeapSize); i++) {
                if(PartIsDamaged(mech, section, i)) {
                        tHasDamagedPart = 1;
                        damflag = GetPartDamageFlags(mech, section, i);
                        awDamage[0] += damflag & WEAP_DAM_MODERATE;
                        awDamage[1] += (damflag & (WEAP_DAM_EN_FOCUS|WEAP_DAM_BALL_BARREL|WEAP_DAM_MSL_RANGING));
                        awDamage[2] += (damflag & (WEAP_DAM_EN_CRYSTAL|WEAP_DAM_BALL_AMMO|WEAP_DAM_MSL_AMMO));
                        awNonOpCrits[0]++;
                } else if(PartIsDestroyed(mech, section, i)) {
                        awNonOpCrits[1]++;
                } else if(PartIsDisabled(mech, section, i)) {
                        awNonOpCrits[2]++;
                }
        }

        if (count < wWeapSize && MechType(mech) == CLASS_MECH) {
                if (GetSplitData(mech, section, wFirstCrit, &nloc, &ncrit, &stype)) {
                        for (i = ncrit; i < (wWeapSize - count); i++) {
                                if(PartIsDamaged(mech, nloc, i)) {
                                        tHasDamagedPart = 1;
                                        damflag = GetPartDamageFlags(mech, nloc, i);
                                        awDamage[0] += damflag & WEAP_DAM_MODERATE;
                                        awDamage[1] += (damflag & (WEAP_DAM_EN_FOCUS|WEAP_DAM_BALL_BARREL|WEAP_DAM_MSL_RANGING));
                                        awDamage[2] += (damflag & (WEAP_DAM_EN_CRYSTAL|WEAP_DAM_BALL_AMMO|WEAP_DAM_MSL_AMMO));
                                        awNonOpCrits[0]++;
                                } else if(PartIsDestroyed(mech, nloc, i)) {
                                        awNonOpCrits[1]++;
                                } else if(PartIsDisabled(mech, nloc, i)) {
                                        awNonOpCrits[2]++;
                                }
                        }
                }
        }



	if(tHasDamagedPart) {
		tPrintSpace = 1;

		if(awDamage[0] > 0) {
			notify_printf(player,
						  "      General damage (%d hit%s): +%d to hit.",
						  awDamage[0], awDamage[0] > 1 ? "s" : "",
						  awDamage[0]);
        }

		if(IsEnergy(wWeapIndex)) {
			if(awDamage[1] > 0) {
				notify_printf(player,
							  "      Focus misalignment (%d hit%s): -%d damage. +%d to hit at >%d hexes.",
							  awDamage[1], awDamage[1] > 1 ? "s" : "",
							  awDamage[1], awDamage[1],
							  MechWeapons[wWeapIndex].shortrange);
            }

			if(awDamage[2] > 0) {
				notify_printf(player,
							  "      Charging crystal damage (%d hit%s): +%d heat. Explodes on %d or less.%%c",
							  awDamage[2], awDamage[2] > 1 ? "s" : "",
							  awDamage[2], awDamage[2] + 1);
            }
		} else if(IsMissile(wWeapIndex)) {
			if(awDamage[1] > 0) {
				notify_printf(player,
							  "      Ranging system damage (%d hit%s): +%d to hit at >%d hexes.",
							  awDamage[1], awDamage[1] > 1 ? "s" : "",
							  awDamage[1],
							  MechWeapons[wWeapIndex].shortrange);
            }

			if(awDamage[2] > 0) {
				notify_printf(player,
							  "      Ammo feed damage (%d hit%s): Can't switch ammo. Explodes on %d or less.",
							  awDamage[2], awDamage[2] > 1 ? "s" : "",
							  awDamage[2] + 1);
            }
		} else if(IsBallistic(wWeapIndex) || IsArtillery(wWeapIndex)) {
			if(awDamage[1] > 0) {
				notify_printf(player,
							  "      %cr%chBarrel damage (%d hit%s): Jams on a %d or less.%c",
							  awDamage[1], awDamage[1] > 1 ? "s" : "",
							  awDamage[1] + 1);
            }

			if(awDamage[2] > 0) {
				notify_printf(player,
							  "      Ammo feed damage (%d hit%s): Can't switch ammo. Explodes on %d or less.",
							  awDamage[2], awDamage[2] > 1 ? "s" : "",
							  awDamage[2] + 1);
            }
		}

		if((awDamage[0] == 0) && (awDamage[1] == 0) && (awDamage[2] == 0))
			notify_printf(player, "      Damaged, but fully operational.");
		tPrintSpace = 1;
	}

	if(wAmmoLoc >= 0) {
		ArmorStringFromIndex(wAmmoLoc, strLocation, MechType(mech),
							 MechMove(mech));
		notify_printf(player, "      Prefered ammo source: %s",
					  strLocation);
		tPrintSpace = 1;
	}

	if((awNonOpCrits[0] > 0) || (awNonOpCrits[1] > 0) ||
	   (awNonOpCrits[2] > 0)) {
		notify_printf(player,
					  "      Slot status: Damaged: %d. Destroyed: %d. Disabled: %d",
					  awNonOpCrits[0], awNonOpCrits[1], awNonOpCrits[2]);
		tPrintSpace = 1;
	}

	if(tPrintSpace)
		notify(player, " ");
}
