
/*
   p.mech.combat.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Mar 22 10:40:19 CET 1999 from mech.combat.c */

#ifndef _P_MECH_COMBAT_H
#define _P_MECH_COMBAT_H

/* mech.combat.c */
void mech_target(dbref player, void *data, char *buffer);
void sixth_sense_check(MECH * mech, MECH * target);
void mech_settarget(dbref player, void *data, char *buffer);
void mech_fireweapon(dbref player, void *data, char *buffer);
int FireWeaponNumber(dbref player, MECH * mech, MAP * mech_map,
    int weapnum, int argc, char **args, int sight);
char *hex_target_id(MECH * mech);
int canClearOrIgnite(int weapindx);
void possibly_ignite(MECH * mech, MAP * map, int weapindx, int ammoMode,
    int x, int y, int intentional);
void possibly_clear(MECH * mech, MAP * map, int weapindx, int ammoMode,
    int damage, int x, int y, int intentional);
void possibly_ignite_or_clear(MECH * mech, int weapindx, int ammoMode,
    int damage, int x, int y, int intentional);
void hex_hit(MECH * mech, int x, int y, int weapindx, int ammoMode,
    int damage, int ishit);
int weapon_failure_stuff(MECH * mech, int *weapnum, int *weapindx,
    int *section, int *critical, int *ammoLoc, int *ammoCrit,
    int *ammoLoc1, int *ammoCrit1, int *modifier, int *type, float range,
    int *range_ok, int wGattlingShots);
void FireWeapon(MECH * mech, MAP * mech_map, MECH * target, int LOS,
    int weapindx, int weapnum, int section, int critical, float enemyX,
    float enemyY, int mapx, int mapy, float range, int indirectFire,
    int sight, int ishex);
int determineDamageFromHit(MECH * mech, int wSection, int wCritSlot,
    MECH * hitMech, int hitX, int hitY, int weapindx,
    int wGattlingShots, int wBaseWeapDamage, int wAmmoMode, int type,
    int modifier, int isTempCalc);
void HitTarget(MECH * mech, int weapindx, int wSection, int wCritSlot,
    MECH * hitMech, int hitX, int hitY, int LOS, int type, int modifier,
    int reallyhit, int bth, int wGattlingShots, int tIsSwarmAttack, int player_roll);

#endif				/* _P_MECH_COMBAT_H */
