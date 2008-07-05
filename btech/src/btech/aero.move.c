/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 */

#define MIN_TAKEOFF_SPEED 3

#include <math.h>
#include "muxevent.h"
#include "mech.h"
#include "mech.events.h"
#include "p.mech.sensor.h"
#include "p.mech.update.h"
#include "p.artillery.h"
#include "p.mech.combat.h"
#include "p.mech.combat.misc.h"
#include "p.mech.utils.h"
#include "p.econ_cmds.h"
#include "p.mech.ecm.h"
#include "p.mech.tag.h"
#include "p.mech.lite.h"
#include "spath.h"
#include "p.mine.h"

struct land_data_type {
	int type;
	int maxvertup;
	int maxvertdown;
	int minhoriz;
	int maxhoriz;
	int launchvert;
	int launchtime;				/* In secs */
	char *landmsg;
	char *landmsg_others;
	char *takeoff;
	char *takeoff_others;
}								/*            maxvertup / maxvertdown / minhoriz / maxhoriz / launchv /

								   launchtime */ land_data[] = {
	{
	CLASS_VTOL, 10, -60, -15, 15, 5, 0,
			"You bring your VTOL to a safe landing.", "lands.",
			"The rotor whines overhead as you lift off into the sky.",
			"takes off!"}, {
	CLASS_AERO, 10, -30, MIN_TAKEOFF_SPEED * MP1, 999, 20, 10,
			"You land your AeroFighter safely.", "lands safely.",
			"The Aerofighter launches into the air!",
			"launches into the air!"}, {
	CLASS_DS, 10, -25, MIN_TAKEOFF_SPEED * MP1, 999, 20, 300,
			"The DropShip lands safely.", "lands safely.",
			"The DropShip's nose lurches upward, and it starts climbing to the sky!",
			"starts climbing to the sky!"}, {
	CLASS_SPHEROID_DS, 15, -40, -40, 40, 20, 300,
			"The DropShip touches down safely.",
			"touches down, and settles.",
			"The DropShip slowly lurches upwards as engines battle the gravity..",
			"starts climbing up to the sky!"}
};

#define NUM_LAND_TYPES (sizeof(land_data)/sizeof(struct land_data_type))

static void aero_takeoff_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;
	MAP *map = getMap(mech->mapindex);
	int i = -1;
	int count = (int) e->data2;

	if(IsDS(mech))
		for(i = 0; i < NUM_LAND_TYPES; i++)
			if(MechType(mech) == land_data[i].type)
				break;
	if(count > 0) {
		if(count > 5) {
			if(!(count % 10))
				mech_printf(mech, MECHALL, "Launch countdown: %d.", count);
		} else
			mech_printf(mech, MECHALL, "Launch countdown: %d.", count);
		if(i >= 0) {
			if(count == (land_data[i].launchtime / 4))
				DSSpam_O(mech,
						 "'s engines start to glow with unbearable intensity..");
			switch (count) {
			case 10:
				DSSpam_O(mech, "'s engines are almost ready to lift off!");
				break;
			case 6:
				DSSpam_O(mech, "'s engines generate a tremendous heat wave!");
				ScrambleInfraAndLiteAmp(mech, 2, 0,
										"The blinding flash of light momentarily blinds you!",
										"The blinding flash of light momentarily blinds you!");
				break;
			case 2:
				mech_notify(mech, MECHALL,
							"The engines pulse out a stream of superheated plasma!");
				DSSpam_O(mech,
						 "'s engines send forth a tremendous stream of superheated plasma!");
				ScrambleInfraAndLiteAmp(mech, 4, 0,
										"The blinding flash of light blinds you!",
										"The blinding flash of light blinds you!");
				break;
			case 1:
				DS_BlastNearbyMechsAndTrees(mech,
											"You receive a direct hit!",
											"is caught in the middle of the inferno!",
											"You are hit by the wave!",
											"gets hit by the wave!",
											"are instantly burned to ash!",
											400);
				break;
			}
		}
		MECHEVENT(mech, EVENT_TAKEOFF, aero_takeoff_event, 1, (void *)
				  (count - 1));
		return;
	}
	if(i < 0) {
		if(RollingT(mech) && MechSpeed(mech) < (MIN_TAKEOFF_SPEED * MP1)) {
			mech_notify(mech, MECHALL,
						"You're moving too slowly to lift off!");
			return;
		}
		for(i = 0; i < NUM_LAND_TYPES; i++)
			if(MechType(mech) == land_data[i].type)
				break;
	}
	StopSpinning(mech);
	mech_notify(mech, MECHALL, land_data[i].takeoff);
	MechLOSBroadcast(mech, land_data[i].takeoff_others);
	MechStartFX(mech) = 0;
	MechStartFY(mech) = 0;
	MechStartFZ(mech) = 0;
	if(IsDS(mech))
		SendDSInfo(tprintf("DS #%d has lifted off at %d %d "
						   "on map #%d", mech->mynum, MechX(mech),
						   MechY(mech), map->mynum));
	if(MechCritStatus(mech) & HIDDEN) {
		mech_notify(mech, MECHALL, "You move too much and break your cover!");
		MechLOSBroadcast(mech, "breaks its cover in the brush.");
		MechCritStatus(mech) &= ~(HIDDEN);
	}
	if(MechType(mech) != CLASS_VTOL) {
		MechDesiredAngle(mech) = 90;
		MechDesiredSpeed(mech) = MechMaxSpeed(mech) * 2 / 3;
	} else {
		MechSpeed(mech) = 0;
		MechDesiredSpeed(mech) = 0;
		MechVerticalSpeed(mech) = 60.0;
	}
	ContinueFlying(mech);
	MaybeMove(mech);
}

void aero_takeoff(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MAP *map = getMap(mech->mapindex);
	int i, j;

	for(i = 0; i < NUM_LAND_TYPES; i++)
		if(MechType(mech) == land_data[i].type)
			break;

	if((j = atoi(buffer)))
		DOCHECK(!WizP(player), "Insufficient access!");

	DOCHECK(TakingOff(mech),
			"The launch sequence has already been initiated!");
	DOCHECK(i == NUM_LAND_TYPES, "This vehicle type cannot takeoff!");
	cch(MECH_USUAL);
	DOCHECK(Fortified(mech), "Your fortified state prevents you from moving.");
	DOCHECK(!FlyingT(mech),
			"Only VTOL, Aerospace fighters and Dropships can take off.");
	DOCHECK(!Landed(mech), "You haven't landed!");
	if(Fallen(mech) || (MMaxSpeed(mech) <= MP1) ||
	   ((SectIsDestroyed(mech, ROTOR)) && MechType(mech) == CLASS_VTOL)) {
		DOCHECK(MechType(mech) == CLASS_VTOL, "The rotor's dead!");
		notify(player, "The engines are dead!");
		return;
	}
	if(!AeroFreeFuel(mech) && AeroFuel(mech) < 1) {
		DOCHECK(MechType(mech) == CLASS_VTOL, "Your VTOL's out of fuel!");
		notify(player,
			   "Your craft's out of fuel! No taking off until it's refueled.");
		return;
	}
	DOCHECK(MechType(mech) == CLASS_AERO &&
			MechSpeed(mech) < (MIN_TAKEOFF_SPEED * MP1),
			"You're moving too slowly to take off!");
	DOCHECK(MapIsUnderground(map),
			"Realize the ceiling in this grotto is a bit to low for that!");
	if(land_data[i].launchtime > 0)
		mech_notify(mech, MECHALL,
					"Launch sequence initiated.. type 'land' to abort it.");
	DSSpam(mech, "starts warming engines for liftoff!");
	if(IsDS(mech))
		SendDSInfo(tprintf("DS #%d has started takeoff at %d %d on map #%d",
						   mech->mynum, MechX(mech), MechY(mech),
						   map->mynum));
	if(MechCritStatus(mech) & HIDDEN) {
		mech_notify(mech, MECHALL, "You break your cover to takeoff!");
		MechLOSBroadcast(mech, "breaks its cover as it begins takeoff.");
		MechCritStatus(mech) &= ~(HIDDEN);
	}
	StopHiding(mech);
	MECHEVENT(mech, EVENT_TAKEOFF, aero_takeoff_event, 1, (void *)
			  j ? j : land_data[i].launchtime);
}

#define NUM_NEIGHBORS 7

void DS_BlastNearbyMechsAndTrees(MECH * mech, char *hitmsg, char *hitmsg1,
								 char *nearhitmsg, char *nearhitmsg1,
								 char *treehitmsg, int damage)
{
	MAP *map = getMap(mech->mapindex);
	int x = MechX(mech), y = MechY(mech), z = MechZ(mech);
	int x1, y1, x2, y2, d;
	int rng = (damage > 100 ? 5 : 3);

	for(x1 = x - rng; x1 <= (x + rng); x1++)
		for(y1 = y - rng; y1 <= (y + rng); y1++) {
			x2 = BOUNDED(0, x1, map->map_width - 1);
			y2 = BOUNDED(0, y1, map->map_height - 1);
			if(x1 != x2 || y1 != y2)
				continue;
			if((d = MyHexDist(x, y, x1, y1, 0)) > rng)
				continue;
			d = MAX(1, d);
			switch (GetRTerrain(map, x1, y1)) {
			case LIGHT_FOREST:
			case HEAVY_FOREST:
				if(!find_decorations(map, x1, y1)) {
					HexLOSBroadcast(map, x1, y1,
									tprintf("%%ch%%crThe trees in $h %s%%cn",
											treehitmsg));
					if((damage / d) > 100) {
						SetTerrain(map, x1, y1, ROUGH);
					} else {
						add_decoration(map, x1, y1, TYPE_FIRE, FIRE,
									   FIRE_DURATION);
					}
				}
				break;
			}
		}
	MechZ(mech) = z + 6;
	blast_hit_hexesf(map, damage, 5, damage / 2, MechFX(mech),
					 MechFY(mech), MechFX(mech), MechFY(mech), hitmsg,
					 hitmsg1, nearhitmsg, nearhitmsg1, 0, 4, 4, 1, rng);
	MechZ(mech) = z;
}

enum {
	NO_ERROR, INVALID_TERRAIN, UNEVEN_TERRAIN, BLOCKED_LZ
};

char *reasons[] = {
	"Improper terrain",
	"Uneven ground",
	"Blocked landing zone"
};

static int improper_lz_status;
static int improper_lz_height;

static void ImproperLZ_callback(MAP * map, int x, int y)
{
	if(Elevation(map, x, y) != improper_lz_height)
		improper_lz_status = 0;
	else
		improper_lz_status++;
}

#define MechCheckLZ(m)	ImproperLZ((m), MechX((m)), MechY((m)))

int ImproperLZ(MECH * mech, int x, int y)
{
	MAP *map = getMap(mech->mapindex);

	if(GetRTerrain(map, x, y) != GRASSLAND && GetRTerrain(map, x, y) != ROAD)
		return INVALID_TERRAIN;

	improper_lz_status = 0;
	improper_lz_height = Elevation(map, x, y);
	visit_neighbor_hexes(map, x, y, ImproperLZ_callback);

	if(improper_lz_status != 6)
		return UNEVEN_TERRAIN;
	if(is_blocked_lz(mech, map, x, y))
		return BLOCKED_LZ;
	return NO_ERROR;
}

void aero_land(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	MAP *map = getMap(mech->mapindex);
	int i, t;
	double horiz = 0.0;
	int vert, vertmin = 0, vertmax = 0;

	DOCHECK(MechType(mech) != CLASS_VTOL &&
			MechType(mech) != CLASS_AERO &&
			!IsDS(mech), "You can't land this type of vehicle.");
	DOCHECK(MechType(mech) == CLASS_VTOL &&
			AeroFuel(mech) <= 0 &&
			!AeroFreeFuel(mech), "You lack fuel to maneuver for landing!");

	for(i = 0; i < NUM_LAND_TYPES; i++)
		if(MechType(mech) == land_data[i].type)
			break;
	if(i == NUM_LAND_TYPES)
		return;
	DOCHECK((Fallen(mech)) &&
			(MechType(mech) == CLASS_VTOL), "The rotor's dead!");
	DOCHECK((Fallen(mech)) &&
			(MechType(mech) != CLASS_VTOL), "The engines are dead!");
	if(MechStatus(mech) & LANDED) {
		if(TakingOff(mech)) {
			mech_printf(mech, MECHALL, "Launch aborted by %s.", Name(player));
			if(IsDS(mech))
				SendDSInfo(tprintf("DS #%d aborted takeoff at %d %d "
								   "on map #%d", mech->mynum, MechX(mech),
								   MechY(mech), map->mynum));
			StopTakeOff(mech);
			return;
		}
		notify(player, "You're already landed!");
		return;
	}
	DOCHECK(MechZ(mech) > MechElevation(mech) + 1,
			"You are too high to land here.");
	DOCHECK(((horiz =
			  my_sqrtm((double) MechDesiredSpeed(mech),
					   (double) MechVerticalSpeed(mech))) >=
			 ((double) 1.0 + land_data[i].maxhoriz)),
			"You're moving too fast to land.");
	DOCHECK(horiz < land_data[i].minhoriz,
			"You're moving too slowly to land.");
	DOCHECK(((vert = MechVerticalSpeed(mech)) > (vertmax =
												 land_data[i].maxvertup)) ||
			(MechVerticalSpeed(mech) < (vertmin =
										land_data[i].maxvertdown)),
			"You are moving too fast to land. ");
	if(MechSpeed(mech) < land_data[i].minhoriz) {
		if(MechStartFZ(mech) <= 0)
			notify(player,
				   "You're falling, not landing! Pick up some horizontal speed first.");
		else
			notify(player, "You're climbing not landing!");
		return;
	}
	t = MechRTerrain(mech);
	DOCHECK(!(t == GRASSLAND || t == ROAD || (MechType(mech) == CLASS_VTOL
											  && t == BUILDING)),
			"You can't land on this type of terrain.");
	if(MechType(mech) != CLASS_VTOL && MechCheckLZ(mech)) {
		mech_notify(mech, MECHALL, "This location is no good for landing!");
		return;
	}
	if(IsDS(mech))
		SendDSInfo(tprintf("DS #%d has landed at %d %d on map #%d",
						   mech->mynum, MechX(mech), MechY(mech),
						   map->mynum));

	mech_notify(mech, MECHALL, land_data[i].landmsg);
	MechLOSBroadcast(mech, land_data[i].landmsg_others);
	MechZ(mech) = MechElevation(mech);
	MechFZ(mech) = ZSCALE * MechZ(mech);
	MechStatus(mech) |= LANDED;
	MechSpeed(mech) = 0.0;
	MechVerticalSpeed(mech) = 0.0;
	MechStartFX(mech) = 0.0;
	MechStartFY(mech) = 0.0;
	MechStartFZ(mech) = 0.0;
	possible_mine_poof(mech, MINE_LAND);
}

void aero_ControlEffect(MECH * mech)
{
	if(Spinning(mech))
		return;
	if(Destroyed(mech))
		return;
	if(Landed(mech))
		return;
	mech_notify(mech, MECHALL, "You lose control of your craft!");
	MechLOSBroadcast(mech, "spins out of control!");
	StartSpinning(mech);
	MechStartSpin(mech) = mudstate.now;
}

void ds_BridgeHit(MECH * mech)
{
	/* Implementation: Kill all players on bridge :-) */
	if(Destroyed(mech))
		return;
	if(In_Character(mech->mynum))
		mech_notify(mech, MECHALL,
					"SHIT! The shot seems to be coming straight for the bridge!");
	KillMechContentsIfIC(mech->mynum);
}

#define degsin(a) ((double) sin((double) (a) * M_PI / 180.0))
#define degcos(a) ((double) cos((double) (a) * M_PI / 180.0))

void aero_UpdateHeading(MECH * mech)
{
	/* Heading things are done in speed now, odd as though it might 
	   seem */
	if(SpheroidDS(mech))
		UpdateHeading(mech);
}

double length_hypotenuse(double x, double y)
{
	if(x < 0)
		x = -x;
	if(y < 0)
		y = -y;
	return sqrt(x * x + y * y);
}

double my_sqrtm(double x, double y)
{
	double d;

	if(x < 0)
		x = -x;
	if(y < 0)
		y = -y;
	if(y > x) {
		d = y;
		y = x;
		x = d;
	}
	return sqrt(x * x - y * y);
}

#define AERO_BONUS 3

void aero_UpdateSpeed(MECH * mech)
{
	float xypart;
	float wx, wy, wz;
	float nx, ny, nz;
	float nh;
	float dx, dy, dz;
	float vlen, mod;
	float sp;
	float ab = 0.7;
	float m = 1.0;

	if(Spinning(mech)) {
		MechDesiredSpeed(mech) =
			BOUNDED(0, MechDesiredSpeed(mech) + Number(1, 10),
					MMaxSpeed(mech));
		MechDesiredAngle(mech) =
			MAX(-90, MechDesiredAngle(mech) - Number(1, 15));
		MechDesiredFacing(mech) =
			AcceptableDegree(MechDesiredFacing(mech) + Number(-3, 3));
	}
	wz = MechDesiredSpeed(mech) * degsin(MechDesiredAngle(mech));
	if(MechType(mech) == CLASS_AERO)
		ab = 2.5;
	if(MechZ(mech) < ATMO_Z)
		ab = ab / 2;
	/* First, we calculate the vector we want to be going */
	xypart = MechDesiredSpeed(mech) * degcos(MechDesiredAngle(mech));
	if(AeroFuel(mech) < 0) {
		wz = wz / 5.0;
		xypart = xypart / 5.0;
	}
	if(xypart < 0)
		xypart = 0 - xypart;
	m = ACCEL_MOD;
	FindComponents(m * xypart, MechDesiredFacing(mech), &wx, &wy);
	wz = wz * m;

	/* Then, we calculate the present heading / speed */
	nx = MechStartFX(mech);
	ny = MechStartFY(mech);
	nz = MechStartFZ(mech);

	/* Ok, we've present heading / speed */
	/* Next, we make vector from n[xyz] -> w[xyz] */
	dx = wx - nx;
	dy = wy - ny;
	dz = wz - nz;
	vlen = length_hypotenuse(length_hypotenuse(dx, dy), dz);
	if(!(vlen > 0.0))
		return;
	if(vlen > (m * ab * MMaxSpeed(mech) / AERO_SECS_THRUST)) {
		mod = (float) ab *m * MMaxSpeed(mech) / AERO_SECS_THRUST / vlen;

		dx *= mod;
		dy *= mod;
		dz *= mod;
		/* Ok.. we've a new modified speed vector */
	}
	nx += dx;
	ny += dy;
	nz += dz;
	/* Then, we need to calculate present heading / speed / verticalspeed */
	nh = (float) atan2(ny, nx) / TWOPIOVER360;
	if(!SpheroidDS(mech))
		SetFacing(mech, AcceptableDegree((int) nh + 90));
	xypart = length_hypotenuse(nx, ny);
	MechSpeed(mech) = xypart;
	sp = length_hypotenuse(length_hypotenuse(nx, ny), nz);	/* Whole speed */
	MechVerticalSpeed(mech) = nz;
	if(!SpheroidDS(mech) && fabs(MechSpeed(mech)) < MP1)
		SetFacing(mech, MechDesiredFacing(mech));
	MechStartFX(mech) = nx;
	MechStartFY(mech) = ny;
	MechStartFZ(mech) = nz;
}

int FuelCheck(MECH * mech)
{
	int fuelcost = 1;

	/* We don't do anything particularly nasty to shutdown things */
	if(!Started(mech))
		return 0;
	if(AeroFreeFuel(mech))
		return 0;
	if(fabs(MechSpeed(mech)) > MMaxSpeed(mech)) {
		if(MechZ(mech) < ATMO_Z)
			fuelcost = abs(MechSpeed(mech) / MMaxSpeed(mech));
	} else if(fabs(MechSpeed(mech)) < MP1 &&
			  fabs(MechVerticalSpeed(mech)) < MP2)
		if(Number(0, 1) == 0)
			return 0;			/* Approximately half of the time free */
	if(AeroFuel(mech) > 0) {
		if(AeroFuel(mech) <= fuelcost)
			AeroFuel(mech) = 0;
		else
			AeroFuel(mech) -= fuelcost;
		return 0;
	}
	/* DropShips do not need crash ; they switch to (VERY SLOW) secondary
	   power source. */
	if(IsDS(mech)) {
		if(AeroFuel(mech) < 0)
			return 0;
		AeroFuel(mech)--;
		mech_notify(mech, MECHALL,
					"As the fuel runs out, the engines switch to backup power.");
		return 0;
	}
	if(AeroFuel(mech) < 0)
		return 1;
	/* Now, the true nastiness begins ;) */
	AeroFuel(mech)--;
	if(!(AeroFuel(mech) % 100) && AeroFuel(mech) >= AeroFuelOrig(mech))
		SetCargoWeight(mech);
	if(MechType(mech) == CLASS_VTOL) {
		MechLOSBroadcast(mech, "'s rotors suddenly stop!");
		mech_notify(mech, MECHALL, "The sound of rotors slowly stops..");
	} else {
		MechLOSBroadcast(mech, "'s engines die suddenly..");
		mech_notify(mech, MECHALL, "Your engines die suddenly..");
	}
	MechSpeed(mech) = 0.0;
	MechDesiredSpeed(mech) = 0.0;
	if(!Landed(mech)) {
		mech_notify(mech, MECHALL,
					"You ponder F = ma, S = F/m, S = at^2 => S=agt^2 in relation to the ground..");
		/* Start free-fall */
		MechVerticalSpeed(mech) = 0;
		/* Hmm. This _can_ be ugly if things crash in middle of fall. Oh well. */
		mech_notify(mech, MECHALL, "You start free-fall.. Enjoy the ride!");
		MECHEVENT(mech, EVENT_FALL, mech_fall_event, FALL_TICK, -1);
	}
	return 1;
}

void aero_update(MECH * mech)
{
	if(Destroyed(mech))
		return;
	if(Started(mech) || Uncon(mech)) {
		UpdatePilotSkillRolls(mech);
	}
	if(Started(mech) || MechPlusHeat(mech) > 0.)
		UpdateHeat(mech);
	if(!(mudstate.now / 3 % 5)) {
		if(!Spinning(mech))
			return;
		if(Destroyed(mech))
			return;
		if(Landed(mech))
			return;
		if(MadePilotSkillRoll(mech,
							  (MechStartSpin(mech) - mudstate.now) / 15 +
							  8)) {
			mech_notify(mech, MECHALL, "You recover control of your craft.");
			StopSpinning(mech);
		}
	}
	if(Started(mech))
		MechVisMod(mech) =
			BOUNDED(0, MechVisMod(mech) + Number(-40, 40), 100);
	checkECM(mech);
	checkTAG(mech);
	end_lite_check(mech);
}

void aero_thrust(dbref player, void *data, char *arg)
{
	MECH *mech = (MECH *) data;
	char *args[1];
	float newspeed, maxspeed;

	DOCHECK(Landed(mech), "You're landed!");
	DOCHECK(is_aero(mech) && Spinning(mech) &&
			!Landed(mech),
			"You are unable to control your craft at the moment.");
	if(mech_parseattributes(arg, args, 1) != 1) {
		notify_printf(player, "Your current thrust is %.2f.",
					  MechDesiredSpeed(mech));
		return;
	}
	newspeed = atof(args[0]);
	if(RollingT(mech))
		DOCHECK(newspeed < (MP1 * MIN_TAKEOFF_SPEED / ACCEL_MOD),
				tprintf("Minimum thrust you stay in air with is %.1f kph.",
						(float) MP1 * MIN_TAKEOFF_SPEED / ACCEL_MOD));
	maxspeed = MMaxSpeed(mech);
	if(!(maxspeed > 0.0))
		maxspeed = 0.0;
	DOCHECK(Fallen(mech), "Your engine's dead, no way to thrust!");
	DOCHECK(newspeed < 0,
			"Doh, thrust backwards.. where's your sense of adventure?");
	if(newspeed > maxspeed) {
		notify_printf(player, "Maximum thrust: %.2f (%.2f kb/sec2)",
					  maxspeed, maxspeed / 10);
		return;
	}
	MechDesiredSpeed(mech) = newspeed;
	mech_printf(mech, MECHALL, "Thrust set to %.2f.", newspeed);
	MaybeMove(mech);
}

void aero_vheading(dbref player, void *data, char *arg, int flag)
{
	char *args[1];
	int i = 0;
	MECH *mech = (MECH *) data;

	if(mech_parseattributes(arg, args, 1) != 1) {
		notify_printf(player, "Present angle: %d degrees.",
					  MechDesiredAngle(mech));
		return;
	}
	i = flag * atoi(args[0]);
	if(abs(i) > 90)
		i = 90 * flag;
	DOCHECK(abs(i) != 90 && MechZ(mech) < ATMO_Z &&
			SpheroidDS(mech), tprintf("You can go only up / down at <%d z!",
									  ATMO_Z));
	if(i >= 0)
		mech_printf(mech, MECHALL, "Climbing angle set to %d degrees.", i);
	else
		mech_printf(mech, MECHALL, "Diving angle set to %d degrees.", 0 - i);
	MechDesiredAngle(mech) = i;
}

void aero_climb(dbref player, MECH * mech, char *arg)
{
	aero_vheading(player, mech, arg, 1);
}

void aero_dive(dbref player, MECH * mech, char *arg)
{
	aero_vheading(player, mech, arg, -1);
}

static char *colorstr(int serious)
{
	if(serious == 1)
		return "%ch%cr";
	if(serious == 0)
		return "%ch%cy";
	return "";
}

void DS_LandWarning(MECH * mech, int serious)
{
	int ilz = MechCheckLZ(mech);

	if(!ilz)
		return;
	ilz--;
	mech_printf(mech, MECHALL, "%sWARNING: %s - %s%%cn",
				colorstr(serious), reasons[ilz],
				serious == 1 ? "CLIMB UP NOW!!!" : serious ==
				0 ? "No further descent is advisable." :
				"Please do not even consider landing here.");
}

void aero_checklz(dbref player, MECH * mech, char *buffer)
{
	int ilz, argc;
	char *args[3];
	int x, y;

	cch(MECH_USUAL);

	argc = mech_parseattributes(buffer, args, 3);
	switch (argc) {
	case 2:
		x = atoi(args[0]);
		y = atoi(args[1]);
		if(!MechIsObservator(mech)) {
			float fx, fy;
			MapCoordToRealCoord(x, y, &fx, &fy);
			DOCHECK(FindHexRange(MechFX(mech), MechFY(mech), fx, fy) >
					MechTacRange(mech), "Out of range!");
		}
		break;
	case 0:
		x = MechX(mech);
		y = MechY(mech);
		break;
	default:
		notify(player, "Invalid number of parameters!");
		return;
	}

	ilz = ImproperLZ(mech, x, y);
	DOCHECKMA(!ilz,
			  tprintf("The hex (%d,%d) looks good enough for a landing.",
					  x, y));
	ilz--;
	mech_printf(mech, MECHALL,
				"The hex (%d,%d) doesn't look good for landing: %s.",
				x, y, reasons[ilz]);
}
