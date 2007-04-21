
/*
   p.mech.hitloc.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Tue Feb  9 14:31:33 CET 1999 from mech.hitloc.c */

#ifndef _P_MECH_HITLOC_H
#define _P_MECH_HITLOC_H

/* mech.hitloc.c */
int FindPunchLocation(MECH *target, int hitGroup);
int FindKickLocation(MECH *target, int hitGroup);
int get_bsuit_hitloc(MECH * mech);
int TransferTarget(MECH * mech, int hitloc);
int FindSwarmHitLocation(int *iscritical, int *isrear);
int crittable(MECH * m, int loc, int tres);
int FindHitLocation(MECH * mech, int hitGroup, int *iscritical,
    int *isrear);
int FindFasaHitLocation(MECH * mech, int hitGroup, int *iscritical,
    int *isrear);
void DoMotiveSystemHit(MECH * mech, int wRollMod);
int FindAdvFasaVehicleHitLocation(MECH * mech, int hitGroup,
    int *iscritical, int *isrear);
int findNARCHitLoc(MECH * mech, MECH * hitMech, int *tIsRearHit);
int FindTargetHitLoc(MECH * mech, MECH * target, int *isrear,
    int *iscritical);
int FindTCHitLoc(MECH * mech, MECH * target, int *isrear, int *iscritical);
int FindAimHitLoc(MECH * mech, MECH * target, int *isrear,
    int *iscritical);
int FindAreaHitGroup(MECH * mech, MECH * target);

#endif				/* _P_MECH_HITLOC_H */
