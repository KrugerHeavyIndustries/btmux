
/*
 * $Id: glue.c,v 1.4 2005/08/08 09:43:09 murrayma Exp $
 *
 * Original author: unknown
 *
 * Copyright (c) 1996-2002 Markus Stenberg
 * Copyright (c) 1998-2002 Thomas Wouters 
 * Copyright (c) 2000-2002 Cord Awtry 
 *
 * Last modified: Thu Jul  9 02:40:16 1998 fingon
 *
 * This includes the basic code to allow objects to have hardcoded
 * commands / properties.
 *
 */

#include "config.h"

#include <stdio.h>
#include <sys/file.h>
#include <string.h>
#include <math.h>

#define FAST_WHICHSPECIAL

#include "create.h"

#define _GLUE_C

/*** #include all the prototype here! ****/
#include "mech.h"
#include "mech.events.h"
#include "debug.h"
#include "mechrep.h"
#include "mech.tech.h"
#include "autopilot.h"
#include "turret.h"
#include "p.ds.turret.h"
#include "coolmenu.h"
#include "mech.custom.h"
#include "p.bsuit.h"
#include "scen.h"
#include "glue.h"
#include "mux_tree.h"
#include "powers.h"
#include "ansi.h"
#include "coolmenu.h"
#include "mycool.h"
#include "p.mux_tree.h"
#include "p.mechfile.h"
#include "p.mech.stat.h"
#include "p.mech.partnames.h"

/* Prototypes */

/*************CALLABLE PROTOS*****************/

/* Main entry point */
int HandledCommand(dbref player, dbref loc, char *command);

/* called when user creates/removes hardcode flag */
void CreateNewSpecialObject(dbref player, dbref key);
void DisposeSpecialObject(dbref player, dbref key);
void list_hashstat(dbref player, const char *tab_name, HASHTAB * htab);
void raw_notify(dbref player, const char *msg);
void AddEntry(Tree * tree, muxkey_t key, dtype_t type, dsize_t size,
    void *data);
void DeleteEntry(Tree * tree, muxkey_t key);
int SaveTree(FILE * f, Tree tree);
void UpdateTree(FILE * f, Tree * tree, int (*sizefunc)(int));
void GoThruTree(Tree tree, int (*func) (Node *));


/*************PERSONAL PROTOS*****************/
void *NewSpecialObject(int id, int type);
void *FindObjectsData(dbref key);
static Node *FindObjectsNode(dbref key);
static void DoSpecialObjectHelp(dbref player, char *type, int id, int loc,
    int powerneeded, int objid, char *arg);
void initialize_colorize();

#ifndef FAST_WHICHSPECIAL

#define WhichSpecialS WhichSpecial
#define WhichType(node) WhichSpecial(NodeKey(node))
int WhichSpecial(dbref key);

#else

#define WhichType(node) NodeType(node)
int WhichSpecial(dbref key);
static int WhichSpecialS(dbref key);

#endif

/*********************************************/

HASHTAB SpecialCommandHash[NUM_SPECIAL_OBJECTS];
Tree xcode_tree = NULL;
extern int map_sizefun();


static int Can_Use_Command(MECH * mech, int cmdflag)
{
#define TYPE2FLAG(a) \
((a)==CLASS_MECH?GFLAG_MECH:(a)==CLASS_VEH_GROUND?GFLAG_GROUNDVEH:\
 (a)==CLASS_AERO?GFLAG_AERO:DropShip(a)?GFLAG_DS:(a)==CLASS_VTOL?GFLAG_VTOL:\
 (a)==CLASS_VEH_NAVAL?GFLAG_NAVAL:\
 (a)==CLASS_BSUIT?GFLAG_BSUIT:\
 (a)==CLASS_MW?GFLAG_MW:0)
    int i;

    if (!cmdflag)
	return 1;
    if (!mech || !(i = TYPE2FLAG(MechType(mech))))
	return 0;
    if (cmdflag > 0) {
	if (cmdflag & i)
	    return 1;
    } else if (!((0 - cmdflag) & i))
	return 1;
    return 0;
}

int HandledCommand_sub(dbref player, dbref location, char *command)
{
    struct SpecialObjectStruct *typeOfObject;
    Node *n = NULL;
    int type;
    CommandsStruct *cmd;
    HASHTAB *damnedhash;
    char *tmpc, *tmpchar;
    int ishelp;

    type = WhichSpecial(location);
    if (type < 0 || (SpecialObjects[type].datasize > 0 &&
	    !(n = FindNode(xcode_tree, location)))) {
	if (type >= 0 || !Hardcode(location) || Zombie(location))
	    return 0;
	if ((type = WhichSpecialS(location)) >= 0) {
	    if (SpecialObjects[type].datasize > 0)
		return 0;
	} else
	    return 0;
    }
#ifdef FAST_WHICHSPECIAL
    if (type > NUM_SPECIAL_OBJECTS)
	return 0;
#endif
    typeOfObject = &SpecialObjects[type];
    damnedhash = &SpecialCommandHash[type];
    tmpc = strstr(command, " ");
    if (tmpc)
	*tmpc = 0;
    ishelp = !strcmp(command, "HELP");
    for (tmpchar = command; *tmpchar; tmpchar++)
	*tmpchar = ToLower(*tmpchar);
    cmd = (CommandsStruct *) hashfind(command, &SpecialCommandHash[type]);
    if (tmpc)
	*tmpc = ' ';
    if (cmd && (type != GTYPE_MECH || (type == GTYPE_MECH &&
		Can_Use_Command(((MECH *)
			(NodeData(n))), cmd->flag)))) {
#define SKIPSTUFF(a) while (*a && *a != ' ') a++;while (*a == ' ') a++
	if (cmd->helpmsg[0] != '@' ||
	    Have_MechPower(Owner(player), typeOfObject->power_needed)) {
	    SKIPSTUFF(command);
	    cmd->func(player, !n ? NULL : NodeData(n), command);
	} else
	    notify(player, "Sorry, that command is restricted!");
	return 1;
    } else if (ishelp) {
	SKIPSTUFF(command);
	DoSpecialObjectHelp(player, typeOfObject->type, type, location,
	    typeOfObject->power_needed, location, command);
	return 1;
    }
    return 0;
}

#define OkayHcode(a) (a >= 0 && Hardcode(a) && !Zombie(a))

/* Main entry point */
int HandledCommand(dbref player, dbref loc, char *command)
{
    dbref curr, temp;

    if (Slave(player))
	return 0;
    if (strlen(command) > (LBUF_SIZE - MBUF_SIZE))
	return 0;
    if (OkayHcode(player) && HandledCommand_sub(player, player, command))
	return 1;
    if (OkayHcode(loc) && HandledCommand_sub(player, loc, command))
	return 1;
    SAFE_DOLIST(curr, temp, Contents(player)) {
	if (OkayHcode(curr))
	    if (HandledCommand_sub(player, curr, command))
		return 1;
#if 0				/* Recursion is evil ; let's not do that, this time */
	if (Has_contents(curr))
	    if (HandledCommand_contents(player, curr, command))
		return 1;
#endif
    }
    return 0;
}

void InitSpecialHash(int which);
void initialize_partname_tables();

static MECH *global_kludge_mech;
int global_specials = NUM_SPECIAL_OBJECTS;

static int remove_from_all_maps_func(Node * tmp)
{
    if (WhichType(tmp) == GTYPE_MAP) {
	MAP *map;
	int i;

	if (!(map = getMap(NodeKey(tmp))))
	    return 1;
	for (i = 0; i < map->first_free; i++)
	    if (map->mechsOnMap[i] == global_kludge_mech->mynum)
		map->mechsOnMap[i] = -1;
    }
    return 1;
}

void mech_remove_from_all_maps(MECH * mech)
{
    global_kludge_mech = mech;
    GoThruTree(xcode_tree, remove_from_all_maps_func);
}

static dbref except_map = -1;

static int remove_from_all_maps_except_func(Node * tmp)
{
    if (WhichType(tmp) == GTYPE_MAP) {
	int i;
	MAP *map;

	if (NodeKey(tmp) == except_map)
	    return 1;
	if (!(map = getMap(NodeKey(tmp))))
	    return 1;
	for (i = 0; i < map->first_free; i++)
	    if (map->mechsOnMap[i] == global_kludge_mech->mynum)
		map->mechsOnMap[i] = -1;
    }
    return 1;
}
void mech_remove_from_all_maps_except(MECH * mech, int num)
{
    global_kludge_mech = mech;
    except_map = num;
    GoThruTree(xcode_tree, remove_from_all_maps_except_func);
    except_map = -1;
}

static int load_update2(Node * tmp)
{
    int i = WhichType(tmp);;

    if (i == GTYPE_MECH)
	mech_map_consistency_check(NodeData(tmp));
    return 1;
}

static int load_update4(Node * tmp)
{
    MECH *mech;
    MAP *map;

    if (WhichType(tmp) == GTYPE_MECH) {
	mech = NodeData(tmp);
	if (!(map = getMap(mech->mapindex))) {
	    /* Ugly kludge */
	    if ((map = getMap(Location(mech->mynum))))
		mech_Rsetmapindex(GOD, mech, tprintf("%d",
			Location(mech->mynum)));
	    if (!(map = getMap(mech->mapindex)))
		return 1;
	}
	if (!Started(mech))
	    return 1;
	StartSeeing(mech);
	MaybeRecycle(mech, 1);
	MaybeMove(mech);
	if (!FlyingT(mech) && Started(mech) && Jumping(mech))
	    mech_Rsetxy(GOD, (void *) mech, tprintf("%d %d", MechX(mech),
		    MechY(mech)));
    }
    return 1;
}

static int load_update3(Node * tmp)
{
    int i = WhichType(tmp);

    if (i == GTYPE_MAP) {
	eliminate_empties((MAP *) NodeData(tmp));
	recalculate_minefields((MAP *) NodeData(tmp));
    }
    return 1;
}

static FILE *global_file_kludge;

static int load_update1(Node * tmp)
{
    MAP *map;
    int doh;
    char mapbuffer[MBUF_SIZE];
    MECH *mech;
    int i;

    switch ((i = WhichType(tmp))) {
    case GTYPE_MAP:
	map = (MAP *) NodeData(tmp);
	bzero(map->mapobj, sizeof(map->mapobj));
	map->map = NULL;
	strcpy(mapbuffer, map->mapname);
	doh = (map->flags & MAPFLAG_MAPO);
	if (strcmp(map->mapname, "Default Map"))
	    map_loadmap(1, map, mapbuffer);
	if (!strcmp(map->mapname, "Default Map") || !map->map)
	    initialize_map_empty(map, NodeKey(tmp));
	if (!feof(global_file_kludge)) {
	    load_mapdynamic(global_file_kludge, map);
	    if (!feof(global_file_kludge))
		if (doh)
		    load_mapobjs(global_file_kludge, map);
	}
	if (feof(global_file_kludge)) {
	    map->first_free = 0;
	    map->mechflags = NULL;
	    map->mechsOnMap = NULL;
	    map->LOSinfo = NULL;
	}
	debug_fixmap(GOD, map, NULL);
	break;
    case GTYPE_MECH:
	mech = (MECH *) NodeData(tmp);
	if (!(FlyingT(mech) && !Landed(mech))) {
	    MechDesiredSpeed(mech) = 0;
	    MechSpeed(mech) = 0;
	    MechVerticalSpeed(mech) = 0;
	}
	MechCocoon(mech) = 0;
	MechStatus(mech) &= ~(BLINDED | UNCONSCIOUS | JUMPING | TOWED);
	MechSpecials2(mech) &=
	    ~(ECM_ENABLED | ECM_DISTURBANCE | ECM_PROTECTED |
	    ECCM_ENABLED | ANGEL_ECM_ENABLED | ANGEL_ECCM_ENABLED |
	    ANGEL_ECM_PROTECTED | ANGEL_ECM_DISTURBED);
	MechCritStatus(mech) &=
	    ~(JELLIED | LOAD_OK | OWEIGHT_OK | SPEED_OK);
	MechWalkXPFactor(mech) = 999;
	MechCarrying(mech) = -1;
	MechBoomStart(mech) = 0;
	MechC3iNetworkSize(mech) = -1;
	MechHeatLast(mech) = 0;
	MechCommLast(mech) = 0;
	for (i = 0; i < FREQS; i++)
	    if (mech->freq[i] < 0)
		mech->freq[i] = 0;
	break;
    }
    return 1;
}

/*
 * Read in autopilot data
 */
static int load_autopilot_data(Node *tmp) {

    AUTO *autopilot;
    int i;

    if (WhichType(tmp) == GTYPE_AUTO) {

        autopilot = (AUTO *) NodeData(tmp);

        /* Save the AI Command List */
        auto_load_commands(global_file_kludge, autopilot);

        /* Reset the Astar Path */
        autopilot->astar_path = NULL;

        /* Reset the weaplist */
        autopilot->weaplist = NULL;

        /* Reset the profile */
        for (i = 0; i < AUTO_PROFILE_MAX_SIZE; i++) {
            autopilot->profile[i] = NULL;
        }

        /* Check to see if the AI is in a mech */
        /* Need to make this better, check if its got a target whatnot */
	    if (!autopilot->mymechnum || 
                !(autopilot->mymech = getMech(autopilot->mymechnum))) {
	        DoStopGun(autopilot);
        } else {
	        if (Gunning(autopilot))
	    	    DoStartGun(autopilot);
        }

    }

    return 1;

}

static int get_specialobjectsize(int type)
{
    if (type < 0 || type >= NUM_SPECIAL_OBJECTS)
    	return -1;
    return SpecialObjects[type].datasize;
}

#ifdef BT_ADVANCED_ECON
static void load_econdb()
{
FILE *f;
/* Econ DB */
    extern unsigned long long int specialcost[SPECIALCOST_SIZE];
    extern unsigned long long int ammocost[AMMOCOST_SIZE];
    extern unsigned long long int weapcost[WEAPCOST_SIZE];
    extern unsigned long long int cargocost[CARGOCOST_SIZE];
    extern unsigned long long int bombcost[BOMBCOST_SIZE];
    int count;

    fprintf(stderr, "LOADING: %s\n", mudconf.econ_db);
    f =  fopen(mudconf.econ_db, "r");
    if (!f) {
        fprintf(stderr, "ERROR: %s not found.\n", mudconf.econ_db);
        return;
        }
    count = fread(&specialcost, sizeof(unsigned long long int), SPECIALCOST_SIZE, f);
    if (count < SPECIALCOST_SIZE) {
        fprintf(stderr, "ERROR: %s specialcost read : %d expected %d\n", mudconf.econ_db, count, SPECIALCOST_SIZE);
        fclose(f);
        return;
        }
    count = fread(&ammocost, sizeof(unsigned long long int), AMMOCOST_SIZE, f);
    if (count < AMMOCOST_SIZE) {
        fprintf(stderr, "ERROR: %s ammocost read : %d expected %d\n", mudconf.econ_db, count, AMMOCOST_SIZE);
        fclose(f);
        return;
        }
    count = fread(&weapcost, sizeof(unsigned long long int), WEAPCOST_SIZE, f);
    if (count < WEAPCOST_SIZE) {
        fprintf(stderr, "ERROR: %s weapcost read : %d expected %d\n", mudconf.econ_db, count, WEAPCOST_SIZE);
        fclose(f);
        return;
        }
    count = fread(&cargocost, sizeof(unsigned long long int), CARGOCOST_SIZE, f);
    if (count < CARGOCOST_SIZE) {
        fprintf(stderr, "ERROR: %s cargocost read : %d expected %d\n", mudconf.econ_db, count, CARGOCOST_SIZE);
        fclose(f);
        return;
        }
    count = fread(&bombcost, sizeof(unsigned long long int), BOMBCOST_SIZE, f);
    if (count < BOMBCOST_SIZE) {
        fprintf(stderr, "ERROR: %s bombcost read : %d expected %d\n", mudconf.econ_db, count, BOMBCOST_SIZE);
        fclose(f);
        return;
        }
    fclose(f);
    fprintf(stderr, "LOADING: %s (done\n", mudconf.econ_db);
}
#endif

static void load_xcode()
{
    FILE *f;
    byte xcode_version;
    int filemode;

    RCache_Flush();
    initialize_colorize();
    fprintf(stderr, "LOADING: %s\n", mudconf.hcode_db);
    f = my_open_file(mudconf.hcode_db, "r", &filemode);
    if (!f) {
	fprintf(stderr, "ERROR: %s not found.\n", mudconf.hcode_db);
	return;
    }
    fread(&xcode_version, 1, 1, f);
    if (xcode_version != XCODE_VERSION) {
	fprintf(stderr,
	    "LOADING: %s (skipped xcodetree - version difference: %d vs %d)\n",
	    mudconf.hcode_db, (int) xcode_version, (int) XCODE_VERSION);
	return;
    }
    UpdateTree(f, &xcode_tree, get_specialobjectsize);
    global_file_kludge = f;
    GoThruTree(xcode_tree, load_update1);
    GoThruTree(xcode_tree, load_update2);
    GoThruTree(xcode_tree, load_update3);
    GoThruTree(xcode_tree, load_update4);

    /* Read in autopilot data */
    GoThruTree(xcode_tree, load_autopilot_data);

    if (!feof(f))
	loadrepairs(f);

    my_close_file(f, &filemode);
    fprintf(stderr, "LOADING: %s (done)\n", mudconf.hcode_db);
#ifdef BT_ADVANCED_ECON
    load_econdb();
#endif
}

static int zappable_node;

static int zap_check(Node * n)
{
    if (zappable_node >= 0)
	return 0;
    if (!Hardcode(NodeKey(n))) {
	zappable_node = NodeKey(n);
	return 0;
    }
    return 1;
}

void zap_unneccessary_hcode()
{
    while (1) {
	zappable_node = -1;
	GoThruTree(xcode_tree, zap_check);
	if (zappable_node >= 0)
	    DeleteEntry(&xcode_tree, zappable_node);
	else
	    break;
    }
}

void LoadSpecialObjects(void)
{
    dbref i;
    int id, brand;
    int type;
    void *tmpdat;

    muxevent_initialize();
    muxevent_count_initialize();
    init_stat();
    initialize_partname_tables();
    for (i = 0; MissileHitTable[i].key != -1; i++) {
	if (find_matching_vlong_part(MissileHitTable[i].name, NULL, &id,
		&brand))
	    MissileHitTable[i].key = Weapon2I(id);
	else
	    MissileHitTable[i].key = -2;
    }
    /* Loop through the entire database, and if it has the special */
    /* object flag, add it to our linked list. */
    for (i = 0; i < mudstate.db_top; i++)
	if (Hardcode(i) && !Going(i) && !Halted(i)) {
	    type = WhichSpecialS(i);
	    if (type >= 0) {
		if (SpecialObjects[type].datasize > 0)
		    tmpdat = NewSpecialObject(i, type);
		else
		    tmpdat = NULL;
	    } else
		c_Hardcode(i);	/* Reset the flag */
	}
    for (i = 0; i < NUM_SPECIAL_OBJECTS; i++) {
	InitSpecialHash(i);
	if (!SpecialObjects[i].updatefunc)
	    SpecialObjects[i].updateTime = 0;
    }
    init_btechstats();
    load_xcode();
    zap_unneccessary_hcode();
}

static FILE *global_file_kludge;

static int save_maps_func(Node * tmp)
{
    MAP *map;

    if (WhichType(tmp) == GTYPE_MAP) {
	/* Write mapobjs, if neccessary */
	map = (MAP *) NodeData(tmp);
	save_mapdynamic(global_file_kludge, map);
	if (map->flags & MAPFLAG_MAPO)
	    save_mapobjs(global_file_kludge, map);
    }
    return 1;
}

/* 
 * Save any extra info for the autopilots 
 *
 * Like their command lists
 * or the Astar path if there is one
 *
 */
static int save_autopilot_data(Node *tmp) {

    AUTO *a;

    if (WhichType(tmp) == GTYPE_AUTO) {

        a = (AUTO *) NodeData(tmp);

        /* Save the AI Command List */
        auto_save_commands(global_file_kludge, a);

        /* Save the AI Astar Path */

    }

    return 1;

}

void ChangeSpecialObjects(int i)
{
    /* XXX Unneccessary for now ; 'latest' db
       (db.new) is equivalent to 'db' because we don't
       _have_ new-db concept ; this is to-be-done project, however */
}

#ifdef BT_ADVANCED_ECON
static void save_econdb(char *target, int i)
{
FILE *f;
    extern unsigned long long int specialcost[SPECIALCOST_SIZE];
    extern unsigned long long int ammocost[AMMOCOST_SIZE];
    extern unsigned long long int weapcost[WEAPCOST_SIZE];
    extern unsigned long long int cargocost[CARGOCOST_SIZE];
    extern unsigned long long int bombcost[BOMBCOST_SIZE];
    int count;

    switch (i) {
    case DUMP_KILLED:
        sprintf(target, "%s.KILLED", mudconf.econ_db);
        break;
    case DUMP_CRASHED:
        sprintf(target, "%s.CRASHED", mudconf.econ_db);
        break;
    default:                    /* RESTART / normal */
        sprintf(target, "%s.tmp", mudconf.econ_db);
        break;
    }
    f =  fopen(target, "w");
    if (!f) {
        log_perror("SAVE", "FAIL", "Opening econ-save file", target);
        SendDB("ERROR occured during opening of new econ-savefile.");
        return;
        }
    count = fwrite(&specialcost, sizeof(unsigned long long int), SPECIALCOST_SIZE, f);
    if (count < SPECIALCOST_SIZE) {
        log_perror("SAVE", "FAIL", tprintf("ERROR: %s specialcost wrote : %d expected %d", target, count, SPECIALCOST_SIZE), target);
        SendDB("ERROR occured during saving of econ-save file");
        fclose(f);
        return;
        }
    count = fwrite(&ammocost, sizeof(unsigned long long int), AMMOCOST_SIZE, f);
    if (count < AMMOCOST_SIZE) {
        log_perror("SAVE", "FAIL", tprintf("ERROR: %s ammocost wrote : %d expected %d", target, count, AMMOCOST_SIZE), target);
        SendDB("ERROR occured during saving of econ-save file");
        fclose(f);
        return;
        }
    count = fwrite(&weapcost, sizeof(unsigned long long int), WEAPCOST_SIZE, f);
    if (count < WEAPCOST_SIZE) {
        log_perror("SAVE", "FAIL", tprintf("ERROR: %s weapcost wrote : %d expected %d", target, count, WEAPCOST_SIZE), target);
        SendDB("ERROR occured during saving of econ-save file");
        fclose(f);
        return;
        }
    count = fwrite(&cargocost, sizeof(unsigned long long int), CARGOCOST_SIZE, f);
    if (count < CARGOCOST_SIZE) {
        log_perror("SAVE", "FAIL", tprintf("ERROR: %s cargocost wrote : %d expected %d", target, count, CARGOCOST_SIZE), target);
        SendDB("ERROR occured during saving of econ-save file");
        fclose(f);
        return;
        }
    count = fwrite(&bombcost, sizeof(unsigned long long int), BOMBCOST_SIZE, f);
    if (count < BOMBCOST_SIZE) {
        log_perror("SAVE", "FAIL", tprintf("ERROR: %s bombcost wrote : %d expected %d", target, count, BOMBCOST_SIZE), target);
        SendDB("ERROR occured during saving of econ-save file");
        fclose(f);
        return;
        }
    fclose(f);
    if (i == DUMP_RESTART || i == DUMP_NORMAL) {
        if (rename(target, mudconf.econ_db) < 0) {
            log_perror("SAV", "FAIL", "Renaming econ-save file ", target);
            SendDB("ERROR occured during renaming of econ save-file.");
            }
        }
}
#endif

void SaveSpecialObjects(int i)
{
    FILE *f;
    int filemode, count;
    byte xcode_version = XCODE_VERSION;
    char target[LBUF_SIZE];

    switch (i) {
    case DUMP_KILLED:
	sprintf(target, "%s.KILLED", mudconf.hcode_db);
	break;
    case DUMP_CRASHED:
	sprintf(target, "%s.CRASHED", mudconf.hcode_db);
	break;
    default:			/* RESTART / normal */
	sprintf(target, "%s.tmp", mudconf.hcode_db);
	break;
    }
    f = my_open_file(target, "w", &filemode);
    if (!f) {
	log_perror("SAV", "FAIL", "Opening new hcode-save file", target);
	SendDB("ERROR occured during opening of new hcode-savefile.");
	return;
    }
    fwrite(&xcode_version, 1, 1, f);
    count = SaveTree(f, xcode_tree);
    global_file_kludge = f;
    /* Then, check each xcode thing for stuff */
    GoThruTree(xcode_tree, save_maps_func);

    /* Save autopilot data */
    GoThruTree(xcode_tree, save_autopilot_data);

    saverepairs(f);
    my_close_file(f, &filemode);
    if (i == DUMP_RESTART || i == DUMP_NORMAL) {
	if (rename(mudconf.hcode_db, tprintf("%s.prev", mudconf.hcode_db))
	    < 0) {
	    log_perror("SAV", "FAIL", "Renaming old hcode-save file ",
		target);
	    SendDB
		("ERROR occured during renaming of old hcode save-file.");
	}
	if (rename(target, mudconf.hcode_db) < 0) {
	    log_perror("SAV", "FAIL", "Renaming new hcode-save file ",
		target);
	    SendDB
		("ERROR occured during renaming of new hcode save-file.");
	}
    }
    if (count)
	SendDB(tprintf("Hcode saved. %d xcode entries dumped.", count));
#ifdef BT_ADVANCED_ECON
    save_econdb(target, i);
#endif
}

static int UpdateSpecialObject_func(Node * tmp)
{
    int i;

    i = WhichType(tmp);
    if (!SpecialObjects[i].updateTime)
        return 1;
    if ((mudstate.now % SpecialObjects[i].updateTime))
        return 1;
    SpecialObjects[i].updatefunc(NodeKey(tmp), NodeData(tmp));
    return 1;
}

/* This is called once a second for each special object */

/* Note the new handling for calls being done at <1second intervals,
   or possibly at >1second intervals */

static time_t lastrun = 0;
void UpdateSpecialObjects(void)
{
    char *cmdsave;
    int i;
    int times = lastrun ? (mudstate.now - lastrun) : 1;

    if (times > 20)
        times = 20;		/* Machine's hopelessly lagged,
                           we don't want to make it [much] worse */
    cmdsave = mudstate.debug_cmd;
    for (i = 0; i < times; i++) {
        muxevent_run();
        mudstate.debug_cmd = (char *) "< Generic hcode update handler>";
        GoThruTree(xcode_tree, UpdateSpecialObject_func);
    }
    lastrun = mudstate.now;
    mudstate.debug_cmd = cmdsave;
}

void *NewSpecialObject(int id, int type)
{
    void *foo;
    int i;
    dbref *t;

    if (SpecialObjects[type].datasize) {
	Create(foo, char, (i = SpecialObjects[type].datasize));

	t = (dbref *) foo;
	*t = id;
	if (SpecialObjects[type].allocfreefunc)
	    SpecialObjects[type].allocfreefunc(id, &(foo), SPECIAL_ALLOC);
	AddEntry(&xcode_tree, id, type, i, foo);
    }
    return foo;
}

void CreateNewSpecialObject(dbref player, dbref key)
{
    void *new;
    struct SpecialObjectStruct *typeOfObject;
    int type;
    char *str;

    str = silly_atr_get(key, A_XTYPE);
    if (!(str && *str)) {
	notify(player,
	    "You must first set the XTYPE using @xtype <object>=<type>");
	notify(player, "Valid XTYPEs include: MECH, MECHREP, MAP, DEBUG, "
	    "AUTOPILOT, TURRET, CUSTOM, SCEN, SSIDE, SSOBJ, "
	    "SSINS, SSEXT");
	notify(player, "Resetting hardcode flag.");
	c_Hardcode(key);	/* Reset the flag */
	return;
    }

    /* Find the special objects */
    type = WhichSpecialS(key);
    if (type > -1) {
	/* We found the proper special object */
	typeOfObject = &SpecialObjects[type];
	if (typeOfObject->datasize) {
	    new = NewSpecialObject(key, type);
	    if (!new)
		notify(player, "Memory allocation failure!");
	}
    } else {
	notify(player, "That is not a valid XTYPE!");
	notify(player, "Valid XTYPEs include: MECH, MECHREP, MAP, DEBUG, "
	    "AUTOPILOT, TURRET, CUSTOM, SCEN, SSIDE, SSOBJ, "
	    "SSINS, SSEXT");
	notify(player, "Resetting HARDCODE flag.");
	c_Hardcode(key);
    }
}

void DisposeSpecialObject(dbref player, dbref key)
{
    Node *tmp;
    int i;
    struct SpecialObjectStruct *typeOfObject;

    tmp = FindNode(xcode_tree, key);

    i = WhichSpecialS(key);
    if (i < 0) {
	notify(player,
	    "CRITICAL: Unable to free data, inconsistency somewhere. Please");
	notify(player, "contact a wizard about this _NOW_!");
	return;
    }
    typeOfObject = &SpecialObjects[i];

    if (typeOfObject->datasize > 0 && WhichSpecial(key) != i) {
	notify(player,
	    "Semi-critical error has occured. For some reason the object's data differs\nfrom the data on the object. Please contact a wizard about this.");
	i = WhichSpecial(key);
    }
    if (tmp) {
	void *t = NodeData(tmp);

	if (typeOfObject->allocfreefunc)
	    typeOfObject->allocfreefunc(key, &NodeData(tmp), SPECIAL_FREE);
	NodeData(tmp) = NULL;
	DeleteEntry(&xcode_tree, key);
	muxevent_remove_data(t);
	free(t);
    } else if (typeOfObject->datasize > 0) {
	notify(player, "This object is not in the special object DBASE.");
	notify(player, "Please contact a wizard about this bug. ");
    }
}


void Dump_Mech(dbref player, int type, char *typestr)
{
    notify(player,
	"Support discontinued. Bother a wiz if this bothers you.");
#if 0
    MECH *mech;
    char buff[100];
    int i, running = 0, count = 0;
    Node *temp;

    notify(player, "ID    # STATUS      MAP #      PILOT #");
    notify(player, "----------------------------------------");
    for (temp = TreeTop(xcode_tree); temp; temp = TreeNext(temp))
	if (WhichSpecial((i = NodeKey(temp))) == type) {
	    mech = (MECH *) NodeData(temp);
	    sprintf(buff, "#%5d %-8s    #%5d    #%5d", mech->mynum,
		!Started(mech) ? "SHUTDOWN" : "RUNNING", mech->mapindex,
		MechPilot(mech));
	    notify(player, buff);
	    if (MechStatus(mech) & STARTED)
		running++;
	    count++;
	}
    sprintf(buff, "%d %ss running out of %d %ss allocated.", running,
	typestr, count, typestr);
    notify(player, buff);
    notify(player, "Done listing");
#endif
}

void DumpMechs(dbref player)
{
    Dump_Mech(player, GTYPE_MECH, "mech");
}

void DumpMaps(dbref player)
{
    notify(player,
	"Support discontinued. Bother a wiz if this bothers you.");
#if 0
    MAP *map;
    char buff[100];
    int j, count;
    Node *temp;

    notify(player, "MAP #       NAME              X x Y   MECHS");
    notify(player, "-------------------------------------------");
    for (temp = TreeTop(xcode_tree); temp; temp = TreeNext(temp))
	if (WhichSpecial(NodeKey(temp)) == GTYPE_MAP) {
	    count = 0;
	    map = (MAP *) NodeData(temp);
	    for (j = 0; j < map->first_free; j++)
		if (map->mechsOnMap[j] != -1)
		    count++;
	    sprintf(buff, "#%5d    %-17.17s %3d x%3d       %d", map->mynum,
		map->mapname, map->map_width, map->map_height, count);
	    notify(player, buff);
	}
    notify(player, "Done listing");
#endif
}

/***************** INTERNAL ROUTINES *************/
#ifdef FAST_WHICHSPECIAL
int WhichSpecial(dbref key)
{
    Node *n;

    if (!Hardcode(key))
	return -1;
    if (!(n = FindObjectsNode(key)))
	return -1;
    return NodeType(n);
}

#endif

#ifdef FAST_WHICHSPECIAL
static int WhichSpecialS(dbref key)
#else
int WhichSpecial(dbref key)
#endif
{
    int i;
    int returnValue = -1;
    char *str;

    if (!Hardcode(key))
	return -1;
    str = silly_atr_get(key, A_XTYPE);
    if (str && *str) {
	for (i = 0; i < NUM_SPECIAL_OBJECTS; i++) {
	    if (!strcmp(SpecialObjects[i].type, str)) {
		returnValue = i;
		break;
	    }
	}
    }
    return (returnValue);
}

int IsMech(dbref num)
{
    return WhichSpecial(num) == GTYPE_MECH;
}

int IsAuto(dbref num) {
    return WhichSpecial(num) == GTYPE_AUTO;
}

int IsMap(dbref num) {
    return WhichSpecial(num) == GTYPE_MAP;
}

/*** Support routines ***/
static Node *FindObjectsNode(dbref key)
{
    Node *tmp;

    if ((tmp = FindNode(xcode_tree, (int) key)))
	return tmp;
    return NULL;
}

void *FindObjectsData(dbref key)
{
    Node *tmp;

    if ((tmp = FindObjectsNode(key)))
	return NodeData(tmp);
    return NULL;
}


char *center_string(char *c, int len)
{
    static char buf[LBUF_SIZE];
    int l = strlen(c);
    int p, i;

    p = MAX(0, (len - l) / 2);
    for (i = 0; i < p; i++)
	buf[i] = ' ';
    strcpy(buf + p, c);
    return buf;
}

static void help_color_initialize(char *from, char *to)
{
    int i;
    char buf[LBUF_SIZE];

    for (i = 0; from[i] && from[i] != ' '; i++);
    if (from[i]) {

/*      from[i]=0; */
	strncpy(buf, from, i);
	buf[i] = 0;
	sprintf(to, "%s%s%s %s", "%ch%cb", buf, "%cn", &from[i + 1]);

/*      from[i]=' '; */
    } else
	sprintf(to, "%s%s%s", "%cc", from, "%cn");

}

#define ONE_LINE_TEXTS

#ifdef ONE_LINE_TEXTS
#define MLen CM_ONE
#else
#define MLen CM_TWO
#endif

static char *do_ugly_things(coolmenu ** d, char *msg, int len, int initial)
{
    coolmenu *c = *d;
    char *e;
    int i;
    char buf[LBUF_SIZE];
    char msg2[LBUF_SIZE];

    strcpy(msg2, msg);
#ifndef ONE_LINE_TEXTS
    if (!msg) {
	sim(" ", MLen);
	*d = c;
	return NULL;
    }
#endif
    if (strlen(msg) < len) {
	if (initial) {
	    if (initial > 0)
		help_color_initialize(msg, buf);
	    else {
		for (i = 0; i < -initial; i++)
		    buf[i] = ' ';
		strcpy(&buf[i], msg);
	    }
	    sim(buf, MLen);
	} else
	    sim(msg, MLen);
	*d = c;
	return NULL;
    }
    for (e = msg2 + len - 1; *e != ' '; e--);
    *e = 0;
    if (initial) {
	if (initial > 0)
	    help_color_initialize(msg2, buf);
	else {
	    for (i = 0; i < -initial; i++)
		buf[i] = ' ';
	    strcpy(&buf[i], msg2);
	}
	sim(buf, MLen);
    } else
	sim(msg2, MLen);
    *e = ' ';
    *d = c;
    if ((*e + 1))
	return e + 1;
    return NULL;
}

#define Len(s) ((!s || !*s) ? 0 : strlen(s))

#define TAB 3

static void cut_apart_helpmsgs(coolmenu ** d, char *msg1, char *msg2,
    int len, int initial)
{
    int l1 = Len(msg1);
    int l2 = Len(msg2);
    int nl1, nl2;

#ifndef ONE_LINE_TEXTS

    msg1 = do_ugly_things(d, msg1, len, initial);
    msg2 =
	do_ugly_things(d, msg2, initial ? len : len - TAB,
	initial ? 0 : 0 - TAB);
    if (!msg1 && !msg2)
	return;
    nl1 = Len(msg1);
    nl2 = Len(msg2);
    if (nl1 != l1 || nl2 != l2)	/* To prevent infinite loops */
	cut_apart_helpmsgs(d, msg1, msg2, len, 0);
#else
    int first = 1;

    while (msg1 && *msg1) {
	msg1 = do_ugly_things(d, msg1, len * 2 - 1, first);
	nl1 = Len(msg1);
	if (nl1 == l1)
	    break;
	l1 = nl1;
	first = 0;
    }
    while (msg2 && *msg2) {
	msg2 = do_ugly_things(d, msg2, len * 2 - TAB, 0 - TAB);
	nl2 = Len(msg2);
	if (nl2 == l2)
	    break;
	l2 = nl2;
    }

#endif
}

static void DoSpecialObjectHelp(dbref player, char *type, int id, int loc,
    int powerneeded, int objid, char *arg)
{
    int i, j;
    MECH *mech = NULL;
    int pos[100][2];
    int count = 0, csho = 0;
    coolmenu *c = NULL;
    char buf[LBUF_SIZE];
    char *d;
    int dc;

    if (id == GTYPE_MECH)
	mech = getMech(loc);
    bzero(pos, sizeof(pos));
    for (i = 0; SpecialObjects[id].commands[i].name; i++) {
	if (!SpecialObjects[id].commands[i].func &&
	    (SpecialObjects[id].commands[i].helpmsg[0] != '@' ||
		Have_MechPower(Owner(player), powerneeded)))
	    if (id != GTYPE_MECH ||
		Can_Use_Command(mech,
		    SpecialObjects[id].commands[i].flag)) {
		if (count)
		    pos[count - 1][1] = i - pos[count - 1][0];
		pos[count][0] = i;
		count++;
	    }
    }
    if (count)
	pos[count - 1][1] = i - pos[count - 1][0];
    else {
	pos[0][0] = 0;
	pos[0][1] = i;
	count = 1;
    }
    sim(NULL, CM_ONE | CM_LINE);
    if (!arg || !*arg) {
#define HELPMSG(a) \
      &SpecialObjects[id].commands[a].helpmsg[SpecialObjects[id].commands[a].helpmsg[0]=='@']
	for (i = 0; i < count; i++) {
	    if (count > 1) {
		d = center_string(HELPMSG(pos[i][0]), 70);
		sim(tprintf("%s%s%s", "%cg", d, "%c"), CM_ONE);
	    } else
		sim(tprintf("%s command listing: ", type),
		    CM_ONE | CM_CENTER);
	    for (j = pos[i][0] + (count == 1 ? 0 : 1);
		j < pos[i][0] + pos[i][1]; j++)
		if (SpecialObjects[id].commands[j].helpmsg[0] != '@' ||
		    Have_MechPower(Owner(player), powerneeded))
		    if (id != GTYPE_MECH ||
			Can_Use_Command(mech,
			    SpecialObjects[id].commands[j].flag)) {
			strcpy(buf, SpecialObjects[id].commands[j].name);
			d = buf;
			while (*d && *d != ' ')
			    d++;
			if (*d == ' ')
			    *d = 0;
			sim(buf, CM_FOUR);
			csho++;
		    }
	}
	if (!csho)
	    vsi(tprintf
		("There are no commands you are authorized to use here."));
	else {
	    sim(NULL, CM_ONE | CM_LINE);
	    if (count > 1)
		vsi("Additional info available with 'HELP SUBTOPIC'");
	    else
		vsi("Additional info available with 'HELP ALL'");
	}
    } else {
	/* Try to find matching subtopic, or ALL */
	if (!strcasecmp(arg, "all")) {
	    if (count > 1) {
		vsi("ALL not available for objects with subcategories.");
		dc = -2;
	    } else
		dc = -1;
	} else {
	    if (count == 1) {
		vsi("This object doesn't have any other detailed help than 'HELP ALL'");
		dc = -2;
	    } else {
		for (i = 0; i < count; i++)
		    if (!strcasecmp(arg, HELPMSG(pos[i][0])))
			break;
		if (i == count) {
		    vsi("Subcategory not found.");
		    dc = -2;
		} else
		    dc = i;
	    }
	}
	if (dc > -2) {
	    for (i = 0; i < count; i++)
		if (dc == -1 || i == dc) {
		    if (count > 1)
			vsi(tprintf("%s%s%s", "%cg",
				center_string(HELPMSG(pos[i][0]), 70),
				"%c"));
		    for (j = pos[i][0] + (count == 1 ? 0 : 1);
			j < pos[i][0] + pos[i][1]; j++)
			if (SpecialObjects[id].commands[j].helpmsg[0] !=
			    '@' ||
			    Have_MechPower(Owner(player), powerneeded))
			    if (id != GTYPE_MECH ||
				Can_Use_Command(mech,
				    SpecialObjects[id].commands[j].flag))
				cut_apart_helpmsgs(&c,
				    SpecialObjects[id].commands[j].name,
				    HELPMSG(j), 37, 1);
		}
	}
    }
    sim(NULL, CM_ONE | CM_LINE);
    ShowCoolMenu(player, c);
    KillCoolMenu(c);
}

void InitSpecialHash(int which)
{
    char *tmp, *tmpc;
    int i;
    char buf[MBUF_SIZE];

    hashinit(&SpecialCommandHash[which], 20 * HASH_FACTOR);
    for (i = 0; (tmp = SpecialObjects[which].commands[i].name); i++) {
	if (!SpecialObjects[which].commands[i].func)
	    continue;
	tmpc = buf;
	for (; *tmp && *tmp != ' '; tmp++)
	    *(tmpc++) = ToLower(*tmp);
	*tmpc = 0;
	if ((tmpc = strstr(buf, " ")))
	    *tmpc = 0;
	hashadd(buf, (int *) &SpecialObjects[which].commands[i],
	    &SpecialCommandHash[which]);
    }
}

void handle_xcode(dbref player, dbref obj, int from, int to)
{
    if (from == to)
	return;
    if (!to) {
	s_Hardcode(obj);
	DisposeSpecialObject(player, obj);
	c_Hardcode(obj);
    } else
	CreateNewSpecialObject(player, obj);
}


#define DEFAULT 0		/* Normal */
#define ANSI_START "\033["
#define ANSI_START_LEN 2
#define ANSI_END "m"
#define ANSI_END_LEN 1

struct color_entry {
    int bit;
    int negbit;
    char ltr;
    char *string;
    char *sstring;
} color_table[] = {
    {
    0x0008, 7, 'n', ANSI_NORMAL, NULL}, {
    0x0001, 0, 'h', ANSI_HILITE, NULL}, {
    0x0002, 0, 'i', ANSI_INVERSE, NULL}, {
    0x0004, 0, 'f', ANSI_BLINK, NULL}, {
    0x0010, 0, 'x', ANSI_BLACK, NULL}, {
    0x0010, 0x10, 'l', ANSI_BLACK, NULL}, {
    0x0020, 0, 'r', ANSI_RED, NULL}, {
    0x0040, 0, 'g', ANSI_GREEN, NULL}, {
    0x0080, 0, 'y', ANSI_YELLOW, NULL}, {
    0x0100, 0, 'b', ANSI_BLUE, NULL}, {
    0x0200, 0, 'm', ANSI_MAGENTA, NULL}, {
    0x0400, 0, 'c', ANSI_CYAN, NULL}, {
    0x0800, 0, 'w', ANSI_WHITE, NULL}, {
    0, 0, 0, NULL, NULL}
};

#define CHARS 256

char colorc_reverse[CHARS];

void initialize_colorize()
{
    int i;
    char buf[20];
    char *c;

    c = buf + ANSI_START_LEN;
    for (i = 0; i < CHARS; i++)
	colorc_reverse[i] = DEFAULT;
    for (i = 0; color_table[i].string; i++) {
	colorc_reverse[(short) color_table[i].ltr] = i;
	strcpy(buf, color_table[i].string);
	buf[strlen(buf) - ANSI_END_LEN] = 0;
	color_table[i].sstring = strdup(c);
    }

}


#undef notify
char *colorize(dbref player, char *from)
{
    char *to;
    char *p, *q;
    int color_wanted = 0;
    int i;
    int cnt;

    q = to = alloc_lbuf("colorize");
#if 1
    for (p = from; *p; p++) {
	if (*p == '%' && *(p + 1) == 'c') {
	    p += 2;
	    if (*p <= 0)
		i = DEFAULT;
	    else
		i = colorc_reverse[(short) *p];
	    if (i == DEFAULT && *p != 'n')
		p--;
	    color_wanted &= ~color_table[i].negbit;
	    color_wanted |= color_table[i].bit;
	} else {
	    if (color_wanted && Ansi(player)) {
		*q = 0;
		/* Generate efficient color string */
		strcpy(q, ANSI_START);
		q += ANSI_START_LEN;
		cnt = 0;
		for (i = 0; color_table[i].string; i++)
		    if (color_wanted & color_table[i].bit &&
			color_table[i].bit != color_table[i].negbit) {
			if (cnt)
			    *q++ = ';';
			strcpy(q, color_table[i].sstring);
			q += strlen(color_table[i].sstring);
			cnt++;
		    }
		strcpy(q, ANSI_END);
		q += ANSI_END_LEN;
		color_wanted = 0;
	    }
	    *q++ = *p;
	}
    }
    *q = 0;
    if (color_wanted && Ansi(player)) {
	/* Generate efficient color string */
	strcpy(q, ANSI_START);
	q += ANSI_START_LEN;
	cnt = 0;
	for (i = 0; color_table[i].string; i++)
	    if (color_wanted & color_table[i].bit &&
		color_table[i].bit != color_table[i].negbit) {
		if (cnt)
		    *q++ = ';';
		strcpy(q, color_table[i].sstring);
		q += strlen(color_table[i].sstring);
		cnt++;
	    }
	strcpy(q, ANSI_END);
	q += ANSI_END_LEN;
	color_wanted = 0;
    }
#else
    strcpy(to, p);
#endif
    return to;
}

void mecha_notify(dbref player, char *msg)
{
    char *tmp;

    tmp = colorize(player, msg);
    raw_notify(player, tmp);
    free_lbuf(tmp);
}

void mecha_notify_except(dbref loc, dbref player, dbref exception,
    char *msg)
{
    dbref first;

    if (loc != exception)
	notify_checked(loc, player, msg,
	    (MSG_ME_ALL | MSG_F_UP | MSG_S_INSIDE | MSG_NBR_EXITS_A |
		MSG_COLORIZE));
    DOLIST(first, Contents(loc)) {
	if (first != exception) {
	    notify_checked(first, player, msg,
		(MSG_ME | MSG_F_DOWN | MSG_S_OUTSIDE | MSG_COLORIZE));
	}
    }
}

/* 
   Basically, finish all the repairs etc in one fell swoop. That's the
   best we can do for now, I'm afraid. 
   */
void ResetSpecialObjects()
{
#if 0				/* Nowadays no longer neccessary, see mech.tech.saverepair.c */
    int i;

    for (i = FIRST_TECH_EVENT; i <= LAST_TECH_EVENT; i++)
	while (muxevent_run_by_type(i));
#endif
    muxevent_run_by_type(EVENT_HIDE);
    muxevent_run_by_type(EVENT_BLINDREC);
}


MAP *getMap(dbref d)
{
    Node *tmp;

    if (!(tmp = FindObjectsNode(d)))
	return NULL;
    if (NodeType(tmp) != GTYPE_MAP)
	return NULL;
    return (MAP *) NodeData(tmp);
}

MECH *getMech(dbref d)
{
    Node *tmp;

    if (d < 0)
    	return NULL;
    if (!(tmp = FindObjectsNode(d)))
	return NULL;
    if (NodeType(tmp) != GTYPE_MECH)
	return NULL;
    return (MECH *) NodeData(tmp);
}
