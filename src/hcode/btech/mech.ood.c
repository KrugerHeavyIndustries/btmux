
/*
 * $Id: mech.ood.c,v 1.1.1.1 2005/01/11 21:18:20 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Thu Feb 27 18:36:49 1997 fingon
 * Last modified: Thu Jul  9 02:08:18 1998 fingon
 *
 */

#include "mech.h"
#include "mech.events.h"
#include "p.mech.utils.h"
#include "p.mech.update.h"
#include "p.mech.restrict.h"
#include "p.template.h"

void mech_ood_damage(MECH * wounded, MECH * attacker, int damage)
{
    mech_notify(attacker, MECHALL,
	tprintf("%%cgYou hit the cocoon for %d points of damage!%%cn",
	    damage));
    mech_notify(wounded, MECHALL,
	tprintf
	("%%ch%%cyYour cocoon has been hit for %d points of damage!%%cn",
	    damage));
    MechCocoon(wounded) = MAX(0, MechCocoon(wounded) - damage);
    if (MechCocoon(wounded))
	return;
    /* Abort the OOD and initiate falling */
    if (MechZ(wounded) > MechElevation(wounded)) {
	if (MechJumpSpeed(wounded) >= MP1) {
	    mech_notify(wounded, MECHALL,
		"You initiate your jumpjets to compensate for the breached cocoon!");
	    MechCocoon(wounded) = -1;
	    return;
	}
	mech_notify(wounded, MECHALL,
	    "Your cocoon has been destroyed - have a nice fall!");
	MechLOSBroadcast(wounded,
	    "starts plummeting down, as the final blast blows the cocoon apart!");
	StopOOD(wounded);
	MECHEVENT(wounded, EVENT_FALL, mech_fall_event, FALL_TICK, -1);
    }
}

void mech_ood_event(MUXEVENT * e)
{
    MECH *mech = (MECH *) e->data;

    if (!OODing(mech))
	return;
    MarkForLOSUpdate(mech);
    if ((MechsElevation(mech) - DropGetElevation(mech)) > OOD_SPEED) {
	MechZ(mech) -= OOD_SPEED;
	MechFZ(mech) = MechZ(mech) * ZSCALE;
	MECHEVENT(mech, EVENT_OOD, mech_ood_event, OOD_TICK, 0);
	return;
    }
    /* Time to hit da ground */
    MechCocoon(mech) = 0;
    mech_notify(mech, MECHALL, "You land on the ground!");
    if (!MadePilotSkillRoll(mech, 4)) {
	if (MechType(mech) == CLASS_MECH) {
	    mech_notify(mech, MECHALL,
		"You are unable to control your momentum and fall on your face!");
	    MechLOSBroadcast(mech,
		"touches down on the ground, twists, and falls down!");
	} else {
	    mech_notify(mech, MECHALL,
		"You are unable to control your momentum and crash!");
	    MechLOSBroadcast(mech, "crashes at the ground!");

	}
	MechFalls(mech, 1, 0);
    } else
	MechLOSBroadcast(mech, "touches down!");
    DropSetElevation(mech, 1);
    if (!Fallen(mech))
	domino_space(mech, 2);
    if (WaterBeast(mech) && NotInWater(mech))
	MechDesiredSpeed(mech) = 0.0;
    MaybeMove(mech);
}

void initiate_ood(dbref player, MECH * mech, char *buffer)
{
    char *args[4];
    int x, y, z = ORBIT_Z, argc;

    DOCHECK((argc =
	    mech_parseattributes(buffer, args, 3)) < 2,
	"Invalid attributes!");
    DOCHECK(Readnum(x, args[0]), "Invalid number! (x)");
    DOCHECK(Readnum(y, args[1]), "Invalid number! (y)");
    if (argc == 3)
	DOCHECK(Readnum(z, args[2]), "Invalid number! (z)");
    DOCHECK(OODing(mech), "OOD already in progress!");
    mech_Rsetxy(GOD, (void *) mech, tprintf("%d %d", x, y));
    DOCHECK(MechX(mech) != x || MechY(mech) != y, "Invalid co-ordinates!");
    DOCHECK(Fallen(mech), "You'll have to get up first.");
    DOCHECK(Digging(mech), "You're too busy digging in.");
    MechZ(mech) = z;
    MechFZ(mech) = ZSCALE * MechZ(mech);
    MarkForLOSUpdate(mech);
    notify(player, "OOD initiated.");
    if (FlyingT(mech)) {
	MechStatus(mech) &= ~LANDED;
	MechDesiredSpeed(mech) = MechMaxSpeed(mech) / 2;
	if (is_aero(mech))
	    MechDesiredAngle(mech) = 0;
	MaybeMove(mech);
    } else {
	MechCocoon(mech) = MechRTons(mech) / 5 / 1024 + 1;
	StopMoving(mech);
	MECHEVENT(mech, EVENT_OOD, mech_ood_event, OOD_TICK, 0);
    }
}
