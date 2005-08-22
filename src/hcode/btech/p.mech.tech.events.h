
/*
   p.mech.tech.events.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:33:00 CET 1999 from mech.tech.events.c */

#ifndef _P_MECH_TECH_EVENTS_H
#define _P_MECH_TECH_EVENTS_H

/* mech.tech.events.c */
void muxevent_tickmech_removesection(MUXEVENT * e);
void muxevent_tickmech_removegun(MUXEVENT * e);
void muxevent_tickmech_removepart(MUXEVENT * e);
void muxevent_tickmech_repairarmor(MUXEVENT * e);
void muxevent_tickmech_repairinternal(MUXEVENT * e);
void muxevent_tickmech_reattach(MUXEVENT * e);
void muxevent_tickmech_replacesuit(MUXEVENT * e);
void muxevent_tickmech_replacegun(MUXEVENT * e);
void muxevent_tickmech_repairgun(MUXEVENT * e);
void event_mech_repairenhcrit(MUXEVENT * e);
void muxevent_tickmech_repairpart(MUXEVENT * e);
void muxevent_tickmech_reload(MUXEVENT * e);
void muxevent_tickmech_mountbomb(MUXEVENT * e);
void muxevent_tickmech_umountbomb(MUXEVENT * e);
void muxevent_tickmech_replacesuit(MUXEVENT * e);

#endif				/* _P_MECH_TECH_EVENTS_H */
