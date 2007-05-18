
/* p.mech.combat.misc.h */

#ifndef _P_MECH_COMBAT_MISC_H
#define _P_MECH_COMBAT_MISC_H

void decrement_ammunition(MECH * mech, int weapindx, int section,
    int critical, int ammoLoc, int ammoCrit, int ammoLoc1, int ammoCrit1,
    int wGattlingShots);
void ammo_expedinture_check(MECH * mech, int weapindx, int ns);
void heat_effect(MECH * mech, MECH * tempMech, int heatdam,
    int fromInferno);
void Inferno_Hit(MECH * mech, MECH * hitMech, int missiles, int LOS);
void KillMechContentsIfIC(dbref aRef);
void DestroyMech(MECH * target, MECH * mech, int showboom, const char *reason);
char *short_hextarget(MECH * mech);

#endif				/* _P_MECH_COMBAT_MISC_H */
