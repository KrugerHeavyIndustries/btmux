/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1999-2000 Marco Peter Hoogeveen
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 *
 */

#include <math.h>

#include "mech.h"
#include "mech.events.h"
#include "p.mech.utils.h"
#include "p.mech.combat.h"
#include "p.mech.damage.h"
#include "p.mech.los.h"
#include "p.mech.move.h"
#include "p.crit.h"
#include "p.mech.bth.h"
#include "p.mech.update.h"

/*! \todo {The Bsuit code needs an overhaul} */

/* 2 battlesuit-specific attacks:
   - attackleg
   - swarm
 */

#define MyHiddenTurns(mech) ((MechType(mech) == CLASS_MW ? 1 : MechType(mech) == CLASS_BSUIT ? 3 : MechType(mech) == CLASS_VTOL ? 4 : 5) * ((MechSpecials2(mech) & CAMO_TECH) ? 1 : 2))

/* Stops everyone who's swarming this poor guy */

#define RECYCLE_SWARM (PHYSICAL_RECYCLE_TIME / 3)
#define RECYCLE_ATTACKLEG (PHYSICAL_RECYCLE_TIME / 2)
#define RECYCLE_INT_STOPSWARM (PHYSICAL_RECYCLE_TIME / 3)
#define RECYCLE_UNINT_STOPSWARM (PHYSICAL_RECYCLE_TIME / 2)
#define RECYCLE_FALL_STOPSWARM ((PHYSICAL_RECYCLE_TIME / 4) * 3)

char *GetBSuitName(MECH * mech)
{
	return (MechSpecials(mech) & CLAN_TECH) ? "Point" : "Squad";
}

char *GetLCaseBSuitName(MECH * mech)
{
	return (MechSpecials(mech) & CLAN_TECH) ? "point" : "squad";
}

void StartBSuitRecycle(MECH * mech, int time)
{
	int i;

	for(i = 0; i < NUM_BSUIT_MEMBERS; i++)
		if(GetSectInt(mech, i))
			SetRecycleLimb(mech, i, time);
}

void StopSwarming(MECH * mech, int intentional)
{
	MECH *target = getMech(MechSwarmTarget(mech));

	if(!target || MechSwarmTarget(mech) <= 0)
		return;

	MechSwarmTarget(mech) = -1;
	MechSwarmer(target) = -1;

	MechStatus2(mech) &= ~UNIT_MOUNTING;
	MechStatus2(target) &= ~UNIT_MOUNTED;

	if(intentional > 0) {
		mech_notify(mech, MECHALL,
					"You let your hold loosen and you drop from the 'mech!");
		mech_printf(target, MECHALL, "%s lets go of you!",
					GetMechToMechID(target, mech));
		MechLOSBroadcasti(mech, target, "lets go of %s!");

		StartBSuitRecycle(mech, RECYCLE_INT_STOPSWARM);
	} else {
		if(MadePilotSkillRoll(mech, 4)) {
			mech_notify(mech, MECHALL,
						"The hold loosens and you drop from the 'mech!");
			MechLOSBroadcasti(mech, target, "jumps off of %s!");
			mech_printf(target, MECHALL, "%s jumps off!",
						GetMechToMechID(target, mech));

			StartBSuitRecycle(mech, RECYCLE_UNINT_STOPSWARM);
		} else {
			mech_notify(mech, MECHALL,
						"You're suprised by the sudden action and find yourself rapidly approaching the ground!");
			MechLOSBroadcasti(mech, target, "falls off %s!");
			mech_printf(target, MECHALL, "%s falls off!",
						GetMechToMechID(target, mech));

			DamageMech(mech, mech, 1, -1, Number(0, NUM_BSUIT_MEMBERS - 1),
					   0, 0, 11, 0, -1, 0, -1, 0, 1);

			StartBSuitRecycle(mech, RECYCLE_FALL_STOPSWARM);
		}
	}

	MechSpeed(mech) = 0;
	MaybeMove(mech);
	DropSetElevation(mech, 0);
	MechFloods(mech);
}

int IsMechSwarmed(MECH * mech)
{
	MAP *map = FindObjectsData(mech->mapindex);
	int count = 0, i, j;
	MECH *t;

	if(!map)
		return 0;

	for(i = 0; i < map->first_free; i++)
		if((j = map->mechsOnMap[i]) > 0 && i != mech->mapnumber) {
			if(!(t = FindObjectsData(j)))
				continue;

			if(MechSwarmTarget(t) != mech->mynum)
				continue;

			if(MechTeam(mech) == MechTeam(t))
				continue;

			count++;
			break;
		}
	return count > 0;
}

int IsMechMounted(MECH * mech)
{
	MAP *map = FindObjectsData(mech->mapindex);
	int count = 0, i, j;
	MECH *t;

	if(!map)
		return 0;

	for(i = 0; i < map->first_free; i++)
		if((j = map->mechsOnMap[i]) > 0 && i != mech->mapnumber) {
			if(!(t = FindObjectsData(j)))
				continue;

			if(MechSwarmTarget(t) != mech->mynum)
				continue;

			if(MechTeam(mech) != MechTeam(t))
				continue;

			count++;
			break;
		}
	return count > 0;
}

int CountSwarmers(MECH * mech)
{
	MAP *map = FindObjectsData(mech->mapindex);
	int count = 0, i, j;
	MECH *t;

	if(!map)
		return 0;
	for(i = 0; i < map->first_free; i++)
		if((j = map->mechsOnMap[i]) > 0 && i != mech->mapnumber) {
			if(!(t = FindObjectsData(j)))
				continue;
			if(MechSwarmTarget(t) != mech->mynum)
				continue;
			count++;
		}
	return count;
}

MECH *findSwarmers(MECH * mech)
{
	MAP *map = FindObjectsData(mech->mapindex);
	int i, j;
	MECH *t;

	if(!map)
		return 0;

	for(i = 0; i < map->first_free; i++)
		if((j = map->mechsOnMap[i]) > 0 && i != mech->mapnumber) {
			if(!(t = FindObjectsData(j)))
				continue;

			if(MechSwarmTarget(t) == mech->mynum) {
				return t;
			}
		}

	return NULL;
}

void StopBSuitSwarmers(MAP * map, MECH * mech, int intentional)
{
	int i, j;
	MECH *t;

	if(!map || !mech)
		return;
	for(i = 0; i < map->first_free; i++)
		if((j = map->mechsOnMap[i]) > 0 && i != mech->mapnumber) {
			if(!(t = FindObjectsData(j)))
				continue;
			if(MechSwarmTarget(t) != mech->mynum)
				continue;
			StopSwarming(t, intentional);
		}
}

void BSuitMirrorSwarmedTarget(MAP * map, MECH * mech)
{
	int i, j;
	MECH *t;

	for(i = 0; i < map->first_free; i++)
		if((j = map->mechsOnMap[i]) > 0 && i != mech->mapnumber) {
			if(!(t = FindObjectsData(j)))
				continue;
			if(MechSwarmTarget(t) != mech->mynum)
				continue;
			MirrorPosition(mech, t, 1);
		}
}

int doBSuitCommonChecks(MECH * mech, dbref player)
{
	int i;

	DOCHECK1(Jumping(mech), "Unavailable when jumping - sorry.");
	DOCHECK1(MechSwarmTarget(mech) > 0,
			 "You are already busy with a special attack!");
#ifdef BT_MOVEMENT_MODES
	DOCHECK1(MoveModeLock(mech),
			 "Unavailable when performing movement modes - deal.");
#endif
	for(i = 0; i < NUM_BSUIT_MEMBERS; i++) {
		DOCHECK1(!SectIsDestroyed(mech, i) &&
				 MechSections(mech)[i].recycle,
				 tprintf("Suit %d is still recovering from attack.", i + 1));
		DOCHECK1(SectHasBusyWeap(mech,i),"You have weapons recycling!");
	}

	return 0;
}

int CountBSuitMembers(MECH * mech)
{
	int i, j = 0;

	for(i = 0; i < NUM_BSUIT_MEMBERS; i++)
		if(GetSectInt(mech, i))
			j++;
	return j;
}

int FindBSuitTarget(dbref player, MECH * mech, MECH ** target, char *buffer)
{
	int argc;
	char *args[3];
	float range;
	char targetID[2];
	int targetnum;
	MECH *t = NULL;

	DOCHECK1((argc =
			  mech_parseattributes(buffer, args, 3)) > 1,
			 "Invalid arguments!");
	switch (argc) {
	case 0:
		DOCHECK1(MechTarget(mech) <= 0,
				 "You do not have a default target set!");
		t = getMech(MechTarget(mech));
		if(!(t)) {
			mech_notify(mech, MECHALL, "Invalid default target!");
			MechTarget(mech) = -1;
			return 1;
		}
		break;
	case 1:
		targetID[0] = args[0][0];
		targetID[1] = args[0][1];
		targetnum = FindTargetDBREFFromMapNumber(mech, targetID);
		DOCHECK1(targetnum <= 0, "Target is not in line of sight!");
		t = getMech(targetnum);
		DOCHECK1(!(t), "Invalid default target!");
		break;
	default:
		notify(player, "Invalid target!");
		return 1;
	}
	range = FaMechRange(mech, t);
	DOCHECK1(!InLineOfSight_NB(mech, t, MechX(t), MechY(t), range),
			 "Target is not in line of sight!");
	DOCHECK1(range >= 1.0, "Target out of range!");
	DOCHECK1(Jumping(t), "That target's unreachable right now!");
	DOCHECK1(MechType(t) != CLASS_MECH, "That target is of invalid type.");
	DOCHECK1(Destroyed(t), "A dead 'mech? C'mon :P");
	*target = t;
	return 0;
}

int doJettisonChecks(MECH * mech)
{
	int i, j;

	if(!(MechInfantrySpecials(mech) & MUST_JETTISON_TECH))
		return 0;

	for(i = 0; i < NUM_BSUIT_MEMBERS; i++) {
		for(j = 0; j < NUM_CRITICALS; j++) {
			if((GetPartFireMode(mech, i, j) & WILL_JETTISON_MODE) &&
			   (!(GetPartFireMode(mech, i, j) & IS_JETTISONED_MODE))) {
				mech_printf(mech, MECHALL,
							"Suit %d can not perform this feat before it jettisons its backpack!",
							i + 1);

				return 1;
			}
		}
	}

	return 0;
}

void bsuit_swarm(dbref player, void *data, char *buffer)
{
	MECH *mech = data;
	MECH *target;
	int baseToHit = 4;
	int tIsMount = 0;

	cch(MECH_USUALO);
	skipws(buffer);

	/* Stop swarming... */
	if(!strcmp(buffer, "-")) {
		if(MechSwarmTarget(mech) > 0) {
			StopSwarming(mech, 1);
			return;
		}
	}

	if(doBSuitCommonChecks(mech, player))
		return;

	if(FindBSuitTarget(player, mech, &target, buffer))
		return;

	/* See if we're 'swarming' or 'mounting' */
	if(MechTeam(target) == MechTeam(mech)) {
		/* Make sure this type of bsuit has the ability to mount */
		if(!(MechInfantrySpecials(mech) & INF_MOUNT_TECH)) {
			mech_notify(mech, MECHALL,
						"These battlesuits are not capable of mounting mechs!");
			return;
		}

		tIsMount = 1;
	} else {
		if(doJettisonChecks(mech))
			return;

		/* Make sure this type of bsuit has the ability to swarm */
		if(!(MechInfantrySpecials(mech) & INF_SWARM_TECH)) {
			mech_notify(mech, MECHALL,
						"These battlesuits are not capable of performing swarm attacks!");
			return;
		}
	}

	/* Make sure there are no suits already on us */
	if(CountSwarmers(mech) > 0) {
		mech_notify(mech, MECHALL,
					"That target already have battlesuits crawling all over it! There's no room for you!");

		return;
	}

	/* get our BTH... we make it easier for mounting */
	switch (CountBSuitMembers(mech)) {
	case 1:
	case 2:
	case 3:
		baseToHit = 5;
		break;

	default:
		baseToHit = 2;
		break;
	}

	if(tIsMount)
		baseToHit -= 4;

	if(MechCritStatus(mech) & HIDDEN) {
		if(Immobile(target))
			baseToHit -= 4;

		if(Fallen(target))
			baseToHit -= 4;
	} else {
		baseToHit += TargetMovementMods(mech, target, 0.0);

		if(Fallen(target))
			baseToHit -= 2;
	}

	/* Well, we're here. Let's see if it works. */
	if(MadePilotSkillRoll(mech, baseToHit)) {
		mech_printf(target, MECHALL, "%s %s you!",
					GetMechToMechID(target, mech),
					(tIsMount ? "mounts" : "swarms"));
		mech_printf(mech, MECHALL, "You %s %s!",
					(tIsMount ? "mount" : "swarm"), GetMechToMechID(mech,
																	target));

		MechSwarmTarget(mech) = target->mynum;
		MechSwarmer(target) = mech->mynum;
		MechStatus2(mech) |= UNIT_MOUNTING;
		MechStatus2(target) |= UNIT_MOUNTED;

		if(tIsMount) {
			MechLOSBroadcasti(mech, target, "mounts %s!");
		} else {
			MechLOSBroadcasti(mech, target, "swarms %s!");
		}

		MechSpeed(mech) = 0.0;
		MechDesiredSpeed(mech) = 0.0;
		SetFacing(mech, 270);
		MechDesiredFacing(mech) = 270;
		MirrorPosition(target, mech, 1);
		StopLock(mech);
	} else {
		mech_printf(target, MECHALL, "%s attempts to %s you!",
					GetMechToMechID(target, mech),
					(tIsMount ? "mount" : "swarm"));
		mech_printf(mech, MECHALL,
					"Nice try, but you don't succeed in your attempt at %s %s!",
					(tIsMount ? "mounting" : "swarming"),
					GetMechToMechID(mech, target));
	}

                if(MechCritStatus(mech) & HIDDEN) {
			mech_notify(mech, MECHALL,"You move too much and break your cover!");
			MechLOSBroadcast(mech, "breaks from its cover.");
			MechCritStatus(mech) &= ~(HIDDEN);
			StopHiding(mech);
		}


	StartBSuitRecycle(mech, RECYCLE_SWARM);
}

void bsuit_attackleg(dbref player, void *data, char *buffer)
{
	MECH *mech = data;
	MECH *target;
	int baseToHit = 0;
	int wLegTemp = -1;
	int wLegID = -1;
	int wCritRoll = 0;
	char strAttackLoc[50];

	cch(MECH_USUALO);

	if(!(MechInfantrySpecials(mech) & INF_ANTILEG_TECH)) {
		mech_notify(mech, MECHALL,
					"These battlesuits are not capable of performing leg attacks!");
		return;
	}

	if(doBSuitCommonChecks(mech, player))
		return;

	if(doJettisonChecks(mech))
		return;

	if(FindBSuitTarget(player, mech, &target, buffer))
		return;

	DOCHECK(IsMechLegLess(mech), "That mech has no legs to grab!");
	DOCHECK((MechTeam(mech) == MechTeam(target)),
			"You can't attack the leg of a friendly mech!");

	switch (CountBSuitMembers(mech)) {
	case 1:
		baseToHit = 7;
		break;

	case 2:
		baseToHit = 5;
		break;

	case 3:
		baseToHit = 2;
		break;

	default:
		baseToHit = -1;
		break;
	}

	if(MechCritStatus(mech) & HIDDEN) {
		if(Immobile(target))
			baseToHit -= 4;

		if(Fallen(target))
			baseToHit -= 2;
	} else {
		baseToHit += TargetMovementMods(mech, target, 0.0);
	}

	if(MechIsQuad(target)) {
		do {
			switch (Number(0, 3)) {
			case 0:
				wLegTemp = RLEG;
				break;

			case 1:
				wLegTemp = LLEG;
				break;

			case 2:
				wLegTemp = RARM;
				break;

			case 3:
				wLegTemp = LARM;
				break;
			}

			if(GetSectInt(target, wLegTemp))
				wLegID = wLegTemp;

		} while (wLegID == -1);
	} else {
		wLegTemp = (Number(0, 1)) ? RLEG : LLEG;

		if(GetSectInt(target, wLegTemp) == 0) {
			wLegID = (wLegTemp == RLEG) ? LLEG : RLEG;
		} else {
			wLegID = wLegTemp;
		}
	}

	ArmorStringFromIndex(wLegID, strAttackLoc, MechType(target),
						 MechMove(target));

	mech_printf(mech, MECHALL,
				"You go for %s's %s, placing explosives in the joints!",
				GetMechToMechID(mech, target), strAttackLoc);

	if(MadePilotSkillRoll(mech, baseToHit)) {
		mech_printf(target, MECHALL,
					"%s swarms your %s putting small packets of explosives all over it!",
					GetMechToMechID(target, mech), strAttackLoc);

		MechLOSBroadcasti(mech, target, "attacks %s's legs!");

		/* find out if we do a crit or damage */
		wCritRoll = Roll();

		if(wCritRoll >= 8) {
			mech_printf(target, MECHALL,
						"The explosives manage to rip into the internals of your %s!",
						strAttackLoc);

			switch (wCritRoll) {
			case 8:
			case 9:
				HandleCritical(target, mech, 1, wLegID, 1);
				break;
			case 10:
			case 11:
				HandleCritical(target, mech, 1, wLegID, 2);
				break;
			case 12:
				switch (wLegID) {
				case RARM:
				case LARM:
				case RLEG:
				case LLEG:
					/* Limb blown off */
					mech_notify(target, MECHALL, "%ch%cyCRITICAL HIT!!%c");

					MechLOSBroadcast(target,
									 tprintf
									 ("'s %s is blown off in a shower of sparks and smoke!",
									  strAttackLoc));
					DestroySection(target, mech, 1, wLegID);
				default:
					HandleCritical(target, mech, 1, wLegID, 3);
				}
				break;
			default:
				break;
			}
		} else {
			mech_printf(target, MECHALL,
						"The explosives explode on the surface of your %s!",
						strAttackLoc);
			DamageMech(target, mech, 1, MechPilot(mech), wLegID, 0, 1, 4,
					   0, -1, 0, -1, 0, 1);
		}
	} else {
		mech_printf(target, MECHALL,
					"%s attempts to attacks your legs, but misses miserably.",
					GetMechToMechID(target, mech));

		mech_printf(mech, MECHALL,
					"You realize that this is harder than it looks and fail in your attempt at hitting %s's legs!",
					GetMechToMechID(mech, target));

		MechLOSBroadcasti(mech, target,
						  "attempts to climb %s's legs, but fails miserably!");
	}

	StartBSuitRecycle(mech, RECYCLE_ATTACKLEG);
}

static void mech_hide_event(MUXEVENT * e)
{
	MECH *mech = (MECH *) e->data;
	MECH *t;
	MAP *map = getMap(mech->mapindex);
	int fail = 0, i;
	int tic = (int) e->data2;

	if(!map)
		return;

	for(i = 0; i < map->first_free; i++) {
		if(map->mechsOnMap[i] <= 0)
			continue;
		if(!(t = getMech(map->mechsOnMap[i])))
			continue;
		if(MechCritStatus(t) & (CLAIRVOYANT | OBSERVATORIC | INVISIBLE))
			continue;
		if(MechTeam(t) == MechTeam(mech))
			continue;
		if(!Started(t))
			continue;
		if(Destroyed(t))
			continue;
		if(InLineOfSight(t, mech, MechX(mech), MechY(mech),
						 FaMechRange(t, mech)))
			fail = 1;
	}

	if(MechsElevation(mech))
		fail = 1;

	if(fail) {
		mech_notify(mech, MECHALL,
					"Your spidey sense tingles, telling you this isn't going to work......");
		return;
	} else if(tic < (MyHiddenTurns(mech) * HIDE_TICK)) {
		tic++;
		MECHEVENT(mech, EVENT_HIDE, mech_hide_event, 1, tic);
	} else if(!fail) {
		mech_notify(mech, MECHALL, "You are now hidden!");
		MechCritStatus(mech) |= HIDDEN;
	}
	return;
}

void bsuit_hide(dbref player, void *data, char *buffer)
{
	int i;
	MECH *mech = data;
	MECH *t;
	MAP *map = FindObjectsData(mech->mapindex);
	int terrain;

	cch(MECH_USUALO);
	DOCHECK(((HasCamo(mech))
			 || (Wizard(player))) ? 0 : MechType(mech) != CLASS_BSUIT
			&& MechType(mech) != CLASS_MW,
			"You aren't capable of such curious things.");

	if(!map) {
		mech_notify(mech, MECHALL, "You are not on a map!");
		return;
	}

	DOCHECK(Jumping(mech) || OODing(mech), "Hide where? Up here?");
	DOCHECK((fabs(MechSpeed(mech)) > MP1), "Come to a complete stop first.");
	DOCHECK(Hiding(mech), "You are looking for cover already!");
	DOCHECK(MechMove(mech) == MOVE_VTOL
			&& !Landed(mech), "You must be landed!");

	DOCHECK(MechSwarmTarget(mech) > 1, "Hide where? Not while on that!");

	terrain = GetRTerrain(map, MechX(mech), MechY(mech));

	if(IsForest(terrain)) {
		mech_notify(mech, MECHALL, "You start to hide amongst the trees...");
	} else if(IsMountains(terrain)) {
		mech_notify(mech, MECHALL,
					"You start to hide behind some rocky outcroppings...");
	} else if(IsRough(terrain)) {
		mech_notify(mech, MECHALL,
					"You find some boulders to try to hide behind...");
	} else if((IsBuilding(terrain)) && (MechType(mech) == CLASS_BSUIT)) {
		mech_notify(mech, MECHALL,
					"You break into a building and look for a spot to hide...");
	} else {
		mech_notify(mech, MECHALL, "You begin to hide in this terrain...");
		mech_notify(mech, MECHALL,
					"... then realize that just isn't going to work!");
		return;
	}

	MECHEVENT(mech, EVENT_HIDE, mech_hide_event, 1, 0);
}

void JettisonPacks(dbref player, void *data, char *buffer)
{
	MECH *mech = data;
	int wcJettisoned = 0;
	int wcSuits = 0;
	int i, j;

	cch(MECH_USUALO);
	DOCHECK((!(MechInfantrySpecials(mech) & CAN_JETTISON_TECH)),
			"You have no backpack that is capable of being jettisoned!");

	for(i = 0; i < NUM_BSUIT_MEMBERS; i++) {
		for(j = 0; j < NUM_CRITICALS; j++) {
			if((GetPartFireMode(mech, i, j) & WILL_JETTISON_MODE) &&
			   (!(GetPartFireMode(mech, i, j) & IS_JETTISONED_MODE))) {

				GetPartFireMode(mech, i, j) |= DESTROYED_MODE;
				GetPartFireMode(mech, i, j) |= IS_JETTISONED_MODE;
				GetPartFireMode(mech, i, j) &= ~(BROKEN_MODE | DISABLED_MODE);

				wcJettisoned++;
			}
		}
	}

	if(wcJettisoned > 0) {
		wcSuits = CountBSuitMembers(mech);

		if(wcSuits > 1) {
			mech_notify(mech, MECHALL,
						"The explosive bolts that hold the backpacks on blow, allowing them to drop to the ground.");
			MechLOSBroadcast(mech,
							 "'s backpacks blow off in a shower of small explosions!");
		} else {
			mech_notify(mech, MECHALL,
						"The explosive bolts that hold your backpack on blows, allowing it to drop to the ground.");
			MechLOSBroadcast(mech,
							 "'s backpack blows off in a puff of grey smoke!");
		}
	} else {
		mech_notify(mech, MECHALL,
					"You realize you have nothing to jettison. Maybe you already did it?");
	}
}
