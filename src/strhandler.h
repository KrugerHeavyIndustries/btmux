
/*
 * $Id: strhandler.h,v 1.1 2005/06/13 20:50:48 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1998 Markus Stenberg
 *       All rights reserved
 *
 * Created: Thu Jun  4 22:08:16 1998 fingon
 * Last modified: Tue Jun 23 09:55:11 1998 fingon
 *
 */

#ifndef STRHANDLER_H
#define STRHANDLER_H

#include "db.h"
#include "externs.h"

#define MAX_NUM_FIFOS 4
#define MAX_NUM_STRS  62

char *strarray_get_obj_atr(dbref obj, int atr, int qnum, int num);
void strarray_add_obj_atr(dbref obj, int atr, int qnum, int max,
    char *val);
void strarray_add_obj_atr_n(dbref obj, int atr, int qnum, int n,
    char *val);

#endif				/* STRHANDLER_H */
