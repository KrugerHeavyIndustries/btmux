
/*
   p.mech.update.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Mar 22 08:51:16 CET 1999 from mech.update.c */

#ifndef _P_MECH_UPDATE_H
#define _P_MECH_UPDATE_H

/* mech.update.c */
int fiery_death(MECH * mech);
int bridge_w_elevation(MECH * mech);
void bridge_set_elevation(MECH * mech);
int DSOkToNotify(MECH * mech);
int collision_check(MECH * mech, int mode, int le, int lt);
void move_mech(MECH * mech);
void CheckNavalHeight(MECH * mech, int oz);
void CheckVTOLHeight(MECH * mech);
void UpdateHeading(MECH * mech);
float terrain_speed(MECH * mech, float tempspeed, float maxspeed,
    int terrain, int elev);
void UpdateSpeed(MECH * mech);
int OverheatMods(MECH * mech);
void ammo_explosion(MECH * attacker, MECH * mech, int ammoloc,
    int ammocritnum, int damage);
void HandleOverheat(MECH * mech);
void UpdateHeat(MECH * mech);
int recycle_weaponry(MECH * mech);
int SkidMod(float Speed);
void NewHexEntered(MECH * mech, MAP * mech_map, float deltax, float deltay,
    int last_z);
void RemoveStaggerDamage(MECH * mech, int staggerLevel);
void ClearStaggerDamage(MECH * mech);
int CurrentStaggerDamage(MECH * mech);
void CheckDamage(MECH * wounded);
void UpdatePilotSkillRolls(MECH * mech);
void updateAutoturnTurret(MECH * mech);
void mech_update(dbref key, void *data);

#endif				/* _P_MECH_UPDATE_H */
