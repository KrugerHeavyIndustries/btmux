/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry 
 *       All rights reserved
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "coolmenu.h"
#include "p.mech.utils.h"
#include "p.mech.combat.h"
#include "p.mech.build.h"
#include "mech.events.h"

int ftflag = 0;

/*****************************************************************************/

/* TIC Routines                                                              */

/*****************************************************************************/

int cleartic_sub_func(MECH * mech, dbref player, int low, int high)
{
	int i, j;

	for(i = low; i <= high; i++) {
		for(j = 0; j < TICLONGS; j++)
			mech->tic[i][j] = 0;
		notify_printf(player, "TIC #%d cleared!", i);
	}
	return 0;
}

void cleartic_sub(dbref player, MECH * mech, char *buffer)
{
	int argc;
	char *args[3];

	DOCHECK((argc =
			 mech_parseattributes(buffer, args, 3)) != 1,
			"Invalid number of arguments to function");
	multi_weap_sel(mech, player, args[0], 2, cleartic_sub_func);
}

static int present_tic;

int addtic_sub_func(MECH * mech, dbref player, int low, int high)
{
	int i, j;

	for(i = low; i <= high; i++) {
		j = i / SINGLE_TICLONG_SIZE;
		mech->tic[present_tic][j] |= 1 << (i % SINGLE_TICLONG_SIZE);
	}
	if(low != high)
		notify_printf(player, "Weapons #%d - #%d added to TIC %d!", low,
					  high, present_tic);
	else
		notify_printf(player, "Weapon #%d added to TIC %d!", low,
					  present_tic);
	return 0;
}

void addtic_sub(dbref player, MECH * mech, char *buffer)
{
	int ticnum, argc;
	char *args[3];

	DOCHECK((argc =
			 mech_parseattributes(buffer, args, 3)) != 2,
			"Invalid number of arguments to function!");
	ticnum = atoi(args[0]);
	DOCHECK(!(ticnum >= 0 && ticnum < NUM_TICS), "Invalid tic number!");
	present_tic = ticnum;
	multi_weap_sel(mech, player, args[1], 0, addtic_sub_func);
}

int deltic_sub_func(MECH * mech, dbref player, int low, int high)
{
	int i, j;

	for(i = low; i <= high; i++) {
		j = i / SINGLE_TICLONG_SIZE;
		mech->tic[present_tic][j] &= ~(1 << (i % SINGLE_TICLONG_SIZE));
	}
	if(low != high)
		notify_printf(player, "Weapons #%d - #%d removed from TIC %d!",
					  low, high, present_tic);
	else
		notify_printf(player, "Weapon #%d removed from TIC %d!", low,
					  present_tic);
	return 0;
}

void deltic_sub(dbref player, MECH * mech, char *buffer)
{
	int ticnum, argc;
	char *args[3];

	argc = mech_parseattributes(buffer, args, 3);
	DOCHECK(argc < 1 ||
			argc > 2, "Invalid number of arguments to the function!");
	if(argc == 1) {
		cleartic_sub(player, mech, buffer);
		return;
	}
	ticnum = atoi(args[0]);
	DOCHECK(!(ticnum >= 0 && ticnum < NUM_TICS), "Invalid tic number!");
	present_tic = ticnum;
	multi_weap_sel(mech, player, args[1], 0, deltic_sub_func);
}

static char **temp_args;
static int temp_argc;

int firetic_sub_func(MECH * mech, dbref player, int low, int high)
{
	int i, j, k, count, weapnum;
	MAP *mech_map = FindObjectsData(mech->mapindex);
	int f = Fallen(mech);

	for(i = low; i <= high; i++) {
		notify_printf(player, "Firing weapons in tic #%d!", i);
		count = 0;
		for(k = 0; k < TICLONGS; k++)
			if(mech->tic[i][k])
				for(j = 0; j < SINGLE_TICLONG_SIZE; j++)
					if(mech->tic[i][k] & (1 << j)) {
						weapnum = k * SINGLE_TICLONG_SIZE + j;
						FireWeaponNumber(player, mech, mech_map, weapnum,
										 temp_argc, temp_args, 0);
						if(f != (Fallen(mech))) {
							if(Started(mech))
								mech_notify(mech, MECHALL,
											"That fall causes you to stop your fire!");
							return 1;
						} else if(!Started(mech))
							return 1;
						count++;
					}
		if(!count)
			notify_printf(player, "*Click* (the tic contained no weapons)");
	}
	return 0;
}

void firetic_sub(dbref player, MECH * mech, char *buffer)
{
	MAP *mech_map;
	int ticnum, argc;
	char *args[5];
	unsigned long weaps;

	DOCHECK((argc =
			 mech_parseattributes(buffer, args, 5)) < 1,
			"Not enough arguments to function");
	mech_map = getMap(mech->mapindex);
	ticnum = atoi(args[0]);
	DOCHECK(!(ticnum >= 0 && ticnum < NUM_TICS), "TIC out of range!");

/*   notify (player, tprintf ("Firing all weapons in TIC #%d at default target!", ticnum)); */
	weaps = 1;
	ftflag = 1;
	temp_args = args;
	temp_argc = argc;	
        multi_weap_sel(mech, player, args[0], 2, firetic_sub_func);
	ftflag = 0;
}

static MECH *present_mech;
static int present_count;

static char *listtic_fun(int i)
{
	int j, k, l, section, critical;
	static char buf[MBUF_SIZE];
	int count = 0;
	MECH *mech = present_mech;
	int rtar;

	if(!present_count) {
		strcpy(buf, "No weapons in tic.");
		return buf;
	}
	rtar = i / 2 + (i % 2 ? ((present_count + 1) / 2) : 0);
	for(j = 0; j < MAX_WEAPONS_PER_MECH; j++) {
		k = j / SINGLE_TICLONG_SIZE;
		l = j % SINGLE_TICLONG_SIZE;
		if(mech->tic[present_tic][k] & (1 << l)) {
			if(count == rtar) {
				if((FindWeaponNumberOnMech(mech, j, &section, &critical))
				   == -1) {
					mech->tic[present_tic][k] &= ~(1 << l);
					j = MAX_WEAPONS_PER_MECH;
					continue;
				}
				sprintf(buf, "#%2d %3s %-16s %s", j,
						ShortArmorSectionString(MechType(mech),
												MechMove(mech), section),
						&MechWeapons[Weapon2I
									 (GetPartType(mech, section, critical))].
						name[3], PartIsNonfunctional(mech, section,
													 critical) ? "(*)" : "");
				return buf;
			}
			count++;
		}
	}
	strcpy(buf, "Unknown - error of some sort occured");
	return buf;
}

void listtic_sub(dbref player, MECH * mech, char *buffer)
{
	int ticnum, argc;
	char *args[2];
	int i, j, k, count = 0;
	coolmenu *c;

	DOCHECK((argc =
			 mech_parseattributes(buffer, args, 2)) != 1,
			"Invalid number of arguments!");
	ticnum = atoi(args[0]);
	DOCHECK(!(ticnum >= 0 && ticnum < NUM_TICS), "TIC out of range!");
	present_mech = mech;
	present_tic = ticnum;
	for(i = 0; i < MAX_WEAPONS_PER_MECH; i++) {
		j = i / SINGLE_TICLONG_SIZE;
		k = i % SINGLE_TICLONG_SIZE;
		if(mech->tic[ticnum][j] & (1 << k))
			count++;
	}
	present_count = count;
	c = SelCol_FunStringMenuK(2, tprintf("TIC #%d listing for %s", ticnum,
										 GetMechID(mech)), listtic_fun, MAX(1,
																			count));
	ShowCoolMenu(player, c);
	KillCoolMenu(c);
}

void mech_cleartic(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;

	cch(MECH_USUALSM);
	cleartic_sub(player, mech, buffer);
}

void mech_addtic(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;

	cch(MECH_USUALSM);
	addtic_sub(player, mech, buffer);
}

void mech_deltic(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;

	cch(MECH_USUALSM);
	deltic_sub(player, mech, buffer);
}

void mech_firetic(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;

	cch(MECH_USUALO);
	firetic_sub(player, mech, buffer);
}

void mech_listtic(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;

	cch(MECH_USUALSM);
	listtic_sub(player, mech, buffer);
}

void heat_cutoff_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;

	if(e->data2) {
		mech_notify(mech, MECHALL, "%cyHeat dissipation cutoff engaged!%c");
		MechCritStatus(mech) |= HEATCUTOFF;
	} else {
		mech_notify(mech, MECHALL,
					"%cgHeat dissipation cutoff disengaged!%c");
		MechCritStatus(mech) &= ~(HEATCUTOFF);
	}
}

void heat_cutoff(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;

	if(mudconf.btech_heatcutoff < 1) {
		notify(player, "This command has been disabled.");
		return;
	}

	cch(MECH_USUALSMO);
	if(HeatcutoffChanging(mech)) {
		notify(player,
			   "You are already toggling heat cutoff status. Please be patient.");
		return;
	}
	if(Heatcutoff(mech)) {
		notify(player, "Disengaging heat dissipation cutoff...");
		MECHEVENT(mech, EVENT_HEATCUTOFFCHANGING, heat_cutoff_event, 4, 0);
	} else {
		notify(player, "Engaging heat dissipation cutoff...");
		MECHEVENT(mech, EVENT_HEATCUTOFFCHANGING, heat_cutoff_event, 4, 1);
	}
}
