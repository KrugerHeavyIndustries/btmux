/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "mech.events.h"
#include "create.h"
#include "autopilot.h"
#include "p.mech.build.h"
#include "p.mech.utils.h"
#include "p.mechrep.h"
#include "p.mech.c3.h"
#include "p.mech.c3i.h"

/* Selectors for new/free function */
#define SPECIAL_FREE 0
#define SPECIAL_ALLOC 1

void clear_mech_from_LOS(MECH * mech)
{
    MAP *map;
    int i;
    MECH *mek;
    dllist_node *node;

    /* if (mech->mapindex < 0) 
       return;
       */

    if (!(map = getMap(mech->mapindex)))
        return;

#ifdef SENSOR_DEBUG
    SendSensor("LOS info for #%d cleared.", mech->mynum);
#endif

    for (i = 0; i < map->first_free; i++) {
        map->LOSinfo[mech->mapnumber][i] = 0;
        map->LOSinfo[i][mech->mapnumber] = 0;

        if (map->mechsOnMap[i] >= 0 && i != mech->mapnumber) {
            if (!(mek = getMech(map->mechsOnMap[i])))
                continue;
            if ((MechStatus(mek) & LOCK_TARGET) &&
                    MechTarget(mek) == mech->mynum) {
                mech_notify(mek, MECHALL,
                        "Weapon system reports the lock has been lost.");
                LoseLock(mek);
            }
            if ((map->LOSinfo[i][mech->mapnumber] & MECHLOSFLAG_SEEN) &&
                    MechTeam(mek) != MechTeam(mech))
                MechNumSeen(mek) = MAX(0, MechNumSeen(mek) - 1);
        }
    }

    /* Clear the LOS rbtree of the removed unit and remove the unit from
     * the LOS tree of the units still on the map */
    rb_release(mech->UnitsInLOS, (void *) mech_rbtree_release, NULL);
    mech->UnitsInLOS = rb_init((void *) mech_rbtree_compare, NULL);

    for (node = dllist_head(map->mechs); node; node = dllist_next(node)) {
        mek = getMech((int) dllist_data(node));

        if (!mek)
            continue;

        if ((MechStatus(mek) & LOCK_TARGET) &&
                MechTarget(mek) == mech->mynum) {
            mech_notify(mek, MECHALL,
                    "Weapon system reports the lock has been lost.");
            LoseLock(mek);
        }

        /* Remove the unit from the los rbtree */
        rb_delete(mek->UnitsInLOS, (void *) mech->mynum);
    }

    if (MechStatus(mech) & LOCK_MODES) {
        mech_notify(mech, MECHALL,
                "Weapon system reports the lock has been lost.");
        LoseLock(mech);
    }
}

void mech_Rsetxy(dbref player, void *data, char *buffer) {

    MECH *mech = (MECH *) data;
    MAP *map = getMap(mech->mapindex);
    char *args[3];
    int x, y, z, argc;

    if (!common_checks(player, mech, MECH_MAP)) {
        return;
    }

    argc = proper_parseattributes(buffer, args, 3);

    if ((argc != 2) && (argc != 3)) {
        notify(player, "Invalid number of arguments to SETXY!");
        proper_freearguments(args, argc);
        return;
    }

    x = atoi(args[0]);
    y = atoi(args[1]);

    if ((x >= map->width) || (y >= map->height) || (x < 0) || (y < 0)) {
        notify(player, "Invalid coordinates!");
        proper_freearguments(args, argc);
        return;
    }

    MechX(mech) = x;
    MechLastX(mech) = x;
    MechY(mech) = y;
    MechLastY(mech) = y;

    MapCoordToRealCoord(MechX(mech), MechY(mech), &MechFX(mech),
            &MechFY(mech));
    MapCoordToActualCoord(MechX(mech), MechY(mech), &MechRealX(mech),
            &MechRealY(mech));

    MechTerrain(mech) = GetTerrain(map, MechX(mech), MechY(mech));

    MarkForLOSUpdate(mech);

    if (argc == 2) {
        MechElev(mech) = GetElevation(map, MechX(mech), MechY(mech));
        MechRealZ(mech) = MechElev(mech) * HEX_Z_SCALE;

        MechZ(mech) = MechElev(mech) - 1;
        MechFZ(mech) = ZSCALE * MechZ(mech);

        DropSetElevation(mech, 0);
        z = MechZ(mech);
        if (!Landed(mech) && FlyingT(mech)) {
            MechStatus(mech) |= LANDED;
        }

    } else {
        z = atoi(args[2]);
        MechZ(mech) = z;
        MechFZ(mech) = ZSCALE * MechZ(mech);
        MechRealZ(mech) = MechZ(mech) * HEX_Z_SCALE;
        MechElev(mech) = GetElevation(map, MechX(mech), MechY(mech));
    }
    clear_mech_from_LOS(mech);
    notify_printf(player, "Pos changed to %d,%d,%d", x, y, z);
    proper_freearguments(args, argc);
}

/* Team/Map commands */
void mech_Rsetmapindex(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    char *args[2], *tempstr;
    int newindex, nargs, notdone = 0;
    int loop;
    MAP *newmap = NULL;
    MAP *oldmap;
    MECH *tempMech;
    dllist_node *node;
    char targ[2];

    if (!CheckData(player, mech))
        return;
    nargs = mech_parseattributes(buffer, args, 2);
    DOCHECK(nargs < 1, "Invalid number of arguments to SETMAPINDX!");
    newindex = atoi(args[0]);
    DOCHECK(newindex < -1, "Invalid map index!");
    if (newindex != -1) {
        if (!(newmap = ValidMap(player, newindex)))
            return;
    }

    /* Remove the mech from it's old map */
    if (mech->mapindex != -1) {
        if (!(oldmap = ValidMap(player, mech->mapindex)))
            return;
        TAGTarget(mech) = -1;
        clearC3iNetwork(mech, 1);
        clearC3Network(mech, 1);
        remove_mech_from_map(oldmap, mech);
    }

    if (newindex == -1) {
        notify(player, "Mech removed from map.");
        SendLoc(tprintf("#%d removed #%d from map #%d.", player,
                    mech->mynum, oldmap->mynum));
        return;
    }

    /* Just make it random */
    /* Find a clear spot for this mech */
    if (nargs > 1 && strlen(args[1]) > 1) {
        targ[0] = args[1][0];
        targ[1] = args[1][1];
    } else if ((tempstr = silly_atr_get(mech->mynum, A_MECHPREFID))
            && strlen(tempstr) > 1) {
        targ[0] = tempstr[0];
        targ[1] = tempstr[1];
    } else {
        targ[0] = 65 + Number(0, 25);
        targ[1] = 65 + Number(0, 25);
    }
    targ[0] = BOUNDED('A', toupper(targ[0]), 'Z');
    targ[1] = BOUNDED('A', toupper(targ[1]), 'Z');
    for (node = dllist_head(newmap->mechs); node && !notdone; node = dllist_next(node)) {
        if ((tempMech = getMech((int) dllist_data(node))))
            if (MechID(tempMech)[0] == targ[0] &&
                    MechID(tempMech)[1] == targ[1])
                notdone = 1;
    }
    while (notdone) {
        targ[0] = 65 + Number(0, 25);
        targ[1] = 65 + Number(0, 25);
        notdone = 0;
        for (node = dllist_head(newmap->mechs); node && !notdone; node = dllist_next(node)) {
            if ((tempMech = getMech((int) dllist_data(node))))
                if (MechID(tempMech)[0] == targ[0] &&
                        MechID(tempMech)[1] == targ[1])
                    notdone = 1;
        }
    }
    DOCHECK(dllist_size(newmap->mechs) == MAX_MECHS_PER_MAP,
            "There are too many mechs on that map!");
    add_mech_to_map(newmap, mech);
    MechID(mech)[0] = targ[0];
    MechID(mech)[1] = targ[1];
    if (MechX(mech) > (newmap->width - 1) ||
            MechY(mech) > (newmap->height - 1)) {
        MechX(mech) = 0;
        MechLastX(mech) = 0;
        MechY(mech) = 0;
        MechLastY(mech) = 0;
        MapCoordToRealCoord(MechX(mech), MechY(mech), &MechFX(mech),
                &MechFY(mech));
        MechTerrain(mech) = GetTerrain(newmap, MechX(mech), MechY(mech));
        MechElev(mech) = GetElevation(newmap, MechX(mech), MechY(mech));
        notify(player,
                "You're current position is out of bounds, Pos changed to 0,0");
    }
    notify_printf(player, "MapIndex changed to %d", newindex);
    notify_printf(player, "Your ID: %c%c", MechID(mech)[0], MechID(mech)[1]);
    SendLoc("#%d set #%d's mapindex to #%d.", player, mech->mynum,
            newindex);
    UnZombifyMech(mech);
}

void mech_Rsetteam(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	char *args[1];
	int team;
	MAP *newmap;

	if(!CheckData(player, mech))
		return;
	DOCHECK(mech->mapindex == -1, "Mech is not on a map:  Can't set team");
	newmap = ValidMap(player, mech->mapindex);
	if(!newmap) {
		notify(player, "Map index reset!");
		mech->mapindex = -1;
		return;
	}
	DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
			"Invalid number of arguments!");
	team = atoi(args[0]);
	if(team < 0)
		team = 0;
	MechTeam(mech) = team;
	notify_printf(player, "Team set to %d", team);
}

#define SPECIAL_FREE 0
#define SPECIAL_ALLOC 1

extern void auto_stop_pilot(AUTO * autopilot);
/* Alloc/free routine */
void newfreemech(dbref key, void **data, int selector)
{
	MECH *new = *data;
	MAP *map;
	AUTO *a;
	int i;
        command_node *temp;
		


    switch (selector) {
        case SPECIAL_ALLOC:
            new->mynum = key;
            new->mapnumber = 1;
            new->mapindex = -1;
            MechID(new)[0] = ' ';
            MechID(new)[1] = ' ';
            clear_mech(new, 1);
            for(i = 0; i < NUM_SECTIONS; i++)
                FillDefaultCriticals(new, i);
            break;

        case SPECIAL_FREE:
            if(new->mapindex != -1 && (map = getMap(new->mapindex)))
                remove_mech_from_map(map, new);

            /* Clean up rbtree(s) */
            rb_release(new->UnitsInLOS, (void *) mech_rbtree_release, NULL);
            if(MechAuto(new) > 0 ) {
			AUTO *a = (AUTO *) FindObjectsData(MechAuto(new));
			if (a) {
				auto_stop_pilot(a);
				 /* Go through the list and remove any leftover nodes */
                while (dllist_size(a->commands)) {

                        /* Remove the first node on the list and get the data
                         * from it */
                        temp = (command_node *) dllist_remove(a->commands,
                                                                                                  dllist_head(a->
                                                                                                                          commands));

                        /* Destroy the command node */
                        auto_destroy_command_node(temp);

                }

                /* Destroy the list */
                dllist_destroy_list(a->commands);
                a->commands = NULL;

                /* Destroy any astar path list thats on the AI */
                auto_destroy_astar_path(a);

                /* Destroy profile array */
                for(i = 0; i < AUTO_PROFILE_MAX_SIZE; i++) {
                        if(a->profile[i]) {
                                rb_destroy(a->profile[i]);
                        }
                        a->profile[i] = NULL;
                }

                /* Destroy weaponlist */
                auto_destroy_weaplist(a);

				a->mymechnum = -1;
			}
			MechAuto(new) = -1;
		}
					
		
	}

}
