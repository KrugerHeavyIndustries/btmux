
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
#define EVENT_RECYCLE      7	/* mech */
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
#define EVENT_AMMOWARN    22	/* Warn of running out of ammo */

#define FIRST_AUTO_EVENT        23
#define EVENT_AUTOGOTO          23  /* Autopilot goto */
#define EVENT_AUTOLEAVE         24  /* Autopilot leavebase */
#define EVENT_AUTOCOM           25  /* Autopilot next command */
#define EVENT_AUTOGUN           26  /* Autopilot gun control */
#define EVENT_AUTO_SENSOR       27  /* Autopilot gun/sensor change */
#define EVENT_AUTOFOLLOW        28  /* Autopilot follow */
#define EVENT_AUTOENTERBASE     29  /* Autopilot enterbase */
#define EVENT_AUTO_REPLY        30  /* Autopilot reply */
#define EVENT_AUTO_PROFILE      31  /* Autopilot profile change */
#define LAST_AUTO_EVENT         EVENT_AUTO_PROFILE 

#define EVENT_MRECOVERY     32  /* mech */
#define EVENT_BLINDREC      33
#define EVENT_BURN          34
#define EVENT_SS            35

#define EVENT_HIDE          36
#define EVENT_OOD           37
#define EVENT_NUKEMECH      38
#define EVENT_LATERAL       39
#define EVENT_EXPLODE       40
#define EVENT_DIG           41

#define FIRST_TECH_EVENT    42

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
#ifdef BT_MOVEMENT_MODES
#define EVENT_MOVEMODE              79
#define EVENT_SIDESLIP              80
#endif

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
    "MRec",             /* 32 */

    "BlindR",           /* 33 */
    "Burn",             /* 34 */
    "SixthS",           /* 35 */

    "Hidin",            /* 36 */
    "OOD",              /* 37 */

    "Misc",             /* 38 */
    "Lateral",          /* 39 */
    "SelfExp",          /* 40 */

    "DigIn",            /* 41 */

    "TRepl",            /* 42 */
    "TReplG",           /* 43 */
    "TReat",            /* 44 */
    "TRelo",            /* 45 */
    "TFix",             /* 46 */
    "TFixI",            /* 47 */
    "TScrL",            /* 48 */
    "TScrP",            /* 49 */
    "TScrG",            /* 50 */
    "TRepaG",           /* 51 */
    "TRepaP",           /* 52 */
    "TMoB",             /* 53 */
    "TUMoB",            /* 54 */
    "TRese",            /* 55 */
    "TRepSuit",         /* 56 */
    "TRepNHCrit",       /* 57 */
    "58",
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
