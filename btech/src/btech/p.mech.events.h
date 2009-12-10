
/*
   p.mech.events.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:48 CET 1999 from mech.events.c */

#ifndef _P_MECH_EVENTS_H
#define _P_MECH_EVENTS_H

/* mech.events.c */
void mech_standfail_event(MUXEVENT * e);
void mech_fall_event(MUXEVENT * e);
void mech_lock_event(MUXEVENT * e);
void mech_stabilizing_event(MUXEVENT * e);
void mech_jump_event(MUXEVENT * e);
void mech_recovery_event(MUXEVENT * e);
void mech_recycle_event(MUXEVENT * e);
void ProlongUncon(MECH * mech, int len);
void MaybeRecycle(MECH * mech, int wticks);
void mech_lateral_event(MUXEVENT * e);
void mech_move_event(MUXEVENT * e);
void mech_stand_event(MUXEVENT * e);
void mech_plos_event(MUXEVENT * e);
void aero_move_event(MUXEVENT * e);
void very_fake_func(MUXEVENT * e);
void mech_crewstun_event(MUXEVENT * e);
void unstun_crew_event(MUXEVENT * e);
void mech_unjam_ammo_event(MUXEVENT * objEvent);
void check_stagger_event(MUXEVENT * event);
#ifdef BT_MOVEMENT_MODES
void mech_movemode_event(MUXEVENT * e);
#endif
int calcStaggerBTHMod(MECH * mech);
int calcNewStaggerBTHMod(MECH * mech, int staggerLevel);
void mech_staggercheck_heartbeat(MECH * mech);

#endif				/* _P_MECH_EVENTS_H */
