
/*
 * $Id: event.c,v 1.3 2005/06/23 22:02:10 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 *
 * Created: Tue Aug 27 19:01:55 1996 fingon
 * Last modified: Tue Nov 10 16:21:43 1998 fingon
 *
 */

/* Interface for creating pretty damn nasty timed events, with
   additional load balancing in the works.

   Description of the interface:

   void muxevent_add()

   Adds a new event to occur <time> ticks from now on, which calls
   function func with the present event as parameter, and with data as
   the data (also optional type can be supplied ; just makes deletion
   of stuff of particular type far faster, and allows nice statistics)

   void muxevent_initialize()
   Initializes the event system

   void muxevent_run()
   Runs one 'tick' of events (second, 1/10sec, whatever)

   int muxevent_count_type(int type)
   int muxevent_count_type_data(int type, void *data)
   int muxevent_count_data(void *data)
   Counts pending events (count_type is fast ; count_type_data relatively
   slow and count_data a dog)
   int muxevent_last_type()
   Returns # of the last type that has been used
   int muxevent_last_type_data(int type, void *data)
   Finds the event furthest in the future and returns the difference
   in seconds to present time (or actually in event ticks)

   void muxevent_gothru_type_data(int type, void *data, void (*func)(MUXEVENT *))
   Executes the function func for every object in tye first_in_type
   queue matching type, and/or data.


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include "muxevent.h"
#include "debug.h"


#include <assert.h>
#define BROKEN fprintf(stderr, "broken: " __FILE__ ": %s()\n", __FUNCTION__)

/*
 * New priority queue-based code.
 *
 * This code is much more deterministic than the old code; it runs precisely
 * during muxevent_run(), more or less.  Unless you use muxevent_run_by_type().
 * But still, more deterministic and synchronous than the old libevent code.
 */
#include "pqueue.h"

/* Need to collect stats on how many events are actually queued, in general.
 * My allocator is smart enough to amortize the cost of expanding and shrinking
 * the queue heap, but it'd be nice to have a good default.  Or we could make
 * it yet another configuration item...
 */
#define MUXEVENT_PQ_MIN 16	/* minimum item storage for muxevent_pq */

typedef struct pq_event_type PQ_EVENT;
struct pq_event_type {
	int tick;		/* redundant; can we remove from MUXEVENT? */
	MUXEVENT *event;	/* can just include this if we can slim down */
}; /* struct pq_event_type */

static int
pq_event_comp(const bt_pq_item_t *generic_a, const bt_pq_item_t *generic_b)
{
	const PQ_EVENT *a = (const PQ_EVENT *)generic_a;
	const PQ_EVENT *b = (const PQ_EVENT *)generic_b;

	return a->tick - b->tick;
}


/* Initialize muxevent_pq.  */
static bt_pq_t *muxevent_pq = NULL;

static void
pq_init(void)
{
	if (!(muxevent_pq = bt_pq_create(MUXEVENT_PQ_MIN,
	                                 sizeof(PQ_EVENT), pq_event_comp))) {
		/* FIXME: Uh oh, trouble.  Out of memory! */
		BROKEN;
		return;
	}
}


#define pq_size() (bt_pq_nitems(muxevent_pq))
#define pq_is_empty() (pq_size() == 0)

static PQ_EVENT *
pq_peek(void)
{
	if (pq_is_empty()) {
		return NULL;
	}

	return (PQ_EVENT *)bt_pq_peek_top(muxevent_pq);
}

static void
pq_push(int tick, MUXEVENT *event)
{
	const PQ_EVENT tmp_item = { tick, event };

	assert(tick == event->tick);

	if (!bt_pq_add_item(muxevent_pq, (const bt_pq_item_t *)&tmp_item)) {
		/* FIXME: Uh oh, trouble.  Out of memory! */
		BROKEN;
		return;
	}
}

/* Randomly deleting from this priority queue implementation tends to be a bit
 * inefficient, since it rearranges itself and you have to locate the next item
 * from scratch.  So we only support popping the top item, and use the old
 * FLAG_ZOMBIE code.
 *
 * An implementation that used trees and explicit pointers could save a
 * reference in the MUXEVENT object, but would be less space and cache
 * efficient than the current version that uses an array-based heap.
 */
static MUXEVENT *
pq_pop(void)
{
	const bt_pq_item_t *tmp_item;
	MUXEVENT *event;

	if (pq_is_empty()) {
		return NULL;
	}

	tmp_item = bt_pq_peek_top(muxevent_pq);
	event = ((const PQ_EVENT *)tmp_item)->event;

	if (!bt_pq_remove_item(muxevent_pq, (const bt_pq_item_t *)tmp_item)) {
		/* This should just never happen.  */
		BROKEN;
		return NULL;
	}

	return event;
}


/* Global muxevent ticky tock.  */
int muxevent_tick = 0;

/* Stack of the events according to type */
static MUXEVENT **muxevent_first_in_type = NULL;
static int last_muxevent_type = -1;

/* List of 'free' events */
static MUXEVENT *muxevent_free_list = NULL;

extern void prerun_event(MUXEVENT * e);
extern void postrun_event(MUXEVENT * e);

#define Zombie(e) (e->flags & FLAG_ZOMBIE)

#define LoopEvent_BEGIN(var) \
	{ \
		int t_ii; \
		\
		for (t_ii = 0; t_ii < pq_size(); t_ii++) { \
			const PQ_EVENT *const t_pq_e \
			= (const PQ_EVENT *)bt_pq_get_item(muxevent_pq, t_ii); \
			(var) = t_pq_e->event; \
			if (!Zombie(var))

#define LoopEvent_END \
		} \
	}

#define LoopType(type,var) \
	if (type <= last_muxevent_type) \
		for (var = muxevent_first_in_type[type]; var; var = var->next) \
			if (!Zombie(var))


/* Run events of all types by expiration time.  */
void
muxevent_run(void)
{
	muxevent_tick += 1;

	/* Run expired events.  */
	while (!pq_is_empty()) {
		const PQ_EVENT *const t_pq_e = pq_peek();

		/* Check if the event has expired.  */
		MUXEVENT *e;

		if (t_pq_e->tick > muxevent_tick) {
			/* Finished.  */
			return;
		}

		e = t_pq_e->event;

		pq_pop();

		/* Run expired event.  */
		if (!Zombie(e)) {
			/* TODO: prerun_event/postrun_event are legacy cruft? */
			prerun_event(e);
			e->function(e);
			postrun_event(e);
		}

		/* Release MUXEVENT.  */
		REMOVE_FROM_BIDIR_LIST(muxevent_first_in_type[(int) e->type],
		                       prev, next, e);

		if (e->flags & FLAG_FREE_DATA)
			free(e->data);
		if (e->flags & FLAG_FREE_DATA2)
			free(e->data2);

		ADD_TO_LIST_HEAD(muxevent_free_list, next, e);
	}
}

/* Run events of type "type", regardless of expiration time.  */
int
muxevent_run_by_type(int type)
{
	MUXEVENT *e;

	int ran = 0;

	LoopType(type, e) {
		prerun_event(e);
		e->function(e);
		postrun_event(e);

		e->flags |= FLAG_ZOMBIE;

		ran++;
	}

	return ran;
}


/* Add MUXEVENT with an expiration time and type.  */
void
muxevent_add(int time, int flags, int type, void (*func)(MUXEVENT *),
             void *data, void *data2)
{
	MUXEVENT *e;

	/* XXX: MUXEVENT.type is only an unsigned char in size.  Somewhat
	 * pointless, since structure padding will ensure space for a full int
	 * is used there anyway, but whatever.
	 */
	assert(type == (unsigned char)type);

	/* Nasty thing about the new system : we _do_ have to allocate
	   muxevent_first_in_type dynamically. */
	if (type > last_muxevent_type) {
		MUXEVENT **tmp_first_in_type;
		int i;

		tmp_first_in_type
		= (MUXEVENT **)realloc(muxevent_first_in_type,
		                       sizeof(MUXEVENT *) * (type + 1));
		if (!tmp_first_in_type) {
			/* FIXME: Uh oh, trouble.  Out of memory! */
			BROKEN;
			return;
		}

		for (i = last_muxevent_type + 1; i <= type; i++)
			tmp_first_in_type[i] = NULL;

		muxevent_first_in_type = tmp_first_in_type;
		last_muxevent_type = type;
	}

	/* Get a MUXEVENT, preferring one from the free list to a new one.  */
	if (muxevent_free_list) {
		e = muxevent_free_list;
		muxevent_free_list = muxevent_free_list->next;
	} else {
		e = malloc(sizeof(MUXEVENT));
		memset(e, 0, sizeof(MUXEVENT));
	}

	/* Initialize MUXEVENT.  */
	if (time < 1)
		time = 1;

	e->flags = flags;
	e->function = func;
	e->data = data;
	e->data2 = data2;
	e->type = type;
	e->tick = muxevent_tick + time;

	ADD_TO_BIDIR_LIST_HEAD(muxevent_first_in_type[type], prev, next, e);

	/* XXX: Events with the same expiration time will not necessarily run
	 * in the exact same order they were scheduled.  Hopefully, none of the
	 * code is braindead enough to rely on such behavior.
	 *
	 * If you really want such behavior, the easiest thing to do would be
	 * to add a "sequence" field to the PQ_EVENT type, and increment it for
	 * each event during a particular tick.  At the next tick, the sequence
	 * counter would be reset to 0.  The pq_event_comp() function could
	 * then use this sequence field to prioritize by insertion order.
	 *
	 * Again, though, that would be kinda silly.
	 */
	pq_push(e->tick, e);
}


/* Initialize the events */
void
muxevent_initialize(void)
{
	dprintk("muxevent initializing");
	pq_init();
}

/* Event removal functions */
void
muxevent_remove_data(void *data)
{
	MUXEVENT *e;

	LoopEvent_BEGIN(e) {
		if (e->data == data)
			e->flags |= FLAG_ZOMBIE;
	} LoopEvent_END
}

void
muxevent_remove_type_data(int type, void *data)
{
	MUXEVENT *e;

	LoopType(type, e) {
		if (e->data == data)
			e->flags |= FLAG_ZOMBIE;
	}
}

void
muxevent_remove_type_data2(int type, void *data)
{
	MUXEVENT *e;

	LoopType(type, e) {
		if (e->data2 == data)
			e->flags |= FLAG_ZOMBIE;
	}
}

void
muxevent_remove_type_data_data(int type, void *data, void *data2)
{
	MUXEVENT *e;

	LoopType(type, e) {
		if (e->data == data && e->data2 == data2)
			e->flags |= FLAG_ZOMBIE;
	}
}



/* return the args of the event */
void
muxevent_get_type_data(int type, void *data, int *data2)
{
	MUXEVENT *e;

	LoopType(type, e) {
		if (e->data == data)
			*data2 = (int) e->data2;
	}
}

/* All the counting / other kinds of 'useless' functions */
int
muxevent_last_type(void)
{
	return last_muxevent_type;
}

int
muxevent_count_type(int type)
{
	MUXEVENT *e;

	int count = 0;

	LoopType(type, e) {
		count++;
	}

	return count;
}

int
muxevent_count_data(int type, void *data)
{
	MUXEVENT *e;

	int count = 0;

	LoopEvent_BEGIN(e) {
		if (e->data == data)
			count++;
	} LoopEvent_END

	return count;
}


int
muxevent_count_data_data(int type, void *data, void *data2)
{
	MUXEVENT *e;

	int count = 0;

	LoopEvent_BEGIN(e) {
		if (e->data == data && e->data2 == data2)
			count++;
	} LoopEvent_END

	return count;
}

int
muxevent_count_type_data(int type, void *data)
{
	MUXEVENT *e;

	int count = 0;

	LoopType(type, e) {
		if (e->data == data)
			count++;
	}

	return count;
}

int
muxevent_count_type_data2(int type, void *data)
{
	MUXEVENT *e;

	int count = 0;

	LoopType(type, e) {
		if (e->data2 == data)
			count++;
	}

	return count;
}

int
muxevent_count_type_data_data(int type, void *data, void *data2)
{
	MUXEVENT *e;

	int count = 0;

	LoopType(type, e) {
		if (e->data == data && e->data2 == data2)
			count++;
	}

	return count;
}

void
muxevent_gothru_type(int type, void (*func)(MUXEVENT *))
{
	MUXEVENT *e;

	LoopType(type, e) {
		func(e);
	}
}

void
muxevent_gothru_type_data(int type, void *data, void (*func)(MUXEVENT *))
{
	MUXEVENT *e;

	LoopType(type, e) {
		if (e->data == data)
			func(e);
	}
}

int
muxevent_first_type_data(int type, void *data)
{
	MUXEVENT *e;

	int last = -1;

	LoopType(type, e) {
		if (e->data == data) {
			const int t = e->tick - muxevent_tick;

			if (t > 0 && (t < last || last < 0))
				last = t;
		}
	}

	return last;
}

int
muxevent_last_type_data(int type, void *data)
{
	MUXEVENT *e;

	int last = 0;

	LoopType(type, e) {
		if (e->data == data) {
			const int t = e->tick - muxevent_tick;

			if (t > last)
				last = t;
		}
	}

	return last;
}

int
muxevent_count_type_data_firstev(int type, void *data)
{
	MUXEVENT *e;

	LoopType(type, e) {
		if (e->data == data)
			return (int) (e->data2);
	}

	return -1;
}
