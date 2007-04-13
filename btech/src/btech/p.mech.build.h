
/*
   p.mech.build.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:45 CET 1999 from mech.build.c */

#ifndef _P_MECH_BUILD_H
#define _P_MECH_BUILD_H

/* mech.build.c */
int CheckData(dbref player, void *data);
void FillDefaultCriticals(MECH * mech, int index);
char *ShortArmorSectionString(char type, char mtype, int loc);
int ArmorSectionFromString(char type, char mtype, char *string);
int WeaponIndexFromString(char *string);
int FindSpecialItemCodeFromString(char *buffer);

#endif				/* _P_MECH_BUILD_H */
