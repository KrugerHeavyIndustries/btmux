/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 */

#include <math.h>
#include <string.h>
#include "mech.h"
#include "muxevent.h"
#include "mech.events.h"
#include "mech.tech.h"
#include "failures.h"
#include "p.mech.utils.h"
#include "p.mech.tech.h"
#include "p.mech.consistency.h"
#include "p.mech.tech.do.h"
#include "p.bsuit.h"

#define my_parsepart(loc,part) \
switch (tech_parsepart(mech, buffer, loc, part,NULL)) \
{ case -1: notify(player, "Invalid section!");return; \
case -2: notify(player, "Invalid part!");return; }

#define my_parsepart2(loc,part,brand) \
switch (tech_parsepart(mech, buffer, loc, part,brand)) \
{ case -1: notify(player, "Invalid section!");return; \
case -2: notify(player, "Invalid part!");return; }

#define my_parsegun(loc,part,brand) \
switch (tech_parsegun(mech, buffer, loc, part, brand)) \
{ case -1: notify(player, "Invalid gun #!");return; \
  case -2: notify(player, "Invalid object to replace with!");return; \
  case -3: notify(player, "Invalid object type - not matching with original.");return; \
  case -4: notify(player, "Invalid gun location - subscript out of range.");return; }

#define ClanMod(num) \
  MAX(1, (((num) / ((MechSpecials(mech) & CLAN_TECH) ? 2 : 1))))

static int tmp_flag = 0;
static int tmp_loc;
static int tmp_part;

static void tech_check_locpart(MUXEVENT * e)
{
	int loc, pos;
	int l = (int) e->data2;

	UNPACK_LOCPOS(l, loc, pos);
	if(loc == tmp_loc && pos == tmp_part)
		tmp_flag++;
}

static void tech_check_loc(MUXEVENT * e)
{
	int loc;

	loc = (((int) e->data2) % 16);
	if(loc == tmp_loc)
		tmp_flag++;
}

#define CHECK(t,fun) \
  tmp_flag=0;tmp_loc=loc;tmp_part = part; \
  muxevent_gothru_type_data(t, (void *) mech, fun); \
  return tmp_flag

#define CHECKL(t,fun) \
  tmp_flag=0;tmp_loc=loc; \
  muxevent_gothru_type_data(t, (void *) mech, fun); \
  return tmp_flag

#define CHECK2(t,t2,fun) \
  tmp_flag=0;tmp_loc=loc;tmp_part = part; \
  muxevent_gothru_type_data(t, (void *) mech, fun); \
  muxevent_gothru_type_data(t2, (void *) mech, fun); \
  return tmp_flag

/* Replace/reload */
int SomeoneRepairing_s(MECH * mech, int loc, int part, int t)
{
	CHECK(t, tech_check_locpart);
}

#define DAT(t) \
if (SomeoneRepairing_s(mech, loc, part, t)) return 1

int SomeoneRepairing(MECH * mech, int loc, int part)
{
	DAT(EVENT_REPAIR_RELO);
	DAT(EVENT_REPAIR_REPL);
	DAT(EVENT_REPAIR_REPLG);
	DAT(EVENT_REPAIR_REPAP);
	DAT(EVENT_REPAIR_REPAG);
	DAT(EVENT_REPAIR_MOB);
	DAT(EVENT_REPAIR_REPENHCRIT);
	return 0;
}

/* Fixinternal/armor */
int SomeoneFixingA(MECH * mech, int loc)
{
	CHECKL(EVENT_REPAIR_FIX, tech_check_loc);
}

int SomeoneFixingI(MECH * mech, int loc)
{
	CHECKL(EVENT_REPAIR_FIXI, tech_check_loc);
}

int SomeoneFixing(MECH * mech, int loc)
{
	return SomeoneFixingA(mech, loc) || SomeoneFixingI(mech, loc);
}

/* Reattach */
int SomeoneAttaching(MECH * mech, int loc)
{
	CHECKL(EVENT_REPAIR_REAT, tech_check_loc);
}

int SomeoneReplacingSuit(MECH * mech, int loc)
{
	CHECKL(EVENT_REPAIR_REPSUIT, tech_check_loc);
}

/* Reseal
 *
 * Added by Kipsta
 * 8/4/99
 */

int SomeoneResealing(MECH * mech, int loc)
{
	CHECKL(EVENT_REPAIR_RESE, tech_check_loc);
}

int SomeoneScrappingLoc(MECH * mech, int loc)
{
	CHECKL(EVENT_REPAIR_SCRL, tech_check_loc);
}

int SomeoneScrappingPart(MECH * mech, int loc, int part)
{
	DAT(EVENT_REPAIR_SCRP);
	DAT(EVENT_REPAIR_SCRG);
	DAT(EVENT_REPAIR_UMOB);
	return 0;
}

#undef CHECK
#undef CHECK2
#undef DAT

int CanScrapLoc(MECH * mech, int loc)
{
	tmp_flag = 0;
	tmp_loc = loc % 8;
	muxevent_gothru_type_data(EVENT_REPAIR_REPL, (void *) mech,
							  tech_check_loc);
	muxevent_gothru_type_data(EVENT_REPAIR_RELO, (void *) mech,
							  tech_check_loc);
	return !tmp_flag && !SomeoneFixing(mech, loc);
}

int CanScrapPart(MECH * mech, int loc, int part)
{
	return !(SomeoneRepairing(mech, loc, part));
}

#define tech_gun_is_ok(a,b,c) !PartIsNonfunctional(a,b,c)

extern char *silly_get_uptime_to_string(int);

int ValidGunPos(MECH * mech, int loc, int pos)
{
	unsigned char weaparray_f[MAX_WEAPS_SECTION];
	unsigned char weapdata_f[MAX_WEAPS_SECTION];
	int critical_f[MAX_WEAPS_SECTION];
	int i, num_weaps_f;

	if((num_weaps_f =
		FindWeapons_Advanced(mech, loc, weaparray_f, weapdata_f,
							 critical_f, 1)) < 0)
		return 0;
	for(i = 0; i < num_weaps_f; i++)
		if(critical_f[i] == pos)
			return 1;
	return 0;
}

void tech_checkstatus(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	int i = figure_latest_tech_event(mech);
	char *ms;

	DOCHECK(!i, "The mech's ready to rock!");
	ms = silly_get_uptime_to_string(game_lag_time(i));
	notify_printf(player, "The 'mech has approximately %s until done.", ms);
}

TECHCOMMANDH(tech_removegun)
{
	TECHCOMMANDB;
	TECHCOMMANDC;
	my_parsegun(&loc, &part, NULL);
	DOCHECK(SectIsDestroyed(mech, loc),
			"That part's blown off! You can assume the gun's gone too!");
	DOCHECK(!IsWeapon(GetPartType(mech, loc, part)), "That's no gun!");
	DOCHECK(PartIsDestroyed(mech, loc, part), "That gun's gone already!");
	DOCHECK(!ValidGunPos(mech, loc, part),
			"You can't remove middle of a gun!");
	DOCHECK(SomeoneScrappingPart(mech, loc, part),
			"Someone's scrapping it already!");
	DOCHECK(!CanScrapPart(mech, loc, part),
			"Someone's tinkering with it already!");
	DOCHECK(SomeoneScrappingLoc(mech, loc),
			"Someone's scrapping that section - no additional removals are possible!");
	DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	/* Ok.. Everything's valid (we hope). */
	if(tech_weapon_roll(player, mech, REMOVEG_DIFFICULTY) < 0) {
		START
			("Ack! Your attempt is far from perfect, you try to recover the gun..");
		if(tech_weapon_roll(player, mech, REMOVEG_DIFFICULTY) < 0) {
			START("No good. Consider the part gone.");
			FAKEREPAIR(REMOVEG_TIME * ClanMod(GetWeaponCrits(mech,
															 Weapon2I
															 (GetPartType
															  (mech, loc,
															   part)))),
					   EVENT_REPAIR_SCRG, mech, PACK_LOCPOS_E(loc, part,
															  mod));
			mod = 3;
			return;
		}
	}
	START("You start removing the gun..");
	STARTREPAIR(REMOVEG_TIME * ClanMod(GetWeaponCrits(mech,
													  Weapon2I(GetPartType
															   (mech, loc,
																part)))),
				mech, PACK_LOCPOS_E(loc, part, mod),
				muxevent_tickmech_removegun, EVENT_REPAIR_SCRG);
}

TECHCOMMANDH(tech_removepart)
{
	TECHCOMMANDB;
	TECHCOMMANDC;
	my_parsepart(&loc, &part);
	DOCHECK((t =
			 GetPartType(mech, loc, part)) == EMPTY,
			"That location is empty!");
	DOCHECK(SectIsDestroyed(mech, loc),
			"That part's blown off! You can assume the part's gone too!");
	DOCHECK(IsWeapon(t), "That's a gun - use removegun instead!");
	DOCHECK(PartIsDestroyed(mech, loc, part), "That part's gone already!");
	DOCHECK(IsCrap(GetPartType(mech, loc, part)),
			"That type isn't scrappable!");
	DOCHECK(t == Special(ENDO_STEEL) ||
			t == Special(FERRO_FIBROUS) || t == Special(STEALTH_ARMOR) ||
			t == Special(HVY_FERRO_FIBROUS) || t == Special(LT_FERRO_FIBROUS),
			"That type of item can't be removed!");
	DOCHECK(SomeoneScrappingPart(mech, loc, part),
			"Someone's scrapping it already!");
	DOCHECK(SomeoneScrappingLoc(mech, loc),
			"Someone's scrapping that section - no additional removals are possible!");
	DOCHECK(!CanScrapPart(mech, loc, part),
			"Someone's tinkering with it already!");
	DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	/* Ok.. Everything's valid (we hope). */
	START("You start removing the part..");
	if(tech_roll(player, mech, REMOVEP_DIFFICULTY) < 0) {
		START
			("Ack! Your attempt is far from perfect, you try to recover the part..");
		if(tech_roll(player, mech, REMOVEP_DIFFICULTY) < 0) {
			START("No good. Consider the part gone.");
			mod = 3;
			FAKEREPAIR(REMOVEP_TIME, EVENT_REPAIR_SCRP, mech,
					   PACK_LOCPOS_E(loc, part, mod));
			return;
		}
	}
	STARTREPAIR(REMOVEP_TIME, mech, PACK_LOCPOS_E(loc, part, mod),
				muxevent_tickmech_removepart, EVENT_REPAIR_SCRP);
}

#define CHECK_S(nloc) \
if (!SectIsDestroyed(mech,nloc)) return 1; \
if (Invalid_Scrap_Path(mech,nloc)) return 1

#define CHECK(tloc,nloc) \
case tloc: CHECK_S(nloc)

int Invalid_Scrap_Path(MECH * mech, int loc)
{
	if(loc < 0)
		return 0;
	if(MechType(mech) != CLASS_MECH)
		return 0;
	switch (loc) {
		CHECK(CTORSO, HEAD);
		CHECK_S(LTORSO);
		CHECK_S(RTORSO);
		break;
		CHECK(LTORSO, LARM);
		break;
		CHECK(RTORSO, RARM);
		break;
	}
	return 0;
}

#undef CHECK
#undef CHECK_S

TECHCOMMANDH(tech_removesection)
{
	TECHCOMMANDB;
	TECHCOMMANDC;
	my_parsepart(&loc, NULL);
	DOCHECK(SectIsDestroyed(mech, loc), "That section's gone already!");
	DOCHECK(Invalid_Scrap_Path(mech, loc),
			"You need to remove the outer sections first!");
	DOCHECK(SomeoneScrappingLoc(mech, loc),
			"Someone's scrapping it already!");
	DOCHECK(!CanScrapLoc(mech, loc), "Someone's tinkering with it already!");
	DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	/* Ok.. Everything's valid (we hope). */
	if(tech_roll(player, mech, REMOVES_DIFFICULTY) < 0)
		mod = 3;
	START("You start removing the section..");
	STARTREPAIR(REMOVES_TIME, mech, PACK_LOCPOS_E(loc, 0, mod),
				muxevent_tickmech_removesection, EVENT_REPAIR_SCRL);
}

TECHCOMMANDH(tech_replacegun)
{
	int brand = 0, ob = 0;
        
	int base, roll , rollmod, fixtime, base_fixtime, parttype, oparttype, fail_fixtime;

	TECHCOMMANDB;
	TECHCOMMANDC;
	my_parsegun(&loc, &part, &brand);
	DOCHECK(SectIsDestroyed(mech, loc),
			"That part's blown off! Use reattach first!");
	DOCHECK(SectIsFlooded(mech, loc),
			"That location has been flooded! Use reseal first!");
	DOCHECK(SomeoneRepairing(mech, loc, part),
			"Someone's repairing that part already!");
	DOCHECK(!IsWeapon(GetPartType(mech, loc, part)), "That's no gun!");
	DOCHECK(!ValidGunPos(mech, loc, part),
			"You can't replace middle of a gun!");
	DOCHECK(!PartIsNonfunctional(mech, loc, part), "That gun isn't hurtin'!");
	DOCHECK(SomeoneScrappingLoc(mech, loc),
			"Someone's scrapping that section - no repairs are possible!");
	DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	if(brand) {
		ob = GetPartBrand(mech, loc, part);
		SetPartBrand(mech, loc, part, brand);
	}


/*        oparttype=GetPartType(mech,loc,part);
        parttype =   (IsActuator(oparttype) ? Cargo(S_ACTUATOR) :
           (oparttype == Special(ENGINE) ?
               ((MechSpecials(mech) & XL_TECH) ? Cargo(XL_ENGINE) :
                    (MechSpecials(mech) & ICE_TECH) ? Cargo(IC_ENGINE) :
                         (MechSpecials(mech) & XXL_TECH) ? Cargo(XXL_ENGINE) :
                              (MechSpecials(mech) & CE_TECH) ? Cargo(COMP_ENGINE) :
                                   (MechSpecials(mech) & LE_TECH) ? Cargo(LIGHT_ENGINE) : oparttype) :
                                      (oparttype == Special(HEAT_SINK) && MechHasDHS(mech) ? Cargo(DOUBLE_HEAT_SINK) : oparttype)));
*/
	parttype = oparttype = GetPartType(mech,loc,part);


        DOCHECK(econ_find_items(IsDS(mech) ? AeroBay(mech,0) : Location(mech->mynum), parttype,GetPartBrand(mech,loc,part)) < 1 ,
                        tprintf("Not enough units of %s in store.",part_name(parttype,GetPartBrand(mech,loc,part))));

        notify_printf(player,"You start replacing the gun...");
        base = char_getskilltarget(player, "technician-weapons", 0);
        rollmod = REPLACE_DIFFICULTY + WEAPTYPE_DIFFICULTY(GetPartType(mech, loc, part));
        roll = tech_weapon_roll(player,mech, rollmod);
        base_fixtime = REPLACEGUN_TIME * ClanMod(GetWeaponCrits(mech, Weapon2I(GetPartType(mech, loc, part))));
        fail_fixtime = (base_fixtime * 3 )/ 2;

        if(roll < 0) {
                notify_printf(player,"Your attempt is unsuccessful, but you try to save the gun...");
                rollmod = REPLACE_DIFFICULTY;
                roll = tech_roll(player,mech,rollmod);
                if(roll < 0) {
                        fixtime = fail_fixtime;
                        notify_printf(player,"You muck around, wasting the gun for good...");
                        /* part goes , 1.5 * techtime*/
                        econ_change_items(IsDS(mech) ? AeroBay(mech,0) : Location(mech->mynum), parttype,GetPartBrand(mech,loc,part),-1);
                        tech_addtechtime(player, fixtime);
                        muxevent_add(MAX(1, player_techtime(player)*TECH_TICK), 0, EVENT_REPAIR_REPLG, very_fake_func, (void *) mech, (void *) (PACK_LOCPOS_E(loc,part,brand) + player * PLAYERPOS));

                } else {
                        notify_printf(player,"You manage to save the gun...");
                        /* part doesn't go. 1.5 * techtime, but lets mod the fix time if applicable*/
                        /* We should really MIN(100,mod * roll) for the subtract to cap this out */
			if(roll == 0)
				fixtime = fail_fixtime;
			else
				fixtime = mudconf.btech_variable_techtime ? (fail_fixtime* 10)  / (1000 / (100 - (roll ? mudconf.btech_techtime_mod * roll : 0 ))) : fail_fixtime ;
                        if(fail_fixtime - fixtime)
                                notify_printf(player,"Your skill manages to save %d minute%s", fail_fixtime - fixtime, fail_fixtime - fixtime == 1 ? "!" : "s!");
                        tech_addtechtime(player, fixtime);
                        muxevent_add(MAX(1, player_techtime(player)*TECH_TICK), 0, EVENT_REPAIR_REPLG, very_fake_func, (void *) mech, (void *) (PACK_LOCPOS_E(loc,part,brand) + player * PLAYERPOS));
                }

        } else {
		if(roll == 0)
			fixtime = base_fixtime;
		else
			fixtime = mudconf.btech_variable_techtime ? (base_fixtime * 10 ) / (1000 / (100 - (roll ? mudconf.btech_techtime_mod * roll : 0 ))) : base_fixtime;
                if(base_fixtime - fixtime)
                        notify_printf(player,"Your skill manages to save %d minute%s", base_fixtime - fixtime, base_fixtime - fixtime == 1 ? "!" : "s!");

                econ_change_items(IsDS(mech) ? AeroBay(mech,0) : Location(mech->mynum), parttype,GetPartBrand(mech,loc,part),-1);
                tech_addtechtime(player, fixtime);
                muxevent_add(MAX(1, player_techtime(player)*TECH_TICK), 0, EVENT_REPAIR_REPLG, muxevent_tickmech_replacegun, (void *) mech, (void *) (PACK_LOCPOS_E(loc,part,brand) + player * PLAYERPOS));
        }

/*
	DOTECH_LOCPOS(REPLACE_DIFFICULTY +
				  WEAPTYPE_DIFFICULTY(GetPartType(mech, loc, part)),
				  replaceg_fail, replaceg_succ, replace_econ,
				  REPLACEGUN_TIME *
				  ClanMod(GetWeaponCrits
						  (mech, Weapon2I(GetPartType(mech, loc, part)))),
				  mech, PACK_LOCPOS_E(loc, part, brand),
				  muxevent_tickmech_replacegun, EVENT_REPAIR_REPLG,
				  "You start replacing the gun..", 1);

*/
	if(brand)
		SetPartBrand(mech, loc, part, ob);
}

TECHCOMMANDH(tech_repairgun)
{
	int extra_hard = 0;

	TECHCOMMANDB;
	TECHCOMMANDC;
	/* Find the gun for us */
	my_parsegun(&loc, &part, NULL);
	DOCHECK(SectIsDestroyed(mech, loc),
			"That part's blown off! Use reattach first!");
	DOCHECK(SectIsFlooded(mech, loc),
			"That location has been flooded! Use reseal first!");
	DOCHECK(SomeoneRepairing(mech, loc, part),
			"Someone's repairing that part already!");
	DOCHECK(!IsWeapon(GetPartType(mech, loc, part)), "That's no gun!");
	DOCHECK(!ValidGunPos(mech, loc, part),
			"You can't repair middle of a gun!");
	DOCHECK(SomeoneScrappingPart(mech, loc, part),
			"Someone's scrapping it already!");
	DOCHECK(SomeoneScrappingLoc(mech, loc),
			"Someone's scrapping that section - no repairs are possible!");
	DOCHECK(PartIsDisabled(mech, loc, part), "That gun can't be fixed yet!");

	if(PartIsDestroyed(mech, loc, part)) {
		if(GetWeaponCrits(mech, Weapon2I(GetPartType(mech, loc,
													 part))) < 5 ||
		   PartIsDestroyed(mech, loc, part + 1)) {
			notify(player, "That gun is gone for good!");
			return;
		}
		extra_hard = 1;
	} else if(!PartTempNuke(mech, loc, part)) {
		notify(player, "That gun isn't hurtin'!");
		return;
	}
        
	DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	DOTECH_LOCPOS(REPAIR_DIFFICULTY + WEAPTYPE_DIFFICULTY(GetPartType(mech,
																	  loc,
																	  part)) +
				  extra_hard, repairg_fail, repairg_succ, repair_econ,
				  REPAIRGUN_TIME, mech, PACK_LOCPOS(loc, part),
				  muxevent_tickmech_repairgun, EVENT_REPAIR_REPAP,
				  "You start repairing the weapon..", 1);
}

TECHCOMMANDH(tech_fixenhcrit)
{
	int extra_hard = 0;

	TECHCOMMANDB;
	TECHCOMMANDC;
	/* Find the gun for us */
	my_parsegun(&loc, &part, NULL);
	DOCHECK(SectIsDestroyed(mech, loc),
			"That part's blown off! Use reattach first!");
	DOCHECK(SectIsFlooded(mech, loc),
			"That location has been flooded! Use reseal first!");
	DOCHECK(SomeoneRepairing(mech, loc, part),
			"Someone's repairing that part already!");
	DOCHECK(!IsWeapon(GetPartType(mech, loc, part)), "That's no gun!");
	DOCHECK(SomeoneScrappingPart(mech, loc, part),
			"Someone's scrapping it already!");
	DOCHECK(SomeoneScrappingLoc(mech, loc),
			"Someone's scrapping that section - no repairs are possible!");
	DOCHECK(PartIsDisabled(mech, loc, part), "That gun can't be fixed yet!");

	if(!PartIsDamaged(mech, loc, part)) {
		notify(player, "That gun isn't damaged!");
		return;
	}
        
	DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	DOTECH_LOCPOS(ENHCRIT_DIFFICULTY,
				  repairenhcrit_fail,
				  repairenhcrit_succ,
				  repairenhcrit_econ,
				  REPAIRENHCRIT_TIME,
				  mech,
				  PACK_LOCPOS(loc, part),
				  muxevent_tickmech_repairenhcrit,
				  EVENT_REPAIR_REPENHCRIT,
				  "You start repairing the weapon...", 1);
}

TECHCOMMANDH(tech_replacepart)
{
	TECHCOMMANDB;

	TECHCOMMANDC;

	int base, roll , rollmod, fixtime, base_fixtime, parttype, oparttype, fail_fixtime;

	my_parsepart(&loc, &part);
	DOCHECK((t =
			 GetPartType(mech, loc, part)) == EMPTY,
			"That location is empty!");
	DOCHECK(!PartIsNonfunctional(mech, loc, part),
			"That part looks ok to me..");
	DOCHECK(IsCrap(GetPartType(mech, loc, part)), "That part isn't hurtin'!");
	DOCHECK(IsWeapon(t), "That's a weapon! Use replacegun instead.");
	DOCHECK(SectIsDestroyed(mech, loc),
			"That part's blown off! Use reattach first!");
	DOCHECK(SectIsFlooded(mech, loc),
			"That location has been flooded! Use reseal first!");
	DOCHECK(SomeoneRepairing(mech, loc, part),
			"Someone's repairing that part already!");
	DOCHECK(SomeoneScrappingLoc(mech, loc),
			"Someone's scrapping that section - no repairs are possible!");
        DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");


/* little cheating here to get the proper part, since we aren't doing complex repairs */
	oparttype=GetPartType(mech,loc,part);
	parttype =   (IsActuator(oparttype) ? Cargo(S_ACTUATOR) : 
	   (oparttype == Special(ENGINE) ? 
	       ((MechSpecials(mech) & XL_TECH) ? Cargo(XL_ENGINE) : 
	            (MechSpecials(mech) & ICE_TECH) ? Cargo(IC_ENGINE) : 
		         (MechSpecials(mech) & XXL_TECH) ? Cargo(XXL_ENGINE) : 
			      (MechSpecials(mech) & CE_TECH) ? Cargo(COMP_ENGINE) : 
			           (MechSpecials(mech) & LE_TECH) ? Cargo(LIGHT_ENGINE) : oparttype) : 
				      (oparttype == Special(HEAT_SINK) && MechHasDHS(mech) ? Cargo(DOUBLE_HEAT_SINK) : oparttype)));

	

	DOCHECK(econ_find_items(IsDS(mech) ? AeroBay(mech,0) : Location(mech->mynum), parttype,GetPartBrand(mech,loc,part)) < 1 ,
			tprintf("Not enough units of %s in store.",part_name(parttype,GetPartBrand(mech,loc,part))));

	notify_printf(player,"You start replacing the part...");
	base = FindTechSkill(player,mech);
	rollmod = REPLACE_DIFFICULTY + PARTTYPE_DIFFICULTY(GetPartType(mech, loc, part));
	roll = tech_roll(player,mech, rollmod);
	base_fixtime = REPLACEPART_TIME;
	fail_fixtime = (REPLACEPART_TIME * 3 )/ 2;

	if(roll < 0) {
		notify_printf(player,"Your attempt is unsuccessful, but you try to save the part...");
		rollmod = rollmod + 1;
		roll = tech_roll(player,mech,rollmod);
		if(roll < 0) {
			fixtime = fail_fixtime;
			notify_printf(player,"You muck around, wasting the part for good...");
			/* part goes , 1.5 * techtime*/
			econ_change_items(IsDS(mech) ? AeroBay(mech,0) : Location(mech->mynum), parttype,GetPartBrand(mech,loc,part),-1);
			tech_addtechtime(player, fixtime);
			muxevent_add(MAX(1, player_techtime(player)*TECH_TICK), 0, EVENT_REPAIR_REPL, very_fake_func, (void *) mech, (void *) (PACK_LOCPOS(loc,part) + player * PLAYERPOS));

		} else {
			notify_printf(player,"You manage to save the part...");
			/* part doesn't go. 1.5 * techtime, but lets mod the fix time if applicable*/
			/* We should really MIN(100,mod * roll) for the subtract to cap this out */
			if (roll == 0)
				fixtime = fail_fixtime;
			else
				fixtime = mudconf.btech_variable_techtime ? (fail_fixtime * 10)  / (1000 / (100 - (roll ? mudconf.btech_techtime_mod * roll : 0 ))) : fail_fixtime ;
			if(fail_fixtime - fixtime)
				notify_printf(player,"Your skill manages to save %d minute%s", fail_fixtime - fixtime, fail_fixtime - fixtime == 1 ? "!" : "s!");
			tech_addtechtime(player, fixtime);
			muxevent_add(MAX(1, player_techtime(player)*TECH_TICK), 0, EVENT_REPAIR_REPL, very_fake_func, (void *) mech, (void *) (PACK_LOCPOS(loc,part) + player * PLAYERPOS));
		}

	} else {
		if (roll == 0)
			fixtime = base_fixtime;
		else
			fixtime = mudconf.btech_variable_techtime ? (base_fixtime * 10 ) / (1000 / (100 - (roll ? mudconf.btech_techtime_mod * roll : 0 ))) : base_fixtime;
		if(base_fixtime - fixtime)
			notify_printf(player,"Your skill manages to save %d minute%s", base_fixtime - fixtime, base_fixtime - fixtime == 1 ? "!" : "s!");
		
		econ_change_items(IsDS(mech) ? AeroBay(mech,0) : Location(mech->mynum), parttype,GetPartBrand(mech,loc,part),-1);
		tech_addtechtime(player, fixtime);
		muxevent_add(MAX(1, player_techtime(player)*TECH_TICK), 0, EVENT_REPAIR_REPL, muxevent_tickmech_repairpart, (void *) mech, (void *) (PACK_LOCPOS(loc,part) + player * PLAYERPOS));
	}
/*
	DOTECH_LOCPOS(REPLACE_DIFFICULTY +
				  PARTTYPE_DIFFICULTY(GetPartType(mech, loc, part)),
				  replacep_fail, replacep_succ, replace_econ,
				  REPLACEPART_TIME, mech, PACK_LOCPOS(loc, part),
				  muxevent_tickmech_repairpart, EVENT_REPAIR_REPL,
				  "You start replacing the part..", 0);
*/
}

TECHCOMMANDH(tech_repairpart)
{
	TECHCOMMANDB;

	TECHCOMMANDC;
	my_parsepart(&loc, &part);
	DOCHECK((t =
			 GetPartType(mech, loc, part)) == EMPTY,
			"That location is empty!");
	DOCHECK(PartIsDestroyed(mech, loc, part), "That part is gone for good!");
	DOCHECK(PartIsDisabled(mech, loc, part),
			"That part can't be repaired yet!");
	DOCHECK(!PartTempNuke(mech, loc, part), "That part isn't hurtin'!");
	DOCHECK(IsCrap(GetPartType(mech, loc, part)), "That part isn't hurtin'!");
	DOCHECK(IsWeapon(t), "That's a weapon! Use repairgun instead.");
	DOCHECK(SectIsDestroyed(mech, loc),
			"That part's blown off! Use reattach first!");
	DOCHECK(SectIsFlooded(mech, loc),
			"That location has been flooded! Use reseal first!");
	DOCHECK(SomeoneRepairing(mech, loc, part),
			"Someone's repairing that part already!");
	DOCHECK(SomeoneScrappingLoc(mech, loc),
			"Someone's scrapping that section - no repairs are possible!");
        DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	




	DOTECH_LOCPOS(REPAIR_DIFFICULTY + PARTTYPE_DIFFICULTY(GetPartType(mech,
																	  loc,
																	  part)),
				  repairp_fail, repairp_succ, repair_econ, REPAIRPART_TIME,
				  mech, PACK_LOCPOS(loc, part), muxevent_tickmech_repairpart,
				  EVENT_REPAIR_REPAP, "You start repairing the part..", 0);

}

TECHCOMMANDH(tech_toggletype)
{
	int atype;

	TECHCOMMANDB;

	DOCHECK((!Wizard(player)) && In_Character(mech->mynum),
			"This command only works in simpods!");
	my_parsepart2(&loc, &part, &atype);
	DOCHECK(!IsAmmo((t = GetPartType(mech, loc, part))), "That's no ammo!");
	DOCHECK(PartIsNonfunctional(mech, loc, part) ||
			PartIsDisabled(mech, loc, part),
			"The ammo compartment is nonfunctional!");
	DOCHECK(!atype,
			"You need to give a type to toggle to (use - for normal)");
	DOCHECK((t =
			 (valid_ammo_mode(mech, loc, part, atype))) < 0,
			"That is invalid ammo type for this weapon!");
	GetPartAmmoMode(mech, loc, part) &= ~(AMMO_MODES);
	GetPartAmmoMode(mech, loc, part) |= t;
	SetPartData(mech, loc, part, FullAmmo(mech, loc, part));
	mech_notify(mech, MECHALL, "Ammo toggled.");
}

TECHCOMMANDH(tech_reload)
{
	int atype;

	TECHCOMMANDB;
	TECHCOMMANDD;
	my_parsepart2(&loc, &part, &atype);
	DOCHECK(!IsAmmo((t = GetPartType(mech, loc, part))), "That's no ammo!");
	DOCHECK(PartIsNonfunctional(mech, loc, part),
			"The ammo compartment is destroyed ; repair/replacepart it first.");
	DOCHECK(PartIsDisabled(mech, loc, part),
			"The ammo compartment is disabled ; repair/replacepart it first.");
	DOCHECK((now = GetPartData(mech, loc, part)) == (full =
													 FullAmmo(mech, loc,
															  part)),
			"That particular ammo compartment doesn't need reloading.");
	DOCHECK(SomeoneRepairing(mech, loc, part),
			"Someone's playing with that part already!");
	DOCHECK(SectIsDestroyed(mech, loc),
			"That part's blown off! Use reattach first!");
	DOCHECK(SectIsFlooded(mech, loc),
			"That location has been flooded! Use reseal first!");
	DOCHECK(SomeoneScrappingLoc(mech, loc),
			"Someone's scrapping that section - no repairs are possible!");
	if(atype) {
		DOCHECK((t =
				 (valid_ammo_mode(mech, loc, part, atype))) < 0,
				"That is invalid ammo type for this weapon!");
		SetPartData(mech, loc, part, 0);
		GetPartAmmoMode(mech, loc, part) &= ~(AMMO_MODES);
		GetPartAmmoMode(mech, loc, part) |= t;
	}
	change = 0;
        
	DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	DOTECH_LOCPOS_VAL(RELOAD_DIFFICULTY, reload_fail, reload_succ,
					  reload_econ, &change, RELOAD_TIME, mech,
					  PACK_LOCPOS_E(loc, part, change),
					  muxevent_tickmech_reload, EVENT_REPAIR_RELO,
					  "You start reloading the ammo compartment..");
}

TECHCOMMANDH(tech_unload)
{
	TECHCOMMANDB;

	TECHCOMMANDD;
	my_parsepart(&loc, &part);
	DOCHECK(!IsAmmo((t = GetPartType(mech, loc, part))), "That's no ammo!");
	DOCHECK(PartIsNonfunctional(mech, loc, part),
			"The ammo compartment is destroyed ; repair/replacepart it first.");
	DOCHECK(PartIsDisabled(mech, loc, part),
			"The ammo compartment is disabled ; repair/replacepart it first.");
	DOCHECK(!(now =
			  GetPartData(mech, loc, part)),
			"That particular ammo compartment is empty already.");
	DOCHECK(SomeoneRepairing(mech, loc, part),
			"Someone's playing with that part already!");
	DOCHECK(SectIsDestroyed(mech, loc),
			"That part's blown off! Use reattach first!");
	DOCHECK(SectIsFlooded(mech, loc),
			"That location has been flooded! Use reseal first!");
	DOCHECK(SomeoneScrappingLoc(mech, loc),
			"Someone's scrapping that section - no repairs are possible!");
	if((full = FullAmmo(mech, loc, part)) == now)
		change = 2;
	else
		change = 1;
        DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	if(tech_roll(player, mech, REMOVES_DIFFICULTY) < 0)
		mod = 3;
	START("You start unloading the ammo compartment..");
	STARTREPAIR(RELOAD_TIME, mech, PACK_LOCPOS_E(loc, part, change),
				muxevent_tickmech_reload, EVENT_REPAIR_RELO);
}

TECHCOMMANDH(tech_fixarmor)
{
	int ochange;

	TECHCOMMANDB;

	TECHCOMMANDD;
	DOCHECK(tech_parsepart_advanced(mech, buffer, &loc, NULL, NULL, 1) < 0,
			"Invalid section!");
	if(loc >= 8) {
		from = GetSectRArmor(mech, loc % 8);
		to = GetSectORArmor(mech, loc % 8);
	} else {
		from = GetSectArmor(mech, loc);
		to = GetSectOArmor(mech, loc);
	}
	DOCHECK(SectIsDestroyed(mech, loc % 8),
			"That part's blown off! Use reattach first!");
	DOCHECK(SectIsFlooded(mech, loc % 8),
			"That location has been flooded! Use reseal first!");
	DOCHECK(SomeoneFixingA(mech, loc) ||
			SomeoneFixingI(mech, loc % 8),
			"Someone's repairing that section already!");
	DOCHECK(GetSectInt(mech, loc % 8) != GetSectOInt(mech, loc % 8),
			"The internals need to be fixed first!");
	DOCHECK(SomeoneScrappingLoc(mech, loc),
			"Someone's scrapping that section - no repairs are possible!");
	from = MIN(to, from);
	DOCHECK(from == to, "The location doesn't need armor repair!");
	change = to - from;
	ochange = change;	
	DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");
	DOTECH_LOC_VAL_S(FIXARMOR_DIFFICULTY, fixarmor_fail, fixarmor_succ,
					 fixarmor_econ, &change, FIXARMOR_TIME * ochange, loc,
					 EVENT_REPAIR_FIX, mech, "You start fixing the armor..");
	STARTIREPAIR(FIXARMOR_TIME * change, mech, (change * 16 + loc),
				 muxevent_tickmech_repairarmor, EVENT_REPAIR_FIX, change);
}

TECHCOMMANDH(tech_fixinternal)
{
	TECHCOMMANDB int ochange;

	TECHCOMMANDC;
	my_parsepart(&loc, NULL);
	from = GetSectInt(mech, loc);
	to = GetSectOInt(mech, loc);
	DOCHECK(from == to, "The location doesn't need internals' repair!");
	change = to - from;
	DOCHECK(SectIsDestroyed(mech, loc),
			"That part's blown off! Use reattach first!");
	DOCHECK(SectIsFlooded(mech, loc),
			"That location has been flooded! Use reseal first!");
	DOCHECK(SomeoneFixing(mech, loc),
			"Someone's repairing that section already!");
	DOCHECK(SomeoneScrappingLoc(mech, loc),
			"Someone's scrapping that section - no repairs are possible!");
	ochange = change;
        DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	DOTECH_LOC_VAL_S(FIXINTERNAL_DIFFICULTY, fixinternal_fail,
					 fixinternal_succ, fixinternal_econ, &change,
					 FIXINTERNAL_TIME * ochange, loc, EVENT_REPAIR_FIX, mech,
					 "You start fixing the internals..");
	STARTIREPAIR(FIXINTERNAL_TIME * change, mech, (change * 16 + loc),
				 muxevent_tickmech_repairinternal, EVENT_REPAIR_FIXI, change);
}

#define CHECK(tloc,nloc) \
case tloc:if (SectIsDestroyed(mech,nloc))return 1;break;

int Invalid_Repair_Path(MECH * mech, int loc)
{
	if(MechType(mech) != CLASS_MECH)
		return 0;
	switch (loc) {
		CHECK(HEAD, CTORSO);
		CHECK(LTORSO, CTORSO);
		CHECK(RTORSO, CTORSO);
		CHECK(LARM, LTORSO);
		CHECK(RARM, RTORSO);
		CHECK(LLEG, CTORSO);
		CHECK(RLEG, CTORSO);
	}
	return 0;
}

int unit_is_fixable(MECH * mech)
{
	int i;

	for(i = 0; i < NUM_SECTIONS; i++) {
		if(!GetSectOInt(mech, i))
			continue;
		if(!SectIsDestroyed(mech, i))
			continue;
		if(MechType(mech) == CLASS_MECH)
			if(i == CTORSO)
				return 0;
		if(MechType(mech) == CLASS_VTOL)
			if(i != ROTOR)
				return 0;
		if(MechType(mech) == CLASS_VEH_GROUND)
			if(i != TURRET)
				return 0;
	}
	return 1;
};

TECHCOMMANDH(tech_reattach)
{
	TECHCOMMANDB;

	TECHCOMMANDC;
	my_parsepart(&loc, NULL);
	DOCHECK(MechType(mech) == CLASS_BSUIT,
			"You can't reattach a Battlesuit! Use 'replacesuit'!");
	DOCHECK(!SectIsDestroyed(mech, loc), "That section isn't destroyed!");
	DOCHECK(Invalid_Repair_Path(mech, loc),
			"You need to reattach adjacent locations first!");
	DOCHECK(SomeoneAttaching(mech, loc),
			"Someone's attaching that section already!");
	DOCHECK(!unit_is_fixable(mech),
			"You see nothing to reattach it to (read:unit is cored).");
        DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	DOTECH_LOC(REATTACH_DIFFICULTY, reattach_fail, reattach_succ,
			   reattach_econ, REATTACH_TIME, mech, loc,
			   muxevent_tickmech_reattach, EVENT_REPAIR_REAT,
			   "You start replacing the section..");
}

TECHCOMMANDH(tech_replacesuit)
{
	int wSuits = 0;

	TECHCOMMANDB;

	TECHCOMMANDC;
	my_parsepart(&loc, NULL);
	DOCHECK(MechType(mech) != CLASS_BSUIT,
			"You can only use 'replacesuit' on a battlesuit unit!");

	wSuits = CountBSuitMembers(mech);

	DOCHECK(MechMaxSuits(mech) <= wSuits,
			tprintf
			("This %s is already full! This %s only consists of %d suits!",
			 GetLCaseBSuitName(mech), GetLCaseBSuitName(mech),
			 MechMaxSuits(mech)));
	DOCHECK((loc >= MechMaxSuits(mech)) ||
			(loc < 0),
			tprintf("Invalid suit! This %s only consists of %d suits!",
					GetLCaseBSuitName(mech), MechMaxSuits(mech)));

	DOCHECK(!SectIsDestroyed(mech, loc), "That suit isn't destroyed!");

	DOCHECK(SomeoneReplacingSuit(mech, loc),
			"Someone's already rebuilding that suit!");
	DOCHECK(wSuits <= 0,
			"You are unable to replace the suits here! None of the buggers are still alive!");
	DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	DOTECH_LOC(REPLACESUIT_DIFFICULTY, replacesuit_fail, replacesuit_succ,
			   replacesuit_econ, REPLACESUIT_TIME, mech, loc,
			   muxevent_tickmech_replacesuit, EVENT_REPAIR_REPSUIT,
			   "You start replacing the missing suit.");
}

/*
 * Reseal
 * Added by Kipsta
 * 8/4/99
 */

TECHCOMMANDH(tech_reseal)
{
	TECHCOMMANDB;

	TECHCOMMANDC;
	my_parsepart(&loc, NULL);
	DOCHECK(SectIsDestroyed(mech, loc), "That section is destroyed!");
	DOCHECK(!SectIsFlooded(mech, loc), "That has not been flooded!");
	DOCHECK(Invalid_Repair_Path(mech, loc),
			"You need to reattach adjacent locations first!");
	DOCHECK(SomeoneResealing(mech, loc),
			"Someone's sealing that section already!");
	DOCHECK(player_techtime(player) >= mudconf.btech_maxtechtime, "You're too tired to do that!");

	DOTECH_LOC(RESEAL_DIFFICULTY, reseal_fail, reseal_succ, reseal_econ,
			   RESEAL_TIME, mech, loc, muxevent_tickmech_reseal,
			   EVENT_REPAIR_RESE, "You start resealing the section.");
}

TECHCOMMANDH(tech_fixextra)
{
	TECHCOMMANDB;

	TECHCOMMANDC;
	notify(player, "Fixed extra stuff - reseals, ammo feeds, etc.");
	do_fixextra(mech);
}

TECHCOMMANDH(tech_magic)
{
	TECHCOMMANDB;

	TECHCOMMANDC;
	notify(player, "Doing the magic..");
	do_magic(mech);
	mech_int_check(mech, 1);
	notify(player, "Done!");
}
