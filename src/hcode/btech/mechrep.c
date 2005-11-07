
/*
 * $Id: mechrep.c,v 1.5 2005/07/02 19:02:23 av1-op Exp $
 *
 * Last modified: Thu Aug 13 23:41:12 1998 fingon
 * Copyright (c) 1999-2005 Kevin Stevens
 *   All right reserved
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>
#include <dirent.h>
#include <sys/stat.h>

#define MECH_STAT_C		/* want to use the POSIX stat() call. */

#include "externs.h"
#include "mech.h"
#include "mechrep.h"
#include "create.h"
#include "p.mech.build.h"
#include "p.mech.status.h"
#include "p.template.h"
#include "p.mechrep.h"
#include "p.mech.restrict.h"
#include "p.mech.consistency.h"
#include "p.mech.utils.h"
#include "mech.events.h"

/* Selectors */
#define SPECIAL_FREE 0
#define SPECIAL_ALLOC 1

extern char *strtok(char *s, const char *ct);

/* EXTERNS THAT SHOULDN'T BE IN HERE! */
extern void *FindObjectsData(dbref key);
dbref match_thing(dbref player, char *name);
void muxevent_remove_data(void *data);

#define MECHREP_COMMON(a) \
struct mechrep_data *rep = (struct mechrep_data *) data; \
MECH *mech; \
DOCHECK(!Template(player), "I'm sorry Dave, can't do that."); \
if (!CheckData(player, rep)) return; \
if (a) { DOCHECK(rep->current_target == -1, "You must set a target first!"); \
mech = getMech (rep->current_target); \
if (!CheckData(player, mech)) return; }

/*--------------------------------------------------------------------------*/

/* Code Begins                                                              */

/*--------------------------------------------------------------------------*/

/* Alloc free function */

/* Alloc/free routine */

void newfreemechrep(dbref key, void **data, int selector)
{
    struct mechrep_data *new = *data;

    switch (selector) {
    case SPECIAL_ALLOC:
	new->current_target = -1;
	break;
    }
}

/* With cap R means restricted command */

void mechrep_Rresetcrits(dbref player, void *data, char *buffer)
{
    int i;

    MECHREP_COMMON(1);
    notify(player, "Default criticals set!");
    for (i = 0; i < NUM_SECTIONS; i++)
	FillDefaultCriticals(mech, i);
}

void mechrep_Rdisplaysection(dbref player, void *data, char *buffer)
{
    char *args[1];
    int index;

    MECHREP_COMMON(1);
    DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
	"You must specify a section to list the criticals for!");
    index =
	ArmorSectionFromString(MechType(mech), MechMove(mech), args[0]);
    DOCHECK(index == -1, "Invalid section!");
    CriticalStatus(player, mech, index);
}

#define MechComputersRadioRange(mech) \
(DEFAULT_RADIORANGE * generic_radio_multiplier(mech))

void mechrep_Rsetradio(dbref player, void *data, char *buffer)
{
    char *args[2];
    int i;

    MECHREP_COMMON(1);
    switch (mech_parseattributes(buffer, args, 2)) {
    case 0:
	notify(player,
	    "This remains to be done [showing of stuff when no args]");
	return;
    case 2:
	notify(player, "Too many args, unable to cope().");
	return;
    }
    i = BOUNDED(1, atoi(args[0]), 5);
    notify(player, tprintf("Radio level set to %d.", i));
    MechRadio(mech) = i;
    MechRadioType(mech) = generic_radio_type(MechRadio(mech), 0);
    notify(player, tprintf("Number of freqs: %d  Extra stuff: %d",
	    MechRadioType(mech) % 16, (MechRadioType(mech) / 16) * 16));
    MechRadioRange(mech) = MechComputersRadioRange(mech);
    notify(player, tprintf("Radio range set to %d.",
	    (int) MechRadioRange(mech)));
}

void mechrep_Rsettarget(dbref player, void *data, char *buffer)
{
    char *args[2];
    int newmech;

    MECHREP_COMMON(0);
    switch (mech_parseattributes(buffer, args, 2)) {
    case 1:
	newmech = match_thing(player, args[0]);
	DOCHECK(!(Good_obj(newmech) &&
		Hardcode(newmech)),
	    "That is not a BattleMech or Vehicle!");
	rep->current_target = newmech;
	notify(player, tprintf("Mech to repair changed to #%d", newmech));
	break;
    default:
	notify(player, "Too many arguments!");
    }
}

void mechrep_Rsettype(dbref player, void *data, char *buffer)
{
    char *args[1];

    MECHREP_COMMON(1);
    DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
	"Invalid number of arguments!");
    switch (toupper(args[0][0])) {
    case 'M':
	MechType(mech) = CLASS_MECH;
	MechMove(mech) = MOVE_BIPED;
	notify(player, "Type set to MECH");
	break;
    case 'Q':
	MechType(mech) = CLASS_MECH;
	MechMove(mech) = MOVE_QUAD;
	notify(player, "Type set to QUAD");
	break;
    case 'G':
	MechType(mech) = CLASS_VEH_GROUND;
	notify(player, "Type set to VEHICLE");
	break;
    case 'V':
	MechType(mech) = CLASS_VTOL;
	MechMove(mech) = MOVE_VTOL;
	notify(player, "Type set to VTOL");
	break;
    case 'N':
	MechType(mech) = CLASS_VEH_NAVAL;
	notify(player, "Type set to NAVAL");
	break;
    case 'A':
	MechType(mech) = CLASS_AERO;
	MechMove(mech) = MOVE_FLY;
	notify(player, "Type set to AeroSpace");
	break;
    case 'D':
	MechType(mech) = CLASS_DS;
	MechMove(mech) = MOVE_FLY;
	notify(player, "Type set to DropShip");
	break;
    case 'S':
	MechType(mech) = CLASS_SPHEROID_DS;
	MechMove(mech) = MOVE_FLY;
	notify(player, "Type set to SpheroidDropship");
	break;
    case 'B':
    	MechType(mech) = CLASS_BSUIT;
    	MechMove(mech) = MOVE_BIPED;
    	notify(player, "Type set to BattleSuit");
    	break;
    default:
	notify(player,
	    "Types are: MECH, GROUND, VTOL, NAVAL, AERO, DROPSHIP and SPHEROIDDROPSHIP");
	break;
    }
}

#define SETVALUE_FUNCTION_FLOAT(funcname,valname,valstring,modifier) \
void funcname (dbref player, void *data, char *buffer) \
{ char *args[1]; float f; MECHREP_COMMON(1); \
  DOCHECK(mech_parseattributes (buffer, args, 1) != 1, \
	  tprintf("Invalid number of arguments to Set%s!", valstring)); \
  f = atof(args[0]); \
  valname = f * modifier; \
  notify(player, tprintf("%s changed to %.2f.", valstring, valname)); \
}


#define SETVALUE_FUNCTION_INT(funcname,valname,valstring,modifier) \
void funcname (dbref player, void *data, char *buffer) \
{ char *args[1]; int f; MECHREP_COMMON(1); \
  DOCHECK(mech_parseattributes (buffer, args, 1) != 1, \
	  tprintf("Invalid number of arguments to Set%s!", valstring)); \
  f = atoi(args[0]); \
  valname = f * modifier; \
  notify(player, tprintf("%s changed to %d.", valstring, valname)); \
}

SETVALUE_FUNCTION_FLOAT(mechrep_Rsetspeed, MechMaxSpeed(mech), "Maxspeed",
    KPH_PER_MP);
SETVALUE_FUNCTION_FLOAT(mechrep_Rsetjumpspeed, MechJumpSpeed(mech),
    "Jumpspeed", KPH_PER_MP);
SETVALUE_FUNCTION_INT(mechrep_Rsetheatsinks, MechRealNumsinks(mech),
    "Heatsinks", 1);
SETVALUE_FUNCTION_INT(mechrep_Rsetlrsrange, MechLRSRange(mech), "LRSrange",
    1);
SETVALUE_FUNCTION_INT(mechrep_Rsettacrange, MechTacRange(mech), "TACrange",
    1);
SETVALUE_FUNCTION_INT(mechrep_Rsetscanrange, MechScanRange(mech),
    "SCANrange", 1);
SETVALUE_FUNCTION_INT(mechrep_Rsetradiorange, MechRadioRange(mech),
    "RADIOrange", 1);
SETVALUE_FUNCTION_INT(mechrep_Rsettons, MechTons(mech), "Tons", 1);


void mechrep_Rsetmove(dbref player, void *data, char *buffer)
{
    char *args[1];

    MECHREP_COMMON(1);
    DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
	"Invalid number of arguments!");
    switch (toupper(args[0][0])) {
    case 'T':
	MechMove(mech) = MOVE_TRACK;
	notify(player, "Movement set to TRACKED");
	break;
    case 'W':
	MechMove(mech) = MOVE_WHEEL;
	notify(player, "Movement set to WHEELED");
	break;
    case 'H':
	switch (toupper(args[0][1])) {
	case 'O':
	    MechMove(mech) = MOVE_HOVER;
	    notify(player, "Movement set to HOVER");
	    break;
	case 'U':
	    MechMove(mech) = MOVE_HULL;
	    notify(player, "Movement set to HULL");
	    break;
	}
	break;
    case 'V':
	MechMove(mech) = MOVE_VTOL;
	notify(player, "Movement set to VTOL");
	break;
    case 'Q':
	MechMove(mech) = MOVE_QUAD;
	notify(player, "Movement set to QUAD");
	break;
    case 'S':
	MechMove(mech) = MOVE_SUB;
	notify(player, "Movement set to SUB");
	break;
    case 'F':
	switch (toupper(args[0][1])) {
	case 'O':
	    MechMove(mech) = MOVE_FOIL;
	    notify(player, "Movement set to FOIL");
	    break;
	case 'L':
	    MechMove(mech) = MOVE_FLY;
	    notify(player, "Movement set to FLY");
	    break;
	}
    case 'N':
    	MechMove(mech) = MOVE_NONE;
    	notify(player, "Movement set to NONE");
    	break;
    default:
	notify(player,
	    "Types are: TRACK, WHEEL, VTOL, HOVER, HULL, FLY, SUB, FOIL and NONE");
	break;
    }
}

/*
 * Implement a name cache of a template names.  This allows differences
 * in case and characters past the 14th to be ignored in mech references
 * when loading templates.  Templates can also be stored in any subdirectory
 * of the main template directory instead of in just one of list of hard
 * coded subdirectories.  
 * 
 * CACHE_MAXNAME sets the limit on how long a template filename can be.
 * Any template with a filename longer than this is ignored and not stored
 * in the cache.
 *
 * MECH_MAXREF sets the number of signficant characters in a mechref when
 * searching the cache.  This should be equal to the length of the
 * 'mech_type' (minus one for the terminating '\0' character) field of
 * MECH structure.
 * 
 */

enum {
    CACHE_MAXNAME = 24,
    MECH_MAXREF = 14
};

struct tmpldirent {
    char name[CACHE_MAXNAME + 1];
    char const *dir;
};

struct tmpldir {
    char name[CACHE_MAXNAME + 1];
    struct tmpldir *next;
};

struct tmpldirent *tmpl_list = NULL;
int tmpl_pos = 0;
int tmpl_len = 0;

struct tmpldir *tmpldir_list = NULL;

/*
 * The ordering function for the template name cache.  Used to sort and
 * search the cache.
 */
static int tmplcmp(void const *v1, void const *v2)
{
    struct tmpldirent const *p1 = v1;
    struct tmpldirent const *p2 = v2;

    return strncasecmp(p1->name, p2->name, MECH_MAXREF);
}

/*
 * Add all the template names in a directory to the template cache.
 */
static int scan_template_dir(char const *dirname, char const *parent)
{
    char buf[1000];
    int dirnamelen = strlen(dirname);
    DIR *dir = opendir(dirname);

    if (dir == NULL) {
	return -1;
    }

    while (1) {
	struct stat sb;
	struct dirent *ent = readdir(dir);

	if (ent == NULL) {
	    break;
	}

	if (dirnamelen + 1 + strlen(ent->d_name) + 1 > sizeof buf) {
	    continue;
	}

	sprintf(buf, "%s/%s", dirname, ent->d_name);
	if (stat(buf, &sb) == -1) {
	    continue;
	}

	if (parent == NULL && S_ISDIR(sb.st_mode)
	    && ent->d_name[0] != '.' &&
	    strlen(ent->d_name) <= CACHE_MAXNAME) {
	    struct tmpldir *link;

	    Create(link, struct tmpldir, 1);

	    strcpy(link->name, ent->d_name);
	    link->next = tmpldir_list;
	    tmpldir_list = link;
	    continue;
	}

	if (!S_ISREG(sb.st_mode)) {
	    continue;
	}

	if (tmpl_pos == tmpl_len) {
	    if (tmpl_len == 0) {
		tmpl_len = 4;
		Create(tmpl_list, struct tmpldirent, tmpl_len);
	    } else {
		tmpl_len *= 2;
		ReCreate(tmpl_list, struct tmpldirent, tmpl_len);
	    }
	}

	strncpy(tmpl_list[tmpl_pos].name, ent->d_name, CACHE_MAXNAME);
	tmpl_list[tmpl_pos].name[CACHE_MAXNAME] = '\0';
	tmpl_list[tmpl_pos].dir = parent;
	tmpl_pos++;
    }

    closedir(dir);
    return 0;
}

/*
 * Scan all the template names in the mech template directory.  Only looks
 * in the mech template directory and it immediate subdirectories. 
 * It doesn't recursively look any further down the tree.
 */

static int scan_templates(char const *dir)
{
    char buf[1000];
    struct tmpldir *p;

    if (scan_template_dir(dir, NULL) == -1) {
	return -1;
    }

    p = tmpldir_list;
    while (p != NULL) {
	sprintf(buf, "%s/%s", dir, p->name);
	scan_template_dir(buf, p->name);
	p = p->next;
    }

    qsort(tmpl_list, tmpl_pos, sizeof tmpl_list[0], tmplcmp);

    return 0;
}

/*
 * Free all the memory used by the template cache.  Sets the cache to
 * the empty state.
 */
static void free_template_list()
{
    struct tmpldir *p;

    free(tmpl_list);

    p = tmpldir_list;
    while (p != NULL) {
	struct tmpldir *np = p->next;

	free(p);
	p = np;
    }

    tmpl_list = NULL;
    tmpldir_list = NULL;
    tmpl_pos = 0;
    tmpl_len = 0;

    return;
}

char *subdirs[] = {
    "3025",
    "3050",
    "3055",
    "3058",
    "3060",
    "2750",
    "Aero",
    "MISC",
    "Clan",
    "ClanVehicles",
    "Clan2nd",
    "ClanAero",
    "Custom",
    "Solaris",
    "Vehicles",
    "MFNA",
    "Infantry",
    NULL
};

void mechrep_Rloadnew(dbref player, void *data, char *buffer)
{
    char *args[1];

    MECHREP_COMMON(1);
    if (mech_parseattributes(buffer, args, 1) == 1)
	if (mech_loadnew(player, mech, args[0]) == 1) {
	    muxevent_remove_data((void *) mech);
	    clear_mech_from_LOS(mech);
	    notify(player, "Template loaded.");
	    return;
	}
    notify(player, "Unable to read that template.");
}

void clear_mech(MECH * mech, int flag)
{
    int i, j;

    mech->brief = 1;

    bzero(&mech->rd, sizeof(mech_rd));
    bzero(&mech->ud, sizeof(mech_ud));

    MechSpotter(mech) = -1;
    MechTarget(mech) = -1;
    MechChargeTarget(mech) = -1;
    MechChargeTimer(mech) = 0;
    MechChargeDistance(mech) = 0;
    MechSwarmTarget(mech) = -1;
    MechDFATarget(mech) = -1;
    MechTargX(mech) = -1;
    MechStatus(mech) = 0;
    MechTargY(mech) = -1;
    MechPilot(mech) = -1;
    MechAim(mech) = NUM_SECTIONS;
    StopBurning(mech);
    if (flag) {
	for (i = 0; i < NUM_TICS; i++)
	    for (j = 0; j < TICLONGS; j++)
		mech->tic[i][j] = 0;
	for (i = 0; i < FREQS; i++) {
	    mech->freq[i] = 0;
	    mech->freqmodes[i] = 0;
	    mech->chantitle[i][0] = 0;
	}
    }
}

char *mechref_path(char *id)
{
    static char openfile[1024];
    FILE *fp;
    int i = 0;			/* this int has double use... ugly, but effective */

    /*
     * If the template name doesn't have slash search for it in the
     * template name cache.
     */
  redo:
    if (strchr(id, '/') == NULL && (tmpl_list != NULL ||
	    scan_templates(MECH_PATH) != -1)) {
	struct tmpldirent *ent;
	struct tmpldirent key;

	strncpy(key.name, id, CACHE_MAXNAME);
	key.name[CACHE_MAXNAME] = '\0';

	ent =
	    bsearch(&key, tmpl_list, tmpl_pos, sizeof tmpl_list[0],
	    tmplcmp);
	if (ent == NULL) {
	    return NULL;
	}
	if (ent->dir == NULL) {
	    sprintf(openfile, "%s/%s", MECH_PATH, ent->name);
	} else {
	    sprintf(openfile, "%s/%s/%s", MECH_PATH, ent->dir, ent->name);
	}
	if (access(openfile, R_OK) != 0) {
	    /* The file is missing (or unreadable)
	       invalidate the cache and try again,
	       if *that* fails, fall back to the old version. */
	    if (!i) {
		i = 1;
		free_template_list();
		goto redo;
	    } else
		goto oldstyle;
	}
	return openfile;
    }
  oldstyle:
    /*
     * Look up a template name the old way...
     */
    sprintf(openfile, "%s/%s", MECH_PATH, id);
    fp = fopen(openfile, "r");
    for (i = 0; !fp && subdirs[i]; i++) {
	sprintf(openfile, "%s/%s/%s", MECH_PATH, subdirs[i], id);
	fp = fopen(openfile, "r");
    }
    if (fp) {
	fclose(fp);
	return openfile;
    }
    return NULL;
}

int load_mechdata2(dbref player, MECH * mech, char *id)
{
    FILE *fp = NULL;
    char *filename;

    filename = mechref_path(id);

    if (!filename)
	return 0;
    if (!(fp = fopen(filename, "r")))
	return 0;
    fclose(fp);
    return load_template(player, mech, filename) >= 0 ? 1 : 0;
}

extern int num_def_weapons;

int unable_to_find_proper_type(int i)
{
    if (!i)
	return 0;
    if (IsWeapon(i)) {
	if (i > (num_def_weapons))
	    return 1;
    }
    if (IsAmmo(i)) {
	if ((Ammo2Weapon(i) + 1) > (num_def_weapons))
	    return 1;
    }
    if (IsSpecial(i))
	if (Special2I(i) >= count_special_items())
	    return 1;
    return 0;
}

int load_mechdata(MECH * mech, char *id)
{
    FILE *fp = NULL;
    int i, j, k, t;
    int i1, i2, i3, i4, i5, i6;
    char *filename;

    filename = mechref_path(id);
    TEMPLATE_GERR(filename == NULL, tprintf("No matching file for '%s'.",
	    id));
    if (filename)
	fp = fopen(filename, "r");
    TEMPLATE_GERR(!fp, tprintf("Unable to open file %s (%s)!", filename,
	    id));
    strncpy(MechType_Ref(mech), id, 15);
    MechType_Ref(mech)[14] = '\0';
    TEMPLATE_GERR(fscanf(fp, "%d %d %d %d %d %f %f %d\n", &i1, &i2, &i3,
	    &i4, &i5, &MechMaxSpeed(mech), &MechJumpSpeed(mech), &i6) < 8,
	"Old template loading system: %s is invalid template file.", id);
    MechTons(mech) = i1;
    MechTacRange(mech) = i2;
    MechLRSRange(mech) = i3;
    MechScanRange(mech) = i4;
    MechRealNumsinks(mech) = i5;
#define DROP(a) \
  if (i6 & a) i6 &= ~a
    DROP(32768);		/* Quad */
    DROP(16384);		/* Salvagetech */
    DROP(8192);			/* Cargotech */
    DROP(4196);			/* Watergun */
    MechSpecials(mech) = i6;
    for (k = 0; k < NUM_SECTIONS; k++) {
	i = k;
	if (MechType(mech) == 4) {
	    switch (k) {
	    case 3:
		i = 4;
		break;
	    case 4:
		i = 5;
		break;
	    case 5:
		i = 3;
		break;
	    }
	}
	TEMPLATE_GERR(fscanf(fp, "%d %d %d %d\n", &i1, &i2, &i3, &i4) < 4,
	    "Insufficient data reading section %d!", i);
	MechSections(mech)[i].recycle = 0;
	SetSectArmor(mech, i, i1);
	SetSectOArmor(mech, i, i1);
	SetSectInt(mech, i, i2);
	SetSectOInt(mech, i, i2);
	SetSectRArmor(mech, i, i3);
	SetSectORArmor(mech, i, i3);
	/* Remove all rampant AXEs from the arms themselves, we do
	   things differently here */
	if (i4 & 4)
	    i4 &= ~4;
	MechSections(mech)[i].config = i4;
	for (j = 0; j < NUM_CRITICALS; j++) {
	    TEMPLATE_GERR(fscanf(fp, "%d %d %d\n", &i1, &i2, &i3) < 3,
		"Insufficient data reading critical %d/%d!", i, j);
	    MechSections(mech)[i].criticals[j].type = i1;
	    TEMPLATE_GERR(unable_to_find_proper_type(GetPartType(mech, i,
			j)), "Invalid datatype at %d/%d!", i, j);
	    if (IsSpecial(i1))
		i1 += SPECIAL_BASE_INDEX - OSPECIAL_BASE_INDEX;
	    if (IsWeapon(GetPartType(mech, i, j)) &&
		IsAMS((t = Weapon2I(GetPartType(mech, i, j))))) {
		if (MechWeapons[t].special & CLAT)
		    MechSpecials(mech) |= CL_ANTI_MISSILE_TECH;
		else
		    MechSpecials(mech) |= IS_ANTI_MISSILE_TECH;
	    }
	    MechSections(mech)[i].criticals[j].data = i2;
	    MechSections(mech)[i].criticals[j].firemode = i3;
	}
    }
    if (fscanf(fp, "%d %d\n", &i1, &i2) == 2) {
	MechType(mech) = i1;
	TEMPLATE_GERR(MechType(mech) > CLASS_LAST, "Invalid 'mech type!");
	MechMove(mech) = i2;
	TEMPLATE_GERR(MechMove(mech) > MOVENEMENT_LAST,
	    "Invalid movenement type!");
    }
    if (fscanf(fp, "%d\n", &i1) != 1)
	MechRadioRange(mech) = DEFAULT_RADIORANGE;
    else
	MechRadioRange(mech) = i1;
    fclose(fp);
    return 1;
}

#undef  LOADNEW_LOADS_OLD_IF_FAIL
#define LOADNEW_LOADS_MUSE_FORMAT

int mech_loadnew(dbref player, MECH * mech, char *id)
{
    char mech_origid[100];

    strncpy(mech_origid, MechType_Ref(mech), 99);
    mech_origid[99] = '\0';

    if (!strcmp(mech_origid, id)) {
	clear_mech(mech, 0);
	if (load_mechdata2(player, mech, id) <= 0)
	    return load_mechdata(mech, id) > 0;
	return 1;
    } else {
	clear_mech(mech, 1);
	if (load_mechdata2(player, mech, id) < 1)
#ifdef LOADNEW_LOADS_MUSE_FORMAT
	    if (load_mechdata(mech, id) < 1)
#endif
#ifdef LOADNEW_LOADS_OLD_IF_FAIL
		if (load_mechdata2(player, mech, mech_origid) < 1)
#ifdef LOADNEW_LOADS_MUSE_FORMAT
		    if (load_mechdata(mech, mech_origid) < 1)
#endif
#endif
			return 0;
    }
    return 1;
}

void mechrep_Rrestore(dbref player, void *data, char *buffer)
{
    char *c;

    MECHREP_COMMON(1);
    c = silly_atr_get(mech->mynum, A_MECHREF);
    DOCHECK(!c || !*c, "Sorry, I don't know what type of mech this is");
    DOCHECK(mech_loadnew(player, mech, c) == 1, "Restoration complete!");
    notify(player, "Unable to restore this mech!.");
}

void mechrep_Rsavetemp(dbref player, void *data, char *buffer)
{
    char *args[1];
    FILE *fp;
    char openfile[512];
    int i, j;

    MECHREP_COMMON(1);

    free_template_list();

    DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
	"You must specify a template name!");
    DOCHECK(strstr(args[0], "/"), "Invalid file name!");
    notify(player, tprintf("Saving %s", args[0]));
    sprintf(openfile, "%s/", MECH_PATH);
    strcat(openfile, args[0]);
    DOCHECK(!(fp =
	    fopen(openfile, "w")),
	"Unable to open/create mech file! Sorry.");
    fprintf(fp, "%d %d %d %d %d %.2f %.2f %d\n", MechTons(mech),
	MechTacRange(mech), MechLRSRange(mech), MechScanRange(mech),
	MechRealNumsinks(mech), MechMaxSpeed(mech), MechJumpSpeed(mech),
	MechSpecials(mech));
    for (i = 0; i < NUM_SECTIONS; i++) {
	fprintf(fp, "%d %d %d %d\n", GetSectArmor(mech, i),
	    GetSectInt(mech, i), GetSectRArmor(mech, i),
	    MechSections(mech)[i].config);
	for (j = 0; j < NUM_CRITICALS; j++) {
	    fprintf(fp, "%d %d %d\n",
		MechSections(mech)[i].criticals[j].type,
		MechSections(mech)[i].criticals[j].data,
		MechSections(mech)[i].criticals[j].firemode);
	}
    }
    fprintf(fp, "%d %d\n", MechType(mech), MechMove(mech));
    fprintf(fp, "%d\n", MechRadioRange(mech));
    fclose(fp);
    notify(player, "Saving complete!");
}

void mechrep_Rsavetemp2(dbref player, void *data, char *buffer)
{
    char *args[1];
    char openfile[512];

    MECHREP_COMMON(1);

    free_template_list();

    DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
	"You must specify a template name!");
    DOCHECK(strstr(args[0], "/"), "Invalid file name!");
    notify(player, tprintf("Saving %s", args[0]));
    sprintf(openfile, "%s/", MECH_PATH);
    strcat(openfile, args[0]);
    DOCHECK(mech_weight_sub(GOD, mech, -1) > (MechTons(mech) * 1024),
	"Error saving template: Too heavy.");
    DOCHECK(save_template(player, mech, args[0], openfile) < 0,
	"Error saving the template file!");
    notify(player, "Saving complete!");
}

void mechrep_Rsetarmor(dbref player, void *data, char *buffer)
{
    char *args[4];
    int argc;
    int index;
    int temp;

    MECHREP_COMMON(1);
    argc = mech_parseattributes(buffer, args, 4);
    DOCHECK(!argc, "Invalid number of arguments!");
    index =
	ArmorSectionFromString(MechType(mech), MechMove(mech), args[0]);
    if (index == -1) {
	notify(player, "Not a legal area. Must be HEAD, CTORSO");
	notify(player, "LTORSO, RTORSO, RLEG, LLEG, RARM, LARM");
	notify(player, "TURRET, ROTOR, RSIDE, LSIDE, FRONT, AFT");
	return;
    }
    argc--;
    if (argc) {
	temp = atoi(args[1]);
	if (temp < 0)
	    notify(player, "Invalid armor value!");
	else {
	    notify(player, "Front armor set!");
	    SetSectArmor(mech, index, temp);
	    SetSectOArmor(mech, index, temp);
	}
	argc--;
    }
    if (argc) {
	temp = atoi(args[2]);
	if (temp < 0)
	    notify(player, "Invalid Internal armor value!");
	else {
	    notify(player, "Internal armor set!");
	    SetSectInt(mech, index, temp);
	    SetSectOInt(mech, index, temp);
	}
	argc--;
    }
    if (argc) {
	temp = atoi(args[3]);
	if (index == CTORSO || index == RTORSO || index == LTORSO) {
	    if (temp < 0)
		notify(player, "Invalid Rear armor value!");
	    else {
		notify(player, "Rear armor set!");
		SetSectRArmor(mech, index, temp);
		SetSectORArmor(mech, index, temp);
	    }
	} else
	    notify(player, "Only the torso can have rear armor.");
    }
}

/* 
 * Handles the adding of weapons via the 'addweap' command in the form of:
 * addweap <weap> <loc> <crits> [<flags>]
 * Current Flags: O = OS, T = TC, R = Rear
 */
void mechrep_Raddweap(dbref player, void *data, char *buffer)
{
    char *args[20];	/* The argument array */
    int argc;		/* Count of arguments */
    int index;		/* Used to determine section validity */
    int weapindex;	/* Weapon index number */
    int weapnumcrits;	/* Number of crits the desired weapon occupies. */
    int weaptype;	/* The weapon type */
    int loop, temp;	/* Loop Counters */
    int isrear = 0;	/* Rear mounted? */
    int istc = 0;	/* Is the weap TC'd? */
    int isoneshot = 0;  /* If 1, weapon is a One-Shot (OS) Weap */
    int isrocket = 0;	/* Is this a rocket launcher? */
    int argstoiter;	/* Holder for figuring out how many args to scan */
    char flagholder;	/* Holder for flag comparisons */

    MECHREP_COMMON(1);
    
    argc = mech_parseattributes(buffer, args, 20);
    DOCHECK(argc < 3, "Invalid number of arguments!")
	    
    index = ArmorSectionFromString(MechType(mech), MechMove(mech), args[1]);
    if (index == -1) {
	notify_printf(player, "Not a legal area. Must be HEAD, CTORSO");
	notify_printf(player, "LTORSO, RTORSO, RLEG, LLEG, RARM, LARM");
	notify_printf(player, "TURRET, ROTOR, RSIDE, LSIDE, FRONT, AFT");
	return;
    }
    
    weapindex = WeaponIndexFromString(args[0]);
    
    if (weapindex == -1) {
	notify_printf(player, "That is not a valid weapon!");
	DumpWeapons(player);
	return;
    }

    /* 
     * There are always 3 arguments that preceed flags. 
     * addweap <weap> <loc> <crit>, 0, 1, and 2 respectively in args[][]. 
     * By subtracting 3, we figure out how many of our arguments are actually 
     * flags.
     */
    argstoiter = argc - 3;
    
    /*
     * Now we take those additional flags and look for matches. argc is 
     * decremented to keep track of how many of our arguments are crit
     * locations.
     */
    for (loop = 0; loop < argstoiter; loop++) {
	flagholder = toupper(args[3 + loop][0]);

	    if (flagholder == 'T') {
		/* Targeting Computer */
	        istc = 1;
	    } else if (flagholder == 'R') {
		/* Rear Mounted */
		isrear = 1;
	    } else if (flagholder == 'O') {
		/* One-Shot */
		isoneshot = 1;
	    }

	    /* 
	     * If it's a letter, it's not a crit location. If a
	     * player throws numbers in with the crit flags, then
	     * they'll see error messages about crit counts. Need
	     * to find a better way to fool-proof this.
	     */
	    if (isalpha(flagholder))
		    argc--;
	    
    } /* end for */
   
    /* Chop off the first the first two redundant args. */
    argc -= 2;

    weapnumcrits = GetWeaponCrits(mech, weapindex);
    
    /* Check to see if player gives enough crits and start adding if so. */
    if (argc < weapnumcrits) {
	notify_printf(player,
	    "Not enough critical slots specified! (Given: %i, Needed: %i)", 
	    argc, weapnumcrits);
    } else if (argc > weapnumcrits) {
	notify_printf(player,
	    "Too many critical slots specified! (Given: %i, Needed: %i)", 
	    argc, weapnumcrits);
    } else {
	for (loop = 0; loop < GetWeaponCrits(mech, weapindex); loop++) {
	    temp = atoi(args[2 + loop]);
	    temp--;		/* From 1 based to 0 based */
	    DOCHECK(temp < 0 ||
		temp > NUM_CRITICALS, "Bad critical location!");
	    MechSections(mech)[index].criticals[temp].type =
		(I2Weapon(weapindex));
	    MechSections(mech)[index].criticals[temp].firemode = 0;
	    MechSections(mech)[index].criticals[temp].ammomode = 0;

	    /* If this is a Rocket Launcher, use isrocket to set the OS flag */
	    if (MechWeapons[weapindex].special & ROCKET)
		isrocket = 1;

	    if (isrear)
		MechSections(mech)[index].criticals[temp].firemode |=
		    REAR_MOUNT;

	    if (istc)
		MechSections(mech)[index].criticals[temp].firemode |=
		    ON_TC;
	    
	    /* Rockets are OS too */
	    if (isoneshot || isrocket)
		MechSections(mech)[index].criticals[temp].firemode |=
		    OS_MODE;
	}
	if (IsAMS(weapindex)) {
	    if (MechWeapons[weapindex].special & CLAT)
		MechSpecials(mech) |= CL_ANTI_MISSILE_TECH;
	    else
		MechSpecials(mech) |= IS_ANTI_MISSILE_TECH;
	}
	notify_printf(player, "Weapon added.");
    }
} /* end mechrep_Raddweap() */

void mechrep_Rreload(dbref player, void *data, char *buffer)
{
    char *args[4];
    int argc;
    int index;
    int weapindex;
    int subsect;

    MECHREP_COMMON(1);
    argc = mech_parseattributes(buffer, args, 4);
    DOCHECK(argc <= 2, "Invalid number of arguments!");
    weapindex = WeaponIndexFromString(args[0]);
    if (weapindex == -1) {
	notify(player, "That is not a valid weapon!");
	DumpWeapons(player);
	return;
    }
    index =
	ArmorSectionFromString(MechType(mech), MechMove(mech), args[1]);
    if (index == -1) {
	notify(player, "Not a legal area. Must be HEAD, CTORSO");
	notify(player, "LTORSO, RTORSO, RLEG, LLEG, RARM, LARM");
	notify(player, "TURRET, ROTOR, RSIDE, LSIDE, FRONT, AFT");
	return;
    }
    subsect = atoi(args[2]);
    subsect--;			/* from 1 based to 0 based */
    DOCHECK(subsect < 0 ||
	subsect >= CritsInLoc(mech, index), "Critslot out of range!");
    if (MechWeapons[weapindex].ammoperton == 0)
	notify(player, "That weapon doesn't require ammo!");
    else {
	MechSections(mech)[index].criticals[subsect].type =
	    I2Ammo(weapindex);
	MechSections(mech)[index].criticals[subsect].firemode = 0;
	MechSections(mech)[index].criticals[subsect].ammomode = 0;

	if (argc > 3)
	    switch (toupper(args[3][0])) {
	    case 'W':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    SWARM_MODE;
		break;
	    case '1':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    SWARM1_MODE;
		break;
	    case 'I':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    INFERNO_MODE;
		break;
	    case 'L':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    LBX_MODE;
		break;
	    case 'A':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    ARTEMIS_MODE;
		break;
	    case 'N':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    NARC_MODE;
		break;
	    case 'C':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    CLUSTER_MODE;
		break;
	    case 'M':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    MINE_MODE;
		break;
	    case 'S':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    SMOKE_MODE;
		break;
	    case 'X':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    INARC_EXPLO_MODE;
		break;
	    case 'Y':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    INARC_HAYWIRE_MODE;
		break;
	    case 'E':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    INARC_ECM_MODE;
		break;
	    case 'R':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    AC_AP_MODE;
		break;
	    case 'F':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    AC_FLECHETTE_MODE;
		break;
	    case 'D':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    AC_INCENDIARY_MODE;
		break;
	    case 'P':
		MechSections(mech)[index].criticals[subsect].ammomode |=
		    AC_PRECISION_MODE;
		break;
            case 'T':
                MechSections(mech)[index].criticals[subsect].ammomode |=
                    STINGER_MODE;
                break;
	    }

	MechSections(mech)[index].criticals[subsect].data =
	    FullAmmo(mech, index, subsect);
	notify(player, "Weapon loaded!");
    }
}

void mechrep_Rrepair(dbref player, void *data, char *buffer)
{
    char *args[4];
    int argc;
    int index;
    int temp;

    MECHREP_COMMON(1);
    argc = mech_parseattributes(buffer, args, 4);
    DOCHECK(argc <= 2, "Invalid number of arguments!");
    index =
	ArmorSectionFromString(MechType(mech), MechMove(mech), args[0]);
    if (index == -1) {
	notify(player, "Not a legal area. Must be HEAD, CTORSO");
	notify(player, "LTORSO, RTORSO, RLEG, LLEG, RARM, LARM");
	notify(player, "TURRET, ROTOR, RSIDE, LSIDE, FRONT, AFT");
	return;
    }
    temp = atoi(args[2]);
    DOCHECK(temp < 0, "Illegal value for armor!");
    switch (args[1][0]) {
    case 'A':
    case 'a':
	/* armor */
	SetSectArmor(mech, index, temp);
	notify(player, "Armor repaired!");
	break;
    case 'I':
    case 'i':
	/* internal */
	SetSectInt(mech, index, temp);
	notify(player, "Internal structure repaired!");
	break;
    case 'C':
    case 'c':
	/* criticals */
	temp--;
	if (temp >= 0 && temp < NUM_CRITICALS) {
	    MechSections(mech)[index].criticals[temp].data = 0;
	    notify(player, "Critical location repaired!");
	} else {
	    notify(player, "Critical Location out of range!");
	}
	break;
    case 'R':
    case 'r':
	/* rear */
	if (index == CTORSO || index == LTORSO || index == RTORSO) {
	    SetSectRArmor(mech, index, temp);
	    notify(player, "Rear armor repaired!");
	} else {
	    notify(player,
		"Only the center, rear and left torso have rear armor!");
	}
	break;
    default:
	notify(player,
	    "Illegal Type-> must be ARMOR, INTERNAL, CRIT, REAR");
	return;
    }
}

/*
   ADDSP <ITEM> <LOCATION> <SUBSECT> [<DATA>]
 */
void mechrep_Raddspecial(dbref player, void *data, char *buffer)
{
    char *args[4];
    int argc;
    int index;
    int itemcode;
    int subsect;
    int newdata;
    int max;

    MECHREP_COMMON(1);
    argc = mech_parseattributes(buffer, args, 4);
    DOCHECK(argc <= 2, "Invalid number of arguments!");
    itemcode = FindSpecialItemCodeFromString(args[0]);
    if (itemcode == -1)
	if (strcasecmp(args[0], "empty")) {
	    notify(player, "That is not a valid special object!");
	    DumpMechSpecialObjects(player);
	    return;
	}
    index =
	ArmorSectionFromString(MechType(mech), MechMove(mech), args[1]);
    if (index == -1) {
	notify(player, "Not a legal area. Must be HEAD, CTORSO");
	notify(player, "LTORSO, RTORSO, RLEG, LLEG, RARM, LARM");
	notify(player, "TURRET, ROTOR, RSIDE, LSIDE, FRONT, AFT");
	return;
    }
    subsect = atoi(args[2]);
    subsect--;
    max = CritsInLoc(mech, index);
    DOCHECK(subsect < 0 || subsect >= max, "Critslot out of range!");
    if (argc == 4)
	newdata = atoi(args[3]);
    else
	newdata = 0;
    MechSections(mech)[index].criticals[subsect].type =
	itemcode < 0 ? 0 : I2Special(itemcode);
    MechSections(mech)[index].criticals[subsect].data = newdata;
    switch (itemcode) {
    case CASE:
	MechSections(mech)[(MechType(mech) ==
		CLASS_VEH_GROUND) ? BSIDE : index].config |= CASE_TECH;
	notify(player, "CASE Technology added to section.");
	break;
    case TRIPLE_STRENGTH_MYOMER:
	MechSpecials(mech) |= TRIPLE_MYOMER_TECH;
	notify(player,
	    "Triple Strength Myomer Technology added to 'Mech.");
	break;
    case MASC:
	MechSpecials(mech) |= MASC_TECH;
	notify(player,
	    "Myomer Accelerator Signal Circuitry added to 'Mech.");
	break;
    case C3_MASTER:
	MechSpecials(mech) |= C3_MASTER_TECH;
	notify(player, "C3 Command Unit added to 'Mech.");
	break;
    case C3_SLAVE:
	MechSpecials(mech) |= C3_SLAVE_TECH;
	notify(player, "C3 Slave Unit added to 'Mech.");
	break;
    case ARTEMIS_IV:
	MechSections(mech)[index].criticals[subsect].data--;
	MechSpecials(mech) |= ARTEMIS_IV_TECH;
	notify(player, "Artemis IV Fire-Control System added to 'Mech.");
	notify(player,
	    tprintf
	    ("System will control the weapon which starts at slot %d.",
		newdata));
	break;
    case ECM:
	MechSpecials(mech) |= ECM_TECH;
	notify(player, "Guardian ECM Suite added to 'Mech.");
	break;
    case ANGELECM:
	MechSpecials2(mech) |= ANGEL_ECM_TECH;
	notify(player, "Angel ECM Suite added to 'Mech.");
	break;
    case BEAGLE_PROBE:
	MechSpecials(mech) |= BEAGLE_PROBE_TECH;
	notify(player, "Beagle Active Probe added to 'Mech.");
	break;
    case TAG:
	MechSpecials2(mech) |= TAG_TECH;
	notify(player, "TAG added to 'Mech.");
	break;
    case C3I:
	MechSpecials2(mech) |= C3I_TECH;
	notify(player, "Improved C3 added to 'Mech.");
	break;
    case BLOODHOUND_PROBE:
	MechSpecials2(mech) |= BLOODHOUND_PROBE_TECH;
	notify(player, "Bloodhound Active Probe added to 'Mech.");
	break;
    }
    notify(player, "Critical slot filled.");
}

extern char *specials[];
extern char *specials2[];
extern char *infantry_specials[];

char *techstatus_func(MECH * mech)
{
    return (MechSpecials(mech) ||
	MechSpecials2(mech)) ? BuildBitString2(specials, specials2,
	MechSpecials(mech), MechSpecials2(mech)) : "";
}

void mechrep_Rshowtech(dbref player, void *data, char *buffer)
{
    int i;
    char *techstring;
    char location[20];

    MECHREP_COMMON(1);
    notify(player, "--------Advanced Technology--------");
    if (MechSpecials(mech) & TRIPLE_MYOMER_TECH)
	notify(player, "Triple Strength Myomer");
    if (MechSpecials(mech) & MASC_TECH)
	notify(player, "Myomer Accelerator Signal Circuitry");
    for (i = 0; i < NUM_SECTIONS; i++)
	if (MechSections(mech)[i].config & CASE_TECH) {
	    ArmorStringFromIndex(i, location, MechType(mech),
		MechMove(mech));
	    notify(player,
		tprintf("Cellular Ammunition Storage Equipment in %s",
		    location));
	}
    if (MechSpecials(mech) & CLAN_TECH) {
	notify(player, "Mech is set to Clan Tech.  This means:");
	notify(player, "    Mech automatically has Double Heat Sink Tech");
	notify(player, "    Mech automatically has CASE in all sections");
    }
    if (MechSpecials(mech) & DOUBLE_HEAT_TECH)
	notify(player, "Mech uses Double Heat Sinks");
    if (MechSpecials(mech) & CL_ANTI_MISSILE_TECH)
	notify(player, "Clan style Anti-Missile System");
    if (MechSpecials(mech) & IS_ANTI_MISSILE_TECH)
	notify(player, "Inner Sphere style Anti-Missile System");
    if (MechSpecials(mech) & FLIPABLE_ARMS)
	notify(player, "The arms may be flipped into the rear firing arc");
    if (MechSpecials(mech) & C3_MASTER_TECH)
	notify(player, "C3 Command Computer");
    if (MechSpecials(mech) & C3_SLAVE_TECH)
	notify(player, "C3 Slave Computer");
    if (MechSpecials(mech) & ARTEMIS_IV_TECH)
	notify(player, "Artemis IV Fire-Control System");
    if (MechSpecials(mech) & ECM_TECH)
	notify(player, "Guardian ECM Suite");
    if (MechSpecials2(mech) & ANGEL_ECM_TECH)
	notify(player, "Angel ECM Suite");
    if (MechSpecials(mech) & BEAGLE_PROBE_TECH)
	notify(player, "Beagle Active Probe");
    if (MechSpecials2(mech) & TAG_TECH)
	notify(player, "Target Aquisition Gear");
    if (MechSpecials2(mech) & C3I_TECH)
	notify(player, "Improved C3");
    if (MechSpecials2(mech) & BLOODHOUND_PROBE_TECH)
	notify(player, "Bloodhound Active Probe");
    if (MechSpecials(mech) & ICE_TECH)
	notify(player, "It has ICE engine");

    /* Infantry related stuff */
    if (MechInfantrySpecials(mech) & INF_SWARM_TECH)
	notify(player, "Can swarm enemy units");
    if (MechInfantrySpecials(mech) & INF_MOUNT_TECH)
	notify(player, "Can mount friendly units");
    if (MechInfantrySpecials(mech) & INF_ANTILEG_TECH)
	notify(player, "Can do anti-leg attacks");
    if (MechInfantrySpecials(mech) & CS_PURIFIER_STEALTH_TECH)
	notify(player, "Has CS Purifier Stealth");
    if (MechInfantrySpecials(mech) & DC_KAGE_STEALTH_TECH)
	notify(player, "Has DC Kage Stealth");
    if (MechInfantrySpecials(mech) & FWL_ACHILEUS_STEALTH_TECH)
	notify(player, "Has FWL Achileus Stealth");
    if (MechInfantrySpecials(mech) & FC_INFILTRATOR_STEALTH_TECH)
	notify(player, "Has FC Infiltrator Stealth");
    if (MechInfantrySpecials(mech) & FC_INFILTRATORII_STEALTH_TECH)
	notify(player, "Has FC InfiltratorII Stealth");
    if (MechInfantrySpecials(mech) & MUST_JETTISON_TECH)
	notify(player,
	    "Must jettison backpack before jumping/using specials");
    if (MechInfantrySpecials(mech) & CAN_JETTISON_TECH)
	notify(player, "Can jettison backpack");

    notify(player, "Brief version (May have something previous hadn't):");
    techstring = mechrep_gettechstring(mech);
    if (techstring && techstring[0])
        notify(player, techstring);
    else
    	notify(player, "-");
}

char * mechrep_gettechstring(MECH *mech)
{
    return BuildBitString3(specials, specials2, infantry_specials,
			   MechSpecials(mech), MechSpecials2(mech),
			   MechInfantrySpecials(mech));
}

void mechrep_Rdeltech(dbref player, void *data, char *buffer)
{
    int i, j;
    int Type;
    int nv, nv2;

    MECHREP_COMMON(1);
    /* Compare what the user gave to our specials lists */
    nv = BuildBitVector(specials, buffer);
    nv2 = BuildBitVector(specials2, buffer);

    /* Make sure what they gave was valid */
    if (((nv < 0) && (nv2 < 0)) && (strcasecmp(buffer, "all") != 0) && 
            (strcasecmp(buffer, "Case") != 0)) {
	    notify(player, "Invalid tech: Available techs:");
        notify(player, "\tAll");
        notify(player, "\tCase");

	    for (nv = 0; specials[nv]; nv++)
	        notify(player, tprintf("\t%s", specials[nv]));

	    for (nv = 0; specials2[nv]; nv++)
	        notify(player, tprintf("\t%s", specials2[nv]));

	    return;
    }

    /* Check to see if user specified anything */
    if (((!nv) && (!nv2)) && (strcasecmp(buffer, "all") != 0) && 
            (strcasecmp(buffer, "Case") != 0)) {
	    notify(player, "Nothing specified");
	    return;
    }

    /* Check to see if user specified 'ALL' */
    if (strcasecmp(buffer, "all") == 0) {

        for (i = 0; i < NUM_SECTIONS; i++) {

	        if ((MechSections(mech)[i].config & CASE_TECH)
	            || (MechSpecials(mech) & TRIPLE_MYOMER_TECH)
	            || (MechSpecials(mech) & MASC_TECH)) {
	            
                for (j = 0; j < NUM_CRITICALS; j++) {
		            Type = MechSections(mech)[i].criticals[j].type;

		            if (Type == I2Special((CASE))
		                || Type == I2Special((TRIPLE_STRENGTH_MYOMER))
		                || Type == I2Special((MASC))) {
		                MechSections(mech)[i].criticals[j].type = EMPTY;
                    }
                }
	            MechSections(mech)[i].config &= ~CASE_TECH;

            }
        }

        MechSpecials(mech) = 0;
        MechSpecials2(mech) = 0;
        notify(player, "All Advanced Technology Removed");
        return;
    }

    if (strcasecmp(buffer, "Case") == 0) {
        for (i = 0; i < NUM_SECTIONS; i++) {
	        if (MechSections(mech)[i].config & CASE_TECH) { 
                for (j = 0; j < NUM_CRITICALS; j++) {
	                Type = MechSections(mech)[i].criticals[j].type;

		            if (Type == I2Special((CASE))) {
		                MechSections(mech)[i].criticals[j].type = EMPTY;
                    }
                }
	            MechSections(mech)[i].config &= ~CASE_TECH;
            }
        }
        notify(player, "Case Technology Removed");
        return;
    }

    if (nv > 0) {

        if (strcasecmp(buffer, "TripleMyomerTech") == 0) {
            if (MechSpecials(mech) & TRIPLE_MYOMER_TECH) {
                for (i = 0; i < NUM_SECTIONS; i++) {
                    for (j = 0; j < NUM_CRITICALS; j++) {
		                Type = MechSections(mech)[i].criticals[j].type;

		                if (Type == I2Special((TRIPLE_STRENGTH_MYOMER))) {
		                    MechSections(mech)[i].criticals[j].type = EMPTY;
                        }
                    }
                }
            }
        } else if (strcasecmp(buffer, "Masc") == 0) {
            if (MechSpecials(mech) & MASC_TECH) {
                for (i = 0; i < NUM_SECTIONS; i++) {
                    for (j = 0; j < NUM_CRITICALS; j++) {
		                Type = MechSections(mech)[i].criticals[j].type;

		                if (Type == I2Special((MASC))) {
		                    MechSections(mech)[i].criticals[j].type = EMPTY;
                        }
                    }
                }
            }
        }

	    MechSpecials(mech) &= ~nv;
    	notify(player, tprintf("%s Technology Removed", buffer));
        
    } else {

	    MechSpecials2(mech) &= ~nv2;
	    notify(player, tprintf("%s Technology Removed", buffer));

    }
    return;
}

void mechrep_Raddtech(dbref player, void *data, char *buffer)
{
    int nv, nv2;

    MECHREP_COMMON(1);
    nv = BuildBitVector(specials, buffer);
    nv2 = BuildBitVector(specials2, buffer);

    if ((nv < 0) && (nv2 < 0)) {
	notify(player, "Invalid tech: Available techs:");

	for (nv = 0; specials[nv]; nv++)
	    notify(player, tprintf("\t%s", specials[nv]));

	for (nv = 0; specials2[nv]; nv++)
	    notify(player, tprintf("\t%s", specials2[nv]));

	return;
    }

    if ((!nv) && (!nv2)) {
	notify(player, "Nothing set!");
	return;
    }

    if (nv > 0) {
	MechSpecials(mech) |= nv;
	notify(player, tprintf("Set: %s", BuildBitString(specials, nv)));
    } else {
	MechSpecials2(mech) |= nv2;
	notify(player, tprintf("Set: %s", BuildBitString(specials2, nv2)));
    }

}

void mechrep_Rdelinftech(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;

    MechInfantrySpecials(mech) = 0;
    notify(player, "Advanced Infantry Technology Deleted");
}

void mechrep_Raddinftech(dbref player, void *data, char *buffer)
{
    int nv;

    MECHREP_COMMON(1);
    nv = BuildBitVector(infantry_specials, buffer);

    if (nv < 0) {
	notify(player, "Invalid infantry tech: Available techs:");

	for (nv = 0; infantry_specials[nv]; nv++)
	    notify(player, tprintf("\t%s", infantry_specials[nv]));
	return;
    }

    if (!nv) {
	notify(player, "Nothing set!");
	return;
    }

    if (nv > 0) {
	MechInfantrySpecials(mech) |= nv;
	notify(player, tprintf("Set: %s", BuildBitString(infantry_specials,
		    nv)));
    }

}

void mechrep_setcargospace(dbref player, void *data, char *buffer)
{
    char *args[2];
    int argc;
    int cargo;
    int max;

    MECHREP_COMMON(1);
    argc = mech_parseattributes(buffer, args, 2);
    DOCHECK(argc != 2, "Invalid number of arguements!");

    cargo = (atoi(args[0]) * 50);
    DOCHECK(cargo < 0 || cargo > 100000, "Doesn't that seem excessive?");
    CargoSpace(mech) = cargo;

    max = (atoi(args[1]));
    max = (BOUNDED(1,max,100));
    CarMaxTon(mech) = (char) max;

    notify(player, tprintf("%3.2f cargospace and %d tons of maxton space set.", (float) ((float) cargo / 100), (int) max));

}

MECH *load_refmech(char *reference)
{
    static MECH cachemech;
    static char cacheref[1024];

    if (!strcmp(cacheref, reference))
	return &cachemech;
    if (mech_loadnew(GOD, &cachemech, reference) < 1) {
	cacheref[0] = '\0';
	return NULL;
    }
    strncpy(cacheref, reference, 1023);
    cacheref[1023] = '\0';
    return &cachemech;
}
