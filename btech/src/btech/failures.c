
/* This is the code that runs the parts failures.
   Written by: Nim
   9-28-96
   
   Parts copyright (c) 2000-2002 Thomas Wouters
   
 */

/*
 * $Id: failures.c,v 1.1.1.1 2005/01/11 21:18:07 kstevens Exp $
 * Last modified: Sat Jun  6 21:43:52 1998 fingon
 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define _FAILURES_C
#include "mech.h"
#include "failures.h"
#include "mech.events.h"
#include "p.mech.startup.h"

extern int num_def_weapons;

int GetBrandIndex(int type)
{
	if(type == -1)
		return COMPUTER_INDEX;
	if(type == -2)
		return RADIO_INDEX;
	if(IsWeapon(type))
		if(type < I2Weapon(num_def_weapons)) {
			type = Weapon2I(type);
			if(MechWeapons[type].special & PCOMBAT)
				return -1;
			if(IsFlamer(type))
				return FLAMMER_INDEX;
			if(IsEnergy(type))
				return ENERGY_INDEX;
			if(IsAutocannon(type))
				return AC_INDEX;
			if(IsMissile(type))
				return MISSILE_INDEX;
			return -1;
		}
	return -1;
}

char *GetPartBrandName(int type, int level)
{
	int i;

	if(!level)
		return NULL;
	i = GetBrandIndex(type);
	if(i < 0)
		return NULL;
	return brands[i * 5 / 6 + level - 1].name;
}

#define Conv(mech,section,critical) \
(GetBrandIndex(GetPartType(mech, section, critical)) - 1)

void FailureRadioStatic(MECH * mech, int weapnum, int weaptype,
						int section, int critical, int roll, int *modifier,
						int *type)
{
	int mod = failures[GetBrandIndex(-2) + roll - 1].data;

	*modifier = mod;
	*type = FAIL_STATIC;
}

static void mech_rrec_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;
	long val = (long) e->data2;

	MechRadioRange(mech) += val;
	if(!Destroyed(mech) && val == MechRadioRange(mech))
		mech_notify(mech, MECHALL, "Your radio is now operational again.");
}

static void mech_srec_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;
	long val = (long) e->data2;
	int vt = val / 256;

	switch (vt) {
	case 0:
		MechTacRange(mech) = val;
		if(!Destroyed(mech))
			mech_notify(mech, MECHALL,
						"Your tactical scanners are operational again.");
		break;
	case 1:
		MechLRSRange(mech) = val;
		if(!Destroyed(mech))
			mech_notify(mech, MECHALL,
						"Your long-range scanners are operational again.");
		break;
	case 2:
		MechScanRange(mech) = val;
		if(!Destroyed(mech))
			mech_notify(mech, MECHALL,
						"Your scanners are operational again.");
		break;
	}
}

void FailureRadioShort(MECH * mech, int weapnum, int weaptype, int section,
					   int critical, int roll, int *modifier, int *type)
{
	MECHEVENT(mech, EVENT_MRECOVERY, mech_rrec_event, Number(30, Number(40,
																		200)),
			  (long) MechRadioRange(mech));
	MechRadioRange(mech) = 0;
}

void FailureRadioRange(MECH * mech, int weapnum, int weaptype, int section,
					   int critical, int roll, int *modifier, int *type)
{
	int mod = failures[GetBrandIndex(-2) + roll - 1].data;

	mod = MIN(MechRadioRange(mech) - 1, mod);
	MECHEVENT(mech, EVENT_MRECOVERY, mech_rrec_event, Number(30, Number(40,
																		200)),
			  (long) mod);
	MechRadioRange(mech) -= mod;
}

void FailureComputerShutdown(MECH * mech, int weapnum, int weaptype,
							 int section, int critical, int roll,
							 int *modifier, int *type)
{
	if(Started(mech))
		mech_shutdown(mech->mynum, mech, "");
}

void FailureComputerScanner(MECH * mech, int weapnum, int weaptype,
							int section, int critical, int roll,
							int *modifier, int *type)
{
	int tmp = failures[GetBrandIndex(-1) + roll - 1].data;

	switch (tmp) {
	case 1:
		MECHEVENT(mech, EVENT_MRECOVERY, mech_srec_event, Number(30,
																 Number(40,
																		200)),
				  (long) MechTacRange(mech));
		MechTacRange(mech) = 0;
		break;
	case 2:
		MECHEVENT(mech, EVENT_MRECOVERY, mech_srec_event, Number(30,
																 Number(40,
																		200)),
				  (long) (MechLRSRange(mech) + 256));
		MechLRSRange(mech) = 0;
		break;
	case 4:
		MECHEVENT(mech, EVENT_MRECOVERY, mech_srec_event, Number(30,
																 Number(40,
																		200)),
				  (long) (MechScanRange(mech) + 512));
		MechScanRange(mech) = 0;
		break;
	case 7:
		MECHEVENT(mech, EVENT_MRECOVERY, mech_srec_event, Number(30,
																 Number(40,
																		200)),
				  (long) MechTacRange(mech));
		MECHEVENT(mech, EVENT_MRECOVERY, mech_srec_event,
				  Number(30, Number(40, 200)), (long) (MechLRSRange(mech) + 256));
		MECHEVENT(mech, EVENT_MRECOVERY, mech_srec_event,
				  Number(30, Number(40, 200)), (long) (MechScanRange(mech) + 512));
		MechTacRange(mech) = 0;
		MechLRSRange(mech) = 0;
		MechScanRange(mech) = 0;
		break;
	}
}

void FailureComputerTarget(MECH * mech, int weapnum, int weaptype,
						   int section, int critical, int roll, int *modifier,
						   int *type)
{
	MechTarget(mech) = -1;
}

void FailureWeaponMissiles(MECH * mech, int weapnum, int weaptype,
						   int section, int critical, int roll, int *modifier,
						   int *type)
{
	SetPartTempNuke(mech, section, critical, failures[Conv(mech, section,
														   critical) +
													  roll].type);
	*type = CRAZY_MISSILES;
	*modifier = failures[Conv(mech, section, critical) + roll].data;
}

void FailureWeaponDud(MECH * mech, int weapnum, int weaptype, int section,
					  int critical, int roll, int *modifier, int *type)
{
	if(failures[Conv(mech, section, critical) + roll].type == FAIL_NONE) {
		SetRecyclePart(mech, section, critical, MechWeapons[weaptype].vrt);
		return;
	}
	SetPartTempNuke(mech, section, critical, failures[Conv(mech, section,
														   critical) +
													  roll].type);
	*type = WEAPON_DUD;
	if(roll == 6) {
		SetPartTempNuke(mech, section, critical, FAIL_DESTROYED);
	}
	SetRecyclePart(mech, section, critical, 30 + Number(1, 60));
}

void FailureWeaponJammed(MECH * mech, int weapnum, int weaptype,
						 int section, int critical, int roll, int *modifier,
						 int *type)
{
	SetPartTempNuke(mech, section, critical, failures[Conv(mech, section,
														   critical) +
													  roll].type);
	*type = WEAPON_JAMMED;
	SetRecyclePart(mech, section, critical, Number(20, 40));
}

void FailureWeaponRange(MECH * mech, int weapnum, int weaptype,
						int section, int critical, int roll, int *modifier,
						int *type)
{
	*modifier =
		(int) (EGunRangeWithCheck(mech, section,
								  weaptype) * (failures[Conv(mech, section,
															 critical) +
														roll].data / 100.0));
	*type = RANGE;
}

void FailureWeaponDamage(MECH * mech, int weapnum, int weaptype,
						 int section, int critical, int roll, int *modifier,
						 int *type)
{
	*modifier =
		(int) (MechWeapons[weaptype].damage * (failures[Conv(mech, section,
															 critical) +
														roll].data / 100.0));
	*type = DAMAGE;
}

void FailureWeaponHeat(MECH * mech, int weapnum, int weaptype, int section,
					   int critical, int roll, int *modifier, int *type)
{
	*modifier =
		(int) MechWeapons[weaptype].heat * (failures[Conv(mech, section,
														  critical) +
													 roll].data / 100.0);
	*type = HEAT;
}

void FailureWeaponSpike(MECH * mech, int weapnum, int weaptype,
						int section, int critical, int roll, int *modifier,
						int *type)
{
	SetPartTempNuke(mech, section, critical, failures[Conv(mech, section,
														   critical) +
													  roll].type);
	*type = POWER_SPIKE;
	if(roll == 6) {
		SetPartTempNuke(mech, section, critical, FAIL_DESTROYED);
		return;
	}
	SetRecyclePart(mech, section, critical, Number(20, 40));
}

void CheckGenericFail(MECH * mech, int type, int *result, int *mod)
{
	int i = GetBrandIndex(type);
	int l = type == -1 ? MechComputer(mech) : MechRadio(mech);
	int roll, in;

	if(result)
		*result = FAIL_NONE;
	if(i < 0)
		return;
	if(mudconf.btech_parts) {
		if(!l)
			l = 5;
	} else
		return;
	if(Number(1, 5000) != 42)
		return;					/* ~1/5000 chance */
	if(Number(1, 100) <= brands[(i + l - 1) * 5 / 6].success)
		return;
	roll = Number(1, 6);
	if(roll == 6)
		roll = Number(1, 6);
	in = i + roll - 1;
	switch (failures[in].flag) {
	case REQ_TARGET:
		if(MechTarget(mech) <= 0)
			return;
		break;
	case REQ_TAC:
		if(MechTacRange(mech) == 0)
			return;
		break;
	case REQ_LRS:
		if(MechLRSRange(mech) == 0)
			return;
		break;
	case REQ_SCANNERS:
		if(MechTacRange(mech) == 0 || MechLRSRange(mech) == 0 ||
		   MechScanRange(mech) == 0)
			return;
		break;
	case REQ_COMPUTER:
		/* */
		break;
	case REQ_RADIO:
		if(MechRadioRange(mech) == 0)
			return;
		break;
	}
	if(failures[in].message && strcmp(failures[in].message, "none"))
		mech_notify(mech, MECHALL, failures[in].message);
	failures[in].func(mech, -1, -1, -1, -1, roll, mod, result);
}

void CheckWeaponFailed(MECH * mech, int weapnum, int weaptype, int section,
					   int critical, int *modifier, int *type)
{
	short roll;
	int l = GetPartBrand(mech, section, critical);
	int t = GetPartType(mech, section, critical);
	int i = GetBrandIndex(t), in;

	*type = FAIL_NONE;
	if(i < 0)
		return;
	if(mudconf.btech_parts) {
		if(!l)
			l = 5;
		if(MechWeapons[Weapon2I(t)].special & PCOMBAT)
			return;
	} else
		return;
	if(Number(1, 10) < 9)
		return;
	if(Number(1, 100) <= brands[(i + l - 1) * 5 / 6].success)
		return;
	roll = Number(1, 6);
	if(roll == 6)
		roll = Number(1, 6);
	in = i + roll - 1;
	if(failures[in].flag & REQ_HEAT)
		if(!MechWeapons[weaptype].heat)
			return;
	if(failures[in].message && strcmp(failures[in].message, "none"))
		mech_notify(mech, MECHALL, failures[in].message);
	failures[in].func(mech, weapnum, weaptype, section, critical, roll,
					  modifier, type);
}
