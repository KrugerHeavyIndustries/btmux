/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 */

#include <math.h>
#include "config.h"
#include "externs.h"
#include "db.h"
#include "mech.h"
#include "btmacros.h"
#include "mech.events.h"
#include "mech.sensor.h"
#include "failures.h"
#include "p.econ_cmds.h"
#include "p.mech.update.h"
#include "p.bsuit.h"
#include "autopilot.h"
#include "p.mech.combat.h"
#include "p.mech.combat.misc.h"
#include "p.mech.damage.h"
#include "p.mech.utils.h"
#include "p.btechstats.h"
#include "p.mechrep.h"
#include "p.mech.restrict.h"
#include "p.eject.h"
#include "p.mech.pickup.h"
#include "p.mech.tag.h"
#include "p.mech.c3.h"
#include "p.mech.c3i.h"
#include "p.mech.enhanced.criticals.h"
#include "p.mech.ammodump.h"
#include "mt19937ar.h"

void correct_speed(MECH * mech)
{
	float maxspeed = MMaxSpeed(mech);
	int neg = 1;

	if(MechMaxSpeed(mech) < 0.0)
		MechMaxSpeed(mech) = 0.0;
	SetCargoWeight(mech);
	if(MechDesiredSpeed(mech) < -0.1) {
		maxspeed = maxspeed * 2.0 / 3.0;
		neg = -1;
	}
	if(fabs(MechDesiredSpeed(mech)) > maxspeed)
		MechDesiredSpeed(mech) = (float) maxspeed *neg;

	if(fabs(MechSpeed(mech)) > maxspeed)
		MechSpeed(mech) = (float) maxspeed *neg;
}

void explode_unit(MECH * wounded, MECH * attacker)
{
	int j;
	MECH *target;
	dbref i, tmpnext;
	dbref from;

	from = wounded->mynum;

	SAFE_DOLIST(i, tmpnext, Contents(from)) {
		if(Good_obj(i) && Hardcode(i)) {
			if((target = getMech(i))) {
				if(MechType(target) == CLASS_BSUIT) {
					KillMechContentsIfIC(target->mynum);
					discard_mw(target);
				}
			}
		}
	}

	KillMechContentsIfIC(wounded->mynum);
	for(j = 0; j < NUM_SECTIONS; j++) {
		if(GetSectOInt(wounded, j) && !SectIsDestroyed(wounded, j))
			DestroySection(wounded, attacker, wounded == attacker ? 0 : 1, j);
	}
}

void NormalizeArmActuatorCrits(MECH * objMech, int wLoc, int wCritType)
{
	switch (Special2I(wCritType)) {
	case SHOULDER_OR_HIP:
		/* +4 to BTH with weapons in arm */
		MechSections(objMech)[wLoc].basetohit = 4;
		break;

	case UPPER_ACTUATOR:
	case LOWER_ACTUATOR:
		/* +1 BTH */
		MechSections(objMech)[wLoc].basetohit += 1;
		break;
	}
}

void NormalizeLegActuatorCrits(MECH * objMech, int wLoc, int wCritType)
{
	switch (Special2I(wCritType)) {
	case SHOULDER_OR_HIP:
		/*
		   speed cut in half
		   +2 to pskill rolls
		   2nd crit == zero speed on bipeds, but not on quads. Cut current speed in half again
		 */
		DivideMaxSpeed(objMech, 2);
		MechPilotSkillBase(objMech) += 2;
		break;

	case UPPER_ACTUATOR:
	case LOWER_ACTUATOR:
	case HAND_OR_FOOT_ACTUATOR:
		/*
		   -1 walking
		   +1 to pskill rolls
		   +1 to BTHs with this leg
		 */
		LowerMaxSpeed(objMech, MP1);
		MechSections(objMech)[wLoc].basetohit += 1;
		MechPilotSkillBase(objMech) += 1;
		break;
	}
}

void NormalizeLocActuatorCrits(MECH * objMech, int wLoc)
{
	int wCritType;
	int tIsArm = 0;
	int tHasShoulderOrHipCrit = 0;
	int i;

	if(!MechIsQuad(objMech) && ((wLoc == LARM) || (wLoc == RARM)))
		tIsArm = 1;

	/* reset the BTHs for this section */
	MechSections(objMech)[wLoc].basetohit = 0;

	/* Let's first check to see if we have a shoulder or hip crit. If we do, then we ignore all the other mods */
	for(i = 0; i < NUM_CRITICALS; i++) {
		wCritType = GetPartType(objMech, wLoc, i);

		if(PartIsDestroyed(objMech, wLoc, i)) {
			if(IsSpecial(wCritType)) {
				switch (Special2I(wCritType)) {
				case SHOULDER_OR_HIP:
					tHasShoulderOrHipCrit = 1;

					if(tIsArm)
						NormalizeArmActuatorCrits(objMech, wLoc, wCritType);
					else
						NormalizeLegActuatorCrits(objMech, wLoc, wCritType);

					break;
				}
			}
		}

		if(tHasShoulderOrHipCrit)
			break;
	}

	/* Now, we only check the rest of the crits if we don't have a shoulder/hip */
	if(!tHasShoulderOrHipCrit) {

		for(i = 0; i < NUM_CRITICALS; i++) {
			wCritType = GetPartType(objMech, wLoc, i);

			if(PartIsDestroyed(objMech, wLoc, i)) {

				if(IsSpecial(wCritType)) {

					switch (Special2I(wCritType)) {
					case UPPER_ACTUATOR:
					case LOWER_ACTUATOR:
					case HAND_OR_FOOT_ACTUATOR:
						if(tIsArm)
							NormalizeArmActuatorCrits(objMech, wLoc,
													  wCritType);
						else
							NormalizeLegActuatorCrits(objMech, wLoc,
													  wCritType);

						break;
					}
				}
			}
		}
	}

	correct_speed(objMech);
}

/*
	This function will reset all pskill mods and BTH
	mods and attempt to 'correct' them as the current code
	is anything but correct.
*/

void NormalizeAllActuatorCrits(MECH * objMech)
{
	int wLegsDestroyed = CountDestroyedLegs(objMech);
	int wMaxTemplateSpeed = 0;

	/* reset us back to zero */
	MechPilotSkillBase(objMech) = 0;

	SetMaxSpeed(objMech, TemplateMaxSpeed(objMech));

	/*
	   The problem here is all the calcs are based on running speed... ie, max speed. This is lame 'cause
	   it makes EVERYTHING wrong. When you subtract 1 point of speed, if should come off the walking speed
	   and the running should be recal'd from there. Ah well, we leave it as it is now and fix it later.
	 */

	/* If we have a gyro crit, add 3 to our skill */
	/* Hardened gyro is a +2 on first hit */
	if(MechSpecials2(objMech) & HDGYRO_TECH) {
		if(MechCritStatus2(objMech) & HDGYRO_DAMAGED) {
			if(MechCritStatus(objMech) & GYRO_DAMAGED) {
				MechPilotSkillBase(objMech) += 3;
			} else {
				MechPilotSkillBase(objMech) += 2;
			}
		}

	} else if(MechCritStatus(objMech) & GYRO_DAMAGED)
		MechPilotSkillBase(objMech) += 3;

	/*
	   Let's add in the appropriate modifiers for a dead leg.
	   ie. add 5 to the pskill BTH for each dead leg
	 */
	if(wLegsDestroyed > 0) {
		if(MechIsQuad(objMech)) {
			MechPilotSkillBase(objMech) += 2;	/* loose quad bonus */

			switch (wLegsDestroyed) {
			case 1:
				LowerMaxSpeed(objMech, MP1);
				break;

			case 2:
				SetMaxSpeed(objMech, MP1);
				MechPilotSkillBase(objMech) += 5;
				break;

			case 3:
			case 4:
				SetMaxSpeed(objMech, 0.0);
				MakeMechFall(objMech);;
				break;
			}
		} else {
			if(wLegsDestroyed == 1) {
				SetMaxSpeed(objMech, MP1);
				MechPilotSkillBase(objMech) += 5;
			} else {
				SetMaxSpeed(objMech, 0.0);
				MechPilotSkillBase(objMech) += 10;
				MakeMechFall(objMech);
			}
		}
	}

/*
	For BIPED (done)
		Leg destroyed (done)
			add +5 BTH to pskills (done)
			immediate fall (done)
			only 1 MP (done)
			ignore damage in leg (done)
		No legs (done)
			+10 BTH to pskills -- like it matters (done)
			no MP (done)
			ignore damage in legs (done)

	For QUAD (done -- missing L3 mule kick)
		No legs destroyed (done -- missing L3 mule kick)
			-2 pskill BTH bonus (done)
			no pskill to stand up (done)
			no BTH mod when firing while down (done)
			does not need to prop. Fires as though it was standing. (done)
			no torso twist (done)
			no punch, club, axe, sword, push (done)
			forward kick if no hip crits (done)
			L3: can mule kick with rear levels at anyone in rear arc (pending decision)
		One leg destroyed (done)
			auto-fall (done)
			no lateral (done)
			loose -2 pskill BTH bonus (done)
			must make pskill to stand (done)
			must prop with a forward leg and adds +2 BTH when firing (done)
			-1 MP (done)
		With two legs destroyed (done)
			Act as 1 legged BIPED (done)
				immediate fall (done)
				+5 BTH to pskills (done)
				1 MP (done)
		With 3 or more legs destroyed (done)
			No movement (done)
			auto fall (done)
			MP of 0 (done)
			Can not prop to fire (done)
*/

	/* Now, normalize our legs and arms */
	if(!IsLegDestroyed(objMech, LARM))
		NormalizeLocActuatorCrits(objMech, LARM);

	if(!IsLegDestroyed(objMech, RARM))
		NormalizeLocActuatorCrits(objMech, RARM);

	if(!IsLegDestroyed(objMech, LLEG))
		NormalizeLocActuatorCrits(objMech, LLEG);

	if(!IsLegDestroyed(objMech, RLEG))
		NormalizeLocActuatorCrits(objMech, RLEG);

	/*
	   Once were done, we just gotta fix one thing.
	   If both of our hips are marked as destroyed (on a BIPED) then we set our speed to zero.
	 */
	if(MechCritStatus(objMech) & HIP_DESTROYED) {
		SetMaxSpeed(objMech, 0.0);
		MakeMechFall(objMech);
	}

	correct_speed(objMech);
}

int handleWeaponCrit(MECH * attacker, MECH * wounded, int hitloc,
					 int critHit, int critType, int LOS)
{
	int wMaxCrits, wFirstCrit, wWeapDestroyed = 0;
	int wAmmoSection, wAmmoCritSlot;
	int damage, loop;
	char locname[30];
	char msgbuf[MBUF_SIZE];

	ArmorStringFromIndex(hitloc, locname, MechType(wounded),
						 MechMove(wounded));

	/* Get the max number of crits for this weapon */
	wMaxCrits = GetWeaponCrits(wounded, Weapon2I(critType));

	/* Find the first crit */
	wFirstCrit =
		FindFirstWeaponCrit(wounded, hitloc, critHit, 0, critType, wMaxCrits);

	/* See if the weapon is already destroyed */
	if(wFirstCrit != -1) {
		wWeapDestroyed = (PartIsNonfunctional(wounded, hitloc, wFirstCrit)
						  || (PartTempNuke(wounded, hitloc,
										   wFirstCrit) == FAIL_DESTROYED));
	}

	/* Gauss rifle-ish weapons explode when critted */
	if((MechWeapons[Weapon2I(critType)].special & GAUSS) && !wWeapDestroyed) {
		mech_printf(wounded, MECHALL,
					"Your %s has been destroyed!",
					&MechWeapons[Weapon2I(critType)].name[3]);

		mech_printf(wounded, MECHALL,
					"It explodes for %d points damage.",
					MechWeapons[Weapon2I(critType)].explosiondamage);

		if(LOS && wounded != attacker && attacker)
			mech_notify(attacker, MECHALL,
						"Your target is covered in a large electrical discharge.");

		DestroyWeapon(wounded, hitloc, critType, wFirstCrit, wMaxCrits,
					  wMaxCrits);

		if(attacker) {
			DamageMech(wounded, attacker, 0, -1, hitloc, 0, 0, 0,
					   MechWeapons[Weapon2I(critType)].explosiondamage, -1, 7,
					   -1, 0, 1);
		}
/* Rule Reference: BMR Revised, Page 16-17 (Ammo Explosion=2 Bruise) */
/* Rule Reference: Total Warfare, Page 41 (Ammo Explosion=2 Bruise) */
	
		if(MechType(wounded) != CLASS_BSUIT) {
			mech_notify(wounded, MECHPILOT, "You take personal injury from the weapon's explosion!");

/* Rule Reference: MaxTech Revised, Page 46 (Reduce by 1 because of pain resistance) */

		if(HasBoolAdvantage(MechPilot(wounded), "pain_resistance"))
			headhitmwdamage(wounded, wounded, 1);
		else
			headhitmwdamage(wounded, wounded, 2);
}
		return 1;
	} else if(IsAMS(Weapon2I(critType))) {	/* Have to shut down AMS when its critted */
		mech_printf(wounded, MECHALL,
					"Your %s has been destroyed!",
					&MechWeapons[Weapon2I(critType)].name[3]);

		MechStatus(wounded) &= ~AMS_ENABLED;
		MechSpecials(wounded) &=
			~(IS_ANTI_MISSILE_TECH | CL_ANTI_MISSILE_TECH);
	} else if(HotLoading(Weapon2I(critType), GetPartFireMode(wounded, hitloc, wFirstCrit)) && !wWeapDestroyed) {	/* And crit hotloaded LRMs */
		if(FindAmmoForWeapon(wounded, Weapon2I(critType), 0,
							 &wAmmoSection, &wAmmoCritSlot) > 0) {
			damage = MechWeapons[Weapon2I(critType)].damage;

			if(IsMissile(Weapon2I(critType)) ||
			   IsArtillery(Weapon2I(critType))) {
				for(loop = 0; MissileHitTable[loop].key != -1; loop++) {
					if(MissileHitTable[loop].key == Weapon2I(critType)) {
						damage *= MissileHitTable[loop].num_missiles[10];
						break;
					}
				}
			}

			mech_printf(wounded, MECHALL,
						"Your %s has been destroyed!",
						&MechWeapons[Weapon2I(critType)].name[3]);

			mech_printf(wounded, MECHALL,
						"%%ch%%crYour hotloaded launcher explodes for %d points of damage!%%cn",
						damage);

			if(LOS && wounded != attacker && attacker)
				MechLOSBroadcast(wounded,
								 "loses a launcher in a brilliant explosion!");

			DestroyWeapon(wounded, hitloc, critType, wFirstCrit, wMaxCrits,
						  wMaxCrits);

			if(attacker) {
				DamageMech(wounded, attacker, 0, -1, hitloc, 0, 0, 0,
						   damage, -1, 7, -1, 0, 1);
			}

			return 1;
		}
	} else if((GetPartAmmoMode(wounded, hitloc,
							   wFirstCrit) & AC_INCENDIARY_MODE)
			  && !wWeapDestroyed && WpnIsRecycling(wounded, hitloc, wFirstCrit)) {	/* Incendiary ACs blow up too */

		if(FindAmmoForWeapon_sub(wounded, -1, -1, Weapon2I(critType),
								 0, &wAmmoSection, &wAmmoCritSlot, 0,
								 AC_INCENDIARY_MODE) > 0) {

			mech_printf(wounded, MECHALL,
						"Your %s has been destroyed!",
						&MechWeapons[Weapon2I(critType)].name[3]);

			mech_printf(wounded, MECHALL,
						"%%ch%%crThe incendiary ammunition in your launcher ignites for %d points of damage!%%cn",
						MechWeapons[Weapon2I(critType)].damage);

			if(LOS && wounded != attacker && attacker) {
				sprintf(msgbuf,
						"'s %s is engulfed in a brilliant blue flame!",
						locname);
				MechLOSBroadcast(wounded, msgbuf);
			}

			DestroyWeapon(wounded, hitloc, critType, wFirstCrit, wMaxCrits,
						  wMaxCrits);

			if(attacker) {
				DamageMech(wounded, attacker, 0, -1, hitloc, 0, 0, 0,
						   MechWeapons[Weapon2I(critType)].damage, -1, 7, -1,
						   0, 1);

				return 1;
			}
		}
	}

	return 0;
}

void JamMainWeapon(MECH * mech)
{
	unsigned char weaparray[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	int critical[MAX_WEAPS_SECTION];
	int count;
	int loop;
	int ii;
	int tempcrit;
	int maxcrit = 0;
	int maxloc = 0;
	int critfound = 0;
	int critnum = 0;
	unsigned char maxtype = 0;

	for(loop = 0; loop < NUM_SECTIONS; loop++) {
		if(SectIsDestroyed(mech, loop))
			continue;
		count = FindWeapons(mech, loop, weaparray, weapdata, critical);
		if(count > 0) {
			for(ii = 0; ii < count; ii++) {
				if(!PartIsBroken(mech, loop, critical[ii])) {
					/* tempcrit = GetWeaponCrits(mech, weaparray[ii]); */
					tempcrit = (int)genrand_int31();
					if(tempcrit > maxcrit) {
						critfound = 1;
						maxcrit = tempcrit;
						maxloc = loop;
						maxtype = weaparray[ii];
						critnum = critical[ii];
					}
				}
			}
		}
	}

	if(critfound) {
		SetPartTempNuke(mech, maxloc, critnum, FAIL_DESTROYED);
		mech_printf(mech, MECHALL, "%%ch%%crYour %s is jammed!%%c",
					&MechWeapons[maxtype].name[3]);
	}
}

void pickRandomWeapon(MECH * objMech, int wLoc, int *critNum, int wIgnoreJams)
{
	int awCrits[MAX_WEAPS_SECTION];
	int wcWeaps = 0;
	int wIter;

	/*
	 * Find our weapons
	 */

	for(wIter = 0; wIter < MAX_WEAPS_SECTION; wIter++) {
		if(IsWeapon(GetPartType(objMech, wLoc, wIter))) {
			if(!PartIsBroken(objMech, wLoc, wIter)) {
				if(!wIgnoreJams || (wIgnoreJams &&
									!PartTempNuke(objMech, wLoc, wIter))) {
					awCrits[wcWeaps] = wIter;

					wcWeaps++;
				}
			}
		}
	}

	if(wcWeaps <= 0) {
		*critNum = -1;
		return;
	}

	/*
	 * Now randomly pick one
	 */

	*critNum = awCrits[Number(0, wcWeaps - 1)];
}

/*
 * Make sure we're not set to go over our walking/cruise speed
 */
void limitSpeedToCruise(MECH * objMech)
{
	int wMaxSpeed = 0;

	wMaxSpeed = MMaxSpeed(objMech);

	if(MechMove(objMech) == MOVE_VTOL)
		wMaxSpeed =
			sqrt((float) wMaxSpeed * wMaxSpeed -
				 MechVerticalSpeed(objMech) * MechVerticalSpeed(objMech));

	if(WalkingSpeed(wMaxSpeed) < MechDesiredSpeed(objMech))
		MechDesiredSpeed(objMech) = WalkingSpeed(wMaxSpeed) - 0.1;
}

void DoVehicleStablizerCrit(MECH * objMech, int wLoc)
{
	/*
	 * Double attacker movement for all weapons fired from
	 * this location. If no weapons in this location, crit has no
	 * effect. Only first stablizer hit matters, subsequent ones
	 * should be ignored.
	 */

	char strLocName[30];

	ArmorStringFromIndex(wLoc, strLocName, MechType(objMech),
						 MechMove(objMech));

	if(MechSections(objMech)[wLoc].config & STABILIZERS_DESTROYED)
		mech_printf(objMech, MECHALL,
					"The destroyed weapon stabilizers in your %s take another hit!",
					strLocName);
	else {
		mech_printf(objMech, MECHALL,
					"The weapon stabilizers in your %s have been destroyed!",
					strLocName);
		MechSections(objMech)[wLoc].config |= STABILIZERS_DESTROYED;
	}
}

void DoTurretLockCrit(MECH * objMech)
{
	/*
	 * Turret locks in the current direction.
	 */

	if(MechTankCritStatus(objMech) & TURRET_LOCKED) {
		mech_notify(objMech, MECHALL,
					"The shot pierces your armor yet fails to hit a critical system!");
		return;
	}

	if(MechTankCritStatus(objMech) & TURRET_JAMMED)
		MechTankCritStatus(objMech) &= ~TURRET_JAMMED;

	MechTankCritStatus(objMech) |= TURRET_LOCKED;
	mech_notify(objMech, MECHALL,
				"%ch%crThe shot destroys your turret rotation mechanism!%cn");
}

void DoTurretJamCrit(MECH * objMech)
{
	/*
	 * Turret rotation temporarily jams. Vehicle crew must spend
	 * attack phase unjamming (read for mux: no weapons fire/ramming/etc...
	 * while unjamming turret. Second jam crit == turret locked.
	 */

	if(MechTankCritStatus(objMech) & TURRET_LOCKED) {
		mech_notify(objMech, MECHALL,
					"The shot pierces your armor yet fails to hit a critical system!");
		return;
	}

	if(MechTankCritStatus(objMech) & TURRET_JAMMED) {
		DoTurretLockCrit(objMech);
		return;
	}

	MechTankCritStatus(objMech) |= TURRET_JAMMED;
	mech_notify(objMech, MECHALL,
				"%ch%crYour turret gets jammed on its current facing!%cn");
}

void DoWeaponJamCrit(MECH * objMech, int wLoc)
{
	/*
	 * A weapon in this location is stuck. The vehicle crew must spend
	 * the attack phase unjamming this weapon.
	 *
	 * Can this really apply to a non-ammo weapon? Maybe we should just do a
	 * 'shorted/jammed' failure on the weapon?
	 *
	 * ALTERATION: Currently it's coded to 'auto-unjam' after 60 to 120 seconds.
	 */

	int wWeapIdx = 0;
	int wCritType = 0;
	int wCritNum = 0;

	if(SectIsDestroyed(objMech, wLoc))
		return;

	pickRandomWeapon(objMech, wLoc, &wCritNum, 1);

	if(wCritNum < 0) {
		mech_notify(objMech, MECHALL,
					"The shot pierces your armor yet fails to hit a critical system!");
		return;
	}

	wCritType = GetPartType(objMech, wLoc, wCritNum);
	wWeapIdx = Weapon2I(wCritType);

	if(wWeapIdx >= 0) {
		switch (MechWeapons[wWeapIdx].type) {
		case TBEAM:
		case TMISSILE:
		case TARTILLERY:
			wCritType = FAIL_SHORTED;
			mech_printf(objMech, MECHALL,
						"%%ch%%crThe shot causes your %s to temporarily short out!%%c",
						&MechWeapons[wWeapIdx].name[3]);
			break;
		case TAMMO:
			wCritType = FAIL_JAMMED;
			mech_printf(objMech, MECHALL,
						"%%ch%%crThe shot temporarily jams your %s!%%c",
						&MechWeapons[wWeapIdx].name[3]);
			break;
		default:
			wCritType = FAIL_SHORTED;
			mech_printf(objMech, MECHALL,
						"%%ch%%crThe shot causes your %s to temporarily short out!%%c",
						&MechWeapons[wWeapIdx].name[3]);
			break;
		}

		SetPartTempNuke(objMech, wLoc, wCritNum, wCritType);
		SetRecyclePart(objMech, wLoc, wCritNum, Number(60, 120));
	}
}

void DoWeaponDestroyedCrit(MECH * objAttacker, MECH * objMech, int wLoc,
						   int LOS)
{
	/*
	 * A weapon in this location is destroyed.
	 */
	int wWeapIdx = 0;
	int wCritNum = 0;
	int wCritType = 0;
	int firstCrit = 0;

	if(SectIsDestroyed(objMech, wLoc))
		return;

	pickRandomWeapon(objMech, wLoc, &wCritNum, 0);

	if(wCritNum < 0) {
		mech_notify(objMech, MECHALL,
					"The shot pierces your armor yet fails to hit a critical system!");
		return;
	}

	wCritType = GetPartType(objMech, wLoc, wCritNum);
	wWeapIdx = Weapon2I(wCritType);

	if(handleWeaponCrit(objAttacker, objMech, wLoc, wCritNum, wCritType, LOS)) {
		return;
	}

	if(wWeapIdx >= 0) {
		firstCrit = FindFirstWeaponCrit(objMech, wLoc, -1, 0, wCritType, 1);

		DestroyWeapon(objMech, wLoc, wCritType, firstCrit, 1, 1);
		mech_printf(objMech, MECHALL,
					"%%ch%%crYour %s is destroyed!%%c",
					&MechWeapons[wWeapIdx].name[3]);
	}
}

void DoTurretBlownOffCrit(MECH * objMech, MECH * objAttacker, int LOS)
{
	/*
	 * The turret is blown off, destroying everything in there
	 */

	if(SectIsDestroyed(objMech, TURRET))
		return;

	mech_notify(objMech, MECHALL,
				"%ch%crThe shot pops your turret clear off its housing!%cn");
	MechLOSBroadcast(objMech, "'s turret flies off!");
	DestroySection(objMech, objAttacker, LOS, TURRET);
}

void DoAmmunitionCrit(MECH * objMech, MECH * objAttacker, int wLoc, int LOS)
{
	/*
	 * Count total ammo carried on the tank. Apply damage directly to
	 * the internal structure of the vehicle.
	 *
	 * If the vehicle has CASE, apply the damage to the rear ARMOR and also
	 * cause a Driver Hit, Commander Hit and Crew Stunned crit.
	 *
	 * if the vehicle has no ammunition, treat this as a weapon destroyed crit.
	 */

	int wTotalAmmoDamage = 0;
	int wTempDamage = 0;
	int wSecIter, wSlotIter;
	int wPartType = 0;
	int wWeapIdx;

	for(wSecIter = 0; wSecIter <= 7; wSecIter++) {
		if(SectIsDestroyed(objMech, wSecIter))
			continue;

		for(wSlotIter = CritsInLoc(objMech, wSecIter) - 1; wSlotIter >= 0;
			wSlotIter--) {
			wPartType = GetPartType(objMech, wSecIter, wSlotIter);
			wWeapIdx = Ammo2WeaponI(wPartType);

			if(IsAmmo(wPartType) &&
			   GetPartData(objMech, wSecIter, wSlotIter) &&
			   (!(MechWeapons[wWeapIdx].special & GAUSS))) {
				wTempDamage =
					(FindMaxAmmoDamage(Ammo2WeaponI(wPartType)) *
					 GetPartData(objMech, wSecIter, wSlotIter));
				wTotalAmmoDamage += wTempDamage;

				SetPartData(objMech, wSecIter, wSlotIter, 0);
			}
		}
	}

	if(wTotalAmmoDamage == 0) {
		DoWeaponDestroyedCrit(objAttacker, objMech, wLoc, LOS);
		return;
	}

	mech_notify(objMech, MECHALL,
				"%ch%crOne of your ammo bins is struck causing a cascading explosion!%cn");
	MechLOSBroadcast(objMech, "has an internal ammo explosion!");

	DamageMech(objMech, objAttacker, 0, -1, wLoc, 0, 0, 0,
			   wTotalAmmoDamage, 0, 0, -1, 0, 1);
}

void DoCargoInfantryCrit(MECH * objMech, int wLoc)
{
	/*
	 * If there's infantry in the unit, the infantry takes
	 * damage as if the weapon that caused the crit hit them.
	 *
	 * If there's cargo, damage is to be determined.
	 */

	mech_notify(objMech, MECHALL,
				"The shot pierces your armor yet fails to hit a critical system!");
}

void DoVehicleEngineHit(MECH * objMech, MECH * objAttacker)
{
	/*
	 * Vehicle engine is severly damaged.
	 * The vehicle may not move or change facing.
	 *
	 * If the vehicle is a VTOL, it may land successfully.
	 * If the VTOL is over a clear, paved, rough or building hex is can
	 * make a pskill roll to avoid crashing. If the roll fails or it's
	 * above any other terrain, the VTOL crashes.
	 */

	if(Fallen(objMech)) {
		mech_notify(objMech, MECHALL,
					"Your destroyed engine takes another direct hit!");
		return;
	}

	mech_notify(objMech, MECHALL, "%ch%crYour engine takes a direct hit!%cn");

	if(MechType(objMech) == CLASS_VTOL) {
		if(!Landed(objMech)) {
			if(MechRTerrain(objMech) == GRASSLAND ||
			   MechRTerrain(objMech) == ROAD ||
			   MechRTerrain(objMech) == BUILDING) {

				if(MadePilotSkillRoll(objMech,
									  MechZ(objMech) -
									  MechElevation(objMech))) {
					mech_notify(objMech, MECHALL, "You land safely!");
					MechStatus(objMech) |= LANDED;
					MechZ(objMech) = MechElevation(objMech);
					MechFZ(objMech) = ZSCALE * MechZ(objMech);
					SetMaxSpeed(objMech, 0.0);
					MechVerticalSpeed(objMech) = 0.0;
				}
			} else {
				mech_notify(objMech, MECHALL, "The ground rushes up to meet you!");
				mech_notify(objAttacker, MECHALL,
							"You knock the VTOL out of the sky!");
				MechLOSBroadcast(objMech, "falls from the sky!");
				MechFalls(objMech, MechsElevation(objMech), 0);
			}
		}
	} else {
		SetMaxSpeed(objMech, 0.0);
		MakeMechFall(objMech);
	}
}

void DoVehicleFuelTankCrit(MECH * objMech, MECH * objAttacker)
{
	/*
	 * The fuel tank is breached causing the
	 * the vehicle to explode. If the unit does not
	 * have an ICE engine, treat this as an engine crit.
	 */

	if(!(MechSpecials(objMech) & ICE_TECH)) {
		DoVehicleEngineHit(objMech, objAttacker);
		return;
	}

	mech_notify(objMech, MECHALL,
				"%ch%crYour fuel tank explodes in a ball of fire!%cn");

	if(objMech != objAttacker)
		MechLOSBroadcast(objMech, "explodes in a ball of fire!");

	MechZ(objMech) = MechElevation(objMech);
	MechFZ(objMech) = ZSCALE * MechZ(objMech);
	MechSpeed(objMech) = 0.0;
	MechVerticalSpeed(objMech) = 0.0;
	DestroyMech(objMech, objAttacker, 0, KILL_TYPE_NORMAL);
	explode_unit(objMech, objAttacker);
}

void DoVehicleCrewStunnedCrit(MECH * objMech)
{
	/*
	 * For one turn the crew can not move over cruising
	 * speed and not fire weapons/ram/use radio/etc, just turn.
	 */
	MechTankCritStatus(objMech) |= CREW_STUNNED;
	mech_notify(objMech, MECHALL,
				"%ch%crThe shot resonates throughout the crew compartment, temporarily stunning you!%cn");

	StunCrew(objMech);
	limitSpeedToCruise(objMech);
}

void DoVehicleDriverCrit(MECH * objMech)
{
	/*
	 * Driver is hit, apply a +2 to all driving skills
	 */

	mech_notify(objMech, MECHALL,
				"%ch%crYour vehicle's driver takes a piece of shrapnel, making it harder to control the vehicle!%cn");
	MechPilotSkillBase(objMech) += 2;
}

void DoVehicleSensorCrit(MECH * objMech)
{
	/*
	 * Add +1 to BTH for each sensor crit.
	 */

	mech_notify(objMech, MECHALL, "%ch%crYour sensor suite takes a hit!%cn");
	MechBTH(objMech) += 1;
}

void DoVehicleCommanderHit(MECH * objMech)
{
	/*
	 * Commander is hit. Vehicle suffers a +1 to BTH and driving
	 * skills. Also does result of crew stunned.
	 */

	mech_notify(objMech, MECHALL,
				"%ch%crYour vehicle's commander takes a piece of shrapnel!%cn");
	MechPilotSkillBase(objMech) += 1;
	MechBTH(objMech) += 1;

	DoVehicleCrewStunnedCrit(objMech);
}

void DoVehicleCrewKilledCrit(MECH * objMech, MECH * objAttacker)
{
	/*
	 * The whole crew is instantly killed
	 * leaving the tank 'destroyed' but fixable.
	 */

	mech_notify(objMech, MECHALL,
				"%ch%crThe shot ricochets around the crew compartment, instantly killing everyone!%cn");
	DestroyMech(objMech, objAttacker, 0, KILL_TYPE_PILOT);
	KillMechContentsIfIC(objMech->mynum);

	if(MechSpeed(objMech) != 0.0)
		MechLOSBroadcast(objMech,
						 "careens out of control and starts to slow!");

	MakeMechFall(objMech);
}

void DoVTOLCoPilotCrit(MECH * objMech)
{
	/*
	 * +1 BTH for weapons fire
	 */

	mech_notify(objMech, MECHALL,
				"%ch%crYour VTOL's pilot takes a piece of shrapnel, making it harder to aim your weapons!%cn");
	MechBTH(objMech) += 1;
}

void DoVTOLPilotHit(MECH * objMech)
{
	/*
	 * +2 pskill rolls
	 * Must make sucessful pskill roll
	 * or fall 1 elevation.
	 */

	mech_notify(objMech, MECHALL,
				"%ch%crYour VTOL's copilot takes a piece of shrapnel, making it harder to control the VTOL!%cn");
	MechPilotSkillBase(objMech) += 2;

	/* TODO: make vtol drop a level if it fails a pskill roll */
}

void DoVTOLRotorDestroyedCrit(MECH * objMech, MECH * objAttacker, int LOS)
{
	/*
	 * Rotors are destroyed sending the vehicle crashing to the ground,
	 * if it's not already there.
	 */

	if(SectIsDestroyed(objMech, ROTOR))
		return;

	mech_notify(objMech, MECHALL,
				"%ch%crThe shot hits your fragile rotor mechanism!%cn");
	MechLOSBroadcast(objMech, "'s rotor snaps into several parts!");
	DestroySection(objMech, objAttacker, LOS, ROTOR);

	if(!objAttacker) {
		mech_notify(objMech, MECHALL, "Your rotor has been destroyed!");
		MechLOSBroadcast(objMech, "'s rotor has been destroyed!");
	}
}

void DoVTOLRotorDamagedCrit(MECH * objMech)
{
	/*
	 * -1MP
	 */

	/*
	 * KipstaFeature: if we have less than 1 MP left then we don't have
	 * enough rotor speed to keep us aloft, logically... so let's blow the sucker off
	 * and crash the VTOL.
	 */
	if(MMaxSpeed(objMech) <= MP1) {
		DoVTOLRotorDestroyedCrit(objMech, NULL, 1);
		return;
	}

	mech_notify(objMech, MECHALL, "Your rotor is damaged!");

	if(!Fallen(objMech))
		LowerMaxSpeed(objMech, MP1);

}

void DoVTOLTailRotorDamagedCrit(MECH * objMech)
{
	/*
	 * May not move faster than cruising speed.
	 * Turns slower.
	 */

	if(MechTankCritStatus(objMech) & TAIL_ROTOR_DESTROYED)
		mech_notify(objMech, MECHALL,
					"Your damaged tail rotor suffers more damage!");
	else {
		MechTankCritStatus(objMech) |= TAIL_ROTOR_DESTROYED;
		mech_notify(objMech, MECHALL,
					"%ch%crYour tail rotor is damaged, slowing you down!%cn");

		limitSpeedToCruise(objMech);
	}
}

void StartVTOLCrash(MECH * objMech)
{
	if(!Fallen(objMech)) {
		MechSpeed(objMech) = 0.0;
		MechDesiredSpeed(objMech) = 0.0;
		SetMaxSpeed(objMech, 0.0);

		if(!Landed(objMech)) {
			mech_notify(objMech, MECHALL,
						"You ponder F = ma, S = F/m, S = at^2 => S=agt^2 in relation to the ground.");
			MechVerticalSpeed(objMech) = 0;
			mech_notify(objMech, MECHALL,
						"You start free-fall.. Enjoy the ride!");
			MechLOSBroadcast(objMech, "starts to fall to the ground!");
			MECHEVENT(objMech, EVENT_FALL, mech_fall_event, FALL_TICK, -1);

			/*
			   MechVerticalSpeed(objMech) = 0;
			   mech_notify(objMech, MECHALL, "You fall rapidly from the sky!");
			   MechLOSBroadcast(objMech, "plummets from the sky!");
			   MechFalls(objMech, MechsElevation(objMech), 0);
			 */
		}
	}
}

void HandleAdvFasaVehicleCrit(MECH * wounded, MECH * attacker, int LOS,
							  int hitloc, int num)
{
	int wRoll = Roll();

	if(MechMove(wounded) == MOVE_NONE)
		return;

	if(wRoll < 6)
		return;

	mech_notify(wounded, MECHALL, "%ch%cyCRITICAL HIT!%c");

	switch (MechType(wounded)) {
	case CLASS_VEH_GROUND:
		switch (hitloc) {
		case FSIDE:
			switch (wRoll) {
			case 6:
				DoVehicleDriverCrit(wounded);
				break;

			case 7:
				DoWeaponJamCrit(wounded, hitloc);
				break;

			case 8:
				DoVehicleStablizerCrit(wounded, hitloc);
				break;

			case 9:
				DoVehicleSensorCrit(wounded);
				break;

			case 10:
				DoVehicleCommanderHit(wounded);
				break;

			case 11:
				DoWeaponDestroyedCrit(attacker, wounded, hitloc, LOS);
				break;

			case 12:
				DoVehicleCrewKilledCrit(wounded, attacker);
				break;
			}
			break;

		case LSIDE:
		case RSIDE:
			switch (wRoll) {
			case 6:
				DoCargoInfantryCrit(wounded, hitloc);
				break;

			case 7:
				DoWeaponJamCrit(wounded, hitloc);
				break;

			case 8:
				DoVehicleCrewStunnedCrit(wounded);
				break;

			case 9:
				DoVehicleStablizerCrit(wounded, hitloc);
				break;

			case 10:
				DoWeaponDestroyedCrit(attacker, wounded, hitloc, LOS);
				break;

			case 11:
				DoVehicleEngineHit(wounded, attacker);
				break;

			case 12:
				DoVehicleFuelTankCrit(wounded, attacker);
				break;
			}
			break;

		case BSIDE:
			switch (wRoll) {
			case 6:
				DoWeaponJamCrit(wounded, hitloc);
				break;

			case 7:
				DoCargoInfantryCrit(wounded, hitloc);
				break;

			case 8:
				DoVehicleStablizerCrit(wounded, hitloc);
				break;

			case 9:
				DoWeaponDestroyedCrit(attacker, wounded, hitloc, LOS);
				break;

			case 10:
				DoVehicleEngineHit(wounded, attacker);
				break;

			case 11:
				DoAmmunitionCrit(wounded, attacker, hitloc, LOS);
				break;

			case 12:
				DoVehicleFuelTankCrit(wounded, attacker);
				break;
			}
			break;

		case TURRET:
			switch (wRoll) {
			case 6:
				DoVehicleStablizerCrit(wounded, hitloc);
				break;

			case 7:
				DoTurretJamCrit(wounded);
				break;

			case 8:
				DoWeaponJamCrit(wounded, hitloc);
				break;

			case 9:
				DoTurretLockCrit(wounded);
				break;

			case 10:
				DoWeaponDestroyedCrit(attacker, wounded, hitloc, LOS);
				break;

			case 11:
				DoTurretBlownOffCrit(wounded, attacker, LOS);
				break;

			case 12:
				DoAmmunitionCrit(wounded, attacker, hitloc, LOS);
				break;
			}
			break;
		}
		break;

	case CLASS_VTOL:
		switch (hitloc) {
		case FSIDE:
			switch (wRoll) {
			case 6:
				DoVTOLCoPilotCrit(wounded);
				break;

			case 7:
				DoWeaponJamCrit(wounded, hitloc);
				break;

			case 8:
				DoVehicleStablizerCrit(wounded, hitloc);
				break;

			case 9:
				DoVehicleSensorCrit(wounded);
				break;

			case 10:
				DoVTOLPilotHit(wounded);
				break;

			case 11:
				DoWeaponDestroyedCrit(attacker, wounded, hitloc, LOS);
				break;

			case 12:
				DoVehicleCrewKilledCrit(wounded, attacker);
				break;
			}
			break;

		case LSIDE:
		case RSIDE:
			switch (wRoll) {
			case 6:
				DoWeaponJamCrit(wounded, hitloc);
				break;

			case 7:
				DoCargoInfantryCrit(wounded, hitloc);
				break;

			case 8:
				DoVehicleStablizerCrit(wounded, hitloc);
				break;

			case 9:
				DoWeaponDestroyedCrit(attacker, wounded, hitloc, LOS);
				break;

			case 10:
				DoVehicleEngineHit(wounded, attacker);
				break;

			case 11:
				DoAmmunitionCrit(wounded, attacker, hitloc, LOS);
				break;

			case 12:
				DoVehicleFuelTankCrit(wounded, attacker);
				break;
			}
			break;

		case BSIDE:
			switch (wRoll) {
			case 6:
				DoCargoInfantryCrit(wounded, hitloc);
				break;

			case 7:
				DoWeaponJamCrit(wounded, hitloc);
				break;

			case 8:
				DoVehicleStablizerCrit(wounded, hitloc);
				break;

			case 9:
				DoWeaponDestroyedCrit(attacker, wounded, hitloc, LOS);
				break;

			case 10:
				DoVehicleSensorCrit(wounded);
				break;

			case 11:
				DoVehicleEngineHit(wounded, attacker);
				break;

			case 12:
				DoVehicleFuelTankCrit(wounded, attacker);
				break;
			}
			break;

		case ROTOR:
			switch (wRoll) {
			case 6:
			case 7:
			case 8:
				DoVTOLRotorDamagedCrit(wounded);
				break;

			case 9:
			case 10:
				DoVTOLTailRotorDamagedCrit(wounded);
				break;

			case 11:
			case 12:
				DoVTOLRotorDestroyedCrit(wounded, attacker, LOS);
				break;
			}
			break;
		}
		break;
	}

}

void HandleVTOLCrit(MECH * wounded, MECH * attacker, int LOS, int hitloc,
					int num)
{
	mech_notify(wounded, MECHALL, "%ch%cyCRITICAL HIT!%c");
	switch (Number(0,5)) {
	case 0:
		/* Crew killed */
		mech_notify(wounded, MECHALL, "Your cockpit is destroyed!");
		if(!Landed(wounded)) {
			mech_notify(attacker, MECHALL,
						"You knock the VTOL out of the sky!");
			MechLOSBroadcast(wounded, "falls down from the sky!");
			MechFalls(wounded, MechsElevation(wounded), 0);
		}
		DestroyMech(wounded, attacker, 1, KILL_TYPE_PILOT);
		KillMechContentsIfIC(wounded->mynum);
		break;
	case 1:
		/* Weapon jams, set them recylcling maybe */
		/* hmm. nothing for now, tanks are so weak */
		JamMainWeapon(wounded);
		break;
	case 2:
		/* Engine Hit */
		mech_notify(wounded, MECHALL, "Your engine takes a direct hit!");
		if(!Landed(wounded)) {
			if(MechRTerrain(wounded) == GRASSLAND ||
			   MechRTerrain(wounded) == ROAD ||
			   MechRTerrain(wounded) == BUILDING) {
				if(MadePilotSkillRoll(wounded,
									  MechZ(wounded) -
									  MechElevation(wounded))) {
					mech_notify(wounded, MECHALL, "You land safely!");
					MechStatus(wounded) |= LANDED;
					MechZ(wounded) = MechElevation(wounded);
					MechFZ(wounded) = ZSCALE * MechZ(wounded);
					SetMaxSpeed(wounded, 0.0);
					MechVerticalSpeed(wounded) = 0.0;
				}
			} else {
				mech_notify(wounded, MECHALL, "The ground rushes up to meet you!");
				mech_notify(attacker, MECHALL,
							"You knock the VTOL out of the sky!");
				MechLOSBroadcast(wounded, "falls from the sky!");
				MechFalls(wounded, MechsElevation(wounded), 0);
			}
		}
		SetMaxSpeed(wounded, 0.0);
		break;
	case 3:
		/* Crew Killed */
		mech_notify(wounded, MECHALL, "Your cockpit is destroyed!");
		if(!(MechStatus(wounded) & LANDED)) {
			mech_notify(attacker, MECHALL,
						"You knock the VTOL out of the sky!");
			MechLOSBroadcast(wounded, "falls from the sky!");
			MechFalls(wounded, MechsElevation(wounded), 0);
		}

		DestroyMech(wounded, attacker, 1, KILL_TYPE_PILOT);

		KillMechContentsIfIC(wounded->mynum);
		break;
	case 4:
		/* Fuel Tank Explodes */
		mech_notify(wounded, MECHALL,
					"Your fuel tank explodes in a ball of fire!");
		if(wounded != attacker)
			MechLOSBroadcast(wounded, "explodes in a ball of fire!");
		MechZ(wounded) = MechElevation(wounded);
		MechFZ(wounded) = ZSCALE * MechZ(wounded);
		MechSpeed(wounded) = 0.0;
		MechVerticalSpeed(wounded) = 0.0;
		DestroyMech(wounded, attacker, 0, KILL_TYPE_NORMAL);
		explode_unit(wounded, attacker);
		break;
	case 5:
		/* Ammo/Power Plant Explodes */
		mech_notify(wounded, MECHALL, "Your power plant explodes!");
		MechLOSBroadcast(wounded, "suddenly explodes!");
		MechZ(wounded) = MechElevation(wounded);
		MechFZ(wounded) = ZSCALE * MechZ(wounded);
		MechSpeed(wounded) = 0.0;
		MechVerticalSpeed(wounded) = 0.0;
		DestroyMech(wounded, attacker, 0, KILL_TYPE_NORMAL);
		if(!(MechSections(wounded)[BSIDE].config & CASE_TECH))
			explode_unit(wounded, attacker);
		else
			DestroySection(wounded, attacker, LOS, BSIDE);
	}
}

void DestroyMainWeapon(MECH * mech)
{
	unsigned char weaparray[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	int critical[MAX_WEAPS_SECTION];
	int count;
	int loop;
	int ii;
	int tempcrit;
	int maxcrit = 0;
	int maxloc = 0;
	int critfound = 0;
	unsigned char maxtype = 0;
	int firstCrit = 0;

	for(loop = 0; loop < NUM_SECTIONS; loop++) {
		if(SectIsDestroyed(mech, loop))
			continue;
		count = FindWeapons(mech, loop, weaparray, weapdata, critical);
		if(count > 0) {
			for(ii = 0; ii < count; ii++) {
				if(!PartIsBroken(mech, loop, critical[ii])) {
					/* tempcrit = GetWeaponCrits(mech, weaparray[ii]); */
					tempcrit = (int)genrand_int31();
					if(tempcrit > maxcrit) {
						critfound = 1;
						maxcrit = tempcrit;
						maxloc = loop;
						maxtype = weaparray[ii];
					}
				}
			}
		}
	}
	if(critfound) {
		firstCrit = FindFirstWeaponCrit(mech, maxloc, -1, 0,
										I2Weapon(maxtype), 1);
		DestroyWeapon(mech, maxloc, I2Weapon(maxtype), 1, firstCrit,
					  GetWeaponCrits(mech, maxtype));
		mech_printf(mech, MECHALL,
					"%%ch%%crYour %s is destroyed!%%c",
					&MechWeapons[maxtype].name[3]);
	}
}

void HandleFasaVehicleCrit(MECH * wounded, MECH * attacker, int LOS,
						   int hitloc, int num)
{
	if(MechMove(wounded) == MOVE_NONE)
		return;

	mech_notify(wounded, MECHALL, "%ch%cyCRITICAL HIT!%c");
	switch (Number(0,5)) {
	case 0:
		/* Crew stunned for one turn...treat like a head hit */
		headhitmwdamage(wounded, attacker, 1);
		break;
	case 1:
		/* Weapon jams, set them recylcling maybe */
		/* hmm. nothing for now, tanks are so weak */
		JamMainWeapon(wounded);
		break;
	case 2:
		/* Engine Hit */
		mech_notify(wounded, MECHALL,
					"Your engine takes a direct hit!  You can't move anymore.");
		SetMaxSpeed(wounded, 0.0);
		break;
	case 3:
		/* Crew Killed */
		mech_notify(wounded, MECHALL,
					"Your armor is pierced and you are killed instantly!");
		DestroyMech(wounded, attacker, 0, KILL_TYPE_PILOT);
		KillMechContentsIfIC(wounded->mynum);
		break;
	case 4:
		/* Fuel Tank Explodes */
		mech_notify(wounded, MECHALL,
					"Your fuel tank explodes in a ball of fire!");
		if(wounded != attacker)
			MechLOSBroadcast(wounded, "explodes in a ball of fire!");
		DestroyMech(wounded, attacker, 0, KILL_TYPE_NORMAL);
		explode_unit(wounded, attacker);
		break;
	case 5:
		/* Ammo/Power Plant Explodes */
		mech_notify(wounded, MECHALL, "Your power plant explodes!");
		if(wounded != attacker)
			MechLOSBroadcast(wounded, "suddenly explodes!");
		DestroyMech(wounded, attacker, 0, KILL_TYPE_NORMAL);
		if(!(MechSections(wounded)[BSIDE].config & CASE_TECH))
			explode_unit(wounded, attacker);
		else
			DestroySection(wounded, attacker, LOS, BSIDE);
		break;
	}
}

void HandleVehicleCrit(MECH * wounded, MECH * attacker, int LOS,
					   int hitloc, int num)
{
	if(MechMove(wounded) == MOVE_NONE)
		return;
	if(hitloc == TURRET) {
		if(Number(1, 3) == 2) {
			if(!(MechTankCritStatus(wounded) & TURRET_LOCKED)) {
				mech_notify(wounded, MECHALL, "%ch%cyCRITICAL HIT!%c");
				MechTankCritStatus(wounded) |= TURRET_LOCKED;
				mech_notify(wounded, MECHALL,
							"Your turret takes a direct hit and locks up!");
			}
			return;
		}
	} else
		switch (Number(1, 10)) {
		case 1:
		case 2:
		case 3:
		case 4:
			if(!Fallen(wounded)) {
				mech_notify(wounded, MECHALL, "%ch%cyCRITICAL HIT!%c");
				switch (MechMove(wounded)) {
				case MOVE_TRACK:
					mech_notify(wounded, MECHALL,
								"One of your tracks is damaged!");
					break;
				case MOVE_WHEEL:
					mech_notify(wounded, MECHALL,
								"One of your wheels is damaged!");
					break;
				case MOVE_HOVER:
					mech_notify(wounded, MECHALL,
								"Your air skirt is damaged!");
					break;
				case MOVE_HULL:
				case MOVE_SUB:
				case MOVE_FOIL:
					mech_notify(wounded, MECHALL, "Your craft suddenly slows!");
					break;
				}
				LowerMaxSpeed(wounded, MP1);
			}
			return;
			break;
		case 5:
			if(!Fallen(wounded)) {
				mech_notify(wounded, MECHALL, "%ch%cyCRITICAL HIT!%c");
				switch (MechMove(wounded)) {
				case MOVE_TRACK:
					mech_notify(wounded, MECHALL,
								"One of your tracks is destroyed, immobilizing your vehicle!");
					break;
				case MOVE_WHEEL:
					mech_notify(wounded, MECHALL,
								"One of your wheels is destroyed, immobilizing your vehicle!");
					break;
				case MOVE_HOVER:
					mech_notify(wounded, MECHALL,
								"Your lift fan is destroyed, immobilizing your vehicle!");
					break;
				case MOVE_HULL:
				case MOVE_SUB:
				case MOVE_FOIL:
					mech_notify(wounded, MECHALL,
								"Your engines cut out and you drift to a halt!");
				}
				SetMaxSpeed(wounded, 0.0);

				MakeMechFall(wounded);
			}
			return;
			break;
		}
	mech_notify(wounded, MECHALL, "%ch%cyCRITICAL HIT!%c");
	switch (Number(0,5)) {
	case 0:
		/* Crew stunned for one turn...treat like a head hit */
		headhitmwdamage(wounded, attacker, 1);
		break;
	case 1:
		/* Weapon jams, set them recylcling maybe */
		/* hmm. nothing for now, tanks are so weak */
		JamMainWeapon(wounded);
		break;
	case 2:
		/* Engine Hit */
		mech_notify(wounded, MECHALL,
					"Your engine takes a direct hit!  You can't move anymore.");
		SetMaxSpeed(wounded, 0.0);
		break;
	case 3:
		/* Crew Killed */
		mech_notify(wounded, MECHALL,
					"Your armor is pierced and you are killed instantly!");
		DestroyMech(wounded, attacker, 0, KILL_TYPE_PILOT);
		KillMechContentsIfIC(wounded->mynum);
		break;
	case 4:
		/* Fuel Tank Explodes */
		mech_notify(wounded, MECHALL,
					"Your fuel tank explodes in a ball of fire!");
		if(wounded != attacker)
			MechLOSBroadcast(wounded, "explodes in a ball of fire!");
		DestroyMech(wounded, attacker, 0, KILL_TYPE_NORMAL);
		explode_unit(wounded, attacker);
		break;
	case 5:
		/* Ammo/Power Plant Explodes */
		mech_notify(wounded, MECHALL, "Your power plant explodes!");
		if(wounded != attacker)
			MechLOSBroadcast(wounded, "suddenly explodes!");
		DestroyMech(wounded, attacker, 0, KILL_TYPE_NORMAL);
		explode_unit(wounded, attacker);
		break;
	}
}

int HandleMechCrit(MECH * wounded, MECH * attacker, int LOS, int hitloc,
				   int critHit, int critType, int critData)
{
	MECH *mech = wounded;
	int weapindx, damage, loop, destroycrit, weapon_slot;
	int temp;
	char locname[30];
	char msgbuf[MBUF_SIZE];
	int tLocIsArm = ((hitloc == LARM || hitloc == RARM) &&
					 !MechIsQuad(wounded));
	int tLocIsLeg = ((hitloc == LLEG || hitloc == RLEG) || ((hitloc == LARM
															 || hitloc ==
															 RARM)
															&&
															MechIsQuad
															(wounded)));
	char partBuf[100];

	MAP *map = FindObjectsData(wounded->mapindex);

	ArmorStringFromIndex(hitloc, locname, MechType(wounded),
						 MechMove(wounded));
	mech_notify(wounded, MECHALL, "%ch%cyCRITICAL HIT!!%c");

	if(IsAmmo(critType)) {
		/* BOOM! */
		/* That's going to hurt... */
		weapindx = Ammo2WeaponI(critType);
		damage = critData * MechWeapons[weapindx].damage;
		if(IsMissile(weapindx) || IsArtillery(weapindx)) {
			for(loop = 0; MissileHitTable[loop].key != -1; loop++)
				if(MissileHitTable[loop].key == weapindx)
					damage *= MissileHitTable[loop].num_missiles[10];
		}
		if(MechWeapons[weapindx].special & (GAUSS | NOBOOM)) {
			if(MechWeapons[weapindx].special & GAUSS)
				mech_notify(wounded, MECHALL,
							"One of your Gauss Rifle ammo feeds is destroyed");
			DestroyPart(wounded, hitloc, critHit);
		} else if(damage) {
			ammo_explosion(attacker, wounded, hitloc, critHit, damage);
		} else {
			mech_notify(wounded, MECHALL,
						"You have no ammunition left in that location, lucky you!");
			DestroyPart(wounded, hitloc, critHit);
		}
		return 1;
	}

	if(PartIsBroken(wounded, hitloc, critHit) && IsWeapon(critType) &&
	   !PartIsDisabled(wounded, hitloc, critHit)) {
		while (--critHit && GetPartType(wounded, hitloc, critHit) == critType)
			if(PartIsDestroyed(wounded, hitloc, critHit))
				break;
		mech_printf(wounded, MECHALL,
					"Your destroyed %s is damaged some more!",
					&MechWeapons[Weapon2I(critType)].name[3]);
		DestroyPart(wounded, hitloc, critHit + 1);
		return 1;
	}

	if(PartIsNonfunctional(wounded, hitloc, critHit)) {
		if(IsSpecial(critType)) {
			switch (Special2I(critType)) {
			case LIFE_SUPPORT:
				strcpy(partBuf, "life support");
				break;
			case COCKPIT:
				strcpy(partBuf, "cockpit");
				break;
			case SENSORS:
				strcpy(partBuf, "sensors");
				break;
			case HEAT_SINK:
				strcpy(partBuf, "heatsink");
				break;
			case JUMP_JET:
				strcpy(partBuf, "jump jet");
				break;
			case ENGINE:
				strcpy(partBuf, "engine");
				break;
			case TARGETING_COMPUTER:
				strcpy(partBuf, "targeting computer");
				break;
			case GYRO:
				strcpy(partBuf, "gyro");
				break;
			case SHOULDER_OR_HIP:
				if(tLocIsArm)
					strcpy(partBuf, "shoulder");
				else
					strcpy(partBuf, "hip");
				break;
			case LOWER_ACTUATOR:
			case UPPER_ACTUATOR:
			case HAND_OR_FOOT_ACTUATOR:
				if(tLocIsArm) {
					if(Special2I(critType) == HAND_OR_FOOT_ACTUATOR)
						strcpy(partBuf, "hand actuator");
					else
						strcpy(partBuf, "arm actuator");
				} else {
					if(Special2I(critType) == HAND_OR_FOOT_ACTUATOR)
						strcpy(partBuf, "foot actuator");
					else
						strcpy(partBuf, "arm actuator");
				}
				break;
			case C3_MASTER:
				strcpy(partBuf, "C3 system");
				break;
			case C3_SLAVE:
				strcpy(partBuf, "C3 system");
				break;
			case C3I:
				strcpy(partBuf, "C3i system");
				break;
			case TAG:
				strcpy(partBuf, "TAG system");
				break;
			case ECM:
				strcpy(partBuf, "ECM system");
				break;
			case ANGELECM:
				strcpy(partBuf, "Angel ECM system");
				break;
			case BEAGLE_PROBE:
				strcpy(partBuf, "Beagle Active Probe");
				break;
			case BLOODHOUND_PROBE:
				strcpy(partBuf, "Bloodhound Active Probe");
				break;
			case ARTEMIS_IV:
				strcpy(partBuf, "ArtemisIV system");
				break;
			case AXE:
				strcpy(partBuf, "axe");
				break;
			case SWORD:
				strcpy(partBuf, "sword");
				break;
			case MACE:
				strcpy(partBuf, "mace");
				break;
			case DUAL_SAW:
				strcpy(partBuf, "dual saw");
				break;
			case DS_AERODOOR:
				strcpy(partBuf, "aero doors");
				break;
			case DS_MECHDOOR:
				strcpy(partBuf, "mech doors");
				break;
			case NULL_SIGNATURE_SYSTEM:
				strcpy(partBuf, "Null Signature System");
				break;
			}					// end switch() - Part Names
		}						// end if()

		mech_printf(wounded, MECHALL,
					"Part of your non-working %s has been hit!", partBuf);
		DestroyPart(wounded, hitloc, critHit);
		return 1;
	}

	if(IsWeapon(critType)) {
		if(handleWeaponCrit(attacker, wounded, hitloc, critHit, critType,
							LOS)) {
			return 1;
		}

		scoreEnhancedWeaponCriticalHit(mech, attacker, LOS, hitloc, critHit);

		/* Have to destroy all the weapons of this type in this section */
		/* DestroyWeapon(wounded, hitloc, critType, 1, GetWeaponCrits(wounded,  Weapon2I(critType))); */

		return 1;
	}

	if(IsSpecial(critType)) {
		destroycrit = 1;
		switch (Special2I(critType)) {
		case LIFE_SUPPORT:
			MechCritStatus(wounded) |= LIFE_SUPPORT_DESTROYED;
			mech_notify(wounded, MECHALL,
						"Your life support has been destroyed!");
			break;
		case COCKPIT:
			/* Destroy Mech for now, but later kill pilot as well */
			mech_notify(wounded, MECHALL,
						"Your cockpit is destroyed, your blood boils, and your body is fried! %cyYou're dead!%cn");
			if(!Destroyed(wounded)) {
				DestroyMech(wounded, attacker, 0, KILL_TYPE_PILOT);		
			}

			if(LOS && attacker)
				mech_notify(attacker, MECHALL,
							"You destroy the cockpit! The pilot's blood splatters down the sides!");
			MechLOSBroadcast(wounded,
							 "spasms for a second then remains oddly still.");
			MechPilot(wounded) = -1;
			KillMechContentsIfIC(wounded->mynum);
			break;
		case SENSORS:
			if(!(MechCritStatus(wounded) & SENSORS_DAMAGED)) {
				MechLRSRange(wounded) /= 2;
				MechTacRange(wounded) /= 2;
				MechScanRange(wounded) /= 2;
				MechBTH(wounded) += 2;
				MechCritStatus(wounded) |= SENSORS_DAMAGED;
				mech_notify(wounded, MECHALL,
							"Your sensors have been damaged!");
			} else {
				MechLRSRange(wounded) = 0;
				MechTacRange(wounded) = 0;
				MechScanRange(wounded) = 0;
				MechBTH(wounded) = 75;
				mech_notify(wounded, MECHALL,
							"Your sensors have been destroyed!");
			}
			break;
		case HEAT_SINK:
			if(MechHasDHS(mech)) {
				MechRealNumsinks(wounded) -= 2;
				DestroyWeapon(wounded, hitloc, critType, critHit, 1,
							  HS_Size(mech));
				destroycrit = 0;
			} else
				MechRealNumsinks(wounded)--;
			mech_notify(wounded, MECHALL, "You lost a heat sink!");
			if(!Destroyed(wounded)) {
				sprintf(msgbuf, "'s %s is covered in a green mist!", locname);
				MechLOSBroadcast(wounded, msgbuf);
			}
			break;
		case JUMP_JET:
			if(!Destroyed(wounded) && Started(wounded)) {
				sprintf(msgbuf,
						"'s %s flares as superheated plasma spews out!",
						locname);
				MechLOSBroadcast(wounded, msgbuf);
			}
			MechJumpSpeed(wounded) -= MP1;
			if(MechJumpSpeed(wounded) < 0)
				MechJumpSpeed(wounded) = 0;
			mech_notify(wounded, MECHALL,
						"One of your jump jet engines has shut down!");
			if(attacker && MechJumpSpeed(wounded) < MP1 && Jumping(wounded)) {
				mech_notify(wounded, MECHALL,
							"Losing your last jump jet, you fall from the sky!");
				MechLOSBroadcast(wounded, "falls from the sky!");
				MechFalls(wounded, 1, 0);
				domino_space(wounded, 2);
			}
			break;
		case ENGINE:
			if(!Destroyed(wounded) && Started(wounded)) {
				sprintf(msgbuf, "'s %s spews black smoke!", locname);
				MechLOSBroadcast(wounded, msgbuf);
			}
			if(MechEngineHeat(wounded) < 10) {
				MechEngineHeat(wounded) += 5;
				mech_notify(wounded, MECHALL,
							"Your engine shielding takes a hit! It's getting hotter in here!");
			} else if(MechEngineHeat(wounded) < 15) {
				MechEngineHeat(wounded) = 15;
				mech_notify(wounded, MECHALL, "Your engine is destroyed!");
				if(wounded != attacker &&
				   !(MechStatus(wounded) & DESTROYED) && attacker)
					mech_notify(attacker, MECHALL,
								"You destroy the engine!");
				DestroyMech(wounded, attacker, 1, KILL_TYPE_NORMAL);
			}
			break;
		case TARGETING_COMPUTER:
			if(!MechCritStatus(wounded) & TC_DESTROYED) {
				mech_notify(wounded, MECHALL,
							"Your targeting computer is destroyed!");
				MechCritStatus(wounded) |= TC_DESTROYED;
			}
			break;
		case GYRO:
			/* Hardened Gyro's take one extra hit before damaged */
			if(MechSpecials2(wounded) & HDGYRO_TECH)
				if(!MechCritStatus2(wounded) & HDGYRO_DAMAGED) {
					sprintf(msgbuf, "emits a screech as its "
							"hardened gyro buckles slightly!");
					MechLOSBroadcast(wounded, msgbuf);
					MechCritStatus2(wounded) |= HDGYRO_DAMAGED;
					mech_notify(wounded, MECHALL,
								"Your hardened gyro takes a hit!");
					break;
				}

			if(!(MechCritStatus(wounded) & GYRO_DAMAGED)) {
				if(!Destroyed(wounded) && Started(wounded)) {
					sprintf(msgbuf, "emits a loud screech as "
							"its gyro buckles under the impact!");
					MechLOSBroadcast(wounded, msgbuf);
				}
				MechCritStatus(wounded) |= GYRO_DAMAGED;
				MechPilotSkillBase(wounded) += 3;
				mech_notify(wounded, MECHALL, "Your Gyro has been damaged!");
				if(attacker)
					if(!MadePilotSkillRoll(wounded, 0) && !Fallen(wounded)) {
						if(!Jumping(wounded) && !OODing(wounded)) {
							mech_notify(wounded, MECHALL,
										"You lose your balance and fall down!");
							MechLOSBroadcast(wounded,
											 "stumbles and falls down.");
							MechFalls(wounded, 1, 0);
						} else {
							mech_notify(wounded, MECHALL,
										"You fall from the sky!");
							MechLOSBroadcast(wounded, "falls from the sky!");
							MechFalls(wounded, JumpSpeedMP(wounded, map), 0);
							domino_space(wounded, 2);
						}
					}
			} else if(!(MechCritStatus(wounded) & GYRO_DESTROYED)) {
				MechCritStatus(wounded) |= GYRO_DESTROYED;
				mech_notify(wounded, MECHALL,
							"Your Gyro has been destroyed!");

				if(attacker) {
					if(!Fallen(wounded) && !Jumping(wounded) &&
					   !OODing(wounded)) {
						mech_notify(wounded, MECHALL,
									"You fall and you can't get up!");
						MechLOSBroadcast(wounded, "is knocked over!");
						MechFalls(wounded, 1, 0);
					} else if(!Fallen(wounded)
							  && (Jumping(wounded) || OODing(wounded))) {
						mech_notify(wounded, MECHALL,
									"You fall from the sky!");
						MechLOSBroadcast(wounded, "falls from the sky!");
						MechFalls(wounded, JumpSpeedMP(wounded, map), 0);
						domino_space(wounded, 2);
					}
				}
			} else {
				mech_notify(wounded, MECHALL,
							"Your destroyed gyro takes another hit!");
			}
			break;
		case SHOULDER_OR_HIP:
			DestroyPart(wounded, hitloc, critHit);
			destroycrit = 0;

			if(tLocIsArm) {
				mech_notify(wounded, MECHALL,
							"Your shoulder joint takes a hit and is frozen!");
				NormalizeLocActuatorCrits(wounded, hitloc);
			} else if(tLocIsLeg) {
				if(!Destroyed(wounded) && Started(wounded)) {
					sprintf(msgbuf, "'s hip locks into place!");
					MechLOSBroadcast(wounded, msgbuf);
				}

				mech_notify(wounded, MECHALL,
							"Your hip takes a direct hit and freezes up!");

				if(!(MechCritStatus(wounded) & HIP_DAMAGED)) {
					MechCritStatus(wounded) |= HIP_DAMAGED;
				} else {
					if(!MechIsQuad(wounded))
						MechCritStatus(wounded) |= HIP_DESTROYED;
				}

				NormalizeAllActuatorCrits(wounded);

				if(attacker && !Jumping(wounded) && !OODing(wounded)
				   && !MadePilotSkillRoll(wounded, 0)) {
					mech_notify(wounded, MECHALL,
								"You lose your balance and fall down!");
					MechLOSBroadcast(wounded, "stumbles and falls down!");
					MechFalls(wounded, 1, 0);
				}
			}
			break;
		case LOWER_ACTUATOR:
		case UPPER_ACTUATOR:
		case HAND_OR_FOOT_ACTUATOR:
			DestroyPart(wounded, hitloc, critHit);
			destroycrit = 0;

			if(tLocIsArm) {
				if(Special2I(critType) == HAND_OR_FOOT_ACTUATOR)
					mech_printf(wounded, MECHALL,
								"Your %s hand actuator is destroyed!",
								hitloc == LARM ? "left" : "right");
				else
					mech_printf(wounded, MECHALL,
								"Your %s %s arm actuator is destroyed!",
								hitloc == LARM ? "left" : "right",
								Special2I(critType) ==
								LOWER_ACTUATOR ? "lower" : "upper");

				if((Special2I(critType) == HAND_OR_FOOT_ACTUATOR) &&
				   (MechSections(mech)[hitloc].specials & CARRYING_CLUB))
					DropClub(mech);
				if(MechCarrying(mech) > 0) {
					mech_notify(mech, MECHALL,"The hit causes your tow line to let go!");
					MechLOSBroadcast(mech,"'s tow lines release and flap freely behind it!");
					mech_dropoff(GOD, mech, "");
				}		
				NormalizeLocActuatorCrits(wounded, hitloc);
			} else if(tLocIsLeg) {
				mech_notify(wounded, MECHALL,
							"One of your leg actuators is destroyed!");

				if(OkayCritSectS(hitloc, 0, SHOULDER_OR_HIP)) {	/* don't need to bother with crits if we already have a hip crit here */
					if(!Destroyed(wounded) && Started(wounded)) {
						sprintf(msgbuf, "'s %s twists in an odd way!",
								locname);
						MechLOSBroadcast(wounded, msgbuf);
					}

					NormalizeAllActuatorCrits(wounded);

					if(attacker && !Jumping(wounded) && !OODing(wounded)
					   && !MadePilotSkillRoll(wounded, 0)) {
						mech_notify(wounded, MECHALL,
									"You lose your balance and fall down!");
						MechLOSBroadcast(wounded, "stumbles and falls down!");
						MechFalls(wounded, 1, 0);
					}
				}
			}
			break;
		case C3_MASTER:
			temp = MechWorkingC3Masters(mech);
			MechWorkingC3Masters(mech) = countWorkingC3MastersOnMech(mech);

			if(temp == MechWorkingC3Masters(mech))
				mech_notify(wounded, MECHALL,
							"Your destroyed C3 system takes another hit!");
			else {
				if(MechWorkingC3Masters(mech) == 0) {
					MechCritStatus(wounded) |= C3_DESTROYED;

					checkTAG(mech);
				}

				if(MechTotalC3Masters(mech))
					mech_notify(wounded, MECHALL,
								"One of your C3 systems has been destroyed!");
				else
					mech_notify(wounded, MECHALL,
								"Your C3 system has been destroyed!");
			}

			break;
		case C3_SLAVE:
			MechCritStatus(wounded) |= C3_DESTROYED;
			mech_notify(wounded, MECHALL,
						"Your C3 system has been destroyed!");
			break;
		case C3I:
			MechCritStatus(wounded) |= C3I_DESTROYED;
			mech_notify(wounded, MECHALL,
						"Your C3i system has been destroyed!");

			clearC3iNetwork(mech, 1);
			break;
		case TAG:
			MechCritStatus(wounded) |= TAG_DESTROYED;
			mech_notify(wounded, MECHALL,
						"Your TAG system has been destroyed!");

			checkTAG(mech);
			break;
		case ECM:
			MechCritStatus(wounded) |= ECM_DESTROYED;
			mech_notify(wounded, MECHALL,
						"Your ECM system has been destroyed!");
			DisableECM(wounded);
			DisableECCM(wounded);

			if(StealthArmorActive(wounded)) {
				mech_notify(wounded, MECHALL,
							"Your stealth armor system shuts down!");
				DisableStealthArmor(wounded);
			}

			break;
		case ANGELECM:
			MechCritStatus(wounded) |= ANGEL_ECM_DESTROYED;
			mech_notify(wounded, MECHALL,
						"Your Angel ECM system has been destroyed!");
			DisableAngelECM(wounded);
			DisableAngelECCM(wounded);

			break;
		case BEAGLE_PROBE:
			MechCritStatus(wounded) |= BEAGLE_DESTROYED;
			MechSpecials(wounded) &= ~BEAGLE_PROBE_TECH;
			mech_notify(wounded, MECHALL,
						"Your Beagle Active Probe has been destroyed!");
			if(((sensors[(short) MechSensor(wounded)[0]].required_special
				 == BEAGLE_PROBE_TECH) &&
				(sensors[(short) MechSensor(wounded)[0]].specials_set
				 == 1)) ||
			   ((sensors[(short) MechSensor(wounded)[1]].required_special
				 == BEAGLE_PROBE_TECH) &&
				(sensors[(short) MechSensor(wounded)[1]].specials_set
				 == 1))) {

				if((sensors[(short)
							MechSensor(wounded)[0]].required_special ==
					BEAGLE_PROBE_TECH) &&
				   (sensors[(short) MechSensor(wounded)[0]].specials_set
					== 1))
					MechSensor(wounded)[0] = 0;

				if((sensors[(short)
							MechSensor(wounded)[1]].required_special ==
					BEAGLE_PROBE_TECH) &&
				   (sensors[(short) MechSensor(wounded)[1]].specials_set
					== 1))
					MechSensor(wounded)[1] = 0;

				MarkForLOSUpdate(wounded);
			}
			break;
		case BLOODHOUND_PROBE:
			MechCritStatus(wounded) |= BLOODHOUND_DESTROYED;
			MechSpecials2(wounded) &= ~BLOODHOUND_PROBE_TECH;
			mech_notify(wounded, MECHALL,
						"Your Bloodhound Probe has been destroyed!");

			if(((sensors[(short) MechSensor(wounded)[0]].required_special
				 == BLOODHOUND_PROBE_TECH) &&
				(sensors[(short) MechSensor(wounded)[0]].specials_set
				 == 1)) ||
			   ((sensors[(short) MechSensor(wounded)[1]].required_special
				 == BLOODHOUND_PROBE_TECH) &&
				(sensors[(short) MechSensor(wounded)[1]].specials_set
				 == 1))) {

				if((sensors[(short)
							MechSensor(wounded)[0]].required_special ==
					BLOODHOUND_PROBE_TECH) &&
				   (sensors[(short) MechSensor(wounded)[0]].specials_set
					== 1))
					MechSensor(wounded)[0] = 0;

				if((sensors[(short)
							MechSensor(wounded)[1]].required_special ==
					BLOODHOUND_PROBE_TECH) &&
				   (sensors[(short) MechSensor(wounded)[1]].specials_set
					== 1))
					MechSensor(wounded)[1] = 0;

				MarkForLOSUpdate(wounded);
			}
			break;
		case ARTEMIS_IV:
			weapon_slot = GetPartData(wounded, hitloc, critHit);
			if(weapon_slot > NUM_CRITICALS) {
				SendError(tprintf("Artemis IV error on mech %d",
								  wounded->mynum));
				break;
			}
			GetPartAmmoMode(wounded, hitloc, weapon_slot) &= ~ARTEMIS_MODE;
			mech_notify(wounded, MECHALL,
						"Your Artemis IV system has been destroyed!");
			break;
		case AXE:
			mech_notify(wounded, MECHALL, "Your axe has been destroyed!");
			break;
		case SWORD:
			mech_notify(wounded, MECHALL, "Your sword has been destroyed!");
			break;
		case DUAL_SAW:
			mech_notify(wounded, MECHALL,
						"Your dual saw has been destroyed!");
			break;
		case MACE:
			mech_notify(wounded, MECHALL, "Your mace has been destroyed!");
			break;
		case DS_AERODOOR:
			mech_notify(wounded, MECHALL,
						"One of the aero doors has been rendered useless!");
			break;
		case DS_MECHDOOR:
			mech_notify(wounded, MECHALL,
						"One of the 'mech doors has been rendered useless!");
		case NULL_SIGNATURE_SYSTEM:
			mech_notify(wounded, MECHALL,
						"Your Null Signature System has been destroyed!");

			if(NullSigSysActive(wounded)) {
				mech_notify(wounded, MECHALL,
							"Your Null Signature System shuts down!");
				DisableNullSigSys(wounded);
			}

			DestroyNullSigSys(wounded);

			break;
		}

		if(destroycrit)
			DestroyPart(wounded, hitloc, critHit);
	}

	return 1;
}

void HandleCritical(MECH * wounded, MECH * attacker, int LOS, int hitloc,
					int num)
{
	int i;
	int critHit;
	int critType, critData;
	int count, index;
	int critList[NUM_CRITICALS];

	if(MechStatus(wounded) & COMBAT_SAFE)
		return;
	if(MechSpecials(wounded) & CRITPROOF_TECH)
		return;
	if(MechType(wounded) == CLASS_MW && Number(1, 2) == 1)
		return;
	if(MechType(wounded) != CLASS_MECH && !mudconf.btech_vcrit)
		return;
	if(MechType(wounded) == CLASS_VEH_GROUND ||
	   MechType(wounded) == CLASS_VEH_NAVAL) {
		if(mudconf.btech_fasaadvvhlcrit) {
			for(i = 0; i < num; i++)
				HandleAdvFasaVehicleCrit(wounded, attacker, LOS, hitloc, num);

			return;
		} else if(!mudconf.btech_fasacrit) {
			for(i = 0; i < num; i++)
				HandleVehicleCrit(wounded, attacker, LOS, hitloc, num);
			return;
		} else if(mudconf.btech_fasacrit) {
			for(i = 0; i < num; i++)
				HandleFasaVehicleCrit(wounded, attacker, LOS, hitloc, num);
			return;
		}
	}
	if(IsDS(wounded))
		return;
	if(MechType(wounded) == CLASS_VTOL) {
		if(mudconf.btech_fasaadvvtolcrit) {
			for(i = 0; i < num; i++)
				HandleAdvFasaVehicleCrit(wounded, attacker, LOS, hitloc, num);

			return;
		} else {
			for(i = 0; i < num; i++)
				HandleVTOLCrit(wounded, attacker, LOS, hitloc, num);

			return;
		}
	}
	while (num > 0) {
		count = 0;
		while (count == 0) {
			for(i = 0; i < NUM_CRITICALS; i++) {
				critType = GetPartType(wounded, hitloc, i);
				if(!PartIsDestroyed(wounded, hitloc, i)
				   && !PartIsDamaged(wounded, hitloc, i)
				   && critType != EMPTY && critType != Special(CASE)
				   && critType != Special(FERRO_FIBROUS)
				   && critType != Special(STEALTH_ARMOR)
				   && critType != Special(HVY_FERRO_FIBROUS)
				   && critType != Special(LT_FERRO_FIBROUS)
				   && critType != Special(ENDO_STEEL)
				   && critType != Special(TRIPLE_STRENGTH_MYOMER)
				   && critType != Special(SUPERCHARGER)
				   && critType != Special(MASC)) {
					critList[count] = i;
					count++;
				}
			}

			if(!count)			/* transfer Crit to next location - no longer */
				return;
		}

		index = Number(0, count - 1);
		critHit = critList[index];	/* This one should be linear */

		critType = GetPartType(wounded, hitloc, critHit);
		critData = GetPartData(wounded, hitloc, critHit);

		if(HandleMechCrit(wounded, attacker, LOS, hitloc, critHit,
						  critType, critData))
			num--;
	}
}
