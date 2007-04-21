
/*
   p.mech.los.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Mar 22 08:51:13 CET 1999 from mech.los.c */

#ifndef _P_MECH_LOS_H
#define _P_MECH_LOS_H

/* mech.los.c */
float ActualElevation(MAP * map, int x, int y, MECH * mech);
int CalculateLOSFlag(MECH * mech, MECH * target, MAP * map, int x, int y,
    int ff, float hexRange);
int AddTerrainMod(MECH * mech, MECH * target, MAP * map, float hexRange,
    int wAmmoMode);
int InLineOfSight_NB(MECH * mech, MECH * target, int x, int y,
    float hexRange);
int InLineOfSight(MECH * mech, MECH * target, int x, int y,
    float hexRange);
void mech_losemit(dbref player, MECH * mech, char *buffer);

#endif				/* _P_MECH_LOS_H */
