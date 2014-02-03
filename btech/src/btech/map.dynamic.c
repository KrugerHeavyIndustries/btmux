
/*
 * $Id: map.dynamic.c,v 1.1.1.1 2005/01/11 21:18:08 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Sun Oct 13 19:38:31 1996 fingon
 * Last modified: Sun Jun 14 14:54:11 1998 fingon
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "create.h"
#include "mech.h"
#include "autopilot.h"
#include "p.econ_cmds.h"
#include "p.mech.restrict.h"
#include "p.mech.utils.h"
#include "p.map.conditions.h"

/* Code for saving / loading / setting / unsetting the dynamic pieces
   of map structure:
   - mechsOnMap
   - LOSinfo
   - mechflags
   */


/*
 * Dynamic map save/restore.
 */

static const int DYNAMIC_MAGIC = 0x67134269;

static int
do_read(FILE *f, void *buf, size_t size, size_t count, int map_id)
{
	if (fread(buf, size, count, f) != count) {
		fprintf(stderr, "load_mapdynamic(): while reading #%d: %s\n",
		        map_id, strerror(errno));
		return 0;
	}

	return 1;
}

static int
do_write(FILE *f, const void *buf, size_t size, size_t count, int map_id)
{
	if (fwrite(buf, size, count, f) != count) {
		fprintf(stderr, "save_mapdynamic(): while writing #%d: %s\n",
		        map_id, strerror(errno));
		return 0;
	}

	return 1;
}

void
load_mapdynamic(FILE *f, MAP *map)
{
	const int id = map->mynum;
	const int count = map->first_free;

	int i, magic;

	if (count > 0) {
		Create(map->mechsOnMap, dbref, count);

		if (!do_read(f, map->mechsOnMap, sizeof(map->mechsOnMap[0]),
		             count, id)) {
			/* TODO: Could handle this more gracefully... */
			exit(EXIT_FAILURE);
		}

		Create(map->mechflags, char, count);

		if (!do_read(f, map->mechflags, sizeof(map->mechflags[0]),
		             count, id)) {
			/* TODO: Could handle this more gracefully... */
			exit(EXIT_FAILURE);
		}

		/* Read count X count LOSinfo array.  */
		Create(map->LOSinfo, unsigned short *, count);

		for (i = 0; i < count; i++) {
			Create(map->LOSinfo[i], unsigned short, count);

			if (!do_read(f, map->LOSinfo[i],
			             sizeof(map->LOSinfo[i][0]), count, id)) {
				/* TODO: Could handle this more gracefully... */
				exit(EXIT_FAILURE);
			}
		}
	} else {
		map->mechsOnMap = NULL;
		map->mechflags = NULL;
		map->LOSinfo = NULL;
	}

	/* Check magic.  */
	if (!do_read(f, &magic, sizeof(magic), 1, id)) {
		/* TODO: Could handle this more gracefully... */
		exit(EXIT_FAILURE);
	}

	if (magic != DYNAMIC_MAGIC) {
		fprintf(stderr, "load_mapdynamic(): while reading #%d: Magic number mismatch (0x%08X != 0x%08X)\n",
		        id, magic, DYNAMIC_MAGIC);
		exit(EXIT_FAILURE);
	}
}

void
save_mapdynamic(FILE *f, MAP *map)
{
	const int id = map->mynum;
	const int count = map->first_free;

	int i;

	if (count > 0) {
		if (!do_write(f, map->mechsOnMap, sizeof(map->mechsOnMap[0]),
		              count, id)) {
			/* TODO: Could handle this more gracefully... */
			exit(EXIT_FAILURE);
		}

		if (!do_write(f, map->mechflags, sizeof(map->mechflags[0]),
		              count, id)) {
			/* TODO: Could handle this more gracefully... */
			exit(EXIT_FAILURE);
		}

		/* Write count X count LOSinfo array.  */
		for (i = 0; i < count; i++) {
			if (!do_write(f, map->LOSinfo[i],
			              sizeof(map->LOSinfo[i][0]), count, id)) {
				/* TODO: Could handle this more gracefully... */
				exit(EXIT_FAILURE);
			}
		}
	}

	/* Write magic.  */
	if (!do_write(f, &DYNAMIC_MAGIC, sizeof(DYNAMIC_MAGIC), 1, id)) {
		/* TODO: Could handle this more gracefully... */
		exit(EXIT_FAILURE);
	}
}


void mech_map_consistency_check(MECH * mech)
{
	MAP *map = getMap(mech->mapindex);

	if(!map) {
		if(mech->mapindex > 0) {
			mech->mapindex = -1;
			fprintf(stderr, "#%ld on nonexistent map - removing..\n",
					mech->mynum);
		}
		return;
	}
	if(map->first_free <= mech->mapnumber) {
		/* Invalid: possible corruption of data, therefore un-hosing it */
		mech->mapindex = -1;
		mech_remove_from_all_maps(mech);
		fprintf(stderr, "#%ld on invalid map - removing.. (#1)\n",
				mech->mynum);
		return;
	}
	if(map->mechsOnMap[mech->mapnumber] != mech->mynum) {
		fprintf(stderr, "#%ld on invalid map - removing .. (#2) -- mapindex: %ld mapnumber: %d mechsOnMap: %ld\n",
				mech->mynum, mech->mapindex, mech->mapnumber, map->mechsOnMap[mech->mapnumber]);
		mech->mapindex = -1;
		mech_remove_from_all_maps(mech);
		return;
	}
	mech_remove_from_all_maps_except(mech, map->mynum);
}

void eliminate_empties(MAP * map)
{
	int i;
	int j;
	int count, oldcount;
	char tempbuf[SBUF_SIZE];

	if(!map)
		return;
	for(i = map->first_free - 1; i >= 0; i--)
		if(map->mechsOnMap[i] > 0)
			break;
	count = i + 1;
	if(count == (oldcount = map->first_free))
		return;
	fprintf(stderr,
			"Map #%ld contains empty entries ; removing %d (%d->%d)\n",
			map->mynum, oldcount - count, oldcount, count);
	if(i < 0)
		return;
	for(j = count; j < oldcount; j++)
		free((void *) map->LOSinfo[j]);
	ReCreate(map->LOSinfo, unsigned short *, count);

	ReCreate(map->mechsOnMap, dbref, count);
	ReCreate(map->mechflags, char, count);

	map->first_free = count;
	sprintf(tempbuf, "%ld", map->mynum);
	mech_Rfixstuff(GOD, NULL, tempbuf);
}

void remove_mech_from_map(MAP * map, MECH * mech)
{
	int loop = map->first_free;

	clear_mech_from_LOS(mech);
	mech->mapindex = -1;
	if(map->first_free <= mech->mapnumber ||
	   map->mechsOnMap[mech->mapnumber] != mech->mynum) {
		SendError(tprintf
				  ("Map indexing error for mech #%d: Map index %d contains data for #%d instead.",
				   mech->mynum, mech->mapnumber,
				   map->mechsOnMap ? map->mechsOnMap[mech->mapnumber] : -1));
		if(map->mechsOnMap)
			for(loop = 0;
				(loop < map->first_free) &&
				(map->mechsOnMap[loop] != mech->mynum); loop++);
	} else
		loop = mech->mapnumber;
	mech->mapnumber = 0;
	if(loop != (map->first_free)) {
		map->mechsOnMap[loop] = -1;	/* clear it */
		map->mechflags[loop] = 0;
#if 0
		for(i = 0; i < map->first_free; i++)
			if(map->mechsOnMap[i] > 0 && i != loop)
				if((t = getMech(map->mechsOnMap[i])))
					if(MechTeam(t) != MechTeam(mech) &&
					   (map->LOSinfo[i][loop] & MECHLOSFLAG_SEEN)) {
						MechNumSeen(t) = MAX(0, MechNumSeen(t) - 1);
					}
#endif
		if(loop == (map->first_free - 1))
			map->first_free--;	/* Who cares about some lost memory? In realloc
								   we'll gain it back anyway */
	}
	if(Towed(mech)) {
		/* Check that the towing guy isn't left on the map */
		int i;
		MECH *t;

		for(i = 0; i < map->first_free; i++)
			/* Release from towing if tow-guy ain't on same map already */
			if((t = FindObjectsData(map->mechsOnMap[i])))
				if(MechCarrying(t) == mech->mynum) {
					SetCarrying(t, -1);
					MechStatus(mech) &= ~TOWED;	/* Reset the Towed flag */
					break;
				}
	}
	MechNumSeen(mech) = 0;
	if(IsDS(mech))
		SendDSInfo(tprintf("DS #%d has left map #%d", mech->mynum,
						   map->mynum));

}

void add_mech_to_map(MAP * newmap, MECH * mech)
{
	int loop, count, i;

	for(loop = 0; loop < newmap->first_free; loop++)
		if(newmap->mechsOnMap[loop] == mech->mynum)
			break;
	if(loop != newmap->first_free)
		return;
	for(loop = 0; loop < newmap->first_free; loop++)
		if(newmap->mechsOnMap[loop] < 0)
			break;
	if(loop == newmap->first_free) {
		newmap->first_free++;
		count = newmap->first_free;
		ReCreate(newmap->mechsOnMap, dbref, count);
		ReCreate(newmap->mechflags, char, count);
		ReCreate(newmap->LOSinfo, unsigned short *, count);

		newmap->LOSinfo[count - 1] = NULL;
		for(i = 0; i < count; i++) {
			ReCreate(newmap->LOSinfo[i], unsigned short, count);

			newmap->LOSinfo[i][loop] = 0;
		}
		for(i = 0; i < count; i++)
			newmap->LOSinfo[loop][i] = 0;
	}
	mech->mapindex = newmap->mynum;
	mech->mapnumber = loop;
	newmap->mechsOnMap[loop] = mech->mynum;
	newmap->mechflags[loop] = 0;

	/* Is there an autopilot */
	if(MechAuto(mech) > 0) {

		AUTO *a = FindObjectsData(MechAuto(mech));

		/* Reset the AI's comtitle */
		if(a)
			auto_set_comtitle(a, mech);
	}

	if(Towed(mech)) {
		int i;
		MECH *t;

		for(i = 0; i < newmap->first_free; i++)
			/* Release from towing if tow-guy ain't on same map already */
			if((t = FindObjectsData(newmap->mechsOnMap[i])))
				if(MechCarrying(t) == mech->mynum)
					break;
		if(i == newmap->first_free)
			MechStatus(mech) &= ~TOWED;	/* Reset the Towed flag */
	}
	MarkForLOSUpdate(mech);
	UnZombifyMech(mech);
	UpdateConditions(mech, newmap);
	if(IsDS(mech))
		SendDSInfo(tprintf("DS #%d has entered map #%d", mech->mynum,
						   newmap->mynum));
}

int mech_size(MAP * map)
{
	return map->first_free * (sizeof(dbref) + sizeof(char) +
							  sizeof(unsigned short *) +
							  map->first_free * sizeof(unsigned short));
}
