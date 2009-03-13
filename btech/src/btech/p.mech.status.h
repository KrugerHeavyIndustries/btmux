
/*
   p.mech.status.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Mar 22 08:51:16 CET 1999 from mech.status.c */

#include "config.h"

#ifndef _P_MECH_STATUS_H
#define _P_MECH_STATUS_H

/*
 * Armor status flags for ArmorEvaluateSerious().
 *
 * TODO: Can probably coalesce some of these with other subsystems.
 */
#define ARMOR_TYPE_MASK		0x07
#define ARMOR_FRONT		0x00 /* front armor */
#define ARMOR_INTERNAL		0x01 /* internal armor */
#define ARMOR_REAR		0x02 /* rear armor */

#define ARMOR_FLAG_OWNED	0x10 /* armor status by owner */
#define ARMOR_FLAG_SHOW_DEST	0x20 /* show destroyed sections */
#define ARMOR_FLAG_DIVIDE_10	0x40 /* divide displayed value by 10 */

/*
 * Armor levels returned by ArmorEvaluateSerious().
 */
#define ARMOR_LEVEL_GREAT	0
#define ARMOR_LEVEL_GOOD	1
#define ARMOR_LEVEL_LOW		2
#define ARMOR_LEVEL_CRITICAL	3
#define ARMOR_LEVEL_OPEN	4
#define ARMOR_LEVEL_REPAIRING	5

/* mech.status.c */
void DisplayTarget(dbref player, MECH * mech);
void show_miscbrands(MECH * mech, dbref player);
void PrintGenericStatus(dbref player, MECH * mech, int own, int usex);
void PrintHeatBar(dbref player, MECH * mech);
void PrintInfoStatus(dbref player, MECH * mech, int own);
void PrintShortInfo(dbref player, MECH * mech);
void mech_status(dbref player, void *data, char *buffer);
void mech_critstatus(dbref player, void *data, char *buffer);
char *part_name(int type, int brand);
char *part_name_long(int type, int brand);
char *pos_part_name(MECH * mech, int index, int loop);
void mech_weaponspecs(dbref player, void *data, char *buffer);
char *critstatus_func(MECH * mech, char *arg);
char *sectstatus_func(MECH * mech, char *arg);
char *armorstatus_func(MECH * mech, char *arg);
char *weaponstatus_func(MECH * mech, char *arg);
char *critslot_func(MECH * mech, char *buf_section, char *buf_critnum, char *buf_flag);
void CriticalStatus(dbref player, MECH * mech, int index);
char *evaluate_ammo_amount(int now, int max);
void PrintWeaponStatus(MECH * mech, dbref player);
int ArmorEvaluateSerious(MECH * mech, int loc, int flag, int *opt);
void PrintArmorStatus(dbref player, MECH * mech, int owner);
int hasPhysical(MECH * objMech, int wLoc, int wPhysType);
int canUsePhysical(MECH * objMech, int wLoc, int wPhysType);

#endif				/* _P_MECH_STATUS_H */
