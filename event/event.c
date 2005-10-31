
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


/* NOTE:
   This approach turns _very_ costly, if you have regularly events
   further than LOOKAHEAD_STACK_SIZE in the future
   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <event.h>

#include "muxevent.h"
#include "create.h"
#include "debug.h"

int muxevent_tick = 0;

/* Stack of the events according to date */
static MUXEVENT **muxevent_first_in_type = NULL;

/* Whole list (dual linked) */
static MUXEVENT *muxevent_list = NULL;

/* List of 'free' events */
static MUXEVENT *muxevent_free_list = NULL;

static int last_muxevent_type = -1;
/* The main add-to-lists event handling function */

extern void prerun_event(MUXEVENT * e);
extern void postrun_event(MUXEVENT * e);

static void muxevent_delete(MUXEVENT *);

#define Zombie(e) (e->flags & FLAG_ZOMBIE)
#define LoopType(type,var) \
    for (var = muxevent_first_in_type[type] ; var ; var = var->next_in_type) \
if (!Zombie(var))

#define LoopEvent(var) \
    for (var = muxevent_list ; var ; var = var->next_in_main) \
if (!Zombie(var))

static void muxevent_wakeup(int fd, short event, void *arg) {
    MUXEVENT *e = (MUXEVENT *)arg;

    if(Zombie(e)) {
        muxevent_delete(e);
        return;
    }
    prerun_event(e);
    e->function(e);
    postrun_event(e);
    muxevent_delete(e);
}

void muxevent_add(int time, int flags, int type, void (*func) (MUXEVENT *),
        void *data, void *data2) {
    MUXEVENT *e;
    struct timeval tv;

    int i, spot;

    if (time < 1)
        time = 1;
    /* Nasty thing about the new system : we _do_ have to allocate
       muxevent_first_in_type dynamically. */
    if (type > last_muxevent_type) {
        muxevent_first_in_type =
            realloc(muxevent_first_in_type, sizeof(MUXEVENT *) * (type + 1));
        for (i = last_muxevent_type + 1; i <= type; i++)
            muxevent_first_in_type[i] = NULL;
        last_muxevent_type = type;
    }
    if (muxevent_free_list) {
        e = muxevent_free_list;
        muxevent_free_list = muxevent_free_list->next;
    } else
        Create(e, MUXEVENT, 1);

    e->flags = flags;
    e->function = func;
    e->data = data;
    e->data2 = data2;
    e->type = type;
    e->tick = muxevent_tick + time;
    e->next = NULL;


    tv.tv_sec = time;
    tv.tv_usec = 0;
    
    evtimer_set(&e->ev, muxevent_wakeup, e);
    evtimer_add(&e->ev, &tv);

    ADD_TO_BIDIR_LIST_HEAD(muxevent_list, prev_in_main, next_in_main, e);
    ADD_TO_BIDIR_LIST_HEAD(muxevent_first_in_type[type], prev_in_type,
            next_in_type, e);
}

/* Remove event */

static void muxevent_delete(MUXEVENT * e) {
    if(evtimer_pending(&e->ev, NULL)) {
        evtimer_del(&e->ev);
    }

    if (e->flags & FLAG_FREE_DATA)
        free((void *) e->data);
    if (e->flags & FLAG_FREE_DATA2)
        free((void *) e->data2);

    REMOVE_FROM_BIDIR_LIST(muxevent_list, prev_in_main, next_in_main, e);
    REMOVE_FROM_BIDIR_LIST(muxevent_first_in_type[(int) e->type],
            prev_in_type, next_in_type, e);
    ADD_TO_LIST_HEAD(muxevent_free_list, next, e);
}

/* Run the thingy */

void muxevent_run() {
    muxevent_tick += 1;
}

int muxevent_run_by_type(int type) {
    MUXEVENT *e;
    int ran = 0;

    if (type <= last_muxevent_type) {
        for (e = muxevent_first_in_type[type]; e; e = e->next_in_type) {
            if (!Zombie(e)) {
                prerun_event(e);
                e->function(e);
                postrun_event(e);
                e->flags |= FLAG_ZOMBIE;
                ran++;
            }
        }
    }
    return ran;
}

int muxevent_last_type() {
    return last_muxevent_type;
}

/* Initialize the events */

void muxevent_initialize() {
    dprintk("muxevent initializing");
}

/* Event removal functions */

void muxevent_remove_data(void *data) {
    MUXEVENT *e;

    for (e = muxevent_list; e; e = e->next_in_main)
        if (e->data == data)
            e->flags |= FLAG_ZOMBIE;
}

void muxevent_remove_type_data(int type, void *data) {
    MUXEVENT *e;

    if (type > last_muxevent_type)
        return;
    for (e = muxevent_first_in_type[type]; e; e = e->next_in_type)
        if (e->data == data)
            e->flags |= FLAG_ZOMBIE;
}

void muxevent_remove_type_data2(int type, void *data) {
    MUXEVENT *e;

    if (type > last_muxevent_type)
        return;
    for (e = muxevent_first_in_type[type]; e; e = e->next_in_type)
        if (e->data2 == data)
            e->flags |= FLAG_ZOMBIE;
}

void muxevent_remove_type_data_data(int type, void *data, void *data2) {
    MUXEVENT *e;

    if (type > last_muxevent_type)
        return;
    for (e = muxevent_first_in_type[type]; e; e = e->next_in_type)
        if (e->data == data && e->data2 == data2)
            e->flags |= FLAG_ZOMBIE;
}



/* return the args of the event */
void muxevent_get_type_data(int type, void *data, int *data2) {
    MUXEVENT *e;

    LoopType(type, e)
        if (e->data == data)
            *data2 = (int) e->data2;
}

/* All the counting / other kinds of 'useless' functions */
int muxevent_count_type(int type) {
    MUXEVENT *e;
    int count = 0;

    if (type > last_muxevent_type)
        return count;
    LoopType(type, e)
        count++;
    return count;
}


int muxevent_count_type_data(int type, void *data) {
    MUXEVENT *e;
    int count = 0;

    if (type > last_muxevent_type)
        return count;
    LoopType(type, e)
        if (e->data == data)
            count++;
    return count;
}

int muxevent_count_type_data2(int type, void *data) {
    MUXEVENT *e;
    int count = 0;

    if (type > last_muxevent_type)
        return count;
    LoopType(type, e)
        if (e->data2 == data)
            count++;
    return count;
}

int muxevent_count_type_data_data(int type, void *data, void *data2) {
    MUXEVENT *e;
    int count = 0;

    if (type > last_muxevent_type)
        return count;
    LoopType(type, e)
        if (e->data == data && e->data2 == data2)
            count++;
    return count;
}

int muxevent_count_data(int type, void *data) {
    MUXEVENT *e;
    int count = 0;

    LoopEvent(e)
        if (e->data == data)
            count++;
    return count;
}


int muxevent_count_data_data(int type, void *data, void *data2) {
    MUXEVENT *e;
    int count = 0;

    LoopEvent(e)
        if (e->data == data && e->data2 == data2)
            count++;
    return count;
}

void muxevent_gothru_type_data(int type, void *data, void (*func) (MUXEVENT *)) {
    MUXEVENT *e;

    if (type > last_muxevent_type)
        return;
    LoopType(type, e)
        if (e->data == data)
            func(e);
}

void muxevent_gothru_type(int type, void (*func) (MUXEVENT *)) {
    MUXEVENT *e;

    if (type > last_muxevent_type)
        return;
    LoopType(type, e)
        func(e);
}

int muxevent_last_type_data(int type, void *data) {
    MUXEVENT *e;
    int last = 0, t;

    if (type > last_muxevent_type)
        return last;
    LoopType(type, e)
        if (e->data == data)
            if ((t = (e->tick - muxevent_tick)) > last)
                last = t;
    return last;
}

int muxevent_first_type_data(int type, void *data) {
    MUXEVENT *e;
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

int muxevent_count_type_data_firstev(int type, void *data) {
    MUXEVENT *e;

    if (type > last_muxevent_type)
        return -1;
    LoopType(type, e)
        if (e->data == data)
        {
            return (int) (e->data2);
        }
    return -1;
}

