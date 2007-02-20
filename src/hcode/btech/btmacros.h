
/*
 * $Id: btmacros.h,v 1.8 2005/08/10 14:09:34 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1998 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 *
 * Created: Sat Jun  6 15:08:11 1998 fingon
 * Last modified: Mon Jul 20 01:05:41 1998 fingon
 *
 */

/* Descendant of the original macros.h that died of being too bloated :) */

#include "config.h"

#ifndef BTMACROS_H
#define BTMACROS_H

#include <math.h>
#include "macros.h"
#include "floatsim.h"
#include "mech.h"
#include "muxevent.h"
#include "p.event.h"

#define LOS_NB InLineOfSight_NB
#define MWalkingSpeed(maxspeed) ((float) 2.0 * (maxspeed) / 3.0 + 0.1)
#define WalkingSpeed(maxspeed) ((float) 2.0 * (maxspeed) / 3.0)
#define IsRunning(speed,maxspeed) (speed > MWalkingSpeed(maxspeed))
#define is_aero(mech) ((MechType(mech) == CLASS_AERO) || (IsDS(mech)))
#define IsForest(t) (t == LIGHT_FOREST || t == HEAVY_FOREST)
#define IsForestHex(map,x,y) (IsForest(GetRTerrain(map,x,y)))
#define IsMountains(t) (t == MOUNTAINS)
#define IsMountainsHex(map,x,y) (IsMountains(GetRTerrain(map,x,y)))
#define IsRough(t) (t == ROUGH)
#define IsRoughHex(map,x,y) (IsRough(GetRTerrain(map,x,y)))
#define IsBuilding(t) (t == BUILDING)
#define IsBuildingHex(map,x,y) (IsBuilding(GetRTerrain(map,x,y)))
#define BaseElev(terr,elev) ((terr) == WATER ? -(elev) : (terr) == ICE ? -(elev) : (elev))
#define Elevation(mech_map,x,y) \
  BaseElev(GetRTerrain(mech_map,x,y),GetElevation(mech_map,x,y))
#define MechElevation(mech) \
  BaseElev(MechRTerrain(mech),abs(MechElev(mech)))
#define MechEngineSizeC(mech) \
    ((int) rint((2 * MechMaxSpeed(mech) / KPH_PER_MP) / 3) * MechTons(mech))
#define MechLowerElevation(mech) \
 (MechRTerrain(mech) != BRIDGE ? MechElevation(mech) : bridge_w_elevation(mech))
#define MechUpperElevation(mech) (MechRTerrain(mech) == ICE ? 0 : MechElevation(mech))
#define MechsElevation(mech) \
  (MechZ(mech) - ((MechUpperElevation(mech) <= MechZ(mech) ? MechUpperElevation(mech) : MechLowerElevation(mech))))

/* GotPilot checks if mech's pilot is valid and inside his machine */
#define GotPilot(mech) \
     (MechPilot(mech) > 0 && Location(MechPilot(mech)) == mech->mynum)

#define RGotPilot(mech) \
       ((GotPilot(mech)) && (Connected(MechPilot(mech)) || !isPlayer(MechPilot(mech))))

#define GotGPilot(mech) \
     ((pilot_override && GunPilot(mech) > 0) || \
      (!pilot_override && GotPilot(mech)))

#define RGotGPilot(mech) \
     ((pilot_override && GunPilot(mech) > 0 && (Connected(GunPilot(mech)) || \
						!isPlayer(GunPilot(mech)))) \
      || (!pilot_override && RGotPilot(mech)))

#define AeroBay(a,b)		(a)->pd.bay[b]
#define AeroFuel(a)		(a)->ud.fuel
#define AeroFuelMax(a)	 	(a)->rd.maxfuel
#define AeroFuelOrig(a)	 	(a)->ud.fuel_orig
#define AeroSI(a)		(a)->ud.si
#define AeroSIOrig(a)	 	(a)->ud.si_orig
#define AeroTurret(a,b)	 	(a)->pd.turret[b]
#define AeroUnusableArcs(a) 	(a)->pd.unusable_arcs
#define AeroFreeFuel(a) ((MechType(a) == CLASS_VTOL) && mudconf.btech_nofusionvtolfuel && (!(MechSpecials(a) & ICE_TECH)))
#define DSLastMsg(a)		(a)->rd.last_ds_msg
#define GunPilot(a)		(pilot_override>0?pilot_override:MechPilot(a))
#define MechRadioType(a)	((a)->ud.radioinfo)
#define MechRadioInfo(a)	(MechRadioType(a) / FREQS)
#define MechFreqs(a)		(MechRadioType(a) % FREQS)
#define MFreqs(a)		MechFreqs(a)
#define MechAim(a)		(a)->rd.aim
#define MechAimType(a)	 	(a)->rd.aim_type
#define MechAuto(a)		(a)->rd.autopilot_num
#define MechBTH(a)		(a)->rd.basetohit
#define MechBaseWalk(a)	(a)->ud.walkspeed
#define MechBaseRun(a)	(a)->ud.runspeed
#define MechBoomStart(a)	(a)->rd.boom_start
#define MechCarriedCargo(a) 	(a)->rd.cargo_weight
#define MechXPMod(a)		(a)->rd.xpmod
#define SetCWCheck(a)		MechCritStatus(a) &= ~LOAD_OK
#define SetWCheck(a)		MechCritStatus(a) &= ~OWEIGHT_OK
#define SetCarriedCargo(a,b) 	do { MechCarriedCargo(a) = (b) ; SetCWCheck(a); } while (0)
#define MechCarrying(a)	 	(a)->rd.carrying
#define MechTowedBy(a)      (a)->rd.towed_by
#define SetCarrying(a,b)	do { MechCarrying(a) = (b) ; SetCWCheck(a) ; } while (0)
#define MechChargeTarget(a)	(a)->rd.chgtarget
#define MechChargeTimer(a)	(a)->rd.chargetimer
#define MechChargeDistance(a)	(a)->rd.chargedist
#define MechCocoon(a)		(a)->rd.cocoon
#define MechComm(a)		(a)->rd.commconv
#define MechCommLast(a)	 	(a)->rd.commconv_last
#define MechComputer(a)	 	(a)->ud.computer
#define MechCritStatus(a)	(a)->rd.critstatus
#define MechCritStatus2(a)      (a)->rd.critstatus2
#define MechDFATarget(a)	(a)->rd.dfatarget
#define MechDesiredAngle(a) 	(a)->rd.angle
#define MechDesiredFacing(a) 	(a)->rd.desiredfacing
#define MechDesiredSpeed(a) 	(a)->rd.desired_speed
#define MechElev(a)		(a)->pd.elev
#define MechEndFZ(a)		(a)->rd.endfz
#define MechEngineHeat(a)	(a)->rd.engineheat
#define MechFX(a)		(a)->pd.fx
#define MechFY(a)		(a)->pd.fy
#define MechFZ(a)		(a)->pd.fz

#define MechRealX(a)        (a)->pd.real_x
#define MechRealY(a)        (a)->pd.real_y
#define MechRealZ(a)        (a)->pd.real_z

#define MechFacing(a)		(FSIM2SHO((a)->pd.facing))
#define MechRFacing(a)	 	(a)->pd.facing
#define SetRFacing(a,b)	 	MechRFacing(a) = (b)
#define SetFacing(a,b)	 	SetRFacing(a,SHO2FSIM(b))
#define AddRFacing(a,b)	 	MechRFacing(a) += b
#define AddFacing(a,b)	 	AddRFacing(a,SHO2FSIM(b))
#define MechFireAdjustment(a) 	(a)->rd.fire_adjustment
#define MechGoingX(a)		(a)->rd.goingx
#define MechGoingY(a)		(a)->rd.goingy
#define MechHeat(a)		(a)->rd.heat
#define MechHeatLast(a)		(a)->rd.heatboom_last
#define MechHexes(a)		(a)->pd.hexes_walked
#define MechID(a)		(a)->ID
#define MechIsOmniMech(a)		MechSpecials2(a) & OMNIMECH_TECH
#define MechInfantrySpecials(a)	 	(a)->rd.infantry_specials
#define MechJumpHeading(a) 	(a)->rd.jumpheading
#define MechJumpLength(a)	(a)->rd.jumplength
#define MechJumpSpeed(a)	(a)->rd.jumpspeed
#define MechJumpTop(a)	 	(a)->rd.jumptop
#define MechLRSRange(a)	 	(a)->ud.lrs_range
#define MechLWRT(a)		(a)->rd.last_weapon_recycle
#define MechLastRndU(a)		(a)->rd.lastrndu
#define MechLastUse(a)	 	(a)->rd.lastused
#define MechLastStartup(a)	(a)->rd.last_startup
#define MechLastX(a)		(a)->pd.last_x
#define MechLastY(a)		(a)->pd.last_y
#define MechLateral(a)		(a)->rd.lateral
#define MechMASCCounter(a) 	(a)->rd.masc_value
#define MechSChargeCounter(a) 	(a)->rd.scharge_value
#define MechEngineSizeV(a) 	(a)->rd.erat
#define MechEngineSize(a)	(MechEngineSizeV(a) > 0 ? MechEngineSizeV(a) : MechEngineSizeC(a))
#define MechMaxSpeed(a)	 	(a)->ud.maxspeed
#define TemplateMaxSpeed(a)     (a)->ud.template_maxspeed
#define SetMaxSpeed(a,b) 	do {MechMaxSpeed(a) = b;MechCritStatus(a) &= ~SPEED_OK;correct_speed(a);} while (0)
#define LowerMaxSpeed(a,b) 	SetMaxSpeed(a,MechMaxSpeed(a)-b)
#define DivideMaxSpeed(a,b) 	SetMaxSpeed(a,MechMaxSpeed(a)/b)
#define MechRMaxSpeed(a)	(a)->rd.rspd
#define MMaxSpeed(a)		((float) MechCargoMaxSpeed((a),(float) MechMaxSpeed((a))))
#define MechMaxSuits(a)	(a)->rd.maxsuits
#define MechMinusHeat(a)	(a)->rd.minus_heat
#define MechMove(a)		(a)->ud.move
#define MechIsQuad(a)			(MechMove(a) == MOVE_QUAD)
#define MechIsBiped(a)			(MechMove(a) == MOVE_BIPED)
#define MechNumOsinks(a)	(a)->rd.onumsinks
#define MechNumSeen(a)	 	(a)->rd.num_seen
#define MechPNumSeen(a)		(a)->rd.can_see
#define MechRealNumsinks(a)	(a)->ud.numsinks
#define MechActiveNumsinks(a)	(MechRealNumsinks(a) - MechDisabledHS(a))
#define MechDisabledHS(a)	(a)->rd.disabled_hs
#define MechPer(a)		(a)->rd.per
#define MechPrefs(a)		(a)->rd.mech_prefs
#define MechPKiller(a)		(MechPrefs(a) & MECHPREF_PKILL)
#define SetMechPKiller(a)	(MechPrefs(a) |= MECHPREF_PKILL)
#define UnSetMechPKiller(a)	(MechPrefs(a) &= ~MECHPREF_PKILL)
#define MechSLWarn(a)		(MechPrefs(a) & MECHPREF_SLWARN)
#define MechAutoFall(a)		(MechPrefs(a) & MECHPREF_AUTOFALL)
#define MechArmorWarn(a)	(!(MechPrefs(a) & MECHPREF_NOARMORWARN))
#define MechAmmoWarn(a)		(!(MechPrefs(a) & MECHPREF_NOAMMOWARN))
#define MechFailStand(a)	(!(MechPrefs(a) & MECHPREF_NOFAILSTAND))
#define MechAutoconSD(a)	(MechPrefs(a) & MECHPREF_AUTOCON_SD)
#define MechNoFriendlyFire(a)	(MechPrefs(a) & MECHPREF_NOFRIENDLYFIRE)
#define MechWalkXPFactor(a) 	(a)->rd.wxf
#define MechPilot(a)		(a)->pd.pilot
#define MechPilotSkillBase(a) 	(a)->rd.pilotskillbase
#define MechPilotStatus(a) 	(a)->pd.pilotstatus
#define MechPlusHeat(a)	 	(a)->rd.plus_heat
#define MechRadio(a)		(a)->ud.radio
#define MechRadioRange(a) 	(a)->ud.radio_range
#define MechRnd(a)		 (a)->rd.rnd
#define MechScanRange(a)	(a)->ud.scan_range
#define MechSections(a)	 	(a)->ud.sections
#define MechSensor(a)		(a)->rd.sensor
#define MechSpecials(a)	 	(a)->rd.specials
#define MechSpecials2(a) 	(a)->rd.specials2
#define MechSpeed(a)		(a)->rd.speed
#define MechSpotter(a)	 	(a)->rd.spotter
#define MechStall(a)		(a)->pd.stall

#define MechStartFX(a)	 	(a)->rd.startfx
#define MechStartFY(a)	 	(a)->rd.startfy
#define MechStartFZ(a)	 	(a)->rd.startfz

#define MechRealJumpLength(a)   (a)->rd.realjumplength
#define MechStartRealX(a)   (a)->rd.startrealx
#define MechStartRealY(a)   (a)->rd.startrealy
#define MechStartRealZ(a)   (a)->rd.startrealz
#define MechEndRealZ(a)     (a)->rd.endrealz

#define MechStartSpin(a)	(a)->rd.sspin
#define MechStatus(a)       (a)->rd.status
/* Adding in Exile's status2 and removing 3030's specialsstatus */
#define MechStatus2(a)      (a)->rd.status2
/* #define MechSpecialsStatus(a)		(a)->rd.specialsstatus */
#define MechSwarmTarget(a) 	(a)->rd.swarming
#define MechSwarmer(a)		(a)->rd.swarmedby
#define MechTacRange(a)	 	(a)->ud.tac_range
#define MechTankCritStatus(a)	(a)->rd.tankcritstatus
#define MechTargX(a)		(a)->rd.targx
#define MechTargY(a)		(a)->rd.targy
#define MechTargZ(a)		(a)->rd.targz
#define MechTarget(a)		(a)->rd.target
#define MechTeam(a)		(a)->pd.team
#define MechTerrain(a)	 	(a)->pd.terrain
#define MechRTerrain(a) 	((MechTerrain(a) == FIRE || MechTerrain(a) == SMOKE) ? mech_underlying_terrain(a) : MechTerrain(a))
#define MechTons(a)		(a)->ud.tons
#define MechRTons(a)		get_weight(a)
#define MechRTonsV(a)	 	(a)->rd.row
#define MechRealTons(a)		((a)->rd.row / 1024)
#define MechRCTonsV(a)	 	(a)->rd.rcw
#define MechTurnDamage(a) 	(a)->rd.turndamage
#define MechStaggeredLastTurn(a) (a)->rd.staggerstamp
#define MechStaggerStamp(a)	((a)->rd.staggerstamp - 1)
#define SetMechStaggerStamp(a,b) ((a)->rd.staggerstamp = (b) + 1)
#define MechTurretFacing(a) 	(a)->rd.turretfacing
#define MechType(a)		(a)->ud.type
#define MechType_Name(a)	(a)->ud.mech_name
#define MechType_Ref(a)	 	(a)->ud.mech_type
#define MechVFacing(a)	 	AcceptableDegree(MechFacing(a) + MechLateral(a))
#define MechVerticalSpeed(a) 	(a)->rd.verticalspeed
#define MechVisMod(a)	 	(a)->rd.vis_mod
#define MechWeapHeat(a)	 	(a)->rd.weapheat
#define MechX(a)		(a)->pd.x
#define MechY(a)		(a)->pd.y
#define MechZ(a)		(a)->pd.z
#define MechLX(a)		(a)->rd.lx
#define MechLY(a)		(a)->rd.ly

#define MechTargComp(a)              (a)->ud.targcomp
#ifdef BT_CALCULATE_BV
#define MechBVLast(a)		(a)->ud.mechbv_last
#endif
#define MechBV(a)		(a)->ud.mechbv
#define CargoSpace(a)		(a)->ud.cargospace
#define CarMaxTon(a)		(a)->ud.carmaxton
#define Heatcutoff(a)		(MechCritStatus(a) & HEATCUTOFF)

#define MechHasDHS(a)		(MechSpecials(a) & (CLAN_TECH|DOUBLE_HEAT_TECH))
#define ClanMech(a)		(MechSpecials(a) & CLAN_TECH)

#define HS_Size(a)		(MechType(mech) == CLASS_MECH ? (ClanMech(a) ? 2 : ((MechSpecials(a) & DOUBLE_HEAT_TECH) ? 3 : 1)) : 1)
#define HS_Efficiency(a)	(MechHasDHS(a) ? 2 : 1)
#define MechHasHeat(a)		(MechType(a) == CLASS_MECH || MechType(a) == CLASS_AERO || MechType(a) == CLASS_DS || \
				 MechType(a) == CLASS_SPHEROID_DS)

#define DSSpam(mek,msg)		do { if (DropShip(MechType(mek)) && DSOkToNotify(mek)) MechLOSBroadcast(mek,msg); } while (0)
#define DSSpam_O(mek,msg)	do { if (DropShip(MechType(mek))) MechLOSBroadcast(mek,msg); } while (0)

#define MechHasTurret(a)	((MechType(a) == CLASS_VEH_GROUND || \
				  MechType(a) == CLASS_VEH_NAVAL || \
				  MechType(a) == CLASS_VTOL) && \
				 GetSectOInt(a, TURRET))

#define MechSeemsFriend(a, b)	(MechTeam(a) == MechTeam(b) && \
				 InLineOfSight_NB(a, b, 0, 0, 0))

#define SetTurnMode(a,b) do { if (b) MechPrefs(a) |= MECHPREF_TURNMODE; else MechPrefs(a) &= ~MECHPREF_TURNMODE; } while (0)
#define GetTurnMode(a)	(MechPrefs(a) & MECHPREF_TURNMODE)

#define MECHEVENT(mech,type,func,time,data) \
  do { if (mech->mynum > 0) \
     muxevent_add(time, 0, type, func, (void *) (mech), (void *) (data)); } while (0)

#define AUTOEVENT(auto,type,func,time,data) \
  muxevent_add(time, 0, type, func, (void *) (auto), (void *) (data))

#define MAPEVENT(map,type,func,time,data) \
  muxevent_add(time, 0, type, func, (void *) (map), (void *) (data))
#define StopDec(a)    muxevent_remove_type_data2(EVENT_DECORATION, (void *) a)

#define OBJEVENT(obj,type,func,time,data) \
  muxevent_add(time, 0, type, func, (void *) obj, (void *) (data))

#define GetPartType(a,b,c)   MechSections(a)[b].criticals[c].type
#define SetPartType(a,b,c,d) GetPartType(a,b,c)=d

#define GetPartFireMode(a,b,c)   MechSections(a)[b].criticals[c].firemode
#define SetPartFireMode(a,b,c,d) GetPartFireMode(a,b,c)=d

#define GetPartAmmoMode(a,b,c)   MechSections(a)[b].criticals[c].ammomode
#define SetPartAmmoMode(a,b,c,d) GetPartAmmoMode(a,b,c)=d

#define GetPartDamageFlags(a,b,c)   MechSections(a)[b].criticals[c].weapDamageFlags
#define SetPartDamageFlags(a,b,c,d) GetPartDamageFlags(a,b,c)=d

#define GetPartDesiredAmmoLoc(a,b,c)   MechSections(a)[b].criticals[c].desiredAmmoLoc
#define SetPartDesiredAmmoLoc(a,b,c,d) GetPartDesiredAmmoLoc(a,b,c)=d

#define GetPartData(a,b,c)   MechSections(a)[b].criticals[c].data
#define SetPartData(a,b,c,d) GetPartData(a,b,c)=d

#define GetPartRBrand(mech,a,b) MechSections(mech)[a].criticals[b].brand
#define GetPartBrand(mech,a,b) (GetPartRBrand(mech,a,b)%16)

#define SetPartBrand(mech,a,b,d) \
    GetPartRBrand(mech,a,b) = (d) + (PartTempNuke(mech,a,b)<<4)
#define PartTempNuke(mech,a,b)  (GetPartRBrand(mech,a,b)>>4)
#define SetPartTempNuke(mech,a,b,d) \
    GetPartRBrand(mech,a,b) = GetPartBrand(mech,a,b) + ((d) << 4)
#define ClearTempNuke(mech,a,b) GetPartRBrand(mech,a,b) = GetPartBrand(mech,a,b)

#define PartIsNonfunctional(a,b,c)  (PartIsDisabled(a,b,c) || PartIsBroken(a,b,c) || PartIsDestroyed(a,b,c))
#define PartIsDamaged(a,b,c)   (GetPartFireMode(a,b,c) & DAMAGED_MODE)
#define DamagePart(a,b,c)      (GetPartFireMode(a,b,c) |= DAMAGED_MODE)
#define UnDamagePart(a,b,c)    do { (GetPartFireMode(a,b,c) &= ~DAMAGED_MODE); SetPartDamageFlags(a,b,c,0); SetPartTempNuke(a,b,c,0); } while (0)
#define PartIsDisabled(a,b,c)  (GetPartFireMode(a,b,c) & DISABLED_MODE)
#define DisablePart(a,b,c)     (GetPartFireMode(a,b,c) |= DISABLED_MODE)
#define UnDisablePart(a,b,c)   (GetPartFireMode(a,b,c) &= ~DISABLED_MODE)
#define PartIsBroken(a,b,c)    (GetPartFireMode(a,b,c) & (DESTROYED_MODE|BROKEN_MODE))
#define BreakPart(a,b,c)       (GetPartFireMode(a,b,c) |= BROKEN_MODE)
#define UnBreakPart(a,b,c)     (GetPartFireMode(a,b,c) &= ~BROKEN_MODE)
#define PartIsDestroyed(a,b,c) (GetPartFireMode(a,b,c) & DESTROYED_MODE)
#define DestroyPart(a,b,c)     do { (GetPartFireMode(a,b,c) |= DESTROYED_MODE); GetPartFireMode(a,b,c) &= ~(BROKEN_MODE|DISABLED_MODE|DAMAGED_MODE); SetPartDamageFlags(a,b,c,0); SetPartTempNuke(a,b,c,0); } while (0)
#define UnDestroyPart(a,b,c)   do { (GetPartFireMode(a,b,c) &= ~(DESTROYED_MODE|HOTLOAD_MODE|DISABLED_MODE|BROKEN_MODE|DAMAGED_MODE)); SetPartDamageFlags(a,b,c,0); SetPartTempNuke(a,b,c,0); } while (0)

#define WpnIsRecycling(a,b,c)  (GetPartData(a,b,c) > 0 && \
				IsWeapon(GetPartType(a,b,c)) && \
				!PartIsNonfunctional(a,b,c) && \
				!SectIsDestroyed(a,b))
#define SectArmorRepair(a,b)  SomeoneFixingA(a,b)
#define SectRArmorRepair(a,b) SomeoneFixingA(a,b+8)
#define SectIntsRepair(a,b)   SomeoneFixingI(a,b)

#define SectIsDestroyed(a,b)  (!GetSectArmor(a,b) && ((is_aero(a) || !GetSectInt(a,b)) && !IsDS(a)))
#define SetSectDestroyed(a,b)
#define UnSetSectDestroyed(a,b)
#define SectIsBreached(a,b)  ((a)->ud.sections[b].config & SECTION_BREACHED)
#define SetSectBreached(a,b) \
do { MechSections(a)[b].config |= SECTION_BREACHED ; SetWCheck(a); } while (0)
#define UnSetSectBreached(a,b) \
do { MechSections(a)[b].config &= ~SECTION_BREACHED ; SetWCheck(a); } while (0)

/*
 * Added 8/4/99 by Kipsta for new flooding code
 */

#define SectIsFlooded(a,b)  ((a)->ud.sections[b].config & SECTION_FLOODED)
#define SetSectFlooded(a,b) do { MechSections(a)[b].config |= SECTION_FLOODED ; SetWCheck(a); } while (0)
#define UnSetSectFlooded(a,b) do { MechSections(a)[b].config &= ~SECTION_FLOODED ; SetWCheck(a); } while (0)

#define GetSectArmor(a,b)    ((a)->ud.sections[b].armor)
#define GetSectRArmor(a,b)   ((a)->ud.sections[b].rear)
#define GetSectInt(a,b)      ((a)->ud.sections[b].internal)

#define SetSectArmor(a,b,c)  do { (a)->ud.sections[b].armor=c;SetWCheck(a); } while (0)
#define SetSectRArmor(a,b,c) do { (a)->ud.sections[b].rear=c;SetWCheck(a); } while (0)
#define SetSectInt(a,b,c)    do { (a)->ud.sections[b].internal=c;SetWCheck(a); } while (0)

#define GetSectOArmor(a,b)   (a)->ud.sections[b].armor_orig
#define GetSectORArmor(a,b)  (a)->ud.sections[b].rear_orig
#define GetSectOInt(a,b)     (a)->ud.sections[b].internal_orig

#define SetSectOArmor(a,b,c) (a)->ud.sections[b].armor_orig=c
#define SetSectORArmor(a,b,c) (a)->ud.sections[b].rear_orig=c
#define SetSectOInt(a,b,c)   (a)->ud.sections[b].internal_orig=c

#define CanJump(a) (!(Stabilizing(a)) && !(Jumping(a)))

/* #define Jumping(a)           muxevent_count_type_data(EVENT_JUMP,(void *) a) */

/* crew stunned related events and macros */
#define CrewStunned(a)			 muxevent_count_type_data(EVENT_UNSTUN_CREW, (void *) a)
#define StunCrew(a)					 MECHEVENT(a, EVENT_UNSTUN_CREW, unstun_crew_event, 60, 0)

/* Exile Stun code */
#define CrewStunning(a)         muxevent_count_type_data(EVENT_CREWSTUN, (void *) a)
#define StopCrewStunning(a)     muxevent_remove_type_data(EVENT_CREWSTUN, (void *) a)

#define Burning(a)           muxevent_count_type_data(EVENT_VEHICLEBURN, (void *) a)
#define BurningSide(a,side)  muxevent_count_type_data_data(EVENT_VEHICLEBURN, (void *) a, (void *) side)
#define StopBurning(a)       muxevent_remove_type_data(EVENT_VEHICLEBURN, (void *) a)
#define StopBurningSide(a,side) muxevent_remove_type_data_data(EVENT_VEHICLEBURN, (void *) a, (void *) side)
#define Extinguishing(a)     muxevent_count_type_data(EVENT_VEHICLE_EXTINGUISH, (void *) a)
#define Jellied(a)           (MechCritStatus(a) & JELLIED)
#define Exploding(a)         muxevent_count_type_data(EVENT_EXPLODE, (void *) a)
#define Dumping(a)           muxevent_count_type_data(EVENT_DUMP, (void *) a)
#define Dumping_Type(a,type) (muxevent_count_type_data_data(EVENT_DUMP, (void *) a, (void *) type) || muxevent_count_type_data_data(EVENT_DUMP, (void *) a, (void *) 0))
#define DumpingData(a,data2) muxevent_get_type_data(EVENT_DUMP, (void *) a, (void *) data2)
#define ChangingLateral(a)   muxevent_count_type_data(EVENT_LATERAL,(void *) a)
#define Seeing(a)            muxevent_count_type_data(EVENT_PLOS,(void *) a)
#define Locking(a)           muxevent_count_type_data(EVENT_LOCK,(void *) a)
#define Hiding(a)            muxevent_count_type_data(EVENT_HIDE,(void *) a)
#define HasCamo(a)           (MechSpecials2(a) & CAMO_TECH)
#define Digging(a)           (MechTankCritStatus(a) & DIGGING_IN)
#define MechDugIn(a)         (MechTankCritStatus(mech) & DUG_IN)
#define ChangingHulldown(a)  muxevent_count_type_data(EVENT_CHANGING_HULLDOWN,(void *) a)
#define IsHulldown(a)	     (MechStatus(a) & HULLDOWN)
#define Falling(a)           muxevent_count_type_data(EVENT_FALL,(void *) a)
#define Moving(a)            muxevent_count_type_data(EVENT_MOVE,(void *) a)
#define RemovingPods(a)      muxevent_count_type_data(EVENT_REMOVE_PODS,(void *) a)
#define SensorChange(a)      muxevent_count_type_data(EVENT_SCHANGE,(void *) a)
#define Stabilizing(a)       muxevent_count_type_data(EVENT_JUMPSTABIL,(void *) a)
#define Standrecovering(a)   muxevent_count_type_data(EVENT_STANDFAIL, (void *) a)
#define Standing(a)          muxevent_count_type_data(EVENT_STAND,(void *) a)
#define Starting(a)          muxevent_count_type_data(EVENT_STARTUP,(void *) a)
#define Recovering(a)        muxevent_count_type_data(EVENT_RECOVERY,(void *) a)
#define TakingOff(a)         muxevent_count_type_data(EVENT_TAKEOFF,(void *) a)
#define FlyingT(a)           (is_aero(a) || MechMove(a) == MOVE_VTOL)
#define RollingT(a)          ((MechType(a) == CLASS_AERO) || (MechType(a) == CLASS_DS))
#if 0
#define MaybeMove(a) \
do { if (!Moving(a) && Started(a) && (!Fallen(mech) || MechType(a) == CLASS_MECH)) \
   MECHEVENT(a,EVENT_MOVE,is_aero(a) ? aero_move_event : mech_move_event,\
	     MOVE_TICK,0); } while (0)
#endif
#define MaybeMove(a) do { } while(0)

#define SetRecyclePart(a,b,c,d) \
do { UpdateRecycling(a) ; SetPartData(a,b,c,d); } while (0)
#define SetRecycleLimb(a,b,c) \
do { UpdateRecycling(a) ; (a)->ud.sections[b].recycle=c; } while (0)
#define UpdateRecycling(a) \
do { if (Started(a) && !Destroyed(a) && a->rd.last_weapon_recycle != muxevent_tick) \
    recycle_weaponry(a); } while (0)

#define AllLimbsRecycling(mech) \
    (MechSections(mech)[RARM].recycle && \
    MechSections(mech)[LARM].recycle && \
    MechSections(mech)[RLEG].recycle && \
    MechSections(mech)[LLEG].recycle)

#define AnyLimbsRecycling(mech) \
    (MechSections(mech)[RARM].recycle || \
    MechSections(mech)[LARM].recycle || \
    MechSections(mech)[RLEG].recycle || \
    MechSections(mech)[LLEG].recycle)

#define StopExploding(a)    muxevent_remove_type_data(EVENT_EXPLODE, (void *) a)
#define StopLateral(a)      muxevent_remove_type_data(EVENT_LATERAL,(void *) a)
#define StopMasc(a)         muxevent_remove_type_data(EVENT_MASC_FAIL,(void *) a)
#define StopMascR(a)        muxevent_remove_type_data(EVENT_MASC_REGEN,(void *) a)
#define StopSCharge(a)      muxevent_remove_type_data(EVENT_SCHARGE_FAIL,(void *) a)
#define StopSChargeR(a)     muxevent_remove_type_data(EVENT_SCHARGE_REGEN,(void *) a)
#define StopDump(a)          muxevent_remove_type_data(EVENT_DUMP, (void *) a)
#define StopJump(a)          muxevent_remove_type_data(EVENT_JUMP, (void *) a)
#define StopOOD(a)          muxevent_remove_type_data(EVENT_OOD, (void *) a)
#define StopMoving(a)        muxevent_remove_type_data(EVENT_MOVE, (void *) a)
#define StopStand(a)         muxevent_remove_type_data(EVENT_STAND, (void *) a)
#define StopStabilization(a) muxevent_remove_type_data(EVENT_JUMPSTABIL, (void *) a)
#define StopSensorChange(a)  muxevent_remove_type_data(EVENT_SCHANGE,(void *) a)
#define StopStartup(a)       muxevent_remove_type_data(EVENT_STARTUP, (void *) a)
#define StopHiding(a)          muxevent_remove_type_data(EVENT_HIDE, (void *) a)
#define StopDigging(a)          muxevent_remove_type_data(EVENT_DIG, (void *) a);MechTankCritStatus(a) &= ~DIGGING_IN
#define StopHullDown(a)      muxevent_remove_type_data(EVENT_CHANGING_HULLDOWN, (void *) a)
#define StopTakeOff(a)       muxevent_remove_type_data(EVENT_TAKEOFF, (void *) a)
#define UnjammingTurret(a)   muxevent_count_type_data(EVENT_UNJAM_TURRET, (void *) a)
#define UnJammingAmmo(a)		 muxevent_count_type_data(EVENT_UNJAM_AMMO, (void *) a)
#define UnJammingAmmoData(a,type) muxevent_get_type_data(EVENT_UNJAM_AMMO, (void *) a, (void *) type)
#define WeaponUnJammingAmmo(a,type) muxevent_count_type_data_data(EVENT_UNJAM_AMMO, (void *) a, (void *) type)
#define EnteringHangar(a)		 muxevent_count_type_data(EVENT_ENTER_HANGAR, (void *) a)
#define OODing(a)            MechCocoon(a)
#define C_OODing(a)          (MechCocoon(a) > 0)
#define InSpecial(a)         (MechStatus(a) & UNDERSPECIAL)
#define InGravity(a)         (MechStatus(a) & UNDERGRAVITY)
#define InVacuum(a)          (MechStatus(a) & UNDERVACUUM)
#define Jumping(a)           (MechStatus(a) & JUMPING)
#define Started(a)           (MechStatus(a) & STARTED)
#define Destroyed(a)         (MechStatus(a) & DESTROYED)
#define Fallen(a)            (MechStatus(a) & FALLEN)
#define Immobile(a)          ( !Started(a) || Uncon(a) || Blinded(a) || (MechMove(a) == MOVE_NONE) || ((MechStatus(a) & FALLEN) && ( (MechType(a) != CLASS_MECH) && (MechType(a) != CLASS_MW) )) )
#define Landed(a)            (MechStatus(a) & LANDED)
#define Towed(a)             (MechStatus(a) & TOWED)
#define Towable(a)           (MechCritStatus(a) & TOWABLE)
#define PerformingAction(a)  (MechStatus(a) & PERFORMING_ACTION)
#define StopPerformingAction(a) (MechStatus(a) &= ~PERFORMING_ACTION)

#define MakeMechFall(a)      MechStatus(a) |= FALLEN;FallCentersTorso(a);MarkForLOSUpdate(a);MechFloods(a);StopStand(a);StopHullDown(a);MechStatus(a) &= ~HULLDOWN;
#define FallCentersTorso(a)  MechStatus(a) &= ~(TORSO_RIGHT|TORSO_LEFT|FLIPPED_ARMS)
#define MakeMechStand(a)     MechStatus(a) &= ~FALLEN;MarkForLOSUpdate(a)
#define StandMechTime(a)     (30 / BOUNDED(1,(MechMaxSpeed(a)/MP2),30))
#define StopLock(a)          muxevent_remove_type_data(EVENT_LOCK, (void *) a);\
MechStatus(a) &= ~LOCK_MODES
#define SearchlightChanging(a)	muxevent_count_type_data(EVENT_SLITECHANGING, (void *) a)
#define HeatcutoffChanging(a)	muxevent_count_type_data(EVENT_HEATCUTOFFCHANGING, (void *) a)

#define SeeWhenShutdown(a)	(MechStatus(mech) & AUTOCON_WHEN_SHUTDOWN)

#define LoseLock(a)         StopLock(a);MechTarget(a)=-1;MechTargX(a)=-1;MechTargY(a)=-1;if (MechAim(a) != NUM_SECTIONS) { mech_notify(a, MECHALL, "Location-specific targeting powers down."); MechAim(a) = NUM_SECTIONS; }
#ifdef ADVANCED_LOS
#define StartSeeing(a) \
    MECHEVENT(a,EVENT_PLOS,mech_plos_event,INITIAL_PLOS_TICK,0)
#else
#define StartSeeing(a)
#endif

#define Startup(a)           \
    do { MechStatus(a) |= STARTED;MechTurnDamage(a) = 0;UpdateRecycling(a); \
    MechNumSeen(a)=0; StartSeeing(a); } while (0)

#define Shutdown(a)          \
	do { if (!Destroyed(a)) { UpdateRecycling(a); MechSpeed(a) = 0.0; \
		MechCritStatus(a) &= ~(HEATCUTOFF); MechStatus(a) &= ~(STARTED|MASC_ENABLED); \
		MechStatus2(a) &= ~(ECM_ENABLED|ECCM_ENABLED|PER_ECM_ENABLED|PER_ECCM_ENABLED|ANGEL_ECM_ENABLED|ANGEL_ECCM_ENABLED|NULLSIGSYS_ON|STH_ARMOR_ON);\
		MechDesiredSpeed(a) = 0.0; }; \
		MechPilot(a) = -1; MechTarget(a) = -1; StopStartup(a); \
		StopMoveMode(a); MechStatus2(a) &= ~(MOVE_MODES); \
		StopJump(a); StopMoving(a); MechMASCCounter(a) = 0; \
		StopStand(a); StopStabilization(a); StopTakeOff(a); \
		StopHiding(a); StopDigging(a); StopHullDown(a); stopTAG(a); \
		DropClub(a); StopMasc(a); MechChargeTarget(a) = -1;\
		StopSwarming(a,0); MechSChargeCounter(a) = 0; \
		if (MechCarrying(a) > 0) {\
			mech_dropoff(GOD, a, ""); \
		}; \
	} while (0)

#define Destroy(a) \
    do { \
        if (Uncon(a)) { \
            MechStatus(a) &= ~(BLINDED|UNCONSCIOUS); \
            mech_notify(a, MECHALL, "The mech was destroyed while pilot was unconscious!"); \
        } \
        Shutdown(a); \
        MechStatus(a) |= DESTROYED; \
        MechCritStatus(a) &= ~MECH_STUNNED; \
        StopBSuitSwarmers(FindObjectsData(a->mapindex),a,1); \
        muxevent_remove_data((void *) a); \
        if ((MechType(a) == CLASS_MECH && Jumping(a)) || \
                (MechType(a) != CLASS_MECH && MechZ(a) > MechUpperElevation(a))) \
            MECHEVENT(a, EVENT_FALL, mech_fall_event, FALL_TICK, -1); \
    } while (0)

#define DestroyAndDump(a)           \
    do { Destroy(a); MechVerticalSpeed(a) = 0.0; \
    if (MechRTerrain(a) == WATER || MechRTerrain(a) == ICE) \
      MechZ(a) = -MechElev(a); \
    else \
       if (MechRTerrain(a) == BRIDGE) { \
           if (MechZ(a) >= MechUpperElevation(a)) \
               MechZ(a) = MechUpperElevation(a); \
           else \
               MechZ(a) = MechLowerElevation(a); \
      } else \
          MechZ(a) = MechElev(a); \
      MechFZ(a) = ZSCALE * MechZ(a); } while (0)

#define GetTerrain(map, x, y)       (map)->hexes[(x) + (y) * (map)->width].terrain
#define GetRTerrain(map,x,y)      ((GetTerrain(map,x,y)==FIRE || GetTerrain(map,x,y)==SMOKE) ? map_underlying_terrain(map,x,y) : GetTerrain(map,x,y))

#define GetElevation(map, x, y)     (map)->hexes[(x) + (y) * (map)->width].elevation

#define SetTerrain(map, x, y, t)    do {(map)->hexes[(x) + (y) * (map)->width].terrain = t; \
                                        UpdateMechsTerrain(map, x, y, t); } while (0)
#define SetTerrainBase(map, x, y, t)    (map)->hexes[(x) + (y) * (map)->width].terrain = t;
#define SetElevation(map, x, y, e)      (map)->hexes[(x) + (y) * (map)->width].elevation = e;

/* For now I don't care about allocations */
#define ScenError(msg...)        send_channel("ScenErrors",msg)
#define ScenStatus(msg...)       send_channel("ScenStatus",msg)
#define SendAI(msg...)           send_channel("MechAI",msg)
#define SendAlloc(msg)
#define SendLoc(msg...)
#define SendCustom(msg...)          send_channel("MechCustom",msg)
#define SendDB(msg...)              send_channel("DBInfo",msg)
#define SendDebug(msg...)           send_channel("MechDebugInfo",msg)
#define SendEcon(msg...)            send_channel("MechEconInfo",msg)
#define SendError(msg...)           send_channel("MechErrors",msg)
#define SendEvent(msg...)           send_channel("EventInfo",msg)
#define SendSensor(msg...)          send_channel("MechSensor",msg)
#define SendTrigger(msg...)         send_channel("MineTriggers",msg)
#define SendXP(msg...)              send_channel("MechXP",msg)
#define SendDSInfo(msg...)          send_channel("DSInfo",msg)

/*
 * Exile Added Channel Message Emits
 */
#define SendAttackEmits(msg...)     send_channel("MechAttackEmits",msg)
#define SendAttacks(msg...)         send_channel("MechAttacks",msg)
#define SendAttackXP(msg...)        send_channel("MechAttackXP",msg)
#define SendBTHDebug(msg...)        send_channel("MechBTHDebug",msg)
#define SendFreqs(msg...)           send_channel("MechFreqs",msg)
#define SendPilotXP(msg...)         send_channel("MechPilotXP",msg)
#define SendTechXP(msg...)          send_channel("MechTechXP",msg)

/*
 * This is the prototype for functions
 */

#ifdef TEMPLATE_VERBOSE_ERRORS

#define TEMPLATE_ERR(a,b...) \
if (a) { \
notify(player, tprintf(b)); \
if (fp) fclose(fp); return -1; }

#define TEMPLATE_GERR(a,b...) \
if (a) { \
char foobarbuf[512]; \
sprintf(foobarbuf, b); \
SendError(foobarbuf); \
if (fp) fclose(fp); return -1; }
#else

#define TEMPLATE_ERR(a,b...) \
if (a) { \
if (fp) fclose(fp); return -1; }
#define TEMPLATE_GERR TEMPLATE_ERR

#endif

#define HotLoading(weapindx,mode) \
((mode & HOTLOAD_MODE) && (MechWeapons[weapindx].special & IDF))

#define MirrorPosition(from,to,heightMod) \
do {	 MechFX(to) = MechFX(from); \
	  MechFY(to) = MechFY(from); \
	  MechFZ(to) = MechFZ(from) + (heightMod * ZSCALE); \
	  MechX(to) = MechX(from); \
	  MechY(to) = MechY(from); \
	  MechZ(to) = MechZ(from) + heightMod; \
          MechLastX(to) = MechLastX(from); \
          MechLastY(to) = MechLastY(from); \
	  MechTerrain(to) = MechTerrain(from); \
	  MechElev(to) = MechElev(from) + heightMod; MarkForLOSUpdate(to); MechFloods(to); } while (0)


#define FindPunchLoc(mech,hitloc,arc,iscritical,isrear) \
 do { if (MechType(mech) != CLASS_MECH) \
   hitloc = FindHitLocation(mech, arc, &iscritical, &isrear); \
 else \
	hitloc = FindPunchLocation(arc); } while (0)

#define FindKickLoc(mech,hitloc,arc,iscritical,isrear) \
 do { if (MechType(mech) != CLASS_MECH) \
   hitloc = FindHitLocation(mech, arc, &iscritical, &isrear); \
 else \
   hitloc = FindKickLocation(arc); } while (0)



#define ValidCoordA(mech_map,newx,newy,msg) \
DOCHECK(newx < 0 || newx >= mech_map->width || \
	newy < 0 || newy >= mech_map->height, \
	msg)
#define ValidCoord(mech_map,newx,newy) \
        ValidCoordA(mech_map,newx, newy, "Illegal coordinates!")
#define FlMechRange(map,m1,m2) \
        FaMechRange(m1,m2)

#define Readnum(tovar,fromvar) \
        (!(tovar = atoi(fromvar)) && strcmp(fromvar, "0"))

#define SetBit(val,bit)   (val |= bit)
#define UnSetBit(val,bit) (val &= ~(bit))
#define EvalBit(val,bit,state) \
        do {if (state) SetBit(val,bit); else UnSetBit(val,bit);} while (0)
#define ToggleBit(val,bit) \
do { if (!(val & bit)) SetBit(val,bit);else UnSetBit(val,bit); } while (0)
#define Sees360(mech) ((MechMove(mech)==MOVE_NONE) || (MechType(mech) == CLASS_BSUIT))
#define FindWeapons(m,i,wa,wda,cr) FindWeapons_Advanced(m,i,wa,wda,cr,1)

#define ContinueFlying(mech) \
if (FlyingT(mech)) { \
  MechStatus(mech) &= ~LANDED; \
  MechZ(mech) += 1; \
  MechFZ(mech) = ZSCALE * MechZ(mech); \
  StopMoving(mech); }

#define Overwater(mech) \
(MechMove(mech) == MOVE_HOVER || MechType(mech) == CLASS_MW || \
 MechMove(mech) == MOVE_FOIL || MechMove(mech) == MOVE_HULL)

#define MoveMod(mech) \
(MechType(mech) == CLASS_MW ? 3 : \
 (MechIsBiped(mech) || MechIsQuad(mech)) ? 2 : 1)

#define IsWater(t)    ((t) == ICE  || (t) == WATER || (t) == BRIDGE)
#define InWater(mech) (IsWater(MechRTerrain((mech))) && MechZ(mech)<0)
#define OnWater(mech) (IsWater(MechRTerrain((mech))) && MechZ(mech)<=0)

#define IsC3(mech) ((MechSpecials(mech) & (C3_MASTER_TECH|C3_SLAVE_TECH)) && !C3Destroyed(mech))
#define IsC3i(mech) ((MechSpecials2(mech) & C3I_TECH) && !C3iDestroyed(mech))
#define IsAMS(weapindx) (MechWeapons[weapindx].special & AMS)

/* Macro for figuring out the truly ugly stuff - whether ammo
   crit is in fact 1 or 2 'half-tons' of ammo */
#define AmmoMod(mech,loc,pos) \
  ((!IsAmmo(GetPartType(mech,loc,pos)) || \
    GetPartFireMode(mech,loc,pos) & HALFTON_MODE || \
		GetPartAmmoMode(mech,loc,pos) & AC_AP_MODE || \
		GetPartAmmoMode(mech,loc,pos) & AC_PRECISION_MODE) ? 1 : ( GetPartAmmoMode(mech,loc,pos) & AC_CASELESS_MODE ? .5 : 2 ) )

#define JumpSpeed(mech,map) \
  ((InGravity(mech) && map) ? (MechJumpSpeed(mech) * 100 / ((MAX(50, MapGravity(map))))) : MechJumpSpeed(mech))
#define JumpSpeedMP(mech,map) \
  ((int) (JumpSpeed(mech,map) * MP_PER_KPH))

#define NotInWater(mech) (!(OnWater(mech)))
#define WaterBeast(mech) (MechMove(mech)==MOVE_HULL || MechMove(mech)==MOVE_FOIL)

#define IsCrap(val) \
  (((val) == Special(ENDO_STEEL)) || ((val) == Special(FERRO_FIBROUS)) || \
   ((val) == Special(TRIPLE_STRENGTH_MYOMER)) || ((val) == Special(STEALTH_ARMOR)) || \
	 ((val) == Special(HVY_FERRO_FIBROUS)) || ((val) == Special(LT_FERRO_FIBROUS)))

#define Spinning(mech)      (MechCritStatus(mech) & SPINNING)
#define StopSpinning(mech)  (MechCritStatus(mech) &= ~SPINNING)
#define StartSpinning(mech) (MechCritStatus(mech) |= SPINNING)

#define MechLit(mech)		(MechCritStatus(mech) & SLITE_LIT)
#define MechLites(mech)		(MechStatus2(mech) & SLITE_ON)
#define IsLit(mech)		(MechLit(mech) || MechLites(mech))

#define OkayCritSect(sect,num,ok) OkayCritSect2(mech,sect,num,ok)
#define OkayCritSectS(sect,num,ok) OkayCritSect(sect,num,I2Special(ok))
#define OkayCritSect2(mech,sect,num,ok) \
  (GetPartType(mech,sect,num)==(ok) && !PartIsNonfunctional(mech,sect,num))
#define OkayCritSectS2(mech,sect,num,ok) OkayCritSect2(mech,sect,num,I2Special(ok))
#define MAPMOVEMOD(map) ((map)->movemod > 0 ? (float) (map)->movemod / 100.0 : 1.0)

#define RCache_Remove(n)
#define RCache_Flush()

/* Ancient remnant ; of pre-06061998 rangecache-code */
#define FaMechRange(mech,target) \
    FindRange(MechFX(mech), MechFY(mech), MechFZ(mech), \
	      MechFX(target), MechFY(target), MechFZ(target))

#define DSBearMod(ds) \
  ((MechFacing(ds) +30) / 60) % 6

/* ECM related macros */
#define ECMActive(mech)				(ECMEnabled(mech) && !ECMCountered(mech))
#define ECCMActive(mech)			ECCMEnabled(mech)

#define ECMEnabled(mech)			(MechStatus2(mech) & ECM_ENABLED)
#define ECCMEnabled(mech)			(MechStatus2(mech) & ECCM_ENABLED)

#define EnableECM(mech) 			(MechStatus2(mech) |= ECM_ENABLED)
#define EnableECCM(mech) 			(MechStatus2(mech) |= ECCM_ENABLED)

#define DisableECM(mech) 			(MechStatus2(mech) &= ~ECM_ENABLED)
#define DisableECCM(mech)			(MechStatus2(mech) &= ~ECCM_ENABLED)

#define PerECMActive(mech)		(PerECMEnabled(mech) && !ECMCountered(mech))
#define PerECCMActive(mech)		PerECCMEnabled(mech)

#define PerECMEnabled(mech)		(MechStatus2(mech) & PER_ECM_ENABLED)
#define PerECCMEnabled(mech)	(MechStatus2(mech) & PER_ECCM_ENABLED)

#define EnablePerECM(mech) 		(MechStatus2(mech) |= PER_ECM_ENABLED)
#define EnablePerECCM(mech) 	(MechStatus2(mech) |= PER_ECCM_ENABLED)

#define DisablePerECM(mech) 	(MechStatus2(mech) &= ~PER_ECM_ENABLED)
#define DisablePerECCM(mech)	(MechStatus2(mech) &= ~PER_ECCM_ENABLED)

#define AnyECMDisturbed(mech)		(ECMDisturbed(mech) || AngelECMDisturbed(mech))
#define AnyECMProtected(mech)		(ECMProtected(mech) || AngelECMProtected(mech))
#define AnyECMActive(mech)		(ECMActive(mech) || AngelECMActive(mech))
#define AnyECCMActive(mech)		(ECCMActive(mech) || AngelECCMActive(mech))

#define ECMDisturbed(mech)		(MechStatus2(mech) & ECM_DISTURBANCE)
#define ECMProtected(mech)		((MechStatus2(mech) & ECM_PROTECTED) || ECMActive(mech) || PerECMActive(mech))

#define ECMCountered(mech)		(MechStatus2(mech) & ECM_COUNTERED)
#define SetECMCountered(mech)		(MechStatus2(mech) |= ECM_COUNTERED)
#define UnSetECMCountered(mech)	(MechStatus2(mech) &= ~ECM_COUNTERED)

#define SetECMDisturbed(mech)		(MechStatus2(mech) |= ECM_DISTURBANCE)
#define UnSetECMDisturbed(mech)	(MechStatus2(mech) &= ~ECM_DISTURBANCE)

#define SetECMProtected(mech)		(MechStatus2(mech) |= ECM_PROTECTED)
#define UnSetECMProtected(mech)	(MechStatus2(mech) &= ~ECM_PROTECTED)

/* Macro to check for ALL ECM types */
#define HasWorkingECMSuite(mech)    (((MechSpecials(mech) & ECM_TECH) && \
                                    !(MechStatus2(mech) & ECM_DESTROYED)) || \
                                    ((MechSpecials2(mech) & ANGEL_ECM_TECH) && \
                                    !(MechStatus2(mech) & ANGEL_ECM_DESTROYED)) || \
                                    (MechInfantrySpecials(mech) & FC_INFILTRATORII_STEALTH_TECH))

/* Angel ECM macros */

#define AngelECMActive(mech)		(AngelECMEnabled(mech) && !ECMCountered(mech))
#define AngelECCMActive(mech)		AngelECCMEnabled(mech)

#define AngelECMEnabled(mech)		(MechStatus2(mech) & ANGEL_ECM_ENABLED)
#define AngelECCMEnabled(mech)	(MechStatus2(mech) & ANGEL_ECCM_ENABLED)

#define EnableAngelECM(mech) 		(MechStatus2(mech) |= ANGEL_ECM_ENABLED)
#define EnableAngelECCM(mech) 	(MechStatus2(mech) |= ANGEL_ECCM_ENABLED)

#define DisableAngelECM(mech) 	(MechStatus2(mech) &= ~ANGEL_ECM_ENABLED)
#define DisableAngelECCM(mech)	(MechStatus2(mech) &= ~ANGEL_ECCM_ENABLED)

#define AngelECMProtected(mech)	((MechStatus2(mech) & ANGEL_ECM_PROTECTED) || AngelECMActive(mech))
#define AngelECMDisturbed(mech)	(MechStatus2(mech) & ANGEL_ECM_DISTURBED)

#define SetAngelECMDisturbed(mech)	 (MechStatus2(mech) |= ANGEL_ECM_DISTURBED)
#define UnSetAngelECMDisturbed(mech) (MechStatus2(mech) &= ~ANGEL_ECM_DISTURBED)

#define SetAngelECMProtected(mech)	 (MechStatus2(mech) |= ANGEL_ECM_PROTECTED)
#define UnSetAngelECMProtected(mech) (MechStatus2(mech) &= ~ANGEL_ECM_PROTECTED)

#define HasWorkingAngelECMSuite(mech)   ((MechSpecials2(mech) & ANGEL_ECM_TECH) && \
                                        !(MechStatus2(mech) & ANGEL_ECM_DESTROYED))

/* Stealth system macros */
#define StealthArmorActive(mech)		(MechStatus2(mech) & STH_ARMOR_ON)
#define EnableStealthArmor(mech)		(MechStatus2(mech) |= STH_ARMOR_ON)
#define DisableStealthArmor(mech)		(MechStatus2(mech) &= ~STH_ARMOR_ON)
#define StealthArmorChanging(mech)	muxevent_count_type_data(EVENT_STEALTH_ARMOR, (void *) mech)

#define DestroyNullSigSys(mech)	(MechCritStatus(mech) |= NSS_DESTROYED)
#define NullSigSysDest(mech)	  (MechCritStatus(mech) & NSS_DESTROYED)
#define NullSigSysActive(mech)  (MechStatus2(mech) & NULLSIGSYS_ON)
#define EnableNullSigSys(mech)  (MechStatus2(mech) |= NULLSIGSYS_ON)
#define DisableNullSigSys(mech) (MechStatus2(mech) &= ~NULLSIGSYS_ON)
#define NullSigSysChanging(mech) muxevent_count_type_data(EVENT_NSS, (void *) mech)

/* C3 macros */

#define HasC3(mech)							(HasC3m(mech) || HasC3s(mech))
#define HasC3m(mech)						((MechSpecials(mech) & C3_MASTER_TECH))
#define HasC3s(mech)						((MechSpecials(mech) & C3_SLAVE_TECH))
#define C3Destroyed(mech)				(MechCritStatus(mech) & C3_DESTROYED)
#define MechC3Network(a)	 			(a)->sd.C3Network
#define MechC3NetworkElem(a,b)	(a)->sd.C3Network[b]
#define MechC3NetworkSize(a)		(a)->sd.wC3NetworkSize
#define MechTotalC3Masters(a)		(a)->sd.wTotalC3Masters
#define MechWorkingC3Masters(a) (a)->sd.wWorkingC3Masters

/* Improved C3 macros */

#define HasC3i(mech)						((MechSpecials2(mech) & C3I_TECH))
#define C3iDestroyed(mech)			(MechCritStatus(mech) & C3I_DESTROYED)
#define MechC3iNetwork(a)	 			(a)->sd.C3iNetwork
#define MechC3iNetworkElem(a,b)	(a)->sd.C3iNetwork[b]
#define MechC3iNetworkSize(a)		(a)->sd.wC3iNetworkSize

/* TAG macros */

#define HasTAG(mech)						( (MechSpecials2(mech) & TAG_TECH) || HasC3m(mech) )
#define TAGTarget(mech)					(mech)->sd.tagTarget
#define TaggedBy(mech)			    (mech)->sd.taggedBy
#define TagRecycling(a)          muxevent_count_type_data(EVENT_TAG_RECYCLE,(void *) a)

/* Club stuff */
#define CarryingClub(mech)			( (MechSections(mech)[RARM].specials & CARRYING_CLUB) || (MechSections(mech)[LARM].specials & CARRYING_CLUB))
#define DropClub(mech)					do { if(CarryingClub(mech)) { MechSections(mech)[RARM].specials &= ~CARRYING_CLUB; MechSections(mech)[LARM].specials &= ~CARRYING_CLUB; mech_notify(mech, MECHALL, "Your club falls to the ground and shatters."); MechLOSBroadcast(mech, "'s club falls to the ground and shatters."); } } while (0)

/* New stagger stuff */
#define Staggering(mech)        ( StaggerLevel(mech) > 0 )
#define CheckingStaggerDamage(mech) muxevent_count_type_data(EVENT_CHECK_STAGGER,(void *) mech)
#define StartStaggerCheck(mech)   do { MECHEVENT(mech, EVENT_CHECK_STAGGER, check_stagger_event, 5, 0); SendDebug(tprintf("Starting stagger check for %d.", mech->mynum)); } while (0)
#define StopStaggerCheck(mech)    do { muxevent_remove_type_data(EVENT_CHECK_STAGGER, (void *) mech); (mech)->rd.staggerDamage=0; (mech)->rd.lastStaggerNotify=0; SendDebug(tprintf("Stopping stagger check for %d.", mech->mynum)); } while (0)
#define StaggerDamage(mech)     ( (mech)->rd.staggerDamage )
#define LastStaggerNotify(mech) ( (mech)->rd.lastStaggerNotify )
#define StaggerLevel(mech)      ( (mech)->rd.staggerDamage / 20 )

#define MechIsObservator(mech)		(MechCritStatus(mech) & OBSERVATORIC)

/* Macros related to map.h stuff */
#define MechLOSFlag_WoodCount(flag)	\
			(((flag) / MECHLOSFLAG_WOOD) % MECHLOSMAX_WOOD)
#define MechLOSFlag_WaterCount(flag)	\
			(((flag) / MECHLOSFLAG_WATER) % MECHLOSMAX_WATER)

#define MechToMech_LOSFlag(map, from, to)	\
			((map)->LOSinfo[from->mapnumber][to->mapnumber])

#define MoveModeChange(a)	muxevent_count_type_data(EVENT_MOVEMODE,(void *) a)
#define MoveModeLock(a)		(MechStatus2(a) & MOVE_MODES_LOCK || (MoveModeChange(a) && !(MechStatus2(a) & DODGING)))
#define MoveModeData(a)		muxevent_count_type_data_firstev(EVENT_MOVEMODE, (void *) a)
#define StopMoveMode(a)		muxevent_remove_type_data(EVENT_MOVEMODE, (void *) a)
#define Sprinting(a)		(MechStatus2(a) & SPRINTING)
#define Evading(a)		(MechStatus2(a) & EVADING)
#define Dodging(a)		(MechStatus2(a) & DODGING)
#define SideSlipping(a)		muxevent_count_type_data(EVENT_SIDESLIP, (void *) a)
#define StopSideslip(a)		muxevent_remove_type_data(EVENT_SIDESLIP, (void *) a)
											   
#endif				/* BTMACROS_H */
