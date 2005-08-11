
/*
 * $Id: mech.partnames.h,v 1.1.1.1 2005/01/11 21:18:21 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Sun Mar  9 14:02:41 1997 fingon
 * Last modified: Sat Jun  6 21:51:41 1998 fingon
 *
 */

#ifndef MECH_PARTNAMES_H
#define MECH_PARTNAMES_H

typedef struct {
    char *shorty;
    char *longy;
    char *vlongy;
    int index;
} PN;

extern PN **short_sorted;
extern PN **long_sorted;
extern PN **vlong_sorted;
extern int object_count;

#define PACKED_PART(id, brand) (NUM_ITEMS * brand + id)
#define UNPACK_PART(from,id,brand) \
id = from % NUM_ITEMS; brand = from / NUM_ITEMS

char *get_parts_short_name(int, int);
char *get_parts_long_name(int, int);
char *get_parts_vlong_name(int, int);

#include "p.mech.partnames.h"

#endif				/* MECH_PARTNAMES_H */
