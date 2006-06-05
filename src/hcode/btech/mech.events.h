
/*
 * $Id: mech.events.h,v 1.2 2005/08/03 21:40:54 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 *
 * Created: Fri Aug 30 15:32:12 1996 fingon
 * Last modified: Sat Jun  6 20:14:55 1998 fingon
 *
 */

#ifndef MECH_EVENTS_H
#define MECH_EVENTS_H

#include "mech.h"
#include "muxevent.h"
#include "p.event.h"

/* Semi-combat-related events */
#define EVENT_MOVE         1	/* mech */
	/* Updates mech's position and the positionchanged flag */
#define EVENT_DHIT         2	/* <artydata> */
#define EVENT_STARTUP      3	/* mech,timer */
	/* Starts up da mech (timer = int which shows da stage of startup) */
#define EVENT_LOCK         4	/* mech,target */
	/* Engages lock between <mech>,and <target> (breaking LOS stops this) */
#define EVENT_STAND        5	/* mech */
	/* Makes da mech stand */
#define EVENT_JUMP         6
	/* Advances us one jump 'step' */
#define EVENT_RECYCLE      7	/* CONVERTED. mech */
	/* Weapons recycling.. */
#define EVENT_JUMPSTABIL   8	/* mech */
	/* If event of this type doesn't exist for mech, we've finished
	   stabilizing after last jump */
#define EVENT_RECOVERY     9	/* mech */
       /* Mech's pilot has chance of recovering from uncon */
#define EVENT_SCHANGE     10	/* mech, <modes> (with first as higher bytes) */
       /* Sensor mode's changing.. */
#define EVENT_DECORATION  11	/* timed decoration removal/happening thingy */
       /* map, mapobj */
#define EVENT_SPOT_LOCK   12	/* spot-lock (Nim's stuff) */
#define EVENT_PLOS        13	/* Possible-lock (mech) */
#define EVENT_SPOT_CHECK  14	/* Range-check for IDF */
#define EVENT_TAKEOFF     15	/* Aero takeoff (mech, secstilllaunch) */
#define EVENT_FALL        16	/* Shutdown mech falling */
#define EVENT_BREGEN      17	/* Building regeneration */
#define EVENT_BREBUILD    18	/* Building rebuild */
#define EVENT_DUMP        19	/* mech, loc : Dump something */

#define EVENT_MASC_FAIL   20	/* MASC roll for failure of stuff */
#define EVENT_MASC_REGEN  21	/* MASC recovery during non-use */
#define EVENT_AMMOWARN    22	/* CONVERTED. NO EVENT NEEDED NOW.  Warn of running out of ammo */

#define FIRST_AUTO_EVENT        23
#define EVENT_AUTOGOTO          23  /* Autopilot goto */
#define EVENT_AUTOLEAVE         24  /* Autopilot leavebase */
#define EVENT_AUTOCOM           25  /* Autopilot next command */
#define EVENT_AUTOGUN           26  /* Autopilot gun control */
#define EVENT_AUTO_SENSOR       27  /* CONVERTED Autopilot gun/sensor change */
#define EVENT_AUTOFOLLOW        28  /* Autopilot follow */
#define EVENT_AUTOENTERBASE     29  /* Autopilot enterbase */
#define EVENT_AUTO_REPLY        30  /* Autopilot reply */
#define EVENT_AUTO_PROFILE      31  /* Autopilot profile change */
#define EVENT_AUTO_ROAM         32
#define LAST_AUTO_EVENT         EVENT_AUTO_ROAM 

#define EVENT_MRECOVERY     33  /* mech */
#define EVENT_BLINDREC      34
#define EVENT_BURN          35
#define EVENT_SS            36

#define EVENT_HIDE          37
#define EVENT_OOD           38
#define EVENT_NUKEMECH      39
#define EVENT_LATERAL       40
#define EVENT_EXPLODE       41
#define EVENT_DIG           42

#define FIRST_TECH_EVENT    43

#define EVENT_REPAIR_REPL       FIRST_TECH_EVENT        /* mech,<part> */
#define EVENT_REPAIR_REPLG      (FIRST_TECH_EVENT+1)    /* mech,<part> */
#define EVENT_REPAIR_REAT       (FIRST_TECH_EVENT+2)    /* mech,<location> */
#define EVENT_REPAIR_RELO       (FIRST_TECH_EVENT+3)    /* mech,<part/amount> */
#define EVENT_REPAIR_FIX        (FIRST_TECH_EVENT+4)    /* mech,<loc/amount/type> */
#define EVENT_REPAIR_FIXI       (FIRST_TECH_EVENT+5)    /* mech,<loc/amount/type> */
#define EVENT_REPAIR_SCRL       (FIRST_TECH_EVENT+6)    /* mech, loc */
#define EVENT_REPAIR_SCRP       (FIRST_TECH_EVENT+7)    /* mech, loc, part */
#define EVENT_REPAIR_SCRG       (FIRST_TECH_EVENT+8)    /* mech, loc, part */
#define EVENT_REPAIR_REPAG      (FIRST_TECH_EVENT+9)    /* mech,<part> */
#define EVENT_REPAIR_REPAP      (FIRST_TECH_EVENT+10)   /* mech,<part> */
#define EVENT_REPAIR_MOB        (FIRST_TECH_EVENT+11)   /* mech,<part> */
#define EVENT_REPAIR_UMOB       (FIRST_TECH_EVENT+12)   /* mech,<part> */
#define EVENT_REPAIR_RESE       (FIRST_TECH_EVENT+13)   /* mech,<location> */
#define EVENT_REPAIR_REPSUIT    (FIRST_TECH_EVENT+14)   /* mech */
#define EVENT_REPAIR_REPENHCRIT (FIRST_TECH_EVENT+15)   /* mech */

#define LAST_TECH_EVENT    EVENT_REPAIR_REPENHCRIT

#define EVENT_STANDFAIL             60 
#define EVENT_SLITECHANGING         61 
#define EVENT_HEATCUTOFFCHANGING    62 
#define EVENT_VEHICLEBURN           63  /* Burn a side of a vehicle */
#define EVENT_UNSTUN_CREW           64  /* Unstun the crew */
#define EVENT_CREWSTUN              65
#define EVENT_UNJAM_TURRET          66 
#define EVENT_UNJAM_AMMO            67
#define EVENT_STEALTH_ARMOR         68
#define EVENT_NSS                   69
#define EVENT_TAG_RECYCLE           70
#define EVENT_REMOVE_PODS           71
#define EVENT_VEHICLE_EXTINGUISH    72
#define EVENT_ENTER_HANGAR          73
#define EVENT_CHANGING_HULLDOWN     74

/* Not used in the stable branch, just devel */
/* EVENT_BOGDOWNWAIT                75 */

#define EVENT_SCHARGE_FAIL          76  /* SCHARGE roll for failure of stuff */
#define EVENT_SCHARGE_REGEN         77  /* SCHARGE recovery during non-use */

#define EVENT_CHECK_STAGGER         78
#define EVENT_MOVEMODE              79
#define EVENT_SIDESLIP              80

#define ETEMPL(a) void a (MUXEVENT *e)

static char *muxevent_names[] = {
    "NONAME",           /* 0 - */
    "Move",             /* 1 */
    "DHIT",             /* 2 */
    "Startup",          /* 3 */
    "Lock",             /* 4 */
    "Stand",            /* 5 */
    "Jump",             /* 6 */
    "Recycle",          /* 7 */
    "JumpSt",           /* 8 */
    "PRecov",           /* 9 */
    "SChange",          /* 10 */
    "DecRemv",          /* 11 */
    "SpotLck",          /* 12 */
    "PLos",             /* 13 */
    "ChkRng",           /* 14 */
    "Takeoff",          /* 15 */

    "Fall",             /* 16 */
    "BRegen",           /* 17 */
    "BRebuild",         /* 18 */
    "Dump",             /* 19 */

    "MASCF",            /* 20 */
    "MASCR",            /* 21 */
    "AmmoWarn",         /* 22 */

    "AutoGoto",         /* 23 */
    "AutoLeave",        /* 24 */
    "AutoCo",           /* 25 */
    "AutoGun",          /* 26 */
    "AutoSensor",       /* 27 */
    "AutoFollow",       /* 28 */
    "AutoEnter",        /* 29 */
    "AutoReply",        /* 30 */
    "AutoProfile",      /* 31 */
    "AutoRoam",         /* 32 */
    "MRec",             /* 33 */

    "BlindR",           /* 34 */
    "Burn",             /* 35 */
    "SixthS",           /* 36 */

    "Hidin",            /* 37 */
    "OOD",              /* 38 */

    "Misc",             /* 39 */
    "Lateral",          /* 40 */
    "SelfExp",          /* 41 */

    "DigIn",            /* 42 */

    "TRepl",            /* 43 */
    "TReplG",           /* 44 */
    "TReat",            /* 45 */
    "TRelo",            /* 46 */
    "TFix",             /* 47 */
    "TFixI",            /* 48 */
    "TScrL",            /* 49 */
    "TScrP",            /* 50 */
    "TScrG",            /* 51 */
    "TRepaG",           /* 52 */
    "TRepaP",           /* 53 */
    "TMoB",             /* 54 */
    "TUMoB",            /* 55 */
    "TRese",            /* 56 */
    "TRepSuit",         /* 57 */
    "TRepNHCrit",       /* 58 */
    "59",
    "StandF",           /* 60 */
    "SliteC",           /* 61 */
    "HeatCutOff",       /* 62 */
    "VechBurn",         /* 63 */
    "UnStunCrew",       /* 64 */
    "StunCrew",         /* 65 */
    "UnJamTurret",      /* 66 */
    "UnJamAmmo",        /* 67 */
    "StArmor",          /* 68 */
    "NSS",              /* 69 */
    "TagRecycle",       /* 70 */
    "RemPods",          /* 71 */
    "Extinguish",       /* 72 */
    "EntHangar",        /* 73 */
    "Hulldown",         /* 74 */
    "75",               /* 75 */
    "SchFail",          /* 76 */
    "SchRegen",         /* 77 */
    "CkStagger",        /* 78 */
    "MoveMode",         /* 79 */
    "Sideslip",         /* 80 */
    NULL
};

#include "p.aero.move.h"
#include "p.mech.move.h"
#include "p.mech.events.h"

#endif				/* MECH_EVENTS_H */
