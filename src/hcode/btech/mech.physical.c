/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
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

#define ARM_PHYS_CHECK(a) \
DOCHECK(MechType(mech) == CLASS_MW || MechType(mech) == CLASS_BSUIT, \
  tprintf("You cannot %s without a 'mech!", a)); \
DOCHECK(MechType(mech) != CLASS_MECH, \
  tprintf("You cannot %s with this vehicle!", a));

#define GENERIC_CHECK(a,wDeadLegs) \
ARM_PHYS_CHECK(a);\
DOCHECK(!MechIsQuad(mech) && (wDeadLegs > 1), \
  "Without legs? Are you kidding?");\
DOCHECK(!MechIsQuad(mech) && (wDeadLegs > 0),\
  "With one leg? Are you kidding?");\
DOCHECK(wDeadLegs > 1,"It'd unbalance you too much in your condition..");\
DOCHECK(wDeadLegs > 2, "Exactly _what_ are you going to kick with?");

#define QUAD_CHECK(a) \
DOCHECK(MechType(mech) == CLASS_MECH && MechIsQuad(mech), \
  tprintf("What are you going to %s with, your front right leg?", a))


/*
 * All 'mechs with arms can punch.
 */
static int have_punch(MECH * mech, int loc)
{
    return 1;
}

/*
 * Parse a physical attack command's arguments that allow an arm or both
 * to be specified.  eg. AXE [B|L|R] [ID]
 */

static int get_arm_args(int *using, int *argc, char ***args, MECH * mech,
    int (*have_fn) (MECH * mech, int loc), char *weapon)
{

    if (*argc != 0 && args[0][0][0] != '\0' && args[0][0][1] == '\0') {
	char arm = toupper(args[0][0][0]);

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
	}
    }

    switch (*using) {
    case P_LEFT:
	if (!have_fn(mech, LARM)) {
	    mech_printf(mech, MECHALL,
		"You don't have %s in your left arm!", weapon);
	    return 1;
	}
	break;

    case P_RIGHT:
	if (!have_fn(mech, RARM)) {
	    mech_printf(mech, MECHALL,
		"You don't have %s in your right arm!", weapon);
	    return 1;
	}
	break;

    case P_LEFT | P_RIGHT:
	if (!have_fn(mech, LARM))
	    *using &= ~P_LEFT;
	if (!have_fn(mech, RARM))
	    *using &= ~P_RIGHT;
	break;
    }

    return 0;
}

void mech_punch(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    MAP *mech_map;
    char *argl[5];
    char **args = argl;
    int argc, ltohit = 4, rtohit = 4;
    int punching = P_LEFT | P_RIGHT;

    mech_map = getMap(mech->mapindex);
    cch(MECH_USUALO);
    ARM_PHYS_CHECK("punch");
    QUAD_CHECK("punch");
#ifdef BT_MOVEMENT_MODES
    DOCHECK(Dodging(mech) || MoveModeLock(mech),
	"You cannot use physicals while using a special movement mode.");
#endif
    argc = mech_parseattributes(buffer, args, 5);
    if (mudconf.btech_phys_use_pskill)
	ltohit = rtohit = FindPilotPiloting(mech) - 1;

    if (get_arm_args(&punching, &argc, &args, mech, have_punch, "")) {
	return;
    }

    if (punching & P_LEFT) {
	if (SectIsDestroyed(mech, LARM))
	    mech_notify(mech, MECHALL,
		"Your left arm is destroyed, you can't punch with it.");
	else if (!OkayCritSectS(LARM, 0, SHOULDER_OR_HIP))
	    mech_notify(mech, MECHALL,
		"Your left shoulder is destroyed, you can't punch with that arm.");
	else {
	    if (Fallen(mech)) {
		DOCHECK(SectIsDestroyed(mech, RARM),
		    "You need both arms functional to punch while prone.");
		DOCHECK(SectHasBusyWeap(mech, RARM),
		    "You have weapons recycling on your Right Arm.");
		DOCHECK(MechSections(mech)[RARM].recycle,
		    "Your Right Arm is still recovering from your last attack.");
	    }
	    DOCHECK(MechSections(mech)[RARM].specials & CARRYING_CLUB,
		"You're carrying a club in that arm.");
	    PhysicalAttack(mech, 10, ltohit, PA_PUNCH, argc, args,
		mech_map, LARM);
	}
    }
    if (punching & P_RIGHT) {
	if (SectIsDestroyed(mech, RARM))
	    mech_notify(mech, MECHALL,
		"Your right arm is destroyed, you can't punch with it.");
	else if (!OkayCritSectS(RARM, 0, SHOULDER_OR_HIP))
	    mech_notify(mech, MECHALL,
		"Your right shoulder is destroyed, you can't punch with that arm.");
	else {
	    if (Fallen(mech)) {
		DOCHECK(SectIsDestroyed(mech, LARM),
		    "You need both arms functional to punch while prone.");
		DOCHECK(SectHasBusyWeap(mech, LARM),
		    "You have weapons recycling on your Left Arm.");
		DOCHECK(MechSections(mech)[LARM].recycle,
		    "Your Left Arm is still recovering from your last attack.");
	    }
	    DOCHECK(MechSections(mech)[LARM].specials & CARRYING_CLUB,
		"You're carrying a club in that arm.");
	    PhysicalAttack(mech, 10, rtohit, PA_PUNCH, argc, args,
		mech_map, RARM);
	}
    }
}

void mech_club(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    MAP *mech_map;
    char *args[5];
    int argc;
    int clubLoc = -1;

    mech_map = getMap(mech->mapindex);
    cch(MECH_USUALO);
    ARM_PHYS_CHECK("club");
    QUAD_CHECK("club");
#ifdef BT_MOVEMENT_MODES
    DOCHECK(Dodging(mech) || MoveModeLock(mech),
	"You cannot use physicals while using a special movement mode.");
#endif

    if (MechSections(mech)[RARM].specials & CARRYING_CLUB)
	clubLoc = RARM;
    else if (MechSections(mech)[LARM].specials & CARRYING_CLUB)
	clubLoc = LARM;

    if (clubLoc == -1) {
	DOCHECKMA(MechRTerrain(mech) != HEAVY_FOREST &&
	    MechRTerrain(mech) != LIGHT_FOREST,
	    "You can not seem to find any trees around to club with.");

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

    PhysicalAttack(mech, 5,
	(mudconf.btech_phys_use_pskill ? FindPilotPiloting(mech) - 1 : 4) +
	2 * MechSections(mech)[RARM].basetohit +
	2 * MechSections(mech)[LARM].basetohit, PA_CLUB, argc, args,
	mech_map, RARM);
}

int have_axe(MECH * mech, int loc)
{
    return FindObj(mech, loc, I2Special(AXE)) >= (MechTons(mech) / 15);
}

int have_sword(MECH * mech, int loc)
{
    return FindObj(mech, loc,
	I2Special(SWORD)) >= ((MechTons(mech) + 15) / 20);
}

int have_mace(MECH * mech, int loc)
{
    return FindObj(mech, loc, I2Special(MACE)) >= (MechTons(mech) / 15);
}

void mech_axe(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    MAP *mech_map;
    char *argl[5];
    char **args = argl;
    int argc, ltohit = 4, rtohit = 4;
    int using = P_LEFT | P_RIGHT;

    mech_map = getMap(mech->mapindex);
    cch(MECH_USUALO);
    ARM_PHYS_CHECK("axe");
    QUAD_CHECK("axe");
#ifdef BT_MOVEMENT_MODES
    DOCHECK(Dodging(mech) || MoveModeLock(mech),
	"You cannot use physicals while using a special movement mode.");
#endif
    argc = mech_parseattributes(buffer, args, 5);
    if (mudconf.btech_phys_use_pskill)
	ltohit = rtohit = FindPilotPiloting(mech) - 1;

    ltohit += MechSections(mech)[LARM].basetohit;
    rtohit += MechSections(mech)[RARM].basetohit;

    if (get_arm_args(&using, &argc, &args, mech, have_axe, "an axe")) {
	return;
    }

    if (using & P_LEFT) {
	DOCHECK(SectIsDestroyed(mech, LARM),
	    "Your left arm is destroyed, you can't axe with it.");
	DOCHECK(!OkayCritSectS(LARM, 0, SHOULDER_OR_HIP),
	    "Your left shoulder is destroyed, you can't axe with that arm.");
	DOCHECK(!OkayCritSectS(LARM, 3, HAND_OR_FOOT_ACTUATOR),
	    "Your left hand is destroyed, you can't axe with that arm.");

	PhysicalAttack(mech, 5, ltohit, PA_AXE, argc, args, mech_map,
	    LARM);
    }
    if (using & P_RIGHT) {
	DOCHECK(SectIsDestroyed(mech, RARM),
	    "Your right arm is destroyed, you can't axe with it.");
	DOCHECK(!OkayCritSectS(RARM, 0, SHOULDER_OR_HIP),
	    "Your right shoulder is destroyed, you can't axe with that arm.");
	DOCHECK(!OkayCritSectS(RARM, 3, HAND_OR_FOOT_ACTUATOR),
	    "Your right hand is destroyed, you can't axe with that arm.");

	PhysicalAttack(mech, 5, rtohit, PA_AXE, argc, args, mech_map,
	    RARM);
    }

    DOCHECKMA(!using,
	"You may lack the axe, but not the will! Try punch/club until you find one.");
}

void mech_sword(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    MAP *mech_map;
    char *argl[5];
    char **args = argl;
    int argc, ltohit = 3, rtohit = 3;
    int using = P_LEFT | P_RIGHT;

    mech_map = getMap(mech->mapindex);
    cch(MECH_USUALO);
    ARM_PHYS_CHECK("chop");
    QUAD_CHECK("chop");
#ifdef BT_MOVEMENT_MODES
    DOCHECK(Dodging(mech) || MoveModeLock(mech),
	"You cannot use physicals while using a special movement mode.");
#endif
    argc = mech_parseattributes(buffer, args, 5);
    if (mudconf.btech_phys_use_pskill)
	ltohit = rtohit = FindPilotPiloting(mech) - 2;

    ltohit += MechSections(mech)[LARM].basetohit;
    rtohit += MechSections(mech)[RARM].basetohit;

    if (get_arm_args(&using, &argc, &args, mech, have_sword, "a sword")) {
	return;
    }

    if (using & P_LEFT) {
	DOCHECK(SectIsDestroyed(mech, LARM),
	    "Your left arm is destroyed, you can't use a sword with it.");
	DOCHECK(!OkayCritSectS(LARM, 0, SHOULDER_OR_HIP),
	    "Your left shoulder is destroyed, you can't use a sword with that arm.");
	DOCHECK(!OkayCritSectS(LARM, 3, HAND_OR_FOOT_ACTUATOR),
	    "Your left hand is destroyed, you can't use a sword with that arm.");

	PhysicalAttack(mech, 10, ltohit, PA_SWORD, argc, args, mech_map,
	    LARM);
    }
    if (using & P_RIGHT) {
	DOCHECK(SectIsDestroyed(mech, RARM),
	    "Your right arm is destroyed, you can't use a sword with it.");
	DOCHECK(!OkayCritSectS(RARM, 0, SHOULDER_OR_HIP),
	    "Your right shoulder is destroyed, you can't use a sword with that arm.");
	DOCHECK(!OkayCritSectS(RARM, 3, HAND_OR_FOOT_ACTUATOR),
	    "Your right hand is destroyed, you can't use a sword with that arm.");

	PhysicalAttack(mech, 10, rtohit, PA_SWORD, argc, args, mech_map,
	    RARM);
    }
    DOCHECKMA(!using, "You have no sword to chop people with!");
}

void mech_kick(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    MAP *mech_map;
    char *argl[5];
    char **args = argl;
    int argc;
    int rl = RLEG, ll = LLEG;
    int leg;
    int using = P_RIGHT;

    mech_map = getMap(mech->mapindex);
    cch(MECH_USUALO);
    if (MechIsQuad(mech)) {
	rl = RARM;
	ll = LARM;
    }

    GENERIC_CHECK("kick", CountDestroyedLegs(mech));
#ifdef BT_MOVEMENT_MODES
    DOCHECK(Dodging(mech) || MoveModeLock(mech),
	"You cannot use physicals while using a special movement mode.");
#endif
    argc = mech_parseattributes(buffer, args, 5);

    if (get_arm_args(&using, &argc, &args, mech, have_punch, "")) {
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

    PhysicalAttack(mech, 5,
	(mudconf.btech_phys_use_pskill ? FindPilotPiloting(mech) - 2 : 3),
	PA_KICK, argc, args, mech_map, leg);
}

void mech_charge(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data, *target;
    MAP *mech_map;
    int targetnum;
    char targetID[5];
    char *args[5];
    int argc;
    int wcDeadLegs = 0;

    mech_map = getMap(mech->mapindex);
    cch(MECH_USUALO);
#ifdef BT_MOVEMENT_MODES
    DOCHECK(Dodging(mech) || MoveModeLock(mech),
	"You cannot use physicals while using a special movement mode.");
#endif
    DOCHECK(MechType(mech) == CLASS_MW ||
	MechType(mech) == CLASS_BSUIT,
	"You cannot charge without a 'mech!");
    DOCHECK(MechType(mech) != CLASS_MECH &&
	(MechType(mech) != CLASS_VEH_GROUND ||
	    MechSpecials(mech) & SALVAGE_TECH),
	"You cannot charge with this vehicle!");
    if (MechType(mech) == CLASS_MECH) {
	/* set the number of dead legs we have */
	wcDeadLegs = CountDestroyedLegs(mech);

	DOCHECK(!MechIsQuad(mech) && (wcDeadLegs > 0),
	    "With one leg? Are you kidding?");
	DOCHECK(!MechIsQuad(mech) && (wcDeadLegs > 1),
	    "Without legs? Are you kidding?");
	DOCHECK(wcDeadLegs > 1,
	    "It'd unbalance you too much in your condition..");
	DOCHECK(wcDeadLegs > 2,
	    "Exactly _what_ are you going to kick with?");
    }
    argc = mech_parseattributes(buffer, args, 2);
    switch (argc) {
    case 0:
	DOCHECKMA(MechTarget(mech) == -1,
	    "You do not have a default target set!");
	target = getMech(MechTarget(mech));
	if (!target) {
	    mech_notify(mech, MECHALL, "Invalid default target!");
	    MechTarget(mech) = -1;
	    return;
	}
	if (MechType(target) == CLASS_MW) {
	    mech_notify(mech, MECHALL, "You can't charge THAT sack of bones and squishy bits!");
	    return;
	}
	MechChargeTarget(mech) = MechTarget(mech);
	mech_notify(mech, MECHALL, "Charge target set to default target.");
	break;
    case 1:
	if (args[0][0] == '-') {
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

	if (!target) {
	    mech_notify(mech, MECHALL, "Invalid target data!");
	    return;
	}
	if (MechType(target) == CLASS_MW) {
	    mech_notify(mech, MECHALL, "You can't charge THAT sack of bones and squishy bits!");
	    return;
	}
	MechChargeTarget(mech) = targetnum;
	mech_printf(mech, MECHALL, "%s target set to %s.",
		MechType(mech) == CLASS_MECH ? "Charge" : "Ram",
		GetMechToMechID(mech, target));
	break;
    default:
	notify(player, "Invalid number of arguments!");
    }
}


char *phys_form(int at, int flag)
{
    switch (flag) {
    case 0:
	switch (at) {
	case PA_PUNCH:
	    return "punches";
	case PA_CLUB:
	case PA_MACE:
	    return "clubs";
	case PA_SWORD:
	    return "chops";
	case PA_AXE:
	    return "axes";
	case PA_KICK:
	    return "kicks";
	}
	break;
    default:
	switch (at) {
	case PA_PUNCH:
	    return "punch";
	case PA_SWORD:
	    return "chop";
	case PA_CLUB:
	case PA_MACE:
	    return "club";
	case PA_AXE:
	    return "axe";
	case PA_KICK:
	    return "kick";
	}
	break;
    }
    return "??bug??";
}

#define phys_message(txt) \
MechLOSBroadcasti(mech,target,txt)

void phys_succeed(MECH * mech, MECH * target, int at)
{
    phys_message(tprintf("%s %%s!", phys_form(at, 0)));
}

void phys_fail(MECH * mech, MECH * target, int at)
{
    phys_message(tprintf("attempts to %s %%s!", phys_form(at, 1)));
}

void PhysicalAttack(MECH * mech, int damageweight, int baseToHit,
    int AttackType, int argc, char **args, MAP * mech_map, int sect)
{
    MECH *target;
    float range;
    float maxRange = 1;
    char targetID[2];
    int targetnum, roll;
    char location[20];
    int ts = 0, iwa;

    DOCHECKMA(Fallen(mech) &&
	(AttackType != PA_PUNCH),
	"You can't attack from a prone position.");

#define Check(num,okval,mod) \
  if (AttackType == PA_PUNCH) \
    if (MechType(mech) == CLASS_MECH) \
      if (PartIsNonfunctional(mech, sect, num) || GetPartType(mech, sect, num) != \
          I2Special(okval)) \
  baseToHit += mod;

    Check(1, UPPER_ACTUATOR, 2);
    Check(2, LOWER_ACTUATOR, 2);
    Check(3, HAND_OR_FOOT_ACTUATOR, 1);
    if (AttackType == PA_KICK)
	DOCHECKMA((MechCritStatus(mech) & HIP_DAMAGED),
	    "You can not kick if your have a destroyed hip.");

    if (SectHasBusyWeap(mech, sect)) {
	ArmorStringFromIndex(sect, location, MechType(mech),
	    MechMove(mech));
	mech_printf(mech, MECHALL,
	    "You have weapons recycling on your %s.", location);
	return;
    }
    switch (AttackType) {
    case PA_MACE:
    case PA_SWORD:
    case PA_AXE:
	DOCHECKMA(MechSections(mech)[LARM].recycle ||
	    MechSections(mech)[RARM].recycle,
	    "You still have arms recovering from another attack.");
	DOCHECKMA(Fallen(mech), "You can't do this while fallen!");
    case PA_PUNCH:
	DOCHECKMA(MechSections(mech)[LLEG].recycle ||
	    MechSections(mech)[RLEG].recycle,
	    "You're still recovering from another attack.");
	if (MechSections(mech)[LARM].recycle)
	    DOCHECKMA(MechSections(mech)[LARM].config & AXED,
		"You are recovering from another attack.");
	if (MechSections(mech)[RARM].recycle)
	    DOCHECKMA(MechSections(mech)[RARM].config & AXED,
		"You are recovering from another attack.");
	break;
    case PA_CLUB:
	DOCHECKMA(MechSections(mech)[LLEG].recycle ||
	    MechSections(mech)[RLEG].recycle,
	    "You're still recovering from your kick.");
	/* Check Weapons recycling on LARM because we only checked RARM above. */
	DOCHECKMA(SectHasBusyWeap(mech, LARM),
	    "You have weapons recycling on your Left Arm.");
	DOCHECKMA(Fallen(mech), "You can't do this while fallen!");
	break;
    case PA_KICK:
	DOCHECKMA(Fallen(mech), "You can't kick while fallen!");
	if (MechIsQuad(mech)) {
	    DOCHECKMA(MechSections(mech)[LLEG].recycle ||
		MechSections(mech)[RLEG].recycle,
		"Your rear legs are still recovering from your last attack.");
	    DOCHECKMA(MechSections(mech)[RARM].recycle ||
		MechSections(mech)[LARM].recycle,
		"Your front legs are not ready to attack again.");

	} else {
	    DOCHECKMA(MechSections(mech)[LLEG].recycle ||
		MechSections(mech)[RLEG].recycle,
		"Your legs are not ready to attack again.");
	    DOCHECKMA(MechSections(mech)[RARM].recycle ||
		MechSections(mech)[LARM].recycle,
		"Your arms are still recovering from your last attack.");
	}
	break;
    }

    /* major section recycle */
    if (MechSections(mech)[sect].recycle != 0) {
	ArmorStringFromIndex(sect, location, MechType(mech),
	    MechMove(mech));
	mech_printf(mech, MECHALL,
	    "Your %s is not ready to attack again.", location);
	return;
    }
    switch (argc) {
    case -1:
    case 0:
	DOCHECKMA(MechTarget(mech) == -1,
	    "You do not have a default target set!");
	target = getMech(MechTarget(mech));
	DOCHECKMA(!target, "Invalid default target!");

	if ((MechType(target) == CLASS_BSUIT) ||
	    (MechType(target) == CLASS_MW))
	    maxRange = 0.5;

	DOCHECKMA((Fallen(mech) && (AttackType == PA_PUNCH)) &&
	    (MechType(target) != CLASS_VEH_GROUND),
	    "You can only punch vehicles while you're fallen");
	range = FaMechRange(mech, target);
	DOCHECKMA(!InLineOfSight_NB(mech, target, MechX(target),
		MechY(target), range),
	    "You are unable to hit that target at the moment.");

	DOCHECKMA(range >= maxRange, "Target out of range!");

	break;
    default:
	/* Any number of targets, take only the first -- mw 93 Oct */
	targetID[0] = args[0][0];
	targetID[1] = args[0][1];
	targetnum = FindTargetDBREFFromMapNumber(mech, targetID);
	DOCHECKMA(targetnum == -1, "Target is not in line of sight!");
	target = getMech(targetnum);
	DOCHECKMA(!target, "Invalid default target!");

	if ((MechType(target) == CLASS_BSUIT) ||
	    (MechType(target) == CLASS_MW))
	    maxRange = 0.5;

	range = FaMechRange(mech, target);

	DOCHECKMA(!InLineOfSight_NB(mech, target, MechX(target),
		MechY(target), range),
	    "Target is not in line of sight!");

	DOCHECKMA(range >= maxRange,
	    "Target out of range!");
    }

    DOCHECKMA((Fallen(mech) && (AttackType == PA_PUNCH)) &&
	(MechType(target) != CLASS_VEH_GROUND),
	"You can only punch vehicles while you're fallen");

    DOCHECKMA(MechTeam(target) == MechTeam(mech) && MechNoFriendlyFire(mech),
	      "You can't attack a teammate with FFSafeties on!");

    DOCHECKMA(MechType(target) == CLASS_MW && !MechPKiller(mech),
    	"That's a living, breathing person! Switch off the safety first, "
    	"if you really want to assassinate the target.");
    
    if (MechMove(target) != MOVE_VTOL && MechMove(target) != MOVE_FLY) {
	DOCHECKMA((AttackType == PA_PUNCH || AttackType == PA_AXE ||
		AttackType == PA_SWORD) &&
	    (MechZ(mech) - 1) > MechZ(target),
	    tprintf("The target is too low in elevation for you to %s.",
		AttackType == PA_PUNCH ? "punch at." : "axe it."));
	DOCHECKMA(AttackType == PA_KICK &&
	    MechZ(mech) < MechZ(target),
	    "The target is too high in elevation for you to kick at.");
	DOCHECKMA(MechZ(mech) - MechZ(target) > 1 ||
	    MechZ(target) - MechZ(mech) > 1,
	    "You can't attack, the elevation difference is too large.");

	DOCHECKMA(
	    (AttackType == PA_KICK) &&
	    (MechZ(target) < MechZ(mech) &&
		(((MechType(target) == CLASS_MECH) && Fallen(target)) ||
		    (MechType(target) == CLASS_VEH_GROUND) ||
		    (MechType(target) == CLASS_BSUIT) ||
		    (MechType(target) == CLASS_MW))),
	    "The target is too low for you to kick at.")
    } else {
	DOCHECKMA((AttackType == PA_PUNCH || AttackType == PA_AXE ||
		AttackType == PA_SWORD) &&
	    MechZ(target) - MechZ(mech) > 3,
	    tprintf("The target is too far away for you to %s.",
		AttackType == PA_PUNCH ? "punch at" : "axe it"));
	DOCHECKMA(AttackType == PA_KICK &&
	    MechZ(mech) != MechZ(target),
	    "The target is too far away for you to kick at.");
	DOCHECKMA(!(MechZ(target) - MechZ(mech) > -1 &&
		MechZ(target) - MechZ(mech) < 4),
	    "You can't attack, the elevation difference is too large.");
    }

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
    if (AttackType == PA_KICK) {

	    ts = MechStatus(mech) & (TORSO_LEFT | TORSO_RIGHT);
	    MechStatus(mech) &= ~ts;
        iwa = InWeaponArc(mech, MechFX(target), MechFY(target));
	    MechStatus(mech) |= ts;

        DOCHECKMA(!(iwa & FORWARDARC), "Target is not in your 'real' forward arc!");

    } else {

        iwa = InWeaponArc(mech, MechFX(target), MechFY(target));

        if (AttackType == PA_CLUB) {
            DOCHECKMA(!(iwa & FORWARDARC), "Target is not in your forward arc!");
        } else {

            if (sect == RARM) {
                DOCHECKMA(!((iwa & FORWARDARC) || (iwa & RSIDEARC)),
                        "Target is not in your forward or right side arc!");
            } else {
                DOCHECKMA(!((iwa & FORWARDARC) || (iwa & LSIDEARC)),
                        "Target is not in your forward or left side arc!");

            }

        }

    }

    /* Add in the movement modifiers */
    baseToHit += HasBoolAdvantage(MechPilot(mech), "melee_specialist") ? MIN(0, AttackMovementMods(mech) - 1) : AttackMovementMods(mech);
    baseToHit += TargetMovementMods(mech, target, 0.0);
    baseToHit += MechType(target) == CLASS_BSUIT ? 1 : 0;
    baseToHit += ((MechType(target) == CLASS_BSUIT) &&
	(AttackType == PA_KICK)) ? 3 : 0;
#ifdef MOVEMENT_MODES
    if (Dodging(target))
	baseToHit += 2;
#endif

    if ((AttackType == PA_PUNCH || AttackType == PA_AXE ||
	    AttackType == PA_SWORD) && MechType(target) == CLASS_BSUIT &&
	MechSwarmTarget(target) > 0)
	baseToHit += (AttackType == PA_AXE ||
	    AttackType == PA_SWORD) ? 3 : 5;
    DOCHECKMA((AttackType != PA_PUNCH && AttackType != PA_AXE &&
	    AttackType != PA_SWORD) && MechType(target) == CLASS_BSUIT &&
	MechSwarmTarget(target) > 0,
	"You can hit swarming 'suits only with punches (or axe/sword)!");
    roll = Roll();
    switch (AttackType) {
    case PA_PUNCH:
	DOCHECKMA((Fallen(mech) && (AttackType == PA_PUNCH)) &&
	    (MechType(target) != CLASS_VEH_GROUND),
	    "You can only punch vehicles while you're fallen");
	if (MechZ(mech) >= MechZ(target))
	    DOCHECKMA(((Fallen(target) && MechType(target) == CLASS_MECH)
		    || ((MechType(target) != CLASS_MECH &&
			    (MechType(target) != CLASS_BSUIT ||
				MechSwarmTarget(target) != mech->mynum) &&
			    !IsDS(target)) &&
			!Fallen(mech))),
		"The target is too low to punch!");
	DOCHECKMA(Jumping(target) || (Jumping(mech) &&
		MechType(target) == CLASS_MECH),
	    "You cannot physically attack a jumping mech!");
	DOCHECKMA(Standing(mech), "You are still trying to stand up!");
	mech_printf(mech, MECHALL,
	    "You try to punch the %s.  BTH:  %d,\tRoll:  %d",
		GetMechToMechID(mech, target), baseToHit, roll);
	mech_printf(target, MECHSTARTED, "%s tries to punch you!",
		GetMechToMechID(target, mech));

    /* Switching to Exile method of tracking xp, where we split
     * Attacking and Piloting xp into two different channels
     * And since this is neither it goes to its own channel
     */
	SendAttacks(tprintf("#%i attacks #%i (punch) (%i/%i)", mech->mynum,
		target->mynum, baseToHit, roll));
	break;
    case PA_SWORD:
    case PA_AXE:
    case PA_CLUB:
	DOCHECKMA(Jumping(target) || (Jumping(mech) &&
		MechType(target) == CLASS_MECH),
	    "You cannot physically attack a jumping mech!");
	DOCHECKMA(Standing(mech), "You are still trying to stand up!");
	if (AttackType == PA_CLUB) {
	    mech_printf(mech, MECHALL,
		"You try and club %s.  BaseToHit:  %d,\tRoll:  %d",
		    GetMechToMechID(mech, target), baseToHit, roll);
	    mech_printf(target, MECHSTARTED,
		"%s tries to club you!", GetMechToMechID(target,
			mech));

        /* Switching to Exile method of tracking xp, where we split
         * Attacking and Piloting xp into two different channels
         * And since this is neither it goes to its own channel
         */
	    SendAttacks(tprintf("#%i attacks #%i (club) (%i/%i)", mech->mynum,
		    target->mynum, baseToHit, roll));
	} else {
	    mech_printf(mech, MECHALL,
		"You try to swing your %s at %s.  BTH:  %d,\tRoll:  %d",
		    AttackType == PA_SWORD ? "sword" : "axe",
		    GetMechToMechID(mech, target), baseToHit, roll);
	    mech_printf(target, MECHSTARTED, "%s tries to %s you!",
		    GetMechToMechID(target, mech),
		    AttackType == PA_SWORD ? "swing a sword at" : "axe");

        /* Switching to Exile method of tracking xp, where we split
         * Attacking and Piloting xp into two different channels
         * And since this is neither it goes to its own channel
         */
	    SendAttacks(tprintf("#%i attacks #%i (%s) (%i/%i)", mech->mynum,
		    target->mynum,
		    AttackType == PA_SWORD ? "sword" : "axe", baseToHit,
		    roll));
	}

	break;
    case PA_KICK:
	    DOCHECKMA(Jumping(target) || (Jumping(mech) &&
		    MechType(target) == CLASS_MECH),
	        "You cannot physically attack a jumping mech!");
	    DOCHECKMA(Standing(mech), "You are still trying to stand up!");
	    mech_printf(mech, MECHALL,
	        "You try and kick %s.  BaseToHit:  %d,\tRoll:  %d",
		    GetMechToMechID(mech, target), baseToHit, roll);
    	mech_printf(target, MECHSTARTED, "%s tries to kick you!",
		    GetMechToMechID(target, mech));

        /* Switching to Exile method of tracking xp, where we split
         * Attacking and Piloting xp into two different channels
         * And since this is neither it goes to its own channel
         */
	    SendAttacks(tprintf("#%i attacks #%i (kick) (%i/%i)", mech->mynum,
		    target->mynum, baseToHit, roll));
    }

    /* set the sections to recycling */

    SetRecycleLimb(mech, sect, PHYSICAL_RECYCLE_TIME);
    if (AttackType == PA_AXE || AttackType == PA_SWORD)
	MechSections(mech)[sect].config |= AXED;
    if (AttackType == PA_PUNCH)
	MechSections(mech)[sect].config &= ~AXED;
    if (AttackType == PA_CLUB)
	SetRecycleLimb(mech, LARM, PHYSICAL_RECYCLE_TIME);
    if (roll >= baseToHit) {	/*  hit the target */
	phys_succeed(mech, target, AttackType);

	if (AttackType == PA_CLUB) {
	    int clubLoc = -1;

	    if (MechSections(mech)[RARM].specials & CARRYING_CLUB)
		clubLoc = RARM;
	    else if (MechSections(mech)[LARM].specials & CARRYING_CLUB)
		clubLoc = LARM;

	    if (clubLoc > -1) {
		mech_notify(mech, MECHALL,
		    "Your club shatters on contact.");
		MechLOSBroadcast(mech,
		    "'s club shatters with a loud *CRACK*!");

		MechSections(mech)[clubLoc].specials &= ~CARRYING_CLUB;
	    }
	}

	PhysicalDamage(mech, target, damageweight, AttackType, sect);
    } else {
	phys_fail(mech, target, AttackType);
	if (MechType(target) == CLASS_BSUIT &&
	    MechSwarmTarget(target) == mech->mynum) {
	    if (!MadePilotSkillRoll(mech, 4)) {
		mech_notify(mech, MECHALL,
		    "Uh oh. You miss the little buggers, but hit yourself!");
		MechLOSBroadcast(mech, "misses, and hits itself!");
		PhysicalDamage(mech, mech, damageweight, AttackType, sect);
	    }
	}
	if (AttackType == PA_KICK || AttackType == PA_CLUB) {
	    mech_notify(mech, MECHALL,
		"You miss and try to remain standing!");
	    if (!MadePilotSkillRoll(mech, 0)) {
		mech_notify(mech, MECHALL,
		    "You lose your balance and fall down!");
		MechFalls(mech, 1, 1);
	    }
	}
    }
}

extern int global_physical_flag;

#define MyDamageMech(a,b,c,d,e,f,g,h,i) \
        global_physical_flag = 1 ; DamageMech(a,b,c,d,e,f,g,h,i,-1,0,-1,0,0); \
        global_physical_flag = 0
#define MyDamageMech2(a,b,c,d,e,f,g,h,i) \
        global_physical_flag = 2 ; DamageMech(a,b,c,d,e,f,g,h,i,-1,0,-1,0,0); \
        global_physical_flag = 0

void PhysicalDamage(MECH * mech, MECH * target, int weightdmg,
    int AttackType, int sect)
{
    int hitloc = 0, damage, hitgroup = 0, isrear, iscritical, i;

    if (AttackType == PA_SWORD)
	damage = (MechTons(mech) + 5) / weightdmg + 1;
    else
	damage = (MechTons(mech) + weightdmg / 2) / weightdmg;
    if ((MechHeat(mech) >= 9.) &&
	(MechSpecials(mech) & TRIPLE_MYOMER_TECH))
	damage = damage * 2;
    if (HasBoolAdvantage(MechPilot(mech), "melee_specialist"))
	damage++;
    switch (AttackType) {
    case PA_PUNCH:
	if (sect == LARM) {
	    if (!OkayCritSectS(LARM, 2, LOWER_ACTUATOR))
		damage = damage / 2;

	    if (!OkayCritSectS(LARM, 1, UPPER_ACTUATOR))
		damage = damage / 2;
	} else if (sect == RARM) {
	    if (!OkayCritSectS(RARM, 2, LOWER_ACTUATOR))
		damage = damage / 2;

	    if (!OkayCritSectS(RARM, 1, UPPER_ACTUATOR))
		damage = damage / 2;
	}
	hitgroup = FindAreaHitGroup(mech, target);
	if (MechType(mech) == CLASS_MECH) {
	    if (Fallen(mech)) {
		if ((MechType(target) != CLASS_MECH) || (Fallen(target) &&
			(MechElevation(mech) == MechElevation(target))))
		    hitloc =
			FindTargetHitLoc(mech, target, &isrear,
			&iscritical);
		else if (!Fallen(target) &&
		    (MechElevation(mech) > MechElevation(target)))
		    hitloc = FindPunchLocation(hitgroup);
		else if (MechElevation(mech) == MechElevation(target))
		    hitloc = FindKickLocation(hitgroup);
	    } else if (MechElevation(mech) < MechElevation(target)) {
		if (Fallen(target) || MechType(target) != CLASS_MECH)
		    hitloc =
			FindTargetHitLoc(mech, target, &isrear,
			&iscritical);
		else
		    hitloc = FindKickLocation(hitgroup);
	    } else
		hitloc = FindPunchLocation(hitgroup);
	} else {
	    isrear = hitgroup == REAR;
	    hitloc = FindTargetHitLoc(mech, target, &isrear, &iscritical);
	}
	break;


    case PA_SWORD:
    case PA_AXE:
    case PA_CLUB:
	hitgroup = FindAreaHitGroup(mech, target);
	if (MechType(mech) == CLASS_MECH) {
	    if (MechElevation(mech) < MechElevation(target))
		if (Fallen(target) || MechType(target) != CLASS_MECH)
		    hitloc =
			FindTargetHitLoc(mech, target, &isrear,
			&iscritical);
		else
		    hitloc = FindKickLocation(hitgroup);
	    else if (MechElevation(mech) > MechElevation(target))
		hitloc = FindPunchLocation(hitgroup);
	    else
		hitloc =
		    FindTargetHitLoc(mech, target, &isrear, &iscritical);
	} else {
	    isrear = hitgroup == REAR;
	    hitloc = FindTargetHitLoc(mech, target, &isrear, &iscritical);
	}
	break;

    case PA_KICK:
	if (sect == LLEG) {
	    if (!OkayCritSectS(LLEG, 2, LOWER_ACTUATOR))
		damage = damage / 2;

	    if (!OkayCritSectS(LLEG, 1, UPPER_ACTUATOR))
		damage = damage / 2;
	} else if (sect == RLEG) {
	    if (!OkayCritSectS(RLEG, 2, LOWER_ACTUATOR))
		damage = damage / 2;

	    if (!OkayCritSectS(RLEG, 1, UPPER_ACTUATOR))
		damage = damage / 2;
	}
	if (Fallen(target) || MechType(target) != CLASS_MECH)
	    hitloc = FindTargetHitLoc(mech, target, &isrear, &iscritical);
	else {
	    hitgroup = FindAreaHitGroup(mech, target);
	    if (MechElevation(mech) > MechElevation(target))
		hitloc = FindPunchLocation(hitgroup);
	    else {
		hitloc = FindKickLocation(hitgroup);
		if (!GetSectInt(target, hitloc) &&
		    GetSectInt(target, (i =
			    BOUNDED(0, 5 + (6 - hitloc),
				NUM_SECTIONS - 1))))
		    hitloc = i;
	    }
	}
	break;
    }
    MyDamageMech(target, mech, 1, MechPilot(mech), hitloc,
	(hitgroup == BACK) ? 1 : 0, 0, damage, 0);

    if (MechType(target) == CLASS_BSUIT && MechSwarmTarget(target) > 0 &&
	(AttackType == PA_PUNCH || AttackType == PA_AXE ||
	    AttackType == PA_SWORD))
	StopSwarming(target, 0);

    if (MechType(target) == CLASS_MECH && AttackType == PA_KICK)
	if (!MadePilotSkillRoll(target, 0) && !Fallen(target)) {
	    mech_notify(target, MECHSTARTED,
		"The kick knocks you to the ground!");
	    MechLOSBroadcast(target, "stumbles and falls down!");
	    MechFalls(target, 1, 0);
	}
}

#define CHARGE_SECTIONS 6
#define DFA_SECTIONS    4

/* 4 if pure FASA */

const int resect[CHARGE_SECTIONS] =
    { LARM, RARM, LLEG, RLEG, LTORSO, RTORSO };

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

    /* Weapons recycling check on each major section */
    for (i = 0; i < DFA_SECTIONS; i++)
        if (SectHasBusyWeap(mech, resect[i])) {
            ArmorStringFromIndex(resect[i], location, MechType(mech),
                MechMove(mech));
            mech_printf(mech, MECHALL,
                "You have weapons recycling on your %s.",
                    location);
            return 0;
        }

    DOCHECKMA0((mech->mapindex != target->mapindex), "Invalid Target.");

    DOCHECKMA0(((MechTeam(mech) == MechTeam(target)) && (Started(target)) &&
        (!Destroyed(target))), "Friendly units ? I dont Think so..");

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
        "Your target is jumping, you cannot land on it.");

    if ((MechType(target) == CLASS_VTOL) || (MechType(target) == CLASS_AERO) ||
        (MechType(target) == CLASS_DS))
        DOCHECKMA0(!Landed(target), "Your target is airborne, you cannot land on it.");

    if (mudconf.btech_phys_use_pskill)
        baseToHit = FindPilotPiloting(mech);

    baseToHit += (HasBoolAdvantage(MechPilot(mech), "melee_specialist") ? 
        MIN(0, AttackMovementMods(mech)) - 1 : AttackMovementMods(mech));
    baseToHit += TargetMovementMods(mech, target, 0.0);
    baseToHit += MechType(target) == CLASS_BSUIT ? 1 : 0;
#ifdef BT_MOVEMENT_MODES
    if (Dodging(target))
        baseToHit += 2;
#endif

    DOCHECKMA0(baseToHit > 12,
        tprintf("DFA: BTH %d\tYou choose not to attack and land from your jump.",
            baseToHit));

    roll = Roll();
    mech_printf(mech, MECHALL, "DFA: BTH %d\tRoll: %d", baseToHit, roll);

    MechStatus(mech) &= ~JUMPING;
    MechStatus(mech) &= ~DFA_ATTACK;

    if (roll >= baseToHit) {
        /* OUCH */
        mech_printf(target, MECHSTARTED,
            "DEATH FROM ABOVE!!!\n%s lands on you from above!",
                GetMechToMechID(target, mech));
        mech_notify(mech, MECHALL, "You land on your target legs first!");
        MechLOSBroadcasti(mech, target, "lands on %s!");

        hitGroup = FindAreaHitGroup(mech, target);
        if (hitGroup == BACK)
            isrear = 1;

        target_damage = (3 * MechRealTons(mech)) / 10;

        if (MechTons(mech) % 10)
            target_damage++;

        if (HasBoolAdvantage(MechPilot(mech), "melee_specialist"))
            target_damage++;

        spread = target_damage / 5;

        for (i = 0; i < spread; i++) {
            if (Fallen(target) || MechType(target) != CLASS_MECH)
                hitloc =
                    FindHitLocation(target, hitGroup, &iscritical, &isrear);
            else
                hitloc = FindPunchLocation(hitGroup);

            MyDamageMech(target, mech, 1, MechPilot(mech), hitloc, isrear,
                iscritical, 5, 0);
        }

        if (target_damage % 5) {
            if (Fallen(target) || (MechType(target) != CLASS_MECH))
                hitloc =
                    FindHitLocation(target, hitGroup, &iscritical, &isrear);
            else
                hitloc = FindPunchLocation(hitGroup);

            MyDamageMech(target, mech, 1, MechPilot(mech), hitloc, isrear,
                iscritical, (target_damage % 5), 0);
        }

        mech_damage = MechTons(mech) / 5;

        spread = mech_damage / 5;

        for (i = 0; i < spread; i++) {
            hitloc = FindKickLocation(FRONT);
            MyDamageMech2(mech, mech, 0, -1, hitloc, 0, 0, 5, 0);
        }

        if (mech_damage % 5) {
            hitloc = FindKickLocation(FRONT);
            MyDamageMech2(mech, mech, 0, -1, hitloc, 0, 0,
                (mech_damage % 5), 0);
        }

        if (!Fallen(mech)) {
            if (!MadePilotSkillRoll(mech, 4)) {
                mech_notify(mech, MECHALL,
                    "Your piloting skill fails and you fall over!!");
                MechLOSBroadcast(mech, "stumbles and falls down!");
                MechFalls(mech, 1, 0);
            }
            if (MechType(target) == CLASS_MECH &&
                !MadePilotSkillRoll(target, 2)) {
                mech_notify(target, MECHSTARTED,
                    "Your piloting skill fails and you fall over!!");
                MechLOSBroadcast(target, "stumbles and falls down!");
                MechFalls(target, 1, 0);
            }
        }

    } else {
        /* Missed DFA attack */
        if (!Fallen(mech)) {
            mech_notify(mech, MECHALL,
                "You miss your DFA attack and fall on your back!!");
            MechLOSBroadcast(mech, "misses DFA and falls down!");
        }

        mech_damage = MechTons(mech) / 5;
        spread = mech_damage / 5;

        for (i = 0; i < spread; i++) {
            hitloc = FindHitLocation(mech, BACK, &iscritical, &tmpi);
            MyDamageMech2(mech, mech, 0, -1, hitloc, 1, iscritical, 5, 0);
        }

        if (mech_damage % 5) {
            hitloc = FindHitLocation(mech, BACK, &iscritical, &tmpi);
            MyDamageMech2(mech, mech, 0, -1, hitloc, 1, iscritical,
                (mech_damage % 5), 0);
        }

        /* now damage pilot */
        if (!MadePilotSkillRoll(mech, 2)) {
            mech_notify(mech, MECHALL,
                "You take personal injury from the fall!");
            headhitmwdamage(mech, 1);
        }

        MechSpeed(mech) = 0.0;
        MechDesiredSpeed(mech) = 0.0;

        MakeMechFall(mech);
        MechZ(mech) = MechElevation(mech);
        MechFZ(mech) = MechZ(mech) * ZSCALE;

        if (MechZ(mech) < 0)
            MechFloods(mech);

    }

    for (i = 0; i < DFA_SECTIONS; i++)
        SetRecycleLimb(mech, resect[i], PHYSICAL_RECYCLE_TIME);

    return 1;
}

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
    if (MechChargeTarget(target) == mech->mynum) {
        /* They are both charging each other */
        mech_charge = 1;
        target_charge = 1;

        /* Check the sections of the first unit for weapons that are cycling */
        done = 0;
        for (i = 0; i < CHARGE_SECTIONS && !done; i++) {
            if (SectHasBusyWeap(mech, resect[i])) {
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
        for (i = 0; i < CHARGE_SECTIONS && !done; i++) {
            if (SectHasBusyWeap(target, resect[i])) {
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
        if (!Started(target) || Uncon(target) || Blinded(target))
            target_charge = 0;
        /* Is the first unit capable of charging */
        if (!Started(mech) || Uncon(mech) || Blinded(mech))
            mech_charge = 0;

        /* Is the first unit moving fast enough to charge */
        if (MechSpeed(mech) < MP1) {
            mech_notify(mech, MECHALL,
                "You aren't moving fast enough to charge.");
            mech_charge = 0;
        }

        /* Is the second unit moving fast enough to charge */
        if (MechSpeed(target) < MP1) {
            mech_notify(target, MECHALL,
                "You aren't moving fast enough to charge.");
            target_charge = 0;
        }

        /* Check to see if any sections cycling from a previous attack */
        if (MechType(mech) == CLASS_MECH) {
            /* Is the first unit's legs cycling */
            if (MechSections(mech)[LLEG].recycle ||
                MechSections(mech)[RLEG].recycle) {
                mech_notify(mech, MECHALL,
                    "Your legs are still recovering from your last attack.");
                mech_charge = 0;
            } 
            /* Is the first unit's arms cycling */
            if (MechSections(mech)[RARM].recycle ||
                MechSections(mech)[LARM].recycle) {
                mech_notify(mech, MECHALL,
                    "Your arms are still recovering from your last attack.");
                mech_charge = 0;
            }
        } else {
            /* Is the first unit's front side cycling */
            if (MechSections(mech)[FSIDE].recycle) {
                mech_notify(mech, MECHALL,
                    "You are still recovering from your last attack!");
                mech_charge = 0;
            }
        }

        /* Check to see if any sections cycling from a previous attack */
        if (MechType(target) == CLASS_MECH) {
            /* Is the second unit's legs cycling */
            if (MechSections(target)[LLEG].recycle ||
                MechSections(target)[RLEG].recycle) {
                mech_notify(target, MECHALL,
                    "Your legs are still recovering from your last attack.");
                target_charge = 0;
            }
            /* Is the second unit's arms cycling */
            if (MechSections(target)[RARM].recycle ||
                MechSections(target)[LARM].recycle) {
                mech_notify(target, MECHALL,
                    "Your arms are still recovering from your last attack.");
                target_charge = 0;
            }
        } else {
            /* Is the second unit's front side cycling */
            if (MechSections(target)[FSIDE].recycle) {
                mech_notify(target, MECHALL,
                    "You are still recovering from your last attack!");
                target_charge = 0;
            }
        }
  
        /* Is the second unit jumping */
        if (Jumping(target)) {
            mech_notify(mech, MECHALL,
                "Your target is jumping, you charge underneath it.");
            mech_notify(target, MECHALL,
                "You can't charge while jumping, try death from above.");
            mech_charge = 0;
            target_charge = 0;
        }

        /* Is the first unit jumping */
        if (Jumping(mech)) {
            mech_notify(target, MECHALL,
                "Your target is jumping, you charge underneath it.");
            mech_notify(mech, MECHALL,
                "You can't charge while jumping, try death from above.");
            mech_charge = 0;
            target_charge = 0;
        }

        /* Is the second unit fallen and the first unit not a tank */
        if (Fallen(target) && (MechType(mech) != CLASS_VEH_GROUND)) {
            mech_notify(mech, MECHALL,
                "Your target's too low for you to charge it!");
            mech_charge = 0;
        }

        /* Not sure at the moment if I need this here, but I figured
         * couldn't hurt for now */
        /* Is the first unit fallen and the second unit not a tank */
        if (Fallen(mech) && (MechType(target) != CLASS_VEH_GROUND)) {
            mech_notify(target, MECHALL,
                "Your target's too low for you to charge it!");
            target_charge = 0;
        }

        /* If the second unit is a mech it can only charge mechs */
        if ((MechType(target) == CLASS_MECH) && (MechType(mech) != CLASS_MECH)) {
            mech_notify(target, MECHALL, "You can only charge mechs!");
            target_charge = 0;
        }

        /* If the first unit is a mech it can only charge mechs */
        if ((MechType(mech) == CLASS_MECH) && (MechType(target) != CLASS_MECH)) {
            mech_notify(mech, MECHALL, "You can only charge mechs!");
            mech_charge = 0;
        }

        /* If the second unit is a tank, it can only charge tanks and mechs */
        if ((MechType(target) == CLASS_VEH_GROUND) && 
                ((MechType(mech) != CLASS_MECH) && 
                 (MechType(mech) != CLASS_VEH_GROUND))) {
            mech_notify(target, MECHALL, "You can only charge mechs and tanks!");
            target_charge = 0;
        }

        /* If the first unit is a tank, it can only charge tanks and mechs */
        if ((MechType(mech) == CLASS_VEH_GROUND) && 
                ((MechType(target) != CLASS_MECH) && 
                 (MechType(target) != CLASS_VEH_GROUND))) {
            mech_notify(mech, MECHALL, "You can only charge mechs and tanks!");
            mech_charge = 0;
        }

        /* Are they stunned ? */
        if (CrewStunned(mech)) {
            mech_notify(mech, MECHALL, "You are too stunned to ram!");
            mech_charge = 0;
        }

        if (CrewStunned(target)) {
            mech_notify(target, MECHALL, "You are too stunned to ram!");
            target_charge = 0;
        }

        /* Are they trying to unjam their turrets ? */
        if (UnjammingTurret(mech)) {
            mech_notify(mech, MECHALL,
                "You are too busy unjamming your turret!");
            mech_charge = 0;
        }

        if (UnjammingTurret(target)) {
            mech_notify(mech, MECHALL,
                "You are too busy unjamming your turret!");
            target_charge = 0;
        }

        /* Check the arcs to make sure the target is in the front arc */
        ts = MechStatus(mech) & (TORSO_LEFT | TORSO_RIGHT);
        MechStatus(mech) &= ~ts;
        if (!(InWeaponArc(mech, MechFX(target),
            MechFY(target)) & FORWARDARC)) {
            mech_notify(mech, MECHALL,
                "Your charge target is not in your forward arc and you are unable to charge it.");
            mech_charge = 0;
        }
        MechStatus(mech) |= ts;

        ts = MechStatus(target) & (TORSO_LEFT | TORSO_RIGHT);
        MechStatus(mech) &= ~ts;
        if (!(InWeaponArc(target, MechFX(mech),
            MechFY(mech)) & FORWARDARC)) {
            mech_notify(target, MECHALL,
                "Your charge target is not in your forward arc and you are unable to charge it.");
            target_charge = 0;
        }
        MechStatus(mech) |= ts;

        /* Now to calculate how much damage the first unit will do */
        if (mudconf.btech_newcharge)
            target_damage = (((((float)
                MechChargeDistance(mech)) * MP1) -
                MechSpeed(target) * cos((MechFacing(mech) -
                MechFacing(target)) * (M_PI / 180.))) *
                MP_PER_KPH) * (MechRealTons(mech) + 5) / 10;
        else
            target_damage =
                ((MechSpeed(mech) -
                MechSpeed(target) * cos((MechFacing(mech) -
                MechFacing(target)) * (M_PI / 180.))) *
                MP_PER_KPH) * (MechRealTons(mech) + 5) / 10;

        if (HasBoolAdvantage(MechPilot(mech), "melee_specialist"))
            target_damage++;

        /* Not able to do any damage */
        if (target_damage <= 0) {
            mech_notify(mech, MECHPILOT,
                "Your target pulls away from you and you are unable to charge it.");
            mech_charge = 0;
        }

        /* Now see how much damage the second unit will do */
        if (mudconf.btech_newcharge)
            mech_damage = (((((float)
                MechChargeDistance(target)) * MP1) -
                MechSpeed(mech) * cos((MechFacing(target) -
                MechFacing(mech)) * (M_PI / 180.))) *
                MP_PER_KPH) * (MechRealTons(target) + 5) / 10;
        else
            mech_damage =
                ((MechSpeed(target) -
                MechSpeed(mech) * cos((MechFacing(target) -
                MechFacing(mech)) * (M_PI / 180.))) *
                MP_PER_KPH) * (MechRealTons(target) + 5) / 10;

        if (HasBoolAdvantage(MechPilot(target), "melee_specialist"))
            mech_damage++;

        /* Not able to do any damage */
        if (mech_damage <= 0) {
            mech_notify(target, MECHPILOT,
                "Your target pulls away from you and you are unable to charge it.");
            target_charge = 0;
        }

        /* BTH for first unit */
        mech_baseToHit = 5;
        mech_baseToHit +=
            FindPilotPiloting(mech) - FindSPilotPiloting(target);

        mech_baseToHit += (HasBoolAdvantage(MechPilot(mech), "melee_specialist") ? 
            MIN(0, AttackMovementMods(mech) - 1) : AttackMovementMods(mech));

        mech_baseToHit += TargetMovementMods(mech, target, 0.0);

#ifdef BT_MOVEMENT_MODES
        if (Dodging(target))
            mech_baseToHit += 2;
#endif

        /* BTH for second unit */
        targ_baseToHit = 5;
        targ_baseToHit +=
            FindPilotPiloting(target) - FindSPilotPiloting(mech);

        targ_baseToHit += (HasBoolAdvantage(MechPilot(target), "melee_specialist") ? 
            MIN(0, AttackMovementMods(target) - 1 ) : AttackMovementMods(target));

        targ_baseToHit += TargetMovementMods(target, mech, 0.0);

#ifdef BT_MOVEMENT_MODES
        if (Dodging(mech))
            targ_baseToHit += 2;
#endif

        /* Now check to see if its possible for them to even charge */
        if (mech_charge)
            if (mech_baseToHit > 12) {
                mech_printf(mech, MECHALL,
                    "Charge: BTH %d\tYou choose not to charge.",
                    mech_baseToHit);
                mech_charge = 0;
            }

        if (target_charge)
            if (targ_baseToHit > 12) {
                mech_printf(target, MECHALL,
                    "Charge: BTH %d\tYou choose not to charge.",
                    targ_baseToHit);
                target_charge = 0;
            }

        /* Since neither can charge lets exit */
        if (!mech_charge && !target_charge) {
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

        if (mech_charge)
            mech_printf(mech, MECHALL, 
                "Charge: BTH %d\tRoll: %d", mech_baseToHit, 
                mech_roll);

        if (target_charge)
            mech_printf(target, MECHALL,
                "Charge: BTH %d\tRoll: %d", targ_baseToHit,
                targ_roll);

        /* Ok the first unit made its roll */
        if (mech_charge && mech_roll >= mech_baseToHit) {
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

            for (i = 0; i < spread; i++) {
                hitloc =
                    FindHitLocation(target, hitGroup, &iscritical,
                        &isrear);
                MyDamageMech(target, mech, 1, MechPilot(mech), hitloc,
                    isrear, iscritical, 5, 0);
            }

            if (target_damage % 5) {
                hitloc =
                    FindHitLocation(target, hitGroup, &iscritical,
                        &isrear);
                MyDamageMech(target, mech, 1, MechPilot(mech), hitloc,
                    isrear, iscritical, (target_damage % 5), 0);
            }

            hitGroup = FindAreaHitGroup(target, mech);
            isrear = (hitGroup == BACK);

            /* Ok now how much damage will the first unit take from
             * charging */
            if (mudconf.btech_newcharge && mudconf.btech_tl3_charge)
                target_damage = 
                    (((((float) MechChargeDistance(mech)) * MP1) -
                    MechSpeed(target) *
                    cos((MechFacing(mech) - MechFacing(target)) * (M_PI/180.))) *
                    MP_PER_KPH) *
                    (MechRealTons(mech) + 5) / 20;
            else
                target_damage = (MechRealTons(target) + 5) / 10; /* REUSED! */

            /* Record the damage for debugging then dish it out */
            received_damage = target_damage;
            spread = target_damage / 5;

            for (i = 0; i < spread; i++) {
                hitloc =
                    FindHitLocation(mech, hitGroup, &iscritical, &isrear);
                MyDamageMech2(mech, mech, 0, -1, hitloc, isrear,
                    iscritical, 5, 0);
            }

            if (target_damage % 5) {
                hitloc =
                    FindHitLocation(mech, hitGroup, &iscritical, &isrear);
                MyDamageMech2(mech, mech, 0, -1, hitloc, isrear,
                    iscritical, (target_damage % 5), 0);
            }

            /* Stop him */
            MechSpeed(mech) = 0;
            MechDesiredSpeed(mech) = 0;

            /* Emit the damage for debugging purposes */
            snprintf(emit_buff, LBUF_SIZE, "#%i charges #%i (%i/%i) Distance:"
                " %.2f DI: %i DR: %i", mech->mynum, target->mynum, mech_baseToHit,
                mech_roll, MechChargeDistance(mech), inflicted_damage,
                received_damage);
            SendDebug(emit_buff);

            /* Make the first unit roll for doing the charge if it is a mech */
            if (MechType(mech) == CLASS_MECH && !MadePilotSkillRoll(mech, 2)) {
                mech_notify(mech, MECHALL,
                    "Your piloting skill fails and you fall over!!");
                MechFalls(mech, 1, 1);
            }
            /* Make the second unit roll for receiving the charge if it is a mech */
            if (MechType(mech) == CLASS_MECH && !MadePilotSkillRoll(target, 2)) {
                mech_notify(target, MECHALL,
                    "Your piloting skill fails and you fall over!!");
                MechFalls(target, 1, 1);
            }
        }

        /* Ok the second unit made its roll */
        if (target_charge && targ_roll >= targ_baseToHit) {
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

            for (i = 0; i < spread; i++) {
                hitloc =
                    FindHitLocation(mech, hitGroup, &iscritical, &isrear);
                MyDamageMech(mech, target, 1, MechPilot(target), hitloc,
                    isrear, iscritical, 5, 0);
            }

            if (mech_damage % 5) {
                hitloc =
                    FindHitLocation(mech, hitGroup, &iscritical, &isrear);
                MyDamageMech(mech, target, 1, MechPilot(target), hitloc,
                    isrear, iscritical, (mech_damage % 5), 0);
            }

            hitGroup = FindAreaHitGroup(mech, target);
            isrear = (hitGroup == BACK);

            /* Ok now how much damage will the second unit take from
             * charging */
            if (mudconf.btech_newcharge && mudconf.btech_tl3_charge)
                target_damage = 
                    (((((float) MechChargeDistance(target)) * MP1) -
                    MechSpeed(mech) *
                    cos((MechFacing(target) - MechFacing(mech)) * (M_PI/180.))) *
                    MP_PER_KPH) *
                    (MechRealTons(mech) + 5) / 20;
            else
                target_damage = (MechRealTons(mech) + 5) / 10; /* REUSED! */

            /* Record the damage for debugging then dish it out */
            received_damage = target_damage;
            spread = target_damage / 5;

            for (i = 0; i < spread; i++) {
                hitloc =
                    FindHitLocation(target, hitGroup, &iscritical,
                        &isrear);
                MyDamageMech2(target, target, 0, -1, hitloc, isrear,
                    iscritical, 5, 0);
            }

            if (mech_damage % 5) {
                hitloc =
                    FindHitLocation(target, hitGroup, &iscritical,
                        &isrear);
                MyDamageMech2(target, target, 0, -1, hitloc, isrear,
                    iscritical, (mech_damage % 5), 0);
            }

            /* Stop him */
            MechSpeed(target) = 0;
            MechDesiredSpeed(target) = 0;

            /* Emit the damage for debugging purposes */
            snprintf(emit_buff, LBUF_SIZE, "#%i charges #%i (%i/%i) Distance:"
                " %.2f DI: %i DR: %i", target->mynum, mech->mynum, targ_baseToHit,
                targ_roll, MechChargeDistance(target), inflicted_damage,
                received_damage);
            SendDebug(emit_buff);

            if (MechType(mech) == CLASS_MECH && !MadePilotSkillRoll(mech, 2)) {
                mech_notify(mech, MECHALL,
                    "Your piloting skill fails and you fall over!!");
                MechFalls(mech, 1, 1);
            }
            if (MechType(target) == CLASS_MECH && !MadePilotSkillRoll(target, 2)) {
                mech_notify(target, MECHALL,
                    "Your piloting skill fails and you fall over!!");
                MechFalls(target, 1, 1);
            }
        }

        /* Cycle the sections so they can't make another attack for a while */
        if (MechType(mech) == CLASS_MECH) {
            for (i = 0; i < CHARGE_SECTIONS; i++)
                SetRecycleLimb(mech, resect[i], PHYSICAL_RECYCLE_TIME);
        } else {
            SetRecycleLimb(mech, FSIDE, PHYSICAL_RECYCLE_TIME);
            SetRecycleLimb(mech, TURRET, PHYSICAL_RECYCLE_TIME);
        }

        if (MechType(target) == CLASS_MECH) {
            for (i = 0; i < CHARGE_SECTIONS; i++)
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
    for (i = 0; i < CHARGE_SECTIONS; i++) {
        if (SectHasBusyWeap(mech, i)) {
            ArmorStringFromIndex(i, location, MechType(mech),
                MechMove(mech));
            mech_printf(mech, MECHALL,
                "You have weapons recycling on your %s.",
                    location);
            return;
        }
    }

    /* Check if they going fast enough to charge */
    DOCHECKMA(MechSpeed(mech) < MP1,
        "You aren't moving fast enough to charge.");

    /* Check to see if their sections cycling */
    if (MechType(mech) == CLASS_MECH) {
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
        (MechType(target) != CLASS_MECH),
        "You can only charge mechs!");

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
    if (mudconf.btech_newcharge)
        target_damage = (((((float)
            MechChargeDistance(mech)) * MP1) -
            MechSpeed(target) * cos((MechFacing(mech) -
            MechFacing(target)) * (M_PI / 180.))) *
            MP_PER_KPH) * (MechRealTons(mech) + 5) / 10 + 1;
    else
        target_damage =
            ((MechSpeed(mech) - MechSpeed(target) * cos((MechFacing(mech) -
            MechFacing(target)) * (M_PI / 180.))) *
            MP_PER_KPH) * (MechRealTons(mech) + 5) / 10 + 1;

        if (HasBoolAdvantage(MechPilot(mech), "melee_specialist"))
            target_damage++;

    /* Not enough damage done so no charge */
    DOCHECKMP(target_damage <= 0,
        "Your target pulls away from you and you are unable to charge it.");

    /* BTH */
    baseToHit += FindPilotPiloting(mech) - FindSPilotPiloting(target);

    baseToHit += (HasBoolAdvantage(MechPilot(mech), "melee_specialist") ? 
        MIN(0, AttackMovementMods(mech) - 1) : AttackMovementMods(mech));

    baseToHit += TargetMovementMods(mech, target, 0.0);

#ifdef BT_MOVEMENT_MODES
    if (Dodging(target))
        baseToHit += 2;
#endif

    DOCHECKMA(baseToHit > 12,
        tprintf("Charge: BTH %d\tYou choose not to charge.", baseToHit));

    /* Roll */
    roll = Roll();
    mech_printf(mech, MECHALL, "Charge: BTH %d\tRoll: %d",
        baseToHit, roll);

    /* Did the charge work ? */
    if (roll >= baseToHit) {
        /* OUCH */
        MechLOSBroadcasti(mech, target, tprintf("%ss %%s!",
            MechType(mech) == CLASS_MECH ? "charge" : "ram"));
        mech_printf(target, MECHSTARTED,
            "CRASH!!!\n%s %ss into you!", GetMechToMechID(target, mech),
                MechType(mech) == CLASS_MECH ? "charge" : "ram");
        mech_notify(mech, MECHALL, "SMASH!!! You crash into your target!");
        hitGroup = FindAreaHitGroup(mech, target);

        if (hitGroup == BACK)
            isrear = 1;
        else
            isrear = 0;

        /* Record the damage then dish it out */
        inflicted_damage = target_damage;
        spread = target_damage / 5;

        for (i = 0; i < spread; i++) {
            hitloc =
                FindHitLocation(target, hitGroup, &iscritical, &isrear);
            MyDamageMech(target, mech, 1, MechPilot(mech), hitloc, isrear,
                iscritical, 5, 0);
        }

        if (target_damage % 5) {
            hitloc =
                FindHitLocation(target, hitGroup, &iscritical, &isrear);
            MyDamageMech(target, mech, 1, MechPilot(mech), hitloc, isrear,
                iscritical, (target_damage % 5), 0);
        }

        hitGroup = FindAreaHitGroup(target, mech);
        isrear = (hitGroup == BACK);

        /* Damage done to the attacker for the charge */
        if (mudconf.btech_newcharge && mudconf.btech_tl3_charge)
            mech_damage =
                (((((float) MechChargeDistance(mech)) * MP1) -
                MechSpeed(target) *
                cos((MechFacing(mech) - MechFacing(target)) * (M_PI/180.))) *
                MP_PER_KPH) *
                (MechRealTons(target) + 5) / 20;
        else
            mech_damage = (MechRealTons(target) + 5) / 10;

        /* Record the damage then dish it out */
        received_damage = mech_damage;
        spread = mech_damage / 5;

        for (i = 0; i < spread; i++) {
            hitloc = FindHitLocation(mech, hitGroup, &iscritical, &isrear);
            MyDamageMech2(mech, mech, 0, -1, hitloc, isrear, iscritical, 5, 0);
        }

        if (mech_damage % 5) {
            hitloc = FindHitLocation(mech, hitGroup, &iscritical, &isrear);
            MyDamageMech2(mech, mech, 0, -1, hitloc, isrear, iscritical,
                (mech_damage % 5), 0);
        }

        /* Force piloting roll for attacker if they are in a mech */
        if (MechType(mech) == CLASS_MECH && !MadePilotSkillRoll(mech, 2)) {
            mech_notify(mech, MECHALL,
                "Your piloting skill fails and you fall over!!");
            MechFalls(mech, 1, 1);
        }

        /* Force piloting roll for target if they are in a mech */
        if (MechType(target) == CLASS_MECH && !MadePilotSkillRoll(target, 2)) {
            mech_notify(target, MECHSTARTED,
                "Your piloting skill fails and you fall over!!");
            MechFalls(target, 1, 1);
        }

        /* Stop him */
        MechSpeed(mech) = 0;
        MechDesiredSpeed(mech) = 0;

        /* Emit the damage for debugging purposes */
        snprintf(emit_buff, LBUF_SIZE, "#%i charges #%i (%i/%i) Distance:"
            " %.2f DI: %i DR: %i", mech->mynum, target->mynum, baseToHit,
            roll, MechChargeDistance(mech), inflicted_damage,
            received_damage);
        SendDebug(emit_buff);

    }

    /* Cycle the sections so they can't make another attack for a while */
    if (MechType(mech) == CLASS_MECH) {
        for (i = 0; i < CHARGE_SECTIONS; i++)
            SetRecycleLimb(mech, resect[i], PHYSICAL_RECYCLE_TIME);
    } else {
        SetRecycleLimb(mech, FSIDE, PHYSICAL_RECYCLE_TIME);
        SetRecycleLimb(mech, TURRET, PHYSICAL_RECYCLE_TIME);
    }
    return;
}

int checkGrabClubLocation(MECH * mech, int section, int emit)
{
    int tCanGrab = 1;
    char buf[100];
    char location[20];

    ArmorStringFromIndex(section, location, MechType(mech),
	MechMove(mech));

    if (SectIsDestroyed(mech, section)) {
	sprintf(buf, "Your %s is destroyed.", location);
	tCanGrab = 0;
    } else if (!OkayCritSectS(section, 3, HAND_OR_FOOT_ACTUATOR)) {
	sprintf(buf, "Your %s's hand actuator is destroyed or missing.",
	    location);
	tCanGrab = 0;
    } else if (!OkayCritSectS(section, 0, SHOULDER_OR_HIP)) {
	sprintf(buf,
	    "Your %s's shoulder actuator is destroyed or missing.",
	    location);
	tCanGrab = 0;
    } else if (SectHasBusyWeap(mech, section)) {
	sprintf(buf, "Your %s is still recovering from it's last attack.",
	    location);
	tCanGrab = 0;
    }

    if (!tCanGrab && emit)
	mech_notify(mech, MECHALL, buf);

    return tCanGrab;
}

void mech_grabclub(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    int wcArgs = 0;
    int location = 0;
    char *args[1];
    char locname[20];

    cch(MECH_USUALO);

    wcArgs = mech_parseattributes(buffer, args, 1);

    if (wcArgs >= 1) {
	if (toupper(args[0][0]) == '-') {
	    if ((MechSections(mech)[LARM].specials & CARRYING_CLUB) ||
		(MechSections(mech)[RARM].specials & CARRYING_CLUB)) {
		DropClub(mech);
	    } else {
		mech_notify(mech, MECHALL,
		    "You aren't currently carrying a club.");
	    }

	    return;
	}
    }

    DOCHECKMA(MechIsQuad(mech), "Quads can't carry around a club.");
    DOCHECKMA(Fallen(mech),
	"You can't grab a club while lying flat on your face.");
    DOCHECKMA(Jumping(mech), "Um, well, you're like jumping and stuff.");
    DOCHECKMA(OODing(mech), "You're too busy falling from the sky.");
    DOCHECKMA(UnJammingAmmo(mech), "You are too busy unjamming a weapon!");
    DOCHECKMA(RemovingPods(mech), "You are too busy removing iNARC pods!");
    DOCHECKMA(have_axe(mech, LARM) || have_axe(mech, RARM),
	"You can not grab a club if you carry an axe.");
    DOCHECKMA(have_sword(mech, LARM) || have_sword(mech, RARM),
	"You can not grab a club if you carry a sword.");
    DOCHECKMA(have_mace(mech, LARM) || have_mace(mech, RARM),
	"You can not grab a club if you carry an mace.");

    if (wcArgs == 0) {
	if (checkGrabClubLocation(mech, LARM, 0))
	    location = LARM;
	else if (checkGrabClubLocation(mech, RARM, 0))
	    location = RARM;
	else {
	    mech_notify(mech, MECHALL,
		"You don't have a free arm with a working hand actuator!");
	    return;
	}
    } else {
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
	    break;
	}

	if (!checkGrabClubLocation(mech, location, 1))
	    return;
    }

    DOCHECKMA(CarryingClub(mech), "You're already carrying a club.");
    DOCHECKMA(MechRTerrain(mech) != HEAVY_FOREST &&
	MechRTerrain(mech) != LIGHT_FOREST,
	"There don't appear to be any trees within grabbing distance.");

    ArmorStringFromIndex(location, locname, MechType(mech),
	MechMove(mech));

    MechLOSBroadcast(mech,
	"reaches down and yanks a tree out of the ground!");
    mech_printf(mech, MECHALL,
	"You reach down and yank a tree out of the ground with your %s.",
	    locname);

    MechSections(mech)[location].specials |= CARRYING_CLUB;
    SetRecycleLimb(mech, location, PHYSICAL_RECYCLE_TIME);
}
