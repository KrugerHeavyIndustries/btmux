
/*
   p.eject.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Wed Feb 17 23:36:31 CET 1999 from eject.c */

#include "config.h"

#ifndef _P_EJECT_H
#define _P_EJECT_H

/* eject.c */
int tele_contents(dbref from, dbref to, int flag);
void discard_mw(MECH * mech);
void enter_mw_bay(MECH * mech, dbref bay);
void pickup_mw(MECH * mech, MECH * target);
void mech_eject(dbref player, void *data, char *buffer);
void mech_disembark(dbref player, void *data, char *buffer);
void mech_udisembark(dbref player, void *data, char *buffer);
void mech_embark(dbref player, void *data, char *buffer);
void autoeject(dbref player, MECH * mech, int tIsBSuit);
#endif				/* _P_EJECT_H */
