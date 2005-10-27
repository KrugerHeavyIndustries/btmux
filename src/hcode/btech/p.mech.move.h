
/*
   p.mech.move.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Mar 22 08:51:15 CET 1999 from mech.move.c */

#ifndef _P_MECH_MOVE_H
#define _P_MECH_MOVE_H

/* mech.move.c */
const char *LateralDesc(MECH * mech);
void mech_lateral(dbref player, void *data, char *buffer);
void mech_turnmode(dbref player, void *data, char *buffer);
void mech_bootlegger(dbref player, void *data, char *buffer);
void mech_eta(dbref player, void *data, char *buffer);
float MechCargoMaxSpeed(MECH * mech, float mspeed);
void mech_drop(dbref player, void *data, char *buffer);
void mech_stand(dbref player, void *data, char *buffer);
void mech_land(dbref player, void *data, char *buffer);
void mech_heading(dbref player, void *data, char *buffer);
void mech_turret(dbref player, void *data, char *buffer);
void mech_rotatetorso(dbref player, void *data, char *buffer);
void mech_speed(dbref player, void *data, char *buffer);
void mech_vertical(dbref player, void *data, char *buffer);
void mech_thrash(dbref player, void *data, char *buffer);
void mech_jump(dbref player, void *data, char *buffer);
void mech_hulldown(dbref player, void *data, char *buffer);
#ifdef BT_MOVEMENT_MODES
void mech_sprint(dbref player, void *data, char *buffer);
void mech_evade(dbref player, void *data, char *buffer);
void mech_dodge(dbref player, void *date, char *buffer);
#endif
int DropGetElevation(MECH * mech);
void DropSetElevation(MECH * mech, int wantdrop);
void LandMech(MECH * mech);
void MechFloodsLoc(MECH * mech, int loc, int lev);
void MechFloods(MECH * mech);
void MechFalls(MECH * mech, int levels, int seemsg);
int mechs_in_hex(MAP * map, int x, int y, int friendly, int team);
void cause_damage(MECH * att, MECH * mech, int dam, int table);
int domino_space_in_hex(MAP * map, MECH * me, int x, int y, int friendly,
    int mode, int cnt);
int domino_space(MECH * mech, int mode);

#endif				/* _P_MECH_MOVE_H */
