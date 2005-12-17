/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 */

#include "turret.h"
#include "mech.tic.h"
#include "create.h"
#include "p.mech.scan.h"
#include "p.mech.move.h"
#include "p.mech.maps.h"
#include "p.mech.combat.h"
#include "p.mech.contacts.h"
#include "p.mech.status.h"

int arc_override = 0;
dbref pilot_override = 0;

#define LOCK_FUDGE(mech,tur) \
if (tur->gunner > 0) pilot_override = tur->gunner; \
stored_status = MechStatus(mech); \
stored_target = MechTarget(mech); \
stored_targx  = MechTargX(mech); \
stored_targy  = MechTargY(mech); \
stored_targz  = MechTargZ(mech); \
MechStatus(mech) &= ~LOCK_MODES; \
MechStatus(mech) |= tur->lockmode; \
arc_override  = tur->arcs; \
MechTarget(mech) = tur->target; \
MechTargX(mech) = tur->targx; \
MechTargY(mech) = tur->targy; \
MechTargZ(mech) = tur->targz

#define LOCK_FUDGE_R(mech,tur) \
pilot_override = 0; \
tur->target = MechTarget(mech); \
tur->targx = MechTargX(mech); \
tur->targy = MechTargY(mech); \
tur->targz = MechTargZ(mech); \
tur->lockmode = (MechStatus(mech) & LOCK_MODES); \
MechStatus(mech) = stored_status; \
MechTarget(mech) = stored_target; \
MechTargX(mech) = stored_targx ; \
MechTargY(mech) = stored_targy ; \
MechTargZ(mech) = stored_targz ; \
arc_override    = 0;

#define LOCK_FUDGE_VARS \
short stored_targx, stored_targy, stored_targz; \
dbref stored_target; int stored_status; \

#define TUR_BASE \
TURRET_T *tur = (TURRET_T *) data; \
MECH *mech = FindObjectsData(tur->parent); \
DOCHECK(!IsMech(tur->parent), "Error: Turret's parentage is unknown.");

#define TUR_COMMON \
TUR_BASE \
DOCHECK(tur->gunner < 0, "The turret hasn't been initialized yet!"); \
DOCHECK(player!=tur->gunner, "You aren't the registered gunner! Go 'way!"); \
DOCHECK(player==MechPilot(mech), "You'll pilot and gun at once? Yah right :P");

#define TUR_GCOMMON \
LOCK_FUDGE_VARS \
TUR_COMMON \
LOCK_FUDGE(mech,tur)

void turret_addtic(dbref player, void *data, char *buffer)
{
#if 0
    TUR_COMMON;
    addtic_sub(player, tur->tic, mech, buffer);
#endif
}

void turret_deltic(dbref player, void *data, char *buffer)
{
#if 0
    TUR_COMMON;
    deltic_sub(player, tur->tic, mech, buffer);
#endif
}

void turret_listtic(dbref player, void *data, char *buffer)
{
#if 0
    TUR_COMMON;
    listtic_sub(player, tur->tic, mech, buffer);
#endif
}


void turret_cleartic(dbref player, void *data, char *buffer)
{
#if 0
    TUR_COMMON;
    cleartic_sub(player, tur->tic, buffer);
#endif
}

void turret_firetic(dbref player, void *data, char *buffer)
{
#if 0
    TUR_GCOMMON;
    firetic_sub(player, tur->tic, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
#endif
}

void turret_bearing(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_bearing(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_eta(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_eta(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_findcenter(dbref player, void *data, char *buffer)
{
    TUR_COMMON;
    mech_findcenter(player, mech, buffer);
}

void turret_fireweapon(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_fireweapon(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_settarget(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_settarget(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_lrsmap(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_lrsmap(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_navigate(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_navigate(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_range(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_range(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_sight(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_sight(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_tacmap(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_tacmap(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_contacts(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_contacts(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_critstatus(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_critstatus(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_report(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_report(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_scan(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_scan(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_status(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_status(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

void turret_weaponspecs(dbref player, void *data, char *buffer)
{
    TUR_GCOMMON;
    mech_weaponspecs(player, mech, buffer);
    LOCK_FUDGE_R(mech, tur);
}

#define SPECIAL_FREE 0
#define SPECIAL_ALLOC 1

/* Alloc/free routine */
void newturret(dbref key, void **data, int selector)
{
    TURRET_T *new = *data;

    switch (selector) {
    case SPECIAL_ALLOC:
	new->target = -1;
	new->targx = -1;
	new->targy = -1;
	new->mynum = key;
	break;
    }
}

void turret_initialize(dbref player, void *data, char *buffer)
{
    TUR_BASE;
    DOCHECK(player != tur->gunner && Connected(tur->gunner) &&
	Location(tur->gunner) == Location(player),
	tprintf("You need %s to leave or disconnect first.",
	    Name(tur->gunner)));
    DOCHECK(player == tur->gunner,
	"You grap firmer hold on the joystick..");
    notify_except(tur->mynum, NOTHING, tur->mynum,
	tprintf("%s initialized as gunner.", Name(player)));
    tur->gunner = player;
}

void turret_deinitialize(dbref player, void *data, char *buffer)
{
    TUR_BASE;
    DOCHECK(player != tur->gunner, "You aren't gunner!");
    notify_except(tur->mynum, NOTHING, tur->mynum,
	tprintf("%s deinitialized as gunner.", Name(player)));
    tur->gunner = -1;
}
