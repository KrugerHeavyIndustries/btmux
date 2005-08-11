
/*
 * $Id: mech.custom.c,v 1.1.1.1 2005/01/11 21:18:14 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *       All rights reserved
 *
 * Created: Wed Feb 19 18:17:54 1997 fingon
 * Last modified: Sat Jun  6 22:22:23 1998 fingon
 *
 */

/* Mech-customization code */

#include <math.h>
#include "mech.h"
#include "mux_tree.h"
#include "coolmenu.h"
#include "mech.custom.h"
#include "mycool.h"
#include "glue.h"
#include "create.h"
#include "mech.partnames.h"
#include "help.h"
#include "p.mech.utils.h"
#include "p.mech.tech.h"
#include "p.mech.status.h"
#include "p.mech.consistency.h"
#include "p.mech.build.h"

dbref match_thing(dbref player, char *name);
void muxevent_remove_data(void *data);

/* Basically: 

   We keep a tree of our own with information about the 'mechs that
   people want, and update it with menu choices. When save/discard
   is chosen, the data is freed and, in case of save, the new model is saved
   into @MECHCUSTOM attribute
 */

/* @MECHCUSTOM syntax:
   - Each customization starts with string: 
   +#<ref>
   where <ref> is ref of the guy doing this customization.
   After that, without spaces, follow all the changes we need to do
   to a template to make new 'mech exactly like the original, as follows:
   C<num/num/num> Changes the critical at num/num to num
   A<num/value>   Changes the armor at num to value
   R<num/value>   Changes the rear armor at num to value
   (seperator ;)

   Example:
   +#9;C0/2/12;A1/20

   -After that, comes field ';-Accepted by Name<#ref>' if the
   modification has been approved. If none exist, the application
   has yet to be reviewed. Alternatively, it can have:
   ';*Rejected by Name<#ref>'

   Each custom-entry is seperated by a +, as well.
 */

coolmenu *semi_static_menu = NULL;
extern int global_silence;

#define CONVERT_TO(val,loc,pos,extra) \
loc = val % NUM_SECTIONS; \
pos = (val / NUM_SECTIONS - 1) % NUM_CRITICALS ; \
extra = ((val / NUM_SECTIONS - 1) / NUM_CRITICALS) - 1;

#define CALC_FROM(loc,pos,extra) \
(loc + NUM_SECTIONS * ( pos + 1 + (1 + extra) * NUM_CRITICALS))

#define CONVERT_FROM(val,loc,pos,extra) \
val = CALC_FROM(loc, pos, extra)

#define choiceg(text,id,w) \
CreateMenuEntry_Normal(&c, (char *) text, w|CM_TOGGLE|CM_NORIGHT, id, 1)

#define choice(text,id) \
choiceg(text, id, CM_ONE)

#define choice2(text,id) \
choiceg(text, id, CM_TWO)

#define toggle choice

#define numberg(text,id,value,max,w) \
CreateMenuEntry_Killer(&c, (char *) text, w|CM_NUMBER, id, value, max)

#define number(text,id,value,max) \
numberg(text,id,value,max,CM_ONE)

#define number2(text,id,value,max) \
numberg(text,id,value,max,CM_TWO)

#define number2_l(text,id,value,max) \
numberg(text,id,value,max,CM_TWO|CM_NORIGHT)

#define number2_r(text,id,value,max) \
numberg(text,id,value,max,CM_TWO|CM_NOTOG)


#define OkWeap(i,req,cant) \
(MechWeapons[i].type != THAND && \
 (!req || (MechWeapons[i].special & req)) && \
 (!cant || !(MechWeapons[i].special & cant)))

static void custom_info(coolmenu * c, char *str, MECH * mech, int loc,
    int pos)
{
    const char **locs =
	ProperSectionStringFromType(MechType(mech), MechMove(mech));

    cent(tprintf("%s for %s/%d:", str, locs[loc], pos + 1));
}

static void custom_do_weapon(coolmenu * c, MECH * mech, int loc, int pos,
    int req, int cant)
{
    int i;

    custom_info(c, "Weapon selection", mech, loc, pos);
    for (i = 0; MechWeapons[i].name; i++)
	if (OkWeap(i, req, cant) && !(MechWeapons[i].special & PCOMBAT))
	    choice2(MechWeapons[i].name, i);
}

static void custom_do_ammo(coolmenu * c, MECH * mech, int loc, int pos,
    int req, int cant)
{
    int i;

    custom_info(c, "Ammo selection", mech, loc, pos);
    for (i = 0; MechWeapons[i].name; i++)
	if (OkWeap(i, req, cant) && MechWeapons[i].ammoperton &&
	    !(MechWeapons[i].special & PCOMBAT))
	    choice2(MechWeapons[i].name, i);
}

static int ok_specials[][2] = {
    {HEAT_SINK, 0},
    {JUMP_JET, 0},
    {AXE, 0},
    {SWORD, 1},
    {MACE, 2},
    {CLAW, 2},
    {CASE, 1},
    {FERRO_FIBROUS, 1},
    {ENDO_STEEL, 1},
    {TRIPLE_STRENGTH_MYOMER, 1},
    {TARGETING_COMPUTER, 1},
    {MASC, 1},
    {C3_MASTER, 1},
    {C3_SLAVE, 1},
    {BEAGLE_PROBE, 1},
    {ARTEMIS_IV, 1},
    {STEALTH_ARMOR, 1},
    {NULL_SIGNATURE_SYSTEM, 1},
    {HVY_FERRO_FIBROUS, 1},
    {LT_FERRO_FIBROUS, 1},
    {C3I, 1},
    {BLOODHOUND_PROBE, 1},
    {-1, 0}
};


static void custom_do_special(coolmenu * c, MECH * mech, int loc, int pos,
    int spect)
{
    int i;

    custom_info(c, "Special selection", mech, loc, pos);
    for (i = 0; ok_specials[i][0] >= 0; i++)
	if (ok_specials[i][1] == spect)
	    choice(get_parts_long_name(Special(ok_specials[i][0]), 0),
		ok_specials[i][0]);
}

#define CHANGE_ARMOR  0		/* Change armor (location, amount) */
#define CHANGE_RARMOR 1		/* Change armor (location, amount) */
#define REMOVE_CRIT   2		/* Removes crit (location, position) */
#define REMOVE_WEAPON 3		/* Removes crit (location, position) */
#define DO_ADD_CRIT   4		/* Adds crit (location, position, type) */
#define DO_ADD_WEAPON 5		/* Adds weapon (location, position, type) */
#define REORIENT_WEAPON 6	/* Reorient the weapon (alter the mode) */
#define ALTER_AMMO    7		/* Changes the ammo type */

#define CHANGE_ERROR1 8		/* Inconsistent original template (location) */
#define CHANGE_ERROR2 9		/* Inconsistent target template (location) */

struct {
    char *text;
    int type;
} foo_table[] = {
    /*0 */  {
    "%s armor: %d points", 1},
	/*1 */  {
    "%s rear armor: %d points", 1},
	/*2 */  {
    "%s/%d (%s): Removed", 2},
	/*3 */  {
    "%s/%s (%s): Removed", 3},
	/*4 */  {
    "%s/%d (%s): Added", 2},
	/*5 */  {
    "%s/%s (%s): Added", 3},
	/*6 */  {
    "%s/%s (%s): Reoriented", 3},
	/*7 */  {
    "%s/%d (%s): Changed ammo type", 2},
	/*8 */  {
    "%s: %%ch%%crFROM wpn crit error%%cn!", 1},
	/*9 */  {
    "%s: %%ch%%crTO wpn crit error%%cn!", 1}
};

#define CHANGE_ARMOR_TIME  10
#define REMOVE_CRIT_TIME   60
#define REMOVE_WEAPON_TIME 90
#define ADD_CRIT_TIME      120
#define ADD_WEAPON_TIME    180
#define REORIENT_TIME      30
#define ALTER_AMMO_TIME    20

#define MAX_CHANGES 100
static int changes[MAX_CHANGES][6];
static int last_change = 0;

#define addchange(type, loc, pos, val, tons, time) \
{ \
if (last_change < 100) \
changes[last_change][0] = type; \
changes[last_change][1] = loc; \
changes[last_change][2] = pos; \
changes[last_change][3] = val; \
changes[last_change][4] = tons; \
changes[last_change++][5] = time; \
 }

int crit_weight(MECH * mech, int t)
{
    int cl;

    if (IsWeapon(t))
	return MechWeapons[Weapon2I(t)].weight * 1024 / 100 /
	    GetWeaponCrits(mech, Weapon2I(t));
    if (IsAmmo(t))
	return 512;
    if (!(IsSpecial(t)))
	return 1024;

    t = Special2I(t);
    cl = MechSpecials(mech) & CLAN_TECH;

    switch (t) {
    case HEAT_SINK:
	return 1024 / HS_Size(mech);
    case TARGETING_COMPUTER:
    case AXE:
    case MACE:
    case ARTEMIS_IV:
    case MASC:
    case C3_SLAVE:
    case TAG:
    case LAMEQUIP:
    	return 1024;
    case C3I:
	return 1280 * (MechType(mech) == CLASS_MECH ? 1 : 2);
    case ANGELECM:
    case BLOODHOUND_PROBE:
	return 1024 * (MechType(mech) == CLASS_MECH ? 1 : 2);
    case C3_MASTER:
	return 1024 * (MechType(mech) == CLASS_MECH ? 1 : 5);
    case SWORD:
        /* A Sword weighs 1/20th of the 'mech tonnage, rounded up to the half
           ton, and is 1/15th (rounded up to int) number of crits. */
	return (ceil(MechTons(mech) / 10.) * 512) / 
	    (ceil(MechTons(mech) / 15.));
    case BEAGLE_PROBE:
	return 1024 * 3 / (MechType(mech) == CLASS_MECH ? 4 : 2);
    case ECM:
    	/* IS ECM is 1.5 tons for 2 crits, Clan ECM 1 ton for 1 crit. */
	return 1024 * (cl ? 4 : MechType(mech) == CLASS_MECH ? 3 : 6) / 4;
    case CASE:
	return 512;
    case JUMP_JET:
	if (MechTons(mech) <= 55)
	    return 512;
	if (MechTons(mech) <= 85)
	    return 1024;
	return 2048;
    default:
    	return 0;
    }
}

#define CustomWeaponModes (REAR_MOUNT|ON_TC|OS_MODE|HALFTON_MODE)
#define CustomAmmoModes (ARTEMIS_MODE|NARC_MODE|INARC_MODES|SWARM_MODE|SWARM1_MODE|INFERNO_MODE|LBX_MODE)

int generate_change_list(MECH * from, MECH * to)
{
    int dif, i, j, k, noerror;
    int ft, cr, crl;
    unsigned char weaparray_f[MAX_WEAPS_SECTION];
    unsigned char weapdata_f[MAX_WEAPS_SECTION];
    int critical_f[MAX_WEAPS_SECTION];

    unsigned char weaparray_t[MAX_WEAPS_SECTION];
    unsigned char weapdata_t[MAX_WEAPS_SECTION];
    int critical_t[MAX_WEAPS_SECTION];
    int num_weaps_f, num_weaps_t, found;

    last_change = 0;
    for (i = 0; i < NUM_SECTIONS; i++) {
	if ((dif = (GetSectOArmor(to, i) - GetSectOArmor(from, i))))
	    addchange(CHANGE_ARMOR, i, 0, dif, dif * 64,
		abs(dif) * CHANGE_ARMOR_TIME);
	if ((dif = (GetSectORArmor(to, i) - GetSectORArmor(from, i))))
	    addchange(CHANGE_RARMOR, i, 0, dif, dif * 64,
		abs(dif) * CHANGE_ARMOR_TIME);
    }
    global_silence = 1;
    for (i = 0; i < NUM_SECTIONS; i++) {
	noerror = 1;
	crl = CritsInLoc(from, i);
	if ((num_weaps_f =
		FindWeapons_Advanced(from, i, weaparray_f, weapdata_f,
		    critical_f, 1)) < 0) {
	    addchange(CHANGE_ERROR1, i, 0, 0, 0, 0);
	    continue;
	}
	if ((num_weaps_t =
		FindWeapons_Advanced(to, i, weaparray_t, weapdata_t,
		    critical_t, 0)) < 0) {
	    addchange(CHANGE_ERROR2, i, 0, 0, 0, 0);
	    continue;
	}
	for (j = 0; j < crl; j++) {
	    if (!(ft = (GetPartType(from, i, j))))
		continue;
	    if (GetPartType(to, i, j) == ft) {
		if (IsAmmo(ft))
		    if ((GetPartAmmoMode(from, i,
				j) & CustomAmmoModes) !=
			(GetPartAmmoMode(to, i, j) & CustomAmmoModes))
			addchange(ALTER_AMMO, i, j, 0,
			    (GetPartFireMode(from, i, j) & HALFTON_MODE)
			    ? 512 : 0 - (GetPartFireMode(to, i,
				    j) & HALFTON_MODE) ? 512 : 0,
			    ALTER_AMMO_TIME);
		continue;
	    }
	    if (IsWeapon(ft))
		continue;
	    addchange(REMOVE_CRIT, i, j, 0, AmmoMod(from, i,
		    j) * (0 - crit_weight(from, ft)), REMOVE_CRIT_TIME);
	}
	for (j = 0; j < num_weaps_f; j++) {
	    found = 0;
	    for (k = 0; k < num_weaps_t; k++)
		if (weaparray_f[j] == weaparray_t[k] &&
		    critical_f[j] == critical_t[k]
		    && (!mudconf.btech_parts ||
			GetPartBrand(from, i,
			    critical_f[j]) == GetPartBrand(to, i,
			    critical_t[k])))
		    found = 1;
	    if (found) {
		if ((GetPartFireMode(from, i,
			    critical_f[j]) & CustomWeaponModes) !=
		    (GetPartFireMode(to, i,
			    critical_f[j]) & CustomWeaponModes)) {
		    cr = GetWeaponCrits(from, weaparray_f[j]);
		    /* Otherwise reorient the weapon */
		    addchange(REORIENT_WEAPON, i, critical_f[j],
			weaparray_f[j], 0, REORIENT_TIME * cr);
		}
		continue;
	    }
	    cr = GetWeaponCrits(from, weaparray_f[j]);
	    addchange(REMOVE_WEAPON, i, critical_f[j], weaparray_f[j],
		0 - MechWeapons[weaparray_f[j]].weight * 1024 / 100,
		REMOVE_WEAPON_TIME * cr);
	}

	for (j = 0; j < crl; j++) {
	    if (!(ft = (GetPartType(to, i, j))))
		continue;
	    if (IsWeapon(ft))
		continue;
	    if (GetPartType(from, i, j) == ft)
		continue;
	    addchange(DO_ADD_CRIT, i, j, ft, AmmoMod(to, i,
		    j) * crit_weight(to, ft), ADD_CRIT_TIME);
	}
	for (j = 0; j < num_weaps_t; j++) {
	    found = 0;
	    for (k = 0; k < num_weaps_f; k++)
		if (weaparray_f[k] == weaparray_t[j] &&
		    critical_f[k] == critical_t[j]
		    && (!mudconf.btech_parts ||
			GetPartBrand(from, i,
			    critical_f[k]) == GetPartBrand(to, i,
			    critical_t[j])))
		    found = 1;
	    if (found)
		continue;
	    cr = GetWeaponCrits(to, weaparray_t[j]);
	    addchange(DO_ADD_WEAPON, i, critical_t[j], weaparray_t[j],
		MechWeapons[weaparray_t[j]].weight * 1024 / 100,
		ADD_WEAPON_TIME * cr);
	}
    }
    global_silence = 0;
    return last_change;
}

static void display_change_list(coolmenu * c, MECH * from, MECH * mech)
{
    int i;
    int total_tons = 0;
    int total_time = 0;
    int f, t;
    int warn = 0;
    char buf[MBUF_SIZE];
    char *str;

    for (i = 0; i < last_change; i++) {
	str =
	    ShortArmorSectionString(MechType(mech), MechMove(mech),
	    changes[i][1]);
	switch (foo_table[changes[i][0]].type) {
	case 1:
	    sprintf(buf, foo_table[changes[i][0]].text, str,
		changes[i][3]);
	    break;
	case 2:
	    sprintf(buf, foo_table[changes[i][0]].text, str,
		changes[i][2] + 1,
		pos_part_name(changes[i][0] == REMOVE_CRIT ? from : mech,
		    changes[i][1], changes[i][2]));
	    if (changes[i][0] == REMOVE_CRIT &&
		!PartIsDestroyed(from, changes[i][1], changes[i][2])) {
		strcat(buf, "(*)");
		warn++;
	    }
	    break;
	case 3:
	    f = changes[i][2] + 1;
	    t = GetWeaponCrits(mech, changes[i][3]) + f - 1;
	    sprintf(buf, foo_table[changes[i][0]].text, str,
		t == f ? tprintf("%d", f) : tprintf("%d-%d", f, t),
		pos_part_name(changes[i][0] == REMOVE_WEAPON ? from : mech,
		    changes[i][1], changes[i][2]));
	    if (changes[i][0] == REMOVE_WEAPON &&
		!PartIsDestroyed(from, changes[i][1], changes[i][2])) {
		strcat(buf, "(*)");
		warn++;
	    }
	    break;
	default:
	    strcpy(buf, "---- BUG ----");
	}
	addmenu(buf);
	if (changes[i][5]) {
	    sprintf(buf, "%.1f tons / %d mins", changes[i][4] / 1024.0,
		changes[i][5]);
	    total_time += changes[i][5];
	    total_tons += changes[i][4];
	} else
	    strcpy(buf, " ");
	addmenu(buf);
    }
    if (last_change > 1) {
	addmenu("Total");
	sprintf(buf, "%.1f tons / %d mins", total_tons / 1024.0,
	    total_time);
	addmenu(buf);
    }
    if (warn) {
	cent("WARNING: Parts marked with (*) haven't been removed yet.");
	cent("If you do not want to lose parts, remove them first with the");
	cent("tech commands.");
    }
}

static void custom_do_main(coolmenu * c, MECH * new, dbref player)
{
    MECH *mech = FindObjectsData(new->mynum);

    if (generate_change_list(mech, new)) {
	display_change_list(c, mech, new);
	addline();
    }
    choice2("Alter criticals", ALTER_CRIT);
    choice2("Alter armor", ALTER_ARMOR);
    choice2("Discard changes", DISCARD_CHANGES);
    choice2("Apply for approval", APPLY_FOR_APPROVAL);
    if (Wiz(player))
	choice2("Do it", DO_IT);
}

static char *ammo_text(int mode)
{
    if (mode & LBX_MODE)
	return "LBX";
    if (mode & ARTEMIS_MODE)
	return "ArtemisIV";
    if (mode & NARC_MODE)
	return "NARC";
    if (mode & INARC_EXPLO_MODE)
	return "Explosive";
    if (mode & INARC_HAYWIRE_MODE)
	return "Haywire";
    if (mode & INARC_ECM_MODE)
	return "ECM";
    if (mode & INARC_NEMESIS_MODE)
	return "Nemesis";
    if (mode & SWARM_MODE)
	return "Swarm";
    if (mode & SWARM1_MODE)
	return "Swarm1";
    if (mode & INFERNO_MODE)
	return "Inferno";
    if (mode & AC_AP_MODE)
	return "ArmorPiercing";
    if (mode & AC_FLECHETTE_MODE)
	return "Flechette";
    if (mode & AC_INCENDIARY_MODE)
	return "Incendiary";
    if (mode & AC_PRECISION_MODE)
	return "Precision";

    return "Normal";
}

/* Flag / No-weapon-flag / Weapon-flag / Type */

int ammo_mode_list[][4] = {
    {LBX_MODE, 0, LBX, TAMMO},
    {ARTEMIS_MODE, 0, 0, TMISSILE},
    {NARC_MODE, 0, 0, TMISSILE},
    {INARC_EXPLO_MODE, 0, 0, TMISSILE},
    {INARC_HAYWIRE_MODE, 0, 0, TMISSILE},
    {INARC_ECM_MODE, 0, 0, TMISSILE},
    {INARC_NEMESIS_MODE, 0, 0, TMISSILE},
    {SWARM_MODE, DAR, IDF, TMISSILE},
    {SWARM1_MODE, DAR, IDF, TMISSILE},
    {INFERNO_MODE, IDF | DAR, 0, TMISSILE},
    {0, 0, 0, -1}
};

void ToggleAmmo(dbref player, MECH * mech, int loc, int pos)
{
    int idx = Ammo2I(GetPartType(mech, loc, pos));
    int m = GetPartAmmoMode(mech, loc, pos);
    int i, j = 0, prev = 0;

#define TB(v) ToggleBit(GetPartAmmoMode(mech, loc, pos), v)
    for (i = 0; ammo_mode_list[i][0]; i++)
	if ((m & ammo_mode_list[i][0]) && (ammo_mode_list[j][3] < 0 ||
		(MechWeapons[idx].type == ammo_mode_list[j][3])))
	    break;
    if (ammo_mode_list[i][0]) {
	/* _find_ mode to set */
	TB(ammo_mode_list[i][0]);
	for (j = (i + 1); ammo_mode_list[j][0]; j++)
	    if ((ammo_mode_list[j][3] < 0 ||
		    (MechWeapons[idx].type == ammo_mode_list[j][3])) &&
		(ammo_mode_list[j][1] == 0 ||
		    (!(MechWeapons[idx].special & ammo_mode_list[j][1])))
		&& (ammo_mode_list[j][2] == 0 ||
		    (MechWeapons[idx].special & ammo_mode_list[j][2])))
		break;
	if (ammo_mode_list[j][0]) {
	    TB(ammo_mode_list[i][0]);
	    return;
	}
	prev = 1;
    }
    for (j = 0; j < i; j++)
	if ((ammo_mode_list[j][3] < 0 ||
		(MechWeapons[idx].type == ammo_mode_list[j][3])) &&
	    (ammo_mode_list[j][1] == 0 ||
		(!(MechWeapons[idx].special & ammo_mode_list[j][1]))) &&
	    (ammo_mode_list[j][2] == 0 ||
		(MechWeapons[idx].special & ammo_mode_list[j][2])))
	    break;
    if (j == i && !prev) {
	notify(player, "Error: That ammo type cannot be changed.");
	return;
    }
    TB(ammo_mode_list[j][0]);
}

static int custom_flag;

static void custom_do_edit(coolmenu * c, dbref player, MECH * mech,
    int loc, int pos)
{
    if (GetPartType(mech, loc, pos)) {
	choice("Remove critical", REMOVE);
	if (IsWeapon(GetPartType(mech, loc, pos))) {
	    if (MechType(mech) == CLASS_MECH && (loc == CTORSO ||
		    loc == RTORSO || loc == LTORSO))
		choice(tprintf("Toggle rear (now: %s)",
			GetPartFireMode(mech, loc,
			    pos) & REAR_MOUNT ? "Rear" : "Front"),
		    TOGGLE_REAR);
	    if (Wiz(player) || (custom_flag & 8))
		choice(tprintf("Toggle One-Shot (now: %s)",
			GetPartFireMode(mech, loc,
			    pos) & OS_MODE ? "One-shot" : "Multi-shot"),
		    TOGGLE_OS);
	    if (Wiz(player) || (custom_flag & 4))
		choice(tprintf("Toggle TC (now: %s)", GetPartFireMode(mech,
			    loc, pos) & ON_TC ? "On" : "Off"), TOGGLE_TC);
	} else if (IsAmmo(GetPartType(mech, loc, pos))) {
	    choice(tprintf("Change the ammo type (now: %s)",
		    ammo_text(GetPartAmmoMode(mech, loc, pos))),
		TOGGLE_AMMO);
	    if (Wiz(player) || (custom_flag & 16))
		choice(tprintf("Toggle ammo size (full/half - now: %s)",
			GetPartFireMode(mech, loc,
			    pos) & HALFTON_MODE ? "half" : " full"),
		    TOGGLE_HALFAMMO);
	}
    } else {
	choice("Add weapon (oldtech)", ADD_WEAPON);
	if (Wiz(player) || (custom_flag & 1))
	    choice("Add weapon (newtech)", ADD_NWEAPON);
	if (Wiz(player) || (custom_flag & 2))
	    choice("Add weapon (clantech)", ADD_CWEAPON);
	choice("Add ammo (oldtech)", ADD_AMMO);
	if (Wiz(player) || (custom_flag & 1))
	    choice("Add ammo (newtech)", ADD_NAMMO);
	if (Wiz(player) || (custom_flag & 2))
	    choice("Add ammo (clantech)", ADD_CAMMO);
	choice("Add special (oldtech)", ADD_SPECIAL);
	if (Wiz(player) || (custom_flag & 32))
	    choice("Add special (clan/newtech)", ADD_NSPECIAL);
    }
}


static void custom_do_limb(coolmenu * c, MECH * mech)
{
    int i;
    const char **locs =
	ProperSectionStringFromType(MechType(mech), MechMove(mech));

    vsi("%cgSelect location to edit%cn");
    addline();
    for (i = 0; locs[i]; i++)
	if (GetSectOInt(mech, i))
	    choice(locs[i], i);
}

#define SectMax(mech,s) \
(MechType(mech) != CLASS_MECH ? \
((MechType(mech) == CLASS_VTOL && s == ROTOR) ? 2 : 99) : \
(s == HEAD ? 9 : \
(GetSectOInt(mech, i) * 2)))

static void custom_do_armor(coolmenu * c, MECH * new)
{
    int i;
    const char **locs =
	ProperSectionStringFromType(MechType(new), MechMove(new));
    char buf[MBUF_SIZE];
    MECH *mech = FindObjectsData(new->mynum);

    for (i = 0; locs[i]; i++) {
	if (!GetSectOInt(mech, i))
	    continue;
	number2_l(locs[i], i, GetSectOArmor(new, i), SectMax(new,
		i) - GetSectORArmor(new, i));
	sprintf(buf, "%24s%s", " ", GetSectOArmor(mech,
		i) == GetSectOArmor(new,
		i) ? "" : tprintf("%%ch%3d%%cn   ->  ", GetSectOArmor(mech,
		    i)));
	number2_r(buf, i, GetSectOArmor(new, i), 0);
    }
    for (i = 0; locs[i]; i++) {
	if (!GetSectOInt(mech, i))
	    continue;
	if (!GetSectORArmor(mech, i))
	    continue;
	number2_l(tprintf("Rear: %s", locs[i]), i + 8, GetSectORArmor(new,
		i), SectMax(new, i) - GetSectOArmor(new, i));
	sprintf(buf, "%24s%s", " ", GetSectORArmor(mech,
		i) == GetSectORArmor(new,
		i) ? "" : tprintf("%%ch%3d%%cn   ->  ",
		GetSectORArmor(mech, i)));
	number2_r(buf, i + 8, GetSectORArmor(new, i), 0);
    }
}

static void custom_do_crit(coolmenu * c, MECH * new, int loc)
{
    int i, j, t, ok;
    const char **locs =
	ProperSectionStringFromType(MechType(new), MechMove(new));

    vsi(tprintf("%%cgSelect critical to edit in %s%%cn", locs[loc]));
    addline();
    for (i = 0; i < CritsInLoc(new, loc); i++) {
	ok = 1;
	if (IsSpecial((t = GetPartType(new, loc, i)))) {
	    ok = 0;
	    for (j = 0; ok_specials[j][0] >= 0; j++)
		if (Special(ok_specials[j][0]) == t)
		    ok = 1;
	}
	if (!ok)
	    choiceg(tprintf("%2d - %s", i + 1, pos_part_name(new, loc, i)),
		i, CM_ONE | CM_NOTOG);
	else
	    toggle(tprintf("%2d - %s", i + 1, pos_part_name(new, loc, i)),
		i);
    }
}

static void custom_do_brand(coolmenu * c, MECH * mech, int loc, int pos,
    int type)
{
    int i;

    for (i = 1; i <= 5; i++)
	toggle(get_parts_long_name(type, i), i);
}


static coolmenu *custom_menu(dbref player)
{
    coolmenu *c = NULL;
    CUSTOM *cu;
    int loc, pos, s;
    MECH *mech;

    if (semi_static_menu) {
	KillCoolMenu(semi_static_menu);
	semi_static_menu = NULL;
    }
    cu = FindObjectsData(Location(player));
    if (!(!cu || cu->user < 0 || (!Wiz(player) && cu->user != player))) {	/* First criteria : Non-custom place / non-init'ed custom place ->
										   no menu */
	mech = &cu->new;
	addline();
	if (cu->user != player) {
	    cent(tprintf("%s customization in progress (by %s(#%d))",
		    GetMechID(mech), Name(cu->user), cu->user));
	    addline();
	}
	switch (cu->state) {
	case STATE_MAIN:
	    custom_do_main(c, mech, player);
	    break;
	case STATE_LIMB:
	    custom_do_limb(c, mech);
	    break;
	case STATE_ARMOR:
	    custom_do_armor(c, mech);
	    break;
	default:
	    if (cu->state <= NUM_SECTIONS)
		custom_do_crit(c, mech, cu->state - 1);
	    else {
		CONVERT_TO(cu->state, loc, pos, s);
		switch (s) {
		case 0:
		    custom_do_edit(c, player, mech, loc, pos);
		    break;

		case 1:
		    custom_do_weapon(c, mech, loc, pos, 0, CLAT);
		    break;
		case 2:
		    custom_do_ammo(c, mech, loc, pos, 0, CLAT);
		    break;
		case 4:
		    custom_do_weapon(c, mech, loc, pos, 0, 0);
		    break;
		case 8:
		    custom_do_ammo(c, mech, loc, pos, 0, 0);
		    break;
		case 16:
		    custom_do_weapon(c, mech, loc, pos, CLAT, 0);
		    break;
		case 32:
		    custom_do_ammo(c, mech, loc, pos, CLAT, 0);
		    break;
		case 64:
		    custom_do_special(c, mech, loc, pos, 0);
		    break;
		case 128:
		    custom_do_special(c, mech, loc, pos, 1);
		    break;
		default:
		    custom_do_brand(c, mech, loc, pos,
			s / FIRST_UNUSED_BIT);
		    break;
		}
	    }
	}
	addline();
	if (cu->state)
	    addmenu("z - Back");
	addmenu("HELP | EDIT <ref> | FINISH");
	addline();
    }
    semi_static_menu = c;
    return c;
}

static void show_custom(CUSTOM * cu, dbref player)
{
    coolmenu *c;

    custom_flag = cu->allow;
    c = custom_menu(player);

    if (c)
	ShowCoolMenu(player, c);
}

static void advance_state(CUSTOM * cu, dbref player, int state)
{
    cu->state = state;
    show_custom(cu, player);
}

void custom_edit(dbref player, void *data, char *buffer)
{
    CUSTOM *cu = (CUSTOM *) data;
    dbref it;
    MECH *mech;

    /* This should do a lot of magic */
    DOCHECK(cu->user >= 0, "'finish' the previous modification(s) first.");
    DOCHECK(!Wiz(player) && cu->user != player &&
	cu->user >= 0, "This booth is in use by another guy!");
    DOCHECK((it = match_thing(player, buffer)) <= 0, "Invalid target!");
    DOCHECK(!Wiz(player) &&
	Location(it) != Location(cu->mynum), "Invalid target!");
    DOCHECK(!(mech = FindObjectsData(it)), "Invalid target!");
    DOCHECK(figure_latest_tech_event(mech) > 0 &&
	!Wiz(player), "That 'Mech is still under repairs!");
    cu->user = player;
    cu->state = STATE_MAIN;
    cu->submit = -1;
    memcpy(&cu->new, mech, sizeof(MECH));
    show_custom(cu, player);
}

void custom_finish(dbref player, void *data, char *buffer)
{
    CUSTOM *cu = (CUSTOM *) data;

    DOCHECK(cu->user < 0,
	"You have to start the booth with 'EDIT #<num>'");
    DOCHECK(!Wiz(player) && cu->user != player &&
	Location(cu->user) == cu->mynum &&
	Connected(cu->user), "This booth is in use by another guy!");
    if (!buffer && Wiz(player))
	notify(player, "All changes have been saved and booth freed.");
    else
	notify(player,
	    "All changes have been *dumped* and booth is free for anyone to use again.");
    cu->user = -1;
}

void custom_back(dbref player, void *data, char *buffer)
{
    CUSTOM *cu = (CUSTOM *) data;
    int loc, pos, s;

    DOCHECK(cu->user < 0,
	"You have to start the booth with 'EDIT #<num>'");
    DOCHECK(!Wiz(player) &&
	cu->user != player, "This booth is in use by another guy!");
    DOCHECK(cu->state == STATE_MAIN, "There is no going back!");
    switch (cu->state) {
    case STATE_LIMB:
    case STATE_ARMOR:
	advance_state(cu, player, STATE_MAIN);
	break;
    default:
	if (cu->state <= NUM_SECTIONS)
	    advance_state(cu, player, STATE_LIMB);
	else {
	    CONVERT_TO(cu->state, loc, pos, s);
	    switch (s) {
	    case 0:
		advance_state(cu, player, loc + 1);
		break;
	    case 1:
	    case 2:
	    case 4:
	    case 8:
	    case 16:
	    case 32:
	    case 64:
	    case 128:
		advance_state(cu, player, CALC_FROM(loc, pos, 0));
		break;
	    default:
		advance_state(cu, player, CALC_FROM(loc, pos, 1));
		break;
	    }
	}
    }
}

void custom_look(dbref player, void *data, char *buffer)
{
    CUSTOM *cu = (CUSTOM *) data;

    DOCHECK(cu->user < 0,
	"You have to start the booth with 'EDIT #<num>'");
    DOCHECK(!Wiz(player) &&
	cu->user != player, "This booth is in use by another guy!");
    show_custom(cu, player);
}

#define to_be_done(player) \
notify(player, "This function isn't implemented yet!");

void custom_help(dbref player, void *data, char *buffer)
{
    char buf[MBUF_SIZE];

    strcpy(buf, "custom");
    help_write(player, buf, &mudstate.news_htab, mudconf.news_file, 0);
}


#define IsError(i) (changes[i][0] == CHANGE_ERROR2)

static int do_changes(CUSTOM * cu, dbref player, MECH * from, MECH * to)
{
    int i, j, k, loc, pos;
    int total_tons = 0;
    int total_time = 0;
    char buf[MBUF_SIZE];

    if (!generate_change_list(from, to)) {
	notify(player, "There are no changes to make!");
	return 0;
    }
    for (i = 0; i < last_change; i++)
	if (IsError(i)) {
	    notify(player, "Inconsistency detected ; no changes made.");
	    return 0;
	}
    for (i = 0; i < last_change; i++) {
	loc = changes[i][1];
	pos = changes[i][2];
	switch (changes[i][0]) {
	case CHANGE_ARMOR:
	    SetSectOArmor(from, loc, (k = GetSectOArmor(to, loc)));
	    if (GetSectArmor(from, loc) > k)
		SetSectArmor(from, loc, k);
	    if (Wiz(cu->user))
		SetSectArmor(from, loc, k);
	    break;
	case CHANGE_RARMOR:
	    SetSectORArmor(from, loc, (k = GetSectORArmor(to, loc)));
	    if (GetSectRArmor(from, loc) > k)
		SetSectRArmor(from, loc, k);
	    if (Wiz(cu->user))
		SetSectRArmor(from, loc, k);
	    break;
	case REMOVE_CRIT:
	    SetPartType(from, loc, pos, 0);
	    SetPartFireMode(from, loc, pos, 0);
	    SetPartAmmoMode(from, loc, pos, 0);
	    SetPartBrand(from, loc, pos, 0);
	    SetPartData(from, loc, pos, 0);
	    break;
	case REMOVE_WEAPON:
	    for (j = pos; j < (pos + GetWeaponCrits(from, changes[i][3]));
		j++) {
		SetPartType(from, loc, j, 0);
		SetPartFireMode(from, loc, j, 0);
		SetPartAmmoMode(from, loc, j, 0);
		SetPartBrand(from, loc, j, 0);
		SetPartData(from, loc, j, 0);
	    }
	    break;
	case DO_ADD_CRIT:
	    SetPartType(from, loc, pos, GetPartType(to, loc, pos));
	    SetPartBrand(from, loc, pos, GetPartBrand(to, loc, pos));
	    SetPartFireMode(from, loc, pos, GetPartFireMode(to, loc, pos));
	    SetPartAmmoMode(from, loc, pos, GetPartAmmoMode(to, loc, pos));
	    SetPartData(from, loc, pos, GetPartData(to, loc, pos));
	    if (!Wiz(cu->user))
		DestroyPart(from, loc, pos);
	    break;
	case ALTER_AMMO:
	    GetPartAmmoMode(from, loc, pos) &= CustomAmmoModes;
	    GetPartAmmoMode(from, loc, pos) |=
		(GetPartAmmoMode(to, loc, pos) & CustomAmmoModes);
	    GetPartData(from, loc, pos) = 0;
	    break;
	case REORIENT_WEAPON:
	    for (j = pos; j < (pos + GetWeaponCrits(from, changes[i][3]));
		j++) {
		GetPartFireMode(from, loc, j) &= CustomWeaponModes;
		GetPartFireMode(from, loc, j) |=
		    (GetPartFireMode(to, loc, pos) & CustomWeaponModes);
	    }
	    break;
	case DO_ADD_WEAPON:
	    for (j = pos; j < (pos + GetWeaponCrits(from, changes[i][3]));
		j++) {
		SetPartType(from, loc, j, GetPartType(to, loc, j));
		SetPartBrand(from, loc, j, GetPartBrand(to, loc, j));
		SetPartFireMode(from, loc, j, GetPartFireMode(to, loc, j));
		SetPartAmmoMode(from, loc, j, GetPartAmmoMode(to, loc, j));
		SetPartData(from, loc, j, GetPartData(to, loc, j));
		if (!Wiz(cu->user))
		    DestroyPart(from, loc, j);
	    }
	    break;
	}
	total_time += changes[i][5];
	total_tons += changes[i][4];
    }
    if (!Wiz(cu->user))
	tech_addtechtime(cu->user, total_time);
    strcpy(buf, Name(player));
    SendCustom(tprintf("%s(#%d)'s request was granted by %s(#%d)",
	    Name(cu->user), cu->user, buf, player));

/*   silly_atr_set(cu->mynum, A_MECHCUSTOM, tprintf("Approved by %s(#%d)", Name(player), player)); */
    do_magic(from);
    muxevent_remove_data((void *) from);
    return last_change;
}

static void custom_act_main(CUSTOM * cu, MECH * mech, dbref player,
    int choice)
{
    switch (choice) {
    case ALTER_CRIT:
	advance_state(cu, player, STATE_LIMB);
	return;
    case ALTER_ARMOR:
	advance_state(cu, player, STATE_ARMOR);
	return;
    case DISCARD_CHANGES:
	custom_finish(player, cu, "");
	return;
    case APPLY_FOR_APPROVAL:
	DOCHECK(Wiz(player), "You can just DO IT! Duh.");
	if (cu->submit >= 0)
	    notify(player,
		"You have already submitted a request. Please stand by.");
	else {
	    notify(player,
		"An approval request has been submitted. There is NO guaranteed response,");
	    notify(player,
		"but if the design is reasonably IC and not munch, chances are that it might");
	    notify(player,
		"even get through. Thanks for your co-operation.");
	    SendCustom(tprintf
		("%s(#%d) requests customization approval in #%d for #%d.",
		    Name(player), player, cu->mynum, mech->mynum));
	    cu->submit = 0;
	}
	return;
    case DO_IT:
	if (do_changes(cu, player, FindObjectsData(mech->mynum), mech))
	    custom_finish(player, cu, NULL);
    }
}

static void custom_act_ammo(CUSTOM * cu, MECH * mech, dbref player,
    int loc, int pos, int choice)
{
    SetPartType(mech, loc, pos, I2Ammo(choice));
    SetPartFireMode(mech, loc, pos, 0);
    SetPartAmmoMode(mech, loc, pos, 0);
    SetPartData(mech, loc, pos, FullAmmo(mech, loc, pos));
    SetPartBrand(mech, loc, pos, 0);
    advance_state(cu, player, loc + 1);
}

static void custom_act_brand(CUSTOM * cu, MECH * mech, dbref player,
    int loc, int pos, int weapindx, int choice)
{
    SetPartType(mech, loc, pos, weapindx);
    SetPartData(mech, loc, pos, 0);
    SetPartFireMode(mech, loc, pos, 0);
    SetPartAmmoMode(mech, loc, pos, 0);
    SetPartBrand(mech, loc, pos, choice);
    advance_state(cu, player, loc + 1);
}

static void custom_act_weapon(CUSTOM * cu, MECH * mech, dbref player,
    int loc, int pos, int choice)
{
    advance_state(cu, player, CALC_FROM(loc, pos,
	    FIRST_UNUSED_BIT * (choice + 1)));
}

static void custom_act_special(CUSTOM * cu, MECH * mech, dbref player,
    int loc, int pos, int choice)
{
    SetPartType(mech, loc, pos, Special(choice));
    SetPartData(mech, loc, pos, 0);
    SetPartFireMode(mech, loc, pos, 0);
    SetPartAmmoMode(mech, loc, pos, 0);
    SetPartBrand(mech, loc, pos, 0);
    advance_state(cu, player, loc + 1);
}

static void custom_act_edit(CUSTOM * cu, MECH * mech, dbref player,
    int loc, int pos, int choice)
{
    switch (choice) {
    case ADD_WEAPON:
	advance_state(cu, player, CALC_FROM(loc, pos, 1));
	return;
    case ADD_AMMO:
	advance_state(cu, player, CALC_FROM(loc, pos, 2));
	return;
    case ADD_NWEAPON:
	advance_state(cu, player, CALC_FROM(loc, pos, 4));
	return;
    case ADD_NAMMO:
	advance_state(cu, player, CALC_FROM(loc, pos, 8));
	return;
    case ADD_CWEAPON:
	advance_state(cu, player, CALC_FROM(loc, pos, 16));
	return;
    case ADD_CAMMO:
	advance_state(cu, player, CALC_FROM(loc, pos, 32));
	return;
    case ADD_SPECIAL:
	advance_state(cu, player, CALC_FROM(loc, pos, 64));
	return;
    case ADD_NSPECIAL:
	advance_state(cu, player, CALC_FROM(loc, pos, 128));
	return;
    case TOGGLE_REAR:
	ToggleBit(GetPartFireMode(mech, loc, pos), REAR_MOUNT);
	notify(player, "Toggled!");
	break;
    case TOGGLE_OS:
	ToggleBit(GetPartFireMode(mech, loc, pos), OS_MODE);
	notify(player, "Toggled!");
	break;
    case TOGGLE_AMMO:
	ToggleAmmo(player, mech, loc, pos);
	notify(player, tprintf("Ammo is now %s.",
		ammo_text(GetPartAmmoMode(mech, loc, pos))));
	break;
    case TOGGLE_HALFAMMO:
	ToggleBit(GetPartFireMode(mech, loc, pos), HALFTON_MODE);
	SetPartData(mech, loc, pos, FullAmmo(mech, loc, pos));
	notify(player, tprintf("Changed to %s ton of ammo.",
		GetPartFireMode(mech, loc,
		    pos) & HALFTON_MODE ? "half" : "full"));
	break;
    case TOGGLE_TC:
	ToggleBit(GetPartFireMode(mech, loc, pos), ON_TC);
	notify(player, "Toggled!");
	break;
    case REMOVE:
	SetPartType(mech, loc, pos, 0);
	advance_state(cu, player, loc + 1);
    }
}

static void custom_act_limb(CUSTOM * cu, dbref player, int id)
{
    advance_state(cu, player, id + 1);
}

static void custom_act_armor(CUSTOM * cu, MECH * mech, int loc, int value)
{
    int isrear = loc >= NUM_SECTIONS;

    loc = loc % NUM_SECTIONS;
    if (isrear) {
	SetSectORArmor(mech, loc, value);
	SetSectRArmor(mech, loc, value);
    } else {
	SetSectOArmor(mech, loc, value);
	SetSectArmor(mech, loc, value);
    }
}

static void custom_act_crit(CUSTOM * cu, MECH * mech, dbref player,
    int loc, int pos)
{
    advance_state(cu, player, CALC_FROM(loc, pos, 0));
}

static void /* Change */ custom_recalculate_menu(dbref player,
    coolmenu * c, coolmenu * d)
{
    CUSTOM *cu;
    MECH *mech;
    int loc, pos, s;

    cu = FindObjectsData(Location(player));
    if (!cu || cu->user < 0 || (!Wiz(player) && cu->user != player))
	return;
    mech = &cu->new;
    switch (cu->state) {
    case STATE_MAIN:
	custom_act_main(cu, mech, player, d->id);
	break;
    case STATE_LIMB:
	custom_act_limb(cu, player, d->id);
	break;
    case STATE_ARMOR:
	custom_act_armor(cu, mech, d->id, d->value);
	break;
    default:
	if (cu->state <= NUM_SECTIONS)
	    custom_act_crit(cu, mech, player, cu->state - 1, d->id);
	else {
	    CONVERT_TO(cu->state, loc, pos, s);
	    switch (s) {
	    case 0:
		custom_act_edit(cu, mech, player, loc, pos, d->id);
		break;
	    case 1:
	    case 4:
	    case 16:
		if (!mudconf.btech_parts)
		    custom_act_brand(cu, mech, player, loc, pos,
			s / FIRST_UNUSED_BIT, 0);
		else
		    custom_act_weapon(cu, mech, player, loc, pos, d->id);
		break;
	    case 2:
	    case 8:
	    case 32:
		custom_act_ammo(cu, mech, player, loc, pos, d->id);
		break;
	    case 64:
	    case 128:
		custom_act_special(cu, mech, player, loc, pos, d->id);
		break;
	    default:
		custom_act_brand(cu, mech, player, loc, pos,
		    s / FIRST_UNUSED_BIT, d->id);
		break;
	    }
	}
    }
}

#define CUSTOM_COMMON \
CUSTOM *cu = (CUSTOM *) data; \
MECH *mech = &cu->new; \
DOCHECK(!cu || cu->user < 0, "There is nothing to see, yet!"); \
DOCHECK(cu->user != player && !Wiz(player), "Permission denied."); \
global_silence = 1;

#define CUSTOM_COMMON_END \
global_silence = 0

void custom_status(dbref player, void *data, char *buffer)
{
    CUSTOM_COMMON;
    mech_status(player, mech, buffer);
    CUSTOM_COMMON_END;
}

void custom_weight1(dbref player, void *data, char *buffer)
{
    CUSTOM_COMMON;
    mech_weight(player, mech, buffer);
    CUSTOM_COMMON_END;
}

void custom_weight2(dbref player, void *data, char *buffer)
{
    CUSTOM_COMMON;
    if (!(mech = FindObjectsData(mech->mynum))) {
	notify(player, "Error: Invalid 'mech.");
	CUSTOM_COMMON_END;
	return;
    }
    mech_weight(player, mech, buffer);
    CUSTOM_COMMON_END;
}

void custom_critstatus(dbref player, void *data, char *buffer)
{
    CUSTOM_COMMON;
    mech_critstatus(player, mech, buffer);
    CUSTOM_COMMON_END;
}

void custom_weaponspecs(dbref player, void *data, char *buffer)
{
    CUSTOM_COMMON;
    mech_weaponspecs(player, mech, buffer);
    CUSTOM_COMMON_END;
}

/* This is generated on the fly for each show-menu-request */

/* DasMagic is the one that figures how to get coolmenu struct */
#define DASMAGIC  \
  coolmenu *c;custom_flag = ((CUSTOM *) data)->allow;\
   c = custom_menu(player)

/* DasMagic2 is name of the coolmenu struct in the function */
#define DASMAGIC2 c

/* DasMagic3 is called if value in the menu changes */

/* c = main menu, d = the changed entry */
#define DASMAGIC3 custom_recalculate_menu(player,c,d)

/* If DasMagic4 is set, menu is re-shown every time value changes */
#undef DASMAGIC4

/* Don't want some of the stuff being shown */
#define REAL_SNEAKY_SET

void newfreecustom(dbref key, void **data, int selector)
{
    CUSTOM *new = *data;

    if (selector == SPECIAL_ALLOC)
	new->user = -1;

}


#include "coolmenu_interface2.h"

COMMANDSET(cu);
