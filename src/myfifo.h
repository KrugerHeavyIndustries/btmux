
/*
 * $Id: myfifo.h,v 1.1 2005/06/13 20:50:47 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Sun Dec  1 11:46:22 1996 fingon
 * Last modified: Sun Dec  1 12:43:04 1996 fingon
 *
 */

#ifndef MYFIFO_H
#define MYFIFO_H

typedef struct myfifo_entry_struct {
    void *data;
    struct myfifo_entry_struct *next;
    struct myfifo_entry_struct *prev;
} myfifo_e;

typedef struct myfifo_struct {
    myfifo_e *first;		/* First entry (last put in) */
    myfifo_e *last;		/* Last entry (first to get out) */
    int count;			/* Number of entries in the queue */
} myfifo;

/* myfifo.c */
int myfifo_length(myfifo ** foo);
void *myfifo_pop(myfifo ** foo);
void myfifo_push(myfifo ** foo, void *data);
void myfifo_trav(myfifo ** foo, void (*func) ());
void myfifo_trav_r(myfifo ** foo, void (*func) ());

#endif				/* MYFIFO_H */
