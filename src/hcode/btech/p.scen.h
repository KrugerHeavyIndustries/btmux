
/*
   p.scen.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:33:05 CET 1999 from scen.c */

#ifndef _P_SCEN_H
#define _P_SCEN_H

/* scen.c */
dbref scen_map_ref(SCEN * s);
dbref scen_weather_ref(SCEN * s);
MAP *scen_map(SCEN * s);
void newfreescen(dbref key, void **data, int sel);
SCEN *Map_in_Valid_SO(MAP * map);
void scen_set_osucc(SSOBJ * o, int val);
void scen_trigger_mine(MAP * map, MECH * mech, int x, int y);
void scen_base_generic(MAP * map, MECH * mech, mapobj * o, char *type,
    int val, int sside);
void scen_see_base(MAP * map, MECH * mech, mapobj * o);
void scen_damage_base(MAP * map, MECH * mech, mapobj * o);
void scen_destroy_base(MAP * map, MECH * mech, mapobj * o);
void scen_start_oods(SCEN * s);
void scen_update_enemy(int *now, int *best, int mode, MECH * ds,
    MECH * mech);
int scen_update_enemies(SSIDE * si, MAP * map, int dest);
void scen_update_goal(SCEN * s, SSIDE * si, SSOBJ * ob);
void scen_start(dbref player, void *data, char *buffer);
void scen_tport_players(dbref from, int death);
void scen_handle_mech_extraction(SCEN * s, MAP * map, MECH * mech);
void scen_end(dbref player, void *data, char *buffer);
void show_goals_side(coolmenu * c, SCEN * s, SSIDE * si);
void show_goals(coolmenu * c, SCEN * s, char *side);
void scen_status(dbref player, void *data, char *buffer);

#endif				/* _P_SCEN_H */
