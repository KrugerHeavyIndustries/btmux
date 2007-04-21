
/*
   p.map.coding.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:42 CET 1999 from map.coding.c */

#ifndef _P_MAP_CODING_H
#define _P_MAP_CODING_H

/* map.coding.c */
void init_map_coding(void);
int Coding_GetIndex(char terrain, char elevation);
char Coding_GetElevation(int index);
char Coding_GetTerrain(int index);

#endif				/* _P_MAP_CODING_H */
