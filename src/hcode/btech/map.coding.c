
/*
 * $Id: map.coding.c,v 1.1.1.1 2005/01/11 21:18:07 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Tue Oct  8 16:46:12 1996 fingon
 * Last modified: Sat Jun  6 21:44:08 1998 fingon
 *
 */

/* Simple coding scheme to reduce space used by map hexes to 1 byte/hex */

/* NOTE: if we _ever_ use more than 255 terrain/elevation combinations,
   this code becomes a bomb. */

#include <string.h>
#include "map.coding.h"

static int first_free = 0;

/* terr / height -> index mapping */
static unsigned char data_to_id[LASTCHAR][ELEVATIONS];

typedef struct hex_struct {
	char terrain;
	char elev;
} HS;

static HS id_to_data[ENTRIES];

void init_map_coding()
{
	bzero(id_to_data, sizeof(id_to_data));
	bzero(data_to_id, sizeof(data_to_id));
}

int Coding_GetIndex(char terrain, char elevation)
{
	int i;

	if((i = data_to_id[(short) terrain][(short) elevation]))
		return i - 1;
	id_to_data[first_free].terrain = terrain;
	id_to_data[first_free].elev = elevation;
	first_free++;
	data_to_id[(short) terrain][(short) elevation] = first_free;
	return first_free - 1;
}

char Coding_GetElevation(int index)
{
	return id_to_data[index].elev;
}

char Coding_GetTerrain(int index)
{
	return id_to_data[index].terrain;
}
