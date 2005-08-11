
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
void muxevent_tickmech_removesection(EVENT * e);
void muxevent_tickmech_removegun(EVENT * e);
void muxevent_tickmech_removepart(EVENT * e);
void muxevent_tickmech_repairarmor(EVENT * e);
void muxevent_tickmech_repairinternal(EVENT * e);
void muxevent_tickmech_reattach(EVENT * e);
void muxevent_tickmech_replacesuit(EVENT * e);
void muxevent_tickmech_replacegun(EVENT * e);
void muxevent_tickmech_repairgun(EVENT * e);
void event_mech_repairenhcrit(EVENT * e);
void muxevent_tickmech_repairpart(EVENT * e);
void muxevent_tickmech_reload(EVENT * e);
void muxevent_tickmech_mountbomb(EVENT * e);
void muxevent_tickmech_umountbomb(EVENT * e);
void muxevent_tickmech_replacesuit(EVENT * e);

#endif				/* _P_MECH_TECH_EVENTS_H */
