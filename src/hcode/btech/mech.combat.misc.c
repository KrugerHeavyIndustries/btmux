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
#include "mech.events.h"
#include "p.bsuit.h"
#include "p.crit.h"
#include "p.eject.h"
#include "p.mech.combat.misc.h"
#include "p.mech.fire.h"
#include "p.mech.partnames.h"
#include "p.mech.pickup.h"
#include "p.mech.tag.h"
#include "p.mech.update.h"
#include "p.mech.utils.h"

void decrement_ammunition(MECH * mech,
						  int weapindx,
						  int section,
						  int critical,
						  int ammoLoc,
						  int ammoCrit, int ammoLoc1, int ammoCrit1,
						  int wGattlingShots)
{
	int wGatSec = 0, wGatCrit = 0;
	int wShotsLeft = 0;
	int wCurShots = 0;
	int i = 0;
	int weapSize = 0;
	int firstCrit = 0;

	/* If we're an energy weapon or a PC weapon, return */
	if(!(MechWeapons[weapindx].type != TBEAM &&
		 MechWeapons[weapindx].type != THAND))
		return;

	/* If we're a rocket launcher, fire our load and return */
	if(MechWeapons[weapindx].special == ROCKET) {
		weapSize = GetWeaponCrits(mech, weapindx);
		firstCrit = FindFirstWeaponCrit(mech, section, critical, 0,
										GetPartType(mech, section, critical),
										weapSize);

		for(i = firstCrit; i < (firstCrit + weapSize); i++) {
			GetPartFireMode(mech, section, i) |= ROCKET_FIRED;
		}

		return;
	}

	/* If we're a one-shot, set us used and return */
	if(GetPartFireMode(mech, section, critical) & OS_MODE) {
		GetPartFireMode(mech, section, critical) |= OS_USED;
		return;
	}
	/* Check the state of our weapon bins */
	ammo_expedinture_check(mech, weapindx, MAX(wGattlingShots,
											   ((GetPartFireMode
												 (mech, section,
												  critical) & ULTRA_MODE)
												||
												(GetPartFireMode
												 (mech, section,
												  critical) & RFAC_MODE))));

	if((GetPartFireMode(mech, section, critical) & GATTLING_MODE) ||
	   (MechWeapons[weapindx].special & RAC)) {
		if(GetPartFireMode(mech, section, critical) & GATTLING_MODE)
			wShotsLeft = wGattlingShots * 3;
		else {
			if(GetPartFireMode(mech, section, critical) & RAC_TWOSHOT_MODE)
				wShotsLeft = 2;
			else if(GetPartFireMode(mech, section, critical) &
					RAC_FOURSHOT_MODE)
				wShotsLeft = 4;
			else if(GetPartFireMode(mech, section, critical) &
					RAC_SIXSHOT_MODE)
				wShotsLeft = 6;
			else
				wShotsLeft = 1;
		}

		while (wShotsLeft > 0) {
			FindAmmoForWeapon_sub(mech, section, critical, weapindx,
								  section, &wGatSec, &wGatCrit, AMMO_MODES,
								  0);
			wCurShots = GetPartData(mech, wGatSec, wGatCrit);

			if(wCurShots) {
				if(wCurShots >= wShotsLeft) {
					SetPartData(mech, wGatSec, wGatCrit,
								wCurShots - wShotsLeft);
					wShotsLeft = 0;
				} else {
					SetPartData(mech, wGatSec, wGatCrit, 0);
					wShotsLeft -= wCurShots;
				}
			}

			if(CountAmmoForWeapon(mech, weapindx) <= 0)
				break;
		}
	} else {					/* Non-RAC/Gattling */
		/* Decrement our ammo one shot */
		if(GetPartData(mech, ammoLoc, ammoCrit))
			GetPartData(mech, ammoLoc, ammoCrit)--;

		/* If we're ultra or rfac, decrement it again */
		if((GetPartFireMode(mech, section, critical) & ULTRA_MODE) ||
		   (GetPartFireMode(mech, section, critical) & RFAC_MODE))
			if(GetPartData(mech, ammoLoc1, ammoCrit1))
				GetPartData(mech, ammoLoc1, ammoCrit1)--;
	}
}

void ammo_expedinture_check(MECH * mech, int weapindx, int ns)
{
	int targ = I2Ammo(weapindx);
	int cnt = 0, slots = 0;
	int t, t2;
	int i, j, cl;
	int sev = 0;

	SetWCheck(mech);

	if(!MechAmmoWarn(mech))
		return;

	for(i = 0; i < NUM_SECTIONS; i++) {
		cl = CritsInLoc(mech, i);
		for(j = 0; j < cl; j++)
			if(GetPartType(mech, i, j) == targ) {
				cnt += GetPartData(mech, i, j);
				slots += AmmoMod(mech, i, j);
			}
	}
	t = BOUNDED(3, (slots * MechWeapons[weapindx].ammoperton) / 8, 30);
	t2 = 2 * t;
	if((cnt == (t + ns)) || (ns && cnt >= t && cnt < (t + ns)))
		sev = 1;
	else if((cnt == (t2 + ns)) || (ns && cnt >= t2 && cnt < (t2 + ns)))
		sev = 0;
	else
		return;
	/* Okay, we have case of warning here */
        if(Started(mech))
		if ((sev * 65536 + weapindx) % 65536)
		        mech_printf(mech, MECHALL,"%sWARNING: Ammo for %s is running low.%%c",sev? "%ch%cr" : "%ch%cy",
					get_parts_long_name(I2Weapon(weapindx), 0));
}

void heat_effect(MECH * mech, MECH * tempMech, int heatdam, int fromInferno)
{
	if(MechType(tempMech) != CLASS_MECH && MechType(tempMech) != CLASS_MW
	   && MechType(tempMech) != CLASS_BSUIT && !IsDS(tempMech) &&
	   MechMove(tempMech) != MOVE_NONE) {

		if(((MechType(tempMech) == CLASS_VEH_GROUND) ||
			(MechType(tempMech) == CLASS_VTOL)) &&
		   mudconf.btech_fasaadvvhlfire) {
			if(fromInferno)
				vehicle_start_burn(tempMech, mech);
			else
				checkVehicleInFire(tempMech, 0);
		} else {
			if(Roll() > 8) {
				MechLOSBroadcast(tempMech, "explodes!");
				mech_notify(tempMech, MECHALL,
							"The heat's too much for your vehicle! It blows up!");
				Destroy(tempMech);
				ChannelEmitKill(tempMech, mech);
				explode_unit(tempMech, mech ? mech : tempMech);
			}
		}
	} else {

		if(heatdam)
			inferno_burn(tempMech, heatdam * 6);
	}
}

/* Burn.. burn in hell! ;> */
void Inferno_Hit(MECH * mech, MECH * hitMech, int missiles, int LOS)
{
	int hmod = (missiles + 1) / 2;

	if(Jellied(hitMech) || Burning(hitMech)) {
		MechLOSBroadcast(hitMech, "burns a bit more brightly.");
		mech_notify(hitMech, MECHALL,
					"%ch%crMore burning jelly joins the flames!%cn");
	} else {
		MechLOSBroadcast(hitMech, "suddenly bursts into flames!");
		mech_notify(hitMech, MECHALL,
					"%ch%crYou are sprayed with burning jelly!%cn");
	}
	heat_effect(mech, hitMech, hmod * 30, 1);	/* 3min for _each_ missile */
	water_extinguish_inferno(hitMech); /* They could be in -2 standing or -1 prone.. Shooter just wastes his missiles! */
}

//extern int global_kill_cheat;
void KillMechContentsIfIC(dbref aRef)
{
	//global_kill_cheat = 1;
	if(!In_Character(aRef))
		return;
	if(!mudconf.btech_ic || mudconf.btech_xploss >= 1000)
		tele_contents(aRef, AFTERLIFE_DBREF, TELE_LOUD);
	else
		tele_contents(aRef, AFTERLIFE_DBREF, TELE_XP | TELE_LOUD);
}

#define BOOMLENGTH 24
char BOOM[BOOMLENGTH][80] = {
	"                              ________________",
	"                         ____/ (  (    )   )  \\___",
	"                        /( (  (  )   _    ))  )   )\\",
	"                      ((     (   )(    )  )   (   )  )",
	"                    ((/  ( _(   )   (   _) ) (  () )  )",
	"                   ( (  ( (_)   ((    (   )  .((_ ) .  )_",
	"                  ( (  )    (      (  )    )   ) . ) (   )",
	"                 (  (   (  (   ) (  _  ( _) ).  ) . ) ) ( )",
	"                 ( (  (   ) (  )   (  ))     ) _)(   )  )  )",
	"                ( (  ( \\ ) (    (_  ( ) ( )  )   ) )  )) ( )",
	"                 (  (   (  (   (_ ( ) ( _    )  ) (  )  )   )",
	"                ( (  ( (  (  )     (_  )  ) )  _)   ) _( ( )",
	"                 ((  (   )(    (     _    )   _) _(_ (  (_ )",
	"                  (_((__(_(__(( ( ( |  ) ) ) )_))__))_)___)",
	"                  ((__)        \\\\||lll|l||///          \\_))",
	"                           (   /(/ (  )  ) )\\   )",
	"                         (    ( ( ( | | ) ) )\\   )",
	"                          (   /(| / ( )) ) ) )) )",
	"                        (     ( ((((_(|)_)))))     )",
	"                         (      ||\\(|(|)|/||     )",
	"                       (        |(||(||)||||        )",
	"                         (     //|/l|||)|\\\\ \\     )",
	"                       (/ / //  /|//||||\\\\  \\ \\  \\ _)",
	"----------------------------------------------------------------------------"
};

void DestroyMech(MECH * target, MECH * mech, int bc)
{
	int loop;
	MAP *mech_map;
	MECH *ttarget;
	MECH *ctarget;

	dbref a,b;

	if(Destroyed(target)) {
		return;
	}
	//global_kill_cheat = 1;
	if(mech && target)
		ChannelEmitKill(target, mech);
	else
		ChannelEmitKill(target, target);
	if(mech) {
		if(bc) {
			if(mech != target) {
				mech_notify(mech, MECHALL, "You destroyed the target!");
				MechLOSBroadcasti(target, mech, "has been destroyed by %s!");
			} else
				MechLOSBroadcast(target, "has been destroyed!");
		}
		for(loop = 0; loop < BOOMLENGTH; loop++)
			mech_notify(target, MECHALL, BOOM[loop]);
		switch (MechType(target)) {
		case CLASS_MW:
		case CLASS_BSUIT:
			mech_notify(target, MECHALL, "You have been killed!");
			break;
		default:
			mech_notify(target, MECHALL, "You have been destroyed!");
			break;
		}
		mech_map = getMap(target->mapindex);
		if((mudconf.btech_vtol_ice_causes_fire)
		   && (MechSpecials(target) & ICE_TECH)
		   && (MechType(target) == CLASS_VTOL)) {
			MechLOSBroadcast(target, "explodes in a ball of flames!");
			add_decoration(mech_map, MechX(target), MechY(target), TYPE_FIRE,
						   FIRE, FIRE_DURATION);
		}

		if(MechCarrying(target) > 0) {
			if((ttarget = getMech(MechCarrying(target)))) {
				mech_notify(ttarget, MECHALL,
							"Your tow lines go suddenly slack!");
				mech_dropoff(GOD, target, "");
			}
		}
	}

	/* destroy contents if they are units and the containter is IC*/
	SAFE_DOLIST(a,b,Contents(target->mynum))
		if(IsMech(a) && In_Character(a)) {
			ctarget = getMech(a);
			mech_notify(ctarget, MECHALL, "Due to your transport's destruction, your unit has been destroyed!");
			mech_udisembark(a, ctarget, "");
			DestroyMech(ctarget,mech,0);
		}
	
	/* shut it down */
	if(mech) {
		DestroyAndDump(target);
	} else {
		Destroy(target);
	}
	if(MechType(target) == CLASS_MW) {
		if(In_Character(target->mynum)) {
			KillMechContentsIfIC(target->mynum);
			discard_mw(target);
		}
	}
}

char *short_hextarget(MECH * mech)
{

	if(MechStatus(mech) & LOCK_HEX_IGN)
		return "ign";
	if(MechStatus(mech) & LOCK_HEX_CLR)
		return "clr";
	if(MechStatus(mech) & LOCK_HEX)
		return "hex";
	if(MechStatus(mech) & LOCK_BUILDING)
		return "bld";
	return "reg";
}
