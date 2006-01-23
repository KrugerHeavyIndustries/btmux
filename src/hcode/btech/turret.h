
/*
 * $Id: turret.h,v 1.1.1.1 2005/01/11 21:18:33 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Fri Nov 22 17:10:35 1996 fingon
 * Last modified: Fri Nov 22 21:30:46 1996 fingon
 *
 */

#ifndef TURRET_H
#define TURRET_H

#include "mech.h"

typedef struct {
    dbref mynum;
    unsigned char type; /* Xcode Type - GTYPE_TURRET */

    int arcs;			/* arc_override */
    unsigned long tic[NUM_TICS];	/* tics.. */
    dbref parent;		/* ship whose stats we use for this */
    dbref gunner;		/* who's da gunner? */
    dbref target;		/* what do we have locked? */
    short targx, targy, targz;	/* in map coords, target squares */
    int lockmode;		/* lock modes (hex, etc) */
} TURRET_T;

#endif				/* TURRET_H */
