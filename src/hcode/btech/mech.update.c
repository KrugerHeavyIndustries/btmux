/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "failures.h"
#include "mech.events.h"
#include "mech.ice.h"
#include "p.mech.ecm.h"
#include "p.mech.tag.h"
#include "p.mech.combat.h"
#include "p.mech.combat.misc.h"
#include "p.mech.damage.h"
#include "p.mech.fire.h"
#include "p.mech.startup.h"
#include "p.mech.utils.h"
#include "p.mech.update.h"
#include "p.btechstats.h"
#include "p.mech.physical.h"
#include "p.bsuit.h"
#include "p.mine.h"
#include "p.mech.lite.h"
#include "p.mech.pickup.h"

extern int arc_override;

int fiery_death(MECH * mech)
{
	if(MechTerrain(mech) == FIRE) {
		if(is_aero(mech))
			return 0;
		if(MechMove(mech) == MOVE_VTOL)
			return 0;
		if(Destroyed(mech))
			return 0;

		/* Cause various things */
		/* MWs _die_ */
		if(MechType(mech) == CLASS_MW) {
			mech_notify(mech, MECHALL, "You feel a tad bit too warm..");
			mech_notify(mech, MECHALL, "You faint.");
			DestroyMech(mech, mech, 0);
			return 1;
		}

		/* Tanks may die */
		return 0;				/* Dumb idea */
		heat_effect(NULL, mech, 0, 0);
		if(Destroyed(mech))
			return 1;
	}
	return 0;
}

int bridge_w_elevation(MECH * mech)
{
	return -1;
}

void bridge_set_elevation(MECH * mech)
{
	if(MechZ(mech) < (MechUpperElevation(mech) - MoveMod(mech))) {
		if(Overwater(mech))
			MechZ(mech) = 0;
		else
			MechZ(mech) = bridge_w_elevation(mech);
		return;
	}
	MechZ(mech) = MechUpperElevation(mech);
}

int DSOkToNotify(MECH * mech)
{
	if(DSLastMsg(mech) > muxevent_tick ||
	   (muxevent_tick - DSLastMsg(mech)) >= DS_SPAM_TIME) {
		DSLastMsg(mech) = muxevent_tick;
		return 1;
	}
	return 0;
}

enum {
	JUMP, WALK_WALL, WALK_DROP, HIT_UNDER_BRIDGE, WALK_BACK
};

int collision_check(MECH * mech, int mode, int le, int lt)
{
	int e;
	MAP *mech_map = getMap(mech->mapindex);

	if(Overwater(mech) && le < 0)
		le = 0;
	e = MechElevation(mech);
	if(MechRTerrain(mech) == ICE)
		if(le >= 0)
			e = 0;
	if(le < (MechUpperElevation(mech) - MoveMod(mech)) && lt == BRIDGE) {
		if(Overwater(mech))
			le = 0;
		else
			le = bridge_w_elevation(mech);
	}
	if(e < 0 && Overwater(mech))
		e = 0;
	switch (mode) {
	case JUMP:
		if(MechRTerrain(mech) == BRIDGE) {
			if(MechZ(mech) < 0)
				return 1;
			if(MechZ(mech) == (e - 1))
				return 1;
			return 0;
		} else
			return (MechZ(mech) < e);
	case WALK_DROP:
		return (le - e) > MoveMod(mech);
	case WALK_WALL:
		if((MechMove(mech) == MOVE_HOVER) &&
		   (MechRTerrain(mech) == BRIDGE) && (((lt == ICE) ||
											   (lt == WATER)) || ((le == 0)
																  && (lt ==
																	  BRIDGE))))
		{
			return 0;
		} else {
			return (e - le) > MoveMod(mech);
		}
	case WALK_BACK:
		if(MechMove(mech) != MOVE_TRACK && MechType(mech) != CLASS_VTOL)
			return (MechSpeed(mech) < 0 ? abs((le - e)) : 0);
	case HIT_UNDER_BRIDGE:		/* Hovers only... tho it should be fixed for foils and hulls too */
		if((lt == BRIDGE) && (MechRTerrain(mech) == BRIDGE) && (le == 0)
		   && (Elevation(mech_map, MechLastX(mech), MechLastY(mech)) != 0)
		   && (Elevation(mech_map, MechX(mech), MechY(mech)) == 1))
			return 1;
		else
			return 0;
	}
	return 0;
}

void CheckNavalHeight(MECH * mech, int oz);

void move_mech(MECH * mech)
{
	float newx = 0.0, newy = 0.0, dax, day;
	float xy_charge_dist, xscale;
	float jump_pos;

#ifdef ODDJUMP
	float rjump_pos, midmod;
#endif
	int x, y, upd_z = 0;
	int last_z = 0;
	MECH *target;
	MAP *mech_map;
	int oi, oz;
	int iced = 0;

	/* Buffer for printing messages so we don't use the tprintf
	 * function anymore */
	char message_buffer[MBUF_SIZE];

	oz = MechZ(mech);
	mech_map = getMap(mech->mapindex);

	if(!mech_map && MechPilot(mech) >= 0)
		mech_map = ValidMap(MechPilot(mech), mech->mapindex);

	/* Unit is not on a map so don't need to move it - instead
	 * reset its values and shut it down */
	if(!mech_map) {

		mech_notify(mech, MECHALL,
					"You are on an invalid map! Map index reset!");
		MechCocoon(mech) = 0;

		if(Jumping(mech))
			mech_land(MechPilot(mech), (void *) mech, "");

		mech_shutdown(MechPilot(mech), (void *) mech, "");

		snprintf(message_buffer, MBUF_SIZE,
				 "move_mech:invalid map:Mech: %d Index: %d", mech->mynum,
				 mech->mapindex);
		SendError(message_buffer);
		mech->mapindex = -1;
		return;
	}

	/* Is the unit on a valid spot on the map */
	if(MechX(mech) < 0 || MechX(mech) >= mech_map->map_width ||
	   MechLastX(mech) < 0 || MechLastX(mech) >= mech_map->map_width ||
	   MechY(mech) < 0 || MechY(mech) >= mech_map->map_height ||
	   MechLastY(mech) < 0 || MechLastY(mech) >= mech_map->map_height) {

		mech_notify(mech, MECHALL,
					"You are at an invalid map location! Map index reset!");
		MechCocoon(mech) = 0;

		if(Jumping(mech))
			mech_land(MechPilot(mech), (void *) mech, "");

		mech_shutdown(MechPilot(mech), (void *) mech, "");

		snprintf(message_buffer, MBUF_SIZE,
				 "move_mech:invalid map:Mech: %d Index: %d", mech->mynum,
				 mech->mapindex);
		SendError(message_buffer);

		mech->mapindex = -1;
		return;
	}

	/* Is the unit charging - and if so have they been charging to long */
	if(mudconf.btech_newcharge && MechChargeTarget(mech) > 0) {
		if(MechChargeTimer(mech)++ > CHARGE_TIMER_LIMIT) {
			mech_notify(mech, MECHALL, "Charge timed out, charge reset.");
			MechChargeTarget(mech) = -1;
			MechChargeTimer(mech) = 0;
			MechChargeDistance(mech) = 0;
		}
	}

	/*! \todo {now that I think about it maybe make a single block
	 * of code to check for jumping BEFORE anything else.} */

	/* Check the move type of the unit */
	switch (MechMove(mech)) {
	case MOVE_BIPED:
	case MOVE_QUAD:

		/* Biped/Quad Jumping */
		if(Jumping(mech)) {
			MarkForLOSUpdate(mech);
			FindComponents(JumpSpeed(mech, mech_map) * MOVE_MOD *
						   MAPMOVEMOD(mech_map), MechJumpHeading(mech), &newx,
						   &newy);
			MechFX(mech) += newx;
			MechFY(mech) += newy;
			jump_pos = length_hypotenuse((MechFX(mech) - MechStartFX(mech)),
										 (MechFY(mech) - MechStartFY(mech)));

			/*! \todo {Not sure what ODDJUMP is but need to figure it out, possibly
			 * make this a mudconf parameter} */
#ifndef ODDJUMP
			MechFZ(mech) = ((4 * JumpSpeedMP(mech, mech_map) * ZSCALE)
							/ (MechJumpLength(mech) * MechJumpLength(mech))) *
				jump_pos * (MechJumpLength(mech) - jump_pos) +
				MechStartFZ(mech) + jump_pos * (MechEndFZ(mech)
												-
												MechStartFZ(mech)) /
				(MechJumpLength(mech) * HEXLEVEL);
#else
			rjump_pos = MechJumpLength(mech) - jump_pos;
			if(rjump_pos < 0.0)
				rjump_pos = 0.0;

			/* New flight path: Make a direct line from the origin to
			   destination, and imagine a 1-x^4 in relation to the 0 as
			   the line. y=1 = JumpTop offset, x=0 = middle of path,
			   x=-1 = beginning, x=1 = end */
			midmod = jump_pos / MechJumpLength(mech);
			midmod = (midmod - 0.5) * 2;
			if(MechJumpTop(mech) >= (1 + JumpSpeedMP(mech, mech_map))) {
				midmod = (1.0 - (midmod * midmod)) * MechJumpTop(mech);
			} else {
				midmod = (1.0 - (midmod * midmod * midmod * midmod)) *
					MechJumpTop(mech);
			}

			MechFZ(mech) = (rjump_pos * MechStartFZ(mech) +
							jump_pos * MechEndFZ(mech)) /
				MechJumpLength(mech) + midmod * ZSCALE;
#endif

			MechZ(mech) = (int) (MechFZ(mech) / ZSCALE + 0.5);

#ifdef JUMPDEBUG
			snprintf(message_buffer, MBUF_SIZE,
					 "#%d: %d, %d, %d (%d, %d, %d)",
					 mech->mynum, MechX(mech), MechY(mech), MechZ(mech),
					 (int) MechFX(mech), (int) MechFY(mech),
					 (int) MechFZ(mech));
			SendDebug(message_buffer);
#endif

			/* The famous jumping on bridge collision code */
			/*! \todo {possibly turn the bridge collision code into
			 * a mudconf parameter} */
			if(MechRTerrain(mech) == BRIDGE &&
			   collision_check(mech, JUMP, 0, 0) && MechZ(mech) > 0) {

				mech_notify(mech, MECHALL,
							"CRASH! You crash into the bridge!");
				MechLOSBroadcast(mech, "crashes into the bridge!");
				MechFalls(mech, 1, 0);
				return;
			}

			/* Is the jumping unit in its target hex */
			if((MechX(mech) == MechGoingX(mech)) &&
			   (MechY(mech) == MechGoingY(mech))) {

				/*Ok.. in the hex. but no instant landings anymore, laddie */
				/*Range to point of origin is larger than whole jump's length */
				MapCoordToRealCoord(MechX(mech), MechY(mech), &dax, &day);

				/*! \todo {Another instance of that crazy ODDJUMP stuff} */
				/* Basicly checking to see if the unit is in the hex,
				 * the ODDJUMP code looks for the exact center */
#ifdef ODDJUMP
				if(length_hypotenuse(dax - MechStartFX(mech),
									 day - MechStartFY(mech)) <=
				   length_hypotenuse(MechFX(mech) - MechStartFX(mech),
									 MechFY(mech) - MechStartFY(mech))) {

					LandMech(mech);
					MechFX(mech) = (float) dax;
					MechFY(mech) = (float) day;
				}
#else
				LandMech(mech);
				MechFX(mech) = (float) dax;
				MechFY(mech) = (float) day;
#endif
			}

			/* Are we landing on ice */
			if(MechRTerrain(mech) == ICE) {
				if(oz < -1 && MechZ(mech) >= -1)
					break_thru_ice(mech);
				else if(oz >= -1 && MechZ(mech) < -1)
					drop_thru_ice(mech);
			}

		} else if(fabs(MechSpeed(mech)) > 0.0) {

			/* Ok not jumping but we're moving */
			FindComponents(MechSpeed(mech) * MOVE_MOD *
						   MAPMOVEMOD(mech_map),
						   MechLateral(mech) + MechFacing(mech), &newx,
						   &newy);
			MechFX(mech) += newx;
			MechFY(mech) += newy;

			upd_z = 1;

			/* If we're charging record distance traveled */
			if(MechChargeTarget(mech) > 0 && mudconf.btech_newcharge) {
				xscale = 1.0 / SCALEMAP;
				xscale = xscale * xscale;
				xy_charge_dist =
					sqrt(xscale * newx * newx + YSCALE2 * newy * newy);
				MechChargeDistance(mech) += xy_charge_dist;
			}

		} else {

			/* Ok not moving or jumping so don't need to calc a new x,y,z */
			return;
		}
		break;

	case MOVE_TRACK:
	case MOVE_WHEEL:

		/*! \todo {Put the tank JJ code here - for tracked/wheeled} */

		/* Is the tank moving? */
		if(fabs(MechSpeed(mech)) > 0.0) {

			/*! \todo {Possibly put in a mudconf parameter for this since
			 * I think the LATERAL command could check for it} */
#ifndef BT_MOVEMENT_MODES
			FindComponents(MechSpeed(mech) * MOVE_MOD *
						   MAPMOVEMOD(mech_map), MechFacing(mech), &newx,
						   &newy);
#else
			FindComponents(MechSpeed(mech) * MOVE_MOD *
						   MAPMOVEMOD(mech_map),
						   MechLateral(mech) + MechFacing(mech), &newx,
						   &newy);
#endif
			MechFX(mech) += newx;
			MechFY(mech) += newy;
			upd_z = 1;

			/* If we're charging record the distance traveled */
			if(MechChargeTarget(mech) > 0 && mudconf.btech_newcharge) {
				xscale = 1.0 / SCALEMAP;
				xscale = xscale * xscale;
				xy_charge_dist =
					sqrt(xscale * newx * newx + YSCALE2 * newy * newy);
				MechChargeDistance(mech) += xy_charge_dist;
			}

		} else {

			/* Ok not moving or jumping so don't need to calc a new x,y,z */
			return;
		}
		break;

	case MOVE_HOVER:

		/* If we're moving update position */
		if(fabs(MechSpeed(mech)) > 0.0) {

			/*! \todo {Check the todo before this one, mudconf parameter maybe?} */
#ifndef BT_MOVEMENT_MODES
			FindComponents(MechSpeed(mech) * MOVE_MOD *
						   MAPMOVEMOD(mech_map), MechFacing(mech), &newx,
						   &newy);
#else
			FindComponents(MechSpeed(mech) * MOVE_MOD *
						   MAPMOVEMOD(mech_map),
						   MechLateral(mech) + MechFacing(mech), &newx,
						   &newy);
#endif
			MechFX(mech) += newx;
			MechFY(mech) += newy;
			upd_z = 1;

			/* Record the charge distance */
			if(MechChargeTarget(mech) > 0 && mudconf.btech_newcharge) {
				xscale = 1.0 / SCALEMAP;
				xscale = xscale * xscale;
				xy_charge_dist =
					sqrt(xscale * newx * newx + YSCALE2 * newy * newy);
				MechChargeDistance(mech) += xy_charge_dist;
			}

		} else {

			/* Ok not moving or jumping so don't need to calc a new x,y,z */
			return;
		}
		break;

	case MOVE_VTOL:

		/* If we're landed we're not moving */
		if(Landed(mech))
			return;

		/* Use the same code as subs below */
		/*! \todo {VTOLS not able to lateral, nor can subs right now} */

	case MOVE_SUB:

		MarkForLOSUpdate(mech);
		FindComponents(MechSpeed(mech) * MOVE_MOD * MAPMOVEMOD(mech_map),
					   MechFacing(mech), &newx, &newy);
		MechFX(mech) += newx;
		MechFY(mech) += newy;
		MechFZ(mech) += MechVerticalSpeed(mech) * MOVE_MOD;
		MechZ(mech) = MechFZ(mech) / ZSCALE;
		break;

	case MOVE_FLY:

		/* Ok we're in the air */
		if(!Landed(mech)) {

			MarkForLOSUpdate(mech);
			MechFZ(mech) += MechStartFZ(mech) * MOVE_MOD;
			MechZ(mech) = MechFZ(mech) / ZSCALE;
			MechFX(mech) += MechStartFX(mech) * MOVE_MOD;
			MechFY(mech) += MechStartFY(mech) * MOVE_MOD;			

			/*! \todo {Need to rewrite this for aerodyne DSes unless they're
			 * set as large aeros which i'm not sure but either way its
			 * probably not good} */
			if(IsDS(mech)) {

				/* Fun messages to emit to the pilot */
				if(MechZ(mech) < 10 && oz >= 10)
					DS_LandWarning(mech, 1);
				else if(MechZ(mech) < 50 && oz >= 50)
					DS_LandWarning(mech, 0);
				else if(MechZ(mech) < 100 && oz >= 100) {

					if(abs(MechDesiredAngle(mech)) != 90) {
						if(DSOkToNotify(mech)) {
							mech_notify(mech, MECHALL, "As the craft enters "
										"the lower atmosphere, its nose rises up "
										"for a clean landing..");
							snprintf(message_buffer, MBUF_SIZE,
									 "starts descending towards %d, %d..",
									 MechX(mech), MechY(mech));
							MechLOSBroadcast(mech, message_buffer);
						} else {
							mech_notify(mech, MECHALL, "Due to low altitude, "
										"climbing angle set to 90 degrees.");
						}
						MechDesiredAngle(mech) = 90;
					}
					MechStartFX(mech) = 0.0;
					MechStartFY(mech) = 0.0;
					DS_LandWarning(mech, -1);
				}
			}

		} else {

			/* Ok we're rolling around on the ground */
			if(!(fabs(MechSpeed(mech)) > 0.0))
				return;

			FindComponents(MechSpeed(mech) * MOVE_MOD *
						   MAPMOVEMOD(mech_map), MechFacing(mech), &newx,
						   &newy);
			MechFX(mech) += newx;
			MechFY(mech) += newy;
			upd_z = 1;
		}
		break;

	case MOVE_HULL:
	case MOVE_FOIL:

		if(fabs(MechSpeed(mech)) > 0.0) {

			FindComponents(MechSpeed(mech) * MOVE_MOD *
						   MAPMOVEMOD(mech_map), MechFacing(mech), &newx,
						   &newy);
			MechFX(mech) += newx;
			MechFY(mech) += newy;
			MechZ(mech) = 0;
			MechFZ(mech) = 0.0;

		} else {

			/* Not moving so no need to update x,y,z and other stuff */
			return;
		}
		break;
	}

	/* We've already updated the floating x,y values, now to update the hex
	 * x,y values + both the Z values */
	MechLastX(mech) = MechX(mech);
	MechLastY(mech) = MechY(mech);
	last_z = MechZ(mech);

	RealCoordToMapCoord(&MechX(mech), &MechY(mech), MechFX(mech),
						MechFY(mech));

	/*! \todo {Its the ODDJUMP guy again, whats it for?} */
	/* Checking to make sure we didn't jump PAST our target hex */
#ifdef ODDJUMP
	if(Jumping(mech) && MechLastX(mech) == MechGoingX(mech) &&
	   MechLastY(mech) == MechGoingY(mech) &&
	   (MechX(mech) != MechLastX(mech) || MechY(mech) != MechLastY(mech))) {

		LandMech(mech);
		MechFX(mech) -= newx;
		MechFY(mech) -= newy;
		MechFZ(mech) = MechEndFZ(mech);
		MechX(mech) = MechGoingX(mech);
		MechY(mech) = MechGoingY(mech);
		MapCoordToRealCoord(MechX(mech), MechY(mech), &MechFX(mech),
							&MechFY(mech));
		MechZ(mech) = MechFZ(mech) / ZSCALE;
	}
#endif

	/* Store the current map index */
	oi = mech->mapindex;

	/* Did we hit mapedge? */
	CheckEdgeOfMap(mech);

	/* Did our mapindex change? - like did we run out a hangar? */
	if(mech->mapindex != oi)
		mech_map = getMap(mech->mapindex);

	/* We left a hangar and/or moved to a new hex */
	if(oi != mech->mapindex || MechLastX(mech) != MechX(mech) ||
	   MechLastY(mech) != MechY(mech)) {

		/* Either the mech or the map is bad */
		if(!mech || !mech_map) {

			snprintf(message_buffer, MBUF_SIZE,
					 "Invalide pointer (%s) in move_mech()",
					 (!mech ? "mech" : !mech_map ? "mech_map" : "weird...."));
			SendError(message_buffer);

			if(mech) {

				/* Bad Map */
				mech_notify(mech, MECHALL,
							"You are on an invalid map! Map index reset!");
				MechCocoon(mech) = 0;

				if(Jumping(mech))
					mech_land(MechPilot(mech), (void *) mech, "");

				mech_shutdown(MechPilot(mech), (void *) mech, "");
				snprintf(message_buffer, MBUF_SIZE,
						 "move_mech:invalid map:Mech: %d Index: %d",
						 mech->mynum, mech->mapindex);
				SendError(message_buffer);
				mech->mapindex = -1;
			}
			return;
		}

		/* We've moved from our hex so break our cover */
		if(MechCritStatus(mech) & HIDDEN) {
			mech_notify(mech, MECHALL,
						"You move too much and break your cover!");
			MechLOSBroadcast(mech, "breaks from its cover.");
			MechCritStatus(mech) &= ~(HIDDEN);
		}

		StopHiding(mech);

		x = MechX(mech);
		y = MechY(mech);
		MechTerrain(mech) = GetTerrain(mech_map, x, y);
		MechElev(mech) = GetElev(mech_map, x, y);

		/* Update our Z values */
		if(upd_z) {

			/* Chance of breaking through ICE */
			if(MechRTerrain(mech) == ICE) {
				if(oz < -1 && MechZ(mech) >= -1)
					break_thru_ice(mech);
				else if(MechZ(mech) == 0)
					if(possibly_drop_thru_ice(mech))
						iced = 1;
			}

			/* Check for bridges and basic elevation changes */
			DropSetElevation(mech, 0);

			/* To fix certain slide-under-ice-effect for _mechs_ */
			if(MechType(mech) == CLASS_MECH && MechRTerrain(mech) == ICE
			   && oz == -1 && MechZ(mech) == -1) {

				MechZ(mech) = 0;
				MechFZ(mech) = MechZ(mech) * ZSCALE;
			}
		}

		if(!iced)
			NewHexEntered(mech, mech_map, newx, newy, last_z);

		if(MechX(mech) == x && MechY(mech) == y) {

			MarkForLOSUpdate(mech);
			MechFloods(mech);
			water_extinguish_inferno(mech);
			steppable_base_check(mech, x, y);

			/* Pilot XP */
			if(In_Character(mech->mynum)) {
				MechHexes(mech)++;
				if(!(MechHexes(mech) % PIL_XP_EVERY_N_STEPS))
					if(RGotPilot(mech))
						AccumulatePilXP(MechPilot(mech), mech, 1, 0);
			}

			/* Check for stacking */
			domino_space(mech, 0);
		}
	}

	/* If aero/vtol make sure we're not rubbing against the ground or trees */
	if((MechMove(mech) == MOVE_VTOL || is_aero(mech)) && !Landed(mech))
		CheckVTOLHeight(mech);

	/* If we're a boat make sure we've not run a ground */
	if(MechType(mech) == CLASS_VEH_NAVAL)
		CheckNavalHeight(mech, oz);

	/* We're charging lets do some damage */
	if(MechChargeTarget(mech) != -1) {

		/* Valid target? */
		target = getMech(MechChargeTarget(mech));
		if(target) {

			if(FaMechRange(mech, target) < CHARGE_DIST_TRIGGER) {
				ChargeMech(mech, target);
				MechChargeTarget(mech) = -1;
				MechChargeTimer(mech) = 0;
				MechChargeDistance(mech) = 0;
			}

		} else {
			mech_notify(mech, MECHPILOT, "Invalid CHARGE target!");
			MechChargeTarget(mech) = -1;
			MechChargeDistance(mech) = 0;
			MechChargeTimer(mech) = 0;
		}
	}

	/* If we're towing something update its position with us */
	if(MechCarrying(mech) > 0) {
		target = getMech(MechCarrying(mech));
		if(target && target->mapindex == mech->mapindex) {
			MirrorPosition(mech, target, 0);
			SetRFacing(target, MechRFacing(mech));
		}
	}

	/* If a bsuit has swarmed a target update its
	 * position in relation to its target */
	BSuitMirrorSwarmedTarget(mech_map, mech);

	/* This is really for killing MechWarriors 
	 * actual heat code is checked with UpdateHeat */
	fiery_death(mech);
}

void CheckNavalHeight(MECH * mech, int oz)
{
	if(MechRTerrain(mech) != WATER && MechRTerrain(mech) != ICE &&
	   MechRTerrain(mech) != BRIDGE) {
		MechSpeed(mech) = 0.0;
		MechVerticalSpeed(mech) = 0;
		MechDesiredSpeed(mech) = 0.0;
		SetFacing(mech, 0);
		MechDesiredFacing(mech) = 0;
		return;
	}
	if(!oz && MechZ(mech) && MechElev(mech) > 1) {
		MarkForLOSUpdate(mech);
		MechZ(mech) = 0;
		MechLOSBroadcast(mech, "dives!");
		MechZ(mech) = -1;
	}
	if(MechFZ(mech) > 0.0) {
		if(MechVerticalSpeed(mech) > 0 && !MechZ(mech) && oz < 0) {
			mech_notify(mech, MECHALL,
						"Your sub has reached surface and stops rising.");
			MechLOSBroadcast(mech, tprintf("surfaces at %d,%d!",
										   MechX(mech), MechY(mech)));
			/* Possible show-up message? */
		}
		MechZ(mech) = 0;
		MechFZ(mech) = 0.0;
		if(MechVerticalSpeed(mech) > 0)
			MechVerticalSpeed(mech) = 0;
		return;
	}
	if(MechZ(mech) <= (MechLowerElevation(mech))) {
		MechZ(mech) = MIN(0, MechLowerElevation(mech) + 1);
		if(MechElevation(mech) > 0)
			SendError(tprintf
					  ("Oddity: #%d managed to wind up on '%c' (%d elev.)",
					   mech->mynum, MechTerrain(mech), MechElev(mech)));
		MechFZ(mech) = ((5.0 * MechZ(mech) - 4) * ZSCALE) / 5.0;
		if(MechMove(mech) == MOVE_SUB) {
			if(MechVerticalSpeed(mech) < 0) {
				MechVerticalSpeed(mech) = 0;
				mech_notify(mech, MECHALL,
							"The sub has reached bottom and stops diving.");
			}
#if 0
			else
				mech_notify(mech, MECHALL, "The sub has reached bottom.");
#endif
		}
	}
}

void CheckVTOLHeight(MECH * mech)
{
	if(InWater(mech) && MechZ(mech) <= 0) {
		mech_notify(mech, MECHALL, "You crash your vehicle into the water!");
		mech_notify(mech, MECHALL, "Water pours into the cockpit....glub glub!");
		MechLOSBroadcast(mech, "splashes into the water!");
		DestroyMech(mech, mech, 0);
		return;
	}

	if((MechZ(mech) >= ORBIT_Z) && !is_aero(mech)) {
		mech_notify(mech, MECHALL, "You cannot achieve orbit! Vertical movement halted!");
		MechZ(mech) = ORBIT_Z - 1;
		MechFZ(mech) = ZSCALE * MechZ(mech);
		MechVerticalSpeed(mech) = 0.0;
		return;
	}

	if(MechZ(mech) >= MechElevation(mech))
		return;
	if(MechRTerrain(mech) == BRIDGE)
		if(MechZ(mech) != (MechElevation(mech) - 1))
			return;
	aero_land(MechPilot(mech), mech, "");
	if(Landed(mech))
		return;
	mech_notify(mech, MECHALL,
				"CRASH! You smash your toy into the ground!");
	MechLOSBroadcast(mech, "crashes into the ground!");
	MechFalls(mech, 1 + fabs(MechVerticalSpeed(mech) / MP1), 0);

/*   mech_notify (mech, MECHALL, "Your vehicle is inoperable."); */
	MechZ(mech) = MechElevation(mech);
	MechFZ(mech) = ZSCALE * MechZ(mech);
	MechSpeed(mech) = 0.0;
	MechVerticalSpeed(mech) = 0.0;
	MechStatus(mech) |= LANDED;

/*   DestroyMech (mech, mech); */
}

void UpdateHeading(MECH * mech)
{
	int offset;
	int normangle;
	int mw_mod = 1;
	float maxspeed, omaxspeed;
	MAP *mech_map;

	if(MechFacing(mech) == MechDesiredFacing(mech))
		return;
	maxspeed = MMaxSpeed(mech);
	if(is_aero(mech))
		maxspeed = maxspeed * ACCEL_MOD;
	if((MechHeat(mech) >= 9.) && (MechSpecials(mech) & TRIPLE_MYOMER_TECH))
		maxspeed += 1.5 * MP1;
	omaxspeed = maxspeed;
	normangle = MechRFacing(mech) - SHO2FSIM(MechDesiredFacing(mech));
	if(MechType(mech) == CLASS_MW || MechType(mech) == CLASS_BSUIT)
		mw_mod = 60;
	else if(MechIsQuad(mech))
		mw_mod = 2;
	if(mudconf.btech_fasaturn) {
#define FASA_TURN_MOD 3/2
		if(Jumping(mech))
			offset = 2 * SHO2FSIM(1) * 2 * 360 * FASA_TURN_MOD / 60;
		else {
			float ts = MechSpeed(mech);

			if(ts < 0) {
				maxspeed = maxspeed * 2.0 / 3.0;
				ts = -ts;
			}
			if(ts > maxspeed || maxspeed < 0.1)	/* kludge */
				offset = 0;
			else {
				offset = SHO2FSIM(1) * 2 * 360 * FASA_TURN_MOD / 60 * (maxspeed - ts) * (omaxspeed / maxspeed) * mw_mod * MP_PER_KPH / 6;	/* hmm. */
			}
		}
	} else {
		if(Jumping(mech)) {
			mech_map = FindObjectsData(mech->mapindex);
			offset = SHO2FSIM(1) * 6 * JumpSpeedMP(mech, mech_map) * mw_mod;
		} else if(fabs(MechSpeed(mech)) < 1.0)
			offset = SHO2FSIM(1) * 3 * maxspeed * MP_PER_KPH * mw_mod;
		else {
			offset = SHO2FSIM(1) * 2 * maxspeed * MP_PER_KPH * mw_mod;
			if((SHO2FSIM(abs(normangle)) > offset) &&
			   IsRunning(MechSpeed(mech), maxspeed)) {
				if(MechSpeed(mech) > maxspeed)
					offset -= offset / 2 * maxspeed / MechSpeed(mech);
				else
					offset -=
						offset / 2 * (3.0 * MechSpeed(mech) / maxspeed - 2.0);
			}
		}
	}
	/*   offset = offset * 2 * MOVE_MOD; - Twice as fast as this;dunno why - */
	offset = offset * MOVE_MOD;
#ifdef BT_MOVEMENT_MODES
	if(GetTurnMode(mech)
	   && HasBoolAdvantage(MechPilot(mech), "maneuvering_ace"))
		offset = (offset * 3) / 2;
	if(MechStatus2(mech) & (SPRINTING | EVADING)
	   && !HasBoolAdvantage(MechPilot(mech), "maneuvering_ace")) {
		if(HasBoolAdvantage(MechPilot(mech), "speed_demon"))
			offset = (offset * 2) / 3;
		else
			offset = (offset / 2);
	}
#endif
	if(normangle < 0)
		normangle += SHO2FSIM(360);
	if(IsDS(mech) && offset >= SHO2FSIM(10))
		offset = SHO2FSIM(10);
	if(normangle > SHO2FSIM(180)) {
		AddRFacing(mech, offset);
		if(MechFacing(mech) >= 360)
			AddFacing(mech, -360);
		normangle += offset;
		if(normangle >= SHO2FSIM(360))
			SetRFacing(mech, SHO2FSIM(MechDesiredFacing(mech)));
	} else {
		AddRFacing(mech, -offset);
		if(MechRFacing(mech) < 0)
			AddFacing(mech, 360);
		normangle -= offset;
		if(normangle < 0)
			SetRFacing(mech, SHO2FSIM(MechDesiredFacing(mech)));
	}
	MechCritStatus(mech) |= CHEAD;
	MarkForLOSUpdate(mech);
}

/* MPs lost for heat need to go off walking speed, not off total speed */
#define DECREASE_HEAT(spd) \
tempspeed *= (ceil((rint((maxspeed / 1.5) / MP1) - (spd/MP1) ) * 1.5) * MP1) / maxspeed

#define DECREASE_OLD(spd) \
tempspeed *= (maxspeed - (spd)) / maxspeed
#define INCREASE_OLD(spd) DECREASE_OLD(-(spd))

#define DECREASE_NEW(spd) \
tempspeed *= MP1 / (MP1 + spd)
#define INCREASE_NEW(spd) \
tempspeed *= (MP1 + spd/2) / MP1

#define DECREASE(s) DECREASE_NEW(s)
#define INCREASE(s) INCREASE_NEW(s)

/* If you want to simulate _OLDs, you have to add 1MP in some cases (eww) */

float terrain_speed(MECH * mech, float tempspeed, float maxspeed,
					int terrain, int elev)
{
	switch (terrain) {
	case SNOW:
	case ROUGH:
		DECREASE(MP1);
		break;
	case MOUNTAINS:
		DECREASE(MP2);
		break;
	case LIGHT_FOREST:
		if(MechType(mech) != CLASS_BSUIT)
			DECREASE(MP1);
		break;
	case HEAVY_FOREST:
		if(MechType(mech) != CLASS_BSUIT)
			DECREASE(MP2);
		break;
	case BRIDGE:
	case ROAD:
		/* Ground units (wheeled and tracked) get +1 MP moving on paved surface */
#ifndef BT_MOVEMENT_MODES
		if(MechMove(mech) == MOVE_TRACK || MechMove(mech) == MOVE_WHEEL)
#else
		if(!(MechStatus2(mech) & SPRINTING) &&
		   (MechMove(mech) == MOVE_TRACK || MechMove(mech) == MOVE_WHEEL))
#endif
			INCREASE_OLD(MP1);
	case ICE:
		if(MechZ(mech) >= 0)
			break;
		/* FALLTHRU */
		/* if he's under the ice/bridge, treat as water. */
	case WATER:
		if(MechIsBiped(mech) || MechIsQuad(mech)) {
			if(elev <= -2)
				DECREASE(MP3);
			else if(elev == -1)
				DECREASE(MP1);
		}
		break;
	}
	return tempspeed;
}

void UpdateSpeed(MECH * mech)
{
	float acc, tempspeed, maxspeed;
	MECH *target;

	if(!(!Fallen(mech) && !Jumping(mech) && (MechMaxSpeed(mech) > 0.0)))
		return;
	tempspeed = fabs(MechDesiredSpeed(mech));
	maxspeed = MMaxSpeed(mech);
	if(maxspeed < 0.0)
		maxspeed = 0.0;

	if((MechStatus(mech) & MASC_ENABLED) &&
	   (MechStatus(mech) & SCHARGE_ENABLED))
		maxspeed = ceil((rint(maxspeed / 1.5) / MP1) * 2.5) * MP1;
	else if(MechStatus(mech) & MASC_ENABLED)
		maxspeed = (4. / 3.) * maxspeed;
	else if(MechStatus(mech) & SCHARGE_ENABLED)
		maxspeed = (4. / 3.) * maxspeed;

	if(MechSpecials(mech) & TRIPLE_MYOMER_TECH) {
		if(MechHeat(mech) >= 9.)
			maxspeed =
				ceil((rint((MMaxSpeed(mech) / 1.5) / MP1) + 1) * 1.5) * MP1;
		/* maxspeed *= ((maxspeed + 1.5 * MP1) / maxspeed); */
		if(MechDesiredSpeed(mech) >= maxspeed)
			MechDesiredSpeed(mech) = maxspeed;
	}

	if(MechHeat(mech) >= 5.) {

/*  if ((MechHeat(mech) >= 9.) && (MechSpecials(mech) & TRIPLE_MYOMER_TECH)) {
      tempspeed *= ((maxspeed + 1.5 * MP1) / maxspeed);
  }
*/
		if(MechHeat(mech) >= 25.)
			DECREASE_HEAT(MP5);
		else if(MechHeat(mech) >= 20.)
			DECREASE_HEAT(MP4);
		else if(MechHeat(mech) >= 15.)
			DECREASE_HEAT(MP3);
		else if(MechHeat(mech) >= 10.)
			DECREASE_HEAT(MP2);
		else if(!((MechSpecials(mech) & TRIPLE_MYOMER_TECH) &&
				  MechHeat(mech) >= 9))
			DECREASE_HEAT(MP1);
	}
	if(MechType(mech) != CLASS_MW && MechMove(mech) != MOVE_VTOL &&
	   (MechMove(mech) != MOVE_FLY || Landed(mech)))
		tempspeed =
			terrain_speed(mech, tempspeed, maxspeed, MechRTerrain(mech),
						  MechElevation(mech));
	if(MechCritStatus(mech) & CHEAD) {
		if(mudconf.btech_slowdown == 2) {
			/* _New_ slowdown based on facing vs desired difference */
			int dif = MechFacing(mech) - MechDesiredFacing(mech);

			if(dif < 0)
				dif = -dif;
			if(dif > 180)
				dif = 360 - dif;
			if(dif) {
				dif = (dif - 1) / 30;
				dif = (dif + 2);	/* whee */
				/* dif = 2 to 7 */
				tempspeed = tempspeed * (10 - dif) / 10;
			}
		} else if(mudconf.btech_slowdown == 1) {
			if(MechFacing(mech) != MechDesiredFacing(mech))
				tempspeed = tempspeed * 2.0 / 3.0;
			else
				tempspeed = tempspeed * 3.0 / 4.0;
		}
#ifdef BT_MOVEMENT_MODES
		if((Sprinting(mech) || Evading(mech))
		   && !(HasBoolAdvantage(MechPilot(mech), "speed_demon")
				|| HasBoolAdvantage(MechPilot(mech), "maneuvering_ace")))
			tempspeed = (tempspeed * 2) / 3;
#endif
		MechCritStatus(mech) &= ~CHEAD;
	}
	if(MechIsQuad(mech) && MechLateral(mech))
		DECREASE_OLD(MP1);		/* In truth 1 MP */
#ifdef BT_MOVEMENT_MODES
	else if(MechLateral(mech)) {
		if(HasBoolAdvantage(MechPilot(mech), "maneuvering_ace"))
			DECREASE_OLD(MP2);
		else
			DECREASE_OLD(MP3);
	}
#endif
	if(tempspeed <= 0.0)
		tempspeed = 0.0;
	if(MechDesiredSpeed(mech) < 0.)
		tempspeed = -tempspeed;

/*    if (MechSpecials(mech) & TRIPLE_MYOMER_TECH)
        {
        if (MechHeat(mech) >= 9.)
            maxspeed *= ((maxspeed + 1.5 * MP1) / maxspeed);
  if (MechDesiredSpeed(mech) >= maxspeed)
    MechDesiredSpeed(mech) = maxspeed;
        }
*/

	if(tempspeed != MechSpeed(mech)) {
		if(MechIsQuad(mech))
			acc = maxspeed / 10.;
		else
			acc = maxspeed / 20.;
		if(HasBoolAdvantage(MechPilot(mech), "speed_demon"))
			acc *= 1.25;

		if(tempspeed < MechSpeed(mech)) {
			/* Decelerating */
			MechSpeed(mech) -= acc;
			if(tempspeed > MechSpeed(mech))
				MechSpeed(mech) = tempspeed;
		} else {
			/* Accelerating */
			MechSpeed(mech) += acc;
			if(tempspeed < MechSpeed(mech))
				MechSpeed(mech) = tempspeed;
		}
	}
	if(MechCarrying(mech) > 0) {
		target = getMech(MechCarrying(mech));
		if(target)
			MechSpeed(target) = MechSpeed(mech);
	}
}

int OverheatMods(MECH * mech)
{
	int returnValue;

	if(MechHeat(mech) >= 24.) {
		/* +4 to fire... */
		returnValue = 4;
	} else if(MechHeat(mech) >= 17.) {
		/* +3 to fire... */
		returnValue = 3;
	} else if(MechHeat(mech) >= 13.) {
		/* +2 to fire... */
		returnValue = 2;
	} else if(MechHeat(mech) >= 8.) {
		/* +1 to fire... */
		returnValue = 1;
	} else {
		returnValue = 0;
	}
	return (returnValue);
}

void ammo_explosion(MECH * attacker, MECH * mech, int ammoloc,
					int ammocritnum, int damage)
{
	if(MechType(mech) == CLASS_MW) {
		mech_notify(mech, MECHALL, "Your weapon's ammo explodes!");
		MechLOSBroadcast(mech, "'s weapon's ammo explodes!");
	} else {
		mech_notify(mech, MECHALL, "Ammunition explosion!");
		if(GetPartAmmoMode(mech, ammoloc, ammocritnum) & INFERNO_MODE)
			MechLOSBroadcast(mech,
							 "is suddenly enveloped by a brilliant fireball!");
		else
			MechLOSBroadcast(mech, "has an internal ammo explosion!");
	}
	DestroyPart(mech, ammoloc, ammocritnum);
	if(!attacker)
		return;
	if(GetPartAmmoMode(mech, ammoloc, ammocritnum) & INFERNO_MODE) {
		Inferno_Hit(mech, mech, damage / 4, 0);
		if(mudconf.btech_inferno_penalty)
			MechWeapHeat(mech) += 30.0;
		damage = damage / 2;
	}
	if(MechType(mech) == CLASS_BSUIT)
		DamageMech(mech, attacker, 0, -1, ammoloc, 0, 0, damage, 0, -1, 0,
				   -1, 0, 0);
	else
		DamageMech(mech, attacker, 0, -1, ammoloc, 0, 0, -1, damage, -1, 0,
				   -1, 0, 0);

	if(MechType(mech) != CLASS_BSUIT) {
		mech_notify(mech, MECHPILOT,
					"You take personal injury from the ammunition explosion!");
		if(HasBoolAdvantage(MechPilot(mech), "pain_resistance"))
			headhitmwdamage(mech, mech, 1);
		else
			headhitmwdamage(mech, mech, 2);
	}
}

void HandleOverheat(MECH * mech)
{
	int avoided = 0, hasinferno = 0;
	MAP *mech_map;
	int ammoloc, ammocritnum, damage = 0;

	if(MechHeat(mech) < 10.)
		return;
	/* Has it been a TURN already ? */
	if((MechHeatLast(mech) + TURN) > muxevent_tick)
		return;
	MechHeatLast(mech) = muxevent_tick;

	/* Ammo - done first so infernobooms shut you down */
	if(MechHeat(mech) >= 10.) {
		if(mudconf.btech_inferno_penalty)
			hasinferno = FindInfernoAmmo(mech, &ammoloc, &ammocritnum);
		if(MechHeat(mech) >= 28.) {
			/* Ammo explosion (Avoid 8+, infernos 12+) */
			if(hasinferno) {
				if(Roll() >= 12)
					avoided = 1;
			} else {
				if(Roll() >= 8)
					avoided = 1;
			}
		} else if(MechHeat(mech) >= 23.) {
			/* Ammo explosion (Avoid 6+, infernos 10+) */
			if(hasinferno) {
				if(Roll() >= 10)
					avoided = 1;
			} else {
				if(Roll() >= 6)
					avoided = 1;
			}
		} else if(MechHeat(mech) >= 19.) {
			/* Ammo explosion (Avoid 4+, infernos 8+) */
			if(hasinferno) {
				if(Roll() >= 8)
					avoided = 1;
			} else {
				if(Roll() >= 4)
					avoided = 1;
			}

		} else if((MechHeat(mech) >= 14.) && (hasinferno)) {
			if(Roll() >= 6)
				avoided = 1;
		} else if((MechHeat(mech) >= 10.) && (hasinferno)) {
			if(Roll() >= 4)
				avoided = 1;
		} else if((MechHeat(mech) < 19.) && (!hasinferno)) {
			avoided = 1;
		}

		if(!(avoided)) {
			if(!hasinferno)
				damage = FindDestructiveAmmo(mech, &ammoloc, &ammocritnum);
			else
				damage = hasinferno;
			if(damage) {
				/* BOOM! */
				/* That's going to hurt... */
				ammo_explosion(mech, mech, ammoloc, ammocritnum, damage);
			} else
				mech_notify(mech, MECHALL,
							"You have no ammunition, lucky you!");
		}
	}

	avoided = 0;
#ifdef BT_EXILE_MW3STATS
	if(!isPlayer(MechPilot(mech))) {
#endif
		if(MechHeat(mech) >= 30.) {
			/* Shutdown */
		} else if(MechHeat(mech) >= 26.) {
			/* Shutdown avoid on 10+ */
			if(Roll() >= 10)
				avoided = 1;
		} else if(MechHeat(mech) >= 22.) {
			/* Shutdown avoid on 8+ */
			if(Roll() >= 8)
				avoided = 1;
		} else if(MechHeat(mech) >= 18.) {
			/* Shutdown avoid on 6+ */
			if(Roll() >= 6)
				avoided = 1;
		} else if(MechHeat(mech) >= 14.) {
			/* Shutdown avoid on 4+ */
			if(Roll() >= 4)
				avoided = 1;
		}
#ifdef BT_EXILE_MW3STATS
	} else {
		avoided = 1;
		if(MechHeat(mech) >= 14.) {
			mech_notify(mech, MECHALL,
						"You frantically attempt to override the shutdown process!");
			avoided =
				char_getskillsuccess(MechPilot(mech), "computer",
									 (MechHeat(mech) >=
									  30. ? 8 : MechHeat(mech) >=
									  26. ? 6 : MechHeat(mech) >=
									  22. ? 4 : MechHeat(mech) >=
									  18. ? 2 : 0), 1);
			if(avoided)
				AccumulateComputerXP(MechPilot(mech), mech, 1);
		}
	}
#endif
	if(!(avoided) && Started(mech)) {
		if(MechStatus(mech) & STARTED)
			mech_notify(mech, MECHALL, "%ci%crReactor shutting down...%c");
		if(MechStatus2(mech) & SLITE_ON) {
			mech_notify(mech, MECHALL, "Your searchlight shuts off.");
			MechStatus2(mech) &= ~SLITE_ON;
			MechCritStatus(mech) &= ~SLITE_LIT;
		}
		if(Jumping(mech) || OODing(mech) || (is_aero(mech) && !Landed(mech))) {
			mech_notify(mech, MECHALL, "%chYou fall from the sky!%c");
			MechLOSBroadcast(mech, "falls from the sky!");
			mech_map = getMap(mech->mapindex);
			MechFalls(mech, JumpSpeedMP(mech, mech_map), 0);
			domino_space(mech, 2);
		} else {
			MechLOSBroadcast(mech, "stops in mid-motion!");
			if((fabs(MechSpeed(mech) > MP1)) && !Fallen(mech) &&
			   (!MadePilotSkillRoll(mech, 3)))
				MechFalls(mech, 0, 1);
		}
		Shutdown(mech);
		StopMoving(mech);
		StopStand(mech);
	}
}

static int EnableSomeHS(MECH * mech, int numsinks)
{

	numsinks = MIN(numsinks,
				   (MechSpecials(mech) & (DOUBLE_HEAT_TECH | CLAN_TECH)) ? 4 :
				   2);
	numsinks = MIN(numsinks, MechDisabledHS(mech));

	if(!numsinks)
		return 0;

	MechDisabledHS(mech) -= numsinks;
	MechMinusHeat(mech) += numsinks;	/* We dont check for water and 
										   such after enabling them, only the next tic. */
#ifdef HEATCUTOFF_DEBUG
	mech_printf(mech, MECHALL,
				"%%cg%d heatsink%s kick%s into action.%%c", numsinks,
				numsinks == 1 ? "" : "s", numsinks == 1 ? "s" : "");
#endif

	return numsinks;
}

static int DisableSomeHS(MECH * mech, int numsinks)
{

	numsinks = MIN(numsinks,
				   (MechSpecials(mech) & (DOUBLE_HEAT_TECH | CLAN_TECH)) ? 4 :
				   2);
	numsinks = MIN(numsinks, MechActiveNumsinks(mech));

	if(!numsinks)
		return 0;

	MechDisabledHS(mech) += numsinks;
	MechMinusHeat(mech) -= numsinks;	/* Submerged heatsinks silently 
										   still dissipate some heat */
#ifdef HEATCUTOFF_DEBUG
	mech_printf(mech, MECHALL,
				"%%cy%d heatsink%s hum%s into silence.%%c", numsinks,
				numsinks == 1 ? "" : "s", numsinks == 1 ? "s" : "");
#endif

	return numsinks;
}

/* Update the Unit's current heat values as well as 
 * send messages to the pilot based on heat level */
void UpdateHeat(MECH * mech)
{

	int legsinks;
	float maxspeed;
	float intheat;
	float inheat;
	MAP *map;

	// These guys don't get heat updates.
	if(!MechHasHeat(mech)) 
		return;

	inheat = MechHeat(mech);
	maxspeed = MMaxSpeed(mech);
	MechPlusHeat(mech) = 0.;

	if(MechTerrain(mech) == FIRE && MechType(mech) == CLASS_MECH)
		MechPlusHeat(mech) += 5.;

	/* We do a trick here.  We look at the previous heat level to determine
	 * if TSM is/was on.  If it is/was, we recalc what running and walk speeds are
	 * to better set how much heat the unit is putting out */
	if(MechSpecials(mech) & TRIPLE_MYOMER_TECH) {
		if(inheat >= 9.)
			maxspeed =
				ceil((rint((MMaxSpeed(mech) / 1.5) / MP1) + 1) * 1.5) * MP1;
	}

	if(fabs(MechSpeed(mech)) > 0.0) {
#ifndef BT_MOVEMENT_MODES
		if(IsRunning(MechDesiredSpeed(mech), maxspeed))
			MechPlusHeat(mech) += 2.;
#else
		if(Sprinting(mech) || Evading(mech))
			MechPlusHeat(mech) += 3.;
		else if(IsRunning(MechDesiredSpeed(mech), maxspeed))
			MechPlusHeat(mech) += 2.;
#endif
		else
			MechPlusHeat(mech) += 1.;
	}

	if(Jumping(mech))
		MechPlusHeat(mech) += (MechJumpSpeed(mech) * MP_PER_KPH > 3.) ?
			MechJumpSpeed(mech) * MP_PER_KPH : 3.;

	if(Started(mech))
		MechPlusHeat(mech) += (float) MechEngineHeat(mech);

	if(StealthArmorActive(mech))
		MechPlusHeat(mech) += 10;

	if(NullSigSysActive(mech))
		MechPlusHeat(mech) += 10;

	intheat = MechPlusHeat(mech);

	MechPlusHeat(mech) += MechWeapHeat(mech);

	/* ADD Water effects here */
	if(InWater(mech) && MechZ(mech) <= -1) {
		legsinks = FindLegHeatSinks(mech);
		legsinks = (legsinks > 4) ? 4 : legsinks;
		if(MechZ(mech) == -1 && !Fallen(mech)) {
			MechMinusHeat(mech) = MIN(2 * MechActiveNumsinks(mech),
									  legsinks + MechActiveNumsinks(mech));
		} else {
			MechMinusHeat(mech) = MIN(2 * MechActiveNumsinks(mech),
									  6 + MechActiveNumsinks(mech));
		}
	} else {
		MechMinusHeat(mech) = (float) (MechActiveNumsinks(mech));
	}

	/* Infernoed */
	if(Jellied(mech)) {
		MechMinusHeat(mech) = MechMinusHeat(mech) - 6;
		if(MechMinusHeat(mech) < 0)
			MechMinusHeat(mech) = 0;
	}

	if(InSpecial(mech))
		if((map = FindObjectsData(mech->mapindex)))
			if(MapUnderSpecialRules(map))
				if(MapTemperature(map) < -30 || MapTemperature(map) > 50) {
					if(MapTemperature(map) < -30)
						MechMinusHeat(mech) +=
							(-30 - MapTemperature(map) + 9) / 10;
					else
						MechMinusHeat(mech) -=
							(MapTemperature(map) - 50 + 9) / 10;
				}

	/* Handle heat cutoff now */
	/* En/DisableSomeHS() take care of MechMinusHeat also. */
	/* Re-Written to use Exile's code - Dany 12/05 */
	if(Heatcutoff(mech)) {
		float overheat = MechPlusHeat(mech) - MechMinusHeat(mech);

		if(overheat >= 10.)
			EnableSomeHS(mech, floor(overheat - 10.) + 1);
		else if(overheat < 9.)
			DisableSomeHS(mech, floor(9. - overheat) + 1);

	} else if(MechDisabledHS(mech)) {
		EnableSomeHS(mech, 100);
	}

	MechHeat(mech) = MechPlusHeat(mech) - MechMinusHeat(mech);

	/* No lowering of heat if heat is under 9 */
	MechWeapHeat(mech) -=
		(MechMinusHeat(mech) - intheat) / WEAPON_RECYCLE_TIME;

	if(MechWeapHeat(mech) < 0.0)
		MechWeapHeat(mech) = 0.0;

	if(MechHeat(mech) < 0.0)
		MechHeat(mech) = 0.0;

	if((muxevent_tick % TURN) == 0)
		if(MechCritStatus(mech) & LIFE_SUPPORT_DESTROYED ||
		   (MechHeat(mech) > 30. && Number(0, 1) == 0)) {
			if(MechHeat(mech) > 25.) {
				mech_notify(mech, MECHPILOT,
							"You take personal injury from heat!");
				headhitmwdamage(mech, mech,
								MechCritStatus(mech) & LIFE_SUPPORT_DESTROYED
								? 2 : 1);
			} else if(MechHeat(mech) >= 15.) {
				mech_notify(mech, MECHPILOT,
							"You take personal injury from heat!");
				headhitmwdamage(mech, mech, 1);
			}
		}

	if(MechHeat(mech) >= 19) {
		if(inheat < 19) {
			mech_notify(mech, MECHALL,
						"%ch%cr=====================================");
			mech_notify(mech, MECHALL,
						"Your Excess Heat indicator turns RED!");
			mech_notify(mech, MECHALL,
						"=====================================%c");
		}
	} else if(MechHeat(mech) >= 14) {
		if(inheat >= 19 || inheat < 14) {
			mech_notify(mech, MECHALL,
						"%ch%cy=======================================");
			mech_notify(mech, MECHALL,
						"Your Excess Heat indicator turns YELLOW");
			mech_notify(mech, MECHALL,
						"=======================================%c");
		}
	} else {
		if(inheat >= 14) {
			mech_notify(mech, MECHALL,
						"%cg======================================");
			mech_notify(mech, MECHALL,
						"Your Excess Heat indicator turns GREEN");
			mech_notify(mech, MECHALL,
						"======================================%c");
		}
	}
	HandleOverheat(mech);
}

int recycle_weaponry(MECH * mech)
{

	int loop;
	int count, i;
	int crit[MAX_WEAPS_SECTION];
	unsigned char weaptype[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	char location[20];

	int diff = (muxevent_tick - MechLWRT(mech));
	int lowest = 0;

	if(diff < 1) {
		if(diff < 0)
			MechLWRT(mech) = muxevent_tick;
		return 1;
	}
	MechLWRT(mech) = muxevent_tick;

	if(!Started(mech) || Destroyed(mech))
		return 0;

	arc_override = 1;
	for(loop = 0; loop < NUM_SECTIONS; loop++) {
		count = FindWeapons(mech, loop, weaptype, weapdata, crit);
		for(i = 0; i < count; i++) {
			if(WpnIsRecycling(mech, loop, crit[i])) {
				/* Immediate recycle if its destroyed */
				if(PartTempNuke(mech, loop, crit[i]) == FAIL_DESTROYED && SectIsDestroyed(mech, loop))
					GetPartData(mech, loop, crit[i]) = 0;
				if(diff >= GetPartData(mech, loop, crit[i])) {
					GetPartData(mech, loop, crit[i]) = 0;
					mech_printf(mech, MECHSTARTED,
								MechType(mech) ==
								CLASS_MW ?
								"%%cgYou are ready to attack again with %s.%%c"
								: PartTempNuke(mech, loop,
											   crit[i]) !=
								0 ? "%%cg%s is operational again.%%c" :
								(GetPartFireMode(mech, loop,
												 crit[i]) & ROCKET_FIRED) ? ""
								: "%%cg%s finished recycling.%%c",
								&MechWeapons[weaptype[i]].name[3]);
					SetPartTempNuke(mech, loop, crit[i], 0);
				} else {
					if(PartTempNuke(mech, loop, crit[i]) != FAIL_DESTROYED) {
						GetPartData(mech, loop, crit[i]) -= diff;
						if(GetPartData(mech, loop, crit[i]) < lowest ||
						   !lowest)
							lowest = GetPartData(mech, loop, crit[i]);
					}
				}
			}
		}

		/* Cycle a section */
		if(MechSections(mech)[loop].recycle && ((MechType(mech) == CLASS_MECH)
												|| (MechType(mech) ==
													CLASS_BSUIT)
												|| (MechType(mech) ==
													CLASS_VEH_GROUND)
												|| (MechType(mech) ==
													CLASS_VTOL))) {

			/* Is the section finished cycling or do we deincrement it */
			if(diff >= MechSections(mech)[loop].recycle &&
			   !SectIsDestroyed(mech, loop)) {

				MechSections(mech)[loop].recycle = 0;
				ArmorStringFromIndex(loop, location, MechType(mech),
									 MechMove(mech));

				mech_printf(mech, MECHSTARTED,
							"%%cg%s%s has finished its previous action.%%c",
							MechType(mech) == CLASS_BSUIT ? "" : "Your ",
							location);

			} else {

				MechSections(mech)[loop].recycle -= diff;
				if(MechSections(mech)[loop].recycle < lowest || !lowest)
					lowest = MechSections(mech)[loop].recycle;

			}

		}

	}
	arc_override = 0;
	return lowest;
}

int SkidMod(float Speed)
{
	if(Speed < 2.1)
		return -1;
	if(Speed < 4.1)
		return 0;
	if(Speed < 7.1)
		return 1;
	if(Speed < 10.1)
		return 2;
	return 4;
}

/*
 * Move the unit back to its previous location because of cliff or something
 */
void move_unit_back(MECH * mech, float deltax, float deltay,
					int lastelevation, int ot, int le)
{

	MechFX(mech) -= deltax;
	MechFY(mech) -= deltay;
	MechX(mech) = MechLastX(mech);
	MechY(mech) = MechLastY(mech);
	MechZ(mech) = lastelevation;
	MechFZ(mech) = MechZ(mech) * ZSCALE;
	MechTerrain(mech) = ot;
	MechElev(mech) = le;

}

/*
 * Check to see what happens to the unit now that its entered a new hex
 */
void NewHexEntered(MECH * mech, MAP * mech_map, float deltax, float deltay,
				   int last_z)
{
	int elevation, lastelevation;
	int oldterrain;
	int ot, le, ed, done = 0, tt, avoidbth;
	int isunder = 0;
	float f;

	/* Recording the old elevation and terrain */
	/*! \todo {Wasn't lastelevation passed as an argument 'last_z' ?} */
	ot = oldterrain = GetTerrain(mech_map, MechLastX(mech), MechLastY(mech));

	if((MechMove(mech) == MOVE_HOVER) &&
	   (oldterrain == WATER || oldterrain == ICE ||
		((oldterrain == BRIDGE) && (last_z == 0)))) {

		le = lastelevation = elevation = 0;

	} else {

		le = lastelevation =
			Elevation(mech_map, MechLastX(mech), MechLastY(mech));
		elevation = MechElevation(mech);

		if(MechMove(mech) == MOVE_HOVER && elevation < 0)
			elevation = 0;

		if(ot == ICE && MechZ(mech) >= 0) {
			le = lastelevation = 0;
		}

	/*	if(MechZ(mech) < le)
			le = MechZ(mech);
	*/
	}

	switch (MechMove(mech)) {
	case MOVE_BIPED:
	case MOVE_QUAD:

		if(Jumping(mech)) {

			if(MechRTerrain(mech) == WATER)
				return;

#define MOVE_BACK \
                MechFX(mech) -= deltax;\
                MechFY(mech) -= deltay;\
                MechX(mech) = MechLastX(mech);\
                MechY(mech) = MechLastY(mech);\
                MechZ(mech) = lastelevation;\
                MechFZ(mech) = MechZ(mech) * ZSCALE;\
                MechTerrain(mech) = ot;\
                MechElev(mech) = le;

			/* Did we hit something while jumping */
			if(collision_check(mech, JUMP, 0, 0)) {

				ed = MAX(1, 1 + MechZ(mech) - Elevation(mech_map,
														MechX(mech),
														MechY(mech)));
				move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
				mech_notify(mech, MECHALL,
							"%chYou attempt to jump over elevation that is too high!%c");
				if(RGotPilot(mech) &&
				   MadePilotSkillRoll(mech,
									  (int) (MechFZ(mech)) / ZSCALE / 3)) {

					mech_notify(mech, MECHALL, "%chYou land safely.%c");
					LandMech(mech);

				} else {

					mech_notify(mech, MECHALL,
								"%chYou crash into the obstacle and fall from the sky!%c");
					MechLOSBroadcast(mech,
									 "crashes into an obstacle and falls from the sky!");
					MechFalls(mech, ed, 0);
					domino_space(mech, 2);
				}
			}
			return;
		}

		/* Walked into a wall silly */
		if(collision_check(mech, WALK_WALL, lastelevation, oldterrain)) {

			move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
			mech_notify(mech, MECHALL,
						"You attempt to climb a hill too steep for you.");

			if(MechPilot(mech) == -1 || (!mudconf.btech_skidcliff &&
                        MadePilotSkillRoll_NoXP(mech,
                            (int) (fabs((MechSpeed (mech)) + MP1) / MP1) / 3, 1))
			   || (mudconf.btech_skidcliff &&
                    MadePilotSkillRoll_NoXP(mech, 
                        SkidMod(fabs(MechSpeed(mech)) / MP1), 1))) {

				mech_notify(mech, MECHALL,
							"You manage to stop before crashing.");
				MechLOSBroadcast(mech, "stops suddenly to avoid a cliff!");

			} else {

				mech_notify(mech, MECHALL,
							"You run headlong into the cliff and fall down!");
				MechLOSBroadcast(mech,
								 "runs headlong into a cliff and falls down!");
				if(!mudconf.btech_skidcliff)
					MechFalls(mech,
							  (int) (1 + (MechSpeed(mech)) * MP_PER_KPH) / 4,
							  0);
				else
					MechFalls(mech, 1, 0);
			}
			MechDesiredSpeed(mech) = 0;
			MechSpeed(mech) = 0;
			MechZ(mech) = lastelevation;
			return;

		} else if(collision_check(mech, WALK_DROP, lastelevation, oldterrain)) {

			/* Walked off a cliff ... */
			mech_notify(mech, MECHALL,
						"You notice a large drop in front of you");
			avoidbth = mudconf.btech_skidcliff ?
				SkidMod(fabs(MechSpeed(mech)) / MP1) :
				((fabs((MechSpeed(mech)) + MP1) / MP1) / 3);

			if(MechPilot(mech) == -1 || (!MechAutoFall(mech) &&
										 MadePilotSkillRoll_NoXP(mech,
															avoidbth,1))) {

				mech_notify(mech, MECHALL,
							"You manage to stop before falling off.");
				MechLOSBroadcast(mech,
								 "stops suddenly to avoid falling off a cliff!");
				move_unit_back(mech, deltax, deltay, lastelevation, ot, le);

			} else {

				mech_notify(mech, MECHALL,
							"You run off the cliff and fall to the ground below.");
				MechLOSBroadcast(mech,
								 "runs off a cliff and falls to the ground below!");
				MechFalls(mech, lastelevation - elevation, 0);
				MechDesiredSpeed(mech) = 0;
				MechSpeed(mech) = 0;
			}
			MechDesiredSpeed(mech) = 0;
			MechSpeed(mech) = 0;
			return;

		} else if(mudconf.btech_roll_on_backwalk &&
				  (MechSpeed(mech) < 0) &&
				  (collision_check
				   (mech, WALK_BACK, lastelevation, oldterrain))) {

			mech_printf(mech, MECHALL, "You notice a %s behind you!",
						(elevation >
						 lastelevation ? "small incline" : "small drop"));

			if(MechPilot(mech) == -1
			   || (MadePilotSkillRoll(mech,
									  collision_check(mech, WALK_BACK,
													  lastelevation,
													  oldterrain)
									  - 1))) {

				mech_notify(mech, MECHALL,
							"You manage to overcome the obstacle.");

			} else {

				mech_printf(mech, MECHALL, "%s",
							(elevation > lastelevation ?
							 "You stumble on your rear and fall down." :
							 "You fall on your rear off the small incline."));

				/*! \todo {Get rid of this tprintf} */
				MechLOSBroadcast(mech,
								 tprintf("%s",
										 (elevation >
										  lastelevation ?
										  "falls on its back walking up an incline."
										  :
										  "falls off the back of a small incline.")));
				MechFalls(mech, abs(lastelevation - elevation), 1);
				MechDesiredSpeed(mech) = 0;
				MechSpeed(mech) = 0;
				if(elevation > lastelevation) {
					move_unit_back(mech, deltax, deltay, lastelevation, ot,
								   le);
				}
			}
			return;
		}

		/* Slow the unit if its made an elevation change */
		le = elevation - lastelevation;
		le = (le < 0) ? -le : le;
		if(MechZ(mech) != elevation)
			le = 0;
		if(le > 0) {
			deltax = (le == 1) ? MP1 : MP2;
			if(MechSpeed(mech) > 0) {
				MechSpeed(mech) -= deltax;
				if(MechSpeed(mech) < 0)
					MechSpeed(mech) = 0;
			} else if(MechSpeed(mech) < 0) {
				MechSpeed(mech) += deltax;
				if(MechSpeed(mech) > 0)
					MechSpeed(mech) = 0;
			}
		}

		if(MechType(mech) == CLASS_BSUIT) {

			/* Are they in water, also make sure it affects them */
			if(!(MechSpecials2(mech) & WATERPROOF_TECH) &&
			   (MechRTerrain(mech) == WATER ||
				(MechRTerrain(mech) == BRIDGE &&
				 (lastelevation < (elevation - 1)))) && elevation < 0) {

				mech_notify(mech, MECHALL,
							"You notice a body of water in front of you");

				if(MechPilot(mech) == -1 ||
				   MadePilotSkillRoll(mech,
									  (int) (fabs((MechSpeed(mech)) + MP1) /
											 MP1) / 3)) {

					mech_notify(mech, MECHALL,
								"You manage to stop before falling in.");
					MechLOSBroadcast(mech,
									 "stops suddenly to avoid going for a swim!");
				} else {

					mech_notify(mech, MECHALL,
								"You trip at the edge of the water and plunge in...");
					MechFloods(mech);
					return;
				}
				move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
				MechDesiredSpeed(mech) = 0;
				MechSpeed(mech) = 0;
				return;
			}

		} else if(!(MechSpecials2(mech) & WATERPROOF_TECH) &&
				  ((MechRTerrain(mech) == WATER && MechZ(mech) < 0) ||
				   (MechRTerrain(mech) == BRIDGE && MechZ(mech) < 0) ||
				   (MechRTerrain(mech) == ICE && MechZ(mech) < 0) ||
				   MechRTerrain(mech) == HIGHWATER) &&
				  MechType(mech) != CLASS_MW) {

			int skillmod, dammod;
			MechDesiredSpeed(mech) = MIN(MechDesiredSpeed(mech),
										 WalkingSpeed(MMaxSpeed(mech)));
#ifdef BT_MOVEMENT_MODES
			if(MechStatus2(mech) & SPRINTING) {
				MechLOSBroadcast(mech,
								 "breaks out of its sprint as it enters water!");
				mech_notify(mech, MECHALL,
							"You lose your sprinting momentum as you "
							"enter water!");
				if(!MoveModeChange(mech))
					MECHEVENT(mech, EVENT_MOVEMODE, mech_movemode_event, TURN,
						  MODE_OFF | MODE_SPRINT);
			}
#endif
			if(IsRunning(MechSpeed(mech), MMaxSpeed(mech))) {
				mech_notify(mech, MECHPILOT,
							"You struggle to keep control as you run into the water!");
				skillmod = 2;
				dammod = 2;
			} else {
				mech_notify(mech, MECHPILOT, "You use your piloting skill "
							"to maneuver through the water.");
				skillmod = 0;
				dammod = 0;
			}
			skillmod += (MechRTerrain(mech) == HIGHWATER ? -2 :
						 MechRTerrain(mech) ==
						 BRIDGE ? bridge_w_elevation(mech) : MechElev(mech) >
						 3 ? 1 : (MechElev(mech) - 2));

			if(!MadePilotSkillRoll(mech, skillmod)) {
				mech_notify(mech, MECHALL,
							"You slip in the water and fall down");
				MechLOSBroadcast(mech, "slips in the water and falls down!");
				MechFalls(mech, 1, dammod);
				done = 1;
			}
		}
		break;

	case MOVE_TRACK:

		if(collision_check(mech, WALK_WALL, lastelevation, oldterrain)) {
			mech_notify(mech, MECHALL,
						"You attempt to climb a hill too steep for you.");

			if(MechPilot(mech) == -1 || (!mudconf.btech_skidcliff &&
                        MadePilotSkillRoll_NoXP(mech,
                            (int) (fabs((MechSpeed (mech)) + MP1) / MP1) / 3, 1))
			   || (mudconf.btech_skidcliff &&
                    MadePilotSkillRoll_NoXP(mech, 
                        SkidMod(fabs(MechSpeed(mech)) / MP1), 1))) {

				mech_notify(mech, MECHALL,
							"You manage to stop before crashing.");
				MechLOSBroadcast(mech, "stops suddenly to avoid a cliff!");

			} else {

				if(!mudconf.btech_skidcliff) {
					mech_notify(mech, MECHALL, "You smash into a cliff!");
					MechLOSBroadcast(mech, "crashes to a cliff!");
					MechFalls(mech, (int) (MechSpeed(mech) * MP_PER_KPH / 4),
							  0);
				} else {
					mech_notify(mech, MECHALL,
								"You skid to a violent halt!");
					MechLOSBroadcast(mech, "goes into a skid!");
					MechFalls(mech, 0, 0);
				}
			}
			move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
			MechDesiredSpeed(mech) = 0;
			MechSpeed(mech) = 0;
			return;

		} else if(collision_check(mech, WALK_DROP, lastelevation, oldterrain)) {

			mech_notify(mech, MECHALL,
						"You notice a large drop in front of you");
			avoidbth = mudconf.btech_skidcliff ?
				SkidMod(fabs(MechSpeed(mech)) / MP1) :
				((fabs((MechSpeed(mech)) + MP1) / MP1) / 3);
			if(MechPilot(mech) == -1 || (!MechAutoFall(mech) &&
										 MadePilotSkillRoll_NoXP(mech,
															avoidbth,1))) {
				mech_notify(mech, MECHALL,
							"You manage to stop before falling off.");
				MechLOSBroadcast(mech,
								 "stops suddenly to avoid falling off a cliff!");
			} else {
				mech_notify(mech, MECHALL,
							"You drive off the cliff and fall to the ground below.");
				MechLOSBroadcast(mech,
								 "drives off a cliff and falls to the ground below.");
				MechFalls(mech, lastelevation - elevation, 0);
				domino_space(mech, 2);

				if(MechRTerrain(mech) == WATER &&
				   !(MechSpecials2(mech) & WATERPROOF_TECH)) {

					mech_notify(mech, MECHALL,
								"You drive into the water and your vehicle becomes inoperable.");
					Destroy(mech);
					ChannelEmitKill(mech, mech);
				}

				return;
			}
			move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
			MechDesiredSpeed(mech) = 0;
			MechSpeed(mech) = 0;
			return;

		} else if(mudconf.btech_roll_on_backwalk && (MechSpeed(mech) < 0) &&
				  (collision_check
				   (mech, WALK_BACK, lastelevation, oldterrain))) {

			mech_printf(mech, MECHALL, "You notice a %s behind you!",
						(elevation >
						 lastelevation ? "small incline" : "small drop"));

			if(MechPilot(mech) == -1 || (MadePilotSkillRoll(mech,
															collision_check
															(mech, WALK_BACK,
															 lastelevation,
															 oldterrain)
															- 1))) {

				mech_notify(mech, MECHALL,
							"You manage to overcome the obstacle.");
			} else {
				mech_printf(mech, MECHALL, "%s",
							(elevation > lastelevation ?
							 "You stumble on your rear and fall down." :
							 "You fall on your rear off the small incline."));
				MechLOSBroadcast(mech,
								 tprintf("%s",
										 (elevation >
										  lastelevation ?
										  "falls on its back walking up an incline."
										  :
										  "falls off the back of a small incline.")));
				MechFalls(mech, abs(lastelevation - elevation), 1);
				MechDesiredSpeed(mech) = 0;
				MechSpeed(mech) = 0;
				if(elevation > lastelevation) {
					move_unit_back(mech, deltax, deltay, lastelevation, ot,
								   le);
				}
			}
			return;
		}

		if(!(MechSpecials2(mech) & WATERPROOF_TECH) &&
		   (MechRTerrain(mech) == WATER ||
			(MechRTerrain(mech) == BRIDGE
			 && (lastelevation < (elevation - 1)))) && elevation < 0) {

			mech_notify(mech, MECHALL,
						"You notice a body of water in front of you");
			if(MechPilot(mech) == -1 ||
			   MadePilotSkillRoll(mech,
								  (int) (fabs((MechSpeed(mech)) + MP1) /
										 MP1) / 3)) {
				mech_notify(mech, MECHALL,
							"You manage to stop before falling in.");
				MechLOSBroadcast(mech,
								 "stops suddenly to avoid driving into the water!");
			} else {
				mech_notify(mech, MECHALL,
							"You drive into the water and your vehicle becomes inoperable.");
				Destroy(mech);
				ChannelEmitKill(mech, mech);
				return;
			}
			move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
			MechDesiredSpeed(mech) = 0;
			MechSpeed(mech) = 0;
			return;
		}

		/* New terrain restrictions */
		if(mudconf.btech_newterrain) {
			tt = MechRTerrain(mech);
			if((tt == HEAVY_FOREST) && fabs(MechSpeed(mech)) > MP1) {

#if 0
				mech_notify(mech, MECHALL, "You cruise at a bunch of trees!");
#endif
				mech_notify(mech, MECHALL,
							"You try to dodge the larger trees..");

				if(MechPilot(mech) == -1 || MadePilotSkillRoll(mech,
															   (int) (fabs
																	  (MechSpeed
																	   (mech))
																	  / MP1 /
																	  6))) {

					mech_notify(mech, MECHALL, "You manage to dodge 'em!");
				} else {
					mech_notify(mech, MECHALL,
								"You swerve, but not enough! This'll hurt!");
					MechLOSBroadcast(mech, "cruises headlong at a tree!");
					f = fabs(MechSpeed(mech));
					MechSpeed(mech) = MechSpeed(mech) / 2.0;
					MechFalls(mech, MAX(1, (int) sqrt(f / MP1 / 2)), 0);
				}
			}
		}

		/* Slow them if they made an elevation change */
		le = elevation - lastelevation;
		le = (le < 0) ? -le : le;
		if(le > 0) {
			deltax = (le == 1) ? MP2 : MP3;
			if(MechSpeed(mech) > 0) {
				MechSpeed(mech) -= deltax;
				if(MechSpeed(mech) < 0)
					MechSpeed(mech) = 0;
			} else if(MechSpeed(mech) < 0) {
				MechSpeed(mech) += deltax;
				if(MechSpeed(mech) > 0)
					MechSpeed(mech) = 0;
			}
		}
		break;

	case MOVE_WHEEL:

		/* Cliff ! */
		if(collision_check(mech, WALK_WALL, lastelevation, oldterrain)) {

			mech_notify(mech, MECHALL,
						"You attempt to climb a hill too steep for you.");

			if(MechPilot(mech) == -1 || (!mudconf.btech_skidcliff &&
                        MadePilotSkillRoll_NoXP(mech,
                            (int) (fabs((MechSpeed (mech)) + MP1) / MP1) / 3, 1))
			   || (mudconf.btech_skidcliff &&
                    MadePilotSkillRoll_NoXP(mech, 
                        SkidMod(fabs(MechSpeed(mech)) / MP1), 1))) {

				mech_notify(mech, MECHALL,
							"You manage to stop before crashing.");
				MechLOSBroadcast(mech, "stops suddenly to avoid a cliff!");

			} else {

				if(!mudconf.btech_skidcliff) {
					mech_notify(mech, MECHALL, "You smash into a cliff!");
					MechLOSBroadcast(mech, "crashes to a cliff!");
					MechFalls(mech, (int) (MechSpeed(mech) * MP_PER_KPH / 4),
							  0);
				} else {
					mech_notify(mech, MECHALL, "You skid to a violent halt!");
					MechLOSBroadcast(mech, "skids to a halt!");
					MechFalls(mech, 0, 0);
				}
			}

			move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
			MechDesiredSpeed(mech) = 0;
			MechSpeed(mech) = 0;
			return;

		} else if(collision_check(mech, WALK_DROP, lastelevation, oldterrain)) {

			mech_notify(mech, MECHALL,
						"You notice a large drop in front of you");
			avoidbth = mudconf.btech_skidcliff ?
				SkidMod(fabs(MechSpeed(mech)) / MP1) :
				((fabs((MechSpeed(mech)) + MP1) / MP1) / 3);

			if(MechPilot(mech) == -1 || (!MechAutoFall(mech) &&
										 MadePilotSkillRoll_NoXP(mech,
															avoidbth,1))) {

				mech_notify(mech, MECHALL,
							"You manage to stop before falling off.");
				MechLOSBroadcast(mech,
								 "stops suddenly to avoid driving off a cliff!");
			} else {
				mech_notify(mech, MECHALL,
							"You drive off the cliff and fall to the ground below.");
				MechLOSBroadcast(mech,
								 "drives off a cliff and falls to the ground below.");
				MechFalls(mech, lastelevation - elevation, 0);
				domino_space(mech, 2);

				if(MechRTerrain(mech) == WATER &&
				   !(MechSpecials2(mech) & WATERPROOF_TECH)) {

					mech_notify(mech, MECHALL,
								"You drive into the water and your vehicle becomes inoperable.");
					Destroy(mech);
					ChannelEmitKill(mech, mech);
				}

				return;
			}
			move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
			MechDesiredSpeed(mech) = 0;
			MechSpeed(mech) = 0;
			return;

		} else if(mudconf.btech_roll_on_backwalk &&
				  (MechSpeed(mech) < 0) &&
				  (collision_check
				   (mech, WALK_BACK, lastelevation, oldterrain))) {

			mech_printf(mech, MECHALL, "You notice a %s behind you!",
						(elevation >
						 lastelevation ? "small incline" : "small drop"));

			if(MechPilot(mech) == -1 || (MadePilotSkillRoll(mech,
															collision_check
															(mech, WALK_BACK,
															 lastelevation,
															 oldterrain)
															- 1))) {
				mech_notify(mech, MECHALL,
							"You manage to overcome the obstacle.");
			} else {
				mech_printf(mech, MECHALL, "%s",
							(elevation > lastelevation ?
							 "You stumble on your rear and fall down." :
							 "You fall on your rear off the small incline."));
				MechLOSBroadcast(mech,
								 tprintf("%s", (elevation > lastelevation ?
												"falls on its back walking up an incline."
												:
												"falls off the back of a small incline.")));
				MechFalls(mech, abs(lastelevation - elevation), 1);
				MechDesiredSpeed(mech) = 0;
				MechSpeed(mech) = 0;
				if(elevation > lastelevation) {
					move_unit_back(mech, deltax, deltay, lastelevation, ot,
								   le);
				}
			}
			return;
		}

		if(!(MechSpecials2(mech) & WATERPROOF_TECH) &&
		   (MechRTerrain(mech) == WATER ||
			(MechRTerrain(mech) == BRIDGE
			 && (lastelevation < (elevation - 1)))) && elevation < 0) {

			mech_notify(mech, MECHALL,
						"You notice a body of water in front of you");

			if(MechPilot(mech) == -1 ||
			   MadePilotSkillRoll(mech,
								  (int) (fabs((MechSpeed(mech)) + MP1) /
										 MP1) / 3)) {

				mech_notify(mech, MECHALL,
							"You manage to stop before falling in.");
				MechLOSBroadcast(mech,
								 "stops suddenly to driving into the water!");
			} else {
				mech_notify(mech, MECHALL,
							"You drive into the water and your vehicle becomes inoperable.");
				Destroy(mech);
				ChannelEmitKill(mech, mech);
				return;
			}
			move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
			MechDesiredSpeed(mech) = 0;
			MechSpeed(mech) = 0;
			return;
		}

		/* New terrain restrictions */
		if(mudconf.btech_newterrain) {
			tt = MechRTerrain(mech);
			if((tt == HEAVY_FOREST || tt == LIGHT_FOREST) &&
			   fabs(MechSpeed(mech)) > MP1) {

#if 0
				mech_notify(mech, MECHALL, "You cruise at a bunch of trees!");
#endif
				mech_notify(mech, MECHALL,
							"You try to dodge the larger trees..");
				if(MechPilot(mech) == -1 ||
				   MadePilotSkillRoll(mech,
									  (tt == HEAVY_FOREST ? 3 : 0) +
									  (fabs(MechSpeed(mech)) / MP1 / 6))) {

					mech_notify(mech, MECHALL, "You manage to dodge 'em!");

				} else {
					mech_notify(mech, MECHALL,
								"You swerve, but not enough! This'll hurt!");
					MechLOSBroadcast(mech, "cruises headlong at a tree!");
					f = fabs(MechSpeed(mech));
					MechSpeed(mech) = MechSpeed(mech) / 2.0;
					MechFalls(mech, MAX(1, (int) sqrt(f / MP1 / 2)), 0);
				}

			} else if((tt == ROUGH) && fabs(MechSpeed(mech)) > MP1) {
#if 0
				mech_notify(mech, MECHALL,
							"You cruise at some rough terrain!");
#endif
				mech_notify(mech, MECHALL, "You try to avoid the rocks..");
				if(MechPilot(mech) == -1 || MadePilotSkillRoll(mech,
															   (int) (fabs
																	  (MechSpeed
																	   (mech))
																	  / MP1 /
																	  6))) {
					mech_notify(mech, MECHALL, "You manage to dodge 'em!");
				} else {
					mech_notify(mech, MECHALL,
								"You swerve, but not enough! This'll hurt!");
					MechLOSBroadcast(mech, "cruises headlong at a rock!");
					f = fabs(MechSpeed(mech));
					MechSpeed(mech) = MechSpeed(mech) / 2.0;
					MechFalls(mech, MAX(1, (int) sqrt(f / MP1 / 2)), 0);
				}
			}
		}

		/* Slow them down if they change elevations */
		le = elevation - lastelevation;
		le = (le < 0) ? -le : le;
		if(le > 0) {
			deltax = (le == 1) ? MP2 : MP3;
			if(MechSpeed(mech) > 0) {
				MechSpeed(mech) -= deltax;
				if(MechSpeed(mech) < 0)
					MechSpeed(mech) = 0;
			} else if(MechSpeed(mech) < 0) {
				MechSpeed(mech) += deltax;
				if(MechSpeed(mech) > 0)
					MechSpeed(mech) = 0;
			}
		}
		break;

	case MOVE_HULL:
	case MOVE_FOIL:
	case MOVE_SUB:

		if((MechRTerrain(mech) != WATER && MechRTerrain(mech) != BRIDGE)
		   || abs(MechElev(mech)) < (abs(MechZ(mech)) + (MechMove(mech) ==
														 MOVE_FOIL ? -1 : 0)))
		{
			/* Run aground */
			MechElev(mech) = le;
			MechTerrain(mech) = ot;
			mech_notify(mech, MECHALL,
						"You attempt to get too close with ground!");
			if(MechPilot(mech) == -1 ||
			   MadePilotSkillRoll(mech,
								  (int) (fabs((MechSpeed(mech)) + MP1) /
										 MP1) / 3)) {
				mech_notify(mech, MECHALL,
							"You manage to stop before crashing.");
				MechLOSBroadcast(mech,
								 "stops suddenly to avoid running aground!");
				move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
			} else {
				mech_notify(mech, MECHALL, "You smash into the ground!");
				MechLOSBroadcast(mech, "smashes aground!");
				MechFalls(mech, (int) (MechSpeed(mech) * MP_PER_KPH / 4), 0);
			}
			MechSpeed(mech) = 0;
			MechDesiredSpeed(mech) = 0;
			MechVerticalSpeed(mech) = 0;
			return;
		}
		if(elevation > 0)
			elevation = 0;
		break;

	case MOVE_HOVER:

		if(collision_check(mech, WALK_WALL, lastelevation, oldterrain)) {
			MechElev(mech) = le;
			MechTerrain(mech) = ot;
			mech_notify(mech, MECHALL,
						"You attempt to climb a hill too steep for you.");
			if(MechPilot(mech) == -1 || (!mudconf.btech_skidcliff &&
                        MadePilotSkillRoll_NoXP(mech,
                            (int) (fabs((MechSpeed (mech)) + MP1) / MP1) / 3, 1))
			   || (mudconf.btech_skidcliff &&
                    MadePilotSkillRoll_NoXP(mech, 
                        SkidMod(fabs(MechSpeed(mech)) / MP1), 1))) {

				mech_notify(mech, MECHALL,
							"You manage to stop before crashing.");
				MechLOSBroadcast(mech, "stops suddenly to avoid a cliff!");

			} else {

				if(!mudconf.btech_skidcliff) {
					mech_notify(mech, MECHALL, "You smash into a cliff!");
					MechLOSBroadcast(mech, "smashes into a cliff!");
					MechFalls(mech,
							  (int) (MechSpeed(mech) * MP_PER_KPH / 4), 0);
				} else {
					mech_notify(mech, MECHALL, "You skid to a violent halt!");
					MechLOSBroadcast(mech, "skids to a halt!");
					MechFalls(mech, 0, 0);
				}
			}
			move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
			MechDesiredSpeed(mech) = 0;
			MechSpeed(mech) = 0;
			return;

		} else if(collision_check(mech, WALK_DROP, lastelevation, oldterrain)) {

			mech_notify(mech, MECHALL,
						"You notice a large drop in front of you");

			avoidbth = mudconf.btech_skidcliff ?
				SkidMod(fabs(MechSpeed(mech)) / MP1) :
				((fabs((MechSpeed(mech)) + MP1) / MP1) / 3);

			if(MechPilot(mech) == -1 || (!MechAutoFall(mech) &&
										 MadePilotSkillRoll_NoXP(mech,
															avoidbth,1))) {

				mech_notify(mech, MECHALL,
							"You manage to stop before falling off.");
				MechLOSBroadcast(mech,
								 "stops suddenly to avoid falling off a cliff!");

			} else {

				mech_notify(mech, MECHALL,
							"You drive off the cliff and fall to the ground below.");
				MechLOSBroadcast(mech,
								 "drives off a cliff and falls to the ground below.");
				MechFalls(mech, lastelevation - elevation, 0);
				return;
			}

			move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
			MechDesiredSpeed(mech) = 0;
			MechSpeed(mech) = 0;
			return;

		} else if(collision_check(mech, HIT_UNDER_BRIDGE, lastelevation,
								  oldterrain)) {

			mech_notify(mech, MECHALL,
						"You notice the underside of the bridge in front of you!");

			if(MechPilot(mech) == -1 || (!mudconf.btech_skidcliff &&
										 MadePilotSkillRoll(mech,
															(int) (fabs
																   ((MechSpeed
																	 (mech)) +
																	MP1) /
																   MP1) / 3))
			   || (mudconf.btech_skidcliff
				   && MadePilotSkillRoll(mech,
										 SkidMod(fabs(MechSpeed(mech)) /
												 MP1)))) {

				mech_notify(mech, MECHALL,
							"You manage to stop before slamming into the bridge.");
				MechLOSBroadcast(mech,
								 "stops suddenly to avoid slamming into the bridge!");
			} else {
				mech_notify(mech, MECHALL,
							"You drive right into the underside of the bridge.");
				MechLOSBroadcast(mech,
								 "drives right into the underside of the bridge.");
				MechFalls(mech, 1, 0);
			}
			move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
			MechDesiredSpeed(mech) = 0;
			MechSpeed(mech) = 0;
			return;

		} else if(mudconf.btech_roll_on_backwalk &&
				  (MechSpeed(mech) < 0) &&
				  (collision_check
				   (mech, WALK_BACK, lastelevation, oldterrain))
				  && !isunder) {

			mech_printf(mech, MECHALL, "You notice a %s behind you!",
						(elevation >
						 lastelevation ? "small incline" : "small drop"));

			if(MechPilot(mech) == -1 || (MadePilotSkillRoll(mech,
															collision_check
															(mech, WALK_BACK,
															 lastelevation,
															 oldterrain)
															- 1))) {

				mech_notify(mech, MECHALL,
							"You manage to overcome the obstacle.");

			} else {

				mech_printf(mech, MECHALL, "%s",
							(elevation > lastelevation ?
							 "You stumble on your rear and fall down." :
							 "You fall on your rear off the small incline."));
				MechLOSBroadcast(mech,
								 tprintf("%s", (elevation > lastelevation ?
												"falls on its back walking up an incline."
												:
												"falls off the back of a small incline.")));
				MechFalls(mech, abs(lastelevation - elevation), 1);
				MechDesiredSpeed(mech) = 0;
				MechSpeed(mech) = 0;
				if(elevation > lastelevation) {
					move_unit_back(mech, deltax, deltay, lastelevation, ot,
								   le);
				}
			}
			return;
		}

		tt = MechRTerrain(mech);
		if((tt == HEAVY_FOREST || tt == LIGHT_FOREST) &&
		   fabs(MechSpeed(mech)) > MP1) {
#if 0
			mech_notify(mech, MECHALL, "You cruise at a bunch of trees!");
#endif
			mech_notify(mech, MECHALL, "You try to dodge the larger trees..");

			if(MechPilot(mech) == -1 ||
			   MadePilotSkillRoll(mech,
								  (tt == HEAVY_FOREST ? 3 : 0) +
								  (fabs(MechSpeed(mech)) / MP1 / 6))) {

				mech_notify(mech, MECHALL, "You manage to dodge 'em!");

			} else {
				mech_notify(mech, MECHALL,
							"You swerve, but not enough! This'll hurt!");
				MechLOSBroadcast(mech, "cruises headlong at a tree!");
				f = fabs(MechSpeed(mech));
				MechSpeed(mech) = MechSpeed(mech) / 2.0;
				MechFalls(mech, MAX(1, (int) sqrt(f / MP1 / 2)), 0);
			}
		}

		/* Slow the unit down if its made an elevation change */
		le = elevation - lastelevation;
		le = (le < 0) ? -le : le;
		if(le > 0) {
			deltax = (le == 1) ? MP2 : MP3;
			if(MechSpeed(mech) > 0) {
				MechSpeed(mech) -= deltax;
				if(MechSpeed(mech) < 0)
					MechSpeed(mech) = 0;
			} else if(MechSpeed(mech) < 0) {
				MechSpeed(mech) += deltax;
				if(MechSpeed(mech) > 0)
					MechSpeed(mech) = 0;
			}
		}
		break;

	case MOVE_VTOL:
	case MOVE_FLY:
		
		if((Landed(mech) && MechRTerrain(mech) != ROAD &&
		   MechRTerrain(mech) != BRIDGE && MechRTerrain(mech) != GRASSLAND
		   && MechRTerrain(mech) != BUILDING) ||
		   (IsForest(MechRTerrain(mech)) && MechZ(mech) < (MechElevation(mech)+2))) {

			mech_notify(mech, MECHALL,
						"You go where no flying thing has ever gone before..");
			if(RGotPilot(mech) && MadePilotSkillRoll(mech, 5)) {
				mech_notify(mech, MECHALL, "You stop in time!");
				move_unit_back(mech, deltax, deltay, lastelevation, ot, le);
			} else {
				mech_notify(mech, MECHALL,
							"Eww.. You've a bad feeling about this.");
				MechLOSBroadcast(mech, "crashes!");
				MechFalls(mech, 1, 0);
			}
			MechDesiredSpeed(mech) = 0;
			MechSpeed(mech) = 0;
			return;

		} else if(Landed(mech) && mudconf.btech_roll_on_backwalk &&
				  (MechSpeed(mech) < 0) &&
				  (collision_check
				   (mech, WALK_BACK, lastelevation, oldterrain))) {

			mech_printf(mech, MECHALL, "You notice a %s behind you!",
						(elevation >
						 lastelevation ? "small incline" : "small drop"));

			if(MechPilot(mech) == -1 || (MadePilotSkillRoll(mech,
															collision_check
															(mech, WALK_BACK,
															 lastelevation,
															 oldterrain)
															- 1))) {

				mech_notify(mech, MECHALL,
							"You manage to overcome the obstacle.");

			} else {

				mech_printf(mech, MECHALL, "%s", (elevation > lastelevation ?
												  "You stumble on your rear and fall down."
												  :
												  "You fall on your rear off the small incline."));
				MechLOSBroadcast(mech,
								 tprintf("%s",
										 (elevation >
										  lastelevation ?
										  "falls on its back walking up an incline."
										  :
										  "falls off the back of a small incline.")));
				MechFalls(mech, (abs(lastelevation - elevation) + 1000), 1);
				MechDesiredSpeed(mech) = 0;
				MechSpeed(mech) = 0;
				if(elevation > lastelevation) {
					move_unit_back(mech, deltax, deltay, lastelevation, ot,
								   le);
				}
			}
			return;
		}

		if(MechRTerrain(mech) == WATER)
			return;

		if(MechRTerrain(mech) == LIGHT_FOREST ||
		   MechRTerrain(mech) == HEAVY_FOREST)
			elevation = MechElevation(mech) + 2;
		else
			elevation = MechElevation(mech);

		if(collision_check(mech, JUMP, 0, 0)) {
			MechFX(mech) -= deltax;
			MechFY(mech) -= deltay;
			MechX(mech) = MechLastX(mech);
			MechY(mech) = MechLastY(mech);
			MechZ(mech) = lastelevation;
			MechFZ(mech) = MechZ(mech) * ZSCALE;
			MechElev(mech) = le;
			MechTerrain(mech) = ot;
			mech_notify(mech, MECHALL,
						"You attempt to fly over elevation that is too high!");

			if(MechPilot(mech) == -1 || (MadePilotSkillRoll(mech,
															(int) (MechFZ
																   (mech) /
																   ZSCALE /
																   3))
										 && (ot == GRASSLAND || ot == ROAD
											 || ot == BUILDING))) {

				mech_notify(mech, MECHALL, "You land safely.");
				MechStatus(mech) |= LANDED;
				MechSpeed(mech) = 0.0;
				MechVerticalSpeed(mech) = 0.0;

			} else {
				mech_notify(mech, MECHALL,
							"You crash into the obstacle and fall from the sky!");
				MechLOSBroadcast(mech,
								 "crashes into an obstacle and falls from the sky!");
				MechFalls(mech, MechsElevation(mech) + 1, 0);
				domino_space(mech, 2);
			}
		}
		break;
	}

	if(!done) {
		possible_mine_poof(mech, MINE_STEP);
		if(mudconf.btech_fasaadvvhlfire &&
		   (MechType(mech) == CLASS_VEH_GROUND) &&
		   (MechTerrain(mech) == FIRE))
			checkVehicleInFire(mech, 1);
	}
}

void CheckDamage(MECH * wounded)
{
	/* should be called from UpdatePilotSkillRolls */
	/* this is so that a roll will be made only when the mech takes damage */
	int now = muxevent_tick % TURN;
	int staggerLevel = 0;
	int headingChange = 0;

	if(mudconf.btech_newstagger) {

		/* phase out post-stagger assistance - see check_stagger_event() */
		if(StaggerDamage(wounded) < 0) {
			StaggerDamage(wounded) += 1;
			return;
		}
		if(StaggerDamage(wounded) == 0) {
			return;
		}

		staggerLevel = StaggerLevel(wounded);

		if(Fallen(wounded) || (MechType(wounded) != CLASS_MECH)) {
			if(CheckingStaggerDamage(wounded) || (StaggerDamage(wounded) > 0))
				StopStaggerCheck(wounded);

			return;
		}

		if(!CheckingStaggerDamage(wounded)) {
			if((Jumping(wounded) && (staggerLevel < 1)) || !Jumping(wounded))
				StartStaggerCheck(wounded);
		}

		if(staggerLevel < 1) {
			return;
		}

		SendDebug(tprintf("For %d. StaggerDamage: %d. StaggerLevel: %d.",
						  wounded->mynum, StaggerDamage(wounded),
						  StaggerLevel(wounded)));

		if(LastStaggerNotify(wounded) < staggerLevel) {
			LastStaggerNotify(wounded) = staggerLevel;

			if(Jumping(wounded) || OODing(wounded)) {
				switch (staggerLevel) {
				case 1:
					mech_notify(wounded, MECHALL,
								"%cy%chThe damage causes you to spin a little in your flight path.%cn");
					headingChange = 15;
					break;

				case 2:
					mech_notify(wounded, MECHALL,
								"%crThe damage spins you but you maintain your flight!%cn");
					MechLOSBroadcast(wounded,
									 "turns slightly from the damage!");
					headingChange = 45;
					break;

				default:
					mech_notify(wounded, MECHALL,
								"%cr%chThe damage causes you to spin completely around!%cn");
					MechLOSBroadcast(wounded,
									 "spins around from the damage!");
					headingChange = 180;
					break;
				}

				SetFacing(wounded,
						  AcceptableDegree((MechFacing(wounded) +
										   headingChange) * (Roll() >=
															  6 ? 1 : -1)));
			} else {
				switch (staggerLevel) {
				case 1:
					mech_notify(wounded, MECHALL,
								"%cy%chThe damage causes you to stagger a little.%cn");
					break;

				case 2:
					mech_notify(wounded, MECHALL,
								"%crThe damage causes you to stagger even more!%cn");
					MechLOSBroadcast(wounded,
									 "starts to stagger from the damage!");
					break;

				default:
					mech_notify(wounded, MECHALL,
								"%cr%chThe damage causes you to stagger violently while attempting to keep your footing!%cn");
					MechLOSBroadcast(wounded,
									 "staggers back and forth attempting to keep its footing!");
					break;
				}
			}
		}
	} else {
		if(!IsDS(wounded) && MechTurnDamage(wounded) >= 20 &&
		   (!MechStaggeredLastTurn(wounded) ||
			MechStaggerStamp(wounded) == now)) {

			if(!Jumping(wounded) && !Fallen(wounded) && !OODing(wounded)) {
				mech_notify(wounded, MECHALL, "You stagger from the damage!");
				if(!MadePilotSkillRoll(wounded, 1)) {
					mech_notify(wounded, MECHALL,
								"You fall over from all the damage!");
					MechLOSBroadcast(wounded,
									 "falls down, staggered by the damage!");
					MechFalls(wounded, 1, 0);
				}
			}
			MechTurnDamage(wounded) = 0;
			SetMechStaggerStamp(wounded, now);
			return;
		}
		if((MechStaggeredLastTurn(wounded) &&
			MechStaggerStamp(wounded) == now) ||
		   (!MechStaggeredLastTurn(wounded) && !now)) {
			MechTurnDamage(wounded) = 0;
			SetMechStaggerStamp(wounded, -1);
		}
	}
}

void UpdatePilotSkillRolls(MECH * mech)
{
	int makeroll = 0, grav = 0;
	float maxspeed;

	if(((muxevent_tick % TURN) == 0) && !Fallen(mech) && !Jumping(mech) &&
	   !OODing(mech))
		/* do this once a turn (30 secs), only if mech is standing */
	{
		maxspeed = MMaxSpeed(mech);

		if(!Started(mech))
			makeroll = 4;

		if((MechHeat(mech) >= 9.) &&
		   (MechSpecials(mech) & TRIPLE_MYOMER_TECH))
			maxspeed =
				ceil((rint((MMaxSpeed(mech) / 1.5) / MP1) + 1) * 1.5) * MP1;
		/* maxspeed += 1.5 * MP1; */
#ifndef BT_MOVEMENT_MODES
		if(InSpecial(mech) && InGravity(mech))
#else
		if(InSpecial(mech) && InGravity(mech) && !MoveModeChange(mech))
#endif
			if(MechSpeed(mech) > MechMaxSpeed(mech) &&
			   MechType(mech) == CLASS_MECH) {
				grav = 1;
				makeroll = 1;
			}
		if(IsRunning(MechSpeed(mech), maxspeed) &&
		   ((MechCritStatus(mech) & GYRO_DAMAGED) ||
			(MechCritStatus(mech) & HIP_DAMAGED)))
			makeroll = 1;

		if(makeroll) {
			if(!MadePilotSkillRoll(mech, (makeroll - 1))) {
				if(grav) {
					int dam =
						(MechSpeed(mech) - MechMaxSpeed(mech)) / MP1 + 1;
					mech_notify(mech, MECHALL, "Your legs take some damage!");
					if(MechIsQuad(mech)) {
						if(!SectIsDestroyed(mech, LARM))
							DamageMech(mech, mech, 0, -1, LARM, 0, 0, 0,
									   dam, 0, 0, -1, 0, 1);
						if(!SectIsDestroyed(mech, RARM))
							DamageMech(mech, mech, 0, -1, RARM, 0, 0, 0,
									   dam, 0, 0, -1, 0, 1);

					}
					if(!SectIsDestroyed(mech, LLEG))
						DamageMech(mech, mech, 0, -1, LLEG, 0, 0, 0,
								   dam, 0, 0, -1, 0, 1);
					if(!SectIsDestroyed(mech, RLEG))
						DamageMech(mech, mech, 0, -1, RLEG, 0, 0, 0,
								   dam, 0, 0, -1, 0, 1);
				} else {
					mech_notify(mech, MECHALL,
								"Your damaged mech falls as you try to run!");
					MechLOSBroadcast(mech, "falls down.");
					MechFalls(mech, 1, 0);
				}
			}
		}
	}
	if(MechType(mech) == CLASS_MECH)
		CheckDamage(mech);
	else
		MechTurnDamage(mech) = 0;
	if((muxevent_tick % TURN) == 0) {
		if(Started(mech) && MechMove(mech) != MOVE_NONE)
			CheckGenericFail(mech, -1, NULL, NULL);
	}
}

void updateAutoturnTurret(MECH * mech)
{
	MECH *target;
	int bearing;
	float fx, fy;

	if(!Started(mech) || Uncon(mech) || Blinded(mech))
		return;

	if((MechTankCritStatus(mech) & TURRET_JAMMED) ||
	   (MechTankCritStatus(mech) & TURRET_LOCKED))
		return;

	if(!GetSectInt(mech, TURRET))
		return;

	if(MechTarget(mech) == -1 && (MechTargY(mech) == -1 ||
								  MechTargX(mech) == -1))
		return;

	if(MechTarget(mech) != -1) {
		target = getMech(MechTarget(mech));
		fx = MechFX(target);
		fy = MechFY(target);
	} else {
		MapCoordToRealCoord(MechTargX(mech), MechTargY(mech), &fx, &fy);
	}

	bearing =
		AcceptableDegree(FindBearing(MechFX(mech), MechFY(mech), fx,
									 fy) - MechFacing(mech));
	MechTurretFacing(mech) = bearing;
	MarkForLOSUpdate(mech);
}

/* This function is called once every second for every mech in the game */
void mech_update(dbref key, void *data)
{
	MECH *mech = (MECH *) data;

	if(!mech)
		return;
	MechStatus(mech) &= ~FIRED;
	if(is_aero(mech)) {
		aero_update(mech);
		return;
	}
	if(Started(mech) || Uncon(mech))
		UpdatePilotSkillRolls(mech);
	if(Started(mech) || MechPlusHeat(mech) > 0.1)
		UpdateHeat(mech);
	if(Started(mech))
		MechVisMod(mech) =
			BOUNDED(0, MechVisMod(mech) + Number(-40, 40), 100);
	checkECM(mech);
	checkTAG(mech);
	end_lite_check(mech);

	if(MechStatus2(mech) & AUTOTURN_TURRET)
		updateAutoturnTurret(mech);
}
