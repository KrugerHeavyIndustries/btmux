
/* p.mech.bth.h */

#ifndef _P_MECH_BTH_H
#define _P_MECH_BTH_H

int FindNormalBTH(MECH * mech, MAP * mech_map, int section, int critical,
    int weapindx, float range, MECH * target, int indirectFire,
    dbref * c3Ref);
int FindArtilleryBTH(MECH * mech, int section, int weapindx, int indirect,
    float range);
int FindBTHByRange(MECH * mech, MECH * target, int section,
    int weapindx, float frange, int firemode, int ammomode,int *wBTH);
int FindBTHByC3Range(MECH * mech, MECH * target, int section,
    int weapindx, float realRange, float c3Range, int mode, int *wBTH);
int AttackMovementMods(MECH * mech);
int TargetMovementMods(MECH * mech, MECH * target, float range);

#define RANGE_SHORT				0
#define RANGE_MED					1
#define RANGE_LONG				2
#define RANGE_EXTREME			3
#define RANGE_TOFAR				4
#define RANGE_NOWATER			5

#endif				/* _P_MECH_BTH_H */
