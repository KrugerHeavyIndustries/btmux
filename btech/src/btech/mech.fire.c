/*
 * Author: Cord Awtry <kipsta@mediaone.net>
 *
 *  Copyright (c) 2001-2002 Cord Awtry
 *       All rights reserved
 */

#include "mech.h"
#include "mech.events.h"
#include "p.mech.fire.h"
#include "p.mech.combat.h"
#include "p.mech.damage.h"
#include "p.mech.hitloc.h"
#include "p.mech.utils.h"
#include "p.mech.build.h"

#define VEHICLEBURN_TICK 60
#define VEHICLE_EXTINGUISH_TICK 120

static void inferno_end_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;

	MechCritStatus(mech) &= ~JELLIED;
	mech_notify(mech, MECHALL,
				"You feel suddenly far cooler as the fires finally die.");
}

void inferno_burn(MECH * mech, int time)
{
	int l;

	if(!(MechCritStatus(mech) & JELLIED)) {
		MechCritStatus(mech) |= JELLIED;
		MECHEVENT(mech, EVENT_BURN, inferno_end_event, time, 0);
		return;
	}

	l = muxevent_last_type_data(EVENT_BURN, (void *) mech) + time;
	muxevent_remove_type_data(EVENT_BURN, (void *) mech);
	MECHEVENT(mech, EVENT_BURN, inferno_end_event, l, 0);
}

static void vehicle_burn_event(MUXEVENT * objEvent)
{
	MECH *objMech = (MECH *) objEvent->data;	/* get the mech */
	int wLoc = (int) objEvent->data2;	/* and now the loc to damage */
	int wDamRoll = Number(1, 6);	/* do 1d6 damage */
	char strLocName[30];

	if(!objMech)
		return;

	ArmorStringFromIndex(wLoc, strLocName, MechType(objMech),
						 MechMove(objMech));

	if(!GetSectInt(objMech, wLoc))	/* if our loc is gone, no damage to do */
		return;

	mech_printf(objMech, MECHALL,
				"%%cr%%chYour %s takes damage from the fire!%%cn",
				strLocName);
	DamageMech(objMech, objMech, 0, -1, wLoc, 0, 0, wDamRoll, 0, 0, 0, -1,
			   0, 1);

	/*
	 * Only continue the event if the damage was greater than one
	 */
	if((wDamRoll > 1) && (GetSectInt(objMech, wLoc)))
		MECHEVENT(objMech, EVENT_VEHICLEBURN, vehicle_burn_event,
				  VEHICLEBURN_TICK, wLoc);
	else {
		if(GetSectInt(objMech, wLoc))
			mech_printf(objMech, MECHALL,
						"The fire burning on your %s finally goes out.",
						strLocName);
		if(!Burning(objMech))
			MechLOSBroadcast(objMech, "is no longer engulfed in flames.");
	}
}

void vehicle_start_burn(MECH * objMech, MECH * objAttacker)
{
	int wIter;
	int wDamage = 0;
	char strLocName[30];

	if(!objAttacker)
		objAttacker = objMech;

	mech_notify(objMech, MECHALL, "You catch on fire!");
	MechLOSBroadcast(objMech, "catches on fire!");

	for(wIter = 0; wIter < NUM_SECTIONS; wIter++) {
		if(GetSectInt(objMech, wIter) && !BurningSide(objMech, wIter)) {
			wDamage = Number(1, 6);
			ArmorStringFromIndex(wIter, strLocName, MechType(objMech),
								 MechMove(objMech));
			mech_printf(objMech, MECHALL,
						"Your %s catches on fire!", strLocName);

			DamageMech(objMech, objAttacker, 0, -1, wIter, 0, 0, wDamage,
					   0, 0, 0, -1, 0, 1);
			MECHEVENT(objMech, EVENT_VEHICLEBURN, vehicle_burn_event,
					  VEHICLEBURN_TICK, wIter);
		}
	}
}

void vehicle_extinquish_fire_event(MUXEVENT * e)
{
	MECH *objMech = (MECH *) e->data;

	if(!objMech)
		return;

	if(!Burning(objMech))
		return;

	StopBurning(objMech);

	mech_notify(objMech, MECHALL, "You manage to dowse the fire.");
	MechLOSBroadcast(objMech, "is no longer engulfed in flames.");
}

void vehicle_extinquish_fire(dbref player, MECH * mech, char *buffer)
{
	cch(MECH_USUALS);

	DOCHECK(Started(mech),
			"Your tank is started! You can not extinguish the flames while your tank is started!");
	DOCHECK(!Burning(mech), "This unit is not on fire!");
	DOCHECK(Extinguishing(mech),
			"You're already trying to put out the fire!");

	mech_notify(mech, MECHALL, "You begin to extinguish the fires!");

	MECHEVENT(mech, EVENT_VEHICLE_EXTINGUISH,
			  vehicle_extinquish_fire_event, VEHICLE_EXTINGUISH_TICK, 0);
}

/* 
 *  Mechs entering level 2 water, or proning in level 1 water should
 *  extinguish any inferno currently burning.
 */
void water_extinguish_inferno(MECH * mech)
{
	int elev = MechElevation(mech);
	MAP *map = getMap(mech->mapindex);

	if(!InWater(mech) || MechType(mech) != CLASS_MECH ||
	   !Jellied(mech) || (elev == -1 && !Fallen(mech)))
		return;

	muxevent_remove_type_data(EVENT_BURN, (void *) mech);
	MechCritStatus(mech) &= ~JELLIED;

	mech_notify(mech, MECHALL, "The flames extinguish in a roar of steam!");
	MechLOSBroadcast(mech,
					 "is surrounded by a plume of steam as the flames extinguish.");

	/* According to FASA, the inferno jelly should keep on burning on the
	 * water hex. We'll just add some steam (smoke) instead. */
	add_decoration(map, MechX(mech), MechY(mech), TYPE_SMOKE, SMOKE, 120);
}

void checkVehicleInFire(MECH * objMech, int fromHexFire)
{
	int wRoll = Roll();
	int wIter;
	int wDamage = 0;

	switch (MechMove(objMech)) {
	case MOVE_WHEEL:
	case MOVE_VTOL:
		wRoll += 2;
		break;
	case MOVE_HOVER:
		wRoll += 4;
		break;
	}

	if(wRoll < 8)				/* don't do jack if it's < 8 */
		return;

	if(fromHexFire)
		mech_notify(objMech, MECHALL,
					"%cr%chYou drive through a wall of searing flames!%cn");
	else
		mech_notify(objMech, MECHALL,
					"%cr%chThe fires surround your vehicle!%cn");

	switch (wRoll) {
	case 8:					/* roll once on the motive system chart */
	case 9:
		if(MechType(objMech) == CLASS_VTOL) {
			/*
			 * VTOLs _should_ make a pskill or go up one level... not right now tho
			 */
		} else {
			mech_notify(objMech, MECHALL,
						"%cr%chThe fire damages your motive system!%cn");
			DoMotiveSystemHit(objMech, 0);
		}
		break;

	case 10:
	case 11:
		/*
		 * Do 1d6 damage to each loc
		 */
		mech_notify(objMech, MECHALL,
					"%cr%chThe fire sweeps across your unit damaging it!%cn");

		for(wIter = 0; wIter < NUM_SECTIONS; wIter++) {
			wDamage = Number(1, 6);

			if(GetSectInt(objMech, wIter))
				DamageMech(objMech, objMech, 0, -1, wIter, 0, 0, wDamage,
						   0, 0, 0, -1, 0, 1);
		}
		break;

	default:
		vehicle_start_burn(objMech, objMech);
		break;
	}
}
