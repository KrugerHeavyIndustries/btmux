
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
void mech_standfail_event(EVENT * e);
void mech_fall_event(EVENT * e);
void mech_lock_event(EVENT * e);
void mech_stabilizing_event(EVENT * e);
void mech_jump_event(EVENT * e);
void mech_recovery_event(EVENT * e);
void mech_recycle_event(EVENT * e);
void ProlongUncon(MECH * mech, int len);
void MaybeRecycle(MECH * mech, int wticks);
void mech_lateral_event(EVENT * e);
void mech_move_event(EVENT * e);
void mech_stand_event(EVENT * e);
void mech_plos_event(EVENT * e);
void aero_move_event(EVENT * e);
void very_fake_func(EVENT * e);
void unstun_crew_event(EVENT * e);
void mech_unjam_ammo_event(EVENT * objEvent);
void check_stagger_event(EVENT * event);
#ifdef BT_MOVEMENT_MODES
void mech_movemode_event(EVENT * e);
#endif
int calcStaggerBTHMod(MECH * mech);

#endif				/* _P_MECH_EVENTS_H */
