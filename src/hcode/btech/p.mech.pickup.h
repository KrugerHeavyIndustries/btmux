
/*
   p.mech.pickup.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:54 CET 1999 from mech.pickup.c */

#ifndef _P_MECH_PICKUP_H
#define _P_MECH_PICKUP_H

/* mech.pickup.c */
void mech_pickup(dbref player, void *data, char *buffer);
void mech_attachcables(dbref player, void *data, char *buffer);
void mech_detachcables(dbref player, void *data, char *buffer);
void mech_dropoff(dbref player, void *data, char *buffer);

#endif				/* _P_MECH_PICKUP_H */
