/*
 * Debug.c
 *
 *  File for debug of the hardcode routines.
 *
 * Serious knifing / new functions by Markus Stenberg <fingon@iki.fi>
 */

#include "externs.h"
#include "debug.h"
#include "muxevent.h"
#include "mech.h"
#include "create.h"
#include "p.map.obj.h"
#include "p.mech.startup.h"
#include "p.mech.partnames.h"

void debug_list(dbref player, void *data, char *buffer)
{
    char *args[3];
    int argc;

    argc = mech_parseattributes(buffer, args, 3);
    if(argc == 0)
        return;
    else if(args[0][0] == 'M' || args[0][0] == 'm')
        if(args[0][1] == 'E' || args[0][1] == 'e')
            DumpMechs(player);
    if(args[0][1] == 'A' || args[0][1] == 'a')
        DumpMaps(player);
}

void debug_savedb(dbref player, void *data, char *buffer)
{
    notify(player, "--- Saving ---");
    SaveSpecialObjects(DUMP_NORMAL);
    notify(player, "---  Done  ---");
}

void debug_loaddb(dbref player, void *data, char *buffer)
{
    notify(player, "--- Loading ---");
    /*
       LoadSpecialObjects();
       */
    notify(player, "---  Done   ---");
}

static int *number;
static int *smallest;
static int *largest;
static int *total;
static dbref cheat_player;

/* For use for outputing memory info */
struct debug_memory_struct {
    dbref player;
    int number[NUM_SPECIAL_OBJECTS];
    int smallest[NUM_SPECIAL_OBJECTS];
    int largest[NUM_SPECIAL_OBJECTS];
    int total[NUM_SPECIAL_OBJECTS];
};

extern rbtree xcode_rbtree;
extern SpecialObjectStruct SpecialObjects[];

static int debug_check_stuff(void *key, void *data, int depth, void *arg)
{
    XcodeObject *xcode_object = (XcodeObject *) data;
    struct debug_memory_struct *info = (struct debug_memory_struct *) arg;
    int osize, size, type;
    MAP *map;

    type = xcode_object->type;
    osize = size = SpecialObjects[type].datasize;
    switch (type) {
        case GTYPE_MAP:
            map = (MAP *) xcode_object->data;
            if(map->hexes) {
                size += sizeof(hex) * map->width * map->height;
                size += bit_size(map);
                size += obj_size(map);
                size += mech_size(map);
            }
            break;
    }
    if(info->smallest[type] < 0 || size < info->smallest[type])
        info->smallest[type] = size;
    if(info->largest[type] < 0 || size > info->largest[type])
        info->largest[type] = size;
    info->total[type] += size;
    info->number[type]++;
    if(info->player > 0 && osize != size)
        notify_printf(info->player, "#%d: %s (%d bytes)", xcode_object->key,
                SpecialObjects[type].type, size);
    return 1;
}

void debug_memory(dbref player, void *data, char *buffer)
{
    int i, gtotal = 0;

    struct debug_memory_struct info;

    for (i = 0; i < NUM_SPECIAL_OBJECTS; i++) {
        info.number[i] = 0;
        info.smallest[i] = -1;
        info.largest[i] = -1;
        info.total[i] = 0;
    }
    info.player = player;
    skipws(buffer);

    if (strcmp(buffer, ""))
        info.player = player;
    else
        info.player = -1;

    rb_walk(xcode_rbtree, WALK_INORDER, debug_check_stuff, &info);

    for (i = 0; i < NUM_SPECIAL_OBJECTS; i++) {
        if (info.number[i]) {
            if (info.smallest[i] == info.largest[i])
                notify_printf(player,
                        "%4d %-20s: %d bytes total, %d each",
                        info.number[i], SpecialObjects[i].type, info.total[i],
                        info.total[i] / info.number[i]);
            else
                notify_printf(player,
                        "%4d %-20s: %d bytes total, %d avg, %d/%d small/large",
                        info.number[i], SpecialObjects[i].type, info.total[i],
                        info.total[i] / info.number[i], info.smallest[i], info.largest[i]);
        }
        gtotal += info.total[i];
    }
    notify_printf(player, "Grand total: %d bytes.", gtotal);
}

void ShutDownMap(dbref player, dbref mapnumber)
{
    MAP *map;
    MECH *mech;
    int j;
    XcodeObject *xcode_object;

    xcode_object = (XcodeObject *) rb_find(xcode_rbtree, (void *) mapnumber);
    if (xcode_object) {
        
        map = (MAP *) xcode_object->data;

        for (j = 0; j < map->first_free; j++)
            if (map->mechsOnMap[j] != -1) {
                mech = getMech(map->mechsOnMap[j]);
                if (mech) {
                    notify_printf(player,
                            "Shutting down Mech #%d and restting map index to -1....",
                            map->mechsOnMap[j]);
                    mech_shutdown(GOD, (void *) mech, "");
                    MechLastX(mech) = 0;
                    MechLastY(mech) = 0;
                    MechX(mech) = 0;
                    MechY(mech) = 0;
                    remove_mech_from_map(map, mech);
                }
            }

        map->first_free = 0;

        notify(player, "Map Cleared");

        return;
    }
}

void debug_shutdown(dbref player, void *data, char *buffer)
{
    char *args[3];
    int argc;

    argc = mech_parseattributes(buffer, args, 3);
    if (argc > 0)
        ShutDownMap(player, atoi(args[0]));
}

void debug_setvrt(dbref player, void *data, char *buffer)
{
    char *args[3];
    int vrt;
    int id, brand;

    DOCHECK(mech_parseattributes(buffer, args, 3) != 2, "Invalid arguments!");
    DOCHECK(Readnum(vrt, args[1]), "Invalid value!");
    DOCHECK(vrt <= 0, "VRT needs to be >0");
    DOCHECK(vrt > 127, "VRT can be at max 127");
    DOCHECK(!find_matching_vlong_part(args[0], NULL, &id, &brand),
            "That is no weapon!");
    DOCHECK(!IsWeapon(id), "That is no weapon!");
    MechWeapons[Weapon2I(id)].vrt = vrt;
    notify_printf(player, "VRT for %s set to %d.",
            MechWeapons[Weapon2I(id)].name, vrt);
    log_error(LOG_WIZARD, "WIZ", "CHANGE", "VRT for %s set to %d by #%d", 
            MechWeapons[Weapon2I(id)].name, vrt, player);
}
