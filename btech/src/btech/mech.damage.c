/*
* Author: Cord Awtry <kipsta@mediaone.net>
*  Copyright (c) 2000-2002 Cord Awtry
*  Copyright (c) 1999-2005 Kevin Stevens
*       All rights reserved
*
* Based on work that was:
*  Copyright (c) 1997 Markus Stenberg
*  Copyright (c) 1998-2000 Thomas Wouters
*/

#include "mech.h"
#include "btmacros.h"
#include "mech.events.h"
#include "p.bsuit.h"
#include "p.btechstats.h"
#include "p.crit.h"
#include "p.eject.h"
#include "p.map.conditions.h"
#include "p.mech.ammodump.h"
#include "p.mech.build.h"
#include "p.mech.combat.misc.h"
#include "p.mech.combat.missile.h"
#include "p.mech.damage.h"
#include "p.mech.ecm.h"
#include "p.mech.hitloc.h"
#include "p.mech.move.h"
#include "p.mech.ood.h"
#include "p.mech.utils.h"
#include "p.mech.pickup.h"
#include "p.pcombat.h"

static char *MyColorStrings[] = {
	"", "%ch%cg", "%ch%cy", "%cr"
};
static char *MyMessageStrings[] = {
	"ERROR%c",
	"low.%c",
	"critical!%c",
	"BREACHED!%c"
};
static inline char *MySeriousColorStr(MECH * mech, int index)
{
	return MyColorStrings[index % 4];
}

static inline char *MySeriousStr(MECH * mech, int index)
{
	return MyMessageStrings[index % 4];
}

static inline int MySeriousnessCheck(MECH * mech, int hitloc)
{
	int orig, new;

	if(!(orig = GetSectOArmor(mech, hitloc)))
		return 0;
	if(!(new = GetSectArmor(mech, hitloc)))
		return 3;
	if(new < orig / 4)
		return 2;
	if(new < orig / 2)
		return 1;
	return 0;
}

static inline int MySeriousnessCheckR(MECH * mech, int hitloc)
{
	int orig, new;

	if(!(orig = GetSectORArmor(mech, hitloc)))
		return 0;
	if(!(new = GetSectRArmor(mech, hitloc)))
		return 3;
	if(new < orig / 4)
		return 2;
	if(new < orig / 2)
		return 1;
	return 0;
}

int cause_armordamage(MECH * wounded,
					  MECH * attacker,
					  int LOS,
					  int attackPilot,
					  int isrear,
					  int iscritical,
					  int hitloc, int damage, int *crits, int wWeapIndx,
					  int wAmmoMode)
{
	int intDamage = 0, r;
	int seriousness = 0;
	int tAPCritical = 0;
	int wPercentLeft = 0;

	if(MechType(wounded) == CLASS_MW)
		return (damage > 0) ? damage : 0;

	if((MechSpecials(wounded) & HARDA_TECH) && damage > 0)
		damage = (damage + 1) / 2;

	/* Now decrement armor, and if neccessary, handle criticals... */
	if(MechType(wounded) == CLASS_MECH && isrear && (hitloc == CTORSO
													 || hitloc == RTORSO
													 || hitloc == LTORSO)) {

		if((GetSectRArmor(wounded, hitloc) - damage) >= 0) {

			wPercentLeft = (((GetSectRArmor(wounded, hitloc) -
							  damage) * 100) / GetSectORArmor(wounded,
															  hitloc));
		}

		intDamage = damage - GetSectRArmor(wounded, hitloc);

		if(intDamage > 0) {
			SetSectRArmor(wounded, hitloc, 0);
			if(intDamage != damage)
				seriousness = 3;
		} else {
			seriousness = MySeriousnessCheckR(wounded, hitloc);
			SetSectRArmor(wounded, hitloc, GetSectRArmor(wounded,
														 hitloc) - damage);
			seriousness = (seriousness == MySeriousnessCheckR(wounded,
															  hitloc)) ? 0 :
				MySeriousnessCheckR(wounded, hitloc);
		}

	} else {

		/* Silly stuff */
		/*
		   SetSectArmor(wounded, hitloc, MAX(0, intDamage =  
		   GetSectArmor(wounded, hitloc) - damage));
		   intDamage = abs(intDamage);
		 */

		if(GetSectOArmor(wounded, hitloc) &&
		   ((GetSectArmor(wounded, hitloc) - damage) >= 0)) {

			wPercentLeft = (((GetSectArmor(wounded, hitloc) -
							  damage) * 100) / GetSectOArmor(wounded,
															 hitloc));
		}

		intDamage = damage - GetSectArmor(wounded, hitloc);

		if(intDamage > 0) {
			SetSectArmor(wounded, hitloc, 0);
			if(intDamage != damage)
				seriousness = 3;
		} else {
			seriousness = MySeriousnessCheck(wounded, hitloc);
			SetSectArmor(wounded, hitloc, GetSectArmor(wounded,
													   hitloc) - damage);
			seriousness = (seriousness == MySeriousnessCheck(wounded,
															 hitloc)) ? 0 :
				MySeriousnessCheck(wounded, hitloc);
		}

		if(!GetSectArmor(wounded, hitloc))
			MechFloodsLoc(wounded, hitloc, MechZ(wounded));
	}

	if(!iscritical && (wAmmoMode & AC_AP_MODE) && (intDamage <= 0) &&
	   (wPercentLeft < 50))
		tAPCritical = 1;

	if(iscritical || tAPCritical) {
		r = Roll();
		rollstat.critrolls[r - 2]++;
		rollstat.totcrolls++;
		/* Do the AP ammo thang */
		if(tAPCritical) {
			if(!strcmp(&MechWeapons[wWeapIndx].name[3], "AC/2"))
				r -= 4;
			else if(!strcmp(&MechWeapons[wWeapIndx].name[3], "LightAC/2"))
				r -= 4;
			else if(!strcmp(&MechWeapons[wWeapIndx].name[3], "AC/5"))
				r -= 3;
			else if(!strcmp(&MechWeapons[wWeapIndx].name[3], "LightAC/5"))
				r -= 3;
			else if(!strcmp(&MechWeapons[wWeapIndx].name[3], "AC/10"))
				r -= 2;
			else if(!strcmp(&MechWeapons[wWeapIndx].name[3], "AC/20"))
				r -= 1;
			else
				r -= 10;
		}

		switch (r) {
		case 8:
		case 9:
			HandleCritical(wounded, attacker, LOS, hitloc, 1);
			(*crits) += 1;
			break;
		case 10:
		case 11:
			HandleCritical(wounded, attacker, LOS, hitloc, 2);
			(*crits) += 2;
			break;
		case 12:
			HandleCritical(wounded, attacker, LOS, hitloc, 3);
			(*crits) += 3;
			break;
		default:
			break;
		}
		iscritical = 0;
	}

	if(MechType(wounded) == CLASS_AERO && intDamage >= 0) {
		DestroySection(wounded, attacker, LOS, hitloc);
		if(Destroyed(wounded)) {
			intDamage = 0;
			return 0;
		}
		switch (hitloc) {
		case AERO_AFT:
			MakeMechFall(wounded);
			MechSpeed(wounded) = 0;
			SetMaxSpeed(wounded, 0);
			MechVerticalSpeed(wounded) = 0;
			if(!(MechStatus(wounded) & LANDED))
				mech_notify(wounded, MECHALL, "You feel the thrust die..");
			else
				mech_notify(wounded, MECHALL,
							"The computer reports engine destroyed!");
			if(!Landed(wounded))
				MECHEVENT(wounded, EVENT_FALL, mech_fall_event, FALL_TICK,
						  -1);
			break;
		}
	}

	if(seriousness > 0 && MechArmorWarn(wounded))
		mech_printf(wounded, MECHALL, "%sWARNING: %s%s Armor %s",
					MySeriousColorStr(wounded, seriousness),
					ShortArmorSectionString(MechType(wounded),
											MechMove(wounded), hitloc),
					isrear ? " (Rear)" : "", MySeriousStr(wounded,
														  seriousness));

	return intDamage > 0 ? intDamage : 0;
}

int cause_internaldamage(MECH * wounded,
						 MECH * attacker,
						 int LOS,
						 int attackPilot,
						 int isrear, int hitloc, int intDamage, int weapindx,
						 int *crits)
{
	int r = Roll();
	char locname[30];
	char msgbuf[MBUF_SIZE];

	ArmorStringFromIndex(hitloc, locname, MechType(wounded),
						 MechMove(wounded));
	if((MechSpecials(wounded) & REINFI_TECH) && intDamage > 0)
		intDamage = (intDamage + 1) / 2;
	else if(MechSpecials(wounded) & COMPI_TECH)
		intDamage = intDamage * 2;
	/* Critical hits? */
	rollstat.critrolls[r - 2]++;
	rollstat.totcrolls++;
	if(!(*crits))
		switch (r) {
		case 8:
		case 9:
			HandleCritical(wounded, attacker, LOS, hitloc, 1);
			break;
		case 10:
		case 11:
			HandleCritical(wounded, attacker, LOS, hitloc, 2);
			break;
		case 12:
			if((MechType(wounded) == CLASS_MECH) ||
			   (MechType(wounded) == CLASS_MW)) {
				switch (hitloc) {
				case RARM:
				case LARM:
				case RLEG:
				case LLEG:
				case HEAD:
					/* Limb blown off */
					mech_notify(wounded, MECHALL, "%ch%cyCRITICAL HIT!!%c");
					if(!Destroyed(wounded)) {
						sprintf(msgbuf,
								"'s %s is blown off in a shower of sparks and smoke!",
								locname);
						MechLOSBroadcast(wounded, msgbuf);
					}
					DestroySection(wounded, attacker, LOS, hitloc);
					if(MechType(wounded) != CLASS_MW)
						intDamage = 0;
					break;
				default:
					/* Ouch */
					HandleCritical(wounded, attacker, LOS, hitloc, 3);
					break;
				}
			} else {
				HandleCritical(wounded, attacker, LOS, hitloc, 3);
			}

			break;
		default:
			break;
			/* No critical hit */
		}
	/* Hmm.. This should be interesting */
	if(MechType(wounded) == CLASS_MECH && intDamage && (hitloc == CTORSO)
	   && GetSectInt(wounded, hitloc) == GetSectOInt(wounded, hitloc))
		MechBoomStart(wounded) = muxevent_tick;

	if(GetSectInt(wounded, hitloc) <= intDamage) {
		intDamage -= GetSectInt(wounded, hitloc);
		DestroySection(wounded, attacker, LOS, hitloc);

/*    if (Destroyed(wounded)) */

/*      intDamage = 0;        */
	} else {
		SetSectInt(wounded, hitloc, GetSectInt(wounded, hitloc) - intDamage);
		intDamage = 0;
	}
	return intDamage;
}

int global_physical_flag = 0;
void DamageMech(MECH * wounded,
				MECH * attacker,
				int LOS,
				int attackPilot,
				int hitloc,
				int isrear,
				int iscritical,
				int damage,
				int intDamage, int cause, int bth, int wWeapIndx,
				int wAmmoMode, int tIgnoreSwarmers)
{
	char locationBuff[20];
	char notificationBuff[80];
	char rearMessage[10];
	int transfer = 0;
	int was_transfer = 0;
	int kill = 0;
	MAP *map;
	int crits = 0;
	int tBlowDumpingAmmo = 0;
	int wSwarmerHitChance = 0;
	int wRoll = Roll();
	MECH *mechSwarmer;
	int tSnapTowLines = 0;
	MECH *towTarget;
	damageNode *damagePointer;


	/* if:
	   damage = -1 && intDamage>0
	   - ammo expl
	   damage = -2 && intDamage>0
	   - transferred ammo expl
	   damage = n && intDamage = 0
	   - usual damage
	   damage = n && intDamage = -1/-2
	   - usual damage + transfer/+red enable */
	/* if damage>0 && !intDamage usual dam. */
	map = getMap(attacker->mapindex);
	if((map && MapIsCS(map)) || (MechStatus(wounded) & COMBAT_SAFE)) {
		if(wounded != attacker)
			mech_notify(attacker, MECHALL,
						"Your efforts only scratch the paint!");
		return;
	}

/* Rare case something passes through. We're in WEAPONS_HOLD. Don't even allow it */
	if( MechStatus2(attacker) & WEAPONS_HOLD) {
		if(wounded != attacker)
			mech_notify(attacker, MECHALL, 
						"You are currently in weapons hold!");
	}

	/* See if we have suits on us. If we get hit in any rear torso or the left/right front
	 * torsos, there's a chance the bsuits on us will suck up the damage. In fasa rules, there's
	 * no roll, but that's foolish if there's only one suits. 3030 rules are there's a 20 percent
	 * chance per suit on you that the suits will eat up the damage.
	 */
	if((CountSwarmers(wounded) > 0) && (!tIgnoreSwarmers)) {
		if((mechSwarmer = findSwarmers(wounded))) {
			if(!attacker || (attacker->mynum != mechSwarmer->mynum)) {
				wSwarmerHitChance = 20 * CountBSuitMembers(mechSwarmer);
				if(isrear) {
					if((hitloc != CTORSO) && (hitloc != RTORSO) &&
					   (hitloc != LTORSO))
						wSwarmerHitChance = 0;
				} else {
					if((hitloc != RTORSO) && (hitloc != LTORSO))
						wSwarmerHitChance = 0;
				}

				if((wSwarmerHitChance >= wRoll) &&
				   (GetSectArmor(wounded, hitloc))) {
					if(attacker && (attacker->mynum != wounded->mynum)) {
						mech_notify(attacker, MECHALL,
									"The battlesuits crawling all over your target absorb the damage!");
					}

					mech_notify(wounded, MECHALL,
								"The battlesuits crawling all over you absorb the damage!");
					mech_notify(mechSwarmer, MECHALL,
								"You absorb the damage!");
					hitloc =
						FindHitLocation(mechSwarmer, 0, &iscritical, &isrear);
					DamageMech(mechSwarmer, attacker, LOS,
							   attackPilot, hitloc, 0, 0, damage, 0,
							   cause, bth, wWeapIndx, wAmmoMode, 0);
					return;
				}

			}
		}
	}

	if(MechType(wounded) == CLASS_MW || MechType(wounded) == CLASS_MECH)
		transfer = 1;
#ifdef BT_MOVEMENT_MODES
	if((damage > 0 || intDamage > 0) && MechStatus2(wounded) & SPRINTING) {
		MechStatus2(wounded) &= ~SPRINTING;
		MechLOSBroadcast(wounded,
						 "breaks out of its sprint as it takes damage!");
		mech_notify(wounded, MECHALL,
					"You lose your sprinting momentum as you take damage!");
		if(!MoveModeChange(wounded))
			MECHEVENT(wounded, EVENT_MOVEMODE, mech_movemode_event, TURN,
					  MODE_OFF | MODE_SPRINT);
	}

	if((damage > 0 || intDamage > 0) && MechCritStatus(wounded) & HIDDEN) {
		MechCritStatus(wounded) &= ~HIDDEN;
		MechLOSBroadcast(wounded, "loses its cover as it takes damage!");
		mech_notify(wounded, MECHALL,
					"Your cover is ruined as you take damage!");
		if(!MoveModeChange(wounded))
			MechCritStatus(wounded) &= ~HIDDEN;
	}

	if((damage > 0 || intDamage > 0)
	   && (MoveModeLock(wounded)
		   && !(MoveModeData(wounded) &
				(MODE_EVADE | MODE_DODGE | MODE_OFF)))) {
		StopMoveMode(wounded);
		mech_notify(wounded, MECHALL,
					"Your movement mode changes are cancelled as you take damage!");
	}
#endif
	if(damage > 0 && intDamage == 0) {
		/* If we're a VTOL and the hitloc is the rotor,
		   we'll cut the damage by some value */
		if((MechType(wounded) == CLASS_VTOL) && (hitloc == ROTOR)) {
			if(mudconf.btech_divrotordamage > 0)
				damage = damage / mudconf.btech_divrotordamage;
			if(damage < 1)
				damage = 1;
		}

		if(MechCritStatus(wounded) & HIDDEN) {
			mech_notify(wounded, MECHALL,
						"Your cover is ruined as you take damage!");
			MechLOSBroadcast(wounded, "loses its cover as it takes damage.");
			MechCritStatus(wounded) &= ~HIDDEN;
		}

		if(!global_physical_flag)
			AccumulateGunXP(attackPilot, attacker, wounded, damage, 1,
							cause, bth);
		else if(global_physical_flag == 1)
			if(!Destroyed(wounded) && In_Character(wounded->mynum) &&
			   MechTeam(wounded) != MechTeam(attacker))
				if(MechType(wounded) != CLASS_MW ||
				   MechType(attacker) == CLASS_MW)
					AccumulatePilXP(attackPilot, attacker, damage / 3, 1);
		damage = dam_to_pc_conversion(wounded, cause, damage);
	}
	if(isrear) {
		if(!(MechSpecials(wounded) & SALVAGE_TECH) && (Roll() <= 5) &&
		   (hitloc == CTORSO || hitloc == LTORSO || hitloc == RTORSO))
			tSnapTowLines = 1;

		if(MechType(wounded) == CLASS_MECH) {
			strcpy(rearMessage, "(Rear)");
			if(Dumping(wounded) && ((hitloc == CTORSO) ||
									(hitloc == LTORSO) ||
									(hitloc == RTORSO)) && (cause >= 0))
					tBlowDumpingAmmo = 1;
		} else {
			if(hitloc == FSIDE)
				hitloc = BSIDE;
			*rearMessage = '\0';
			isrear = 0;
		}
	} else
		*rearMessage = '\0';
	/* Damage something else, ok? */
	if(damage < 0) {
		switch (damage) {
		case -2:
			was_transfer = 1;
		case -1:
			transfer = 1;
			break;
		}
		damage = 0;
	} else if(intDamage < 0) {
		switch (intDamage) {
		case -2:
			was_transfer = 1;
		case -1:
			transfer = 1;
			break;
		}
		intDamage = 0;
	}

	/*   while (SectIsDestroyed(wounded, hitloc) && !kill) */
	while (((!is_aero(wounded) && !GetSectInt(wounded, hitloc)) ||
			(is_aero(wounded) && !GetSectArmor(wounded, hitloc))) && !kill) {
		if(transfer && (hitloc = TransferTarget(wounded, hitloc)) >= 0
		   && (MechType(wounded) == CLASS_MECH ||
			   MechType(wounded) == CLASS_MW ||
			   MechType(wounded) == CLASS_BSUIT || is_aero(wounded))) {
			DamageMech(wounded, attacker, LOS, attackPilot, hitloc,
					   isrear, iscritical, damage == -1 ? -2 : damage,
					   transfer == 1 ? -2 : damage, cause, bth, wWeapIndx,
					   wAmmoMode, tIgnoreSwarmers);
			return;
		} else {
			if(!((MechType(wounded) == CLASS_MECH ||
				  MechType(wounded) == CLASS_MW ||
				  MechType(wounded) == CLASS_BSUIT ||
				  is_aero(wounded)) &&
				 (hitloc = TransferTarget(wounded, hitloc)) >= 0)) {
				if(is_aero(wounded) && !Destroyed(wounded)) {
					/* Hurt SI instead. */
					if(AeroSI(wounded) <= damage)
						kill = 1;
					else {
						AeroSI(wounded) -= damage;
						kill = -1;
					}
				} else
					return;
			}
			/* Nyah. Damage transferred to waste, shooting a dead mech? */
		}

	}
	if(C_OODing(wounded) && Roll() > 8) {
		mech_ood_damage(wounded, attacker,
						damage + (intDamage < 0 ? 0 : intDamage));
		return;
	}

	if(hitloc != -1) {
		ArmorStringFromIndex(hitloc, locationBuff, MechType(wounded),
							 MechMove(wounded));
		sprintf(notificationBuff,
				"for %d points of damage in the %s %s",
				damage + (intDamage < 0 ? 0 : intDamage), locationBuff,
				rearMessage);
	} else
		sprintf(notificationBuff,
				"for %d points of damage in the structure.",
				damage + (intDamage < 0 ? 0 : intDamage));

	/* Only count initial damage. Transfer is just gonna do that, transfer, not damage again */
	if (!was_transfer) {	
		if(attacker != wounded)
			MechDamageInflicted(attacker) = MechDamageInflicted(attacker) + damage + (intDamage < 0 ? 0 : intDamage);
		MechDamageTaken(wounded) = MechDamageTaken(wounded) + damage + (intDamage < 0 ? 0 : intDamage);
	}

	/*  if (LOS && attackPilot != -1) */
	if(LOS) {
		if(!was_transfer)
			mech_printf(attacker, MECHALL, "%%cgYou hit %s%%c",
						notificationBuff);
		else
			mech_printf(attacker, MECHALL,
						"%%cgDamage transfer.. %s%%c", notificationBuff);
	}
	if(MechType(wounded) == CLASS_MW && !was_transfer)
		if(damage > 0) 
			if(!(damage =
				 armor_effect(wounded, cause, hitloc, damage, intDamage)))
				return;
	mech_printf(wounded, MECHALL,
				"%%ch%%cyYou have been hit %s%s%%c",
				notificationBuff, was_transfer ? "(transfer)" : "");
			/* Always a good policy :-> */
	if(damage > 0 && intDamage <= 0 && !was_transfer && !Fallen(wounded)) {
		if(mudconf.btech_newstagger && MechType(wounded) == CLASS_MECH) {

		  // So now that stagger isn't completely retarded, here's how it works
		  // you take damage and every point of damage gets added to a linked list in the
		  // mech struct's RS object (mech->rd). This damage is a linked list, and each node
		  // contains the time of the damage, the amount, the attacker's dbref and a link to 
		  // the next damage. if the node->next is NULL, you're at the end of the list.


		  // Let's do this right. Get the damageNode, walk to the end, add the new damage
		  damagePointer = (wounded)->rd.staggerDamageList;

		  if(damagePointer == NULL) {
		    (wounded)->rd.staggerDamageList = malloc(sizeof(damageNode));
		    (wounded)->rd.staggerDamageList->amount = damage;
		    (wounded)->rd.staggerDamageList->occuredAt = mudstate.now;
		    (wounded)->rd.staggerDamageList->attackerNum = attacker->mynum;
		    (wounded)->rd.staggerDamageList->counted = 0;
		    (wounded)->rd.staggerDamageList->next = NULL;
		  } else {
		    // walk down to the last node on the list
  		    while(damagePointer->next != NULL) {
		      damagePointer = damagePointer->next;
  		    }
		    // set the last node equal to our new damage pointer
		    damagePointer->next = malloc(sizeof(damageNode));
		    damagePointer->next->amount = damage;
		    damagePointer->next->occuredAt = mudstate.now;
		    damagePointer->next->attackerNum = attacker->mynum;
		    damagePointer->next->counted = 0;
		    damagePointer->next->next = NULL;
		      }
		} else {
			MechTurnDamage(wounded) += damage;
		}
	}

	if(hitloc == HEAD && MechType(wounded) == CLASS_MECH) {

		/*      mech_notify (wounded, MECHALL,
		   "You take 10 points of Lethal damage!!"); */

/* Rule Reference: BMR Revised, Page 16 (Head Hit = 1 Bruise) */
/* Rule Reference: Total Warfare, Page 41 (Head Hit = 1 Bruise) */

		headhitmwdamage(wounded, attacker, 1);
	}
	if(kill) {
		if(kill == 1) {
			mech_notify(wounded, MECHALL,
						"The blast causes the last of your craft's structure to disintegrate, blowing");
			mech_notify(wounded, MECHALL, "its pieces all over the sky!");
			if(!Landed(wounded) && Started(wounded)) {
				mech_notify(attacker, MECHALL,
							"You shoot the craft from the sky!");
				MechLOSBroadcasti(attacker, wounded,
								  "shoots %s from the sky!");
			}
			DestroyMech(wounded, attacker, !(!Landed(wounded) &&
											 Started(wounded)), KILL_TYPE_NORMAL);
		}
		return;
	}
	if(damage > 0) {
		if(MechType(wounded) == CLASS_MECH) {
			if(!isrear && (MechSpecials(wounded) & SLITE_TECH) &&
			   !(MechCritStatus(wounded) & SLITE_DEST) &&
			   (hitloc == LTORSO || hitloc == CTORSO || hitloc == RTORSO)) {
				/* Possibly destroy the light */
				if(Roll() > 6) {
					if((MechStatus2(wounded) & SLITE_ON) || (Roll() > 5)) {
						MechCritStatus(wounded) |= SLITE_DEST;
						MechStatus2(wounded) &= ~SLITE_ON;
						MechLOSBroadcast(wounded,
										 "'s searchlight is blown apart!");
						mech_notify(wounded, MECHALL,
									"%ch%cyYour searchlight is destroyed!%cn");
					}
				}
			}
		}
		if(MechType(wounded) == CLASS_VEH_GROUND) {
			if(!isrear && (MechSpecials(wounded) & SLITE_TECH) &&
			   !(MechCritStatus(wounded) & SLITE_DEST) && (hitloc == FSIDE)) {
				/* Possibly destroy the light */
				if(Roll() > 6) {
					if((MechStatus2(wounded) & SLITE_ON) || (Roll() > 5)) {
						MechCritStatus(wounded) |= SLITE_DEST;
						MechStatus2(wounded) &= ~SLITE_ON;
						MechLOSBroadcast(wounded,
										 "'s searchlight is blown apart!");
						mech_notify(wounded, MECHALL,
									"%ch%cyYour searchlight is destroyed!%cn");
					}
				}
			}
		}
		intDamage +=
			cause_armordamage(wounded, attacker, LOS, attackPilot, isrear,
							  iscritical, hitloc, damage, &crits, wWeapIndx,
							  wAmmoMode);
		/* for Stat Engine */
		/* STATHIT|MAP|ATTACKER PILOT DBREF|WOUNDED PILOT DBREF|ATTACKER MECHREF|WOUNDED MECHREF|ATTACKER MECH DBREF|WOUNDED MECH DBREF|BTH OF SHOT|HITLOC|WEAPON NAME|Armor Damage|Internal Damage */
		/* The last part in the function is how we're handling transfer damage. We're going to check how much internal is left, do some math, and only count the applied damage */
		/* As the transfer damage will come back and send to another section via DamageMech calls */
		/* We're going to skip wWeapindx = -1 for now as well. Those are physicals (currently) and self inflicted */
		/* May make physicals -2, -3, -4, etc, but I'd rather not do all that logic. Maybe change to -2 and just add a 'PHYSICAL'...Though kick vs punch would be neat */
		/* LIGHTBULB:
		 * Make a 'special/physical' weapons table. We'll send the wWeapindx as a negative num. If its negative, (lower then -1 which will stay as selfdamage) we'll
		 * check a 'Physical Weapons Table' and abs() the value and pick the name out from there */
		if(mudconf.btech_statengine_obj > 0 && wWeapIndx != -1)
			notify_with_cause(mudconf.btech_statengine_obj, GOD, tprintf("STATHIT|#%d|#%d|#%d|%s|%s|#%d|#%d|%d|%s%s|%s|%d|%d",
			attacker->mapindex,MechPilot(attacker),MechPilot(wounded), MechType_Ref(attacker),MechType_Ref(wounded),attacker->mynum,wounded->mynum,bth,isrear ? "Rear " : "", hitloc != -1 ? locationBuff : "NONE",&MechWeapons[wWeapIndx].name[0],damage-intDamage,GetSectInt(wounded,hitloc) < intDamage ? intDamage-(intDamage-GetSectInt(wounded,hitloc)) : intDamage));

		if(intDamage >= 0)
			MechFloodsLoc(wounded, hitloc, MechZ(wounded));
		if(intDamage > 0 && !is_aero(wounded)) {
			intDamage =
				cause_internaldamage(wounded, attacker, LOS, attackPilot,
									 isrear, hitloc, intDamage, cause,
									 &crits);
			if(!intDamage && !SectIsDestroyed(wounded, hitloc))
				BreachLoc(attacker, wounded, hitloc);
		} else
			PossiblyBreach(attacker, wounded, hitloc);
		if(intDamage > 0 && transfer && (MechType(wounded) != CLASS_BSUIT)) {
			if((hitloc = TransferTarget(wounded, hitloc)) >= 0)
				DamageMech(wounded, attacker, LOS, attackPilot, hitloc,
						   isrear, iscritical, intDamage, -2, cause, bth,
						   wWeapIndx, wAmmoMode, tIgnoreSwarmers);
			else {
				DestroyMech(wounded, attacker, 1, KILL_TYPE_NORMAL);
				return;
			}
		}
	} else
		/* Cause _INTERNAL_ HAVOC! :-) */
		/* Non-CASE things get _really_ hurt */
	{
		if(intDamage > 0) {
			if(is_aero(wounded))
				intDamage =
					cause_armordamage(wounded, attacker, LOS, attackPilot,
									  isrear, iscritical, hitloc,
									  intDamage, &crits, wWeapIndx,
									  wAmmoMode);
			else
				intDamage =
					cause_internaldamage(wounded, attacker, LOS,
										 attackPilot, isrear, hitloc,
										 intDamage, cause, &crits);
			if(!SectIsDestroyed(wounded, hitloc))
				PossiblyBreach(attacker, wounded, hitloc);
			if(intDamage > 0 && transfer &&
			   !((MechSections(wounded)[hitloc].config & CASE_TECH) ||
				 (MechSpecials(wounded) & CLAN_TECH))) {
				if((hitloc = TransferTarget(wounded, hitloc)) >= 0) {
					if(!is_aero(wounded))
						DamageMech(wounded, attacker, LOS, attackPilot,
								   hitloc, isrear, iscritical, -2,
								   intDamage, cause, bth, wWeapIndx,
								   wAmmoMode, tIgnoreSwarmers);
					else
						DamageMech(wounded, attacker, LOS, attackPilot,
								   hitloc, isrear, iscritical, intDamage,
								   -2, cause, bth, wWeapIndx, wAmmoMode,
								   tIgnoreSwarmers);
				} else {
					DestroyMech(wounded, attacker, 1, KILL_TYPE_NORMAL);
					return;
				}
			}
		}
	}

	/* Check to see if the tow lines should snap */
	if(tSnapTowLines && (MechCarrying(wounded) > 0)) {
		if((towTarget = getMech(MechCarrying(wounded)))) {
			mech_notify(wounded, MECHALL,
						"The hit causes your tow line to let go!");
			mech_notify(towTarget, MECHALL,
						"Your tow lines go suddenly slack!");
			MechLOSBroadcast(wounded,
							 "'s tow lines release and flap freely behind it!");

			mech_dropoff(GOD, wounded, "");
		}
	}

	/* Check to see if we blow up ammo that's dumping. */
	if(tBlowDumpingAmmo) {
		BlowDumpingAmmo(wounded, attacker, hitloc);
	}
}

/* this takes care of setting all the criticals to CRIT_DESTROYED */
void DestroyWeapon(MECH * wounded, int hitloc, int type, int startCrit,
				   int numcrits, int totalcrits)
{
        int i;
        char sum = totalcrits;
        char destroyed = numcrits;
        int checkloc;
        int newcrit;
        int split;
	int disable = 0; //Hax for later destroying all crits or disabling

        for(i = startCrit; i < NUM_CRITICALS; i++) {
                if(GetPartType(wounded, hitloc, i) == type) {
                        if(PartIsDamaged(wounded, hitloc, i)) {
                                if (disable)
                                        DisablePart(wounded, hitloc, i);
                                else
                                        DestroyPart(wounded, hitloc, i);
                        } else if(destroyed) {
                                if (disable)
                                        DisablePart(wounded, hitloc, i);
                                else
                                        DestroyPart(wounded, hitloc, i);
                                destroyed--;
                        } else {
                                BreakPart(wounded, hitloc, i);
                        }
                        sum--;
//                      if(sum == totalcrits)
                        if (!sum)
                                return;
                }
        }
        // if we've gotten here, then sum != total crits, but we've run outta crits in this location, so it must be a split crit.
        if (MechType(wounded) != CLASS_MECH)
                return; // sanity check
        if (GetSplitData(wounded, hitloc, startCrit, &checkloc, &newcrit, &split))
                DestroyWeapon(wounded, checkloc, split, newcrit, destroyed, sum);
}

int CountWeaponsInLoc(MECH * mech, int loc)
{
	int i;
	int j, sec, cri;
	int count = 0;

	j = FindWeaponNumberOnMech(mech, 1, &sec, &cri);
	for(i = 2; j != -1; i++) {
		if(sec == loc)
			count++;
		j = FindWeaponNumberOnMech(mech, i, &sec, &cri);
	}
	return count;
}

int FindWeaponTypeNumInLoc(MECH * mech, int loc, int num)
{
	int i;
	int j, sec, cri;
	int count = 0;

	j = FindWeaponNumberOnMech(mech, 1, &sec, &cri);
	for(i = 2; j != -1; i++) {
		if(sec == loc) {
			count++;
			if(count == num)
				return j;
		}
		j = FindWeaponNumberOnMech(mech, i, &sec, &cri);
	}
	return -1;
}

void LoseWeapon(MECH * mech, int hitloc)
{
	/* Look for hit locations.. */
	int i = CountWeaponsInLoc(mech, hitloc);
	int a, b;
	int firstCrit;

	if(!i)
		return;
	a = Number(1, i);
	b = FindWeaponTypeNumInLoc(mech, hitloc, a);
	if(b < 0)
		return;

	firstCrit =
		FindFirstWeaponCrit(mech, hitloc, -1, 0, I2Weapon(b),
							GetWeaponCrits(mech, b));

	DestroyWeapon(mech, hitloc, I2Weapon(b), firstCrit, 1,
				  GetWeaponCrits(mech, b));
	mech_printf(mech, MECHALL, "%%ch%%crYour %s is destroyed!%%c",
				&MechWeapons[b].name[3]);
}

void DestroyHeatSink(MECH * mech, int hitloc)
{
	/* This can be done easily, or this can be done painfully. */
	/* Let's try the painful way, it's more fun that way. */
	int num;
	int i = I2Special(HEAT_SINK);

	if(FindObj(mech, hitloc, i)) {
		num = HS_Size(mech);
		DestroyWeapon(mech, hitloc, i, 0, 1, num);
		MechRealNumsinks(mech) -= MAX(num, 2);
		mech_notify(mech, MECHALL,
					"The computer shows a heatsink died due to the impact.");
	}
}

void DestroySection(MECH * wounded, MECH * attacker, int LOS, int hitloc)
{
	char locname[30];
	char msgbuf[MBUF_SIZE];
	int i, j;
	int tKillMech;
	int tIsLeg = ((hitloc == RLEG || hitloc == LLEG) ||
				  ((hitloc == RARM || hitloc == LARM)
				   && (MechIsQuad(wounded))));
	dbref wounded_pilot = MechPilot(wounded);
	MECH *ttarget;
	
	/* Prevent the rare occurance of a section getting destroyed twice */
	if(SectIsDestroyed(wounded, hitloc)) {
		fprintf(stderr, "Double-desting section %d on mech #%d\n",
				hitloc, wounded->mynum);
		if(IsDS(wounded))
			return;
		for(i = 0; i < NUM_SECTIONS; i++)
			if(GetSectOInt(wounded, i) && GetSectInt(wounded, i))
				return;
		if(muxevent_count_type_data(EVENT_NUKEMECH, (void *) wounded)) {
			fprintf(stderr, "And nuke event already existed.\n");
			return;
		}
		discard_mw(wounded);
		return;
	}
	/* Ouch. They got toasted */
	SetSectArmor(wounded, hitloc, 0);
	SetSectInt(wounded, hitloc, 0);
	SetSectRArmor(wounded, hitloc, 0);
	SetSectDestroyed(wounded, hitloc);
	MechSections(wounded)[hitloc].specials = 0;

	/* uncycle the section <in the case of an arm/leg that was kicking getting blown */
	SetRecycleLimb(wounded, hitloc, 0);
	
	/* drop off what we were carrying, since we really can't pick it up with one arm */
	if((hitloc == RARM || hitloc == LARM)) {
		if(MechCarrying(wounded) > 0) {
			if((ttarget = getMech(MechCarrying(wounded)))) {
				mech_notify(ttarget, MECHALL, "Your tow lines go suddenly slack!");
				mech_dropoff(GOD, wounded, "");
			}
		}
	}
		
	/* Tell the attacker about it... */
	if(attacker) {
		ArmorStringFromIndex(hitloc, locname, MechType(wounded),
							 MechMove(wounded));
		if(LOS >= 0)
			mech_printf(wounded, MECHALL,
						"Your %s has been destroyed!", locname);
		sprintf(msgbuf, "'s %s has been destroyed!", locname);
		MechLOSBroadcast(wounded, msgbuf);
	}

	/* Destroy everything in the loc */
	DestroyParts(attacker, wounded, hitloc, 0, 0);
	checkECM(wounded);
	/* Stop lateral if we're a quad */
	if((MechType(wounded) == CLASS_MECH) && MechIsQuad(wounded))
		if(MechLateral(wounded) && tIsLeg)
			MechLateral(wounded) = 0;
	/* Check to see if we should destroy the unit */
	if(MechType(wounded) == CLASS_BSUIT) {
		if(CountBSuitMembers(wounded) > 0)
			goto skip_nuke;
		else if(!Destroyed(wounded))
			DestroyMech(wounded, attacker, 1, KILL_TYPE_NORMAL);
	} else {
		for(i = 0; i < NUM_SECTIONS; i++)
			if(GetSectOInt(wounded, i) && GetSectInt(wounded, i))
				goto skip_nuke;
	}

	/* Ensure the template's timely demise */
	if(In_Character(wounded->mynum)) {
		/* Clear the freqs on the unit... */
		for(j = 0; j < FREQS; j++) {
			wounded->freq[j] = 0;
			wounded->freqmodes[j] = 0;
			wounded->chantitle[j][0] = 0;
		}

		/* There's a 25% chance of bsuit pilots living through it */
		if((MechType(wounded) == CLASS_BSUIT) &&
		   (Number(1, 100) <= 25) && wounded_pilot)
			autoeject(wounded_pilot, wounded, 1);
		else
			KillMechContentsIfIC(wounded->mynum);
		/* Schedule removal of the template */
		if(!IsDS(wounded))
			discard_mw(wounded);
	}

	/* We've done everything we should do... */
	return;
  skip_nuke:

	/* Add 4 MW damage if it's a MW loosing a location */
	if(MechType(wounded) == CLASS_MW) {
		mwlethaldam(wounded, attacker, 4);
	}

	/* If it's a MW or a mech, let's see if there's additional stuff we need to do */
	if(MechType(wounded) == CLASS_MW || MechType(wounded) == CLASS_MECH) {
		if(hitloc == LTORSO)
			DestroySection(wounded, attacker, LOS, LARM);
		else if(hitloc == RTORSO)
			DestroySection(wounded, attacker, LOS, RARM);
		else if(hitloc == CTORSO || hitloc == HEAD) {
			if(!Destroyed(wounded))
				if(hitloc == HEAD)
					if(MechAim(attacker) == HEAD) {
						DestroyMech(wounded, attacker, 1, KILL_TYPE_HEAD_TARGET);
					} else {
						DestroyMech(wounded, attacker, 1, KILL_TYPE_BEHEADED);
					}
				else
					DestroyMech(wounded, attacker, 1, KILL_TYPE_NORMAL);
			/* If it's the head or a MW's CT, kill the contents if IC */
			if(hitloc == HEAD || ((MechType(wounded) == CLASS_MW) &&
								  (hitloc == CTORSO))) {
				if(In_Character(wounded->mynum)) {
					for(j = 0; j < FREQS; j++) {
						wounded->freq[j] = 0;
						wounded->freqmodes[j] = 0;
						wounded->chantitle[j][0] = 0;
					}
					KillMechContentsIfIC(wounded->mynum);
				}
			}

			if(MechType(wounded) == CLASS_MW)
				discard_mw(wounded);
		}

		return;
	}

	/* If we're an aero... */
	if(is_aero(wounded)) {
		/* FIXME: Could this be the invincible aero bug? */
		/* Aero handling is trivial ; No destruction whatsoever, for now. */
		/* With one exception.. */
		if(hitloc == COCKPIT && MechType(wounded) == CLASS_AERO) {
			if(!Destroyed(wounded))
				DestroyMech(wounded, attacker, 0, KILL_TYPE_COCKPIT);
			for(j = 0; j < FREQS; j++) {
				wounded->freq[j] = 0;
				wounded->freqmodes[j] = 0;
				wounded->chantitle[j][0] = 0;
			}
			KillMechContentsIfIC(wounded->mynum);
		}
		return;
	}

	/* Last check to see if we destroy the unit... vehicle stuff */
	if(TransferTarget(wounded, 0) < 0)
		tKillMech = 1;
	else
		tKillMech = 0;
	switch (MechType(wounded)) {
	case CLASS_BSUIT:
		tKillMech = 0;
		break;
	case CLASS_VEH_GROUND:
		if(hitloc == TURRET) {
			tKillMech = 0;
			MechStatus2(wounded) &= ~AUTOTURN_TURRET;
		} else
			tKillMech = 1;
		break;
	case CLASS_VTOL:
		if(hitloc == ROTOR) {
			tKillMech = 0;
			StartVTOLCrash(wounded);
		} else
			tKillMech = 1;
		break;
	}

	if(tKillMech) {
		if(!Destroyed(wounded))
			DestroyMech(wounded, attacker, 1, KILL_TYPE_NORMAL);
	}
}

char *setarmorstatus_func(MECH * mech,
						  char *sectstr, char *typestr, char *valuestr)
{
	int index, type, value;

	if(!sectstr || !*sectstr)
		return "#-1 INVALID SECTION";
	index = ArmorSectionFromString(MechType(mech), MechMove(mech), sectstr);
	if(index == -1 || !GetSectOInt(mech, index))
		return "#-1 INVALID SECTION";
	if((value = atoi(valuestr)) < 0 || value > 255)
		return "#-1 INVALID ARMORVALUE";
	switch (type = atoi(typestr)) {
	case 0:
		SetSectArmor(mech, index, value);
		break;
	case 1:
		SetSectInt(mech, index, value);
		break;
	case 2:
		SetSectRArmor(mech, index, value);
		break;
	default:
		return "#-1 INVALID ARMORTYPE";
	}
	return "1";
}

int dodamage_func(dbref player,
				  MECH * mech,
				  int totaldam,
				  int clustersize,
				  int direction, int iscritical, char *mechmsg,
				  char *mechbroadcast)
{

	int hitloc = 1, this_time, isrear = 0, dummy = 0;
	int *dummy1 = &dummy, *dummy2 = &dummy;

	if(direction < 8) {
		hitloc = direction;
	} else if(direction < 16) {
		hitloc = direction - 8;
		isrear = 1;
	} else if(direction > 21) {
		return 0;
	}

	if(mechmsg && *mechmsg)
		mech_notify(mech, MECHALL, mechmsg);
	if(mechbroadcast && *mechbroadcast)
		MechLOSBroadcast(mech, mechbroadcast);
	while (totaldam) {
		if(direction > 18)
			isrear = 1;
		if(direction > 15)
			hitloc =
				FindHitLocation(mech, ((direction - 1) & 3) + 1, dummy1,
								dummy2);
		this_time = MIN(clustersize, totaldam);
		DamageMech(mech, mech, 0, -1, hitloc, isrear, iscritical,
				   this_time, 0, 0, 0, -1, 0, 1);
		totaldam -= this_time;
	}
	return 1;
}

void mech_damage(dbref player, MECH * mech, char *buffer)
{
	char *args[5];
	int damage, clustersize;
	int isrear, iscritical;

	DOCHECK(mech_parseattributes(buffer, args, 5) != 4, "Invalid arguments!");
	DOCHECK(Readnum(damage, args[0]), "Invalid damage!");
	DOCHECK(Readnum(clustersize, args[1]), "Invalid cluster size!");
	DOCHECK(Readnum(isrear, args[2]), "Invalid isrear flag!");
	DOCHECK(Readnum(iscritical, args[3]), "Invalid iscritical flag!");
	DOCHECK(damage <= 0 || damage > 1000, "Invalid damage!");
	DOCHECK(clustersize <= 0, "Invalid cluster size!");
	DOCHECK(clustersize > damage, "Invalid cluster size! (must be smaller than damage amount, but > 0)");
	DOCHECK(MechType(mech) == CLASS_MW, "No MW killings!");
	Missile_Hit(mech, mech, -1, -1, isrear, iscritical, 0, -1, -1,
				clustersize, damage / clustersize, 1, 0, 0, 0);
}

void mech_damage_section(dbref player, MECH * mech, char *buffer)
{
	char *args[5];
	int damage, isrear, iscritical, section;

/* ARGS: <SECTION> <DAMAGE> <ISREAR> <ISCRITICAL> */

	DOCHECK(mech_parseattributes(buffer, args, 5) != 4, "Invalid Arguments: <SECTION> <DAMAGE> <ISREAR> <ISCRITICAL>");

	section = ArmorSectionFromString(MechType(mech), MechMove(mech), args[0]);

	if(section == -1) {
		invalid_section(player, mech);
		return;
	}
	
	DOCHECK(Readnum(damage, args[1]), "Invalid damage (Arg 2) amount! (Must be a number!)");
	DOCHECK(Readnum(isrear, args[2]), "Isrear value (Arg 3) Invalid! (1 or 0)");
	DOCHECK(Readnum(iscritical, args[3]), "Iscritical value (Arg 4) Invalid! (1 or 0)");
	DOCHECK(damage <= 0 || damage > 1000, "Invalid damage (Arg 2 amount! (Must be >0 or <1000)");
	DamageMech(mech,mech,0, -1, section, isrear, iscritical, damage, 0, 0, 0, -1, 0, 1);

}
