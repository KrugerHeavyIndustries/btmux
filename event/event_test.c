
/*
 * $Id: event_test.c,v 1.2 2005/06/22 22:07:15 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Tue Aug 27 20:08:33 1996 fingon
 * Last modified: Sun Nov 24 01:32:49 1996 fingon
 *
 */

#include <stdio.h>

#include "muxevent.h"

#define number(a,b) (a+rand()%(b-a+1))


#if 0

/* In seconds */
#define TEST_TIME            3600
#else

/* In iterations */
#define TEST_ITERATIONS      1000000
#endif

#ifdef TEST_TIME
#define TEST_MSECS           10000
#define TEST_ITERATIONS      TEST_TIME * (1000000 / TEST_MSECS)
#endif

#define LARGE_NUMBER         500000
#define SIMULTANEOUS_EVENTS  600

#define SHORT_TIME           1
#define LONG_TIME            2


void hellow(MUXEVENT * e)
{
    int n;
    int d = (int) e->data;

#if 1
    int d1, d2;

    d1 = d / LARGE_NUMBER;
    d2 = d % LARGE_NUMBER;
    printf("Hello, world!(%d) - %d\n", d1, d2);
    n = number(0, 10);
#endif
    muxevent_add_simple_arg(!n ? number(1, LONG_TIME) : number(1, SHORT_TIME),
	hellow, (int *) (d + 1));
}

void main()
{
    int i;

    muxevent_initialize();
    for (i = 0; i < SIMULTANEOUS_EVENTS; i++)
	muxevent_add_simple_arg(i + 1, hellow, (int *) (LARGE_NUMBER * i));
    for (i = 0; i < TEST_ITERATIONS; i++) {
#ifdef TEST_TIME
	usleep(TEST_MSECS);
#endif
	muxevent_run();
    }
}
