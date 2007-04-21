
/*
   p.mechfile.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Feb 22 14:59:39 CET 1999 from mechfile.c */

#ifndef _P_MECHFILE_H
#define _P_MECHFILE_H

/* mechfile.c */
FILE *my_open_file(char *name, char *mode, int *openway);
void my_close_file(FILE * f, int *openway);

#endif				/* _P_MECHFILE_H */
