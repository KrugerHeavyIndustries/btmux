
/*
 * $Id: mech.c3.c,v 1.1.1.1 2005/01/11 21:18:11 kstevens Exp $
 *
 * Author: Cord Awtry <kipsta@mediaone.net>
 *
 *  Copyright (c) 2001 Cord Awtry
 *       All rights reserved
 */

#include "mech.h"
#include "mech.events.h"
#include "p.mech.c3.h"
#include "p.mech.c3.misc.h"
#include "p.mech.utils.h"
#include "p.mech.los.h"
#include "p.mech.contacts.h"

#define C3_POS_IN_NETWORK -1
#define C3_POS_NO_ROOM		-2

#define C3_MASTER_MECH_SIZE 5
#define C3_MASTER_OTHER_SIZE 1

int getC3MasterSize(MECH * mech)
{
	if(MechType(mech) == CLASS_MECH)
		return C3_MASTER_MECH_SIZE;
	else
		return C3_MASTER_OTHER_SIZE;
}

int isPartOfWorkingC3Master(MECH * mech, int section, int slot)
{
	int x = 0;
	int y, t;
	int wcWorkingSlots = 0;
	int wStartCheck = 0;
	int tDoBump;

	wStartCheck = MAX(0, slot - (getC3MasterSize(mech) - 1));

	while (x < CritsInLoc(mech, section)) {
		tDoBump = 0;

		if((t = GetPartType(mech, section, x))) {
			if(Special2I(t) == C3_MASTER) {
				if(x < wStartCheck) {
					tDoBump = 1;
				} else {
					/* We're within range of our slot, if not already on it */
					for(y = x; y < (x + getC3MasterSize(mech)); y++) {
						if(y != slot) {
							if(!PartIsNonfunctional(mech, section, y))
								wcWorkingSlots++;
						}
					}
				}
			}
		}

		if(tDoBump)
			x += getC3MasterSize(mech);
		else
			x++;
	}

	return (wcWorkingSlots == (getC3MasterSize(mech) - 1));
}

int countWorkingC3MastersOnMech(MECH * mech)
{
	int x, y, t;
	int wcSlots;
	int wcWorkingSlots;
	int wcMasters = 0;

	debugC3(tprintf("Counting working C3 masters for %d", mech->mynum));

	for(x = 0; x < NUM_SECTIONS; x++) {
		wcSlots = 0;
		wcWorkingSlots = 0;

		for(y = 0; y < CritsInLoc(mech, x); y++) {
			if((t = GetPartType(mech, x, y))) {
				if(Special2I(t) == C3_MASTER) {
					debugC3(tprintf
							("...found a C3Master slot at section %d, slot %d on %d.",
							 x, y, mech->mynum));

					wcSlots++;

					if(!PartIsNonfunctional(mech, x, y)) {
						debugC3("......and the slot is functional.");
						wcWorkingSlots++;
					}
				}
			}

			if(wcSlots == getC3MasterSize(mech)) {
				debugC3(tprintf
						("...found enough slots for a C3Master for %d.",
						 mech->mynum));
				wcSlots = 0;

				if(wcWorkingSlots == getC3MasterSize(mech)) {
					debugC3(tprintf
							("...there is even enough working slots to make the computer work on %d.",
							 mech->mynum));
					wcMasters++;
				}
			}
		}
	}

	debugC3(tprintf("Found %d working C3 masters on %d", wcMasters,
					mech->mynum));

	return wcMasters;
}

int countTotalC3MastersOnMech(MECH * mech)
{
	int x, y, t;
	int wcSlots;
	int wcMasters = 0;

	debugC3(tprintf("Counting total C3 masters for %d", mech->mynum));

	for(x = 0; x < NUM_SECTIONS; x++) {
		wcSlots = 0;

		for(y = 0; y < CritsInLoc(mech, x); y++) {
			if((t = GetPartType(mech, x, y))) {
				if(Special2I(t) == C3_MASTER) {
					debugC3(tprintf
							("...found a C3Master slot at section %d, slot %d on %d.",
							 x, y, mech->mynum));

					wcSlots++;
				}
			}

			if(wcSlots == getC3MasterSize(mech)) {
				debugC3(tprintf
						("...found enough slots for a C3Master for %d.",
						 mech->mynum));

				wcSlots = 0;
				wcMasters++;
			}
		}
	}

	debugC3(tprintf("Found %d total C3 masters on %d", wcMasters,
					mech->mynum));

	return wcMasters;
}

int countMaxC3Units(MECH * mech, dbref * myTempNetwork,
					int tempNetworkSize, MECH * targMech)
{
	dbref otherRef;
	MECH *otherMech;
	int i;
	int wcC3Masters = 0;
	int myMasters = 0;
	int maxC3Size;

	debugC3(tprintf("Counting max C3 units in %d's network", mech->mynum));

	if(targMech)
		debugC3(tprintf("...using %d as an additional mech",
						targMech->mynum));

	/* First we iterate over the list and find all the masters */
	for(i = 0; i < tempNetworkSize; i++) {
		otherRef = myTempNetwork[i];
		otherMech = getMech(otherRef);

		if(!otherMech)
			continue;

		wcC3Masters += MechWorkingC3Masters(otherMech);

		debugC3(tprintf("...for %d, we add %d masters", otherMech->mynum,
						MechWorkingC3Masters(otherMech)));
	}

	/* Let's find out the max number of mechs in this network. Make sure we add in any slaves we can control */
	maxC3Size = (wcC3Masters * 4) - wcC3Masters;

	debugC3(tprintf("...we now have a max size of %d", maxC3Size));

	myMasters = MechWorkingC3Masters(mech);

	if(myMasters > 0)
		maxC3Size += (myMasters * 4) - myMasters;

	debugC3(tprintf
			("...and after adding in my masters, we now have a max size of %d",
			 maxC3Size));

	/* Let's see if a 2nd mech has been supplied to us */
	if(targMech) {
		myMasters = MechWorkingC3Masters(targMech);

		if(myMasters > 0)
			maxC3Size += (myMasters * 4) - myMasters;
	}

	maxC3Size = MIN(maxC3Size, 11);

	debugC3(tprintf("...final max size of %d", maxC3Size));

	return maxC3Size;
}

int trimC3Network(MECH * mech, dbref * myTempNetwork, int tempNetworkSize)
{
	dbref otherRef;
	MECH *otherMech;
	int i;
	int newNetworkSize;
	int maxC3Size = 0;			/* This is calc'd based on the number of masters */
	dbref newNetwork[C3_NETWORK_SIZE];

	debugC3(tprintf("C3 TRIM: Trimming %d's C3 network", mech->mynum));

	/* Initialize our data */
	newNetworkSize = tempNetworkSize;

	for(i = 0; i < C3_NETWORK_SIZE; i++)
		newNetwork[i] = -1;

	/* Get our count of max units */
	maxC3Size = countMaxC3Units(mech, myTempNetwork, tempNetworkSize, NULL);

	debugC3(tprintf("C3 TRIM: Max C3 size: %d", maxC3Size));
	debugC3(tprintf("C3 TRIM: Current C3 size: %d", tempNetworkSize));

	/* Now we see if our network is oversized */
	if(maxC3Size < tempNetworkSize) {
		newNetworkSize = 0;

		/* First put our masters in */
		for(i = 0; i < tempNetworkSize; i++) {
			otherRef = myTempNetwork[i];
			otherMech = getMech(otherRef);

			if(!otherMech)
				continue;

			if(MechWorkingC3Masters(otherMech) > 0)
				newNetwork[newNetworkSize++] = otherRef;
		}

		/* Next we put in slaves up to the max amount */
		if(newNetworkSize < maxC3Size) {
			for(i = 0; i < tempNetworkSize; i++) {
				otherRef = myTempNetwork[i];
				otherMech = getMech(otherRef);

				if(!otherMech)
					continue;

				if(MechWorkingC3Masters(otherMech) == 0)
					newNetwork[newNetworkSize++] = otherRef;

				if(newNetworkSize >= maxC3Size)
					break;
			}
		}

		/* Now, refill our other temp network */
		for(i = 0; i < newNetworkSize; i++)
			myTempNetwork[i] = newNetwork[i];
	}

	return newNetworkSize;
}

int getFreeC3NetworkPos(MECH * mech, MECH * mechToAdd)
{
	int i;
	dbref otherRef;

	validateC3Network(mech);

	for(i = 0; i < C3_NETWORK_SIZE; i++) {
		otherRef = MechC3NetworkElem(mech, i);

		if(otherRef > 0) {
			if(otherRef == mechToAdd->mynum)
				return C3_POS_IN_NETWORK;
		} else
			return i;
	}

	return C3_POS_NO_ROOM;
}

void replicateC3Network(MECH * mechSrc, MECH * mechDest)
{
	int i;
	dbref otherRef;

	debugC3(tprintf("C3 REPLICATE: %d's C3 network to %d",
					mechSrc->mynum, mechDest->mynum));

	clearC3Network(mechDest, 0);

	MechC3NetworkElem(mechDest, 0) = mechSrc->mynum;
	MechC3NetworkSize(mechDest) = 1;

	for(i = 0; i < C3_NETWORK_SIZE; i++) {
		otherRef = MechC3NetworkElem(mechSrc, i);

		if(otherRef != mechDest->mynum) {
			MechC3NetworkElem(mechDest, MechC3NetworkSize(mechDest)) =
				otherRef;
			MechC3NetworkSize(mechDest) += 1;
		}
	}

	validateC3Network(mechDest);
}

void addMechToC3Network(MECH * mech, MECH * mechToAdd)
{
	MECH *otherMech;
	MECH *otherNotifyMech;
	dbref otherRef;
	int i;
	int wPos = -1;

	debugC3(tprintf("C3 ADD: %d to the C3 network of %d",
					mechToAdd->mynum, mech->mynum));

	/* Find a position to add the new mech into my network */
	wPos = getFreeC3NetworkPos(mech, mechToAdd);

	/* If we have a number that's less than 0, then we have an invalid position. Either we're already in the network or there's not enough room */
	if(wPos < 0)
		return;

	/* Well, we have a valid position, so let's put this mech in the network */
	debugC3(tprintf("C3 ADD: Position to add to %d's network is %d",
					mech->mynum, wPos));

	MechC3NetworkElem(mech, wPos) = mechToAdd->mynum;
	MechC3NetworkSize(mech) += 1;

	mech_notify(mech, MECHALL,
				tprintf("%s connects to your C3 network.",
						GetMechToMechID(mech, mechToAdd)));

	/* Now let's replicate the new network across the system so that everyone has the same network settings */
	for(i = 0; i < C3_NETWORK_SIZE; i++) {
		otherRef = MechC3NetworkElem(mech, i);

		otherMech = getOtherMechInNetwork(mech, i, 0, 0, 0, 1);

		if(!otherMech)
			continue;

		if(!Good_obj(otherMech->mynum))
			continue;

		if(otherRef != mechToAdd->mynum) {
			otherNotifyMech = getOtherMechInNetwork(mech, i, 1, 1, 1, 1);

			if(otherNotifyMech)
				mech_notify(otherNotifyMech, MECHALL,
							tprintf("%s connects to your C3 network.",
									GetMechToMechID(otherNotifyMech,
													mechToAdd)));
		}

		replicateC3Network(mech, otherMech);
	}

	/* Last, but not least, one final validation of the network */
	validateC3Network(mech);
}

void clearMechFromC3Network(dbref refToClear, MECH * mech)
{
	int i;

	debugC3(tprintf("C3 CLEAR: %d from the C3 network of %d", refToClear,
					mech->mynum));

	if(!MechC3NetworkSize(mech))
		return;

	for(i = 0; i < C3_NETWORK_SIZE; i++) {
		if(MechC3NetworkElem(mech, i) == refToClear)
			MechC3NetworkElem(mech, i) = -1;
	}

	validateC3Network(mech);
}

void clearC3Network(MECH * mech, int tClearFromOthers)
{
	MECH *otherMech;
	int i;

	debugC3(tprintf("C3 CLEAR: %d's C3 network", mech->mynum));

	for(i = 0; i < C3_NETWORK_SIZE; i++) {
		otherMech = getOtherMechInNetwork(mech, i, 0, 0, 0, 1);

		MechC3NetworkElem(mech, i) = -1;

		if(tClearFromOthers) {
			if(!otherMech)
				continue;

			if(!Good_obj(otherMech->mynum))
				continue;

			clearMechFromC3Network(mech->mynum, otherMech);
		}
	}

	MechC3NetworkSize(mech) = 0;
}

void validateC3Network(MECH * mech)
{
	MECH *otherMech;
	dbref myTempNetwork[C3_NETWORK_SIZE];
	int i;
	int networkSize = 0;

	debugC3(tprintf("C3 VALIDATE: %d's C3 network", mech->mynum));

	if(!HasC3(mech) || Destroyed(mech) || C3Destroyed(mech)) {
		clearC3Network(mech, 1);

		return;
	}

	if(MechC3NetworkSize(mech) < 0) {
		clearC3Network(mech, 1);

		return;
	}

	for(i = 0; i < C3_NETWORK_SIZE; i++) {
		otherMech = getOtherMechInNetwork(mech, i, 0, 0, 0, 1);

		if(!otherMech)
			continue;

		if(!Good_obj(otherMech->mynum))
			continue;

		debugC3(tprintf("C3 VALIDATE INFO: %d is now in %d's C3 network",
						otherMech->mynum, mech->mynum));

		myTempNetwork[networkSize] = otherMech->mynum;
		networkSize++;
	}

	clearC3Network(mech, 0);

	for(i = 0; i < networkSize; i++)
		MechC3NetworkElem(mech, i) = myTempNetwork[i];

	MechC3NetworkSize(mech) = networkSize;

	debugC3(tprintf
			("C3 VALIDATE INFO: (PreTrim) %d's C3 network is %d elements",
			 mech->mynum, MechC3NetworkSize(mech)));

	networkSize = trimC3Network(mech, myTempNetwork, networkSize);

	debugC3(tprintf
			("C3 VALIDATE INFO: (PostTrim) %d's C3 network has been trimmed to %d elements",
			 mech->mynum, networkSize));

	if(networkSize != MechC3NetworkSize(mech)) {
		clearC3Network(mech, 0);

		for(i = 0; i < networkSize; i++)
			MechC3NetworkElem(mech, i) = myTempNetwork[i];

		MechC3NetworkSize(mech) = networkSize;
	}

}

void mech_c3_join_leave(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data, *target;
	MAP *objMap;
	char *args[2];
	dbref refTarget;
	int LOS = 1;
	float range = 0.0;
	int maxC3Size = 0;

	cch(MECH_USUALO);

	DOCHECK(mech_parseattributes(buffer, args, 2) != 1,
			"Invalid number of arguments to function!");

	DOCHECK(!HasC3(mech), "This unit is not equipped with C3!");
	DOCHECK(C3Destroyed(mech), "Your C3 system is destroyed!");
	DOCHECK(AnyECMDisturbed(mech),
			"Your C3 system is not currently operational!");

	validateC3Network(mech);

	/* Clear our C3 Network */
	if(!strcmp(args[0], "-")) {
		if(MechC3NetworkSize(mech) <= 0) {
			mech_notify(mech, MECHALL,
						"You are not connected to a C3 network!");

			return;
		}

		clearC3Network(mech, 1);

		mech_notify(mech, MECHALL, "You disconnect from the C3 network.");

		return;
	}

	/* Well, if we're here then we wanna connect to a network */
	/* Let's check to see if we're already in one... can't be in two at the same time */
	DOCHECK(MechC3NetworkSize(mech) > 0, "You are already in a C3 network!");

	objMap = getMap(mech->mapindex);

	/* Find who we're trying to connect to */
	refTarget = FindTargetDBREFFromMapNumber(mech, args[0]);
	target = getMech(refTarget);

	if(target)
		LOS =
			InLineOfSight(mech, target, MechX(target), MechY(target), range);
	else
		refTarget = 0;

	DOCHECK((refTarget < 1) ||
			!LOS, "That is not a valid targetID. Try again.");
	DOCHECK(MechTeam(mech) != MechTeam(target),
			"You can't use the C3 network of unfriendly units!");
	DOCHECK(mech->mynum == target->mynum, "You can't connect to yourself!");
	DOCHECK(Destroyed(target), "That unit is destroyed!");
	DOCHECK(!Started(target), "That unit is not started!");
	DOCHECK(!HasC3(target),
			"That unit does not appear to be equipped with C3!");

	/* validate the network of our target */
	validateC3Network(target);

	/* Let's see how much can actually fit in this network, based on the number of masters and slaves */
	maxC3Size =
		countMaxC3Units(mech, MechC3Network(target),
						MechC3NetworkSize(target), target);

	DOCHECK(maxC3Size < (MechC3NetworkSize(target) + 1),
			"That unit's C3 network is operating at maximum capacity!");

	/* Connect us up */
	mech_notify(mech, MECHALL, tprintf("You connect to %s's C3 network.",
									   GetMechToMechID(mech, target)));

	addMechToC3Network(target, mech);
}

void mech_c3_message(dbref player, MECH * mech, char *buffer)
{
	cch(MECH_USUALO);

	DOCHECK(!HasC3(mech), "This unit is not equipped with C3!");
	DOCHECK(C3Destroyed(mech), "Your C3 system is destroyed!");
	DOCHECK(AnyECMDisturbed(mech),
			"Your C3 system is not currently operational!");

	validateC3Network(mech);

	DOCHECK(MechC3NetworkSize(mech) <= 0,
			"There are no other units in your C3 network!");

	skipws(buffer);
	DOCHECK(!*buffer, "What do you want to send on the C3 Network?");

	sendNetworkMessage(player, mech, buffer, 1);
}

void mech_c3_targets(dbref player, MECH * mech, char *buffer)
{
	cch(MECH_USUALO);

	DOCHECK(!HasC3(mech), "This unit is not equipped with C3!");
	DOCHECK(C3Destroyed(mech), "Your C3 system is destroyed!");
	DOCHECK(AnyECMDisturbed(mech),
			"Your C3 system is not currently operational!");

	validateC3Network(mech);

	DOCHECK(MechC3NetworkSize(mech) <= 0,
			"There are no other units in your C3 network!");

	showNetworkTargets(player, mech, 1);
}

void mech_c3_network(dbref player, MECH * mech, char *buffer)
{
	cch(MECH_USUALO);

	DOCHECK(!HasC3(mech), "This unit is not equipped with C3!");
	DOCHECK(C3Destroyed(mech), "Your C3 system is destroyed!");
	DOCHECK(AnyECMDisturbed(mech),
			"Your C3 system is not currently operational!");

	validateC3Network(mech);

	DOCHECK(MechC3NetworkSize(mech) <= 0,
			"There are no other units in your C3 network!");

	showNetworkData(player, mech, 1);
}
