
/*
 * $Id: mech.startup.c,v 1.2 2005/06/23 18:31:42 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Last modified: Thu Jul  9 06:59:34 1998 fingon
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "mech.events.h"
#include "autopilot.h"
#include "p.btechstats.h"
#include "p.mech.utils.h"
#include "p.econ_cmds.h"
#include "p.mech.tech.h"
#include "p.mech.build.h"
#include "p.mech.update.h"
#include "p.mech.pickup.h"
#include "p.mech.tag.h"
#include "p.bsuit.h"
#include "p.bsuit.h"
#include "p.mech.combat.misc.h"

/* NOTE: Number of boot messages for both types _MUST_ match */

#define BOOTCOUNT 6

char *bsuit_bootmsgs[BOOTCOUNT] = {
    "%%cg->         Initializing powerpack       <-%%c",
    "%%cg->          Powerpack operational       <-%%c",
    "%%cg->             Suit sealed              <-%%c",
    "%%cg->  Computer system is now operational  <-%%c",
    "%%cg->         Air pressure steady          <-%%c",
    "       %%cg- %%cr-=>%%ch%%cw All systems go!%%c %%cr<= %%cg-%%c"
};

char *aero_bootmsgs[BOOTCOUNT] = {
    "%%cg->       Main reactor is now online    <-%%c",
    "%%cg->            Thrusters online         <-%%c",
    "%%cg->  Main computer system is now online <-%%c",
    "%%cg->     Scanners are now operational    <-%%c",
    "%%cg-> Targeting system is now operational <-%%c",
    "       %%cg- %%cr-=>%%ch%%cw All systems go!%%c %%cr<= %%cg-%%c"
};

char *bootmsgs[BOOTCOUNT] = {
    "%%cg->       Main reactor is now online    <-%%c",
    "%%cg->         Gyros are now stable        <-%%c",
    "%%cg->  Main computer system is now online <-%%c",
    "%%cg->     Scanners are now operational    <-%%c",
    "%%cg-> Targeting system is now operational <-%%c",
    "   %%cg- %%cr-=>%%ch%%cw All systems operational!%%c %%cr<=- %%cg-%%c"
};

char *hover_bootmsgs[BOOTCOUNT] = {
    "%%cg->  Powerplant initialized and online  <-%%c",
    "%%cg->   Checking plenum chamber status    <-%%c",
    "%%cg->         Verifying fan status        <-%%c",
    "%%cg->     Scanners are now operational    <-%%c",
    "%%cg-> Targeting system is now operational <-%%c",
    "   %%cg- %%cr-=>%%ch%%cw All systems operational!%%c %%cr<=- %%cg-%%c"
};

char *track_bootmsgs[BOOTCOUNT] = {
    "%%cg->  Powerplant initialized and online  <-%%c",
    "%%cg->      Auto-aligning drive wheels     <-%%c",
    "%%cg->       Adjusting track tension       <-%%c",
    "%%cg->     Scanners are now operational    <-%%c",
    "%%cg-> Targeting system is now operational <-%%c",
    "   %%cg- %%cr-=>%%ch%%cw All systems operational!%%c %%cr<=- %%cg-%%c"
};

char *wheel_bootmsgs[BOOTCOUNT] = {
    "%%cg->  Powerplant initialized and online  <-%%c",
    "%%cg->  Performing steering system checks  <-%%c",
    "%%cg->        Checking wheel status        <-%%c",
    "%%cg->     Scanners are now operational    <-%%c",
    "%%cg-> Targeting system is now operational <-%%c",
    "   %%cg- %%cr-=>%%ch%%cw All systems operational!%%c %%cr<=- %%cg-%%c"
};

char *vtol_bootmsgs[BOOTCOUNT] = {
    "%%cg->     Initializing main powerplant    <-%%c",
    "%%cg-> Main turbine online and operational <-%%c",
    "%%cg->      Rotor transmission engaged     <-%%c",
    "%%cg->     Scanners are now operational    <-%%c",
    "%%cg-> Targeting system is now operational <-%%c",
    "   %%cg- %%cr-=>%%ch%%cw All systems operational!%%c %%cr<=- %%cg-%%c"
};

char *naval_bootmsgs[BOOTCOUNT] = {
    "%%cg->       Main reactor is now online    <-%%c",
    "%%cg->  Main computer system is now online <-%%c",
    "%%cg->   Hull integrity monitoring online  <-%%c",
    "%%cg-> Ballast and propulsion are nominal  <-%%c",
    "%%cg-> Targeting system is now operational <-%%c",
    "   %%cg- %%cr-=>%%ch%%cw All systems operational!%%c %%cr<=- %%cg-%%c"
};

#define SSLEN MechType(mech) == CLASS_BSUIT ? 1 : (STARTUP_TIME / BOOTCOUNT)

static void mech_startup_event(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;
    int timer = (int) e->data2;
    MAP *mech_map;
    int i;

    if(is_aero(mech)) {
        mech_printf(mech, MECHALL, aero_bootmsgs[timer]);
    } else if(MechType(mech) == CLASS_BSUIT) {
        mech_printf(mech, MECHALL, bsuit_bootmsgs[timer]);
    } else switch(MechMove(mech)) {
        case MOVE_HOVER:
            mech_printf(mech, MECHALL, hover_bootmsgs[timer]);
            break;
        case MOVE_TRACK:
            mech_printf(mech, MECHALL, track_bootmsgs[timer]);
            break;
        case MOVE_WHEEL:
            mech_printf(mech, MECHALL, wheel_bootmsgs[timer]);
            break;
        case MOVE_VTOL:
            mech_printf(mech, MECHALL, vtol_bootmsgs[timer]);
            break;
        case MOVE_BIPED:
            mech_printf(mech, MECHALL, bootmsgs[timer]);
            break;
        case MOVE_HULL:
        case MOVE_FOIL:
        case MOVE_SUB:
            mech_printf(mech, MECHALL, naval_bootmsgs[timer]);
            break;
        default:
            mech_printf(mech, MECHALL, bootmsgs[timer]);
            break;
    }
    timer++;

    /* Check if the unit is in water and if it should die */
    /* Make sure it checks pretty early in the startup */
    if (timer>=2) {

        if (InWater(mech) && (MechType(mech) == CLASS_VEH_GROUND || 
                    MechType(mech) == CLASS_VTOL || 
                    MechType(mech) == CLASS_BSUIT ||
                    MechType(mech) == CLASS_AERO ||
                    MechType(mech) == CLASS_DS) &&
                !(MechSpecials2(mech) & WATERPROOF_TECH)) {

            mech_notify(mech, MECHALL, "Water floods your engine and your unit "
                    "becomes inoperable.");
            MechLOSBroadcast(mech, "emits some bubbles as its engines are flooded.");
            DestroyMech(mech, mech, 0);
            return;

        }
    }

    if (timer < BOOTCOUNT) {
        MECHEVENT(mech, EVENT_STARTUP, mech_startup_event, SSLEN, timer);
        return;
    }
    if ((mech_map = getMap(mech->mapindex)))
        for (i = 0; i < mech_map->first_free; i++)
            mech_map->LOSinfo[mech->mapnumber][i] = 0;
    initialize_pc(MechPilot(mech), mech);
    Startup(mech);
    MarkForLOSUpdate(mech);
    SetCargoWeight(mech);
    UnSetMechPKiller(mech);
    MechLOSBroadcast(mech, "powers up!");
    MechVerticalSpeed(mech) = 0;
    EvalBit(MechSpecials(mech), SS_ABILITY, ((MechPilot(mech) > 0 &&
                    isPlayer(MechPilot(mech))) ? char_getvalue(MechPilot(mech),
                    "Sixth_Sense") : 0));
    if (FlyingT(mech)) {
        if (MechZ(mech) <= MechElevation(mech))
            MechStatus(mech) |= LANDED;
    }
    MechComm(mech) = DEFAULT_COMM;
    if (isPlayer(MechPilot(mech)) && !Quiet(mech->mynum)) {
        MechComm(mech) =
            char_getskilltarget(MechPilot(mech), "Comm-Conventional", 0);
        MechPer(mech) =
            char_getskilltarget(MechPilot(mech), "Perception", 0);
    } else {
        MechComm(mech) = 6;
        MechPer(mech) = 6;
    }
    MechCommLast(mech) = 0;
    MechLastStartup(mech) = mudstate.now;
    if (is_aero(mech) && !Landed(mech)) {
        MechDesiredAngle(mech) = -90;
        MechStartFX(mech) = 0.0;
        MechStartFY(mech) = 0.0;
        MechStartFZ(mech) = 0.0;
        MechDesiredSpeed(mech) = MechMaxSpeed(mech);
        MaybeMove(mech);
    }
    UnZombifyMech(mech);
}

void mech_startup(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    int n;

    cch(MECH_CONSISTENT | MECH_MAP | MECH_PILOT_CON);
    skipws(buffer);
    DOCHECK(!(Good_obj(player) && (Alive(player) || isRobot(player) ||
		Hardcode(player))), "That is not a valid player!");
    DOCHECK(MechType(mech) == CLASS_MW &&
	Started(mech), "You're up and about already!");
    DOCHECK(Towed(mech),
	"You're being towed! Wait for drop-off before starting again!");
    DOCHECK(mech->mapindex < 0, "You are not on any map!");
    DOCHECK(Destroyed(mech), "This 'Mech is destroyed!");
    DOCHECK(Started(mech), "This 'Mech is already started!");
    DOCHECK(Starting(mech), "This 'Mech is already starting!");
    DOCHECK(Extinguishing(mech), "You're way too busy putting out fires!");
    n = figure_latest_tech_event(mech);
    DOCHECK(n,
	"This 'Mech is still under repairs (see checkstatus for more info)");
    DOCHECK(MechHeat(mech) > 30.,
	"This 'Mech is too hot to start back up!");
    DOCHECK(In_Character(mech->mynum) && !Wiz(player) &&
	(char_lookupplayer(GOD, GOD, 0, silly_atr_get(mech->mynum,
		    A_PILOTNUM)) != player), "This isn't your mech!");
    n = 0;
    if (*buffer && !strncasecmp(buffer, "override", strlen(buffer))) {
	DOCHECK(!WizP(player), "Insufficient access!");
	n = BOOTCOUNT - 1;
    }
    MechPilot(mech) = player;

/*   if (In_Character(mech->mynum)) */
    /* Initialize the PilotDamage from the new pilot */
    fix_pilotdamage(mech, player);
    mech_notify(mech, MECHALL, "Startup Cycle commencing...");
    MechSections(mech)[RLEG].recycle = 0;
    MechSections(mech)[LLEG].recycle = 0;
    MechSections(mech)[RARM].recycle = 0;
    MechSections(mech)[LARM].recycle = 0;
    MechSections(mech)[RTORSO].recycle = 0;
    MechSections(mech)[LTORSO].recycle = 0;
    MECHEVENT(mech, EVENT_STARTUP, mech_startup_event, (n ||
	    MechType(mech) == CLASS_MW) ? 1 : SSLEN,
	MechType(mech) == CLASS_MW ? BOOTCOUNT - 1 : n);
}

void mech_shutdown(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    if (!CheckData(player, mech))
        return;
    DOCHECK((!Started(mech) && !Starting(mech)), 
            "The 'mech hasn't been started yet!");
    DOCHECK(MechType(mech) == CLASS_MW,
            "You snore for a while.. and then _start_ yourself back up.");
    DOCHECK(IsDS(mech) && !Landed(mech) && !Wiz(player), 
            "No shutdowns in mid-air! Are you suicidal?");
    if (MechPilot(mech) == -1)
        return;
    if (Starting(mech)) {
        mech_notify(mech, MECHALL,
                "The startup sequence has been aborted.");
        StopStartup(mech);
        MechPilot(mech) = -1;
        return;
    }
    mech_printf(mech, MECHALL, "%s has been shutdown!",
                IsDS(mech) ? "Dropship" : is_aero(mech) ? "Fighter" :
                MechType(mech) == CLASS_BSUIT ? "Suit" : ((MechMove(mech) ==
                        MOVE_HOVER) || (MechMove(mech) == MOVE_TRACK) ||
                    (MechMove(mech) ==
                     MOVE_WHEEL)) ? "Vehicle" : MechMove(mech) ==
                MOVE_VTOL ? "VTOL" : "Mech");

    /*
     * Fixed by Kipsta so searchlights shutoff when the mech shuts down
     */

    if (MechStatus2(mech) & SLITE_ON) {
        mech_notify(mech, MECHALL, "Your searchlight shuts off.");
        MechStatus2(mech) &= ~SLITE_ON;
        MechCritStatus(mech) &= ~SLITE_LIT;
    }

    if (MechStatus(mech) & TORSO_RIGHT) {
        mech_notify(mech, MECHSTARTED,
                "Torso rotated back to center for shutdown");
        MechStatus(mech) &= ~TORSO_RIGHT;
    }
    if (MechStatus(mech) & TORSO_LEFT) {
        mech_notify(mech, MECHSTARTED,
                "Torso rotated back to center for shutdown");
        MechStatus(mech) &= ~TORSO_LEFT;
    }
    if ((MechType(mech) == CLASS_MECH && Jumping(mech)) ||
            (MechType(mech) != CLASS_MECH &&
             MechZ(mech) > MechUpperElevation(mech))) {
        mech_notify(mech, MECHALL,
                "You start free-fall.. Enjoy the ride!");
        MECHEVENT(mech, EVENT_FALL, mech_fall_event, FALL_TICK, -1);
    } else if (MechSpeed(mech) > MP1) {
        mech_notify(mech, MECHALL, "Your systems stop in mid-motion!");
        if (MechType(mech) == CLASS_MECH)
            MechLOSBroadcast(mech, "stops in mid-motion, and falls!");
        else {
            mech_notify(mech, MECHALL,
                    "You tumble end over end and come to a crashing halt!");
            MechLOSBroadcast(mech,
                    "tumbles end over end and comes to a crashing halt!");
        }
        MechFalls(mech, 1, 0);
        domino_space(mech, 2);
    }
    Shutdown(mech);
}
