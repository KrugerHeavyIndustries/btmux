
/*
   p.mech.enhanced.criticals.h
*/

#ifndef _P_MECH_ENHANCED_CRITICALS_H
#define _P_MECH_ENHANCED_CRITICALS_H

void getWeapData(MECH * mech, int section, int critical, int *wWeapIndex,
    int *wWeapSize, int *wFirstCrit);
int getCritAddedBTH(MECH * mech, int section, int critical,
    int rangeBracket);
int getCritAddedHeat(MECH * mech, int section, int critical);
int getCritSubDamage(MECH * mech, int section, int critical);
int canWeapExplodeFromDamage(MECH * mech, int section, int critical,
    int roll);
int canWeapJamFromDamage(MECH * mech, int section, int critical, int roll);
int isWeapAmmoFeedLocked(MECH * mech, int section, int critical);
int countDamagedSlots(MECH * mech, int section, int wFirstCrit,
    int wWeapSize);
int countDamagedSlotsFromCrit(MECH * mech, int section, int critical);
int shouldDestroyWeapon(MECH * mech, int section, int critical,
    int incrementCount);
void scoreEnhancedWeaponCriticalHit(MECH * mech, MECH * attacker, int LOS,
    int section, int critical);
void mech_weaponstatus(dbref player, MECH * mech, char *buffer);
void showWeaponDamageAndInfo(dbref player, MECH * mech, int section,
    int critical);

#endif
