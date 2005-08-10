
/*
   p.mech.ice.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:49 CET 1999 from mech.ice.c */

#ifndef _P_MECH_ICE_H
#define _P_MECH_ICE_H

/* mech.ice.c */
void drop_thru_ice(MECH * mech);
void break_thru_ice(MECH * mech);
int possibly_drop_thru_ice(MECH * mech);
int growable(MAP * map, int x, int y);
int meltable(MAP * map, int x, int y);
void ice_growth(dbref player, MAP * map, int num);
void ice_melt(dbref player, MAP * map, int num);
void map_addice(dbref player, MAP * map, char *buffer);
void map_delice(dbref player, MAP * map, char *buffer);
void possibly_blow_ice(MECH * mech, int weapindx, int x, int y);
void possibly_blow_bridge(MECH * mech, int weapindx, int x, int y);

#endif				/* _P_MECH_ICE_H */
