/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 */

#include <math.h>
#include "mech.events.h"
#include "mech.notify.h"
#include "p.aero.move.h"
#include "p.mech.utils.h"
#include "p.mech.update.h"
#include "p.mech.los.h"
#include "p.btechstats.h"
#include "p.mech.sensor.h"
#include "p.mech.partnames.h"
#include "p.mech.combat.misc.h"

#undef WEAPON_RECYCLE_DEBUG

void mech_heartbeat(MECH *mech) {
    UpdateRecycling(mech);
    return;
}

static int factoral(int n)
{
	int i, j = 0;

	for(i = 1; i <= n; i++)
		j += i;
	return j;
}

void mech_standfail_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;

	mech_notify(mech, MECHALL,
				"%cgYou have finally recovered from your attempt to stand.%c");
}

void mech_fall_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;
	int fallspeed = (int) e->data2;
	int fallen_elev;

	if(Started(mech) && fallspeed >= 0)
		return;
	if(fallspeed <= 0 && (!Started(mech) || !(FlyingT(mech)) ||
						  ((AeroFuel(mech) <= 0) && !AeroFreeFuel(mech)) ||
						  ((MechType(mech) == CLASS_VTOL) &&
						   (SectIsDestroyed(mech, ROTOR)))))
		fallspeed -= FALL_ACCEL;
	else
		fallspeed += FALL_ACCEL;
	MarkForLOSUpdate(mech);
	if(MechsElevation(mech) > abs(fallspeed)) {
		MechZ(mech) -= abs(fallspeed);
		MechFZ(mech) = MechZ(mech) * ZSCALE;
		MECHEVENT(mech, EVENT_FALL, mech_fall_event, FALL_TICK, fallspeed);
		return;
	}
	/* Time to hit da ground */
	fallen_elev = factoral(abs(fallspeed));
	mech_notify(mech, MECHALL, "You hit the ground!");
	MechLOSBroadcast(mech, "hits the ground!");
	MechFalls(mech, fallen_elev, 0);
	MechStatus(mech) &= ~JUMPING;
}

/* This is just a 'toy' event */
void mech_lock_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;
	MAP *map;
	MECH *target;

	if(MechTarget(mech) >= 0) {
		map = getMap(mech->mapindex);
		target = FindObjectsData(MechTarget(mech));
		if(!target)
			return;
		if(!InLineOfSight(mech, target, MechX(target), MechY(target),
						  FlMechRange(map, mech, target)))
			return;
		mech_printf(mech, MECHALL,
					"The sensors acquire a stable lock on %s.",
					GetMechToMechID(mech, target));
	} else if(MechTargX(mech) >= 0 && MechTargY(mech) >= 0)
		mech_printf(mech, MECHALL,
					"The sensors acquire a stable lock on (%d,%d).",
					MechTargX(mech), MechTargY(mech));

}

/* Various events that don't fit too well to other categories */

/* Basically the update events + some movenement events */
void mech_stabilizing_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;

	mech_notify(mech, MECHSTARTED,
				"%cgYou have finally stabilized after your jump.%c");
}

void mech_jump_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;

	MECHEVENT(mech, EVENT_JUMP, mech_jump_event, JUMP_TICK, 0);
	move_mech(mech);
	if(!Jumping(mech))
		StopJump(mech);
}

extern int PilotStatusRollNeeded[];

void mech_recovery_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;

	if(Destroyed(mech) || !Uncon(mech))
		return;
	if(handlemwconc(mech, 0)) {
		MechStatus(mech) &= ~UNCONSCIOUS;
		mech_notify(mech, MECHALL, "The pilot regains consciousness!");
		return;
	}
}

void ProlongUncon(MECH * mech, int len)
{
	int l;

	if(Destroyed(mech))
		return;
	if(!Recovering(mech)) {
		MechStatus(mech) |= UNCONSCIOUS;
		MECHEVENT(mech, EVENT_RECOVERY, mech_recovery_event, len, 0);
		return;
	}
	l = muxevent_last_type_data(EVENT_RECOVERY, (void *) mech) + len;
	muxevent_remove_type_data(EVENT_RECOVERY, (void *) mech);
	MECHEVENT(mech, EVENT_RECOVERY, mech_recovery_event, l, 0);
}

struct foo {
	char *name;
	char *full;
	int ofs;
};
extern struct foo lateral_modes[];

#ifdef BT_MOVEMENT_MODES
void mech_sideslip_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;
	int roll;

	if(!mech || !Started(mech))
		return;
	mech_notify(mech, MECHALL, "You make a skill roll while sideslipping!");
	if(!MadePilotSkillRoll
	   (mech,
		HasBoolAdvantage(MechPilot(mech), "maneuvering_ace") ? -1 : 0)) {
		mech_notify(mech, MECHALL, "You fail and spin out!");
		MechLOSBroadcast(mech, "spins out while sideslipping!");
		MechSpeed(mech) = 0.0;
		roll = Number(0, 5);
		AddFacing(mech, roll * 60);
		SetFacing(mech, AcceptableDegree(MechFacing(mech)));
		MechDesiredFacing(mech) = MechFacing(mech);
		MechDesiredSpeed(mech) = 0.0;
		MechLateral(mech) = 0;
		return;
	}
	MECHEVENT(mech, EVENT_SIDESLIP, mech_sideslip_event, TURN, 0);
}
#endif

void mech_lateral_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;
	int latmode = (int) e->data2;

	if(!mech || !Started(mech))
		return;
	mech_printf(mech, MECHALL,
				"Lateral movement mode change to %s (%d offset) completed.",
				lateral_modes[latmode].full, lateral_modes[latmode].ofs);
	MechLateral(mech) = lateral_modes[latmode].ofs;
#ifdef BT_MOVEMENT_MODES
	if(MechMove(mech) != MOVE_QUAD) {
		if(MechLateral(mech) == 0)
			StopSideslip(mech);
		else if(!(SideSlipping(mech)))
			MECHEVENT(mech, EVENT_SIDESLIP, mech_sideslip_event, 1, 0);
	}
#endif
}

void mech_move_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;

	if(MechType(mech) == CLASS_VTOL)
		if(Landed(mech) || FuelCheck(mech))
			return;
	UpdateHeading(mech);
	if((IsMechLegLess(mech)) || Jumping(mech) || OODing(mech)) {
		if(MechDesiredFacing(mech) != MechFacing(mech))
			MECHEVENT(mech, EVENT_MOVE, mech_move_event, MOVE_TICK, 0);
		return;
	}
	UpdateSpeed(mech);
	move_mech(mech);

	if(mech->mapindex < 0)
		return;

	if(MechType(mech) == CLASS_VEH_NAVAL && MechRTerrain(mech) != BRIDGE &&
	   MechRTerrain(mech) != ICE && MechRTerrain(mech) != WATER)
		return;

	if(MechSpeed(mech) || MechDesiredSpeed(mech) ||
	   MechDesiredFacing(mech) != MechFacing(mech) ||
	   ((MechType(mech) == CLASS_VTOL || MechMove(mech) == MOVE_SUB) &&
		MechVerticalSpeed(mech)))
		MECHEVENT(mech, EVENT_MOVE, mech_move_event, MOVE_TICK, 0);
}

void mech_stand_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;

	MechLOSBroadcast(mech, "stands up!");
	mech_notify(mech, MECHALL, "You have finally finished standing up.");
	MakeMechStand(mech);
}

void mech_plos_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data, *target;
	MAP *map;
	int mapvis;
	int maplight;
	float range;
	int i;

	if(!Started(mech))
		return;
	if(!(map = getMap(mech->mapindex)))
		return;
	MECHEVENT(mech, EVENT_PLOS, mech_plos_event, PLOS_TICK, 0);
	if(!MechPNumSeen(mech) && !(MechSpecials(mech) & AA_TECH))
		return;
	mapvis = map->mapvis;
	maplight = map->maplight;
	MechPNumSeen(mech) = 0;
	for(i = 0; i < map->first_free; i++)
		if(map->mechsOnMap[i] > 0 && map->mechsOnMap[i] != mech->mynum)
			if(!(map->LOSinfo[mech->mapnumber][i] & MECHLOSFLAG_SEEN)) {
				target = FindObjectsData(map->mechsOnMap[i]);
				if(!target)
					continue;
				range = FlMechRange(map, mech, target);
				MechPNumSeen(mech)++;
				Sensor_DoWeSeeNow(mech, &map->LOSinfo[mech->mapnumber][i],
								  range, -1, -1, target, mapvis, maplight,
								  map->cloudbase, 1, 0);

			}
}

void aero_move_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;

	if(!Landed(mech)) {
		/* Returns 1 only if we
		   1) Ran out of fuel, and
		   2) Were VTOL, and
		   3) Crashed 
		 */
		if(FuelCheck(mech))
			return;
		/* Genuine CHEAT :-) */
		if(Started(mech)) {
			aero_UpdateHeading(mech);
			aero_UpdateSpeed(mech);
		}
		if(Fallen(mech))
			MechStartFZ(mech) = MechStartFZ(mech) - 1;
		move_mech(mech);
		if(IsDS(mech) && MechZ(mech) <= (MechElevation(mech) + 5) &&
		   ((muxevent_tick / WEAPON_TICK) % 10) == 0)
			DS_BlastNearbyMechsAndTrees(mech,
										"You are hit by the DropShip's plasma exhaust!",
										"is hit directly by DropShip's exhaust!",
										"You are hit by the DropShip's plasma exhaust!",
										"is hit by DropShip's exhaust!",
										"light up and burn.", 8);
		MECHEVENT(mech, EVENT_MOVE, aero_move_event, MOVE_TICK, 0);
	} else if(Landed(mech) && !Fallen(mech) && RollingT(mech)) {
		UpdateHeading(mech);
		UpdateSpeed(mech);
		move_mech(mech);
		if(fabs(MechSpeed(mech)) > 0.0 ||
		   fabs(MechDesiredSpeed(mech)) > 0.0 ||
		   MechDesiredFacing(mech) != MechFacing(mech))
			if(!FuelCheck(mech))
				MECHEVENT(mech, EVENT_MOVE, aero_move_event, MOVE_TICK, 0);
	}
}

void very_fake_func(MUXEVENT * e)
{

}

/*
 * Exile Stun Code Event
 */
void mech_crewstun_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;

	if(!mech)
		return;
	if(!Started(mech) || Destroyed(mech)) {
		if(MechCritStatus(mech) & MECH_STUNNED)
			MechCritStatus(mech) &= ~MECH_STUNNED;
		return;
	}
	if(MechType(mech) != CLASS_MECH)
		mech_notify(mech, MECHALL,
					"%ch%cgThe crew recovers from their bewilderment!%cn");
	else
		mech_notify(mech, MECHALL,
					"%ch%cgYou recover from your stunning experience!%cn");

	if(MechCritStatus(mech) & MECH_STUNNED)
		MechCritStatus(mech) &= ~MECH_STUNNED;
}

void unstun_crew_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;

	if(CrewStunned(mech) > 1)	/* If we've been stunned again! */
		return;

	mech_notify(mech, MECHALL,
				"Your head clears and you're able to control your vehicle again.");
	MechTankCritStatus(mech) &= ~CREW_STUNNED;
}

void mech_unjam_ammo_event(MUXEVENT * objEvent)
{
	MECH *objMech = (MECH *) objEvent->data;	/* get the mech */
	int wWeapNum = (int) objEvent->data2;	/* and now the weapon number */
	int wSect, wSlot, wWeapStatus, wWeapIdx;
	int ammoLoc, ammoCrit,ammoLoc1, ammoCrit1;
	int wRoll = 0;
	int wRollNeeded = 0;

	if(Uncon(objMech) || !Started(objMech))
		return;

	wWeapStatus = FindWeaponNumberOnMech(objMech, wWeapNum, &wSect, &wSlot);

	if(wWeapStatus == TIC_NUM_DESTROYED)	/* return if the weapon has been destroyed */
		return;

	wWeapIdx = FindWeaponIndex(objMech, wWeapNum);

	if(!FindAndCheckAmmo(objMech, wWeapIdx, wSect, wSlot, &ammoLoc, &ammoCrit, &ammoLoc1, &ammoCrit1, 0)) {
		SetPartTempNuke(objMech, wSect, wSlot, 0);

		mech_printf(objMech, MECHALL,
					"You finish bouncing around and realize you no longer have ammo for your %s!",
					get_parts_long_name(I2Weapon(wWeapIdx), 0));
		return;
	}

	if(MechWeapons[wWeapStatus].special & RAC) {
		wRoll = Roll();
		wRollNeeded = FindPilotGunnery(objMech, wWeapStatus) + 3;

		mech_notify(objMech, MECHPILOT,
					"You make a roll to unjam the weapon!");
		mech_printf(objMech, MECHPILOT,
					"Modified Gunnery Skill: BTH %d\tRoll: %d",
					wRollNeeded, wRoll);

		if(wRoll < wRollNeeded) {
			mech_notify(objMech, MECHALL,
						"Your attempt to remove the jammed slug fails. You'll need to try again to clear it.");
			return;
		}
	} else {
		if(!MadePilotSkillRoll(objMech, 0)) {
			mech_notify(objMech, MECHALL,
						"Your attempt to remove the jammed slug fails. You'll need to try again to clear it.");
			return;
		}
	}

	SetPartTempNuke(objMech, wSect, wSlot, 0);
	mech_printf(objMech, MECHALL,
				"You manage to clear the jam on your %s!",
				get_parts_long_name(I2Weapon(wWeapIdx), 0));
	MechLOSBroadcast(objMech, "ejects a mangled shell!");

	decrement_ammunition(objMech, wWeapNum, wSect, wSlot, ammoLoc, ammoCrit, ammoLoc1, ammoCrit1, 0);

}

void check_stagger_event(MUXEVENT * event)
{
	MECH *mech = (MECH *) event->data;	/* get the mech */

	SendDebug(tprintf("Triggered stagger check for %d.", mech->mynum));

	if((StaggerLevel(mech) < 1) || Fallen(mech) ||
	   (MechType(mech) != CLASS_MECH)) {
		StopStaggerCheck(mech);
		return;
	}

	if(Jumping(mech)) {
		return;
	}

	mech_notify(mech, MECHALL, "You stagger from the damage!");
	if(!MadePilotSkillRoll(mech, calcStaggerBTHMod(mech))) {
		mech_notify(mech, MECHALL,
					"You loose the battle with gravity and tumble over!!");
		MechLOSBroadcast(mech, "tumbles over, staggered by the damage!");
		MechFalls(mech, 1, 0);
	}

	StopStaggerCheck(mech);
	/* Since stagger rolls happen much more often now, this adds 10 damage
	 * points of 'buffer' to mech that was just forced to make a stager roll.
	 * Mechs whose damage accumulation times out without making a roll (<20
	 * damage) don't get this help. This 10 points of damage assistance slowly
	 * times out in CheckDamage, or can be erased by weapons fire */
	mech->rd.staggerDamage = -10;
}

#ifdef BT_MOVEMENT_MODES
void mech_movemode_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;
	int i = (int) e->data2;
	int dir = (i & MODE_ON ? 1 : i & MODE_OFF ? 0 : 0);

	if(!mech || !Started(mech) || Destroyed(mech)) {
		MechStatus2(mech) &= ~(MOVE_MODES);
		return;
	}
	if(dir) {
		if(i & MODE_EVADE) {
			MechStatus2(mech) |= EVADING;
			mech_notify(mech, MECHALL,
						"You bounce chaotically as you maximize your movement mode to evade!");
			MechLOSBroadcast(mech,
							 "suddenly begins to move erratically performing evasive maneuvers!");
		} else if(i & MODE_SPRINT) {
			MechStatus2(mech) |= SPRINTING;
			mech_notify(mech, MECHALL,
						"You shimmy side to side as you get more speed from your movement mode.");
			if((MechType(mech) == CLASS_MECH)
			   || (MechType(mech) == CLASS_BSUIT))
				MechLOSBroadcast(mech,
								 "breaks out into a full blown stride as it sprints over the terrain!");
			else
				MechLOSBroadcast(mech,
								 "shifts into high gear as it gains more speed!");
			if(MechSpeed(mech) < 0) {
				mech_notify(mech, MECHALL,
							"You stop your backward momemtum while sprinting and come to a stop!");
				MechDesiredSpeed(mech) = 0;
			}
		} else if(i & MODE_DODGE) {
			if(MechFullNoRecycle(mech, CHECK_PHYS) > 0) {
				mech_notify(mech, MECHALL,
							"You cannot enter DODGE mode due to physical useage.");
				return;
			} else {
				MechStatus2(mech) |= DODGING;
				mech_notify(mech, MECHALL,
							"You brace yourself for any oncoming physicals.");
			}
		}
	} else {
		if(i & MODE_EVADE) {
			MechStatus2(mech) &= ~EVADING;
			mech_notify(mech, MECHALL,
						"Cockpit movement normalizes as you cease your evasive maneuvers.");
			MechLOSBroadcast(mech,
							 "ceases its evasive behavior and calms down.");
		} else if(i & MODE_SPRINT) {
			MechStatus2(mech) &= ~SPRINTING;
			mech_notify(mech, MECHALL,
						"You feel less seasick as you leave your sprint mode and resume normal movement.");
			MechLOSBroadcast(mech,
							 "slows down and enters a normal movement mode.");
		} else if(i & MODE_DODGE) {
			MechStatus2(mech) &= ~DODGING;
			if(i & MODE_DG_USED)
				mech_notify(mech, MECHALL,
							"You're dodge maneuver has been used and you are no longer braced for physicals.");
			else
				mech_notify(mech, MECHALL,
							"You loosen up your stance and no longer dodge physicals.");
		}
	}
	if(MechSpeed(mech) > MMaxSpeed(mech)
	   || MechDesiredSpeed(mech) > MMaxSpeed(mech))
		MechDesiredSpeed(mech) = MMaxSpeed(mech);
	return;
}
#endif

int calcStaggerBTHMod(MECH * mech)
{
	int bthMod = 0;
	int tonnageMod = 0;

	if(!Started(mech)) {
		bthMod = 999;
	} else {
		bthMod = StaggerLevel(mech);

		if(MechTons(mech) <= 35)
			tonnageMod = 1;
		else if(MechTons(mech) <= 55)
			tonnageMod = 0;
		else if(MechTons(mech) <= 75)
			tonnageMod = -1;
		else
			tonnageMod = -2;

		bthMod += tonnageMod;
	}

	return bthMod;
}
