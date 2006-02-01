/*
 * Copyright (c) 2002 Thomas Wouters <thomas@xs4all.net>
 *
 * HUDINFO support.
 */

#include <string.h>
#include <math.h>

#include "config.h"
#include "externs.h"
#include "interface.h"

#include "muxevent.h"
#include "mech.h"
#include "map.h"
#include "map.los.h"
#include "p.mech.utils.h"
#include "p.mech.contacts.h"
#include "failures.h"
#include "p.mech.build.h"
#include "p.mech.notify.h"
#include "p.mech.update.h"
#include "p.mech.move.h"
#include "p.mech.los.h"
#include "p.mech.status.h"

extern void auto_reply(MECH * mech, char *buf);

#ifdef HUDINFO_SUPPORT

#include "hudinfo.h"

void fake_hudinfo(dbref player, dbref cause, int key, char *arg)
{
	notify(player, "HUDINFO does not work from scode or macros.");
}

HASHTAB hudinfo_cmdtab;

void init_hudinfo(void)
{
	HUDCMD *cmd;

	hashinit(&hudinfo_cmdtab, 20 * HASH_FACTOR);

	for(cmd = hudinfo_cmds; cmd->cmd; cmd++)
		hashadd(cmd->cmd, (int *) cmd, &hudinfo_cmdtab);
}

static MECH *getMech_forPlayer(dbref player)
{
	dbref inv, tmp;
	MECH *mech = NULL;

	if(Hardcode(player) && !Zombie(player))
		mech = getMech(player);

	if(!mech) {
		tmp = Location(player);
		if(Hardcode(tmp) && !Zombie(tmp))
			mech = getMech(tmp);
	}

	if(!mech) {
		SAFE_DOLIST(inv, tmp, Contents(player)) {
			if(Hardcode(inv) && !Zombie(inv))
				mech = getMech(inv);
			if(mech)
				break;
		}
	}
	return mech;
}

void do_hudinfo(DESC * d, char *arg)
{
	char *subcmd;
	HUDCMD *cmd;
	MECH *mech = NULL;

	while (*arg && isspace(*arg))
		arg++;

	if(!*arg) {
		hudinfo_notify(d, NULL, NULL, "#HUD hudinfo version "
					   HUD_PROTO_VERSION);
		return;
	}

	if(strncmp(arg, "key=", 4) == 0) {
		arg += 4;
		if(!arg || strlen(arg) > 20) {
			hudinfo_notify(d, "KEY", "E", "Invalid key");
			return;
		}
		for(subcmd = arg; *subcmd; subcmd++) {
			if(!isalnum(*subcmd)) {
				hudinfo_notify(d, "KEY", "E", "Invalid key");
				return;
			}
		}
		strcpy(d->hudkey, arg);
		hudinfo_notify(d, "KEY", "R", "Key set");
		return;
	}

	if(!d->hudkey[0]) {
		hudinfo_notify(d, "???", "E", "No session key set");
		return;
	}

	subcmd = arg;

	while (*arg && !isspace(*arg)) {
		if(!isalnum(*arg)) {
			hudinfo_notify(d, "???", "E", "Invalid subcommand");
			return;
		}
		arg++;
	}

	if(*arg) {
		*arg++ = '\0';
	}

	while (*arg && isspace(*arg))
		arg++;

	cmd = (HUDCMD *) hashfind(subcmd, &hudinfo_cmdtab);
	if(!cmd) {
		hudinfo_notify(d, "???", "E",
					   tprintf("%s: subcommand not found", subcmd));
		return;
	}

	if(cmd->flag & HUDCMD_HASARG) {
		if(!*arg) {
			hudinfo_notify(d, cmd->msgclass, "E", "Not enough arguments");
			return;
		}
	} else if(*arg) {
		hudinfo_notify(d, cmd->msgclass, "E", "Command takes no arguments");
		return;
	}

	if(cmd->flag & HUDCMD_NEEDMECH) {
		mech = getMech_forPlayer(d->player);
		if(!mech) {
			hudinfo_notify(d, cmd->msgclass, "E", "Not in a BattleTech unit");
			return;
		}
		if((cmd->flag & HUDCMD_NONDEST) && Destroyed(mech)) {
			hudinfo_notify(d, cmd->msgclass, "E", "You are destroyed!");
			return;
		}
		if((cmd->flag & HUDCMD_STARTED) && !Started(mech)) {
			hudinfo_notify(d, cmd->msgclass, "E", "Reactor is not online");
			return;
		}
		if((cmd->flag & HUDCMD_AWAKE) &&
		   (MechStatus(mech) & (BLINDED | UNCONSCIOUS))) {
			hudinfo_notify(d, cmd->msgclass, "E",
						   "You are unconscious....zzzzzzz");
			return;
		}
	}

	cmd->handler(d, mech, cmd->msgclass, arg);
	return;
}

static void FindRangeAndBearingToCenter(MECH * mech, float *rtc, int *btc)
{
	float fx, fy;

	MapCoordToRealCoord(MechX(mech), MechY(mech), &fx, &fy);
	*rtc = FindHexRange(fx, fy, MechFX(mech), MechFY(mech));
	*btc = FindBearing(MechFX(mech), MechFY(mech), fx, fy);
}

static void hud_generalstatus(DESC * d, MECH * mech, char *msgclass,
							  char *args)
{
	static char response[LBUF_SIZE];
	char fuel[15];
	char tstat[5];
	char jumpx[8], jumpy[8];
	int btc;
	float rtc;

	if(FlyingT(mech) && !AeroFreeFuel(mech))
		sprintf(fuel, "%d", AeroFuel(mech));
	else
		strcpy(fuel, "-");

	if(MechHasTurret(mech))
		sprintf(tstat, "%d", AcceptableDegree(MechTurretFacing(mech)) - 180);
	else if((MechType(mech) == CLASS_MECH && !MechIsQuad(mech)) ||
			MechType(mech) == CLASS_MW)
		sprintf(tstat, "%d",
				MechStatus(mech) & TORSO_LEFT ? -60 : MechStatus(mech) &
				TORSO_RIGHT ? 60 : 0);
	else
		strcpy(tstat, "-");

	if(Jumping(mech)) {
		snprintf(jumpx, 8, "%d", MechGoingX(mech));
		snprintf(jumpy, 8, "%d", MechGoingY(mech));
	} else {
		strcpy(jumpx, "-");
		strcpy(jumpy, "-");
	};

	FindRangeAndBearingToCenter(mech, &rtc, &btc);

	sprintf(response,
			"%s,%d,%d,%d,%d,%d,%.2f,%.2f,%d,%d,%s,%.2f,%.2f,%.3f,%d,%s,%s,%s,%s",
			MechIDS(mech, 0), MechX(mech), MechY(mech), MechZ(mech),
			MechFacing(mech), MechDesiredFacing(mech),
			MechSpeed(mech), MechDesiredSpeed(mech),
			(int) (10 * MechPlusHeat(mech)),
			(int) (10. * MechActiveNumsinks(mech)),
			fuel, MechVerticalSpeed(mech), MechVerticalSpeed(mech),
			rtc, btc, tstat, getStatusString(mech, 2), jumpx, jumpy);

	hudinfo_notify(d, msgclass, "R", response);
}

static char *hud_getweaponstatus(MECH * mech, int sect, int crit, int data)
{
	static char wstatus[12];

	if(PartIsBroken(mech, sect, crit))
		return "*";
	if(PartIsDisabled(mech, sect, crit))
		return "D";
	switch (PartTempNuke(mech, sect, crit)) {
	case FAIL_JAMMED:
		return "J";
	case FAIL_SHORTED:
		return "S";
	case FAIL_DUD:
		return "d";
	case FAIL_EMPTY:
		return "E";
	case FAIL_AMMOJAMMED:
		return "A";
	case FAIL_DESTROYED:
		return "*";
	}
	if(GetPartFireMode(mech, sect, crit) & IS_JETTISONED_MODE)
		return "j";
	if(data) {
		sprintf(wstatus, "%d", data / WEAPON_TICK);
		return wstatus;
	}
	return "R";
}

static char *hud_getfiremode(MECH * mech, int sect, int crit, int type)
{
	int mode = GetPartFireMode(mech, sect, crit);
	static char wmode[30];
	char *p = wmode;

	if(mode & RAC_TWOSHOT_MODE)
		*p++ = '2';
	if(mode & RAC_FOURSHOT_MODE)
		*p++ = '4';
	if(mode & RAC_SIXSHOT_MODE)
		*p++ = '6';
	if(mode & WILL_JETTISON_MODE)
		*p++ = 'B';
	if(mode & GATTLING_MODE)
		*p++ = 'G';
	if(mode & HOTLOAD_MODE)
		*p++ = 'H';
	if(mode & RFAC_MODE)
		*p++ = 'R';
	if((mode & ON_TC) && !(MechCritStatus(mech) & TC_DESTROYED))
		*p++ = 'T';
	if(mode & ULTRA_MODE)
		*p++ = 'U';
	if(mode & HEAT_MODE)
		*p++ = 'h';

	if(mode & (OS_USED | ROCKET_FIRED))
		*p++ = 'o';
	else if((mode & OS_MODE) || (MechWeapons[type].special & ROCKET))
		*p++ = 'O';

	if(MechWeapons[type].special & INARC)
		*p++ = 'I';
	if(MechWeapons[type].special & NARC)
		*p++ = 'N';
	if(MechWeapons[type].special & STREAK)
		*p++ = 'S';

	if(MechWeapons[type].special & AMS) {
		if(MechStatus(mech) & AMS_ENABLED)
			*p++ = 'A';
		else
			*p++ = 'a';
	}

	/* XXX Do enhanced damage */

	if(p == wmode)
		*p++ = '-';

	*p = '\0';
	return wmode;
}

static char *hud_getammomode(MECH * mech, int mode)
{
	static char amode[20];
	char *p = amode;

	if(mode & SWARM1_MODE)
		*p++ = '1';
	if(mode & ARTEMIS_MODE)
		*p++ = 'A';
	if(mode & CLUSTER_MODE)
		*p++ = 'C';
	if(mode & INARC_ECM_MODE)
		*p++ = 'E';
	if(mode & AC_FLECHETTE_MODE)
		*p++ = 'F';
	if(mode & INARC_HAYWIRE_MODE)
		*p++ = 'H';
	if(mode & INFERNO_MODE)
		*p++ = 'I';
	if(mode & LBX_MODE)
		*p++ = 'L';
	if(mode & MINE_MODE)
		*p++ = 'M';
	if(mode & NARC_MODE)
		*p++ = 'N';
	if(mode & AC_PRECISION_MODE)
		*p++ = 'P';
	if(mode & SWARM_MODE)
		*p++ = 'S';
	if(mode & AC_AP_MODE)
		*p++ = 'a';
	if(mode & INARC_EXPLO_MODE)
		*p++ = 'X';
	if(mode & AC_INCENDIARY_MODE)
		*p++ = 'e';
	if(mode & SMOKE_MODE)
		*p++ = 's';
	if(mode & STINGER_MODE)
		*p++ = 'T';
	if(mode & AC_CASELESS_MODE)
		*p++ = 'U';

	if(p == amode)
		*p++ = '-';
	*p = '\0';
	return amode;
}

static void hud_weapons(DESC * d, MECH * mech, char *msgclass, char *args)
{
	int sect, weapcount, i, weapnum = -1;
	unsigned char weaparray[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	int critical[MAX_WEAPS_SECTION];
	char response[LBUF_SIZE];

	UpdateRecycling(mech);

	for(sect = 0; sect < NUM_SECTIONS; sect++) {
		weapcount = FindWeapons(mech, sect, weaparray, weapdata, critical);
		if(weapcount <= 0)
			continue;
		for(i = 0; i < weapcount; i++) {
			weapnum++;
			sprintf(response, "%d,%d,%d,%s%s,%s,%s,%s",
					weapnum,
					weaparray[i],
					GetPartBrand(mech, sect, critical[i]),
					ShortArmorSectionString(MechType(mech), MechMove(mech),
											sect), GetPartFireMode(mech, sect,
																   critical
																   [i]) &
					REAR_MOUNT ? "r" : "", hud_getweaponstatus(mech, sect,
															   critical[i],
															   weapdata[i]),
					hud_getfiremode(mech, sect, critical[i], weaparray[i]),
					hud_getammomode(mech,
									GetPartAmmoMode(mech, sect,
													critical[i])));
			hudinfo_notify(d, msgclass, "L", response);
		}
	}
	hudinfo_notify(d, msgclass, "D", "Done");
}

static int get_weaponmodes(int weapindx, char *firemodes, char *ammomodes,
						   char *damagetype)
{
	int spec = MechWeapons[weapindx].special;

	if(spec & PCOMBAT)
		return 0;

	switch (MechWeapons[weapindx].type) {
	case TMISSILE:
		strcat(damagetype, "M");
		if(spec & AMS) {
			strcat(damagetype, "d");
			strcat(firemodes, "Aa");
			break;
		}
		if(spec & INARC) {
			strcat(firemodes, "I");
			strcat(ammomodes, "EHe");
			break;
		}
		if(spec & NARC) {
			strcat(firemodes, "N");
			break;
		}
		if(spec & IDF) {		/* LRM */
			strcat(ammomodes, "1ACMNSs");
			strcat(damagetype, "g");
			strcat(firemodes, "Hi");
		} else if(spec & MRM)	/* MRM */
			strcat(damagetype, "g");
		else					/* SRM/SR_DFM */
			strcat(ammomodes, "AIN");
		if(spec & DFM)
			strcat(damagetype, "D");
		if(spec & ELRM)
			strcat(damagetype, "e");
		if(spec & STREAK)
			strcat(firemodes, "S");
		break;
	case TAMMO:
		if(spec & GAUSS) {
			strcat(damagetype, "G");
			if(spec & HVYGAUSS)
				strcat(damagetype, "H");
			break;
		}
		strcat(damagetype, "B");

		if(spec & CASELESS)
			strcat(damagetype, "C");
		if(spec & HYPER)
			strcat(damagetype, "Y");
		if(spec & RAC)
			strcat(firemodes, "246");
		if(spec & GMG)
			strcat(firemodes, "G");
		if(spec & RFAC) {
			strcat(firemodes, "R");
			strcat(ammomodes, "FPai");
		}
		if(spec & LBX)
			strcat(ammomodes, "L");
		if(spec & ULTRA)
			strcat(firemodes, "U");
		break;
	case TARTILLERY:
		strcat(damagetype, "Ag");
		strcat(firemodes, "Hi");
		strcat(ammomodes, "CMs");
		break;
	case TBEAM:
		strcat(damagetype, "E");
		if(spec & HVYW)
			strcat(damagetype, "h");
		if(spec & PULSE)
			strcat(damagetype, "p");
		if(spec & CHEAT)
			strcat(firemodes, "h");
		if(spec & A_POD)
			strcat(damagetype, "a");
		break;
	}
	if(spec & NOSPA)
		ammomodes[0] = '\0';
	return 1;
}

static void hud_weaponlist(DESC * d, MECH * mech, char *msgclass, char *args)
{
	int i;
	char firemodes[30] = "";
	char ammomodes[20] = "";
	char damagetype[10] = "";
	char response[LBUF_SIZE];
	struct weapon_struct *w;

	for(i = 0; i < num_def_weapons; i++) {
		firemodes[0] = ammomodes[0] = damagetype[0] = '\0';
		if(!get_weaponmodes(i, firemodes, ammomodes, damagetype))
			continue;

		if(strlen(firemodes) == 0)
			strcat(firemodes, "-");
		if(strlen(ammomodes) == 0)
			strcat(ammomodes, "-");

		w = &MechWeapons[i];
		sprintf(response,
				"%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%s,%d", i,
				w->name, w->min, w->shortrange, w->medrange, w->longrange,
				w->min_water, w->shortrange_water, w->medrange_water,
				w->longrange_water, w->criticals, w->weight, w->damage,
				w->vrt, firemodes, ammomodes, damagetype, w->heat * 10);
		hudinfo_notify(d, msgclass, "L", response);
	}

	hudinfo_notify(d, msgclass, "D", "Done");
}

static void hud_limbstatus(DESC * d, MECH * mech, char *msgclass, char *args)
{

	int locs[] = { RLEG, LLEG, RARM, LARM };
	int todo = 3;

	UpdateRecycling(mech);

	if(MechType(mech) == CLASS_MECH) {
		for(; todo >= 0; todo--) {
			char *sect = ShortArmorSectionString(MechType(mech),
												 MechMove(mech), locs[todo]);
			char status[10];

			if(SectIsDestroyed(mech, locs[todo]))
				strcpy(status, "*");
			else if(MechSections(mech)[locs[todo]].recycle > 0)
				sprintf(status, "%d",
						MechSections(mech)[locs[todo]].recycle / WEAPON_TICK);
			else
				strcpy(status, "R");

			hudinfo_notify(d, msgclass, "L", tprintf("%s,%s", sect, status));
		}
	}
	hudinfo_notify(d, msgclass, "D", "Done");
}

static void hud_ammostatus(DESC * d, MECH * mech, char *msgclass, char *args)
{
	unsigned char weapnums[8 * MAX_WEAPS_SECTION];
	unsigned short curamm[8 * MAX_WEAPS_SECTION];
	unsigned short maxamm[8 * MAX_WEAPS_SECTION];
	unsigned int ammomode[8 * MAX_WEAPS_SECTION];
	int i, ammonum;
	char response[LBUF_SIZE];

	ammonum = FindAmmunition(mech, weapnums, curamm, maxamm, ammomode);

	for(i = 0; i < ammonum; i++) {
		sprintf(response, "%d,%d,%s,%d,%d", i, weapnums[i],
				hud_getammomode(mech, ammomode[i]), curamm[i], maxamm[i]);
		hudinfo_notify(d, msgclass, "L", response);
	}
	hudinfo_notify(d, msgclass, "D", "Done");
}

static char hud_typechar(MECH * mech)
{

	if(MechMove(mech) == MOVE_NONE)
		return 'i';

	switch (MechType(mech)) {
	case CLASS_MECH:
		if(MechMove(mech) == MOVE_QUAD)
			return 'Q';
		return 'B';
	case CLASS_VEH_GROUND:
	case CLASS_VEH_NAVAL:
		switch (MechMove(mech)) {
		case MOVE_TRACK:
			return 'T';
		case MOVE_WHEEL:
			return 'W';
		case MOVE_HOVER:
			return 'H';
		case MOVE_HULL:
			return 'N';
		case MOVE_FOIL:
			return 'Y';
		case MOVE_SUB:
			return 'U';
		default:
			SendError(tprintf("Unknown movement type on vehicle #%d: %d",
							  mech->mynum, MechMove(mech)));
			return '?';
		}
	case CLASS_VTOL:
		return 'V';
	case CLASS_AERO:
		return 'F';
	case CLASS_DS:
		return 'A';
	case CLASS_SPHEROID_DS:
		return 'D';
	case CLASS_MW:
		return 'I';
	case CLASS_BSUIT:
		return 'S';
	}
	SendError(tprintf("Unknown unit type on unit #%d: %d (move %d)",
					  mech->mynum, MechType(mech), MechMove(mech)));
	return '?';
}

static float MaxSettableSpeed(MECH * mech)
{
	int maxspeed = MMaxSpeed(mech);

	if(MechMove(mech) == MOVE_VTOL)
		maxspeed = sqrt((float) maxspeed * maxspeed -
						MechVerticalSpeed(mech) * MechVerticalSpeed(mech));

	maxspeed = maxspeed > 0.0 ? maxspeed : 0.0;

	if((MechSpecials(mech) & TRIPLE_MYOMER_TECH) && MechHeat(mech) >= 9.)
		maxspeed = ceil((rint((maxspeed / 1.5) / MP1) + 1) * 1.5) * MP1;
	return maxspeed;
}

static float MaxVerticalSpeed(MECH * mech)
{
	int maxspeed = MMaxSpeed(mech);

	if(MechMove(mech) != MOVE_VTOL)
		return 0.0;

	maxspeed = sqrt((float) maxspeed * maxspeed -
					MechDesiredSpeed(mech) * MechDesiredSpeed(mech));
	return maxspeed;
}

static char *hud_advtech(MECH * mech)
{
	static char advtech[30];
	char *p = advtech;
	int spec = MechSpecials(mech);

	if(spec & ECM_TECH)
		*p++ = 'E';
	if(spec & BEAGLE_PROBE_TECH)
		*p++ = 'B';
	if(spec & MASC_TECH)
		*p++ = 'M';
	if(spec & AA_TECH)
		*p++ = 'R';
	if(spec & SLITE_TECH)
		*p++ = 'S';
	if(spec & TRIPLE_MYOMER_TECH)
		*p++ = 't';

	spec = MechSpecials2(mech);
	if(spec & ANGEL_ECM_TECH)
		*p++ = 'A';
	if(spec & NULLSIGSYS_TECH)
		*p++ = 'N';
	if(spec & BLOODHOUND_PROBE_TECH)
		*p++ = 'b';
	if(spec & STEALTH_ARMOR_TECH)
		*p++ = 's';

	spec = MechInfantrySpecials(mech);
	if(spec & FC_INFILTRATORII_STEALTH_TECH) {
		*p++ = 'I';
		*p++ = 'P';
	}
	if(spec & CAN_JETTISON_TECH)
		*p++ = 'J';
	if(spec & DC_KAGE_STEALTH_TECH)
		*p++ = 'K';
	if(spec & INF_ANTILEG_TECH)
		*p++ = 'L';
	if(spec & INF_SWARM_TECH)
		*p++ = 'W';
	if(spec & FWL_ACHILEUS_STEALTH_TECH)
		*p++ = 'a';
	if(spec & INF_MOUNT_TECH)
		*p++ = 'f';
	if(spec & FC_INFILTRATOR_STEALTH_TECH)
		*p++ = 'i';
	if(spec & MUST_JETTISON_TECH)
		*p++ = 'j';
	if(spec & CS_PURIFIER_STEALTH_TECH)
		*p++ = 'p';

	if(HasC3i(mech))
		*p++ = 'C';
	if(HasTAG(mech))
		*p++ = 'T';
	if(HasC3s(mech))
		*p++ = 'c';
	if(HasC3m(mech))
		*p++ = 'm';

	if(p == advtech)
		*p++ = '-';

	*p = '\0';
	return advtech;
}

static void hud_templateinfo(DESC * d, MECH * mech, char *msgclass,
							 char *args)
{
	char response[LBUF_SIZE];
	char fuel[20];

	if(FlyingT(mech) && AeroFuelOrig(mech))
		sprintf(fuel, "%d", AeroFuelOrig(mech));
	else
		strcpy(fuel, "-");

	sprintf(response, "%c,%s,%s,%.3f,%.3f,%.3f,%.3f,%s,%d,%s",
			hud_typechar(mech), MechType_Ref(mech), MechType_Name(mech),
			WalkingSpeed(MaxSettableSpeed(mech)), MaxSettableSpeed(mech),
			-WalkingSpeed(MaxSettableSpeed(mech)),
			MaxVerticalSpeed(mech), fuel, MechRealNumsinks(mech),
			hud_advtech(mech));
	hudinfo_notify(d, msgclass, "R", response);
}

static void hud_templatearmor(DESC * d, MECH * mech, char *msgclass,
							  char *args)
{
	char response[LBUF_SIZE];
	int sect;

	for(sect = 0; sect < NUM_SECTIONS; sect++) {
		if(GetSectOInt(mech, sect)) {
			sprintf(response, "%s,%d,%d,%d",
					ShortArmorSectionString(MechType(mech), MechMove(mech),
											sect), GetSectOArmor(mech, sect),
					GetSectORArmor(mech, sect), GetSectOInt(mech, sect));
			hudinfo_notify(d, msgclass, "L", response);
		}
	}
	hudinfo_notify(d, msgclass, "D", "Done");
}

static void hud_armorstatus(DESC * d, MECH * mech, char *msgclass, char *args)
{
	char response[LBUF_SIZE];
	int sect;

	for(sect = 0; sect < NUM_SECTIONS; sect++) {
		if(GetSectOInt(mech, sect)) {
			sprintf(response, "%s,%d,%d,%d",
					ShortArmorSectionString(MechType(mech), MechMove(mech),
											sect), GetSectArmor(mech, sect),
					GetSectRArmor(mech, sect), GetSectInt(mech, sect));
			hudinfo_notify(d, msgclass, "L", response);
		}
	}
	hudinfo_notify(d, msgclass, "D", "Done");
}

static char *hud_arcstring(int arc)
{
	static char arcstr[5];
	char *p = arcstr;

	if(arc & FORWARDARC)
		*p++ = '*';
	if(arc & LSIDEARC)
		*p++ = 'l';
	if(arc & RSIDEARC)
		*p++ = 'r';
	if(arc & REARARC)
		*p++ = 'v';
	if(arc & TURRETARC)
		*p++ = 't';

	if(p == arcstr)
		*p++ = '-';

	*p = '\0';
	return arcstr;
}

static char *hud_sensorstring(MECH * mech, int losflag)
{
	if((losflag & MECHLOSFLAG_SEESP) && (losflag & MECHLOSFLAG_SEESS))
		return "PS";
	if(losflag & MECHLOSFLAG_SEESP)
		return "P";
	if(losflag & MECHLOSFLAG_SEESS)
		return "S";
	return "-";
}

static void hud_contacts(DESC * d, MECH * mech, char *msgclass, char *args)
{
	char response[LBUF_SIZE];
	MECH *other;
	MAP *map = getMap(mech->mapindex);
	int i, losflag, bearing, btc, weaponarc, x, y;
	float range, rtc;
	char jumph[12];
	char *mechname, *constat;

	if(!map) {
		hudinfo_notify(d, msgclass, "E", "You are on no map");
		return;
	}

	for(i = 0; i < map->first_free; i++) {
		if(map->mechsOnMap[i] == mech->mynum || map->mechsOnMap[i] == -1)
			continue;

		other = (MECH *) FindObjectsData(map->mechsOnMap[i]);
		if(!other || !Good_obj(other->mynum))
			continue;

		range = FlMechRange(map, mech, other);
		x = MechX(other);
		y = MechY(other);
		losflag = InLineOfSight(mech, other, x, y, range);
		if(!losflag)
			continue;
		bearing = FindBearing(MechFX(mech), MechFY(mech), MechFX(other),
							  MechFY(other));
		weaponarc = InWeaponArc(mech, MechFX(other), MechFY(other));

		if(Jumping(other))
			sprintf(jumph, "%d", MechJumpHeading(other));
		else
			strcpy(jumph, "-");

		FindRangeAndBearingToCenter(other, &rtc, &btc);

		if(!InLineOfSight_NB(mech, other, MechX(other), MechY(other), 0.0))
			mechname = "something";
		else
			mechname = silly_atr_get(other->mynum, A_MECHNAME);

		if(!mechname || !*mechname)
			mechname = "-";

		constat = getStatusString(other, !MechSeemsFriend(mech, other));
		if(strlen(constat) == 0)
			constat = "-";

		sprintf(response,
				"%s,%s,%s,%c,%s,%d,%d,%d,%.3f,%d,%.3f,%.3f,%d,%s,%.3f,%d,%d,%.0f,%s",
				MechIDS(other, MechSeemsFriend(mech, other)),
				hud_arcstring(weaponarc), hud_sensorstring(mech, losflag),
				hud_typechar(other), mechname, MechX(other),
				MechY(other), MechZ(other), range, bearing, MechSpeed(other),
				MechVerticalSpeed(other), MechVFacing(other), jumph, rtc, btc,
				MechTons(other), MechHeat(other), constat);
		hudinfo_notify(d, msgclass, "L", response);
	}
	hudinfo_notify(d, msgclass, "D", "Done");
}

static char *building_status(MAP * map, int locked)
{
	static char buildingstatus[7];
	char *p = buildingstatus;

	if(BuildIsCS(map))
		*p++ = 'C';

	if(locked)
		*p++ = 'E';
	else
		*p++ = 'F';

	if(BuildIsHidden(map))
		*p++ = 'H';

	if(BuildIsSafe(map) || (locked && BuildIsCS(map)))
		*p++ = 'X';
	else if(locked)
		*p++ = 'x';

	if(p == buildingstatus)
		*p++ = '-';

	*p = '\0';

	return buildingstatus;
}

static void hud_building_contacts(DESC * d, MECH * mech, char *msgclass,
								  char *args)
{
	char response[LBUF_SIZE];
	MAP *map = getMap(mech->mapindex);
	MAP *building_map;
	char *building_name;
	mapobj *building;
	float fx, fy, range;
	int z, losflag, locked, bearing, weaponarc;

	if(!map) {
		hudinfo_notify(d, msgclass, "E", "You are on no map");
		return;
	}

	for(building = first_mapobj(map, TYPE_BUILD); building;
		building = next_mapobj(building)) {

		MapCoordToRealCoord(building->x, building->y, &fx, &fy);
		z = Elevation(map, building->x, building->y) + 1;
		range = FindRange(MechFX(mech), MechFY(mech), MechFZ(mech), fx, fy,
						  ZSCALE * z);

		if(!building->obj)
			continue;

		building_map = getMap(building->obj);
		if(!building_map)
			continue;

		losflag = InLineOfSight(mech, NULL, building->x, building->y, range);
		if(!losflag || (losflag & MECHLOSFLAG_BLOCK))
			continue;

		if(BuildIsInvis(building_map))
			continue;

		locked = !can_pass_lock(mech->mynum, building->obj, A_LENTER);
		if(locked && BuildIsHidden(building_map))
			continue;

		bearing = FindBearing(MechFX(mech), MechFY(mech), fx, fy);
		weaponarc = InWeaponArc(mech, fx, fy);
		building_name = silly_atr_get(building->obj, A_MECHNAME);
		if(!building_name || !*building_name)
			building_name = strip_ansi(Name(building->obj));

		if(!building_name || !*building_name)
			building_name = "-";

		sprintf(response, "%s,%s,%d,%d,%d,%f,%d,%d,%d,%s",
				hud_arcstring(weaponarc), building_name, building->x,
				building->y, z, range, bearing, building_map->cf,
				building_map->cfmax, building_status(building_map, locked));
		hudinfo_notify(d, msgclass, "L", response);
	}
	hudinfo_notify(d, msgclass, "D", "Done");

};

static char hud_damstr[] = "OoxX*?";
static char hud_damagechar(MECH * mech, int sect, int type)
{
	int dummy;

	switch (type) {
	case 1:
		if(GetSectOArmor(mech, sect))
			return hud_damstr[ArmorEvaluateSerious(mech, sect, 1, &dummy)];
		return '-';
	case 2:
		if(GetSectOInt(mech, sect))
			return hud_damstr[ArmorEvaluateSerious(mech, sect, 2, &dummy)];
		return '-';
	case 4:
		if(GetSectORArmor(mech, sect))
			return hud_damstr[ArmorEvaluateSerious(mech, sect, 4, &dummy)];
		return '-';
	}
	return '?';
}

static MECH *hud_scantarget(DESC * d, MECH * mech, char *msgclass, char *args)
{
	MECH *targ;
	float range;

	if(!MechScanRange(mech)) {
		hudinfo_notify(d, msgclass, "E",
					   "Your system seems to be inoperational");
		return NULL;
	}

	if(strlen(args) != 2 ||
	   !(targ = getMech(FindTargetDBREFFromMapNumber(mech, args)))) {
		hudinfo_notify(d, msgclass, "E", "No such target");
		return NULL;
	}

	range = FaMechRange(mech, targ);

	if(!InLineOfSight(mech, targ, MechX(targ), MechY(targ), range)) {
		hudinfo_notify(d, msgclass, "E", "No such target");
		return NULL;
	}

	if(!MechIsObservator(mech) && (int) range > MechScanRange(mech)) {
		hudinfo_notify(d, msgclass, "E", "Out of range");
		return NULL;
	}

	if(MechType(targ) == CLASS_MW ||
	   !InLineOfSight_NB(mech, targ, MechX(targ), MechY(targ), range)) {
		hudinfo_notify(d, msgclass, "E", "Unable to scan target");
		return NULL;
	}

	if(!MechIsObservator(mech)) {
		mech_notify(targ, MECHSTARTED,
					tprintf("You are being scanned by %s",
							GetMechToMechID(targ, mech)));
		auto_reply(targ, tprintf("%s just scanned me.",
								 GetMechToMechID(targ, mech)));
	}
	return targ;
}

static void hud_armorscan(DESC * d, MECH * mech, char *msgclass, char *args)
{
	char response[LBUF_SIZE];
	int sect;
	MECH *targ;

	targ = hud_scantarget(d, mech, msgclass, args);
	if(!targ)
		return;

	for(sect = 0; sect < NUM_SECTIONS; sect++) {
		if(GetSectOInt(targ, sect)) {
			sprintf(response, "%s,%c,%c,%c",
					ShortArmorSectionString(MechType(targ), MechMove(targ),
											sect), hud_damagechar(targ, sect,
																  1),
					hud_damagechar(targ, sect, 4), hud_damagechar(targ, sect,
																  2));
			hudinfo_notify(d, msgclass, "L", response);
		}
	}
	hudinfo_notify(d, msgclass, "D", "Done");
}

static char *hud_getweapscanstatus(MECH * mech, int sect, int crit, int data)
{
	if(PartIsNonfunctional(mech, sect, crit))
		return "*";
	if(data)
		return "-";
	return "R";
}

static void hud_weapscan(DESC * d, MECH * mech, char *msgclass, char *args)
{
	int sect, weapcount, i, weapnum = -1;
	unsigned char weaparray[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	int critical[MAX_WEAPS_SECTION];
	char response[LBUF_SIZE];
	MECH *targ;

	targ = hud_scantarget(d, mech, msgclass, args);
	if(!targ)
		return;

	UpdateRecycling(targ);

	for(sect = 0; sect < NUM_SECTIONS; sect++) {
		if(SectIsDestroyed(targ, sect))
			continue;
		weapcount = FindWeapons(targ, sect, weaparray, weapdata, critical);
		if(weapcount <= 0)
			continue;
		for(i = 0; i < weapcount; i++) {
			weapnum++;
			sprintf(response, "%d,%d,%s,%s", weapnum, weaparray[i],
					ShortArmorSectionString(MechType(targ), MechMove(targ),
											sect), hud_getweapscanstatus(targ,
																		 sect,
																		 critical
																		 [i],
																		 weapdata
																		 [i] /
																		 WEAPON_TICK));
			hudinfo_notify(d, msgclass, "L", response);
		}
	}
	hudinfo_notify(d, msgclass, "D", "Done");
}

static void hud_tactical(DESC * d, MECH * mech, char *msgclass, char *args)
{
	MAP *map = getMap(mech->mapindex);
	char *argv[5], result[LBUF_SIZE], *p;
	char mapid[21], mapname[MBUF_SIZE];
	int argc;
	int height = 24;
	int cx = MechX(mech), cy = MechY(mech);
	int sx, sy, ex, ey, x, y, losflag, lostactical = 0;
	hexlosmap_info *losmap = NULL;

	if(!MechLRSRange(mech)) {
		hudinfo_notify(d, msgclass, "E",
					   "Your system seems to be inoperational");
		return;
	}

	if(!map) {
		hudinfo_notify(d, msgclass, "E", "You are on no map");
		return;
	}

	argc = mech_parseattributes(args, argv, 4);

	switch (argc) {
	case 4:
		if(strcmp("l", argv[3])) {
			hudinfo_notify(d, msgclass, "E", "Invalid fourth argument");
			return;
		}
		lostactical = 1;
		/* FALLTHROUGH */
	case 3:{
			float fx, fy;
			int bearing = atoi(argv[1]);
			float range = atof(argv[2]);

			if(!MechIsObservator(mech) &&
			   abs((int) range) > MechLRSRange(mech)) {
				hudinfo_notify(d, msgclass, "E", "Out of range");
				return;
			}
			FindXY(MechFX(mech), MechFY(mech), bearing, range, &fx, &fy);
			RealCoordToMapCoord(&cx, &cy, fx, fy);
		}
		/* FALLTHROUGH */
	case 1:
		height = atoi(argv[0]);
		if(!height || height < 0 || height > 40) {
			hudinfo_notify(d, msgclass, "E", "Invalid 1st argument");
			return;
		}
		break;
	default:
		hudinfo_notify(d, msgclass, "E", "Invalid arguments");
		return;
	}

	height = MIN(height, 2 * MechLRSRange(mech));
	height = MIN(height, map->height);

	sy = MAX(0, cy - height / 2);
	ey = MIN(map->height, cy + height / 2);

	sx = MAX(0, cx - LRS_DISPLAY_WIDTH / 2);
	ex = MIN(map->width, cx + LRS_DISPLAY_WIDTH / 2);

	if(lostactical || MapIsDark(map) || (MechType(mech) == CLASS_MW &&
										 mudconf.btech_mw_losmap))
		losmap = CalculateLOSMap(map, mech, sx, sy, ex - sx, ey - sy);

	if(!mudconf.hudinfo_show_mapinfo ||
	   (mudconf.hudinfo_show_mapinfo == 1 && In_Character(map->mynum))) {
		strcpy(mapid, "-1");
		strcpy(mapname, "-1");
	} else {
		sprintf(mapid, "%d", map->mynum);
		sprintf(mapname, "%s", map->mapname);
	};

	sprintf(result, "%d,%d,%d,%d,%s,%s,-1,%d,%d", sx, sy, ex - 1, ey - 1,
			mapid, mapname, map->width, map->height);
	hudinfo_notify(d, msgclass, "S", result);

	for(y = sy; y < ey; y++) {
		sprintf(result, "%d,", y);
		p = result + strlen(result);

		for(x = sx; x < ex; x++) {
			if(losmap)
				losflag = LOSMap_GetFlag(losmap, x, y);
			else
				losflag = MAPLOSHEX_SEE;

			if(losflag & MAPLOSHEX_SEETERRAIN) {
				*p = GetTerrain(map, x, y);
				if(*p == ' ')
					*p = '.';
				*p++;
			} else
				*p++ = '?';

			if(losflag & MAPLOSHEX_SEEELEV)
				*p++ = GetElev(map, x, y) + '0';
			else
				*p++ = '?';
		}
		*p = '\0';
		hudinfo_notify(d, msgclass, "L", result);
	}

	hudinfo_notify(d, msgclass, "D", "Done");
}

static char *hud_getmapflags(MAP * map)
{
	static char res[5];
	char *p = res;

	if(map->flags & MAPFLAG_VACUUM)
		*p++ = 'V';
	if(map->flags & MAPFLAG_UNDERGROUND)
		*p++ = 'U';
	if(map->flags & MAPFLAG_DARK)
		*p++ = 'D';

	if(p == res)
		*p++ = '-';

	*p = '\0';
	return res;
}

static void hud_conditions(DESC * d, MECH * mech, char *msgclass, char *args)
{
	MAP *map = getMap(mech->mapindex);
	char res[200];
	char lt;

	switch (map->maplight) {
	case 0:
		lt = 'N';
		break;
	case 1:
		lt = 'T';
		break;
	case 2:
		lt = 'D';
		break;
	default:
		lt = '?';
		SendError(tprintf("Unknown light type %d on map #%d",
						  map->maplight, map->mynum));
		break;
	}

	sprintf(res, "%c,%d,%d,%d,%s", lt, map->mapvis, map->grav,
			map->temp, hud_getmapflags(map));
	hudinfo_notify(d, msgclass, "R", res);
}

#endif
