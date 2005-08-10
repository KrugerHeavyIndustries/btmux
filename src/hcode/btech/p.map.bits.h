
/*
   p.map.bits.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:41 CET 1999 from map.bits.c */

#ifndef _P_MAP_BITS_H
#define _P_MAP_BITS_H

/* map.bits.c */
void map_load_bits(FILE * f, MAP * map);
void map_save_bits(FILE * f, MAP * map, mapobj * obj);
void set_hex_enterable(MAP * map, int x, int y);
void set_hex_mine(MAP * map, int x, int y);
void unset_hex_enterable(MAP * map, int x, int y);
void unset_hex_mine(MAP * map, int x, int y);
int is_mine_hex(MAP * map, int x, int y);
int is_hangar_hex(MAP * map, int x, int y);
void clear_hex_bits(MAP * map, int bits);
int bit_size(MAP * map);

#endif				/* _P_MAP_BITS_H */
