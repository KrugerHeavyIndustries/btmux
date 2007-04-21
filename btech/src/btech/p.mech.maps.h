
/*
   p.mech.maps.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Mar 22 08:51:14 CET 1999 from mech.maps.c */

#ifndef _P_MECH_MAPS_H
#define _P_MECH_MAPS_H

/* mech.maps.c */
void mech_findcenter(dbref player, void *data, char *buffer);
const char *GetTerrainName_base(int t);
const char *GetTerrainName(MAP * map, int x, int y);
void mech_navigate(dbref player, void *data, char *buffer);
char GetLRSMechChar(MECH * mech, MECH * tempMech);
void mech_lrsmap(dbref player, void *data, char *buffer);
char *TerrainColor(char terrain, int elev);
void TacMapTerr(MAP * mech_map, int x, int y, char *terr, char *elev,
    int isdown);
char **MakeMapText(dbref player, MECH * mech, MAP * mech_map, int x, int y,
    int xw, int yw, int labels, int dohexlos);
void mech_tacmap(dbref player, void *data, char *buffer);
void mech_enterbase(dbref player, void *data, char *buffer);

#endif				/* _P_MECH_MAPS_H */
