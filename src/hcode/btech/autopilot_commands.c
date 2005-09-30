
/*
 * $Id: autopilot.c,v 1.2 2005/08/03 21:40:54 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Wed Oct 30 19:35:21 1996 fingon
 * Last modified: Sun Jun 14 14:13:13 1998 fingon
 *
 */

#include <math.h>
#include "mech.h"
#include "mech.events.h"
#include "autopilot.h"
#include "create.h"
#include "p.mech.startup.h"
#include "p.econ.h"
#include "p.econ_cmds.h"
#include "p.mech.maps.h"
#include "p.mech.utils.h"
#include "p.eject.h"
#include "p.btechstats.h"
#include "p.bsuit.h"
#include "p.mech.pickup.h"
#include "p.mech.los.h"
#include "p.ds.bay.h"
#include "p.glue.h"

/*
 * List of all the available autopilot commands
 * that can be given to the AI.  These use a
 * large enum that is located in autopilot.h
 */
ACOM acom[AUTO_NUM_COMMANDS + 1] = {
    {"chasetarget", 1, GOAL_CHASETARGET, NULL}
    ,                           /* Extension of follow, for chasetarget */
    {"dumbfollow", 1, GOAL_DUMBFOLLOW, NULL}
    ,                           /* [dumbly] follow the given target */
    {"dumbgoto", 2, GOAL_DUMBGOTO, NULL}
    ,                           /* [dumbly] goto a given hex */
    {"follow", 1, GOAL_FOLLOW, NULL}
    ,                           /* follow the given target */
    {"goto", 2, GOAL_GOTO, NULL}
    ,                           /* goto a given hex */
    {"leavebase", 1, GOAL_LEAVEBASE, NULL}
    ,                           /* leave a hangar */
    {"oldgoto", 2, GOAL_OLDGOTO, NULL}
    ,                           /* Old style goto - will phase out */
    {"roam", 2, GOAL_ROAM, NULL}
    ,                           /* roam around an area - like patroling */
    {"wait", 2, GOAL_WAIT, NULL}
    ,                           /* sit there and don't do anything for a while */
    {"attackleg", 1, COMMAND_ATTACKLEG, NULL}
    ,                           /* ? */
    {"autogun", 1, COMMAND_AUTOGUN, NULL}
    ,                           /* Let the AI decide what to shoot */
    {"chasemode", 1, COMMAND_CHASEMODE, NULL}
    ,                           /* chase after a target or not */
    {"cmode", 2, COMMAND_CMODE, NULL}
    ,                           /* ? */
    {"dropoff", 0, COMMAND_DROPOFF, NULL}
    ,                           /* dropoff whatever the AI is towing */
    {"embark", 1, COMMAND_EMBARK, NULL}
    ,                           /* embark a carrier */
    {"enterbase", 1, COMMAND_ENTERBASE, NULL}
    ,                           /* enterbase via <dir> */
    {"enterbay", 0, COMMAND_ENTERBAY, NULL}
    ,                           /* enter a DS's bay */
    {"jump", 1, COMMAND_JUMP, NULL}
    ,                           /* jump */
    {"load", 0, COMMAND_LOAD, NULL}
    ,                           /* load cargo */
    {"pickup", 1, COMMAND_PICKUP, NULL}
    ,                           /* pickup a given target */
    {"report", 0, COMMAND_REPORT, NULL}
    ,                           /* report current conditions */
    {"roammode", 1, COMMAND_ROAMMODE, NULL}
    ,                           /* more roam stuff */
    {"shutdown", 0, COMMAND_SHUTDOWN, NULL}
    ,                           /* shutdown the AI's unit */
    {"speed", 1, COMMAND_SPEED, NULL}
    ,                           /* set a given speed (% of max) */
    {"startup", 0, COMMAND_STARTUP, NULL}
    ,                           /* startup an AI's unit */
    {"stopgun", 0, COMMAND_STOPGUN, NULL}
    ,                           /* make the AI stop shooting stuff */
    {"swarm", 1, COMMAND_SWARM, NULL}
    ,                           /* ? */
    {"swarmmode", 1, COMMAND_SWARMMODE, NULL}
    ,                           /* ? */
    {"udisembark", 0, COMMAND_UDISEMBARK, NULL}
    ,                           /* disembark from a carrier */
    {"unload", 0, COMMAND_UNLOAD, NULL}
    ,                           /* unload cargo */
    {NULL, 0, AUTO_NUM_COMMANDS, NULL}
};

/* backwards compat till I can fix all of these */
/* \todo {Get rid of these once we're done redoing the AI} */
#define GSTART      AUTO_GSTART
#define PSTART      AUTO_PSTART
#define CCH         AUTO_CHECKS
#define REDO        AUTO_COM

/*
 * AI Startup - force AI to startup if its not
 */
void auto_command_startup(AUTO *autopilot, MECH *mech) {

    if (Started(mech))
        return;

    mech_startup(autopilot->mynum, mech, "");

}

/*
 * AI Shutdown - force AI to shutdown if its not
 */
void auto_command_shutdown(AUTO *autopilot, MECH *mech) {
    
    if (!Started(mech))
        return;

    mech_shutdown(autopilot->mynum, mech, "");

}

#if 0
/*! \todo {Not really sure what this does and don't really care
    I just know we need to do something about this} */
void gradually_load(MECH * mech, int loc, int percent)
{
    int pile[BRANDCOUNT + 1][NUM_ITEMS];
    float spd = (float) MMaxSpeed(mech);
    float nspd = (float) MechCargoMaxSpeed(mech, (float) spd);
    int cnt = 0;
    char *t;
    int i, j;
    int i1, i2, i3;
    int lastid = -1, lastbrand = -1;

    /* XXX Fix this - was broken when CargoMaxSpeed interface changed */
    bzero(pile, sizeof(pile));
    t = silly_atr_get(loc, A_ECONPARTS);
    while (*t) {
	if (*t == '[')
	    if ((sscanf(t, "[%d,%d,%d]", &i1, &i2, &i3)) == 3) {
		pile[i2][i1] += i3;
		cnt++;
	    }
	t++;
    }
    while (nspd > ((float) spd * percent / 100) && cnt) {
	for (j = 0; j <= BRANDCOUNT; j++) {
	    for (i = 0; i < NUM_ITEMS; i++)
		if (pile[j][i])
		    break;
	    if (i != NUM_ITEMS)
		break;
	}
	if (i == NUM_ITEMS)
	    break;
	lastid = i;
	lastbrand = j;
	econ_change_items(loc, i, j, -1);
	econ_change_items(mech->mynum, i, j, 1);
	pile[j][i]--;
	cnt--;
	SetCargoWeight(mech);
	nspd = (float) MechCargoMaxSpeed(mech, (float) spd);
    }
    if (lastid >= 0) {
	i = lastid;
	j = lastbrand;
	econ_change_items(loc, i, j, 1);
	econ_change_items(mech->mynum, i, j, -1);
    }
    SetCargoWeight(mech);
}

void autopilot_load_cargo(dbref player, MECH * mech, int percent)
{
    DOCHECK(fabs(MechSpeed(mech)) > MP1, "You're moving too fast!");
    DOCHECK(Location(mech->mynum) != mech->mapindex ||
	In_Character(Location(mech->mynum)), "You aren't inside hangar!");
    if (loading_bay_whine(player, Location(mech->mynum), mech))
	return;
    gradually_load(mech, mech->mapindex, percent);
    SetCargoWeight(mech);
}
#endif

/* Recal the AI to the proper map */
/*! \todo{Possibly move this to autopilot_core.c} */
void auto_cal_mapindex(MECH * mech) {
    
    AUTO *autopilot;
    char error_buf[MBUF_SIZE];

    if (!mech) {
        SendError("Null pointer catch in auto_cal_mapindex");
        return;
    }

    if (MechAuto(mech) > 0) {
        if (!(autopilot = FindObjectsData(MechAuto(mech))) || 
                !Good_obj(MechAuto(mech)) || 
                Location(MechAuto(mech)) != mech->mynum) {
            snprintf(error_buf, MBUF_SIZE, "Mech #%d thinks it has the Autopilot #%d on it"
                    " but FindObj breaks", mech->mynum, MechAuto(mech));
            SendError(error_buf);
            MechAuto(mech) = -1;
        } else {

            /* Check here so if the AI is leaving by command it doesn't
             * reset the mapindex (which was messing up auto_leave_event) */
            if (auto_get_command_enum(autopilot, 1) != GOAL_LEAVEBASE) {
                autopilot->mapindex = mech->mapindex;
            }

        }
    }
    return; 
}

/*
 * Function to turn chasetarget on/off as well as let the AI
 * remember that it was on.
 *
 * Figured this was easier then coding a bunch of blocks of
 * stuff all over the place.
 */
void auto_set_chasetarget_mode(AUTO *autopilot, int mode) {

    /* Depending on the mode we do different things*/
    switch (mode) {

        case AUTO_CHASETARGET_ON:

            /* Start Chasing */
            if (!ChasingTarget(autopilot))
                StartChasingTarget(autopilot);

            /* Reset this flag because we don't need it set */
            if (WasChasingTarget(autopilot))
                ForgetChasingTarget(autopilot);

            /* Flags to reset */
            autopilot->chase_target = -10;
            autopilot->chasetarg_update_tick = AUTOPILOT_CHASETARG_UPDATE_TICK;

            break;

       case AUTO_CHASETARGET_OFF:

            /* Stop Chasing */
            if (ChasingTarget(autopilot))
                StopChasingTarget(autopilot);

            /* Reset this flag because we don't need it set */
            if (WasChasingTarget(autopilot))
                ForgetChasingTarget(autopilot);

            break;

       case AUTO_CHASETARGET_REMEMBER:

            /* If we we had chasetarget on - turn it back on */
            if (WasChasingTarget(autopilot)) {

                /* Start chasing */
                if (!ChasingTarget(autopilot))
                    StartChasingTarget(autopilot);

                /* Reset the values */
                autopilot->chase_target = -10;
                autopilot->chasetarg_update_tick = AUTOPILOT_CHASETARG_UPDATE_TICK;

                /* Unset the flag because we don't need it now */
                ForgetChasingTarget(autopilot);

            }

            break;

       case AUTO_CHASETARGET_SAVE:

            /* If we are chasing a target turn this off 
             * but save it */
            if (ChasingTarget(autopilot)) {

                StopChasingTarget(autopilot);
                RememberChasingTarget(autopilot);

            }

            break;

    }

}

#if 0
void autopilot_cmode(AUTO * a, MECH * mech, int mode, int range)
{
    static char buf[MBUF_SIZE];
    if (!a || !mech)
    return;
    if (mode < 0 || mode > 2)
        return;
    if (range < 0 || range > 40)
        return;
    a->auto_cdist = range;
    a->auto_cmode = mode;
    return;

}

void autopilot_swarm(MECH * mech, char *id)
{
    if (MechType(mech) == CLASS_BSUIT)
        bsuit_swarm(GOD, mech, id);
}

void autopilot_attackleg(MECH * mech, char *id)
{
    bsuit_attackleg(GOD, mech, id);
}

#endif

/*
 * Interface to the autogun system
 * Even tho it takes 1 argument, we will parse that
 * 1 argument looking for pieces.
 */
void auto_command_autogun(AUTO *autopilot, MECH *mech) {

    dbref target_dbref;
    MECH *target;
    char *argument;
    char error_buf[MBUF_SIZE];
    char *args[AUTOPILOT_MAX_ARGS - 1];
    int argc;
    int i;

    /* Read in the argument */
    argument = auto_get_command_arg(autopilot, 1, 1);

    /* Parse the argument */
    argc = proper_explodearguments(argument, args, AUTOPILOT_MAX_ARGS - 1);

    /* Free the argument */
    free(argument);

    /* Now we check to see how many arguments it found */
    if (argc == 1) {
        
        /* Ok its either going to be on or off */
        if (strcmp(args[0], "on") == 0) {

            /* Reset the AI parameters */
            autopilot->target = -1;
            autopilot->target_score = 0;
            autopilot->target_update_tick = AUTO_GUN_UPDATE_TICK;

            /* Check if assigned target flag on */
            if (AssignedTarget(autopilot)) {
                UnassignTarget(autopilot);
            }

            /* Get the AI going */
            AUTO_GSTART(autopilot, mech);

            if (Gunning(autopilot)) {
                DoStopGun(autopilot);
            }

            DoStartGun(autopilot);

        } else if (strcmp(args[0], "off") == 0) {

            /* Reset the target */
            autopilot->target = -2;
            autopilot->target_score = 0;
            autopilot->target_update_tick = 0;

            /* Check if Assigned Target Flag on */
            if (AssignedTarget(autopilot)) {
                UnassignTarget(autopilot);
            }

            if (Gunning(autopilot)) {
                DoStopGun(autopilot);
            }

        } else {

            /* Invalid command */
            snprintf(error_buf, MBUF_SIZE, "AI Error - AI #%d given bad"
                    " argument for autogun command", autopilot->mynum);
            SendAI(error_buf);

        }

    } else if (argc == 2) {

        /* Check for 'target' */
        if (strcmp(args[0], "target") == 0) {

            /* Read in the 2nd argument - the target */
            if (Readnum(target_dbref, args[1])) {

                /* Invalid command */
                snprintf(error_buf, MBUF_SIZE, "AI Error - AI #%d given bad"
                        " argument for autogun command", autopilot->mynum);
                SendAI(error_buf);

                /* Free Args */
                for (i = 0; i < AUTOPILOT_MAX_ARGS - 1; i++) {
                    if (args[i])
                        free(args[i]);
                }

                return;
            }

            /* Now see if its a mech */
            if (!(target = getMech(target_dbref))) {

                snprintf(error_buf, MBUF_SIZE, "AI Error - AI #%d given bad"
                        " target for autogun command", autopilot->mynum);
                SendAI(error_buf);

                /* Free Args */
                for (i = 0; i < AUTOPILOT_MAX_ARGS - 1; i++) {
                    if (args[i])
                        free(args[i]);
                }

                return;
            }

            /* Ok valid unit so lets lock it and setup parameters */
            autopilot->target = target_dbref;
            autopilot->target_score = 0;
            autopilot->target_update_tick = 0;

            /* Set the Assigned Flag */
            if (!AssignedTarget(autopilot)) {
                AssignTarget(autopilot);
            }

            /* Get the AI going */
            AUTO_GSTART(autopilot, mech);

            if (Gunning(autopilot)) {
                DoStopGun(autopilot);
            }

            DoStartGun(autopilot);

        } else {

            /* Invalid command */
            snprintf(error_buf, MBUF_SIZE, "AI Error - AI #%d given bad"
                    " argument for autogun command", autopilot->mynum);
            SendAI(error_buf);

        }

    }


    /* Free Args */
    for (i = 0; i < AUTOPILOT_MAX_ARGS - 1; i++) {
        if (args[i])
            free(args[i]);
    }

}

/*
 * Command to interface between chasetarget and follow
 */
void auto_command_chasetarget(AUTO *autopilot) {

    /* Fire off follow event */
    AUTOEVENT(autopilot, EVENT_AUTOFOLLOW, auto_astar_follow_event,
            AUTOPILOT_FOLLOW_TICK, 0);

    return;
}

/*
 * Command to try to get AI to pickup a target
 */
void auto_command_pickup(AUTO *autopilot, MECH *mech) {

    char *argument;
    int target;
    char error_buf[MBUF_SIZE];
    char buf[SBUF_SIZE];
    MECH *tempmech;

    /*! \todo {Add in more checks for picking up target} */

    /* Read in the argument */
    argument = auto_get_command_arg(autopilot, 1, 1);
    if (Readnum(target, argument)) {
        
        snprintf(error_buf, MBUF_SIZE, "AI Error - AI #%d given bad"
                " argument for pickup command", autopilot->mynum);
        SendAI(error_buf);
        free(argument);
        return;

    }
    free(argument);

    /* Check the target */
    if (!(tempmech = getMech(target))) {
        snprintf(error_buf, MBUF_SIZE, "AI Error - AI #%d unable to pickup"
                " unit #%d", autopilot->mynum, target);
        SendAI(error_buf);
        return;
    }

    /* Now try and pick it up */
    strcpy(buf, MechIDS(tempmech, 1));
    mech_pickup(GOD, mech, buf);

    /*! \todo {Possibly add in something either here or in autopilot_radio.c
     * so that when the unit is picked up or not, it radios a message} */

}

/*
 * Tell AI to drop whatever they're carrying
 */
void auto_command_dropoff(MECH *mech) {
    mech_dropoff(GOD, mech, NULL); 
}

/*
 * Tell AI to set its speed (in %)
 */
void auto_command_speed(AUTO *autopilot) {

    char *argument;
    unsigned short speed;
    char error_buf[MBUF_SIZE];

    /* Read in the argument */
    argument = auto_get_command_arg(autopilot, 1, 1);
    if (Readnum(speed, argument)) {
        
        snprintf(error_buf, MBUF_SIZE, "AI Error - AI #%d given bad"
                " argument for speed command", autopilot->mynum);
        SendAI(error_buf);
        free(argument);
        return;

    }
    free(argument);

    /* Make sure its a valid speed value */
    if (speed < 1 || speed > 100) {

        snprintf(error_buf, MBUF_SIZE, "AI Error - AI #%d given bad"
                " argument for speed command - out side of the range",
                autopilot->mynum);
        SendAI(error_buf);
        return;

    }

    /* Now set it */
    autopilot->speed = speed;

}

/*
 * Command to get AI to embark a carrier
 */
void auto_command_embark(AUTO *autopilot, MECH *mech) {

    char *argument;
    int target;
    char error_buf[MBUF_SIZE];
    char buf[SBUF_SIZE];
    MECH *tempmech;

    /* Make sure the mech is on and standing */
    AUTO_GSTART(autopilot, mech);

    /* Read in the argument */
    argument = auto_get_command_arg(autopilot, 1, 1);
    if (Readnum(target, argument)) {
        
        snprintf(error_buf, MBUF_SIZE, "AI Error - AI #%d given bad"
                " argument for embark command", autopilot->mynum);
        SendAI(error_buf);
        free(argument);
        return;

    }
    free(argument);

    /* Check the target */
    if (!(tempmech = getMech(target))) {
        snprintf(error_buf, MBUF_SIZE, "AI Error - AI #%d unable to embark"
                " unit #%d", autopilot->mynum, target);
        SendAI(error_buf);
        return;
    }

    strcpy(buf, MechIDS(tempmech, 1));
    mech_embark(GOD, mech, buf); 

}

/*
 * Function to force AI to disembark a carrier
 */
void auto_command_udisembark(MECH *mech) {
    
    dbref pil = -1;
    char *buf;

    buf = silly_atr_get(mech->mynum, A_PILOTNUM);
    sscanf(buf, "#%d", &pil);
    mech_udisembark(pil, mech, "");

}

#if 0
void autopilot_enterbase(MECH * mech, int dir)
{
    static char strng[2];

    switch (dir) {
    case 0:
        strcpy(strng, "n");
        break;
    case 1:
        strcpy(strng, "e");
        break;
    case 2:
        strcpy(strng, "s");
        break;
    case 3:
        strcpy(strng, "w");
        break;
    default:
        sprintf(strng, "%c", dir);
        break;
    }
    mech_enterbase(GOD, mech, strng);
}
#endif

/*
 * Main Autopilot event, checks to see what command we should
 * be running and tries to run it
 */
void auto_com_event(MUXEVENT *muxevent) {

    AUTO *autopilot = (AUTO *) muxevent->data;
    MECH *mech = autopilot->mymech;
    MECH *tempmech;
    char buf[SBUF_SIZE];
    int i, j, t;

    command_node *command;

    /* No mech and/or no AI */
    if (!IsMech(mech->mynum) || !IsAuto(autopilot->mynum))
        return;

    /* Make sure the map exists */
    if (!(FindObjectsData(mech->mapindex))) {
        autopilot->mapindex = mech->mapindex;
        PilZombify(autopilot);
        /*
        if (GVAL(a, 0) != COMMAND_UDISEMBARK && GVAL(a, 0) != GOAL_WAIT)
            return;
        */
        if (auto_get_command_enum(autopilot, 1) != COMMAND_UDISEMBARK)
            return;
    }

    /* Set the MAP on the AI */
    if (autopilot->mapindex < 0)
        autopilot->mapindex = mech->mapindex;
   
    /* Basic Checks */
    AUTO_CHECKS(autopilot);

    /* Get the enum value for the FIRST command */
    switch (auto_get_command_enum(autopilot, 1)) {

        /* First check the various GOALs then the COMMANDs */
        case GOAL_CHASETARGET:
            auto_command_chasetarget(autopilot);
            return;
        case GOAL_DUMBGOTO:
            AUTOEVENT(autopilot, EVENT_AUTOGOTO, auto_dumbgoto_event,
                    AUTOPILOT_GOTO_TICK, 0);
            return;
        case GOAL_DUMBFOLLOW:
            AUTOEVENT(autopilot, EVENT_AUTOFOLLOW, auto_dumbfollow_event,
                   AUTOPILOT_FOLLOW_TICK, 0);
            return;

        case GOAL_FOLLOW:
            AUTO_GSTART(autopilot, mech);
            AUTOEVENT(autopilot, EVENT_AUTOFOLLOW, auto_astar_follow_event,
                    AUTOPILOT_FOLLOW_TICK, 0);
            return;

        case GOAL_GOTO:
            AUTO_GSTART(autopilot, mech);
            AUTOEVENT(autopilot, EVENT_AUTOGOTO, auto_astar_goto_event,
                    AUTOPILOT_GOTO_TICK, 1);
            return;

        case GOAL_LEAVEBASE:
            AUTO_GSTART(autopilot, mech);
            autopilot->mapindex = mech->mapindex;
            AUTOEVENT(autopilot, EVENT_AUTOLEAVE, auto_leave_event,
                    AUTOPILOT_LEAVE_TICK, 0);
            return;

        case GOAL_OLDGOTO:
            AUTO_GSTART(autopilot, mech);
            AUTOEVENT(autopilot, EVENT_AUTOGOTO, auto_goto_event, 
                    AUTOPILOT_GOTO_TICK, 0);
            return;

#if 0
        case GOAL_WAIT:
            i = GVAL(a, 1);
            j = GVAL(a, 2);
            if (!i) {
                PG(a) += CCLEN(a);
                AUTOEVENT(a, EVENT_AUTOCOM, auto_com_event, MAX(1, j), 0);
            } else {
                if (i == 1) {
                    if (MechNumSeen(mech)) {
                        ADVANCE_PG(a);
                    } else {
                        AUTOEVENT(a, EVENT_AUTOCOM, auto_com_event,
                                AUTOPILOT_WAITFOE_TICK, 0);
                    }
                } else {
                    ADVANCE_PG(a);
                }
            }
            return;
#endif
#if 0
        case COMMAND_ATTACKLEG:
            if (!(tempmech = getMech(GVAL(a, 1)))) {
                SendAI(tprintf("AIAttacklegError #%d", GVAL(a,1)));
                //ADVANCE_PG(a);
                auto_goto_next_command(a);
                return;
            }
            strcpy(buf, MechIDS(tempmech, 1));
            autopilot_attackleg(mech, buf);
            //ADVANCE_PG(a);
            auto_goto_next_command(a);
            return; 
#endif

        case COMMAND_AUTOGUN:
            auto_command_autogun(autopilot, mech);
            auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
            break;

#if 0
        case COMMAND_CHASEMODE:
            if (GVAL(a,1))
                a->flags |= AUTOPILOT_CHASETARG;
            else
                a->flags &= ~AUTOPILOT_CHASETARG;
            //ADVANCE_PG(a);
            auto_goto_next_command(a);
            return;
#endif
#if 0
        case COMMAND_CMODE:
            i = GVAL(a,1);
            j = GVAL(a,2);
            autopilot_cmode(a, mech, i, j);
            //ADVANCE_PG(a);
            auto_goto_next_command(a);
            return;
#endif

        case COMMAND_DROPOFF:
            auto_command_dropoff(mech);
            auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
            return;

        case COMMAND_EMBARK:
            auto_command_embark(autopilot, mech);
            auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
            return;

        case COMMAND_ENTERBASE:
            AUTO_GSTART(autopilot, mech);
            AUTOEVENT(autopilot, EVENT_AUTOENTERBASE, auto_enter_event, 1, 0);
            return;

#if 0
        case COMMAND_ENTERBAY:
            PSTART(a, mech);
            mech_enterbay(GOD, mech, my2string(""));
            //ADVANCE_PG(a);
            auto_goto_next_command(a);
            return;
#endif
#if 0
        case COMMAND_JUMP:
            if (auto_valid_progline(a, GVAL(a, 1))) {
                PG(a) = GVAL(a, 1);
                AUTOEVENT(a, EVENT_AUTOCOM, auto_com_event, 
                        AUTOPILOT_NC_DELAY, 0);
            } else {
                ADVANCE_PG(a);
            }
            return;
#endif
#if 0
        case COMMAND_LOAD:
/*          mech_loadcargo(GOD, mech, "50"); */
            autopilot_load_cargo(GOD, mech, 50);
            //ADVANCE_PG(a);
            auto_goto_next_command(a);
            break;
#endif
        case COMMAND_PICKUP:
            auto_command_pickup(autopilot, mech);
            auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
            return;
#if 0
        case COMMAND_ROAMMODE:
            t = a->flags;
            if (GVAL(a,1)) {
                a->flags |= AUTOPILOT_ROAMMODE;
                if (!(t & AUTOPILOT_ROAMMODE)) {
                    if (MechType(mech) == CLASS_BSUIT)
                        a->flags |= AUTOPILOT_SWARMCHARGE;
                    auto_addcommand(a->mynum, a, tprintf("roam 0 0"));
                }
            } else {
                a->flags &= ~AUTOPILOT_ROAMMODE;
            }
            //ADVANCE_PG(a);
            auto_goto_next_command(a);
            return;
#endif
        case COMMAND_SHUTDOWN:
            auto_command_shutdown(autopilot, mech);
            auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
            return;

        case COMMAND_SPEED:
            auto_command_speed(autopilot);
            auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
            return;

        case COMMAND_STARTUP:
            auto_command_startup(autopilot, mech);
            auto_goto_next_command(autopilot, AUTOPILOT_STARTUP_TICK);
            return;
#if 0
        case COMMAND_STOPGUN:
            if (Gunning(a))
                DoStopGun(a);
            //ADVANCE_PG(a);
            auto_goto_next_command(a);
            break;
#endif
#if 0 
        case COMMAND_SWARM:
            if (!(tempmech = getMech(GVAL(a, 1)))) {
                SendAI(tprintf("AISwarmError #%d", GVAL(a,1)));
                //ADVANCE_PG(a);
                auto_goto_next_command(a);
                return;
            }
            strcpy(buf, MechIDS(tempmech, 1));
            autopilot_swarm(mech, buf);
            //ADVANCE_PG(a);
            auto_goto_next_command(a);
            return; 
#endif
#if 0
        case COMMAND_SWARMMODE:
            if (MechType(mech) != CLASS_BSUIT) {
                //ADVANCE_PG(a);
                auto_goto_next_command(a);
                return;
            }
            if (GVAL(a,1))
                a->flags |= AUTOPILOT_SWARMCHARGE;
            else
                a->flags &= ~AUTOPILOT_SWARMCHARGE;
            //ADVANCE_PG(a);
            auto_goto_next_command(a);
            return;
#endif

        case COMMAND_UDISEMBARK:
            auto_command_udisembark(mech);
            auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
            return; 

#if 0
        case COMMAND_UNLOAD:
            mech_unloadcargo(GOD, mech, my2string(" * 9999"));
            //ADVANCE_PG(a);
            auto_goto_next_command(a);
            break;
#endif
    }
}

/*! \todo {Make the speed up and slow down functions behave a little better} */

/*
 * Function to force the AI to move if its not near its target
 */
static void speed_up_if_neccessary(AUTO * a, MECH * mech, int tx, int ty,
    int bearing) {
    
    if (bearing < 0 || abs((int) MechDesiredSpeed(mech)) < 2)
        if (bearing < 0 || abs(bearing - MechFacing(mech)) <= 30)
            if (MechX(mech) != tx || MechY(mech) != ty) {
                ai_set_speed(mech, a, MMaxSpeed(mech));
            }
}

/*
 * Quick function to change the AI's heading to the current
 * bearing of its target
 */
static void update_wanted_heading(AUTO * a, MECH * mech, int bearing) {
    
    if (MechDesiredFacing(mech) != bearing)
        mech_heading(a->mynum, mech, tprintf("%d", bearing));

}

/*
 * Slow down the AI if its close to its target hex
 */
/*! \todo {Make this more variable perhaps so it wont always slow down?} */
static int slow_down_if_neccessary(AUTO * a, MECH * mech, float range,
        int bearing, int tx, int ty) {
    
    if (range < 0)
        range = 0;
    if (range > 2.0)
        return 0;
    if (abs(bearing - MechFacing(mech)) > 30) {
        /* Fix the bearing as well */
        ai_set_speed(mech, a, 0);
        update_wanted_heading(a, mech, bearing);
    } else if (tx == MechX(mech) && ty == MechY(mech)) {
        ai_set_speed(mech, a, 0);
    } else {    /* slowdown */
        ai_set_speed(mech, a, (float) (0.4 + range / 2.0) * MMaxSpeed(mech));
    }
    return 1;
}


/*
 * Quick calcualtion of range and bearing from mech to target
 * hex
 */
void figure_out_range_and_bearing(MECH * mech, int tx, int ty,
        float *range, int *bearing) {
    
    float x, y;

    MapCoordToRealCoord(tx, ty, &x, &y);
    *bearing = FindBearing(MechFX(mech), MechFY(mech), x, y);
    *range = FindHexRange(MechFX(mech), MechFY(mech), x, y);
}


/* Basically, all we need to do is course correction now and then.
   In case we get disabled, we call for help now and then */
/*
 * Old goto system - will phase it out
 */
void auto_goto_event(MUXEVENT * e) {
    
    AUTO *autopilot = (AUTO *) e->data;
    int tx, ty;
    float dx, dy;
    MECH *mech = autopilot->mymech;
    float range;
    int bearing;

    char *argument;

    if (!IsMech(mech->mynum) || !IsAuto(autopilot->mynum))
        return;

    /* Basic Checks */
    AUTO_CHECKS(autopilot);

    /* Make sure mech is started and standing */
    AUTO_GSTART(autopilot, mech);

    /* Get the first argument - x coord */
    argument = auto_get_command_arg(autopilot, 1, 1);
    if (Readnum(tx, argument)) {
        /*! \todo {add a thing here incase the argument isn't a number} */
        free(argument);
    }
    free(argument);

    /* Get the second argument - y coord */
    argument = auto_get_command_arg(autopilot, 1, 2);
    if (Readnum(ty, argument)) {
        /*! \todo {add a thing here incase the argument isn't a number} */
        free(argument);
    }
    free(argument);
    
    if (MechX(mech) == tx && MechY(mech) == ty &&
            abs(MechSpeed(mech)) < 0.5) {

        /* We've reached this goal! Time for next one. */
        ai_set_speed(mech, autopilot, 0);

        auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
        return;
    }

    MapCoordToRealCoord(tx, ty, &dx, &dy);
    figure_out_range_and_bearing(mech, tx, ty, &range, &bearing);
    if (!slow_down_if_neccessary(autopilot, mech, range, bearing, tx, ty)) {

        /* Use the AI */
        if (ai_check_path(mech, autopilot, dx, dy, 0.0, 0.0))
            AUTOEVENT(autopilot, EVENT_AUTOGOTO, auto_goto_event,
                    AUTOPILOT_GOTO_TICK, 0);

    } else {
        AUTOEVENT(autopilot, EVENT_AUTOGOTO, auto_goto_event, 
                AUTOPILOT_GOTO_TICK, 0);
    }

}

#if 0
/* ROAMMODE is a funky beast */
void auto_roam_event(MUXEVENT * e)
{
    AUTO *a = (AUTO *) e->data;
    int tx, ty;
    float dx, dy, range;
    MECH *mech = a->mymech;
    MAP *map;
    int bearing, i = 1, t;

    if (!IsMech(mech->mynum) || !IsAuto(a->mynum))
        return;

    CCH(a);
    GSTART(a, mech);
    tx = GVAL(a, 1);
    ty = GVAL(a, 2);

    if (!mech || !(map = FindObjectsData(mech->mapindex))) {
        return;
    }

    if (!(a->flags & AUTOPILOT_ROAMMODE) || MechTarget(mech) > 0) {
        return;
    }

    if ((tx == 0 && ty == 0) || e->data2 > 0 || (MechX(mech) == tx 
            && MechY(mech) == ty && abs(MechSpeed(mech)) < 0.5)) {
        while (i) {
            tx = BOUNDED(1, Number(20, map->map_width - 21), map->map_width - 1);
            ty = BOUNDED(1, Number(20, map->map_height - 21), map->map_height - 1);
            MapCoordToRealCoord(tx, ty, &dx, &dy);
            t = GetRTerrain(map, tx, ty);
            range = FindRange(MechFX(mech), MechFY(mech), MechFZ(mech), 
                    dx, dy, ZSCALE * GetElev(map, tx, ty));
            if ((InLineOfSight(mech, NULL, tx, ty, range) && 
                    t != WATER && t != HIGHWATER && t != MOUNTAINS ) || i > 5000) {
                i = 0;  
            } else {
                i++;
            }
        }
        a->commands[a->program_counter + 1] = tx;
        a->commands[a->program_counter + 2] = ty;
        AUTOEVENT(a, EVENT_AUTOGOTO, auto_roam_event, AUTOPILOT_GOTO_TICK, 0);
        return;
    }
    MapCoordToRealCoord(tx, ty, &dx, &dy);
    figure_out_range_and_bearing(mech, tx, ty, &range, &bearing);
    if (!slow_down_if_neccessary(a, mech, range, bearing, tx, ty)) {
        /* Use the AI */
        if (ai_check_path(mech, a, dx, dy, 0.0, 0.0))
            AUTOEVENT(a, EVENT_AUTOGOTO, auto_roam_event, AUTOPILOT_GOTO_TICK, 0);
    } else {
        AUTOEVENT(a, EVENT_AUTOGOTO, auto_roam_event, AUTOPILOT_GOTO_TICK, 0);
    }
}
#endif

/*
 * Dumbly[goto] a given a hex
 */
void auto_dumbgoto_event(MUXEVENT * e) {
    
    AUTO *autopilot = (AUTO *) e->data;
    int tx, ty;
    MECH *mech = autopilot->mymech;
    float range;
    int bearing;

    char *argument;

    if (!IsMech(mech->mynum) || !IsAuto(autopilot->mynum))
        return;

    /* Basic Checks */
    AUTO_CHECKS(autopilot);

    /* Make sure mech is started and standing */
    AUTO_GSTART(autopilot, mech);

    /* Get the first argument - x coord */
    argument = auto_get_command_arg(autopilot, 1, 1);
    if (Readnum(tx, argument)) {
        /*! \todo {add a thing here incase the argument isn't a number} */
        free(argument);
    }
    free(argument);

    /* Get the second argument - y coord */
    argument = auto_get_command_arg(autopilot, 1, 2);
    if (Readnum(ty, argument)) {
        /*! \todo {add a thing here incase the argument isn't a number} */
        free(argument);
    }
    free(argument);

    /* If we're at the target hex - stop */
    if (MechX(mech) == tx && MechY(mech) == ty &&
            abs(MechSpeed(mech)) < 0.5) {
        
        /* We've reached this goal! Time for next one. */
        ai_set_speed(mech, autopilot, 0);
            
        auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
        return;
    }

    /* Make our way to the goal */
    figure_out_range_and_bearing(mech, tx, ty, &range, &bearing);
    speed_up_if_neccessary(autopilot, mech, tx, ty, bearing);
    slow_down_if_neccessary(autopilot, mech, range, bearing, tx, ty);
    update_wanted_heading(autopilot, mech, bearing);
    AUTOEVENT(autopilot, EVENT_AUTOGOTO, auto_dumbgoto_event, 
            AUTOPILOT_GOTO_TICK, 0);
}

/*
 * The Astar goto event
 * Uses the A* (Astar) pathfinding method used
 * in common games to get the AI from point A
 * to point B
 */
void auto_astar_goto_event(MUXEVENT *muxevent) {

    AUTO *autopilot = (AUTO *) muxevent->data;
    int tx, ty;
    MECH *mech = autopilot->mymech;
    float range;
    int bearing;

    int generate_path = (int) muxevent->data2;

    char *argument;
    astar_node *temp_astar_node;

    char error_buf[MBUF_SIZE];

    if (!IsMech(mech->mynum) || !IsAuto(autopilot->mynum))
        return;

    /* Basic Checks */
    AUTO_CHECKS(autopilot);

    /* Make sure mech is started and standing */
    AUTO_GSTART(autopilot, mech);

    /* Do we need to generate the path */
    if (generate_path) {

        /* Get the first argument - x coord */
        argument = auto_get_command_arg(autopilot, 1, 1);
        if (Readnum(tx, argument)) {
            /*! \todo {add a thing here incase the argument isn't a number} */
            free(argument);
        }
        free(argument);

        /* Get the second argument - y coord */
        argument = auto_get_command_arg(autopilot, 1, 2);
        if (Readnum(ty, argument)) {
            /*! \todo {add a thing here incase the argument isn't a number} */
            free(argument);
        }
        free(argument);

        /* Look for a path */
        if(!(auto_astar_generate_path(autopilot, mech, tx, ty))) {

            /* Couldn't find a path for some reason */
            snprintf(error_buf, MBUF_SIZE, "Internal AI Error - Attempting to"
                    " generate an astar path for AI #%d to hex %d,%d but was"
                    " unable to", autopilot->mynum, tx, ty);
            SendAI(error_buf);

            /*! \todo {add in some message the AI can give if it can't find a path} */

            auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
            return;

        }

    }
    
    /* Make sure list is ok */ 
    if (!(autopilot->astar_path) || (dllist_size(autopilot->astar_path) <= 0)) {

        snprintf(error_buf, MBUF_SIZE, "Internal AI Error - Attempting to follow"
                " Astar path for AI #%d - but the path is not there",
                autopilot->mynum);
        SendAI(error_buf);

        auto_destroy_astar_path(autopilot);
        auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
        return;

    }

    /* Get the current hex target */
    temp_astar_node = (astar_node *) dllist_get_node(autopilot->astar_path, 1);

    if (!(temp_astar_node)) {

        snprintf(error_buf, MBUF_SIZE, "Internal AI Error - Attemping to follow"
                " Astar path for AI #%d - but the current astar node does not"
                " exist", autopilot->mynum);
        SendAI(error_buf);
        
        auto_destroy_astar_path(autopilot);
        auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
        return;

    }

    /* Are we in the current target hex */
    if ((MechX(mech) == temp_astar_node->x) && 
            (MechY(mech) == temp_astar_node->y)) {

        /* Is this the last hex */
        if (dllist_size(autopilot->astar_path) == 1) {

            /* Done! */
            ai_set_speed(mech, autopilot, 0);

            /* Destroy the path and goto the next command */
            auto_destroy_astar_path(autopilot);
            auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
            return;

        } else {

            /* Delete the node and goto the next one */
            temp_astar_node = 
                (astar_node *) dllist_remove_node_at_pos(autopilot->astar_path, 1);
            free(temp_astar_node);

            /* Call this event again */
            AUTOEVENT(autopilot, EVENT_AUTOGOTO, auto_astar_goto_event, 
                    AUTOPILOT_GOTO_TICK, 0);
            return;

        }

    }
   
    /* Set our current goal - not the end goal tho - unless this is
     * the end hex but whatever */
    tx = temp_astar_node->x;
    ty = temp_astar_node->y;

    /* Move towards our next hex */
    figure_out_range_and_bearing(mech, tx, ty, &range, &bearing);
    speed_up_if_neccessary(autopilot, mech, tx, ty, bearing);
    slow_down_if_neccessary(autopilot, mech, range, bearing, tx, ty);
    update_wanted_heading(autopilot, mech, bearing);

    AUTOEVENT(autopilot, EVENT_AUTOGOTO, auto_astar_goto_event, 
            AUTOPILOT_GOTO_TICK, 0);

}

/*
 * New follow system based on astar goto
 */
void auto_astar_follow_event(MUXEVENT *muxevent) {

    AUTO *autopilot = (AUTO *) muxevent->data;
    MECH *mech = autopilot->mymech;
    MECH *target;
    MAP *map;

    dbref target_dbref;

    float range;
    float fx, fy;
    short x, y;
    int bearing;

    char *argument;
    astar_node *temp_astar_node;

    char error_buf[MBUF_SIZE];

    if (!IsMech(mech->mynum) || !IsAuto(autopilot->mynum))
        return;

    /* Basic Checks */
    AUTO_CHECKS(autopilot);

    /* Make sure mech is started and standing */
    AUTO_GSTART(autopilot, mech);

    /* Get the only argument - dbref of target */
    argument = auto_get_command_arg(autopilot, 1, 1);
    if (Readnum(target_dbref, argument)) {
        /*! \todo {add a thing here incase the argument isn't a number} */
        free(argument);
    }
    free(argument);

    /* Get the Map */
    if (!(map = getMap(autopilot->mapindex))) {

        /* Bad Map */
        snprintf(error_buf, MBUF_SIZE, "Internal AI Error - Attempting to"
                " follow unit #%d with AI #%d but AI is not on a valid"
                " Map (#%d).", target_dbref, autopilot->mynum,
                autopilot->mapindex);
        SendAI(error_buf);
        
        auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
        return;

    }

    /* Get the target */
    if (!(target = getMech(target_dbref))) {

        /* Bad Target */
        snprintf(error_buf, MBUF_SIZE, "Internal AI Error - Attempting to"
                " follow unit #%d with AI #%d but its not a valid unit.",
                target_dbref, autopilot->mynum);
        SendAI(error_buf);

        ai_set_speed(mech, autopilot, 0);
        auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
        return;
    }

    if (Destroyed(target) || (target->mapindex != mech->mapindex)) {

        /* Target Dead or not on map */
        snprintf(error_buf, MBUF_SIZE, "Internal AI Error - Attempting to"
                " follow unit #%d with AI #%d but it either is dead or"
                " not on the same map.", target_dbref, autopilot->mynum);
        SendAI(error_buf);

        ai_set_speed(mech, autopilot, 0);
        auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
        return;
    }

    /* Generate the target hex */
    FindXY(MechFX(target), MechFY(target), MechFacing(target) + autopilot->ofsx,
            autopilot->ofsy, &fx, &fy);

    RealCoordToMapCoord(&x, &y, fx, fy);

    /* Make sure the hex is sane */
    if (x < 0 || y < 0 || x >= map->map_width || y >= map->map_height) {

        /* Bad Target Hex */

        /* Reset the hex to the Target's current hex */
        x = MechX(target);
        y = MechY(target);

    }

    /* Are we in the target hex and the target isn't moving ? */
    if ((MechX(mech) == x) && (MechY(mech) == y) && (MechSpeed(target) < 0.5)) {

        /* Ok go into holding pattern */
        ai_set_speed(mech, autopilot, 0);
        AUTOEVENT(autopilot, EVENT_AUTOFOLLOW, auto_astar_follow_event,
                AUTOPILOT_FOLLOW_TICK, 0);
        return;
    }

    /* Do we need to generate the path - only switch paths if we don't have
     * one or if the ticker has gone high enough */
    if (!autopilot->astar_path ||
            autopilot->follow_update_tick >= AUTOPILOT_FOLLOW_UPDATE_TICK) {

        /* Look for a path */
        if(!(auto_astar_generate_path(autopilot, mech, x, y))) {
            /* Couldn't find a path for some reason */
            snprintf(error_buf, MBUF_SIZE, "Internal AI Error - Attempting to"
                    " generate an astar path for AI #%d to hex %d,%d to follow"
                    " unit #%d, but was unable to.",
                    autopilot->mynum, x, y, target_dbref);
            SendAI(error_buf);

            /*! \todo {add in some message the AI can give if it can't find a path} */

            ai_set_speed(mech, autopilot, 0);
            auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
            return;

        }

        /* Zero the ticker */
        autopilot->follow_update_tick = 0;

    }
    
    /* Make sure list is ok */ 
    if (!(autopilot->astar_path) || (dllist_size(autopilot->astar_path) <= 0)) {

        snprintf(error_buf, MBUF_SIZE, "Internal AI Error - Attempting to follow"
                " Astar path for AI #%d - but the path is not there",
                autopilot->mynum);
        SendAI(error_buf);

        /* Destroy List */
        auto_destroy_astar_path(autopilot);
        ai_set_speed(mech, autopilot, 0);
        auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
        return;

    }

    /* Get the current hex target */
    temp_astar_node = (astar_node *) dllist_get_node(autopilot->astar_path, 1);

    if (!(temp_astar_node)) {

        snprintf(error_buf, MBUF_SIZE, "Internal AI Error - Attemping to follow"
                " Astar path for AI #%d - but the current astar node does not"
                " exist", autopilot->mynum);
        SendAI(error_buf);

        /* Destroy List */
        auto_destroy_astar_path(autopilot);
        ai_set_speed(mech, autopilot, 0);
        auto_goto_next_command(autopilot, AUTOPILOT_NC_DELAY);
        return;

    }

    /* Are we in the current target hex */
    if ((MechX(mech) == temp_astar_node->x) && 
            (MechY(mech) == temp_astar_node->y)) {

        /* Is this the last hex */
        if (dllist_size(autopilot->astar_path) == 1) {

            /* Done! */
            ai_set_speed(mech, autopilot, 0);
            auto_destroy_astar_path(autopilot);

            /* Re-Run Follow */
            AUTOEVENT(autopilot, EVENT_AUTOFOLLOW, auto_astar_follow_event,
                AUTOPILOT_FOLLOW_TICK, 0);
            return;

        } else {

            /* Delete the node and goto the next one */
            temp_astar_node = 
                (astar_node *) dllist_remove_node_at_pos(autopilot->astar_path, 1);
            free(temp_astar_node);

            /* Call this event again */
            AUTOEVENT(autopilot, EVENT_AUTOFOLLOW, auto_astar_follow_event,
                AUTOPILOT_FOLLOW_TICK, 0);
            return;

        }

    }
   
    /* Set our current goal - not the end goal tho - unless this is
     * the end hex but whatever */
    x = temp_astar_node->x;
    y = temp_astar_node->y;

    /* Move towards our next hex */
    figure_out_range_and_bearing(mech, x, y, &range, &bearing);
    speed_up_if_neccessary(autopilot, mech, x, y, bearing);
    slow_down_if_neccessary(autopilot, mech, range, bearing, x, y);
    update_wanted_heading(autopilot, mech, bearing);

    /* Increase Tick */
    autopilot->follow_update_tick++;

    AUTOEVENT(autopilot, EVENT_AUTOFOLLOW, auto_astar_follow_event,
            AUTOPILOT_FOLLOW_TICK, 0);

}

#if 0
/* Old follow system - will phase out */
void auto_follow_event(MUXEVENT * e)
{
    AUTO *a = (AUTO *) e->data;
    float fx, fy, newx, newy;
    int h;
    MECH *leader;
    MECH *mech = a->mymech;

    if (!IsMech(mech->mynum) || !IsAuto(a->mynum))
        return;

    CCH(a);
    GSTART(a, mech);
    if (!(leader = getMech(GVAL(a, 1)))) {
        /* For some reason, leader is missing(?) */
        ADVANCE_PG(a);
        return;
    }
    h = MechFacing(leader);
    FindXY(MechFX(leader), MechFY(leader), h + a->ofsx, a->ofsy, &fx, &fy);
    FindComponents(MechSpeed(leader) * MOVE_MOD, MechFacing(leader), 
            &newx, &newy);
    if (ai_check_path(mech, a, fx, fy, newx, newy))
        AUTOEVENT(a, EVENT_AUTOFOLLOW, auto_follow_event,
                AUTOPILOT_FOLLOW_TICK, 0);
}
#endif

/*
 * Make the AI [dumbly]follow the given target
 */
void auto_dumbfollow_event(MUXEVENT * e) {
    
    AUTO *a = (AUTO *) e->data;
    int tx, ty, x, y;
    int h;
    MECH *leader;
    MECH *mech = a->mymech;
    float range;
    int bearing;

    char *argument;
    int target;

    if (!IsMech(mech->mynum) || !IsAuto(a->mynum))
        return;

    /* Basic Checks */
    AUTO_CHECKS(a);

    /* Make sure the mech is started and standing */
    AUTO_GSTART(a, mech);

    /* Get the target */
    argument = auto_get_command_arg(a, 1, 1);
    if (Readnum(target, argument)) {

        /* Not proper number so skip command goto next */
        free(argument);
        auto_goto_next_command(a, AUTOPILOT_NC_DELAY);
        return;

    }
    free(argument);

    if (!(leader = getMech(target)) || Destroyed(leader)) {
        /* For some reason, leader is missing(?) */
        auto_goto_next_command(a, AUTOPILOT_NC_DELAY); 
        return;
    }

    h = MechDesiredFacing(leader);
    x = a->ofsy * cos(TWOPIOVER360 * (270.0 + (h + a->ofsx)));
    y = a->ofsy * sin(TWOPIOVER360 * (270.0 + (h + a->ofsx)));
    tx = MechX(leader) + x;
    ty = MechY(leader) + y;
    if (MechX(mech) == tx && MechY(mech) == ty) {
        /* Do ugly stuff */
        /* For now, try to match speed (if any) and heading (if any) of the
           leader */
        if (MechSpeed(leader) > 1 || MechSpeed(leader) < -1 ||
                MechSpeed(mech) > 1 || MechSpeed(mech) < -1) {
            if (MechDesiredFacing(mech) != MechFacing(leader))
                    mech_heading(a->mynum, mech, tprintf("%d",
                    MechFacing(leader)));
            if (MechSpeed(mech) != MechSpeed(leader))
                    mech_speed(a->mynum, mech, tprintf("%.2f",
                    MechSpeed(leader)));
        }
        AUTOEVENT(a, EVENT_AUTOFOLLOW, auto_dumbfollow_event,
                AUTOPILOT_FOLLOW_TICK, 0);
        return;
    }
    figure_out_range_and_bearing(mech, tx, ty, &range, &bearing);
    speed_up_if_neccessary(a, mech, tx, ty, -1);
    if (MechSpeed(leader) < MP1)
        slow_down_if_neccessary(a, mech, range + 1, bearing, tx, ty);
    update_wanted_heading(a, mech, bearing);
    AUTOEVENT(a, EVENT_AUTOFOLLOW, auto_dumbfollow_event,
            AUTOPILOT_FOLLOW_TICK, 0);
}

/*
 * Command the AI to leave a hangar or base
 */
void auto_leave_event(MUXEVENT * e) {
    
    AUTO *a = (AUTO *) e->data;
    int dir;
    MECH *mech = a->mymech;
    int num;

    char *argument;

    if (!IsMech(mech->mynum) || !IsAuto(a->mynum))
        return;

    /* Basic Checks */
    AUTO_CHECKS(a);

    /* Make sure mech started and standing */
    AUTO_GSTART(a, mech);

    argument = auto_get_command_arg(a, 1, 1);
    if (Readnum(dir, argument)) {
        
        /* Not valid dir so lets just leave heading 0 */
        dir = 0;
    }
    free(argument);

    if (mech->mapindex != a->mapindex) {

        /* We're elsewhere, pal! */
        a->mapindex = mech->mapindex;
        ai_set_speed(mech, a, 0);
        auto_goto_next_command(a, AUTOPILOT_NC_DELAY);
        return;
    }

    /* Still not out yet so keep trying */
    num = a->speed;
    a->speed = 100;
    speed_up_if_neccessary(a, mech, -1, -1, dir);
    a->speed = num;
    update_wanted_heading(a, mech, dir);
    AUTOEVENT(a, EVENT_AUTOLEAVE, auto_leave_event, 
            AUTOPILOT_LEAVE_TICK, 0);
}

/*
 * Function to get the AI to enter a base hex given
 * a certain direction (n w s e)
 */
void auto_enter_event(MUXEVENT * e) {
    
    AUTO *a = (AUTO *) e->data;
    MECH *mech = a->mymech;
    int num;

    char *argument;
    char dir[2];

    if (!mech || !a || !IsMech(mech->mynum) || !IsAuto(a->mynum))
        return;

    /* Basic AI checks */
    AUTO_CHECKS(a);
    AUTO_GSTART(a, mech);

    /* Get enter direction */
    argument = auto_get_command_arg(a, 1, 1);

    switch (argument[0]) {

        case 'n':
        case 'N':
            strcpy(dir, "n");
            break;
        case 's':
        case 'S':
            strcpy(dir, "s");
            break;
        case 'w':
        case 'W':
            strcpy(dir, "w");
            break;
        case 'e':
        case 'E':
            strcpy(dir, "e");
            break;
        default:
            strcpy(dir, "");

    }
    free(argument);

    if (MechDesiredSpeed(mech) != 0.0)
        ai_set_speed(mech, a, 0);

    if (MechSpeed(mech) == 0.0) {
        mech_enterbase(GOD, mech, dir);
        auto_goto_next_command(a, AUTOPILOT_NC_DELAY);
        return;
    }
    AUTOEVENT(a, EVENT_AUTOENTERBASE, auto_enter_event, 1, 0);
}
