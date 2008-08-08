
/*
   p.map.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 22 09:50:26 CET 1999 from map.c */

#ifndef _P_MAP_H
#define _P_MAP_H

/* map.c */
void debug_fixmap(dbref player, void *data, char *buffer);
void map_view(dbref player, void *data, char *buffer);
void map_addhex(dbref player, void *data, char *buffer);
void map_mapemit(dbref player, void *data, char *buffer);
int water_distance(MAP * map, int x, int y, int dir, int max);
int map_load(MAP * map, char * mapname);
int map_checkmapfile(MAP * map, char * mapname);
void map_loadmap(dbref player, void *data, char *buffer);
void map_savemap(dbref player, void *data, char *buffer);
void map_setmapsize(dbref player, void *data, char *buffer);
void map_clearmechs(dbref player, void *data, char *buffer);
void map_update(dbref obj, void *data);
void initialize_map_empty(MAP * new, dbref key);
void newfreemap(dbref key, void **data, int selector);
int map_sizefun(void *data, int flag);
void map_listmechs(dbref player, void *data, char *buffer);
void clear_hex(MECH * mech, int x, int y, int meant);
void UpdateMechsTerrain(MAP * map, int x, int y, int t);

#endif				/* _P_MAP_H */
