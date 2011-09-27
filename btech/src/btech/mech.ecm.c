
/*
 * $Id: mech.ecm.c,v 1.2 2005/06/23 18:31:42 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Fri Mar 21 15:13:06 1997 fingon
 * Last modified: Sat Oct 25 18:03:03 1997 fingon
 *
 */

#include "mech.h"
#include "p.mech.utils.h"
#include "mech.ecm.h"

	/*
	 * This is a rewrite of the ECM code to support ECCM. I've redone everything
	 * so that multiple ECM/ECCM units can operate and gain the advantage in numbers
	 *
	 * Mar.10.2001
	 * Kipsta
	 */

#define LOUD_ECM				/* No complete stealth */
#undef VERY_LOUD_ECM			/* But no spam, either */

void sendECMNotification(MECH * objMech, int wMsgType)
{
	switch (wMsgType) {
	case ECM_NOTIFY_DISTURBED:

#ifdef LOUD_ECM
		mech_notify(objMech, MECHALL,
					"Half your screens are suddenly filled with static!");

#ifdef VERY_LOUD_ECM
		if(MechSpecials(objMech) & BEAGLE_PROBE_TECH &&
		   !(MechCritStatus(objMech) & BEAGLE_DESTROYED))
			mech_notify(objMech, MECHALL,
						"Your Beagle Active Probe is inoperative!");
		if(MechSpecials2(objMech) & BLOODHOUND_PROBE_TECH &&
		   !(MechCritStatus(objMech) & BLOODHOUND_DESTROYED))
			mech_notify(objMech, MECHALL,
						"Your Bloodhound Active Probe is inoperative!");
		if(MechSpecials(objMech) & LIGHT_BAP_TECH &&
		   !(MechCritStatus2(objMech) & LIGHT_BAP_DESTROYED))
		   	mech_notify(objMech, MECHALL,
						"Your Light Beagle Active Probe is inoperative!");
		if(IsC3(objMech))
			mech_notify(objMech, MECHALL, "Your C3 network is inoperative!");
		if(IsC3i(objMech))
			mech_notify(objMech, MECHALL, "Your C3i network is inoperative!");

#endif /*  */
#endif /*  */
		break;
	case ECM_NOTIFY_UNDISTURBED:

#ifdef LOUD_ECM
		mech_notify(objMech, MECHALL,
					"All your systems are back to normal again!");

#ifdef VERY_LOUD_ECM
		if(MechSpecials(objMech) & BEAGLE_PROBE_TECH &&
		   !(MechCritStatus(objMech) & BEAGLE_DESTROYED))
			mech_notify(objMech, MECHALL,
						"Your Beagle Active Probe is operational again!");
		if(MechSpecials2(objMech) & BLOODHOUND_PROBE_TECH &&
		   !(MechCritStatus(objMech) & BLOODHOUND_DESTROYED))
			mech_notify(objMech, MECHALL,
						"Your Bloodhound Active Probe is operational again!");
		if(MechSpecials(objMech) & LIGHT_BAP_TECH &&
		   !(MechCritStatus2(objMech) & LIGHT_BAP_DESTROYED))
		   	mech_notify(objMech, MECHALL,
						"Your Light Beagle Active Probe is operational again!");
		if(IsC3(objMech))
			mech_notify(objMech, MECHALL,
						"Your C3 network is operational again!");
		if(IsC3i(objMech))
			mech_notify(objMech, MECHALL,
						"Your C3i network is operational again!");

#endif /*  */
#endif /*  */
		break;
	case ECM_NOTIFY_COUNTERED:
		if(HasWorkingECMSuite(objMech))
			mech_notify(objMech, MECHALL,
						"Your ECM suite's ready light turns red, countered by enemy ECCM!");
		break;
	case ECM_NOTIFY_UNCOUNTERED:
		if(HasWorkingECMSuite(objMech))
			mech_notify(objMech, MECHALL,
						"Your ECM suite's ready light turns green, enemy ECCM is out of range.");
		break;
	}
}
void checkECM(MECH * objMech)
{
	MAP *objMapmap;
	MECH *objOtherMech;
	float range = 0.0;

	int wFriendlyECM = 0;
	int wFriendlyECCM = 0;
	int wUnFriendlyECM = 0;
	int wUnFriendlyECCM = 0;

	int wFriendlyAngelECM = 0;
	int wFriendlyAngelECCM = 0;
	int wUnFriendlyAngelECM = 0;
	int wUnFriendlyAngelECCM = 0;

	int wFriendlyECMDelta = 0;
	int wFriendlyECCMDelta = 0;

	int tCheckECM = 0;
	int tCheckECCM = 0;

	int wIter = 0;
	int tMark = 0;

	if(!(objMapmap = FindObjectsData(objMech->mapindex)))	/* get our map */
		return;

	for(wIter = 0; wIter < objMapmap->first_free; wIter++) {
		if(!(objOtherMech = FindObjectsData(objMapmap->mechsOnMap[wIter])))
			continue;

		if((range = FaMechRange(objOtherMech, objMech)) > ECM_RANGE)
			continue;

		if(MechTeam(objOtherMech) == MechTeam(objMech)) {
			if(ECMEnabled(objOtherMech))
				wFriendlyECM++;

			if(ECCMEnabled(objOtherMech))
				wFriendlyECCM++;

			if(AngelECMEnabled(objOtherMech))
				wFriendlyAngelECM++;

			if(AngelECCMEnabled(objOtherMech))
				wFriendlyAngelECCM++;

			if(range <= 0.5) {
				if(PerECMEnabled(objOtherMech))
					wFriendlyECM++;

				if(PerECCMEnabled(objOtherMech))
					wFriendlyECCM++;
			}
		} else {
			if(ECMEnabled(objOtherMech))
				wUnFriendlyECM++;

			if(ECCMEnabled(objOtherMech))
				wUnFriendlyECCM++;

			if(AngelECMEnabled(objOtherMech))
				wUnFriendlyAngelECM++;

			if(AngelECCMEnabled(objOtherMech))
				wUnFriendlyAngelECCM++;

			if(range <= 0.5) {
				if(PerECMEnabled(objOtherMech))
					wUnFriendlyECM++;

				if(PerECCMEnabled(objOtherMech))
					wUnFriendlyECCM++;
			}
		}
	}

	if((MechStatus2(objMech) & STH_ARMOR_ON) ||
	   checkAllSections(objMech, INARC_ECM_ATTACHED))
		wUnFriendlyECM += 1000;

	/* Generate our deltas */
	wFriendlyECMDelta =
		wFriendlyECM + (2 * wFriendlyAngelECM) - wUnFriendlyECCM -
		(2 * wUnFriendlyAngelECCM);
	wFriendlyECCMDelta =
		wFriendlyECCM + (2 * wFriendlyAngelECCM) - wUnFriendlyECM -
		(2 * wUnFriendlyAngelECM);

	tCheckECM = ((wFriendlyECM != 0) || (wFriendlyAngelECM != 0) ||
				 (wUnFriendlyECCM != 0) || (wUnFriendlyAngelECCM != 0));
	tCheckECCM = ((wFriendlyECCM != 0) || (wFriendlyAngelECCM != 0) ||
				  (wUnFriendlyECM != 0) || (wUnFriendlyAngelECM != 0));

	/* SendDebug(tprintf("Checking unit %d. ECMDelta: %d. ECCMDelta: %d. CheckECM: %d. CheckECCM: %d",objMech->mynum,wFriendlyECMDelta,wFriendlyECCMDelta,tCheckECM,tCheckECCM)); */

	/* Now we do our checks... */
	/* Let's first see if we should just reset our flags... 'cause there's no ECM or ECCM around */
	if(!tCheckECM) {
		if(ECMCountered(objMech)) {
			sendECMNotification(objMech, ECM_NOTIFY_UNCOUNTERED);
			UnSetECMCountered(objMech);
			tMark = 1;
		}

		if(ECMProtected(objMech) || AngelECMProtected(objMech)) {
			UnSetECMProtected(objMech);
			UnSetAngelECMProtected(objMech);
			tMark = 1;
		}
	}

	if(!tCheckECCM) {
		if(AnyECMDisturbed(objMech)) {
			sendECMNotification(objMech, ECM_NOTIFY_UNDISTURBED);
			UnSetECMDisturbed(objMech);
			UnSetAngelECMDisturbed(objMech);
			tMark = 1;
		}
	}

	/* Sanity check so we don't bother to do all the other checks */
	if(!tCheckECM && !tCheckECCM) {
		if(tMark)
			MarkForLOSUpdate(objMech);

		return;
	}

	/* Now we see if our ECM has been countered */
	if(tCheckECM) {
		if(wFriendlyECMDelta <= 0) {	/* They have the same or more ECCM than we have ECM */
			if(!ECMCountered(objMech)) {
				sendECMNotification(objMech, ECM_NOTIFY_COUNTERED);
				SetECMCountered(objMech);
				UnSetECMProtected(objMech);
				UnSetAngelECMProtected(objMech);
			}
		} else {
			if(ECMCountered(objMech)) {
				sendECMNotification(objMech, ECM_NOTIFY_UNCOUNTERED);
				UnSetECMCountered(objMech);
			}

			if(wFriendlyECM > 0)
				SetECMProtected(objMech);
			else
				UnSetECMProtected(objMech);

			if(wFriendlyAngelECM > 0)
				SetAngelECMProtected(objMech);
			else
				UnSetAngelECMProtected(objMech);
		}
	}

	/* Now we see if we're under an enemy ECM umbrella */
	if(tCheckECCM) {
		if(wFriendlyECCMDelta < 0) {	/* They have more ECM than we have ECCM */
			if(!AnyECMDisturbed(objMech)) {
				sendECMNotification(objMech, ECM_NOTIFY_DISTURBED);

				if(wUnFriendlyECM > 0)
					SetECMDisturbed(objMech);
				else
					UnSetECMDisturbed(objMech);

				if(wUnFriendlyAngelECM > 0)
					SetAngelECMDisturbed(objMech);
				else
					UnSetAngelECMDisturbed(objMech);

				MarkForLOSUpdate(objMech);
			}
		} else {
			if(AnyECMDisturbed(objMech)) {
				sendECMNotification(objMech, ECM_NOTIFY_UNDISTURBED);

				UnSetECMDisturbed(objMech);
				UnSetAngelECMDisturbed(objMech);
				MarkForLOSUpdate(objMech);
			}
		}
	}
}

/* mech_ecm moved to mech.advanced.c */
