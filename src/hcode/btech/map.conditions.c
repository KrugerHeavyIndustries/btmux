
/*
 * $Id: map.conditions.c,v 1.2 2005/01/15 16:57:14 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Wed Apr 23 15:18:01 1997 fingon
 * Last modified: Thu Sep 10 07:35:26 1998 fingon
 *
 */

#include "mech.h"
#include "mech.events.h"
#include "p.map.conditions.h"
#include "p.mech.utils.h"
#include "p.mech.combat.h"
#include "p.mech.combat.misc.h"
#include "p.mech.damage.h"
#include "p.artillery.h"
#include "p.btechstats.h"
#include "p.eject.h"
#include "p.mech.sensor.h"
#include "p.crit.h"

void alter_conditions(MAP * map)
{
    MECH *mech;
    dllist_node *node;

    for (node = dllist_head(map->mechs); node; node = dllist_next(node)) {

        if ((mech = getMech((int) dllist_data(node)))) {
            UpdateConditions(mech, map);
#if 0
            mech_notify(mech, MECHALL,
                    "You notice a great disturbance in the Force..");
#endif
        }
    }
}

void map_setconditions(dbref player, MAP * map, char *buffer)
{
	char *args[5];
	int vacuum = -1, underground = -1, grav, temp, argc;
	int fl;

	DOCHECK((argc =
			 mech_parseattributes(buffer, args, 4)) < 2,
			"(At least) 2 options required (gravity + temperature)");
	DOCHECK(argc > 4,
			"Too many options! Command accepts only 4 at max (gravity + temperature + vacuum-flag + underground-flag)");
	DOCHECK(Readnum(grav, args[0]),
			"Invalid gravity (must be integer in range of 0 to 255)");
	DOCHECK(grav < 0 ||
			grav > 255,
			"Invalid gravity (must be integer in range of 0 to 255)");
	DOCHECK(Readnum(temp, args[1]),
			"Invalid temperature (must be integer in range of -128 to 127");
	DOCHECK(temp < -128 ||
			temp > 127,
			"Invalid temperature (must be integer in range of -128 to 127");
	if(argc > 2) {
		DOCHECK(Readnum(vacuum, args[2]),
				"Invalid vacuum flag (must be integer, 0 or 1)");
		DOCHECK(vacuum < 0 ||
				vacuum > 1, "Invalid vacuum flag (must be integer, 0 or 1)");
	}
	if(argc > 3) {
		DOCHECK(Readnum(underground, args[3]),
				"Invalid underground flag (must be integer, 0 or 1)");
		DOCHECK(underground < 0 ||
				underground > 1,
				"Invalid underground flag (must be integer, 0 or 1)");
	}
	fl = (map->flags & (~(MAPFLAG_SPEC | MAPFLAG_VACUUM)));
	if(vacuum > 0)
		fl |= MAPFLAG_VACUUM;
	if(underground > 0)
		fl |= MAPFLAG_UNDERGROUND;
	if(fl & MAPFLAG_VACUUM)
		fl |= MAPFLAG_SPEC;
	if(temp < -30 || temp > 50 || grav != 100)
		fl |= MAPFLAG_SPEC;
	map->temp = temp;
	map->grav = grav;
	map->flags = fl;
	notify(player, "Conditions set!");
	alter_conditions(map);
}

void UpdateConditions(MECH * mech, MAP * map)
{
	if(!mech)
		return;
	MechStatus(mech) &= ~CONDITIONS;
	if(!map)
		return;
	if(!MapUnderSpecialRules(map))
		return;
	MechStatus(mech) |= UNDERSPECIAL;
	if(MapTemperature(map) < -30 || MapTemperature(map) > 50)
		MechStatus(mech) |= UNDERTEMPERATURE;
	if(MapGravity(map) != 100)
		MechStatus(mech) |= UNDERGRAVITY;
	if(MapIsVacuum(map))
		MechStatus(mech) |= UNDERVACUUM;
}

void reactor_explosion(MECH * wounded, MECH * attacker)
{
	int z = MechZ(wounded);
	MAP *map = getMap(wounded->mapindex);
	dbref wounded_pilot = MechPilot(wounded);
	int dam;

	DestroySection(wounded, attacker, 0, CTORSO);
	DestroySection(wounded, attacker, 0, LTORSO);
	DestroySection(wounded, attacker, 0, RTORSO);
	DestroySection(wounded, attacker, 0, LLEG);
	DestroySection(wounded, attacker, 0, RLEG);

	/* Need to autoeject before the explosion reaches the head */
	if(!MapIsUnderground(map))
		autoeject(wounded_pilot, wounded, 0);

	DestroySection(wounded, attacker, 0, HEAD);
	MechZ(wounded) += 6;
	dam = MAX(MechTons(wounded) / 5, MechEngineSize(wounded) / 10);

	ScrambleInfraAndLiteAmp(wounded, 4, 0,
			"The searing blast of heat burns out your sensors!",
			"The blinding flash of light overloads your sensors!");

	blast_hit_hexesf(map, dam, 3,
			MAX(MechTons(wounded) / 10,
				MechEngineSize(wounded) / 25),
			MechFX(wounded), MechFY(wounded),
			MechFX(wounded), MechFY(wounded),
			"%ch%crYou bear full brunt of the blast!%cn",
			"is hit badly by the blast!",
			"%ch%cyYou receive some damage from the blast!%cn",
			"is hit by the blast!",
			mudconf.btech_explode_reactor > 1, 3, 5, 1, 2);
	MechZ(wounded) = z;
	headhitmwdamage(wounded, attacker, 4);
}

void DestroyParts(MECH * attacker, MECH * wounded, int hitloc, int breach,
				  int IsDisable)
{
	float oldjs;
	int i;
	int critType;
	int nhs = 0;
	int tDoAutoFall = 0;
	int tNormalizeAllCrits = 0;
	int tNormalizeLocCrits = 0;
	int tIsLeg = ((hitloc == RLEG || hitloc == LLEG) || ((hitloc == RARM
														  || hitloc == LARM)
														 &&
														 (MechIsQuad
														  (wounded))));

	if(!(MechType(wounded) == CLASS_MECH || MechType(wounded) == CLASS_MW
		 || MechType(wounded) == CLASS_BSUIT)) {
		for(i = 0; i < CritsInLoc(wounded, hitloc); i++)
			if(GetPartType(wounded, hitloc, i) &&
			   !PartIsDestroyed(wounded, hitloc, i)) {
				if(IsDisable == 1)
					DisablePart(wounded, hitloc, i);
				else
					DestroyPart(wounded, hitloc, i);
			}
		return;
	}
	oldjs = MechJumpSpeed(wounded);
	for(i = 0; i < CritsInLoc(wounded, hitloc); i++)
		if(!PartIsDestroyed(wounded, hitloc, i)) {
			if(IsDisable == 1)
				DisablePart(wounded, hitloc, i);
			else if(PartIsDisabled(wounded, hitloc, i)) {
				DestroyPart(wounded, hitloc, i);
				continue;
			} else
				DestroyPart(wounded, hitloc, i);

			critType = GetPartType(wounded, hitloc, i);
			if(IsAmmo(critType)) {
				GetPartData(wounded, hitloc, i) = 0;
			}
			if(IsSpecial(critType)) {
				switch (Special2I(critType)) {
				case UPPER_ACTUATOR:
				case LOWER_ACTUATOR:
				case HAND_OR_FOOT_ACTUATOR:
					tNormalizeLocCrits = 1;
					break;
				case SHOULDER_OR_HIP:
					if(tIsLeg) {
						 if(!(MechCritStatus(wounded) & HIP_DAMAGED)) {
							 MechCritStatus(wounded) |= HIP_DAMAGED;
							 } else {
								 if(!MechIsQuad(wounded))
									 MechCritStatus(wounded) |= HIP_DESTROYED;
							 }
					tNormalizeAllCrits = 1;
					}
					else
						tNormalizeLocCrits = 1;
					break;
				case HEAT_SINK:
					if(MechSpecials(wounded) & DOUBLE_HEAT_TECH) {
						if((nhs++) % 3 == 2)
							MechRealNumsinks(wounded)++;
					}
					MechRealNumsinks(wounded)--;
					break;
				case JUMP_JET:
					MechJumpSpeed(wounded) -= MP1;
					if(MechJumpSpeed(wounded) < 0)
						MechJumpSpeed(wounded) = 0;
					if(attacker && MechJumpSpeed(wounded) == 0 &&
					   Jumping(wounded)) {
						mech_notify(wounded, MECHALL,
									"Losing your last Jump Jet you fall from the sky!!!!!");
						MechLOSBroadcast(wounded, "falls from the sky!");
						MechFalls(wounded, (int) (oldjs * MP_PER_KPH), 0);
						domino_space(wounded, 2);
					}
					break;
				case ENGINE:
					if(MechEngineHeat(wounded) < 10)
						MechEngineHeat(wounded) += 5;
					else if(MechEngineHeat(wounded) < 15) {
						MechEngineHeat(wounded) = 15;
						if(attacker) {
							mech_notify(wounded, MECHALL,
										"Your engine is destroyed!!");
							if(wounded != attacker)
								mech_notify(attacker, MECHALL,
											"You destroy the engine!!");
						}
						//check_stackpole(wounded, attacker);

						if(mudconf.btech_stackpole && 
								(MechBoomStart(wounded) + MAX_BOOM_TIME) >= muxevent_tick &&
								Roll() >= BOOM_BTH && (Started(wounded) || Starting(wounded))) {

							HexLOSBroadcast(getMap(wounded->mapindex), MechX(wounded), MechY(wounded),
									"%ch%crThe hit destroys the last safety systems, "
									"releasing the fusion reaction!%cn");


							reactor_explosion(wounded, attacker);	
						}


						DestroyMech(wounded, attacker, 1);
					}
					break;
				case TARGETING_COMPUTER:
					if(!MechCritStatus(wounded) & TC_DESTROYED) {
						if(attacker)
							mech_notify(wounded, MECHALL,
										"Your Targeting Computer is Destroyed");
						MechCritStatus(wounded) |= TC_DESTROYED;
					}
					break;
				}
			}
		}
	if(breach)
		if(MechType(wounded) == CLASS_VEH_GROUND ||
		   MechType(wounded) == CLASS_VEH_NAVAL)
			DestroyMech(wounded, attacker, 0);
	if(MechType(wounded) == CLASS_MECH || MechType(wounded) == CLASS_MW) {
		if(breach && hitloc == HEAD) {
			if(InVacuum(wounded))
				mech_notify(wounded, MECHALL, "You are exposed to vacuum!");
			else
				mech_notify(wounded, MECHALL,
							"Water floods into your cockpit!");

			KillMechContentsIfIC(wounded->mynum);
			DestroyMech(wounded, attacker, 0);
			return;
		}
		if(!MechIsQuad(wounded))
			if(hitloc == LARM || hitloc == RARM)
				return;
		if(hitloc == RLEG || hitloc == LLEG || hitloc == LARM ||
		   hitloc == RARM) {
			tDoAutoFall = 1;
			StopStand(wounded);
		}
/*		if(tNormalizeAllCrits) */
			NormalizeAllActuatorCrits(wounded);
/*		else if(tNormalizeLocCrits)
			NormalizeLocActuatorCrits(wounded, hitloc); */
		if(tIsLeg && !Fallen(wounded) && !Jumping(wounded) &&
		   !OODing(wounded) && attacker) {
			if(tDoAutoFall) {
				mech_notify(wounded, MECHALL,
							"You realize remaining standing is no longer an option and crash to the ground!");
				MechLOSBroadcast(wounded, "crashes to the ground!");
				MechFalls(wounded, 1, 0);
			} else if(!MadePilotSkillRoll(wounded, 0)) {
				mech_notify(wounded, MECHALL,
							"You lose your balance and fall down!");
				MechLOSBroadcast(wounded, "loses balance and falls down!");
				MechFalls(wounded, 1, 0);
			}
		}
	}
}

int BreachLoc(MECH * attacker, MECH * mech, int hitloc)
{
	char buf[SBUF_SIZE];

	if(!InSpecial(mech))
		return 0;
	if(!InVacuum(mech))
		return 0;
	if(SectIsDestroyed(mech, hitloc) || SectIsBreached(mech, hitloc))
		return 0;
	ArmorStringFromIndex(hitloc, buf, MechType(mech), MechMove(mech));
	mech_notify(mech, MECHALL, tprintf("Your %s has been breached!", buf));
	SetSectBreached(mech, hitloc);
	DestroyParts(attacker, mech, hitloc, 1, 1);
	return 1;
}

int PossiblyBreach(MECH * attacker, MECH * mech, int hitloc)
{
	if(!InSpecial(mech))
		return 0;
	if(Roll() < 10)
		return 0;
	return BreachLoc(attacker, mech, hitloc);
}
