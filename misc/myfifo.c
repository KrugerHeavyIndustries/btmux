
/* 
 * $Id: myfifo.c,v 1.3 2005/08/08 09:43:05 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Sun Dec  1 11:46:18 1996 fingon
 * Last modified: Sun Dec  1 12:43:01 1996 fingon
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include "myfifo.h"

/* A little shortcut to save me some typing */
#define PFOO   (*foo)

void check_fifo(myfifo ** foo)
{
    if (PFOO == NULL) {
	PFOO = (myfifo *) malloc(sizeof(myfifo));
	PFOO->first = NULL;
	PFOO->last = NULL;
	PFOO->count = 0;
    }
}

int myfifo_length(myfifo ** foo)
{
    check_fifo(foo);
    return PFOO->count;
}

void *myfifo_pop(myfifo ** foo)
{
    void *tmpd;
    myfifo_e *tmp;

    check_fifo(foo);
    tmp = PFOO->last;
    /* Is the list empty? */
    if (tmp != NULL) {
	/* Are we removeing the only element? */
	if (PFOO->first == PFOO->last) {
	    PFOO->first = NULL;
	    PFOO->last = NULL;
	} else
	    tmp->prev->next = NULL;
	PFOO->last = tmp->prev;
	/* Are we going down to only one element? */
	if (PFOO->last->prev == NULL)
	    PFOO->first = PFOO->last;
	PFOO->count--;
	tmpd = tmp->data;
	free(tmp);
	return tmpd;
    } else
	return NULL;
}

void myfifo_push(myfifo ** foo, void *data)
{
    myfifo_e *tmp;

    check_fifo(foo);
    tmp = (myfifo_e *) malloc(sizeof(myfifo_e));
    tmp->data = data;
    tmp->next = PFOO->first;
    tmp->prev = NULL;
    PFOO->count++;
    if (PFOO->first == NULL) {
	PFOO->first = tmp;
	PFOO->last = tmp;
    } else
	PFOO->first->prev = tmp;
    PFOO->first = tmp;
}

void myfifo_trav(myfifo ** foo, void (*func) ())
{
    myfifo_e *tmp;

    check_fifo(foo);
    for (tmp = PFOO->first; tmp != NULL; tmp = tmp->next)
	func(tmp->data);
}

void myfifo_trav_r(myfifo ** foo, void (*func) ())
{
    myfifo_e *tmp;

    check_fifo(foo);
    for (tmp = PFOO->last; tmp != NULL; tmp = tmp->prev)
	func(tmp->data);
}
