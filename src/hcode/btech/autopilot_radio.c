
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

#define Clear(a) \
    auto_disengage(a->mynum, a, ""); \
    auto_delcommand(a->mynum, a, "-1");
#if 0
    if (a->targ >= -1) auto_addcommand(a->mynum, a, tprintf("autogun"));
#endif

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
#if 0
    "att", "attackleg", 1, 0, auto_attackleg}, {
    "chanf", "chanfreq", 2, 0, auto_setchanfreq}, {
    "chanm", "chanmode", 2, 0, auto_setchanmode}, {
    "chase", "chasetarg", 1, 0, auto_chasetarg}, {
    "cm", "cmode", 2, 0, auto_cmode}, {
#endif
    "dfo", "dfollow", 1, 0, auto_radio_command_dfollow}, {
    "dgo", "dgoto", 2, 0, auto_radio_command_dgoto}, {
#if 0
    "dr", "drally", 2, 0, auto_drally}, {
    "dr", "drally", 3, 0, auto_drally}, {
#endif
    "drop", "dropoff", 0, 0, auto_radio_command_dropoff}, {
#if 0
    "emb", "embark", 1, 0, auto_embark}, {
    "en", "enterbase", 0, 0, auto_enterbase}, {
    "en", "enterbase", 1, 0, auto_enterbase}, {
    "en", "enterbay", 0, 0, auto_enterbay}, {
    "en", "enterbay", 1, 0, auto_enterbay}, {
    "fo", "follow", 1, 0, auto_follow}, {
    "fr", "freq", 1, 0, auto_freq}, {
    "go", "goto", 2, 0, auto_goto}, {
    "he", "heading", 1, 0, auto_heading}, {
#endif
    "he", "help", 0, 1, auto_radio_command_help}, {
#if 0
    "hi", "hide", 0, 0, auto_hide }, {
    "jump", "jumpjet", 1, 0, auto_jumpjet }, {
    "jump", "jumpjet", 2, 0, auto_jumpjet }, {
    "le", "leavebase", 1, 0, auto_leavebase }, {
    "nog", "nogun", 0, 0, auto_nogun}, {
    "not", "notarget", 0, 0, auto_notarget}, {
#endif
    "pick", "pickup", 1, 0, auto_radio_command_pickup}, {
#if 0
    "pr", "prone", 0, 0, auto_prone}, {
    "pos", "position", 2, 0, auto_position}, {
    "ra", "rally", 2, 0, auto_rally}, {
    "ra", "rally", 3, 0, auto_rally}, {
    "re", "report", 0, 1, auto_report}, {
    "reset", "reset", 0, 0, auto_reset}, {
    "roam", "roammode", 1, 0, auto_roammode }, {
    "se", "sensor", 2, 0, auto_sensor}, {
    "se", "sensor", 0, 0, auto_sensor}, {
#endif
    "sh", "shutdown", 0, 0, auto_radio_command_shutdown}, {
#if 0
    "sp", "speed", 1, 0, auto_speed}, {
    "st", "stand", 0, 0, auto_stand}, {
#endif
    "st", "startup", 0, 0, auto_radio_command_startup}, {
    "st", "startup", 1, 0, auto_radio_command_startup}, {
#if 0
    "st", "stop", 0, 0, auto_stop}, {
    "sw", "sweight", 2, 1, auto_sweight}, {
    "swa", "swarm", 1, 0, auto_swarm}, {
    "swarmc", "swarmcharge", 1, 0, auto_swarmcharge }, {
    "swarmm", "swarmmode", 1, 0, auto_swarmmode }, {
    "ta", "target", 1, 0, auto_target}, {
    "ta", "target", 2, 0, auto_target}, {
#endif
    NULL, NULL, 0, 0, NULL}
};

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
 * Help message, lists the various commands for the AI
 */
void auto_radio_command_help(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg) {

    int i;

    snprintf(mesg, LBUF_SIZE, "The following commands are possible:");

    for (i = 0; auto_cmds[i].name; i++) {
        if (i > 0 && !strcmp(auto_cmds[i].name, auto_cmds[i-1].name))
            continue;
        snprintf(mesg, LBUF_SIZE, "%s %s", mesg, auto_cmds[i].name);
    }

    auto_reply(mech, mesg);

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

    Clear(autopilot);
    snprintf(buffer, SBUF_SIZE, "pickup %d", targetref);
    auto_addcommand(autopilot->mynum, autopilot, buffer);
    auto_engage(autopilot->mynum, autopilot, "");
    snprintf(mesg, LBUF_SIZE, "picking up %s", args[1]);

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

#if 0
/* Smart (?) goto command for AI */
ACMD(auto_goto)
{
    int x, y;

    if (Readnum(x, args[0]))
        return tprintf("!First number not integer");
    if (Readnum(y, args[1]))
        return tprintf("!Second number not integer");
    Clear(a);
    auto_addcommand(a->mynum, a, tprintf("goto %d %d", x, y));
    auto_engage(a->mynum, a, "");
    return tprintf("going to %d,%d", x, y);
}
#endif

#if 0
/* Smart (?) follow command for AI */
ACMD(auto_follow)
{
    dbref targetref;

    targetref = FindTargetDBREFFromMapNumber(mech, args[0]);
    if (targetref <= 0)
        return "!Invalid target to follow";
    Clear(a);
    auto_addcommand(a->mynum, a, tprintf("follow %d", targetref));
    auto_engage(a->mynum, a, "");
    return tprintf("following %s (%d degrees, %d away)", args[0], a->ofsx,
            a->ofsy);
}
#endif
#if 0
/* Tell AI to embark a carrier */
ACMD(auto_embark)
{
    dbref targetref;

    targetref = FindTargetDBREFFromMapNumber(mech, args[0]);
    if (targetref <= 0)
        return "!Invalid target to embark";
    Clear(a);
    auto_addcommand(a->mynum, a, tprintf("embark %d", targetref));
    auto_engage(a->mynum, a, "");
    return tprintf("embarking %s", args[0]); 
}
#endif
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
ACMD(auto_speed)
{
    int speed = 100;

    speed = atoi(args[0]);
    if (speed < 1 || speed > 100)
        return "!Invalid speed";
    a->speed = speed;
    return tprintf("setting speed to %d %%.", atoi(args[0]));
}
#endif
#if 0
ACMD(auto_hide)
{

    if ((HasCamo(mech)) ? 
            0 : MechType(mech) != CLASS_BSUIT && MechType(mech) != CLASS_MW) {
        return "!Last I checked I was kind of big for that";
    }

    if (!(MechRTerrain(mech) == HEAVY_FOREST ||
            MechRTerrain(mech) == LIGHT_FOREST ||
            MechRTerrain(mech) == ROUGH ||
            MechRTerrain(mech) == MOUNTAINS ||
            (MechType(mech) == CLASS_BSUIT ?  MechRTerrain(mech) == BUILDING : 0))) {
        return "!Invalid Terrain";
    }

    bsuit_hide(a->mynum, mech, "");
    return "Begining to hide";
}
#endif
#if 0
ACMD(auto_reset)
{
    Clear(a);
    ai_init(a, a->mymech);
    auto_engage(a->mynum, a, "");

    return tprintf("all internal events and flags reset!");
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
ACMD(auto_heading)
{
    int i;
    char buf[3];

    if (Readnum(i, args[0]))
        return tprintf("!Number not integer");
    Clear(a);
    auto_engage(a->mynum, a, "");
    strcpy(buf, "0");
    mech_heading(a->mynum, mech, tprintf("%d", i));
    mech_speed(a->mynum, mech, buf);
    return tprintf("stopped and heading changed to %d", i);
}
#endif
#if 0
ACMD(auto_stop)
{

    char buf[5];

    strcpy(buf, "0");
    Clear(a);
    auto_engage(a->mynum, a, "");
    mech_speed(a->mynum, mech, buf);
    return "halting";
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
ACMD(auto_position)
{
    int x, y;

    if (Readnum(x, args[0]))
        return "!Invalid first int";
    if (Readnum(y, args[1]))
        return "!Invalid second int";
    a->ofsx = x;
    a->ofsy = y;
    return tprintf("following %d degrees, %d away", a->ofsx, a->ofsy);
}
#endif
#if 0
ACMD(auto_sweight)
{
    int x, y;

    if (Readnum(x, args[0]))
        return "!Invalid first int";
    if (Readnum(y, args[1]))
        return "!Invalid second int";
    x = MAX(1, x);
    y = MAX(1, y);
    a->auto_goweight = x;
    a->auto_fweight = y;
    return tprintf("sweight'ed to %d:%d. (go:fight)", x, y);
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
ACMD(auto_sensor)
{
    if (argc) {
        /* Alter sensors */
        char buf[LBUF_SIZE];

        sprintf(buf, "%s %s", args[0], args[1]);
        mech_sensor(a->mynum, mech, buf);
        a->flags |= AUTOPILOT_LSENS;
        return "updated my sensors";
    }
    a->flags &= ~AUTOPILOT_LSENS;
    UpdateAutoSensor(a);
    return "using my own judgement with sensors";
}
#endif
#if 0
ACMD(auto_chasetarg)
{
    if (strcmp(args[0], "on") == 0) {
        a->flags |= AUTOPILOT_CHASETARG;
        return "Chase Target mode is ON";
    } else if (strcmp(args[0], "off") == 0) {
        a->flags &= ~AUTOPILOT_CHASETARG;
        return "Chase Target mode is OFF";
    }
    return "!Invalid input use on or off";
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
ACMD(auto_target)
{
    dbref targetref;

    if (!strcmp(args[0], "-")) {
        a->targ = -1;
        if (Gunning(a))
            DoStopGun(a);
        DoStartGun(a);
        return "aiming for nobody in particular";
    } else {
        targetref = FindTargetDBREFFromMapNumber(mech, args[0]);
        if (targetref <= 0)
            return "!Unable to see such a target";
    }
    a->targ = targetref;
    if (Gunning(a))
        DoStopGun(a);
    DoStartGun(a);
    return tprintf("aiming for [%s] (and ignoring everyone else)",
            args[0]);
}
#endif
#if 0
ACMD(auto_jumpjet)
{
    dbref targetref;
    char buf[50];
    int bear, rng;

    if (!abs(MechJumpSpeed(mech)))
        return "!I don't do hiphop and jump around";

    if (argc == 1) {
        if ((targetref = FindTargetDBREFFromMapNumber(mech, args[0])) <= 0) {
            return "!Unable to see such a target";
        }
        mech_jump(a->mynum, mech, args[0]);
        return tprintf("jumping on [%s]", args[0]);
    } else {
        if (Readnum(bear, args[0]))
            return "!Invalid bearing";
        if (Readnum(rng, args[1]))
            return "!Invalid range";
        snprintf(buf, sizeof(char) * 50, "%s %s", args[0], args[1]);
        mech_jump(a->mynum, mech, buf);
        return tprintf("jump %s degrees %s hexes", args[0], args[1]);
    }
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
ACMD(auto_report)
{
    static char buf[MBUF_SIZE];

    if (Jumping(mech))
        strcpy(buf, "Jumping");
    else if (Fallen(mech))
        strcpy(buf, "Prone");
    else if (IsRunning(MechSpeed(mech), MMaxSpeed(mech)))
        strcpy(buf, "Running");
    else if (MechSpeed(mech) > 1.0)
        strcpy(buf, "Walking");
    else
        strcpy(buf, "Standing");

    strcat(buf, tprintf(" at %d, %d", MechX(mech), MechY(mech)));
    if (MechSpeed(mech) > 1.0)
        strcat(buf, tprintf(", headed %d speed %.2f", MechFacing(mech),
            MechSpeed(mech)));
    else
        strcat(buf, tprintf(", headed %d", MechFacing(mech)));
    if (MechTarget(mech) != -1) {
        MECH *tempMech = getMech(MechTarget(mech));

        if (tempMech)
            strcat(buf, tprintf(", targetting %s %s", GetMechToMechID(mech,
                    tempMech), InLineOfSight(mech, tempMech,
                    MechX(tempMech), MechY(tempMech), FaMechRange(mech,
                    tempMech)) ? "" : "(not in LOS)"));
    }
    auto_reply(mech, buf);
    return NULL;
}
#endif

#if 0
ACMD(auto_enterbase)
{
    mech_enterbase(a->mynum, mech, argc ? tprintf("%s",
            args[0]) : tprintf(""));
    return argc ? tprintf("entering base (%s side)",
            args[0]) : "entering base";
}
#endif
#if 0
ACMD(auto_leavebase)
{ 
    Clear(a);
    auto_addcommand(a->mynum, a, tprintf("leavebase %d", atoi(args[0])));
    auto_engage(a->mynum, a, ""); 
    return tprintf("leaving base at %d heading", atoi(args[0]));
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
ACMD(auto_prone)
{
    mech_drop(a->mynum, mech, tprintf(""));
    return "hitting the deck";
}
#endif
#if 0
ACMD(auto_stand)
{
    mech_stand(a->mynum, mech, tprintf(""));
    return "standing up";
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
