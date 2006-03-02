
/* p.mech.combat.h */

#ifndef _P_MECH_COMBAT_MISSILE_H
#define _P_MECH_COMBAT_MISSILE_H

void Missile_Hit(MECH * mech, MECH * target, int hitX, int hitY,
    int isrear, int iscritical, int weapindx, int fireMode, int ammoMode,
    int num_missiles_hit, int damage, int salvo_size, int LOS,
    int bth, int tIsSwarmAttack);
int AMSMissiles(MECH * mech, MECH * hitMech, int incoming, int type,
    int ammoLoc, int ammoCrit, int LOS, int missilesDidHit);
int LocateAMSDefenses(MECH * target, int *AMStype, int *ammoLoc,
    int *ammoCrit);
int MissileHitIndex(MECH * mech, MECH * hitMech, int weapindx,
    int wSection, int wCritSlot, int glance);
int MissileHitTarget(MECH * mech, int weapindx, int wSection,
    int wCritSlot, MECH * hitMech, int hitX, int hitY, int LOS,
    int baseToHit, int roll, int incoming, int tIsSwarmAttack, int player_roll);
void SwarmHitTarget(MECH * mech, int weapindx, int wSection, int wCritSlot,
    MECH * hitMech, int LOS, int baseToHit, int roll, int incoming,
    int fof, int tIsSwarmAttack, int player_roll);

#endif              /* _P_MECH_COMBAT_MISSILE_H */
