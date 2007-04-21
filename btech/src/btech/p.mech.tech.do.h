
/*
   p.mech.tech.do.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:59 CET 1999 from mech.tech.do.c */

#ifndef _P_MECH_TECH_DO_H
#define _P_MECH_TECH_DO_H

/* mech.tech.do.c */
int valid_ammo_mode(MECH * mech, int loc, int part, int let);
int FindAmmoType(MECH * mech, int loc, int part);
int replace_econ(dbref player, MECH * mech, int loc, int part);
int reload_econ(dbref player, MECH * mech, int loc, int part, int *val);
int fixarmor_econ(dbref player, MECH * mech, int loc, int *val);
int fixinternal_econ(dbref player, MECH * mech, int loc, int *val);
int repair_econ(dbref player, MECH * mech, int loc, int part);
int repairenhcrit_econ(dbref player, MECH * mech, int loc, int part);
int reattach_econ(dbref player, MECH * mech, int loc);
int replacesuit_econ(dbref player, MECH * mech, int loc);
int reseal_econ(dbref player, MECH * mech, int loc);
int replacep_succ(dbref player, MECH * mech, int loc, int part);
int replaceg_succ(dbref player, MECH * mech, int loc, int part);
int reload_succ(dbref player, MECH * mech, int loc, int part, int *val);
int fixinternal_succ(dbref player, MECH * mech, int loc, int *val);
int fixarmor_succ(dbref player, MECH * mech, int loc, int *val);
int reattach_succ(dbref player, MECH * mech, int loc);
int replacesuit_succ(dbref player, MECH * mech, int loc);
int reseal_succ(dbref player, MECH * mech, int loc);
int repairg_succ(dbref player, MECH * mech, int loc, int part);
int repairenhcrit_succ(dbref player, MECH * mech, int loc, int part);
int repairp_succ(dbref player, MECH * mech, int loc, int part);
int replacep_fail(dbref player, MECH * mech, int loc, int part);
int repairp_fail(dbref player, MECH * mech, int loc, int part);
int replaceg_fail(dbref player, MECH * mech, int loc, int part);
int repairg_fail(dbref player, MECH * mech, int loc, int part);
int repairenhcrit_fail(dbref player, MECH * mech, int loc, int part);
int reload_fail(dbref player, MECH * mech, int loc, int part, int *val);
int fixarmor_fail(dbref player, MECH * mech, int loc, int *val);
int fixinternal_fail(dbref player, MECH * mech, int loc, int *val);
int reattach_fail(dbref player, MECH * mech, int loc);
int replacesuit_fail(dbref player, MECH * mech, int loc);
int reseal_fail(dbref player, MECH * mech, int loc);

#endif				/* _P_MECH_TECH_DO_H */
