
/* p.mech.damage.h */

#ifndef _P_MECH_DAMAGE_H
#define _P_MECH_DAMAGE_H

int cause_armordamage(MECH * wounded, MECH * attacker, int LOS,
    int attackPilot, int isrear, int iscritical, int hitloc, int damage,
    int *crits, int wWeapIndx, int wAmmoMode);
int cause_internaldamage(MECH * wounded, MECH * attacker, int LOS,
    int attackPilot, int isrear, int hitloc, int intDamage, int weapindx,
    int *crits);
void DamageMech(MECH * wounded, MECH * attacker, int LOS, int attackPilot,
    int hitloc, int isrear, int iscritical, int damage, int intDamage,
    int cause, int bth, int wWeapIndx, int wAmmoMode, int tIgnoreSwarmers);
void DestroyWeapon(MECH * wounded, int hitloc, int type, int startCrit,
    int numcrits, int totalcrits);
int CountWeaponsInLoc(MECH * mech, int loc);
int FindWeaponTypeNumInLoc(MECH * mech, int loc, int num);
void LoseWeapon(MECH * mech, int hitloc);
void DestroyHeatSink(MECH * mech, int hitloc);
void DestroySection(MECH * wounded, MECH * attacker, int LOS, int hitloc);
char *setarmorstatus_func(MECH * mech, char *sectstr, char *typestr,
    char *valuestr);
int dodamage_func(dbref player, MECH * mech, int totaldam, int clustersize,
    int direction, int critical, char *mechmsg, char *mechbroadcast);
void mech_damage(dbref player, MECH * mech, char *buffer);

#endif				/* _P_MECH_DAMAGE_H */
