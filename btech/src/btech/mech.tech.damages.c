
/*
 * $Id: mech.tech.damages.c,v 1.1.1.1 2005/01/11 21:18:25 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Mon Dec  2 19:00:35 1996 fingon
 * Last modified: Thu Sep 10 09:55:13 1998 fingon
 *
 */

#include "mech.h"
#include "mech.events.h"
#include "mech.tech.h"
#include "mech.tech.damages.h"
#include "coolmenu.h"
#include "mycool.h"
#include "failures.h"
#include "p.mech.utils.h"
#include "p.mech.tech.commands.h"
#include "p.mech.status.h"
#include "p.mech.build.h"

/* 0 = type, 1 = loc, 2 = (pos/amnt), 3 = (val) */
short damage_table[MAX_DAMAGES][3];
int damage_last;

const char *repair_need_msgs[] = {
	"Reattachment",
	"Repairs on %s",
	"Repairs on %s",
	"Repairs on %s",
	"Realign focus on %s",
	"Charging crystal repairs on %s",
	"Barrel repairs on %s",
	"Ammo feed repairs on %s",
	"Ranging system repairs on %s",
	"Ammo feed repairs on %s",
	"Replacement of %s",
	"Reload of %s%s (%d rounds)",
	"Repairs on%s armor (%d points)",
	"Repairs on rear%s armor (%d points)",
	"Repairs on%s internals (%d points)",
	"Removal of section",
	"Removal of %s",
	"Removal of %s",
	"Unload of %s%s(%d rounds)",
	"Reseal",
	"Replace suit",
};

#define CHECK(loc) check_for_damage(mech,loc)
#define DAMAGE2(a,b) do {\
   damage_table[damage_last][0]=a;\
   damage_table[damage_last++][1]=b;\
 } while (0)
#define DAMAGE3(a,b,c) do {\
   damage_table[damage_last][0]=a;\
   damage_table[damage_last][1]=b;\
   damage_table[damage_last++][2]=c;\
 } while (0)

#define ClanMod(num) \
  MAX(1, (((num) / ((MechSpecials(mech) & CLAN_TECH) ? 2 : 1))))

static int check_for_damage(MECH * mech, int loc)
{
	int a, b, c, d;

	if(SectIsDestroyed(mech, loc)) {
		if(MechType(mech) != CLASS_BSUIT)
			DAMAGE2(REATTACH, loc);
		else
			DAMAGE2(REPLACESUIT, loc);
		return 0;
	}

	/*
	 * Added by Kipsta
	 * 8/4/99
	 */

	if(SectIsFlooded(mech, loc)) {
		DAMAGE2(RESEAL, loc);
		return 0;
	}
	if((a = GetSectInt(mech, loc)) != (b = GetSectOInt(mech, loc)))
		DAMAGE3(FIXINTERNAL, loc, (b - a));
	else {
		if((a = GetSectArmor(mech, loc)) != (b = GetSectOArmor(mech, loc)))
			DAMAGE3(FIXARMOR, loc, (b - a));
		if((a = GetSectRArmor(mech, loc)) != (b = GetSectORArmor(mech, loc)))
			DAMAGE3(FIXARMOR_R, loc, (b - a));
	}
	for(a = 0; a < NUM_CRITICALS; a++) {
		if(!(b = GetPartType(mech, loc, a)))
			continue;
		if(IsAmmo(b) && !PartIsDestroyed(mech, loc, a) &&
		   (c = GetPartData(mech, loc, a)) != (d = FullAmmo(mech, loc, a)))
			DAMAGE3(RELOAD, loc, a);
		if(!PartIsNonfunctional(mech, loc, a) &&
		   !PartTempNuke(mech, loc, a) && !PartIsDamaged(mech, loc, a))
			continue;
		if(IsCrap(b))
			continue;
		/* Destroyed / tempnuke'd part. Either case, it works for us :) */

		if(PartIsDamaged(mech, loc, a)) {
			if(GetPartDamageFlags(mech, loc, a) & WEAP_DAM_EN_FOCUS)
				DAMAGE3(ENHCRIT_FOCUS, loc, a);
			else if(GetPartDamageFlags(mech, loc, a) & WEAP_DAM_EN_CRYSTAL)
				DAMAGE3(ENHCRIT_CRYSTAL, loc, a);
			else if(GetPartDamageFlags(mech, loc, a) & WEAP_DAM_BALL_BARREL)
				DAMAGE3(ENHCRIT_BARREL, loc, a);
			else if(GetPartDamageFlags(mech, loc, a) & WEAP_DAM_BALL_AMMO)
				DAMAGE3(ENHCRIT_AMMOB, loc, a);
			else if(GetPartDamageFlags(mech, loc, a) & WEAP_DAM_MSL_RANGING)
				DAMAGE3(ENHCRIT_RANGING, loc, a);
			else if(GetPartDamageFlags(mech, loc, a) & WEAP_DAM_MSL_AMMO)
				DAMAGE3(ENHCRIT_AMMOM, loc, a);
			else
				DAMAGE3(ENHCRIT_MISC, loc, a);

		} else if(IsWeapon(b) && !PartIsDestroyed(mech, loc, a))
			DAMAGE3(REPAIRP_T, loc, a);
		else
			DAMAGE3(IsWeapon(b) ? REPAIRG : REPAIRP, loc, a);

		if(IsWeapon(b))
			a += GetWeaponCrits(mech, Weapon2I(b)) - 1;
	}
	return 1;
}

static int check_for_scrappage(MECH * mech, int loc)
{
	int a, b;
	int ret = 1;

	if(SectIsDestroyed(mech, loc))
		return 1;

	if(SomeoneScrappingLoc(mech, loc)) {
		DAMAGE2(DETACH, loc);
		return 1;
	}
	for(a = 0; a < NUM_CRITICALS; a++) {
		if(!(b = GetPartType(mech, loc, a)))
			continue;
		if(PartIsBroken(mech, loc, a))
			continue;
		if(IsCrap(b))
			continue;
		if(IsAmmo(b) && GetPartData(mech, loc, a)) {
			DAMAGE3(UNLOAD, loc, a);
			if(ret && !SomeoneRepairing(mech, loc, a))
				ret = 0;
			continue;
		}
		DAMAGE3(IsWeapon(b) ? SCRAPG : SCRAPP, loc, a);
		if(ret && !SomeoneScrappingPart(mech, loc, a))
			ret = 0;
		if(IsWeapon(b))
			a += GetWeaponCrits(mech, Weapon2I(b)) - 1;
	}

	if(ret && !Invalid_Scrap_Path(mech, loc))
		DAMAGE2(DETACH, loc);

	return 0;
}

void make_scrap_table(MECH * mech)
{
	int i = 4;

	damage_last = 0;
	if(MechType(mech) == CLASS_MECH) {
		if(check_for_scrappage(mech, RARM))
			i -= check_for_scrappage(mech, RTORSO);
		if(check_for_scrappage(mech, LARM))
			i -= check_for_scrappage(mech, LTORSO);
		i -= check_for_scrappage(mech, RLEG);
		i -= check_for_scrappage(mech, LLEG);

		if(!i)
			check_for_scrappage(mech, CTORSO);

		check_for_scrappage(mech, HEAD);
	} else
		for(i = 0; i < NUM_SECTIONS; i++)
			if(GetSectOInt(mech, i))
				check_for_scrappage(mech, i);
}

void make_damage_table(MECH * mech)
{
	int i;

	damage_last = 0;
	if(MechType(mech) == CLASS_MECH) {
		if(check_for_damage(mech, CTORSO)) {
			if(check_for_damage(mech, LTORSO)) {
				CHECK(LARM);
			}
			if(check_for_damage(mech, RTORSO)) {
				CHECK(RARM);
			}
			CHECK(LLEG);
			CHECK(RLEG);
			CHECK(HEAD);
		}
	} else
		for(i = 0; i < NUM_SECTIONS; i++)
			if(GetSectOInt(mech, i))
				check_for_damage(mech, i);
}

int is_under_repair(MECH * mech, int i)
{
	int v1 = damage_table[i][1];
	int v2 = damage_table[i][2];

	switch (damage_table[i][0]) {
	case RELOAD:
	case REPAIRP:
	case REPAIRP_T:
	case REPAIRG:
	case UNLOAD:
	case ENHCRIT_MISC:
	case ENHCRIT_FOCUS:
	case ENHCRIT_CRYSTAL:
	case ENHCRIT_BARREL:
	case ENHCRIT_AMMOB:
	case ENHCRIT_RANGING:
	case ENHCRIT_AMMOM:
		return SomeoneRepairing(mech, v1, v2);
	case REATTACH:
		return SomeoneAttaching(mech, v1);
	case RESEAL:
		return SomeoneResealing(mech, v1);
	case FIXARMOR_R:
		return SomeoneFixing(mech, v1 + 8);
	case FIXARMOR:
	case FIXINTERNAL:
		return SomeoneFixing(mech, v1);
	case DETACH:
		return SomeoneScrappingLoc(mech, v1);
	case SCRAPP:
	case SCRAPG:
		return SomeoneScrappingPart(mech, v1, v2);
	case REPLACESUIT:
		return SomeoneReplacingSuit(mech, v1);
	}
	return 0;
}

char *damages_func(MECH * mech)
{
/* Give this a 32k buffer instead of 16k. Temporary fix. Think of something more intelligent to make this work better */
	static char buffer[LBUF_SIZE*2];
	int i;

	if(unit_is_fixable(mech))
		make_damage_table(mech);
	else
		make_scrap_table(mech);

	buffer[0] = '\0';
	if(!damage_last)
		return "";
	for(i = 0; i < damage_last; i++) {
		/* Ok... i think we want: */
		/* repairnum|location|typenum|data|fixing? */
		if(i)
			sprintf(buffer, "%s,", buffer);
		sprintf(buffer, "%s%d|%s|%d|", buffer, i + 1,
				ShortArmorSectionString(MechType(mech), MechMove(mech),
										damage_table[i][1]),
				(int) damage_table[i][0]);
		switch (damage_table[i][0]) {
		case REPAIRP:
		case REPAIRP_T:
		case REPAIRG:
		case ENHCRIT_MISC:
		case ENHCRIT_FOCUS:
		case ENHCRIT_CRYSTAL:
		case ENHCRIT_BARREL:
		case ENHCRIT_AMMOB:
		case ENHCRIT_RANGING:
		case ENHCRIT_AMMOM:
		case SCRAPP:
		case SCRAPG:
			sprintf(buffer, "%s%s", buffer, pos_part_name(mech,
														  damage_table[i][1],
														  damage_table[i]
														  [2]));
			break;
		case RELOAD:
			sprintf(buffer, "%s%s:%d", buffer, pos_part_name(mech,
															 damage_table[i]
															 [1],
															 damage_table[i]
															 [2]),
					FullAmmo(mech, damage_table[i][1],
							 damage_table[i][2]) - GetPartData(mech,
															   damage_table[i]
															   [1],
															   damage_table[i]
															   [2]));
			break;
		case UNLOAD:
			sprintf(buffer, "%s%s:%d", buffer, pos_part_name(mech,
															 damage_table[i]
															 [1],
															 damage_table[i]
															 [2]),
					GetPartData(mech, damage_table[i][1], damage_table[i]
								[2]));
			break;
		case FIXARMOR:
		case FIXARMOR_R:
		case FIXINTERNAL:
			sprintf(buffer, "%s%d", buffer, damage_table[i][2]);
			break;
		default:
			sprintf(buffer, "%s-", buffer);
		}
		sprintf(buffer, "%s|%d", buffer, is_under_repair(mech, i));
	}
	return buffer;
}

void show_mechs_damage(dbref player, void *data, char *buffer)
{
	MECH *mech = data;
	coolmenu *c = NULL;
	int i, j, v1, v2;
	char buf[MBUF_SIZE];
	char buf2[MBUF_SIZE];
	char buf3[MBUF_SIZE];
	int isds;
	int fix_time = 0;
	int fix_bth = 0;
	int extra_hard = 1;

	TECHCOMMANDD;
	if(unit_is_fixable(mech))
		make_damage_table(mech);
	else
		make_scrap_table(mech);
	DOCHECK(!damage_last &&
			MechType(mech) == CLASS_MECH,
			"The 'mech is in pristine condition!");
	DOCHECK(!damage_last, "It's in pristine condition!");
	addline();
	cent(tprintf("Damage for %s", GetMechID(mech)));
	addline();
	vsi("   Fix# Time  BTH Loc Description");
	for(i = 0; i < damage_last; i++) {
		v1 = damage_table[i][1];
		v2 = damage_table[i][2];
		switch (damage_table[i][0]) {
		case REATTACH:
			fix_bth = FindTechSkill(player, mech) + REATTACH_DIFFICULTY;
			fix_time = REATTACH_TIME;
                        strcpy(buf, repair_need_msgs[(int) damage_table[i][0]]);
			break;
		case DETACH:
			fix_bth = FindTechSkill(player, mech) + REMOVES_DIFFICULTY;
			fix_time = REMOVES_TIME;
                        strcpy(buf, repair_need_msgs[(int) damage_table[i][0]]);
			break;
		case RESEAL:
			fix_bth = FindTechSkill(player, mech) + RESEAL_DIFFICULTY;
			fix_time = RESEAL_TIME;
                        strcpy(buf, repair_need_msgs[(int) damage_table[i][0]]);
			break;
		case REPLACESUIT:
			strcpy(buf, repair_need_msgs[(int) damage_table[i][0]]);
			fix_time = REPLACESUIT_TIME;
			fix_bth = FindTechSkill(player, mech) + REPLACESUIT_DIFFICULTY;
			break;
		case REPAIRP:
			fix_bth = FindTechSkill(player, mech) + REPLACE_DIFFICULTY + PARTTYPE_DIFFICULTY(GetPartType(mech, v1, v2));
			fix_time = REPLACEPART_TIME;
                        sprintf(buf, repair_need_msgs[(int) damage_table[i][0]],
                                        pos_part_name(mech, v1, v2));
                        break;
		case REPAIRP_T:	
			if(GetWeaponCrits(mech, Weapon2I(GetPartType(mech, v1, v2))) < 5)
				extra_hard = 0;
                        fix_bth = char_getskilltarget(player, "technician-weapons", 0) + REPLACE_DIFFICULTY + WEAPTYPE_DIFFICULTY(GetPartType(mech,v1,v2)) + extra_hard;
			fix_time = REPAIRGUN_TIME;
                        sprintf(buf, repair_need_msgs[(int) damage_table[i][0]],
                                        pos_part_name(mech, v1, v2));
                        break;
		case REPAIRG:                
			fix_bth = char_getskilltarget(player, "technician-weapons", 0) + REPLACE_DIFFICULTY + WEAPTYPE_DIFFICULTY(GetPartType(mech,v1,v2));
			fix_time = REPLACEGUN_TIME * ClanMod(GetWeaponCrits(mech, Weapon2I(GetPartType(mech, v1, v2))));
                        sprintf(buf, repair_need_msgs[(int) damage_table[i][0]],
                                        pos_part_name(mech, v1, v2));
                        break;
		case ENHCRIT_MISC:
		case ENHCRIT_FOCUS:
		case ENHCRIT_CRYSTAL:
		case ENHCRIT_BARREL:
		case ENHCRIT_AMMOB:
		case ENHCRIT_RANGING:
		case ENHCRIT_AMMOM:
                        fix_bth = char_getskilltarget(player, "technician-weapons", 0) + ENHCRIT_DIFFICULTY;
			fix_time = REPAIRENHCRIT_TIME;
                        sprintf(buf, repair_need_msgs[(int) damage_table[i][0]],
					pos_part_name(mech, v1, v2));
                        break;
		case SCRAPP:
			fix_bth = FindTechSkill(player, mech) + REMOVEP_DIFFICULTY;
			fix_time = REMOVEP_TIME;
			sprintf(buf, repair_need_msgs[(int) damage_table[i][0]],
					pos_part_name(mech, v1, v2));
			break;
		case SCRAPG:
			fix_bth = char_getskilltarget(player, "technician-weapons", 0) + REMOVEG_DIFFICULTY;
			fix_time = REMOVEG_TIME * ClanMod(GetWeaponCrits(mech, Weapon2I(GetPartType(mech, v1, v2))));
			sprintf(buf, repair_need_msgs[(int) damage_table[i][0]],
					pos_part_name(mech, v1, v2));
			break;
		case RELOAD:
			sprintf(buf, repair_need_msgs[(int) damage_table[i][0]],
					pos_part_name(mech, v1, v2), GetPartAmmoMode(mech,v1,v2) ?
					GetAmmoDesc_Model_Mode(Ammo2WeaponI(GetPartType(mech,v1,v2)),GetPartAmmoMode(mech,v1,v2)) : "", 
					FullAmmo(mech, v1, v2) - GetPartData(mech, v1, v2) );
			fix_time = RELOAD_TIME;
			fix_bth = FindTechSkill(player, mech) + RELOAD_DIFFICULTY;
			break;
		case UNLOAD:
			sprintf(buf, repair_need_msgs[(int) damage_table[i][0]],
					pos_part_name(mech, v1, v2), GetPartAmmoMode(mech,v1,v2) ?
					GetAmmoDesc_Model_Mode(Ammo2WeaponI(GetPartType(mech,v1,v2)),GetPartAmmoMode(mech,v1,v2)) : "", 
					GetPartData(mech, v1, v2));
			fix_time = RELOAD_TIME;
			fix_bth = FindTechSkill(player, mech) + REMOVES_DIFFICULTY;
			break;
		case FIXARMOR:
		case FIXARMOR_R:
		case FIXINTERNAL:
			sprintf(buf, repair_need_msgs[(int) damage_table[i][0]],
					damage_table[i][0] == FIXINTERNAL ? 
					((MechSpecials(mech) & ES_TECH) ? " Endosteel" :
					(MechSpecials(mech) & REINFI_TECH) ? " Reinforced" : 
					(MechSpecials(mech) & COMPI_TECH) ? " Composite" :
							            "" ) :
				       ((MechSpecials(mech) & FF_TECH) ? " Ferrofibrous":
				       (MechSpecials(mech) & HARDA_TECH) ? " Hardened" :
				       (MechSpecials2(mech) & STEALTH_ARMOR_TECH) ? " Stealth" :
				       (MechSpecials2(mech) & HVY_FF_ARMOR_TECH) ? " Heavy Ferrofibrous" :
				       (MechSpecials2(mech) & LT_FF_ARMOR_TECH) ? " Light Ferrofibrous" : 
				       (MechInfantrySpecials(mech) & CS_PURIFIER_STEALTH_TECH) ? " Purifier Stealth" : ""),
					damage_table[i][2]);
			fix_bth = FindTechSkill(player, mech) + (damage_table[i][0] == FIXINTERNAL ? FIXINTERNAL_DIFFICULTY : FIXARMOR_DIFFICULTY);
			fix_time = damage_table[i][0] == FIXINTERNAL ? FIXINTERNAL_TIME * damage_table[i][2] : FIXARMOR_TIME * damage_table[i][2];
			break;
		}
		j = is_under_repair(mech, i);
		if(j) {
			sprintf(buf3, "%4s %4s", "N/A", "N/A");
		} else {
			sprintf(buf3, "%4d %4d", fix_time, fix_bth);
		}
		sprintf(buf2, "%%ch%s%3s %3d %9s %3s %s%%cn", j ? "%cg" : "%cy", j ? "(*)" :"",
				i + 1,
				buf3,
				ShortArmorSectionString(MechType(mech), MechMove(mech), v1),
				buf, j ? " (*)" : "");
		vsi(buf2);
	}
	addline();
	vsi("(*) / %ch%cgGreen%cn = Job already done. %ch%cyYellow%cn = To be done.");
	vsi("Time = Normal Time (in minutes) to complete fix. BTH = Your BTH to fix.");
	addline();
	ShowCoolMenu(player, c);
	KillCoolMenu(c);
}

static void fix_entry(dbref player, MECH * mech, int n)
{
	char buf[MBUF_SIZE];
	char *c;

	/* whee */
	n--;
	c = ShortArmorSectionString(MechType(mech), MechMove(mech),
								damage_table[n][1]);
	switch (damage_table[n][0]) {
	case REPAIRP_T:
		sprintf(buf, "%s %d", c, damage_table[n][2] + 1);
		tech_repairgun(player, mech, buf);
		break;
	case ENHCRIT_MISC:
	case ENHCRIT_FOCUS:
	case ENHCRIT_CRYSTAL:
	case ENHCRIT_BARREL:
	case ENHCRIT_AMMOB:
	case ENHCRIT_RANGING:
	case ENHCRIT_AMMOM:
		sprintf(buf, "%s %d", c, damage_table[n][2] + 1);
		tech_fixenhcrit(player, mech, buf);
		break;
	case REPAIRG:
		sprintf(buf, "%s %d", c, damage_table[n][2] + 1);
		tech_replacegun(player, mech, buf);
		break;
	case REPAIRP:
		sprintf(buf, "%s %d", c, damage_table[n][2] + 1);
		tech_replacepart(player, mech, buf);
		break;
	case RELOAD:
		sprintf(buf, "%s %d", c, damage_table[n][2] + 1);
		tech_reload(player, mech, buf);
		break;
	case REATTACH:
		sprintf(buf, "%s", c);
		tech_reattach(player, mech, buf);
		break;
	case RESEAL:
		sprintf(buf, "%s", c);
		tech_reseal(player, mech, buf);
		break;
	case FIXARMOR:
		sprintf(buf, "%s", c);
		tech_fixarmor(player, mech, buf);
		break;
	case FIXARMOR_R:
		sprintf(buf, "%s r", c);
		tech_fixarmor(player, mech, buf);
		break;
	case FIXINTERNAL:
		sprintf(buf, "%s", c);
		tech_fixinternal(player, mech, buf);
		break;
	case DETACH:
		sprintf(buf, "%s", c);
		tech_removesection(player, mech, buf);
		break;
	case SCRAPP:
		sprintf(buf, "%s %d", c, damage_table[n][2] + 1);
		tech_removepart(player, mech, buf);
		break;
	case SCRAPG:
		sprintf(buf, "%s %d", c, damage_table[n][2] + 1);
		tech_removegun(player, mech, buf);
		break;
	case UNLOAD:
		sprintf(buf, "%s %d", c, damage_table[n][2] + 1);
		tech_unload(player, mech, buf);
		break;
	case REPLACESUIT:
		sprintf(buf, "%s", c);
		tech_replacesuit(player, mech, buf);
		break;
	}
}

void tech_fix(dbref player, void *data, char *buffer)
{
	MECH *mech = data;
	int n = atoi(buffer);
	int low, high;
	int isds;

	skipws(buffer);
	TECHCOMMANDC;
	if(unit_is_fixable(mech))
		make_damage_table(mech);
	else
		make_scrap_table(mech);
	DOCHECK(!damage_last &&
			MechType(mech) == CLASS_MECH,
			"The 'mech is in pristine condition!");
	DOCHECK(!damage_last, "It's in pristine condition!");
	if(sscanf(buffer, "%d-%d", &low, &high) == 2) {
		DOCHECK(low < 1 || low > damage_last, "Invalid low #!");
		DOCHECK(high < 1 || high > damage_last, "Invalid high #!");
		for(n = low; n <= high; n++)
			fix_entry(player, mech, n);
		return;
	}
	DOCHECK(n < 1 || n > damage_last, "Invalid #!");
	fix_entry(player, mech, n);
}
