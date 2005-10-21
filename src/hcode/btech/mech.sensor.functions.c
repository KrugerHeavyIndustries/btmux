
/*
 * $Id: mech.sensor.functions.c,v 1.1.1.1 2005/01/11 21:18:23 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Mon Sep  2 16:57:30 1996 fingon
 * Last modified: Tue Oct  6 17:21:27 1998 fingon
 *
 */

#include "mech.h"
#include "mech.sensor.h"
#include "p.map.obj.h"
#include "p.mech.update.h"
#include "p.mech.utils.h"
#include "p.template.h"

/* Full chance of seeing, except if range > conditionrange */
SEEFUNC(vislight_see, r > (c * ((!l && t &&
		IsLit(t)) ? 3 : 1)) ? 0 : (100 - (r / 3)) / ((t &&
	    (MechType(t) == CLASS_BSUIT || MechType(t) == CLASS_MW))
	? 3 : 1));

/* In perfect darkness, 2x conditionrange. Otherwise conditionrange,
   always same chance of seeing */
SEEFUNC(liteamp_see, ((!l && r > (2 * c)) ? 0 : (l &&
	    r > c) ? 0 : (70 - r)) / ((t && (MechType(t) == CLASS_BSUIT ||
		MechType(t) == CLASS_MW))
	? 3 : 1));

/* Always same chance, if within range */
SEEFUNC(infrared_see, (80 - r));
SEEFUNC(electrom_see, MAX(r < 24 ? 2 : 0, (60 - (r * 2))) / 2);
SEEFUNC(seismic_see, 50 - (r * 4));

SEEFUNC(radar_see, BOUNDED(10, (180 - r), 90));
SEEFUNC(bap_see, 101);
SEEFUNC(blood_see, 101);

/* Prior requirement: the seechance > 0. We assume it so,
   and only examine the flag. */

extern float ActualElevation(MAP * map, int x, int y, MECH * mech);

/* Visual methods are hampered by excess woods / water */
CSEEFUNC(vislight_csee, !(map->sensorflags & (1 << SENSOR_VIS)) &&
    !(f & (MECHLOSFLAG_BLOCK | MECHLOSFLAG_FIRE | MECHLOSFLAG_SMOKE)) &&
    MechLOSFlag_WoodCount(f) < 3 && (!t || MechZ(t) >= 0 ||
	ActualElevation(getMap(t->mapindex), MechX(t), MechY(t), t) >= 0.0
	|| MechLOSFlag_WaterCount(f) < 6));

/* Liteamp doesn't see into water, thanks to reflections etc */
CSEEFUNC(liteamp_csee, !(map->sensorflags & (1 << SENSOR_LA)) &&
    !(f & (MECHLOSFLAG_BLOCK | MECHLOSFLAG_FIRE | MECHLOSFLAG_SMOKE)) &&
    (!t || !IsLit(t)) && MechLOSFlag_WoodCount(f) < 2 && !(MechLOSFlag_WaterCount(f)));

/* Not too good with woods, infra.. too much variation in temperature */
CSEEFUNC(infrared_csee, !(map->sensorflags & (1 << SENSOR_IR)) &&
    !(f & (MECHLOSFLAG_BLOCK | MECHLOSFLAG_FIRE)) &&
    MechLOSFlag_WoodCount(f) < 6 && (!t || (MechType(t) != CLASS_BSUIT &&
	    MechType(t) != CLASS_MW)));

/* Mountains provide too much hindarance in terms of electromagnetic
   detection */
CSEEFUNC(electrom_csee, !(map->sensorflags & (1 << SENSOR_EM)) &&
    !(f & (MECHLOSFLAG_BLOCK | MECHLOSFLAG_MNTN)) &&
    MechLOSFlag_WoodCount(f) < 8 && !AnyECMDisturbed(m) && (!t ||
	(MechType(t) != CLASS_MW)));

/* Seismic sees, as long as there is a target, and it isn't jumping or
   flying, or hovering, or otherwise with little or no contact to the
   ground. Period. */
CSEEFUNC(seismic_csee, 
    !(map->sensorflags & (1 << SENSOR_SE)) &&
    t && 
    (!Jumping(m) && mudconf.btech_seismic_see_stopped ? 1 : 
        (abs(MechSpeed(t)) > MP1) &&
    ((MechMove(m) != MOVE_VTOL) || ((MechMove(m) == MOVE_VTOL) && Landed(m))) && 
    ((MechMove(m) != MOVE_FLY) || ((MechMove(m) == MOVE_FLY) && Landed(m))) && 
    Started(t) &&
    !Jumping(t) && 
    (MechType(t) != CLASS_BSUIT) &&
    (MechType(t) != CLASS_MW) && 
    (MechMove(t) != MOVE_HOVER) &&
    ((MechMove(t) != MOVE_VTOL) || ((MechMove(t) == MOVE_VTOL) &&
        Landed(t))) && ((MechMove(t) != MOVE_FLY) ||
        ((MechMove(t) == MOVE_FLY) && Landed(t)))) &&
    (MechMove(t) != MOVE_NONE));

/* Radar sees, as long as the target is flying and not near disruptions. */
CSEEFUNC(radar_csee, 
    !(map->sensorflags & (1 << SENSOR_RA)) &&
    t && 
    MechZ(t) > 2 && 
    !(f & MECHLOSFLAG_BLOCK) &&
    (MechZ(t) >= 10 || (r < (MechZ(t) * MechZ(t)))) &&
    MechsElevation(t) > 1);

/* BAP is disrupted by ECM and can't pick up units with nullsig active. */
CSEEFUNC(bap_csee, 
    !(map->sensorflags & (1 << SENSOR_BAP)) &&
    !AnyECMDisturbed(m) && 
    t && 
    !AngelECMProtected(t) &&
    !StealthArmorActive(t) && 
    !NullSigSysActive(t));

/* Bloodhound is only disrupted by ECM. */
CSEEFUNC(blood_csee, 
    !(map->sensorflags & (1 << SENSOR_BHAP)) &&
    !AnyECMDisturbed(m) && 
    t && 
    !AngelECMProtected(t));

/* Basically, mechs w/o heat are +2 tohit, mechs in utter overheat are
   -2 tohit */

/*
#define HEAT_MODIFIER(a) ((a) <= 7 ? 2 : (a) > 28 ? -2 : (a) > 21 ? -1 : (a) > 14 ? 0 : 1)
*/

#define HEAT_MODIFIER(a) ((a) <= 7 ? 2 : (a) <= 10 ? 1 : (a) <= 15 ? 0 : (a) <= 22 ? -1 : -2)

/* Heavy/assault -1 tohit, medium 0, light +1 */
#define WEIGHT_MODIFIER(a) (a > 65 ? -1 : a > 35 ? 0 : 1)

/* If target's moving, +1 tohit */
#define MOVE_MODIFIER(a) (abs(a) >= 10.75 ? 1 : 0)

#define nwood_count(mech,a)  (MechLOSFlag_WoodCount(a) + \
                             ((MechElevation(mech) + 2) < MechZ(mech) ? 0 : \
			      MechRTerrain(mech) == LIGHT_FOREST ? 1 : \
			      MechRTerrain(mech) == HEAVY_FOREST ? 2 : 0))

/* To-hit functions */
/* Visual - Non-Day, Interrupting woods, partial LOS, and hulldown. */
TOHITFUNC(vislight_tohit, ((!t ||
	    !IsLit(t)) ? (2 - l) : 0) + nwood_count(t,
	f) + ((f & MECHLOSFLAG_PARTIAL) ? 3 +
	(IsHulldown(t) ? 2 : 0) : 0));

/* Liteamp - Interrupting woods, partial LOS, hulldown. */
TOHITFUNC(liteamp_tohit, ((2 - l) / 2) + ((nwood_count(t,
		f) * 3) / 2) + ((f & MECHLOSFLAG_PARTIAL) ? 3 +
	(IsHulldown(t) ? 2 : 0) : 0));

/* Infrared - Interrupting Woods, partial LOS, Hulldown, heat. */
TOHITFUNC(infrared_tohit, ((nwood_count(t,
		f) * 4) / 3) + ((f & MECHLOSFLAG_PARTIAL) ? 3 +
	(IsHulldown(t) ? 2 : 0) : 0) + HEAT_MODIFIER(MechHeat(t) + 7));

/* Emag - Interrupting woods, partial LOS, hulldwon, weight/movement,
 * recently fired. */
TOHITFUNC(electrom_tohit, ((nwood_count(t,
		f) * 2) / 3) + ((f & MECHLOSFLAG_PARTIAL) ? 3 +
	(IsHulldown(t) ? 2 : 0) : 0) +
    WEIGHT_MODIFIER(MechTons(t)) + MOVE_MODIFIER(MechSpeed(t)) +
    (MechStatus(t) & FIRED ? -1 : 0) + MNumber(m, 0, 1));

/* Seismic - Partial LOS, hulldown, weight, speed */
TOHITFUNC(seismic_tohit,
    2 + ((f & MECHLOSFLAG_PARTIAL) ? 3 + (IsHulldown(t) ? 2 : 0) : 0) +
    WEIGHT_MODIFIER(MechRealTons(t)) - MOVE_MODIFIER(MechSpeed(t)) +
    MNumber(m, 0, 1));

/* BAP - Flat BTH? */
TOHITFUNC(bap_tohit, MNumber(m, 0, 2));	   /* Very evil */

/* BloodHound - Flat BTH? */
TOHITFUNC(blood_tohit, MNumber(m, 0, 2));  /* Very evil */

/* Radar - Only 10z and above, partial LOS, interrupting woods. */
TOHITFUNC(radar_tohit, ((MechZ(t) >= 10 ||
	    FlyingT(t)) ? -3 : 0) + ((f & MECHLOSFLAG_PARTIAL) ? 2 +
	(IsHulldown(t) ? 2 : 0) : 0) + nwood_count(t, f));
