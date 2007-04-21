
/*
   p.ds.bay.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Tue Feb  9 14:31:30 CET 1999 from ds.bay.c */

#ifndef _P_DS_BAY_H
#define _P_DS_BAY_H

/* ds.bay.c */
void mech_createbays(dbref player, void *data, char *buffer);
int Find_DS_Bay_Number(MECH * ds, int dir);
int Find_DS_Bay_Dir(MECH * ds, int num);
int Find_DS_Bay_In_MechHex(MECH * seer, MECH * ds, int *bayn);
void mech_enterbay(dbref player, void *data, char *buffer);
int Leave_DS(MAP * map, MECH * mech);

#endif				/* _P_DS_BAY_H */
