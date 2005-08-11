
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

void muxevent_gothru_type_data(int type, void *data, void (*func)(EVENT *))
   Executes the function func for every object in tye first_in_type
   queue matching type, and/or data.


*/


/* NOTE:
   This approach turns _very_ costly, if you have regularly events
   further than LOOKAHEAD_STACK_SIZE in the future
   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "muxevent.h"
#include "create.h"

/* Stack of the events according to timing */
static EVENT *muxevent_stack[LOOKAHEAD_STACK_SIZE];

/* Stack of the events according to date */
static EVENT **muxevent_first_in_type = NULL;

/* Whole list (dual linked) */
static EVENT *muxevent_list = NULL;

/* List of 'free' events */
static EVENT *muxevent_free_list = NULL;

/* Present 'time' (you can calculate position for a new event by just
   knowing current 'tick' and target 'tick') */
int muxevent_tick = 0, muxevent_counter = 0;

static int last_muxevent_type = -1;
int events_scheduled = 0;
int events_executed = 0;
int events_zombies = 0;

/* The main add-to-lists event handling function */

extern void prerun_event(EVENT * e);
extern void postrun_event(EVENT * e);

void muxevent_add(int time, int flags, int type, void (*func) (EVENT *),
    void *data, void *data2)
{
    EVENT *e;

#ifdef EVENT_DEBUG
    int tl, tp;
#endif
    int i, spot;

    events_scheduled++;
    if (time < 1)
	time = 1;
    /* Nasty thing about the new system : we _do_ have to allocate
       muxevent_first_in_type dynamically. */
    if (type > last_muxevent_type) {
	muxevent_first_in_type =
	    realloc(muxevent_first_in_type, sizeof(EVENT *) * (type + 1));
	for (i = last_muxevent_type + 1; i <= type; i++)
	    muxevent_first_in_type[i] = NULL;
	last_muxevent_type = type;
    }
    if (muxevent_free_list) {
	e = muxevent_free_list;
	muxevent_free_list = muxevent_free_list->next;
    } else
	Create(e, EVENT, 1);
    e->flags = flags;
    e->function = func;
    e->data = data;
    e->data2 = data2;
    e->type = type;
    e->tick = muxevent_tick + time;
    e->next = NULL;

    ADD_TO_BIDIR_LIST_HEAD(muxevent_list, prev_in_main, next_in_main, e);
    ADD_TO_BIDIR_LIST_HEAD(muxevent_first_in_type[type], prev_in_type,
	next_in_type, e);
    spot = (muxevent_tick + time) % LOOKAHEAD_STACK_SIZE;
    ADD_TO_LIST_HEAD(muxevent_stack[spot], next, e);
#ifdef EVENT_DEBUG
    e->tick_scheduled = muxevent_tick;
    e->count_0_scheduled_at = muxevent_tick_counter[0];
    e->count_1_scheduled_at = muxevent_tick_counter[1];
    e->count_0_scheduled_to = tl;
    e->count_1_scheduled_to = tp;
#endif
}

/* Remove event */

static void muxevent_delete(EVENT * e, int fp)
{
    if (e->flags & FLAG_FREE_DATA)
	free((void *) e->data);
    if (e->flags & FLAG_FREE_DATA2)
	free((void *) e->data2);
    REMOVE_FROM_BIDIR_LIST(muxevent_list, prev_in_main, next_in_main, e);
    REMOVE_FROM_BIDIR_LIST(muxevent_first_in_type[(int) e->type],
	prev_in_type, next_in_type, e);
    REMOVE_FROM_LIST(muxevent_stack[fp], next, e);
    ADD_TO_LIST_HEAD(muxevent_free_list, next, e);
}

/* Run the thingy */

#define Zombie(e) (e->flags & FLAG_ZOMBIE)
#define LoopType(type,var) \
for (var = muxevent_first_in_type[type] ; var ; var = var->next_in_type) \
if (!Zombie(var))

#define LoopEvent(var) \
for (var = muxevent_list ; var ; var = var->next_in_main) \
if (!Zombie(var))

void muxevent_run()
{
    EVENT *e, *e2;

    muxevent_tick++;
    muxevent_counter = muxevent_tick % LOOKAHEAD_STACK_SIZE;
    for (e = muxevent_stack[muxevent_counter]; e; e = e2) {
	e2 = e->next;
	if (e->tick > muxevent_tick && !Zombie(e))
	    continue;		/* For next tick [bad, very bad] */
	if (!Zombie(e)) {
	    prerun_event(e);
	    e->function(e);
	    postrun_event(e);
	    events_executed++;
	} else
	    events_zombies++;
	muxevent_delete(e, muxevent_counter);
    }
}

int muxevent_run_by_type(int type)
{
    EVENT *e;
    int ran = 0;

    if (type <= last_muxevent_type)
	for (e = muxevent_first_in_type[type]; e; e = e->next_in_type)
	    if (!Zombie(e)) {
		prerun_event(e);
		e->function(e);
		postrun_event(e);
		e->flags |= FLAG_ZOMBIE;
		ran++;
	    }
    return ran;
}

int muxevent_last_type()
{
    return last_muxevent_type;
}

/* Initialize the events */

void muxevent_initialize()
{
    debug("muxevent_initialize\n");
    bzero(muxevent_stack, sizeof(muxevent_stack));
}

/* Event removal functions */

void muxevent_remove_data(void *data)
{
    EVENT *e;

    for (e = muxevent_list; e; e = e->next_in_main)
	if (e->data == data)
	    e->flags |= FLAG_ZOMBIE;
}

void muxevent_remove_type_data(int type, void *data)
{
    EVENT *e;

    if (type > last_muxevent_type)
	return;
    for (e = muxevent_first_in_type[type]; e; e = e->next_in_type)
	if (e->data == data)
	    e->flags |= FLAG_ZOMBIE;
}

void muxevent_remove_type_data2(int type, void *data)
{
    EVENT *e;

    if (type > last_muxevent_type)
	return;
    for (e = muxevent_first_in_type[type]; e; e = e->next_in_type)
	if (e->data2 == data)
	    e->flags |= FLAG_ZOMBIE;
}

void muxevent_remove_type_data_data(int type, void *data, void *data2)
{
    EVENT *e;

    if (type > last_muxevent_type)
	return;
    for (e = muxevent_first_in_type[type]; e; e = e->next_in_type)
	if (e->data == data && e->data2 == data2)
	    e->flags |= FLAG_ZOMBIE;
}



/* return the args of the event */
void muxevent_get_type_data(int type, void *data, int *data2)
{
    EVENT *e;

    LoopType(type, e)
    	if (e->data == data)
	    *data2 = (int) e->data2;
}

/* All the counting / other kinds of 'useless' functions */
int muxevent_count_type(int type)
{
    EVENT *e;
    int count = 0;

    if (type > last_muxevent_type)
	return count;
    LoopType(type, e)
	count++;
    return count;
}


int muxevent_count_type_data(int type, void *data)
{
    EVENT *e;
    int count = 0;

    if (type > last_muxevent_type)
	return count;
    LoopType(type, e)
	if (e->data == data)
	    count++;
    return count;
}

int muxevent_count_type_data2(int type, void *data)
{
    EVENT *e;
    int count = 0;

    if (type > last_muxevent_type)
	return count;
    LoopType(type, e)
	if (e->data2 == data)
	    count++;
    return count;
}

int muxevent_count_type_data_data(int type, void *data, void *data2)
{
    EVENT *e;
    int count = 0;

    if (type > last_muxevent_type)
	return count;
    LoopType(type, e)
	if (e->data == data && e->data2 == data2)
	    count++;
    return count;
}

int muxevent_count_data(int type, void *data)
{
    EVENT *e;
    int count = 0;

    LoopEvent(e)
	if (e->data == data)
	count++;
    return count;
}


int muxevent_count_data_data(int type, void *data, void *data2)
{
    EVENT *e;
    int count = 0;

    LoopEvent(e)
	if (e->data == data && e->data2 == data2)
	count++;
    return count;
}

void muxevent_gothru_type_data(int type, void *data, void (*func) (EVENT *))
{
    EVENT *e;

    if (type > last_muxevent_type)
	return;
    LoopType(type, e)
	if (e->data == data)
	    func(e);
}

void muxevent_gothru_type(int type, void (*func) (EVENT *))
{
    EVENT *e;

    if (type > last_muxevent_type)
	return;
    LoopType(type, e)
	func(e);
}

int muxevent_last_type_data(int type, void *data)
{
    EVENT *e;
    int last = 0, t;

    if (type > last_muxevent_type)
	return last;
    LoopType(type, e)
	if (e->data == data)
	    if ((t = (e->tick - muxevent_tick)) > last)
	    	last = t;
    return last;
}

int muxevent_first_type_data(int type, void *data)
{
    EVENT *e;
    int last = -1, t;

    if (type > last_muxevent_type)
	return last;
    LoopType(type, e)
	if (e->data == data)
	    if ((t = (e->tick - muxevent_tick)) < last || last < 0)
	    	if (t > 0)
		    last = t;
    return last;
}

int muxevent_count_type_data_firstev(int type, void *data)
{
    EVENT *e;

    if (type > last_muxevent_type)
        return -1;
    LoopType(type, e)
        if (e->data == data)
            {
            return (int) (e->data2);
            }
    return -1;
}

