
/*
   p.pcombat.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:33:04 CET 1999 from pcombat.c */

#ifndef _P_PCOMBAT_H
#define _P_PCOMBAT_H

/* pcombat.c */
int pc_to_dam_conversion(MECH * target, int weapindx, int dam);
int dam_to_pc_conversion(MECH * target, int weapindx, int dam);
int armor_effect(MECH * wounded, int cause, int hitloc, int intDamage,
    int id);

#endif				/* _P_PCOMBAT_H */
