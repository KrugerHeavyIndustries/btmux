
/*
 * $Id: autopilot_command.c,v 1.1 2005/06/13 20:50:49 murrayma Exp $
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

void sendchannelstuff(MECH * mech, int freq, char *msg);

#define Clear(a) \
  auto_disengage(a->mynum, a, ""); \
  auto_delcommand(a->mynum, a, "-1"); \
  PG(a)=0; \
  if (a->targ >= -1) auto_addcommand(a->mynum, a, tprintf("autogun"));


#define BACMD(name) char * name (AUTO *a, MECH *mech, char ** args, int argc, int chn)
#define ACMD(name) static BACMD(name)

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

ACMD(auto_dgoto)
{
    int x, y;

    if (Readnum(x, args[0]))
	return tprintf("!First number not integer");
    if (Readnum(y, args[1]))
	return tprintf("!Second number not integer");
    Clear(a);
    auto_addcommand(a->mynum, a, tprintf("dumbgoto %d %d", x, y));
    auto_engage(a->mynum, a, "");
    return tprintf("going [dumbly] to %d,%d", x, y);
}

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

ACMD(auto_dfollow)
{
    dbref targetref;

    targetref = FindTargetDBREFFromMapNumber(mech, args[0]);
    if (targetref <= 0)
	return "!Invalid target to follow";
    Clear(a);
    auto_addcommand(a->mynum, a, tprintf("dumbfollow %d", targetref));
    auto_engage(a->mynum, a, "");
    return tprintf("following %s [dumbly] (%d degrees, %d away)", args[0],
	a->ofsx, a->ofsy);
}

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

ACMD(auto_stop)
{

    char buf[5];

    strcpy(buf, "0");
    Clear(a);
    auto_engage(a->mynum, a, "");
    mech_speed(a->mynum, mech, buf);
    return "halting";
}

ACMD(auto_notarget)
{
    a->targ = -1;
    if (Gunning(a)) {
	DoStopGun(a);
	DoStartGun(a);
    }
    return "shooting at anything that moves";
}

ACMD(auto_nogun)
{
    a->targ = -2;
    if (Gunning(a))
	DoStopGun(a);
    return "powering down weapons";
}


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
    return NULL;
}

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

ACMD(auto_freq)
{
    int freq;

    if (Readnum(freq, args[0]))
	return "!Invalid freq";
    mech_set_channelfreq(a->mynum, mech, tprintf("a=%s", args[0]));
    return "channel changed";
}

ACMD(auto_setchanfreq)
{
    mech_set_channelfreq(a->mynum, mech, tprintf("%s=%s", args[0],
	    args[1]));
    return tprintf("set channelfreq %s=%s", args[0], args[1]);
}

ACMD(auto_setchanmode)
{
    mech_set_channelmode(a->mynum, mech, tprintf("%s=%s", args[0],
	    args[1]));
    return tprintf("set channelmode %s=%s", args[0], args[1]);
}

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
    auto_replyA(mech, buf);
    return NULL;
}

ACMD(auto_shutdown)
{
    mech_shutdown(a->mynum, mech, tprintf(""));
    return "shutting down";
}

ACMD(auto_startup)
{
    mech_startup(a->mynum, mech, argc ? tprintf("override") : tprintf(""));
    return argc ? "emergency override startup triggered" : "starting up";
}

ACMD(auto_enterbase)
{
    mech_enterbase(a->mynum, mech, argc ? tprintf("%s",
	    args[0]) : tprintf(""));
    return argc ? tprintf("entering base (%s side)",
	args[0]) : "entering base";
}

ACMD(auto_enterbay)
{
    mech_enterbay(a->mynum, mech, argc ? tprintf("%s",
	    args[0]) : tprintf(""));
    return argc ? tprintf("entering bay (of %s)",
	args[0]) : "entering bay";
}

ACMD(auto_prone)
{
    mech_drop(a->mynum, mech, tprintf(""));
    return "hitting the deck";
}

ACMD(auto_stand)
{
    mech_stand(a->mynum, mech, tprintf(""));
    return "standing up";
}

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

ACMD(auto_help)
{
    auto_replyA(mech, "The following commands are possible:  "
	"cmode dfollow follow freq dgoto goto heading nogun notarget"
	" position rally drally sensor sweight stop target report"
	" chanfreq chanmode startup prone stand shutdown enterbase"
	" enterbay help");
    return NULL;
}


struct {
    char *sho;
    char *name;
    int args;
    int silent;
     BACMD((*fun));
} auto_cmds[] = {
    {
    "cm", "cmode", 2, 0, auto_cmode}, {
    "dfo", "dfollow", 1, 0, auto_dfollow}, {
    "fo", "follow", 1, 0, auto_follow}, {
    "fr", "freq", 1, 0, auto_freq}, {
    "dgo", "dgoto", 2, 0, auto_dgoto}, {
    "go", "goto", 2, 0, auto_goto}, {
    "he", "heading", 1, 0, auto_heading}, {
    "nog", "nogun", 0, 0, auto_nogun}, {
    "not", "notarget", 0, 0, auto_notarget}, {
    "pos", "position", 2, 0, auto_position}, {
    "ra", "rally", 2, 0, auto_rally}, {
    "ra", "rally", 3, 0, auto_rally}, {
    "dr", "drally", 2, 0, auto_drally}, {
    "dr", "drally", 3, 0, auto_drally}, {
    "se", "sensor", 2, 0, auto_sensor}, {
    "se", "sensor", 0, 0, auto_sensor}, {
    "sw", "sweight", 2, 1, auto_sweight}, {
    "st", "stop", 0, 0, auto_stop}, {
    "ta", "target", 1, 0, auto_target}, {
    "ta", "target", 2, 0, auto_target}, {
    "re", "report", 0, 1, auto_report}, {
    "chanf", "chanfreq", 2, 0, auto_setchanfreq}, {
    "chanm", "chanmode", 2, 0, auto_setchanmode}, {
    "he", "help", 0, 1, auto_help}, {
    "st", "startup", 0, 0, auto_startup}, {
    "st", "startup", 1, 0, auto_startup}, {
    "sh", "shutdown", 0, 0, auto_shutdown}, {
    "en", "enterbase", 0, 0, auto_enterbase}, {
    "en", "enterbase", 1, 0, auto_enterbase}, {
    "en", "enterbay", 0, 0, auto_enterbay}, {
    "en", "enterbay", 1, 0, auto_enterbay}, {
    "pr", "prone", 0, 0, auto_prone}, {
    "st", "stand", 0, 0, auto_stand}, {
    NULL, NULL, 0, 0, NULL}
};				/* ALSO UPDATE THE HELP TEXT (ABOVE!) */

int auto_parse_command_sub(AUTO * a, MECH * mech, char *buffer,
    char ***gargs, int *argc_n)
{
    static char *args[5];
    int argc = mech_parseattributes(buffer, args, 5);
    int i;

    *argc_n = argc;
    for (i = 0; auto_cmds[i].sho; i++)
	if (!strncmp(auto_cmds[i].sho, args[0], strlen(auto_cmds[i].sho)))
	    if (!strncmp(auto_cmds[i].name, args[0], strlen(args[0]))) {
		if (argc != (auto_cmds[i].args + 1))
		    continue;
		*gargs = args;
		return i;
	    }
    return -1;
}

void auto_reply_event(EVENT * e)
{
    MECH *mech = (MECH *) e->data;
    char *buf = (char *) e->data2;

    sendchannelstuff(mech, 0, buf);
    free_lbuf(buf);
}

void auto_reply(MECH * mech, char *buf)
{
    /* This can be _seriously_ *ouch* if the mech in question blows up before it's
       time. Oh well. Can't win always. */
    if (!mech->freq[0])
	return;			/* funny freq - no freq0 spam */
    if (MechAuto(mech) <= 0)
	return;			/* No autopilot in mech */
    MECHEVENT(mech, EVENT_AUTO_REPLY, auto_reply_event, Number(1, 2), buf);
}

void auto_replyA(MECH * mech, char *buf)
{
    char *c;

    if (!mech->freq[0])
	return;			/* funny freq - no freq0 spam */
    if (MechAuto(mech) <= 0)
	return;			/* No autopilot in mech */
    c = alloc_lbuf("auto_replyA");
    strcpy(c, buf);
    MECHEVENT(mech, EVENT_AUTO_REPLY, auto_reply_event, Number(1, 2), c);
}

void auto_parse_command(AUTO * a, MECH * mech, int chn, char *buffer)
{
    int argc, cmd;
    char *args[2];
    char **argsi, *res;
    char buf[LBUF_SIZE];

    if (!a || !mech)
	return;
    if (Destroyed(mech))
	return;
    if (silly_parseattributes(buffer, args, 2) < 2)
	return;
    /* Is it me? If not, not worth doing things about */
    if (strcmp(args[0], "all")) {
	buf[0] = MechID(mech)[0];
	buf[1] = MechID(mech)[1];
	buf[2] = 0;

/*       sprintf(buf, "#%d", mech->mynum); */
	if (strcasecmp(buf, args[0]))
	    return;
    }
    if ((cmd =
	    auto_parse_command_sub(a, mech, args[1], &argsi, &argc)) < 0) {
	auto_replyA(mech, "Unable to comprehend the command.");
	return;
    }
    if ((res =
	    (*(auto_cmds[cmd].fun)) (a, mech, &(argsi[1]), argc - 1,
		chn))) {
	char *buf = alloc_lbuf("auto_parse");

	if (auto_cmds[cmd].silent)
	    return;
	if (res[0] == '!') {
	    sprintf(buf, "ERROR:%s!", res + 1);
	} else {
	    const char *res_start;
	    const char *res_end = ".";

	    switch (Number(0, 20)) {
	    case 0:
	    case 1:
	    case 2:
	    case 4:
		res_start = "Affirmative, ";
		break;
	    case 5:
		res_start = "Nod, ";
		break;
	    case 6:
		res_start = "Fine, ";
		break;
	    case 7:
		res_start = "Aye aye, Captain, ";
		res_end = "!";
		break;
	    case 8:
	    case 9:
	    case 10:
		res_start = "Da, boss, ";
		res_end = "!";
		break;
	    case 11:
	    case 12:
		res_start = "Ok, ";
		break;
	    case 13:
		res_start = "Okay, okay, ";
		res_end = ", happy now?";
		break;
	    case 14:
		res_start = "Okidoki, ";
		res_end = "!";
		break;
	    case 15:
	    case 16:
	    case 17:
		res_start = "Aye, ";
		break;
	    default:
		res_start = "Roger, roger, ";
		break;
	    }
	    sprintf(buf, "%s%s%s", res_start, res, res_end);
	}
	auto_reply(mech, buf);
    } else if (!auto_cmds[cmd].silent) {
	strcpy(buf, "Ok.");
	auto_replyA(mech, buf);
    }
}
