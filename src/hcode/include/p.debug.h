
/*
   p.debug.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Feb 22 14:59:36 CET 1999 from debug.c */

#ifndef _P_DEBUG_H
#define _P_DEBUG_H

/* debug.c */
void debug_list(dbref player, void *data, char *buffer);
void debug_savedb(dbref player, void *data, char *buffer);
void debug_loaddb(dbref player, void *data, char *buffer);
void debug_memory(dbref player, void *data, char *buffer);
void ShutDownMap(dbref player, dbref mapnumber);
void debug_shutdown(dbref player, void *data, char *buffer);
void debug_setvrt(dbref player, void *data, char *buffer);

#endif				/* _P_DEBUG_H */
