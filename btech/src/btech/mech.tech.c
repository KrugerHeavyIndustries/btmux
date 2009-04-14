/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 */

#include "mech.h"
#include "muxevent.h"
#include "mech.events.h"
#include "mech.tech.h"
#include "p.mech.utils.h"
#include "p.btechstats.h"
#include "p.mech.build.h"
#include "p.mech.partnames.h"

int game_lag(void)
{
	if(!muxevent_tick)
		return 0;
	return 100 * (mudstate.now - mudstate.restart_time) / muxevent_tick - 100;
}

int game_lag_time(int i)
{
	return (100 + game_lag()) * i / 100;
}

int player_techtime(dbref player)
{
/* Returns tech time, in minutes, for given player */

        time_t techtime;
        char *tt_attr;
	int tused;

        tt_attr = silly_atr_get(player, A_TECHTIME);

        if (tt_attr) {
                techtime = (time_t) atoi(tt_attr);
                if (techtime < mudstate.now)
                        techtime = mudstate.now;
                } else {
                        techtime = mudstate.now;
        }

        tused = ( techtime - mudstate.now ) / TECH_TICK;

        return tused;
}

int tech_roll(dbref player, MECH * mech, int diff)
{
	int s;
	int succ;
	int r =
		(HasBoolAdvantage(player, "tech_aptitude") ? char_rollsaving() :
		 Roll());

	s = FindTechSkill(player, mech);
	s += diff;
	succ = r >= s;
	if(Wizard(player)) {
		notify_printf(player, "Tech - BTH: %d(Base:%d, Mod:%d) Roll: %d",
					  s, s - diff, diff, r);
	} else {
		notify_printf(player, "BTH: %d Roll: %d", s, r);
	}
	if(succ && In_Character(mech->mynum))
		AccumulateTechXP(player, mech, BOUNDED(1, s - 7, MAX(2, 1 + diff)));
	return (r - s);
}

int tech_weapon_roll(dbref player, MECH * mech, int diff)
{
	int s;
	int succ;
	int r =
		(HasBoolAdvantage(player, "tech_aptitude") ? char_rollsaving() :
		 Roll());

	s = char_getskilltarget(player, "technician-weapons", 0);
	s += diff;
	succ = r >= s;
	if(Wizard(player)) {
		notify_printf(player,
					  "Tech-W - BTH: %d(Base:%d, Mod:%d) Roll: %d", s,
					  s - diff, diff, r);
	} else {
		notify_printf(player, "BTH: %d Roll: %d", s, r);
	}
	if(succ && In_Character(mech->mynum))
		AccumulateTechWeaponsXP(player, mech, BOUNDED(1, s - 7, MAX(2,
																	1 +
																	diff)));
	return (r - s);
}

/* Basic idea: Check for attribute, if not set, set it, and do interesting
   stuff */

void tech_status(dbref player, time_t dat)
{
	char buf[MBUF_SIZE];
	char *olds;
	int un;

	if(dat <= 0) {
		olds = silly_atr_get(player, A_TECHTIME);
		if(olds) {
			dat = (time_t) atoi(olds);
			if(dat < mudstate.now)
				dat = mudstate.now;
		} else
			dat = mudstate.now;
	}
	if(dat <= mudstate.now)
		notify(player, "You have no jobs pending!");
	else {
		un = (dat - mudstate.now) / TECH_TICK;
		sprintf(buf, "You have %d %s%s of repairs pending", un, TECH_UNIT,
				un != 1 ? "s" : "");
		if(un >= MAX_TECHTIME)
			sprintf(buf + strlen(buf),
					" and you're too tired to do more efficiently.");
		else {
			un = MAX_TECHTIME - un;
			sprintf(buf + strlen(buf),
					" and you're ready to do at least %d more %s%s of work.",
					un, TECH_UNIT, un == 1 ? "" : "s");
		}
		notify(player, buf);
	}
}

int tech_addtechtime(dbref player, int time)
{
	time_t old;
	char *olds = silly_atr_get(player, A_TECHTIME);

	if(olds) {
		old = (time_t) atoi(olds);
		if(old < mudstate.now)
			old = mudstate.now;
	} else
		old = mudstate.now;
	old += time * TECH_TICK;
	silly_atr_set(player, A_TECHTIME, tprintf("%u", old));
	tech_status(player, old);
	return (old - mudstate.now);
}

int tech_parsepart_advanced(MECH * mech, char *buffer, int *loc, int *pos,
							int *extra, int allowrear)
{
	char *args[5];
	int l, argc, isrear = 0;

	if(!(argc = mech_parseattributes(buffer, args, 4)))
		return -1;
	if(argc > (2 + (extra != NULL)))
		return -1;
	if(!allowrear) {
		if((!extra && argc != (1 + (pos != NULL))) || (extra &&
													   (argc <
														(1 + (pos != NULL))
														|| argc >
														(2 + (pos != NULL)))))
			return -1;
	} else {
		if(argc == 2) {
			if(toupper(args[1][0]) != 'R')
				return -1;
			isrear = 8;
		}
	}
	if((*loc =
		ArmorSectionFromString(MechType(mech), MechMove(mech), args[0])) < 0)
		return -1;
	if(allowrear)
		*loc += isrear;
	if(pos) {
		l = atoi(args[1]) - 1;
		if(l < 0 || l >= CritsInLoc(mech, *loc))
			return -2;
		*pos = l;
	}
	if(extra) {
		if(argc > 2)
			*extra = args[2][0];
		else
			*extra = 0;
	}
	return 0;
}

int tech_parsepart(MECH * mech, char *buffer, int *loc, int *pos, int *extra)
{
	return tech_parsepart_advanced(mech, buffer, loc, pos, extra, 0);
}

int tech_parsegun(MECH * mech, char *buffer, int *loc, int *pos, int *brand)
{
	char *args[3];
	int l, argc, t, c = 0, pi, pb;

	argc = mech_parseattributes(buffer, args, 3);
	if(argc < 1 || argc > (2 + (brand != NULL)))
		return -1;
	if(argc == (2 + (brand != NULL)) || (brand && argc == 2 && atoi(args[1]))) {
		if((*loc =
			ArmorSectionFromString(MechType(mech), MechMove(mech),
								   args[0])) < 0)
			return -1;
		l = atoi(args[1]);
		if(l <= 0 || l > CritsInLoc(mech, *loc))
			return -4;
		*pos = l - 1;
	} else {
		/* Check if it's a number */
		if(args[0][0] < '0' || args[0][0] > '9')
			return -1;
		l = atoi(args[0]);
		if(l < 0)
			return -1;
		if((t = FindWeaponNumberOnMech(mech, l, loc, pos)) == -1)
			return -1;
	}
	t = GetPartType(mech, *loc, *pos);
	if(brand != NULL && argc > 1 && !atoi(args[argc - 1])) {
		if(!find_matching_long_part(args[argc - 1], &c, &pi, &pb))
			return -2;
		if(pi != t)
			return -3;
		*brand = pb;
	} else if(brand != NULL)
		*brand = GetPartBrand(mech, *loc, *pos);
	return 0;
}

int cheated_last = 0;

static void cheat_find_last(MUXEVENT * e)
{
	int ofs = e->tick - muxevent_tick;
	int amount = (((int) e->data2) % PLAYERPOS) / 16 - 1;

	switch (e->type) {
	case EVENT_REPAIR_FIXI:
		ofs += amount * FIXINTERNAL_TIME * TECH_TICK;
		break;
	case EVENT_REPAIR_FIX:
		ofs += amount * FIXARMOR_TIME * TECH_TICK;
		break;
	}
	if(ofs > cheated_last)
		cheated_last = ofs;
}

int figure_latest_tech_event(MECH * mech)
{
	int i;

	cheated_last = 0;
	for(i = FIRST_TECH_EVENT; i <= LAST_TECH_EVENT; i++)
		muxevent_gothru_type_data(i, (void *) mech, cheat_find_last);
	return cheated_last;
}
