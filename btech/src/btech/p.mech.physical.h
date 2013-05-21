
/*
   p.mech.physical.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Wed Feb 17 23:36:34 CET 1999 from mech.physical.c */

#ifndef _P_MECH_PHYSICAL_H
#define _P_MECH_PHYSICAL_H

/* mech.physical.c */
void mech_punch(dbref player, void *data, char *buffer);
void mech_club(dbref player, void *data, char *buffer);
int have_axe(MECH * mech, int loc);
int have_sword(MECH * mech, int loc);
int have_mace(MECH * mech, int loc);
int have_saw(MECH * mech, int loc);
int have_claw(MECH * mech, int loc);
void mech_saw(dbref player, void *data, char *buffer);
void mech_axe(dbref player, void *data, char *buffer);
void mech_sword(dbref player, void *data, char *buffer);
void mech_mace(dbref player, void *data, char *buffer);
void mech_claw(dbref player, void *data, char *buffer);
void mech_kick(dbref player, void *data, char *buffer);
void mech_trip(dbref player, void *data, char *buffer);
void mech_kickortrip(dbref player, void *data, char *buffer, int AttackType);
void mech_charge(dbref player, void *data, char *buffer);
char *phys_form(int AttackType, int add_s);
void phys_succeed(MECH * mech, MECH * target, int at);
void phys_fail(MECH * mech, MECH * target, int at);
void PhysicalAttack(MECH * mech, int damageweight, int baseToHit,
    int AttackType, int argc, char **args, MAP * mech_map, int sect);
void PhysicalTrip(MECH * mech, MECH * target);
void PhysicalDamage(MECH * mech, MECH * target, int weightdmg,
    int AttackType, int sect, int glance);
int DeathFromAbove(MECH * mech, MECH * target);
void ChargeMech(MECH * mech, MECH * target);
int checkGrabClubLocation(MECH * mech, int section, int emit);
void mech_grabclub(dbref player, void *data, char *buffer);

#endif				/* _P_MECH_PHYSICAL_H */
