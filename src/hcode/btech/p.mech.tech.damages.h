
/*
   p.mech.tech.damages.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:59 CET 1999 from mech.tech.damages.c */

#ifndef _P_MECH_TECH_DAMAGES_H
#define _P_MECH_TECH_DAMAGES_H

/* mech.tech.damages.c */
void make_scrap_table(MECH * mech);
void make_damage_table(MECH * mech);
int is_under_repair(MECH * mech, int i);
char *damages_func(MECH * mech);
void show_mechs_damage(dbref player, void *data, char *buffer);
void tech_fix(dbref player, void *data, char *buffer);

#endif				/* _P_MECH_TECH_DAMAGES_H */
