
/*
   p.mech.tic.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:33:01 CET 1999 from mech.tic.c */

#ifndef _P_MECH_TIC_H
#define _P_MECH_TIC_H

/* mech.tic.c */
int cleartic_sub_func(MECH * mech, dbref player, int low, int high);
void cleartic_sub(dbref player, MECH * mech, char *buffer);
int addtic_sub_func(MECH * mech, dbref player, int low, int high);
void addtic_sub(dbref player, MECH * mech, char *buffer);
int deltic_sub_func(MECH * mech, dbref player, int low, int high);
void deltic_sub(dbref player, MECH * mech, char *buffer);
int firetic_sub_func(MECH * mech, dbref player, int low, int high);
void firetic_sub(dbref player, MECH * mech, char *buffer);
void listtic_sub(dbref player, MECH * mech, char *buffer);
void mech_cleartic(dbref player, void *data, char *buffer);
void mech_addtic(dbref player, void *data, char *buffer);
void mech_deltic(dbref player, void *data, char *buffer);
void mech_firetic(dbref player, void *data, char *buffer);
void mech_listtic(dbref player, void *data, char *buffer);
void heat_cutoff(dbref player, void *data, char *buffer);
#endif				/* _P_MECH_TIC_H */
