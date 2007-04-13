
/*
   p.bsuit.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Mar 22 08:51:11 CET 1999 from bsuit.c */

#ifndef _P_BSUIT_H
#define _P_BSUIT_H

/* bsuit.c */
char *GetBSuitName(MECH * mech);
char *GetLCaseBSuitName(MECH * mech);
void StartBSuitRecycle(MECH * mech, int time);
void StopSwarming(MECH * mech, int intentional);
int CountSwarmers(MECH * mech);
MECH *findSwarmers(MECH * mech);
void StopBSuitSwarmers(MAP * map, MECH * mech, int intentional);
int IsMechSwarmed(MECH * mech);
int IsMechMounted(MECH * mech);
void BSuitMirrorSwarmedTarget(MAP * map, MECH * mech);
int doBSuitCommonChecks(MECH * mech, dbref player);
int CountBSuitMembers(MECH * mech);
int FindBSuitTarget(dbref player, MECH * mech, MECH ** target,
    char *buffer);
int doJettisonChecks(MECH * mech);
void bsuit_swarm(dbref player, void *data, char *buffer);
void bsuit_attackleg(dbref player, void *data, char *buffer);
void bsuit_hide(dbref player, void *data, char *buffer);
void JettisonPacks(dbref player, void *data, char *buffer);

#endif				/* _P_BSUIT_H */
