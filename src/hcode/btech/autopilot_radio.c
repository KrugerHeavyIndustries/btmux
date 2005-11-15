
/*
 * $Id: autopilot_command.c,v 1.4 2005/08/10 14:09:34 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Tue Sep 23 20:33:33 1997 fingon
 * Last modified: Sat Jun  6 21:47:38 1998 fingon
 *
 */

/* Most of the BattleSheep(tm) code is here.. */

#include <math.h>
#include "mech.h"
#include "autopilot.h"
#include "spath.h"
#include "mech.notify.h"
#include "p.mech.utils.h"
#include "p.mech.sensor.h"
#include "p.mech.los.h"
#include "p.mech.startup.h"
#include "p.mech.maps.h"
#include "p.ds.bay.h"
#include "p.mech.maps.h"
#include "p.bsuit.h"
#include "p.glue.h"

void sendchannelstuff(MECH * mech, int freq, char *msg);

/*! \todo {Should really do away with this some how but i'm being lazy right now} */
#define Clear(a) \
    auto_disengage(a->mynum, a, ""); \
    auto_delcommand(a->mynum, a, "-1"); \
    if (a->target >= -1) { \
        if (AssignedTarget(a) && a->target != -1) { \
            snprintf(buffer, SBUF_SIZE, "autogun target %d", a->target); \
        } else { \
            snprintf(buffer, SBUF_SIZE, "autogun on"); \
        } \
        auto_addcommand(a->mynum, autopilot, buffer); \
    } \

#define BACMD(name) char * name (AUTO *autopilot, MECH *mech, char **args, int argc, int chn)
#define ACMD(name) static BACMD(name)

/* 
 * Master list of AI - radio commands
 */
struct {
    char *sho;
    char *name;
    int args;
    int silent;
    void (*fun)(AUTO *autopilot, MECH *mech, char **args, int argc, char *mesg);
} auto_cmds[] = {
    {
    "auto", "autogun", 1, 0, auto_radio_command_autogun}, {
    "auto", "autogun", 2, 0, auto_radio_command_autogun}, {
#if 0
    "att", "attackleg", 1, 0, auto_attackleg}, {
    "chanf", "chanfreq", 2, 0, auto_setchanfreq}, {
    "chanm", "chanmode", 2, 0, auto_setchanmode}, {
#endif
    "chase", "chasetarg", 1, 0, auto_radio_command_chasetarg}, {
#if 0
    "cm", "cmode", 2, 0, auto_cmode}, {
#endif
    "dfo", "dfollow", 1, 0, auto_radio_command_dfollow}, {
    "dgo", "dgoto", 2, 0, auto_radio_command_dgoto}, {
#if 0
    "dr", "drally", 2, 0, auto_drally}, {
    "dr", "drally", 3, 0, auto_drally}, {
#endif
    "drop", "dropoff", 0, 0, auto_radio_command_dropoff}, {
    "emb", "embark", 1, 0, auto_radio_command_embark}, {
    "en", "enterbase", 0, 0, auto_radio_command_enterbase}, {
    "en", "enterbase", 1, 0, auto_radio_command_enterbase}, {
#if 0
    "en", "enterbay", 0, 0, auto_enterbay}, {
    "en", "enterbay", 1, 0, auto_enterbay}, {
#endif
    "fo", "follow", 1, 0, auto_radio_command_follow}, {
#if 0
    "fr", "freq", 1, 0, auto_freq}, {
#endif
    "go", "goto", 2, 0, auto_radio_command_goto}, {
    "he", "heading", 1, 0, auto_radio_command_heading}, {
    "he", "help", 0, 1, auto_radio_command_help}, {
    "hi", "hide", 0, 0, auto_radio_command_hide}, {
    "jump", "jumpjet", 1, 0, auto_radio_command_jumpjet}, {
    "jump", "jumpjet", 2, 0, auto_radio_command_jumpjet}, {
    "le", "leavebase", 1, 0, auto_radio_command_leavebase}, {
#if 0
    "nog", "nogun", 0, 0, auto_nogun}, {
    "not", "notarget", 0, 0, auto_notarget}, {
#endif
    "ogo", "ogoto", 2, 0, auto_radio_command_ogoto}, {
    "pick", "pickup", 1, 0, auto_radio_command_pickup}, {
    "pos", "position", 2, 0, auto_radio_command_position}, {
    "pr", "prone", 0, 0, auto_radio_command_prone}, {
#if 0
    "ra", "rally", 2, 0, auto_rally}, {
    "ra", "rally", 3, 0, auto_rally}, {
#endif
    "re", "report", 0, 1, auto_radio_command_report}, {
    "reset", "reset", 0, 0, auto_radio_command_reset}, {
#if 0
    "roam", "roammode", 1, 0, auto_roammode }, {
#endif
    "se", "sensor", 2, 0, auto_radio_command_sensor}, {
    "se", "sensor", 0, 0, auto_radio_command_sensor}, {
    "sh", "shutdown", 0, 0, auto_radio_command_shutdown}, {
    "sp", "speed", 1, 0, auto_radio_command_speed}, {
    "st", "stand", 0, 0, auto_radio_command_stand}, {
    "st", "startup", 0, 0, auto_radio_command_startup}, {
    "st", "startup", 1, 0, auto_radio_command_startup}, {
    "st", "stop", 0, 0, auto_radio_command_stop}, {
    "sw", "sweight", 2, 1, auto_radio_command_sweight}, {
#if 0
    "swa", "swarm", 1, 0, auto_swarm}, {
    "swarmc", "swarmcharge", 1, 0, auto_swarmcharge }, {
    "swarmm", "swarmmode", 1, 0, auto_swarmmode }, {
#endif
    "ta", "target", 1, 0, auto_radio_command_target}, {
#if 0
    "ta", "target", 2, 0, auto_target}, {
#endif
    NULL, NULL, 0, 0, NULL}
};

/*
 * The autogun radio interface
 */
void auto_radio_command_autogun(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg) {

    int threshold;

    if (strcmp(args[1], "on") == 0) {

        autopilot->target = -1;
        autopilot->target_score = 0;
        autopilot->target_update_tick = AUTO_GUN_UPDATE_TICK;

        /* Reset the Assigned target flag */
        if (AssignedTarget(autopilot)) {
            UnassignTarget(autopilot);
        }

        if (Gunning(autopilot)) {
            DoStopGun(autopilot);
        }
        DoStartGun(autopilot);

        snprintf(mesg, LBUF_SIZE, "shooting at whatever I want");
        return;

    } else if (strcmp(args[1], "off") == 0) {

        /* Reset the AI */
        autopilot->target = -2;
        autopilot->target_score = 0;
        autopilot->target_update_tick = 0;

        /* Reset this flag since we don't want to be shooting anything */
        if (AssignedTarget(autopilot)) {
            UnassignTarget(autopilot);
        }

        if (Gunning(autopilot))
            DoStopGun(autopilot);

        snprintf(mesg, LBUF_SIZE, "powering down weapons");
        return;

    } else if (strcmp(args[1], "threshold") == 0) {

        /* Ok user specifying a threshold" */
        /* Right now we're only going to allow them to specify a value
         * between 0 and 100 - basicly how much percentage wise over
         * the current value does the new target have to be to switch */
        if (argc == 3 && !Readnum(threshold, args[2]) &&
                threshold >= 0 && threshold <= 100) {

            /* Set the new threshold value */
            autopilot->target_threshold = threshold;

            snprintf(mesg, LBUF_SIZE, "new threshold set to %d%%",
                    threshold);
            return;

        } else {

            /* Bad value for threshold */
            snprintf(mesg, LBUF_SIZE, "!Invalid value used with threshold: "
                    "Usage autogun threshold [0-100]");
            return;

        }

    }

    snprintf(mesg, LBUF_SIZE, "!Invalid Input for autogun:"
            " use 'on' or 'off'");

}

/*
 * Tell the AI to chase whatever its targeting
 */
void auto_radio_command_chasetarg(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg) {

    if (strcmp(args[1], "on") == 0) {

        auto_set_chasetarget_mode(autopilot, AUTO_CHASETARGET_ON);
        snprintf(mesg, LBUF_SIZE, "Chase Target Mode is Activated");
        return;

    } else if (strcmp(args[1], "off") == 0) {

        auto_set_chasetarget_mode(autopilot, AUTO_CHASETARGET_OFF);
        snprintf(mesg, LBUF_SIZE, "Chase Target Mode is Deactivated");
        return;
    }
    snprintf(mesg, LBUF_SIZE, "!Invalid Input for chasetarg: use 'on' or 'off'");

}

/*
 * Radio command to force AI to [dumbly] follow a given target
 */
void auto_radio_command_dfollow(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg) {

    dbref targetref;
    char buffer[SBUF_SIZE];

    targetref = FindTargetDBREFFromMapNumber(mech, args[1]);
    if (targetref <= 0) {
        snprintf(mesg, LBUF_SIZE, "!Invalid target to follow");
        return;
    }

    Clear(autopilot);

    snprintf(buffer, SBUF_SIZE, "dumbfollow %d", targetref);
    auto_addcommand(autopilot->mynum, autopilot, buffer);
    auto_engage(autopilot->mynum, autopilot, "");
    snprintf(mesg, LBUF_SIZE, "following %s [dumbly] (%d degrees, %d away)",
            args[1], autopilot->ofsx, autopilot->ofsy);
}

/*
 * Radio command to force AI to [dumbly] goto a given hex
 */
void auto_radio_command_dgoto(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {
    
    int x, y;
    char buffer[SBUF_SIZE];

    if (Readnum(x, args[1])) {
        snprintf(mesg, LBUF_SIZE, "!First number not an integer");
        return;
    }
    if (Readnum(y, args[2])) {
        snprintf(mesg, LBUF_SIZE, "!First number not an integer");
        return;
    }

    Clear(autopilot);

    snprintf(buffer, SBUF_SIZE, "dumbgoto %d %d", x, y);
    auto_addcommand(autopilot->mynum, autopilot, buffer);
    auto_engage(autopilot->mynum, autopilot, "");
    snprintf(mesg, LBUF_SIZE, "going [dumbly] to %d,%d", x, y);

}

/*
 * Radio command to force AI to drop whatever its carrying
 */
void auto_radio_command_dropoff(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    char buffer[SBUF_SIZE];

    Clear(autopilot);

    snprintf(buffer, SBUF_SIZE, "dropoff");
    auto_addcommand(autopilot->mynum, autopilot, buffer);
    auto_engage(autopilot->mynum, autopilot, "");
    snprintf(mesg, LBUF_SIZE, "dropping off");

}

/*
 * Radio command to force AI to embark a carrier
 */
void auto_radio_command_embark(AUTO* autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    dbref targetref;
    char buffer[SBUF_SIZE];

    targetref = FindTargetDBREFFromMapNumber(mech, args[1]);
    if (targetref <= 0) {
        snprintf(mesg, LBUF_SIZE, "!Invalid target to embark");
        return;
    }

    Clear(autopilot);
    snprintf(buffer, SBUF_SIZE, "embark %d", targetref);
    auto_addcommand(autopilot->mynum, autopilot, buffer);
    auto_engage(autopilot->mynum, autopilot, "");
    snprintf(mesg, LBUF_SIZE, "embarking %s", args[1]);
    return;

}

/*
 * Radio command to force AI to enterbase
 */
void auto_radio_command_enterbase(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    char buffer[SBUF_SIZE];

    if (argc - 1) {
        snprintf(buffer, SBUF_SIZE, "enterbase %s", args[1]);
        snprintf(mesg, LBUF_SIZE, "entering base (%s side)",
                args[1]);
    } else {
        strncpy(buffer, "enterbase n", SBUF_SIZE);
        snprintf(mesg, LBUF_SIZE, "entering base");
    }

    Clear(autopilot);
    auto_addcommand(autopilot->mynum, autopilot, buffer);
    auto_engage(autopilot->mynum, autopilot, "");

}

/*
 * New smart follow system based on A*'s goto
 */
void auto_radio_command_follow(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    char buffer[SBUF_SIZE];
    dbref targetref;

    targetref = FindTargetDBREFFromMapNumber(mech, args[1]);
    if (targetref <= 0) {
        snprintf(mesg, LBUF_SIZE, "!Invalid target to follow");
        return;
    }

    Clear(autopilot);
  
    snprintf(buffer, SBUF_SIZE, "follow %d", targetref);
    auto_addcommand(autopilot->mynum, autopilot, buffer);
    auto_engage(autopilot->mynum, autopilot, "");
    snprintf(mesg, LBUF_SIZE, "following %s (%d degrees, %d away)", args[1], 
            autopilot->ofsx, autopilot->ofsy);

}

/*
 * Smart goto system based on Astar path finding
 */
void auto_radio_command_goto(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {
    
    int x, y;
    char buffer[SBUF_SIZE];

    if (Readnum(x, args[1])) {
        snprintf(mesg, LBUF_SIZE, "!First number not integer");
        return;
    }
    if (Readnum(y, args[2])) {
        snprintf(mesg, LBUF_SIZE, "!Second number not integer");
        return;
    }

    if (MechX(mech) == x && MechY(mech) == y) {
        snprintf(mesg, LBUF_SIZE, "!Already in that hex");
        return;
    }

    Clear(autopilot);

    snprintf(buffer, SBUF_SIZE, "goto %d %d", x, y);
    auto_addcommand(autopilot->mynum, autopilot, buffer);
    auto_engage(autopilot->mynum, autopilot, "");
    snprintf(mesg, LBUF_SIZE, "going to %d,%d", x, y);

}

/*
 * Radio command to alter an AI's heading
 */
void auto_radio_command_heading(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    int heading;
    char buffer[SBUF_SIZE];

    if (Readnum(heading, args[1])) {
        snprintf(mesg, LBUF_SIZE, "!Number not integer");
        return;
    }

    Clear(autopilot);
    auto_engage(autopilot->mynum, autopilot, "");
    snprintf(buffer, SBUF_SIZE, "%d", heading);
    mech_heading(autopilot->mynum, mech, buffer);
    strcpy(buffer, "0");
    mech_speed(autopilot->mynum, mech, buffer);
    snprintf(buffer, LBUF_SIZE, "stopped and heading changed to %d", heading);

}

/*
 * Help message, lists the various commands for the AI
 */
void auto_radio_command_help(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg) {

    int i;

    /*! \todo {Add a short form of this command} */

    snprintf(mesg, LBUF_SIZE, "The following commands are possible:");

    for (i = 0; auto_cmds[i].name; i++) {
        if (i > 0 && !strcmp(auto_cmds[i].name, auto_cmds[i-1].name))
            continue;
        strncat(mesg, " ", LBUF_SIZE);
        strncat(mesg, auto_cmds[i].name, LBUF_SIZE);
    }

    auto_reply(mech, mesg);

}

/*
 * Radio command to force AI to try and hide itself
 */
void auto_radio_command_hide(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    if ((HasCamo(mech)) ? 
            0 : MechType(mech) != CLASS_BSUIT && MechType(mech) != CLASS_MW) {
        snprintf(mesg, LBUF_SIZE, "!Last I checked I was kind of big for that");
        return;
    }

    if (!(MechRTerrain(mech) == HEAVY_FOREST ||
            MechRTerrain(mech) == LIGHT_FOREST ||
            MechRTerrain(mech) == ROUGH ||
            MechRTerrain(mech) == MOUNTAINS ||
            (MechType(mech) == CLASS_BSUIT ?  MechRTerrain(mech) == BUILDING : 0))) {
        snprintf(mesg, LBUF_SIZE, "!Invalid Terrain");
        return;
    }

    bsuit_hide(autopilot->mynum, mech, "");
    snprintf(mesg, LBUF_SIZE, "Begining to hide");

}

/*
 * Radio command to force AI to jump either on a target or 
 * in a given direction range
 */
void auto_radio_command_jumpjet(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {
    
    dbref target;
    char buffer[SBUF_SIZE];
    int bear, rng;

    if (!abs(MechJumpSpeed(mech))) {
        snprintf(mesg, LBUF_SIZE, "!I don't do hiphop and jump around");
        return;
    }

    if ((argc - 1) == 1) {
        if ((target = FindTargetDBREFFromMapNumber(mech, args[1])) <= 0) {
            snprintf(mesg, LBUF_SIZE, "!Unable to see such a target");
            return;
        }
        strcpy(buffer, args[1]);
        mech_jump(autopilot->mynum, mech, buffer);
        snprintf(mesg, LBUF_SIZE, "jumping on [%s]", args[1]);
        return;
    } else {
        if (Readnum(bear, args[1])) {
            snprintf(mesg, LBUF_SIZE, "!Invalid bearing");
            return;
        }
        if (Readnum(rng, args[2])) {
            snprintf(mesg, LBUF_SIZE, "!Invalid range");
            return;
        }
        snprintf(buffer, SBUF_SIZE, "%s %s", args[1], args[2]);
        mech_jump(autopilot->mynum, mech, buffer);
        snprintf(mesg, LBUF_SIZE, "jump %s degrees %s hexes", args[1], args[2]);
        return;
    }
}

/*
 * Radio command to force AI to leavebase
 */
void auto_radio_command_leavebase(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    char buffer[SBUF_SIZE];
    int direction;

    if (Readnum(direction, args[1])) {
        snprintf(mesg, LBUF_SIZE, "!Invalid value for direction");
        return;
    }

    /* Make sure chasetarget doesn't interfere with this */
    if (ChasingTarget(autopilot)) {
        StopChasingTarget(autopilot);
    }

    Clear(autopilot);

    snprintf(buffer, SBUF_SIZE, "leavebase %d", direction);
    auto_addcommand(autopilot->mynum, autopilot, buffer);
    auto_engage(autopilot->mynum, autopilot, ""); 
    snprintf(mesg, LBUF_SIZE, "leaving base at %d heading", direction);

}

/*
 * Old goto system - will phase out
 */
void auto_radio_command_ogoto(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {
    
    int x, y;
    char buffer[SBUF_SIZE];

    if (Readnum(x, args[1])) {
        snprintf(mesg, LBUF_SIZE, "!First number not integer");
        return;
    }
    if (Readnum(y, args[2])) {
        snprintf(mesg, LBUF_SIZE, "!Second number not integer");
        return;
    }

    Clear(autopilot);

    snprintf(buffer, SBUF_SIZE, "oldgoto %d %d", x, y);
    auto_addcommand(autopilot->mynum, autopilot, buffer);
    auto_engage(autopilot->mynum, autopilot, "");
    snprintf(mesg, LBUF_SIZE, "going [old version] to %d,%d", x, y);

}

/*
 * Radio command to force AI to pickup a target
 */
void auto_radio_command_pickup(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    dbref targetref;
    char buffer[SBUF_SIZE];

    targetref = FindTargetDBREFFromMapNumber(mech, args[1]);
    if (targetref <= 0) {
        snprintf(mesg, LBUF_SIZE, "!Invalid target to pickup");
        return;
    }

    /* Make sure chasetarget doesn't interfere with this */
    if (ChasingTarget(autopilot)) {
        StopChasingTarget(autopilot);
    }

    Clear(autopilot);

    snprintf(buffer, SBUF_SIZE, "pickup %d", targetref);
    auto_addcommand(autopilot->mynum, autopilot, buffer);
    auto_engage(autopilot->mynum, autopilot, "");
    snprintf(mesg, LBUF_SIZE, "picking up %s", args[1]);

}

/*
 * Radio command to make AI take up a given position (dir & range) from
 * their current target (hex or unit)
 */
void auto_radio_command_position(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    int x, y;

    /*! \todo {Add in some checks for validity of the arguments} */

    if (Readnum(x, args[1])) {
        snprintf(mesg, LBUF_SIZE, "!Invalid first int");
        return;
    }
    if (Readnum(y, args[2])) {
        snprintf(mesg, LBUF_SIZE, "!Invalide second int");
        return;
    }

    autopilot->ofsx = x;
    autopilot->ofsy = y;
    snprintf(mesg, LBUF_SIZE, "following %d degrees, %d away", x, y);

}

/*
 * Radio command to force AI to go prone
 */
void auto_radio_command_prone(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    mech_drop(autopilot->mynum, mech, "");
    snprintf(mesg, LBUF_SIZE, "hitting the deck");

}

/*
 * Radio command so the AI can report its status
 */
/*! \todo {Add something that tells more info then this} */
void auto_radio_command_report(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    char buffer[MBUF_SIZE];
    MECH *target;

    /* Is the AI moving or something */
    if (Jumping(mech))
        strcpy(buffer, "Jumping");
    else if (Fallen(mech))
        strcpy(buffer, "Prone");
    else if (IsRunning(MechSpeed(mech), MMaxSpeed(mech)))
        strcpy(buffer, "Running");
    else if (MechSpeed(mech) > 1.0)
        strcpy(buffer, "Walking");
    else
        strcpy(buffer, "Standing");

    snprintf(mesg, LBUF_SIZE, "%s at %d, %d", buffer, MechX(mech), MechY(mech));

    /* Which way is the AI going */
    if (MechSpeed(mech) > 1.0) {
        snprintf(buffer, MBUF_SIZE, ", headed %d speed %.2f",
                MechFacing(mech), MechSpeed(mech));
        strncat(mesg, buffer, LBUF_SIZE);
    } else {
        snprintf(buffer, MBUF_SIZE, ", headed %d",
                MechFacing(mech));
        strncat(mesg, buffer, LBUF_SIZE);
    }

    /* Is the AI targeting something */
    if (MechTarget(mech) != -1) {
        target = getMech(MechTarget(mech));

        if (target) {
            snprintf(buffer, MBUF_SIZE, ", targeting %s %s",
                    GetMechToMechID(mech, target),
                    InLineOfSight(mech, target, MechX(target),
                        MechY(target), FaMechRange(mech, target)) ?
                    "" : "(not in LOS)");
            strncat(mesg, buffer, LBUF_SIZE);
        }
    }
    
    /* Send the mesg to the reply system, this is a silent command */
    auto_reply(mech, mesg);

}

/*
 * Radio command to reset the AI's internal flags what not
 */
void auto_radio_command_reset(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    char buffer[SBUF_SIZE];

    auto_disengage(autopilot->mynum, autopilot, "");
    auto_delcommand(autopilot->mynum, autopilot, "-1");
    auto_init(autopilot, mech);
    auto_engage(autopilot->mynum, autopilot, "");
    snprintf(mesg, LBUF_SIZE, "all internal events and flags reset!");

}

/*
 * Radio command to alter or let the AI alter
 * its sensors
 */
void auto_radio_command_sensor(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    char buf[SBUF_SIZE];

    /* Make sure no sensor event running */
    muxevent_remove_type_data(EVENT_AUTO_SENSOR, autopilot);

    if ((argc - 1) == 2) {

        /* Set the user specified sensors */
        snprintf(buf, SBUF_SIZE, "%s %s", args[1], args[2]);
        mech_sensor(autopilot->mynum, mech, buf);
        autopilot->flags |= AUTOPILOT_LSENS;
        snprintf(mesg, LBUF_SIZE, "updated my sensors");
        return;

    }

    /* Let AI decide */
    autopilot->flags &= ~AUTOPILOT_LSENS;
    UpdateAutoSensor(autopilot, 0);
    snprintf(mesg, LBUF_SIZE, "using my own judgement with sensors");
    return;

}

/*
 * Radio command to force AI to shutdown
 */
void auto_radio_command_shutdown(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg) {

    mech_shutdown(autopilot->mynum, mech, "");
    snprintf(mesg, LBUF_SIZE, "shutting down");

}

/*
 * Radio command to alter the speed of an AI (% of speed)
 */
void auto_radio_command_speed(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    int speed = 100;

    if (Readnum(speed, args[1])) {
        snprintf(mesg, LBUF_SIZE, "!Invalid value - not a number");
        return;
    }

    if (speed < 1 || speed > 100) {
        snprintf(mesg, LBUF_SIZE, "!Invalid speed");
        return;
    }

    autopilot->speed = speed;
    snprintf(mesg, LBUF_SIZE, "setting speed to %d %%", speed);

}

/*
 * Radio Command to force AI to stand
 */
void auto_radio_command_stand(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    mech_stand(autopilot->mynum, mech, "");
    snprintf(mesg, LBUF_SIZE, "standing up");

}

/*
 * Radio command to force AI to startup
 */
void auto_radio_command_startup(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    if (argc > 1) {
        if (!strncasecmp(args[1], "override", strlen(args[1]))) {
            mech_startup(autopilot->mynum, mech, "override");
            snprintf(mesg, LBUF_SIZE, "emergency override startup triggered");
            return;
        }
    }

    mech_startup(autopilot->mynum, mech, "");
    snprintf(mesg, LBUF_SIZE, "starting up");

}

/*
 * Radio command to stop the AI
 */
void auto_radio_command_stop(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    char buffer[SBUF_SIZE];

    strcpy(buffer, "0");

    /* Turn chasetarget off */
    auto_set_chasetarget_mode(autopilot, AUTO_CHASETARGET_OFF);

    Clear(autopilot);

    auto_engage(autopilot->mynum, autopilot, "");
    mech_speed(autopilot->mynum, mech, buffer);
    snprintf(mesg, LBUF_SIZE, "halting");

}

/*
 * Command for the old goto, will phase it out
 */
void auto_radio_command_sweight(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    int x, y;

    if (Readnum(x, args[1])) {
        snprintf(mesg, LBUF_SIZE, "!Invalid first int");
        return;
    }
    if (Readnum(y, args[2])) {
        snprintf(mesg, LBUF_SIZE, "!Invalide second int");
        return;
    }
    x = MAX(1, x);
    y = MAX(1, y);
    autopilot->auto_goweight = x;
    autopilot->auto_fweight = y;
    snprintf(mesg, LBUF_SIZE, "sweight'ed to %d:%d. (go:fight)", x, y);
    return;

}

/*
 * Tell the AI to target a specific unit
 */
void auto_radio_command_target(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg) {

    dbref targetref;

    if (!strcmp(args[1], "-")) {
       
        /* Basicly doing the same as 'autogun on' */
        autopilot->target = -1;
        autopilot->target_score = 0;
        autopilot->target_update_tick = AUTO_GUN_UPDATE_TICK;

        if (AssignedTarget(autopilot)) {
            UnassignTarget(autopilot);
        }

        if (Gunning(autopilot)) {
            DoStopGun(autopilot);
        }
        DoStartGun(autopilot);

        snprintf(mesg, LBUF_SIZE, "shooting at whatever I want");
        return;

    } else {

        targetref = FindTargetDBREFFromMapNumber(mech, args[1]);
        if (targetref <= 0) {
            snprintf(mesg, LBUF_SIZE, "!Unable to see such a target");
            return;
        }

    }

    autopilot->target = targetref;
    autopilot->target_score = 0;
    autopilot->target_update_tick = 0;

    /* Let the AI know its an assigned target */
    if(!AssignedTarget(autopilot)) {
        AssignTarget(autopilot);
    }
    
    if (Gunning(autopilot)) {
        DoStopGun(autopilot);
    }
    DoStartGun(autopilot);

    snprintf(mesg, LBUF_SIZE, "aiming for [%s] (and ignoring everyone else)",
            args[1]);

}

#if 0
ACMD(auto_swarm)
{
    dbref targetref;

    if (!mech || MechType(mech) != CLASS_BSUIT)
    return "!Not a bsuit";
    targetref = FindTargetDBREFFromMapNumber(mech, args[0]);
    if (targetref <= 0)
        return "!Invalid target to swarm"; 
    Clear(a);
    auto_addcommand(a->mynum, a, tprintf("swarm %d", targetref));
    auto_engage(a->mynum, a, "");
    return tprintf("swarming %s.", args[0]); 
}
#endif
#if 0
ACMD(auto_attackleg)
{
    dbref targetref;

    if (!mech || MechType(mech) != CLASS_BSUIT)
    return "!Not a bsuit";
    targetref = FindTargetDBREFFromMapNumber(mech, args[0]);
    if (targetref <= 0)
        return "!Invalid target to attackleg"; 
    Clear(a);
    auto_addcommand(a->mynum, a, tprintf("attackleg %d", targetref));
    auto_engage(a->mynum, a, "");
    return tprintf("attacklegging %s.", args[0]); 
}
#endif
#if 0
ACMD(auto_notarget)
{
    a->targ = -1;
    if (Gunning(a)) {
        DoStopGun(a);
        DoStartGun(a);
    }
    return "shooting at anything that moves";
}
#endif
#if 0
ACMD(auto_nogun)
{
    a->targ = -2;
    if (Gunning(a))
        DoStopGun(a);
    return "powering down weapons";
}
#endif
#if 0
ACMD(auto_rally)
{
    char xb[6], yb[6];
    int x, y, h;

    if (Readnum(x, args[0]))
        return "!Invalid X coord";
    if (Readnum(y, args[1]))
        return "!Invalid Y coord";
    if (argc < 3)
        h = MechFacing(mech);
    else if (Readnum(h, args[2]))
        return "!Invalid heading";
    /* Base things on the <h = heading> */
    x = x + a->ofsy * cos(TWOPIOVER360 * (270 + (a->ofsx + h)));
    y = y + a->ofsy * sin(TWOPIOVER360 * (270 + (a->ofsx + h)));
    sprintf(xb, "%d", x);
    sprintf(yb, "%d", y);
    args[0] = xb;
    args[1] = yb;
    return auto_goto(a, mech, args, 2, chn);
}
#endif
#if 0
ACMD(auto_drally)
{
    char xb[6], yb[6];
    int x, y, h;

    if (Readnum(x, args[0]))
        return "!Invalid X coord";
    if (Readnum(y, args[1]))
        return "!Invalid Y coord";
    if (argc < 3)
        h = MechFacing(mech);
    else if (Readnum(h, args[2]))
        return "!Invalid heading";
    /* Base things on the <h = heading> */
    x = x + a->ofsy * cos(TWOPIOVER360 * (270 + (a->ofsx + h)));
    y = y + a->ofsy * sin(TWOPIOVER360 * (270 + (a->ofsx + h)));
    sprintf(xb, "%d", x);
    sprintf(yb, "%d", y);
    args[0] = xb;
    args[1] = yb;
    return auto_dgoto(a, mech, args, 2, chn);
}
#endif
#if 0
ACMD(auto_roammode)
{
    if (strcmp(args[0], "on") == 0) {
    Clear(a);
    auto_addcommand(a->mynum, a, tprintf("roammode 1"));
    auto_engage(a->mynum, a, "");
        return "Roam mode is ON";
        } else if (strcmp(args[0], "off") == 0) {
    Clear(a);
    auto_addcommand(a->mynum, a, tprintf("roammode 0"));
    auto_engage(a->mynum, a, "");
        return "Roam mode is OFF";
        }
    return "!Invalid input use on or off";
}
#endif
#if 0
ACMD(auto_swarmmode)
{
    if (MechType(mech) != CLASS_BSUIT)
    return "!I am not a battlesuit";

    if (strcmp(args[0], "on") == 0) {
        a->flags |= AUTOPILOT_SWARMCHARGE;
        return "Swarm Mode is ON";
        } else if (strcmp(args[0], "off") == 0) {
        a->flags &= ~AUTOPILOT_SWARMCHARGE;
        return "Swarm Mode is OFF";
        }
    return "!Invalid input use on or off";
}
#endif
#if 0
ACMD(auto_swarmcharge)
{
    dbref targetref;

    if (MechType(mech) != CLASS_BSUIT)
    return "!I am not a battlesuit";

    if (strcmp(args[0], "cancel") == 0) {
    a->flags &= ~AUTOPILOT_SWARMCHARGE;
    return "Canceling swarm charge";
    }

    targetref = FindTargetDBREFFromMapNumber(mech, args[0]);
    if (targetref <= 0)
        return "!Invalid target to swarmcharge";
    Clear(a);
    auto_addcommand(a->mynum, a, tprintf("dumbfollow %d", targetref));
    auto_engage(a->mynum, a, "");
    a->flags |= AUTOPILOT_SWARMCHARGE; 
    return tprintf("swarmcharging %s (%d degrees, %d away)", args[0],
        a->ofsx, a->ofsy);
}
#endif
#if 0
ACMD(auto_freq)
{
    int freq;

    if (Readnum(freq, args[0]))
        return "!Invalid freq";
    mech_set_channelfreq(a->mynum, mech, tprintf("a=%s", args[0]));
    return "channel changed";
}
#endif
#if 0
ACMD(auto_setchanfreq)
{
    mech_set_channelfreq(a->mynum, mech, tprintf("%s=%s", args[0],
        args[1]));
    return tprintf("set channelfreq %s=%s", args[0], args[1]);
}
#endif
#if 0
ACMD(auto_setchanmode)
{
    mech_set_channelmode(a->mynum, mech, tprintf("%s=%s", args[0],
        args[1]));
    return tprintf("set channelmode %s=%s", args[0], args[1]);
}
#endif
#if 0
ACMD(auto_enterbay)
{
    mech_enterbay(a->mynum, mech, argc ? tprintf("%s",
            args[0]) : tprintf(""));
    return argc ? tprintf("entering bay (of %s)",
            args[0]) : "entering bay";
}
#endif
#if 0
ACMD(auto_cmode)
{
    int mod, ran;
    static char buf[MBUF_SIZE];

    if (Readnum(mod, args[0]))
        return "!Invalid mode [0-2]";
    if (mod < 0 || mod > 2)
        return "!Invalid mode [0-2]";
    if (Readnum(ran, args[1]))
        return "!Invalid range [0-999]";
    if (ran < 0 || ran > 999)
        return "!Invalid range [0-999]";
    a->auto_cdist = ran;
    a->auto_cmode = mod;
    switch (mod) {
    case 0:
        sprintf(buf, "fleeing, at least to range %d {from all foes}", ran);
        break;
    case 1:
        sprintf(buf, "trying to maintain range %d {from all foes}", ran);
        break;
    case 2:
        sprintf(buf, "charging to range %d {from all foes}", ran);
        break;
    }
    return buf;
}
#endif

/*
 * Event to get AI to radio a message
 */
void auto_reply_event(MUXEVENT *muxevent) {

    MECH *mech = (MECH *) muxevent->data;
    char *buf = (char *) muxevent->data2;
    MAP *map;

    /* Make sure its a mech */
    if (!IsMech(mech->mynum)) {
        free(buf);
        return;
    }

    /* If valid object */
    if (mech)
        if ((map = (getMap(mech->mapindex))))
            sendchannelstuff(mech, 0, buf);

    free(buf);
}

/*
 * Force the AI to reply over radio
 */
void auto_reply(MECH *mech, char *buf) {

    char *reply;

    /* No zero freq messages */
    if (!mech->freq[0])
        return;

    /* Make sure there is an autopilot */
    if (MechAuto(mech) <= 0)
        return;

    /* Make sure valid objects */
    if (!(FindObjectsData(MechAuto(mech))) || 
            !Good_obj(MechAuto(mech)) || 
            Location(MechAuto(mech)) != mech->mynum) {
        MechAuto(mech) = -1;
        return;
    }

    /* Copy the buffer */
    reply = strdup(buf); 

    if (reply) {
        MECHEVENT(mech, EVENT_AUTO_REPLY, auto_reply_event, Number(1, 2), reply);
    } else {
        SendAI("Interal AI Error: Attempting to radio reply but unable to copy string");
    }

}

/*
 * Parse an AI radio command
 */
void auto_parse_command(AUTO *autopilot, MECH *mech, int chn, char *buffer) {
    
    int argc, cmd;
    char *args[2];
    char *command_args[AUTOPILOT_MAX_ARGS];
    char mech_id[3];
    char message[LBUF_SIZE];
    char reply[LBUF_SIZE];
    int i;

    /* Basic checks */
    if (!autopilot || !mech)
        return;
    if (Destroyed(mech))
        return;

    /* Get the args - just need the first one */
    if (proper_explodearguments(buffer, args, 2) < 2) {
        /* free args */
        for(i = 0; i < 2; i++) {
            if(args[i]) free(args[i]);
        }
        return;
    }

    /* Check to see if the command was given to this AI */
    if (strcmp(args[0], "all")) {
        mech_id[0] = MechID(mech)[0];
        mech_id[1] = MechID(mech)[1];
        mech_id[2] = '\0';

        if (strcasecmp(mech_id, args[0])) {
            /* free args */
            for(i = 0; i < 2; i++) {
                if(args[i]) free(args[i]);
            }
            return;
        }

    }

    /* Parse the command */
    cmd = -1;
    argc = proper_explodearguments(args[1], command_args, AUTOPILOT_MAX_ARGS);

    /* Loop through the various possible commands looking for ours */
    for (i = 0; auto_cmds[i].sho; i++) {
        if (!strncmp(auto_cmds[i].sho, command_args[0], strlen(auto_cmds[i].sho)))
            if (!strncmp(auto_cmds[i].name, command_args[0], strlen(command_args[0]))) {
                if (argc == (auto_cmds[i].args + 1)) {
                    cmd = i;
                    break;
                }
            }
    }

    /* Did we find a command */
    if (cmd < 0) {

        snprintf(message, LBUF_SIZE, "Unable to comprehend the command.");
        auto_reply(mech, message); 

        /* free args */
        for(i = 0; i < 2; i++) {
            if(args[i]) free(args[i]);
        }
        for (i = 0; i < AUTOPILOT_MAX_ARGS; i++) {
            if (command_args[i]) free(command_args[i]);
        }
        return;

    }

    /* Zero the buffer */
    memset(message, 0, sizeof(message));
    memset(reply, 0, sizeof(reply));

    /* Call the radio command function */
    (*(auto_cmds[cmd].fun)) (autopilot, mech, command_args, argc, message);

    /* If its a silent command there is no reply */
    if (auto_cmds[cmd].silent) {

        /* Free args and exit */
        for(i = 0; i < 2; i++) {
            if(args[i]) free(args[i]);
        }
        for (i = 0; i < AUTOPILOT_MAX_ARGS; i++) {
            if (command_args[i]) free(command_args[i]);
        }
        return;

    }

    /* Check to see if a message was returned */
    if (*message) {

        /* Check if there was an error message
         * otherwise add a front and back to the message */
        if (message[0] == '!') {
            snprintf(reply, LBUF_SIZE, "ERROR: %s!", message + 1);
        } else {

            switch (Number(0, 20)) {
                case 0:
                case 1:
                case 2:
                case 4:
                    snprintf(reply, LBUF_SIZE, "%s%s%s", "Affirmative, ",
                            message, ".");
                    break;
                case 5:
                    snprintf(reply, LBUF_SIZE, "%s%s%s", "Nod, ",
                            message, ".");
                    break;
                case 6:
                    snprintf(reply, LBUF_SIZE, "%s%s%s", "Fine, ",
                            message, ".");
                    break;
                case 7:
                    snprintf(reply, LBUF_SIZE, "%s%s%s", "Aye aye, Captain, ",
                            message, "!");
                    break;
                case 8:
                case 9:
                case 10:
                    snprintf(reply, LBUF_SIZE, "%s%s%s", "Da, boss, ",
                            message, "!");
                    break;
                case 11:
                case 12:
                    snprintf(reply, LBUF_SIZE, "%s%s%s", "Ok, ",
                            message, ".");
                    break;
                case 13:
                    snprintf(reply, LBUF_SIZE, "%s%s%s", "Okay, okay, ",
                            message, ", happy now?");
                    break;
                case 14:
                    snprintf(reply, LBUF_SIZE, "%s%s%s", "Okidoki, ",
                            message, "!");
                    break;
                case 15:
                case 16:
                case 17:
                    snprintf(reply, LBUF_SIZE, "%s%s%s", "Aye, ",
                            message, ".");
                    break;
                default:
                    snprintf(reply, LBUF_SIZE, "%s%s%s", "Roger, Roger, ",
                            message, ".");
                    break;
            } /* End of switch */

        } 

        auto_reply(mech, reply);

    } else if (!auto_cmds[cmd].silent) {

        /* Command isn't silent but it didn't return a message */
        snprintf(reply, LBUF_SIZE, "Ok.");
        auto_reply(mech, reply);

    }

    /* free args */
    for(i = 0; i < 2; i++) {
        if(args[i]) free(args[i]);
    }
    for (i = 0; i < AUTOPILOT_MAX_ARGS; i++) {
        if (command_args[i]) free(command_args[i]);
    }

}
