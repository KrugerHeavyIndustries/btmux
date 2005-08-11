

/*
 * $Id: artillery.h,v 1.1.1.1 2005/01/11 21:18:01 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry 
 *       All rights reserved
 *
 * Created: Thu Sep 12 17:25:22 1996 fingon
 * Last modified: Sun Sep 15 20:35:39 1996 fingon
 *
 */

#ifndef ARTILLERY_H
#define ARTILLERY_H

typedef struct artillery_shot_type {
    int from_x, from_y;		/* hex this is shot from */
    int to_x, to_y;		/* hex this lands in */
    int type;			/* weapon index in MechWeapons */
    int mode;			/* weapon mode */
    int ishit;			/* did we hit target hex? */
    dbref shooter;		/* nice to know type of information */
    dbref map;			/* map we're on */
    struct artillery_shot_type *next;
    /* next in stack of unused things */
} artillery_shot;

/* Weapon values for artillery guns */
#define IS_LTOM       30
#define IS_THUMPER    31
#define IS_SNIPER     32
#define IS_ARROW      27

#define CL_ARROW      71

#endif				/* ARTILLERY_H */
