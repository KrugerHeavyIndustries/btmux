/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *  Copyright (c) 2005-2006 Gregory Taylor
 *       All rights reserved
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "map.h"
#include "mech.events.h"
#include "mech.physical.h"
#include "p.mech.physical.h"
#include "p.mech.combat.h"
#include "p.mech.damage.h"
#include "p.mech.utils.h"
#include "p.mech.los.h"
#include "p.mech.hitloc.h"
#include "p.bsuit.h"
#include "p.btechstats.h"
#include "p.template.h"
#include "p.mech.bth.h"

// Only allows arm physical attacks for CLASS_MECH.
#define ARM_PHYS_CHECK(a) \
DOCHECK(MechType(mech) == CLASS_MW || MechType(mech) == CLASS_BSUIT, \
  tprintf("You cannot %s without a 'mech!", a)); \
DOCHECK(MechType(mech) != CLASS_MECH, \
  tprintf("You cannot %s with this vehicle!", a));

// Checks a unit's legs for kicking.
#define GENERIC_CHECK(a,wDeadLegs) \
ARM_PHYS_CHECK(a);\
DOCHECK(!MechIsQuad(mech) && (wDeadLegs > 1), \
  "Without legs? Are you kidding?");\
DOCHECK(!MechIsQuad(mech) && (wDeadLegs > 0),\
  "With one leg? Are you kidding?");\
DOCHECK(wDeadLegs > 1,"It'd unbalance you too much in your condition..");\
DOCHECK(wDeadLegs > 2, "Exactly _what_ are you going to kick with?");

// If it's a quad, we can't play with sharp things (Swords, Axes, etc.)
#define QUAD_CHECK(a) \
DOCHECK(MechType(mech) == CLASS_MECH && MechIsQuad(mech), \
  tprintf("What are you going to %s with, your front right leg?", a))

/**
 * Checks to see if all limbs have recycled from any previous physical attacks.
 */
int all_limbs_recycled(MECH * mech)
{
	if(MechSections(mech)[LARM].recycle || MechSections(mech)[RARM].recycle) {
		mech_notify(mech, MECHALL,
					"You still have arms recovering from another attack.");
		return 0;
	}

	if(MechSections(mech)[RLEG].recycle || MechSections(mech)[LLEG].recycle) {
		mech_notify(mech, MECHALL,
					"Your legs are still recovering from your last attack.");
		return 0;
	}
	// Fall through to success.
	return 1;
}								// end all_limbs_recycled()

/**
 * Returns the correct verb for each physical attack.
 */
char *phys_form(int AttackType, int add_s)
{
	// Holds our attack verb.
	char *verb;

	// See if we need the verb with an s on the end.
	if(add_s) {
		// With the S.
		switch (AttackType) {
		case PA_PUNCH:
			verb = "punchs";
			break;
		case PA_CLUB:
			verb = "clubs";
			break;
		case PA_MACE:
			verb = "maces";
			break;
		case PA_SWORD:
			verb = "chops";
			break;
		case PA_AXE:
			verb = "axes";
			break;
		case PA_KICK:
			verb = "kicks";
			break;
		case PA_TRIP:
			verb = "trips";
			break;
		case PA_SAW:
			verb = "saws";
			break;
		case PA_CLAW:
			verb = "claws";
			break;
			// Ohboy, we're using some funky, unknown physical.
		default:
			verb = "??bugs??";
		}						// end switch()
	} else {
		// Without the S.
		switch (AttackType) {
		case PA_PUNCH:
			verb = "punch";
			break;
		case PA_CLUB:
			verb = "club";
			break;
		case PA_MACE:
			verb = "maces";
			break;
		case PA_SWORD:
			verb = "chop";
			break;
		case PA_AXE:
			verb = "axe";
			break;
		case PA_KICK:
			verb = "kick";
			break;
		case PA_TRIP:
			verb = "trip";
			break;
		case PA_SAW:
			verb = "saw";
			break;
		case PA_CLAW:
			verb = "claw";
			break;
			// Ohboy, we're using some funky, unknown physical.
		default:
			verb = "??bugs??";
		}						// end switch()
	}							// end if/else()

	return verb;
}								// end phys_form

#define phys_message(txt) \
MechLOSBroadcasti(mech,target,txt)

void phys_succeed(MECH * mech, MECH * target, int at)
{
	phys_message(tprintf("%s %%s!", phys_form(at, 1)));
}

void phys_fail(MECH * mech, MECH * target, int at)
{
	phys_message(tprintf("attempts to %s %%s!", phys_form(at, 0)));
}

/*
 * All 'mechs with arms can punch.
 */
static int have_punch(MECH * mech, int loc)
{
	return 1;
}

/**
 * Does our unit have an axe?
 */
int have_axe(MECH * mech, int loc)
{
	return FindObj(mech, loc, I2Special(AXE)) >= (MechTons(mech) / 15);
}

int have_claw(MECH * mech, int loc)
{
	return FindObj(mech, loc, I2Special(CLAW)) >= (MechTons(mech) / 15);
}

/**
 * Does our unit have a dual_saw?
 */
int have_saw(MECH * mech, int loc)
{
	return FindObj(mech, loc, I2Special(DUAL_SAW)) >= 7;
}

/**
 * Does our unit have a sword?
 */
int have_sword(MECH * mech, int loc)
{
	return FindObj(mech, loc,
				   I2Special(SWORD)) >= ((MechTons(mech) + 15) / 20);
}

/**
 * Does our unit have a mace?
 */
int have_mace(MECH * mech, int loc)
{
	return FindObj(mech, loc, I2Special(MACE)) >= (MechTons(mech) / 10);
}

/*
 * Carry out some checks common to all types of physical attacks.
 */
int phys_common_checks(MECH * mech)
{
	if(Jumping(mech)) {
		mech_notify(mech, MECHALL,
					"You can't perform physical attacks while in the air!");
		return 0;
	}

	if(Standing(mech)) {
		mech_notify(mech, MECHALL, "You are still trying to stand up!");
		return 0;
	}
#ifdef BT_MOVEMENT_MODES
	if(Dodging(mech) || MoveModeLock(mech)) {
		mech_notify(mech, MECHALL,
					"You cannot use physicals while using a special movement mode.");
		return 0;
	}
#endif

	if(!all_limbs_recycled(mech)) {
		return 0;
	}
	// Fall through to success.
	return 1;
}								// end phys_common_checks()

/*
 * Parse a physical attack command's arguments that allow an arm or both
 * to be specified. eg. AXE [B|L|R] [ID]
 */
static int get_arm_args(int *using, int *argc, char ***args, MECH * mech,
						int (*have_fn) (MECH * mech, int loc), char *weapon)
{

	if(*argc != 0 && args[0][0][0] != '\0' && args[0][0][1] == '\0') {
		char arm = toupper(args[0][0][0]);

		// Determine which flag we're dealing with (Both, Left, Right)
		switch (arm) {
		case 'B':
			*using = P_LEFT | P_RIGHT;
			--*argc;
			++*args;
			break;

		case 'L':
			*using = P_LEFT;
			--*argc;
			++*args;
			break;

		case 'R':
			*using = P_RIGHT;
			--*argc;
			++*args;
		}						// end switch()
	}							// end if()

	// Check for the presence of specified arms, or pick one. *using set in
	// the above switch statement.
	switch (*using) {
	case P_LEFT:
		if(!have_fn(mech, LARM)) {
			mech_printf(mech, MECHALL,
						"You don't have %s in your left arm!", weapon);
			return 1;
		}
		break;

	case P_RIGHT:
		if(!have_fn(mech, RARM)) {
			mech_printf(mech, MECHALL,
						"You don't have %s in your right arm!", weapon);
			return 1;
		}
		break;

	case P_LEFT | P_RIGHT:
		if(!have_fn(mech, LARM))
			*using &= ~P_LEFT;
		if(!have_fn(mech, RARM))
			*using &= ~P_RIGHT;
		break;
	}							// end switch()

	// Fall through to success.
	return 0;
}								// end get_arm_args()

/**
 * Performs some generic checks for arms to punch with.
 */
int punch_checkArm(MECH * mech, int arm)
{
	char *arm_used = (arm == LARM ? "left" : "right");

	if(SectIsDestroyed(mech, arm)) {
		mech_printf(mech, MECHALL,
					"Your %s arm is destroyed, you can't punch with it.",
					arm_used);
		return 0;
	} else if(!OkayCritSectS(arm, 0, SHOULDER_OR_HIP)) {
		mech_printf(mech, MECHALL,
					"Your %s shoulder is destroyed, you can't punch with that arm.",
					arm_used);
		return 0;
	} else if(MechSections(mech)[arm].specials & CARRYING_CLUB) {
		mech_printf(mech, MECHALL,
					"You're carrying a club in your %s arm and can't punch with it.",
					arm_used);
		return 0;
	}
	// Fall through to success.
	return 1;
}								// end checkArm()

/**
 * Mech punch routines.
 */
void mech_punch(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MAP *mech_map = getMap(mech->mapindex);
	char *argl[5];
	char **args = argl;
	int argc, ltohit = 4, rtohit = 4;
	int punching = P_LEFT | P_RIGHT;

	// Carry out the common checks (started, on map, etc.)
	cch(MECH_USUALO);
	// Make sure we have arms to punch with.
	ARM_PHYS_CHECK("punch");
	// Disallow quads from punching.
	QUAD_CHECK("punch");

	argc = mech_parseattributes(buffer, args, 5);

	// If the directive is true, use the pilot's piloting skill. If not, we
	// use a constant BTH of 4.
	if(mudconf.btech_phys_use_pskill) 
	 	rtohit = ltohit = FindPilotPiloting(mech);


	// Manipulate punching var to contain only the arms we're punching with.
	if(get_arm_args(&punching, &argc, &args, mech, have_punch, "")) {
		return;
	}
	// Carry out our standard physical checks. This happens in PhysicalAttack
	// but the player gets double-spammed since PhysicalAttack can be called
	// twice in here. So, we add the check before.
	if(!phys_common_checks(mech))
		return;

	// For each arm we're using, check to make sure it's good to punch with
	// and carry out the roll if it is. 
	if(punching & P_LEFT) {
		if(punch_checkArm(mech, LARM))
			PhysicalAttack(mech, 10, ltohit, PA_PUNCH, argc, args,
						   mech_map, LARM);
	}

	if(punching & P_RIGHT) {
		if(punch_checkArm(mech, RARM))
			PhysicalAttack(mech, 10, rtohit, PA_PUNCH, argc, args,
						   mech_map, RARM);
	}
}								// end mech_punch()

/**
 * Mech clubbing routines.
 */
void mech_club(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MAP *mech_map = getMap(mech->mapindex);
	char *args[5];
	int argc;
	int clubLoc = -1;

	// Make sure unit is started, on map, etc.
	cch(MECH_USUALO);
	// Make sure we're in a biped.
	ARM_PHYS_CHECK("club");
	// Don't let quads club.
	QUAD_CHECK("club");

	if(MechSections(mech)[RARM].specials & CARRYING_CLUB)
		clubLoc = RARM;
	else if(MechSections(mech)[LARM].specials & CARRYING_CLUB)
		clubLoc = LARM;

	if(clubLoc == -1) {
		DOCHECKMA(MechRTerrain(mech) != HEAVY_FOREST &&
				  MechRTerrain(mech) != LIGHT_FOREST,
				  "You can not seem to find any trees around to club with.");
		// Since we have trees nearby, assume the club goes to right hand.
		clubLoc = RARM;
	}

	argc = mech_parseattributes(buffer, args, 5);

	DOCHECKMA(SectIsDestroyed(mech, LARM),
			  "Your left arm is destroyed, you can't club.");
	DOCHECKMA(SectIsDestroyed(mech, RARM),
			  "Your right arm is destroyed, you can't club.");
	DOCHECKMA(!OkayCritSectS(RARM, 0, SHOULDER_OR_HIP),
			  "You can't club anyone with a destroyed or missing right shoulder.");
	DOCHECKMA(!OkayCritSectS(LARM, 0, SHOULDER_OR_HIP),
			  "You can't club anyone with a destroyed or missing left shoulder.");
	DOCHECKMA(!OkayCritSectS(RARM, 3, HAND_OR_FOOT_ACTUATOR),
			  "You can't club anyone with a destroyed or missing right hand.");
	DOCHECKMA(!OkayCritSectS(LARM, 3, HAND_OR_FOOT_ACTUATOR),
			  "You can't club anyone with a destroyed or missing left hand.");

	// Clubbing is usually done with the right arm but a club may be
	// grabbed by the left hand. Clubbing requires both arms to be cycled,
	// but only one is checked by PhysicalAttack(). So, we check both
	// here just in case.
	DOCHECKMA(SectHasBusyWeap(mech, LARM) || SectHasBusyWeap(mech, RARM),
			  "You have weapons recycling on your arms.");

	PhysicalAttack(mech, 5,
				   (mudconf.btech_phys_use_pskill ? FindPilotPiloting(mech) - 1 
                    : 4), PA_CLUB, argc,
				   args, mech_map, RARM);
}								// end mech_club()

/**
 * Check to see if the specified arm can be used to axe with.
 */
int axe_checkArm(MECH * mech, int arm)
{
	char *arm_used = (arm == RARM ? "right" : "left");

	if(SectIsDestroyed(mech, arm)) {
		mech_printf(mech, MECHALL,
					"Your %s arm is destroyed, you can't axe with it",
					arm_used);
		return 0;
	} else if(!OkayCritSectS(arm, 0, SHOULDER_OR_HIP)) {
		mech_printf(mech, MECHALL,
					"Your %s shoulder is destroyed, you can't axe with that arm.",
					arm_used);
		return 0;
	} else if(!OkayCritSectS(arm, 3, HAND_OR_FOOT_ACTUATOR)) {
		mech_printf(mech, MECHALL,
					"Your left hand is destroyed, you can't axe with that arm.",
					arm_used);
		return 0;
	}
	// Fall through to success.    
	return 1;
}								// end axe_checkArm()

/**
 * Mech axe routines.
 */
void mech_axe(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MAP *mech_map = getMap(mech->mapindex);
	char *argl[5];
	char **args = argl;
	int argc, ltohit = 4, rtohit = 4;
	int using = P_LEFT | P_RIGHT;

	// Make sure we're started, on a map, etc.
	cch(MECH_USUALO);
	// Do we have arms?
	ARM_PHYS_CHECK("axe");
	// Make sure we're not a quad.
	QUAD_CHECK("axe");

	argc = mech_parseattributes(buffer, args, 5);

	// If btech_phys_use_pskill is on, use the player's piloting skill.
	// If not, assume a skill level of 4.
	if(mudconf.btech_phys_use_pskill)
		ltohit = rtohit = FindPilotPiloting(mech) - 1;

	// Figure out which arm to use.
	if(get_arm_args(&using, &argc, &args, mech, have_axe, "an axe")) {
		return;
	}

	if(using & P_LEFT) {
		if(axe_checkArm(mech, LARM))
			PhysicalAttack(mech, 5, ltohit, PA_AXE, argc, args, mech_map,
						   LARM);
	}
	if(using & P_RIGHT) {
		if(axe_checkArm(mech, RARM))
			PhysicalAttack(mech, 5, rtohit, PA_AXE, argc, args, mech_map,
						   RARM);
	}
	// We don't have an axe.
	DOCHECKMA(!using,
			  "You may lack the axe, but not the will! Try punch/club until you find one.");
}								// end mech_axe()

/**
 * Check to see if the specified arm can be used to saw with.
 */
int saw_checkArm(MECH * mech, int arm)
{
	char *arm_used = (arm == RARM ? "right" : "left");

	if(SectIsDestroyed(mech, arm)) {
		mech_printf(mech, MECHALL,
					"Your %s arm is destroyed, you can't saw with it",
					arm_used);
		return 0;
	} else if(!OkayCritSectS(arm, 0, SHOULDER_OR_HIP)) {
		mech_printf(mech, MECHALL,
					"Your %s shoulder is destroyed, you can't saw with that arm.",
					arm_used);
		return 0;
	}
	// Fall through to success.    
	return 1;
}								// end saw_checkArm()

/**
 * Mech dual saw routines.
 */
void mech_saw(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MAP *mech_map = getMap(mech->mapindex);
	char *argl[5];
	char **args = argl;
	int argc, ltohit = 4, rtohit = 4;
	int using = P_LEFT | P_RIGHT;

	// Make sure we're started, on a map, etc.
	cch(MECH_USUALO);
	// Do we have arms?
	ARM_PHYS_CHECK("saw");
	// Make sure we're not a quad.
	QUAD_CHECK("saw");

	argc = mech_parseattributes(buffer, args, 5);

	// If btech_phys_use_pskill is on, use the player's piloting skill.
	// If not, assume a skill level of 4.
	if(mudconf.btech_phys_use_pskill)
		ltohit = rtohit = FindPilotPiloting(mech) - 1;

	// Figure out which arm to use.
	if(get_arm_args(&using, &argc, &args, mech, have_saw, "a saw")) {
		return;
	}

	if(using & P_LEFT) {
		if(saw_checkArm(mech, LARM))
			PhysicalAttack(mech, 7, ltohit, PA_SAW, argc, args, mech_map,
						   LARM);
	}
	if(using & P_RIGHT) {
		if(saw_checkArm(mech, RARM))
			PhysicalAttack(mech, 7, rtohit, PA_SAW, argc, args, mech_map,
						   RARM);
	}
	// We don't have a saw.
	DOCHECKMA(!using, "You don't have a dual saw!");
}								// end mech_saw()

/**
 * Mech punch routines.
 */
void mech_claw(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MAP *mech_map = getMap(mech->mapindex);
	char *argl[5];
	char **args = argl;
	int argc, ltohit = 4, rtohit = 4;
	int using = P_LEFT | P_RIGHT;

	// Carry out the common checks (started, on map, etc.)
	cch(MECH_USUALO);
	// Make sure we have arms to claw with.
	ARM_PHYS_CHECK("claw");
	// Disallow quads from clawing.
	QUAD_CHECK("claw");

	argc = mech_parseattributes(buffer, args, 5);

	// If the directive is true, use the pilot's piloting skill. If not, we
	// use a constant BTH of 4.
	if(mudconf.btech_phys_use_pskill) 
	 	rtohit = ltohit = FindPilotPiloting(mech);


	// Manipulate punching var to contain only the arms we're punching with.
	if(get_arm_args(&using, &argc, &args, mech, have_claw, "a claw")) {
		return;
	}
	// Carry out our standard physical checks. This happens in PhysicalAttack
	// but the player gets double-spammed since PhysicalAttack can be called
	// twice in here. So, we add the check before.
	if(!phys_common_checks(mech))
		return;

	// For each arm we're using, check to make sure it's good to punch with
	// and carry out the roll if it is. 
	if(using & P_LEFT) {
			PhysicalAttack(mech, 7, ltohit, PA_CLAW, argc, args,
						   mech_map, LARM);
	}

	if(using & P_RIGHT) {
			PhysicalAttack(mech, 7, rtohit, PA_CLAW, argc, args,
						   mech_map, RARM);
	}
 
        // We don't have a claw
	DOCHECKMA(!using, "You do not have any claws! Try punching/clubbing instead!");
		
}								// end mech_claw()


/**
 * Check our arms to see if they can mace.
 */
int mace_checkArm(MECH * mech, int arm)
{
	char *arm_used = (arm == RARM ? "right" : "left");

	if(SectIsDestroyed(mech, arm)) {
		mech_printf(mech, MECHALL,
					"Your %s arm is destroyed, you can't use a mace with it.",
					arm_used);
		return 0;
	} else if(!OkayCritSectS(arm, 0, SHOULDER_OR_HIP)) {
		mech_printf(mech, MECHALL,
					"Your %s shoulder is destroyed, you can't use a mace with that arm.",
					arm_used);
		return 0;
	} else if(!OkayCritSectS(arm, 3, HAND_OR_FOOT_ACTUATOR)) {
		mech_printf(mech, MECHALL,
					"Your %s hand is destroyed, you can't use a mace with that arm.",
					arm_used);
		return 0;
	}
	// Fall through to success.
	return 1;
}								// end mace_checkArm()

/**
 * Mech mace routines.
 */
void mech_mace(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MAP *mech_map = getMap(mech->mapindex);
	char *argl[5];
	char **args = argl;
	int argc, ltohit = 4, rtohit = 4;
	int using = P_LEFT | P_RIGHT;

	// Make sure we're started, on a map, etc.
	cch(MECH_USUALO);
	// Do we have arms?
	ARM_PHYS_CHECK("mace");
	// Make sure we're not a quad.
	QUAD_CHECK("mace");

	argc = mech_parseattributes(buffer, args, 5);

	// If btech_phys_use_pskill is on, use the player's piloting skill.
	// If not, assume a skill level of 4.
	if(mudconf.btech_phys_use_pskill)
		ltohit = rtohit = FindPilotPiloting(mech) - 1;

	// Figure out which arm to use.
	if(get_arm_args(&using, &argc, &args, mech, have_mace, "a mace")) {
		return;
	}

	if(using & P_LEFT) {
		if(mace_checkArm(mech, LARM))
			PhysicalAttack(mech, 4, ltohit, PA_MACE, argc, args, mech_map,
						   LARM);
	}
	if(using & P_RIGHT) {
		if(mace_checkArm(mech, RARM))
			PhysicalAttack(mech, 4, rtohit, PA_MACE, argc, args, mech_map,
						   RARM);
	}
	// We don't have a mace.
	DOCHECKMA(!using, "You don't have a mace!");
}								// end mech_mace()

/**
 * Check our arms to see if they can chop.
 */
int sword_checkArm(MECH * mech, int arm)
{
	char *arm_used = (arm == RARM ? "right" : "left");

	if(SectIsDestroyed(mech, arm)) {
		mech_printf(mech, MECHALL,
					"Your %s arm is destroyed, you can't use a sword with it.",
					arm_used);
		return 0;
	} else if(!OkayCritSectS(arm, 0, SHOULDER_OR_HIP)) {
		mech_printf(mech, MECHALL,
					"Your %s shoulder is destroyed, you can't use a sword with that arm.",
					arm_used);
		return 0;
	} else if(!OkayCritSectS(arm, 3, HAND_OR_FOOT_ACTUATOR)) {
		mech_printf(mech, MECHALL,
					"Your %s hand is destroyed, you can't use a sword with that arm.",
					arm_used);
		return 0;
	}
	// Fall through to success.
	return 1;
}								// end sword_checkArm()

/**
 * Mech sword routines.
 */
void mech_sword(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MAP *mech_map = getMap(mech->mapindex);
	char *argl[5];
	char **args = argl;
	int argc, ltohit = 3, rtohit = 3;
	int using = P_LEFT | P_RIGHT;

	// Make sure we're started, on a map, etc.
	cch(MECH_USUALO);
	// Do we have arms to chop with?
	ARM_PHYS_CHECK("chop");
	// Quads can't do it.
	QUAD_CHECK("chop");

	argc = mech_parseattributes(buffer, args, 5);

	// If btech_phys_use_pskill is defined, use the pilot's piloting skill,
	// otherwise use a constant skill 3.
	if(mudconf.btech_phys_use_pskill)
		ltohit = rtohit = FindPilotPiloting(mech) - 2;

	// Which arm(s) have sword crits?
	if(get_arm_args(&using, &argc, &args, mech, have_sword, "a sword")) {
		return;
	}

	if(using & P_LEFT) {
		if(sword_checkArm(mech, LARM))
			PhysicalAttack(mech, 10, ltohit, PA_SWORD, argc, args, mech_map,
						   LARM);
	}

	if(using & P_RIGHT) {
		if(sword_checkArm(mech, RARM))
			PhysicalAttack(mech, 10, rtohit, PA_SWORD, argc, args, mech_map,
						   RARM);
	}
	// Ninja what?
	DOCHECKMA(!using, "You have no sword to chop people with!");
}								// end mech_sword()

/**
 * Mech tripping command hook.
 */
void mech_trip(dbref player, void *data, char *buffer)
{
	mech_kickortrip(player, data, buffer, PA_TRIP);
}								// end mech_trip()

/**
 * Mech kick command hook.
 */
void mech_kick(dbref player, void *data, char *buffer)
{
	mech_kickortrip(player, data, buffer, PA_KICK);
}								// end mech_trip()

/**
 * Mech kick/trip routines.
 */
void mech_kickortrip(dbref player, void *data, char *buffer, int AttackType)
{
	MECH *mech = (MECH *) data;
	MAP *mech_map = getMap(mech->mapindex);
	char *argl[5];
	char **args = argl;
	int argc;
	int rl = RLEG, ll = LLEG;
	int leg;
	int using = P_RIGHT;

	// Make sure we're started, on a map, etc.
	cch(MECH_USUALO);
	// If we're a quad, re-map front legs.
	if(MechIsQuad(mech)) {
		rl = RARM;
		ll = LARM;
	}
	// See if we have enough usable legs to kick/trip with.
	GENERIC_CHECK("kick", CountDestroyedLegs(mech));

	argc = mech_parseattributes(buffer, args, 5);

	// Figure out which leg we're using.
	if(get_arm_args(&using, &argc, &args, mech, have_punch, "")) {
		return;
	}

	switch (using) {
	case P_LEFT:
		leg = ll;
		break;

	case P_RIGHT:
		leg = rl;
		break;

	default:
	case P_LEFT | P_RIGHT:
		mech_notify(mech, MECHALL,
					"What, yer gonna LEVITATE? I Don't Think So.");
		return;
	}

	if((MechCritStatus(mech) & HIP_DAMAGED))
	{
		mech_printf(mech, MECHALL,
					"You can't %s with a destroyed hip.",
					phys_form(AttackType, 0));
		return;
	}

	PhysicalAttack(mech, 5,
				   (mudconf.btech_phys_use_pskill ? FindPilotPiloting(mech) -
					2 : 3), AttackType, argc, args, mech_map, leg);
}								// end mech_kickortrip()

/**
 * Mech/tank charge routines
 */
void mech_charge(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data, *target;
	MAP *mech_map = getMap(mech->mapindex);
	int targetnum;
	char targetID[5];
	char *args[5];
	int argc;
	int wcDeadLegs = 0;

	// Make sure we're started, on a map, etc.
	cch(MECH_USUALO);

	// Mechwarriors can't chage.
	DOCHECK(MechType(mech) == CLASS_MW ||
			MechType(mech) == CLASS_BSUIT,
			"You cannot charge without a 'mech!");

	// Salvage vehicles can't charge.
	DOCHECK(MechType(mech) != CLASS_MECH &&
			(MechType(mech) != CLASS_VEH_GROUND ||
			 MechSpecials(mech) & SALVAGE_TECH),
			"You cannot charge with this vehicle!");

	// Figure out if we have enough legs to kick with.
	if(MechType(mech) == CLASS_MECH) {
		/* set the number of dead legs we have */
		wcDeadLegs = CountDestroyedLegs(mech);

		DOCHECK(!MechIsQuad(mech) && (wcDeadLegs > 0),
				"With one leg? Are you kidding?");
		DOCHECK(!MechIsQuad(mech) && (wcDeadLegs > 1),
				"Without legs? Are you kidding?");
		DOCHECK(wcDeadLegs > 1,
				"It'd unbalance you too much in your condition..");
		DOCHECK(wcDeadLegs > 2, "Exactly _what_ are you going to kick with?");
	}							// end if() - Dead leg counting.

	argc = mech_parseattributes(buffer, args, 2);

	DOCHECK(MoveModeChange(mech), "You cannot charge while changing movement modes!");

	DOCHECK(Sprinting(mech) || Evading(mech), "You cannot charge while in a special movement mode!");
	DOCHECK(Dodging(mech), "You cannot charge while dodging!");

	switch (argc) {
		// No arguments given with charge. Assume default target.
	case 0:
		DOCHECKMA(MechTarget(mech) == -1,
				  "You do not have a default target set!");

		target = getMech(MechTarget(mech));

		if(!target) {
			mech_notify(mech, MECHALL, "Invalid default target!");
			MechTarget(mech) = -1;
			return;
		}
		// Don't allow charging Mechwarriors.
		if(MechType(target) == CLASS_MW) {
			mech_notify(mech, MECHALL,
						"You can't charge THAT sack of bones and squishy bits!");
			return;
		}

		if(MapNoFriendlyFire(mech_map) && (MechTeam(mech) == MechTeam(target))) {
			mech_notify(mech, MECHALL, "You can't charge your own team!");
			MechChargeTarget(mech) = -1;
			return;
		}

		MechChargeTarget(mech) = MechTarget(mech);
		mech_notify(mech, MECHALL, "Charge target set to default target.");
		break;

		// We've supplied an argument, either a '-' or an ID.
	case 1:
		if(args[0][0] == '-') {
			MechChargeTarget(mech) = -1;
			MechChargeTimer(mech) = 0;
			MechChargeDistance(mech) = 0;
			mech_notify(mech, MECHPILOT, "You are no longer charging.");
			return;
		}

		targetID[0] = args[0][0];
		targetID[1] = args[0][1];
		targetnum = FindTargetDBREFFromMapNumber(mech, targetID);

		DOCHECKMA(targetnum == -1, "Target is not in line of sight!");

		target = getMech(targetnum);
		DOCHECKMA(!InLineOfSight_NB(mech, target, MechX(target),
									MechY(target), FaMechRange(mech, target)),
				  "Target is not in line of sight!");

		if(!target) {
			mech_notify(mech, MECHALL, "Invalid target data!");
			return;
		}

		if(MapNoFriendlyFire(mech_map) && (MechTeam(mech) == MechTeam(target))) {
			mech_notify(mech, MECHALL, "You can't charge your own team!");
			MechChargeTarget(mech) = -1;
			return;
		}

		// Don't allow charging mechwarriors.
		if(MechType(target) == CLASS_MW) {
			mech_notify(mech, MECHALL,
						"You can't charge THAT sack of bones and squishy bits!");
			return;
		}

		MechChargeTarget(mech) = targetnum;

		mech_printf(mech, MECHALL, "%s target set to %s.",
					MechType(mech) == CLASS_MECH ? "Charge" : "Ram",
					GetMechToMechID(mech, target));
		break;

		// Something other than 0-1 arguments.
	default:
		notify(player, "Invalid number of arguments!");
	}
}								// end mech_charge()

/*
 * Home to the code to carry out physical attacks.
 *
 * NOTE: Do NOT put any logic/checker code in here that is specific to a
 * certain type of attack if possible. Put it in its respective function.
 * Only things that are generic and on-specific to an attack type go here.
 */
void PhysicalAttack(MECH * mech, int damageweight, int baseToHit,
					int AttackType, int argc, char **args, MAP * mech_map,
					int sect)
{
	MECH *target;
	float range;
	float maxRange = 1;
	char targetID[2];
	int targetnum, roll, swarmingUs;
	char location[20];
	int ts = 0, iwa;
	int RbaseToHit, glance = 0;
	

	/*
	 * Common Checks
	 */

	// Since we can punch with two arms, often back to back, we want to run
	// these generic checks in mech_punch() -BEFORE- PhysicalAttack() is called
	// twice (if we have two working arms).
	if(AttackType == PA_PUNCH || AttackType == PA_CLAW)
	{
		// Do Nothing
	} else {
		if(!phys_common_checks(mech))
			return;
	}

    /* BTH Adjustments for crits to limbs - seperate one for 
     * club because it checks for both limbs */
    if ((AttackType == PA_PUNCH) || (AttackType == PA_KICK) || 
            (AttackType == PA_AXE) || (AttackType == PA_SWORD) ||
	    (AttackType == PA_CLAW) ||
            (AttackType == PA_MACE) || (AttackType == PA_SAW)) {

        if (PartIsNonfunctional(mech, sect, 1) ||
                GetPartType(mech, sect, 1) != I2Special(UPPER_ACTUATOR)) {
            baseToHit += 2;
        }

        if (PartIsNonfunctional(mech, sect, 2) ||
                GetPartType(mech, sect, 2) != I2Special(LOWER_ACTUATOR)) {
            baseToHit += 2;
        }

        /* Hand/Foot crits only affect punch/kick since with the other attacks
         * are not allowed if they're broken */
        if ((AttackType == PA_PUNCH) || (AttackType == PA_KICK)) {
                if (PartIsNonfunctional(mech, sect, 3) ||
                    GetPartType(mech, sect, 3) != I2Special(HAND_OR_FOOT_ACTUATOR)) {
                baseToHit += 1;
                }
        }

    } else if (AttackType == PA_CLUB) {

        /* Only check lower/upper acts since without shoulder or hand you can't
         * club */
        if (PartIsNonfunctional(mech, RARM, 1) ||
                GetPartType(mech, sect, 1) != I2Special(UPPER_ACTUATOR)) {
            baseToHit += 2;
        }
        if (PartIsNonfunctional(mech, RARM, 2) ||
                GetPartType(mech, sect, 2) != I2Special(LOWER_ACTUATOR)) {
            baseToHit += 2;
        }
        if (PartIsNonfunctional(mech, LARM, 1) ||
                GetPartType(mech, sect, 1) != I2Special(UPPER_ACTUATOR)) {
            baseToHit += 2;
        }
        if (PartIsNonfunctional(mech, LARM, 2) ||
                GetPartType(mech, sect, 2) != I2Special(LOWER_ACTUATOR)) {
            baseToHit += 2;
        }
    }

    // All weapons must be cycled in the target limb.
    if(SectHasBusyWeap(mech, sect)) {
		ArmorStringFromIndex(sect, location, MechType(mech), MechMove(mech));
		mech_printf(mech, MECHALL,
					"You have weapons recycling on your %s.", location);
		return;
	}
	// Figure out what to do with the arguments provided with the physical
	// command.
	switch (argc) {
	case -1:
	case 0:
		// No argument
		DOCHECKMA(MechTarget(mech) == -1, "You do not have a target set!");

		// Populate target variable with current lock.
		target = getMech(MechTarget(mech));
		DOCHECKMA(!target, "Invalid default target!");

		break;
	default:
		// In this case, default means user has specified an argument
		// with the physical attack.

		// Populate target variable from user input.
		targetID[0] = args[0][0];
		targetID[1] = args[0][1];
		targetnum = FindTargetDBREFFromMapNumber(mech, targetID);
		target = getMech(targetnum);

		DOCHECKMA(targetnum == -1, "Target is not in line of sight!");
		DOCHECKMA(!target, "Invalid default target!");
	}							// end switch() - argc checking

	// Is the target swarming us?
	swarmingUs = (MechSwarmTarget(target) == mech->mynum ? 1 : 0);

	/*
	 * Common checks.
	 */

	// If we're attacking something while fallen that isn't swarming us,
	// no-go it. Kicks/trips are automatically stopped.
	if(Fallen(mech) && (AttackType == PA_KICK || AttackType == PA_TRIP)) {
		mech_printf(mech, MECHALL, "You can't %s from a prone position.",
					phys_form(AttackType, 0));

		return;
		// If we are fallen AND
		//   The target is not a BSuit AND We're not punching
	} else if(Fallen(mech) &&
			  (MechType(target) != CLASS_VEH_GROUND &&
			   MechType(target) != CLASS_BSUIT)) {
		mech_printf(mech, MECHALL,
					"You can't %s from a prone position.",
					phys_form(AttackType, 0));

		return;
	} else if(Fallen(mech) && MechType(target) == CLASS_BSUIT && !swarmingUs) {
		mech_notify(mech, MECHALL,
					"You may only physical suits that are swarming you while prone.");
		return;
	} else if(Fallen(mech) && MechType(target) == CLASS_VEH_GROUND &&
			  AttackType != PA_PUNCH) {
		mech_notify(mech, MECHALL,
					"You may only punch vehicles while prone.");
		return;
	}							// end if() - Physical while fallen.

	range = FaMechRange(mech, target);

	DOCHECKMA(!InLineOfSight_NB(mech, target, MechX(target),
								MechY(target), range),
			  "Target is not in line of sight!");

	// BSuits have to be <= 0.5 hexes to attack units.
	if((MechType(target) == CLASS_BSUIT) || (MechType(target) == CLASS_MW))
		maxRange = 0.5;

	DOCHECKMA(range >= maxRange, "Target out of range!");

	DOCHECKMA(Jumping(target),
			  "You can't perform physical attacks on airborne mechs!");

	DOCHECKMA(MapNoPhysicals(mech_map),"You cannot perform physical attacks here!");

    DOCHECKMA(MechTeam(target) == MechTeam(mech) && 
            MechNoFriendlyFire(mech),
            "You can't attack a teammate with FFSafeties on!");

    DOCHECKMA(MechTeam(target) == MechTeam(mech) && 
            MapNoFriendlyFire(mech_map),
            "Friendly Fire? I don't think so...");

	DOCHECKMA(MechType(target) == CLASS_MW && !MechPKiller(mech),
			  "That's a living, breathing person! Switch off the safety first, "
			  "if you really want to assassinate the target.");

	DOCHECKMA(MechCritStatus(mech) & MECH_STUNNED,
		"You are still recovering from your stunning experience!");
	/*
	 * Attack-Specific checks.
	 */
	DOCHECKMA(AttackType == PA_PUNCH &&
			  (MechType(target) == CLASS_VEH_GROUND) &&
			  !Fallen(mech),
			  "You can't punch vehicles unless you are prone!");

	// As per BMR, can only trip mechs.
	DOCHECKMA(AttackType == PA_TRIP && MechType(target) != CLASS_MECH,
			  "You can only trip mechs!");

	// Can't trip mechs that are fallen or in the process of standing.
	DOCHECKMA(AttackType == PA_TRIP && (Fallen(target) || Standing(target)),
			  "Your target is already down!");

	// We're attacking a ground/naval unit.    
	if(MechMove(target) != MOVE_VTOL && MechMove(target) != MOVE_FLY) {

		if((AttackType != PA_KICK && AttackType != PA_TRIP) &&
		   (MechZ(mech) >= MechZ(target))) {
			int isTooLow = 0;	// Track whether we're too low or not.

			// If it's a fallen mech, too low.
			if(MechType(target) == CLASS_MECH && Fallen(target))
				isTooLow = 1;

            /* Target is to low to punch */
            if ((MechType(target) == CLASS_MECH) && 
                    (MechZ(mech) > MechZ(target)) && 
                    (AttackType == PA_PUNCH)) {
                isTooLow = 1;
            }

			// If it's not a mech, bsuit, or DS, too low.
			if(MechType(target) != CLASS_MECH &&
			   MechType(target) != CLASS_BSUIT && !IsDS(target))
				isTooLow = 1;

			// If it's a ground vehicle and we're fallen, then we can
			// punch as per BMR.
			if(AttackType == PA_PUNCH &&
			   MechType(target) == CLASS_VEH_GROUND && Fallen(mech))
				isTooLow = 0;

			// If it's a suit that's not on us, we can't physical it.
			if(MechType(target) == CLASS_BSUIT && MechSwarmTarget(target) > 0) {
				mech_printf(mech, MECHALL,
							"You can't directly physical suits that are swarmed or mounted on another mech!");
				return;
			}					// end if() - Disallow physicals on swarmed/mounted suits.

			if(isTooLow == 1) {
				mech_printf(mech, MECHALL,
							"The target is too low in elevation for you to %s.",
							phys_form(AttackType, 0));
				return;
			}					// end if() - Check isTooLow
		}						// end if() - Target is too low checks.

		DOCHECKMA((AttackType == PA_KICK || AttackType == PA_TRIP) &&
				  MechZ(mech) < MechZ(target),
				  "The target is too high in elevation for you to kick at.");

		DOCHECKMA(MechZ(mech) - MechZ(target) > 1 ||
				  MechZ(target) - MechZ(mech) > 1,
				  "You can't attack, the elevation difference is too large.");

		DOCHECKMA((AttackType == PA_KICK || AttackType == PA_TRIP) &&
				  (MechZ(target) < MechZ(mech) &&
				   (((MechType(target) == CLASS_MECH) && Fallen(target)) ||
					(MechType(target) == CLASS_VEH_GROUND) ||
					(MechType(target) == CLASS_BSUIT) ||
					(MechType(target) == CLASS_MW))),
				  "The target is too low in elevation for you to kick.")

	} else {					// We're attacking a VTOL/Aero.

		if((AttackType != PA_KICK) && MechZ(target) - MechZ(mech) > 3) {
			mech_printf(mech, MECHALL,
						"The target is too far away for you to %s.",
						phys_form(AttackType, 0));
		}

		if((AttackType == PA_KICK || AttackType == PA_TRIP) &&
		   MechZ(mech) != MechZ(target)) {
			mech_printf(mech, MECHALL,
						"The target is too far away for you to %s.",
						phys_form(AttackType, 0));
			return;
		}

		DOCHECKMA(!(MechZ(target) - MechZ(mech) > -1 &&
					MechZ(target) - MechZ(mech) < 4),
				  "You can't attack, the elevation difference is too large.");
	}							// end if/else() - Ground/VTOL + Physical Z comparisons

	/* Check weapon arc! */
	/* Theoretically, physical attacks occur only to 'real' forward
	   arc, not rottorsoed one, but we let it pass this time */
	/* This is wrong according to BMR 
	 *
	 * Which states that the Torso twist is taken into account
	 * as well as punching/axing/swords can attack in their
	 * respective arcs - Dany
	 *
	 * So I went and changed it according to FASA rules */
	if(AttackType == PA_KICK || AttackType == PA_TRIP) {

		ts = MechStatus(mech) & (TORSO_LEFT | TORSO_RIGHT);
		MechStatus(mech) &= ~ts;
		iwa = InWeaponArc(mech, MechFX(target), MechFY(target));
		MechStatus(mech) |= ts;

		DOCHECKMA(!(iwa & FORWARDARC),
				  "Target is not in your 'real' forward arc!");

	} else {					// We're punching, clubbing, or other sharp things.

		iwa = InWeaponArc(mech, MechFX(target), MechFY(target));

		if(AttackType == PA_CLUB) {
			// Clubs are a frontal attack. Go off of the forward arc, don't
			// take arms into account.
			DOCHECKMA(!(iwa & FORWARDARC) && swarmingUs != 1,
					  "Target is not in your forward arc!");
		} else {
			// For other attacks, check on a per-arm basis.
			if(sect == RARM) {
				// We're attacking with right arm. Forward or right will do.
				DOCHECKMA(!
						  ((iwa & FORWARDARC) || (iwa & RSIDEARC)
						   || swarmingUs),
						  "Target is not in your forward or right side arc!");
			} else {
				// We're attacking with left arm. Forward or left will do.
				DOCHECKMA(!((iwa & FORWARDARC) || (iwa & LSIDEARC))
						  || swarmingUs,
						  "Target is not in your forward or left side arc!");

			}					// end 

		}						// end if/else() - club/punch arc check

	}							// end if/else() - kick/punch arc check

	/**
     * Add in the movement modifiers 
     */

	// If we have melee_specialist advantage, knock -1 off the BTH.
	baseToHit += HasBoolAdvantage(MechPilot(mech), "melee_specialist") ?
		MIN(0, AttackMovementMods(mech) - 1) : AttackMovementMods(mech);

	baseToHit += TargetMovementMods(mech, target, 0.0);

	// BSuits get +1 BTH
	baseToHit += MechType(target) == CLASS_BSUIT ? 1 : 0;

	// Kicking a BSuit is +3 BTH
	baseToHit += ((MechType(target) == CLASS_BSUIT) &&
				  (AttackType == PA_KICK)) ? 3 : 0;

#ifdef BT_MOVEMENT_MODES
	// A dodging unit is +2, requires maneuvering_ace.
	if(Dodging(target))
		baseToHit += 2;
#endif

	// Saws get a +1 BTH.
	if(AttackType == PA_SAW)
		baseToHit += 1;

	// Maces get a +2 BTH.
	if(AttackType == PA_MACE)
		baseToHit += 2;

	// Claws get a +1 BTH.
	if(AttackType == PA_CLAW)
		baseToHit += 1;

	// If we're axing or chopping a bsuit, add +3 to BTH, else (punching) +5.
	if(AttackType != PA_PUNCH &&
	   MechType(target) == CLASS_BSUIT && MechSwarmTarget(target) > 0)
		baseToHit += (AttackType != PA_PUNCH) ? 3 : 5;

	// As per BMR, can only physical bsuits with punches, axes, or swords.
	// Added saw since it's the same idea.
	DOCHECKMA(AttackType == PA_KICK &&
			  MechType(target) == CLASS_BSUIT &&
			  MechSwarmTarget(target) > 0,
			  "You can't hit a swarmed suit with that, try a hand-held weapon!");

    // Terrain mods - Courtesy of RST
    // Heavy & Light are from Total Warfare and
    // Smoke from MaxTech old BMR 
    // Check Smoke first since it can sit on top of other terrain
    // Might want to check for Fire also at some point?
    if (MechTerrain(target) == SMOKE) {
        baseToHit += 2;
    } else if (MechRTerrain(target) == HEAVY_FOREST) {
        baseToHit += 2;
    } else if (MechRTerrain(target) == LIGHT_FOREST) {
        baseToHit += 1;
    }

	roll = Roll();

	// Carry out the attack.
	mech_printf(mech, MECHALL,
				"You try to %s %s.  BTH:  %d,\tRoll:  %d",
				phys_form(AttackType, 0),
				GetMechToMechID(mech, target), baseToHit, roll);

	mech_printf(target, MECHSTARTED, "%s tries to %s you!",
				GetMechToMechID(target, mech), phys_form(AttackType, 0));

	// We send to MechAttacks channel
	SendAttacks(tprintf("#%i attacks #%i (%s) (%i/%i)",
						mech->mynum,
						target->mynum,
						phys_form(AttackType, 0), baseToHit, roll));

	// Set the appropriate section(s) to recycle.
	SetRecycleLimb(mech, sect, PHYSICAL_RECYCLE_TIME);

	/*
	 * Attack-specific recycles and flags.
	 */
	if(AttackType == PA_AXE || AttackType == PA_SWORD || AttackType == PA_SAW
	   || AttackType == PA_MACE)
		MechSections(mech)[sect].config |= AXED;

	if(AttackType == PA_PUNCH)
		MechSections(mech)[sect].config &= ~AXED;

	// Clubbing recycles both arms. 
	if(AttackType == PA_CLUB)
		SetRecycleLimb(mech, LARM, PHYSICAL_RECYCLE_TIME);

	RbaseToHit = baseToHit;
	if(mudconf.btech_glancing_blows == 2)
		RbaseToHit = baseToHit - 1;
	// We've successfully hit the target.
	if(roll >= RbaseToHit) {
		phys_succeed(mech, target, AttackType);
		if (mudconf.btech_glancing_blows && (roll == RbaseToHit)) {
			MechLOSBroadcast(target,"is nicked by a glancing blow!");
			mech_notify(target, MECHALL, "You are nicked by a glancing blow!");
			glance = 1 ;
		}
		if(AttackType == PA_CLUB) {
			int clubLoc = -1;

			if(MechSections(mech)[RARM].specials & CARRYING_CLUB)
				clubLoc = RARM;
			else if(MechSections(mech)[LARM].specials & CARRYING_CLUB)
				clubLoc = LARM;

			if(clubLoc > -1) {
				mech_notify(mech, MECHALL, "Your club shatters on contact.");
				MechLOSBroadcast(mech,
								 "'s club shatters with a loud *CRACK*!");

				MechSections(mech)[clubLoc].specials &= ~CARRYING_CLUB;
			}
		}						// End if() - Club shattering

		// Do the deed - Damage the victim. If we're tripping, we don't do
		// damage but try to make a skill roll.
		if(AttackType != PA_TRIP)
			PhysicalDamage(mech, target, damageweight, AttackType, sect, glance);
		else
			PhysicalTrip(mech, target);

	} else {					// We have failed!
		phys_fail(mech, target, AttackType);

		if(MechType(target) == CLASS_BSUIT &&
		   MechSwarmTarget(target) == mech->mynum) {

			if(!MadePilotSkillRoll(mech, 4)) {
				mech_notify(mech, MECHALL,
							"Uh oh. You miss the little buggers, but hit yourself!");
				MechLOSBroadcast(mech, "misses, and hits itself!");

				PhysicalDamage(mech, mech, damageweight, AttackType, sect, glance);
			}					// If we really screw up against suits swarmed on ourselves,
			// nail us for damage.
		}						// end if() - Suit + Swarmed + Physical + Self Damage checks

/* Removed fall check for clubs -- Power_Shaper 09/25/06 */
		if(AttackType == PA_KICK ||
		   AttackType == PA_MACE) {
			int failRoll = (AttackType == PA_KICK ? 0 : 2);

			mech_notify(mech, MECHALL,
						"You miss and try to remain standing!");

			// We fail the piloting skill roll and flop on our face.
			if(!MadePilotSkillRoll(mech, failRoll)) {
				mech_notify(mech, MECHALL,
							"You lose your balance and fall down!");
				MechFalls(mech, 1, 1);
			}					// end if() - Miss/fall.
		}						// end if() - Miss kick/club and risk falling.
	}							// end if() - Physical failure handling.
}								//end PhysicalAttack()

extern int global_physical_flag;

#define MyDamageMech(a,b,c,d,e,f,g,h,i) \
        global_physical_flag = 1 ; DamageMech(a,b,c,d,e,f,g,h,i,-1,0,-1,0,0); \
        global_physical_flag = 0
#define MyDamageMech2(a,b,c,d,e,f,g,h,i) \
        global_physical_flag = 2 ; DamageMech(a,b,c,d,e,f,g,h,i,-1,0,-1,0,0); \
        global_physical_flag = 0

/*
 * Try to trip the victim.
 */
void PhysicalTrip(MECH * mech, MECH * target)
{
	// If we trip our target (who is a mech), make a roll to see if he falls.
	if(!MadePilotSkillRoll(target, 0) && !Fallen(target)) {

		// Emit to Attacker
		mech_printf(mech, MECHALL,
					"You trip %s!", GetMechToMechID(mech, target));

		// Emit to victim and LOS.
		mech_notify(target, MECHSTARTED,
					"You are tripped and fall to the ground!");
		MechLOSBroadcast(target, "trips up and falls down!");

		MechFalls(target, 1, 0);
	} else {
		MechLOSBroadcast(target, "manages to stay upright!");
	}
}								// end PhysicalTrip()

/*
 * Damage the victim.
 */
void PhysicalDamage(MECH * mech, MECH * target, int weightdmg,
        int AttackType, int sect, int glance) {

    int hitloc = 0, damage, hitgroup = 0, isrear, iscritical, i;

    isrear = 0;
    iscritical = 0;

    /* Two types of physical attack weapons - Those affected by TSM
     * and those not - Right now just saw but can add more to the list via
     * || (AttackType == PA_BLAH) */
    if (AttackType == PA_SAW) {

        /* Saws do a constant 7 damage due to their mechanical nature. */
        damage = 7;

    } else {

        /* Sword attack uses an odd weapon damage amount */
        if (AttackType == PA_SWORD) {
            damage = (MechTons(mech) + 5) / weightdmg + 1;
        } else {
	/* Round Down to nearest ton -- TW Page 145 */
	    damage = (int) floor((((float) MechTons(mech)) / weightdmg));
        }

        /* Calc in affect by TSM */
        if ((MechHeat(mech) >= 9.) && (MechSpecials(mech) & TRIPLE_MYOMER_TECH)) {
            damage = damage * 2;
        }

    }

    /* If we have melee_specialist, add a point of damage. */
    if (HasBoolAdvantage(MechPilot(mech), "melee_specialist")) {
        damage++;
    }

    switch (AttackType) {
        case PA_PUNCH:

            if (!OkayCritSectS(sect, 2, LOWER_ACTUATOR)) {
                damage = damage / 2;
            }

            if (!OkayCritSectS(sect, 1, UPPER_ACTUATOR)) {
                damage = damage / 2;
            }

            hitgroup = FindAreaHitGroup(mech, target);
            if (hitgroup == BACK) {
                isrear = 1;
            }

            if (MechType(mech) == CLASS_MECH) {

                if (Fallen(mech)) {

                    /* Total Warfare page 151 - Prone mechs can only make
                     * two types of physical attacks - Punching (with one arm)
                     * vehicles in same hex and thrashing - But for now including
                     * this. - Dany 01/2007 */
                    if ((MechType(target) != CLASS_MECH) || (Fallen(target) &&
                                (MechElevation(mech) == MechElevation (target)))) {
                        hitloc = FindTargetHitLoc(mech, target, &isrear, &iscritical);
                    } else if (!Fallen(target) && 
                            (MechElevation(mech) > MechElevation(target))) {
                        hitloc = FindPunchLocation(target, hitgroup);
                    } else if (MechElevation(mech) == MechElevation(target)) {
                        hitloc = FindKickLocation(target, hitgroup);
                    }

                } else if (MechElevation(mech) < MechElevation(target)) {

                    if (Fallen(target) || MechType(target) != CLASS_MECH) {
                        hitloc = FindTargetHitLoc(mech, target, &isrear, &iscritical);
                    } else {
                        hitloc = FindKickLocation(target, hitgroup);
                    }

                } else {
                    hitloc = FindPunchLocation(target, hitgroup);
                }

            } else {
                hitloc = FindTargetHitLoc(mech, target, &isrear, &iscritical);
            }

            break;

        case PA_SWORD:
        case PA_AXE:
        case PA_MACE:
        case PA_CLUB:

            hitgroup = FindAreaHitGroup(mech, target);
            if (hitgroup == BACK) {
                isrear = 1;
            }

            if (MechType(mech) == CLASS_MECH) {

                if (MechElevation(mech) < MechElevation(target)) {

                    if (Fallen(target) || MechType(target) != CLASS_MECH) {
                        hitloc = FindTargetHitLoc(mech, target, &isrear, &iscritical);
                    } else {
                        hitloc = FindKickLocation(target, hitgroup);
                    }

                } else if (MechElevation(mech) > MechElevation(target)) {
                    hitloc = FindPunchLocation(target, hitgroup);
                } else {
                    hitloc = FindTargetHitLoc(mech, target, &isrear, &iscritical);
                }

            } else {
                hitloc = FindTargetHitLoc(mech, target, &isrear, &iscritical);
            }
            break;

        case PA_KICK:

            if (!OkayCritSectS(sect, 2, LOWER_ACTUATOR))
                damage = damage / 2;

            if (!OkayCritSectS(sect, 1, UPPER_ACTUATOR))
                damage = damage / 2;

            if (Fallen(target) || MechType(target) != CLASS_MECH) {
                hitloc = FindTargetHitLoc(mech, target, &isrear, &iscritical);
            } else {

                hitgroup = FindAreaHitGroup(mech, target);
                if (hitgroup == BACK) {
                    isrear = 1;
                }

                if (MechElevation(mech) > MechElevation(target)) {
                    hitloc = FindPunchLocation(target, hitgroup);
                } else {
                    hitloc = FindKickLocation(target, hitgroup);
                }
            }
            break;

    }

    if (glance) {
        damage = (damage + 1) / 2;
    }

    // Damage the target.
    MyDamageMech(target, mech, 1, MechPilot(mech), hitloc,
            isrear, iscritical, damage, 0);

    // If we've successfully hit a suit, knock him off.
    if (MechType(target) == CLASS_BSUIT && MechSwarmTarget(target) > 0 &&
            AttackType != PA_KICK) {
        StopSwarming(target, 0);
    }

    // If we kick our target (who is a mech), make a roll to see if he falls.
    if (MechType(target) == CLASS_MECH && AttackType == PA_KICK) {
        if (!MadePilotSkillRoll(target, 0) && !Fallen(target)) {
            mech_notify(target, MECHSTARTED,
                    "The kick knocks you to the ground!");
            MechLOSBroadcast(target, "stumbles and falls down!");
            MechFalls(target, 1, 0);
        }
    }

} // end PhysicalDamage()

#define CHARGE_SECTIONS 6
#define DFA_SECTIONS    6
/* Rules make no distinction about Torso not needing recycled  We'll let Head slide for now */


const int resect[CHARGE_SECTIONS] =
	{ LARM, RARM, LLEG, RLEG, LTORSO, RTORSO };

/*
 * Executed at the end of a DFA
 */
int DeathFromAbove(MECH * mech, MECH * target)
{
	int baseToHit = 5;
	int roll;
	int hitGroup;
	int hitloc;
	int isrear = 0;
	int iscritical = 0;
	int target_damage;
	int mech_damage;
	int spread;
	int i, tmpi;
	char location[50];
	MAP *map = getMap(mech->mapindex);

	/* Weapons recycling check on each major section */
	for(i = 0; i < DFA_SECTIONS; i++)
		if(SectHasBusyWeap(mech, resect[i])) {
			ArmorStringFromIndex(resect[i], location, MechType(mech),
								 MechMove(mech));
			mech_printf(mech, MECHALL,
						"You have weapons recycling on your %s.", location);
			return 0;
		}
	// Our target is no longer on the map.
	DOCHECKMA0((mech->mapindex != target->mapindex),
			   "Your target is no longer valid.");

#ifdef BT_MOVEMENT_MODES
	DOCHECKMA0(Dodging(mech) || MoveModeLock(mech),
			   "You cannot use physicals while using a special movement mode.");
#endif

	DOCHECKMA0(MechSections(mech)[LLEG].recycle ||
			   MechSections(mech)[RLEG].recycle,
			   "Your legs are still recovering from your last attack.");
	DOCHECKMA0(MechSections(mech)[RARM].recycle ||
			   MechSections(mech)[LARM].recycle,
			   "Your arms are still recovering from your last attack.");

	DOCHECKMA0(Jumping(target),
			   "Your target is airborne, you cannot land on it.");

	if((MechType(target) == CLASS_VTOL) || (MechType(target) == CLASS_AERO) ||
	   (MechType(target) == CLASS_DS))
		DOCHECKMA0(!Landed(target),
				   "Your target is airborne, you cannot land on it.");

	DOCHECKMA0((MechTeam(mech) == MechTeam(target)) && MapNoFriendlyFire(map),
			"Friendly DFA? I don't think so....");
	if(mudconf.btech_phys_use_pskill)
		baseToHit = FindPilotPiloting(mech);

	baseToHit += (HasBoolAdvantage(MechPilot(mech), "melee_specialist") ?
				  MIN(0,
					  AttackMovementMods(mech)) -
				  1 : AttackMovementMods(mech));
	baseToHit += TargetMovementMods(mech, target, 0.0);
	baseToHit += MechType(target) == CLASS_BSUIT ? 1 : 0;

#ifdef BT_MOVEMENT_MODES
	if(Dodging(target))
		baseToHit += 2;
#endif

	DOCHECKMA0(baseToHit > 12,
			   tprintf
			   ("DFA: BTH %d\tYou choose not to attack and land from your jump.",
				baseToHit));

	roll = Roll();
	mech_printf(mech, MECHALL, "DFA: BTH %d\tRoll: %d", baseToHit, roll);

	MechStatus(mech) &= ~JUMPING;
	MechStatus(mech) &= ~DFA_ATTACK;

	if(roll >= baseToHit) {
		/* OUCH */
		mech_printf(target, MECHSTARTED,
					"DEATH FROM ABOVE!!!\n%s lands on you from above!",
					GetMechToMechID(target, mech));
		mech_notify(mech, MECHALL, "You land on your target legs first!");
		MechLOSBroadcasti(mech, target, "lands on %s!");

		hitGroup = FindAreaHitGroup(mech, target);
		if(hitGroup == BACK)
			isrear = 1;

		target_damage = (3 * MechRealTons(mech)) / 10;

		if(MechTons(mech) % 10)
			target_damage++;

		if(HasBoolAdvantage(MechPilot(mech), "melee_specialist"))
			target_damage++;

		spread = target_damage / 5;

		for(i = 0; i < spread; i++) {
			if(Fallen(target) || MechType(target) != CLASS_MECH)
				hitloc =
					FindHitLocation(target, hitGroup, &iscritical, &isrear);
			else
				hitloc = FindPunchLocation(target, hitGroup);

			MyDamageMech(target, mech, 1, MechPilot(mech), hitloc, isrear,
						 iscritical, 5, 0);
		}

		if(target_damage % 5) {
			if(Fallen(target) || (MechType(target) != CLASS_MECH))
				hitloc =
					FindHitLocation(target, hitGroup, &iscritical, &isrear);
			else
				hitloc = FindPunchLocation(target, hitGroup);

			MyDamageMech(target, mech, 1, MechPilot(mech), hitloc, isrear,
						 iscritical, (target_damage % 5), 0);
		}

		mech_damage = MechTons(mech) / 5;

		spread = mech_damage / 5;

		for(i = 0; i < spread; i++) {
			hitloc = FindKickLocation(mech, FRONT);
			MyDamageMech2(mech, mech, 0, -1, hitloc, 0, 0, 5, 0);
		}

		if(mech_damage % 5) {
			hitloc = FindKickLocation(mech, FRONT);
			MyDamageMech2(mech, mech, 0, -1, hitloc, 0, 0,
						  (mech_damage % 5), 0);
		}

		if(!Fallen(mech)) {
			if(!MadePilotSkillRoll(mech, 4)) {
				mech_notify(mech, MECHALL,
							"Your piloting skill fails and you fall over!!");
				MechLOSBroadcast(mech, "stumbles and falls down!");
				MechFalls(mech, 1, 0);
			}
			if(MechType(target) == CLASS_MECH &&
			   !MadePilotSkillRoll(target, 2)) {
				mech_notify(target, MECHSTARTED,
							"Your piloting skill fails and you fall over!!");
				MechLOSBroadcast(target, "stumbles and falls down!");
				MechFalls(target, 1, 0);
			}
		}

	} else {
		/* Missed DFA attack */
		if(!Fallen(mech)) {
			mech_notify(mech, MECHALL,
						"You miss your DFA attack and fall on your back!!");
			MechLOSBroadcast(mech, "misses DFA and falls down!");
		}

		mech_damage = MechTons(mech) / 5;
		spread = mech_damage / 5;

		for(i = 0; i < spread; i++) {
			hitloc = FindHitLocation(mech, BACK, &iscritical, &tmpi);
			MyDamageMech2(mech, mech, 0, -1, hitloc, 1, iscritical, 5, 0);
		}

		if(mech_damage % 5) {
			hitloc = FindHitLocation(mech, BACK, &iscritical, &tmpi);
			MyDamageMech2(mech, mech, 0, -1, hitloc, 1, iscritical,
						  (mech_damage % 5), 0);
		}

		/* now damage pilot */
		if(!MadePilotSkillRoll(mech, 2)) {
			mech_notify(mech, MECHALL,
						"You take personal injury from the fall!");
			headhitmwdamage(mech, mech, 1);
		}

		MechSpeed(mech) = 0.0;
		MechDesiredSpeed(mech) = 0.0;

		MakeMechFall(mech);
		MechZ(mech) = MechElevation(mech);
		MechFZ(mech) = MechZ(mech) * ZSCALE;

		if(MechZ(mech) < 0)
			MechFloods(mech);

	}

	for(i = 0; i < DFA_SECTIONS; i++)
		SetRecycleLimb(mech, resect[i], PHYSICAL_RECYCLE_TIME);

	return 1;
}								// end DeathFromAbove()

/*
 * Executed when we're ready to finish the charge.
 */
void ChargeMech(MECH * mech, MECH * target)
{
	int baseToHit = 5;
	int roll;
	int hitGroup;
	int hitloc;
	int isrear = 0;
	int iscritical = 0;
	int target_damage;
	int mech_damage;
	int received_damage;
	int inflicted_damage;
	int spread;
	int i;
	int mech_charge;
	int target_charge;
	int mech_baseToHit;
	int targ_baseToHit;
	int mech_roll;
	int targ_roll;
	int done = 0;
	char location[50];
	int ts, iwa;
	char emit_buff[LBUF_SIZE];

	/* Are they both charging ? */
	if(MechChargeTarget(target) == mech->mynum) {
		/* They are both charging each other */
		mech_charge = 1;
		target_charge = 1;

		/* Check the sections of the first unit for weapons that are cycling */
		done = 0;
		for(i = 0; i < CHARGE_SECTIONS && !done; i++) {
			if(SectHasBusyWeap(mech, resect[i])) {
				ArmorStringFromIndex(resect[i], location, MechType(mech),
									 MechMove(mech));
				mech_printf(mech, MECHALL,
							"You have weapons recycling on your %s.",
							location);
				mech_charge = 0;
				done = 1;
			}
		}

		/* Check the sections of the second unit for weapons that are cycling */
		done = 0;
		for(i = 0; i < CHARGE_SECTIONS && !done; i++) {
			if(SectHasBusyWeap(target, resect[i])) {
				ArmorStringFromIndex(resect[i], location, MechType(target),
									 MechMove(target));
				mech_printf(target, MECHALL,
							"You have weapons recycling on your %s.",
							location);
				target_charge = 0;
				done = 1;
			}
		}

		/* Is the second unit capable of charging */
		if(!Started(target) || Uncon(target) || Blinded(target))
			target_charge = 0;
		/* Is the first unit capable of charging */
		if(!Started(mech) || Uncon(mech) || Blinded(mech))
			mech_charge = 0;

		/* Is the first unit moving fast enough to charge */
		if(MechSpeed(mech) < MP1) {
			mech_notify(mech, MECHALL,
						"You aren't moving fast enough to charge.");
			mech_charge = 0;
		}

		/* Is the second unit moving fast enough to charge */
		if(MechSpeed(target) < MP1) {
			mech_notify(target, MECHALL,
						"You aren't moving fast enough to charge.");
			target_charge = 0;
		}

		/* Check to see if any sections cycling from a previous attack */
		if(MechType(mech) == CLASS_MECH) {
			/* Is the first unit's legs cycling */
			if(MechSections(mech)[LLEG].recycle ||
			   MechSections(mech)[RLEG].recycle) {
				mech_notify(mech, MECHALL,
							"Your legs are still recovering from your last attack.");
				mech_charge = 0;
			}
			/* Is the first unit's arms cycling */
			if(MechSections(mech)[RARM].recycle ||
			   MechSections(mech)[LARM].recycle) {
				mech_notify(mech, MECHALL,
							"Your arms are still recovering from your last attack.");
				mech_charge = 0;
			}
		} else {
			/* Is the first unit's front side cycling */
			if(MechSections(mech)[FSIDE].recycle) {
				mech_notify(mech, MECHALL,
							"You are still recovering from your last attack!");
				mech_charge = 0;
			}
		}

		/* Check to see if any sections cycling from a previous attack */
		if(MechType(target) == CLASS_MECH) {
			/* Is the second unit's legs cycling */
			if(MechSections(target)[LLEG].recycle ||
			   MechSections(target)[RLEG].recycle) {
				mech_notify(target, MECHALL,
							"Your legs are still recovering from your last attack.");
				target_charge = 0;
			}
			/* Is the second unit's arms cycling */
			if(MechSections(target)[RARM].recycle ||
			   MechSections(target)[LARM].recycle) {
				mech_notify(target, MECHALL,
							"Your arms are still recovering from your last attack.");
				target_charge = 0;
			}
		} else {
			/* Is the second unit's front side cycling */
			if(MechSections(target)[FSIDE].recycle) {
				mech_notify(target, MECHALL,
							"You are still recovering from your last attack!");
				target_charge = 0;
			}
		}

		/* Is the second unit jumping */
		if(Jumping(target)) {
			mech_notify(mech, MECHALL,
						"Your target is jumping, you charge underneath it.");
			mech_notify(target, MECHALL,
						"You can't charge while jumping, try death from above.");
			mech_charge = 0;
			target_charge = 0;
		}

		/* Is the first unit jumping */
		if(Jumping(mech)) {
			mech_notify(target, MECHALL,
						"Your target is jumping, you charge underneath it.");
			mech_notify(mech, MECHALL,
						"You can't charge while jumping, try death from above.");
			mech_charge = 0;
			target_charge = 0;
		}

		/* Is the second unit fallen and the first unit not a tank */
		if(Fallen(target) && (MechType(mech) != CLASS_VEH_GROUND)) {
			mech_notify(mech, MECHALL,
						"Your target's too low for you to charge it!");
			mech_charge = 0;
		}

		/* Not sure at the moment if I need this here, but I figured
		 * couldn't hurt for now */
		/* Is the first unit fallen and the second unit not a tank */
		if(Fallen(mech) && (MechType(target) != CLASS_VEH_GROUND)) {
			mech_notify(target, MECHALL,
						"Your target's too low for you to charge it!");
			target_charge = 0;
		}

		/* If the second unit is a mech it can only charge mechs */
		if((MechType(target) == CLASS_MECH) && (MechType(mech) != CLASS_MECH)) {
			mech_notify(target, MECHALL, "You can only charge mechs!");
			target_charge = 0;
		}

		/* If the first unit is a mech it can only charge mechs */
		if((MechType(mech) == CLASS_MECH) && (MechType(target) != CLASS_MECH)) {
			mech_notify(mech, MECHALL, "You can only charge mechs!");
			mech_charge = 0;
		}

		/* If the second unit is a tank, it can only charge tanks and mechs */
		if((MechType(target) == CLASS_VEH_GROUND) &&
		   ((MechType(mech) != CLASS_MECH) &&
			(MechType(mech) != CLASS_VEH_GROUND))) {
			mech_notify(target, MECHALL,
						"You can only charge mechs and tanks!");
			target_charge = 0;
		}

		/* If the first unit is a tank, it can only charge tanks and mechs */
		if((MechType(mech) == CLASS_VEH_GROUND) &&
		   ((MechType(target) != CLASS_MECH) &&
			(MechType(target) != CLASS_VEH_GROUND))) {
			mech_notify(mech, MECHALL,
						"You can only charge mechs and tanks!");
			mech_charge = 0;
		}

		/* Are they stunned ? */
		if(CrewStunned(mech)) {
			mech_notify(mech, MECHALL, "You are too stunned to ram!");
			mech_charge = 0;
		}

		if(CrewStunned(target)) {
			mech_notify(target, MECHALL, "You are too stunned to ram!");
			target_charge = 0;
		}

		/* Are they trying to unjam their turrets ? */
		if(UnjammingTurret(mech)) {
			mech_notify(mech, MECHALL,
						"You are too busy unjamming your turret!");
			mech_charge = 0;
		}

		if(UnjammingTurret(target)) {
			mech_notify(mech, MECHALL,
						"You are too busy unjamming your turret!");
			target_charge = 0;
		}

		/* Check the arcs to make sure the target is in the front arc */
		ts = MechStatus(mech) & (TORSO_LEFT | TORSO_RIGHT);
		MechStatus(mech) &= ~ts;
		if(!(InWeaponArc(mech, MechFX(target), MechFY(target)) & FORWARDARC)) {
			mech_notify(mech, MECHALL,
						"Your charge target is not in your forward arc and you are unable to charge it.");
			mech_charge = 0;
		}
		MechStatus(mech) |= ts;

		ts = MechStatus(target) & (TORSO_LEFT | TORSO_RIGHT);
		MechStatus(mech) &= ~ts;
		if(!(InWeaponArc(target, MechFX(mech), MechFY(mech)) & FORWARDARC)) {
			mech_notify(target, MECHALL,
						"Your charge target is not in your forward arc and you are unable to charge it.");
			target_charge = 0;
		}
		MechStatus(mech) |= ts;

		/* Now to calculate how much damage the first unit will do */
		if(mudconf.btech_newcharge)
			target_damage = (((((float)
								MechChargeDistance(mech)) * MP1) -
							  MechSpeed(target) * cos((MechFacing(mech) -
													   MechFacing(target)) *
													  (M_PI / 180.))) *
							 MP_PER_KPH) * (MechRealTons(mech) + 5) / 10;
		else
			target_damage =
				((MechSpeed(mech) -
				  MechSpeed(target) * cos((MechFacing(mech) -
										   MechFacing(target)) * (M_PI /
																  180.))) *
				 MP_PER_KPH) * (MechRealTons(mech) + 5) / 10;

		if(HasBoolAdvantage(MechPilot(mech), "melee_specialist"))
			target_damage++;

		/* Not able to do any damage */
		if(target_damage <= 0) {
			mech_notify(mech, MECHPILOT,
						"Your target unit will not sustain any damage. Charge aborted!");
			mech_charge = 0;
		}

		/* Now see how much damage the second unit will do */
		mech_damage = (MechRealTons(target) + 5) /10;

		if(HasBoolAdvantage(MechPilot(target), "melee_specialist"))
			mech_damage++;

		/* Not able to do any damage */
		if(mech_damage <= 0) {
			mech_notify(target, MECHPILOT,
						"Your unit won't sustain any dmage. Charge aborted!");
			target_charge = 0;
		}

		/* BTH for first unit */
		mech_baseToHit = 5;
		mech_baseToHit +=
			FindPilotPiloting(mech) - FindPilotPiloting(target);

		mech_baseToHit +=
			(HasBoolAdvantage(MechPilot(mech), "melee_specialist") ?
			 MIN(0, AttackMovementMods(mech) - 1) : AttackMovementMods(mech));

		mech_baseToHit += TargetMovementMods(mech, target, 0.0);

#ifdef BT_MOVEMENT_MODES
		if(Dodging(target))
			mech_baseToHit += 2;
#endif

		/* BTH for second unit */
		targ_baseToHit = 5;
		targ_baseToHit +=
			FindPilotPiloting(target) - FindPilotPiloting(mech);

		targ_baseToHit +=
			(HasBoolAdvantage(MechPilot(target), "melee_specialist") ?
			 MIN(0,
				 AttackMovementMods(target) -
				 1) : AttackMovementMods(target));

		targ_baseToHit += TargetMovementMods(target, mech, 0.0);

#ifdef BT_MOVEMENT_MODES
		if(Dodging(mech))
			targ_baseToHit += 2;
#endif

		/* Now check to see if its possible for them to even charge */
		if(mech_charge)
			if(mech_baseToHit > 12) {
				mech_printf(mech, MECHALL,
							"Charge: BTH %d\tYou choose not to charge.",
							mech_baseToHit);
				mech_charge = 0;
			}

		if(target_charge)
			if(targ_baseToHit > 12) {
				mech_printf(target, MECHALL,
							"Charge: BTH %d\tYou choose not to charge.",
							targ_baseToHit);
				target_charge = 0;
			}

		/* Since neither can charge lets exit */
		if(!mech_charge && !target_charge) {
			/* MechChargeTarget(mech) and the others are set
			   after the return */
			MechChargeTarget(target) = -1;
			MechChargeTimer(target) = 0;
			MechChargeDistance(target) = 0;
			return;
		}

		/* Roll */
		mech_roll = Roll();
		targ_roll = Roll();

		if(mech_charge)
			mech_printf(mech, MECHALL,
						"Charge: BTH %d\tRoll: %d", mech_baseToHit,
						mech_roll);

		if(target_charge)
			mech_printf(target, MECHALL,
						"Charge: BTH %d\tRoll: %d", targ_baseToHit,
						targ_roll);

		/* Ok the first unit made its roll */
		if(mech_charge && mech_roll >= mech_baseToHit) {
			/* OUCH */
			mech_printf(target, MECHALL,
						"CRASH!!!\n%s charges into you!",
						GetMechToMechID(target, mech));
			mech_notify(mech, MECHALL,
						"SMASH!!! You crash into your target!");
			hitGroup = FindAreaHitGroup(mech, target);
			isrear = (hitGroup == BACK);

			/* Record the damage for debugging then dish it out */
			inflicted_damage = target_damage;
			spread = target_damage / 5;

			for(i = 0; i < spread; i++) {
				hitloc =
					FindHitLocation(target, hitGroup, &iscritical, &isrear);
				MyDamageMech(target, mech, 1, MechPilot(mech), hitloc,
							 isrear, iscritical, 5, 0);
			}

			if(target_damage % 5) {
				hitloc =
					FindHitLocation(target, hitGroup, &iscritical, &isrear);
				MyDamageMech(target, mech, 1, MechPilot(mech), hitloc,
							 isrear, iscritical, (target_damage % 5), 0);
			}

			hitGroup = FindAreaHitGroup(target, mech);
			isrear = (hitGroup == BACK);

			/* Ok now how much damage will the first unit take from
			 * charging */
			if(mudconf.btech_newcharge && mudconf.btech_tl3_charge)
				target_damage =
					(((((float) MechChargeDistance(mech)) * MP1) -
					  MechSpeed(target) *
					  cos((MechFacing(mech) -
						   MechFacing(target)) * (M_PI / 180.))) *
					 MP_PER_KPH) * (MechRealTons(mech) + 5) / 20;
			else
				target_damage = (MechRealTons(target) + 5) / 10;	/* REUSED! */

			/* Record the damage for debugging then dish it out */
			received_damage = target_damage;
			spread = target_damage / 5;

			for(i = 0; i < spread; i++) {
				hitloc =
					FindHitLocation(mech, hitGroup, &iscritical, &isrear);
				MyDamageMech2(mech, mech, 0, -1, hitloc, isrear,
							  iscritical, 5, 0);
			}

			if(target_damage % 5) {
				hitloc =
					FindHitLocation(mech, hitGroup, &iscritical, &isrear);
				MyDamageMech2(mech, mech, 0, -1, hitloc, isrear,
							  iscritical, (target_damage % 5), 0);
			}

			/* Stop him */
			MechSpeed(mech) = 0;
			MechDesiredSpeed(mech) = 0;

			/* Emit the damage for debugging purposes */
			snprintf(emit_buff, LBUF_SIZE, "#%li charges #%li (%i/%i) Distance:"
					 " %.2f DI: %i DR: %i", mech->mynum, target->mynum,
					 mech_baseToHit, mech_roll, MechChargeDistance(mech),
					 inflicted_damage, received_damage);
			SendDebug(emit_buff);

			/* Make the first unit roll for doing the charge if it is a mech */
			if(MechType(mech) == CLASS_MECH && !MadePilotSkillRoll(mech, 2)) {
				mech_notify(mech, MECHALL,
							"Your piloting skill fails and you fall over!!");
				MechFalls(mech, 1, 1);
			}
			/* Make the second unit roll for receiving the charge if it is a mech */
			if(MechType(mech) == CLASS_MECH && !MadePilotSkillRoll(target, 2)) {
				mech_notify(target, MECHALL,
							"Your piloting skill fails and you fall over!!");
				MechFalls(target, 1, 1);
			}
		}

		/* Ok the second unit made its roll */
		if(target_charge && targ_roll >= targ_baseToHit) {
			/* OUCH */
			mech_printf(mech, MECHALL,
						"CRASH!!!\n%s charges into you!",
						GetMechToMechID(mech, target));
			mech_notify(target, MECHALL,
						"SMASH!!! You crash into your target!");
			hitGroup = FindAreaHitGroup(target, mech);
			isrear = (hitGroup == BACK);

			/* Record the damage for debugging then dish it out */
			inflicted_damage = mech_damage;
			spread = mech_damage / 5;

			for(i = 0; i < spread; i++) {
				hitloc =
					FindHitLocation(mech, hitGroup, &iscritical, &isrear);
				MyDamageMech(mech, target, 1, MechPilot(target), hitloc,
							 isrear, iscritical, 5, 0);
			}

			if(mech_damage % 5) {
				hitloc =
					FindHitLocation(mech, hitGroup, &iscritical, &isrear);
				MyDamageMech(mech, target, 1, MechPilot(target), hitloc,
							 isrear, iscritical, (mech_damage % 5), 0);
			}

			hitGroup = FindAreaHitGroup(mech, target);
			isrear = (hitGroup == BACK);

			/* Ok now how much damage will the second unit take from
			 * charging */
			if(mudconf.btech_newcharge && mudconf.btech_tl3_charge)
				target_damage =
					(((((float) MechChargeDistance(target)) * MP1) -
					  MechSpeed(mech) *
					  cos((MechFacing(target) -
						   MechFacing(mech)) * (M_PI / 180.))) * MP_PER_KPH) *
					(MechRealTons(mech) + 5) / 20;
			else
				target_damage = (MechRealTons(mech) + 5) / 10;	/* REUSED! */

			/* Record the damage for debugging then dish it out */
			received_damage = target_damage;
			spread = target_damage / 5;

			for(i = 0; i < spread; i++) {
				hitloc =
					FindHitLocation(target, hitGroup, &iscritical, &isrear);
				MyDamageMech2(target, target, 0, -1, hitloc, isrear,
							  iscritical, 5, 0);
			}

			if(mech_damage % 5) {
				hitloc =
					FindHitLocation(target, hitGroup, &iscritical, &isrear);
				MyDamageMech2(target, target, 0, -1, hitloc, isrear,
							  iscritical, (mech_damage % 5), 0);
			}

			/* Stop him */
			MechSpeed(target) = 0;
			MechDesiredSpeed(target) = 0;

			/* Emit the damage for debugging purposes */
			snprintf(emit_buff, LBUF_SIZE, "#%li charges #%li (%i/%i) Distance:"
					 " %.2f DI: %i DR: %i", target->mynum, mech->mynum,
					 targ_baseToHit, targ_roll, MechChargeDistance(target),
					 inflicted_damage, received_damage);
			SendDebug(emit_buff);

			if(MechType(mech) == CLASS_MECH && !MadePilotSkillRoll(mech, 2)) {
				mech_notify(mech, MECHALL,
							"Your piloting skill fails and you fall over!!");
				MechFalls(mech, 1, 1);
			}
			if(MechType(target) == CLASS_MECH
			   && !MadePilotSkillRoll(target, 2)) {
				mech_notify(target, MECHALL,
							"Your piloting skill fails and you fall over!!");
				MechFalls(target, 1, 1);
			}
		}

		/* Cycle the sections so they can't make another attack for a while */
		if(MechType(mech) == CLASS_MECH) {
			for(i = 0; i < CHARGE_SECTIONS; i++)
				SetRecycleLimb(mech, resect[i], PHYSICAL_RECYCLE_TIME);
		} else {
			SetRecycleLimb(mech, FSIDE, PHYSICAL_RECYCLE_TIME);
			SetRecycleLimb(mech, TURRET, PHYSICAL_RECYCLE_TIME);
		}

		if(MechType(target) == CLASS_MECH) {
			for(i = 0; i < CHARGE_SECTIONS; i++)
				SetRecycleLimb(target, resect[i], PHYSICAL_RECYCLE_TIME);
		} else {
			SetRecycleLimb(target, FSIDE, PHYSICAL_RECYCLE_TIME);
			SetRecycleLimb(target, TURRET, PHYSICAL_RECYCLE_TIME);
		}

		/* MechChargeTarget(mech) and the others are set
		   after the return */
		MechChargeTarget(target) = -1;
		MechChargeTimer(target) = 0;
		MechChargeDistance(target) = 0;
		return;
	}

	/* Check to see if any weapons cycling in any of the sections */
	for(i = 0; i < CHARGE_SECTIONS; i++) {
		if(SectHasBusyWeap(mech, i)) {
			ArmorStringFromIndex(i, location, MechType(mech), MechMove(mech));
			mech_printf(mech, MECHALL,
						"You have weapons recycling on your %s.", location);
			return;
		}
	}

	/* Check if they going fast enough to charge */
	DOCHECKMA(MechSpeed(mech) < MP1,
			  "You aren't moving fast enough to charge.");

	/* Check to see if their sections cycling */
	if(MechType(mech) == CLASS_MECH) {
		DOCHECKMA(MechSections(mech)[LLEG].recycle ||
				  MechSections(mech)[RLEG].recycle,
				  "Your legs are still recovering from your last attack.");
		DOCHECKMA(MechSections(mech)[RARM].recycle ||
				  MechSections(mech)[LARM].recycle,
				  "Your arms are still recovering from your last attack.");
	} else {
		DOCHECKMA(MechSections(mech)[FSIDE].recycle,
				  "You are still recovering from your last attack!");
	}

	/* See if either the target or the attacker are jumping */
	DOCHECKMA(Jumping(target),
			  "Your target is jumping, you charge underneath it.");
	DOCHECKMA(Jumping(mech),
			  "You can't charge while jumping, try death from above.");

	/* If target is fallen make sure you in a tank */
	DOCHECKMA(Fallen(target) &&
			  (MechType(mech) != CLASS_VEH_GROUND),
			  "Your target's too low for you to charge it!");

	/* Only mechs can charge mechs */
	DOCHECKMA((MechType(mech) == CLASS_MECH) &&
			  (MechType(target) != CLASS_MECH), "You can only charge mechs!");

	/* Only tanks can charge tanks and mechs */
	DOCHECKMA((MechType(mech) == CLASS_VEH_GROUND) &&
			  ((MechType(target) != CLASS_MECH) &&
			   (MechType(target) != CLASS_VEH_GROUND)),
			  "You can only charge mechs and tanks!");

	/* Check the arc make sure target is in front arc */
	ts = MechStatus(mech) & (TORSO_LEFT | TORSO_RIGHT);
	MechStatus(mech) &= ~ts;
	iwa = InWeaponArc(mech, MechFX(target), MechFY(target));
	MechStatus(mech) |= ts;
	DOCHECKMA(!(iwa & FORWARDARC),
			  "Your charge target is not in your forward arc and you are unable to charge it.");

	/* Damage inflicted by the charge */
	if(mudconf.btech_newcharge)
		target_damage = (((((float)
							MechChargeDistance(mech)) * MP1) -
						  MechSpeed(target) * cos((MechFacing(mech) -
												   MechFacing(target)) *
												  (M_PI / 180.))) *
						 MP_PER_KPH) * (MechRealTons(mech) + 5) / 10 + 1;
	else
		target_damage =
			((MechSpeed(mech) - MechSpeed(target) * cos((MechFacing(mech) -
														 MechFacing(target)) *
														(M_PI / 180.))) *
			 MP_PER_KPH) * (MechRealTons(mech) + 5) / 10 + 1;

	if(HasBoolAdvantage(MechPilot(mech), "melee_specialist"))
		target_damage++;

	/* Not enough damage done so no charge */
	DOCHECKMP(target_damage <= 0,
			  "Your target pulls away from you and you are unable to charge it.");

	/* BTH */
	baseToHit += FindPilotPiloting(mech) - FindSPilotPiloting(target);

	baseToHit += (HasBoolAdvantage(MechPilot(mech), "melee_specialist") ?
				  MIN(0,
					  AttackMovementMods(mech) -
					  1) : AttackMovementMods(mech));

	baseToHit += TargetMovementMods(mech, target, 0.0);

#ifdef BT_MOVEMENT_MODES
	if(Dodging(target))
		baseToHit += 2;
#endif

	DOCHECKMA(baseToHit > 12,
			  tprintf("Charge: BTH %d\tYou choose not to charge.",
					  baseToHit));

	/* Roll */
	roll = Roll();
	mech_printf(mech, MECHALL, "Charge: BTH %d\tRoll: %d", baseToHit, roll);

	/* Did the charge work ? */
	if(roll >= baseToHit) {
		/* OUCH */
		MechLOSBroadcasti(mech, target, tprintf("%ss %%s!",
												MechType(mech) ==
												CLASS_MECH ? "charge" :
												"ram"));
		mech_printf(target, MECHSTARTED, "CRASH!!!\n%s %ss into you!",
					GetMechToMechID(target, mech),
					MechType(mech) == CLASS_MECH ? "charge" : "ram");
		mech_notify(mech, MECHALL, "SMASH!!! You crash into your target!");
		hitGroup = FindAreaHitGroup(mech, target);

		if(hitGroup == BACK)
			isrear = 1;
		else
			isrear = 0;

		/* Record the damage then dish it out */
		inflicted_damage = target_damage;
		spread = target_damage / 5;

		for(i = 0; i < spread; i++) {
			hitloc = FindHitLocation(target, hitGroup, &iscritical, &isrear);
			MyDamageMech(target, mech, 1, MechPilot(mech), hitloc, isrear,
						 iscritical, 5, 0);
		}

		if(target_damage % 5) {
			hitloc = FindHitLocation(target, hitGroup, &iscritical, &isrear);
			MyDamageMech(target, mech, 1, MechPilot(mech), hitloc, isrear,
						 iscritical, (target_damage % 5), 0);
		}

		hitGroup = FindAreaHitGroup(target, mech);
		isrear = (hitGroup == BACK);

		/* Damage done to the attacker for the charge */
		if(mudconf.btech_newcharge && mudconf.btech_tl3_charge)
			mech_damage =
				(((((float) MechChargeDistance(mech)) * MP1) -
				  MechSpeed(target) *
				  cos((MechFacing(mech) -
					   MechFacing(target)) * (M_PI / 180.))) * MP_PER_KPH) *
				(MechRealTons(target) + 5) / 20;
		else
			mech_damage = (MechRealTons(target) + 5) / 10;

		/* Record the damage then dish it out */
		received_damage = mech_damage;
		spread = mech_damage / 5;

		for(i = 0; i < spread; i++) {
			hitloc = FindHitLocation(mech, hitGroup, &iscritical, &isrear);
			MyDamageMech2(mech, mech, 0, -1, hitloc, isrear, iscritical, 5,
						  0);
		}

		if(mech_damage % 5) {
			hitloc = FindHitLocation(mech, hitGroup, &iscritical, &isrear);
			MyDamageMech2(mech, mech, 0, -1, hitloc, isrear, iscritical,
						  (mech_damage % 5), 0);
		}

		/* Force piloting roll for attacker if they are in a mech */
		if(MechType(mech) == CLASS_MECH && !MadePilotSkillRoll(mech, 2)) {
			mech_notify(mech, MECHALL,
						"Your piloting skill fails and you fall over!!");
			MechFalls(mech, 1, 1);
		}

		/* Force piloting roll for target if they are in a mech */
		if(MechType(target) == CLASS_MECH && !MadePilotSkillRoll(target, 2)) {
			mech_notify(target, MECHSTARTED,
						"Your piloting skill fails and you fall over!!");
			MechFalls(target, 1, 1);
		}

		/* Stop him */
		MechSpeed(mech) = 0;
		MechDesiredSpeed(mech) = 0;

		/* Emit the damage for debugging purposes */
		snprintf(emit_buff, LBUF_SIZE, "#%li charges #%li (%i/%i) Distance:"
				 " %.2f DI: %i DR: %i", mech->mynum, target->mynum, baseToHit,
				 roll, MechChargeDistance(mech), inflicted_damage,
				 received_damage);
		SendDebug(emit_buff);

	}

	/* Cycle the sections so they can't make another attack for a while */
	if(MechType(mech) == CLASS_MECH) {
		for(i = 0; i < CHARGE_SECTIONS; i++)
			SetRecycleLimb(mech, resect[i], PHYSICAL_RECYCLE_TIME);
	} else {
		SetRecycleLimb(mech, FSIDE, PHYSICAL_RECYCLE_TIME);
		SetRecycleLimb(mech, TURRET, PHYSICAL_RECYCLE_TIME);
	}
	return;
}								// end ChargeMech()

/*
 * Checks to see if we can grab a club with our arms.
 */
int checkGrabClubLocation(MECH * mech, int section, int emit)
{
	int tCanGrab = 1;
	char buf[100];
	char location[20];

	ArmorStringFromIndex(section, location, MechType(mech), MechMove(mech));

	if(SectIsDestroyed(mech, section)) {
		sprintf(buf, "Your %s is destroyed.", location);
		tCanGrab = 0;
	} else if(!OkayCritSectS(section, 3, HAND_OR_FOOT_ACTUATOR)) {
		sprintf(buf, "Your %s's hand actuator is destroyed or missing.",
				location);
		tCanGrab = 0;
	} else if(!OkayCritSectS(section, 0, SHOULDER_OR_HIP)) {
		sprintf(buf,
				"Your %s's shoulder actuator is destroyed or missing.",
				location);
		tCanGrab = 0;
	} else if(SectHasBusyWeap(mech, section)) {
		sprintf(buf, "Your %s is still recovering from it's last attack.",
				location);
		tCanGrab = 0;
	}

	if(!tCanGrab && emit)
		mech_notify(mech, MECHALL, buf);

	return tCanGrab;
}								// end checkGrabClubLocation()

/*
 * Handles the grabbing of a club.
 */
void mech_grabclub(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	int wcArgs = 0;
	int location = 0;
	char *args[1];
	char locname[20];

	cch(MECH_USUALO);

	wcArgs = mech_parseattributes(buffer, args, 1);

	// If we grabclub -, we're attempting to drop it.
	if(wcArgs >= 1 && toupper(args[0][0]) == '-') {
		if((MechSections(mech)[LARM].specials & CARRYING_CLUB) ||
		   (MechSections(mech)[RARM].specials & CARRYING_CLUB)) {
			DropClub(mech);
		} else {
			mech_notify(mech, MECHALL,
						"You aren't currently carrying a club.");
		}
		return;
	}							// end if() - Check to drop club.

	DOCHECKMA(MechIsQuad(mech), "Quads can't carry a club.");
	DOCHECKMA(Fallen(mech),
			  "You can't grab a club while lying flat on your face.");
	DOCHECKMA(Jumping(mech), "You can't grab a club while jumping!");
	DOCHECKMA(OODing(mech), "Your rapid descent prevents that.");
	DOCHECKMA(UnJammingAmmo(mech), "You are too busy unjamming a weapon!");
	DOCHECKMA(RemovingPods(mech), "You are too busy removing iNARC pods!");

	// If they already have a physical weapon, disallow the grabbing of a club.
	DOCHECKMA(have_axe(mech, LARM) || have_axe(mech, RARM),
			  "You can not grab a club if you carry an axe.");
	DOCHECKMA(have_sword(mech, LARM) || have_sword(mech, RARM),
			  "You can not grab a club if you carry a sword.");
	DOCHECKMA(have_mace(mech, LARM) || have_mace(mech, RARM),
			  "You can not grab a club if you carry an mace.");

	if(wcArgs == 0) {
		if(checkGrabClubLocation(mech, LARM, 0))
			location = LARM;
		else if(checkGrabClubLocation(mech, RARM, 0))
			location = RARM;
		else {
			mech_notify(mech, MECHALL,
						"You don't have a free arm with a working hand actuator!");
			return;
		}
	} else {

		// Figure out which arm to use.
		switch (toupper(args[0][0])) {
		case 'R':
			location = RARM;
			break;
		case 'L':
			location = LARM;
			break;
		default:
			mech_notify(mech, MECHALL, "Invalid option for 'grabclub'");
			return;
		}						// end switch() - Determine location.

		// see if we have actuators and a working arm.
		if(!checkGrabClubLocation(mech, location, 1))
			return;
	}

	DOCHECKMA(CarryingClub(mech), "You're already carrying a club.");
	DOCHECKMA(MechRTerrain(mech) != HEAVY_FOREST &&
			  MechRTerrain(mech) != LIGHT_FOREST,
			  "There don't appear to be any trees within grabbing distance.");

	ArmorStringFromIndex(location, locname, MechType(mech), MechMove(mech));

	MechLOSBroadcast(mech,
					 "reaches down and yanks a tree out of the ground!");
	mech_printf(mech, MECHALL,
				"You reach down and yank a tree out of the ground with your %s.",
				locname);

	// Grabbing a club sets a flag and recycles the arm used.
	MechSections(mech)[location].specials |= CARRYING_CLUB;
	SetRecycleLimb(mech, location, PHYSICAL_RECYCLE_TIME);
}								// end mech_grabclub()
