
/*
   p.mech.avail.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:44 CET 1999 from mech.avail.c */

#ifndef _P_MECH_AVAIL_H
#define _P_MECH_AVAIL_H

/* mech.avail.c */
void debug_makemechsub(dbref player, void *data, char *buffer);
void fun_btmakemechsub(char *buff, char **bufc, dbref player, dbref cause,
    char *fargs[], int nfargs, char *cargs[], int ncargs);

#endif				/* _P_MECH_AVAIL_H */
