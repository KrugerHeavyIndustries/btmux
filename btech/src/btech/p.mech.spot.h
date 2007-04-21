
/* p.mech.spot.h */

#ifndef _P_MECH_SPOT_H
#define _P_MECH_SPOT_H

int IsArtyMech(MECH * mech);
void ClearFireAdjustments(MAP * map, dbref mech);
void mech_spot(dbref player, void *data, char *buffer);
int FireSpot(dbref player, MECH * mech, MAP * mech_map, int weaponnum,
    int weapontype, int sight, int section, int critical);

#endif				/* _P_MECH_SPOT_H */
