
/*
   p.mech.tech.commands.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Tue Feb  9 14:31:36 CET 1999 from mech.tech.commands.c */

#ifndef _P_MECH_TECH_COMMANDS_H
#define _P_MECH_TECH_COMMANDS_H

/* mech.tech.commands.c */
int SomeoneRepairing_s(MECH * mech, int loc, int part, int t);
int SomeoneRepairing(MECH * mech, int loc, int part);
int SomeoneReplacingSuit(MECH * mech, int loc);
int SomeoneFixingA(MECH * mech, int loc);
int SomeoneFixingI(MECH * mech, int loc);
int SomeoneFixing(MECH * mech, int loc);
int SomeoneAttaching(MECH * mech, int loc);
int SomeoneResealing(MECH * mech, int loc);
int SomeoneScrappingLoc(MECH * mech, int loc);
int SomeoneScrappingPart(MECH * mech, int loc, int part);
int CanScrapLoc(MECH * mech, int loc);
int CanScrapPart(MECH * mech, int loc, int part);
int ValidGunPos(MECH * mech, int loc, int pos);
void tech_checkstatus(dbref player, void *data, char *buffer);
void tech_removegun(dbref player, void *data, char *buffer);
void tech_removepart(dbref player, void *data, char *buffer);
int Invalid_Scrap_Path(MECH * mech, int loc);
void tech_removesection(dbref player, void *data, char *buffer);
void tech_replacegun(dbref player, void *data, char *buffer);
void tech_repairgun(dbref player, void *data, char *buffer);
void tech_fixenhcrit(dbref player, void *data, char *buffer);
void tech_replacepart(dbref player, void *data, char *buffer);
void tech_repairpart(dbref player, void *data, char *buffer);
void tech_toggletype(dbref player, void *data, char *buffer);
void tech_reload(dbref player, void *data, char *buffer);
void tech_unload(dbref player, void *data, char *buffer);
void tech_fixarmor(dbref player, void *data, char *buffer);
void tech_fixinternal(dbref player, void *data, char *buffer);
int Invalid_Repair_Path(MECH * mech, int loc);
int unit_is_fixable(MECH * mech);
void tech_reattach(dbref player, void *data, char *buffer);
void tech_reseal(dbref player, void *data, char *buffer);
void tech_magic(dbref player, void *data, char *buffer);
void tech_replacesuit(dbref player, void *data, char *buffer);

#endif				/* _P_MECH_TECH_COMMANDS_H */
