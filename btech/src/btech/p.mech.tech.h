
/*
   p.mech.tech.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:58 CET 1999 from mech.tech.c */

#ifndef _P_MECH_TECH_H
#define _P_MECH_TECH_H

/* mech.tech.c */
int game_lag(void);
int game_lag_time(int i);
int tech_roll(dbref player, MECH * mech, int diff);
int tech_weapon_roll(dbref player, MECH * mech, int diff);
void tech_status(dbref player, time_t dat);
int tech_addtechtime(dbref player, int time);
int tech_parsepart_advanced(MECH * mech, char *buffer, int *loc, int *pos,
    int *extra, int allowrear);
int tech_parsepart(MECH * mech, char *buffer, int *loc, int *pos,
    int *extra);
int tech_parsegun(MECH * mech, char *buffer, int *loc, int *pos,
    int *brand);
int figure_latest_tech_event(MECH * mech);

#endif				/* _P_MECH_TECH_H */
