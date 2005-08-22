
/*
   p.events.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Feb 22 14:59:36 CET 1999 from events.c */

#ifndef _P_EVENTS_H
#define _P_EVENTS_H

/* events.c */
void muxevent_count_initialize(void);
void debug_EventTypes(dbref player, void *data, char *buffer);
void prerun_event(MUXEVENT * e);
void postrun_event(MUXEVENT * e);

#endif				/* _P_EVENTS_H */
