
/*
   p.mech.scan.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Tue Feb  9 14:31:35 CET 1999 from mech.scan.c */

#ifndef _P_MECH_SCAN_H
#define _P_MECH_SCAN_H

/* mech.scan.c */
void mech_scan(dbref player, void *data, char *buffer);
void mech_report(dbref player, void *data, char *buffer);
void ShowTurretFacing(dbref player, int spaces, MECH * mech);
void PrintReport(dbref player, MECH * mech, MECH * tempMech, float range);
void PrintEnemyStatus(dbref player, MECH * mymech, MECH * mech,
    float range, int opt);
void mech_bearing(dbref player, void *data, char *buffer);
void mech_range(dbref player, void *data, char *buffer);
void PrintEnemyWeaponStatus(MECH * mech, dbref player);
void mech_sight(dbref player, void *data, char *buffer);
void mech_view(dbref player, void *data, char *buffer);

#endif				/* _P_MECH_SCAN_H */
