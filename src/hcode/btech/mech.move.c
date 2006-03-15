/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "mech.events.h"
#include "p.mech.events.h"
#include "p.mech.ice.h"
#include "p.mech.utils.h"
#include "p.mine.h"
#include "p.bsuit.h"
#include "p.mech.los.h"
#include "p.mech.update.h"
#include "p.mech.physical.h"
#include "p.mech.combat.h"
#include "p.mech.combat.misc.h"
#include "p.mech.damage.h"
#include "p.btechstats.h"
#include "p.mech.hitloc.h"
#include "p.template.h"
#include "p.map.conditions.h"
#include "p.mech.fire.h"
#include "mech.events.h"

struct {
	char *name;
	char *full;
	int ofs;
} lateral_modes[] = {
	{
	"nw", "Front/Left", 300}, {
	"fl", "Front/Left", 300}, {
	"ne", "Front/Right", 60}, {
	"fr", "Front/Right", 60}, {
	"sw", "Rear/Left", 240}, {
	"rl", "Rear/Left", 240}, {
	"se", "Rear/Right", 120}, {
	"rr", "Rear/Right", 120}, {
	"-", "None", 0}, {
	NULL, 0}
};

const char *LateralDesc(MECH * mech)
{
	int i;

	for(i = 0; MechLateral(mech) != lateral_modes[i].ofs; i++);
	return lateral_modes[i].full;
}

void mech_lateral(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	int i;

	cch(MECH_USUALO);
#ifndef BT_MOVEMENT_MODES
	DOCHECK(MechType(mech) != CLASS_MECH ||
			!MechIsQuad(mech),
			"Only quadrupeds can alter their lateral movement!");
#else
	DOCHECK(!((MechType(mech) == CLASS_MECH && MechIsQuad(mech)) ||
			  (MechType(mech) == CLASS_VTOL) ||
			  (MechMove(mech) == MOVE_HOVER)),
			"You cannot alter your lateral movement!");
#endif
	DOCHECK(CountDestroyedLegs(mech) > 0,
			"You need all four legs to use lateral movement!");
	skipws(buffer);

	for(i = 0; lateral_modes[i].name; i++)
		if(!strcasecmp(lateral_modes[i].name, buffer))
			break;
	DOCHECK(!lateral_modes[i].name, "Invalid mode!");
	if(lateral_modes[i].ofs == MechLateral(mech)) {
		DOCHECK(!ChangingLateral(mech), "You are going that way already!");
		mech_notify(mech, MECHALL, "Lateral mode change aborted.");
		StopLateral(mech);
		return;
	}
	mech_printf(mech, MECHALL,
				"Wanted lateral movement mode changed to %s.",
				lateral_modes[i].full);
	StopLateral(mech);
	MECHEVENT(mech, EVENT_LATERAL, mech_lateral_event, LATERAL_TICK, i);
}
void mech_turnmode(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;

	if(!GotPilot(mech) || MechPilot(mech) != player) {
		notify(player, "You're not the pilot!");
		return;
	}

	if(!HasBoolAdvantage(player, "maneuvering_ace")) {
		mech_notify(mech, MECHPILOT, "You're not skilled enough to do that.");
		return;
	}

	if(buffer && !strcasecmp(buffer, "tight")) {
		SetTurnMode(mech, 1);
		mech_notify(mech, MECHALL, "You brace for tighter turns.");
		return;
	}
	if(buffer && !strcasecmp(buffer, "normal")) {
		SetTurnMode(mech, 0);
		mech_notify(mech, MECHALL, "You assume a normal turn mode.");
		return;
	}
	mech_printf(mech, MECHALL, "Your turning type is : %s",
				GetTurnMode(mech) ? "TIGHT" : "NORMAL");
	return;
}

void mech_bootlegger(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	float fMinSpeed = (4 * MP1);
	int wBTHMod = 0;
	int wFallLevels = 0;
	int i;
	int wHeadingChange = 0;
	int wNewHeading;
	float fMechSpeed = MechSpeed(mech);
	int wMechTons = MechTons(mech);
	char strLocation[50];
	char *args[1];

	cch(MECH_USUALO);

	DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
			"Invalid number of arguments!");
	DOCHECK(CountDestroyedLegs(mech) > 0,
			"You can't perform a bootlegger with destroyed legs!");
	DOCHECK(fMechSpeed < fMinSpeed,
			tprintf
			("You are going too slow to perform a bootlegger! The required minimum speed is %4.1f KPH.",
			 fMinSpeed));

	switch (toupper(args[0][0])) {
	case 'R':
		wHeadingChange = 90;
		break;
	case 'L':
		wHeadingChange = -90;
		break;
	}

	DOCHECK(wHeadingChange == 0, "Invalid turn direction!");

	for(i = 0; i < NUM_SECTIONS; i++) {
		if((i == LLEG) || (i == RLEG) || (MechIsQuad(mech) && ((i == LARM)
															   || (i ==
																   RARM)))) {
			ArmorStringFromIndex(i, strLocation, MechType(mech),
								 MechMove(mech));

			if(SectHasBusyWeap(mech, i)) {
				mech_printf(mech, MECHALL,
							"You have weapons recycling in your %s.",
							strLocation);
				return;
			}

			if(MechSections(mech)[i].recycle) {
				mech_printf(mech, MECHALL,
							"Your %s is still recovering from its last action.",
							strLocation);
				return;
			}

			wBTHMod += MechSections(mech)[i].basetohit;
		}
	}

	if(fMechSpeed <= (4 * MP1)) {
		wBTHMod += 0;
	} else if(fMechSpeed <= (8 * MP1)) {
		wBTHMod += 1;
	} else if(fMechSpeed <= (12 * MP1)) {
		wBTHMod += 2;
	} else {
		wBTHMod += 3;
	}

	if(wMechTons <= 35) {
		wBTHMod += 0;
	} else if(wMechTons <= 55) {
		wBTHMod += 1;
	} else if(wMechTons <= 75) {
		wBTHMod += 2;
	} else {
		wBTHMod += 3;
	}

	wBTHMod += (InWater(mech) ? 2 : 0);

	wBTHMod = MAX(wBTHMod, 1);

	skipws(buffer);

	SendDebug(tprintf
			  ("#%d attempts to do a bootlegger (mech). Tonnage: %d, Speed: %4.1f, BTHMod: %d",
			   mech->mynum, wMechTons, fMechSpeed, wBTHMod));

	if(MadePilotSkillRoll(mech, wBTHMod)) {
		wNewHeading = AcceptableDegree(MechFacing(mech) + wHeadingChange);

		SetFacing(mech, wNewHeading);
		MechDesiredFacing(mech) = wNewHeading;
		MechSpeed(mech) = MechSpeed(mech) / 2;

		mech_printf(mech, MECHALL,
					"You plant a foot and swivel, changing your heading to %d.",
					wNewHeading);

		for(i = 0; i < NUM_SECTIONS; i++) {
			if((i == LLEG) || (i == RLEG) || (MechIsQuad(mech) &&
											  ((i == LARM) || (i == RARM))))
				SetRecycleLimb(mech, i, 30);
		}

	} else {
		wFallLevels = MAX(wBTHMod, 1);

		mech_notify(mech, MECHALL, "You plant a foot and try to swivel...");
		mech_notify(mech, MECHALL,
					"... but realize a little late that this is harder than it looks!");
		MechLOSBroadcast(mech,
						 "attempts to fight the forces of inertia but looses the battle miserably!");

		if(wFallLevels > 2)
			MechLOSBroadcast(mech, "tumbles over and over and over!");

		MechFalls(mech, wFallLevels, 1);
	}
}

void mech_eta(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MAP *mech_map;
	int argc, eta_x, eta_y;
	float fx, fy, range;
	int etahr, etamin;
	char *args[3];

	cch(MECH_USUAL);
	mech_map = getMap(mech->mapindex);
	argc = mech_parseattributes(buffer, args, 2);
	DOCHECK(argc == 1, "Invalid number of arguments!");
	switch (argc) {
	case 0:
		DOCHECK(!(MechTargX(mech) >= 0 &&
				  MechTarget(mech) < 0),
				"You have invalid default target for ETA!");
		eta_x = MechTargX(mech);
		eta_y = MechTargY(mech);
		break;
	case 2:
		eta_x = atoi(args[0]);
		eta_y = atoi(args[1]);
		break;
	default:
		notify(player, "Invalid arguments!");
		return;
	}
	MapCoordToRealCoord(eta_x, eta_y, &fx, &fy);
	range = FindRange(MechFX(mech), MechFY(mech), 0, fx, fy, 0);
	if(fabs(MechSpeed(mech)) < 0.1)
		mech_printf(mech, MECHALL,
					"Range to hex (%d,%d) is %.1f.  ETA: Never, mech not moving.",
					eta_x, eta_y, range);
	else {
		etamin = abs(range / (MechSpeed(mech) / KPH_PER_MP));
		etahr = etamin / 60;
		etamin = etamin % 60;
		mech_printf(mech, MECHALL,
					"Range to hex (%d,%d) is %.1f.  ETA: %.2d:%.2d.",
					eta_x, eta_y, range, etahr, etamin);
	}
}

float MechCargoMaxSpeed(MECH * mech, float mspeed)
{
	int lugged = 0, mod = 2;
	MECH *c;
	MAP *map;

	if(MechCarrying(mech) > 0) {	/* Ug-lee! */
		MECH *t;

		if((t = getMech(MechCarrying(mech))))
			if(!(MechCritStatus(t) & OWEIGHT_OK))
				MechCritStatus(mech) &= ~LOAD_OK;

	}

	/*! \todo {Fix this calculation to include gravity and TSM for
	 * when BT_MOVMENT_MODES is enabled} */
	if((MechCritStatus(mech) & LOAD_OK) &&
	   (MechCritStatus(mech) & OWEIGHT_OK) &&
	   (MechCritStatus(mech) & SPEED_OK)) {

		mspeed = MechRMaxSpeed(mech);
#ifndef BT_MOVEMENT_MODES

		/* Is masc and/or scharge on */
		if((MechStatus(mech) & MASC_ENABLED) &&
		   (MechStatus(mech) & SCHARGE_ENABLED))
			mspeed = ceil((rint(mspeed / 1.5) / MP1) * 2.5) * MP1;
		else if(MechStatus(mech) & MASC_ENABLED)
			mspeed *= 4. / 3.;
		else if(MechStatus(mech) & SCHARGE_ENABLED)
			mspeed *= 4. / 3.;

		if(InSpecial(mech) && InGravity(mech))
			if((map = FindObjectsData(mech->mapindex)))
				mspeed = mspeed * 100 / MAX(50, MapGravity(map));

#else
		/* Is masc and/or scharge and/or sprinting on */
		if(MechStatus(mech) & (MASC_ENABLED | SCHARGE_ENABLED) ||
		   MechStatus2(mech) & SPRINTING)
			mspeed = WalkingSpeed(mspeed) * (1.5 +
											 (MechStatus(mech) & MASC_ENABLED
											  ? 0.5 : 0.0) +
											 (MechStatus(mech) &
											  SCHARGE_ENABLED ? 0.5 : 0.0) +
											 (!MoveModeChange(mech)
											  && MechStatus2(mech) & SPRINTING
											  ? 0.5 : 0.0));

		/* if the player has speed demon give him his boost in speed */
		if(!MoveModeChange(mech) && MechStatus2(mech) & SPRINTING
		   && HasBoolAdvantage(MechPilot(mech), "speed_demon"))
			mspeed += MP1;
#endif
		return mspeed;
	}
	MechRTonsV(mech) = get_weight(mech);

	/*! \todo {Check some of this math better} */
	if(!(MechCritStatus(mech) & LOAD_OK)) {
		if(MechCarrying(mech) > 0)
			if((c = getMech(MechCarrying(mech)))) {
				lugged = get_weight(c) * 2;
				if(MechSpecials(mech) & SALVAGE_TECH)
					lugged = lugged / 2;
				if((MechSpecials(mech) & TRIPLE_MYOMER_TECH) &&
				   (MechHeat(mech) >= 9.))
					lugged = lugged / 2;

				if(MechSpecials2(mech) & CARRIER_TECH)
					lugged = lugged / 2;
			}

		if(MechSpecials(mech) & CARGO_TECH)
			mod = 1;

		if(MechType(mech) == CLASS_MECH)
			mod = mod * 2;

		lugged += MechCarriedCargo(mech) * mod / 2;
		MechRCTonsV(mech) = lugged;
		MechCritStatus(mech) |= LOAD_OK;
	}
	if(Destroyed(mech))
		mspeed = 0.0;
	else {
		int mv = MechRTonsV(mech);
		int sv = MechTons(mech) * 1024;

		if(mv == 1 && !Destroyed(mech))
			mv = sv;
		else {
			if(mv > sv)
				mv = mv + (mv - sv) / 2;
			else
				mv = mv + (sv - mv) / 3;
		}
		if(3 * sv < (MechRCTonsV(mech) + mv))
			mspeed = 0.0;
		else
#ifdef WEIGHT_OVERSPEEDING
			mspeed =
				MechMaxSpeed(mech) * MechTons(mech) * 1024.0 / MAX(1024 *
																   MechRealTons
																   (mech) +
																   MechRCTonsV
																   (mech) / 3,
																   (MAX
																	(1024,
																	 mv +
																	 MechRCTonsV
																	 (mech))));
#else
			mspeed =
				MechMaxSpeed(mech) * MechTons(mech) * 1024.0 / MAX(1024 *
																   MechTons
																   (mech) +
																   MechRCTonsV
																   (mech) / 3,
																   (MAX
																	(1024,
																	 mv +
																	 MechRCTonsV
																	 (mech))));
#endif /* WEIGHT_OVERSPEEDING */
	}
	MechRMaxSpeed(mech) = mspeed;
	MechWalkXPFactor(mech) = MAX(1, (int) mspeed / MP1) * 2;
	MechCritStatus(mech) |= SPEED_OK;
	return MMaxSpeed(mech);
}

void mech_drop(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	float s1;
	int wDropLevels = 0;
	int wDropBTH = 0;
	int tHasSwarmers = 0;

	cch(MECH_USUAL);
	DOCHECK(MechType(mech) == CLASS_BSUIT, "No crawling!");
	DOCHECK(MechType(mech) != CLASS_MECH &&
			MechType(mech) != CLASS_MW, "You can't prone in this!");
	DOCHECK(Fallen(mech), "You are already prone.");
	DOCHECK(Jumping(mech) || OODing(mech), "You can't prone in the air!");
	DOCHECK(Standing(mech), "You can't drop while trying to stand up!");

	s1 = MMaxSpeed(mech) / 3.0;

	if((MechType(mech) == CLASS_MECH) && CountSwarmers(mech))
		tHasSwarmers = 1;

	if(MechType(mech) != CLASS_MW && fabs(MechSpeed(mech)) > s1 * 2) {
		mech_notify(mech, MECHALL,
					"You attempt a controlled drop while running.");
		wDropLevels = 2;
		wDropBTH = 2;
	} else if(fabs(MechSpeed(mech)) > s1) {
		mech_notify(mech, MECHALL,
					"You attempt a controlled drop from your fast walk.");
		wDropLevels = 1;
	}

	if(Staggering(mech)) {
		mech_notify(mech, MECHALL,
					"Still staggering, you try not to fall on your face.");
		wDropLevels = (wDropLevels == 0 ? 1 : wDropLevels);
		wDropBTH = wDropBTH + StaggerLevel(mech);
	}

	if(tHasSwarmers)
		mech_notify(mech, MECHALL,
					"The suits hanging off you make a controlled drop harder!");

	if((wDropLevels > 0) || tHasSwarmers) {
		if(MadePilotSkillRoll(mech, wDropBTH)) {
			mech_notify(mech, MECHALL,
						"You hit the ground with minimal damage");
			MechLOSBroadcast(mech, "drops to the ground!");

			if(tHasSwarmers)
				StopBSuitSwarmers(FindObjectsData(mech->mapindex), mech, 0);

		} else {
			mech_notify(mech, MECHALL, "You fall to the ground hard");
			MechLOSBroadcast(mech, "falls hard to the ground!");

			if(wDropLevels <= 0)
				wDropLevels = 1;

			if(tHasSwarmers)
				StopBSuitSwarmers(FindObjectsData(mech->mapindex), mech, 0);

			MechFalls(mech, wDropLevels, 1);
		}
	} else {
		mech_notify(mech, MECHALL, "You drop to the ground prone!");
		MechLOSBroadcast(mech, "drops to the ground!");
	}

	MakeMechFall(mech);
	MechDesiredSpeed(mech) = 0;
	MechSpeed(mech) = 0;
	MechFloods(mech);
	water_extinguish_inferno(mech);

	possible_mine_poof(mech, MINE_STEP);
}

void mech_stand(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	char *args[2];
	int wcDeadLegs = 0;
	int tNeedsPSkill = 1;
	int tDoStand = 1;
	int bth, mechstandtime, standanyway = 0, standcarefulmod = 0;
	int i;

	cch(MECH_USUAL);
	DOCHECK(MechType(mech) == CLASS_BSUIT, "You're standing already!");
	DOCHECK(MechType(mech) != CLASS_MECH &&
			MechType(mech) != CLASS_MW,
			"This vehicle cannot stand like a 'Mech.");
	DOCHECK(Jumping(mech), "You're standing while jumping!");
	DOCHECK(OODing(mech), "You're standing while flying!");

	/* set the number of dead legs we have */
	wcDeadLegs = CountDestroyedLegs(mech);

	DOCHECK(((MechIsQuad(mech) && (wcDeadLegs > 3)) || (!MechIsQuad(mech)
														&& (wcDeadLegs > 1))),
			"You have no legs to stand on!");
	DOCHECK(wcDeadLegs > 2, "You'd be far too unstable!");
	DOCHECK(MechCritStatus(mech) & GYRO_DESTROYED,
			"You cannot stand with a destroyed gyro!");

	DOCHECK(!Fallen(mech), "You're already standing!");
	DOCHECK(Standrecovering(mech),
			"You're still recovering from your last attempt!");
	DOCHECK(IsHulldown(mech), "You can not stand while hulldown");
	DOCHECK(ChangingHulldown(mech),
			"You are busy changing your hulldown mode");

	bth = MechPilotSkillRoll_BTH(mech, 0);

	/* Check to see if the user specified an argument for the command */
	if(proper_explodearguments(buffer, args, 2)) {
		if(strcmp(args[0], "check") == 0) {
			notify_printf(player, "Your BTH to stand would be: %d", bth);
			for(i = 0; i < 2; i++) {
				if(args[i])
					free(args[i]);
			}
			return;
		} else if(strcmp(args[0], "anyway") == 0) {
			standanyway = 1;
		} else if(strcmp(args[0], "careful") == 0) {
			standcarefulmod = -2;
		} else {
			notify(player, "Unknown argument! use 'stand check', "
				   "'stand anyway', or 'stand careful'");
			for(i = 0; i < 2; i++) {
				if(args[i])
					free(args[i]);
			}
			return;
		}
	}

	DOCHECK(!standanyway && bth > 12,
			"You would fail; use 'stand anyway' if you really want to stand.");

	MakeMechStand(mech);

	/*  quads with all 4 legs don't have to roll to stand */
	if(((wcDeadLegs == 0) && MechIsQuad(mech)) ||
	   (MechType(mech) == CLASS_MW)) {
		tNeedsPSkill = 0;
	}

	MechLOSBroadcast(mech, "attempts to stand up.");

	if(MechRTerrain(mech) == ICE && MechZ(mech) == -1)
		break_thru_ice(mech);

	if(tNeedsPSkill) {
		if(!MadePilotSkillRoll(mech, standcarefulmod)) {
			mech_notify(mech, MECHALL,
						"You fail your attempt to stand and fall back on the ground");
			MechFalls(mech, 1, 1);
			mechstandtime = ((MechType(mech) == CLASS_MW) ?
							 DROP_TO_STAND_RECYCLE / 3 : StandMechTime(mech));
			/* Not strictly FASA, but allows legged mechs to stand careful */
			if(standcarefulmod) {
				mechstandtime = MAX(30, mechstandtime * 2);
			}
			MECHEVENT(mech, EVENT_STANDFAIL, mech_standfail_event,
					  mechstandtime, 0);
			tDoStand = 0;
		}
	}

	if(tDoStand) {
		/* Now we set a counter in goingy to keep him from moving or jumping until he is finished standing */
		mech_notify(mech, MECHALL, "You begin to stand up.");
		mechstandtime = ((MechType(mech) == CLASS_MW) ?
						 DROP_TO_STAND_RECYCLE / 3 : StandMechTime(mech));
		/* Not strictly FASA, but allows legged mechs to stand careful */
		if(standcarefulmod) {
			mechstandtime = mechstandtime * 2;
		}
		MECHEVENT(mech, EVENT_STAND, mech_stand_event, mechstandtime, 0);
	}
	/* Free args */
	for(i = 0; i < 2; i++) {
		if(args[i])
			free(args[i]);
	}

}

void mech_land(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;

	cch(MECH_USUAL);
	if(MechType(mech) != CLASS_MECH && MechType(mech) != CLASS_MW &&
	   MechType(mech) != CLASS_BSUIT && MechType(mech) != CLASS_VEH_GROUND) {
		aero_land(player, data, buffer);
		return;
	}
	if(Jumping(mech)) {
		mech_notify(mech, MECHALL,
					"You abort your full jump and attempt to land early");
		if(MadePilotSkillRoll(mech, 0)) {
			mech_notify(mech, MECHALL, "You are able to abort the jump.");

/*        MechLOSBroadcast (mech, "lands abruptly!"); */
			LandMech(mech);
		} else {
			mech_notify(mech, MECHALL, "You don't quite make it.");
			MechLOSBroadcast(mech,
							 "attempts a landing, but crashes to the ground!");
			MechFalls(mech, 1, 0);
			MechDFATarget(mech) = -1;
			MechGoingX(mech) = MechGoingY(mech) = 0;
			MechSpeed(mech) = 0;
			MaybeMove(mech);

		}
	} else
		notify(player, "You're not jumping!");
}

/* Facing related */
void mech_heading(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	char *args[1];
	int newheading;

	cch(MECH_USUAL);
	if(mech_parseattributes(buffer, args, 1) == 1) {
		DOCHECK(MechMove(mech) == MOVE_NONE,
				"This piece of equipment is stationary!");
		DOCHECK(WaterBeast(mech) &&
				NotInWater(mech),
				"You are regrettably unable to move at this time. We apologize for the inconvenience.");
		DOCHECK(is_aero(mech) && Spinning(mech) &&
				!Landed(mech),
				"You are unable to control your craft at the moment.");
		DOCHECK(PerformingAction(mech),
				"You are too busy at the moment to turn.");
		DOCHECK(MechDugIn(mech),
				"You are in a hole you dug, unable to move [use speed cmd to get out].");
		DOCHECK(IsHulldown(mech), "You can not turn while hulldown");
		DOCHECK(ChangingHulldown(mech),
				"You are busy changing your hulldown mode");
		if(Digging(mech)) {
			mech_notify(mech, MECHALL,
						"You cease your attempts at digging in.");
			StopDigging(mech);
		}
		newheading = AcceptableDegree(atoi(args[0]));
		MechDesiredFacing(mech) = newheading;
		mech_printf(mech, MECHALL, "Heading changed to %d.", newheading);
		MaybeMove(mech);
	} else {
		notify_printf(player, "Your current heading is %i.",
					  MechFacing(mech));
	}
}

void mech_turret(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	char *args[1];
	int newheading;

	cch(MECH_USUALO);
	DOCHECK(MechType(mech) == CLASS_MECH || MechType(mech) == CLASS_MW ||
			MechType(mech) == CLASS_BSUIT || is_aero(mech) ||
			!GetSectInt(mech, TURRET), "You don't have a turret.");
	DOCHECK(MechTankCritStatus(mech) & TURRET_JAMMED,
			"Your turret is jammed in position.");
	DOCHECK(MechTankCritStatus(mech) & TURRET_LOCKED,
			"Your turret is locked in position.");
	if(mech_parseattributes(buffer, args, 1) == 1) {
		newheading = AcceptableDegree(atoi(args[0]) - MechFacing(mech));
		MechTurretFacing(mech) = newheading;
		mech_printf(mech, MECHALL, "Turret facing changed to %d.",
					AcceptableDegree(MechTurretFacing(mech) +
									 MechFacing(mech)));
	} else {
		notify_printf(player, "Your turret is currently facing %d.",
					  AcceptableDegree(MechTurretFacing(mech) +
									   MechFacing(mech)));
	}

	MarkForLOSUpdate(mech);
}

void mech_rotatetorso(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	char *args[2];

	cch(MECH_USUALO);
	DOCHECK(MechType(mech) == CLASS_BSUIT, "Huh?");
	DOCHECK(MechType(mech) != CLASS_MECH &&
			MechType(mech) != CLASS_MW, "You don't have a torso.");
	DOCHECK(Fallen(mech),
			"You're lying flat on your face, you can't rotate your torso.");
	DOCHECK((MechType(mech) == CLASS_MECH) &&
			(MechIsQuad(mech)), "Quads can't rotate their torsos.");
	if(mech_parseattributes(buffer, args, 2) == 1) {
		switch (args[0][0]) {
		case 'L':
		case 'l':
			DOCHECK(MechStatus(mech) & TORSO_LEFT,
					"You cannot rotate torso beyond 60 degrees!");
			if(MechStatus(mech) & TORSO_RIGHT)
				MechStatus(mech) &= ~TORSO_RIGHT;
			else
				MechStatus(mech) |= TORSO_LEFT;
			mech_notify(mech, MECHALL, "You rotate your torso left.");
			break;
		case 'R':
		case 'r':
			DOCHECK(MechStatus(mech) & TORSO_RIGHT,
					"You cannot rotate torso beyond 60 degrees!");
			if(MechStatus(mech) & TORSO_LEFT)
				MechStatus(mech) &= ~TORSO_LEFT;
			else
				MechStatus(mech) |= TORSO_RIGHT;
			mech_notify(mech, MECHALL, "You rotate your torso right.");
			break;
		case 'C':
		case 'c':
			MechStatus(mech) &= ~(TORSO_RIGHT | TORSO_LEFT);
			mech_notify(mech, MECHALL, "You center your torso.");
			break;
		default:
			notify(player, "Rotate must have LEFT RIGHT or CENTER.");
			break;
		}
	} else
		notify(player, "Invalid number of arguments!");
	MarkForLOSUpdate(mech);
}

struct {
	char *name;
	int flag;
} speed_tables[] = {
	{
	"walk", 1}, {
	"run", 2}, {
	"stop", 0}, {
	"back", -1}, {
	"cruise", 1}, {
	"flank", 2}, {
	NULL, 0.0}
};

void mech_speed(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	char *args[1];
	float newspeed, walkspeed, maxspeed;
	int i;

	cch(MECH_USUAL);
	if(RollingT(mech)) {
		DOCHECK(!Landed(mech), "Use thrust command instead!");
	} else if(FlyingT(mech)) {
		DOCHECK(MechType(mech) != CLASS_VTOL, "Use thrust command instead!");
	}
	DOCHECK(MechMove(mech) == MOVE_NONE,
			"This piece of equipment is stationary!");
	DOCHECK(PerformingAction(mech),
			"You are too busy at the moment to turn.");
	DOCHECK(Standing(mech), "You are currently standing up and cannot move.");
	DOCHECK((Fallen(mech)) && (MechType(mech) != CLASS_MECH &&
							   MechType(mech) != CLASS_MW),
			"Your vehicle's movement system is destroyed.");
	DOCHECK(Fallen(mech), "You are currently prone and cannot move.");
	DOCHECK(WaterBeast(mech) &&
			NotInWater(mech),
			"You are regrettably unable to move at this time. We apologize for the inconvenience.");

	if(MechType(mech) != CLASS_MECH)
		DOCHECK(RemovingPods(mech), "You are too busy removing iNARC pods!");
	DOCHECK(IsHulldown(mech), "You can not move while hulldown");
	DOCHECK(ChangingHulldown(mech),
			"You are busy changing your hulldown mode");

	if(mech_parseattributes(buffer, args, 1) != 1) {
		notify_printf(player, "Your current speed is %.2f.", MechSpeed(mech));
		return;
	}
	DOCHECK(FlyingT(mech) && AeroFuel(mech) <= 0 &&
			!AeroFreeFuel(mech), "You're out of fuel!");
	maxspeed = MMaxSpeed(mech);

	if(MechMove(mech) == MOVE_VTOL)
		maxspeed = sqrt((float) maxspeed * maxspeed -
						MechVerticalSpeed(mech) * MechVerticalSpeed(mech));

	maxspeed = maxspeed > 0.0 ? maxspeed : 0.0;

	if((MechHeat(mech) >= 9.) && (MechSpecials(mech) & TRIPLE_MYOMER_TECH))
		maxspeed = ceil((rint((maxspeed / 1.5) / MP1) + 1) * 1.5) * MP1;

	/*   if (MechStatus(mech) & MASC_ENABLED) maxspeed = (4. / 3. ) * maxspeed; */
	walkspeed = WalkingSpeed(maxspeed);
	newspeed = atof(args[0]);

	if(newspeed < 0.1) {

		/* Possibly a string speed instead? */
		for(i = 0; speed_tables[i].name; i++)
			if(!strcasecmp(speed_tables[i].name, args[0])) {
				switch (speed_tables[i].flag) {
				case 0:
					newspeed = 0.0;
					break;
				case -1:
					newspeed = -walkspeed;
					break;
				case 1:
					newspeed = walkspeed;
					break;
				case 2:
					newspeed = maxspeed;
					break;
				}
				break;
			}
	}

	if(newspeed > maxspeed)
		newspeed = maxspeed;
	if(newspeed < -walkspeed)
		newspeed = -walkspeed;

	DOCHECK((newspeed < 0) && (MechCarrying(mech) > 0) &&
			(!(MechSpecials(mech) & SALVAGE_TECH)),
			"You can not backup while towing!");

	DOCHECK((newspeed < 0) && Sprinting(mech),
			"You can not backup while sprinting!");

	if(IsRunning(newspeed, maxspeed)) {
		DOCHECK(Dumping(mech), "You can not run while dumping ammo!");
		DOCHECK(UnJammingAmmo(mech),
				"You can not run while unjamming your weapon!");

		/* Exile Stun Code Effect */
		if(MechCritStatus(mech) & MECH_STUNNED) {
			mech_notify(mech, MECHALL, "You cannot move faster than cruise"
						" speed while stunned!");
			return;
		}

		DOCHECK(CrewStunned(mech),
				"Your cannot possibly control a vehicle going this fast in your "
				"current mental state!");
		DOCHECK(MechTankCritStatus(mech) & TAIL_ROTOR_DESTROYED,
				"Your cannot possibly control a VTOL going this fast with a destroyed tail rotor!");
		DOCHECK(MechType(mech) == CLASS_MECH && ((MechZ(mech) < 0 &&
												  (MechRTerrain(mech) == WATER
												   || MechRTerrain(mech) ==
												   BRIDGE
												   || MechRTerrain(mech) ==
												   ICE))
												 || MechRTerrain(mech) ==
												 HIGHWATER),
				"You can't run through water!");
	}
	if(!Wizard(player) && In_Character(mech->mynum) &&
	   MechPilot(mech) != player) {
		if(newspeed < 0.0) {
			notify(player,
				   "Not being the Pilot of this beast, you cannot move it backwards.");
			return;
		} else if(newspeed > walkspeed) {
			notify(player,
				   "Not being the Pilot of this beast, you cannot go faster than walking speed.");
			return;
		}
	}
	MechDesiredSpeed(mech) = newspeed;
	MaybeMove(mech);
	if(fabs(newspeed) > 0.1) {
		if(MechSwarmTarget(mech) > 0) {
			StopSwarming(mech, 1);
			MechCritStatus(mech) &= ~HIDDEN;
		}
		if(Digging(mech)) {
			mech_notify(mech, MECHALL,
						"You cease your attempts at digging in.");
			StopDigging(mech);
		}
		MechTankCritStatus(mech) &= ~DUG_IN;
	}
	mech_printf(mech, MECHALL, "Desired speed changed to %d KPH",
				(int) newspeed);
}

void mech_vertical(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	char *args[1], buff[50];
	float newspeed, maxspeed;

	cch(MECH_USUAL);
	DOCHECK(MechType(mech) != CLASS_VTOL &&
			MechMove(mech) != MOVE_SUB, "This command is for VTOLs only.");
	DOCHECK(MechType(mech) == CLASS_VTOL &&
			AeroFuel(mech) <= 0
			&& !AeroFreeFuel(mech), "You're out of fuel!");
	DOCHECK(WaterBeast(mech)
			&& NotInWater(mech),
			"You are regrettably unable to move at this time. We apologize for the inconvenience.");
	DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
			tprintf("Current vertical speed is %.2f KPH.",
					(float) MechVerticalSpeed(mech)));
	newspeed = atof(args[0]);
	maxspeed = MMaxSpeed(mech);
	maxspeed =
		sqrt((float) maxspeed * maxspeed -
			 MechDesiredSpeed(mech) * MechDesiredSpeed(mech));
	if((newspeed > maxspeed) || (newspeed < -maxspeed)) {
		sprintf(buff, "Max vertical speed is + %d KPH and - %d KPH",
				(int) maxspeed, (int) maxspeed);
		notify(player, buff);
	} else {
		DOCHECK(Fallen(mech), "Your vehicle's movement system is destroyed.");
		DOCHECK(MechType(mech) == CLASS_VTOL &&
				Landed(mech), "You need to take off first.");
		MechVerticalSpeed(mech) = newspeed;
		mech_printf(mech, MECHALL,
					"Vertical speed changed to %d KPH", (int) newspeed);
		MaybeMove(mech);
	}
}

/*
 * - Only when fallen
 * - Tonnage / 3 (rounded up for .5)
 * - 5 Point groups to PA
 * - Clear or paved terrain only
 * - Automatically works
 * - Doesn't hit suits that are swarmed or jumping
 * - No weapons recycling in arms and legs
 * - Arms and legs recycle after attack
 * - Make pskill roll or take damage as if 1 level fall
 */

void mech_thrash(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MECH *target;
	MAP *map = getMap(mech->mapindex);
	int terrain;
	int limbs = 4;
	int aLimbs[] = { RARM, LARM, LLEG, RLEG };
	int i;
	int tempLoc;
	char locName[50];
	int damage, tempDamage;

	cch(MECH_USUALO);
	DOCHECK(!Fallen(mech), "You need to be prone to thrash!");
	DOCHECK(!map, "Invalid map! Contact a wizard!");

	terrain = GetRTerrain(map, MechX(mech), MechY(mech));

	DOCHECK(!((terrain == GRASSLAND) || (terrain == ROAD) ||
			 (terrain == BRIDGE)),
			"Thrashing only works in clear terrain or on roads or bridges.");

	/* Check locations */
	for(i = 0; i < 4; i++) {
		tempLoc = aLimbs[i];

		if(SectIsDestroyed(mech, tempLoc)) {
			limbs--;
			continue;
		}

		ArmorStringFromIndex(tempLoc, locName, MechType(mech),
							 MechMove(mech));

		DOCHECK(SectHasBusyWeap(mech, tempLoc),
				tprintf("You have weapons recycling on your %s.", locName));
		DOCHECK(MechSections(mech)[tempLoc].recycle,
				tprintf("Your %s is still recovering from your last attack.",
						locName));
	}

	/* Can't thrash if we have no limbs */
	if(!limbs) {
		mech_notify(mech, MECHALL, "You can't thrash if you have no limbs!");
		return;
	}
#ifndef REALWEIGHT_DAMAGE
	damage = MechTons(mech) / 3;
#else
	damage = MechRealTons(mech) / 3;
#endif /* REALWEIGHT_DAMAGE */
	damage = (damage * limbs) / 4;

	mech_notify(mech, MECHALL,
				"You start to flail your arms and legs like a wild man!");
	MechLOSBroadcast(mech,
					 "starts to flail its arms and legs like a wild beast!");

	/* Let's see who we can smack around */
	for(i = 0; i < map->first_free; i++) {
		if(map->mechsOnMap[i] >= 0) {
			target = (MECH *) FindObjectsData(map->mechsOnMap[i]);

			if(!target)
				continue;

			if(MechType(target) != CLASS_BSUIT)
				continue;

			if(MechTeam(target) == MechTeam(mech))
				continue;

			if(Jumping(target) || OODing(target))
				continue;

			if(FaMechRange(mech, target) > 1.0)
				continue;

			mech_printf(mech, MECHALL, "You manage to hit %s!",
						GetMechToMechID(mech, target));
			mech_printf(target, MECHALL,
						"You get hit by %s's thrashing limbs!",
						GetMechToMechID(target, mech));

			tempDamage = damage;

			while (tempDamage > 0) {
				if(tempDamage > 5) {
					DamageMech(target, mech, 1, MechPilot(mech), Number(0,
																		NUM_BSUIT_MEMBERS
																		- 1),
							   0, 0, 5, 0, -1, 0, -1, 0, 1);
					tempDamage -= 5;
				} else {
					DamageMech(target, mech, 1, MechPilot(mech), Number(0,
																		NUM_BSUIT_MEMBERS
																		- 1),
							   0, 0, tempDamage, 0, -1, 0, -1, 0, 1);
					tempDamage = 0;
				}
			}
		}
	}

	/* Make our roll and recycle our limbs -- Removed. You gotta be prone anyways! */
/*	if(!MadePilotSkillRoll_Advanced(mech, 0, 0)) {
		MechFalls(mech, 1, 1);
	}
*/

	for(i = 0; i < 4; i++) {
		tempLoc = aLimbs[i];

		if(SectIsDestroyed(mech, tempLoc))
			continue;

		SetRecycleLimb(mech, tempLoc, PHYSICAL_RECYCLE_TIME);
	}
}

void mech_jump(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MECH *tempMech = NULL;
	MAP *mech_map;
	char *args[3];
	int argc;
	int target;
	char targetID[2];
	short mapx, mapy;
	int bearing;
	float range = 0.0;
	float realx, realy;
	int sz, tz, jps;

	mech_map = getMap(mech->mapindex);
	cch(MECH_USUALO);
#ifdef BT_MOVEMENT_MODES
	DOCHECK(MoveModeLock(mech), "Movement modes disallow jumping.");
#endif
	DOCHECK(MechType(mech) != CLASS_MECH && MechType(mech) != CLASS_MW &&
			MechType(mech) != CLASS_BSUIT &&
			MechType(mech) != CLASS_VEH_GROUND, "This unit cannot jump.");
	DOCHECK(MechCarrying(mech) > 0, "You can't jump while towing someone!");
	DOCHECK((MechMaxSpeed(mech) - MMaxSpeed(mech)) > MP1,
			"No, with this cargo you won't!");
	DOCHECK(Fallen(mech), "You can't Jump from a FALLEN position");
	DOCHECK(IsHulldown(mech), "You can't Jump while hulldown");
	DOCHECK(ChangingHulldown(mech),
			"You are busy changing your hulldown mode");
	DOCHECK(Jumping(mech), "You're already jumping!");
	DOCHECK(Stabilizing(mech),
			"You haven't stabilized from your last jump yet.");
	DOCHECK(Standing(mech), "You haven't finished standing up yet.");
	DOCHECK(fabs(MechJumpSpeed(mech)) <= 0.0,
			"This mech doesn't have jump jets!");
	argc = mech_parseattributes(buffer, args, 3);
	DOCHECK(Dumping(mech), "You can not jump while dumping ammo!");
	DOCHECK(UnJammingAmmo(mech),
			"You can not jump while unjamming your weapon!");
	DOCHECK(RemovingPods(mech), "You are too busy removing iNARC pods!");
	DOCHECK(MapIsUnderground(mech_map),
			"Realize the ceiling in this grotto is a bit to low for that!");
	DOCHECK(OODing(mech), "You can't jump while orbital dropping!");

	if(Staggering(mech)) {
		mech_notify(mech, MECHALL,
					"The damage inhibits your coordination...");

		if(!MadePilotSkillRoll(mech, calcStaggerBTHMod(mech))) {
			mech_notify(mech, MECHALL,
						"... something you apparently can't handle!");
			MechLOSBroadcast(mech,
							 "engages jumpjets, rolls to the side and slams into the ground!");
			MechFalls(mech, 1, 0);
			return;
		}
	}

	if(doJettisonChecks(mech))
		return;

	DOCHECK(argc > 2, "Too many arguments to JUMP function!");
	MechStatus(mech) &= ~DFA_ATTACK;	/* By default no DFA */
	switch (argc) {
	case 0:
		/* DFA current target... */

		DOCHECK(MechType(mech) != CLASS_MECH,
				"Only mechs can do Death From Above attacks!");

		target = MechTarget(mech);
		tempMech = getMech(target);
		DOCHECK(!tempMech, "Invalid Target!");
		range = FaMechRange(mech, tempMech);
		DOCHECK(!InLineOfSight(mech, tempMech, MechX(tempMech),
							   MechY(tempMech), range),
				"Target is not in line of sight!");
		DOCHECK(MechType(tempMech) == CLASS_MW,
				"Even you can't aim your jump well enough to squish that!");
		mapx = MechX(tempMech);
		mapy = MechY(tempMech);
		MechDFATarget(mech) = MechTarget(mech);
		break;
	case 1:
		/* Jump Target */
		DOCHECK(MechType(mech) != CLASS_MECH,
				"Only mechs can do Death From Above attacks!");

		targetID[0] = args[0][0];
		targetID[1] = args[0][1];
		target = FindTargetDBREFFromMapNumber(mech, targetID);
		tempMech = getMech(target);
		DOCHECK(!tempMech, "Target is not in line of sight!");
		range = FaMechRange(mech, tempMech);
		DOCHECK(!InLineOfSight(mech, tempMech, MechX(tempMech),
							   MechY(tempMech), range),
				"Target is not in line of sight!");
		DOCHECK(MechType(tempMech) == CLASS_MW,
				"Even you can't aim your jump well enough to squish that!");
		mapx = MechX(tempMech);
		mapy = MechY(tempMech);
		MechDFATarget(mech) = tempMech->mynum;
		break;
	case 2:
		bearing = atoi(args[0]);
		range = atof(args[1]);
		FindXY(MechFX(mech), MechFY(mech), bearing, range, &realx, &realy);

		/* This is so we are jumping to the center of a hex */
		/* and the bearing jives with the target hex */
		RealCoordToMapCoord(&mapx, &mapy, realx, realy);
		break;
	}
	DOCHECK(mapx >= mech_map->map_width || mapy >= mech_map->map_height ||
			mapx < 0 || mapy < 0, "That would take you off the map!");
	DOCHECK(MechX(mech) == mapx &&
			MechY(mech) == mapy, "You're already in the target hex.");
	sz = MechZ(mech);
	tz = Elevation(mech_map, mapx, mapy);
	jps = JumpSpeedMP(mech, mech_map);
	DOCHECK(range > jps, "That target is out of range!");
	if(MechType(mech) != CLASS_BSUIT && tempMech)
		MechStatus(mech) |= DFA_ATTACK;
	/*   MechJumpTop(mech) = BOUNDED(3, (jps - range) + 2, jps - 1); */
	/* New idea: JumpTop = (JP + 1 - range / 3) - in another words,
	   SDR jumping for 1 hexes has 8 + 1 = 9 hex elevation in mid-flight,
	   SDR jumping for 8 hexes has 8 + 1 - 2 = 7 hex elevation in mid-flight,
	   TDR jumping for 4 hexes has 4 + 1 - 1 = 4 hex elevation in mid-flight

	   Come to think of it, the last SDR figure was ridiculous. New
	   value: 2 * 1 + 2 = 4
	 */
	MechJumpTop(mech) = MIN(jps + 1 - range / 3, 2 * range + 2);
	DOCHECK((tz - sz) > jps,
			"That target's high for you to reach with a single jump!");
	DOCHECK((sz - tz) > jps,
			"That target's low for you to reach with a single jump!");
	DOCHECK(sz < -1, "Glub glub glub.");
	MapCoordToRealCoord(mapx, mapy, &realx, &realy);
	bearing = FindBearing(MechFX(mech), MechFY(mech), realx, realy);

	/* TAKE OFF! */
	MechCocoon(mech) = 0;
	MechJumpHeading(mech) = bearing;
	MechStatus(mech) |= JUMPING;
	MechStartFX(mech) = MechFX(mech);
	MechStartFY(mech) = MechFY(mech);
	MechStartFZ(mech) = MechFZ(mech);
	MechJumpLength(mech) =
		length_hypotenuse((double) (realx - MechStartFX(mech)),
						  (double) (realy - MechStartFY(mech)));
	MechGoingX(mech) = mapx;
	MechGoingY(mech) = mapy;
	MechEndFZ(mech) = ZSCALE * tz;
	MechSpeed(mech) = 0.0;
	if(MechStatus(mech) & DFA_ATTACK)
		mech_notify(mech, MECHALL,
					"You engage your jump jets for a Death From Above attack!");
	else
		mech_notify(mech, MECHALL, "You engage your jump jets.");
	MechSwarmTarget(mech) = -1;
	MechLOSBroadcast(mech, "engages jumpjets!");
	MECHEVENT(mech, EVENT_JUMP, mech_jump_event, JUMP_TICK, 0);
}

static void mech_hulldown_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;
	int type = (int) e->data2;

	if(!ChangingHulldown(mech))
		return;

	if(!Started(mech))
		return;

	if(type == 0) {
		MechStatus(mech) &= ~HULLDOWN;
		mech_notify(mech, MECHALL, "You finish lifting yourself up.");
		MechLOSBroadcast(mech, "finishes lifting itself up");
	} else {
		MechStatus(mech) |= HULLDOWN;
		mech_notify(mech, MECHALL,
					"You finish lowering yourself to the ground.");
		MechLOSBroadcast(mech, "finishes lowering itself to the ground.");
	}
}

#ifdef BT_MOVEMENT_MODES
void mech_sprint(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	int d = 0, i;

	cch(MECH_USUALO);
	DOCHECK(MechMove(mech) == MOVE_NONE,
			"This piece of equipment is stationary!");
	DOCHECK(MechCarrying(mech) > 0, "You cannot sprint while towing!");
	DOCHECK(Standing(mech), "You are currently standing up and cannot move.");
	DOCHECK(Jumping(mech), "You cannot do this while jumping.");
	DOCHECK((Fallen(mech)) && (MechType(mech) != CLASS_MECH &&
							   MechType(mech) != CLASS_MW),
			"Your vehicle's movement system is destroyed.");
	DOCHECK(Fallen(mech), "You are currently prone and cannot move.");
	DOCHECK(WaterBeast(mech) &&
			NotInWater(mech),
			"You are regrettably unable to move at this time. We apologize for the inconvenience.");
	DOCHECK(MoveModeChange(mech), "You are already changing movement modes!");
	DOCHECK(MechStatus2(mech) & (EVADING | DODGING),
			"You cannot perform multiple movement modes!");
	DOCHECK(MechSwarmTarget(mech) > 0, "You cannot sprint while mounted!");
	if(MechType(mech) == CLASS_MECH)
		DOCHECK(SectIsDestroyed(mech, RLEG) || SectIsDestroyed(mech, LLEG)
				|| (MechMove(mech) !=
					MOVE_QUAD ? 0 : SectIsDestroyed(mech, RLEG)
					|| SectIsDestroyed(mech, LLEG)),
				"That's kind of hard while limping.");

	d |= MODE_SPRINT | ((MechStatus2(mech) & SPRINTING) ? MODE_OFF : MODE_ON);
	if(d & MODE_ON) {
		if((i = MechFullNoRecycle(mech, CHECK_BOTH)) > 0) {
			mech_printf(mech, MECHALL, "You have %s recycling!",
						(i == 1 ? "weapons" : i == 2 ? "limbs" : "error"));
			return;
		}
		mech_notify(mech, MECHALL, "You begin the process of sprinting.....");
		MECHEVENT(mech, EVENT_MOVEMODE, mech_movemode_event,
				  (MechType(mech) == CLASS_BSUIT
				   || MechType(mech) == CLASS_MW) ? TURN / 2 : TURN, d);
	} else {
		mech_notify(mech, MECHALL,
					"You begin the process of ceasing to sprint.");
		MECHEVENT(mech, EVENT_MOVEMODE, mech_movemode_event,
				  (MechType(mech) == CLASS_BSUIT
				   || MechType(mech) == CLASS_MW) ? TURN / 2 : TURN, d);
	}
	return;
}

void mech_evade(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	int d = 0, i;

	cch(MECH_USUALO);
	DOCHECK(MechMove(mech) == MOVE_NONE,
			"This piece of equipment is stationary!");
	DOCHECK(Standing(mech), "You are currently standing up and cannot move.");
	DOCHECK(Jumping(mech), "You cannot do this while jumping.");
	DOCHECK((Fallen(mech))
			&& (MechType(mech) != CLASS_MECH
				&& MechType(mech) != CLASS_MW),
			"Your vehicle's movement system is destroyed.");
	DOCHECK(MechCarrying(mech) > 0, "You can't do that while towing");
	DOCHECK(Fallen(mech), "You are currently prone and cannot move.");
	DOCHECK(!(MechStatus2(mech) & EVADING) && MechType(mech) == CLASS_MECH
			&& (PartIsNonfunctional(mech, LLEG, 0)
				|| PartIsNonfunctional(mech, RLEG, 0)),
			"You need both hip functional to evade.");
	DOCHECK(WaterBeast(mech)
			&& NotInWater(mech),
			"You are regrettably unable to move at this time. We apologize for the inconvenience.");
	DOCHECK(MoveModeChange(mech), "You are already changing movement modes!");
	DOCHECK(MechStatus2(mech) & (SPRINTING | DODGING),
			"You cannot perform multiple movement modes!");
	DOCHECK(MechSwarmTarget(mech) > 0, "You cannot evade while mounted!");
	if(MechType(mech) == CLASS_MECH)
		DOCHECK(SectIsDestroyed(mech, RLEG) || SectIsDestroyed(mech, LLEG)
				|| (MechMove(mech) !=
					MOVE_QUAD ? 0 : SectIsDestroyed(mech, RLEG)
					|| SectIsDestroyed(mech, LLEG)),
				"That's kind of hard while limping.");

	d |= MODE_EVADE | ((MechStatus2(mech) & EVADING) ? MODE_OFF : MODE_ON);
	if(d & MODE_ON) {
		if((i = MechFullNoRecycle(mech, CHECK_BOTH)) > 0) {
			mech_printf(mech, MECHALL, "You have %s recycling!",
						(i == 1 ? "weapons" : i == 2 ? "limbs" : "error"));
			return;
		}
		mech_notify(mech, MECHALL, "You begin the process of evading.....");
		MECHEVENT(mech, EVENT_MOVEMODE, mech_movemode_event,
				  (MechType(mech) == CLASS_BSUIT
				   || MechType(mech) == CLASS_MW) ? TURN / 2 : TURN, d);
	} else {
		mech_notify(mech, MECHALL,
					"You begin the process of ceasing to evade.");
		MECHEVENT(mech, EVENT_MOVEMODE, mech_movemode_event,
				  (MechType(mech) == CLASS_BSUIT
				   || MechType(mech) == CLASS_MW) ? TURN / 2 : TURN, d);
	}
	return;
}

void mech_dodge(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	int d = 0, i;

	cch(MECH_USUALO);
	DOCHECK(MechMove(mech) == MOVE_NONE,
			"This piece of equipment is stationary!");
	DOCHECK(Standing(mech), "You are currently standing up and cannot move.");
	DOCHECK((Fallen(mech)) && (MechType(mech) != CLASS_MECH &&
							   MechType(mech) != CLASS_MW),
			"Your vehicle's movement system is destroyed.");
	DOCHECK(Fallen(mech), "You are currently prone and cannot move.");
	DOCHECK(WaterBeast(mech) &&
			NotInWater(mech),
			"You are regrettably unable to move at this time. We apologize for the inconvenience.");
	DOCHECK(MoveModeChange(mech), "You are already changing movement modes!");
	DOCHECK(MechStatus2(mech) & (SPRINTING | EVADING),
			"You cannot perform multiple movement modes!");
	DOCHECK(!(HasBoolAdvantage(player, "dodge_maneuver"))
			|| player != MechPilot(mech),
			"You either are not the pilot of this mech, have no Dodge Maneuver adavantage, or both.");
	d |= MODE_DODGE | ((MechStatus2(mech) & DODGING) ? MODE_OFF : MODE_ON);
	if(d & MODE_ON) {
		if((i = MechFullNoRecycle(mech, CHECK_PHYS)) > 0) {
			mech_printf(mech, MECHALL, "You have %s recycling!",
						(i == 1 ? "weapons" : i == 2 ? "limbs" : "error"));
			return;
		}
		mech_notify(mech, MECHALL, "You begin the process of dodging.....");
		MECHEVENT(mech, EVENT_MOVEMODE, mech_movemode_event, 1, d);
	} else {
		mech_notify(mech, MECHALL,
					"You begin the process of ceasing to dodge.");
		MECHEVENT(mech, EVENT_MOVEMODE, mech_movemode_event, TURN, d);
	}
	return;
}
#endif

void mech_hulldown(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	char *args[1];
	int argc;

	cch(MECH_USUALO);

	DOCHECK(!MechIsQuad(mech), "Only QUADs can hulldown.");
	DOCHECK(Fallen(mech), "You can't hulldown from a FALLEN position");
	DOCHECK(Jumping(mech), "You can't hulldown while jumping!");
	DOCHECK(MechSpeed(mech) > 0.5, "You can't hulldown while moving!");
	DOCHECK(Stabilizing(mech),
			"You are still stabilizing from your last jump.");
	DOCHECK(Standing(mech), "You haven't finished standing up yet.");

	argc = mech_parseattributes(buffer, args, 1);

	if(argc > 0) {
		if(!strcmp(args[0], "-")) {
			if(!IsHulldown(mech))
				mech_notify(mech, MECHALL, "You are not hulldown.");
			else if(ChangingHulldown(mech))
				mech_notify(mech, MECHALL,
							"You are busy changing your hulldown mode.");
			else {
				mech_notify(mech, MECHALL, "You start to lift yourself up.");
				MechLOSBroadcast(mech, "begins to raise up on its legs.");

				MECHEVENT(mech, EVENT_CHANGING_HULLDOWN,
						  mech_hulldown_event, StandMechTime(mech), 0);
			}
		} else if(!strcasecmp(args[0], "stop")) {
			if(!ChangingHulldown(mech))
				mech_notify(mech, MECHALL,
							"You are not currently changing your hulldown mode.");
			else {
				StopHullDown(mech);
				mech_notify(mech, MECHALL,
							"You stop changing your hulldown mode.");
			}
		} else
			mech_notify(mech, MECHALL, "Invalid argument for 'hulldown'.");

		return;
	}

	DOCHECK(IsHulldown(mech), "You are already hulldown.");
	DOCHECK(ChangingHulldown(mech),
			"You are busy changing your hulldown mode.");

	mech_notify(mech, MECHALL, "You start to lower yourself to the ground.");
	MechLOSBroadcast(mech, "begins to lower itself to the ground.");
	MechDesiredSpeed(mech) = 0;

	MECHEVENT(mech, EVENT_CHANGING_HULLDOWN, mech_hulldown_event,
			  StandMechTime(mech), 1);
}

int DropGetElevation(MECH * mech)
{
	if(MechRTerrain(mech) == BRIDGE) {
		if(MechZ(mech) < (MechElev(mech))) {
			if(Overwater(mech))
				return 0;
			return bridge_w_elevation(mech);
		}
		return MechElevation(mech);
	}
	if(Overwater(mech) || (MechRTerrain(mech) == ICE && MechZ(mech) >= 0))
		return MAX(0, MechElevation(mech));
	else
		return MechElevation(mech);
}

void DropSetElevation(MECH * mech, int wantdrop)
{
	if(MechRTerrain(mech) == BRIDGE) {
		bridge_set_elevation(mech);
		return;
	}
	MechZ(mech) = DropGetElevation(mech);
	MechFZ(mech) = MechZ(mech) * ZSCALE;
	if(wantdrop)
		if(MechRTerrain(mech) == ICE && MechZ(mech) >= 0)
			possibly_drop_thru_ice(mech);
}

void LandMech(MECH * mech)
{
	MECH *target;
	MAP *mech_map = getMap(mech->mapindex);
	int dfa = 0;
	int done = 0;

	/*
	 * Added check to see if we're actually awake when we try to land
	 * - Kipsta
	 * - 8/3/99
	 */

	if(Uncon(mech)) {
		mech_notify(mech, MECHALL,
					"Your lack of conciousness makes you fall to the ground. Not like you can read this anyway.");
		MechFalls(mech, 1, 0);
		dfa = 1;
		done = 1;
	} else {
		/* Handle DFA attack */
		if(MechStatus(mech) & DFA_ATTACK) {
			/* is the target here? */
			target = getMech(MechDFATarget(mech));
			if(target) {
				if(MechX(target) == MechX(mech) &&
				   MechY(target) == MechY(mech))
					dfa = DeathFromAbove(mech, target);
				else
					mech_notify(mech, MECHPILOT,
								"Your DFA target has moved!");
			} else
				mech_notify(mech, MECHPILOT,
							"Your target has become invalid.");
		}

		if(!dfa)
			mech_notify(mech, MECHALL, "You finish your jump.");

		/* Better reset the FZ */
		MechElev(mech) = GetElev(mech_map, MechX(mech), MechY(mech));
		MechZ(mech) = MechElev(mech) - 1;
		MechFZ(mech) = ZSCALE * MechZ(mech);
		DropSetElevation(mech, 1);

		if(Staggering(mech)) {
			mech_notify(mech, MECHALL,
						"The damage you've taken makes the landing a bit harder...");

			if(!MadePilotSkillRoll(mech, calcStaggerBTHMod(mech))) {
				mech_notify(mech, MECHALL,
							"... something you apparently can't handle!");
				MechLOSBroadcast(mech, "lands, staggers, and falls down!");
				MechFalls(mech, 1, 0);
				return;
			}
		}

		/* Check piloting rolls, etc. */
		if(MechType(mech) == CLASS_MECH) {
			if(CountDestroyedLegs(mech) > 0) {
				mech_notify(mech, MECHPILOT,
							"Your missing leg makes it harder to land");
				if(!MadePilotSkillRoll(mech, 0)) {
					mech_notify(mech, MECHALL,
								"Your missing leg has caused you to fall upon landing!");
					MechLOSBroadcast(mech,
									 "lands, unbalanced, and falls down!");
					dfa = 1;
					MechFalls(mech, 1, 0);
					done = 1;
				}
			} else if(MechSections(mech)[RLEG].basetohit ||
					  MechSections(mech)[LLEG].basetohit) {
				mech_notify(mech, MECHPILOT,
							"Your damaged leg actuators make it harder to land");
				if(!MadePilotSkillRoll(mech, 0)) {
					mech_notify(mech, MECHALL,
								"Your damaged leg actuators have caused you to fall upon landing!");
					MechLOSBroadcast(mech,
									 "lands, stumbles, and falls down!");
					dfa = 1;
					done = 1;
					MechFalls(mech, 1, 0);
				}
			} else if((MechCritStatus(mech) & GYRO_DAMAGED) || (MechCritStatus(mech) & GYRO_DESTROYED)) {
				mech_notify(mech, MECHPILOT, "Your damaged gyro makes it harder to land");
				if(!MadePilotSkillRoll(mech, 0)) {
					mech_notify(mech, MECHALL,
							"Your damaged gyro has caused you to fall upon landing!");
					MechLOSBroadcast(mech,
							"lands, twists awkwardly, and falls down!");
					dfa = 1;
					done = 1;
					MechFalls(mech,1,0);
				}
			}
		}
	}

	if((MechType(mech) == CLASS_MECH) && CountSwarmers(mech)) {
		mech_notify(mech, MECHALL,
					"The suits hanging off you make landing harder!");

		if(MadePilotSkillRoll(mech, 4)) {
			StopBSuitSwarmers(FindObjectsData(mech->mapindex), mech, 0);
		} else {
			mech_notify(mech, MECHALL,
						"You fail to properly control your unbalanced landing!");
			MechLOSBroadcast(mech,
							 "lands and crashes to the ground from the weight of the battlesuits!");
			MechFalls(mech, 1, 0);
		}
	}

	if(!dfa && !Fallen(mech) && !domino_space(mech, 1)) {
		if(MechType(mech) != CLASS_VEH_GROUND)
			MechLOSBroadcast(mech, "lands gracefully.");
		else
			MechLOSBroadcast(mech, "returns to the ground where it belongs.");
	}

	/* If we aren't jumping anymore, we already took care of the event.
	   (e.g. in MechFalls()) */
	if(Jumping(mech))
		MECHEVENT(mech, EVENT_JUMPSTABIL, mech_stabilizing_event,
				  JUMP_TO_HIT_RECYCLE, 0);
	MechStatus(mech) &= ~JUMPING;
	MechStatus(mech) &= ~DFA_ATTACK;
	MechDFATarget(mech) = -1;
	MechGoingX(mech) = MechGoingY(mech) = 0;
	MechSpeed(mech) = 0;
	StopJump(mech);				/* Kill the event for moving 'round */
	MaybeMove(mech);			/* Possibly start movin' on da ground */


	if(!done)
		possible_mine_poof(mech, MINE_LAND);

	MechFloods(mech);
	water_extinguish_inferno(mech);
	StopStaggerCheck(mech);
}

/* Flooding code. Once we're in water, this is checked
   now and then (basically when DamageMech'ed and/or
   depth changes and/or we fall) */

void MechFloodsLoc(MECH * mech, int loc, int lev)
{
	char locbuff[32];;

	if((GetSectArmor(mech, loc) && (GetSectRArmor(mech, loc) ||
									!GetSectORArmor(mech, loc)))
	   || !GetSectInt(mech, loc))
		return;
	if(!InWater(mech))
		return;
	if(lev >= 0)
		return;
	/* No armor, and in water. */
	if(lev == -1 && (!Fallen(mech) && loc != LLEG && loc != RLEG &&
					 (!MechIsQuad(mech) || (loc != LARM && loc != RARM))))
		return;
	if(MechType(mech) != CLASS_MECH)
		return;

	if(SectIsFlooded(mech, loc))
		return;

	/* Woo, valid target. */
	ArmorStringFromIndex(loc, locbuff, MechType(mech), MechMove(mech));
	mech_printf(mech, MECHALL,
				"%%ch%%crWater floods into your %s disabling everything that was there!%%c",
				locbuff);
	MechLOSBroadcast(mech,
					 tprintf("has a gaping hole in %s, and water pours in!",
							 locbuff));

	SetSectFlooded(mech, loc);
	DestroyParts(mech, mech, loc, 1, 1);

}

void MechFloods(MECH * mech)
{
	int i;
	int elev = MechElevation(mech);

	if(!InWater(mech))
		return;

	/* Waterproof Tech - no flooding if we have this */
	if(MechSpecials2(mech) & WATERPROOF_TECH)
		return;

	if(MechType(mech) == CLASS_BSUIT) {

		if(MechSwarmTarget(mech) > 0)
			return;

		mech_notify(mech, MECHALL,
					"You somehow find yourself in water and realize this may really really suck...");
		mech_notify(mech, MECHALL,
					"Everything gets very dark as water starts to fill your suit "
					"and you sink towards the bottom!");

		MechLOSBroadcast(mech,
						 "shudders, splashes in the water for a second, then goes limp "
						 "and sinks to the bottom.");

		KillMechContentsIfIC(mech->mynum);
		DestroyMech(mech, mech, 0);
		return;
	}

	if(MechType(mech) != CLASS_MECH)
		return;

	if(MechZ(mech) >= 0)
		return;

	for(i = 0; i < NUM_SECTIONS; i++)
		MechFloodsLoc(mech, i, elev);
}

void MechFalls(MECH * mech, int levels, int seemsg)
{
	int roll, spread, i, hitloc, hitGroup = 0;
	int isrear = 0, damage, iscritical = 0;
	MAP *map;

	/* get rid of our swarmers */
	if(CountSwarmers(mech))
		StopBSuitSwarmers(FindObjectsData(mech->mapindex), mech, 0);

	/* damage pilot */
	MechCocoon(mech) = 0;
	if(MechType(mech) == CLASS_MECH || MechType(mech) == CLASS_MW || seemsg)
		mech_notify(mech, MECHPILOT,
					"You try to avoid taking damage in the fall.");
	else
		mech_notify(mech, MECHPILOT, "You try to avoid taking damage.");
	if(!MadePilotSkillRoll(mech, levels)) {
		if(MechType(mech) == CLASS_MECH || MechType(mech) == CLASS_MW ||
		   seemsg)
			mech_notify(mech, MECHPILOT,
						"You take personal injury from the fall!");
		else
			mech_notify(mech, MECHPILOT, "You take personal injury!");
		headhitmwdamage(mech, 1);
	}
	MechSpeed(mech) = 0;
	MechDesiredSpeed(mech) = 0;
	if(Jumping(mech)) {
		MechStatus(mech) &= ~JUMPING;
		MechStatus(mech) &= ~DFA_ATTACK;
		StopJump(mech);
		MECHEVENT(mech, EVENT_JUMPSTABIL, mech_stabilizing_event,
				  JUMP_TO_HIT_RECYCLE, 0);
	}
#ifdef BT_MOVEMENT_MODES
	if(MoveModeChange(mech))
		StopMoveMode(mech);
	if(MechStatus2(mech) & SPRINTING)
		MECHEVENT(mech, EVENT_MOVEMODE, mech_movemode_event,
				  (MechType(mech) == CLASS_BSUIT
				   || MechType(mech) == CLASS_MW) ? TURN / 2 : TURN,
				  MODE_SPRINT | MODE_OFF);
	if(MechStatus2(mech) & EVADING)
		MECHEVENT(mech, EVENT_MOVEMODE, mech_movemode_event,
				  (MechType(mech) == CLASS_BSUIT
				   || MechType(mech) == CLASS_MW) ? TURN / 2 : TURN,
				  MODE_EVADE | MODE_OFF);
#endif
	if(MechMove(mech) == MOVE_VTOL || MechMove(mech) == MOVE_FLY) {
		MechVerticalSpeed(mech) = 0;
		MechGoingY(mech) = 0;
		MechStartFX(mech) = 0.0;
		MechStartFY(mech) = 0.0;
		MechStartFZ(mech) = 0.0;
		MechStatus(mech) |= LANDED;
		MechStatus(mech) |= FALLEN;
		StopMoving(mech);
	} else
		MaybeMove(mech);
	if(MechType(mech) == CLASS_MECH || MechType(mech) == CLASS_MW)
		MakeMechFall(mech);

	if(seemsg)
		MechLOSBroadcast(mech, "falls down!");
	DropSetElevation(mech, 1);
	MechFZ(mech) = MechZ(mech) * ZSCALE;

	roll = Number(1, 6);
	switch (roll) {
	case 1:
		hitGroup = FRONT;
		break;
	case 2:
		AddFacing(mech, 60);
		hitGroup = RIGHTSIDE;
		break;
	case 3:
		AddFacing(mech, 120);
		hitGroup = RIGHTSIDE;
		break;
	case 4:
		AddFacing(mech, 180);
		hitGroup = BACK;
		break;
	case 5:
		AddFacing(mech, 240);
		hitGroup = LEFTSIDE;
		break;
	case 6:
		AddFacing(mech, 300);
		hitGroup = LEFTSIDE;
		break;
	}
	if(hitGroup == BACK)
		isrear = 1;
	SetFacing(mech, AcceptableDegree(MechFacing(mech)));
	MechDesiredFacing(mech) = MechFacing(mech);
	if(!InWater(mech) && MechRTerrain(mech) != HIGHWATER)
#ifndef REALWEIGHT_DAMAGE
		damage = (levels * (MechTons(mech) + 5)) / 10;
#else
		damage = (levels * (MechRealTons(mech) + 5)) / 10;
#endif /* REALWEIGHT_DAMAGE */
	else
#ifndef REALWEIGHT_DAMAGE
		damage = (levels * (MechTons(mech) + 5)) / 20;
#else
		damage = (levels * (MechRealTons(mech) + 5)) / 20;
#endif /* REALWEIGHT_DAMAGE */
	if(InSpecial(mech))
		if((map = FindObjectsData(mech->mapindex)))
			if(MapUnderSpecialRules(map))
				damage = damage * MIN(100, MapGravity(map)) / 100;

	if(MechType(mech) == CLASS_MW)
		damage *= 40;

	spread = damage / 5;

	for(i = 0; i < spread; i++) {
		hitloc = FindHitLocation(mech, hitGroup, &iscritical, &isrear);
		DamageMech(mech, mech, 0, -1, hitloc, isrear, iscritical, 5, -1,
				   -1, 0, -1, 0, 0);
		MechFloods(mech);
		water_extinguish_inferno(mech);
	}
	if(damage % 5) {
		hitloc = FindHitLocation(mech, hitGroup, &iscritical, &isrear);
		DamageMech(mech, mech, 0, -1, hitloc, isrear, iscritical,
				   (damage % 5), -1, -1, 0, -1, 0, 0);
		MechFloods(mech);
		water_extinguish_inferno(mech);
	}

	possible_mine_poof(mech, MINE_FALL);
	MarkForLOSUpdate(mech);
}

int mechs_in_hex(MAP * map, int x, int y, int friendly, int team)
{
	MECH *mech;
	int i, cnt = 0;

	for(i = 0; i < map->first_free; i++)
		if((mech = FindObjectsData(map->mechsOnMap[i]))) {
			if(MechX(mech) != x || MechY(mech) != y)
				continue;
			if(Destroyed(mech))
				continue;
			if(!(MechSpecials2(mech) & CARRIER_TECH) && IsDS(mech)
			   && (Landed(mech) || !Started(mech))) {
				cnt += 2;
				continue;
			}
			if(MechType(mech) != CLASS_MECH)
				continue;
			if(Jumping(mech) || OODing(mech))
				continue;
			if(friendly < 0 || ((MechTeam(mech) == team) == friendly))
				cnt++;
		}
	return cnt;
}

enum {
	NORMAL, PUNCH, KICK
};

void cause_damage(MECH * att, MECH * mech, int dam, int table)
{
	int hitGroup, isrear, iscrit = 0, hitloc = 0;
	int i, sp = (dam - 1) / 5;

	if(!dam)
		return;
	if(att == mech)
		hitGroup = FRONT;
	else
		hitGroup = FindAreaHitGroup(att, mech);
	isrear = (hitGroup == BACK);
	if(Fallen(mech))
		table = NORMAL;
	for(i = 0; i <= sp; i++) {
		switch (table) {
		case NORMAL:
			hitloc = FindHitLocation(mech, hitGroup, &iscrit, &isrear);
			break;
		case PUNCH:
			FindPunchLoc(mech, hitloc, hitGroup, iscrit, isrear);
			break;
		case KICK:
			FindKickLoc(mech, hitloc, hitGroup, iscrit, isrear);
			break;
		}
		if(dam <= 0)
			return;
		DamageMech(mech, att, (att == mech) ? 0 : 1,
				   (att == mech) ? -1 : MechPilot(att), hitloc, isrear,
				   iscrit, dam > 5 ? 5 : dam, 0, -1, 0, -1, 0, 0);
		dam -= 5;
	}
}

int domino_space_in_hex(MAP * map, MECH * me, int x, int y, int friendly,
						int mode, int cnt)
{
	int tar = Number(0, cnt - 1), i, head, td;
	MECH *mech = NULL;
	int team = MechTeam(me);

	for(i = 0; i < map->first_free; i++)
		if((mech = FindObjectsData(map->mechsOnMap[i]))) {
			if(MechX(mech) != x || MechY(mech) != y)
				continue;
			if(mech == me)
				continue;
			if(IsDS(mech) && (Landed(mech) || !Started(mech))) {
				tar -= 2;
			} else {
				if(!Started(mech))
					continue;
				if(MechType(mech) != CLASS_MECH)
					continue;
				if(Jumping(mech) || OODing(mech))
					continue;
				if(friendly < 0 || ((MechTeam(mech) == team) == friendly))
					tar--;
				else
					continue;
			}
			if(tar <= 0)
				break;
		}
	if(i == map->first_free)
		return 0;
	/* Now we got a mech we hit, accidentally or otherwise */
	/* Next, we figure out what'll happen */

	/* 'wannabe-charge' is entirely based on the directional difference */
	/* Multiplied by the speed - if both go in same direction at same speed,
	   nothing untoward happens (unlikely, though) */
	/* Jumping to a hex with multiple guys is BAD Thing(tm), though */

	switch (mode) {
	case 1:
	case 2:
		head = MechJumpHeading(me);
		td = JumpSpeedMP(me, map) * (MechRTons(me) / 1024 + 5) / 10;
		break;
	default:
		head = MechFacing(me) + MechLateral(me);
		td = fabs(((MechSpeed(me) - MechSpeed(mech) * cos((head -
														   (MechFacing(mech) +
															MechLateral
															(mech))) * (M_PI /
																		180.)))
				   * MP_PER_KPH) * (MechRTons(me) / 1024 + 5) / 15);
		break;
	}
	if(td > 10)
		td = 10 + (td - 10) / 3;
	if(td <= 1)					/* No point in 1pt hits */
		return 0;
	switch (mode) {
	case 1:
	case 2:
		if(mudconf.btech_stacking == 2) {
			int factor = mudconf.btech_stackdamage;
			mech_printf(me, MECHALL, "You land on %s!",
						GetMechToMechID(me, mech));
			mech_printf(mech, MECHALL, "%s lands on you!",
						GetMechToMechID(mech, me));
			MechLOSBroadcasti(me, mech, "lands on %s!");
			if(IsDS(mech)) {
				cause_damage(me, mech, MAX(1, td * factor / 500), PUNCH);
				cause_damage(me, me, MAX(1, td * factor / 100), KICK);
			} else {
				cause_damage(me, mech, MAX(1, td * factor / 100), PUNCH);
				cause_damage(me, me, MAX(1, td * factor / 500), KICK);
			}
		} else {
			mech_printf(me, MECHALL, "You nearly land on %s!",
						GetMechToMechID(me, mech));
			mech_printf(mech, MECHALL, "%s nearly lands on you!",
						GetMechToMechID(mech, me));
			MechLOSBroadcasti(me, mech, "nearly lands on %s!");
			if(!MadePilotSkillRoll(me, cnt + JumpSpeedMP(me, map) / 2))
				MechFalls(me, 1, JumpSpeedMP(me, map) / 2);
		}
		return 1;
	}
	if(mudconf.btech_stacking == 2) {
		int factor = mudconf.btech_stackdamage;
		mech_printf(me, MECHALL, "You bump into %s!",
					GetMechToMechID(me, mech));
		mech_printf(mech, MECHALL, "%s bumps into you!",
					GetMechToMechID(mech, me));
		MechLOSBroadcasti(me, mech, "bumps into %s!");
		if(IsDS(mech)) {
			cause_damage(me, mech, MAX(1, td * factor / 500), NORMAL);
			cause_damage(me, me, MAX(1, td * factor / 100), NORMAL);
		} else {
			cause_damage(me, mech, MAX(1, td * factor / 100), NORMAL);
			cause_damage(me, me, MAX(1, td * factor / 500), NORMAL);
		}
	} else {
		mech_printf(me, MECHALL, "You nearly bump into %s!",
					GetMechToMechID(me, mech));
		mech_printf(mech, MECHALL, "%s nearly bumps into you!",
					GetMechToMechID(mech, me));
		MechLOSBroadcasti(me, mech, "nearly bumps into %s!");
		if(!MadePilotSkillRoll(me, cnt))
			MechFalls(me, 1, 0);
		MechDesiredSpeed(me) = 0;
		MechSpeed(me) = 0;
	}
	MechChargeTarget(me) = -1;
	MechChargeTimer(me) = 0;
	MechChargeDistance(me) = 0;
	return 1;
}

int domino_space(MECH * mech, int mode)
{
	MAP *map = FindObjectsData(mech->mapindex);
	int cnt, fcnt;

	if(!map)
		return 0;
	if(MechType(mech) != CLASS_MECH)
		return 0;
	if(mudconf.btech_stacking == 0)
		return 0;
	cnt = mechs_in_hex(map, MechX(mech), MechY(mech), -1, 0);
	if(cnt <= 2)
		return 0;
	/* Possible nastiness */
	if((fcnt =
		mechs_in_hex(map, MechX(mech), MechY(mech), 1, MechTeam(mech))) > 2)
		return domino_space_in_hex(map, mech, MechX(mech), MechY(mech), 1,
								   mode, fcnt);
	else if(cnt > 6)
		return domino_space_in_hex(map, mech, MechX(mech), MechY(mech), 0,
								   mode, cnt - fcnt);
	return 0;
}
