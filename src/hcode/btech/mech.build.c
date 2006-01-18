
/*
 * $Id: mech.build.c,v 1.1.1.1 2005/01/11 21:18:11 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Last modified: Wed Apr 29 21:04:14 1998 fingon
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "weapons.h"
#include "p.mech.partnames.h"
#include "p.mech.utils.h"

const int num_def_weapons = NUM_DEF_WEAPONS;

int CheckData(dbref player, void *data)
{
	int returnValue = 1;

	if(data == NULL) {
		notify(player, "There is a problem with that item.");
		notify(player, "The data is not properly allocated.");
		notify(player, "Please notify a director of this.");
		returnValue = 0;
	}
	return (returnValue);
}

void FillDefaultCriticals(MECH * mech, int index)
{
	int loop;

	for(loop = 0; loop < NUM_CRITICALS; loop++) {
		MechSections(mech)[index].criticals[loop].type = EMPTY;
		MechSections(mech)[index].criticals[loop].data = 0;
		MechSections(mech)[index].criticals[loop].firemode = 0;
		MechSections(mech)[index].criticals[loop].ammomode = 0;
	}

	if(MechType(mech) == CLASS_AERO)
		switch (index) {
		case AERO_COCKPIT:
			MechSections(mech)[index].criticals[0].type =
				I2Special(LIFE_SUPPORT);
			MechSections(mech)[index].criticals[1].type = I2Special(SENSORS);
			MechSections(mech)[index].criticals[2].type = I2Special(COCKPIT);
			MechSections(mech)[index].criticals[3].type = I2Special(SENSORS);
			MechSections(mech)[index].criticals[4].type =
				I2Special(LIFE_SUPPORT);
			break;
		case AERO_ENGINE:
			for(loop = 0; loop < 12; loop++)
				MechSections(mech)[index].criticals[loop].type =
					I2Special(HEAT_SINK);
			MechSections(mech)[index].criticals[2].type = I2Special(ENGINE);
			MechSections(mech)[index].criticals[10].type = I2Special(ENGINE);
			break;
		}
	if(MechType(mech) == CLASS_MECH)
		switch (index) {
		case HEAD:
			MechSections(mech)[index].criticals[0].type =
				I2Special(LIFE_SUPPORT);
			MechSections(mech)[index].criticals[1].type = I2Special(SENSORS);
			MechSections(mech)[index].criticals[2].type = I2Special(COCKPIT);
			MechSections(mech)[index].criticals[4].type = I2Special(SENSORS);
			MechSections(mech)[index].criticals[5].type =
				I2Special(LIFE_SUPPORT);
			break;

		case CTORSO:
			MechSections(mech)[index].criticals[0].type = I2Special(ENGINE);
			MechSections(mech)[index].criticals[1].type = I2Special(ENGINE);
			MechSections(mech)[index].criticals[2].type = I2Special(ENGINE);
			MechSections(mech)[index].criticals[3].type = I2Special(GYRO);
			MechSections(mech)[index].criticals[4].type = I2Special(GYRO);
			MechSections(mech)[index].criticals[5].type = I2Special(GYRO);
			MechSections(mech)[index].criticals[6].type = I2Special(GYRO);
			MechSections(mech)[index].criticals[7].type = I2Special(ENGINE);
			MechSections(mech)[index].criticals[8].type = I2Special(ENGINE);
			MechSections(mech)[index].criticals[9].type = I2Special(ENGINE);
			break;

		case RTORSO:
		case LTORSO:
			break;

		case LARM:
		case RARM:
		case LLEG:
		case RLEG:
			MechSections(mech)[index].criticals[0].type =
				I2Special(SHOULDER_OR_HIP);
			MechSections(mech)[index].criticals[1].type =
				I2Special(UPPER_ACTUATOR);
			MechSections(mech)[index].criticals[2].type =
				I2Special(LOWER_ACTUATOR);
			MechSections(mech)[index].criticals[3].type =
				I2Special(HAND_OR_FOOT_ACTUATOR);
			break;
		}
}

char *ShortArmorSectionString(char type, char mtype, int loc)
{
	char **locs;
	static char buf[4];
	char *c = buf;
	int i;

	locs = ProperSectionStringFromType(type, mtype);
	for(i = 0; locs[loc][i]; i++)
		if(isupper(locs[loc][i]) || isdigit(locs[loc][i]))
			*(c++) = locs[loc][i];
	*c = 0;
	return buf;
}

int ArmorSectionFromString(char type, char mtype, char *string)
{
	char **locs;
	int i, j;
	char *c, *d;

	if(!string[0])
		return -1;
	locs = ProperSectionStringFromType(type, mtype);
	if(!locs)
		return -1;
	/* Then, methodically compare against each other until a suitable
	   match is found */
	for(i = 0; locs[i]; i++)
		if(!strcasecmp(string, locs[i]))
			return i;
	for(i = 0; locs[i]; i++) {
		if(toupper(string[0]) != locs[i][0])
			continue;
		for(j = (i + 1); locs[j]; j++)
			if(toupper(string[0]) == locs[j][0])
				break;
		if(!locs[j])
			return i;
		/* Ok, comparison between these two, then */
		c = strstr(locs[i], " ");
		d = strstr(locs[j], " ");
		if(!c && !string[1] && d)
			return i;
		if(!c && !d)
			return -1;
		if(!string[1])
			continue;
		if(c && toupper(string[1]) == *(++c))
			return i;
		if(d && toupper(string[1]) == *(++d))
			return j;
	}
	return -1;
}

int WeaponIndexFromString(char *string)
{
	int id, brand;

	if(find_matching_vlong_part(string, NULL, &id, &brand))
		if(IsWeapon(id))
			return Weapon2I(id);
	return -1;
}

int FindSpecialItemCodeFromString(char *buffer)
{
	int id, brand;

	if(find_matching_vlong_part(buffer, NULL, &id, &brand))
		if(IsSpecial(id))
			return Special2I(id);
	return -1;
}
