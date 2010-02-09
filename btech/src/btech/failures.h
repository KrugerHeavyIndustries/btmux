
/* Brand level modifiers and failure resultant data here after included.
   Failure.h
   Created By: Nim
   Dated:      9 - 21 - 96
   
   Parts copyright (c) 2002 Thomas Wouters

   $Id: failures.h,v 1.1.1.1 2005/01/11 21:18:07 kstevens Exp $
   Last modified: Sat Jun  6 20:27:26 1998 fingon
 */

#ifndef _FAILURES_H
#define _FAILURES_H

#include "p.failures.h"

#define IsAutocannon(a) (MechWeapons[a].type == TAMMO)
#define IsEnergy(a) (MechWeapons[a].type==TBEAM)
/*#define IsFlamer(a) (MechWeapons[a].type==TBEAM && \
	strstr(MechWeapons[a].name, "Flamer")) */

/* these are types of modifiers */
#define HEAT	        1
#define RANGE	        2
#define DAMAGE	        3
#define POWER_SPIKE     4
#define WEAPON_JAMMED   5
#define WEAPON_DUD      6
#define CRAZY_MISSILES  7

#define FAIL_STATIC     1

/* these are catagories of damage */
#define FAIL_NONE 	  0
#define FAIL_JAMMED 	  1
#define FAIL_SHORTED      2
#define FAIL_DUD	  3
#define FAIL_EMPTY	  4
#define FAIL_DESTROYED    5
#define FAIL_AMMOJAMMED	  6
#define FAIL_AMMOCRITJAMMED 7

struct brand_data {
    char *name;
    short level;
    int success;
    int modifier;
};

struct failure_data {
    char *message;
    int data;			/* things like percent to alter */
    void (*func) (MECH *, int, int, int, int, int, int *, int *);
    int type;
    int flag;
};

/*  Brand keys 
   1 - This is absolute crap.
   2 - This is low end.
   3 - This is average.
   4 - These are supieror parts.
   5 - These are EXTREMELY RARE and EXTREMELY reliable
 */

#ifndef _FAILURES_C

extern struct brand_data brands[];
extern struct failure_data failures[];
#else
struct brand_data brands[] = {
    {"Lords", 1, 80, -40},	/* Energy weapons */
    {"Hesperus", 2, 90, -20},
    {"Martell", 3, 95, 0},
    {"Magna", 4, 100, 20},
    {"Agra", 5, 101, 40},

    {"Luxor", 1, 80, -40},	/* Autocannons */
    {"SperryBrowning", 2, 90, -20},
    {"Oriente", 3, 95, 0},
    {"Deprus", 4, 100, 20},
    {"Armstrong", 5, 101, 40},

    {"Coventry", 1, 80, -40},	/* Missiles */
    {"Shannon", 2, 90, -20},
    {"Bical", 3, 95, 0},
    {"Holly", 4, 100, 20},
    {"Telos", 5, 101, 40},

    {"Pynes", 1, 80, -40},	/* Flamers */
    {"Hotshot", 2, 90, -20},
    {"Firestorm", 3, 95, 0},
    {"Purity", 4, 100, 20},
    {"Ventra", 5, 101, 40},

    {"Dalban", 1, 80, -40},	/* Computers */
    {"Hartford", 2, 90, -20},
    {"Garet", 3, 95, 0},
    {"Ares", 4, 100, 20},
    {"Tek", 5, 101, 40},

    {"Duoteck", 1, 80, -40},	/* Radios */
    {"CeresCom", 2, 90, -20},
    {"Achernar", 3, 95, 0},
    {"Tek", 4, 100, 20},
    {"Iriad", 5, 101, 40},
};

#define REQ_HEAT 1
#define REQ_TARGET 2
#define REQ_TAC 3
#define REQ_LRS 4
#define REQ_SCANNERS 5
#define REQ_COMPUTER 6
#define REQ_RADIO 7

struct failure_data failures[] = {
#define 	ENERGY_INDEX	0
    /* Energy Weapons - 0 */

    {"%ch%crYour weapon fails to charge properly!%cn", 15,
	FailureWeaponDamage, FAIL_NONE, 0},
    {"%ch%crYour weapon fails to charge properly!%cn", 30,
	FailureWeaponDamage, FAIL_NONE, 0},
    {"%ch%crYour weapon fails to charge properly!%cn", 45,
	FailureWeaponDamage, FAIL_NONE, 0},
    {"%ch%crFailure in the weapon's cooling system ; too much heat produced!%cn",
	30, FailureWeaponHeat, FAIL_NONE, REQ_HEAT},
    {"%ch%crOdd energy reading from the weapon ; It seems to have gone offline!%cn",
	0, FailureWeaponSpike, FAIL_SHORTED, 0},
    {"%ch%crWeapon melts down!%cn", 0, FailureWeaponSpike, FAIL_SHORTED,
	0},

    /* Autocannons - 6 */
#define		AC_INDEX	6

    {"%ch%crRound misfires! .. and spirals off!%cn", 0,
	FailureWeaponDud, FAIL_NONE, 0},
    {"%ch%crRound not fired!  Dud!%cn", 0, FailureWeaponDud, FAIL_DUD, 0},
    {"%ch%crWeapon JAMS... clearing!%cn", 0, FailureWeaponJammed,
	FAIL_JAMMED, 0},
    {"%ch%crFailure in the weapon's cooling system, too much heat produced!%cn",
	20, FailureWeaponHeat, FAIL_NONE, REQ_HEAT},
    {"%ch%crFailure in the weapon's cooling system, too much heat produced!%cn",
	40, FailureWeaponHeat, FAIL_NONE, REQ_HEAT},
    {"%ch%crRound not fired!  STUCK in chamber!%cn", 0, FailureWeaponDud,
	FAIL_DUD, 0},

    /* Missiles - 12 */
#define 	MISSILE_INDEX	12

    {"%ch%crRack jams, attemping to clear!%cn", 0, FailureWeaponJammed,
	FAIL_JAMMED, 0},
    {"%ch%crSome of your missiles veer off course!%cn", 20,
	FailureWeaponMissiles, FAIL_NONE, 0},
    {"%ch%crSome of your missiles veer off course!%cn", 40,
	FailureWeaponMissiles, FAIL_NONE, 0},
    {"%ch%crGuidance Failure!  All missile veer off course!%cn", 100,
	FailureWeaponMissiles, FAIL_NONE, 0},
    {"%ch%crWeapon power spikes.. attempting to restart!%cn", 0,
	FailureWeaponSpike, FAIL_SHORTED, 0},
    {"%ch%crWeapon power spikes.. Electronics fused!!%cn", 0,
	FailureWeaponSpike, FAIL_SHORTED, 0},

    /* Flamer - 18 */
#define		FLAMMER_INDEX	18

    {"%ch%crGel line clogs, sending pressure through it now!%cn", 0,
	FailureWeaponJammed, FAIL_JAMMED, 0},
    {"%ch%crElectric ignition shorts out! Restarting!%cn", 0,
	FailureWeaponSpike, FAIL_SHORTED, 0},
    {"%ch%crFuel leaks on the chassis and ignites!%cn", 100,
	FailureWeaponHeat, FAIL_NONE, 0},

    {"%ch%crFuel at critical point!! Shutting down weapon to vent heat!%cn",
	0, FailureWeaponSpike, FAIL_SHORTED, 0},
    {"%ch%crEjection nozzle gums up!  Please wait while pressure is applied!%cn",
	0, FailureWeaponJammed, FAIL_JAMMED, 0},
    {"%ch%crFuel canisters explode!  No fuel left to burn!%cn", 0,
	FailureWeaponSpike, FAIL_EMPTY, 0},

    /* Computer - 24 */
#define		COMPUTER_INDEX	24

    {"%ch%crComputer Glitch!  Target lost, please reacquire!%cn", 0,
	FailureComputerTarget, FAIL_NONE, REQ_TARGET},
    {"%ch%crTactical shorts out! Fixing .. Please stand by.%cn", 1,
	FailureComputerScanner, FAIL_NONE, REQ_TAC},
    {"%ch%crLong Range Sensors short out! .. Fixing .. Please stand by.%cn",
	2, FailureComputerScanner, FAIL_NONE, REQ_LRS},
    {"%ch%crScanners short out! Fixing .. Please stand by.%cn", 4,
	FailureComputerScanner, FAIL_NONE, REQ_SCANNERS},
    {"%ch%crA sudden *SNAP* echos in your cockpit then all your displays die!%cn",
	7, FailureComputerScanner, FAIL_NONE, REQ_SCANNERS},
    {"%ch%crYou hear a loud *SNAP* *CRACKLE* and then everything powers down!%cn",
	0, FailureComputerShutdown, FAIL_NONE, REQ_COMPUTER},

    /* Radio - 30 */
#define		RADIO_INDEX	30
    {"none", 50, FailureRadioStatic, FAIL_NONE, 0},
    {"none", 70, FailureRadioStatic, FAIL_NONE, 0},
    {"%ch%crYour readouts register a power loss in your radio!%cn", 15,
	FailureRadioRange, FAIL_NONE, REQ_RADIO},
    {"%ch%crYour readouts register a power loss in your radio!%cn", 30,
	FailureRadioRange, FAIL_NONE, REQ_RADIO},
    {"%ch%crYour radio suddenly shorts out! Please wait for backup to come online!%cn",
	0, FailureRadioShort, FAIL_NONE, REQ_RADIO},
    {"%ch%crYour entire radio system suddenly shorts out!%cn", 0,
	FailureRadioShort, FAIL_NONE, REQ_RADIO}
};

#endif
#endif
