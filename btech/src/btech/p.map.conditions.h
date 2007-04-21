
/*
   p.map.conditions.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:42 CET 1999 from map.conditions.c */

#ifndef _P_MAP_CONDITIONS_H
#define _P_MAP_CONDITIONS_H

/* map.conditions.c */
void alter_conditions(MAP * map);
void map_setconditions(dbref player, MAP * map, char *buffer);
void UpdateConditions(MECH * mech, MAP * map);
void DestroyParts(MECH * attacker, MECH * wounded, int hitloc, int breach,
    int IsDisable);
void reactor_explosion(MECH *wounded, MECH *attacker);

int BreachLoc(MECH * attacker, MECH * mech, int hitloc);
int PossiblyBreach(MECH * attacker, MECH * mech, int hitloc);

#endif				/* _P_MAP_CONDITIONS_H */
