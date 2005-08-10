
/*
   p.failures.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:41 CET 1999 from failures.c */

#ifndef _P_FAILURES_H
#define _P_FAILURES_H

/* failures.c */
int GetBrandIndex(int type);
char *GetPartBrandName(int type, int level);
void FailureRadioStatic(MECH * mech, int weapnum, int weaptype,
    int section, int critical, int roll, int *modifier, int *type);
void FailureRadioShort(MECH * mech, int weapnum, int weaptype, int section,
    int critical, int roll, int *modifier, int *type);
void FailureRadioRange(MECH * mech, int weapnum, int weaptype, int section,
    int critical, int roll, int *modifier, int *type);
void FailureComputerShutdown(MECH * mech, int weapnum, int weaptype,
    int section, int critical, int roll, int *modifier, int *type);
void FailureComputerScanner(MECH * mech, int weapnum, int weaptype,
    int section, int critical, int roll, int *modifier, int *type);
void FailureComputerTarget(MECH * mech, int weapnum, int weaptype,
    int section, int critical, int roll, int *modifier, int *type);
void FailureWeaponMissiles(MECH * mech, int weapnum, int weaptype,
    int section, int critical, int roll, int *modifier, int *type);
void FailureWeaponDud(MECH * mech, int weapnum, int weaptype, int section,
    int critical, int roll, int *modifier, int *type);
void FailureWeaponJammed(MECH * mech, int weapnum, int weaptype,
    int section, int critical, int roll, int *modifier, int *type);
void FailureWeaponRange(MECH * mech, int weapnum, int weaptype,
    int section, int critical, int roll, int *modifier, int *type);
void FailureWeaponDamage(MECH * mech, int weapnum, int weaptype,
    int section, int critical, int roll, int *modifier, int *type);
void FailureWeaponHeat(MECH * mech, int weapnum, int weaptype, int section,
    int critical, int roll, int *modifier, int *type);
void FailureWeaponSpike(MECH * mech, int weapnum, int weaptype,
    int section, int critical, int roll, int *modifier, int *type);
void CheckGenericFail(MECH * mech, int type, int *result, int *mod);
void CheckWeaponFailed(MECH * mech, int weapnum, int weaptype, int section,
    int critical, int *modifier, int *type);

#endif				/* _P_FAILURES_H */
