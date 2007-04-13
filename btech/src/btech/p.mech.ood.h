
/*
   p.mech.ood.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:52 CET 1999 from mech.ood.c */

#ifndef _P_MECH_OOD_H
#define _P_MECH_OOD_H

/* mech.ood.c */
void mech_ood_damage(MECH * wounded, MECH * attacker, int damage);
void mech_ood_event(MUXEVENT * e);
void initiate_ood(dbref player, MECH * mech, char *buffer);

#endif				/* _P_MECH_OOD_H */
