
/*
 * $Id: muxevent.h,v 1.1 2005/06/22 22:09:45 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Tue Aug 27 19:02:00 1996 fingon
 * Last modified: Sat Jun  6 22:20:36 1998 fingon
 *
 */

#ifndef MUXEVENT_H
#define MUXEVENT_H

/* EVENT_DEBUG adds some useful debugging information to the structure
   / allows more diverse set of error messages to be shown. However,
   for a run-time version it's practically useless. */

/* #undef EVENT_DEBUG */

#define FLAG_FREE_DATA      1	/* Free the 1st data segment after execution */
#define FLAG_FREE_DATA2     2	/* Free the 2nd data segment after execution */
#define FLAG_ZOMBIE         4	/* Exists there just because we're too
				   lazy to search for it everywhere - dud */

/* ZOMBIE events aren't moved/deleted during reschedule, that tends to break
 * stuff.  Instead, they're just deleted when the events are processed.
 */

typedef struct my_event_type MUXEVENT;
struct my_event_type {
    char flags;
    unsigned char type;

    void (*function) (struct my_event_type *);
    void *data;
    void *data2;

    int tick;			/* The tick this baby was first scheduled to go off */

    MUXEVENT *prev;
    MUXEVENT *next;
}; /* struct my_event_type */

/* Some external things _do_ use this one */
extern int muxevent_tick;

/* Multiply evaluates l and v.  Could avoid if we had the types of l and v.  */
/* TODO: Why (l)->p = NULL?  Isn't that already handled by the second if?  Or
 * is the head of the list's prev field not set to NULL already? */
#define REMOVE_FROM_BIDIR_LIST(l,p,n,v) \
	do { \
		if ((v)->p) \
			(v)->p->n = (v)->n; \
		if ((v)->n) \
			(v)->n->p = (v)->p; \
		if ((l) == (v)) { \
			(l) = (v)->n; \
			if (l) \
				(l)->p = NULL; \
		} \
	} while (0)

/* Multiply evaluates l and v.  Could avoid if we had the types of l and v.  */
#define ADD_TO_LIST_HEAD(l,n,v) \
	do { \
		(v)->n = (l); \
		(l) = (v); \
	} while (0)

/* Multiply evaluates l and v.  Could avoid if we had the types of l and v.  */
#define ADD_TO_BIDIR_LIST_HEAD(l,p,n,v) \
	do { \
		(v)->n = (l); \
		if (l) \
			(l)->p = (v); \
		(l) = (v); \
		(v)->p = NULL; \
	} while (0)

#endif				/* MUXEVENT_H */
