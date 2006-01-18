/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 2002 Dr. Martin Brumm
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 */

#include "config.h"
#include <math.h>

#include "mech.h"
#include "mech.events.h"
#include "muxevent.h"
#include "p.event.h"
#include "p.mech.utils.h"
#include "p.mech.los.h"
#include "p.eject.h"
#include "p.mech.pickup.h"
#include "p.mech.update.h"
#include "p.crit.h"
#include "p.mech.tag.h"
#include "p.bsuit.h"
#include "p.mech.ice.h"

void mech_pickup(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MECH *target;
	dbref target_num;
	MAP *newmap;
	int argc, through_ice;
	char *args[4];

	if(player != GOD)
		cch(MECH_USUAL);
	argc = mech_parseattributes(buffer, args, 1);
#ifdef BT_MOVEMENT_MODES
	DOCHECK(MoveModeLock(mech),
			"You cannot tow currently in this movement mode!");
#endif
	DOCHECK(argc != 1, "Invalid number of arguments.");
	target_num = FindTargetDBREFFromMapNumber(mech, args[0]);
	DOCHECK(target_num == -1, "That target is not in your line of sight.");
	target = getMech(target_num);
	DOCHECK(!target ||
			!InLineOfSight(mech, target, MechX(target), MechY(target),
						   FaMechRange(mech, target)),
			"That target is not in your line of sight.");
	DOCHECK(MechSpecials2(target) & CARRIER_TECH
			&& !MechSpecials2(mech) & CARRIER_TECH,
			"You cannot handle the mass on that carrier.");
	DOCHECK(CarryingClub(mech),
			"You can't pickup while you're carrying a club!");
	DOCHECK(Jumping(mech), "You can't pickup while jumping!");
	DOCHECK(Jumping(target),
			"What are you going to do? Grab it from mid air?");
	DOCHECK(Fallen(mech), "You are in no position to pick anything up!");
	DOCHECK(MechZ(mech) > (MechZ(target) + 3),
			"You are too high above the target.");
	DOCHECK(MechZ(mech) < MechZ(target) - 2,
			"You are too far below the target.");
	DOCHECK(MechX(mech) != MechX(target) ||
			MechY(mech) != MechY(target), "You need to be in the same hex!");
	DOCHECK(((MechZ(target) <= 0) && (MechRTerrain(target) == BRIDGE) &&
			 (MechZ(mech) > 0)),
			"You need to be under the bridge to pick up this unit.");
	DOCHECK(Towed(target), "That target's already being towed by someone!");
	DOCHECK(MechSwarmTarget(target) == mech->mynum, "You can't grab hold!");
	DOCHECK(MechTons(mech) < 5 || (!In_Character(target->mynum) &&
								   !Towable(target)), "You can't tow that!");
	DOCHECK(MechCritStatus(target) & HIDDEN,
			"You cannot pickup hiding targets....");
	DOCHECK(Burning(target), "You can't tow a burning unit!");
	if(MechType(target) == CLASS_MW) {
		pickup_mw(mech, target);
		return;
	} else {
		if(MechType(mech) == CLASS_MECH) {
			DOCHECKMA(MechIsQuad(mech),
					  "You've got four left feet, you can't tow!");
			DOCHECKMA(SectIsDestroyed(mech, LARM),
					  "Your left arm is destroyed, you can't pick up anything.");
			DOCHECKMA(SectIsDestroyed(mech, RARM),
					  "Your right arm is destroyed, you can't pick up anything.");
			DOCHECKMA(!(OkayCritSectS(RARM, 3, HAND_OR_FOOT_ACTUATOR) &&
						OkayCritSectS(RARM, 0, SHOULDER_OR_HIP)) &&
					  !(OkayCritSectS(LARM, 3, HAND_OR_FOOT_ACTUATOR) &&
						OkayCritSectS(LARM, 0, SHOULDER_OR_HIP)),
					  "You need functioning arm to pick things up!");
		} else
			DOCHECK(!(MechSpecials(mech) & SALVAGE_TECH),
					"You can't pick that up in this MECH/VEHICLE");
	}
	DOCHECK(MechCarrying(mech) > 0, "You are already carrying a Mech");
	DOCHECK(((fabs(MechSpeed(mech)) > 1.0) ||
			 (fabs((float) MechVerticalSpeed(mech)) > 1.0)),
			"You are moving too fast to attempt a pickup.");
	DOCHECK(IsDS(target), "You can't pick that up!");
	DOCHECK(MechMove(target) == MOVE_NONE, "That's simply immobile!");
	DOCHECK(MechTeam(mech) != MechTeam(target) &&
			Started(target), "You can't pick that up!");

	if(Moving(target) && !Falling(target) && !OODing(target))
		StopMoving(target);

	DOCHECK(Moving(target), "You can't pick up a moving target!");

	mech_printf(target, MECHALL,
				"%s attaches his tow lines to you.",
				GetMechToMechID(target, mech));
	mech_printf(mech, MECHALL, "You attach your tow lines to %s.",
				GetMechToMechID(mech, target));
	if(MechCarrying(target) > 0)
		mech_dropoff(GOD, target, "");
	if((newmap = getMap(target->mapindex)))
		MechLOSBroadcasti(mech, target, "picks up %s!");
	SetCarrying(mech, target->mynum);
	MechSwarmTarget(target) = -1;
	if(MechType(target) == CLASS_MECH) {
		MechStatus(target) |= FALLEN;
		FallCentersTorso(target);
		StopStand(target);
	}
	MechStatus(target) |= TOWED;
	MechStatus(target) &= ~HULLDOWN;
	MechTankCritStatus(target) &= ~DUG_IN;

	through_ice = (MechRTerrain(target) == ICE && MechZ(mech) >= 0 &&
				   MechZ(target) < 0);
	MirrorPosition(mech, target, 0);
	if(through_ice) {
		if(MechZ(mech) == 0 && MechMove(mech) != MOVE_HOVER)
			drop_thru_ice(mech);
		else
			break_thru_ice(mech);
	}
	if(!Destroyed(target))
		Shutdown(target);

	/* Adjust the speed involved */
	correct_speed(mech);

	/* Send emit for triggers/debugging */
	SendDebug(tprintf("#%d has picked up #%d", mech->mynum, target->mynum));
}

void mech_attachcables(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MECH *towMech;
	MECH *target;
	dbref towMech_num;
	dbref target_num;
	int argc;
	char *args[3];
	char mechName[SBUF_SIZE];
	char towMechName[SBUF_SIZE];
	char targetName[SBUF_SIZE];

	if(player != GOD)
		cch(MECH_USUAL);

	argc = mech_parseattributes(buffer, args, 2);
	DOCHECK(argc != 2, "Invalid number of arguments.");

	DOCHECK(OODing(mech),
			"You can't attach cables while floating in the air!");

	/* Check the towing unit. */
	towMech_num = FindTargetDBREFFromMapNumber(mech, args[0]);
	DOCHECK(towMech_num == -1,
			"That towing unit is not in your line of sight.");
	towMech = getMech(towMech_num);
	DOCHECK(!towMech ||
			!InLineOfSight(mech, towMech, MechX(towMech), MechY(towMech),
						   FaMechRange(mech, towMech)),
			"That towing unit is not in your line of sight.");
	DOCHECK(MechX(mech) != MechX(towMech) ||
			MechY(mech) != MechY(towMech),
			"You need to be in the same hex as the towing unit!");
	DOCHECK(Jumping(towMech),
			"That towing unit is currently flying through the air!");
	DOCHECK(MechZ(mech) != MechZ(towMech),
			"You must be on the same elevation as the towing unit!");
	DOCHECK(MechCarrying(towMech) > 0,
			"That towing unit is towing someone else!");
	DOCHECK(Towed(towMech),
			"That towing unit is already being towed by someone!");
	DOCHECK((MechType(towMech) == CLASS_MW), "That unit can not tow!");
	DOCHECK(MechMove(towMech) == MOVE_NONE, "That unit can not tow!");
	DOCHECK(MechTons(towMech) < 5, "That unit can not tow!");
	DOCHECK(Destroyed(towMech), "Destroyed units can not tow!");
	DOCHECK((MechTons(towMech) < 5) ||
			(!In_Character(towMech->mynum)), "That unit can not tow!");
	DOCHECK(Burning(towMech),
			"You can not attach tow cables to a burning unit!");
	DOCHECK((MechSpecials(towMech) & SALVAGE_TECH),
			"That is a dedicated towing unit and can pick up the target itself!");
	DOCHECK(((fabs(MechSpeed(towMech)) > 0.0) ||
			 (fabs((float) MechVerticalSpeed(towMech)) > 0.0)),
			"The towing unit is moving to fast for you to grab the tow cables!");
	DOCHECK((MechTeam(towMech) != MechTeam(mech)),
			"You can not grab the tow cables from that unit!");
	DOCHECK((MechType(towMech) != CLASS_MECH) &&
			(MechType(towMech) != CLASS_VEH_GROUND),
			"That unit can not tow!");

	/* Check the target */
	target_num = FindTargetDBREFFromMapNumber(mech, args[1]);
	DOCHECK(target_num == -1, "That target is not in your line of sight.");
	target = getMech(target_num);
	DOCHECK(!target ||
			!InLineOfSight(mech, target, MechX(target), MechY(target),
						   FaMechRange(mech, target)),
			"That target is not in your line of sight.");
	DOCHECK(MechX(mech) != MechX(target) ||
			MechY(mech) != MechY(target),
			"You need to be in the same hex as the target!");
	DOCHECK(Jumping(target),
			"That target is currently flying through the air!");
	DOCHECK(MechZ(mech) != MechZ(target),
			"You must be on the same elevation as the target!");
	DOCHECK(MechCarrying(target) > 0, "That target is towing someone else!");
	DOCHECK(Towed(target), "That target is already being towed by someone!");
	DOCHECK((!In_Character(target->mynum) &&
			 !Towable(target)), "That unit can not be towed!");
	DOCHECK(MechType(target) == CLASS_MW, "That unit can not be towed!");
	DOCHECK(MechMove(target) == MOVE_NONE, "That unit can not be towed!");
	DOCHECK(IsDS(target), "That unit can not be towed!");
	DOCHECK(Burning(target),
			"You can not attach tow cables to a burning unit!");
	DOCHECK(((fabs(MechSpeed(target)) > 0.0) ||
			 (fabs((float) MechVerticalSpeed(target)) > 0.0)),
			"The target is moving to fast for you to attach the tow cables!");
	DOCHECK((MechTeam(target) != MechTeam(mech)) &&
			Started(target), "That unit can not be towed!");

	if(Moving(target) && !Falling(target) && !OODing(target))
		StopMoving(target);

	DOCHECK(Moving(target),
			"The target is moving to fast for you to attach the tow cables!");

	strcpy(mechName, GetMechID(mech));
	strcpy(towMechName, GetMechID(towMech));
	strcpy(targetName, GetMechID(target));

	mech_printf(target, MECHALL,
				"%s attaches tow lines from %s to you.", mechName,
				towMechName);
	mech_printf(towMech, MECHALL,
				"%s attaches your tow lines to %s.", mechName, targetName);
	mech_printf(mech, MECHALL, "You attach %s's tow lines to %s.",
				towMechName, targetName);

	MechLOSBroadcast(mech,
					 tprintf("attaches tow cables from %s to %s!",
							 towMechName, targetName));

	SetCarrying(towMech, target->mynum);
	MechSwarmTarget(target) = -1;

	if(MechType(target) == CLASS_MECH) {
		MechStatus(target) |= FALLEN;
		FallCentersTorso(target);
		StopStand(target);
	}

	MechStatus(target) |= TOWED;
	MechStatus(target) &= ~HULLDOWN;
	MechTankCritStatus(target) &= ~DUG_IN;

	if(!Destroyed(target))
		Shutdown(target);

	/* Adjust the speed involved */
	correct_speed(towMech);
}

void mech_detachcables(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MECH *towMech;
	MECH *target;
	dbref towMech_num;
	MAP *newmap;
	dbref aRef;
	int argc;
	char *args[2];
	char mechName[SBUF_SIZE];
	char towMechName[SBUF_SIZE];
	char targetName[SBUF_SIZE];

	cch(MECH_USUAL);

	argc = mech_parseattributes(buffer, args, 1);
	DOCHECK(argc != 1, "Invalid number of arguments.");

	towMech_num = FindTargetDBREFFromMapNumber(mech, args[0]);
	DOCHECK(towMech_num == -1,
			"That towing unit is not in your line of sight.");
	towMech = getMech(towMech_num);
	DOCHECK(!towMech ||
			!InLineOfSight(mech, towMech, MechX(towMech), MechY(towMech),
						   FaMechRange(mech, towMech)),
			"That towing unit is not in your line of sight.");
	DOCHECK(MechX(mech) != MechX(towMech) ||
			MechY(mech) != MechY(towMech),
			"You need to be in the same hex as the towing unit!");
	DOCHECK(MechZ(mech) != MechZ(towMech),
			"You must be on the same elevation as the towing unit!");
	DOCHECK(MechCarrying(towMech) <= 0, "That unit is not towing anyone!");

	aRef = MechCarrying(towMech);
	SetCarrying(towMech, -1);
	target = getMech(aRef);
	DOCHECK(!target, "The towed unit was invalid!");
	MechStatus(target) &= ~TOWED;	/* Reset the Towed flag */

	strcpy(mechName, GetMechID(mech));
	strcpy(towMechName, GetMechID(towMech));
	strcpy(targetName, GetMechID(target));

	mech_printf(mech, MECHALL,
				"You detach %s's tow lines from %s.", towMechName,
				targetName);
	mech_printf(towMech, MECHALL,
				"%s detaches your tow lines from %s.", mechName, targetName);
	mech_notify(target, MECHALL, "You have been released from towing.");

	StopMoving(target);
	MechSpeed(target) = 0;
	MechDesiredSpeed(target) = 0;

	MechLOSBroadcast(mech, tprintf("detaches %s's tow cables from %s!",
								   towMechName, targetName));

	if((newmap = getMap(target->mapindex))) {
		MechZ(target) = Elevation(newmap, MechX(towMech), MechY(towMech));
	}

	correct_speed(towMech);
}

void mech_dropoff(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MECH *target;
	MAP *newmap;
	dbref aRef;
	int x, y;

	if(player != GOD)
		cch(MECH_USUAL);

	DOCHECK(MechCarrying(mech) <= 0, "You aren't carrying a mech!");
	aRef = MechCarrying(mech);
	SetCarrying(mech, -1);
	target = getMech(aRef);
	DOCHECK(!target, "You were towing invalid target!");
	MechStatus(target) &= ~TOWED;	/* Reset the Towed flag */
	mech_notify(mech, MECHALL, "You drop the mech you were carrying.");
	mech_notify(target, MECHALL, "You have been released from towing.");

	StopMoving(target);
	MechSpeed(target) = 0;
	MechDesiredSpeed(target) = 0;

	if((newmap = getMap(target->mapindex))) {
		MechLOSBroadcasti(mech, target, "drops %s!");
		if((x = MechZ(target)) > ((y =
								   Elevation(newmap, MechX(target),
											 MechY(target))) + 2)) {
			mech_notify(mech, MECHALL,
						"Maybe you should have done this closer to the ground.");
			mech_notify(target, MECHALL,
						"You wish he had done that a might bit closer to the ground.");
			MechLOSBroadcast(target, "falls through the sky.");
			MECHEVENT(target, EVENT_FALL, mech_fall_event, FALL_TICK, -1);
		} else {
			MechZ(target) = Elevation(newmap, MechX(mech), MechY(mech));
		}
	}
	correct_speed(mech);
	SendDebug(tprintf("#%d has dropped off #%d", mech->mynum, target->mynum));
}
