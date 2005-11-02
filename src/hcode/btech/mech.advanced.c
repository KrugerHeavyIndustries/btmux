
/*
 * $Id: mech.advanced.c,v 1.3 2005/06/23 18:31:42 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Last modified: Tue Sep  8 10:12:37 1998 fingon
 *
 * Complete rewrite of the original MUSE code (because original sucked)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "mech.events.h"
#include "coolmenu.h"
#include "p.mech.ecm.h"
#include "mech.ecm.h"
#include "p.mech.utils.h"
#include "p.mech.update.h"
#include "p.mech.combat.h"
#include "p.mech.damage.h"
#include "p.artillery.h"
#include "p.btechstats.h"
#include "failures.h"
#include "p.crit.h"
#include "p.mech.build.h"
#include "p.mech.hitloc.h"
#include "p.mech.enhanced.criticals.h"
#include "p.mech.combat.misc.h"

#define SILLY_TOGGLE_MACRO(neededspecial,setstatus,msgon,msgoff,donthave) \
if (MechSpecials(mech) & (neededspecial)) \
{ if (MechStatus(mech) & (setstatus)) { mech_notify(mech, MECHALL, msgoff); \
MechStatus(mech) &= ~(setstatus); } else { mech_notify(mech,MECHALL, msgon); \
MechStatus(mech) |= (setstatus); } } else notify(player, donthave)

#define TOGGLE_SPECIALS_MACRO_CHECK(neededspecial,setstatus,offstatus,msgon,msgoff,donthave) \
if (MechSpecials(mech) & (neededspecial)) \
{ if (MechStatus2(mech) & (setstatus)) { mech_notify(mech, MECHALL, msgoff); \
MechStatus2(mech) &= ~(setstatus); } else { mech_notify(mech,MECHALL, msgon); \
MechStatus2(mech) |= (setstatus); MechStatus2(mech) &= ~(offstatus); } } else notify(player, donthave)

#define TOGGLE_SPECIALS_MACRO_CHECK2(neededspecial,setstatus,offstatus,msgon,msgoff,donthave) \
if (MechSpecials2(mech) & (neededspecial)) \
{ if (MechStatus2(mech) & (setstatus)) { mech_notify(mech, MECHALL, msgoff); \
MechStatus2(mech) &= ~(setstatus); } else { mech_notify(mech,MECHALL, msgon); \
MechStatus2(mech) |= (setstatus); MechStatus2(mech) &= ~(offstatus); } } else notify(player, donthave)

#define TOGGLE_INFANTRY_MACRO_CHECK(neededspecial,setstatus,offstatus,msgon,msgoff,donthave) \
if (MechInfantrySpecials(mech) & (neededspecial)) \
{ if (MechStatus2(mech) & (setstatus)) { mech_notify(mech, MECHALL, msgoff); \
MechStatus2(mech) &= ~(setstatus); } else { mech_notify(mech,MECHALL, msgon); \
MechStatus2(mech) |= (setstatus); MechStatus2(mech) &= ~(offstatus); } } else notify(player, donthave)

/* Toggles ECM on / off */
void mech_ecm(dbref player, MECH * mech, char *buffer)
{
    cch(MECH_USUALO);
    DOCHECK(MechCritStatus(mech) & ECM_DESTROYED,
	"Your Guardian ECM has been destroyed already!");
    TOGGLE_SPECIALS_MACRO_CHECK(ECM_TECH, ECM_ENABLED, ECCM_ENABLED,
	"You turn your ECM suite online (ECM mode).",
	"You turn your ECM suite offline.",
	"This unit isn't equipped with an ECM suite!");
    MarkForLOSUpdate(mech);
}

void mech_eccm(dbref player, MECH * mech, char *buffer)
{
    cch(MECH_USUALO);
    DOCHECK(MechCritStatus(mech) & ECM_DESTROYED,
	"Your Guardian ECM has been destroyed already!");
    TOGGLE_SPECIALS_MACRO_CHECK(ECM_TECH, ECCM_ENABLED, ECM_ENABLED,
	"You turn your ECM suite online (ECCM mode).",
	"You turn your ECM suite offline.",
	"This unit isn't equipped with an ECM suite!");
    MarkForLOSUpdate(mech);
}

void mech_perecm(dbref player, MECH * mech, char *buffer)
{
    cch(MECH_USUALO);
    TOGGLE_INFANTRY_MACRO_CHECK(FC_INFILTRATORII_STEALTH_TECH,
	PER_ECM_ENABLED, PER_ECCM_ENABLED,
	"You turn your Personal ECM suite online (ECM mode).",
	"You turn your Personal ECM suite offline.",
	"This unit isn't equipped with a Personal ECM suite!");
    MarkForLOSUpdate(mech);
}

void mech_pereccm(dbref player, MECH * mech, char *buffer)
{
    cch(MECH_USUALO);
    TOGGLE_INFANTRY_MACRO_CHECK(FC_INFILTRATORII_STEALTH_TECH,
	PER_ECCM_ENABLED, PER_ECM_ENABLED,
	"You turn your Personal ECM suite online (ECCM mode).",
	"You turn your Personal ECM suite offline.",
	"This unit isn't equipped with a Personal ECM suite!");
    MarkForLOSUpdate(mech);
}

void mech_angelecm(dbref player, MECH * mech, char *buffer)
{
    cch(MECH_USUALO);
    DOCHECK(MechCritStatus(mech) & ANGEL_ECM_DESTROYED,
	"Your Angel ECM has been destroyed already!");
    TOGGLE_SPECIALS_MACRO_CHECK2(ANGEL_ECM_TECH, ANGEL_ECM_ENABLED,
	ANGEL_ECCM_ENABLED,
	"You turn your Angel ECM suite online (ECM mode).",
	"You turn your Angel ECM suite offline.",
	"This unit isn't equipped with an Angel ECM suite!");
    MarkForLOSUpdate(mech);
}

void mech_angeleccm(dbref player, MECH * mech, char *buffer)
{
    cch(MECH_USUALO);
    DOCHECK(MechCritStatus(mech) & ANGEL_ECM_DESTROYED,
	"Your Angel ECM has been destroyed already!");
    TOGGLE_SPECIALS_MACRO_CHECK2(ANGEL_ECM_TECH, ANGEL_ECCM_ENABLED,
	ANGEL_ECM_ENABLED,
	"You turn your Angel ECM suite online (ECCM mode).",
	"You turn your Angel ECM suite offline.",
	"This unit isn't equipped with an Angel ECM suite!");
    MarkForLOSUpdate(mech);
}

void MechSliteChangeEvent(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;
    int wType = (int) e->data2;

    if (MechCritStatus(mech) & SLITE_DEST)
	return;

    if (!Started(mech))
	return;

    if (!Started(mech)) {
	MechStatus2(mech) &= ~SLITE_ON;
	MechCritStatus(mech) &= ~SLITE_LIT;
	return;
    }

    if (wType == 1) {
	MechStatus2(mech) |= SLITE_ON;
	MechCritStatus(mech) |= SLITE_LIT;

	mech_notify(mech, MECHALL,
	    "Your searchlight comes on to full power.");
	MechLOSBroadcast(mech, "turns on a searchlight!");
    } else {
	MechStatus2(mech) &= ~SLITE_ON;
	MechCritStatus(mech) &= ~SLITE_LIT;

	mech_notify(mech, MECHALL, "Your searchlight shuts off.");
	MechLOSBroadcast(mech, "turns off a searchlight!");
    }

    MarkForLOSUpdate(mech);
}

void mech_slite(dbref player, MECH * mech, char *buffer)
{
    cch(MECH_USUALO);

    if (!(MechSpecials(mech) & SLITE_TECH)) {
	mech_notify(mech, MECHALL,
	    "Your 'mech isn't equipped with searchlight!");
	return;
    }

    DOCHECK(MechCritStatus(mech) & SLITE_DEST,
	"Your searchlight has been destroyed already!");

    if (SearchlightChanging(mech)) {
	if (MechStatus2(mech) & SLITE_ON)
	    mech_notify(mech, MECHALL,
		"Your searchlight is already in the process of turning off.");
	else
	    mech_notify(mech, MECHALL,
		"Your searchlight is already in the process of turning on.");

	return;
    }

    if (MechStatus2(mech) & SLITE_ON) {
	mech_notify(mech, MECHALL,
	    "Your searchlight starts to cool down.");
	MECHEVENT(mech, EVENT_SLITECHANGING, MechSliteChangeEvent, 5, 0);
    } else {
	mech_notify(mech, MECHALL, "Your searchlight starts to warm up.");
	MECHEVENT(mech, EVENT_SLITECHANGING, MechSliteChangeEvent, 5, 1);
    }
}

void mech_ams(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);

    SILLY_TOGGLE_MACRO(IS_ANTI_MISSILE_TECH | CL_ANTI_MISSILE_TECH,
	AMS_ENABLED, "Anti-Missile System turned ON",
	"Anti-Missile System turned OFF",
	"This mech is not equipped with AMS");
}

void mech_fliparms(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    DOCHECK(Fallen(mech),
	"You're using your arms to support yourself. Try flipping something else.");
    SILLY_TOGGLE_MACRO(FLIPABLE_ARMS, FLIPPED_ARMS,
	"Arms have been flipped to BACKWARD position",
	"Arms have been flipped to FORWARD position",
	"You cannot flip the arms in this mech");
}

/* Parameters:
   <player,mech,buffer> = parent's input
   nspecisspec, nspec
	 5 = check that it's a TAMMO weapon and has the specified spec
   4 = check that weapon's SRM (or SSRM)
   3 = check that weapon's NARC beacon
   2 = check that weapon's LRM
   1 = compare nspec to weapon's special
   0 = compare nspec to weapon's type and ensure it isn't NARCbcn
   -1 = compare nspec to weapon's type & check for Artemis
   mode           = mode to set if nspec check is successful
   onmsg          = msg for turning mode on
   offmsg         = msg for turning mode off
   cant           = system lacks nspec
 */

static int temp_nspecisspec, temp_nspec, temp_mode, temp_firemode;
static char *temp_onmsg, *temp_offmsg, *temp_cant;

static int mech_toggle_mode_sub_func(MECH * mech, dbref player, int index,
    int high)
{
    int section, critical, weaptype;

    weaptype =
	FindWeaponNumberOnMech_Advanced(mech, index, &section, &critical,
	0);

    DOCHECK0(weaptype == -1,
	"The weapons system chirps: 'Illegal Weapon Number!'");
    DOCHECK0(weaptype == -2,
	"The weapons system chirps: 'That Weapon has been destroyed!'");
    DOCHECK0(weaptype == -3,
	"The weapon system chirps: 'That weapon is still reloading!'");
    DOCHECK0(weaptype == -4,
	"The weapon system chirps: 'That weapon is still recharging!'");
    DOCHECK0(GetPartFireMode(mech, section, critical) & OS_MODE,
	"One-shot weapons' mode cannot be altered!");
    DOCHECK0(isWeapAmmoFeedLocked(mech, section, critical),
	"That weapon's ammo feed mechanism is damaged!");

    if (temp_nspec == -1)
	DOCHECK0(!FindArtemisForWeapon(mech, section, critical),
	    "You have no Artemis system for that weapon.");

    weaptype = Weapon2I(GetPartType(mech, section, critical));

    DOCHECK0(MechWeapons[weaptype].special & ROCKET,
	"Rocket launchers' mode cannot be altered!");

    if ((temp_nspecisspec == 5 && (MechWeapons[weaptype].type == TAMMO)
	    && !(MechWeapons[weaptype].special & temp_nspec)) ||
	(temp_nspecisspec == 4 && (MechWeapons[weaptype].type == TMISSILE)
	    && !(MechWeapons[weaptype].type & (IDF | DAR))) ||
	(temp_nspecisspec == 2 && (MechWeapons[weaptype].special & IDF) &&
	    !(MechWeapons[weaptype].special & DAR)) ||
	(temp_nspecisspec == 1 && temp_nspec &&
	    (MechWeapons[weaptype].special & temp_nspec)) ||
	(temp_nspecisspec <= 0 && temp_nspec &&
	    (MechWeapons[weaptype].type == temp_nspec &&
		!(MechWeapons[weaptype].special & NARC)))) {

	if (temp_nspecisspec == 0 && (temp_nspec & TARTILLERY))
	    DOCHECK0((GetPartAmmoMode(mech, section,
			critical) & ARTILLERY_MODES) &&
		!(GetPartAmmoMode(mech, section, critical) & temp_mode),
		"That weapon has already been set to fire special rounds!");

	if (temp_firemode) {
	    if (GetPartFireMode(mech, section, critical) & temp_mode) {
		GetPartFireMode(mech, section, critical) &= ~temp_mode;
		mech_notify(mech, MECHALL, tprintf(temp_offmsg, index));
		return 0;
	    }
	} else {
	    if (GetPartAmmoMode(mech, section, critical) & temp_mode) {
		GetPartAmmoMode(mech, section, critical) &= ~temp_mode;
		mech_notify(mech, MECHALL, tprintf(temp_offmsg, index));
		return 0;
	    }
	}

	if (temp_firemode) {
	    GetPartFireMode(mech, section, critical) &= ~FIRE_MODES;
	    GetPartFireMode(mech, section, critical) |= temp_mode;
	} else {
	    GetPartAmmoMode(mech, section, critical) &= ~AMMO_MODES;
	    GetPartAmmoMode(mech, section, critical) |= temp_mode;
	}

	mech_notify(mech, MECHALL, tprintf(temp_onmsg, index));

	return 0;
    }

    notify(player, temp_cant);
    return 0;
}

static void mech_toggle_mode_sub(dbref player, MECH * mech, char *buffer,
    int nspecisspec, int nspec, int mode, int tFireMode, char *onmsg,
    char *offmsg, char *cant)
{
    char *args[1];

    DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
	"Please specify a weapon number.");
    temp_nspecisspec = nspecisspec;
    temp_nspec = nspec;
    temp_mode = mode;
    temp_onmsg = onmsg;
    temp_offmsg = offmsg;
    temp_cant = cant;
    temp_firemode = tFireMode;
    multi_weap_sel(mech, player, args[0], 1, mech_toggle_mode_sub_func);
}

void mech_flamerheat(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 1, CHEAT, HEAT_MODE, 1,
	"Weapon %d has been set to HEAT mode",
	"Weapon %d has been set to normal mode",
	"That weapon cannot be set HEAT!");
}

void mech_ultra(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 1, ULTRA, ULTRA_MODE, 1,
	"Weapon %d has been set to ultra fire mode",
	"Weapon %d has been set to normal fire mode",
	"That weapon cannot be set ULTRA!");
}

void mech_inarc_ammo_toggle(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    int wSection, wCritSlot, wWeapType;
    int wWeapNum = 0;
    int wcArgs = 0;
    char *args[2];
    char strMode[30];

    cch(MECH_USUALO);

    wcArgs = mech_parseattributes(buffer, args, 2);

    DOCHECK(wcArgs < 1, "Please specify a weapon number.");
    DOCHECK(Readnum(wWeapNum, args[0]), tprintf("Invalid value: %s",
	    args[0]));

    wWeapType =
	FindWeaponNumberOnMech(mech, wWeapNum, &wSection, &wCritSlot);

    DOCHECK(wWeapType == -1,
	"The weapons system chirps: 'Illegal Weapon Number!'");
    DOCHECK(wWeapType == -2,
	"The weapons system chirps: 'That Weapon has been destroyed!'");
    DOCHECK(wWeapType == -3,
	"The weapon system chirps: 'That weapon is still reloading!'");
    DOCHECK(wWeapType == -4,
	"The weapon system chirps: 'That weapon is still recharging!'");
    DOCHECK(!(MechWeapons[wWeapType].special & INARC),
	"The weapon system chirps: 'That weapon is not an iNARC launcher!'");
    DOCHECK(isWeapAmmoFeedLocked(mech, wSection, wCritSlot),
	"That weapon's ammo feed mechanism is damaged!");

    /* Change our modes... */

    GetPartAmmoMode(mech, wSection, wCritSlot) &= ~AMMO_MODES;

    if (wcArgs < 2)
	strcpy(strMode, "Homing");
    else {
	switch (toupper(args[1][0])) {
	case 'X':
	    strcpy(strMode, "Explosive");
	    GetPartAmmoMode(mech, wSection, wCritSlot) |= INARC_EXPLO_MODE;
	    break;
	case 'Y':
	    strcpy(strMode, "Haywire");
	    GetPartAmmoMode(mech, wSection, wCritSlot) |=
		INARC_HAYWIRE_MODE;
	    break;
	case 'E':
	    strcpy(strMode, "ECM");
	    GetPartAmmoMode(mech, wSection, wCritSlot) |= INARC_ECM_MODE;
	    break;
	case 'Z':
	    strcpy(strMode, "Nemesis");
	    GetPartAmmoMode(mech, wSection, wCritSlot) |=
		INARC_NEMESIS_MODE;
	    break;
	default:
	    strcpy(strMode, "Homing");
	    break;
	}
    }

    mech_notify(mech, MECHALL,
	tprintf("Weapon %d set to fire iNARC %s pods.", wWeapNum,
	    strMode));
}

void mech_explosive(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);

    mech_toggle_mode_sub(player, mech, buffer, 1, NARC, NARC_MODE, 0,
	"Weapon %d has been set to fire explosive rounds",
	"Weapon %d has been set to fire NARC beacons",
	"That weapon cannot be set to fire explosive rounds!");
}

void mech_lbx(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 1, LBX, LBX_MODE, 0,
	"Weapon %d has been set to LBX fire mode",
	"Weapon %d has been set to normal fire mode",
	"That weapon cannot be set LBX!");
}

void mech_armorpiercing(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 1, RFAC, AC_AP_MODE, 0,
	"Weapon %d has been set to fire AP rounds",
	"Weapon %d has been set to fire normal rounds",
	"That weapon cannot fire AP rounds!");
}

void mech_flechette(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 1, RFAC, AC_FLECHETTE_MODE,
	0, "Weapon %d has been set to fire Flechette rounds",
	"Weapon %d has been set to fire normal rounds",
	"That weapon cannot fire Flechette rounds!");
}

void mech_incendiary(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 1, RFAC, AC_INCENDIARY_MODE,
	0, "Weapon %d has been set to fire Incendiary rounds",
	"Weapon %d has been set to fire normal rounds",
	"That weapon cannot fire Incendiary rounds!");
}

void mech_precision(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 1, RFAC, AC_PRECISION_MODE,
	0, "Weapon %d has been set to fire Precision rounds",
	"Weapon %d has been set to fire normal rounds",
	"That weapon cannot fire Precision rounds!");
}

void mech_rapidfire(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 1, RFAC, RFAC_MODE, 1,
	"Weapon %d has been set to Rapid Fire mode",
	"Weapon %d has been set to normal fire mode",
	"That weapon cannot be set to do rapid fire!");
}

void mech_rac(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    int wSection, wCritSlot, wWeapType;
    int wWeapNum = 0;
    int wcArgs = 0;
    char *args[2];
    char strMode[30];

    cch(MECH_USUALO);

    wcArgs = mech_parseattributes(buffer, args, 2);

    DOCHECK(wcArgs < 1, "Please specify a weapon number.");
    DOCHECK(Readnum(wWeapNum, args[0]), tprintf("Invalid value: %s",
	    args[0]));

    wWeapType =
	FindWeaponNumberOnMech(mech, wWeapNum, &wSection, &wCritSlot);

    DOCHECK(wWeapType == -1,
	"The weapons system chirps: 'Illegal Weapon Number!'");
    DOCHECK(wWeapType == -2,
	"The weapons system chirps: 'That Weapon has been destroyed!'");
    DOCHECK(wWeapType == -3,
	"The weapon system chirps: 'That weapon is still reloading!'");
    DOCHECK(wWeapType == -4,
	"The weapon system chirps: 'That weapon is still recharging!'");
    DOCHECK(!(MechWeapons[wWeapType].special & RAC),
	"The weapon system chirps: 'That weapon is not an Rotary AC!'");

    /* Change our modes... */

    GetPartFireMode(mech, wSection, wCritSlot) &= ~FIRE_MODES;
    GetPartFireMode(mech, wSection, wCritSlot) &=
	~(RAC_TWOSHOT_MODE | RAC_FOURSHOT_MODE | RAC_SIXSHOT_MODE);

    if (wcArgs < 2)
	strcpy(strMode, "one shot");
    else {
	switch (args[1][0]) {
	case '2':
	    strcpy(strMode, "two shots");
	    GetPartFireMode(mech, wSection, wCritSlot) |= RAC_TWOSHOT_MODE;
	    break;
	case '4':
	    strcpy(strMode, "four shots");
	    GetPartFireMode(mech, wSection, wCritSlot) |=
		RAC_FOURSHOT_MODE;
	    break;
	case '6':
	    strcpy(strMode, "six shots");
	    GetPartFireMode(mech, wSection, wCritSlot) |= RAC_SIXSHOT_MODE;
	    break;
	default:
	    strcpy(strMode, "one shot");
	    break;
	}
    }

    mech_notify(mech, MECHALL,
	tprintf("Weapon %d set to fire %s at a time.", wWeapNum, strMode));
}

static int mech_unjamammo_func(MECH * mech, dbref player, int index,
    int high)
{
    int section, critical, weaptype;
    int i;
    char location[50];

    weaptype = FindWeaponNumberOnMech(mech, index, &section, &critical);
    DOCHECK0(weaptype == -1,
	"The weapons system chirps: 'Illegal Weapon Number!'");
    DOCHECK0(weaptype == -2,
	"The weapons system chirps: 'That Weapon has been destroyed!'");
    DOCHECK0(PartTempNuke(mech, section, critical) != FAIL_AMMOJAMMED,
	"The ammo feed mechanism for that weapon is not jammed.");
    DOCHECK0(Jumping(mech),
	"You can't unjam the ammo feed while jumping!");
    DOCHECK0(IsRunning(MechDesiredSpeed(mech), MMaxSpeed(mech)),
	"You can't unjam the ammo feed while running!");

    for (i = 0; i < NUM_SECTIONS; i++) {
	if (SectHasBusyWeap(mech, i)) {
	    ArmorStringFromIndex(i, location, MechType(mech),
		MechMove(mech));
	    mech_notify(mech, MECHALL,
		tprintf("You have weapons recycling on your %s.",
		    location));
	    return 0;
	}
    }

    DOCHECK0(UnJammingAmmo(mech), "You are already unjamming a weapon!");

    MECHEVENT(mech, EVENT_UNJAM_AMMO, mech_unjam_ammo_event, 60, index);
    mech_notify(mech, MECHALL,
	tprintf("You begin to shake the jammed ammo loose on weapon #%d",
	    index));
    return 0;
}
void mech_unjamammo(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    char *args[1];

    cch(MECH_USUALMO);
    DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
	"Please specify a weapon number.");
    multi_weap_sel(mech, player, args[0], 1, mech_unjamammo_func);
}

void mech_gattling(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 1, GMG, GATTLING_MODE, 1,
	"Weapon %d has been set to Gattling mode",
	"Weapon %d has been set to normal fire mode",
	"That weapon cannot be set to do gattling fire!");
}

void mech_artemis(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, -1, TMISSILE, ARTEMIS_MODE,
	0,
	"Weapon %d has been set to fire Artemis IV compatible missiles.",
	"Weapon %d has been set to fire normal missiles",
	"That weapon cannot be set ARTEMIS!");
}

void mech_narc(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 0, TMISSILE, NARC_MODE, 0,
	"Weapon %d has been set to fire Narc Beacon compatible missiles.",
	"Weapon %d has been set to fire normal missiles",
	"That weapon cannot be set NARC!");
}

void mech_swarm(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 2, 0, SWARM_MODE, 0,
	"Weapon %d has been set to fire Swarm missiles.",
	"Weapon %d has been set to fire normal missiles",
	"That weapon cannot be set to fire Swarm missiles!");
}

void mech_swarm1(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 2, 0, SWARM1_MODE, 0,
	"Weapon %d has been set to fire Swarm1 missiles.",
	"Weapon %d has been set to fire normal missiles",
	"That weapon cannot be set to fire Swarm1 missiles!");
}

void mech_inferno(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 4, 0, INFERNO_MODE, 0,
	"Weapon %d has been set to fire Inferno missiles.",
	"Weapon %d has been set to fire normal missiles",
	"That weapon cannot be set to fire Inferno missiles!");
}

void mech_hotload(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    int wSection, wCritSlot, wWeapType;
    int wWeapNum = 0;
    int wcArgs = 0;
    char *args[1];

    cch(MECH_USUALO);

    wcArgs = mech_parseattributes(buffer, args, 1);

    DOCHECK(wcArgs < 1, "Please specify a weapon number.");
    DOCHECK(Readnum(wWeapNum, args[0]), tprintf("Invalid value: %s",
	    args[0]));

    wWeapType =
	FindWeaponNumberOnMech(mech, wWeapNum, &wSection, &wCritSlot);

    DOCHECK(wWeapType == -1,
	"The weapons system chirps: 'Illegal Weapon Number!'");
    DOCHECK(wWeapType == -2,
	"The weapons system chirps: 'That Weapon has been destroyed!'");
    DOCHECK(wWeapType == -3,
	"The weapon system chirps: 'That weapon is still reloading!'");
    DOCHECK(wWeapType == -4,
	"The weapon system chirps: 'That weapon is still recharging!'");
    DOCHECK(!(MechWeapons[wWeapType].special & IDF),
	"The weapon system chirps: 'That weapon can not be hotloaded!'");

    if (GetPartFireMode(mech, wSection, wCritSlot) & HOTLOAD_MODE) {
	mech_notify(mech, MECHALL,
	    tprintf("Hotloading for weapon %d has been toggled off.",
		wWeapNum));
	GetPartFireMode(mech, wSection, wCritSlot) &= ~HOTLOAD_MODE;
    } else {
	mech_notify(mech, MECHALL,
	    tprintf("Hotloading for weapon %d has been toggled on.",
		wWeapNum));
	GetPartFireMode(mech, wSection, wCritSlot) |= HOTLOAD_MODE;
    }
}

void mech_cluster(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 0, TARTILLERY, CLUSTER_MODE,
	0, "Weapon %d has been set to fire cluster rounds.",
	"Weapon %d has been set to fire normal rounds",
	"Invalid weapon type!");
}

void mech_smoke(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 0, TARTILLERY, SMOKE_MODE,
	0, "Weapon %d has been set to fire smoke rounds.",
	"Weapon %d has been set to fire normal rounds",
	"Invalid weapon type!");
}

void mech_mine(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    mech_toggle_mode_sub(player, mech, buffer, 0, TARTILLERY, MINE_MODE, 0,
	"Weapon %d has been set to fire mine rounds.",
	"Weapon %d has been set to fire normal rounds",
	"Invalid weapon type!");
}

static void mech_mascr_event(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;

    if (MechMASCCounter(mech) > 0) {
	MechMASCCounter(mech)--;
	MECHEVENT(mech, EVENT_MASC_REGEN, mech_mascr_event, MASC_TICK, 0);
    }
}

static void mech_masc_event(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;
#ifndef BT_MOVEMENT_MODES
    int needed = 2 * (1 + (MechMASCCounter(mech)++));
#else
    int needed = (2 * (1 + (MechMASCCounter(mech)++))) +
	    	 ((MechStatus2(mech) & SCHARGE_ENABLED) ? 1 : 0) +
		 (MechStatus2(mech) & SPRINTING ? 2 : 0);
#endif
    int roll = Roll();

    if (!Started(mech))
	return;
    if (!(MechSpecials(mech) & MASC_TECH))
	return;
    if (MechStatus(mech) & SCHARGE_ENABLED)
	roll--;
    if (needed < 10 && Good_obj(MechPilot(mech)) && WizP(MechPilot(mech)))
	roll = Number(needed + 1, 12);
    mech_notify(mech, MECHALL, tprintf("MASC: BTH %d+, Roll: %d",
	    needed + 1, roll));
    if (roll > needed) {
	MECHEVENT(mech, EVENT_MASC_FAIL, mech_masc_event, MASC_TICK, 0);
	return;
    }
    MechSpecials(mech) &= ~MASC_TECH;
    MechStatus(mech) &= ~MASC_ENABLED;
    if (fabs(MechSpeed(mech)) > MP1) {
	mech_notify(mech, MECHALL,
	    "Your leg actuators freeze suddenly, and you fall!");
	MechLOSBroadcast(mech, "stops and falls in mid-step!");
	MechFalls(mech, 1, 0);
    } else {
	mech_notify(mech, MECHALL, "Your leg actuators freeze suddenly!");
	if (MechSpeed(mech) > 0.0)
	    MechLOSBroadcast(mech, "stops suddenly!");
    }

    /* Break the Hips - FASA canon rule about MASC */
    DestroyPart(mech, RLEG, 0);
    DestroyPart(mech, LLEG, 0);
    if (MechMove(mech) == MOVE_QUAD) {
        DestroyPart(mech, RARM, 0);
        DestroyPart(mech, LARM, 0);
    }
    
    /* Let the MUX know both hips gone */
    MechCritStatus(mech) |= HIP_DESTROYED;

    /* Reset the Speeds, this sets all 3 of them */
    SetMaxSpeed(mech, 0.0);
}

void mech_masc(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    DOCHECK(!(MechSpecials(mech) & MASC_TECH),
	"Your toy ain't prepared for what you're askin' it!");
    DOCHECK(MMaxSpeed(mech) < MP1, "Uh huh.");
    if (MechStatus(mech) & MASC_ENABLED) {
	mech_notify(mech, MECHALL, "MASC has been turned off.");
	MechStatus(mech) &= ~MASC_ENABLED;
	MechDesiredSpeed(mech) = MechDesiredSpeed(mech) * 3. / 4.;
	StopMasc(mech);
	MECHEVENT(mech, EVENT_MASC_REGEN, mech_mascr_event, MASC_TICK, 0);
	return;
    }
    mech_notify(mech, MECHALL, "MASC has been turned on.");
    MechStatus(mech) |= MASC_ENABLED;
    StopMascR(mech);
    MechDesiredSpeed(mech) = MechDesiredSpeed(mech) * 4. / 3.;
    MECHEVENT(mech, EVENT_MASC_FAIL, mech_masc_event, 1, 0);
}

static void mech_scharger_event(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;

    if (MechSChargeCounter(mech) > 0) {
	MechSChargeCounter(mech)--;
	MECHEVENT(mech, EVENT_SCHARGE_REGEN, mech_scharger_event,
	    SCHARGE_TICK, 0);
    }
}

static void mech_scharge_event(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;
#ifndef BT_MOVEMENT_MODES
    int needed = 2 * (1 + (MechSChargeCounter(mech)++));
#else
    int needed = (2 * (1 + (MechMASCCounter(mech)++))) +
	    	 ((MechStatus(mech) & MASC_ENABLED) ? 1 : 0) +
		 (MechStatus2(mech) & SPRINTING ? 2 : 0);
#endif
    int roll = Roll();
    int j, count = 0;
    int maxspeed, newmaxspeed = 0;
    int critType;
    char msgbuf[MBUF_SIZE];

    if (!Started(mech))
	return;
    if (!(MechSpecials2(mech) & SUPERCHARGER_TECH))
	return;
    if (MechStatus(mech) & MASC_ENABLED)
	roll = roll - 1;
    if (needed < 10 && Good_obj(MechPilot(mech)) && WizP(MechPilot(mech)))
	roll = Number(needed + 1, 12);
    mech_notify(mech, MECHALL, tprintf("Supercharger: BTH %d, Roll: %d",
	    needed + 1, roll));
    if (roll > needed) {
	MECHEVENT(mech, EVENT_SCHARGE_FAIL, mech_scharge_event,
	    SCHARGE_TICK, 0);
	return;
    }

    MechSpecials2(mech) &= ~SUPERCHARGER_TECH;
    MechStatus(mech) &= ~SCHARGE_ENABLED;

    mech_notify(mech, MECHALL,
	"Your supercharger overloads and explodes!");

    if (MechType(mech) == CLASS_MECH) {
	for (j = 0; j < CritsInLoc(mech, CTORSO); j++) {
	    critType = GetPartType(mech, CTORSO, j);
	    if (critType == Special(SUPERCHARGER)) {
		if (!PartIsDestroyed(mech, CTORSO, j))
		    DestroyPart(mech, CTORSO, j);
	    }
	}

	count = Number(1, 4);

	for (j = 0; count && j < CritsInLoc(mech, CTORSO); j++) {
	    critType = GetPartType(mech, CTORSO, j);
	    if (critType == Special(ENGINE) &&
		!PartIsDestroyed(mech, CTORSO, j)) {
		DestroyPart(mech, CTORSO, j);
		if (!Destroyed(mech) && Started(mech)) {
		    sprintf(msgbuf, "'s center torso spews black smoke!");
		    MechLOSBroadcast(mech, msgbuf);
		}
		if (MechEngineHeat(mech) < 10) {
		    MechEngineHeat(mech) += 5;
		    mech_notify(mech, MECHALL,
			"Your engine shielding takes a hit!  It's getting hotter in here!!");
		} else if (MechEngineHeat(mech) < 15) {
		    MechEngineHeat(mech) = 15;
		    mech_notify(mech, MECHALL,
			"Your engine is destroyed!!");
		    DestroyMech(mech, mech, 1);
		}
		count--;
	    }
	}
    }

    if ((MechType(mech) == CLASS_VTOL) ||
	(MechType(mech) == CLASS_VEH_GROUND)) {
	sprintf(msgbuf, " coughs think black smoke from its exhaust.");
	MechLOSBroadcast(mech, msgbuf);
	maxspeed = MechMaxSpeed(mech);
	newmaxspeed = (maxspeed * .5);
	SetMaxSpeed(mech, newmaxspeed);
    }
}

void mech_scharge(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALMO);
    DOCHECK(!(MechSpecials2(mech) & SUPERCHARGER_TECH),
	"Your toy ain't prepared for what you're askin' it!");
    DOCHECK(MMaxSpeed(mech) < MP1, "Uh huh.");
    if (MechStatus(mech) & SCHARGE_ENABLED) {
	mech_notify(mech, MECHALL, "Supercharger has been turned off.");
	MechStatus(mech) &= ~SCHARGE_ENABLED;
	MechDesiredSpeed(mech) = MechDesiredSpeed(mech) * 3. / 4.;
	StopSCharge(mech);
	MECHEVENT(mech, EVENT_SCHARGE_REGEN, mech_scharger_event,
	    SCHARGE_TICK, 0);
	return;
    }
    mech_notify(mech, MECHALL, "Supercharger has been turned on.");
    MechStatus(mech) |= SCHARGE_ENABLED;
    StopSChargeR(mech);
    MechDesiredSpeed(mech) = MechDesiredSpeed(mech) * 4. / 3.;
    MECHEVENT(mech, EVENT_SCHARGE_FAIL, mech_scharge_event, 1, 0);
}

int doing_explode = 0;

static void mech_explode_event(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;
    MAP *map;
    int extra = (int) e->data2;
    int i, j, damage;
    int z;
    int dam;

    if (Destroyed(mech) || !Started(mech))
        return;

    if (extra > 256 && !FindDestructiveAmmo(mech, &i, &j))
        return;

    if ((--extra) % 256) {

        mech_notify(mech, MECHALL,
                tprintf("Self-destruction in %d second%s..", extra % 256,
                    extra > 1 ? "s" : ""));
        MECHEVENT(mech, EVENT_EXPLODE, mech_explode_event, 1, extra);

    } else {
        
        SendDebug(tprintf("#%d explodes.", mech->mynum));
        if (extra >= 256) {
            SendDebug(tprintf("#%d explodes [ammo]", mech->mynum));
            mech_notify(mech, MECHALL, "All your ammo explodes!");
            while ((damage = FindDestructiveAmmo(mech, &i, &j)))
                ammo_explosion(mech, mech, i, j, damage);
        } else {
            SendDebug(tprintf("#%d explodes [reactor]", mech->mynum));
            MechLOSBroadcast(mech, "suddenly explodes!");
            doing_explode = 1;
            mech_notify(mech, MECHALL,
                    "Suddenly you feel great heat overcoming your senses.. you faint.. (and die)");
            z = MechZ(mech);
            DestroySection(mech, mech, -1, LTORSO);
            DestroySection(mech, mech, -1, RTORSO);
            DestroySection(mech, mech, -1, CTORSO);
            DestroySection(mech, mech, -1, HEAD);
            MechZ(mech) += 6;
            doing_explode = 0;

            if (mudconf.btech_explode_reactor > 1)
                dam = MAX(MechTons(mech) / 5, MechEngineSize(mech) / 15);
            else
                dam = MAX(MechTons(mech) / 5, MechEngineSize(mech) / 10);

            /* If the guy is on a map have it hit the hexes around it */
            map = FindObjectsData(mech->mapindex);
            if (map) {
                blast_hit_hexesf(map, dam, 1, MAX(MechTons(mech) / 10,
                            MechEngineSize(mech) / 25), MechFX(mech), MechFY(mech),
                        MechFX(mech), MechFY(mech),
                        "%ch%crYou bear full brunt of the blast!%cn",
                        "is hit badly by the blast!",
                        "%ch%cyYou receive some damage from the blast!%cn",
                        "is hit by the blast!", mudconf.btech_explode_reactor > 1,
                        mudconf.btech_explode_reactor > 1 ? 5 : 3, 5, 1, 2);
            }

            MechZ(mech) = z;
            headhitmwdamage(mech, 4);
        }
    }
}

void mech_explode(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    char *args[3];
    int i;
    int ammoloc, ammocritnum;
    int time = mudconf.btech_explode_time;
    int ammo = 1;
    int argc;

    cch(MECH_USUALO);
    argc = mech_parseattributes(buffer, args, 2);
    DOCHECK(argc != 1, "Invalid number of arguments!");

    /* Can't do any of the explosion routine if we're recycling! */
    for (i = 0; i < NUM_SECTIONS; i++)
	{
	if (!SectIsDestroyed(mech, i))
            DOCHECK(SectHasBusyWeap(mech, i), "You have weapons recycling!");
            DOCHECK(MechSections(mech)[i].recycle, "You are still recovering from your last attack.");
	}
    
    if (!strcasecmp(buffer, "stop")) {
	DOCHECK(!mudconf.btech_explode_stop,
	    "It's too late to turn back now!");
	DOCHECK(!Exploding(mech),
	    "Your mech isn't undergoing a self-destruct sequence!");

	StopExploding(mech);
	mech_notify(mech, MECHALL, "Self-destruction sequence aborted.");
	SendDebug(tprintf
	    ("#%d in #%d stopped the self-destruction sequence.", player,
		mech->mynum));
	MechLOSBroadcast(mech, "regains control over itself.");
	return;
    }
    DOCHECK(Exploding(mech),
	"Your mech is already undergoing a self-destruct sequence!");
    if (!strcasecmp(buffer, "ammo")) {
	/*
	   Find SOME ammo to explode ; if possible, we engage the 'boom' process
	 */
	DOCHECK(!mudconf.btech_explode_ammo,
	    "You can't bring yourself to do it!");
	i = FindDestructiveAmmo(mech, &ammoloc, &ammocritnum);
	DOCHECK(!i, "There is no 'damaging' ammo on your 'mech!");
	/* Engage the boom-event */
	SendDebug(tprintf
	    ("#%d in #%d initiates the ammo explosion sequence.", player,
		mech->mynum));
	MechLOSBroadcast(mech, "starts billowing smoke!");
	time = time / 2;
	MECHEVENT(mech, EVENT_EXPLODE, mech_explode_event, 1, time);
    } else {
	DOCHECK(!mudconf.btech_explode_reactor,
	    "You can't bring yourself to do it!");
	DOCHECK(MechType(mech) != CLASS_MECH,
	    "Only mechs can do the 'big boom' effect.");
	DOCHECK(MechSpecials(mech) & ICE_TECH, "You need a fusion reactor.");
	SendDebug(tprintf
	    ("#%d in #%d initiates the reactor explosion sequence.",
		player, mech->mynum));
	MechLOSBroadcast(mech, "loses reactions containment!");
	MECHEVENT(mech, EVENT_EXPLODE, mech_explode_event, 1, time);
	ammo = 0;
    }
    mech_notify(mech, MECHALL,
	"Self-destruction sequence engaged ; please stand by.");
    mech_notify(mech, MECHALL, tprintf("%s in %d seconds.",
	    ammo ? "The ammunition will explode" :
	    "The reactor will blow up", time));
    /* Null out the pilot to disallow further commands */
    MechPilot(mech) = -1;
}

static void mech_dig_event(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;

    if (!Digging(mech))
	return;

    if (!Started(mech))
	return;

    MechTankCritStatus(mech) &= ~DIGGING_IN;
    MechTankCritStatus(mech) |= DUG_IN;
    mech_notify(mech, MECHALL,
	"You finish burrowing for cover - only turret weapons are available now.");
}

void mech_dig(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALO);
    DOCHECK(fabs(MechSpeed(mech)) > 0.0, "You are moving!");
    DOCHECK(MechFacing(mech) != MechDesiredFacing(mech),
	"You are turning!");
    DOCHECK(MechMove(mech) == MOVE_NONE, "You are stationary!");
    DOCHECK(MechDugIn(mech), "You are already dug in!");
    DOCHECK(Digging(mech), "You are already digging in!");
    DOCHECK(OODing(mech), "While dropping? I think not.");
    DOCHECK(MechRTerrain(mech) == ROAD ||
    	    MechRTerrain(mech) == BRIDGE ||
    	    MechRTerrain(mech) == BUILDING ||
    	    MechRTerrain(mech) == WALL,
    	"The surface is slightly too hard for you to dig in.");
    DOCHECK(MechRTerrain(mech) == WATER, "In water? Who are you kidding?");

    MechTankCritStatus(mech) |= DIGGING_IN;
    MECHEVENT(mech, EVENT_DIG, mech_dig_event, 20, 0);
    mech_notify(mech, MECHALL,
	"You start digging yourself in a nice hole..");
}

static void mech_unjam_turret_event(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;

    if (Destroyed(mech))
	return;

    if (Uncon(mech))
	return;

    if (!GetSectInt(mech, TURRET))
	return;

    if (!Started(mech))
	return;

    if (MechTankCritStatus(mech) & TURRET_LOCKED) {
	mech_notify(mech, MECHALL, "You are unable to unjam the turret!");
	return;
    }

    MechTankCritStatus(mech) &= ~TURRET_JAMMED;
    mech_notify(mech, MECHALL, "You manage to unjam your turret!");
}

void mech_fixturret(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    cch(MECH_USUALO);
    DOCHECK(MechTankCritStatus(mech) & TURRET_LOCKED,
	"Your turret is locked! You need a repairbay to fix it!");
    DOCHECK(!(MechTankCritStatus(mech) & TURRET_JAMMED),
	"Your turret is not jammed!");
    MECHEVENT(mech, EVENT_UNJAM_TURRET, mech_unjam_turret_event, 60, 0);
    mech_notify(mech, MECHALL, "You start to repair your jammed turret.");
}

static int mech_disableweap_func(MECH * mech, dbref player, int index,
    int high)
{
    int section, critical, weaptype;

    weaptype =
	FindWeaponNumberOnMech_Advanced(mech, index, &section, &critical,
	1);
    DOCHECK0(weaptype == -1,
	"The weapons system chirps: 'Illegal Weapon Number!'");
    DOCHECK0(weaptype == -2,
	"The weapons system chirps: 'That Weapon has been destroyed!'");
    weaptype = Weapon2I(GetPartType(mech, section, critical));
    DOCHECK0(!(MechWeapons[weaptype].special & GAUSS),
	"You can only disable Gauss weapons.");

    SetPartTempNuke(mech, section, critical, FAIL_DESTROYED);
    mech_notify(mech, MECHALL, tprintf("You power down weapon %d.",
	    index));
    return 0;
}

void mech_disableweap(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    char *args[1];

    cch(MECH_USUALO);
    DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
	"Please specify a weapon number.");

    multi_weap_sel(mech, player, args[0], 1, mech_disableweap_func);
}

int FindMainWeapon(MECH * mech, int (*callback) (MECH *, int, int, int,
	int))
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
    int maxcount = 0;

    for (loop = 0; loop < NUM_SECTIONS; loop++) {
	if (SectIsDestroyed(mech, loop))
	    continue;
	count = FindWeapons(mech, loop, weaparray, weapdata, critical);
	if (count > 0) {
	    for (ii = 0; ii < count; ii++) {
		if (!PartIsBroken(mech, loop, critical[ii])) {
		    /* tempcrit = GetWeaponCrits(mech, weaparray[ii]); */
		    tempcrit = rand();
		    if (tempcrit > maxcrit) {
			critfound = 1;
			maxcrit = tempcrit;
			maxloc = loop;
			maxcount = ii;
		    }
		}
	    }
	}
    }
    if (critfound)
	return callback(mech, maxloc, weaparray[maxcount], maxcount,
	    maxcrit);
    else
	return 0;
}

void changeStealthArmorEvent(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;
    int wType = (int) e->data2;

    if (!Started(mech))
	return;

    if (!HasWorkingECMSuite(mech))
	return;

    if (wType) {
	mech_notify(mech, MECHALL, "Stealth Armor system engaged!");

	EnableStealthArmor(mech);
	checkECM(mech);
	MarkForLOSUpdate(mech);
    } else {
	mech_notify(mech, MECHALL, "Stealth Armor system disengaged!");

	DisableStealthArmor(mech);
	checkECM(mech);
	MarkForLOSUpdate(mech);
    }
}

void mech_stealtharmor(dbref player, MECH * mech, char *buffer)
{
    cch(MECH_USUALO);

    if (!(MechSpecials2(mech) & STEALTH_ARMOR_TECH)) {
	mech_notify(mech, MECHALL,
	    "Your 'mech isn't equipped with a Stealth Armor system!");

	return;
    }

    if (!HasWorkingECMSuite(mech)) {
	mech_notify(mech, MECHALL,
	    "Your 'mech doesn't have a working Guardian ECM suite!");

	return;
    }

    if (StealthArmorChanging(mech)) {
	mech_notify(mech, MECHALL,
	    "You are already changing the status of your Stealth Armor system!");

	return;
    }

    if (!StealthArmorActive(mech)) {
	mech_notify(mech, MECHALL,
	    "Your Stealth Armor system begins to come online.");

	MECHEVENT(mech, EVENT_STEALTH_ARMOR, changeStealthArmorEvent, 30,
	    1);
    } else {
	mech_notify(mech, MECHALL,
	    "Your Stealth Armor system begins to shutdown.");

	MECHEVENT(mech, EVENT_STEALTH_ARMOR, changeStealthArmorEvent, 30,
	    0);
    }
}

void changeNullSigSysEvent(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;
    int wType = (int) e->data2;

    if (!Started(mech))
	return;

    if (NullSigSysDest(mech))
	return;

    if (wType) {
	mech_notify(mech, MECHALL, "Null Signature System engaged!");

	EnableNullSigSys(mech);
	MarkForLOSUpdate(mech);
    } else {
	mech_notify(mech, MECHALL, "Null Signature System disengaged!");

	DisableNullSigSys(mech);
	MarkForLOSUpdate(mech);
    }
}

void mech_nullsig(dbref player, MECH * mech, char *buffer)
{
    cch(MECH_USUALO);

    if (!(MechSpecials2(mech) & NULLSIGSYS_TECH)) {
	mech_notify(mech, MECHALL,
	    "Your 'mech isn't equipped with a Null Signature System!");

	return;
    }

    if (NullSigSysDest(mech)) {
	mech_notify(mech, MECHALL,
	    "Your Null Signature System is destroyed!");

	return;
    }

    if (NullSigSysChanging(mech)) {
	mech_notify(mech, MECHALL,
	    "You are already changing the status of your Null Signature System!");

	return;
    }

    if (!NullSigSysActive(mech)) {
	mech_notify(mech, MECHALL,
	    "Your Null Signature System begins to come online.");

	MECHEVENT(mech, EVENT_NSS, changeNullSigSysEvent, 30, 1);
    } else {
	mech_notify(mech, MECHALL,
	    "Your Null Signature System begins to shutdown.");

	MECHEVENT(mech, EVENT_NSS, changeNullSigSysEvent, 30, 0);
    }
}

void show_narc_pods(dbref player, MECH * mech, char *buffer)
{
    char location[50];
    int i;

    cch(MECH_USUALO);

    if (!(checkAllSections(mech, NARC_ATTACHED) ||
	    checkAllSections(mech, INARC_HOMING_ATTACHED) ||
	    checkAllSections(mech, INARC_HAYWIRE_ATTACHED) ||
	    checkAllSections(mech, INARC_ECM_ATTACHED) ||
	    checkAllSections(mech, INARC_NEMESIS_ATTACHED))) {

	notify(player,
	    "There are no NARC or iNARC pods attached to this unit.");

	return;
    }

    notify(player,
	"=========================Attached NARC and iNARC Pods========================");
    notify(player,
	"-- Location ---||- NARC -||- iHoming -||- iHaywire -||- iECM -||- iNemesis --");

    for (i = 0; i < NUM_SECTIONS; i++) {
	if (GetSectOInt(mech, i) > 0) {
	    ArmorStringFromIndex(i, location, MechType(mech),
		MechMove(mech));

	    if (SectIsDestroyed(mech, i)) {
		notify(player,
		    tprintf
		    (" %-14.13s||********||***********||************||********||************* ",
			location));
	    } else {
		notify(player,
		    tprintf
		    (" %-14.13s||....%s...||.....%s.....||......%s.....||....%s...||......%s...... ",
			location, checkSectionForSpecial(mech,
			    NARC_ATTACHED, i) ? "X" : ".",
			checkSectionForSpecial(mech, INARC_HOMING_ATTACHED,
			    i) ? "X" : ".", checkSectionForSpecial(mech,
			    INARC_HAYWIRE_ATTACHED, i) ? "X" : ".",
			checkSectionForSpecial(mech, INARC_ECM_ATTACHED,
			    i) ? "X" : ".", checkSectionForSpecial(mech,
			    INARC_NEMESIS_ATTACHED, i) ? "X" : "."));
	    }
	}
    }
}

int findArmBTHMod(MECH * mech, int wSec)
{
    int wRet = 0;

    if (PartIsNonfunctional(mech, wSec, 1) ||
	GetPartType(mech, wSec, 1) != I2Special(UPPER_ACTUATOR))
	wRet += 2;
    if (PartIsNonfunctional(mech, wSec, 2) ||
	GetPartType(mech, wSec, 2) != I2Special(LOWER_ACTUATOR))
	wRet += 2;
    if (PartIsNonfunctional(mech, wSec, 3) ||
	GetPartType(mech, wSec, 3) != I2Special(HAND_OR_FOOT_ACTUATOR))
	wRet += 1;

    return wRet;
}

void remove_inarc_pods_mech(dbref player, MECH * mech, char *buffer)
{
    int wLoc;
    int wArmToUse = -1;
    char *args[2];
    char strLocation[50], strPunchWith[50];
    int wBTH = 0;
    int wBTHModLARM = 0;
    int wBTHModRARM = 0;
    int wRAAvail = 1;
    int wLAAvail = 1;
    int wRoll;
    int wSelfDamage;
    int wPodType = INARC_HOMING_ATTACHED;
    char strPodType[30];

    cch(MECH_USUALO);

    DOCHECK(MechIsQuad(mech), "Quads can not knock of iNARC pods!");
    DOCHECK(mech_parseattributes(buffer, args, 2) != 2,
	"Invalid number of arguments!");

    wLoc = ArmorSectionFromString(MechType(mech), MechMove(mech), args[0]);

    DOCHECK(wLoc == -1, "Invalid section!");
    DOCHECK(!GetSectOInt(mech, wLoc), "Invalid section!");
    DOCHECK(!GetSectInt(mech, wLoc), "That section is destroyed!");

    ArmorStringFromIndex(wLoc, strLocation, MechType(mech),
	MechMove(mech));

    /* Figure out wot type of pods we want to remove */
    switch (toupper(args[1][0])) {
    case 'Y':
	strcpy(strPodType, "Haywire");
	wPodType = INARC_HAYWIRE_ATTACHED;
	break;

    case 'E':
	strcpy(strPodType, "ECM");
	wPodType = INARC_ECM_ATTACHED;
	break;

    default:
	strcpy(strPodType, "Homing");
	wPodType = INARC_HOMING_ATTACHED;
	break;
    }

    DOCHECK(!checkSectionForSpecial(mech, wPodType, wLoc),
	tprintf("There are no iNarc %s pods attached to your %s!",
	    strPodType, strLocation));

    DOCHECK(((!GetSectInt(mech, RARM)) &&
	    (!GetSectInt(mech, LARM))),
	"You need atleast one functioning arm to remove iNarc pods!");

    if (wLoc == RARM) {
	DOCHECK(!GetSectInt(mech, LARM),
	    "Your Left Arm needs to be intact to take iNarc pods off your right arm!");
	DOCHECK(SectHasBusyWeap(mech, LARM),
	    "You have weapons recycling on your Left Arm.");
	DOCHECK(MechSections(mech)[LARM].recycle,
	    "Your Left Arm is still recovering from your last attack.");

	wArmToUse = LARM;
    }

    if (wLoc == LARM) {
	DOCHECK(!GetSectInt(mech, RARM),
	    "Your Right Arm needs to be intact to take iNarc pods off your Left Arm!");
	DOCHECK(SectHasBusyWeap(mech, RARM),
	    "You have weapons recycling on your Right Arm.");
	DOCHECK(MechSections(mech)[RARM].recycle,
	    "Your Right Arm is still recovering from your last attack.");

	wArmToUse = RARM;
    }

    if (wArmToUse == -1) {
	if (SectHasBusyWeap(mech, RARM) || MechSections(mech)[RARM].recycle
	    || (!GetSectInt(mech, RARM)))
	    wRAAvail = 0;

	if (SectHasBusyWeap(mech, LARM) || MechSections(mech)[LARM].recycle
	    || (!GetSectInt(mech, LARM)))
	    wLAAvail = 0;

	DOCHECK(!(wLAAvail ||
		wRAAvail),
	    "You need atleast one arm that is not recycling and does not have weapons recycling in it!");

	if (!wLAAvail)
	    wBTHModLARM = 1000;
	else
	    wBTHModLARM = findArmBTHMod(mech, LARM);

	if (!wRAAvail)
	    wBTHModRARM = 1000;
	else
	    wBTHModRARM = findArmBTHMod(mech, RARM);

	if (wBTHModRARM < wBTHModLARM) {
	    wBTH = wBTHModRARM;
	    wArmToUse = RARM;
	} else {
	    wBTH = wBTHModLARM;
	    wArmToUse = LARM;
	}
    } else {
	wBTH = findArmBTHMod(mech, wArmToUse);
    }

    wBTH += FindPilotPiloting(mech) + 4;
    wRoll = Roll();

    ArmorStringFromIndex(wArmToUse, strPunchWith, MechType(mech),
	MechMove(mech));

    mech_notify(mech, MECHALL,
	tprintf
	("You try to swat at the iNarc pods attached to your %s with your %s.  BTH:  %d,\tRoll:  %d",
	    strLocation, strPunchWith, wBTH, wRoll));

    /* Oops, we failed! */
    if (wRoll < wBTH) {
	mech_notify(mech, MECHALL,
	    "Uh oh. You miss the pod and hit yourself!");
	MechLOSBroadcast(mech,
	    "tries to swat off an iNarc pod, but misses and hits itself!");

	wSelfDamage = (MechTons(mech) + 10 / 2) / 10;

	if (!OkayCritSectS(wArmToUse, 2, LOWER_ACTUATOR))
	    wSelfDamage = wSelfDamage / 2;

	if (!OkayCritSectS(wArmToUse, 1, UPPER_ACTUATOR))
	    wSelfDamage = wSelfDamage / 2;

	DamageMech(mech, mech, 1, MechPilot(mech), wLoc, 0, 0, wSelfDamage,
	    0, -1, 0, -1, 0, 0);
    } else {
	MechSections(mech)[wLoc].specials &= ~wPodType;

	mech_notify(mech, MECHALL,
	    tprintf("You knock a %s pod off your %s!", strPodType,
		strLocation));
	MechLOSBroadcast(mech, "knocks an iNarc pod off itself.");
    }

    SetRecycleLimb(mech, wArmToUse, PHYSICAL_RECYCLE_TIME);
}

void removeiNarcPodsTank(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;
    int i;

    if (Destroyed(mech))
	return;

    mech_notify(mech, MECHALL,
	"You remove all the iNARC pods from your unit.");

    MechLOSBroadcast(mech,
	"'s crew climbs out and knocks off all the attached iNarc pods!");

    for (i = 0; i < NUM_SECTIONS; i++) {
	if (GetSectOInt(mech, i) > 0) {
	    MechSections(mech)[i].specials &=
		~(INARC_HOMING_ATTACHED | INARC_HAYWIRE_ATTACHED |
		INARC_ECM_ATTACHED | INARC_NEMESIS_ATTACHED);
	}
    }

}

void remove_inarc_pods_tank(dbref player, MECH * mech, char *buffer)
{
    cch(MECH_USUALSO);

    DOCHECK((MechDesiredSpeed(mech) > 0),
	"You can not be moving when attempting to remove iNarc pods!");
    DOCHECK((MechSpeed(mech) > 0),
	"You can not be moving when attempting to remove iNarc pods!");

    if (MechType(mech) == CLASS_VTOL)
	DOCHECK(!Landed(mech),
	    "You must land before attempting to remove iNarc pods!");

    DOCHECK(CrewStunned(mech), "You're too stunned to remove iNarc pods!");
    DOCHECK(UnjammingTurret(mech),
	"You're too busy unjamming your turret to remove iNarc pods!");
    DOCHECK(UnJammingAmmo(mech),
	"You're too busy unjamming a weapon to remove iNarc pods!");

    if (!(checkAllSections(mech, INARC_HOMING_ATTACHED) ||
	    checkAllSections(mech, INARC_HAYWIRE_ATTACHED) ||
	    checkAllSections(mech, INARC_ECM_ATTACHED) ||
	    checkAllSections(mech, INARC_NEMESIS_ATTACHED))) {

	mech_notify(mech, MECHALL,
	    "There are no iNarc pods attached to this unit.");

	return;
    }

    mech_notify(mech, MECHALL,
	"You begin to systematically remove all the iNarc pods from your unit.");

    MECHEVENT(mech, EVENT_REMOVE_PODS, removeiNarcPodsTank, 60, 0);
}

void mech_auto_turret(dbref player, MECH * mech, char *buffer)
{
    cch(MECH_USUALSO);

    DOCHECK(!GetSectInt(mech, TURRET), "You have no turret to autoturn!");

    mech_notify(mech, MECHALL,
	tprintf("Automatic turret turning is now %s",
	    (MechStatus2(mech) & AUTOTURN_TURRET) ? "OFF" : "ON"));

    if (MechStatus2(mech) & AUTOTURN_TURRET)
	MechStatus2(mech) &= ~AUTOTURN_TURRET;
    else
	MechStatus2(mech) |= AUTOTURN_TURRET;
}

void mech_usebin(dbref player, MECH * mech, char *buffer)
{
    char strLocation[80];
    int wLoc, wCurLoc;
    int wSection, wCritSlot, wWeapNum, wWeapType;
    char *args[2];

    cch(MECH_USUALSO);

    DOCHECK(mech_parseattributes(buffer, args, 2) != 2,
	"Invalid number of arguments!");

    DOCHECK(Readnum(wWeapNum, args[0]), tprintf("Invalid value: %s",
	    args[0]));
    wWeapType =
	FindWeaponNumberOnMech(mech, wWeapNum, &wSection, &wCritSlot);

    DOCHECK(wWeapType == -1,
	"The weapons system chirps: 'Illegal Weapon Number!'");
    DOCHECK(wWeapType == -2,
	"The weapons system chirps: 'That Weapon has been destroyed!'");
    DOCHECK(wWeapType == -3,
	"The weapon system chirps: 'That weapon is still reloading!'");
    DOCHECK(wWeapType == -4,
	"The weapon system chirps: 'That weapon is still recharging!'");
    DOCHECK(IsEnergy(wWeapType), "Energy weapons do not use ammo!");

    if (args[1][0] == '-') {
	mech_notify(mech, MECHALL,
	    tprintf("Prefered ammo source reset for weapon #%d",
		wWeapNum));
	SetPartDesiredAmmoLoc(mech, wSection, wCritSlot, -1);
	return;
    }

    wLoc = ArmorSectionFromString(MechType(mech), MechMove(mech), args[1]);

    DOCHECK(wLoc == -1, "Invalid section!");
    DOCHECK(!GetSectOInt(mech, wLoc), "Invalid section!");
    DOCHECK(!GetSectInt(mech, wLoc), "That section is destroyed!");

    ArmorStringFromIndex(wLoc, strLocation, MechType(mech),
	MechMove(mech));
    wCurLoc = GetPartDesiredAmmoLoc(mech, wSection, wCritSlot);

    DOCHECK(wCurLoc == wLoc,
	tprintf("Prefered ammo source already set to %s for weapon #%d",
	    strLocation, wWeapNum));

    mech_notify(mech, MECHALL,
	tprintf("Prefered ammo source set to %s for weapon #%d",
	    strLocation, wWeapNum));
    SetPartDesiredAmmoLoc(mech, wSection, wCritSlot, wLoc);
}

void mech_safety(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    DOCHECK(MechType(mech) == CLASS_MW,
	"Your weapons dont have safeties.");
    if (buffer && !strcasecmp(buffer, "on")) {
	UnSetMechPKiller(mech);
	mech_notify(mech, MECHALL, "Safeties flipped %ch%cgON%cn.");
	return;
    }
    if (buffer && !strcasecmp(buffer, "off")) {
	SetMechPKiller(mech);
	mech_notify(mech, MECHALL, "Safeties flipped %ch%crOFF%cn.");
	return;
    }

    mech_notify(mech, MECHPILOT, tprintf("Weapon safeties are %%ch%s%%cn",
	    MechPKiller(mech) ? "%crOFF" : "%cgON"));
    return;
}

#define MECHPREF_FLAG_INVERTED  0x01
#define MECHPREF_FLAG_NEGATIVE  0x02

static struct mechpref_info {
    unsigned char bit;
    unsigned char flags;
    char * name;
    char * msg;
} mech_preferences[] = {
    { MECHPREF_PKILL, MECHPREF_FLAG_INVERTED, "MWSafety",
      "MechWarrior Safeties flipped" },
    { MECHPREF_SLWARN, 0, "SLWarn",
      "The warning when lit by searchlight is now" },
    { MECHPREF_AUTOFALL, MECHPREF_FLAG_NEGATIVE, "AutoFall",
      "Suicidal jumps off cliffs toggled" },
    { MECHPREF_NOARMORWARN, MECHPREF_FLAG_INVERTED, "ArmorWarn",
      "Low-armor warnings turned" },
    { MECHPREF_NOAMMOWARN, MECHPREF_FLAG_INVERTED, "AmmoWarn",
      "Warning when running out of Ammunition switched" },
    { MECHPREF_AUTOCON_SD, MECHPREF_FLAG_NEGATIVE, "AutoconShutdown",
      "Autocon on shutdown units turned" },
    { MECHPREF_NOFRIENDLYFIRE, 0, "FFSafety",
      "Friendly Fire Safeties flipped" },
};
#define NUM_MECHPREFERENCES (sizeof(mech_preferences) / sizeof(struct mechpref_info))

static MECH *target_mech;

static char *display_mechpref(int i)
{
    static char buf[256];
    struct mechpref_info info = mech_preferences[i];
    char * state;

    if (!target_mech) {
        SendError("Invalid target_mech in display_mechpref!");
        return "Unknown error; contact a Wizard.";
    }

    if (((MechPrefs(target_mech) & info.bit) &&
            (info.flags & MECHPREF_FLAG_INVERTED)) ||
            (!(MechPrefs(target_mech) & info.bit) &&
             !(info.flags & MECHPREF_FLAG_INVERTED))) {
        if (info.flags & MECHPREF_FLAG_NEGATIVE)
            state = "%ch%cgOFF%cn";
        else
            state = "%ch%crOFF%cn";
    } else {
        if (info.flags & MECHPREF_FLAG_NEGATIVE)
            state = "%ch%crON%cn";
        else
            state = "%ch%cgON%cn";
    }

    sprintf(buf, "        %-40s%s", info.name, state);
    return buf;
}

void mech_mechprefs(dbref player, void * data, char * buffer)
{
    MECH * mech = (MECH *) data;
    int nargs;
    char *args[3];
    char buf[LBUF_SIZE]; 
    coolmenu *c;

    cch(MECH_USUALSMO);
    nargs = mech_parseattributes(buffer, args, 2);

    /* Default, no arguments passed */
    if (!nargs) {

        /* Show mechprefs */
        target_mech = mech;
        c = SelCol_FunStringMenuK(1, "Mech Preferences", display_mechpref,
                NUM_MECHPREFERENCES);
        ShowCoolMenu(player, c);
        KillCoolMenu(c);
        target_mech = NULL;

    } else {

        int i;
        struct mechpref_info info;
        char *newstate;

        /* Looking through the different mech preferences to find the
         * one the user wants to change */
        for (i = 0; i < NUM_MECHPREFERENCES; i++) {
            if (strcasecmp(args[0], mech_preferences[i].name) == 0)
                break;
        }
        if (i == NUM_MECHPREFERENCES) {
            snprintf(buf, LBUF_SIZE, "Unknown MechPreference: %s", args[0]);
            notify(player, buf);
            return;
        }

        /* Get the current setting */
        info = mech_preferences[i];

        /* Did they provide a ON or OFF flag */
        if (nargs == 2) {

            /* Check to make sure its either ON or OFF */
            if ((strcasecmp(args[1], "ON") != 0) &&
                    (strcasecmp(args[1], "OFF") != 0)) {

                /* Insert notify here */
                notify(player, "Only accept ON or OFF as valid extra "
                        "parameter for mechprefs pref");
                return;
            }

            /* Set the value to what they want */
            if (strcasecmp(args[1], "ON") == 0) {

                /* Set the bit */
                if (info.flags & MECHPREF_FLAG_INVERTED) {
                    MechPrefs(mech) &= ~(info.bit);
                } else {
                    MechPrefs(mech) |= (info.bit);
                }

            } else {

                /* Unset the bit */
                if (info.flags & MECHPREF_FLAG_INVERTED) {
                    MechPrefs(mech) |= (info.bit);
                } else {
                    MechPrefs(mech) &= ~(info.bit);
                }

            }

        } else {

            /* If set, unset it, otherwise set the preference */
            if (MechPrefs(mech) & info.bit)
                MechPrefs(mech) &= ~(info.bit);
            else
                MechPrefs(mech) |= (info.bit);

        }

        /* Which way did the preference get changed and
         * is it the default or non-standard mode of
         * the preference */
        if (((MechPrefs(mech) & info.bit) &&
                (info.flags & MECHPREF_FLAG_INVERTED)) ||
                (!(MechPrefs(mech) & info.bit) &&
                 !(info.flags & MECHPREF_FLAG_INVERTED))) {

            if (info.flags & MECHPREF_FLAG_NEGATIVE)
                newstate = "%ch%cgOFF%cn";
            else
                newstate = "%ch%crOFF%cn";

        } else {

            if (info.flags & MECHPREF_FLAG_NEGATIVE)
                newstate = "%ch%crON%cn";
            else
                newstate = "%ch%cgON%cn";

        }

        /* Tell them the preference has been changed */
        snprintf(buf, LBUF_SIZE, "%s %s", info.msg, newstate);
        notify(player, buf);
    }
}    
