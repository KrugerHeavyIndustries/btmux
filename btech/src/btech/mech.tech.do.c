/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 */

/* All the *_{succ|fail|econ} functions belong here */
#include "config.h"

#include "mech.h"
#include "muxevent.h"
#include "mech.events.h"
#include "mech.tech.h"
#include "p.econ.h"
#include "p.mech.tech.h"
#include "p.mech.status.h"
#include "p.mech.utils.h"

#define PARTCHECK_SUB(m,a,b,c) \
AVCHECKM(m,a,b,c); \
GrabPartsM(m,a,b,c);

#ifndef BT_COMPLEXREPAIRS
#define PARTCHECK(m,a,b,c) \
{ PARTCHECK_SUB(m, alias_part(m, a), b, c); }
#else
#define PARTCHECK(m,a,b,c) \
{ PARTCHECK_SUB(m, alias_part(m, a, loc), b, c); }
#endif

#define PARTCHECKTWO(m,a,b,c,d,e,f) \
AVCHECKM(m,a,b,c); \
AVCHECKM(m,d,e,f); \
GrabPartsM(m,a,b,c); \
GrabPartsM(m,d,e,f);

#define PARTCHECKTHREE(m,a,b,c,d,e,f,g,h,i) \
AVCHECKM(m,a,b,c); \
AVCHECKM(m,d,e,f); \
AVCHECKM(m,g,h,i); \
GrabPartsM(m,a,b,c); \
GrabPartsM(m,d,e,f); \
GrabPartsM(m,g,h,i);

#define PARTCHECKFOUR(m,a,b,c,d,e,f,g,h,i,j,k,l) \
AVCHECKM(m,a,b,c); \
AVCHECKM(m,d,e,f); \
AVCHECKM(m,g,h,i); \
AVCHECKM(m,j,k,l); \
GrabPartsM(m,a,b,c); \
GrabPartsM(m,d,e,f); \
GrabPartsM(m,g,h,i); \
GrabPartsM(m,j,k,l);

static struct {
	char name;					/* Letter identifying the ammo in 'reload' */
	char *lname;				/* Long name (for printing) */
	int aflag;					/* Flag to set on the crittype */
	int rtype;					/* required type flag: if non-negative, weapon has
								   to be this type to allow this ammo */
	int ntype;					/* disallowed type flag: if non-negative, weapon
								   cannot be this type to allow this ammo */
	int rspec;					/* required 'special' flags: if non-zero,
								   weapon has to have at least one of these
								   bits in the 'special' flag for it to allow
								   this ammo */
	int nspec;					/* disallowes 'special' flags: if non-zero,
								   weapon cannot have any of these bits set,
								   in the special flag, to allow this ammo */
} ammo_types[] = {
	{
	'-', "normal", 0, -1, -1, 0, 0}, {
	'L', "cluster", LBX_MODE, -1, -1, LBX, 0}, {
	'A', "artemis", ARTEMIS_MODE, TMISSILE, -1, 0, DAR | NARC | INARC}, {
	'N', "narc", NARC_MODE, TMISSILE, -1, 0, DAR | NARC | INARC}, {
	'S', "swarm", SWARM_MODE, TMISSILE, -1, IDF, DAR | NARC | INARC}, {
	'1', "swarm-1", SWARM1_MODE, TMISSILE, -1, IDF, DAR | NARC | INARC}, {
	'I', "inferno", INFERNO_MODE, TMISSILE, -1, 0,
			IDF | DAR | NARC | INARC}, {
	'X', "explosive", INARC_EXPLO_MODE, TMISSILE, -1, INARC, 0}, {
	'Y', "haywire", INARC_HAYWIRE_MODE, TMISSILE, -1, INARC, 0}, {
	'E', "ecm", INARC_ECM_MODE, TMISSILE, -1, INARC, 0}, {
	'Z', "nemesis", INARC_NEMESIS_MODE, TMISSILE, -1, INARC, 0}, {
	'R', "ap", AC_AP_MODE, TAMMO, -1, RFAC, 0}, {
	'F', "flechette", AC_FLECHETTE_MODE, TAMMO, -1, RFAC, 0}, {
	'D', "incendiary", AC_INCENDIARY_MODE, TAMMO, -1, RFAC, 0}, {
	'P', "precision", AC_PRECISION_MODE, TAMMO, -1, RFAC, 0}, {
	'T', "stinger", STINGER_MODE, TMISSILE, -1, IDF, DAR}, {
	'U', "caseless", AC_CASELESS_MODE, TAMMO, -1, RFAC, 0}, {
	'G', "semiguided", SGUIDED_MODE, TMISSILE, -1, IDF, DAR}, {
	'H', "highexplosive", ATM_HE_MODE, TMISSILE, -1, IDF, DAR}, {
	'V', "extendedrange", ATM_ER_MODE, TMISSILE, -1, IDF, DAR}, {
	'#', "lrmmode", MML_LRM_MODE, TMISSILE, -1, IDF, DAR}, {
	0, NULL, 0, 0, 0, 0, 0}
};

int valid_ammo_mode(MECH * mech, int loc, int part, int let)
{
	int w, i;

	if(!IsAmmo(GetPartType(mech, loc, part)) || !let)
		return -1;
	let = toupper(let);
	w = Ammo2I(GetPartType(mech, loc, part));

	if(MechWeapons[w].special & NOSPA)
		return -1;

	for(i = 0; ammo_types[i].name; i++) {
		if(ammo_types[i].name != let)
			continue;
		if(ammo_types[i].rtype >= 0 &&
		   MechWeapons[w].type != ammo_types[i].rtype)
			continue;
		if(ammo_types[i].rspec &&
		   !(MechWeapons[w].special & ammo_types[i].rspec))
			continue;
		if(ammo_types[i].ntype >= 0 &&
		   MechWeapons[w].type == ammo_types[i].ntype)
			continue;
		if(ammo_types[i].nspec &&
		   (MechWeapons[w].special & ammo_types[i].nspec))
			continue;
		return ammo_types[i].aflag;
	}
	return -1;
}

int FindAmmoType(MECH * mech, int loc, int part)
{
	int t = GetPartType(mech, loc, part);
	int m = GetPartAmmoMode(mech, loc, part);
	int base = -1;

	if(!IsAmmo(t))
		return t;
	t = Ammo2I(t);

	if(strstr(MechWeapons[t].name, "StreakSRM"))
		base = SSRM_AMMO;
	else if(strstr(MechWeapons[t].name, "StreakLRM"))
		base = SLRM_AMMO;
	else if(strstr(MechWeapons[t].name, "ELRM"))
		base = ELRM_AMMO;
	else if(strstr(MechWeapons[t].name, "LR_DFM"))
		base = LR_DFM_AMMO;
	else if(strstr(MechWeapons[t].name, "SR_DFM"))
		base = SR_DFM_AMMO;
	else if(strstr(MechWeapons[t].name, "LRM"))
		base = LRM_AMMO;
	else if(strstr(MechWeapons[t].name, "SRM"))
		base = SRM_AMMO;
	else if(strstr(MechWeapons[t].name, "MRM"))
		base = MRM_AMMO;

	if(!(m & AMMO_MODES)) {
		if(base < 0)
			return I2Ammo(t);
		else
			return Cargo(base);
	}

	if(m & LBX_MODE) {
		if(strstr(MechWeapons[t].name, "LB20"))
			base = LBX20_AMMO;
		else if(strstr(MechWeapons[t].name, "LB10"))
			base = LBX10_AMMO;
		else if(strstr(MechWeapons[t].name, "LB5"))
			base = LBX5_AMMO;
		else if(strstr(MechWeapons[t].name, "LB2"))
			base = LBX2_AMMO;
		if(base < 0)
			return I2Ammo(t);
		return Cargo(base);
	}

	if(m & AC_MODES) {
		if(m & AC_AP_MODE) {
			if(strstr(MechWeapons[t].name, "AC/2"))
				base = AC2_AP_AMMO;
			if(strstr(MechWeapons[t].name, "AC/5"))
				base = AC5_AP_AMMO;
			if(strstr(MechWeapons[t].name, "AC/10"))
				base = AC10_AP_AMMO;
			if(strstr(MechWeapons[t].name, "AC/20"))
				base = AC20_AP_AMMO;
			if(strstr(MechWeapons[t].name, "LightAC/2"))
				base = LAC2_AP_AMMO;
			if(strstr(MechWeapons[t].name, "LightAC/5"))
				base = LAC5_AP_AMMO;
		}

		if(m & AC_FLECHETTE_MODE) {
			if(strstr(MechWeapons[t].name, "AC/2"))
				base = AC2_FLECHETTE_AMMO;
			if(strstr(MechWeapons[t].name, "AC/5"))
				base = AC5_FLECHETTE_AMMO;
			if(strstr(MechWeapons[t].name, "AC/10"))
				base = AC10_FLECHETTE_AMMO;
			if(strstr(MechWeapons[t].name, "AC/20"))
				base = AC20_FLECHETTE_AMMO;
			if(strstr(MechWeapons[t].name, "LightAC/2"))
				base = LAC2_FLECHETTE_AMMO;
			if(strstr(MechWeapons[t].name, "LightAC/5"))
				base = LAC5_FLECHETTE_AMMO;
		}

		if(m & AC_INCENDIARY_MODE) {
			if(strstr(MechWeapons[t].name, "AC/2"))
				base = AC2_INCENDIARY_AMMO;
			if(strstr(MechWeapons[t].name, "AC/5"))
				base = AC5_INCENDIARY_AMMO;
			if(strstr(MechWeapons[t].name, "AC/10"))
				base = AC10_INCENDIARY_AMMO;
			if(strstr(MechWeapons[t].name, "AC/20"))
				base = AC20_INCENDIARY_AMMO;
			if(strstr(MechWeapons[t].name, "LightAC/2"))
				base = LAC2_INCENDIARY_AMMO;
			if(strstr(MechWeapons[t].name, "LightAC/5"))
				base = LAC5_INCENDIARY_AMMO;
		}

		if(m & AC_PRECISION_MODE) {
			if(strstr(MechWeapons[t].name, "AC/2"))
				base = AC2_PRECISION_AMMO;
			if(strstr(MechWeapons[t].name, "AC/5"))
				base = AC5_PRECISION_AMMO;
			if(strstr(MechWeapons[t].name, "AC/10"))
				base = AC10_PRECISION_AMMO;
			if(strstr(MechWeapons[t].name, "AC/20"))
				base = AC20_PRECISION_AMMO;
			if(strstr(MechWeapons[t].name, "LightAC/2"))
				base = LAC2_PRECISION_AMMO;
			if(strstr(MechWeapons[t].name, "LightAC/5"))
				base = LAC5_PRECISION_AMMO;
		}

		if(m & AC_CASELESS_MODE) {
			if(strstr(MechWeapons[t].name, "AC/2"))
				base = AC2_CASELESS_AMMO;
			if(strstr(MechWeapons[t].name, "AC/5"))
				base = AC5_CASELESS_AMMO;
			if(strstr(MechWeapons[t].name, "AC/10"))
				base = AC10_CASELESS_AMMO;
			if(strstr(MechWeapons[t].name, "AC/20"))
				base = AC20_CASELESS_AMMO;
			if(strstr(MechWeapons[t].name, "LightAC/2"))
				base = LAC2_CASELESS_AMMO;
			if(strstr(MechWeapons[t].name, "LightAC/5"))
				base = LAC5_CASELESS_AMMO;
		}
		if(base < 0)
			return I2Ammo(t);
		return Cargo(base);
	}

	if(m & INARC_EXPLO_MODE)
		return Cargo(INARC_EXPLO_AMMO);
	else if(m & INARC_HAYWIRE_MODE)
		return Cargo(INARC_HAYWIRE_AMMO);
	else if(m & INARC_ECM_MODE)
		return Cargo(INARC_ECM_AMMO);
	else if(m & INARC_NEMESIS_MODE)
		return Cargo(INARC_NEMESIS_AMMO);

	if(base < 0)
		return I2Ammo(t);
	if(m & NARC_MODE)
		return Cargo(base) + NARC_LRM_AMMO - LRM_AMMO;
	if(m & ARTEMIS_MODE)
		return Cargo(base) + ARTEMIS_LRM_AMMO - LRM_AMMO;
	if(m & SWARM_MODE)
		return Cargo(base) + SWARM_LRM_AMMO - LRM_AMMO;
	if(m & SWARM1_MODE)
		return Cargo(base) + SWARM1_LRM_AMMO - LRM_AMMO;
	if(m & INFERNO_MODE)
		return Cargo(base) + INFERNO_SRM_AMMO - SRM_AMMO;
	if(m & STINGER_MODE)
		return Cargo(base) + AMMO_LRM_STINGER - LRM_AMMO;
	if(m & SGUIDED_MODE)
	        return Cargo(base) + AMMO_LRM_SGUIDED - LRM_AMMO;
	return Cargo(base);
}

TFUNC_LOCPOS(replace_econ)
{
	if(IsAmmo(GetPartType(mech, loc, part)))
		return 0;
	PARTCHECK(mech, GetPartType(mech, loc, part), GetPartBrand(mech, loc,
															   part), 1);
	return 0;
}

TFUNC_LOCPOS_VAL(reload_econ)
{
	int ammotype = FindAmmoType(mech, loc, part);

	PARTCHECK(mech, ammotype, GetPartBrand(mech, loc, part), 1);
	return 0;
}

TFUNC_LOC_VAL(fixarmor_econ)
{
	PARTCHECK(mech, ProperArmor(mech), 0, *val);
	return 0;
}

TFUNC_LOC_VAL(fixinternal_econ)
{
	PARTCHECK(mech, ProperInternal(mech), 0, *val);
	return 0;
}

TFUNC_LOCPOS(repair_econ)
{
	if(IsAmmo(GetPartType(mech, loc, part)))
		return 0;
	PARTCHECKTWO(mech, Cargo(S_ELECTRONIC), 0, PartIsDestroyed(mech, loc,
															   part) ? 3 : 1,
				 ProperInternal(mech), 0, PartIsDestroyed(mech, loc,
														  part) ? 3 : 1);
	return 0;
}

TFUNC_LOCPOS(repairenhcrit_econ)
{
	PARTCHECK(mech, Cargo(S_ELECTRONIC), 0, 1);
	return 0;
}

TFUNC_LOC(reattach_econ)
{
#ifndef BT_COMPLEXREPAIRS
	PARTCHECKTWO(mech, ProperInternal(mech), 0, GetSectOInt(mech, loc),
				 Cargo(S_ELECTRONIC), 0, GetSectOInt(mech, loc));
#else
	if(mudconf.btech_complexrepair) {
		if(MechType(mech) == CLASS_MECH) {
			PARTCHECKTWO(mech, ProperInternal(mech), 0,
						 GetSectOInt(mech, loc), ProperMyomer(mech), 0, 1);
		} else {
			PARTCHECK(mech, ProperInternal(mech), 0, GetSectOInt(mech, loc));
		}
	} else {
		PARTCHECKTWO(mech, ProperInternal(mech), 0, GetSectOInt(mech, loc),
					 Cargo(S_ELECTRONIC), 0, GetSectOInt(mech, loc));
	}
#endif
	return 0;
}

#define BSUIT_REPAIR_INTERNAL_NEEDED			10
#define BSUIT_REPAIR_SENSORS_NEEDED				2
#define BSUIT_REPAIR_LIFESUPPORT_NEEDED		2
#define BSUIT_REPAIR_ELECTRONICS_NEEDED		10

TFUNC_LOC(replacesuit_econ)
{
	PARTCHECKFOUR(mech,
				  ProperInternal(mech), 0, BSUIT_REPAIR_INTERNAL_NEEDED,
				  Cargo(BSUIT_SENSOR), 0, BSUIT_REPAIR_SENSORS_NEEDED,
				  Cargo(BSUIT_LIFESUPPORT), 0,
				  BSUIT_REPAIR_LIFESUPPORT_NEEDED, Cargo(BSUIT_ELECTRONIC), 0,
				  BSUIT_REPAIR_ELECTRONICS_NEEDED);
	return 0;
}

/*
 * Added for new flood code by Kipsta
 * 8/4/99
 */

TFUNC_LOC(reseal_econ)
{
	PARTCHECKTWO(mech, ProperInternal(mech), 0, GetSectOInt(mech, loc),
				 Cargo(S_ELECTRONIC), 0, GetSectOInt(mech, loc));
	return 0;
}

/* -------------------------------------------- Successes */

/* Replace success is just that ; success, therefore the fake
   functions here */
NFUNC(TFUNC_LOCPOS(replacep_succ));
NFUNC(TFUNC_LOCPOS(replaceg_succ));
NFUNC(TFUNC_LOCPOS_VAL(reload_succ));
NFUNC(TFUNC_LOC_VAL(fixinternal_succ));
NFUNC(TFUNC_LOC_VAL(fixarmor_succ));
NFUNC(TFUNC_LOC(reattach_succ));
NFUNC(TFUNC_LOC_RESEAL(reseal_succ));
NFUNC(TFUNC_LOC(replacesuit_succ));

/* Repairs _Should_ have some averse effects */
NFUNC(TFUNC_LOCPOS(repairg_succ));
NFUNC(TFUNC_LOCPOS(repairenhcrit_succ));
NFUNC(TFUNC_LOCPOS(repairp_succ));

/* -------------------------------------------- Failures */

/* Replace failures give you one chance to roll for object recovery,
   otherwise it's irretrieavbly lost */
TFUNC_LOCPOS(replaceg_fail)
{
	int w = (IsWeapon(GetPartType(mech, loc, part)));

	if(tech_roll(player, mech, REPLACE_DIFFICULTY) < 0) {
		notify_printf(player,
					  "You muck around, wasting the %s in the progress.",
					  w ? "weapon" : "part");
		return -1;
	}
	notify_printf(player,
				  "Despite messing the repair, you manage not to waste the %s.",
				  w ? "weapon" : "part");
#ifndef BT_COMPLEXREPAIRS
	AddPartsM(mech, FindAmmoType(mech, loc, part), GetPartBrand(mech, loc,
																part), 1);
#else
	AddPartsM(mech, loc, FindAmmoType(mech, loc, part),
			  GetPartBrand(mech, loc, part), 1);
#endif
	return -1;
}

TFUNC_LOCPOS(repairg_fail)
{
	if(PartIsDestroyed(mech, loc, part))
		/* If we are calling repairgun on a thing that is actually destroyed
		 * the following check *should not* be necessary. Nevertheless... */
		if(GetWeaponCrits(mech, Weapon2I(GetPartType(mech, loc, part)))
		   > 4) {
			DestroyPart(mech, loc, part + 1);
			notify(player,
				   "You muck around, trashing the gun in the process.");
			return -1;
		}
	notify(player, "Your repair fails.. all the parts are wasted for good.");
	return -1;
}

TFUNC_LOCPOS(repairenhcrit_fail)
{
	notify(player, "You don't manage to repair the damage.");
	return -1;
}

/* Replacepart = Replacegun, for now */
TFUNC_LOCPOS(replacep_fail)
{
	notify(player, "Your repair fails.. all the parts are wasted for good.");
	return -1;
}

/* Repairpart = Repairgun, for now */
TFUNC_LOCPOS(repairp_fail)
{
	return repairg_fail(player, mech, loc, part);
}

/* Reload fail = ammo is wasted and some time, but no averse effects (yet) */
TFUNC_LOCPOS_VAL(reload_fail)
{
	notify(player, "You fumble around, wasting the ammo in the progress.");
	return -1;
}

/* Fixarmor/fixinternal failure means that at least 1, or at worst
   _all_, points are wasted */
TFUNC_LOC_VAL(fixarmor_fail)
{
	int tot = 0;
	int should = *val;

	if(tech_roll(player, mech, FIXARMOR_DIFFICULTY) >= 0)
		tot += 50;
	tot += Number(5, 44);
	tot = (tot * should) / 100;
	if(tot == 0)
		tot = 1;
	if(tot == should)
		tot = should - 1;
	notify_printf(player, "Your armor patching isn't exactly perfect.. "
				  "You managed to fix %d out of %d.", tot, should);
	*val = tot;
	return 0;
}

TFUNC_LOC_VAL(fixinternal_fail)
{
	int tot = 0;
	int should = *val;

	if(tech_roll(player, mech, FIXARMOR_DIFFICULTY) >= 0)
		tot += 50;
	tot += Number(5, 44);
	tot = (tot * should) / 100;
	if(tot == 0)
		tot = 1;
	if(tot == should)
		tot = should - 1;
	notify_printf(player,
				  "Your internal patching isn't exactly perfect.. You managed to fix %d out of %d.",
				  tot, should);
	*val = tot;
	return 0;
}

/* Reattach has 2 failures:
   - if you succeed in second roll, it takes just 1.5x time
   - if you don't, some (random %) of stuff is wasted and nothing is
   done (yet some techtime goes nonetheless */
TFUNC_LOC(reattach_fail)
{
	int tot;

	if(tech_roll(player, mech, REATTACH_DIFFICULTY) >= 0)
		return 0;
	tot = Number(5, 94);
	notify_printf(player,
				  "Despite your disastrous failure, you recover %d%% of the materials.",
				  tot);
	tot = (tot * GetSectOInt(mech, loc)) / 100;
	if(tot == 0)
		tot = 1;
	if(tot == GetSectOInt(mech, loc))
		tot = GetSectOInt(mech, loc) - 1;
#ifndef BT_COMPLEXREPAIRS
	AddPartsM(mech, Cargo(S_ELECTRONIC), 0, tot);
	AddPartsM(mech, ProperInternal(mech), 0, tot);
#else
	AddPartsM(mech, loc, Cargo(S_ELECTRONIC), 0, tot);
	AddPartsM(mech, loc, ProperInternal(mech), 0, tot);
	if(mudconf.btech_complexrepair && MechType(mech) == CLASS_MECH)
		AddPartsM(mech, loc, ProperMyomer(mech), 0, 1);
#endif
	return -1;
}

TFUNC_LOC(replacesuit_fail)
{
	int wRand = 0;

	if(tech_roll(player, mech, REATTACH_DIFFICULTY) >= 0)
		return 0;

	wRand = Number(5, 94);
	notify_printf(player,
				  "Despite your disastrous failure, you recover %d%% of the materials.",
				  wRand);
#ifndef BT_COMPLEXREPAIRS
	AddPartsM(mech, Cargo(BSUIT_SENSOR), 0,
			  MAX(((BSUIT_REPAIR_SENSORS_NEEDED * wRand) / 100), 1));
	AddPartsM(mech, Cargo(BSUIT_LIFESUPPORT), 0,
			  ((BSUIT_REPAIR_LIFESUPPORT_NEEDED * wRand) / 100));
	AddPartsM(mech, Cargo(BSUIT_ELECTRONIC), 0,
			  ((BSUIT_REPAIR_ELECTRONICS_NEEDED * wRand) / 100));
	AddPartsM(mech, ProperInternal(mech), 0,
			  MAX(((BSUIT_REPAIR_INTERNAL_NEEDED * wRand) / 100), 1));
#else
	AddPartsM(mech, loc, Cargo(BSUIT_SENSOR), 0,
			  MAX(((BSUIT_REPAIR_SENSORS_NEEDED * wRand) / 100), 1));
	AddPartsM(mech, loc, Cargo(BSUIT_LIFESUPPORT), 0,
			  ((BSUIT_REPAIR_LIFESUPPORT_NEEDED * wRand) / 100));
	AddPartsM(mech, loc, Cargo(BSUIT_ELECTRONIC), 0,
			  ((BSUIT_REPAIR_ELECTRONICS_NEEDED * wRand) / 100));
	AddPartsM(mech, loc, ProperInternal(mech), 0,
			  MAX(((BSUIT_REPAIR_INTERNAL_NEEDED * wRand) / 100), 1));
#endif
	return -1;
}

/*
 * Added by Kipsta for flooding code
 * 8/4/99
 */

TFUNC_LOC_RESEAL(reseal_fail)
{
	int tot;

	if(tech_roll(player, mech, RESEAL_DIFFICULTY) >= 0)
		return 0;
	tot = Number(5, 94);
	notify_printf(player,
				  "You don't manage to get all the water out and seal the section, though you recover %d%% of the materials.",
				  tot);
	tot = (tot * GetSectOInt(mech, loc)) / 100;
	if(tot == 0)
		tot = 1;
	if(tot == GetSectOInt(mech, loc))
		tot = GetSectOInt(mech, loc) - 1;
#ifndef BT_COMPLEXREPAIRS
	AddPartsM(mech, Cargo(S_ELECTRONIC), 0, tot);
	AddPartsM(mech, ProperInternal(mech), 0, tot);
#else
	AddPartsM(mech, loc, Cargo(S_ELECTRONIC), 0, tot);
	AddPartsM(mech, loc, ProperInternal(mech), 0, tot);
#endif
	return -1;
}
