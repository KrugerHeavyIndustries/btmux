
/*
 * $Id: mech.contacts.c,v 1.1.1.1 2005/01/11 21:18:14 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Last modified: Tue Oct  6 17:15:16 1998 fingon
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "mech.events.h"
#include "map.h"
#include "p.mech.utils.h"
#include "p.mech.los.h"

char *default_contactoptions = "!db";

static char *ac_desc[] = {
    "0 - See enemies and friends, long text, color",
    "1 - See enemies and friends, short text, color",
    "2 - See enemies only, long text, color",
    "3 - See enemies only, short text, color",
    "4 - See enemies and friends, short text, no color",
    "5 - See enemies only, short text, no color",

    "6 - Disabled"
};

static char *c_desc[] = {
    "0 - Very verbose",
    "1 - Short form, the usual one",
    "2 - Short form, the usual one, but do not see buildings",
    "3 - Shorter form"
};

void show_brief_flags(dbref player, MECH * mech)
{
    notify(player, tprintf("Brief status for %s:", GetMechToMechID(mech,
		mech)));
#ifdef ADVANCED_LOS
    notify(player, tprintf("    (A)utocontacts: %s",
	    ac_desc[mech->brief / 4]));
#endif
    notify(player, tprintf("    (C)ontacts:     %s",
	    c_desc[mech->brief % 4]));
}

void mech_brief(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    char c;
    int v;

    cch(MECH_USUALSM);
    skipws(buffer);
    if (!*buffer) {
	show_brief_flags(player, mech);
	return;
    }
    c = *buffer;
    buffer++;
    skipws(buffer);
    DOCHECK(!*buffer, "Argument missing!");
    DOCHECK(Readnum(v, buffer), "Invalid number!");
    switch (toupper(c)) {
#ifdef ADVANCED_LOS
    case 'A':
	DOCHECK(v < 0 || v > 6, "Number out of range!");
	v = BOUNDED(0, v, 6);
	mech->brief = mech->brief % 4;
	mech->brief += v * 4;
	mech_notify(mech, MECHALL,
	    tprintf("Autocontact brevity set to %s.", ac_desc[v]));
	return;
#endif
    case 'C':
	DOCHECK(v < 0 || v > 3, "Number out of range!");
	v = BOUNDED(0, v, 3);
	mech->brief = ((mech->brief / 4) * 4) + v;
	mech_notify(mech, MECHALL, tprintf("Contact brevity set to %s.",
		c_desc[v]));
	return;
    }
}


#define SEE_DEAD	0x01
#define SEE_SHUTDOWN	0x02
#define SEE_ALLY	0x04
#define SEE_ENEMA	0x08
#define SEE_TARGET	0x10
#define SEE_BUILDINGS	0x20
#define SEE_NEGNEXT	0x80

char getWeaponArc(MECH * mech, int arc)
{
    if (arc & FORWARDARC)
    	return '*';
    else if (arc & TURRETARC)
    	return 't';
    else if (arc & RSIDEARC)
    	return 'r';
    else if (arc & LSIDEARC)
    	return 'l';
    else if (arc & REARARC)
    	return 'v';
    else
    	return '?';
}

/* who: 0 for friend, 1 for enemy, 2 for 'self' */
char *getStatusString(MECH * target, int who)
{
    static char statusstr[20];
    int sptr = 0;

    if (Destroyed(target))
	statusstr[sptr++] = 'D';

    if (Starting(target))
	statusstr[sptr++] = 's';
    else if (!Started(target))
	statusstr[sptr++] = 'S';

    if (Standing(target))
	statusstr[sptr++] = 'f';
    else if (Fallen(target))
	statusstr[sptr++] = 'F';

    if (ChangingHulldown(target))
	statusstr[sptr++] = 'h';
    else if (IsHulldown(target))
	statusstr[sptr++] = 'H';

    if (Towed(target))
	statusstr[sptr++] = 'T';
    else if (MechCarrying(target) > 0)
	statusstr[sptr++] = 't';

    if (Jumping(target))
	statusstr[sptr++] = 'J';

    if (OODing(target))
	statusstr[sptr++] = 'O';

    if (MechHeat(target))
	statusstr[sptr++] = '+';

    if (Jellied(target))
	statusstr[sptr++] = 'I';

    if (Burning(target))
	statusstr[sptr++] = 'B';

    if (MechLites(target))
	statusstr[sptr++] = 'L';

    if (MechLit(target))
	statusstr[sptr++] = 'l';

    if (MechSwarmTarget(target) > 0)
	statusstr[sptr++] = 'W';

    if (CarryingClub(target))
	statusstr[sptr++] = 'C';

    if (checkAllSections(target, NARC_ATTACHED) ||
	checkAllSections(target, INARC_HOMING_ATTACHED)) {
	if (who == 1)
	    statusstr[sptr++] = 'N';
	else
	    statusstr[sptr++] = 'n';
    }
    
#ifndef ECM_ON_CONTACTS
    if (who > 1) {
#endif
    if (AnyECCMActive(target))
    	statusstr[sptr++] = 'P';
    	
    if (AnyECMActive(target))
    	statusstr[sptr++] = 'E';
    	
    if (AnyECMProtected(target))
    	statusstr[sptr++] = 'p';
    	
    if (AnyECMDisturbed(target))
    	statusstr[sptr++] = 'e';
#ifndef ECM_ON_CONTACTS
    }
#endif
    
    if (Spinning(target))
    	statusstr[sptr++] = 'X';

#ifdef BT_MOVEMENT_MODES
    if (Sprinting(target))
	statusstr[sptr++] = 'M';
    if (Evading(target))
	statusstr[sptr++] = 'm';
#endif
    
    statusstr[sptr] = '\0';
    return statusstr;
}

char getStatusChar(MECH * mech, MECH * mechTarget, int wCharNum)
{
    char cRet = ' ';

    switch (wCharNum) {
    case 1:
	cRet = MechSwarmTarget(mechTarget) > 0 ? 'W' :
	    Towed(mechTarget) ? 'T' : MechCarrying(mechTarget) >
	    0 ? 't' : CarryingClub(mechTarget) ? 'C' :
#ifdef BT_MOVEMENT_MODES
	    Sprinting(mechTarget) ? 'M' : Evading(mechTarget) ? 'm' :
#endif
	    ' ';
	break;
    case 2:
	cRet = Destroyed(mechTarget) ? 'D' :
	    MechLites(mechTarget) ? 'L' : MechLit(mechTarget) ? 'l' : ' ';
	break;
    case 3:
	cRet = Jumping(mechTarget) ? 'J' : OODing(mechTarget) ? 'O' :
	    Fallen(mechTarget) ? 'F' : Standing(mechTarget) ? 'f' :
	    ChangingHulldown(mechTarget) ? 'h' : IsHulldown(mechTarget) ?
	    'H' : Spinning(mech) ? 'X' : ' ';
	break;
    case 4:
	cRet = Started(mechTarget) ?
	    (MechHeat(mechTarget) ? '+' :
	    Jellied(mechTarget) ? 'I' :
	    Burning(mechTarget) ? 'B' : ' ') :
	    Staggering(mechTarget) ? 'G' :
	    Starting(mechTarget) ? 's' : 'S';
	break;
    case 5:
	cRet = (checkAllSections(mechTarget, NARC_ATTACHED) ||
	    checkAllSections(mechTarget, INARC_HOMING_ATTACHED)) ?
	    (MechTeam(mechTarget) == MechTeam(mech) ? 'n' : 'N') :
#ifdef ECM_ON_CONTACTS
	    AnyECCMActive(mech) ? 'P' : 
	    AnyECMActive(mech) ? 'E' : AnyECMProtected(mech) ? 'p' : 
	    AnyECMDisturbed(mech) ? 'e' :
#endif
	    ' ';
	break;
    }

    return cRet;
}

void mech_contacts(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data, *tempMech;
    MAP *mech_map = getMap(mech->mapindex), *tmp_map;
    mapobj *building;
    int loop, i, j, argc, bearing, buffindex = 0;
    char *args[1], bufflist[MAX_MECHS_PER_MAP][120], buff[100];
    int sbuff[MAX_MECHS_PER_MAP];
    float range, rangelist[MAX_MECHS_PER_MAP], fx, fy;
    int mechfound;
    char weaponarc;
    char *mech_name;
    char see_what;
    char *str;
    char move_type[30];
    char cStatus1, cStatus2, cStatus3, cStatus4, cStatus5;
    int losflag;
    int isvb;
    int inlos;
    int IsUsingHUD = 0;

    cch(MECH_USUAL);
    mechfound = 0;
    argc = mech_parseattributes(buffer, args, 1);

    isvb = (mech->brief % 4);
    if (argc > 0) {
	if (args[0][0] == 'h') {
	    IsUsingHUD = 1;
	    see_what =
		(SEE_DEAD | SEE_SHUTDOWN | SEE_ENEMA | SEE_ALLY |
		SEE_TARGET);
	} else {
	    if (args[0][0] == '+') {
		str = silly_atr_get(player, A_CONTACTOPT);
		if (!*str)
		    strcpy(buff, default_contactoptions);
		else {
		    strncpy(buff, str, 50);
		    buff[49] = 0;

		    if (strlen(buff) == 0)
			strcpy(buff, default_contactoptions);
		}
	    } else {
		strncpy(buff, args[0], 50);
		buff[49] = 0;
	    }

	    if (isvb == 1)
		see_what = SEE_BUILDINGS;
	    else
		see_what = 0x0;

	    for (loop = 0; loop < 50 && buff[loop]; loop++) {
		char c;

		c = buff[loop];

		if (c == 'd')

		    (see_what & SEE_NEGNEXT) ? (see_what &=
			~SEE_DEAD) : (see_what |= SEE_DEAD);
		else if (c == 's')
		    (see_what & SEE_NEGNEXT) ? (see_what &=
			~SEE_SHUTDOWN) : (see_what |= SEE_SHUTDOWN);
		else if (c == 'b')
		    (see_what & SEE_NEGNEXT) ? (see_what &=
			~SEE_BUILDINGS) : (see_what |= SEE_BUILDINGS);
		else if (c == 'e')
		    (see_what & SEE_NEGNEXT) ? (see_what &=
			~SEE_ENEMA) : (see_what |= SEE_ENEMA);
		else if (c == 'a')
		    (see_what & SEE_NEGNEXT) ? (see_what &=
			~SEE_ALLY) : (see_what |= SEE_ALLY);
		else if (c == 't')
		    (see_what & SEE_NEGNEXT) ? (see_what &=
			~SEE_TARGET) : (see_what |= SEE_TARGET);
		else if (c == '!') {
		    see_what =
			(SEE_NEGNEXT | SEE_DEAD | SEE_SHUTDOWN | SEE_ENEMA
			| SEE_ALLY | SEE_TARGET);
		} else
		    notify(player,
			tprintf("Ignoring %c as contact option.", c));
	    }
	}
    } else {
	see_what =
	    (SEE_DEAD | SEE_SHUTDOWN | SEE_ENEMA | SEE_ALLY | SEE_TARGET);
	if (isvb == 1)
	    see_what |= SEE_BUILDINGS;
    }

    if (IsUsingHUD)
	notify(player, "#HUDINFO:CON#Line of Sight Contacts:");
    else if (isvb <= 2)
	notify(player, "Line of Sight Contacts:");

    for (loop = 0; loop < mech_map->first_free; loop++) {
	if (!(mech_map->mechsOnMap[loop] != mech->mynum &&
		mech_map->mechsOnMap[loop] != -1))
	    continue;

	tempMech = (MECH *) FindObjectsData(mech_map->mechsOnMap[loop]);

	if (!tempMech)
	    continue;
	if (argc) {
	    if (!((MechSeemsFriend(mech, tempMech) ? (see_what & SEE_ALLY)
			: (see_what & SEE_ENEMA)) ||
		    ((see_what & SEE_TARGET) &&
			(tempMech->mynum == MechTarget(mech)))))
		continue;
	    if (!(((see_what & SEE_SHUTDOWN) || Started(tempMech)) ||
		    Destroyed(tempMech) || ((see_what & SEE_TARGET) &&
			(tempMech->mynum == MechTarget(mech)))))
		continue;
	    if (!(((see_what & SEE_DEAD) || !Destroyed(tempMech)) ||
		    ((see_what & SEE_TARGET) &&
			(tempMech->mynum == MechTarget(mech)))))
		continue;
	}
	range = FlMechRange(mech_map, mech, tempMech);
	if (!(losflag =
		InLineOfSight(mech, tempMech, MechX(tempMech),
		    MechY(tempMech), range)))
	    continue;
	if (Good_obj(tempMech->mynum)) {
	    if (!InLineOfSight_NB(mech, tempMech, MechX(tempMech),
		    MechY(tempMech), 0.0)) {
		mech_name = "something";
		inlos = 0;
	    } else {
		mech_name = silly_atr_get(tempMech->mynum, A_MECHNAME);
		inlos = 1;
	    }
	} else
	    continue;
	bearing =
	    FindBearing(MechFX(mech), MechFY(mech), MechFX(tempMech),
	    MechFY(tempMech));
	weaponarc = getWeaponArc(mech, InWeaponArc(mech, MechFX(tempMech),
						   MechFY(tempMech)));

	strcpy(move_type, GetMoveTypeID(MechMove(tempMech)));

	if (isvb) {
	    if (!inlos) {
		cStatus1 = ' ';
		cStatus2 = ' ';
		cStatus3 = ' ';
		cStatus4 = ' ';
		cStatus5 = ' ';
	    } else {
		cStatus1 = getStatusChar(mech, tempMech, 1);
		cStatus2 = getStatusChar(mech, tempMech, 2);
		cStatus3 = getStatusChar(mech, tempMech, 3);
		cStatus4 = getStatusChar(mech, tempMech, 4);
		cStatus5 = getStatusChar(mech, tempMech, 5);
	    }

	    if (!IsUsingHUD) {
#ifdef SIMPLE_SENSORS
		sprintf(buff,
		    "%s%c[%s]%c %-12.12s x:%3d y:%3d z:%3d r:%4.1f b:%3d s:%5.1f h:%3d S:%c%c%c%c%c%s",
#else
		sprintf(buff,
		    "%s%c%c%c[%s]%c %-12.12s x:%3d y:%3d z:%3d r:%4.1f b:%3d s:%5.1f h:%3d S:%c%c%c%c%c%s",
#endif
		    tempMech->mynum == MechTarget(mech) ? "%ch%cr" :
		    !MechSeemsFriend(mech, tempMech) ? "%ch%cy" : "",
#ifndef SIMPLE_SENSORS
		    (losflag & MECHLOSFLAG_SEESP) ? 'P' : ' ',
		    (losflag & MECHLOSFLAG_SEESS) ? 'S' : ' ',
#endif
		    weaponarc, MechIDS(tempMech,
			MechSeemsFriend(mech, tempMech)),
		    move_type[0], mech_name, MechX(tempMech),
		    MechY(tempMech), MechZ(tempMech), range, bearing,
		    MechSpeed(tempMech), MechVFacing(tempMech),
		    cStatus1,
		    cStatus2,
		    cStatus3,
		    cStatus4,
		    cStatus5,
		    (tempMech->mynum == MechTarget(mech) ||
			!MechSeemsFriend(mech, tempMech)) ? "%c" : "");
	    } else {
#ifdef SIMPLE_SENSORS
		sprintf(buff, "#HUDINFO:CON#%c,%c,%s,%c,%-12.12s,%3d,%3d,%3d,%4.1f,%3d,%4.1f,%3d,%c%c%c%c%c",	/* ) <- balance */
#else
		sprintf(buff,
		    "#HUDINFO:CON#%c,%c,%c,%c,%s,%c,%-12.12s,%3d,%3d,%3d,%4.1f,%3d,%4.1f,%3d,%c%c%c%c%c",
#endif
		    (tempMech->mynum == MechTarget(mech)) ? 'T' :
		    !MechSeemsFriend(mech, tempMech) ? 'E' : 'F',
#ifndef SIMPLE_SENSORS
		    (losflag & MECHLOSFLAG_SEESP) ? 'P' : ' ',
		    (losflag & MECHLOSFLAG_SEESS) ? 'S' : ' ',
#endif
		    weaponarc, MechIDS(tempMech,
			MechSeemsFriend(mech, tempMech)),
		    move_type[0], mech_name, MechX(tempMech),
		    MechY(tempMech), MechZ(tempMech), range, bearing,
		    MechSpeed(tempMech), MechVFacing(tempMech),
		    cStatus1, cStatus2, cStatus3, cStatus4, cStatus5);
	    }

	    rangelist[buffindex] = range;
	    rangelist[buffindex] +=
		(MechStatus(tempMech) & DESTROYED) ? 10000 : 0;
	    strcpy(bufflist[buffindex++], buff);
	} else {
	    sprintf(buff, "[%s] %-17s  Tonnage: %d", MechIDS(tempMech,
		    MechSeemsFriend(mech, tempMech)), mech_name,
		MechTons(tempMech));
	    notify(player, buff);
	    sprintf(buff, "      Range: %.1f hex\tBearing: %d degrees",
		range, bearing);
	    notify(player, buff);
	    sprintf(buff, "      Speed: %.1f KPH\tHeading: %d degrees",
		MechSpeed(tempMech), MechVFacing(tempMech));
	    notify(player, buff);
	    sprintf(buff, "      X, Y: %3d, %3d \tHeat: %.0f deg C.",
		MechX(tempMech), MechY(tempMech), MechHeat(tempMech));
	    notify(player, buff);
	    sprintf(buff, "      Movement Type: %s", move_type);
	    notify(player, buff);
	    notify(player, tprintf("      Mech is in %s Arc",
		    GetArcID(mech, InWeaponArc(mech, MechFX(tempMech),
			    MechFY(tempMech)))));
	    if (MechStatus(tempMech) & DESTROYED)
		notify(player, "      Mech Destroyed");
	    if (!(MechStatus(tempMech) & STARTED))
		notify(player, "      Mech Shutdown");
	    if (Fallen(tempMech))
		notify(player, "      Mech has Fallen!");
	    if (Jumping(tempMech))
		notify(player,
		    tprintf("      Mech is Jumping!\tJump Heading: %d",
			MechJumpHeading(tempMech)));
	    notify(player, " ");
	}
    }

    if (see_what & SEE_BUILDINGS) {
	for (building = first_mapobj(mech_map, TYPE_BUILD); building;
	    building = next_mapobj(building)) {

	    MapCoordToRealCoord(building->x, building->y, &fx, &fy);
	    range =
		FindRange(MechFX(mech), MechFY(mech), MechFZ(mech), fx, fy,
		ZSCALE * ((i =
			Elevation(mech_map, building->x,
			    building->y)) + 1));

	    losflag =
		InLineOfSight(mech, NULL, building->x, building->y, range);
	    if (!losflag || (losflag & MECHLOSFLAG_BLOCK))
		continue;

	    if (!(building->obj && (tmp_map = getMap(building->obj))))
		continue;
	    if (BuildIsInvis(tmp_map))
		continue;
	    if ((j = !can_pass_lock(mech->mynum, tmp_map->mynum, A_LENTER))
		&& BuildIsHidden(tmp_map))
		continue;
	    bearing = FindBearing(MechFX(mech), MechFY(mech), fx, fy);
	    weaponarc = getWeaponArc(mech, InWeaponArc(mech, fx, fy));

	    mech_name = silly_atr_get(building->obj, A_MECHNAME);
	    if (!mech_name || !*mech_name)
		mech_name = strip_ansi(Name(building->obj));

	    if (!IsUsingHUD) {
#ifdef SIMPLE_SENSORS
		sprintf(buff,
		    "%s%c %-23.23s x:%3d y:%3d z:%2d r:%4.1f b:%3d CF:%4d /%4d S:%c%c%s",
#else
		sprintf(buff,
		    "%s%c%c%c %-23.23s x:%3d y:%3d z:%2d r:%4.1f b:%3d CF:%4d /%4d S:%c%c%s",
#endif
		    j ? "%ch%cy" : "",
#ifndef SIMPLE_SENSORS
		    (losflag & MECHLOSFLAG_SEESP) ? 'P' : ' ',
		    (losflag & MECHLOSFLAG_SEESS) ? 'S' : ' ',
#endif
		    weaponarc, mech_name, building->x, building->y, i,
		    range, bearing, tmp_map->cf, tmp_map->cfmax,
		    (BuildIsSafe(tmp_map) || (j &&
			    BuildIsCS(tmp_map))) ? 'X' : j ? 'x' :
		    BuildIsCS(tmp_map) ? 'C' : ' ',
		    BuildIsHidden(tmp_map) ? 'H' : ' ', j ? "%c" : "");
	    } else {
#ifdef SIMPLE_SENSORS
		sprintf(buff, "#HUDINFO:CON#%c,%c,%-21.21s,%3d,%3d,%3d,%4.1f,%3d,%4d,%4d,%c%c",	/* ) <- balance */
#else
		sprintf(buff,
		    "#HUDINFO:CON#%c,%c,%c,%c,%-21.21s,%3d,%3d,%3d,%4.1f,%3d,%4d,%4d,%c%c",
#endif
		    j ? 'E' : 'F',
#ifndef SIMPLE_SENSORS
		    (losflag & MECHLOSFLAG_SEESP) ? 'P' : ' ',
		    (losflag & MECHLOSFLAG_SEESS) ? 'S' : ' ',
#endif
		    weaponarc, mech_name, building->x, building->y, i,
		    range, bearing, tmp_map->cf, tmp_map->cfmax,
		    (BuildIsSafe(tmp_map) || (j &&
			    BuildIsCS(tmp_map))) ? 'X' : j ? 'x' :
		    BuildIsCS(tmp_map) ? 'C' : ' ',
		    BuildIsHidden(tmp_map) ? 'H' : ' ');
	    }
	    rangelist[buffindex] = range + 20000;
	    strcpy(bufflist[buffindex++], buff);
	}
    }


    if (isvb) {
	for (i = 0; i < buffindex; i++)
	    sbuff[i] = i;
	/* print a sorted list of detected mechs */
	/* use the ever-popular bubble sort */
	for (i = 0; i < (buffindex - 1); i++)
	    for (j = (i + 1); j < buffindex; j++)
		if (rangelist[sbuff[j]] > rangelist[sbuff[i]]) {
		    loop = sbuff[i];
		    sbuff[i] = sbuff[j];
		    sbuff[j] = loop;
		}
	for (loop = 0; loop < buffindex; loop++)
	    notify(player, bufflist[sbuff[loop]]);
    }

    if (IsUsingHUD)
	notify(player, "#HUDINFO:CON#End Contact List");
    else if (isvb <= 2)
	notify(player, "End Contact List");
}

#undef SEE_DEAD
#undef SEE_SHUTDOWN
#undef SEE_ALLY
#undef SEE_ENEMA
#undef SEE_TARGET
#undef SEE_BUILDINGS
#undef SEE_NEGNEXT
