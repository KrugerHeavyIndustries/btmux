
/*
   p.autopilot.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:34 CET 1999 from autopilot.c */

#ifndef _P_AUTOPILOT_H
#define _P_AUTOPILOT_H

/* autopilot.c */
void gradually_load(MECH * mech, int loc, int percent);
void autopilot_load_cargo(dbref player, MECH * mech, int percent);
void figure_out_range_and_bearing(MECH * mech, int tx, int ty,
    float *range, int *bearing);
void auto_goto_event(MUXEVENT * e);
void auto_follow_event(MUXEVENT * e);
#endif				/* _P_AUTOPILOT_H */
