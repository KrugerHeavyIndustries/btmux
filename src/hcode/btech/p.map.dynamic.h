
/*
   p.map.dynamic.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:43 CET 1999 from map.dynamic.c */

#ifndef _P_MAP_DYNAMIC_H
#define _P_MAP_DYNAMIC_H

/* map.dynamic.c */
void load_mapdynamic(FILE * f, MAP * map);
void save_mapdynamic(FILE * f, MAP * map);
void mech_map_consistency_check(MECH * mech);
void eliminate_empties(MAP * map);
void remove_mech_from_map(MAP * map, MECH * mech);
void add_mech_to_map(MAP * newmap, MECH * mech);
int mech_size(MAP * map);

#endif				/* _P_MAP_DYNAMIC_H */
