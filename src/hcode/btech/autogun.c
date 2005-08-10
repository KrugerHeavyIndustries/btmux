
/*
 * $Id: autogun.c,v 1.1 2005/06/13 20:50:49 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Sun Nov 17 13:23:20 1996 fingon
 * Last modified: Sun Jun 14 16:29:44 1998 fingon
 *
 */

/* Main idea:
   - Use the cheat-variables on MECHstruct to determine when / if
   to do LOS checks (num_mechs_seen)
   - Also, check for range (maxgunrange / etc variables in the
   autopilot struct)

   - If we have target(s):
   - Try to acquire a target with best BTH
   - Decide if it's worth shooting at
   - If yes, try to acquire it, if not possible, check other targets,
   repeat until all guns fired (/ tried to fire)
 */

#include "mech.h"
#include "mech.events.h"
#include "autopilot.h"
#include "failures.h"
#include "mech.sensor.h"
#include "p.mech.sensor.h"
#include "p.mech.utils.h"
#include "p.mech.physical.h"
#include "p.mech.combat.h"
#include "p.mech.advanced.h"
#include "p.mech.bth.h"

#define AUTOGUN_TICK 3		/* Every 3 secs */
#define AUTOGS_TICK  180	/* Every 3 minutes or so */
#define MAXHEAT      3		/* Last heat we let heat go to */
#define MAX_TARGETS  100

int global_kill_cheat = 0;

static char *my2string(const char *old)
{
    static char new[64];

    strncpy(new, old, 63);
    new[63] = '\0';
    return new;
}

void auto_gun_sensor_event(EVENT * e)
{
    AUTO *a = (AUTO *) e->data;
    MECH *mech = (MECH *) a->mymech;
    MAP *map = getMap(mech->mapindex);
    int flag = (int) e->data2;
    char buf[16];
    int wanted_s[2];
    int rvis;

    if (Destroyed(mech)) {
	DoStopGun(a);
	return;
    }
    if (a->flags & AUTOPILOT_LSENS)
	return;
    if (!map) {
	Zombify(a);
	return;
    }
    rvis = (map->maplight ? (map->mapvis) : (map->mapvis * 3));
    if (rvis <= 15) {
	wanted_s[0] = SENSOR_EM;
	wanted_s[1] = SENSOR_IR;
    } else {
	if (map->maplight > 0) {
	    wanted_s[0] = SENSOR_VIS;
	} else {
	    wanted_s[0] = SENSOR_LA;
	}
	if (rvis >= 20)
	    wanted_s[1] = SENSOR_SE;
	else {
	    if (Number(1, 2) == 1)
		wanted_s[1] = SENSOR_IR;
	    else
		wanted_s[1] = SENSOR_EM;
	}
    }
    if (!Sees360(mech))
	wanted_s[1] = wanted_s[0];
    if (MechSensor(mech)[0] != wanted_s[0] ||
	MechSensor(mech)[1] != wanted_s[1]) {
	sprintf(buf, "%c  %c", sensors[wanted_s[0]].matchletter[0],
	    sensors[wanted_s[1]].matchletter[0]);
	mech_sensor(a->mynum, mech, buf);
    }
    if (!flag)
	AUTOEVENT(a, EVENT_AUTOGS, auto_gun_sensor_event, AUTOGS_TICK, 0);
}

void auto_gun_event(EVENT * e)
{
    AUTO *a = (AUTO *) e->data;
    MECH *mech = (MECH *) a->mymech;
    MECH *targets[MAX_TARGETS], *t;
    int targetscore[MAX_TARGETS];
    int targetrange[MAX_TARGETS];
    int i, j, k, f;
    MAP *map;
    unsigned char weaparray[MAX_WEAPS_SECTION];
    unsigned char weapdata[MAX_WEAPS_SECTION];
    int critical[MAX_WEAPS_SECTION];
    int weapnum = 0, ii, loop, target_count = 0, ttarget_count = 0, count;
    char buf[LBUF_SIZE];
    int b;
    int h;
    int rt = 0;
    int fired = 0;
    int locked = 0;
    float save = 0.0;
    int cheating = 0;
    int locktarg_num = -1;
    int wMaxGunRange = 0;
    dbref c3Ref;

    if (Destroyed(mech)) {
	DoStopGun(a);
	return;
    }
    if (!Started(mech)) {
	Zombify(a);
	return;
    }
    if (!(map = getMap(mech->mapindex))) {
	Zombify(a);
	return;
    }
#if 0
    if (!MechNumSeen(mech)) {
	Zombify(a);
	return;
    }
#endif
    if (MechType(mech) == CLASS_MECH &&
	(MechPlusHeat(mech) - MechActiveNumsinks(mech)) > MAXHEAT) {
	AUTOEVENT(a, EVENT_AUTOGUN, auto_gun_event, AUTOGUN_TICK, 0);
	return;
    }
    global_kill_cheat = 0;
    for (i = 0; i < map->first_free; i++)
	if (i != mech->mapnumber && (j = map->mechsOnMap[i]) > 0) {
	    if (!(t = getMech(j)))
		continue;
	    if (Destroyed(t))
		continue;
	    if (MechStatus(t) & COMBAT_SAFE)
		continue;
	    if (MechTeam(t) == MechTeam(mech))
		continue;
	    if (!(MechToMech_LOSFlag(map, mech, t) & MECHLOSFLAG_SEEN))
		continue;
	    if ((targetrange[target_count] =
		    (int) FlMechRange(map, mech, t)) > 28)
		continue;
	    ttarget_count++;
	    if (!(a->targ <= 0 || ((a->flags & AUTOPILOT_FFA) ||
			(t->mynum == a->targ))))
		continue;
	    if (t->mynum == MechTarget(mech))
		locktarg_num = target_count;
	    targets[target_count] = t;
	    if (MechType(mech) == CLASS_MECH &&
		(targetrange[target_count] <= 1.0) &&
		!(CountDestroyedLegs(mech) > 0)) {
		char tb[6];

		sprintf(tb, "l %s", MechIDS(t, 0));
		mech_kick(a->mynum, mech, tb);
	    }
	    target_count++;
	}
    MechNumSeen(mech) = ttarget_count;
    if (!target_count) {
	Zombify(a);
	return;
    }
    /* Then, we'll see about our guns.. */
    for (loop = 0; loop < NUM_SECTIONS; loop++) {
	count = FindWeapons(mech, loop, weaparray, weapdata, critical);
	if (count <= 0)
	    continue;
	for (ii = 0; ii < count; ii++) {
	    weapnum++;
	    if (IsAMS(weaparray[ii]))
		continue;
	    if (PartIsNonfunctional(mech, loop, critical[ii]))
		continue;
	    if (PartTempNuke(mech, loop, critical[ii]))
		continue;
	    if (weapdata[ii])
		continue;
	    if (MechType(mech) == CLASS_MECH &&
		(rt + MechWeapons[weaparray[ii]].heat +
		    MechPlusHeat(mech) - MechActiveNumsinks(mech)) >
		MAXHEAT)
		continue;
	    /* Whee, it's not recycling or anything -> it's ready to fire! */
	    wMaxGunRange = EGunRangeWithCheck(mech, loop, weaparray[ii]);
	    for (i = 0; i < target_count; i++)
		if (wMaxGunRange > targetrange[i])
		    targetscore[i] =
			FindNormalBTH(mech, map, loop, critical[ii],
			weaparray[ii], (float) targetrange[i], targets[i],
			1000, &c3Ref);
		else
		    targetscore[i] = 999;
	    for (i = 0; i < (target_count - 1); i++)
		for (j = i + 1; j < target_count; j++)
		    if (targetscore[i] > targetscore[j]) {
			if (locktarg_num == i)
			    locktarg_num = j;
			else if (locktarg_num == j)
			    locktarg_num = i;
			t = targets[i];
			k = targetscore[i];
			f = targetrange[i];
			targets[i] = targets[j];
			targetscore[i] = targetscore[j];
			targetrange[i] = targetrange[j];
			targets[j] = t;
			targetscore[j] = k;
			targetrange[j] = f;
		    }
	    for (i = 0; i < target_count; i++) {
		/* This is .. simple, for now: We don't bother with 10+/12+ 
		   BTHs (depending on if locked or not) */
		if (locktarg_num >= 0 && i > locktarg_num)
		    break;
		if (!IsInWeaponArc(mech, MechFX(targets[i]),
			MechFY(targets[i]), loop, critical[ii])) {
		    b = FindBearing(MechFX(mech), MechFY(mech),
			MechFX(targets[i]), MechFY(targets[i]));
		    if (MechType(mech) == CLASS_MECH) {
			h = MechFacing(mech);
			if (GetPartFireMode(mech, loop,
				critical[ii]) & REAR_MOUNT)
			    h -= 180;
			h = AcceptableDegree(h);
			h -= b;
			if (h > 180)
			    h -= 360;
			if (h < -180)
			    h += 360;
			if (abs(h) > 120) {
			    /* Not arm weapon and not fliparm'able */
			    if ((loop != LARM && loop != RARM) ||
				!MechSpecials(mech) & FLIPABLE_ARMS)
				continue;
			    /* Woot. We can [possibly] fliparm to aim at foe */
			    if (MechStatus(mech) & (TORSO_LEFT |
				    TORSO_RIGHT))
				mech_rotatetorso(a->mynum, mech,
				    my2string("center"));
			    if (!(MechStatus(mech) & FLIPPED_ARMS))
				mech_fliparms(a->mynum, mech,
				    my2string(""));
			} else {
			    if (abs(h) < 60) {
				if (MechStatus(mech) & (TORSO_LEFT |
					TORSO_RIGHT))
				    mech_rotatetorso(a->mynum, mech,
					my2string("center"));
			    } else if (h < 0) {
				if (!(MechStatus(mech) & TORSO_RIGHT)) {
				    if (MechStatus(mech) & (TORSO_LEFT |
					    TORSO_RIGHT))
					mech_rotatetorso(a->mynum,
					    mech, my2string("center"));
				    mech_rotatetorso(a->mynum, mech,
					my2string("right"));
				}
			    } else {
				if (!(MechStatus(mech) & TORSO_LEFT)) {
				    if (MechStatus(mech) & (TORSO_LEFT |
					    TORSO_RIGHT))
					mech_rotatetorso(a->mynum,
					    mech, my2string("center"));
				    mech_rotatetorso(a->mynum, mech,
					my2string("left"));
				}
			    }
			    if (MechStatus(mech) & FLIPPED_ARMS)
				mech_fliparms(a->mynum, mech,
				    my2string(""));
			}
		    } else {
			/* Do we have a turret? */
			if (MechType(mech) == CLASS_MECH ||
			    MechType(mech) == CLASS_MW ||
			    MechType(mech) == CLASS_BSUIT || is_aero(mech)
			    || !GetSectInt(mech, TURRET))
			    continue;
			/* Hrm, is the gun on turret? */
			if (loop != TURRET)
			    continue;
			/* We've a turret to turn! Whoopee! */
			sprintf(buf, "%d", b);
			mech_turret(a->mynum, mech, buf);
		    }
		}
		if (MechTarget(mech) != targets[i]->mynum) {
		    sprintf(buf, "%c%c", MechID(targets[i])[0],
			MechID(targets[i])[1]);
		    mech_settarget(a->mynum, mech, buf);
		    locked = 1;
		}
		if (targetscore[i] > (10 + ((MechTarget(mech) !=
				targets[i]->mynum) ? 0 : 2) + Number(-1,
			    Number(0, 2))))
		    break;
		if (targetscore[i] > 5 &&
		    (((targetscore[i] >= (7 + (Number(0, 5)))) &&
			    MechWeapons[weaparray[ii]].ammoperton) ||
			(Locking(mech) && Number(1, 6) != 5) ||
			(IsRunning(MechSpeed(mech), MMaxSpeed(mech)) &&
			    Number(1, 4) == 4)))
		    break;
		if (!IsRunning(MechSpeed(mech), MMaxSpeed(mech)) &&
		    IsRunning(MechDesiredSpeed(mech), MMaxSpeed(mech))) {
		    cheating = 1;
		    save = MechDesiredSpeed(mech);
		    MechDesiredSpeed(mech) = MechSpeed(mech);
		}
		if (Uncon(targets[i])) {
		    if (MechAim(mech) == NUM_SECTIONS) {
			sprintf(buf, "h");
			mech_target(a->mynum, mech, buf);
		    }
		} else if (MechAim(mech) != NUM_SECTIONS) {
		    sprintf(buf, "-");
		    mech_target(a->mynum, mech, buf);
		}
		sprintf(buf, "%d", weapnum - 1);
		mech_fireweapon(a->mynum, mech, buf);
		if (cheating) {
		    cheating = 0;
		    MechDesiredSpeed(mech) = save;
		}
		if (global_kill_cheat) {
		    AUTOEVENT(a, EVENT_AUTOGUN, auto_gun_event, 1, 0);
		    return;
		}
		if (WpnIsRecycling(mech, loop, critical[ii])) {
		    fired++;
		    rt += MechWeapons[weaparray[ii]].heat;
		    break;
		}
	    }
	}
    }
    AUTOEVENT(a, EVENT_AUTOGUN, auto_gun_event, AUTOGUN_TICK, 0);
}
