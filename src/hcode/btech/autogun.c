
/*
 * $Id: autogun.c,v 1.5 2005/08/03 21:40:54 av1-op Exp $
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

/*! \todo { Add more code to the sensor system so the AI can be
 * more aware of its terrain } */

#include "mech.h"
#include "mech.events.h"
#include "autopilot.h"
#include "failures.h"
#include "mech.sensor.h"
#include "p.autogun.h"
#include "p.bsuit.h"
#include "p.glue.h"
#include "p.mech.sensor.h"
#include "p.mech.utils.h"
#include "p.mech.physical.h"
#include "p.mech.combat.h"
#include "p.mech.advanced.h"
#include "p.mech.bth.h"

#define AUTOGUN_TICK 1      /* Every second */
#define AUTOGS_TICK  30     /* Every 30 seconds or so */
#define MAXHEAT      6      /* Last heat we let heat go to */
#define MAX_TARGETS  100

int global_kill_cheat = 0;

static char *my2string(const char * old)
{
    static char new[64];

    strncpy(new, old, 63);
    new[63] = '\0';
    return new;
}

/* Function to determine if there are any slites affecting the AI */
int SearchLightInRange(MECH * mech, MAP *map)
{
    MECH *t;
    int i;

    /* Make sure theres a valid mech or map */
    if (!mech || !map)
        return 0;

    /* Loop through all the units on the map */
    for (i = 0; i < map->first_free; i++) {
        /* No units on the map */
        if (!(t = FindObjectsData(map->mechsOnMap[i])))
            continue;
        /* The unit doesn't have slite on */
        if (!(MechSpecials(t) & SLITE_TECH) || MechCritStatus(mech) & SLITE_DEST)
            continue;
        
        /* Is the mech close enough to be affected by the slite */
        if (FaMechRange(t, mech) < LITE_RANGE) {
        /* Returning true, but let's differentiate also between being in-arc. */
            if ((MechStatus(t) & SLITE_ON) && 
                    InWeaponArc(t, MechFX(mech), MechFY(mech)) & FORWARDARC) {
                if (!(map->LOSinfo[t->mapnumber][mech->mapnumber] & MECHLOSFLAG_BLOCK))
                    /* Slite on and, arced, and LoS to you */
                    return 3;
                else
                    /* Slite on, arced, but LoS blocked */
                    return 4;
            } else if (!MechStatus(t) & SLITE_ON && 
                    InWeaponArc(t, MechFX(mech), MechFY(mech)) & FORWARDARC) {
                if (!(map->LOSinfo[t->mapnumber][mech->mapnumber] & MECHLOSFLAG_BLOCK))
                    /* Slite off, arced, and LoS to you */
                    return 5;
                else
                    /* Slite off, arced, and LoS blocked */
                    return 6;
            }
            /* Slite is in range of you, but apparently not arced on you. 
             * Return tells wether on or off */
            return (MechStatus(t) & SLITE_ON ? 1 : 2);
        }

    }
    return 0;
}

/* Function to determine if the AI should use V or L sensor */
int PrefVisSens(MECH * mech, MAP * map, int slite, MECH * target)
{
    /* No map or mech so use default till we get put somewhere */
    if (!mech || !map)
        return SENSOR_VIS;

    /* Ok the AI is lit or using slite so use V */
    if (MechStatus(mech) & SLITE_ON || MechCritStatus(mech) & SLITE_LIT)
        return SENSOR_VIS;
    /* The target is lit so use V */
    if (target && MechCritStatus(target) & SLITE_LIT)
        return SENSOR_VIS;

    /* Ok if its night/dawn/dusk and theres no slite use L */
    if (map->maplight <= 1 && slite != 3 && slite != 5)
        return SENSOR_LA;

    /* Default sensor */
    return SENSOR_VIS;
}

/* EVENT system to let the AI rotate through different sets
 * of sensors depending on the conditions and target */
void auto_gun_sensor_event(MUXEVENT * e)
{
    AUTO *a = (AUTO *) e->data;
    MECH *mech = (MECH *) a->mymech;
    MECH *target = NULL;
    MAP *map;
    int flag = (int) e->data2;
    char buf[16];
    int wanted_s[2];
    int rvis;
    int slite, prefvis;
    float trng;
    int set = 0;

    /* Make sure its a MECH Xcode Object and the AI is
     * an AUTOPILOT Xcode Object */
    if (!IsMech(mech->mynum) || !IsAuto(a->mynum))
        return;

    /* Mech is dead so stop trying to shoot things */
    if (Destroyed(mech)) {
        DoStopGun(a);
        return;
    }

    /* Mech isn't started */
    if (!Started(mech)) {
        Zombify(a);
        return;
    }

    /* The mech is using user defined sensors so don't try
     * and change them */
    if (a->flags & AUTOPILOT_LSENS)
        return;

    /* Get the map */
    map = getMap(mech->mapindex);

    /* No Map */
    if (!map) {
        Zombify(a);
        return;
    }

    /* Get the target if there is one */
    if (MechTarget(mech) > 0)
        target = getMech(MechTarget(mech));

    /* Checks to see if there is slite, and what types of vis
     * and which visual sensor (V or L) to use */ 
    slite = (map->mapvis != 2 ? SearchLightInRange(mech, map) : 0);
    rvis = (map->maplight ? (map->mapvis) : (map->mapvis * (slite ? 1 : 3)));
    prefvis = PrefVisSens(mech, map, slite, target);

    /* Is there a target */
    if (target) {
        trng = FaMechRange(mech, target);

        /* If the target is running hot and is close switch to IR */
        if (!set && HeatFactor(target) > 35 && (int) trng < 15) {
            wanted_s[0] = SENSOR_IR;
            wanted_s[1] = ((MechTons(target) >= 60) ? SENSOR_EM : prefvis);
            set++;
        }

        /* If the target is BIG and close enough, use EM */
        if (!set && MechTons(target) >= 60 && (int) trng <= 20) {
            wanted_s[0] = SENSOR_EM;
            wanted_s[1] = SENSOR_IR;
            set++;
        }

        /* If the target is flying switch to Radar */
        if (!set && !Landed(target) && FlyingT(target)) {
            wanted_s[0] = SENSOR_RA;
            wanted_s[1] = prefvis;
            set++;
        }

        /* If the target is really close and the unit has BAP, use it */
        if (!set && (int) trng <= 4 && MechSpecials(mech) & BEAGLE_PROBE_TECH 
                && !(MechCritStatus(mech) & BEAGLE_DESTROYED)) {
            wanted_s[0] = SENSOR_BAP;
            wanted_s[1] = SENSOR_BAP;
            set++;
        }

        /* If the target is really close and the unit has Bloodhound, use it */
        if (!set && (int) trng <= 8 && MechSpecials2(mech) & BLOODHOUND_PROBE_TECH 
                && !(MechCritStatus(mech) & BLOODHOUND_DESTROYED)) {
            wanted_s[0] = SENSOR_BHAP;
            wanted_s[1] = SENSOR_BHAP;
            set++;
        }

        /* Didn't stop at any of the others so use selected visual sensors */
        if (!set) {
            wanted_s[0] = prefvis;
            wanted_s[1] = (rvis <= 15 ? SENSOR_EM : prefvis);
            set++;
        }
    
    }

    /* Ok no target and no sensors set yet so lets go for defaults */
    if (!set) {
        if (rvis <= 15) {
            /* Vis is less then or equal to 15 so go to E I for longer range */
            wanted_s[0] = SENSOR_EM;
            wanted_s[1] = SENSOR_IR;
        } else {
            /* Ok lets go with default visual sensors */
            wanted_s[0] = prefvis;
            wanted_s[1] = prefvis;
        }
    }

    /* Check to make sure valid sensors are selected and then set them */
    if (wanted_s[0] >= SENSOR_VIS && wanted_s[0] <= SENSOR_BHAP &&
            wanted_s[1] >= SENSOR_VIS && wanted_s[1] <= SENSOR_BHAP &&
            (MechSensor(mech)[0] != wanted_s[0] || MechSensor(mech)[1] != wanted_s[1])) {

        wanted_s[0] = BOUNDED(SENSOR_VIS, wanted_s[0], SENSOR_BHAP);
        wanted_s[1] = BOUNDED(SENSOR_VIS, wanted_s[1], SENSOR_BHAP);

        memset(buf, '\0', sizeof(char) * 16);
        sprintf(buf, "%c  %c", 
                sensors[wanted_s[0]].matchletter[0],
                sensors[wanted_s[1]].matchletter[0]);

        mech_sensor(a->mynum, mech, buf);

    }

    if (!flag)
        AUTOEVENT(a, EVENT_AUTOGS, auto_gun_sensor_event, AUTOGS_TICK, 0);
}

/* Function to return the Average weapon range for a given unit */
int AverageWpnRange(MECH * mech)
{
    int loop, count, i;
    unsigned char weaparray[MAX_WEAPS_SECTION];
    unsigned char weapdata[MAX_WEAPS_SECTION];
    int critical[MAX_WEAPS_SECTION];
    int tot_weap = 0, tot_rng = 0;

    /* Not a unit so return error */
    if (!mech)
        return 1;

    /* Go through all the sections and find guns */
    for (loop = 0; loop < NUM_SECTIONS; loop++) {
        count = FindWeapons(mech, loop, weaparray, weapdata, critical);

        /* No guns here so go to next section */
        if (count <= 0)
            continue;

        /* For each weapon found lets check them */
        for (i = 0; i < count; i++) {
            if (IsAMS(weaparray[i]))
                continue;

            /* Is the weapon working */
            if (WeaponIsNonfunctional(mech, loop, critical[i], 
                    GetWeaponCrits(mech, Weapon2I(weaparray[i]))) > 0)
                continue;

            tot_weap++;
            tot_rng += EGunRangeWithCheck(mech, loop, weaparray[i]);
        }

    }

    return (tot_rng / tot_weap);
}

/* Function to help the AI determine what to shoot first */
int TargetScore(MECH * mech, MECH * target, int range) {
/*
    int avg_rng;
    int bv;

    if (!mech || !target)
        return 0;

    bv = MechBV(target);
    avg_rng = AverageWpnRange(mech); 

    return (int) ((float) bv * ((float) 1.0 - ((float) range / (float) avg_rng)));
*/
    return MechBV(target);
}

/* Event system to for the AI to constantly shoot at stuff */
void auto_gun_event(MUXEVENT * e)
{
    AUTO *a = (AUTO *) e->data;
    MECH *mech = (MECH *) a->mymech;
    MECH *targets[MAX_TARGETS], *t;
    int targetscore[MAX_TARGETS];
    int targetrange[MAX_TARGETS];
    int targetbth[MAX_TARGETS];
    int i, j, k, f, m;
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
    int bth, score;
    dbref c3Ref;

    if (!IsMech(mech->mynum) || !IsAuto(a->mynum))
        return;

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

    if (MechType(mech) == CLASS_MECH &&
            (MechPlusHeat(mech) - MechActiveNumsinks(mech)) > MAXHEAT) {
        AUTOEVENT(a, EVENT_AUTOGUN, auto_gun_event, AUTOGUN_TICK, 0);
        return;
    }
#endif

    /* OODing so don't shoot any guns */
    if (OODing(mech)) {
        AUTOEVENT(a, EVENT_AUTOGUN, auto_gun_event, AUTOGUN_TICK, 0);
        return;
    }

    /* Cycle through possible targets and pick something to shoot */
    global_kill_cheat = 0;
    for (i = 0; i < map->first_free; i++)
        if (i != mech->mapnumber && (j = map->mechsOnMap[i]) > 0) {
            if (!(t = getMech(j)))
                continue;
            if (Destroyed(t))
                continue;
            if (MechStatus(t) & COMBAT_SAFE)
                continue;
            if (MechTeam(t) == MechTeam(mech) && t->mynum != a->targ)
                continue;
/*          if (!(MechToMech_LOSFlag(map, mech, t) & MECHLOSFLAG_SEEN))
                continue; */
            if ((targetrange[target_count] = (int) FlMechRange(map, mech, t)) > 30)
                continue;
            if (MechType(mech) == CLASS_BSUIT && 
                    MechSwarmTarget(mech) > 0 && 
                    MechSwarmTarget(mech) != t->mynum)
                continue;
            ttarget_count++;
            if (!(a->targ <= 0 || t->mynum == a->targ))
                continue;
            if (t->mynum == MechTarget(mech))
                locktarg_num = target_count;

            /* Ok we got a target lets see if we can do a physical attack */
            targets[target_count] = t;
            if (MechType(mech) == CLASS_MECH && (targetrange[target_count] < 1.0)) {
                char ib[6], tb[10];
                int st_ra = SectIsDestroyed(mech, RARM);
                int st_la = SectIsDestroyed(mech, LARM);
                int st_ll = SectIsDestroyed(mech, LLEG);
                int st_rl = SectIsDestroyed(mech, RLEG);
                int iwa, iwa_nt, ts;

                snprintf(ib, sizeof(char) * 6, "%s", MechIDS(t, 0));
                ts = MechStatus(mech) & (TORSO_LEFT|TORSO_RIGHT);
                MechStatus(mech) &= ~ts;
                iwa_nt = InWeaponArc(mech, MechFX(t), MechFY(t));
                MechStatus(mech) |= ts;
                iwa = InWeaponArc(mech, MechFX(t), MechFY(t));

                /* Check for physical attacks */
                if (iwa & FORWARDARC &&
                        !MechSections(mech)[RARM].recycle &&
                        !MechSections(mech)[LARM].recycle &&
                        !MechSections(mech)[RLEG].recycle &&
                        !MechSections(mech)[LLEG].recycle &&
                        MechMove(mech) == MOVE_BIPED) {

                    /* Check for physical attack weapons */
                    /* RARM */
                    if (!SectHasBusyWeap(mech, RARM) && !st_ra) {
                        sprintf(tb, "r %s", ib);
                        if (have_axe(mech, RARM))
                            mech_axe(a->mynum, mech, tb);
                        /* else if (have_mace(mech, RARM)) */
                            /*! \todo {Add in mace code here} */
                        else if (have_sword(mech, RARM))
                            mech_sword(a->mynum, mech, tb);
                    }
                    
                    /* LARM */
                    if (!SectHasBusyWeap(mech, LARM) && !st_la) {
                        sprintf(tb, "l %s", ib);
                        if (have_axe(mech, LARM))
                            mech_axe(a->mynum, mech, tb);
                        /* else if (have_mace(mech, LARM)) */
                            /*! \todo {Add in mace code here} */
                        else if (have_sword(mech, LARM))
                            mech_sword(a->mynum, mech, tb);
                    }
                    
                    /* Going to do Kick first but can easily switch
                     * the code for punch first */
                    if ((!SectHasBusyWeap(mech, RLEG) || !SectHasBusyWeap(mech, LLEG)) && 
                            !st_rl && !st_ll) {
                        int rleg_bth = 0, lleg_bth = 0;

                        /* Check the RLEG for any crits or weaps cycling */
                        if (!SectHasBusyWeap(mech, RLEG)) {
                            if (!OkayCritSectS(RLEG, 0, SHOULDER_OR_HIP))
                                rleg_bth += 3;
                            if (!OkayCritSectS(RLEG, 1, UPPER_ACTUATOR))
                                rleg_bth++;
                            if (!OkayCritSectS(RLEG, 2, LOWER_ACTUATOR))
                                rleg_bth++;
                            if (!OkayCritSectS(RLEG, 3, HAND_OR_FOOT_ACTUATOR))
                                rleg_bth++;
                        } else {
                            rleg_bth = 99;
                        }

                        /* Check the LLEG for any crits or weaps cycling */
                        if (!SectHasBusyWeap(mech, LLEG)) {
                            if (!OkayCritSectS(LLEG, 0, SHOULDER_OR_HIP))
                                lleg_bth += 3;
                            if (!OkayCritSectS(LLEG, 1, UPPER_ACTUATOR))
                                lleg_bth++;
                            if (!OkayCritSectS(LLEG, 2, LOWER_ACTUATOR))
                                lleg_bth++;
                            if (!OkayCritSectS(LLEG, 3, HAND_OR_FOOT_ACTUATOR))
                                lleg_bth++;
                        } else {
                            rleg_bth = 99;
                        }

                        /* Now kick depending on which one would be better 
                         * to kick with */
                        if (rleg_bth <= lleg_bth) {
                            sprintf(tb, "r %s", ib);
                        } else {
                            sprintf(tb, "l %s", ib);
                        }
                        mech_kick(a->mynum, mech, tb);
                    }

                    /* Now check the arms and go for punches */
                    if ((!SectHasBusyWeap(mech, RARM) && 
                            !MechSections(mech)[RARM].recycle && !st_ra) ||
                            (!SectHasBusyWeap(mech, LARM) &&
                            !MechSections(mech)[LARM].recycle && !st_la)) {
                                
                        int rarm_rdy = 0, larm_rdy = 0;

                        if (!SectHasBusyWeap(mech, RARM) &&
                                !MechSections(mech)[RARM].recycle &&
                                !st_ra)
                            rarm_rdy = 1;
                        else if (!SectHasBusyWeap(mech, LARM) &&
                                !MechSections(mech)[LARM].recycle &&
                                !st_la)
                            larm_rdy = 1;

                        if (rarm_rdy == 1 && larm_rdy == 1)
                            sprintf(tb, "b %s", ib);
                        else if (rarm_rdy == 1)
                            sprintf(tb, "r %s", ib);
                        else
                            sprintf(tb, "l %s", ib);

                        mech_punch(a->mynum, mech, tb);
                    }

                }

                /* Now if we a quad ... */
                if ((MechMove(mech) == MOVE_QUAD && iwa_nt & FORWARDARC) &&
                        !MechSections(mech)[RARM].recycle && 
                        !MechSections(mech)[LARM].recycle &&
                        !MechSections(mech)[RLEG].recycle && 
                        !MechSections(mech)[LLEG].recycle &&
                        !st_ll && !st_rl) {

                    int rleg_bth = 0, lleg_bth = 0;

                    /* Check the RLEG for any crits or weaps cycling */
                    if (!SectHasBusyWeap(mech, RLEG)) {
                        if (!OkayCritSectS(RLEG, 0, SHOULDER_OR_HIP))
                            rleg_bth += 3;
                        if (!OkayCritSectS(RLEG, 1, UPPER_ACTUATOR))
                            rleg_bth++;
                        if (!OkayCritSectS(RLEG, 2, LOWER_ACTUATOR))
                            rleg_bth++;
                        if (!OkayCritSectS(RLEG, 3, HAND_OR_FOOT_ACTUATOR))
                            rleg_bth++;
                    } else {
                        rleg_bth = 99;
                    }

                    /* Check the LLEG for any crits or weaps cycling */
                    if (!SectHasBusyWeap(mech, LLEG)) {
                        if (!OkayCritSectS(LLEG, 0, SHOULDER_OR_HIP))
                            lleg_bth += 3;
                        if (!OkayCritSectS(LLEG, 1, UPPER_ACTUATOR))
                            lleg_bth++;
                        if (!OkayCritSectS(LLEG, 2, LOWER_ACTUATOR))
                            lleg_bth++;
                        if (!OkayCritSectS(LLEG, 3, HAND_OR_FOOT_ACTUATOR))
                            lleg_bth++;
                    } else {
                        rleg_bth = 99;
                    }

                    /* Now kick depending on which one would be better 
                     * to kick with */
                    if (rleg_bth <= lleg_bth) {
                        sprintf(tb, "r %s", ib);
                    } else {
                        sprintf(tb, "l %s", ib);
                    }
                    mech_kick(a->mynum, mech, tb);
                }

            } else if ((MechType(mech) == CLASS_BSUIT) && 
                    (targetrange[target_count] < 1.0)) {
                /* Our unit is a Bsuit so lets try a Bsuit attack */
                /*! \todo { Add check for BTH for swarmage } */
                char tb[6];
                sprintf(tb, "%s", MechIDS(t, 0));
                if (MechJumpSpeed(mech) > 0)
                    bsuit_swarm(a->mynum, mech, tb);
                else
                    bsuit_attackleg(a->mynum, mech, tb);

            }
            target_count++;

        }
    /* End of for loop for looking for targets and doing physical attacks */

/*! \todo { Finish & Fix the AI roam code}
    if (a->flags & AUTOPILOT_ROAMMODE && target_count == 0 && 
            a->commands[a->program_counter] != GOAL_ROAM) {
        auto_disengage(a->mynum, a, "");
        auto_delcommand(a->mynum, a, "-1");
        PG(a) = 0;
        auto_addcommand(a->mynum, a, tprintf("autogun"));
        auto_addcommand(a->mynum, a, tprintf("roam 0 0"));
        auto_engage(a->mynum, a, "");
    }
*/

    if (a->flags & AUTOPILOT_SWARMCHARGE) {
        if (MechSwarmTarget(mech) > 0)
            a->flags &= ~AUTOPILOT_SWARMCHARGE;
        else
            if (MechTarget(mech) > 0) {
                AUTOEVENT(a, EVENT_AUTOGUN, auto_gun_event, AUTOGUN_TICK, 0);
                return;
            }
    }

    if (((muxevent_tick % 4) != 0) || (MechType(mech) == CLASS_MECH && 
            (MechPlusHeat(mech) - MechActiveNumsinks(mech)) > MAXHEAT)) {
        AUTOEVENT(a, EVENT_AUTOGUN, auto_gun_event, AUTOGUN_TICK, 0);
        return;
    }

    /* No targets lets just idle */
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

        /* Loop through all the weapons and try and fire them */
        for (ii = 0; ii < count; ii++) {
            weapnum++;
            if (IsAMS(weaparray[ii]))
                continue;
#if 0
            if (PartIsNonfunctional(mech, loop, critical[ii]))
                continue;
            if (PartTempNuke(mech, loop, critical[ii]))
                continue;
#else
            if (WeaponIsNonfunctional(mech, loop, critical[ii], 
                    GetWeaponCrits(mech, Weapon2I(weaparray[ii]))) > 0)
                continue;
#endif
            if (weapdata[ii])
                continue;
            /*! \todo { Exile uses a different system here using a function
             * called GunStat and MTX_HEAT_MODE, should look into adding
             * this } */
            if (MechType(mech) == CLASS_MECH &&
                    (rt + MechWeapons[weaparray[ii]].heat +
                    MechPlusHeat(mech) - MechActiveNumsinks(mech)) > MAXHEAT)
                continue;

            /* Whee, it's not recycling or anything -> it's ready to fire! */
            /*! \todo { Add in stuff here as well as with the other
             * EGunRange* stuff to allow various weapon modes } */
            /* Cycling through different targets getting a score for each one */
            wMaxGunRange = EGunRangeWithCheck(mech, loop, weaparray[ii]);
            for (i = 0; i < target_count; i++) {
                if (wMaxGunRange > targetrange[i]) {
                    score = TargetScore(mech, targets[i], targetrange[i]);
                    bth = FindNormalBTH(mech, map, loop, critical[ii],
                        weaparray[ii], (float) targetrange[i], targets[i],
                        1000, &c3Ref);
                    targetscore[i] = (score / (MAX(1, bth)));
                    targetbth[i] = bth;
                    /*
                    SendDebug(tprintf("TargetScoring - #%d to #%d scores %d (BV %d BTH %d)", 
                                mech->mynum, targets[i]->mynum, targetscore[i], score, bth));
                                */
                } else {
                    targetscore[i] = 1; /* 999 */
                    targetbth[i] = 20;
                }
            }
            /* Sort the targets based on score */
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
                        m = targetbth[i];
                        targets[i] = targets[j];
                        targetscore[i] = targetscore[j];
                        targetrange[i] = targetrange[j];
                        targetbth[i] = targetbth[j];
                        targets[j] = t;
                        targetscore[j] = k;
                        targetrange[j] = f;
                        targetbth[j] = m;
                    }

            /* Now cycle through the targets checking our arc/torsodir/turret */
            for (i = 0; i < target_count; i++) {
                /* This is .. simple, for now: We don't bother with 10+/12+ 
                   BTHs (depending on if locked or not) */
                if (locktarg_num >= 0 && i > locktarg_num)
                    break;
                /* Modified to account for BV over BTH - courtesy of DJ */
                if (targetbth[i] > 13 || targetrange[i] > wMaxGunRange)
                    continue;
                /*! \todo { Add this whenever we add coolant guns to the system } 
                if (MechTeam(mech) == MechTeam(targets[i]) && !IsCoolant(weaparray[ii]))
                    continue;
                */
                if (!IsInWeaponArc(mech, MechFX(targets[i]),
                        MechFY(targets[i]), loop, critical[ii])) {
                    b = FindBearing(MechFX(mech), MechFY(mech),
                    MechFX(targets[i]), MechFY(targets[i]));

                    /* Ok now we rotate torso or turn our turret to nail the guy */
                    if (MechType(mech) == CLASS_MECH) {
                        h = MechFacing(mech);
                        if (GetPartFireMode(mech, loop, critical[ii]) & REAR_MOUNT)
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
                            if (MechStatus(mech) & (TORSO_LEFT | TORSO_RIGHT))
                                mech_rotatetorso(a->mynum, mech, 
                                        my2string("center"));
                            if (!(MechStatus(mech) & FLIPPED_ARMS))
                                mech_fliparms(a->mynum, mech, my2string(""));
                        } else {
                            if (abs(h) < 60) {
                                if (MechStatus(mech) & (TORSO_LEFT | TORSO_RIGHT))
                                    mech_rotatetorso(a->mynum, mech, 
                                            my2string("center"));
                            } else if (h < 0) {
                                if (!(MechStatus(mech) & TORSO_RIGHT)) {
                                    if (MechStatus(mech) & (TORSO_LEFT | TORSO_RIGHT))
                                        mech_rotatetorso(a->mynum, mech, 
                                                my2string("center"));
                                    mech_rotatetorso(a->mynum, mech, 
                                            my2string("right"));
                                }
                            } else {
                                if (!(MechStatus(mech) & TORSO_LEFT)) {
                                    if (MechStatus(mech) & (TORSO_LEFT | TORSO_RIGHT))
                                        mech_rotatetorso(a->mynum, mech, 
                                                my2string("center"));
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
                    if (a->flags & AUTOPILOT_CHASETARG && a->targ >= -1) {
                        /* Do I salivate over my contacts? */
                        auto_disengage(a->mynum, a, "");
                        auto_delcommand(a->mynum, a, "-1");
                        PG(a) = 0;
                        auto_addcommand(a->mynum, a, tprintf("autogun"));
                        auto_addcommand(a->mynum, a, tprintf("dumbfollow %d", targets[i]->mynum));
                        auto_engage(a->mynum, a, "");
                    }
                }
/*
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
*/
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
            } /* End of heading/turret for loop */
        } /* End of weapon loop */
    }
    AUTOEVENT(a, EVENT_AUTOGUN, auto_gun_event, AUTOGUN_TICK, 0);
}
