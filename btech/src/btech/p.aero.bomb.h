
/*
   p.aero.bomb.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:32 CET 1999 from aero.bomb.c */

#ifndef _P_AERO_BOMB_H
#define _P_AERO_BOMB_H

/* aero.bomb.c */
void DestroyBomb(MECH * mech, int loc);
int BombWeight(int i);
char *bomb_name(int i);
void bomb_list(MECH * mech, int player);
float calc_dest(MECH * mech, short *x, short *y);
void bomb_aim(MECH * mech, dbref player);
void bomb_hit_hexes(MAP * map, int x, int y, int hitnb, int iscluster,
    int aff_d, int aff_h, char *tomsg, char *otmsg, char *tomsg1,
    char *otmsg1);
void simulate_flight(MECH * mech, MAP * map, short *x, short *y, float t);
void bomb_drop(MECH * mech, int player, int bn);
void mech_bomb(dbref player, void *data, char *buffer);

#endif				/* _P_AERO_BOMB_H */
