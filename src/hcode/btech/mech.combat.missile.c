
/*
 * $Id: mech.combat.missile.c,v 1.2 2005/01/15 16:57:14 kstevens Exp $
 *
 * Author: Cord Awtry <kipsta@mediaone.net>
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Based on work that was:
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2000 Thomas Wouters
 */

#include <stdio.h>
#include <stdlib.h>

#include "mech.h"
#include "btmacros.h"
#include "mech.combat.h"
#include "mech.events.h"
#include "p.pcombat.h"
#include "p.mech.combat.h"
#include "p.mech.combat.misc.h"
#include "p.mech.combat.missile.h"
#include "p.mech.damage.h"
#include "p.mech.ecm.h"
#include "p.mech.hitloc.h"
#include "p.mech.los.h"
#include "p.mech.utils.h"

int pilot_override;

void Missile_Hit(MECH * mech,
    MECH * target,
    int hitX,
    int hitY,
    int isrear,
    int iscritical,
    int weapindx,
    int fireMode,
    int ammoMode,
    int num_missiles_hit, int damage, int salvo_size, int LOS, int bth,
    int tIsSwarmAttack)
{
    int orig_num_missiles = num_missiles_hit;
    int this_time;
    int this_damage;
    int total_damage = 0;
    int clear_damage = 0;
    int hitloc;
    int tCheckWoodsDamageDecrement = 0;
    MAP *mech_map = getMap(mech->mapindex);
    char buf[SBUF_SIZE];

    total_damage = num_missiles_hit * damage;

    if (target && mudconf.btech_moddamagewithwoods &&
    IsForestHex(mech_map, MechX(target), MechY(target)) &&
    (fireMode > -1) && (ammoMode > -1) &&
    ((MechZ(target) - 2) <= Elevation(mech_map, MechX(target),
        MechY(target)))) {
    tCheckWoodsDamageDecrement = 1;
    clear_damage = total_damage;

    if (GetRTerrain(mech_map, MechX(target),
        MechY(target)) == LIGHT_FOREST)
        total_damage -= 2;
    else if (GetRTerrain(mech_map, MechX(target),
        MechY(target)) == HEAVY_FOREST)
        total_damage -= 4;

    if (total_damage <= 0)
        num_missiles_hit = 0;
    else
        num_missiles_hit = total_damage / damage;

    possibly_ignite_or_clear(mech, weapindx, ammoMode, clear_damage,
        MechX(target), MechY(target), 1);

    strcpy(buf, "");

    if (IsMissile(weapindx))
        sprintf(buf, "%s%s", "missile",
        orig_num_missiles > 1 ? "s" : "");
    else if (ammoMode & LBX_MODE)
        sprintf(buf, "%s%s", "pellet",
        orig_num_missiles > 1 ? "s" : "");
    else if ((fireMode && ULTRA_MODE) || (fireMode && RFAC_MODE) ||
        (fireMode && RAC_MODES))
        sprintf(buf, "%s%s", "slug", orig_num_missiles > 1 ? "s" : "");
    else
        sprintf(buf, "%s", "damage");

    mech_notify(mech, MECHALL,
        tprintf("%s %s %s absorbed by the trees!",
        (orig_num_missiles == 1 ? "The" : num_missiles_hit ==
            0 ? "All of the" : "Some of the"), buf,
        (orig_num_missiles == 1 ? "is" : "are")));
    mech_notify(target, MECHALL, tprintf("The trees absorb %s %s",
        ((orig_num_missiles == 1) ||
            (num_missiles_hit == 0) ? "the" : "some of the"),
        buf));
    }

    while (num_missiles_hit) {
    this_time = MIN(salvo_size, num_missiles_hit);
    this_damage = this_time * damage;

    if (target) {
        hitloc = FindTargetHitLoc(mech, target, &isrear, &iscritical);

        DamageMech(target, mech, LOS, GunPilot(mech), hitloc, isrear,
        iscritical, pc_to_dam_conversion(target, weapindx,
            this_damage), 0, weapindx, bth, -1, 0, tIsSwarmAttack);
    } else {
        hex_hit(mech, hitX, hitY, weapindx, ammoMode, this_damage, 1);
    }

    num_missiles_hit -= this_time;
    }
}

int MissileHitIndex(MECH * mech,
    MECH * hitMech, int weapindx, int wSection, int wCritSlot)
{
    int hit_roll;
    int r1, r2, r3, rtmp;
    int tHotloading =
    HotLoading(weapindx, GetPartFireMode(mech, wSection, wCritSlot));
    int wRollInc = 0;
    int wFinalRoll = 0;
    int tUseArtemisBonus =
    GetPartAmmoMode(mech, wSection, wCritSlot) & ARTEMIS_MODE;
    int tUseNARCBonus = 0;

    if (hitMech) {
    if (ECMProtected(hitMech) || AngelECMProtected(hitMech)) {
        tUseArtemisBonus = 0;
        tUseNARCBonus = 0;
    } else {
        tUseNARCBonus =
        (GetPartAmmoMode(mech, wSection, wCritSlot) & NARC_MODE) &&
        (checkAllSections(hitMech, NARC_ATTACHED) ||
        checkAllSections(hitMech, INARC_HOMING_ATTACHED));
    }
    }

    if (AnyECMDisturbed(mech)) {
    tUseArtemisBonus = 0;
    tUseNARCBonus = 0;
    }

    /*
       * Figure out the modifiers to the roll table for missiles
     */
    if (IsMissile(weapindx) && (tUseArtemisBonus || tUseNARCBonus))
    wRollInc = 2;

    /* Roll 3 times... if we're hotloading, we'll use the 2 lowest */
    r1 = Number(1, 6);
    r2 = Number(1, 6);
    r3 = Number(1, 6);

    if (r1 > r2)
    Swap(r1, r2);
    if (r2 > r3)
    Swap(r2, r3);

    if (tHotloading)
    hit_roll = r1 + r2 - 2;
    else
    hit_roll = Roll() - 2;

    if ((!hitMech || (hitMech && !AngelECMProtected(hitMech))) &&
    !AngelECMDisturbed(mech) &&
    (MechWeapons[weapindx].special & STREAK)) {
    return 10;
    }

    if (wRollInc)
    hit_roll = hit_roll + wRollInc;

    wFinalRoll = MAX(MIN(hit_roll, 10), 0);

    return wFinalRoll;
}

int MissileHitTarget(MECH * mech,
    int weapindx,
    int wSection,
    int wCritSlot,
    MECH * hitMech, int hitX, int hitY, int LOS, int baseToHit, int roll,
    int incoming, int tIsSwarmAttack, int player_roll)
{
    int isrear = 0, iscritical = 0;
    int AMStype, ammoLoc, ammoCrit;
    int AMSShotdown = 0;
    int hit;
    int i, j = -1, k, l = 0;
    int wNARCType = 0;
    int ammoMode = GetPartAmmoMode(mech, wSection, wCritSlot);
    int tIsInferno = (ammoMode & INFERNO_MODE);
    int wNARCHitLoc = 0;
    int tIsRear = 0;
    char strLocName[30];

    /* Check to see if we're a NARC or iNARC launcher firing homing missiles */
    if (IsMissile(weapindx)) {
    if ((MechWeapons[weapindx].special & NARC) &&
        !(GetPartAmmoMode(mech, wSection, wCritSlot) & NARC_MODE))
        wNARCType = 1;
    else if ((MechWeapons[weapindx].special & INARC) &&
        !(GetPartAmmoMode(mech, wSection,
            wCritSlot) & INARC_EXPLO_MODE)) {

        if (GetPartAmmoMode(mech, wSection,
            wCritSlot) & INARC_HAYWIRE_MODE)
        wNARCType = 3;
        else if (GetPartAmmoMode(mech, wSection,
            wCritSlot) & INARC_ECM_MODE)
        wNARCType = 4;
        else
        wNARCType = 2;
    }

    /* Prefill our AMS data */
    if (hitMech && (!((ammoMode & SWARM_MODE) ||
            (ammoMode & SWARM1_MODE) || (ammoMode & MINE_MODE)))) {
        if (LocateAMSDefenses(hitMech, &AMStype, &ammoLoc, &ammoCrit))
        AMSShotdown =
            AMSMissiles(mech, hitMech, wNARCType ? 1 : incoming,
            AMStype, ammoLoc, ammoCrit, LOS, roll >= baseToHit);
    }

    if (wNARCType) {
        if (roll >= baseToHit) {
        if (hitMech) {
            if (AMSShotdown > 0) {
            if (LOS)
                mech_notify(mech, MECHALL,
                "The pod is shot down by the target!");

            mech_notify(hitMech, MECHALL,
                "Your Anti-Missile System activates and shoots down the incoming pod!");

            return 0;
            }

            wNARCHitLoc = findNARCHitLoc(mech, hitMech, &tIsRear);

            /* sanity check */
            if (wNARCHitLoc < 0) {
            mech_notify(mech, MECHALL,
                "Your NARC Beacon attaches to the target!");

            return 0;
            }

            ArmorStringFromIndex(wNARCHitLoc, strLocName,
            MechType(hitMech), MechMove(hitMech));

            if (wNARCType == 1)
            MechSections(hitMech)[wNARCHitLoc].specials |=
                NARC_ATTACHED;
            else if (wNARCType == 2)
            MechSections(hitMech)[wNARCHitLoc].specials |=
                INARC_HOMING_ATTACHED;
            else if (wNARCType == 3) {
            MechSections(hitMech)[wNARCHitLoc].specials |=
                INARC_HAYWIRE_ATTACHED;

            mech_notify(hitMech, MECHALL,
                "Your targetting systems goes a bit haywire!");
            } else if (wNARCType == 4) {
            MechSections(hitMech)[wNARCHitLoc].specials |=
                INARC_ECM_ATTACHED;

            checkECM(hitMech);
            }

            mech_notify(hitMech, MECHALL,
            tprintf
            ("A NARC Beacon has been attached to your %s%s!",
                strLocName, tIsRear == 1 ? " (Rear)" : ""));
            mech_notify(mech, MECHALL,
            tprintf
            ("Your NARC Beacon attaches to the target's %s%s!",
                strLocName, tIsRear == 1 ? " (Rear)" : ""));
        }
        } else
        mech_notify(mech, MECHALL,
            "Your NARC Beacon flies off into the distance.");

        return 0;
    }
    }

    if (roll < baseToHit)
    return incoming;

    for (i = 0; MissileHitTable[i].key >= 0; i++)
    if ((k = MissileHitTable[i].num_missiles[10]) <= incoming &&
        ((MechWeapons[MissileHitTable[i].key].special & STREAK) ==
        (MechWeapons[weapindx].special & STREAK)))
        if (k >= l && (j < 0 || MissileHitTable[i].key != weapindx ||
            k > l)) {
        j = i;
        l = k;
        }

    if (j < 0)
    return 0;

    hit =
    MIN(incoming, MissileHitTable[j].num_missiles[MissileHitIndex(mech,
        hitMech, weapindx, wSection, wCritSlot)]);

    if (LOS) {
    mech_notify(mech, MECHALL, tprintf("%%cg%s with %d missile%s!%%c",
        LOS == 1 ? "You hit" : "The swarm hits", hit,
        hit > 1 ? "s" : ""));
    }

    if (AMSShotdown > 0) {
    if (AMSShotdown >= hit) {
        if (LOS)
        mech_notify(mech, MECHALL,
            "All of your missiles are shot down by the target!");

        mech_notify(hitMech, MECHALL,
        "Your Anti-Missile System activates and shoots all the incoming missiles!");
    } else {
        mech_notify(mech, MECHALL,
        tprintf("The target shoots down %d of your missiles!",
            AMSShotdown));

        mech_notify(hitMech, MECHALL,
        tprintf
        ("Your Anti-Missile System activates and shoots down %d incoming missiles!",
            AMSShotdown));
    }
    }

    hit = MAX(0, hit - AMSShotdown);

    if (hit <= 0)
    return 0;

    if (tIsInferno) {
    if (hitMech)
        Inferno_Hit(mech, hitMech, hit, LOS);
    else
        hex_hit(mech, hitX, hitY, weapindx, GetPartAmmoMode(mech,
            wSection, wCritSlot), 0, 0);
    } else {
      if(mudconf.btech_use_glancing_blows && (player_roll == (baseToHit-1))){
        MechLOSBroadcast(hitMech, "is nicked by a glancing blow!");
        mech_notify(hitMech, MECHALL, "You are nicked by a glancing blow!");
        Missile_Hit(mech, hitMech, hitX, hitY, isrear, iscritical,
          weapindx, GetPartFireMode(mech, wSection, wCritSlot),
          GetPartAmmoMode(mech, wSection, wCritSlot), hit,
          (int)(MechWeapons[weapindx].damage+1)/2, Clustersize(weapindx), LOS,
          baseToHit, tIsSwarmAttack);

        }
      else
        Missile_Hit(mech, hitMech, hitX, hitY, isrear, iscritical,
          weapindx, GetPartFireMode(mech, wSection, wCritSlot),
          GetPartAmmoMode(mech, wSection, wCritSlot), hit,
          MechWeapons[weapindx].damage, Clustersize(weapindx), LOS,
          baseToHit, tIsSwarmAttack);
    }

    return incoming - hit;
}

void SwarmHitTarget(MECH * mech,
    int weapindx,
    int wSection,
    int wCritSlot,
    MECH * hitMech,
    int LOS, int baseToHit, int roll, int incoming, int fof,
    int tIsSwarmAttack, int player_roll)
{
#define MAX_STAR 10
    /* Max # of targets we'll try to hit: 10 */
    MECH *star[MAX_STAR];
    int present_target = 0;
    int missiles;
    int loop;
    MAP *map = FindObjectsData(mech->mapindex);
    float r = 0.0, ran = 0, flrange = 0.0;
    MECH *source = mech, *tempMech;
    int i, j;

    for (loop = 0; MissileHitTable[loop].key != -1; loop++)
    if (MissileHitTable[loop].key == weapindx)
        break;

    if (!(MissileHitTable[loop].key == weapindx))
    return;

    missiles = MissileHitTable[loop].num_missiles[10];
    while (missiles > 0) {
    flrange = flrange + FaMechRange(source, hitMech);
    ran = FaMechRange(mech, hitMech);
    if (flrange > EGunRange(weapindx)) {
        mech_notify(hitMech, MECHALL,
        "Luckily, the missiles fall short of you!");
        return;
    }
    if (!(missiles =
        MissileHitTarget(mech, weapindx, wSection, wCritSlot,
            hitMech, -1, -1, InLineOfSight_NB(mech, hitMech,
            MechX(mech),
            MechY(mech),
            ran) ?
            present_target == 0 ? 1 : 2 : 0, baseToHit,
            present_target == 0 ? roll : Roll(), missiles,
            tIsSwarmAttack, player_roll)))
        return;
    /* Try to acquire a new target NOT in the star */
    if (present_target == MAX_STAR)
        return;
    star[present_target++] = hitMech;
    source = hitMech;
    hitMech = NULL;
    for (i = 0; i < map->first_free; i++)
        if ((tempMech = FindObjectsData(map->mechsOnMap[i])))
        if (!fof || (MechTeam(tempMech) != MechTeam(mech))) {
            for (j = 0; j < present_target; j++)
            if (tempMech == star[j])
                break;
            if (j != present_target)
            continue;
            if (!hitMech ||
            (r = FaMechRange(source, tempMech)) < 1.9)
            if (InLineOfSight_NB(source, tempMech,
                MechX(source), MechY(source), r)) {
                hitMech = tempMech;
                ran = r;
            }
        }
    if (!hitMech)
        return;
    if (mech != hitMech)
        mech_notify(hitMech, MECHALL,
        "The missile-swarm turns towards you!");
    if (InLineOfSight_NB(mech, source, MechX(mech), MechY(mech),
        FaMechRange(mech, source)))
        mech_notify(mech, MECHALL,
        tprintf
        ("Your missile-swarm of %d missile%s targets %s!",
            missiles, missiles > 1 ? "s" : "",
            mech == hitMech ? "YOU!!" : GetMechToMechID(mech,
            hitMech)));
    MechLOSBroadcasti(mech, hitMech, "'s missile-swarm targets %s!");
    }
}

/*
 * Fix AMS:
 *
 * - Applied after number missiles is determined
 * - Ammo used == missiles shot down
 * - d6 for IS, 2d6 for clan
 * - Not used against Arrow IV, Thunder, Flare, Swarm or Swarm-1
 */

/****************************************
 * START: AMS related functions 
 ****************************************/
int AMSMissiles(MECH * mech,
    MECH * hitMech,
    int incoming, int type,
    int ammoLoc, int ammoCrit, int LOS, int missilesDidHit)
{
    int num_missiles_shotdown;

    if (MechWeapons[type].special & CLAT)
    num_missiles_shotdown = Roll();
    else
    num_missiles_shotdown = Number(1, 6);

    if (num_missiles_shotdown > incoming)
    num_missiles_shotdown = incoming;

    if (num_missiles_shotdown >= GetPartData(hitMech, ammoLoc, ammoCrit))
    GetPartData(hitMech, ammoLoc, ammoCrit) = 0;
    else
    GetPartData(hitMech, ammoLoc, ammoCrit) -= num_missiles_shotdown;

    if (!missilesDidHit) {
    mech_notify(hitMech, MECHALL,
        "Your Anti-Missile System activates and shoots at the incoming missiles!");
    return 0;
    }

    return num_missiles_shotdown;
}

int LocateAMSDefenses(MECH * target,
    int *AMStype, int *ammoLoc, int *ammoCrit)
{
    int AMSsect, AMScrit;
    int i, j = 0, w, t = 0;

    if (!(MechSpecials(target) & (IS_ANTI_MISSILE_TECH |
        CL_ANTI_MISSILE_TECH)) ||
    !Started(target) || !(MechStatus(target) & AMS_ENABLED))
    return 0;

    for (i = 0; i < NUM_SECTIONS; i++) {
    for (j = 0; j < NUM_CRITICALS; j++)
        if (IsWeapon((t = GetPartType(target, i, j))))
        if (IsAMS(Weapon2I(t)))
            if (!(PartIsNonfunctional(target, i, j) ||
                WpnIsRecycling(target, i, j)))
            break;
    if (j < NUM_CRITICALS)
        break;
    }

    if (i == NUM_SECTIONS)
    return 0;

    w = Weapon2I(t);
    AMSsect = i;
    AMScrit = j;
    *AMStype = w;

    if (!(FindAmmoForWeapon(target, w, AMSsect, ammoLoc, ammoCrit)))
    return 0;

    if (!(GetPartData(target, *ammoLoc, *ammoCrit)))
    return 0;

    SetRecyclePart(target, AMSsect, AMScrit, MechWeapons[w].vrt);
    MechWeapHeat(target) += (float) MechWeapons[w].heat;
    return 1;
}

/****************************************
 * END: AMS related functions 
 ****************************************/
