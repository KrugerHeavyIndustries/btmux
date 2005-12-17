/*
 * Author: Cord Awtry <kipsta@mediaone.net>
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
#include "p.mech.ammodump.h"
#include "p.mech.build.h"
#include "p.mech.combat.misc.h"
#include "p.mech.damage.h"
#include "p.mech.partnames.h"
#include "p.mech.utils.h"

static void mech_dump_event(MUXEVENT * ev)
{
    MECH *mech = (MECH *) ev->data;
    int arg = (int) ev->data2;
    int loc;
    int i, l;
    int d, e = 0;
    char buf[SBUF_SIZE];
    int weapindx;

    if (!Started(mech))
	return;
    i = MechType(mech) == CLASS_MECH ? 7 : 5;
    /* Global ammo droppage */
    if (!arg) {
	for (; i >= 0; i--)
	    for (l = CritsInLoc(mech, i) - 1; l >= 0; l--)
		if (IsAmmo(GetPartType(mech, i, l)))
		    if (GetPartData(mech, i, l))
			Dump_Decrease(mech, i, l, &e);
	if (e > 1)
	    MECHEVENT(mech, EVENT_DUMP, mech_dump_event, DUMP_GRAD_TICK,
		arg);
	else {
	    mech_notify(mech, MECHALL, "All ammunition dumped.");
	    MechLOSBroadcast(mech,
		"no longer has ammo dumping from hatches on its back.");
	}
	return;
    }
    if (arg < 256) {
	loc = arg - 1;
	l = CritsInLoc(mech, loc);
	for (i = 0; i < l; i++)
	    if (IsAmmo(GetPartType(mech, loc, i)))
		if (!PartIsNonfunctional(mech, loc, i))
		    if ((d = GetPartData(mech, loc, i)))
			Dump_Decrease(mech, loc, i, &e);
	if (e > 1)
	    MECHEVENT(mech, EVENT_DUMP, mech_dump_event, DUMP_GRAD_TICK,
		arg);
	else if (e == 1 && Started(mech)) {
	    ArmorStringFromIndex(loc, buf, MechType(mech), MechMove(mech));
	    mech_printf(mech, MECHALL,
		"All ammunition in %s dumped.", buf);
	    MechLOSBroadcast(mech,
		"no longer has ammo dumping from hatches on its back.");
	}
	return;
    }
    if (arg < 65536) {
	weapindx = (arg / 256) - 1;
	for (; i >= 0; i--)
	    for (l = CritsInLoc(mech, i) - 1; l >= 0; l--)
		if (IsAmmo(GetPartType(mech, i, l)))
		    if (Ammo2WeaponI(GetPartType(mech, i, l)) == weapindx)
			if (GetPartData(mech, i, l))
			    Dump_Decrease(mech, i, l, &e);
	if (e > 1)
	    MECHEVENT(mech, EVENT_DUMP, mech_dump_event, DUMP_GRAD_TICK,
		arg);
	else {
	    mech_printf(mech, MECHALL, "Ammunition for %s dumped!",
		    get_parts_long_name(I2Weapon(weapindx), 0));
	    MechLOSBroadcast(mech,
		"no longer has ammo dumping from hatches on its back.");
	}
	return;
    }
    l = ((arg >> 16) & 0xFF) - 1;
    i = ((arg >> 24) & 0xFF) - 1;
    e = 0;
    if (GetPartData(mech, l, i))
	Dump_Decrease(mech, l, i, &e);
    if (e > 1) {
	MECHEVENT(mech, EVENT_DUMP, mech_dump_event, DUMP_GRAD_TICK, arg);
    } else {
	ArmorStringFromIndex(l, buf, MechType(mech), MechMove(mech));
	mech_printf(mech, MECHALL,
	    "Ammunition in %s crit %i dumped!", buf, i + 1);
	MechLOSBroadcast(mech,
	    "no longer has ammo dumping from hatches on its back.");
    }
}

void mech_dump(dbref player, void *data, char *buffer)
{
    MECH *mech = (MECH *) data;
    int argc;
    char *args[2];
    int weapnum;
    int weapindx;
    int section;
    int critical;
    int ammoLoc;
    int ammoCrit;
    int loc;
    int i, l, count = 0, d;
    char buf[MBUF_SIZE];
    int type = 0;

    cch(MECH_USUAL);
    argc = mech_parseattributes(buffer, args, 2);
    DOCHECK(argc < 1, "Not enough arguments to the function");
    weapnum = atoi(args[0]);

    DOCHECKMA(Jumping(mech), "You can't dump ammo while jumping!");
    DOCHECKMA(IsRunning(MechDesiredSpeed(mech), MMaxSpeed(mech)),
	"You can't dump ammo while running!");

    if (!strcasecmp(args[0], "stop")) {
	DOCHECKMA(!Dumping(mech), "You aren't dumping anything!");
	mech_notify(mech, MECHALL, "Ammo dumping halted.");
	StopDump(mech);
	MechLOSBroadcast(mech,
	    "no longer has ammo dumping from hatches on its back.");
	return;
    } else if (!strcasecmp(args[0], "all")) {
	count = 0;
	i = MechType(mech) == CLASS_MECH ? 7 : 5;
	for (; i >= 0; i--)
	    for (l = CritsInLoc(mech, i) - 1; l >= 0; l--)
		if (IsAmmo(GetPartType(mech, i, l)))
		    if (GetPartData(mech, i, l))
			count++;
	DOCHECKMA(!count, "You have no ammo to dump!");
	DOCHECKMA(Dumping_Type(mech, 0),
	    "You're already dumping your ammo!");
	StopDump(mech);
	mech_notify(mech, MECHALL, "Starting dumping of all ammunition..");
	MechLOSBroadcast(mech,
	    "starts dumping ammo from hatches on its back.");
	MECHEVENT(mech, EVENT_DUMP, mech_dump_event, DUMP_GRAD_TICK, 0);
	return;
    } else if (!weapnum && strcmp(args[0], "0")) {
	/* Try to find hitloc instead */
	DOCHECKMA(Dumping(mech), "You're already dumping some ammo!");
	loc =
	    ArmorSectionFromString(MechType(mech), MechMove(mech),
	    args[0]);
	DOCHECK(loc < 0, "Invalid location or weapon number!");
	ArmorStringFromIndex(loc, buf, MechType(mech), MechMove(mech));
	if (args[1]) {
	    i = atoi(args[1]);
	    i--;
	    if (i >= 0 && i < 12) {
		if (IsAmmo(GetPartType(mech, loc, i)))
		    if (!PartIsNonfunctional(mech, loc, i))
			if ((d = GetPartData(mech, loc, i)))
			    count++;
		DOCHECKMA(!count,
		    tprintf("There is no ammunition in %s crit %i!",
			buf, i + 1));
		type = (((i + 1) << 8) | (loc + 1));
		DOCHECKMA(type & ~0xFFFF,
		    "Internal inconsistency, dump failed!");
		type = type << 16;
		mech_printf(mech, MECHALL,
		    "Starting dumping of ammunition in %s crit %i..",
			buf, i + 1);
		MechLOSBroadcast(mech,
		    "starts dumping ammo from hatches on its back.");
		MECHEVENT(mech, EVENT_DUMP, mech_dump_event,
		    DUMP_GRAD_TICK, type);
		return;
	    }
	}
	l = CritsInLoc(mech, loc);
	for (i = 0; i < l; i++)
	    if (IsAmmo(GetPartType(mech, loc, i)))
		if (!PartIsNonfunctional(mech, loc, i))
		    if ((d = GetPartData(mech, loc, i)))
			count++;
	DOCHECKMA(!count, tprintf("There is no ammunition in %s!", buf));
	type = loc + 1;
	mech_printf(mech, MECHALL,
	    "Starting dumping of ammunition in %s..", buf);
	MechLOSBroadcast(mech,
	    "starts dumping ammo from hatches on its back.");
	MECHEVENT(mech, EVENT_DUMP, mech_dump_event, DUMP_GRAD_TICK, type);
	return;
    }
    weapindx = FindWeaponIndex(mech, weapnum);
    if (weapnum < 0)
	SendError(tprintf
	    ("CHEATER: #%d tried to crash mux with command 'dump %d'!",
		(int) player, weapnum));
    DOCHECKMA(Dumping(mech), "You're already dumping some ammo!");
    DOCHECK(weapindx < 0, "Invalid weapon number!");
    FindWeaponNumberOnMech(mech, weapnum, &section, &critical);
    DOCHECK(MechWeapons[weapindx].type == TBEAM ||
	MechWeapons[weapindx].type == THAND,
	"That weapon doesn't use ammunition!");
    DOCHECK(!FindAmmoForWeapon_sub(mech, -1, -1, weapindx, 0, &ammoLoc,
	    &ammoCrit, 0, 0),
	"You don't have any ammunition for that weapon stored on this mech!");
    DOCHECK(GetPartData(mech, ammoLoc, ammoCrit) == 0,
	"You are out of ammunition for that weapon already!");
    type = 256 * (weapindx + 1);
    mech_printf(mech, MECHALL, "Starting dumping %s ammunition..",
	    get_parts_long_name(I2Weapon(weapindx), 0));
    MechLOSBroadcast(mech,
	"starts dumping ammo from hatches on its back.");
    MECHEVENT(mech, EVENT_DUMP, mech_dump_event, DUMP_GRAD_TICK, type);
#if 0
    while (FindAmmoForWeapon(mech, weapindx, 0, &ammoLoc, &ammoCrit))
	GetPartData(mech, ammoLoc, ammoCrit) = 0;
    mech_printf(mech, MECHALL, "Ammunition for %s dumped!",
	    get_parts_long_name(I2Weapon(weapindx), 0));
#endif
}

int Dump_Decrease(MECH * mech, int loc, int pos, int *hm)
{
    int c, index, weapindx, rem;

#define RUP(a) { if (*hm < a) *hm = a; return a; }
    /* It _is_ ammo, and contains something */

    if (IsAmmo((index = GetPartType(mech, loc, pos))))
	if (!PartIsNonfunctional(mech, loc, pos))
	    if ((c = GetPartData(mech, loc, pos))) {
		weapindx = Ammo2WeaponI(index);
		if (MechWeapons[weapindx].ammoperton < DUMP_SPEED) {
		    if ((muxevent_tick % (DUMP_SPEED /
				MechWeapons[weapindx].ammoperton)))
			RUP(2);
		    /* fine, we remove 1 */
		    rem = 1;
		} else
		    rem =
			MIN(c,
			MechWeapons[weapindx].ammoperton / DUMP_SPEED);
		ammo_expedinture_check(mech, weapindx, rem - 1);
		SetPartData(mech, loc, pos, c - rem);
		if (c <= rem)
		    RUP(1);
		RUP(2);
	    }
    return 0;
}

/*
 * The function is for blowing up some of the ammo that's being dumped from
 * a mech when it takes a rear torso shot.
 *
 * FASA rules state that if a mech takes a rear torso shot while dumping ammo,
 * all the dumping ammo explodes and goes to the armor of that location. That's
 * a bit harsh in RS as getting behind someone ain't that hard. So what we do is
 * if you're dumping ammo and take a rear torso shot, we, on a roll of 7 or less, 
 * call this BlowDumpingAmmo function. This function finds all the ammo you're dumping
 * and blows up ONE ROUND of one type, randomly. If you're dumping a lot and get hit
 * a few times (like from an LRM) you could get a bunch of little booms which
 * could really ruin your day.
 */

void BlowDumpingAmmo(MECH * mech, MECH * attacker, int wHitLoc)
{
    struct objDumpingAmmo aobjAmmoItems[MAX_WEAPONS_PER_MECH];
    int wEventData = -1;
    int wSecIter, wSlotIter;
    int wcAmmoItems = 0;
    int wPartType = 0, wPartData = 0;
    int wLoc = 0;
    int wWeapIdx = 0;
    int wRndIdx = 0;
    int wBlowDamage = 0;

    DumpingData(mech, &wEventData);
    if (wEventData < 0)
	return;
    if (!wEventData) {		/* Global ammo dump */
	for (wSecIter = 7; wSecIter >= 0; wSecIter--)
	    for (wSlotIter = CritsInLoc(mech, wSecIter) - 1;
		wSlotIter >= 0; wSlotIter--) {
		wPartType = GetPartType(mech, wSecIter, wSlotIter);
		if (IsAmmo(wPartType))
		    if (GetPartData(mech, wSecIter, wSlotIter)) {
			aobjAmmoItems[wcAmmoItems].wDamage =
			    FindMaxAmmoDamage(Ammo2WeaponI(wPartType));
			aobjAmmoItems[wcAmmoItems].wLocation = wSecIter;
			aobjAmmoItems[wcAmmoItems].wSlot = wSlotIter;
			aobjAmmoItems[wcAmmoItems].wWeapIdx =
			    Ammo2WeaponI(wPartType);
			aobjAmmoItems[wcAmmoItems].wPartType = wPartType;
			wcAmmoItems++;
		    }
	    }
    } else if (wEventData < 256) {	/* Location specific ammo dump */
	wLoc = wEventData - 1;
	for (wSlotIter = 0; wSlotIter < CritsInLoc(mech, wLoc);
	    wSlotIter++) {
	    wPartType = GetPartType(mech, wLoc, wSlotIter);

/*     wPartType = GetPartType(mech, wSecIter, wSlotIter); */
	    if (IsAmmo(wPartType))
		if (!PartIsNonfunctional(mech, wLoc, wSlotIter) &&
		    GetPartData(mech, wLoc, wSlotIter)) {
		    aobjAmmoItems[wcAmmoItems].wDamage =
			FindMaxAmmoDamage(Ammo2WeaponI(wPartType));
		    aobjAmmoItems[wcAmmoItems].wLocation = wLoc;
		    aobjAmmoItems[wcAmmoItems].wSlot = wSlotIter;
		    aobjAmmoItems[wcAmmoItems].wWeapIdx =
			Ammo2WeaponI(wPartType);
		    aobjAmmoItems[wcAmmoItems].wPartType = wPartType;
		    wcAmmoItems++;
		}
	}
    } else if (wEventData < 65536) {	/* Weapon specific ammo dump */
	wWeapIdx = (wEventData / 256) - 1;
	for (wSecIter = 7; wSecIter >= 0; wSecIter--)
	    for (wSlotIter = CritsInLoc(mech, wSecIter) - 1;
		wSlotIter >= 0; wSlotIter--) {
		wPartType = GetPartType(mech, wSecIter, wSlotIter);
		if (IsAmmo(wPartType) &&
		    (Ammo2WeaponI(wPartType) == wWeapIdx)) {
		    aobjAmmoItems[wcAmmoItems].wDamage =
			FindMaxAmmoDamage(Ammo2WeaponI(wPartType));
		    aobjAmmoItems[wcAmmoItems].wLocation = wSecIter;
		    aobjAmmoItems[wcAmmoItems].wSlot = wSlotIter;
		    aobjAmmoItems[wcAmmoItems].wWeapIdx =
			Ammo2WeaponI(wPartType);
		    aobjAmmoItems[wcAmmoItems].wPartType = wPartType;
		    wcAmmoItems++;
		}
	    }
    } else {			/* crit specific dump */
	wSecIter = ((wEventData >> 16) & 0xFF) - 1;
	wSlotIter = ((wEventData >> 24) & 0xFF) - 1;
	wPartType = GetPartType(mech, wSecIter, wSlotIter);
	aobjAmmoItems[wcAmmoItems].wDamage =
	    FindMaxAmmoDamage(Ammo2WeaponI(wPartType));
	aobjAmmoItems[wcAmmoItems].wLocation = wSecIter;
	aobjAmmoItems[wcAmmoItems].wSlot = wSlotIter;
	aobjAmmoItems[wcAmmoItems].wWeapIdx = Ammo2WeaponI(wPartType);
	aobjAmmoItems[wcAmmoItems].wPartType = wPartType;
	wcAmmoItems++;
    }

    if (wcAmmoItems > 0) {
	wRndIdx = Number(0, wcAmmoItems - 1);
	wBlowDamage = aobjAmmoItems[wRndIdx].wDamage;
	wSecIter = aobjAmmoItems[wRndIdx].wLocation;
	wSlotIter = aobjAmmoItems[wRndIdx].wSlot;
	wWeapIdx = aobjAmmoItems[wRndIdx].wWeapIdx;
	if (wBlowDamage > 0) {
	    MechLOSBroadcast(mech,
		"'s rear armor lights up as ammo being dumped ignites!");
	    mech_printf(mech, MECHALL,
		"%%ch%%crSome of the %s ammo dumping out of your mech ignites!%%cn",
		    get_parts_long_name(I2Weapon(wWeapIdx), 0));
	    DamageMech(mech, attacker, 0, -1, wHitLoc, 1, 0,
		wBlowDamage, -1, -1, 0, -1, 0, 1);
	    /*
	     * Decrement the ammo one round
	     */
	    wPartData = GetPartData(mech, wSecIter, wSlotIter);
	    if (wPartData > 0)
		SetPartData(mech, wSecIter, wSlotIter, wPartData - 1);
	    mech_notify(mech, MECHALL,
		"%ch%crAll ammo dumping operations have stopped!%cn");
	    StopDump(mech);
	}
    }
}

int FindMaxAmmoDamage(int wWeapIdx)
{
    int wDamage = MechWeapons[wWeapIdx].damage;
    int wIter = 0;

    if (IsMissile(wWeapIdx) || IsArtillery(wWeapIdx)) {
	for (wIter = 0; MissileHitTable[wIter].key != -1; wIter++)
	    if (MissileHitTable[wIter].key == wWeapIdx)
		wDamage *= MissileHitTable[wIter].num_missiles[10];
    }

    return wDamage;
}
