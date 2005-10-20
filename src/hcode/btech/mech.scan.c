
/*
 * $Id: mech.scan.c,v 1.1.1.1 2005/01/11 21:18:22 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Last modified: Tue Oct  6 17:07:19 1998 fingon
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "p.mech.utils.h"
#include "p.mech.los.h"
#include "p.map.obj.h"
#include "p.mech.scan.h"
#include "autopilot.h"
#include "p.autopilot_command.h"
#include "p.mine.h"
#include "p.mech.build.h"
#include "p.mech.status.h"
#include "p.mech.update.h"
#include "p.mech.combat.h"
#include "p.mech.combat.misc.h"

#define SHOW_INFO    1
#define SHOW_ARMOR   2
#define SHOW_WEAPONS 4

void mech_scan(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    MAP *mech_map;
    char *args[4];
    int mapx = 0, mapy = 0;
    char targetID[2];
    dbref target;
    int numargs;
    MECH *tempMech = NULL;
    float fx, fy, fz = 0.0;
    float range = 0.0, enemyX, enemyY, enemyZ;
    int dob = 0, doh = 0;
    int options = SHOW_INFO | SHOW_ARMOR | SHOW_WEAPONS;

    mech_map = getMap(mech->mapindex);
    cch(MECH_USUAL);
    numargs = mech_parseattributes(buffer, args, 4);
    DOCHECK(numargs > 3, "Wrong number of arguments to scan!");
    DOCHECK(!MechScanRange(mech),
	"Your system seems to be inoperational.");
    switch (numargs) {
    case 1:
	/* Scan Target */
	targetID[0] = args[0][0];
	if (args[0][1]) {
	    targetID[1] = args[0][1];
	    target = FindTargetDBREFFromMapNumber(mech, targetID);
	    tempMech = getMech(target);
	    DOCHECK(!tempMech, "Target is not in line of sight!");
	    range = FaMechRange(mech, tempMech);
	    DOCHECK(!InLineOfSight(mech, tempMech, MechX(tempMech),
		    MechY(tempMech), range),
		"Target is not in line of sight!");
	    DOCHECK(!InLineOfSight_NB(mech, tempMech, MechX(tempMech),
		    MechY(tempMech), range),
		"That target isn't seen well enough by the scanners for scanning!");
	    DOCHECK(!MechIsObservator(mech) &&
		(int) range > MechScanRange(mech),
		"Target is out of scanner range.");
	    break;
	} else {		/* Default target */
	    switch (toupper(args[0][0])) {
	    case 'A':
		options = SHOW_ARMOR;
		break;
	    case 'I':
		options = SHOW_INFO;
		break;
	    case 'W':
		options = SHOW_WEAPONS;
		break;
	    default:
		notify(player, "Truly odd option!");
		return;
	    }
	}
    case 0:
	/* scan current target... */
	target = MechTarget(mech);
	tempMech = getMech(target);
	if (tempMech) {
	    range = FaMechRange(mech, tempMech);
	    DOCHECK(!MechIsObservator(mech) &&
		(int) range > MechScanRange(mech),
		"Target is out of scanner range.");
	    DOCHECK(!InLineOfSight(mech, tempMech, MechX(tempMech),
		    MechY(tempMech), range),
		"Target is not in line of sight!");
	    DOCHECK(!InLineOfSight_NB(mech, tempMech, MechX(tempMech),
		    MechY(tempMech), range),
		"That target isn't seen well enough by the scanners for scanning!");
	} else {
	    if (!(MechStatus(mech) & LOCK_BUILDING))
		DOCHECK(!FindTargetXY(mech, &enemyX, &enemyY, &enemyZ),
		    "No default target set!");
	    mapx = MechTargX(mech);
	    mapy = MechTargY(mech);
	    MapCoordToRealCoord(mapx, mapy, &fx, &fy);
	    fz = ZSCALE * Elevation(mech_map, mapx, mapy);
	    range =
		FindRange(MechFX(mech), MechFY(mech), MechFZ(mech), fx, fy,
		fz);
	    ValidCoordA(mech_map, mapx, mapy,
		"Those coordinates are out of scanner range.");
	    DOCHECK(!MechIsObservator(mech) &&
		(int) range > MechScanRange(mech),
		"Those coordinates are out of scanner range.");
	    DOCHECK(!InLineOfSight_NB(mech, tempMech, mapx, mapy, range),
		"Target hex is not in line of sight!");
	    /* look for enemies in that hex... */
	    if (MechStatus(mech) & LOCK_BUILDING)
		dob = 1;
	    else if (MechStatus(mech) & LOCK_HEX) {
		dob = 1;
		doh = 1;
	    } else if (!(tempMech =
		    find_mech_in_hex(mech, mech_map, mapx, mapy, 1)))
		tempMech = (MECH *) NULL;
	}
	break;
    case 3:
	/* scan x, y b */
	mapx = atoi(args[0]);
	mapy = atoi(args[1]);
	ValidCoordA(mech_map, mapx, mapy,
	    "Those coordinates are out of scanner range.");
	switch (toupper(args[2][0])) {
	case 'H':
	    doh = 1;
	case 'B':
	    dob = 1;
	    break;
	default:
	    notify(player, "Invalid 3rd argument!");
	    return;
	}
	MapCoordToRealCoord(mapx, mapy, &fx, &fy);
	fz = ZSCALE * Elevation(mech_map, mapx, mapy);
	range =
	    FindRange(MechFX(mech), MechFY(mech), MechFZ(mech), fx, fy,
	    fz);
	DOCHECK((int) range > MechScanRange(mech),
	    "Those coordinates are out of scanner range.");
	DOCHECK(!InLineOfSight(mech, tempMech, mapx, mapy, range),
	    "Coordinates are not in line of sight!");
	break;
    case 2:
	/* scan x, y */
	mapx = atoi(args[0]);
	mapy = atoi(args[1]);
	if (!mapx && strcmp(args[0], "0")) {
	    targetID[0] = args[0][0];
	    targetID[1] = args[0][1];
	    target = FindTargetDBREFFromMapNumber(mech, targetID);
	    tempMech = getMech(target);
	    DOCHECK(!tempMech, "Target is not in line of sight!");
	    range = FaMechRange(mech, tempMech);
	    DOCHECK(!InLineOfSight(mech, tempMech, MechX(tempMech),
		    MechY(tempMech), range),
		"Target is not in line of sight!");
	    DOCHECK(!MechIsObservator(mech) &&
		(int) range > MechScanRange(mech),
		"Target is out of scanner range.");
	    switch (toupper(args[1][0])) {
	    case 'A':
		options = SHOW_ARMOR;
		break;
	    case 'I':
		options = SHOW_INFO;
		break;
	    case 'W':
		options = SHOW_WEAPONS;
		break;
	    default:
		notify(player, "Truly odd option!");
		return;
	    }
	    break;
	}
	ValidCoordA(mech_map, mapx, mapy,
	    "Those coordinates are out of scanner range.");
	MapCoordToRealCoord(mapx, mapy, &fx, &fy);
	range =
	    FindRange(MechFX(mech), MechFY(mech), MechFZ(mech), fx, fy,
	    fz);
	DOCHECK(!MechIsObservator(mech) &&
	    (int) range > MechScanRange(mech),
	    "Those coordinates are out of scanner range.");
	DOCHECK(!InLineOfSight(mech, tempMech, mapx, mapy, range),
	    "Coordinates are not in line of sight!");
	fz = ZSCALE * Elevation(mech_map, mapx, mapy);
	/* look for enemies in that hex... */
	if (!(tempMech = find_mech_in_hex(mech, mech_map, mapx, mapy, 1)))
	    tempMech = (MECH *) NULL;
	break;
    }
    if (tempMech) {
	DOCHECK(!InLineOfSight_NB(mech, tempMech, MechX(tempMech),
		MechY(tempMech), range),
	    "That target isn't seen well enough by the scanners for report!");
	DOCHECK((MechType(tempMech) == CLASS_MW),
	    "Your scanners cannot give you precise information on targets that small!");
	PrintEnemyStatus(player, mech, tempMech, range, options);
	if (!MechIsObservator(mech)) {
	    mech_notify(tempMech, MECHSTARTED,
		tprintf("You are being scanned by %s",
		    GetMechToMechID(tempMech, mech)));
	    auto_reply(tempMech, tprintf("%s just scanned me.",
		    GetMechToMechID(tempMech, mech)));
	}
	return;
    }
    if (!dob && !doh) {
	notify(player, "You see nobody in the hex!");
	return;
    }
    if (dob)
	show_building_in_hex(mech, mapx, mapy);
    if (doh)
	show_mines_in_hex(player, mech, range, mapx, mapy);
}

void mech_report(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    MAP *mech_map;
    char *args[3];
    int mapx = 0, mapy = 0;
    char targetID[2];
    dbref target;
    int numargs;
    MECH *tempMech = NULL;
    float fx, fy, fz = 0.0;
    float range = 0.0, enemyX, enemyY, enemyZ;

    mech_map = getMap(mech->mapindex);
    cch(MECH_USUAL);
    numargs = mech_parseattributes(buffer, args, 3);
    DOCHECK(numargs > 2, "Wrong number of arguments to report!");
    DOCHECK(!MechScanRange(mech),
	"Your system seems to be inoperational.");
    switch (numargs) {
    case 1:
	/* Scan Target */
	targetID[0] = args[0][0];
	targetID[1] = args[0][1];
	target = FindTargetDBREFFromMapNumber(mech, targetID);
	tempMech = getMech(target);
	DOCHECK(!tempMech, "Target is not in line of sight!");
	range = FaMechRange(mech, tempMech);
	DOCHECK(!InLineOfSight(mech, tempMech, MechX(tempMech),
		MechY(tempMech), range),
	    "Target is not in line of sight!");
	DOCHECK(!InLineOfSight_NB(mech, tempMech, MechX(tempMech),
		MechY(tempMech), range),
	    "That target isn't seen well enough by the scanners for a report!");
	break;
    case 2:
	/* report x, y */
	mapx = atoi(args[0]);
	mapy = atoi(args[1]);
	MapCoordToRealCoord(mapx, mapy, &fx, &fy);
	range =
	    FindRange(MechFX(mech), MechFY(mech), MechFZ(mech), fx, fy,
	    fz);
	ValidCoordA(mech_map, mapx, mapy,
	    "Those coordinates are out of scanner range.");
	DOCHECK((int) range > MechScanRange(mech),
	    "Those coordinates are out of scanner range.");
	DOCHECK(!InLineOfSight(mech, tempMech, mapx, mapy, range),
	    "Coordinates are not in line of sight!");
	DOCHECK(!InLineOfSight_NB(mech, tempMech, mapx, mapy, range),
	    "That target isn't seen well enough by the scanners for a report!");
	fz = ZSCALE * Elevation(mech_map, mapx, mapy);
	/* look for enemies in that hex... */
	tempMech = find_mech_in_hex(mech, mech_map, mapx, mapy, 1);
	DOCHECK(!tempMech, "No target found.");
	break;
    case 0:
	/* report current target... */
	target = MechTarget(mech);
	tempMech = getMech(target);
	if (tempMech) {
	    range = FaMechRange(mech, tempMech);
	    DOCHECK(!InLineOfSight(mech, tempMech, MechX(tempMech),
		    MechY(tempMech), range),
		"Target is not in line of sight!");
	    DOCHECK(!InLineOfSight_NB(mech, tempMech, MechX(tempMech),
		    MechY(tempMech), range),
		"That target isn't seen well enough by the scanners for a report!");
	} else {
	    DOCHECK(!FindTargetXY(mech, &enemyX, &enemyY, &enemyZ),
		"No default target set!");
	    /* look for enemies in that hex... */
	    tempMech = find_mech_in_hex(mech, mech_map, mapx, mapy, 1);
	    DOCHECK(!tempMech, "You don't see a thing.");
	    DOCHECK(!InLineOfSight(mech, tempMech, MechX(tempMech),
		    MechY(tempMech), range), "You don't see a thing.");
	}
    }
    if (tempMech)
	PrintReport(player, mech, tempMech, range);
}

void ShowTurretFacing(dbref player, int spaces, MECH * mech)
{
    int i;
    int j;
    char buff[MBUF_SIZE];

    if (GetSectInt(mech, TURRET) && !(MechType(mech) == CLASS_MECH ||
	    MechType(mech) == CLASS_BSUIT || MechType(mech) == CLASS_MW) &&
	!is_aero(mech)) {
	i = AcceptableDegree(MechTurretFacing(mech));
	if (i > 180)
	    i -= 360;
	j = AcceptableDegree(MechTurretFacing(mech) + MechFacing(mech));
	if (MechMove(mech) != MOVE_NONE)
	    sprintf(buff, "      Turret Facing: %d degrees%s", j,
		i ? tprintf(" (%d offset from heading)", i) : "");
	else
	    sprintf(buff, "      Turret Facing: %d degrees", j);
	notify(player, buff);
    }
}

void PrintReport(dbref player, MECH * mech, MECH * tempMech, float range)
{
    int bearing;
    char buff[100];
    int weaponarc;
    char *mech_name;

    mech_name = silly_atr_get(tempMech->mynum, A_MECHNAME);
    sprintf(buff, "[%s]  %-25.25s Tonnage: %d", MechIDS(tempMech,
	    MechSeemsFriend(mech, tempMech)), mech_name,
	MechTons(tempMech));
    notify(player, buff);
    bearing =
	FindBearing(MechFX(mech), MechFY(mech), MechFX(tempMech),
	MechFY(tempMech));
    sprintf(buff, "      Range: %.1f hex\t\tBearing: %d degrees", range,
	bearing);
    notify(player, buff);
    sprintf(buff, "      Speed: %.1f KPH\t\tHeading: %d degrees",
	MechSpeed(tempMech), MechVFacing(tempMech));
    notify(player, buff);
    if (FlyingT(tempMech))
	notify(player, tprintf("      Vertical speed: %.1f KPH",
		MechVerticalSpeed(tempMech)));
    sprintf(buff, "      X, Y, Z: %3d, %3d, %3d\tHeat: %.0f deg C.",
	MechX(tempMech), MechY(tempMech), MechZ(tempMech),
	10. * MechHeat(tempMech));
    notify(player, buff);
    if (MechLateral(tempMech))
	notify(player, tprintf("      Mech is moving laterally %s",
		LateralDesc(tempMech)));
    ShowTurretFacing(player, 6, tempMech);

    switch (MechMove(tempMech)) {
    case MOVE_NONE:
	notify(player, "      Type: INSTALLATION");
	break;
    case MOVE_BIPED:
	switch (MechType(tempMech)) {
	case CLASS_MW:
	    notify(player,
		"      Type: MECHWARRIOR         Movement: BIPED");
	    break;
	case CLASS_MECH:
	    notify(player,
		"      Type: MECH                Movement: BIPED");
	    break;
	case CLASS_BSUIT:
	    notify(player,
		"      Type: BATTLESUIT(S)       Movement: BIPED");

	}
	break;
    case MOVE_QUAD:
	notify(player, "      Type: MECH                Movement: QUAD");
	break;
    case MOVE_TRACK:
	notify(player,
	    "      Type: VEHICLE             Movement: TRACKED");
	break;
    case MOVE_WHEEL:
	notify(player,
	    "      Type: VEHICLE             Movement: WHEELED");
	break;
    case MOVE_HOVER:
	notify(player, "      Type: VEHICLE             Movement: HOVER");
	break;
    case MOVE_VTOL:
	notify(player, "      Type: VTOL                Movement: VTOL");
	break;
    case MOVE_FLY:
	notify(player,
	    tprintf("      Type: %-9s             Movement: FLIGHT",
		MechType(tempMech) ==
		CLASS_AERO ? "AEROSPACE" : "DROPSHIP"));
	break;
    case MOVE_HULL:
	notify(player, "      Type: NAVAL               Movement: HULL");
	break;
    case MOVE_SUB:
	notify(player,
	    "      Type: NAVAL               Movement: SUBMARINE");
	break;
    case MOVE_FOIL:
	notify(player,
	    "      Type: NAVAL               Movement: HYDROFOIL");
	break;
    }

    weaponarc = InWeaponArc(mech, MechFX(tempMech), MechFY(tempMech));
    if (weaponarc & TURRETARC) {
	notify(player, "      In Turret Arc");
	weaponarc &= ~TURRETARC;
    }
    notify(player, tprintf("      In %s Weapons Arc",
	    GetArcID(mech, weaponarc)));
    Mech_ShowFlags(player, tempMech, 6, 1);
    if (Jumping(tempMech))
	notify(player, tprintf("      Mech is Jumping!\tJump Heading: %d",
		MechJumpHeading(tempMech)));
    notify(player, " ");
}

void PrintEnemyStatus(dbref player, MECH * mymech, MECH * mech,
    float range, int opt)
{
    MECH *tempMech;

    if (!CheckData(player, mech))
	return;
    PrintReport(player, mymech, mech, range);
    if (opt & SHOW_ARMOR)
	PrintArmorStatus(player, mech, 0);
    if (opt & SHOW_INFO) {
	if (MechStatus(mech) & TORSO_RIGHT)
	    notify(player, "Torso is 60 degrees right");
	if (MechStatus(mech) & TORSO_LEFT)
	    notify(player, "Torso is 60 degrees left");
	if (MechCarrying(mech) > 0)
	    if ((tempMech = getMech(MechCarrying(mech))))
		notify(player, tprintf("Towing %s.", GetMechToMechID(mech,
			    tempMech)));
	notify(player, " ");
    }
    if (opt & SHOW_WEAPONS)
	PrintEnemyWeaponStatus(mech, player);
}

void mech_bearing(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data, *tempMech;
    MAP *mech_map;
    char *args[4];
    int argc;
    int ix0, iy0;
    float x0, y0;
    int ix1, iy1;
    float x1, y1, z1;
    float temp;
    char trash[20];
    char buff[100];

    x1 = y1 = -1;

    cch(MECH_USUAL);
    x0 = MechFX(mech);
    y0 = MechFY(mech);
    if (mech->mapindex != -1) {
	mech_map = getMap(mech->mapindex);
	argc = mech_parseattributes(buffer, args, 4);
	if (argc == 0) {
	    /* Bearing to current target */
	    if (MechTarget(mech) != -1) {
		tempMech = getMech(MechTarget(mech));
		if (tempMech) {
		    if (!InLineOfSight(mech, tempMech, MechX(tempMech),
			    MechY(tempMech), FaMechRange(mech,
				tempMech))) {
			notify(player, "Target is not in line of sight!");
			return;
		    }
		}
	    }
	    if (!FindTargetXY(mech, &x1, &y1, &z1)) {
		notify(player, "There is no default target!");
	    } else {
		strcpy(buff, "Bearing to default target is: ");
	    }
	} else if (argc == 2) {
	    /* Bearing to X, Y */
	    ix1 = atoi(args[0]);
	    iy1 = atoi(args[1]);
	    if (!(ix1 >= 0 && ix1 < mech_map->map_width && iy1 >= 0 &&
		    iy1 < mech_map->map_height)) {
		notify(player, "Invalid map coordinates!");
		x1 = y1 = -1.;
	    } else {
		sprintf(buff, "Bearing to  %d,%d is: ", ix1, iy1);
		MapCoordToRealCoord(ix1, iy1, &x1, &y1);
	    }
	} else if (argc == 4) {
	    ix0 = atoi(args[0]);
	    iy0 = atoi(args[1]);
	    ix1 = atoi(args[2]);
	    iy1 = atoi(args[3]);

	    if (!(ix1 >= 0 && ix1 < mech_map->map_width && iy1 >= 0 &&
		    y1 < mech_map->map_height && ix0 >= 0 &&
		    ix0 <= mech_map->map_width && iy0 >= 0 &&
		    iy0 < mech_map->map_height)) {
		notify(player, "Invalid map coordinates!");
		x1 = y1 = -1;
	    } else {
		sprintf(buff, "Bearing to %d,%d from %d,%d is: ", ix1, iy1,
		    ix0, iy0);
		MapCoordToRealCoord(ix0, iy0, &x0, &y0);
		MapCoordToRealCoord(ix1, iy1, &x1, &y1);
	    }
	} else {
	    notify(player,
		"Invalid number of attributes to Bearing function!");
	}
	if (x1 != -1) {
	    temp = FindBearing(x0, y0, x1, y1);
	    sprintf(trash, "%.0f degrees.", temp);
	    strcat(buff, trash);
	    notify(player, buff);
	}
    } else {
	notify(player, "You are not on a map!");
    }
}

void mech_range(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data, *tempMech = NULL;
    MAP *mech_map;
    char *args[4];
    int argc;
    int ix0, iy0;
    float x0, y0, z0;
    int ix1, iy1;
    float x1, y1, z1, hr;
    float temp;
    char trash[40];
    char buff[100];
    char buf1[20];
    char buf2[20];

    x1 = y1 = -1;

    cch(MECH_USUAL);
    x0 = MechFX(mech);
    y0 = MechFY(mech);
    z0 = MechFZ(mech);
    if (mech->mapindex != -1) {
	mech_map = getMap(mech->mapindex);
	argc = mech_parseattributes(buffer, args, 4);
	if (argc == 0) {
	    /* Range to current target */
	    if (MechTarget(mech) != -1) {
		tempMech = getMech(MechTarget(mech));
		if (tempMech) {
		    if (!InLineOfSight(mech, tempMech, MechX(tempMech),
			    MechY(tempMech), FaMechRange(mech,
				tempMech))) {
			notify(player, "Target is not in line of sight!");
			return;
		    }
		}
	    }
	    DOCHECK(!FindTargetXY(mech, &x1, &y1, &z1),
		"There is no default target!");
	    if (MapIsDark(mech_map) && !tempMech)
	    	z1 = ZSCALE * MechZ(mech);
	    strcpy(buff, "Range to default target is: ");
	} else if (argc == 2) {
	    /* Range to X, Y */
	    ix1 = atoi(args[0]);
	    iy1 = atoi(args[1]);
	    if (!(ix1 >= 0 && ix1 < mech_map->map_width && iy1 >= 0 &&
		    iy1 < mech_map->map_height)) {
		notify(player, "Invalid map coordinates!");
		x1 = y1 = -1.;
	    } else {
		sprintf(buff, "Range to  %d,%d is: ", ix1, iy1);
		MapCoordToRealCoord(ix1, iy1, &x1, &y1);
		if (MapIsDark(mech_map))
		    z1 = ZSCALE * MechZ(mech);
		else
		    z1 = ZSCALE * Elevation(mech_map, ix1, iy1);
	    }
	} else if (argc == 4) {
	    /* Range to X, Y from given X, Y */
	    ix0 = atoi(args[0]);
	    iy0 = atoi(args[1]);
	    ix1 = atoi(args[2]);
	    iy1 = atoi(args[3]);

	    if (!(ix1 >= 0 && ix1 < mech_map->map_width && iy1 >= 0 &&
		    iy1 < mech_map->map_height && ix0 >= 0 &&
		    ix0 <= mech_map->map_width && iy0 >= 0 &&
		    iy0 < mech_map->map_height)) {
		notify(player, "Invalid map coordinates!");
		x1 = y1 = -1;
	    } else {
		sprintf(buff, "Range to %d,%d from %d,%d is: ", ix1, iy1,
		    ix0, iy0);
		MapCoordToRealCoord(ix1, iy1, &x1, &y1);
		MapCoordToRealCoord(ix0, iy0, &x0, &y0);
		if (MapIsDark(mech_map))
		    z1 = z0 = 0;
		else {
		    z1 = ZSCALE * Elevation(mech_map, ix1, iy1);
		    z0 = ZSCALE * Elevation(mech_map, ix0, iy0);
		}
	    }
	} else {
	    notify(player,
		"Invalid number of attributes to Range function!");
	    x1 = y1 = -1;
	}
	if (x1 != -1) {
	    temp = FindRange(x0, y0, z0, x1, y1, z1);
	    hr = FindHexRange(x0, y0, x1, y1);
	    sprintf(buf1, "%.1f", temp);
	    sprintf(buf2, "%.1f", hr);
	    if (strcmp(buf1, buf2))
		sprintf(trash, "%s hexes (%s ground hexes).", buf1, buf2);
	    else
		sprintf(trash, "%s hexes.", buf1);
	    strcat(buff, trash);
	    notify(player, buff);
	}
    } else {
	notify(player, "You are not on a map!");
    }
}

void mech_vector(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data, *tempMech;
    MAP *mech_map;
    char *args[6];
    int argc;
    int ix0, iy0, iz0;
    float x0, y0, z0;
    int ix1, iy1, iz1;
    float x1, y1, z1, hr;
    float temp;
    char trash[40];
    char buff[100];
    char buf1[20];
    char buf2[20];

    x1 = y1 = -1;

    cch(MECH_USUAL);
    x0 = MechFX(mech);
    y0 = MechFY(mech);
    z0 = MechFZ(mech);
    if (mech->mapindex != -1) {
	mech_map = getMap(mech->mapindex);
	argc = mech_parseattributes(buffer, args, 6);
	if (argc == 0) {
	    /* Range to current target */
	    if (MechTarget(mech) != -1) {
		tempMech = getMech(MechTarget(mech));
		if (tempMech) {
		    if (!InLineOfSight(mech, tempMech, MechX(tempMech),
			    MechY(tempMech), FaMechRange(mech, tempMech))) {
			notify(player, "Target is not in line of sight!");
			return;
		    }
		}
	    }
	    DOCHECK(!FindTargetXY(mech, &x1, &y1, &z1),
		"There is no default target!");
	    strcpy(buff, "Vector to default target is: ");
	} else if (argc == 2) {
	    /* Range to X, Y */
	    ix1 = atoi(args[0]);
	    iy1 = atoi(args[1]);
	    if (!(ix1 >= 0 && ix1 < mech_map->map_width && iy1 >= 0 &&
		    iy1 < mech_map->map_height)) {
		notify(player, "Invalid map coordinates!");
		x1 = y1 = -1.;
	    } else {
		sprintf(buff, "Vector to  %d,%d is: ", ix1, iy1);
		MapCoordToRealCoord(ix1, iy1, &x1, &y1);
		z1 = ZSCALE * Elevation(mech_map, ix1, iy1);
	    }
	} else if (argc == 3) {
	    iz0 = z0 / ZSCALE;
            ix1 = atoi(args[0]);
            iy1 = atoi(args[1]);
	    iz1 = atoi(args[2]);
            if (!(ix1 >= 0 && ix1 < mech_map->map_width && iy1 >= 0 &&
                    iy1 < mech_map->map_height)) {
                notify(player, "Invalid map coordinates!");
                x1 = y1 = -1.;
            } else {
                sprintf(buff, "Vector to  %d,%d,%d is: ", ix1, iy1, iz1);
                MapCoordToRealCoord(ix1, iy1, &x1, &y1);
                z1 = ZSCALE * iz1;
            } 
	} else if (argc == 4) {
	    /* Range to X, Y from given X, Y */
	    ix0 = atoi(args[0]);
	    iy0 = atoi(args[1]);
	    ix1 = atoi(args[2]);
	    iy1 = atoi(args[3]);

	    if (!(ix1 >= 0 && ix1 < mech_map->map_width && iy1 >= 0 &&
		    y1 < mech_map->map_height && ix0 >= 0 &&
		    ix0 <= mech_map->map_width && iy0 >= 0 &&
		    iy0 < mech_map->map_height)) {
		notify(player, "Invalid map coordinates!");
		x1 = y1 = -1;
	    } else {
		sprintf(buff, "Vector to %d,%d from %d,%d is: ", ix1, iy1,
		    ix0, iy0);
		MapCoordToRealCoord(ix1, iy1, &x1, &y1);
		MapCoordToRealCoord(ix0, iy0, &x0, &y0);
		z1 = ZSCALE * Elevation(mech_map, ix1, iy1);
		z0 = ZSCALE * Elevation(mech_map, ix0, iy0);
	    }
	} else if (argc == 6) {
            ix0 = atoi(args[0]);
            iy0 = atoi(args[1]);
 	    iz0 = atoi(args[2]);
            ix1 = atoi(args[3]);
            iy1 = atoi(args[4]);
 	    iz1 = atoi(args[5]);
                                                                                                                                                                                      
            if (!(ix1 >= 0 && ix1 < mech_map->map_width && iy1 >= 0 &&
                    y1 < mech_map->map_height && ix0 >= 0 &&
                    ix0 <= mech_map->map_width && iy0 >= 0 &&
                    iy0 < mech_map->map_height)) {
                notify(player, "Invalid map coordinates!");
                x1 = y1 = -1;
            } else {
                sprintf(buff, "Vector to %d,%d,%d from %d,%d,%d is: ", ix1, iy1, iz1,
                    ix0, iy0, iz0);
                MapCoordToRealCoord(ix1, iy1, &x1, &y1);
                MapCoordToRealCoord(ix0, iy0, &x0, &y0);
                z1 = ZSCALE * iz1;
                z0 = ZSCALE * iz0;
            } 

	} else {
	    notify(player,
		"Invalid number of attributes to Vector function!");
	    x1 = y1 = -1;
	}
	if (x1 != -1) {
	    /* range */
	    temp = FindRange(x0, y0, z0, x1, y1, z1);
	    hr = FindHexRange(x0, y0, x1, y1);
	    sprintf(buf1, "%.1f", temp);
	    sprintf(buf2, "%.1f", hr);
	    if (strcmp(buf1, buf2))
		sprintf(trash, "%s hexes (%s ground hexes) and ", buf1,
		    buf2);
	    else
		sprintf(trash, "%s hexes and ", buf1);
	    strcat(buff, trash);

	    /* bearing */
	    temp = FindBearing(x0, y0, x1, y1);
	    if (argc != 0 && argc != 3 && argc != 6) 
		sprintf(trash, "%.0f degrees.", temp);
	    else
	        sprintf(trash, "%.0f degrees mark %c%d.", temp, (z1 > z0 ? '+' : z1 < z0 ? '-' : ' '), FindZBearing(x0, y0, z0, x1, y1, z1));
	    strcat(buff, trash);

	    notify(player, buff);
	}
    } else {
	notify(player, "You are not on a map!");
    }
}

void PrintEnemyWeaponStatus(MECH * mech, dbref player)
{
    unsigned char weaparray[MAX_WEAPS_SECTION];
    unsigned char weapdata[MAX_WEAPS_SECTION];
    int critical[MAX_WEAPS_SECTION];
    int count;
    int loop;
    int ii;
    char weapbuff[70];
    char tempbuff[50];
    char location[20];
    int running_sum = 0;

    recycle_weaponry(mech);
    notify(player, "================WEAPON SYSTEMS================");
    if (MechType(mech) == CLASS_BSUIT)
	notify(player, "----- Weapon ------ [##]  Holder ------ Status");
    else
	notify(player, "----- Weapon ------ [##]  Location ---- Status");
    for (loop = 0; loop < NUM_SECTIONS; loop++) {
	if (SectIsDestroyed(mech, loop))
	    continue;
	count = FindWeapons(mech, loop, weaparray, weapdata, critical);
	if (count > 0) {
	    ArmorStringFromIndex(loop, tempbuff, MechType(mech),
		MechMove(mech));
	    sprintf(location, "%-14.14s", tempbuff);

	    for (ii = 0; ii < count; ii++) {
		sprintf(weapbuff, " %-18.18s [%2d]  ",
		    &MechWeapons[weaparray[ii]].name[3], running_sum + ii);
		strcat(weapbuff, location);

		if (PartIsNonfunctional(mech, loop, critical[ii])) {
		    strcat(weapbuff, "%ch%cx*****%c");
		} else {
		    if (weapdata[ii]) {
			strcat(weapbuff, "-----");
		    } else {
			strcat(weapbuff, "%cgReady%c");
		    }
		}
		notify(player, weapbuff);
	    }
	    running_sum += count;
	}
    }
}

void mech_sight(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    MAP *mech_map;
    char *args[5];
    int argc;
    int weapnum;

    mech_map = getMap(mech->mapindex);
    cch(MECH_USUAL);
    argc = mech_parseattributes(buffer, args, 5);
    if (argc >= 1) {
	weapnum = atoi(args[0]);
	FireWeaponNumber(player, mech, mech_map, weapnum, argc, args, 1);
    } else {
	notify(player, "Not enough arguments to the function");
    }
}

void mech_view(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data, *target;
    MAP *mech_map;
    int targetnum;
    char targetID[5];
    char *args[5];
    int argc;
    char *target_desc;

    cch(MECH_USUAL);
    argc = mech_parseattributes(buffer, args, 2);
    if (argc == 0) {		/* default target */
	if (MechTarget(mech) == -1) {
	    mech_notify(mech, MECHALL,
		"You do not have a default target set!");
	    return;
	}
	target = getMech(MechTarget(mech));
	if (!target) {
	    mech_notify(mech, MECHALL, "Invalid default target!");
	    MechTarget(mech) = -1;
	    return;
	}
	if (*(target_desc = silly_atr_get(target->mynum, A_MECHDESC)))
	    notify(player, target_desc);
	else
	    notify(player, "That target has no markings.");
    } else if (argc == 1) {	/* ID number */
	targetID[0] = args[0][0];
	targetID[1] = args[0][1];
	targetnum = FindTargetDBREFFromMapNumber(mech, targetID);
	if (targetnum == -1) {
	    mech_notify(mech, MECHPILOT, "Target is not in line of sight!");
	    return;
	}
	target = getMech(targetnum);
	mech_map = getMap(mech->mapindex);
	
	if (!target ||
	    !InLineOfSight(mech, target, MechX(target), MechY(target),
		FlMechRange(mech_map, mech, target))) {
	    mech_notify(mech, MECHPILOT, "Target is not in line of sight!");
	    return;
	}
	
	DOCHECK(!InLineOfSight_NB(mech, target, MechX(target),
	    MechY(target), FlMechRange(mech_map, mech, target)),
		"That target isn't seen well enough by the scanners for viewing!");
		    
	if (*(target_desc = silly_atr_get(target->mynum, A_MECHDESC)))
	    notify(player, target_desc);
	else
	    notify(player, "That target has no markings.");
    } else
	notify(player, "Invalid number of arguments to function.");
}
