/*
 * Author: Cord Awtry <kipsta@mediaone.net>
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Based on work that was:
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2000 Thomas Wouters
 */

#include "mech.h"
#include "p.mech.c3.misc.h"
#include "p.mech.c3.h"
#include "p.mech.c3i.h"
#include "p.mech.utils.h"
#include "p.mech.los.h"
#include "p.mech.contacts.h"

#define TARG_LOS_NONE 0
#define TARG_LOS_CLEAR 1
#define TARG_LOS_SOMETHING 2

#define DEBUG_C3 0

MECH *getMechInTempNetwork(int wIdx, dbref * myNetwork, int networkSize)
{
	MECH *tempMech;
	dbref refOtherMech;

	if((wIdx > networkSize) || (wIdx < 0))
		return NULL;

	refOtherMech = myNetwork[wIdx];

	if(refOtherMech > 0) {
		tempMech = getMech(refOtherMech);

		if(!tempMech)
			return NULL;

		if(Destroyed(tempMech))
			return NULL;

		return tempMech;
	}

	return NULL;
}

MECH *getOtherMechInNetwork(MECH * mech, int wIdx, int tCheckECM,
							int tCheckStarted, int tCheckUncon, int tIsC3)
{
	MECH *tempMech;
	dbref refOtherMech;
	int networkSize;

	networkSize =
		(tIsC3 ? MechC3NetworkSize(mech) : MechC3iNetworkSize(mech));

	if((wIdx >= networkSize) || (wIdx < 0))
		return NULL;

	refOtherMech =
		(tIsC3 ? MechC3NetworkElem(mech, wIdx) : MechC3iNetworkElem(mech,
																	wIdx));

	if(refOtherMech > 0) {
		tempMech = getMech(refOtherMech);

		if(!tempMech)
			return NULL;

		if(MechTeam(tempMech) != MechTeam(mech))
			return NULL;

		if(tempMech->mapindex != mech->mapindex)
			return NULL;

		if(Destroyed(tempMech))
			return NULL;

		if(tIsC3) {
			if(!HasC3(tempMech))	/* Sanity check */
				return NULL;

			if(C3Destroyed(tempMech))
				return NULL;
		} else {
			if(!HasC3i(tempMech))	/* Sanity check */
				return NULL;

			if(C3iDestroyed(tempMech))
				return NULL;
		}

		if(tCheckECM)
			if(AnyECMDisturbed(tempMech))
				return NULL;

		if(tCheckStarted)
			if(!Started(tempMech))
				return NULL;

		if(tCheckUncon)
			if(Uncon(tempMech))
				return NULL;

		return tempMech;
	}

	return NULL;
}

void buildTempNetwork(MECH * mech, dbref * myNetwork, int *networkSize,
					  int tCheckECM, int tCheckStarted, int tCheckUncon,
					  int tIsC3)
{
	int tempNetworkSize = 0;
	int baseNetworkSize;
	MECH *otherMech;
	dbref myTempNetwork[C3_NETWORK_SIZE];
	int i;

	/* Re-init the network */
	for(i = 0; i < C3_NETWORK_SIZE; i++)
		myNetwork[i] = -1;

	*networkSize = 0;

	baseNetworkSize =
		(tIsC3 ? MechC3NetworkSize(mech) : MechC3iNetworkSize(mech));

	if(baseNetworkSize == 0)
		return;

	/*
	 * Build the base netork of all the mechs that fit the criteria we passed in
	 */
	for(i = 0; i < baseNetworkSize; i++) {
		otherMech =
			getOtherMechInNetwork(mech, i, tCheckECM, tCheckStarted,
								  tCheckUncon, tIsC3);

		if(!otherMech)
			continue;

		if(!Good_obj(otherMech->mynum))
			continue;

		myTempNetwork[tempNetworkSize] = otherMech->mynum;
		tempNetworkSize++;
	}

	/*
	 * Once we're here, we're done with the C3i stuff, but we need to make sure that this is a valid C3 network
	 * still. For example, we may have lost a master due to death or something else, so we need to make sure we
	 * have enough masters left to actually do something.
	 *
	 * A valid network is one where there are MIN((((NUM_MASTERS * 4) - NUM_MASTERS) + ((MY_MASTERS * 4) - MY_MASTERS), 11) units in the network
	 */
	if(tIsC3) {
		if(tempNetworkSize > 0)
			tempNetworkSize =
				trimC3Network(mech, myTempNetwork, tempNetworkSize);
	}

	for(i = 0; i < tempNetworkSize; i++)
		myNetwork[i] = myTempNetwork[i];

	*networkSize = tempNetworkSize;
}

void sendNetworkMessage(dbref player, MECH * mech, char *msg, int tIsC3)
{
	int i;
	MECH *otherMech;
	const char *c = GetMechID(mech);
	char buf[LBUF_SIZE];
	int networkSize;
	dbref myNetwork[C3_NETWORK_SIZE];

	buildTempNetwork(mech, myNetwork, &networkSize, 1, 1, 1, tIsC3);

	for(i = 0; i < networkSize; i++) {
		otherMech = getMechInTempNetwork(i, myNetwork, networkSize);

		if(!otherMech)
			continue;

		if(!Good_obj(otherMech->mynum))
			continue;

		sprintf(buf, "%%ch%s/%s: %s%%cn", (tIsC3 ? "C3" : "C3i"), c, msg);
		mech_notify(otherMech, MECHALL, buf);
	}

	sprintf(buf, "%%ch%s/You: %s%%cn", (tIsC3 ? "C3" : "C3i"), msg);
	mech_notify(mech, MECHALL, buf);
}

void showNetworkTargets(dbref player, MECH * mech, int tIsC3)
{
	MAP *objMap = getMap(mech->mapindex);
	int i, j, wTemp, bearing;
	MECH *otherMech;
	float realRange, c3Range;
	char buff[100];
	char *mech_name;
	char move_type[30];
	char cStatus1, cStatus2, cStatus3, cStatus4, cStatus5;
	char weaponarc;
	int losFlag;
	int arc;
	int wSeeTarget = TARG_LOS_NONE;
	int wC3SeeTarget = TARG_LOS_NONE;
	int tShowStatusInfo = 0;
	char bufflist[MAX_MECHS_PER_MAP][120];
	float rangelist[MAX_MECHS_PER_MAP];
	int buffindex = 0;
	int sbuff[MAX_MECHS_PER_MAP];
	int networkSize;
	dbref myNetwork[C3_NETWORK_SIZE];
	dbref c3Ref;

	buildTempNetwork(mech, myNetwork, &networkSize, 1, 1, 0, tIsC3);

	/*
	 * Send then a 'contacts' style report. This is different from the
	 * normal contacts since it has a 'physical' range in it too.
	 */
	notify_printf(player, "%s Contacts:", tIsC3 ? "C3" : "C3i");

	for(i = 0; i < objMap->first_free; i++) {
		if(!(objMap->mechsOnMap[i] != mech->mynum &&
			 objMap->mechsOnMap[i] != -1))
			continue;

		otherMech = (MECH *) FindObjectsData(objMap->mechsOnMap[i]);

		if(!otherMech)
			continue;

		if(!Good_obj(otherMech->mynum))
			continue;

		tShowStatusInfo = 0;
		realRange = FlMechRange(objMap, mech, otherMech);
		losFlag =
			InLineOfSight(mech, otherMech, MechX(otherMech),
						  MechY(otherMech), realRange);

		/*
		 * If we do see them, let's make sure it's not just a 'something'
		 */
		if(losFlag) {
			if(InLineOfSight_NB(mech, otherMech, MechX(otherMech),
								MechY(otherMech), 0.0))
				wSeeTarget = TARG_LOS_CLEAR;
			else
				wSeeTarget = TARG_LOS_SOMETHING;
		} else
			wSeeTarget = TARG_LOS_NONE;

		/*
		 * If I don't see it, let's see if someone else in the network does
		 */
		if(wSeeTarget != TARG_LOS_CLEAR)
			wC3SeeTarget = mechSeenByNetwork(mech, otherMech, tIsC3);

		/* If noone sees it, we continue */
		if(!wSeeTarget && !wC3SeeTarget)
			continue;

		/* Get our network range */
		c3Range =
			findC3RangeWithNetwork(mech, otherMech, realRange, myNetwork,
								   networkSize, &c3Ref);

		/* Figure out if we show the info or not... ie, do we actually 'see' it */
		if((wSeeTarget != TARG_LOS_CLEAR) && (wC3SeeTarget != TARG_LOS_CLEAR)) {
			tShowStatusInfo = 0;
			mech_name = "something";
		} else {
			tShowStatusInfo = 1;
			mech_name = silly_atr_get(otherMech->mynum, A_MECHNAME);
		}

		bearing =
			FindBearing(MechFX(mech), MechFY(mech), MechFX(otherMech),
						MechFY(otherMech));
		strcpy(move_type, GetMoveTypeID(MechMove(otherMech)));

		/* Get our weapon arc */
		arc = InWeaponArc(mech, MechFX(otherMech), MechFY(otherMech));
		weaponarc = getWeaponArc(mech, arc);

		/* Now get our status chars */
		if(!tShowStatusInfo) {
			cStatus1 = ' ';
			cStatus2 = ' ';
			cStatus3 = ' ';
			cStatus4 = ' ';
			cStatus5 = ' ';
		} else {
			cStatus1 = getStatusChar(mech, otherMech, 1);
			cStatus2 = getStatusChar(mech, otherMech, 2);
			cStatus3 = getStatusChar(mech, otherMech, 3);
			cStatus4 = getStatusChar(mech, otherMech, 4);
			cStatus5 = getStatusChar(mech, otherMech, 5);
		}

		/* Now, build the string */
		sprintf(buff,
				"%s%c%c%c[%s]%c %-11.11s x:%3d y:%3d z:%3d r:%4.1f c:%4.1f b:%3d s:%5.1f h:%3d S:%c%c%c%c%c%s",
				otherMech->mynum == MechTarget(mech) ? "%ch%cr" :
				(tShowStatusInfo &&
				 !MechSeemsFriend(mech, otherMech)) ? "%ch%cy" : "",
				(losFlag & MECHLOSFLAG_SEESP) ? 'P' : ' ',
				(losFlag & MECHLOSFLAG_SEESS) ? 'S' : ' ', weaponarc,
				MechIDS(otherMech, MechSeemsFriend(mech, otherMech) ||
						!tShowStatusInfo), move_type[0], mech_name,
				MechX(otherMech), MechY(otherMech), MechZ(otherMech),
				realRange, c3Range, bearing, MechSpeed(otherMech),
				MechVFacing(otherMech), cStatus1, cStatus2, cStatus3,
				cStatus4, cStatus5, (otherMech->mynum == MechTarget(mech)
									 || !MechSeemsFriend(mech,
														 otherMech)) ? "%c" :
				"");

		rangelist[buffindex] = realRange;
		rangelist[buffindex] +=
			(MechStatus(otherMech) & DESTROYED) ? 10000 : 0;
		strcpy(bufflist[buffindex++], buff);
	}

	for(i = 0; i < buffindex; i++)
		sbuff[i] = i;

	/* print a sorted list of detected mechs */
	/* use the ever-popular bubble sort */
	for(i = 0; i < (buffindex - 1); i++)
		for(j = (i + 1); j < buffindex; j++)
			if(rangelist[sbuff[j]] > rangelist[sbuff[i]]) {
				wTemp = sbuff[i];
				sbuff[i] = sbuff[j];
				sbuff[j] = wTemp;
			}

	for(i = 0; i < buffindex; i++)
		notify(player, bufflist[sbuff[i]]);

	notify_printf(player, "End %s Contact List", tIsC3 ? "C3" : "C3i");
}

void showNetworkData(dbref player, MECH * mech, int tIsC3)
{
	int i, bearing;
	MECH *otherMech;
	float range;
	char buff[100];
	char *mech_name;
	char move_type[30];
	int networkSize;
	dbref myNetwork[C3_NETWORK_SIZE];

	notify_printf(player, "%s Network Status:", tIsC3 ? "C3" : "C3i");

	buildTempNetwork(mech, myNetwork, &networkSize, 1, 1, 0, tIsC3);

	for(i = 0; i < networkSize; i++) {
		otherMech = getMechInTempNetwork(i, myNetwork, networkSize);

		if(!otherMech)
			continue;

		if(!Good_obj(otherMech->mynum))
			continue;

		range = FlMechRange(objMap, mech, otherMech);
		bearing =
			FindBearing(MechFX(mech), MechFY(mech), MechFX(otherMech),
						MechFY(otherMech));

		strcpy(move_type, GetMoveTypeID(MechMove(otherMech)));

		mech_name = silly_atr_get(otherMech->mynum, A_MECHNAME);

		sprintf(buff,
				"%%ch%%cy[%s]%c %-12.12s x:%3d y:%3d z:%3d r:%4.1f b:%3d s:%5.1f h:%3d a: %3d i: %3d%%cn",
				MechIDS(otherMech, 1), move_type[0], mech_name,
				MechX(otherMech), MechY(otherMech), MechZ(otherMech), range,
				bearing, MechSpeed(otherMech), MechVFacing(otherMech),
				getRemainingArmorPercent(otherMech),
				getRemainingInternalPercent(otherMech));

		notify(player, buff);

	}

	notify_printf(player, "End %s Network Status", tIsC3 ? "C3" : "C3i");
}

int mechSeenByNetwork(MECH * mech, MECH * mechTarget, int tIsC3)
{
	int los = TARG_LOS_NONE;
	float range = 0.0;
	int i;
	int networkSize;
	dbref myNetwork[C3_NETWORK_SIZE];
	MECH *otherMech;

	buildTempNetwork(mech, myNetwork, &networkSize, 1, 1, 0, tIsC3);

	if(networkSize == 0)
		return TARG_LOS_NONE;

	for(i = 0; i < networkSize; i++) {
		otherMech = getMechInTempNetwork(i, myNetwork, networkSize);

		if(!otherMech)
			continue;

		if(!Good_obj(otherMech->mynum))
			continue;

		if(otherMech == mechTarget)
			continue;

		range = FaMechRange(otherMech, mechTarget);
		los =
			InLineOfSight(otherMech, mechTarget, MechX(mechTarget),
						  MechY(mechTarget), range);

		if(los) {
			if(!InLineOfSight_NB(otherMech, mechTarget, MechX(mechTarget),
								 MechY(mechTarget), range))
				los = TARG_LOS_SOMETHING;
			else {
				los = TARG_LOS_CLEAR;
				break;
			}
		}
	}

	return los;
}

float findC3Range(MECH * mech, MECH * mechTarget, float realRange,
				  dbref * c3Ref, int tIsC3)
{
	int networkSize;
	dbref myNetwork[C3_NETWORK_SIZE];

	if(tIsC3) {
		if(C3Destroyed(mech)) {
			return realRange;
		}
	} else {
		if(C3iDestroyed(mech)) {
			validateC3iNetwork(mech);

			return realRange;
		}
	}

	if(AnyECMDisturbed(mech))
		return realRange;

	buildTempNetwork(mech, myNetwork, &networkSize, 1, 1, 0, tIsC3);

	return findC3RangeWithNetwork(mech, mechTarget, realRange, myNetwork,
								  networkSize, c3Ref);
}

float findC3RangeWithNetwork(MECH * mech, MECH * mechTarget,
							 float realRange, dbref * myNetwork,
							 int networkSize, dbref * c3Ref)
{
	float c3Range = 0.0;
	float bestRange = 0.0;
	int i;
	int inLOS = 0;
	int mapX, mapY;
	float hexX, hexY, hexZ;
	MECH *otherMech;
	MAP *map;

	bestRange = realRange;
	*c3Ref = 0;

	if(networkSize == 0)
		return realRange;

	for(i = 0; i < networkSize; i++) {
		otherMech = getMechInTempNetwork(i, myNetwork, networkSize);

		if(!otherMech)
			continue;

		if(!Good_obj(otherMech->mynum))
			continue;

		if(mechTarget) {
			if(otherMech == mechTarget)
				continue;

			debugC3(tprintf
					("C3RANGE-NETWORK (mech): Finding range from %d to %d.",
					 mech->mynum, mechTarget->mynum));

			c3Range = FaMechRange(otherMech, mechTarget);
			inLOS =
				InLineOfSight(otherMech, mechTarget, MechX(mechTarget),
							  MechY(mechTarget), c3Range);
		} else if((MechTargX(mech) > 0) && (MechTargY(mech) > 0)) {
			mapX = MechTargX(mech);
			mapY = MechTargY(mech);
			map = getMap(mech->mapindex);

			debugC3(tprintf
					("C3RANGE-NETWORK (hex): Finding range from %d to %d %d.",
					 mech->mynum, mapX, mapY));

			MechTargZ(mech) = Elevation(map, mapX, mapY);
			hexZ = ZSCALE * MechTargZ(mech);
			MapCoordToRealCoord(mapX, mapY, &hexX, &hexY);

			c3Range =
				FindRange(MechFX(otherMech), MechFY(otherMech),
						  MechFZ(otherMech), hexX, hexY, hexZ);
			inLOS = LOS_NB(otherMech, NULL, mapX, mapY, c3Range);
		} else {
			continue;
		}

		if(inLOS && (c3Range < bestRange)) {
			bestRange = c3Range;
			*c3Ref = otherMech->mynum;
		}
	}

	return bestRange;
}

void debugC3(char *msg)
{
	if(DEBUG_C3)
		SendDebug(msg);
}
