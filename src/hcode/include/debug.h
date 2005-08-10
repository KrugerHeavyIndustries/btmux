
/*
 * $Id: debug.h,v 1.1 2005/06/13 20:50:52 murrayma Exp $
 *
 * Last modified: Sun Nov  3 19:47:34 1996 fingon
 *
 */

#ifndef DEBUG_H
#define DEBUG_H
#ifndef DOCHECK
#define DOCHECK(a,b) if (a) { notify(player, b); return; }
#endif
void debug_allocfree(dbref key, void **data, int selector);
void debug_list(dbref player, void *data, char *buffer);
void debug_savedb(dbref player, void *data, char *buffer);
void debug_loaddb(dbref player, void *data, char *buffer);
void debug_shutdown(dbref player, void *data, char *buffer);
#endif				/* DEBUG_H */
