
/*
 * $Id: mech.bth.c,v 1.2 2005/06/23 15:27:04 av1-op Exp $
 *
 * Author: Cord Awtry <kipsta@mediaone.net>
 * Author: Cord Awtry <kipsta@mediaone.net>
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 *
 * Based on work that was:
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2000 Thomas Wouters
 */

#include <math.h>

#include "mech.h"
#include "mech.events.h"
#include "p.mech.bth.h"
#include "p.mech.c3.misc.h"
#include "p.mech.combat.h"
#include "p.mech.enhanced.criticals.h"
#include "p.mech.hitloc.h"
#include "p.mech.los.h"
#include "p.mech.update.h"
#include "p.mech.utils.h"

#ifndef BTH_DEBUG
#define BTHBASE(m,t,n)  baseToHit = n;
#define BTHADD(desc,n)  baseToHit += n;
#define BTHEND(m)
#else
#define BTHBASE(m,t,n)  do { if (t) sprintf(buf, "#%d -> #%d: Base %d", m->mynum, t->mynum, n); else sprintf(buf, "#%d -> (hex): Base %d", m->mynum, n); baseToHit = n; } while (0)
#define BTHADD(desc,n)  do { i = n ; if (i) { sprintf(buf+strlen(buf), ", %s: %s%d", desc, i>0 ? "+" : "", i); baseToHit += i; } } while (0)
#define BTHEND(m)       SendBTHDebug(tprintf("%s.", buf))
#endif

int arc_override;

int FindNormalBTH(MECH * mech,
				  MAP * mech_map,
				  int section,
				  int critical,
				  int weapindx, float range, MECH * target, int indirectFire,
				  dbref * c3Ref)
{
	MECH *spotter = NULL;
	int baseToHit;
	int wFireMode = GetPartFireMode(mech, section, critical);
	int wAmmoMode = GetPartAmmoMode(mech, section, critical);
	int tInWater = 0;
	int tTargetInWater = 0;
	int wTargMoveMod = 0;
	int rangecheck = 0;
#ifdef BTH_DEBUG
	char buf[LBUF_SIZE];
	int i;
#endif
	int j, rbth = 0;
	float enemyX, enemyY, enemyZ;
	int wRangeBracket = RANGE_TOFAR;

	*c3Ref = -1;

	if(target) {
		tInWater = ((MechRTerrain(mech) == WATER) && (MechZ(mech) < 0));
		tTargetInWater = ((MechRTerrain(target) == WATER) &&
						  (MechZ(target) < 0));
	}

	BTHBASE(mech, target, FindPilotGunnery(mech, weapindx));

	if(indirectFire < 1000) {
		spotter = getMech(MechSpotter(mech));

		if(!spotter) {
			mech_notify(mech, MECHALL,
						"Error finding your spotter! (notify a wiz)");
			return 0;
		}

		BTHADD("Spotting", FindPilotSpotting(spotter) - 4);
	}

	/* Our special bother for aeros */
	if(is_aero(mech) && target && !is_aero(target) && !Landed(mech)) {
		BTHADD("Aero strafing", 2);
	};

	/* MW need +2 added per FASA */
	if(target && MechType(target) == CLASS_MW)
		BTHADD("MechWarrior", 2);

	/* add in to-hit mods from criticals */
	BTHADD("MechBTHMod", MechBTH(mech));

	/* add in to-hit mods for section damage */
	BTHADD("MechLocBTHMod", MechSections(mech)[section].basetohit);

	/* Add +1 if we're firing from water */
	if(tInWater)
		BTHADD("InWater", 1);

	/* Add in the rangebase.. */
	rangecheck = EGunRange(weapindx);
	if(wAmmoMode & STINGER_MODE)
		rangecheck += 7;
	if(rangecheck < range) {
		BTHADD("OutOfRange", 1000);
	} else {
		if((MechWeapons[weapindx].min >= range) &&
		   (MechWeapons[weapindx].min > 0)) {
			if(!HotLoading(weapindx, GetPartFireMode(mech, section,
													 critical))) {
				/* if the target is in minimum range then the BTH is as good as it will get */
				rbth = (MechWeapons[weapindx].min - range + 1);
			} else {
				if(mudconf.btech_hotloadaddshalfbthmod) {
					rbth = ((MechWeapons[weapindx].min - range + 2) / 2);
				}
			}

			BTHADD("MinRange", rbth);
		} else if(HasC3(mech) && !C3Destroyed(mech) &&
				  !AnyECMDisturbed(mech) && (MechC3NetworkSize(mech) > 0)) {
			wRangeBracket = FindBTHByC3Range(mech, target,
											 section, weapindx, range,
											 findC3Range(mech, target, range,
														 c3Ref, 1), wAmmoMode,
											 &rbth);

			BTHADD("C3Range", rbth);
		} else if(HasC3i(mech) && !C3iDestroyed(mech) &&
				  !AnyECMDisturbed(mech) && (MechC3iNetworkSize(mech) > 0)) {
			wRangeBracket = FindBTHByC3Range(mech, target,
											 section, weapindx, range,
											 findC3Range(mech, target, range,
														 c3Ref, 0), wAmmoMode,
											 &rbth);

			BTHADD("C3iRange", rbth);
		} else {
			wRangeBracket = FindBTHByRange(mech, target, section,
										   weapindx, range, wFireMode,
										   wAmmoMode, &rbth);
			BTHADD("Range", rbth);
		}
	}
/* I decided to put it here in this rewritten form. To add it to FindBTH*() was a bit
 * convoluted compared to original source method, Exile and 3030/btechmux went a few
 * different path's on internal representation and organization of various weapon data
 * and BTH handling. Someday I might, after I review how some of the same things were
 * done here, port some of that stuff over. (Like GunStat() and such)
 *
 * For now we just do the below stuff. Besides, it might make BTH debug more obvious
 * that putting it into FindBTH*().
 */
	if(MechTargComp(mech) == TARGCOMP_SHORT
	   || MechTargComp(mech) == TARGCOMP_LONG) {
		int tmp_range;

		if(MechWeapons[weapindx].special & PCOMBAT)
			tmp_range = (int) (range * 10 + 0.95);
		else
			tmp_range = (int) (range + 0.95);

		if(tmp_range >
		   (SectionUnderwater(mech, section) ? MechWeapons[weapindx].
			medrange_water : MechWeapons[weapindx].medrange))
			BTHADD("TargComp/Long",
				   MechTargComp(mech) == TARGCOMP_LONG ? -1 : 1);
		else if(tmp_range <=
				(SectionUnderwater(mech, section) ? MechWeapons[weapindx].
				 medrange_water : MechWeapons[weapindx].medrange))
			BTHADD("TargComp/Short",
				   MechTargComp(mech) == TARGCOMP_SHORT ? -1 : 1);
	}

	if(target && MechInfantrySpecials(target) & STEALTH_TECH) {
		if(MechInfantrySpecials(target) & FWL_ACHILEUS_STEALTH_TECH) {
			if(wRangeBracket == RANGE_SHORT)
				BTHADD("FWLStealthBonus", 1);
			else if(wRangeBracket == RANGE_MED)
				BTHADD("FWLStealthBonus", 2);
			else if(wRangeBracket == RANGE_LONG)
				BTHADD("FWLStealthBonus", 3);
		} else if(MechInfantrySpecials(target) & DC_KAGE_STEALTH_TECH) {
			if(wRangeBracket == RANGE_MED)
				BTHADD("DCStealthBonus", 1);
			else if(wRangeBracket == RANGE_LONG)
				BTHADD("DCStealthBonus", 2);
		} else if(MechInfantrySpecials(target) & FC_INFILTRATOR_STEALTH_TECH) {
			if(wRangeBracket == RANGE_MED)
				BTHADD("FCStealthBonus", 1);
			else if(wRangeBracket == RANGE_LONG)
				BTHADD("FCStealthBonus", 2);
		} else if(MechInfantrySpecials(target) &
				  FC_INFILTRATORII_STEALTH_TECH) {
			if(wRangeBracket == RANGE_SHORT)
				BTHADD("FCStealthIIBonus", 1);
			if(wRangeBracket == RANGE_MED)
				BTHADD("FCStealthIIBonus", 1);
			else if(wRangeBracket == RANGE_LONG)
				BTHADD("FCStealthIIBonus", 2);
		}
	}

	/* Add in the movement modifiers */
	if(MechSections(mech)[section].config & STABILIZERS_DESTROYED)
		BTHADD("AttackMoveX2", AttackMovementMods(mech) * 2);
	else
		BTHADD("AttackMove", AttackMovementMods(mech));

	/* Add mods for overheating */
	BTHADD("Overheat", OverheatMods(mech));

	/* Add special weapon mods */
	if(wAmmoMode & AC_AP_MODE)
		BTHADD("ArmorPiercing", 1);

	if(checkAllSections(mech, INARC_HAYWIRE_ATTACHED))
		BTHADD("HaywirePod", 1);

	if(target && (wAmmoMode & NARC_MODE) &&
	   (!(MechWeapons[weapindx].special & NARC)) &&
	   checkAllSections(target, INARC_HOMING_ATTACHED))
		BTHADD("iNARC", -1);

	if(MechWeapons[weapindx].special & PULSE)
		BTHADD("Pulse", -2);

	if(MechWeapons[weapindx].special & MRM)
		BTHADD("MRM", 1);

	if(MechWeapons[weapindx].special & HVYW)
		BTHADD("HeavyWeapon", 1);
	if(target && (wAmmoMode & STINGER_MODE)) {
		if(FlyingT(target) && !Landed(target))
			BTHADD("Stinger (Flying)", -3);
		else if(OODing(target))
			BTHADD("Stinger (OOD)", -1);
		else if(Jumping(target))
			BTHADD("Stinger (Jumping)", 0);
	}

	if(MechWeapons[weapindx].special & ROCKET)
		BTHADD("Rocket Launcher", 1);

	if(target && (MechType(target) == CLASS_VTOL) &&
	   (fabs(MechSpeed(target)) > 0.0 ||
		fabs(MechVerticalSpeed(target)) > 0.0))
		BTHADD("TargetVTOL", 1);

	if(target && MechTargComp(mech) == TARGCOMP_AA) {
		if(!Landed(target)
		   && (FlyingT(target) || Jumping(target) || OODing(target)))
			BTHADD("TargComp/AA-Fly", MechSpecials(mech) & AA_TECH ? -3 : -2);
		else
			BTHADD("TargComp/AA-Ground", 1);
	}

	/* -1 for LBX, unless it's a VTOL... then -3 */
	if(wAmmoMode & LBX_MODE)
		BTHADD("LBX", (target && (MechType(target) == CLASS_VTOL) ? -3 : -1));

	/* Unstable lock */
	if(!arc_override && (!spotter && target &&
						 ((MechTarget(mech) != target->mynum) ||
						  (Locking(mech)
						   && MechTargComp(mech) != TARGCOMP_MULTI)))) {
		if(FindTargetXY(mech, &enemyX, &enemyY, &enemyZ)) {
			if(InWeaponArc(mech, enemyX, enemyY) & (FORWARDARC | TURRETARC))
				BTHADD("UnstableLock/Fwarc", 1);
			else
				BTHADD("UnstableLock", 2);
		} else {
			BTHADD("HipShot-NoLock", 2);
		}
	}

	if(MechTargComp(mech) == TARGCOMP_MULTI) {
		if(FindTargetXY(mech, &enemyX, &enemyY, &enemyZ)) {
			if(!(InWeaponArc(mech, enemyX, enemyY) & FORWARDARC))
				BTHADD("TargComp/MultiSideArc", 1);
		}
	}

	/* -4 for firing at a hex */
	if(!target &&
	   (MechStatus(mech) & (LOCK_HEX | LOCK_BUILDING | LOCK_HEX_IGN |
							LOCK_HEX_CLR)))
		BTHADD("HexBonus", -4);

	/* -2 for firing at someone dropping out of the sky */
	if(target && C_OODing(target))
		BTHADD("OODbonus", -2);

	/* Indirect fire terrain modifiers */
	if(indirectFire < 1000)
		BTHADD("IDFTerrain", indirectFire);

	/* +1 if spotting */
	if(MechSpotter(mech) == mech->mynum)
		BTHADD("Spotting", 1);

	/* if our target is another unit... */
	if(target) {
		/* Add the dig-in bonus */
		if(MechDugIn(target) && (!mudconf.btech_dig_only_fs || (FindAreaHitGroup(mech, target) == FRONT)) &&
		   (MechZ(target) >= MechZ(mech)))
			BTHADD("DugIn", mudconf.btech_digbonus);

		/* -3 if it's a DS... most people can hit the broadside of a barn */
		if(IsDS(target))
			BTHADD("DSBonus", -3);

		/* Add +1 for BSuit dispersion */
		if(MechType(target) == CLASS_BSUIT)
			BTHADD("Bsuitbonus", 1);

		/* Let's see if we're targetting the head */
		if(target && !IsMissile(weapindx) && MechAim(mech) == HEAD && 
			(MechType(target) == CLASS_MECH || MechType(target) == CLASS_MW)) {
			if(Immobile(target))
				BTHADD("HeadTarget", 7);
			else
				BTHADD("HeadTarget-Fake", 25);
		} else {
			if((GetPartFireMode(mech, section, critical) & ON_TC) &&
			   !(MechCritStatus(mech) & TC_DESTROYED) &&
			   !(wAmmoMode & LBX_MODE)) {
				if(MechAim(mech) != NUM_SECTIONS && !Immobile(target))
					BTHADD("TC-Target-NotImmobile", 3);
				else
					BTHADD("TC", -1);
			}
		}

		/* Add aero targetting mods. TODO: Rewrite aero code :) */
		if(MechType(mech) == CLASS_AERO) {
			wTargMoveMod = TargetMovementMods(mech, target, range) * 3 / 4;
		} else {
			wTargMoveMod = TargetMovementMods(mech, target, range);
		}

		if(wAmmoMode & AC_PRECISION_MODE)
			wTargMoveMod = MAX(wTargMoveMod -= 2, 0);

		BTHADD("TargetMove", wTargMoveMod);

		/* Add in the terrain modifier */
		if(indirectFire >= 1000) {
			j = AddTerrainMod(mech, target, mech_map, range, wAmmoMode);
			if(j < 1000)
				BTHADD("Terrain/Light(Sensor)", j);
		}

		if(mudconf.btech_moddamagewithwoods &&
		   IsForestHex(mech_map, MechX(target), MechY(target)) &&
		   ((MechZ(target) - 2) <= Elevation(mech_map, MechX(target),
											 MechY(target)))) {
			if(GetRTerrain(mech_map, MechX(target),
						   MechY(target)) == LIGHT_FOREST)
				BTHADD("Light Woods bonus", -1);
			else if(GetRTerrain(mech_map, MechX(target),
								MechY(target)) == HEAVY_FOREST)
				BTHADD("Heavy Woods bonus", -2);
		}
#ifdef BT_MOVEMENT_MODES
		if(MechStatus2(target) & (SPRINTING | EVADING)) {
			if(MechStatus2(target) & SPRINTING)
				BTHADD("SprintingTarget", -4);
			if(!Fallen(target) && MechStatus2(target) & EVADING)
				BTHADD("EvadingTarget", 1);
/*		BTHADD("EvadingTarget", (FindPilotPiloting(target) >= 6 ? 1 :
					 FindPilotPiloting(target) >= 4 ? 2 :
					 FindPilotPiloting(target) >= 2 ? 3 : 4) +
			(HasBoolAdvantage(MechPilot(target), "speed_demon") ? 1 : 0)); */
		} else if(MoveModeChange(target)) {
			int i = MoveModeData(target);
			if(i & MODE_SPRINT)
				BTHADD("SprintingTargetChanging", -4);
			if(i & MODE_EVADE)
				BTHADD("EvadingTargetChanging", 1);
/*		BTHADD("EvadingTarget", (FindPilotPiloting(target) >= 6 ? 1 :
					 FindPilotPiloting(target) >= 4 ? 2 :
					 FindPilotPiloting(target) >= 2 ? 3 : 4) +
			(HasBoolAdvantage(MechPilot(target), "speed_demon") ? 1 : 0)); */
		}
#endif
	}

	/* Check for damage */
	BTHADD("CritDamage", getCritAddedBTH(mech, section, critical,
										 wRangeBracket));

	BTHEND(mech);
	return baseToHit;
}

int FindArtilleryBTH(MECH * mech,
					 int section, int weapindx, int indirect, float range)
{
	int baseToHit = 11;
	MECH *spotter;

	if(SectionUnderwater(mech, section))
		return 5000;

	if(EGunRange(weapindx) < range)
		return 1000;

	baseToHit += (FindPilotArtyGun(mech) - 4);
	if(indirect) {
		spotter = getMech(MechSpotter(mech));
		if(spotter && spotter != mech)
			baseToHit += (FindPilotSpotting(spotter) - 4) / 2;
		/* the usual +2, added by +1 make +3 */
		if(indirect && (MechSpotter(mech) == -1 ||
						MechSpotter(mech) == mech->mynum))
			baseToHit += 1;
	} else
		baseToHit -= 2;
	return baseToHit - MechFireAdjustment(mech);
}

int FindBTHByRange(MECH * mech, MECH * target, int section,
				   int weapindx, float frange, int firemode, int ammomode,
				   int *wBTH)
{
	int range;
	int wTargetStealth = 0;

	if(target)
		wTargetStealth = (StealthArmorActive(target) ||
						  NullSigSysActive(target));

	if(MechWeapons[weapindx].special & PCOMBAT)
		range = (int) (frange * 10 + 0.95);
	else
		range = (int) (frange + 0.95);

	if(SectionUnderwater(mech, section)) {
		if(MechWeapons[weapindx].shortrange_water <= 0) {
			*wBTH = 5000;
			return RANGE_NOWATER;
		}

		/* Out of range range */
		if(range > EGunWaterRange(weapindx)) {
			*wBTH = 1000;
			return RANGE_TOFAR;
		}

		/* Very long range */
		if(range > GunWaterRange(weapindx)) {
			*wBTH = wTargetStealth ? 12 : 8;
			return RANGE_EXTREME;
		}

		/* Long range... */
		if(range > MechWeapons[weapindx].medrange_water) {
			*wBTH = wTargetStealth ? 6 : 4;
			return RANGE_LONG;
		}

		/* Medium range */
		if(range > MechWeapons[weapindx].shortrange_water) {
			*wBTH = wTargetStealth ? 3 : 2;
			return RANGE_MED;
		}

		/* Short range */
		if(range > MechWeapons[weapindx].min_water) {
			*wBTH = 0;
			return RANGE_SHORT;
		}

		if(range == 0) {
			if(MechWeapons[weapindx].min_water == 0) {
				*wBTH = 0;
				return RANGE_SHORT;
			} else {
				*wBTH = MechWeapons[weapindx].min_water - range;
				return RANGE_SHORT;
			}
		}

		/* Less than or equal to minimum range */
		*wBTH = MechWeapons[weapindx].min_water - range + 1;
	}

	/* Beyond range */
	if(range >
	   ((ammomode & STINGER_MODE) ? (EGunRange(weapindx) + 7)
		: (EGunRange(weapindx)))) {
		*wBTH = 1000;
		return RANGE_TOFAR;
	}

	/* V. Long range */
	if(range > GunRange(weapindx)) {
		*wBTH = wTargetStealth ? 12 : 8;
		return RANGE_EXTREME;
	}

	/* Long range... */
	if(range > MechWeapons[weapindx].medrange) {
		*wBTH = wTargetStealth ? 6 : 4;
		return RANGE_LONG;
	}

	/* Medium range */
	if(range > MechWeapons[weapindx].shortrange) {
		*wBTH = wTargetStealth ? 3 : 2;
		return RANGE_MED;
	}

	/* Short range */
	if(range > MechWeapons[weapindx].min) {
		*wBTH = 0;
		return RANGE_SHORT;
	}
	/* If we are at range 0.0

	 * Added 8/3/99 by Kipsta (to fix a 0.0 bug)
	 */

	if(range == 0) {
		if(MechWeapons[weapindx].min == 0) {
			*wBTH = 0;
			return RANGE_SHORT;
		} else {
			if(!HotLoading(weapindx, firemode)) {
				*wBTH = MechWeapons[weapindx].min - range;
			} else {
				if(mudconf.btech_hotloadaddshalfbthmod)
					*wBTH = ((MechWeapons[weapindx].min - range + 1) / 2);
				else
					*wBTH = 0;
			}

			return RANGE_SHORT;
		}
	}

	if(HotLoading(weapindx, firemode)) {
		if(mudconf.btech_hotloadaddshalfbthmod)
			*wBTH = ((MechWeapons[weapindx].min - range + 1) / 2);
		else
			*wBTH = 0;

		return RANGE_SHORT;
	}

	/* Less than or equal to minimum range */
	*wBTH = MechWeapons[weapindx].min - range + 1;
	return RANGE_SHORT;
}

int FindBTHByC3Range(MECH * mech, MECH * target, int section,
					 int weapindx, float realRange, float c3Range, int mode,
					 int *wBTH)
{
	int realRangeAdj = 0.0;
	int c3RangeAdj = 0.0;
	int wTargetStealth = 0;

	if(target)
		wTargetStealth = (StealthArmorActive(target) ||
						  NullSigSysActive(target));

	if(MechWeapons[weapindx].special & PCOMBAT) {
		realRangeAdj = (int) (realRange * 10 + 0.95);
		c3RangeAdj = (int) (c3Range * 10 + 0.95);
	} else {
		realRangeAdj = (int) (realRange + 0.95);
		c3RangeAdj = (int) (c3Range + 0.95);
	}

	if(SectionUnderwater(mech, section)) {
		if(MechWeapons[weapindx].shortrange_water <= 0) {
			*wBTH = 5000;
			return RANGE_NOWATER;
		}

		/* Out of range. No ERange in C3 */
		if(realRangeAdj > GunWaterRange(weapindx)) {
			*wBTH = 1000;
			return RANGE_TOFAR;
		}

		/* Long range... */
		if(c3RangeAdj > MechWeapons[weapindx].medrange_water) {
			*wBTH = wTargetStealth ? 6 : 4;
			return RANGE_LONG;
		}

		/* Medium range */
		if(c3RangeAdj > MechWeapons[weapindx].shortrange_water) {
			*wBTH = wTargetStealth ? 3 : 2;
			return RANGE_MED;
		}

		/* Short range */
		*wBTH = 0;
		return RANGE_SHORT;
	}

	/* Beyond range */
	if(realRangeAdj > GunRange(weapindx)) {
		*wBTH = 1000;
		return RANGE_TOFAR;
	}

	/* No V. Long range in a C3 network */
	/* Long range... */
	if(c3RangeAdj > MechWeapons[weapindx].medrange) {
		*wBTH = wTargetStealth ? 6 : 4;
		return RANGE_LONG;
	}

	/* Medium range */
	if(c3RangeAdj > MechWeapons[weapindx].shortrange) {
		*wBTH = wTargetStealth ? 3 : 2;
		return RANGE_MED;
	}

	/* Short range */
	if(realRange > MechWeapons[weapindx].min) {
		*wBTH = 0;
		return RANGE_SHORT;
	}

	/* Check for range 0.0 */
	if(c3RangeAdj == 0) {
		if(MechWeapons[weapindx].min == 0) {
			*wBTH = 0;
			return RANGE_SHORT;
		}
	}

	/* We don't care about min range if we're Hotloading */
	if(!HotLoading(weapindx, mode)) {
		if(mudconf.btech_hotloadaddshalfbthmod)
			*wBTH = ((MechWeapons[weapindx].min - realRange + 1) / 2);
		else
			*wBTH = 0;

		return RANGE_SHORT;
	}

	/* Less than or equal to minimum PHYSICAL range */
	*wBTH = MechWeapons[weapindx].min - realRange + 1;
	return RANGE_SHORT;
}

int AttackMovementMods(MECH * mech)
{
	float maxspeed;
	float speed;
	int base = 0;

	if(MechType(mech) == CLASS_BSUIT)
		return 0;

	maxspeed = MechMaxSpeed(mech);
	if((MechHeat(mech) >= 9.) && (MechSpecials(mech) & TRIPLE_MYOMER_TECH))
		maxspeed += 1.5 * MP1;
	if(Jumping(mech))
		return 3;

	/* quads don't suffer the +2 BTH firing while prone if they have all 4 legs */
	if((!MechIsQuad(mech) || (MechIsQuad(mech) &&
							  CountDestroyedLegs(mech) > 0)) && Fallen(mech)
	   && !IsDS(mech))
		return 2;

	if(!Jumping(mech) && (Stabilizing(mech) || Standing(mech)))
		return 2;

	if(fabs(MechSpeed(mech)) > fabs(MechDesiredSpeed(mech)))
		speed = MechSpeed(mech);
	else
		speed = MechDesiredSpeed(mech);

	if(mudconf.btech_fasaturn)
		if(MechFacing(mech) != MechDesiredFacing(mech))
			base++;

	if(!(fabs(speed) > 0.0))
		return base + 0;
	if(IsRunning(speed, maxspeed))
		return 2;
	return base + 1;
}

int TargetMovementMods(MECH * mech, MECH * target, float range)
{
	float target_speed = 0.0;
	int returnValue = 0;
	float m = 1.0;
	MAP *map = FindObjectsData(target->mapindex);
	MECH *swarmTarget;

	if(is_aero(target)) {
		if(is_aero(mech))
			m = ACCEL_MOD;
		target_speed =
			(float) length_hypotenuse((double) MechSpeed(target) / m,
									  (double) MechVerticalSpeed(target) / m);
	} else {
		if(Jumping(target)) {
			target_speed = JumpSpeed(target, map);
		} else if(MechSwarmTarget(target) > 0) {
			if((swarmTarget = getMech(MechSwarmTarget(target)))) {
				if(Jumping(swarmTarget))
					target_speed = JumpSpeed(swarmTarget, map);
				else
					target_speed = fabs(MechSpeed(swarmTarget));
			}
		} else {
			target_speed = fabs(MechSpeed(target));
		}
	}

	if(MechInfantrySpecials(target) & CS_PURIFIER_STEALTH_TECH) {
		if(target_speed == 0.0) {
			/* Mech moved 0-2 hexes */
			returnValue = 3;
		} else if(target_speed <= MP1) {
			/* Mech moved 3-4 hexes */
			returnValue = 2;
		} else if(target_speed <= MP2) {
			/* Mech moved 5-6 hexes */
			returnValue = 1;
		} else {
			returnValue = 0;
		}
	} else {
		if(target_speed <= MP2) {
			/* Mech moved 0-2 hexes */
			returnValue = 0;
		} else if(target_speed <= MP4) {
			/* Mech moved 3-4 hexes */
			returnValue = 1;
		} else if(target_speed <= MP6) {
			/* Mech moved 5-6 hexes */
			returnValue = 2;
		} else if(target_speed <= MP9) {
			/* Mech moved 7-9 hexes */
			returnValue = 3;
		} else {
			/* Moving more than 9 hexes */
			if(mudconf.btech_extendedmovemod)
				returnValue = 4 + (target_speed - 10 * MP1) / MP4;
			else
				returnValue = 4;
		}
	}

	if(Immobile(target))
		returnValue += -4;

	if(Fallen(target) && ((MechType(target) == CLASS_MECH) ||
						  (MechType(target) == CLASS_MW)))
		returnValue += (range <= 1.0) ? -2 : 1;

	if(Jumping(target))
		returnValue++;

	return (returnValue);
}
