
/*
   p.aero.move.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Mar 22 08:51:10 CET 1999 from aero.move.c */

#ifndef _P_AERO_MOVE_H
#define _P_AERO_MOVE_H

/* aero.move.c */
void aero_takeoff(dbref player, void *data, char *buffer);
void DS_BlastNearbyMechsAndTrees(MECH * mech, char *hitmsg, char *hitmsg1,
    char *nearhitmsg, char *nearhitmsg1, char *treehitmsg, int damage);
void aero_land(dbref player, void *data, char *buffer);
void aero_ControlEffect(MECH * mech);
void ds_BridgeHit(MECH * mech);
void aero_UpdateHeading(MECH * mech);
double length_hypotenuse(double x, double y);
double my_sqrtm(double x, double y);
void aero_UpdateSpeed(MECH * mech);
int FuelCheck(MECH * mech);
void aero_update(MECH * mech);
void aero_thrust(dbref player, void *data, char *arg);
void aero_vheading(dbref player, void *data, char *arg, int flag);
void aero_climb(dbref player, MECH * mech, char *arg);
void aero_dive(dbref player, MECH * mech, char *arg);
int ImproperLZ(MECH * mech, int x, int y);
void DS_LandWarning(MECH * mech, int serious);
void aero_checklz(dbref player, MECH * mech, char *buffer);

#endif				/* _P_AERO_MOVE_H */
