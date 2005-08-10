
/*
   p.mine.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:33:04 CET 1999 from mine.c */

#ifndef _P_MINE_H
#define _P_MINE_H

/* mine.c */
void add_mine(MAP * map, int x, int y, int dam);
void make_mine_explode(MECH * mech, MAP * map, mapobj * o, int x, int y,
    int reason);
void possible_mine_poof(MECH * mech, int reason);
void possibly_remove_mines(MECH * mech, int x, int y);
void recalculate_minefields(MAP * map);
void map_add_mine(dbref player, void *data, char *buffer);
void explode_mines(MECH * mech, int chn);
void show_mines_in_hex(dbref player, MECH * mech, float range, int x,
    int y);

#endif				/* _P_MINE_H */
