
/* db.h */

#include "copyright.h"

#ifndef	__DB_H
#define	__DB_H

#include "config.h"
#include "mudconf.h" /* for LBUF_SIZE */

#include "btpr_db.h"

/* Database format information */

/* special dbref's */
#define	NOTHING		(-1)	/* null dbref */
#define NOSLAVE         (-5)	/* Don't send to slaves */

extern dbref btpr_Location(dbref) __attribute__ ((pure));
extern dbref btpr_Contents(dbref) __attribute__ ((pure));
extern dbref btpr_Next(dbref) __attribute__ ((pure));
extern dbref btpr_Owner(dbref) __attribute__ ((pure));

#define Location btpr_Location
#define Contents btpr_Contents
#define Next btpr_Next
#define Owner btpr_Owner

extern void destroy_thing(dbref);

#define	DOLIST(thing,list) \
	for ((thing)=(list); \
	     ((thing)!=NOTHING) && (Next(thing)!=(thing)); \
	     (thing)=Next(thing))
/* TODO: The MUX version of this macro didn't check if next == NOTHING before
 * assigning next = Next(next).  This wouldn't cause a crash 99% of the time,
 * but it's still wrong.  While I was at it, I made things slightly more
 * symmetrical.
 */
#define	SAFE_DOLIST(thing,next,list) \
	for ((thing)=(list),(next)=((thing)==NOTHING ? NOTHING: Next(thing)); \
	     (thing)!=NOTHING && (Next(thing)!=(thing)); \
	     (thing)=(next),(next)=((thing)==NOTHING ? NOTHING: Next(thing)))
#define	DO_WHOLE_DB(thing) \
	for ((thing)=0; (thing)<MUDSTATE_DB_TOP; (thing)++)

#endif				/* __DB_H */
