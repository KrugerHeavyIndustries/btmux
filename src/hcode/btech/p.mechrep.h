
/*
   p.mechrep.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Tue Feb  9 14:31:38 CET 1999 from mechrep.c */

#include "config.h"

#ifndef _P_MECHREP_H
#define _P_MECHREP_H

/* mechrep.c */
void newfreemechrep(dbref key, void **data, int selector);
void mechrep_Rresetcrits(dbref player, void *data, char *buffer);
void mechrep_Rdisplaysection(dbref player, void *data, char *buffer);
void mechrep_Rsetradio(dbref player, void *data, char *buffer);
void mechrep_Rsettarget(dbref player, void *data, char *buffer);
void mechrep_Rsettype(dbref player, void *data, char *buffer);
void mechrep_Rsetspeed(dbref player, void *data, char *buffer);
void mechrep_Rsetjumpspeed(dbref player, void *data, char *buffer);
void mechrep_Rsetheatsinks(dbref player, void *data, char *buffer);
void mechrep_Rsetlrsrange(dbref player, void *data, char *buffer);
void mechrep_Rsettacrange(dbref player, void *data, char *buffer);
void mechrep_Rsetscanrange(dbref player, void *data, char *buffer);
void mechrep_Rsetradiorange(dbref player, void *data, char *buffer);
void mechrep_Rsettons(dbref player, void *data, char *buffer);
void mechrep_Rsetmove(dbref player, void *data, char *buffer);
void mechrep_Rloadnew(dbref player, void *data, char *buffer);
void clear_mech(MECH * mech, int flag);
char *mechref_path(char *id);
int load_mechdata2(dbref player, MECH * mech, char *id);
int unable_to_find_proper_type(int i);
int load_mechdata(MECH * mech, char *id);
int mech_loadnew(dbref player, MECH * mech, char *id);
MECH *load_refmech(char *reference);
void mechrep_Rrestore(dbref player, void *data, char *buffer);
void mechrep_Rsavetemp(dbref player, void *data, char *buffer);
void mechrep_Rsavetemp2(dbref player, void *data, char *buffer);
void mechrep_Rsetarmor(dbref player, void *data, char *buffer);
void mechrep_Raddweap(dbref player, void *data, char *buffer);
void mechrep_Rreload(dbref player, void *data, char *buffer);
void mechrep_Rrepair(dbref player, void *data, char *buffer);
void mechrep_Raddspecial(dbref player, void *data, char *buffer);
char *techstatus_func(MECH * mech);
void mechrep_Rshowtech(dbref player, void *data, char *buffer);
char *mechrep_gettechstring(MECH *mech);
void mechrep_Rdeltech(dbref player, void *data, char *buffer);
void mechrep_Raddtech(dbref player, void *data, char *buffer);
void mechrep_Rdelinftech(dbref player, void *data, char *buffer);
void mechrep_Raddinftech(dbref player, void *data, char *buffer);
void mechrep_setcargospace(dbref player, void *data, char *buffer);
void invalid_section(dbref player, MECH * mech);

#endif				/* _P_MECHREP_H */
