
/*
   p.econ_cmds.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:40 CET 1999 from econ_cmds.c */

#ifndef _P_ECON_CMDS_H
#define _P_ECON_CMDS_H

/* econ_cmds.c */
void SetCargoWeight(MECH * mech);
int loading_bay_whine(dbref player, dbref cargobay, MECH * mech);
void mech_Rfixstuff(dbref player, void *data, char *buffer);
void list_matching(dbref player, char *header, dbref loc, char *buf);
void mech_manifest(dbref player, void *data, char *buffer);
void mech_stores(dbref player, void *data, char *buffer);
void mech_Raddstuff(dbref player, void *data, char *buffer);
void mech_Rremovestuff(dbref player, void *data, char *buffer);
void mech_loadcargo(dbref player, void *data, char *buffer);
void mech_unloadcargo(dbref player, void *data, char *buffer);
void mech_Rresetstuff(dbref player, void *data, char *buffer);

#endif				/* _P_ECON_CMDS_H */
