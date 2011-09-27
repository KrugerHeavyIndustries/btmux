/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>

#include "mech.h"
#include "mech.events.h"
#include "failures.h"
#include "coolmenu.h"
#include "create.h"
#include "mycool.h"
#include "mech.partnames.h"
#include "p.mech.los.h"
#include "p.mech.utils.h"
#include "p.mech.scan.h"
#include "p.mech.status.h"
#include "p.mech.build.h"
#include "p.mech.update.h"
#include "p.mech.tech.commands.h"
#include "p.bsuit.h"
#include "p.mech.tech.do.h"
#include "p.mech.combat.h"
#include "p.mech.tag.h"
#include "p.mech.enhanced.criticals.h"
#include "p.mech.contacts.h"
#include "p.btechstats.h"
#include "p.mech.notify.h"
#include "mech.tech.h"

static int doweird = 0;
static char *weirdbuf;

#define PHY_AXE		1
#define PHY_SWORD   2
#define PHY_MACE    3
#define PHY_SAW     4

void DisplayTarget(dbref player, MECH * mech)
{
	int arc;
	MECH *tempMech = NULL;
	char location[50];
	char buff[100], buff1[100];

	if(MechTarget(mech) != -1) {
		tempMech = getMech(MechTarget(mech));
		if(tempMech) {
			if(InLineOfSight(mech, tempMech, MechX(tempMech),
							 MechY(tempMech), FaMechRange(mech, tempMech))) {
				sprintf(buff,
						"Target: %s\t   Range: %.1f hexes   Bearing: %d deg\n",
						GetMechToMechID(mech, tempMech), FaMechRange(mech,
																	 tempMech),
						FindBearing(MechFX(mech), MechFY(mech),
									MechFX(tempMech), MechFY(tempMech)));
				notify(player, buff);
				arc = InWeaponArc(mech, MechFX(tempMech), MechFY(tempMech));
				strcpy(buff, tprintf("Target in %s Weapons Arc",
									 (arc & TURRETARC) ? "Turret" :
									 GetArcID(mech, arc)));
				if(MechAim(mech) == NUM_SECTIONS
				   || MechAimType(mech) != MechType(tempMech))
					strcpy(location, "None");
				else
					ArmorStringFromIndex(MechAim(mech), location,
										 MechType(tempMech),
										 MechMove(tempMech));
				sprintf(buff1, "\t   Aimed Shot Location: %s", location);
				strcat(buff, buff1);
			} else
				sprintf(buff, "Target: NOT in line of sight!\n");
		}
		notify(player, buff);
	} else if(MechTargX(mech) != -1 && MechTargY(mech) != -1) {
		if(MechStatus(mech) & LOCK_BUILDING)
			notify_printf(player, "Target: Building at %d %d\n",
						  MechTargX(mech), MechTargY(mech));
		else if(MechStatus(mech) & LOCK_HEX)
			notify_printf(player, "Target: Hex %d %d\n", MechTargX(mech),
						  MechTargY(mech));
		else
			notify_printf(player, "Target: %d %d\n", MechTargX(mech),
						  MechTargY(mech));

	}
	if(MechPKiller(mech))
		notify(player, "Weapon Safeties are %ch%crOFF%cn.\n");
	if(GotPilot(mech) && HasBoolAdvantage(MechPilot(mech), "maneuvering_ace"))
		notify_printf(player, "Turn Mode: %s",
					  GetTurnMode(mech) ? "TIGHT" : "NORMAL");
	if(MechChargeTarget(mech) > 0 && mudconf.btech_newcharge) {
		tempMech = getMech(MechChargeTarget(mech));
		if(!tempMech)
			return;
		if(InLineOfSight(mech, tempMech, MechX(tempMech), MechY(tempMech),
						 FaMechRange(mech, tempMech))) {
			notify_printf(player, "ChargeTarget: %s\t  ChargeTimer: %d\n",
						  GetMechToMechID(mech, tempMech),
						  MechChargeTimer(mech) / 2);
		} else {
			notify_printf(player,
						  "ChargeTarget: NOT in line of sight!\t Timer: %d\n",
						  MechChargeTimer(mech) / 2);
		}
	}
}

void show_miscbrands(MECH * mech, dbref player)
{
/*   notify(player, tprintf("Radio: %s (%3d range)     Computer: %s (%d Scan / %d LRS / %d Tac)", brands[BOUNDED(1, MechRadio(mech), 5)+RADIO_INDEX].name, (int) MechRadioRange(mech), brands[BOUNDED(1, MechComputer(mech), 5)+COMPUTER_INDEX].name, (int) MechScanRange(mech), (int) MechLRSRange(mech), (int) MechTacRange(mech))); */
}

void PrintGenericStatus(dbref player, MECH * mech, int own, int usex)
{
	MECH *tempMech = NULL;
	MAP *map = FindObjectsData(mech->mapindex);
	char buff[100];
	char mech_name[100];
	char mech_ref[100];
	char move_type[50];

	strcpy(mech_name,
		   usex ? MechType_Name(mech) : silly_atr_get(mech->mynum,
													  A_MECHNAME));
	strcpy(mech_ref,
		   usex ? MechType_Ref(mech) : silly_atr_get(mech->mynum, A_MECHREF));

	switch (MechType(mech)) {
	case CLASS_MW:
		notify_printf(player, "MechWarrior: %-18.18s ID:[%s]",
					  Name(player), MechIDS(mech, 0));
		notify_printf(player, "MaxSpeed: %3d", (int) MMaxSpeed(mech));
		break;
	case CLASS_BSUIT:
		sprintf(buff, "%s Name: %-18.18s  ID:[%s]   %s Reference: %s",
				GetBSuitName(mech), mech_name, MechIDS(mech, 0),
				GetBSuitName(mech), mech_ref);
		notify(player, buff);
		notify_printf(player,
					  "MaxSpeed: %3d                  JumpRange: %d",
					  (int) MMaxSpeed(mech), JumpSpeedMP(mech, map));
		show_miscbrands(mech, player);
		if(MechPilot(mech) == -1)
			notify(player, "Leader: NONE");
		else {
			sprintf(buff, "%s Leader Name: %-16.16s %s Leader injury: %d",
					GetBSuitName(mech), Name(MechPilot(mech)),
					GetBSuitName(mech), MechPilotStatus(mech));
			notify(player, buff);
		}

		sprintf(buff, "Max Suits: %d", MechMaxSuits(mech));
		notify(player, buff);

		Mech_ShowFlags(player, mech, 0, 0);

		if(Jumping(mech)) {
			sprintf(buff, "JUMPING --> %3d,%3d", MechGoingX(mech),
					MechGoingY(mech));
			if((MechStatus(mech) & DFA_ATTACK) && MechDFATarget(mech) != -1) {
				tempMech = getMech(MechDFATarget(mech));
				sprintf(buff + strlen(buff),
						"  Death From Above Target: %s",
						GetMechToMechID(mech, tempMech));
			}
			notify(player, buff);
		}
		break;
	case CLASS_MECH:
		sprintf(buff, "Mech Name: %-18.18s  ID:[%s]   Mech Reference: %s",
				mech_name, MechIDS(mech, 0), mech_ref);
		notify(player, buff);
		notify_printf(player,
					  "Tonnage:   %3d     MaxSpeed: %3d       JumpRange: %d",
					  MechTons(mech), (int) MMaxSpeed(mech), JumpSpeedMP(mech,
																		 map));
		show_miscbrands(mech, player);
		if(MechPilot(mech) == -1)
			notify(player, "Pilot: NONE");
		else {
			sprintf(buff, "Pilot Name: %-28.28s Pilot Injury: %d",
					Name(MechPilot(mech)), MechPilotStatus(mech));
			notify(player, buff);
		}
		Mech_ShowFlags(player, mech, 0, 0);
		if(!Jumping(mech) && !Fallen(mech) && Started(mech) &&
		   (MechChargeTarget(mech) != -1)) {
			tempMech = getMech(MechChargeTarget(mech));
			if(tempMech) {
				sprintf(buff, "CHARGING --> %s", GetMechToMechID(mech,
																 tempMech));
				notify(player, buff);
			}
		}
		if(Jumping(mech)) {
			sprintf(buff, "JUMPING --> %3d,%3d", MechGoingX(mech),
					MechGoingY(mech));
			if((MechStatus(mech) & DFA_ATTACK) && MechDFATarget(mech) != -1) {
				tempMech = getMech(MechDFATarget(mech));
				sprintf(buff + strlen(buff),
						"  Death From Above Target: %s",
						GetMechToMechID(mech, tempMech));
			}
			notify(player, buff);
		}
		break;
	case CLASS_VTOL:
	case CLASS_VEH_GROUND:
	case CLASS_VEH_NAVAL:
	case CLASS_AERO:
	case CLASS_DS:
	case CLASS_SPHEROID_DS:
		switch (MechMove(mech)) {
		case MOVE_TRACK:
			strcpy(move_type, "Tracked");
			break;
		case MOVE_WHEEL:
			strcpy(move_type, "Wheeled");
			break;
		case MOVE_HOVER:
			strcpy(move_type, "Hover");
			break;
		case MOVE_VTOL:
			strcpy(move_type, "VTOL");
			break;
		case MOVE_FLY:
			strcpy(move_type, "Flight");
			break;
		case MOVE_HULL:
			strcpy(move_type, "Displacement Hull");
			break;
		case MOVE_SUB:
			strcpy(move_type, "Submarine");
			break;
		case MOVE_FOIL:
			strcpy(move_type, "Hydrofoil");
			break;
		default:
			strcpy(move_type, "Magic");
			break;
		}
		if(MechMove(mech) != MOVE_NONE) {
			sprintf(buff,
					"Vehicle Name: %-15.15s  ID:[%s]   Vehicle Reference: %s",
					mech_name, MechIDS(mech, 0), mech_ref);
			notify(player, buff);
			sprintf(buff,
					"Tonnage:   %3d      %s: %3d       Movement Type: %s",
					MechTons(mech),
					is_aero(mech) ? "Max thrust" : "FlankSpeed",
					(int) MMaxSpeed(mech), move_type);
			notify(player, buff);
			show_miscbrands(mech, player);
			if(MechPilot(mech) == -1)
				notify(player, "Pilot: NONE");
			else {
				sprintf(buff, "Pilot Name: %-28.28s Pilot Injury: %d",
						Name(MechPilot(mech)), MechPilotStatus(mech));
				notify(player, buff);
			}
		} else {
			sprintf(buff, "Name: %-15.15s  ID:[%s]   Reference: %s",
					mech_name, MechIDS(mech, 0), mech_ref);
			notify(player, buff);
		}
		if(MechType(mech) != CLASS_VTOL && !is_aero(mech))
			if(GetSectInt(mech, TURRET)) {
				if(MechTankCritStatus(mech) & TURRET_JAMMED)
					notify(player, "     TURRET JAMMED");
				else if(MechTankCritStatus(mech) & TURRET_LOCKED)
					notify(player, "     TURRET LOCKED");
			}
		if(FlyingT(mech) && Landed(mech))
			notify(player, "LANDED");
		Mech_ShowFlags(player, mech, 0, 0);
	}
}

void PrintShortInfo(dbref player, MECH * mech)
{
	char buff[100];
	char typespecific[50];

	switch (MechType(mech)) {
	case CLASS_VTOL:
		sprintf(typespecific, " VSPD: %3.1f ", MechVerticalSpeed(mech));
		break;
	case CLASS_MECH:
		sprintf(typespecific, " HT: %3d/%3d/%-3d ",
				(int) (10. * MechPlusHeat(mech)),
				(int) (10. * MechActiveNumsinks(mech)), (int) (10. * MechMinusHeat(mech)));
		break;
	case CLASS_AERO:
	case CLASS_DS:
	case CLASS_SPHEROID_DS:
		sprintf(typespecific, " VSPD: %3.1f  ANG: %2d  HT: %3d/%3d ",
				MechVerticalSpeed(mech), MechDesiredAngle(mech),
				(int) (10 * MechPlusHeat(mech)),
				(int) (10 * MechActiveNumsinks(mech)));
		break;
	case CLASS_VEH_NAVAL:
		if(MechMove(mech) == MOVE_FOIL)
			sprintf(typespecific, " VSPD: %3.1f ", MechVerticalSpeed(mech));
		/* FALLTHROUGH */
	case CLASS_VEH_GROUND:
		/* XXX This won't work for subs with turrets.. are they possible ? */
		if(GetSectOInt(mech, TURRET)) {
			sprintf(typespecific, " TUR: %3d ",
					AcceptableDegree(MechTurretFacing(mech) +
									 MechFacing(mech)));
			break;
		}
		/* FALLTHROUGH */
	default:
		typespecific[0] = '\0';
		break;
	}

	snprintf(buff, 100,
			 "LOC: %3d,%3d,%3d  HD: %3d/%3d  SP: %3.1f/%3.1f %s ST:%s",
			 MechX(mech), MechY(mech), MechZ(mech), MechFacing(mech),
			 MechDesiredFacing(mech), MechSpeed(mech), MechDesiredSpeed(mech),
			 typespecific, getStatusString(mech, 2));
	buff[99] = '\0';
	notify(player, buff);
	DisplayTarget(player, mech);
}

#define HEAT_LEVEL_LGREEN 0
#define HEAT_LEVEL_BGREEN 7
#define HEAT_LEVEL_LYELLOW 13
#define HEAT_LEVEL_BYELLOW 16
#define HEAT_LEVEL_LRED 18
#define HEAT_LEVEL_BRED 24
#define HEAT_LEVEL_TOP 40

#define HEAT_LEVEL_NONE 27

static char *MakeHeatScaleInfo(MECH * mech, char *fillchar, char *heatstr,
							   int length)
{
	int counter = 0, heat = MechPlusHeat(mech), minheat =
		MechMinusHeat(mech), start = 0;
	char state = 1;

	memset(heatstr, 0, sizeof(char) * length);

	strcat(heatstr, "%cx%ch");

	if(minheat > HEAT_LEVEL_NONE)
		start = minheat - HEAT_LEVEL_NONE;

	if(heat <= start) {
		heat = 0;
		state = 0;
	} else
		heat -= start;

	if(start)
		strcat(heatstr, "<%cx%ch");
	else
		strcat(heatstr, " %cx%ch");

	for(counter = start; counter < minheat; counter++) {
		strncat(heatstr, &fillchar[(short) state], 1);
		if(heat && !--heat)
			state = 0;
	}
	if(state)
		state++;

	strcat(heatstr, "%cg%ch|%c%cg");
	for(; counter < minheat + HEAT_LEVEL_BGREEN; counter++) {
		strncat(heatstr, &fillchar[(short) state], 1);
		if(heat && !--heat)
			state = 0;
	}
	if(state)
		state++;

	strcat(heatstr, "%ch");
	for(; counter < minheat + HEAT_LEVEL_LYELLOW; counter++) {
		strncat(heatstr, &fillchar[(short) state], 1);
		if(heat && !--heat)
			state = 0;
	}
	if(state)
		state++;

	strcat(heatstr, "%c%cy%ch|%c%cy");
	for(; counter < minheat + HEAT_LEVEL_BYELLOW; counter++) {
		strncat(heatstr, &fillchar[(short) state], 1);
		if(heat && !--heat)
			state = 0;
	}
	if(state)
		state++;

	strcat(heatstr, "%ch");
	for(; counter < minheat + HEAT_LEVEL_LRED; counter++) {
		strncat(heatstr, &fillchar[(short) state], 1);
		if(heat && !--heat)
			state = 0;
	}
	if(state)
		state++;

	strcat(heatstr, "%c%cr%ch|%c%cr");
	for(; counter < minheat + HEAT_LEVEL_BRED; counter++) {
		strncat(heatstr, &fillchar[(short) state], 1);
		if(heat && !--heat)
			state = 0;
	}
	if(state)
		state++;

	strcat(heatstr, "%ch");
	for(; counter < minheat + HEAT_LEVEL_TOP; counter++) {
		strncat(heatstr, &fillchar[(short) state], 1);
		if(heat && !--heat)
			state = 0;
	}
	strcat(heatstr, "%cw%ch|%c");
	return heatstr;
}

void PrintHeatBar(dbref player, MECH * mech)
{
	char subbuff[256];
	char buff[256];
	char heatstr[9] = ".:::::::";

	MakeHeatScaleInfo(mech, heatstr, subbuff, 256);
	snprintf(buff, 256, "Temp:%s", subbuff);
	notify(player, buff);
}

void PrintInfoStatus(dbref player, MECH * mech, int own)
{
	char buff[256];
	MECH *tempMech;
	int f;

	switch (MechType(mech)) {
	case CLASS_MECH:
		snprintf(buff, 256,
				 "X, Y, Z:%3d,%3d,%3d  Excess Heat:  %3d deg C.  Heat Production:  %3d deg C.",
				 MechX(mech), MechY(mech), MechZ(mech),
				 (int) (10. * MechHeat(mech)),
				 (int) (10. * MechPlusHeat(mech)));
		notify(player, buff);
		snprintf(buff, 256,
				 "Speed:      %%ch%%cg%3d%%cn KPH  Heading:      %%ch%%cg%3d%%cn deg     Heat Sinks:       %3d",
				 (int) (MechSpeed(mech)), MechFacing(mech),
				 MechActiveNumsinks(mech));
		notify(player, buff);
		sprintf(buff,
				"Des. Speed: %3d KPH  Des. Heading: %3d deg     Heat Dissipation: %3d deg C.",
				(int) MechDesiredSpeed(mech), MechDesiredFacing(mech),
				(int) (10. * MechMinusHeat(mech)));
		notify(player, buff);
		
		if(MechLateral(mech))
			notify_printf(player, "You are moving laterally %s",
						  LateralDesc(mech));
		break;
	case CLASS_VEH_GROUND:
	case CLASS_VEH_NAVAL:
	case CLASS_VTOL:
	case CLASS_AERO:
	case CLASS_DS:
	case CLASS_SPHEROID_DS:
		snprintf(buff, 256,
				 "X, Y, Z:%3d,%3d,%3d  Heat Sinks:          %3d       %s",
				 MechX(mech), MechY(mech), MechZ(mech),
				 MechActiveNumsinks(mech),
				 is_aero(mech) ? tprintf("%s angle: %%ch%%cg%d%%cn",
										 MechDesiredAngle(mech) >=
										 0 ? "Climbing" : "Diving",
										 abs(MechDesiredAngle(mech))) : "");
		notify(player, buff);
		if(FlyingT(mech) || MechMove(mech) == MOVE_SUB) {
			sprintf(buff,
					"Speed:      %%ch%%cg%3d%%cn KPH  Vertical Speed:      %%ch%%cg%3d%%cn KPH   Des. Speed %3d KPH",
					(int) (MechSpeed(mech)), (int) (MechVerticalSpeed(mech)),
					(int) (MechDesiredSpeed(mech)));
			notify(player, buff);
			f = MAX(0, AeroFuel(mech));
			if(MechMove(mech) == MOVE_SUB) {
				sprintf(buff, "Heading: %3d KPH  Des. Heading: %3d deg",
						(int) MechFacing(mech), MechDesiredFacing(mech));
			} else if(AeroFreeFuel(mech)) {
				sprintf(buff,
						"Heading:    %%ch%%cg%3d%%cn deg  Des. Heading:        %3d deg   Fuel: Unlimited",
						MechFacing(mech), MechDesiredFacing(mech));
			} else {
				sprintf(buff,
						"Heading:    %%ch%%cg%3d%%cn deg  Des. Heading:        %3d deg   Fuel: %d (%.2f %%)",
						MechFacing(mech), MechDesiredFacing(mech), f,
						100.0 * f / AeroFuelOrig(mech));
			}

			notify(player, buff);
		} else if(MechMove(mech) != MOVE_NONE) {
			sprintf(buff,
					"Speed:      %%ch%%cg%3d%%cn KPH  Heading:      %%ch%%cg%3d%%cn deg",
					(int) (MechSpeed(mech)), MechFacing(mech));
			notify(player, buff);
			sprintf(buff, "Des. Speed: %3d KPH  Des. Heading: %3d deg",
					(int) MechDesiredSpeed(mech), MechDesiredFacing(mech));
			notify(player, buff);

		}
		ShowTurretFacing(player, 0, mech);
		if(MechHasHeat(mech)) {
			notify_printf(player, "Excess Heat:%3d deg  Heat Production:     %3d deg   Heat Dissipation: %3d deg",
				(int) (10. * MechHeat(mech)), (int) (10. * MechPlusHeat(mech)),
				(int) (10. * MechMinusHeat(mech)));
		}
		break;
	case CLASS_MW:
	case CLASS_BSUIT:
		sprintf(buff,
				"X, Y, Z:%3d,%3d,%3d  Speed:      %%ch%%cg%3d%%cn KPH  Heading:      %%ch%%cg%3d%%cn deg",
				MechX(mech), MechY(mech), MechZ(mech),
				(int) (MechSpeed(mech)), MechFacing(mech));
		notify(player, buff);
		sprintf(buff,
				"                     Des. Speed: %3d KPH  Des. Heading: %3d deg",
				(int) MechDesiredSpeed(mech), MechDesiredFacing(mech));
		notify(player, buff);
		break;
	}

	if(MechHasHeat(mech)) {
		PrintHeatBar(player, mech);

		// Little extra space to preserve formatting.
//		if(MechTarget(mech) == -1 && MechTargX(mech) == -1)
//			notify(player, "  ");
	}
	notify(player, "  ");
	// Show our locked target info (hex or unit).	
	DisplayTarget(player, mech);

	if(MechCarrying(mech) > 0)
		if((tempMech = getMech(MechCarrying(mech))))
			notify_printf(player, "Towing %s.", GetMechToMechID(mech,
																tempMech));
}

/* Status commands! */
void mech_status(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	int doweap = 0, doinfo = 0, doarmor = 0, doshort = 0, doheat = 0, loop;
	int i;
	int usex = 0;
	char buf[LBUF_SIZE];
	char subbuff[256];

	doweird = 0;
	cch(MECH_USUALSM);
	if(!buffer || !strlen(buffer))
		// No arguments, we'll go with our default 'status' output.
		doweap = doinfo = doarmor = doheat = 1;
	else {
		// Argument provided, only show certain parts.
		for(loop = 0; buffer[loop]; loop++) {
			switch (toupper(buffer[loop])) {
			case 'R':
				doweap = doinfo = doarmor = doheat = usex = 1;
				break;
			case 'A':
				// Armor status
				if(toupper(buffer[loop + 1]) == 'R')
					while (buffer[loop + 1] && buffer[loop + 1] != ' ')
						loop++;
				doarmor = 1;
				break;
			case 'I':
				// Speed/Heading/Heat
				doinfo = 1;
				if(toupper(buffer[loop + 1]) == 'N')
					while (buffer[loop + 1] && buffer[loop + 1] != ' ')
						loop++;
				break;
			case 'W':
				// Weapons list.
				doweap = 1;
				if(toupper(buffer[loop + 1]) == 'E')
					while (buffer[loop + 1] && buffer[loop + 1] != ' ')
						loop++;
				break;
			case 'N':
				// Really weird status display.
				doweird = 1;
				break;
			case 'S':
				// Very short one-line status.
				doshort = 1;
				break;
			case 'H':
				// Just the heat bar.
				doheat = 1;
				break;
			}
		}
	}

	// Very short one-line status.
	if(doshort) {
		PrintShortInfo(player, mech);
		return;
	}

	// Really weird status display.
	if(doweird) {
		sprintf(buf, "%s %s %d %d/%d/%d %d ", MechType_Ref(mech),
				MechType_Name(mech), MechTons(mech),
				(int) (MechMaxSpeed(mech) / MP1) * 2 / 3,
				(int) (MechMaxSpeed(mech) / MP1),
				(int) (MechJumpSpeed(mech) / MP1), MechActiveNumsinks(mech));
		weirdbuf = buf;

	} else if(!doheat || (doarmor | doinfo | doweap))
		PrintGenericStatus(player, mech, 1, usex);

	// Show our armor diagram.
	if(doarmor) {
		if(!doweird) {
			PrintArmorStatus(player, mech, 1);
			notify(player, " ");
		} else {
			for(i = 0; i < NUM_SECTIONS; i++)
				if(GetSectOArmor(mech, i)) {
					if(GetSectORArmor(mech, i))
						sprintf(buf + strlen(buf), "%d|%d|%d ",
								GetSectOArmor(mech, i), GetSectOInt(mech, i),
								GetSectORArmor(mech, i));
					else
						sprintf(buf + strlen(buf), "%d|%d ",
								GetSectOArmor(mech, i), GetSectOInt(mech, i));
				}
		}
	}

	// Standard heat/heading/dive/etc.
	if(doinfo && !doweird) {
		PrintInfoStatus(player, mech, 1);
		//notify(player, " ");
	}

	// Show our heat bar by itself.
	if(!doinfo && doheat && MechHasHeat(mech)) { 
		PrintHeatBar(player, mech);
	}

	// Weapons readout.
	if(doweap)
		PrintWeaponStatus(mech, player);

	// Really strange, short status info.
	if(doweird)
		notify(player, buf);
}

void mech_critstatus(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	char *args[1];
	int index;

	cch(MECH_USUALSM);
	if(!CheckData(player, mech))
		return;
	DOCHECK(MechType(mech) == CLASS_MW, "Huh?");
	DOCHECK(mech_parseattributes(buffer, args, 1) != 1,
			"You must specify a section to list the criticals for!");
	index = ArmorSectionFromString(MechType(mech), MechMove(mech), args[0]);
	DOCHECK(index == -1, "Invalid section!");
	DOCHECK(!GetSectOInt(mech, index), "Invalid section!");
	CriticalStatus(player, mech, index);
}

static int wspec_weaps[MAX_WEAPONS_PER_MECH];
static int wspec_weapcount;

char *part_name(int type, int brand)
{
	char *c;
	static char buffer[SBUF_SIZE];

	if(type == EMPTY)
		return "Empty";
	c = get_parts_long_name(type, brand);
	if(!c)
		return NULL;
	strcpy(buffer, c);
	if(!strcmp(c, "LifeSupport"))
		strcpy(buffer, "Life Support");
	else if(!strcmp(c, "TripleStrengthMyomer"))
		strcpy(buffer, "Triple Strength Myomer");
	else
		strcpy(buffer, c);
	if((c = strstr(buffer, "Actuator")))
		if(c != buffer)
			strcpy(c, " Actuator");
	while ((c = strchr(buffer, '_')))
		*c = ' ';
	while ((c = strchr(buffer, '.')))
		*c = ' ';
	return buffer;
}

char *part_name_long(int type, int brand)
{
	char *c;
	static char buffer[SBUF_SIZE];

	if(type == EMPTY)
		return "Empty";
	c = get_parts_vlong_name(type, brand);
	if(!c)
		return NULL;
	strcpy(buffer, c);
	if(!strcmp(c, "LifeSupport"))
		strcpy(buffer, "Life Support");
	else if(!strcmp(c, "TripleStrengthMyomer"))
		strcpy(buffer, "Triple Strength Myomer");
	else
		strcpy(buffer, c);
	if((c = strstr(buffer, "Actuator")))
		if(c != buffer)
			strcpy(c, " Actuator");
	while ((c = strchr(buffer, '_')))
		*c = ' ';
	while ((c = strchr(buffer, '.')))
		*c = ' ';
	return buffer;
}

char *pos_part_name(MECH * mech, int index, int loop)
{
	int t, b;
	char *c;
	int newloop, newindex;

	if(index < 0 || index >= NUM_SECTIONS || loop < 0 ||
	   loop >= NUM_CRITICALS) {
		SendError(tprintf("INVALID: For mech #%d, %d/%d was requested.",
						  mech->mynum, index, loop));
		return "--?LocationBug?--";
	}
	t = GetPartType(mech, index, loop);
	b = GetPartBrand(mech, index, loop);
	if(t == Special(HAND_OR_FOOT_ACTUATOR)) {
		if(index == LLEG || index == RLEG || MechIsQuad(mech))
			return "Foot Actuator";
		return "Hand Actuator";
	}
	if(t == Special(SHOULDER_OR_HIP)) {
		if(index == LLEG || index == RLEG || MechIsQuad(mech))
			return "Hip";
		return "Shoulder";
	}

	if (t == Special(HEAT_SINK) ) {
		return (MechSpecials(mech) & DOUBLE_HEAT_TECH ? "Double Heatsink" :
			"Heatsink");
	}

        if (t == Special(SPLIT_CRIT_RIGHT) || t == Special(SPLIT_CRIT_LEFT)) { // we got a split crit
	                newindex = ReverseSplitCritLoc(mech, index, loop);
	                newloop = GetPartData(mech, index, loop);
	                if (newindex >= 0) {
	                        t = GetPartType(mech, newindex, newloop);
	                        b = GetPartBrand(mech, newindex, newloop);
	                }
	}

	/* LETS CHECK IF ITS A SPECIAL ENGINE */
	if (t == Special(ENGINE) ) {
		return (MechSpecials(mech) & LE_TECH ? "Engine (Light)" :
			MechSpecials(mech) & CE_TECH ? "Engine (Compact)" :
			MechSpecials(mech) & XXL_TECH ? "Engine (XXL)" :
			MechSpecials(mech) & XL_TECH ? "Engine (XL)" :
			"Engine");
	}

	if(!(c = part_name(t, b)))
		return "--?ErrorInTemplate?--";

	return c;

}

static char *wspec_fun(int i)
{
	static char buf[MBUF_SIZE];
	int j;

	buf[0] = 0;
	if(!i)
		if(mudconf.btech_erange)
			sprintf(buf, WSDUMP_MASKS_ER);
		else
			sprintf(buf, WSDUMP_MASKS_NOER);
	else {
		i--;
		j = wspec_weaps[i];
		if(mudconf.btech_erange)
			sprintf(buf, WSDUMP_MASK_ER, MechWeapons[j].name,
					MechWeapons[j].heat, MechWeapons[j].damage,
					MechWeapons[j].min, MechWeapons[j].shortrange,
					MechWeapons[j].medrange, GunRange(j), EGunRange(j),
					MechWeapons[j].vrt);
		else
			sprintf(buf, WSDUMP_MASK_NOER, MechWeapons[j].name,
					MechWeapons[j].heat, MechWeapons[j].damage,
					MechWeapons[j].min, MechWeapons[j].shortrange,
					MechWeapons[j].medrange, GunRange(j), MechWeapons[j].vrt);
	}
	return buf;
}

void mech_weaponspecs(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	int loop;
	unsigned char weaparray[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	int critical[MAX_WEAPS_SECTION];

/*   unsigned char weaps[8 * MAX_WEAPS_SECTION]; */
	int num_weaps;
	int index;
	int duplicate, ii;
	coolmenu *c;

	wspec_weapcount = 0;
	if(!CheckData(player, mech))
		return;
	for(loop = 0; loop < NUM_SECTIONS; loop++) {
		num_weaps = FindWeapons(mech, loop, weaparray, weapdata, critical);
		for(index = 0; index < num_weaps; index++) {
			duplicate = 0;
			for(ii = 0; ii < wspec_weapcount; ii++)
				if(weaparray[index] == wspec_weaps[ii])
					duplicate = 1;
			if(!duplicate && wspec_weapcount < MAX_WEAPONS_PER_MECH)
				wspec_weaps[wspec_weapcount++] = weaparray[index];
		}
	}
	DOCHECK(!wspec_weapcount, "You have no weapons!");
	if(strcmp(MechType_Name(mech), MechType_Ref(mech)))
		c = SelCol_FunStringMenuK(1,
								  tprintf("Weapons statistics for %s: %s",
										  MechType_Name(mech),
										  MechType_Ref(mech)), wspec_fun,
								  wspec_weapcount + 1);
	else
		c = SelCol_FunStringMenuK(1, tprintf("Weapons statistics for %s",
											 MechType_Ref(mech)), wspec_fun,
								  wspec_weapcount + 1);
	ShowCoolMenu(player, c);
	KillCoolMenu(c);
}

char *sectstatus_func(MECH * mech, char *arg)
{
/* Show if Section is destroyed or not
 * -1 = Section Flooded
 * 1 = Section Exists
 * 0 = Section Destroyed
*/
	static char buffer[MBUF_SIZE];
	int index;

	if(!arg || !*arg)
		return "#-1 INVALID SECTION";

	index = ArmorSectionFromString(MechType(mech), MechMove(mech), arg);
	if(index == -1)
		return "#-1 INVALID SECTION";
	
	sprintf(buffer, "%d", SectIsFlooded(mech,index) ? -1 : !(SectIsDestroyed(mech,index)));

	return buffer;
}

char *critstatus_func(MECH * mech, char *arg)
{
	static char buffer[MBUF_SIZE];
	char *tmp;
	int index, i, max_crits;
	int type;

	if(!arg || !*arg)
		return "#-1 INVALID SECTION";

	index = ArmorSectionFromString(MechType(mech), MechMove(mech), arg);
	if(index == -1 || !GetSectOInt(mech, index))
		return "#-1 INVALID SECTION";

	buffer[0] = '\0';
	max_crits = CritsInLoc(mech, index);
	for(i = 0; i < max_crits; i++) {
		if(buffer[0])
			sprintf(buffer, "%s,", buffer);
		sprintf(buffer, "%s%d|", buffer, i + 1);
		type = GetPartType(mech, index, i);
		if(IsAmmo(type))
			type = FindAmmoType(mech, index, i);
		tmp = get_parts_long_name(type, GetPartBrand(mech, index, i));
		sprintf(buffer, "%s|%s", buffer, tmp ? tmp : "Empty");
		sprintf(buffer, "%s|%d", buffer, (PartIsNonfunctional(mech, index,
															  i)
										  && type != EMPTY && (!IsCrap(type)
															   ||
															   SectIsDestroyed
															   (mech,
																index))) ? -1
				: PartTempNuke(mech, index, i));
		sprintf(buffer, "%s|%d", buffer,
				IsWeapon(type) ? 1 : IsAmmo(type) ? 2 : IsActuator(type) ? 3 :
				IsCargo(type) ? 4 : (IsCrap(type) || type == EMPTY) ? 5 : 0);
	}
	return buffer;
}

char *armorstatus_func(MECH * mech, char *arg)
{
	static char buffer[MBUF_SIZE];
	char **locs;
	int index;
	int iter, curarm, curint, totarm, totint;

	if(!arg || !*arg)
		return "#-1 INVALID SECTION";

	if(strcmp(arg, "all") == 0) {
		locs = ProperSectionStringFromType(MechType(mech), MechMove(mech));
		curarm = totarm = curint = totint = 0;
		for(iter = 0; locs[iter]; iter++) {
			curarm += GetSectArmor(mech, iter) + GetSectRArmor(mech, iter);
			totarm += GetSectOArmor(mech, iter) + GetSectORArmor(mech, iter);
			curint += GetSectInt(mech, iter);
			totint += GetSectOInt(mech, iter);
		}
		buffer[0] = '\0';
		snprintf(buffer, MBUF_SIZE, "%d/%d|%d/%d", curarm, totarm, curint,
				 totint);
		return buffer;
	}

	index = ArmorSectionFromString(MechType(mech), MechMove(mech), arg);
	if(index == -1 || !GetSectOInt(mech, index))
		return "#-1 INVALID SECTION";

	buffer[0] = '\0';
	snprintf(buffer, MBUF_SIZE, "%d/%d|%d/%d|%d/%d",
			 GetSectArmor(mech, index), GetSectOArmor(mech, index),
			 GetSectInt(mech, index), GetSectOInt(mech, index),
			 GetSectRArmor(mech, index), GetSectORArmor(mech, index));
	return buffer;
}

/* weaponstatus_func. Returns a string containing:
   
   <weapon number> | <weapon (long) name> | <number of crits> |
   	<part quality> | <weapon recycle time> | <recycle time left> |
   	<weapon type> | <weapon status>
   [ , <next weapon> ]
   	
   Weapon number is the number of the weapon in this particular 'mech.
   Long weapon name is 'agra.mediumlaser' and such.
   Weapon type is as defined in mech.h:
	   #define TBEAM      0
	   #define TMISSILE   1
	   #define TARTILLERY 2
	   #define TAMMO      3
	   #define THAND      4
   Weapon status is:
   	0 - weapon operational
   	1 - weapon (temporarily) glitched
   	2 - weapon destroyed/flooded
*/

char *weaponstatus_func(MECH * mech, char *arg)
{
   static char buffer[MBUF_SIZE];
   int count, sect, loopsect, i, type, totalcount = 0;
   unsigned char weaparray[MAX_WEAPS_SECTION];
   unsigned char weapdata[MAX_WEAPS_SECTION];
   int criticals[MAX_WEAPS_SECTION];

   if(!arg)
      sect = -1;
   else if(!*arg)
      return "#-1 INVALID SECTION";
   else if((sect = ArmorSectionFromString(MechType(mech),
      MechMove(mech), arg)) == -1 || !GetSectOInt(mech, sect))
      return "#-1 INVALID SECTION";

   buffer[0] = '\0';
   for((sect == -1) ? (loopsect = 0) : (loopsect = sect);
      (sect == -1) ? (loopsect < NUM_SECTIONS) : (loopsect < sect + 1);
      loopsect++) {
      count = FindWeapons(mech, loopsect, weaparray, weapdata, criticals);
      for(i = 0; i < count; i++, totalcount++) {
            if(buffer[0])
               sprintf(buffer, "%s,", buffer);
               type = Weapon2I(GetPartType(mech, loopsect, criticals[i]));
               sprintf(buffer, "%s%d|%s|%d|%d|%d|%d|%d|%d", buffer,
                  totalcount, get_parts_long_name(I2Weapon(type),
                  GetPartBrand(mech, loopsect, criticals[i])),
                  GetWeaponCrits(mech, type), GetPartBrand(mech, loopsect,
                  criticals[i]), MechWeapons[type].vrt, weapdata[i],
                  MechWeapons[type].type, PartIsNonfunctional(mech, loopsect,
                  criticals[i]) ? 2 : PartTempNuke(mech, loopsect,
                  criticals[i]) ? 1 : 0);
      }
   }
	return buffer;
}

char *critslot_func(MECH * mech, char *buf_section, char *buf_critnum,
					char *buf_flag)
{
	int index, crit, flag, type;
	static char buffer[MBUF_SIZE];

	index =
		ArmorSectionFromString(MechType(mech), MechMove(mech), buf_section);
	if(index == -1)
		return "#-1 INVALID SECTION";
	if(!GetSectOInt(mech, index))
		return "#-1 INVALID SECTION";
	crit = atoi(buf_critnum);
	if(crit < 1 || crit > CritsInLoc(mech, index))
		return "#-1 INVALID CRITICAL";
	crit--;
	if(!buf_flag)
		flag = 0;
	else if(strcasecmp(buf_flag, "NAME") == 0)
		flag = 0;
	else if(strcasecmp(buf_flag, "STATUS") == 0)
		flag = 1;
	else if(strcasecmp(buf_flag, "DATA") == 0)
		flag = 2;
	else if(strcasecmp(buf_flag, "MAXAMMO") == 0)
		flag = 3;
	else if(strcasecmp(buf_flag, "AMMOTYPE") == 0)
		flag = 4;
	else if(strcasecmp(buf_flag, "MODE") == 0)
		flag = 5;
	else if(strcasecmp(buf_flag, "HALFTON") == 0)
		flag = 6;
	else
		flag = 0;

	type = GetPartType(mech, index, crit);

	if(flag == 1) {
		if(PartIsDisabled(mech, index, crit))
			return "Disabled";
		if(PartIsDestroyed(mech, index, crit))
			return "Destroyed";
		return "Operational";
	} else if(flag == 2) {
		snprintf(buffer, MBUF_SIZE, "%d", GetPartData(mech, index, crit));
		return buffer;
	} else if(flag == 3) {
		if(!IsAmmo(type))
			return "#-1 NOT AMMO";
		snprintf(buffer, MBUF_SIZE, "%d", FullAmmo(mech, index, crit));
		return buffer;
	} else if(flag == 4) {
		if(!IsAmmo(type))
			return "#-1 NOT AMMO";
		type = FindAmmoType(mech, index, crit);
	} else if(flag == 5) {
		int weapindex;
		if(!IsWeapon(type))
			return "#-1 NOT AMMO OR WEAPON";
		else {
			weapindex = Weapon2I(type);
			snprintf(buffer, MBUF_SIZE, "%c%c",
					 GetWeaponFireModeLetter_Model_Mode(weapindex,
														GetPartFireMode(mech,
																		index,
																		crit)),
					 GetWeaponAmmoModeLetter_Model_Mode(weapindex,
														GetPartAmmoMode(mech,
																		index,
																		crit)));
			return buffer;
		}
	} else if(flag == 6) {
		if(!IsAmmo(type))
			return "#-1 NOT AMMO";
		snprintf(buffer, MBUF_SIZE, "%d", GetPartFireMode(mech, index, crit) & HALFTON_MODE ? 1 : 0);
		return buffer;

	}

	if(type == EMPTY || IsCrap(type))
		return "Empty";
	if(flag == 0)
#ifndef BT_COMPLEXREPAIRS
		type = alias_part(mech, type);
#else
		type = alias_part(mech, type, index);
#endif
	snprintf(buffer, MBUF_SIZE, "%s",
			 get_parts_vlong_name(type, GetPartBrand(mech, index, crit)));
	return buffer;
}

void CriticalStatus(dbref player, MECH * mech, int index)
{
	int loop, i;
	char buffer[LBUF_SIZE];
	int type, data, wFireMode;
	int max_crits = CritsInLoc(mech, index);
	char **foo;
	int count = 0;
	coolmenu *cm;

	Create(foo, char *, NUM_CRITICALS + 1);

	for(i = 0; i < max_crits; i++) {
		loop = ((i % 2) ? (max_crits / 2) : 0) + i / 2;
		sprintf(buffer, "%2d ", loop + 1);
		type = GetPartType(mech, index, loop);
		data = GetPartData(mech, index, loop);
		wFireMode = GetPartFireMode(mech, index, loop);
		if(IsAmmo(type)) {
			char trash[50];

			strcat(buffer, &MechWeapons[Ammo2WeaponI(type)].name[3]);
			strcat(buffer, GetAmmoDesc_Model_Mode(Ammo2WeaponI(type),
												  GetPartAmmoMode(mech, index,
																  loop)));
			strcat(buffer, " Ammo");
		        if(!PartIsNonfunctional(mech, index, loop)) {
                                sprintf(trash, " [%3.3d/%3.3d]", data, FullAmmo(mech,index,loop));
                                strcat(buffer, trash);
                        }


		} else {
			if(IsWeapon(type) && (wFireMode & OS_MODE))
				strcat(buffer, "OS ");
			strcat(buffer, pos_part_name(mech, index, loop));
			if(IsWeapon(type) && (((wFireMode & OS_MODE) &&
								   (wFireMode & OS_USED)) ||
								  (wFireMode & ROCKET_FIRED)))
				strcat(buffer, " (Empty)");
				if(wFireMode & WILL_JETTISON_MODE)
					strcat(buffer, " (backpack)");

			if(!PartIsNonfunctional(mech, index, loop)) {
				if(Special2I(type) == ARTEMIS_IV) {
					char trash[50];
					if(data) {
						sprintf(trash, " [Controls Slot %d]", data);
						strcat(buffer, trash);
					}
				}
			}
		}

		if(PartIsBroken(mech, index, loop) && type != EMPTY &&
		   (!IsCrap(type) || SectIsDestroyed(mech, index)))
			strcat(buffer, PartIsDestroyed(mech, index, loop) ?
				   " (Destroyed)" : " (Broken)");
		else if(PartIsDisabled(mech, index, loop) && type != EMPTY)
			strcat(buffer, " (Disabled)");
		else if(PartIsDamaged(mech, index, loop) && type != EMPTY)
			strcat(buffer, " (Damaged)");

		foo[count++] = strdup(buffer);
	}

	ArmorStringFromIndex(index, buffer, MechType(mech), MechMove(mech));
	strcat(buffer, " Criticals");
	cm = SelCol_StringMenu(2, buffer, foo);
	ShowCoolMenu(player, cm);
	KillCoolMenu(cm);
	KillText(foo);
}

char *evaluate_ammo_amount(int now, int max)
{
	int f = (now * 100) / max;

	if(f >= 50)
		return "%ch%cg";
	if(f >= 25)
		return "%ch%cy";
	return "%ch%cr";
}

void PrintWeaponStatus(MECH * mech, dbref player)
{
	unsigned char weaparray[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	int critical[MAX_WEAPS_SECTION];
	unsigned char ammoweap[8 * MAX_WEAPS_SECTION];
	unsigned short ammo[8 * MAX_WEAPS_SECTION];
	unsigned short ammomax[8 * MAX_WEAPS_SECTION];
	unsigned int modearray[8 * MAX_WEAPS_SECTION];
	char tmpbuf[LBUF_SIZE];
	int count, ammoweapcount;
	int loop;
	int ii, i = 0;
	char weapname[80], *tmpc;
	char weapbuff[120];
	char tempbuff[160];
	char location[80];
	char astrAmmoSpacer[MBUF_SIZE]; /* mem is cheap. over allocate */
	int running_sum = 0;
	short ammo_mode;

	if((MechSpecials(mech) & ECM_TECH) ||
	   (MechSpecials2(mech) & STEALTH_ARMOR_TECH) ||
	   (MechSpecials2(mech) & NULLSIGSYS_TECH) ||
	   (MechSpecials(mech) & SLITE_TECH) ||
	   HasC3(mech) ||
	   HasC3i(mech) ||
	   (MechSpecials(mech) & MASC_TECH) ||
	   (MechSpecials2(mech) & SUPERCHARGER_TECH) ||
	   (MechSpecials(mech) & TRIPLE_MYOMER_TECH) ||
	   (MechSpecials2(mech) & ANGEL_ECM_TECH) || HasTAG(mech) ||
	   (MechInfantrySpecials(mech) & FC_INFILTRATORII_STEALTH_TECH)) {
		strcpy(tempbuff, "AdvTech: ");

		if(MechSpecials(mech) & ECM_TECH) {
			sprintf(tempbuff + strlen(tempbuff), "ECM(%s)  ",
					(MechCritStatus(mech) & ECM_DESTROYED) ? "%ch%crXX%cn"
					: ECMEnabled(mech) ? (ECMActive(mech) ? "%ch%cgECM%cn" :
										  "%ch%crECM%cn") : ECCMEnabled(mech)
					? "%ch%cgECCM%cn" : ECMCountered(mech) ? "%crOff%cn" :
					"%cgOff%cn");
		}

		if(MechSpecials2(mech) & ANGEL_ECM_TECH) {
			sprintf(tempbuff + strlen(tempbuff), "AngelECM(%s)  ",
					(!HasWorkingAngelECMSuite(mech)) ? "%ch%crXX%cn"
					: AngelECMEnabled(mech) ? (AngelECMActive(mech) ?
											   "%ch%cgECM%cn" :
											   "%ch%crECM%cn") :
					AngelECCMEnabled(mech) ? "%ch%cgECCM%cn" :
					ECMCountered(mech) ? "%crOff%cn" : "%cgOff%cn");

		}

		if(MechInfantrySpecials(mech) & FC_INFILTRATORII_STEALTH_TECH) {
			sprintf(tempbuff + strlen(tempbuff), "PersonalECM(%s)  ",
					PerECMEnabled(mech) ? (PerECMActive(mech) ?
										   "%ch%cgECM%cn" : "%ch%crECM%cn") :
					PerECCMEnabled(mech) ? "%ch%cgECCM%cn" :
					ECMCountered(mech) ? "%crOff%cn" : "%cgOff%cn");

		}

		if(MechSpecials2(mech) & STEALTH_ARMOR_TECH) {
			sprintf(tempbuff + strlen(tempbuff), "SthArmor(%s)  ",
					(MechCritStatus(mech) & ECM_DESTROYED) ? "%ch%crXX%cn"
					: StealthArmorActive(mech) ? "%ch%cgOn%cn" : "%cgRdy%cn");
		}

		if(MechSpecials2(mech) & NULLSIGSYS_TECH) {
			sprintf(tempbuff + strlen(tempbuff), "NullSigSys(%s)  ",
					NullSigSysDest(mech) ? "%ch%crXX%cn" :
					NullSigSysActive(mech) ? "%ch%cgOn%cn" : "%cgRdy%cn");
		}

		if(MechSpecials(mech) & SLITE_TECH) {
			sprintf(tempbuff + strlen(tempbuff), "SLITE(%s)  ",
					(MechCritStatus(mech) & SLITE_DEST) ? "%cr%chXX%cn"
					: (MechStatus2(mech) & SLITE_ON) ? "%ch%cgOn%cn" :
					"%cgOff%cn");
		}

		if(HasC3m(mech))
			sprintf(tempbuff + strlen(tempbuff), "%sC3M%%cn  ",
					C3Destroyed(mech) ? "%cr" :
					AnyECMDisturbed(mech) ? "%cy" :
					MechC3NetworkSize(mech) > 0 ? "%cg%ch" : "%cg");

		if(HasC3s(mech))
			sprintf(tempbuff + strlen(tempbuff), "%sC3S%%cn  ",
					C3Destroyed(mech) ? "%cr" :
					AnyECMDisturbed(mech) ? "%cy" :
					MechC3NetworkSize(mech) > 0 ? "%cg%ch" : "%cg");

		if(HasC3i(mech))
			sprintf(tempbuff + strlen(tempbuff), "%sC3i%%cn  ",
					C3iDestroyed(mech) ? "%cr" :
					AnyECMDisturbed(mech) ? "%cy" :
					MechC3iNetworkSize(mech) > 0 ? "%cg%ch" : "%cg");

		if(MechSpecials(mech) & TRIPLE_MYOMER_TECH)
			sprintf(tempbuff + strlen(tempbuff), "TSM(%s)  ",
					((MechHeat(mech) >= 9.0) ? "%ch%cgOn%cn" : "%cgOff%cn"));

		if(HasTAG(mech)) {
			sprintf(tempbuff + strlen(tempbuff), "TAG(%s)  ",
					isTAGDestroyed(mech) ? "%cr%chXX%cn" :
					((getMech(TAGTarget(mech)) <= 0) ||
					 (TaggedBy(getMech(TAGTarget(mech))) !=
					  mech->mynum)) ? (TagRecycling(mech) ?
									   "%cy%chNot Rdy%cn" : "%cgRdy%cn") :
					tprintf("%s%s%%cn",
							(TagRecycling(mech) ? "%cy%ch" : "%ch"),
							GetMechToMechID(mech, getMech(TAGTarget(mech)))));
		}

		if(MechSpecials2(mech) & SUPERCHARGER_TECH)
			sprintf(tempbuff + strlen(tempbuff), "SCHARGE: %s%d%%cn (%s)",
					MechSChargeCounter(mech) >
					3 ? "%ch%cr" : MechSChargeCounter(mech) >
					0 ? "%ch%cy" : "%cg", MechSChargeCounter(mech),
					MechStatus(mech) & SCHARGE_ENABLED ? "On" : "Off");

		if(MechSpecials(mech) & MASC_TECH)
			sprintf(tempbuff + strlen(tempbuff), "MASC: %s%d%%cn (%s)",
					MechMASCCounter(mech) >
					3 ? "%ch%cr" : MechMASCCounter(mech) >
					0 ? "%ch%cy" : "%cg", MechMASCCounter(mech),
					MechStatus(mech) & MASC_ENABLED ? "On" : "Off");

		notify(player, tempbuff);
		tempbuff[0] = 0;
	}

	if(MechSpecials2(mech) & CARRIER_TECH) {
		strcpy(tempbuff, "Carrier: ");

		sprintf(tempbuff + strlen(tempbuff),
				"%d tons free, %d tons max unit size",
				(CargoSpace(mech) / 100), CarMaxTon(mech));
		notify(player, tempbuff);
		tempbuff[0] = 0;
	}

	if((MechSpecials(mech) & AA_TECH) ||
	   (MechSpecials(mech) & BEAGLE_PROBE_TECH) ||
	   (MechSpecials2(mech) & BLOODHOUND_PROBE_TECH) ||
	   (MechSpecials(mech) & LIGHT_BAP_TECH)) {

		strcpy(tempbuff, "AdvSensors:");

		if(MechSpecials(mech) & AA_TECH)
			sprintf(tempbuff + strlen(tempbuff), " Radar");

		if(MechSpecials(mech) & BEAGLE_PROBE_TECH)
			sprintf(tempbuff + strlen(tempbuff), " BeagleProbe");

		if(MechSpecials2(mech) & BLOODHOUND_PROBE_TECH)
			sprintf(tempbuff + strlen(tempbuff), " BloodhoundProbe");
		
		if(MechSpecials(mech) & LIGHT_BAP_TECH)
			sprintf(tempbuff + strlen(tempbuff), " LightBAP");

		notify(player, tempbuff);
		tempbuff[0] = 0;
	}

	if((MechInfantrySpecials(mech) & CS_PURIFIER_STEALTH_TECH) ||
	   (MechInfantrySpecials(mech) & DC_KAGE_STEALTH_TECH) ||
	   (MechInfantrySpecials(mech) & FWL_ACHILEUS_STEALTH_TECH) ||
	   (MechInfantrySpecials(mech) & FC_INFILTRATOR_STEALTH_TECH) ||
	   (MechInfantrySpecials(mech) & FC_INFILTRATORII_STEALTH_TECH)) {

		strcpy(tempbuff, "AdvItems:");

		if(MechInfantrySpecials(mech) & CS_PURIFIER_STEALTH_TECH)
			sprintf(tempbuff + strlen(tempbuff), " PurifierStealth");

		if(MechInfantrySpecials(mech) & DC_KAGE_STEALTH_TECH)
			sprintf(tempbuff + strlen(tempbuff), " KageStealth");

		if(MechInfantrySpecials(mech) & FWL_ACHILEUS_STEALTH_TECH)
			sprintf(tempbuff + strlen(tempbuff), " AchileusStealth");

		if(MechInfantrySpecials(mech) & FC_INFILTRATOR_STEALTH_TECH)
			sprintf(tempbuff + strlen(tempbuff), " InfiltratorStealth");

		if(MechInfantrySpecials(mech) & FC_INFILTRATORII_STEALTH_TECH)
			sprintf(tempbuff + strlen(tempbuff), " InfiltratorIIStealth");

		notify(player, tempbuff);
		tempbuff[0] = 0;
	}

	if((MechInfantrySpecials(mech) & INF_SWARM_TECH) ||
	   (MechInfantrySpecials(mech) & INF_MOUNT_TECH) ||
	   (MechInfantrySpecials(mech) & INF_ANTILEG_TECH) ||
	   (MechInfantrySpecials(mech) & CAN_JETTISON_TECH)) {

		strcpy(tempbuff, "Special Actions:");

		if(MechInfantrySpecials(mech) & INF_MOUNT_TECH)
			sprintf(tempbuff + strlen(tempbuff), " MountFriends");

		if(MechInfantrySpecials(mech) & INF_SWARM_TECH)
			sprintf(tempbuff + strlen(tempbuff), " SwarmAttack");

		if(MechInfantrySpecials(mech) & INF_ANTILEG_TECH)
			sprintf(tempbuff + strlen(tempbuff), " AntiLegAttack");

		if(MechInfantrySpecials(mech) & CAN_JETTISON_TECH)
			sprintf(tempbuff + strlen(tempbuff), " BackPackJettison");

		notify(player, tempbuff);
		tempbuff[0] = 0;
	}

	if(MechInfantrySpecials(mech) & MUST_JETTISON_TECH) {
		strcpy(tempbuff,
			   "Requirements: Must jettison backpack before using special abilities or jumping");
		notify(player, tempbuff);
		tempbuff[0] = 0;
	}
#define SHOWSECTSTAT(a) \
	 (SectIsDestroyed(mech, a) ? "%ch%cx*****%c" : \
	 (MechSections(mech)[(a)].recycle > 0) ? \
	 tprintf("%-5d", (MechSections(mech)[(a)].recycle / WEAPON_TICK) \
         + (MechSections(mech)[(a)].recycle%WEAPON_TICK)) : "%cgReady%c")

	UpdateRecycling(mech);
	if(MechType(mech) == CLASS_MECH && !doweird) {
		tempbuff[0] = 0;

#define SHOWPHYSTATUS(a,b) \
	 (!canUsePhysical(mech,a,b) ? "%ch%crXX%c" : \
	 (MechSections(mech)[(a)].recycle > 0) ? \
		tprintf("%-3d", (MechSections(mech)[(a)].recycle / WEAPON_TICK) + \
        + (MechSections(mech)[(a)].recycle%WEAPON_TICK)) : "%cgRdy%c")

#define SHOW(part,loc) \
		sprintf(tempbuff + strlen(tempbuff), "%s: %s  ", part, loc)

		SHOW(MechIsQuad(mech) ? "FLLEG" : "LARM", SHOWSECTSTAT(LARM));
		SHOW(MechIsQuad(mech) ? "FRLEG" : "RARM", SHOWSECTSTAT(RARM));
		SHOW(MechIsQuad(mech) ? "RLLEG" : "LLEG", SHOWSECTSTAT(LLEG));
		SHOW(MechIsQuad(mech) ? "RRLEG" : "RLEG", SHOWSECTSTAT(RLEG));

		if(hasPhysical(mech, LARM, PHY_AXE))
			SHOW("Axe[LA]", SHOWPHYSTATUS(LARM, PHY_AXE));

		if(hasPhysical(mech, RARM, PHY_AXE))
			SHOW("Axe[RA]", SHOWPHYSTATUS(RARM, PHY_AXE));

		if(hasPhysical(mech, LARM, PHY_SWORD))
			SHOW("Sword[LA]", SHOWPHYSTATUS(LARM, PHY_SWORD));

		if(hasPhysical(mech, RARM, PHY_SWORD))
			SHOW("Sword[RA]", SHOWPHYSTATUS(RARM, PHY_SWORD));

		if(hasPhysical(mech, LARM, PHY_MACE))
			SHOW("Mace[LA]", SHOWPHYSTATUS(LARM, PHY_MACE));

		if(hasPhysical(mech, RARM, PHY_MACE))
			SHOW("Mace[RA]", SHOWPHYSTATUS(RARM, PHY_MACE));

		if(hasPhysical(mech, LARM, PHY_SAW))
			SHOW("Saw[LA]", SHOWPHYSTATUS(LARM, PHY_SAW));

		if(hasPhysical(mech, RARM, PHY_SAW))
			SHOW("Saw[RA]", SHOWPHYSTATUS(RARM, PHY_SAW));

		notify(player, tempbuff);

		if(MechStatus(mech) & FLIPPED_ARMS)
			notify(player, "*** Mech arms are flipped into the rear arc ***");
	} else if(MechType(mech) == CLASS_BSUIT && !doweird) {
		for(i = 0; i < NUM_BSUIT_MEMBERS; i++)
			if(GetSectInt(mech, i))
				break;
		if(i < NUM_BSUIT_MEMBERS) {
			sprintf(tempbuff, "Team status (special attacks): %s",
					SHOWSECTSTAT(i));
			notify(player, tempbuff);
		}

	} else if(((MechType(mech) == CLASS_VEH_GROUND) ||
			   (MechType(mech) == CLASS_VTOL)) && !doweird) {

		*tempbuff = 0;

		if(MechSections(mech)[FSIDE].recycle) {
			sprintf(tempbuff + strlen(tempbuff),
					"Vehicle status (charge): %s", SHOWSECTSTAT(FSIDE));
		}

		if(*tempbuff)
			notify(player, tempbuff);
	}

	ammoweapcount = FindAmmunition(mech, ammoweap, ammo, ammomax, modearray, 0);
	if(!doweird) {
		notify(player,
			   "==================WEAPON SYSTEMS===========================AMMUNITION========");
		if(MechType(mech) == CLASS_BSUIT)
			notify(player,
				   "------ Weapon --------- [##] Holder ------ Status ||--- Ammo Type ---- Rounds");
		else
			notify(player,
				   "------ Weapon --------- [##] Location ---- Status ||--- Ammo Type ---- Rounds");
	}
	for(loop = 0; loop < NUM_SECTIONS; loop++) {
		count = FindWeapons(mech, loop, weaparray, weapdata, critical);
		if(count <= 0)
			continue;
		ArmorStringFromIndex(loop, tempbuff, MechType(mech), MechMove(mech));
		sprintf(location, "%-14.14s", tempbuff);
		if(doweird) {
			strcpy(location, tempbuff);
			if((tmpc = strchr(location, ' ')))
				*tmpc = '_';
		}
		for(ii = 0; ii < count; ii++) {
			if(IsAMS(weaparray[ii]))
				sprintf(weapbuff, " %-16.16s %c%c%c%c%c [%2d] ",
						&MechWeapons[weaparray[ii]].name[3],
						' ',
						(MechStatus(mech) & AMS_ENABLED) ? ' ' : 'O',
						(MechStatus(mech) & AMS_ENABLED) ? 'O' : 'F',
						(MechStatus(mech) & AMS_ENABLED) ? 'N' : 'F',
						' ', running_sum + ii);
			else {
				if(GetPartFireMode(mech, loop, critical[ii]) & OS_MODE)
					strcpy(tmpbuf, "OS ");
				else
					tmpbuf[0] = 0;
				strcat(tmpbuf, &MechWeapons[weaparray[ii]].name[3]);
				sprintf(weapbuff, " %-16.16s %c%c%c%c%c [%2d] ", tmpbuf,
						(GetPartFireMode(mech, loop,
										 critical[ii]) & REAR_MOUNT) ? 'R' :
						' ',
						(((GetPartFireMode(mech, loop, critical[ii]) &
						   OS_USED)
						  || (GetPartFireMode(mech, loop, critical[ii]) &
							  ROCKET_FIRED)) ? '-' : (GetPartFireMode(mech,
																	  loop,
																	  critical
																	  [ii]) &
													  OS_MODE) ? 'O' : ' '),
						GetWeaponAmmoModeLetter(mech, loop, critical[ii]),
						GetWeaponFireModeLetter(mech, loop, critical[ii]),
						((GetPartFireMode(mech, loop, critical[ii]) & ON_TC)
						 && (!(MechCritStatus(mech) & TC_DESTROYED))) ? 'T'
						: (GetPartFireMode(mech, loop, critical[ii]) &
						   IS_JETTISONED_MODE) ? 'J' : (GetPartFireMode(mech,
																		loop,
																		critical
																		[ii])
														& WILL_JETTISON_MODE)
						? 'P' : ' ', running_sum + ii);
			}
			if(doweird)
				sprintf(weirdbuf + strlen(weirdbuf), "%s|%s",
						&MechWeapons[weaparray[ii]].name[3], location);
			strcat(weapbuff, location);

			if(PartIsBroken(mech, loop, critical[ii]) ||
			   PartTempNuke(mech, loop, critical[ii]) == FAIL_DESTROYED)
				strcat(weapbuff, "%ch%cx*****%c  || ");
			else if(PartIsDisabled(mech, loop, critical[ii]))
				strcat(weapbuff, "%crDISABLE%c|| ");
			else if(PartTempNuke(mech, loop, critical[ii])) {
				switch (PartTempNuke(mech, loop, critical[ii])) {
				case FAIL_JAMMED:
					strcat(weapbuff, "%crJAMMED%c || ");
					break;
				case FAIL_SHORTED:
					strcat(weapbuff, "%crSHORTED%c|| ");
					break;
				case FAIL_EMPTY:
					strcat(weapbuff, " %crEMPTY%c || ");
					break;
				case FAIL_DUD:
					strcat(weapbuff, "%crDUD%c    || ");
					break;
				case FAIL_AMMOJAMMED:
					strcat(weapbuff, "%crAMMOJAM%c|| ");
					break;
				}
			} else if(GetPartFireMode(mech, loop,
									  critical[ii]) & ROCKET_FIRED)
				strcat(weapbuff, "%ch%cxEmpty%c  || ");
			else if(weapdata[ii])
				strcat(weapbuff,
					   tprintf(" %2d    || ",
							   weapdata[ii] / WEAPON_TICK +
							   (weapdata[ii] % WEAPON_TICK ? 1 : 0)));
			else if(countDamagedSlotsFromCrit(mech, loop, critical[ii]))
				strcat(weapbuff, "%crDAMAGED%c|| ");
			else
				strcat(weapbuff, "%cgReady%c  || ");

			if((ii + running_sum) < ammoweapcount) {
				ammo_mode =
					GetWeaponAmmoModeLetter_Model_Mode(ammoweap[ii +
																running_sum],
													   modearray[ii +
																 running_sum]);
				sprintf(weapname, "%-16.16s %c",
						&MechWeapons[ammoweap[ii + running_sum]].name[3],
						ammo_mode);
				sprintf(tempbuff, "  %s%3d%s",
						evaluate_ammo_amount(ammo[ii + running_sum],
											 ammomax[ii + running_sum]),
						ammo[ii + running_sum], "%cn");
				strcat(weapname, tempbuff);
				if(doweird) {
					if(ammo_mode && ammo_mode != ' ')
						sprintf(weirdbuf + strlen(weirdbuf), "|%s|%d|%c ",
								&MechWeapons[ammoweap[ii +
													  running_sum]].name[3],
								ammo[ii + running_sum], ammo_mode);
					else
						sprintf(weirdbuf + strlen(weirdbuf), "|%s|%d ",
								&MechWeapons[ammoweap[ii +
													  running_sum]].name[3],
								ammo[ii + running_sum]);
				}
			} else {
				if(doweird)
					strcat(weirdbuf, " ");
				sprintf(weapname, "   ");
			}
			strcat(weapbuff, weapname);
			if(!doweird)
				notify(player, weapbuff);
		}
		running_sum += count;
	}

	if(running_sum < ammoweapcount) {
		while (running_sum < ammoweapcount) {
			strcpy(astrAmmoSpacer,
				   "                                                  || ");
			ammo_mode =
				GetWeaponAmmoModeLetter_Model_Mode(ammoweap[running_sum],
												   modearray[running_sum]);
			sprintf(weapname, "%-16.16s %c",
					&MechWeapons[ammoweap[running_sum]].name[3], ammo_mode);
			sprintf(tempbuff, "  %s%3d%s",
					evaluate_ammo_amount(ammo[running_sum],
										 ammomax[running_sum]),
					ammo[running_sum], "%cn");
			strcat(astrAmmoSpacer, weapname);
			strcat(astrAmmoSpacer, tempbuff);

			notify(player, astrAmmoSpacer);

			/*
			   if (doweird) {
			   if (ammo_mode && ammo_mode != ' ')
			   sprintf(weirdbuf + strlen(weirdbuf), "|%s|%d|%c ",
			   &MechWeapons[ammoweap[running_sum]].name[3], ammo[running_sum], ammo_mode);
			   else
			   sprintf(weirdbuf + strlen(weirdbuf), "|%s|%d ",
			   &MechWeapons[ammoweap[running_sum]].name[3], ammo[running_sum]);
			   }
			 */
			running_sum++;
		}
	}
}


/* Don't indent the entire next section -- it contains ASCII graphics */
/* *INDENT-OFF* */

/*
 * Status descriptions are evaluated with the following language:
 *
 * 1-6 at beginning of line = armor status symbol key
 *
 * && = &
 *
 * &[<width>]<type><loc> = armor status symbol
 *
 *              <width> = optional column width
 *
 *              <type>:
 *              + = external (front)
 *              - = external (rear)
 *              : = internal
 *
 *              <loc> = location number
 *
 * TODO: Provide a way to group characters together, which is what we usually
 * want to do, rather than forcing us to @<loc> every <char>.
 *
 * @<loc><char> = Show <char> only if location number <loc> is intact
 *
 * !<loc1><loc2><char> = Show <char> only if location number <loc1> or <loc2> 
 *                       is intact
 *
 * TODO: Use the same escape character for everything.  Or provide a way to
 * escape @ and ! characters. (&@ and &!, probably.)
 *
 * TODO: Because of custom status descriptions, we can't just change this
 * language willy-nilly.  Power Shaper said it was OK, though, so I got rid of
 * some backward compatibility code.  Done properly, it's just a few lines here
 * and there to check and activate flags.
 *
 * Old            New          Description
 * ------------- ------------- ------------------------------------
 * &(<type><loc> &1<type><loc> One digit column for mechwarriors.
 * &)<type><loc> &3<type><loc> Three digit column for dropships.
 * &S<?><?>      &:0           Aero structural integrity.
 * \n0                         End of script.
 */

#ifdef WEIGHTVARIABLE_STATUS

/*
 Light Bipedal BattleMech:
          __                   __                    __
       __( 9)__             __(**)__              __( 9)__
      /99|99|99\           /99|99|99\            /99|99|99\
     (99/-\/-\99)         (    \/    )          (99/-\/-\99)
       / /  \ \               /  \                / /  \ \
      (99|  |99)             /    \              (99|  |99)
*/
static const char *const lightmechdesc =
    "7         FRONT                REAR                INTERNAL\n"
    "1          @7_@7_                   @7_@7_                    @7_@7_\n"
    "2       @2_@2_@7(&+7@7)@3_@3_             @2_@2_@7(@7*@7*@7)@3_@3_              @2_@2_@7(&:7@7)@3_@3_\n"
    "3      @2/&+2!24|&+4!43|&+3@3\\           @2/&-2!24|&-4!34|&-3@3\\            @2/&:2!24|&:4!34|&:3@3\\\n"
    "4     @0(&+0!05/@2-@4\\@4/@3-!16\\&+1@1)         @0(    @4\\@4/    @1)          @0(&:0!05/@2-@4\\@4/@3-!16\\&:1@1)\n"
    "5       @5/ @5/  @6\\ @6\\               @5/  @6\\                @5/ @5/  @6\\ @6\\\n"
    "6      @5(&+5@5|  @6|&+6@6)             @5/    @6\\              @5(&:5@5|  @6|&:6@6)";

/*
 Heavy Bipedal Battlemech
          __                   __                    __
       __( 9)__             __(**)__              __( 9)__
      /99|99|99\           /99|99|99\            /99|99|99\
     (99/|==|\99)         (   |==|   )          (99/|==|\99)
       /  /\  \               /  \                /  /\  \
      (99/  \99)             /    \              (99/  \99)

*/

static const char *const heavymechdesc =
    "7         FRONT                REAR                INTERNAL\n"
    "1          @7_@7_                   @7_@7_                    @7_@7_\n"
    "2       @2_@2_@7(&+7@7)@3_@3_             @2_@2_@7(@7*@7*@7)@3_@3_              @2_@2_@7(&:7@7)@3_@3_\n"
    "3      @2/&+2!24|&+4!43|&+3@3\\           @2/&-2!24|&-4!34|&-3@3\\            @2/&:2!24|&:4!34|&:3@3\\\n"
    "4     @0(&+0!05/@4|@4=@4=@4|!16\\&+1@1)         @0(   !54|@4=@4=!64|   @1)          @0(&:0!05/@4|@4=@4=@4|!16\\&:1@1)\n"
    "5       @5/  @5/@6\\  @6\\               @5/  @6\\                @5/  @5/@6\\  @6\\\n"
    "6      @5(&+5@5/  @6\\&+6@6)             @5/    @6\\              @5(&:5@5/  @6\\&:6@6)";

/*
 Medium Bipedal Battlemech
          __                   __                    __
       __( 9)__               (**)                  ( 9)
      /99|99|99\           /99|99|99\            /99|99|99\
     (99/\__/\99)         (   \__/   )          (99/\__/\99)
       / /  \ \               /  \                / /  \ \
      (99|  |99)             /    \              (99|  |99)

*/
static const char *const mediummechdesc =

    "7         FRONT                REAR                INTERNAL\n"
    "1          @7_@7_                   @7_@7_                    @7_@7_\n"
    "2       @2_@2_@7(&+7@7)@3_@3_             @2_@2_@7(@7*@7*@7)@3_@3_              @2_@2_@7(&:7@7)@3_@3_\n"
    "3      @2/&+2!24|&+4!43|&+3@3\\           @2/&-2!24|&-4!34|&-3@3\\            @2/&:2!24|&:4!34|&:3@3\\\n"
    "4     @0(&+0!05/@4\\@4_@4_@4/!16\\&+1@1)         @0(   @4\\@4_@4_@4/   @1)          @0(&:0!05/@4\\@4_@4_@4/!16\\&:1@1)\n"
    "5       @5/ @5/  @6\\ @6\\               @5/  @6\\                @5/ @5/  @6\\ @6\\\n"
    "6      @5(&+5@5|  @6|&+6@6)             @5/    @6\\              @5(&:5@5|  @6|&:6@6)";

/*
 Assault Bipedal Battlemech
          __                   __                    __
       __[ 9]__             __[**]__              __[ 9]__
      /99|99|99\           /99|99|99\            /99|99|99\
     (99||==||99)         (   ||||   )          (99||==||99)
       / \||/ \               \||/                / \||/ \
      (99/  \99)             /    \              (99/  \99)

*/

static const char *const assaultmechdesc =
    "7         FRONT                REAR                INTERNAL\n"
    "1          @7_@7_                   @7_@7_                    @7_@7_\n"
    "2       @2_@2_@7[&+7@7]@3_@3_             @2_@2_@7[@7*@7*@7]@3_@3_              @2_@2_@7[&:7@7]@3_@3_\n"
    "3      @2/&+2!24|&+4!43|&+3@3\\           @2/&-2!24|&-4!34|&-3@3\\            @2/&:2!24|&:4!34|&:3@3\\\n"
    "4     @0(&+0@2|@4|@4=@4=@4|@3|&+1@1)         @0(   @4|@4|@4|@4|   @1)          @0(&:0@2|@4|@4=@4=@4|@3|&:1@1)\n"
    "5       @5/ @4\\@4|@4|@4/ @6\\               @4\\@4|@4|@4/                @5/ @4\\@4|@4|@4/ @6\\\n"
    "6      @5(&+5@5/  @6\\&+6@6)             @5/    @6\\              @5(&:5@5/  @6\\&:6@6)";

#else /* WEIGHTVARIABLE_STATUS */

/* 
  Bipedal BattleMech:
         ( 9)                 (**)                  ( 9)
      /99|99|99\           /99|99|99\            /99|99|99\
     (99/ || \99)         (   |  |   )          (99/ || \99)
       /  /\  \               /  \                /  /\  \
      (99/  \99)             /    \              (99/  \99)
*/

static const char *const mechdesc =
    "1         FRONT                REAR                INTERNAL\n"
    "2         @7(&+7@7)                 @7(@7*@7*@7)                  @7(&:7@7)\n"
    "3      @2/&+2!24|&+4!43|&+3@3\\           @2/&-2!24|&-4!34|&-3@3\\            @2/&:2!24|&:4!34|&:3@3\\\n"
    "4     @0(&+0!05/ !54|!64| !16\\&+1@1)         @0(   !54|  !64|   @1)          @0(&:0!05/ !54|!64| !16\\&:1@1)\n"
    "5       @5/  @5/@6\\  @6\\               @5/  @6\\                @5/  @5/@6\\  @6\\\n"
    "6      @5(&+5@5/  @6\\&+6@6)             @5/    @6\\              @5(&:5@5/  @6\\&:6@6)";

#endif /* WEIGHTVARIABLE_STATUS */

/*
  Quadruped BattleMech:
         FRONT                REAR                INTERNAL
           ___                                      ___
   ___  __/ 9 \_  ___                       ___  __/ 9 \_  ___
  ( 99\(99|99|99)/99 )     (99|99|99)      ( 99\(99|99|99)/99 )
   \ \(99/    \99)/ /                       \ \(99/    \99)/ /
    \ \||      ||/ /                         \ \||      ||/ /
    /_/__\    /__\_\                         /_/__\    /__\_\
*/

static const char *const quaddesc =
    "7         FRONT                REAR                INTERNAL\n"
    "1           @7_@7_@7_                                      @7_@7_@7_\n"
    "2   @5_@5_@5_  @2_@2_@7/&+7 @7\\@3_  @6_@6_@6_                       @5_@5_@5_  @2_@2_@7/&:7 @7\\@3_  @6_@6_@6_\n"
    "3  @5( &+5@5\\@2(&+2!24|&+4!43|&+3@3)@6/&+6 @6)     @2(&-2!24|&-4!43|&-3@3)      @5( &:5@5\\@2(&:2!24|&:4!43|&:3@3)@6/&:6 @6)\n"
    "4   @5\\ @5\\!05(&+0@0/    @1\\&+1!16)@6/ @6/                       @5\\ @5\\!05(&:0@0/    @1\\&:1!16)@6/ @6/\n"
    "5    @5\\ @5\\@0|@0|      @1|@1|@6/ @6/                         @5\\ @5\\@0|@0|      @1|@1|@6/ @6/\n"
    "6    @5/@5_!05/@0_@0_@0\\    @1/@1_@1_!16\\@6_@6\\                         @5/@5_!05/@0_@0_@0\\    @1/@1_@1_!16\\@6_@6\\";

/*
  MechWarrior
                       (9)
                     /9|9|9\
                    (9/ _ \9)
                     (9/ \9)
*/

static const char *const mwdesc =
    "1                       @7(&1:7@7)\n"
    "2                     @2/&1:2@4|&1:4@4|&1:3@3\\\n"
    "6                    @0(&1:0@0/ @4_ @1\\&1:1@1)\n"
    "7                     @5(&1:5@5/ @6\\&1:6@6)";


/*
  Naval vehicle

         FRONT                    INTERNAL
         ./99\.                    ./99\. 
        |'.--.`|                  |'.--.`|
        |`|99|'|                  |`|99|'|
        |\`--'/|                  |\`--'/|  
        |99><99|                  |99><99|  
        |./||\.|                  |./||\.|  
        || 99 ||                  || 99 ||  
         `~~~~'                    `~~~~' 
*/

static const char *const shipdesc =
    "7         FRONT                    INTERNAL\n"
    "1         @2.@2/&+2@2\\@2.                    @2.@2/&:2@2\\@2. \n"
    "2        @2|@2'@4.@4-@4-@4.@2`@2|                  @2|@2'@4.@4-@4-@4.@2`@2|\n"
    "3        @2|@2`@4|&+4@4|@2'@2|                  @2|@2`@4|&:4@4|@2'@2|\n"
    "4        !20|!04\\@4`@4-@4-@4'!14/!12|                  !20|!04\\@4`@4-@4-@4'!14/!12|  \n"
    "5        @0|&+0@0>@1<&+1@1|                  @0|&:0@0>@1<&:1@1|  \n"
    "7        @0|!03.!03/!03|!13|!13\\!13.@1|                  @0|!03.!03/!03|!13|!13\\!13.@1|  \n"
    "7        @0|@3| &+3 @3|@1|                  @0|@3| &:3 @3|@1|  \n"
    "7         !03`@3~@3~@3~@3~!13'                    !03`@3~@3~@3~@3~!13'";

/*
  Hydrofoil vehicle:
         FRONT                    INTERNAL
         ./\.                       ./\.
      ___|99|___                 ___|99|___
      ---|><|---                 ---|><|---
        ||99||                     ||99|| 
     __|99::99|__               __|99::99|__
     --_|'99`|_--               --_|'99`|_--
        `~~~~'                     `~~~~'
*/

static const char *const foildesc =
    "7         FRONT                    INTERNAL\n"
    "7         @2.@2/@2\\@2.                       @2.@2/@2\\@2.\n"
    "1      @2_@2_@2_@2|&+2@2|@2_@2_@2_                 @2_@2_@2_@2|&:2@2|@2_@2_@2_\n"
    "2      @2-@2-@2-@2|!24>!24<@2|@2-@2-@2-                 @2-@2-@2-@2|!24>!24<@2|@2-@2-@2-\n"
    "3        !04|@4|&+4@4|!14|                     !04|@4|&:4@4|!14| \n"
    "4     @0_@0_@0|&+0!01:!01:&+1@1|@1_@1_               @0_@0_@0|&:0!01:!01:&:1@1|@1_@1_\n"
    "5     @0-@0-@0_@0|!03'&+3!13`@1|@1_@1-@1-               @0-@0-@0_@0|!03'&:3!13`@1|@1_@1-@1-\n"
    "7        !03`@3~@3~@3~@3~!13'                     !03`@3~@3~@3~@3~!13'";

/*
  Submarine vehicle:

        FRONT                     INTERNAL
         --                         --      
       =|99|=                     =|99|=    
       |\__/|                     |\__/|    
      | |99| |                   | |99| |   
      |99||99|                   |99||99|   
       \|--|/                     \|--|/    
       =|99|=                     =|99|=    
*/

static const char *const subdesc =
    "7        FRONT                     INTERNAL\n"
    "1         @2-@2-                         @2-@2-      \n"
    "1       @2=@2|&+2@2|@2=                     @2=@2|&:2@2|@2=    \n"
    "1       !02|!24\\@4_@4_!24/!12|                     !02|!24\\@4_@4_!24/!12|    \n"
    "1      @0| @4|&+4@4| @1|                   @0| @4|&:4@4| @1|   \n"
    "1      @0|&+0@0|@1|&+1@1|                   @0|&:0@0|@1|&:1@1|   \n"
    "1       @0\\!03|@3-@3-!13|@1/                     @0\\!03|@3-@3-!13|@1/    \n"
    "1       !03=@3|&+3@3|!13=                     !03=@3|&:3@3|!13=";

/*
  Aero
              /^^\
            /|`18'|\
     |     |_|.--.|_|     |
     |      /||  ||\      |
     |    /'.-|--|-.`\    |
     |---'18| |20| |20`---|
     `--_____\||||/_____--'
             '===='
 */

static const char *const aerodesc =
    "7              @0/@0^@0^@0\\\n"
    "1            @1/@1|@0`&+0@0'@2|@2\\\n"
    "2     @1|     @1|@1_@1|.--.@2|@2_@2|     @2|\n"
    "3     @1|      @1/@1||&:0||@2\\      @2|\n"
    "4     @1|    @1/@1'@1.@1-@3|@3-@3-@3|@2-@2.@2`@2\\    @2|\n"
    "5     @1|@1-@1-@1-'&+1@1| @3|&+3@3| @2|&+2`---@2|\n"
    "6     @1`@1-@1-@1_@1_@1_@1_@1_\\||||/@2_@2_@2_@2_@2_@2-@2-'\n"
    "7             @1'===='";

/*
  Spheroid Dropship
          FRONT
          _______
         /`.999,'\
        |999|~|999|
        |  |   |  |
        |999|_|999|
         \,'999`./
          ~~~~~~~
*/

static const char *const spher_ds_desc =
    "7          FRONT\n"
    "1          @1_@1_@1_@1_@1_@1_@1_\n"
    "2         @1/!15`!15.&3+5!05,!05'@0\\\n"
    "3        @1|&3+1@1|@1~@0|&3+0@0|\n"
    "4        !12|  !12|   !03|  !03|\n"
    "5        @2|&3+2@2|@4_@3|&3+3@3|\n"
    "6         @2\\!24,!24'&3+4!34`!34.@3/\n"
    "7          @4~@4~@4~@4~@4~@4~@4~";

/*
  Aerodine Dropship
       .--.     
     ,`.99.'.   
     |.|__|.|   
    | | 99 | |  
    | |:  :| |  
   |'99|--|99`| 
   |  .'99`.  | 
   `|`_\~~/_`|' 
*/

static const char *const aerod_ds_desc =
    "7           .--.\n"
    "1         ,`.&+5.'.\n"
    "2         |.|__|.|\n"
    "3        | | ?? | |\n"
    "4        | |:  :| |\n"
    "5       |'&+1|--|&+0`|\n"
    "6       |  .'&+4`.  |\n"
    "7       `|`_\\~~/_`|'";

/*
  Vehicle (with turret):
          FRONT                                INTERNAL
         ,`.99,'.                              ,`.99,'.
        |  |__|  |                            |  |__|  |
        |  |99|  |                            |  |99|  |
        |99|~~|99|                            |99|~~|99|
         \,'99`./                              \,'99`./
          ~~~~~~                                ~~~~~~
*/

static const char *const vehdesc =
    "1          FRONT                                INTERNAL\n"
    "2         @0,@0`!02.&+2!12,@1'@1.                              @0,@0`!02.&:2!12,@1'@1.\n"
    "3        @0|  !02|@4_@4_!12|  @1|                            @0|  !02|@4_@4_!12|  @1|\n"
    "4        @0|  @4|&+4@4|  @1|                            @0|  @4|&:4@4|  @1|\n"
    "5        @0|&+0@4|@4~@4~@4|&+1@1|                            @0|&:0@4|@4~@4~@4|&:1@1|\n"
    "6         @0\\!03,!03'&+3!13`!13.@1/                              @0\\!03,!03'&:3!13`!13.@1/\n"
    "7          @3~@3~@3~@3~@3~@3~                                @3~@3~@3~@3~@3~@3~";

/*
  Vehicle (no turret, by design)
          FRONT                                INTERNAL
         ,`.99,'.                              ,`.99,'.
        |  |__|  |                            |  |__|  |
        |  |  |  |                            |  |  |  |
        |99|~~|99|                            |99|~~|99|
         \,'99`./                              \,'99`./
          ~~~~~~                                ~~~~~~
*/

static const char *const veh_not_desc =
    "1          FRONT                                INTERNAL\n"
    "2         @0,@0`!02.&+2!12,@1'@1.                              @0,@0`!02.&:2!12,@1'@1.\n"
    "3        @0|  !02|@2_@2_!12|  @1|                            @0|  !02|@2_@2_!12|  @1|\n"
    "4        @0|  @0|  @1|  @1|                            @0|  @0|  @1|  @1|\n"
    "5        @0|&+0@0|!01~!01~@1|&+1@1|                            @0|&:0@0|!01~!01~@1|&:1@1|\n"
    "6         @0\\!03,!03'&+3!13`!13.@1/                              @0\\!03,!03'&:3!13`!13.@1/\n"
    "7          @3~@3~@3~@3~@3~@3~                                @3~@3~@3~@3~@3~@3~";

/*
  VTOL
        FRONT                               INTERNAL
     .   ..   .                            .   ..   .   
     \\ `__` //                            \\ `__` //   
      \\.99.//                              \\.99.//    
     __\\  //__                            __\\  //__   
    (99|(99)|99)                          (99|(99)|99)  
    *--|// \\--*                          *--|// \\--*  
       //99 \\                               //99 \\    
      //\__/ \\                             //\__/ \\   
      ~       ~                             ~       ~   
*/


static const char *const vtoldesc =
    "7        FRONT                               INTERNAL\n"
    "7     @5.   @2.@2.   @5.                            @5.   @2.@2.   @5.   \n"
    "1     @5\\@5\\ @2`@2_@2_@2` @5/@5/                            @5\\@5\\ @2`@2_@2_@2` @5/@5/   \n"
    "2      @5\\@5\\@2.&+2@2.@5/@5/                              @5\\@5\\@2.&:2@2.@5/@5/    \n"
    "3     @0_@0_@5\\@5\\  @5/@5/@1_@1_                            @0_@0_@5\\@5\\  @5/@5/@1_@1_   \n"
    "4    @0(&+0!05|@5(&+5@5)!15|&+1@1)                          @0(&:0!05|@5(&:5@5)!15|&:1@1)  \n"
    "5    @0*@0-@0-@0|@5/@5/ @5\\@5\\@1-@1-@1*                          @0*@0-@0-@0|@5/@5/ @5\\@5\\@1-@1-@1*  \n"
    "6       @5/@5/&+3 @5\\@5\\                               @5/@5/&:3 @5\\@5\\    \n"
    "7      @5/@5/@3\\@3_@3_@3/ @5\\@5\\                             @5/@5/@3\\@3_@3_@3/ @5\\@5\\   \n"
    "7      @5~       @5~                             @5~       @5~";

/* 
 Battlesuit

 SQUAD STATUS
  Member#       1  2  3  4  5
   Health      99 99 99 99 99
   Armor       99 99 99 99 99

*/


static const char *const bsuitdesc =
    "7 SQUAD STATUS\n"
    "7   Member#      @01  @12  @23  @34  @45  @56  @67  @78\n"
    "7   Health      &:0 &:1 &:2 &:3 &:4 &:5 &:6 &:7\n"
    "7   Armor       &+0 &+1 &+2 &+3 &+4 &+5 &+6 &+7";

/* *INDENT-ON* */

/*
 * Picks the character to show for the armor location on enemy scans.
 *
 * mech: mech
 * loc: armor location
 * flag: ARMOR_FRONT, ARMOR_INTERNAL, or ARMOR_REAR
 * ret_armor_value: return value pointer for remaining armor points
 *
 * For aero structural integrity, use loc 0 and flag ARMOR_INTERNAL.
 *
 * Returns the "armor level", a value from 0 to 5 indicating the qualitative
 * condition of the armor:
 *
 * 0 - >90%
 * 1 - >75%
 * 2 - >45%
 * 3 - >0%
 * 4 - 0% (breached)
 * 5 - under repair
 */
int
ArmorEvaluateSerious(MECH *mech, int loc, int flag, int *ret_armor_value)
{
	int armor_value;
	int armor_percent, armor_denom;
	int repair_flag = 0;

	/*
	 * TODO: What happens when a custom template plugs in bogus values?
	 * Make sure to check for aeros, too!
	 */
	switch (flag & ARMOR_TYPE_MASK) {
	case ARMOR_FRONT:
		/* Front armor.  */
		armor_value = GetSectArmor(mech, loc);
		armor_denom = GetSectOArmor(mech, loc);

		if (SectArmorRepair(mech, loc))
			repair_flag = 1;
		break;

	case ARMOR_INTERNAL:
		if (MechType(mech) == CLASS_AERO && loc == 0) {
			/* Aero SI.  loc doesn't actually matter, but we check
			 * it in case we want to use other locs later. */
			armor_value = AeroSI(mech);
			armor_denom = AeroSIOrig(mech);
		} else {
			/* Internal armor.  */
			armor_value = GetSectInt(mech, loc);
			armor_denom = GetSectOInt(mech, loc);

			if (SectIntsRepair(mech, loc))
				repair_flag = 1;
		}
		break;

	case ARMOR_REAR:
		/* Rear armor.  */
		armor_value = GetSectRArmor(mech, loc);
		armor_denom = GetSectORArmor(mech, loc);

		if (SectRArmorRepair(mech, loc))
			repair_flag = 1;
		break;

	default:
		/* FIXME: We were given a bad flag.  Panic! */
		armor_value = -1; /* XXX: hack to make problems obvious */
		repair_flag = 1; /* XXX: hack selecting ? (repairing) */
		break;
	}

	if (ret_armor_value) {
		*ret_armor_value = armor_value;
	}

	if (repair_flag) {
		/* Under repair.  */
		return ARMOR_LEVEL_REPAIRING;
	} else if (!armor_value) {
		/* Breached.  */
		return ARMOR_LEVEL_OPEN;
	} else {
		/* Armor condition level.  */
		armor_percent = (armor_value + 1) * 100 / (armor_denom + 1);

		if (armor_percent <= 45) {
			return ARMOR_LEVEL_CRITICAL;
		} else if (armor_percent <= 70) {
			return ARMOR_LEVEL_LOW;
		} else if (armor_percent <= 90) {
			return ARMOR_LEVEL_GOOD;
		} else {
			return ARMOR_LEVEL_GREAT;
		}
	}
}

/* bright green, dark green, bright yellow, dark red, black */
static const char *const armordamcolorstr[] = {
	"%ch%cg",
	"%cg",
	"%ch%cy",
	"%cr",
	"%ch%cx",
	"%ch%cb"
};

/* Armor location character (enemy scan). Last one is for armor under repair. */
static const char armordamltrstr[] = "OoxX*?";

/*
 * XXX: memcpy/memset() are technically only standard as of C99, so strictly we
 * should autoconf-ize this with portability wrappers.  They're pretty common
 * these days, though.
 */
static char *
PrintArmorDamageString(const int armor_level, int armor_value,
                       const int flag, const int width)
{
	/* This array has to be at least as large as the maximum possible
	 * width (+ 1), which is defined in show_armor().  */
	static char armor_string[23 + 1];
	char *asp;

	char armor_buf[23 + 1];

	if (flag & ARMOR_FLAG_DIVIDE_10) {
		/* Divide by 10 (rounded up).  Used for mechwarriors.  */
		armor_value = (armor_value + 9) / 10;
	}

	if (flag & ARMOR_FLAG_OWNED) {
		int armor_len;

		/* TODO: snprintf() is a C99-ism, please autoconf-ize.  */
		/* XXX: Aeros 0-filled spaces.  That's silly.  */
		snprintf(armor_buf, sizeof(armor_buf), "%d", armor_value);

		/* XXX: Return values aren't standardized until C99.  */
		armor_len = strlen(armor_buf);

		/* Fixed width.  Some sprintf()s have a $*d extension that we
		 * aren't going to use.  */
		asp = armor_string;

		if (armor_len < width) {
			/* Right justify.  */
			memset(asp, ' ', width - armor_len);
			asp += width - armor_len;

			memcpy(asp, armor_buf, armor_len);
			asp += armor_len;
		} else {
			/* Right truncate.  */
			memcpy(asp, armor_buf + (armor_len - width), width);
			asp += width;
		}
	} else {
		/* Use adversarial (scan) fill characters.  */
		memset(armor_string, armordamltrstr[armor_level], width);
	}

	armor_string[width] = '\0';

	return armor_string;
}

/*
 * TODO: Probably better to make this a substitution.  That would allow the key
 * width to match the actual width on the status display, too; right now, it's
 * always two characters, regardless of width.
 */
static char *
ArmorKeyInfo(dbref player, int line_key, int owner)
{
	static char str[6 + 3 + 2 + 1];

	if (owner) {
		/* Only show key on scans.  */
		str[0] = '\0';
	} else if (line_key == 1) {
		/* Line 1 = "Key".  */
		strcpy(str, "Key");
	} else if (line_key > 6) {
		/* Line >6 = empty.  */
		strcpy(str, "   ");
	} else {
		/* Line 2-6 = armor level symbols.  */
		/* XXX: Probably safe from buffer overflows.  */
		sprintf(str, "%s%c%c %%c", armordamcolorstr[6 - line_key],
		        armordamltrstr[6 - line_key],
		        armordamltrstr[6 - line_key]);
	}

	return str;
}

/*
 * XXX: memcpy/memset() are technically only standard as of C99, so strictly we
 * should autoconf-ize this with portability wrappers.  They're pretty common
 * these days, though.
 */
static char *
show_armor(MECH *mech, const int loc, const int flag, int width)
{
	/* XXX: color_string must be 6 chars or less.  */ 
	static char fieldbuf[6 + 23 + 2 + 1];

	char *fbp = fieldbuf;

	const char *color_string;
	const char *armor_string;

	int armor_level, armor_value;

	/* Sanity check arguments.  */
	if (width > sizeof(fieldbuf) - 6 - 2 - 1) {
		width = sizeof(fieldbuf) - 6 - 2 - 1;
	}

	/* Get armor status.  */
	armor_level = ArmorEvaluateSerious(mech, loc, flag, &armor_value);

	/* Get strings.  */
	if (!(flag & ARMOR_FLAG_SHOW_DEST) && !GetSectInt(mech, loc)) {
		/* Blank field. (Destroyed section.) */
		memset(fieldbuf, ' ', width);
		fieldbuf[width] = '\0';
		return fieldbuf;
	} else {
		color_string = armordamcolorstr[armor_level];
		armor_string = PrintArmorDamageString(armor_level, armor_value,
		                                      flag, width);
	}

	/* XXX: sprintf() should be safe here.  Emphasis on "should".  */
	sprintf(fieldbuf, "%s%s%%c", color_string, armor_string);

	return fieldbuf;
}

/* See if the 'mech has a 'custom' template (@mechstatus attr)
 * if so, exec() it to evaluate color/newlines.
 */
static int
get_statustemplate_attr(dbref player, MECH *mech, char *result)
{
	char *resultc, *statattr = silly_atr_get(mech->mynum, A_MECHSTATUS);

	if (!statattr || !*statattr)
		return 0;

	/* this is safe because tmpbuf is larger than LBUF_SIZE */
	resultc = result;

	exec(result, &resultc, 0, player, mech->mynum,
	     EV_STRIP_AROUND | EV_NO_COMPRESS | EV_NO_LOCATION | EV_NOFCHECK
	     | EV_NOTRACE | EV_FIGNORE, &statattr, NULL, 0);

	return 1;
}

/* BTS = BattleTech status. */
typedef enum {
	BTS_START_OF_LINE, /* start state */

	BTS_NORMAL, /* normal input */

	BTS_SUBSTITUTE_ARMOR, /* armor status substitution */
	BTS_CONDITIONAL_1, /* unary conditional  */
	BTS_CONDITIONAL_2 /* binary conditional */
} BTS_State;

void
PrintArmorStatus(dbref player, MECH *mech, int owner)
{
	const char *srcbuf, *sbp, *saved_sbp;

	char destbuf[LBUF_SIZE], *dbp;

	BTS_State current_state = BTS_START_OF_LINE;
	int tmp_value1, tmp_value2;
	int flag;

	char tmpbuf[8192];

	/* Select status template.  */
	switch (MechType(mech)) {
	case CLASS_MW:
		/* TODO: Should probably make this user-selectable by adding
		 * some more formatting flags.  */
		flag = ARMOR_FLAG_DIVIDE_10;
		break;

	case CLASS_AERO:
	case CLASS_DS:
		flag = ARMOR_FLAG_SHOW_DEST;
		break;

	default:
		flag = 0;
		break;
	}

	if (get_statustemplate_attr(player, mech, tmpbuf)) {
		/* Use custom template.  */
		srcbuf = tmpbuf;
	} else {
		/* Use standard template.  */
		switch (MechType(mech)) {
		case CLASS_MW:
			srcbuf = mwdesc;
			break;

		case CLASS_MECH:
			if (MechIsQuad(mech)) {
				srcbuf = quaddesc;
			} else {
#ifdef WEIGHTVARIABLE_STATUS
				if (MechTons(mech) <= 35)
					srcbuf = lightmechdesc;
				else if (MechTons(mech) <= 55)
					srcbuf = mediummechdesc;
				else if (MechTons(mech) <= 75)
					srcbuf = heavymechdesc;
				else
					srcbuf = assaultmechdesc;
#else /* WEIGHTVARIABLE_STATUS */
				srcbuf = mechdesc;
#endif /* WEIGHTVARIABLE_STATUS */
			}
			break;

		case CLASS_BSUIT:
			srcbuf = bsuitdesc;
			break;

		case CLASS_VTOL:
			srcbuf = vtoldesc;
			break;

		case CLASS_AERO:
			srcbuf = aerodesc;
			break;

		case CLASS_DS:
			srcbuf = aerod_ds_desc;
			break;

		case CLASS_SPHEROID_DS:
			srcbuf = spher_ds_desc;
			break;

		case CLASS_VEH_GROUND:
			if (GetSectOInt(mech, TURRET))
				srcbuf = vehdesc;
			else
				srcbuf = veh_not_desc;
			break;

		case CLASS_VEH_NAVAL:
			if (MechMove(mech) == MOVE_FOIL)
				srcbuf = foildesc;
			else if (MechMove(mech) == MOVE_HULL)
				srcbuf = shipdesc;
			else
				srcbuf = subdesc;
			break;

		default:
			srcbuf = " This 'toy' is of unknown type. It has yet to be templated\n for status.";
			break;
		}
	}

	/* Perform substitution on template.  */
	dbp = destbuf;

	saved_sbp = srcbuf;
#define COMMIT_SAVED_SBP() \
	/* TODO: It's fine to make this a real function.  */ \
	do { \
		const int maxlen = sizeof(destbuf) - 1; \
		const int dstlen = dbp - destbuf; \
		\
		int srclen = sbp - saved_sbp; \
		\
		if (dstlen < maxlen) { \
			if (dstlen + srclen > maxlen) \
				srclen = maxlen - dstlen; \
			\
			memcpy(dbp, saved_sbp, srclen); \
			dbp += srclen; \
		} \
		\
		saved_sbp = sbp; \
	} while (0)

	for (sbp = srcbuf; *sbp; sbp++) {
#define SAFE_CHR_DBP(c) \
	do { \
		if ((dbp - destbuf) < (sizeof(destbuf) - 1)) \
			*dbp++ = (c); \
	} while (0)

#define ASCII_ATOI(c) \
	/* TODO: This is dependent on the way ASCII encodes digits.  */ \
	((c) - '0')

		BTS_State next_state = current_state;

		/* Dispatch on current state.  */
		switch (current_state) {
		case BTS_START_OF_LINE: /* start of line */
			/*
			 * XXX: Portability note: Depends on a specific way of
			 * encoding the digits from 0 to 7.
			 */
			if (*sbp >= '1' && *sbp <= '7') {
				COMMIT_SAVED_SBP();
				saved_sbp = sbp + 1;

				safe_str(ArmorKeyInfo(player, ASCII_ATOI(*sbp),
				                      owner),
				         destbuf, &dbp);
			}

			next_state = BTS_NORMAL;
			break;

		case BTS_NORMAL: /* normal characters */
			switch (*sbp) {
			case '&':
				COMMIT_SAVED_SBP();
				next_state = BTS_SUBSTITUTE_ARMOR;
				break;

			case '@':
				COMMIT_SAVED_SBP();
				next_state = BTS_CONDITIONAL_1;
				break;

			case '!':
				COMMIT_SAVED_SBP();
				next_state = BTS_CONDITIONAL_2;
				break;
			}
			break;

		case BTS_SUBSTITUTE_ARMOR: /* armor status substitution */
			switch (sbp - saved_sbp) {
				int tmp_flag;

			case 1: /* optional width digit or type flag */
				switch (*sbp) {
				case '&':
					saved_sbp = sbp + 1;
					next_state = BTS_NORMAL;

					SAFE_CHR_DBP('&');
					break;

				case '+':
				case ':':
				case '-':
					tmp_value1 = *sbp;
					break;

				default:
					if (isdigit(*sbp)) {
						tmp_value1 = *sbp;
					} else {
						next_state = BTS_NORMAL;
					}
					break;
				}
				break;

			case 2: /* location or type flag */
				if (isdigit(tmp_value1)) {
					/* Expect type code.  */
					switch (*sbp) {
					case '+':
					case '-':
					case ':':
						tmp_value2 = *sbp;
						break;

					default:
						next_state = BTS_NORMAL;
						break;
					}

					tmp_value1 = ASCII_ATOI(tmp_value1);
					break;
				} else {
					/* Expect section number.  */
					tmp_value2 = tmp_value1;
					tmp_value1 = 2;
				}
			case 3: /* location */
				/* Expect section number.  */
				tmp_flag = flag;

				tmp_flag |= owner ? ARMOR_FLAG_OWNED : 0;

				switch (tmp_value2) {
				case '+':
					tmp_flag |= ARMOR_FRONT;
					break;

				case ':':
					tmp_flag |= ARMOR_INTERNAL;
					break;

				case '-':
					tmp_flag |= ARMOR_REAR;
					break;

				default:
					break;
				}

				/* FIXME: Ponder semantics of gflag.  */
				safe_str(show_armor(mech, ASCII_ATOI(*sbp),
				                    tmp_flag, tmp_value1),
				         destbuf, &dbp);

				saved_sbp = sbp + 1;
				next_state = BTS_NORMAL;
				break;

			default: /* XXX: should never happen */
				break;
			}
			break;

		case BTS_CONDITIONAL_1: /* '@' unary conditional */
			switch (sbp - saved_sbp) {
			case 1: /* get critical section */
				if (isdigit(*sbp)) {
					tmp_value1 = ASCII_ATOI(*sbp);
				} else {
					next_state = BTS_NORMAL;
				}
				break;

			case 2: /* copy conditional character */
				saved_sbp = sbp + 1;
				next_state = BTS_NORMAL;

				if (GetSectInt(mech, tmp_value1)) {
					SAFE_CHR_DBP(*sbp);
				} else {
					SAFE_CHR_DBP(' ');
				}
				break;

			default: /* XXX: should never happen */
				break;
			}
			break;

		case BTS_CONDITIONAL_2: /* '!' binary conditional */
			switch (sbp - saved_sbp) {
			case 1: /* get first critical section */
				if (isdigit(*sbp)) {
					tmp_value1 = ASCII_ATOI(*sbp);
				} else {
					next_state = BTS_NORMAL;
				}
				break;

			case 2: /* get second critical section */
				if (isdigit(*sbp)) {
					tmp_value2 = ASCII_ATOI(*sbp);
				} else {
					next_state = BTS_NORMAL;
				}
				break;

			case 3: /* copy conditional character */
				saved_sbp = sbp + 1;
				next_state = BTS_NORMAL;

				if (GetSectInt(mech, tmp_value1)
				    || GetSectInt(mech, tmp_value2)) {
					SAFE_CHR_DBP(*sbp);
				} else {
					SAFE_CHR_DBP(' ');
				}
				break;

			default: /* XXX: should never happen */
				break;
			}
			break;

		default: /* XXX: should never happen */
			break;
		}

		/* Common logic.  */
		if (*sbp == '\n') {
			current_state = BTS_START_OF_LINE;

			/*
			 * MUX expects \r\n for line endings in buffers.
			 * PennMUSH, where this code was originally developed,
			 * expects \n, and converts to \r\n as needed.
			 *
			 * FIXME: This is sorta a hack.  We don't really want
			 * to be dealing with line ending issues in individual
			 * functions, but more extensive changes would be
			 * disruptive.
			 */
			COMMIT_SAVED_SBP();
			SAFE_CHR_DBP('\r');
			/* \n written later.  */
		} else {
			current_state = next_state;
		}
#undef ASCII_ATOI
#undef SAFE_CHR_DBP
	}

	/* Finish up.  */
	COMMIT_SAVED_SBP();
#undef COMMIT_SAVED_SBP

	/* Send formatted status.  */
	*dbp = '\0';

	notify(player, destbuf);
}

/*
 * Figure out if we have a certain kind of physical weapon.
 */
int hasPhysical(MECH * objMech, int wLoc, int wPhysType)
{
	int wType;
	int wSize;

	switch (wPhysType) {
	case PHY_AXE:
		wType = AXE;
		wSize = MechTons(objMech) / 15;
		break;

	case PHY_SWORD:
		wType = SWORD;
		wSize = MechTons(objMech) / 15;
		break;

	case PHY_MACE:
		wType = MACE;
		wSize = MechTons(objMech) / 15;
		break;

	case PHY_SAW:
		wType = DUAL_SAW;
		wSize = 7;
		break;

	default:
		return 0;
	}							// end switch()

	return FindObjWithDest(objMech, wLoc, I2Special(wType)) >= wSize;
}								// end hasPhysical()

int canUsePhysical(MECH * objMech, int wLoc, int wPhysType)
{
	int tRet = 1;

	switch (wPhysType) {
	case PHY_AXE:
	case PHY_SWORD:
		if(SectIsDestroyed(objMech, wLoc))
			tRet = 0;
		else if(!OkayCritSectS2(objMech, wLoc, 0, SHOULDER_OR_HIP))
			tRet = 0;
		else if(!OkayCritSectS2(objMech, wLoc, 3, HAND_OR_FOOT_ACTUATOR))
			tRet = 0;
		break;

	case PHY_MACE:
		if(SectIsDestroyed(objMech, LARM) || SectIsDestroyed(objMech, RARM))
			tRet = 0;
		else if((!OkayCritSectS2(objMech, LARM, 0, SHOULDER_OR_HIP)) ||
				(!OkayCritSectS2(objMech, RARM, 0, SHOULDER_OR_HIP)))
			tRet = 0;
		else if((!OkayCritSectS2(objMech, LARM, 3, HAND_OR_FOOT_ACTUATOR))
				|| (!OkayCritSectS2(objMech, RARM, 3, HAND_OR_FOOT_ACTUATOR)))
			tRet = 0;
		break;

	case PHY_SAW:
		if(SectIsDestroyed(objMech, wLoc))
			tRet = 0;
		else if(!OkayCritSectS2(objMech, wLoc, 0, SHOULDER_OR_HIP))
			tRet = 0;
		break;

	default:
		tRet = 0;
	}							// end switch()

	return tRet;
}								// end canUsePhysical()
