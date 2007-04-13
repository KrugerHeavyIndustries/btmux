
/*
   p.artillery.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Mar 22 08:51:11 CET 1999 from artillery.c */

#ifndef _P_ARTILLERY_H
#define _P_ARTILLERY_H

/* artillery.c */
int artillery_round_flight_time(float fx, float fy, float tx, float ty);
void artillery_shoot(MECH * mech, int targx, int targy, int windex,
    int wmode, int ishit);
void blast_hit_hexf(MAP * map, int dam, int singlehitsize, int heatdam,
    float fx, float fy, float tfx, float tfy, char *tomsg, char *otmsg,
    int table, int safeup, int safedown, int isunderwater);
void blast_hit_hex(MAP * map, int dam, int singlehitsize, int heatdam,
    int fx, int fy, int tx, int ty, char *tomsg, char *otmsg, int table,
    int safeup, int safedown, int isunderwater);
void blast_hit_hexesf(MAP * map, int dam, int singlehitsize, int heatdam,
    float fx, float fy, float ftx, float fty, char *tomsg, char *otmsg,
    char *tomsg1, char *otmsg1, int table, int safeup, int safedown,
    int isunderwater, int doneighbors);
void blast_hit_hexes(MAP * map, int dam, int singlehitsize, int heatdam,
    int tx, int ty, char *tomsg, char *otmsg, char *tomsg1, char *otmsg1,
    int table, int safeup, int safedown, int isunderwater,
    int doneighbors);
void artillery_FriendlyAdjustment(dbref mechnum, MAP * map, int x, int y);

#endif				/* _P_ARTILLERY_H */
