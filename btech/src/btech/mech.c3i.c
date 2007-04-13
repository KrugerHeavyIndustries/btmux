
/*
 * $Id: mech.c3i.c,v 1.1.1.1 2005/01/11 21:18:12 kstevens Exp $
 *
 * Author: Cord Awtry <kipsta@mediaone.net>
 * Author: Cord Awtry <kipsta@mediaone.net>
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Based on work that was:
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2000 Thomas Wouters
 */

#include "mech.h"
#include "mech.events.h"
#include "p.mech.c3i.h"
#include "p.mech.c3.misc.h"
#include "p.mech.utils.h"
#include "p.mech.los.h"
#include "p.mech.contacts.h"

#define C3_POS_IN_NETWORK -1
#define C3_POS_NO_ROOM		-2

int getFreeC3iNetworkPos(MECH * mech, MECH * mechToAdd)
{
	int i;
	dbref otherRef;

	validateC3iNetwork(mech);

	for(i = 0; i < C3I_NETWORK_SIZE; i++) {
		otherRef = MechC3iNetworkElem(mech, i);

		if(otherRef > 0) {
			if(otherRef == mechToAdd->mynum)
				return C3_POS_IN_NETWORK;
		} else
			return i;
	}

	return C3_POS_NO_ROOM;
}

void replicateC3iNetwork(MECH * mechSrc, MECH * mechDest)
{
	int i;
	dbref otherRef;

	debugC3(tprintf("REPLICATE: %d's C3i network to %d", mechSrc->mynum,
					mechDest->mynum));

	clearC3iNetwork(mechDest, 0);

	MechC3iNetworkElem(mechDest, 0) = mechSrc->mynum;
	MechC3iNetworkSize(mechDest) = 1;

	for(i = 0; i < C3I_NETWORK_SIZE; i++) {
		otherRef = MechC3iNetworkElem(mechSrc, i);

		if(otherRef != mechDest->mynum) {
			MechC3iNetworkElem(mechDest, MechC3iNetworkSize(mechDest)) =
				otherRef;
			MechC3iNetworkSize(mechDest) += 1;
		}
	}

	validateC3iNetwork(mechDest);
}

void addMechToC3iNetwork(MECH * mech, MECH * mechToAdd)
{
	MECH *otherMech;
	MECH *otherNotifyMech;
	dbref otherRef;
	int i;
	int wPos = -1;

	debugC3(tprintf("ADD: %d to the C3i network of %d", mechToAdd->mynum,
					mech->mynum));

	/* Find a position to add the new mech into my network */
	wPos = getFreeC3iNetworkPos(mech, mechToAdd);

	/* If we have a number that's less than 0, then we have an invalid position. Either we're already in the network or there's not enough room */
	if(wPos < 0)
		return;

	/* Well, we have a valid position, so let's put this mech in the network */
	MechC3iNetworkElem(mech, wPos) = mechToAdd->mynum;
	MechC3iNetworkSize(mech) += 1;

	mech_notify(mech, MECHALL,
				tprintf("%s connects to your C3i network.",
						GetMechToMechID(mech, mechToAdd)));

	/* Now let's replicate the new network across the system so that everyone has the same network settings */
	for(i = 0; i < C3I_NETWORK_SIZE; i++) {
		otherRef = MechC3iNetworkElem(mech, i);

		otherMech = getOtherMechInNetwork(mech, i, 0, 0, 0, 0);

		if(!otherMech)
			continue;

		if(!Good_obj(otherMech->mynum))
			continue;

		if(otherRef != mechToAdd->mynum) {
			otherNotifyMech = getOtherMechInNetwork(mech, i, 1, 1, 1, 0);

			if(otherNotifyMech)
				mech_notify(otherNotifyMech, MECHALL,
							tprintf("%s connects to your C3i network.",
									GetMechToMechID(otherNotifyMech,
													mechToAdd)));
		}

		replicateC3iNetwork(mech, otherMech);
	}

	/* Last, but not least, one final validation of the network */
	validateC3iNetwork(mech);
}

void clearMechFromC3iNetwork(dbref refToClear, MECH * mech)
{
	int i;

	debugC3(tprintf("CLEAR: %d from the C3i network of %d", refToClear,
					mech->mynum));

	if(!MechC3iNetworkSize(mech))
		return;

	for(i = 0; i < C3I_NETWORK_SIZE; i++) {
		if(MechC3iNetworkElem(mech, i) == refToClear)
			MechC3iNetworkElem(mech, i) = -1;
	}

	validateC3iNetwork(mech);
}

void clearC3iNetwork(MECH * mech, int tClearFromOthers)
{
	MECH *otherMech;
	int i;

	debugC3(tprintf("CLEAR: %d's C3i network", mech->mynum));

	for(i = 0; i < C3I_NETWORK_SIZE; i++) {
		otherMech = getOtherMechInNetwork(mech, i, 0, 0, 0, 0);

		MechC3iNetworkElem(mech, i) = -1;

		if(tClearFromOthers) {
			if(!otherMech)
				continue;

			if(!Good_obj(otherMech->mynum))
				continue;

			clearMechFromC3iNetwork(mech->mynum, otherMech);
		}
	}

	MechC3iNetworkSize(mech) = 0;
}

void validateC3iNetwork(MECH * mech)
{
	MECH *otherMech;
	dbref myTempNetwork[C3I_NETWORK_SIZE];
	int i;
	int networkSize = 0;

	debugC3(tprintf("VALIDATE: %d's C3i network", mech->mynum));

	if(!HasC3i(mech) || Destroyed(mech) || C3iDestroyed(mech)) {
		clearC3iNetwork(mech, 1);

		return;
	}

	if(MechC3iNetworkSize(mech) < 0) {
		clearC3iNetwork(mech, 1);

		return;
	}

	for(i = 0; i < C3I_NETWORK_SIZE; i++) {
		otherMech = getOtherMechInNetwork(mech, i, 0, 0, 0, 0);

		if(!otherMech)
			continue;

		if(!Good_obj(otherMech->mynum))
			continue;

		debugC3(tprintf("VALIDATE INFO: %d is now in %d's C3i network",
						otherMech->mynum, mech->mynum));

		myTempNetwork[networkSize++] = otherMech->mynum;
	}

	clearC3iNetwork(mech, 0);

	for(i = 0; i < networkSize; i++)
		MechC3iNetworkElem(mech, i) = myTempNetwork[i];

	MechC3iNetworkSize(mech) = networkSize;

	debugC3(tprintf("VALIDATE INFO: %d's C3i network is %d elements",
					mech->mynum, MechC3iNetworkSize(mech)));
}

void mech_c3i_join_leave(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data, *target;
	MAP *objMap;
	char *args[2];
	dbref refTarget;
	int LOS = 1;
	float range = 0.0;

	cch(MECH_USUALO);

	DOCHECK(mech_parseattributes(buffer, args, 2) != 1,
			"Invalid number of arguments to function!");

	DOCHECK(!HasC3i(mech), "This unit is not equipped with C3i!");
	DOCHECK(C3iDestroyed(mech), "Your C3i system is destroyed!");
	DOCHECK(AnyECMDisturbed(mech),
			"Your C3i system is not currently operational!");

	validateC3iNetwork(mech);

	/* Clear our C3i Network */
	if(!strcmp(args[0], "-")) {
		if(MechC3iNetworkSize(mech) <= 0) {
			mech_notify(mech, MECHALL,
						"You are not connected to a C3i network!");

			return;
		}

		clearC3iNetwork(mech, 1);

		mech_notify(mech, MECHALL, "You disconnect from the C3i network.");

		return;
	}

	/* Well, if we're here then we wanna connect to a network */
	/* Let's check to see if we're already in one... can't be in two at the same time */
	DOCHECK(MechC3iNetworkSize(mech) > 0,
			"You are already in a C3i network!");

	objMap = getMap(mech->mapindex);

	/* Find who we're trying to connect to */
	refTarget = FindTargetDBREFFromMapNumber(mech, args[0]);
	target = getMech(refTarget);

	if(target) {
		LOS =
			InLineOfSight(mech, target, MechX(target), MechY(target), range);
	} else
		refTarget = 0;

	DOCHECK((refTarget < 1) ||
			!LOS, "That is not a valid targetID. Try again.");
	DOCHECK(MechTeam(mech) != MechTeam(target),
			"You can't use the C3i network of unfriendly units!");
	DOCHECK(mech == target, "You can't connect to yourself!");
	DOCHECK(Destroyed(target), "That unit is destroyed!");
	DOCHECK(!Started(target), "That unit is not started!");
	DOCHECK(!HasC3i(target),
			"That unit does not appear to be equipped with C3i!");

	/* validate the network of our target */
	validateC3iNetwork(target);
	DOCHECK(MechC3iNetworkSize(target) >= C3I_NETWORK_SIZE,
			"That unit's C3i network is operating at maximum capacity!");

	/* Connect us up */
	mech_notify(mech, MECHALL, tprintf("You connect to %s's C3i network.",
									   GetMechToMechID(mech, target)));

	addMechToC3iNetwork(target, mech);
}

void mech_c3i_message(dbref player, MECH * mech, char *buffer)
{
	cch(MECH_USUALO);

	DOCHECK(!HasC3i(mech), "This unit is not equipped with C3i!");
	DOCHECK(C3iDestroyed(mech), "Your C3i system is destroyed!");
	DOCHECK(AnyECMDisturbed(mech),
			"Your C3i system is not currently operational!");

	validateC3iNetwork(mech);

	DOCHECK(MechC3iNetworkSize(mech) <= 0,
			"There are no other units in your C3i network!");

	skipws(buffer);
	DOCHECK(!*buffer, "What do you want to send on the C3i Network?");

	sendNetworkMessage(player, mech, buffer, 0);
}

void mech_c3i_targets(dbref player, MECH * mech, char *buffer)
{
	cch(MECH_USUALO);

	DOCHECK(!HasC3i(mech), "This unit is not equipped with C3i!");
	DOCHECK(C3iDestroyed(mech), "Your C3i system is destroyed!");
	DOCHECK(AnyECMDisturbed(mech),
			"Your C3i system is not currently operational!");

	validateC3iNetwork(mech);

	DOCHECK(MechC3iNetworkSize(mech) <= 0,
			"There are no other units in your C3i network!");

	showNetworkTargets(player, mech, 0);
}

void mech_c3i_network(dbref player, MECH * mech, char *buffer)
{
	cch(MECH_USUALO);

	DOCHECK(!HasC3i(mech), "This unit is not equipped with C3i!");
	DOCHECK(C3iDestroyed(mech), "Your C3i system is destroyed!");
	DOCHECK(AnyECMDisturbed(mech),
			"Your C3i system is not currently operational!");

	validateC3iNetwork(mech);

	DOCHECK(MechC3iNetworkSize(mech) <= 0,
			"There are no other units in your C3i network!");

	showNetworkData(player, mech, 0);
}
