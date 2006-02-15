/*
 *  Copyright (c) 1999-2005 Kevin Stevens
 *	All rights reserved
 */

/* 
   Code to read and write mech and vehicle templates
   Created by Nim 9/16/96

   $Id: template.c,v 1.9 2005/08/10 14:09:34 av1-op Exp $
   Last modified: Fri Sep 18 13:02:31 1998 fingon
 */

/* 01/21/02 Added many commods <KM> */

/* 09/16/96 Some touches by Markus Stenberg <fingon@iki.fi> */

/* 09/16/96 Some?? ... ya right ;-) (nim)                  */

/* 09/17/96 Ok, ton of touches then :-P (Mark) */

#include "config.h"

#define MAX_STRING_LENGTH 8192
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mech.h"
#include "create.h"
#include "mech.events.h"
#include "coolmenu.h"
#include "failures.h"
#include "aero.bomb.h"
#include "mech.partnames.h"
#include "p.mech.utils.h"
#include "p.mech.partnames.h"
#include "p.mech.consistency.h"
#include "p.map.conditions.h"
#include "p.aero.bomb.h"
#include "p.mech.mechref_ident.h"
#include "p.crit.h"
#include "p.bsuit.h"
#include "p.mech.c3.h"

#define MODE_UNKNOWN 0
#define MODE_NORMAL 1

char *load_cmds[] = {
	"Reference", "Type", "Move_Type", "Tons", "Tac_Range",
	"LRS_Range", "Radio_Range", "Scan_Range", "Heat_Sinks",
	"Max_Speed", "Specials", "Armor", "Internals", "Rear",
	"Config", "Computer", "Name", "Jump_Speed", "Radio",
	"SI", "Fuel", "Comment", "RadioType",
	"Mech_BV", "Cargo_Space", "Max_Suits", "InfantrySpecials",
	"Max_Ton",
	NULL
};

char *internals[] = {
	"ShoulderOrHip",
	"UpperActuator",
	"LowerActuator",
	"HandOrFootActuator",
	"LifeSupport",
	"Sensors",
	"Cockpit",
	"Engine",
	"Gyro",
	"HeatSink",
	"JumpJet",
	"Case",
	"FerroFibrous",
	"EndoSteel",
	"TripleStrengthMyomer",
	"TargetingComputer",
	"Masc",
	"C3Master",
	"C3Slave",
	"BeagleProbe",
	"ArtemisIV",
	"Ecm",
	"Axe",
	"Sword",
	"Mace",
	"Claw",
	"DSAeroDoor",
	"DSMechDoor",
	"Fuel_Tank",
	"TAG",
	"DSVehicleDoor",
	"DSCargoDoor",
	"LAM_Equipment",
	"CaseII",
	"StealthArmor",
	"NullSig_Device",
	"C3i",
	"AngelEcm",
	"HvyFerroFibrous",
	"LtFerroFibrous",
	"BloodhoundProbe",
	"PurifierArmor",
	"KageStealthUnit",
	"AchileusStealthUnit",
	"InfiltratorStealthUnit",
	"InfiltratorIIStealthUnit",
	"SuperCharger",
	"Dual_Saw",
	NULL
};

#ifdef BT_PART_WEIGHTS
int internalsweight[] = {
	102,						/* ShoulderOrHip */
	102,						/* UpperActuator */
	102,						/* LowerActuator */
	102,						/* HandOrFootActuator */
	204,						/* LifeSupport */
	102,						/* Sensors */
	512,						/* Cockpit */
	1024,						/* Engine */
	512,						/* Gyro */
	204,						/* HeatSink */
	204,						/* JumpJet */
	51,							/* Case */
	153,						/* FerroFibrous */
	153,						/* EndoSteel */
	51,							/* TripleStrengthMyomer */
	204,						/* TargetingComputer */
	51,							/* Masc */
	512,						/* C3Master */
	102,						/* C3Slave */
	102,						/* BeagleProbe */
	153,						/* ArtemisIV */
	204,						/* Ecm */
	51,							/* Axe */
	25,							/* Sword */
	102,						/* Mace */
	75,							/* Claw */
	1024,						/* DSAeroDoor */
	1024,						/* DSMechDoor */
	102,						/* Fuel_Tank */
	51,							/* TAG */
	1024,						/* DSVehicleDoor */
	1024,						/* DSCargoDoor */
	306,						/* LAM_Equipment */
	306,						/* CaseII */
	3,							/* StealthArmor */
	1024,						/* NullSig_Device */
	512,						/* C3i */
	204,						/* AngelECM */
	7,							/* HvyFerroFibrous */
	1,							/* LtFerroFibrous */
	102,						/* BloodhoundProbe */
	2,							/* PurifierArmor */
	102,						/* KageStealthUnit */
	102,						/* AchileusStealthUnit */
	102,						/* InfiltratorStealthUnit */
	102,						/* InfiltratorIIStealthUnit */
	1024,						/* Dual_Saw */
};
#endif /* BT_PART_WEIGHTS */

char *cargo[] = {
	"Ammo_LBX2",
	"Ammo_LBX5_LBX",
	"Ammo_LBX10_LBX",
	"Ammo_LBX20_LBX",
	"Ammo_LRM",
	"Ammo_SRM",
	"Ammo_SSRM",
	"Ammo_LRM_NARC",
	"Ammo_SRM_NARC",
	"Ammo_SSRM_NARC",
	"Ammo_LRM_Artemis",
	"Ammo_SRM_Artemis",
	"Ammo_SSRM_Artemis",
	"Petroleum",
	"Phosphorus",
	"Hydrogen",
	"Gold",
	"Natural_Extracts",
	"Marijuana",
	"Sulfur",
	"Sodium",
	"Plutonium",
	"Ore",
	"Steel",
	"Plastics",
	"Medical_Supplies",
	"Computers",
	"Explosives",
	"ES_Internal",
	"FF_Armor",
	"XL_Engine",
	"Double_HeatSink",
	"ICE_Engine",
	"Electric",
	"Internal",
	"Armor",
	"Actuator",
	"Aero_Fuel",
	"DS_Fuel",
	"VTOL_Fuel",
	"Ammo_LRM_Swarm",
	"Ammo_LRM_Swarm1",
	"Ammo_SRM_Inferno",
	"XXL_Engine",
	"Compact_Engine",
	"HD_Armor",
	"RE_Internals",
	"CO_Internals",
	"Ammo_MRM",
	"Light_Engine",
	"CaseII",
	"STH_Armor",
	"NullSigSys",
	"Silicon",
	"HVY_FF_Armor",
	"LT_FF_Armor",
	"Ammo_iNARC_Explosive",
	"Ammo_iNARC_Haywire",
	"Ammo_iNARC_ECM",
	"Ammo_iNARC_Nemesis",
	"Ammo_AC2_Pierce",
	"Ammo_AC5_Pierce",
	"Ammo_AC10_Pierce",
	"Ammo_AC20_Pierce",
	"Ammo_LAC2_Pierce",
	"Ammo_LAC5_Pierce",
	"Ammo_AC2_Flechette",
	"Ammo_AC5_Flechette",
	"Ammo_AC10_Flechette",
	"Ammo_AC20_Flechette",
	"Ammo_LAC2_Flechette",
	"Ammo_LAC5_Flechette",
	"Ammo_AC2_Incendiary",
	"Ammo_AC5_Incendiary",
	"Ammo_AC10_Incendiary",
	"Ammo_AC20_Incendiary",
	"Ammo_LAC2_Incendiary",
	"Ammo_LAC5_Incendiary",
	"Ammo_AC2_Precision",
	"Ammo_AC5_Precision",
	"Ammo_AC10_Precision",
	"Ammo_AC20_Precision",
	"Ammo_LAC2_Precision",
	"Ammo_LAC5_Precision",
	"Ammo_LR_DFM",
	"Ammo_SR_DFM",
	"Ammo_SLRM",
	"Ammo_ELRM",
	"BSuit_Sensor",
	"BSuit_LifeSupport",
	"BSuit_Electronic",
	"Oil",
	"Water",
	"Earth",
	"Oxygen",
	"Nitrogen",
	"Nickel",
	"Steel",
	"Iron",
	"Brass",
	"Platinum",
	"Copper",
	"Aluminum",
	"Consumer_Good",
	"Machinery",
	"Slaves",
	"Timbiqui_Dark",
	"Cocaine",
	"Heroine",
	"Marble",
	"Glass",
	"Diamond",
	"Coal",
	"Food",
	"Zinc",
	"Fabric",
	"Clothing",
	"Wood",
	"Pulp",
	"Lumber",
	"Rubber",
	"Seeds",
	"Fertilizer",
	"Salt",
	"Lithium",
	"Helium",
	"Larium",
	"Uranium",
	"Iridium",
	"Titanium",
	"Concrete",
	"FerroCrete",
	"Building_Supplies",
	"Kevlar",
	"Waste",
	"Livestock",
	"Paper",
	"XL_Gyro",
	"HeavyDuty_Gyro",
	"Compact_Gyro",
	"Compact_HeatSink",
	"SearchLight",
#ifdef BT_COMPLEXREPAIRS
	"Sensors_10",
	"Sensors_20",
	"Sensors_30",
	"Sensors_40",
	"Sensors_50",
	"Sensors_60",
	"Sensors_70",
	"Sensors_80",
	"Sensors_90",
	"Sensors_100",
	"Myomer_10",
	"Myomer_20",
	"Myomer_30",
	"Myomer_40",
	"Myomer_50",
	"Myomer_60",
	"Myomer_70",
	"Myomer_80",
	"Myomer_90",
	"Myomer_100",
	"TripleMyomer_10",
	"TripleMyomer_20",
	"TripleMyomer_30",
	"TripleMyomer_40",
	"TripleMyomer_50",
	"TripleMyomer_60",
	"TripleMyomer_70",
	"TripleMyomer_80",
	"TripleMyomer_90",
	"TripleMyomer_100",
	"Internal_10",
	"Internal_20",
	"Internal_30",
	"Internal_40",
	"Internal_50",
	"Internal_60",
	"Internal_70",
	"Internal_80",
	"Internal_90",
	"Internal_100",
	"ES_Internal_10",
	"ES_Internal_20",
	"ES_Internal_30",
	"ES_Internal_40",
	"ES_Internal_50",
	"ES_Internal_60",
	"ES_Internal_70",
	"ES_Internal_80",
	"ES_Internal_90",
	"ES_Internal_100",
	"JumpJet_10",
	"JumpJet_20",
	"JumpJet_30",
	"JumpJet_40",
	"JumpJet_50",
	"JumpJet_60",
	"JumpJet_70",
	"JumpJet_80",
	"JumpJet_90",
	"JumpJet_100",
	"UpperArmActuator_10",
	"UpperArmActuator_20",
	"UpperArmActuator_30",
	"UpperArmActuator_40",
	"UpperArmActuator_50",
	"UpperArmActuator_60",
	"UpperArmActuator_70",
	"UpperArmActuator_80",
	"UpperArmActuator_90",
	"UpperArmActuator_100",
	"LowerArmActuator_10",
	"LowerArmActuator_20",
	"LowerArmActuator_30",
	"LowerArmActuator_40",
	"LowerArmActuator_50",
	"LowerArmActuator_60",
	"LowerArmActuator_70",
	"LowerArmActuator_80",
	"LowerArmActuator_90",
	"LowerArmActuator_100",
	"HandActuator_10",
	"HandActuator_20",
	"HandActuator_30",
	"HandActuator_40",
	"HandActuator_50",
	"HandActuator_60",
	"HandActuator_70",
	"HandActuator_80",
	"HandActuator_90",
	"HandActuator_100",
	"UpperLegActuator_10",
	"UpperLegActuator_20",
	"UpperLegActuator_30",
	"UpperLegActuator_40",
	"UpperLegActuator_50",
	"UpperLegActuator_60",
	"UpperLegActuator_70",
	"UpperLegActuator_80",
	"UpperLegActuator_90",
	"UpperLegActuator_100",
	"LowerLegActuator_10",
	"LowerLegActuator_20",
	"LowerLegActuator_30",
	"LowerLegActuator_40",
	"LowerLegActuator_50",
	"LowerLegActuator_60",
	"LowerLegActuator_70",
	"LowerLegActuator_80",
	"LowerLegActuator_90",
	"LowerLegActuator_100",
	"FootActuator_10",
	"FootActuator_20",
	"FootActuator_30",
	"FootActuator_40",
	"FootActuator_50",
	"FootActuator_60",
	"FootActuator_70",
	"FootActuator_80",
	"FootActuator_90",
	"FootActuator_100",
	"Engine_20",
	"Engine_40",
	"Engine_60",
	"Engine_80",
	"Engine_100",
	"Engine_120",
	"Engine_140",
	"Engine_160",
	"Engine_180",
	"Engine_200",
	"Engine_220",
	"Engine_240",
	"Engine_260",
	"Engine_280",
	"Engine_300",
	"Engine_320",
	"Engine_340",
	"Engine_360",
	"Engine_380",
	"Engine_400",
	"XL_Engine_20",
	"XL_Engine_40",
	"XL_Engine_60",
	"XL_Engine_80",
	"XL_Engine_100",
	"XL_Engine_120",
	"XL_Engine_140",
	"XL_Engine_160",
	"XL_Engine_180",
	"XL_Engine_200",
	"XL_Engine_220",
	"XL_Engine_240",
	"XL_Engine_260",
	"XL_Engine_280",
	"XL_Engine_300",
	"XL_Engine_320",
	"XL_Engine_340",
	"XL_Engine_360",
	"XL_Engine_380",
	"XL_Engine_400",
	"ICE_Engine_20",
	"ICE_Engine_40",
	"ICE_Engine_60",
	"ICE_Engine_80",
	"ICE_Engine_100",
	"ICE_Engine_120",
	"ICE_Engine_140",
	"ICE_Engine_160",
	"ICE_Engine_180",
	"ICE_Engine_200",
	"ICE_Engine_220",
	"ICE_Engine_240",
	"ICE_Engine_260",
	"ICE_Engine_280",
	"ICE_Engine_300",
	"ICE_Engine_320",
	"ICE_Engine_340",
	"ICE_Engine_360",
	"ICE_Engine_380",
	"ICE_Engine_400",
	"Light_Engine_20",
	"Light_Engine_40",
	"Light_Engine_60",
	"Light_Engine_80",
	"Light_Engine_100",
	"Light_Engine_120",
	"Light_Engine_140",
	"Light_Engine_160",
	"Light_Engine_180",
	"Light_Engine_200",
	"Light_Engine_220",
	"Light_Engine_240",
	"Light_Engine_260",
	"Light_Engine_280",
	"Light_Engine_300",
	"Light_Engine_320",
	"Light_Engine_340",
	"Light_Engine_360",
	"Light_Engine_380",
	"Light_Engine_400",
	"CO_Internal_10",
	"CO_Internal_20",
	"CO_Internal_30",
	"CO_Internal_40",
	"CO_Internal_50",
	"CO_Internal_60",
	"CO_Internal_70",
	"CO_Internal_80",
	"CO_Internal_90",
	"CO_Internal_100",
	"RE_Internal_10",
	"RE_Internal_20",
	"RE_Internal_30",
	"RE_Internal_40",
	"RE_Internal_50",
	"RE_Internal_60",
	"RE_Internal_70",
	"RE_Internal_80",
	"RE_Internal_90",
	"RE_Internal_100",
	"Gyro_1",
	"Gyro_2",
	"Gyro_3",
	"Gyro_4",
	"XL_Gyro_1",
	"XL_Gyro_2",
	"XL_Gyro_3",
	"XL_Gyro_4",
	"HD_Gyro_1",
	"HD_Gyro_2",
	"HD_Gyro_3",
	"HD_Gyro_4",
	"Compact_Gyro_1",
	"Compact_Gyro_2",
	"Compact_Gyro_3",
	"Compact_Gyro_4",
	"XXL_Engine_20",
	"XXL_Engine_40",
	"XXL_Engine_60",
	"XXL_Engine_80",
	"XXL_Engine_100",
	"XXL_Engine_120",
	"XXL_Engine_140",
	"XXL_Engine_160",
	"XXL_Engine_180",
	"XXL_Engine_200",
	"XXL_Engine_220",
	"XXL_Engine_240",
	"XXL_Engine_260",
	"XXL_Engine_280",
	"XXL_Engine_300",
	"XXL_Engine_320",
	"XXL_Engine_340",
	"XXL_Engine_360",
	"XXL_Engine_380",
	"XXL_Engine_400",
	"Compact_Engine_20",
	"Compact_Engine_40",
	"Compact_Engine_60",
	"Compact_Engine_80",
	"Compact_Engine_100",
	"Compact_Engine_120",
	"Compact_Engine_140",
	"Compact_Engine_160",
	"Compact_Engine_180",
	"Compact_Engine_200",
	"Compact_Engine_220",
	"Compact_Engine_240",
	"Compact_Engine_260",
	"Compact_Engine_280",
	"Compact_Engine_300",
	"Compact_Engine_320",
	"Compact_Engine_340",
	"Compact_Engine_360",
	"Compact_Engine_380",
	"Compact_Engine_400",
#endif
	NULL
};

#ifdef BT_PART_WEIGHTS
int cargoweight[] = {
	1024,						/* Ammo_LBX2 */
	1024,						/* Ammo_LBX5_LBX */
	1024,						/* Ammo_LBX10_LBX */
	1024,						/* Ammo_LBX20_LBX */
	1024,						/* Ammo_LRM */
	1024,						/* Ammo_SRM */
	1024,						/* Ammo_SSRM */
	1024,						/* Ammo_LRM_NARC */
	1024,						/* Ammo_SRM_NARC */
	1024,						/* Ammo_SSRM_NARC */
	1024,						/* Ammo_LRM_Artemis */
	1024,						/* Ammo_SRM_Artemis */
	1024,						/* Ammo_SSRM_Artemis */
	102,						/* Petroleum */
	102,						/* Phosphorus */
	102,						/* Hydrogen */
	204,						/* Gold */
	51,							/* Natural_Extracts */
	10,							/* Marijuana */
	10,							/* Sulfur */
	10,							/* Sodium */
	306,						/* Plutonium */
	51,							/* Ore */
	204,						/* Steel */
	51,							/* Plastics */
	8,							/* Medical_Supplies */
	306,						/* Computers */
	204,						/* Explosives */
	5,							/* ES_Internal */
	2,							/* FF_Armor */
	512,						/* XL_Engine */
	342,						/* Double_Heatsink */
	512,						/* ICE_Engine */
	51,							/* Electric */
	5,							/* Internal */
	3,							/* Armor */
	102,						/* Actuator */
	10,							/* Aero_Fuel */
	20,							/* DS_Fuel */
	2,							/* VTOL_Fuel */
	1024,						/* Ammo_LRM_Swarm */
	1024,						/* Ammo_LRM_Swarm1 */
	1024,						/* Ammo_SRM_Inferno */
	1024,						/* XXL_Engine */
	2048,						/* Compact_Engine */
	6,							/* HD_Armor */
	4,							/* RE_Internals */
	2,							/* CO_Internals */
	1024,						/* Ammo_MRM */
	512,						/* Light_Engine */
	62,							/* CaseII */
	3,							/* STH_Armor */
	1024,						/* NullSigSys */
	80,							/* Silicon */
	7,							/* HVY_FF_Armor */
	1,							/* LT_FF_Armor */
	1024,						/* Ammo_iNARC_Explosive */
	1024,						/* Ammo_iNARC_Haywire */
	1024,						/* Ammo_iNARC_ECM */
	1024,						/* Ammo_iNARC_Nemesis */
	1024,						/* Ammo_AC2_AP */
	1024,						/* Ammo_AC5_AP */
	1024,						/* Ammo_AC10_AP */
	1024,						/* Ammo_AC20_AP */
	1024,						/* Ammo_LAC2_AP */
	1024,						/* Ammo_LAC5_AP */
	1024,						/* Ammo_AC2_Flechette */
	1024,						/* Ammo_AC5_Flechette */
	1024,						/* Ammo_AC10_Flechette */
	1024,						/* Ammo_AC20_Flechette */
	1024,						/* Ammo_LAC2_Flechette */
	1024,						/* Ammo_LAC5_Flechette */
	1024,						/* Ammo_AC2_Incendiary */
	1024,						/* Ammo_AC5_Incendiary */
	1024,						/* Ammo_AC10_Incendiary */
	1024,						/* Ammo_AC20_Incendiary */
	1024,						/* Ammo_LAC2_Incendiary */
	1024,						/* Ammo_LAC5_Incendiary */
	1024,						/* Ammo_AC2_Precision */
	1024,						/* Ammo_AC5_Precision */
	1024,						/* Ammo_AC10_Precision */
	1024,						/* Ammo_AC20_Precision */
	1024,						/* Ammo_LAC2_Precision */
	1024,						/* Ammo_LAC5_Precision */
	1024,						/* Ammo_LR_DFM */
	1024,						/* Ammo_SR_DFM */
	1024,						/* Ammo_SLRM */
	1024,						/* Ammo_ELRM */
	102,						/* BSuit_Sensor */
	61,							/* BSuit_LifeSupport */
	10,							/* BSuit_Electronic */
	306,						/* Oil */
	102,						/* Water */
	204,						/* Earth */
	21,							/* Oxygen */
	102,						/* Nitrogen */
	102,						/* Nickel */
	204,						/* Steel */
	204,						/* Iron */
	204,						/* Brass */
	204,						/* Platinum */
	204,						/* Copper */
	204,						/* Aluminum */
	61,							/* Consumer_Good */
	312,						/* Machinery */
	102,						/* Slaves */
	21,							/* Timbiqui_Dark */
	10,							/* Cocaine */
	10,							/* Heroine */
	424,						/* Marble */
	408,						/* Glass */
	204,						/* Diamond */
	306,						/* Coal */
	204,						/* Food */
	102,						/* Zinc */
	204,						/* Fabric */
	204,						/* Clothing */
	408,						/* Wood */
	204,						/* Pulp */
	520,						/* Lumber */
	204,						/* Rubber */
	102,						/* Seeds */
	102,						/* Fertilizer */
	102,						/* Salt */
	204,						/* Lithium */
	102,						/* Helium */
	306,						/* Larium */
	306,						/* Uranium */
	306,						/* Iridium */
	408,						/* Titanium */
	306,						/* Concrete */
	204,						/* FerroCrete */
	204,						/* Building_Supplies */
	204,						/* Kevlar */
	204,						/* Waste */
	308,						/* Livestock */
	102,						/* Paper */
	1024,						/* XL_Gyro */
	2048,						/* HeavyDuty_Gyro */
	2048,						/* Compact_Gyro */
	2048,						/* Compact_HeatSink */
	204,						/* SearchLight */
#ifdef BT_COMPLEXREPAIRS
	102,						/* Sensors_10 */
	204,						/* Sensors_20 */
	306,						/* Sensors_30 */
	408,						/* Sensors_40 */
	510,						/* Sensors_50 */
	612,						/* Sensors_60 */
	714,						/* Sensors_70 */
	816,						/* Sensors_80 */
	918,						/* Sensors_90 */
	1020,						/* Sensors_100 */
	102,						/* Myomer_10 */
	102,						/* Myomer_20 */
	102,						/* Myomer_30 */
	102,						/* Myomer_40 */
	102,						/* Myomer_50 */
	102,						/* Myomer_60 */
	102,						/* Myomer_70 */
	102,						/* Myomer_80 */
	102,						/* Myomer_90 */
	102,						/* Myomer_100 */
	102,						/* TripleMyomer_10 */
	102,						/* TripleMyomer_20 */
	102,						/* TripleMyomer_30 */
	102,						/* TripleMyomer_40 */
	102,						/* TripleMyomer_50 */
	102,						/* TripleMyomer_60 */
	102,						/* TripleMyomer_70 */
	102,						/* TripleMyomer_80 */
	102,						/* TripleMyomer_90 */
	102,						/* TripleMyomer_100 */
	128,						/* Internal_10 */
	128,						/* Internal_20 */
	128,						/* Internal_30 */
	128,						/* Internal_40 */
	128,						/* Internal_50 */
	128,						/* Internal_60 */
	128,						/* Internal_70 */
	128,						/* Internal_80 */
	128,						/* Internal_90 */
	128,						/* Internal_100 */
	128,						/* ES_Internal_10 */
	128,						/* ES_Internal_20 */
	128,						/* ES_Internal_30 */
	128,						/* ES_Internal_40 */
	128,						/* ES_Internal_50 */
	128,						/* ES_Internal_60 */
	128,						/* ES_Internal_70 */
	128,						/* ES_Internal_80 */
	128,						/* ES_Internal_90 */
	128,						/* ES_Internal_100 */
	102,						/* JumpJet_10 */
	204,						/* JumpJet_20 */
	306,						/* JumpJet_30 */
	408,						/* JumpJet_40 */
	510,						/* JumpJet_50 */
	612,						/* JumpJet_60 */
	714,						/* JumpJet_70 */
	816,						/* JumpJet_80 */
	918,						/* JumpJet_90 */
	1020,						/* JumpJet_100 */
	8,							/* UpperArmActuator_10 */
	8,							/* UpperArmActuator_20 */
	8,							/* UpperArmActuator_30 */
	8,							/* UpperArmActuator_40 */
	8,							/* UpperArmActuator_50 */
	8,							/* UpperArmActuator_60 */
	8,							/* UpperArmActuator_70 */
	8,							/* UpperArmActuator_80 */
	8,							/* UpperArmActuator_90 */
	8,							/* UpperArmActuator_100 */
	8,							/* LowerArmActuator_10 */
	8,							/* LowerArmActuator_20 */
	8,							/* LowerArmActuator_30 */
	8,							/* LowerArmActuator_40 */
	8,							/* LowerArmActuator_50 */
	8,							/* LowerArmActuator_60 */
	8,							/* LowerArmActuator_70 */
	8,							/* LowerArmActuator_80 */
	8,							/* LowerArmActuator_90 */
	8,							/* LowerArmActuator_100 */
	8,							/* HandActuator_10 */
	8,							/* HandActuator_20 */
	8,							/* HandActuator_30 */
	8,							/* HandActuator_40 */
	8,							/* HandActuator_50 */
	8,							/* HandActuator_60 */
	8,							/* HandActuator_70 */
	8,							/* HandActuator_80 */
	8,							/* HandActuator_90 */
	8,							/* HandActuator_100 */
	8,							/* UpperLegActuator_10 */
	8,							/* UpperLegActuator_20 */
	8,							/* UpperLegActuator_30 */
	8,							/* UpperLegActuator_40 */
	8,							/* UpperLegActuator_50 */
	8,							/* UpperLegActuator_60 */
	8,							/* UpperLegActuator_70 */
	8,							/* UpperLegActuator_80 */
	8,							/* UpperLegActuator_90 */
	8,							/* UpperLegActuator_100 */
	8,							/* LowerLegActuator_10 */
	8,							/* LowerLegActuator_20 */
	8,							/* LowerLegActuator_30 */
	8,							/* LowerLegActuator_40 */
	8,							/* LowerLegActuator_50 */
	8,							/* LowerLegActuator_60 */
	8,							/* LowerLegActuator_70 */
	8,							/* LowerLegActuator_80 */
	8,							/* LowerLegActuator_90 */
	8,							/* LowerLegActuator_100 */
	8,							/* FootActuator_10 */
	8,							/* FootActuator_20 */
	8,							/* FootActuator_30 */
	8,							/* FootActuator_40 */
	8,							/* FootActuator_50 */
	8,							/* FootActuator_60 */
	8,							/* FootActuator_70 */
	8,							/* FootActuator_80 */
	8,							/* FootActuator_90 */
	8,							/* FootActuator_100 */
	51,							/* Engine_20 */
	102,						/* Engine_40 */
	153,						/* Engine_60 */
	204,						/* Engine_80 */
	255,						/* Engine_100 */
	306,						/* Engine_120 */
	357,						/* Engine_140 */
	408,						/* Engine_160 */
	459,						/* Engine_180 */
	510,						/* Engine_200 */
	561,						/* Engine_220 */
	612,						/* Engine_240 */
	763,						/* Engine_260 */
	814,						/* Engine_280 */
	865,						/* Engine_300 */
	916,						/* Engine_320 */
	967,						/* Engine_340 */
	1018,						/* Engine_360 */
	1070,						/* Engine_380 */
	1020,						/* Engine_400 */
	51,							/* XL_Engine_20 */
	102,						/* XL_Engine_40 */
	153,						/* XL_Engine_60 */
	204,						/* XL_Engine_80 */
	255,						/* XL_Engine_100 */
	306,						/* XL_Engine_120 */
	357,						/* XL_Engine_140 */
	408,						/* XL_Engine_160 */
	459,						/* XL_Engine_180 */
	510,						/* XL_Engine_200 */
	561,						/* XL_Engine_220 */
	612,						/* XL_Engine_240 */
	763,						/* XL_Engine_260 */
	814,						/* XL_Engine_280 */
	865,						/* XL_Engine_300 */
	916,						/* XL_Engine_320 */
	967,						/* XL_Engine_340 */
	1018,						/* XL_Engine_360 */
	1070,						/* XL_Engine_380 */
	1020,						/* XL_Engine_400 */
	51,							/* ICE_Engine_20 */
	102,						/* ICE_Engine_40 */
	153,						/* ICE_Engine_60 */
	204,						/* ICE_Engine_80 */
	255,						/* ICE_Engine_100 */
	306,						/* ICE_Engine_120 */
	357,						/* ICE_Engine_140 */
	408,						/* ICE_Engine_160 */
	459,						/* ICE_Engine_180 */
	510,						/* ICE_Engine_200 */
	561,						/* ICE_Engine_220 */
	612,						/* ICE_Engine_240 */
	763,						/* ICE_Engine_260 */
	814,						/* ICE_Engine_280 */
	865,						/* ICE_Engine_300 */
	916,						/* ICE_Engine_320 */
	967,						/* ICE_Engine_340 */
	1018,						/* ICE_Engine_360 */
	1070,						/* ICE_Engine_380 */
	1020,						/* ICE_Engine_400 */
	51,							/* Light_Engine_20 */
	102,						/* Light_Engine_40 */
	153,						/* Light_Engine_60 */
	204,						/* Light_Engine_80 */
	255,						/* Light_Engine_100 */
	306,						/* Light_Engine_120 */
	357,						/* Light_Engine_140 */
	408,						/* Light_Engine_160 */
	459,						/* Light_Engine_180 */
	510,						/* Light_Engine_200 */
	561,						/* Light_Engine_220 */
	612,						/* Light_Engine_240 */
	763,						/* Light_Engine_260 */
	814,						/* Light_Engine_280 */
	865,						/* Light_Engine_300 */
	916,						/* Light_Engine_320 */
	967,						/* Light_Engine_340 */
	1018,						/* Light_Engine_360 */
	1070,						/* Light_Engine_380 */
	1020,						/* Light_Engine_400 */
	128,						/* CO_Internal_10 */
	128,						/* CO_Internal_20 */
	128,						/* CO_Internal_30 */
	128,						/* CO_Internal_40 */
	128,						/* CO_Internal_50 */
	128,						/* CO_Internal_60 */
	128,						/* CO_Internal_70 */
	128,						/* CO_Internal_80 */
	128,						/* CO_Internal_90 */
	128,						/* CO_Internal_100 */
	128,						/* RE_Internal_10 */
	128,						/* RE_Internal_20 */
	128,						/* RE_Internal_30 */
	128,						/* RE_Internal_40 */
	128,						/* RE_Internal_50 */
	128,						/* RE_Internal_60 */
	128,						/* RE_Internal_70 */
	128,						/* RE_Internal_80 */
	128,						/* RE_Internal_90 */
	128,						/* RE_Internal_100 */
	1024,						/* Gyro_1 */
	2048,						/* Gyro_2 */
	3072,						/* Gyro_3 */
	4096,						/* Gyro_4 */
	1024,						/* XL_Gyro_1 */
	2048,						/* XL_Gyro_2 */
	3072,						/* XL_Gyro_3 */
	4096,						/* XL_Gyro_4 */
	1024,						/* HD_Gyro_1 */
	2048,						/* HD_Gyro_2 */
	3072,						/* HD_Gyro_3 */
	4096,						/* HD_Gyro_4 */
	1024,						/* Compact_Gyro_1 */
	2048,						/* Compact_Gyro_2 */
	3072,						/* Compact_Gyro_3 */
	4096,						/* Compact_Gyro_4 */
	51,							/* XXL_Engine_20 */
	102,						/* XXL_Engine_40 */
	153,						/* XXL_Engine_60 */
	204,						/* XXL_Engine_80 */
	255,						/* XXL_Engine_100 */
	306,						/* XXL_Engine_120 */
	357,						/* XXL_Engine_140 */
	408,						/* XXL_Engine_160 */
	459,						/* XXL_Engine_180 */
	510,						/* XXL_Engine_200 */
	561,						/* XXL_Engine_220 */
	612,						/* XXL_Engine_240 */
	763,						/* XXL_Engine_260 */
	814,						/* XXL_Engine_280 */
	865,						/* XXL_Engine_300 */
	916,						/* XXL_Engine_320 */
	967,						/* XXL_Engine_340 */
	1018,						/* XXL_Engine_360 */
	1070,						/* XXL_Engine_380 */
	1020,						/* XXL_Engine_400 */
	51,							/* Compact_Engine_20 */
	102,						/* Compact_Engine_40 */
	153,						/* Compact_Engine_60 */
	204,						/* Compact_Engine_80 */
	255,						/* Compact_Engine_100 */
	306,						/* Compact_Engine_120 */
	357,						/* Compact_Engine_140 */
	408,						/* Compact_Engine_160 */
	459,						/* Compact_Engine_180 */
	510,						/* Compact_Engine_200 */
	561,						/* Compact_Engine_220 */
	612,						/* Compact_Engine_240 */
	763,						/* Compact_Engine_260 */
	814,						/* Compact_Engine_280 */
	865,						/* Compact_Engine_300 */
	916,						/* Compact_Engine_320 */
	967,						/* Compact_Engine_340 */
	1018,						/* Compact_Engine_360 */
	1070,						/* Compact_Engine_380 */
	1020,						/* Compact_Engine_400 */
#endif
};
#endif /* BT_PART_WEIGHTS */

#ifdef BT_ADVANCED_ECON
unsigned long long int specialcost[SPECIALCOST_SIZE] = { 0 };
unsigned long long int ammocost[AMMOCOST_SIZE] = { 0 };
unsigned long long int weapcost[WEAPCOST_SIZE] = { 0 };
unsigned long long int cargocost[CARGOCOST_SIZE] = { 0 };
unsigned long long int bombcost[BOMBCOST_SIZE] = { 0 };
#endif

int count_special_items()
{
	int i = 0;

	while (internals[i])
		i++;
	return i;
}

char *section_configs[] = {
	"Case", "Destroyed",
	NULL
};

char *move_types[] = {
	"Biped", "Track", "Wheel", "Hover", "VTOL", "Hull", "Foil", "Fly",
	"Quad",
	"Sub", "None",
	NULL
};

char *mech_types[] = {
	"Mech", "Vehicle", "VTOL", "Naval", "Spheroid_DropShip", "AeroFighter",
	"Mechwarrior", "Aerodyne_DropShip", "Battlesuit", NULL
};

char *crit_fire_modes[] = {
	"Destroyed",
	"Disabled",
	"Broken",
	"Damaged",
	"OnTC",
	"RearMount",
	"Hotload",
	"Halfton",
	"OneShot",
	"OneShot_Used",
	"UltraMode",
	"RapidFire",
	"Gattling",
	"Rotary_TwoShot",
	"Rotary_FourShot",
	"Rotary_SixShot",
	"Heat",
	"BackPack",
	"Jettisoned",
	"OmniBase",
	NULL
};

char *crit_ammo_modes[] = {
	"LBX/Cluster",
	"Artemis/Mine",
	"Narc/Smoke",
	"Cluster",
	"Mine",
	"Smoke",
	"Inferno",
	"Swarm",
	"Swarm1",
	"iNarc_Explosive",
	"iNarc_Haywire",
	"iNarc_ECM",
	"iNarc_Nemesis",
	"AP",
	"Flechette",
	"Incendiary",
	"Precision",
	"Stinger",
	NULL
};

/* 'specials' is *full* */
char *specials[] = {
	"TripleMyomerTech",
	"CL_AMS",
	"IS_AMS",
	"DoubleHS",
	"Masc",
	"Clan",
	"FlipArms",
	"C3MasterTech",
	"C3SlaveTech",
	"ArtemisIV",
	"ECM",
	"BeagleProbe",
	"SalvageTech",
	"CargoTech",
	"SearchLight",
	"LoaderTech",
	"AntiAircraft",
	"NoSensors",
	"SS_Ability",
	"FerroFibrous_Tech",
	"EndoSteel_Tech",
	"XLEngine_Tech",
	"ICEEngine_Tech",
	"LifterTech",
	"LightEngine_Tech",
	"XXL_Tech",
	"CompactEngine_Tech",
	"ReinforcedInternal_Tech",
	"CompositeInternal_Tech",
	"HardenedArmor_Tech",
	"CritProof_Tech",
	NULL
};

char *specialsabrev[] = {
	"TSM", "CLAMS", "ISAMS", "DHS", "MASC", "CLTECH", "FA", "C3M", "C3S",
	"AIV", "ECM", "BAP", "SAL", "CAR", "SL", "LOAD", "AA", "NOSEN", "SS",
	"FF", "ES", "XL", "ICE", "LIFT", "LENG", "XXL", "CENG", "RINT", "CINT",
	"HARM", "CP",
	NULL
};
/* 'specials' is *full* */

char *specials2[] = {
	"StealthArmor_Tech",
	"HvyFerroFibrous_Tech",
	"LaserRefArmor_Tech",
	"ReactiveArmor_Tech",
	"NullSigSys_Tech",
	"C3I_Tech",
	"SuperCharger_Tech",
	"ImprovedJJ_Tech",
	"MechanicalJJ_Tech",
	"CompactHS",
	"LaserHS_Tech",
	"BloodhoundProbe_Tech",
	"AngelECM_Tech",
	"WatchDog_Tech",
	"LtFerroFibrous_Tech",
	"TAG_Tech",
	"OmniMech_Tech",
	"ArtemisV_Tech",
	"Camo_Tech",
	"Carrier_Tech",
	"Waterproof_Tech",
	"XLGyro_Tech",
	"HDGyro_Tech",
	"CompactGyro_Tech",
	NULL
};

char *specialsabrev2[] = {
	"STHA", "HFF", "LRARM", "REACTARM", "NULL", "C3I", "SCHARGE",
	"IJJ", "MJJ", "CHS", "LHS", "BLP", "AECM", "WDOG", "LFF",
	"TAG", "OMNI", "AV",
	"CAMO",
	"CART",
	"WPRF",
	"XLGRYO", "HDGYRO", "CGYRO",
	NULL
};

char *infantry_specials[] = {
	"Swarm_Attack_Tech",
	"Mount_Friends_Tech",
	"AntiLeg_Attack_Tech",
	"CS_Purifier_Stealth_Tech",
	"DC_Kage_Stealth_Tech",
	"FWL_Achileus_Stealth_Tech",
	"FC_Infiltrator_Stealth_Tech",
	"FC_InfiltratorII_Stealth_Tech",
	"Must_Jettison_Pack_Tech",
	"Can_Jettison_Pack_Tech",
	NULL
};

int compare_array(char *list[], char *command)
{
	int x;

	if(!list)
		return -1;
	for(x = 0; list[x]; x++)
		if(!strcasecmp(list[x], command))
			return x;

	return -1;
}

char *one_arg(char *argument, char *first_arg)
{
	if(isspace(*argument))
		for(argument++; isspace(*argument); argument++);

	while (*argument && !isspace(*argument))
		*(first_arg++) = *(argument++);
	*first_arg = '\0';
	return argument;
}

char *one_arg_delim(char *argument, char *first_arg)
{
	if(isspace(*argument) || (*argument == '|'))
		for(argument++; (isspace(*argument) || (*argument == '|'));
			argument++);

	while (*argument && (!(*argument == '|')))
		*(first_arg++) = *(argument++);

	*first_arg = '\0';
	return argument;
}

char *BuildBitString(char *bitdescs[], int data)
{
	static char crit[MAX_STRING_LENGTH];
	int bv;
	int x;

	crit[0] = 0;
	for(x = 0; bitdescs[x]; x++) {
		bv = 1U << x;
		if(data & bv) {
			strcat(crit, bitdescs[x]);
			strcat(crit, " ");
		}
	}
	if((x = strlen(crit)) > 0 && crit[x - 1] == ' ')
		crit[x - 1] = '\0';
	return crit;
}

char *BuildBitString2(char *bitdescs[], char *bitdescs2[], int data,
					  int data2)
{
	static char crit[MAX_STRING_LENGTH];
	int bv;
	int x;

	crit[0] = 0;

	for(x = 0; bitdescs[x]; x++) {
		bv = 1U << x;
		if(data & bv) {
			strcat(crit, bitdescs[x]);
			strcat(crit, " ");
		}
	}

	for(x = 0; bitdescs2[x]; x++) {
		bv = 1U << x;
		if(data2 & bv) {
			strcat(crit, bitdescs2[x]);
			strcat(crit, " ");
		}
	}

	if((x = strlen(crit)) > 0 && crit[x - 1] == ' ')
		crit[x - 1] = '\0';

	return crit;
}

char *BuildBitString3(char *bitdescs[], char *bitdescs2[],
					  char *bitdescs3[], int data, int data2, int data3)
{
	static char crit[MAX_STRING_LENGTH];
	int bv;
	int x;

	crit[0] = 0;

	for(x = 0; bitdescs[x]; x++) {
		bv = 1U << x;
		if(data & bv) {
			strcat(crit, bitdescs[x]);
			strcat(crit, " ");
		}
	}

	for(x = 0; bitdescs2[x]; x++) {
		bv = 1U << x;
		if(data2 & bv) {
			strcat(crit, bitdescs2[x]);
			strcat(crit, " ");
		}
	}

	for(x = 0; bitdescs3[x]; x++) {
		bv = 1U << x;
		if(data3 & bv) {
			strcat(crit, bitdescs3[x]);
			strcat(crit, " ");
		}
	}

	if((x = strlen(crit)) > 0 && crit[x - 1] == ' ')
		crit[x - 1] = '\0';

	return crit;
}

#define QDM(a) case I2Special(a): return 1
static int InvalidVehicleItem(MECH * mech, int x, int y)
{
	int t;

	t = GetPartType(mech, x, y);
	switch (t) {
		QDM(SHOULDER_OR_HIP);
		QDM(UPPER_ACTUATOR);
		QDM(LOWER_ACTUATOR);
		QDM(HAND_OR_FOOT_ACTUATOR);
		QDM(ENGINE);
		QDM(GYRO);
		QDM(JUMP_JET);
	}
	return 0;
}

extern int num_def_weapons;
int internal_count = sizeof(internals) / sizeof(char *) - 1;
int cargo_count = sizeof(cargo) / sizeof(char *) - 1;

#ifndef CLAN_SUPPORT
#define CLCH(a) do { if (MechWeapons[a].special & (CLAT)) return NULL; } while (0)
#else
#define CLCH(a) \
do { if (temp_brand_flag) { if ((!mudconf.btech_parts) || (MechWeapons[a].special & CLAT)) return NULL; else if (MechWeapons[a].special & CLAT) isclan=1; }} while (0)
#endif

extern int temp_brand_flag;

static char *part_figure_out_name_sub(int i, int j)
{
	static char buf[MBUF_SIZE];
	int isclan = 0;

	if(!i)
		return NULL;
	if(IsWeapon(i) && i < I2Weapon(num_def_weapons)) {
		CLCH(Weapon2I(i));
		return &MechWeapons[Weapon2I(i)].name[(j && !isclan) ? 3 : 0];
	} else if(IsAmmo(i) && i < I2Ammo(num_def_weapons)) {
		CLCH(Ammo2WeaponI(i));
		if(MechWeapons[Ammo2WeaponI(i)].type != TBEAM &&
		   MechWeapons[Ammo2WeaponI(i)].type != THAND &&
		   !(MechWeapons[Ammo2WeaponI(i)].special & PCOMBAT)) {
			sprintf(buf, "Ammo_%s", &MechWeapons[Ammo2WeaponI(i)].name[(j
																		&&
																		!isclan)
																	   ? 3 :
																	   0]);
			return buf;
		}
	} else if(!temp_brand_flag) {
		if(IsBomb(i)) {
			sprintf(buf, "Bomb_%s", bomb_name(Bomb2I(i)));
			return buf;
		} else if(IsSpecial(i) && i < I2Special(internal_count))
			return internals[Special2I(i)];
		else if(IsCargo(i) && i < I2Cargo(cargo_count))
			return cargo[Cargo2I(i)];
	}
	return NULL;
}

char *my_shortform(char *buf)
{
	static char buf2[MBUF_SIZE];
	char *c, *d;

	if(!buf)
		return NULL;
	if(strlen(buf) <= 4 && !strchr(buf, '/'))
		strcpy(buf2, buf);
	else {
		for(c = buf, d = buf2; *c; c++)
			if(isdigit(*c) || isupper(*c) || *c == '_')
				*d++ = *c;
		*d = 0;
		if(strlen(buf2) == 1)
			strcat(d, tprintf("%c", buf[1]));
	}
	return buf2;
}

#undef CLCH
#ifdef CLAN_SUPPORT
#define CLCH(a) ((!strncasecmp(MechWeapons[a].name, "CL.", 2)) ? 0 : 3)
#else
#define CLCH(a) 3
#endif

char *part_figure_out_shname(int i)
{
	char buf[MBUF_SIZE];

	if(!i)
		return NULL;
	buf[0] = 0;
	if(IsWeapon(i) && i < I2Weapon(num_def_weapons)) {
		strcpy(buf, &MechWeapons[Weapon2I(i)].name[CLCH(Weapon2I(i))]);
	} else if(IsAmmo(i) && i < I2Ammo(num_def_weapons)) {
		sprintf(buf, "Ammo_%s",
				&MechWeapons[Ammo2WeaponI(i)].name[CLCH(Ammo2WeaponI(i))]);
	} else if(IsBomb(i))
		sprintf(buf, "Bomb_%s", bomb_name(Bomb2I(i)));
	else if(IsSpecial(i) && i < I2Special(internal_count))
		strcpy(buf, internals[Special2I(i)]);
	if(IsCargo(i) && i < I2Cargo(cargo_count))
		strcpy(buf, cargo[Cargo2I(i)]);
	if(!buf[0])
		return NULL;
	return my_shortform(buf);
}

char *part_figure_out_name(int i)
{
	return part_figure_out_name_sub(i, 0);
}

char *part_figure_out_sname(int i)
{
	return part_figure_out_name_sub(i, 1);
}

#define TCAble(t) \
	((MechWeapons[Weapon2I(t)].type == TBEAM || MechWeapons[Weapon2I(t)].type == TAMMO) && \
	strcmp(&MechWeapons[Weapon2I(t)].name[3], "Flamer") && \
	strcmp(&MechWeapons[Weapon2I(t)].name[3], "MachineGun") && \
	strcmp(&MechWeapons[Weapon2I(t)].name[3], "HeavyMachineGun") && \
	!(MechWeapons[Weapon2I(t)].special & PCOMBAT))

static int dump_item(FILE * fp, MECH * mech, int x, int y)
{
	char crit[32];
	int y1;
	int flaggo = 0;
	int z;
	int wFireModes, wAmmoModes;

	if(!GetPartType(mech, x, y))
		return 1;
	if(MechType(mech) != CLASS_MECH && InvalidVehicleItem(mech, x, y))
		return 1;
	for(y1 = y + 1; y1 < 12; y1++) {
		if(GetPartType(mech, x, y1) != GetPartType(mech, x, y))
			break;
		if(GetPartData(mech, x, y1) != GetPartData(mech, x, y))
			break;
		if(GetPartFireMode(mech, x, y1) != GetPartFireMode(mech, x, y))
			break;
		if(GetPartAmmoMode(mech, x, y1) != GetPartAmmoMode(mech, x, y))
			break;
		if(mudconf.btech_parts)
			if(GetPartBrand(mech, x, y1) != GetPartBrand(mech, x, y))
				break;
	}
	y1--;
	if(IsWeapon(GetPartType(mech, x, y))) {
		/* Nonbeams, or flamers don't have TC */
		if(!TCAble(GetPartType(mech, x, y)))
			flaggo = ON_TC;
		if(((y1 - y) + 1) > (z =
							 GetWeaponCrits(mech,
											Weapon2I(GetPartType
													 (mech, x, y)))))
			y1 = y + z - 1;
	}
	if(y != y1)
		snprintf(crit, 32, "CRIT_%d-%d", y + 1, y1 + 1);
	else
		snprintf(crit, 32, "CRIT_%d", y + 1);

	wFireModes = GetPartFireMode(mech, x, y);
	wFireModes &= ~flaggo;
	wAmmoModes = GetPartAmmoMode(mech, x, y);

	if(IsWeapon(GetPartType(mech, x, y)))
		fprintf(fp, "    %s		  { %s - %s %s}\n", crit,
				get_parts_vlong_name(GetPartType(mech, x, y), 0),
				(wFireModes ||
				 wAmmoModes) ? BuildBitString2(crit_fire_modes,
											   crit_ammo_modes, wFireModes,
											   wAmmoModes) : "-",
				!mudconf.btech_parts ? "" : tprintf("%d ",
													GetPartBrand(mech, x,
																 y)));
	else if(IsAmmo(MechSections(mech)[x].criticals[y].type))
		fprintf(fp, "    %s		  { %s %d %s - }\n", crit,
				get_parts_vlong_name(GetPartType(mech, x, y), 0),
				FullAmmo(mech, x, y),
				(MechSections(mech)[x].criticals[y].firemode ||
				 MechSections(mech)[x].criticals[y].
				 ammomode) ? BuildBitString2(crit_fire_modes,
											 crit_ammo_modes,
											 MechSections(mech)[x].
											 criticals[y].firemode,
											 MechSections(mech)[x].
											 criticals[y].ammomode) : "-");
	else if(IsBomb(MechSections(mech)[x].criticals[y].type))
		fprintf(fp, "    %s		  { %s - - - }\n", crit,
				get_parts_vlong_name(GetPartType(mech, x, y), 0));
	else {
		fprintf(fp, "    %s		  { %s - - %s}\n", crit,
				get_parts_vlong_name(GetPartType(mech, x, y), 0),
				!mudconf.btech_parts ? "" : tprintf("%d ",
													GetPartBrand(mech, x,
																 y)));
	}
	return (y1 - y + 1);
}

void dump_locations(FILE * fp, MECH * mech, const char *locdesc[])
{
	int x, y, l;
	char buf[512];
	char *ch;

	for(x = 0; locdesc[x]; x++) {
		if(!GetSectOInt(mech, x))
			continue;
		strcpy(buf, locdesc[x]);
		for(ch = buf; *ch; ch++)
			if(*ch == ' ')
				*ch = '_';
		fprintf(fp, "%s\n", buf);
		if(GetSectOArmor(mech, x))
			fprintf(fp, "  Armor            { %d }\n", GetSectOArmor(mech,
																	 x));
		if(GetSectOInt(mech, x))
			fprintf(fp, "  Internals        { %d }\n", GetSectOInt(mech, x));
		if(GetSectORArmor(mech, x))
			fprintf(fp, "  Rear             { %d }\n", GetSectORArmor(mech,
																	  x));
#if 0							/* Shouldn't be neccessary to save at all */
		fprintf(fp, "  Recycle          { %d }\n",
				MechSections(mech)[x].recycle);
#endif
		y = MechSections(mech)[x].config;
		y &= ~CASE_TECH;
		if(y)
			fprintf(fp, "  Config           { %s }\n",
					BuildBitString(section_configs, y));
		l = CritsInLoc(mech, x);
		for(y = 0; y < l;)
			y += dump_item(fp, mech, x, y);
	}
}

float generic_computer_multiplier(MECH * mech)
{
	switch (MechComputer(mech)) {
	case 1:
		return 0.8;
	case 2:
		return 1;
	case 3:
		return 1.25;
	case 4:
		return 1.5;
	case 5:
		return 1.75;
	}
	return 0;
}

int generic_radio_type(int i, int isClan)
{
	int f = DEFAULT_FREQS;

	if(isClan || i >= 4)
		f += FREQS * RADIO_RELAY;
	if(i < 3)
		f -= (3 - i) * 2 - 1;	/* 2 or 4 */
	else
		f += (i - 3) * 3;		/* 5 / 8 / 11 */
	return f;
}

float generic_radio_multiplier(MECH * mech)
{
	switch (MechRadio(mech)) {
	case 1:
		return 0.8;
	case 2:
		return 1;
	case 3:
		return 1.25;
	case 4:
		return 1.5;
	case 5:
		return 1.75;
	}
	return 0.0;
}

#define MechComputersScanRange(mech) \
(generic_computer_multiplier(mech) * DEFAULT_SCANRANGE)

#define MechComputersLRSRange(mech) \
(generic_computer_multiplier(mech) * DEFAULT_LRSRANGE)

#define MechComputersTacRange(mech) \
(generic_computer_multiplier(mech) * DEFAULT_TACRANGE)

#define MechComputersRadioRange(mech) \
(DEFAULT_RADIORANGE * generic_radio_multiplier(mech))

void computer_conversion(MECH * mech)
{
	int l = 0;

	switch (MechScanRange(mech)) {
	case 20:
		l = 2;
		break;
	case 25:
		l = 3;
		break;
	case 30:
		l = 4;
		break;
	}
	if(l) {
		MechComputer(mech) = l;
		MechScanRange(mech) = MechComputersScanRange(mech);
		MechTacRange(mech) = MechComputersTacRange(mech);
		MechLRSRange(mech) = MechComputersLRSRange(mech);
		MechRadioRange(mech) = MechComputersRadioRange(mech);
	}
}

void try_to_find_name(char *mechref, MECH * mech)
{
	const char *c;

	if((c = find_mechname_by_mechref(mechref)))
		strcpy(MechType_Name(mech), c);
}

int DefaultFuelByType(MECH * mech)
{
	int mod = 2;

	switch (MechType(mech)) {
	case CLASS_VTOL:
		return 2000 * mod;
	case CLASS_AERO:
		return 1200 * mod;
	case CLASS_DS:
	case CLASS_SPHEROID_DS:
		return 3600 * mod;
	}
	return 0;
}

int save_template(dbref player, MECH * mech, char *reference, char *filename)
{
	FILE *fp;
	int x, x2, inf_x;
	char **locs;
	char *d, *c = ctime(&mudstate.now);

	if(!MechComputer(mech))
		computer_conversion(mech);
	if(!MechType_Name(mech)[0])
		try_to_find_name(reference, mech);
	if(!(fp = fopen(filename, "w")))
		return -1;
	if(MechType_Name(mech)[0])
		fprintf(fp, "Name             { %s }\n", MechType_Name(mech));
	fprintf(fp, "Reference        { %s }\n", reference);
	fprintf(fp, "Type             { %s }\n",
			mech_types[(short) MechType(mech)]);
	fprintf(fp, "Move_Type        { %s }\n",
			move_types[(short) MechMove(mech)]);
	fprintf(fp, "Tons             { %d }\n", MechTons(mech));
	if((d = strrchr(c, '\n')))
		*d = 0;
	fprintf(fp, "Comment          { Saved by: %s(#%d) at %s }\n",
			Name(player), player, c);
#define SILLY_UTTERANCE(ran,cran,dran,name) \
  if ((!MechComputer(mech) && ran != dran) || \
      (MechComputer(mech) && ran != cran)) \
      fprintf(fp, "%-16s { %d }\n", name, ran)

	SILLY_UTTERANCE(MechTacRange(mech), MechComputersTacRange(mech),
					DEFAULT_TACRANGE, "Tac_Range");
	SILLY_UTTERANCE(MechLRSRange(mech), MechComputersLRSRange(mech),
					DEFAULT_LRSRANGE, "LRS_Range");
	SILLY_UTTERANCE(MechScanRange(mech), MechComputersScanRange(mech),
					DEFAULT_SCANRANGE, "Scan_Range");
	SILLY_UTTERANCE(MechRadioRange(mech), MechComputersRadioRange(mech),
					DEFAULT_RADIORANGE, "Radio_Range");

#define SILLY_OUTPUT(def,now,name) \
  if ((def) != (now)) \
     fprintf(fp, "%-16s { %d }\n", name, now)

	SILLY_OUTPUT(DEFAULT_COMPUTER, MechComputer(mech), "Computer");
	SILLY_OUTPUT(DEFAULT_RADIO, MechRadio(mech), "Radio");
	SILLY_OUTPUT((MechSpecials(mech) & ICE_TECH) ? 0 : DEFAULT_HEATSINKS,
				 MechRealNumsinks(mech), "Heat_Sinks");
	SILLY_OUTPUT(generic_radio_type(MechRadio(mech),
									MechSpecials(mech) & CLAN_TECH),
				 MechRadioType(mech), "RadioType");
	SILLY_OUTPUT(2000, MechBV(mech), "Mech_BV");
	SILLY_OUTPUT(2000, CargoSpace(mech), "Cargo_Space");
	SILLY_OUTPUT(0, CarMaxTon(mech), "Max_Ton");
	SILLY_OUTPUT(2000, MechMaxSuits(mech), "Max_Suits");
	SILLY_OUTPUT(0, AeroSIOrig(mech), "SI");

	SILLY_OUTPUT(DefaultFuelByType(mech), AeroFuelOrig(mech), "Fuel");

	fprintf(fp, "Max_Speed        { %.2f }\n", MechMaxSpeed(mech));
	if(MechJumpSpeed(mech) > 0.0)
		fprintf(fp, "Jump_Speed       { %.2f }\n", MechJumpSpeed(mech));
	x = MechSpecials(mech);
	x2 = MechSpecials2(mech);
	/* Remove AMS'es, they're re-generated back on loadtime */
	x &= ~(CL_ANTI_MISSILE_TECH | IS_ANTI_MISSILE_TECH | SS_ABILITY);
	x &=						/* Calculated at load-time */
		~(BEAGLE_PROBE_TECH | TRIPLE_MYOMER_TECH | MASC_TECH | ECM_TECH |
		  C3_SLAVE_TECH | C3_MASTER_TECH | ARTEMIS_IV_TECH | ES_TECH |
		  FF_TECH);

	if(MechType(mech) == CLASS_MECH)
		x &= ~(XL_TECH | XXL_TECH | CE_TECH | LE_TECH);

	/* Get rid of our specials2 */
	x2 &=
		~(STEALTH_ARMOR_TECH | NULLSIGSYS_TECH | ANGEL_ECM_TECH |
		  HVY_FF_ARMOR_TECH | LT_FF_ARMOR_TECH | TAG_TECH | C3I_TECH |
		  BLOODHOUND_PROBE_TECH);

	if(x || x2)
		fprintf(fp, "Specials         { %s }\n", BuildBitString2(specials,
																 specials2, x,
																 x2));

	inf_x = MechInfantrySpecials(mech);

	if(inf_x)
		fprintf(fp, "InfantrySpecials { %s }\n",
				BuildBitString(infantry_specials, inf_x));

	if((locs = (char **)ProperSectionStringFromType(MechType(mech), MechMove(mech)))) {
		dump_locations(fp, mech, (const char **)locs);
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return -1;
}

char *read_desc(FILE * fp, char *data)
{
	char keep[MAX_STRING_LENGTH + 500];
	char *t, *tmp;
	char *point;
	int length = 0;
	static char buf[MAX_STRING_LENGTH];

	keep[0] = '\0';
	if((tmp = strchr(data, '{'))) {
		fscanf(fp, "\n");
		while (isspace(*(++tmp)));
		if((t = strchr(tmp, '}'))) {
			while (isspace(*(t--)));
			*(t++) = '\0';
			length = strlen(tmp);
			strcpy(buf, tmp);
			return buf;
		} else {
			strcpy(keep, tmp);
			strcat(keep, "\r\n");
			t = tmp + strlen(tmp) - 1;
			length = strlen(t);
			while (fgets(data, 512, fp)) {
				fscanf(fp, "\n");
				if((tmp = strchr(data, '}')) != NULL) {
					*tmp = 0;
					length += strlen(data);
					strcat(keep, data);
					break;
				} else {
					point = data + strlen(data) - 1;
					*(point++) = '\r';
					*(point++) = '\n';
					*point = '\0';
					strcat(keep, data);
					length += strlen(data);
				}
			}
		}
	}
	strcpy(buf, keep);
	return buf;
}

int find_section(char *cmd, int type, int mtype)
{
    char section[20];
    char *ch;
    char **locs;

    strcpy(section, cmd);
    for(ch = section; *ch; ch++)
        if(*ch == '_')
            *ch = ' ';
    locs = (char **)ProperSectionStringFromType(type, mtype);
    return compare_array((char **) locs, section);
    return -1; // Who's retarded? not me!
}

long BuildBitVector(char **list, char *line)
{
	long bv = 0;
	int temp;
	char buf[30];

	if(!strcasecmp(line, "-"))
		return 0;

	while (*line) {
		line = one_arg(line, buf);
		if((temp = compare_array(list, buf)) == -1)
			return -1;
		bv |= 1U << temp;
	}
	return bv;
}

long BuildBitVectorWithDelim(char **list, char *line)
{
	long bv = 0;
	int temp;
	char buf[30];

	if(!strcasecmp(line, "-"))
		return 0;

	while (*line) {
		line = one_arg_delim(line, buf);

		if((temp = compare_array(list, buf)) == -1)
			return -1;

		bv |= 1U << temp;
	}

	return bv;
}

long BuildBitVectorNoErr(char **list, char *line)
{
	long bv = 0;
	int temp;
	char buf[30];

	if(!strcasecmp(line, "-"))
		return 0;

	while (*line) {
		line = one_arg(line, buf);

		if((temp = compare_array(list, buf)) != -1)
			bv |= 1U << temp;
	}

	return bv;
}

int CheckSpecialsList(char **specials, char **specials2, char *line)
{
	int wSpecCheck = -1, wSpec2Check = -1;
	char buf[30];

	if(!strcasecmp(line, "-"))
		return 0;

	while (*line) {
		line = one_arg(line, buf);

		if(specials)
			wSpecCheck = compare_array(specials, buf);

		if(specials2)
			wSpec2Check = compare_array(specials2, buf);

		if((wSpecCheck == -1) && (wSpec2Check == -1))
			return 0;
	}

	return 1;
}

int WeaponIFromString(char *data)
{
	int x = 0;;

	while (MechWeapons[x].name) {
		if(!strcasecmp(MechWeapons[x].name, data))
			return x + 1;		/* weapons start at 1 not 0 */
		x++;
	}
	return -1;
}

int AmmoIFromString(char *data)
{
	int x = 0;
	char *ptr;

	ptr = data;
	while (*ptr != '_')
		ptr++;
	ptr++;
	while (MechWeapons[x].name) {
		if(!strcasecmp(MechWeapons[x].name, ptr))
			return x + 101;
		x++;
	}
	return -1;
}

void update_specials(MECH * mech)
{
	int x, y, t;
	int masc_count = 0;
	int c3_master_count = 0;
	int tsm_count = 0;
	int ff_count = 0;
	int es_count = 0;
	int tc_count = 0;
	int awcSthArmor[NUM_SECTIONS];
	int awcNSS[NUM_SECTIONS];
	int wcSthArmor = 0;
	int wcNSS = 0;
	int wcAngel = 0;
	int cl = MechSpecials(mech) & CLAN_TECH;
	int e_count = 0;
	int tTechOK = 1;
	int wcHvyFF = 0;
	int wcLtFF = 0;
	int wcC3i = 0;
	int wcBloodhound = 0;
	int awInfSpec[5];
	int wcSuits = 0;

	MechSpecials(mech) &=
		~(BEAGLE_PROBE_TECH | TRIPLE_MYOMER_TECH | MASC_TECH | ECM_TECH |
		  C3_SLAVE_TECH | C3_MASTER_TECH | ARTEMIS_IV_TECH | ES_TECH |
		  FF_TECH | IS_ANTI_MISSILE_TECH | CL_ANTI_MISSILE_TECH);
	if(MechType(mech) == CLASS_MECH)
		MechSpecials(mech) &= ~(XL_TECH | XXL_TECH | CE_TECH | LE_TECH);

	MechSpecials2(mech) &=
		~(STEALTH_ARMOR_TECH | NULLSIGSYS_TECH | ANGEL_ECM_TECH |
		  HVY_FF_ARMOR_TECH | LT_FF_ARMOR_TECH | TAG_TECH | C3I_TECH |
		  BLOODHOUND_PROBE_TECH);

	MechInfantrySpecials(mech) &=
		~(CS_PURIFIER_STEALTH_TECH | DC_KAGE_STEALTH_TECH |
		  FWL_ACHILEUS_STEALTH_TECH | FC_INFILTRATOR_STEALTH_TECH |
		  FC_INFILTRATORII_STEALTH_TECH);

	for(x = 0; x < 5; x++)
		awInfSpec[x] = 0;

	for(x = 0; x < NUM_SECTIONS; x++) {
		e_count = 0;
		MechSections(mech)[x].config &= ~CASE_TECH;
		awcSthArmor[x] = 0;
		awcNSS[x] = 0;

		for(y = 0; y < CritsInLoc(mech, x); y++)
			if((t = GetPartType(mech, x, y))) {
				switch (Special2I(t)) {
#define TECHC(item,name) case item: name++; break;
#define TECHCU(item,name) \
				case item: if (!PartIsNonfunctional(mech, x, y)) name++; break;
#define TECH(item,name) case item: MechSpecials(mech) |= name; break
#define TECH2(item,name) case item: MechSpecials2(mech) |= name; break
#define TECHU(item, name) \
				case item: if (!PartIsNonfunctional(mech, x, y)) MechSpecials(mech) |= name ; break
					TECH(ARTEMIS_IV, ARTEMIS_IV_TECH);
					TECHU(BEAGLE_PROBE, BEAGLE_PROBE_TECH);
					TECH(ECM, ECM_TECH);
					TECH2(TAG, TAG_TECH);
					TECHU(C3_SLAVE, C3_SLAVE_TECH);
					TECHCU(MASC, masc_count);
					TECHC(C3_MASTER, c3_master_count);
					TECHCU(C3I, wcC3i);
					TECHCU(ANGELECM, wcAngel);
					TECHC(TRIPLE_STRENGTH_MYOMER, tsm_count);
					TECHC(FERRO_FIBROUS, ff_count);
					TECHC(HVY_FERRO_FIBROUS, wcHvyFF);
					TECHC(LT_FERRO_FIBROUS, wcLtFF);
					TECHC(BLOODHOUND_PROBE, wcBloodhound);
					TECHCU(TARGETING_COMPUTER, tc_count);
					TECHC(ENDO_STEEL, es_count);
					TECHC(PURIFIER_ARMOR, awInfSpec[0]);
					TECHCU(KAGE_STEALTH_UNIT, awInfSpec[1]);
					TECHCU(ACHILEUS_STEALTH_UNIT, awInfSpec[2]);
					TECHCU(INFILTRATOR_STEALTH_UNIT, awInfSpec[3]);
					TECHCU(INFILTRATORII_STEALTH_UNIT, awInfSpec[4]);
				case ENGINE:
					e_count++;
					break;
				case CASE:
					MechSections(mech)[(MechType(mech) == CLASS_VEH_GROUND)
									   ? BSIDE : x].config |= CASE_TECH;
					break;
				case STEALTH_ARMOR:
					awcSthArmor[x]++;
					wcSthArmor++;
					break;
				case NULL_SIGNATURE_SYSTEM:
					awcNSS[x]++;
					wcNSS++;
					break;
				}
				if(IsWeapon(t) && IsAMS(Weapon2I(t))) {
					if(MechWeapons[Weapon2I(t)].special & CLAT)
						MechSpecials(mech) |= CL_ANTI_MISSILE_TECH;
					else
						MechSpecials(mech) |= IS_ANTI_MISSILE_TECH;
				}
			}
		if(x != CTORSO && e_count) {
			if(e_count > 3)
				MechSpecials(mech) |= XXL_TECH;

			else if(e_count == 2)
				if(cl)
					MechSpecials(mech) |= XL_TECH;

				else
					MechSpecials(mech) |= LE_TECH;

			else
				MechSpecials(mech) |= XL_TECH;
		} else {
			if(x == CTORSO && e_count < 4 && MechType(mech) == CLASS_MECH)
				MechSpecials(mech) |= CE_TECH;
		}

	}
	if((MechSpecials(mech) & (XXL_TECH | XL_TECH | LE_TECH)) &&
	   (MechSpecials(mech) & CE_TECH))
		SendError(tprintf
				  ("#%d apparently is very weird: Compact engine AND XL/XXL?",
				   mech->mynum));
	if(tc_count)
		for(x = 0; x < NUM_SECTIONS; x++)
			for(y = 0; y < CritsInLoc(mech, x); y++)
				if(IsWeapon((t = GetPartType(mech, x, y))))
					if(TCAble(t))
						GetPartFireMode(mech, x, y) |= ON_TC;
	if(masc_count >= MAX(1, (MechTons(mech) / (cl ? 25 : 20))))
		MechSpecials(mech) |= MASC_TECH;
#define ITech(var,cnt,spec) \
  if (((var)) >= ((cnt)) || (MechType(mech) != CLASS_MECH && ((var)>0))) \
					MechSpecials(mech) |= spec

#define ITech2(var,cnt,spec) \
  if (((var)) >= ((cnt)) || (MechType(mech) != CLASS_MECH && ((var)>0))) \
					MechSpecials2(mech) |= spec

	ITech(ff_count, (cl ? 7 : 14), FF_TECH);
	ITech(es_count, (cl ? 7 : 14), ES_TECH);
	ITech(tsm_count, 6, TRIPLE_MYOMER_TECH);
	ITech2(wcAngel, 2, ANGEL_ECM_TECH);
	ITech2(wcHvyFF, 21, HVY_FF_ARMOR_TECH);
	ITech2(wcLtFF, 7, LT_FF_ARMOR_TECH);
	ITech2(wcC3i, 2, C3I_TECH);
	ITech2(wcBloodhound, 2, BLOODHOUND_PROBE_TECH);

	/*
	 * Check our NSS. Need 1 crit in each loc except H
	 */
	tTechOK = 0;

	if(wcNSS > 0) {
		tTechOK = 1;

		if(MechType(mech) != CLASS_MECH) {
			if(wcNSS < 1)
				tTechOK = 0;
		} else {
			for(x = 0; x < NUM_SECTIONS; x++) {
				if(x != HEAD) {
					if(awcNSS[x] < 1) {
						tTechOK = 0;
						break;
					}
				}
			}
		}

		if(tTechOK)
			MechSpecials2(mech) |= NULLSIGSYS_TECH;
	}

	/*
	 * Check our Stealth armor. Need 2 crits in each loc except H and CT
	 */
	tTechOK = 0;

	if(wcSthArmor > 0) {
		tTechOK = 1;

		if(!(MechSpecials(mech) & ECM_TECH)) {
			tTechOK = 0;
		} else {
			if(MechType(mech) != CLASS_MECH) {
				if(wcSthArmor < 1)
					tTechOK = 0;
			} else {
				for(x = 0; x < NUM_SECTIONS; x++) {
					if((x != HEAD) && (x != CTORSO)) {
						if(awcSthArmor[x] < 2) {
							tTechOK = 0;
							break;
						}
					}
				}
			}
		}

		if(tTechOK)
			MechSpecials2(mech) |= STEALTH_ARMOR_TECH;
	}

	/* Let's do our suit checks */
	if(MechType(mech) == CLASS_BSUIT) {
		wcSuits = CountBSuitMembers(mech);

		if(awInfSpec[0] >= wcSuits)
			MechInfantrySpecials(mech) |= CS_PURIFIER_STEALTH_TECH;

		if(awInfSpec[1] >= wcSuits)
			MechInfantrySpecials(mech) |= DC_KAGE_STEALTH_TECH;

		if(awInfSpec[2] >= wcSuits)
			MechInfantrySpecials(mech) |= FWL_ACHILEUS_STEALTH_TECH;

		if(awInfSpec[3] >= wcSuits)
			MechInfantrySpecials(mech) |= FC_INFILTRATOR_STEALTH_TECH;

		if(awInfSpec[4] >= wcSuits)
			MechInfantrySpecials(mech) |= FC_INFILTRATORII_STEALTH_TECH;
	}

	/* New C3 Master code */
	if(c3_master_count > 0) {
		MechTotalC3Masters(mech) = countTotalC3MastersOnMech(mech);
		MechWorkingC3Masters(mech) = countWorkingC3MastersOnMech(mech);

		if(MechTotalC3Masters(mech) > 0)
			MechSpecials(mech) |= C3_MASTER_TECH;

		if(MechWorkingC3Masters(mech) == 0)
			MechCritStatus(mech) |= C3_DESTROYED;
		else
			MechCritStatus(mech) &= ~C3_DESTROYED;
	}
}

int update_oweight(MECH * mech, int value)
{
	MechCritStatus(mech) |= OWEIGHT_OK;

	/* Check to prevent silliness */
	if(!mudconf.btech_dynspeed || (value == 1 && !Destroyed(mech)))
		value = MechTons(mech) * 1024;
	MechRTonsV(mech) = value;
	return value;
}

int get_weight(MECH * mech)
{
	if(MechCritStatus(mech) & OWEIGHT_OK)
		return MechRTonsV(mech);
	return update_oweight(mech, mech_weight_sub(GOD, mech, -1));
}

int load_template(dbref player, MECH * mech, char *filename)
{
	char line[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
	int x, y, value, i;
	char cmd[MAX_STRING_LENGTH];
	char *ptr, *j, *k, *line2;
	int section = 0, critical, selection, type, brand;
	FILE *fp = fopen(filename, "r");
	char *tmpc;
	int lpos, hpos;
	int ok_count = 0;
	int isClan = 0;
	int t;
	int wFireModes, wAmmoModes;
	MAP *map;

	if(!fp)
		return -1;

	ptr = strrchr(filename, '/');
	if(ptr == NULL) {
		ptr = filename;
	} else {
		ptr++;
	}
	strncpy(MechType_Ref(mech), ptr, 15);
	MechType_Ref(mech)[14] = '\0';

	silly_atr_set(mech->mynum, A_MECHREF, MechType_Ref(mech));
	MechRadioType(mech) = 0;
	while (fgets(line, 512, fp)) {
		line[strlen(line) - 1] = '\0';
		j = line;
		while (isspace(*j))
			j++;
		if(j != line)
			memmove(line, j, strlen(j) + 1);
		if((ptr = strchr(line, ' '))) {
			if((tmpc = strchr(line, '\t')) < ptr)
				if(tmpc)
					ptr = tmpc;
			j = line;
			k = cmd;
			while (j != ptr)
				*(k++) = *(j++);
			*k = 0;
			for(ptr++; isspace(*ptr); ptr++);
		} else {
			strcpy(cmd, line);
			strcpy(line, "");
			ptr = NULL;
		}
		if(!strncasecmp(cmd, "CRIT_", 5))
			selection = 9999;
		else if((selection = compare_array(load_cmds, cmd)) == -1) {
			/* Initial premise: we will have a mech type before we get to this */
			section = find_section(cmd, MechType(mech), MechMove(mech));
			TEMPLATE_ERR(section == -1 &&
						 !ok_count,
						 "New template loading system: %s is invalid template file.",
						 filename);
			TEMPLATE_GERR(section == -1,
						  "Error while loading: Section %s not found.", cmd);
			MechSections(mech)[section].recycle = 0;
			ok_count++;
			continue;
		}
		ok_count++;
		switch (selection) {
		case 0:				/* Reference */
			tmpc = read_desc(fp, ptr);
			if(strcmp(tmpc, MechType_Ref(mech))) {
				SendError(tprintf
						  ("Template %s has Reference <-> Filename mismatch : %s <-> %s - It is automatically fixed by saving again.",
						   filename, tmpc, MechType_Ref(mech)));
				tmpc = MechType_Ref(mech);
			}
			silly_atr_set(mech->mynum, A_MECHREF, tmpc);
			strcpy(MechType_Ref(mech), tmpc);
			break;
		case 1:				/* Type */
			tmpc = read_desc(fp, ptr);
			MechType(mech) = compare_array(mech_types, tmpc);
			TEMPLATE_GERR(MechType(mech) == -1,
						  "Error while loading: Type %s not found.", tmpc);
			AeroFuel(mech) = AeroFuelOrig(mech) = DefaultFuelByType(mech);
			break;
		case 2:				/* Movement Type */
			tmpc = read_desc(fp, ptr);
			MechMove(mech) = compare_array(move_types, tmpc);
			TEMPLATE_GERR(MechMove(mech) == -1,
						  "Error while loading: Type %s not found.", tmpc);
			break;
		case 3:				/* Tons */
			MechTons(mech) = atoi(read_desc(fp, ptr));
			break;
		case 4:				/* Tac_Range */
			MechTacRange(mech) = atoi(read_desc(fp, ptr));
			break;
		case 5:				/* LRS_Range */
			MechLRSRange(mech) = atoi(read_desc(fp, ptr));
			break;
		case 6:				/* Radio Range */
			MechRadioRange(mech) = atoi(read_desc(fp, ptr));
			break;
		case 7:				/* Scan Range */
			MechScanRange(mech) = atoi(read_desc(fp, ptr));
			break;
		case 8:				/* Heat Sinks */
			MechRealNumsinks(mech) = atoi(read_desc(fp, ptr));
			break;
		case 9:				/* Max Speed */
			SetMaxSpeed(mech, atof(read_desc(fp, ptr)));
			TemplateMaxSpeed(mech) = MechMaxSpeed(mech);
			break;
		case 10:				/* Specials */
			tmpc = read_desc(fp, ptr);

			if(CheckSpecialsList(specials, specials2, tmpc)) {
				MechSpecials(mech) |= BuildBitVectorNoErr(specials, tmpc);
				MechSpecials2(mech) |= BuildBitVectorNoErr(specials2, tmpc);
			} else
				TEMPLATE_GERR(MechSpecials(mech) == -1,
							  "Error while loading: Invalid specials - %s.",
							  tmpc);
			break;
		case 11:				/* Armor */
			SetSectOArmor(mech, section, atoi(read_desc(fp, ptr)));
			SetSectArmor(mech, section, GetSectOArmor(mech, section));
			break;
		case 12:				/* Internals */
			SetSectOInt(mech, section, atoi(read_desc(fp, ptr)));
			SetSectInt(mech, section, GetSectOInt(mech, section));
			break;
		case 13:				/* Rear */
			SetSectORArmor(mech, section, atoi(read_desc(fp, ptr)));
			SetSectRArmor(mech, section, GetSectORArmor(mech, section));
			break;
		case 14:				/* Config */
			tmpc = read_desc(fp, ptr);
			MechSections(mech)[section].config =
				BuildBitVector(section_configs,
							   tmpc) & ~(CASE_TECH | SECTION_DESTROYED);
			TEMPLATE_GERR(MechSections(mech)[section].config == -1,
						  "Error while loading: Invalid location config: %s.",
						  tmpc);
			break;
		case 9999:
			if((sscanf(cmd, "CRIT_%d-%d", &x, &y)) == 2) {
				lpos = x - 1;
				hpos = y - 1;
			} else if((sscanf(cmd, "CRIT_%d", &x)) == 1) {
				lpos = x - 1;
				hpos = x - 1;
			} else
				break;
			critical = lpos;
			line2 = read_desc(fp, ptr);
			line2 = one_arg(line2, buf);
			if(!strncasecmp(buf, "CL.", 3))
				isClan = 1;
			TEMPLATE_GERR(!(find_matching_vlong_part(buf, NULL, &type,
													 &brand)),
						  "Unable to find %s", buf);
			SetPartType(mech, section, critical, type);
			if(!mudconf.btech_parts)
				brand = 0;
			SetPartBrand(mech, section, critical, brand);
			SetPartDesiredAmmoLoc(mech, section, critical, -1);

			if(IsWeapon(type)) {
				/* Thanks to legacy of past, we _do_ have to do this.. sniff */
				if(IsAMS(Weapon2I(type))) {
					if(MechWeapons[Weapon2I(type)].special & CLAT)
						MechSpecials(mech) |= CL_ANTI_MISSILE_TECH;
					else
						MechSpecials(mech) |= IS_ANTI_MISSILE_TECH;
				}
				SetPartData(mech, section, critical, 0);
				line2 = one_arg(line2, buf);	/* Don't need the '-' */
				line2 = one_arg(line2, buf);

/*              wFireModes = BuildBitVector(crit_fire_modes, buf); */

/*              wAmmoModes = BuildBitVector(crit_ammo_modes, buf); */

				wFireModes = BuildBitVectorWithDelim(crit_fire_modes, buf);
				wAmmoModes = BuildBitVectorWithDelim(crit_ammo_modes, buf);

				TEMPLATE_GERR(((wFireModes < 0) && (wAmmoModes < 0)),
							  "Error while loading: Invalid crit modes for weapon: %s.",
							  buf);

				if(wFireModes < 0)
					wFireModes = 0;

				if(wAmmoModes < 0)
					wAmmoModes = 0;

				GetPartFireMode(mech, section, critical) = wFireModes;
				GetPartAmmoMode(mech, section, critical) = wAmmoModes;

				line2 = one_arg(line2, buf);
				if(mudconf.btech_parts)
					if(atoi(buf)) {
						SetPartBrand(mech, section, critical, atoi(buf));
					}
			} else if(IsAmmo(type)) {
				line2 = one_arg(line2, buf);
				GetPartData(mech, section, critical) = atoi(buf);
				line2 = one_arg(line2, buf);

/*              wFireModes = BuildBitVector(crit_fire_modes, buf); */

/*              wAmmoModes = BuildBitVector(crit_ammo_modes, buf); */

				wFireModes = BuildBitVectorWithDelim(crit_fire_modes, buf);
				wAmmoModes = BuildBitVectorWithDelim(crit_ammo_modes, buf);

				TEMPLATE_GERR(((wFireModes < 0) && (wAmmoModes < 0)),
							  "Error while loading: Invalid crit modes for ammo: %s.",
							  buf);

				if(wFireModes < 0)
					wFireModes = 0;

				if(wAmmoModes < 0)
					wAmmoModes = 0;

				GetPartFireMode(mech, section, critical) = wFireModes;
				GetPartAmmoMode(mech, section, critical) = wAmmoModes;

				if(GetPartData(mech, section, critical) < FullAmmo(mech,
																   section,
																   critical))
				{
					GetPartFireMode(mech, section, critical) |= HALFTON_MODE;
					if(GetPartData(mech, section,
								   critical) > FullAmmo(mech, section,
														critical))
						GetPartFireMode(mech, section, critical) &=
							~HALFTON_MODE;
				}

				if(GetPartData(mech, section, critical) != FullAmmo(mech,
																	section,
																	critical)
				   && MechType(mech) != CLASS_MW
				   && MechType(mech) != CLASS_BSUIT) {
					SendError(tprintf
							  ("Invalid ammo crit for %s in #%d %s (%d/%d)",
							   MechWeapons[Ammo2I(type)].name, mech->mynum,
							   filename, GetPartData(mech, section, critical),
							   FullAmmo(mech, section, critical))
						);
					SetPartData(mech, section, critical, FullAmmo(mech,
																  section,
																  critical));
				}
			} else {
				GetPartData(mech, section, critical) = 0;
				GetPartFireMode(mech, section, critical) = 0;
				GetPartAmmoMode(mech, section, critical) = 0;
				if((line2 = one_arg(line2, buf)))
					if((line2 = one_arg(line2, buf))) {
						line2 = one_arg(line2, buf);
						if(mudconf.btech_parts)
							if(atoi(buf)) {
								SetPartBrand(mech, section, critical,
											 atoi(buf));
							}
					}
			}
			for(x = (lpos + 1); x <= hpos; x++) {
				SetPartType(mech, section, x, GetPartType(mech, section,
														  lpos));
				SetPartData(mech, section, x, GetPartData(mech, section,
														  lpos));
				SetPartFireMode(mech, section, x, GetPartFireMode(mech,
																  section,
																  lpos));
				SetPartAmmoMode(mech, section, x,
								GetPartAmmoMode(mech, section, lpos));
				SetPartBrand(mech, section, x,
							 GetPartBrand(mech, section, lpos));
			}
			break;
		case 15:				/* Mech's Computer level */
			MechComputer(mech) = atoi(read_desc(fp, ptr));
			break;
		case 16:				/* Name of the mech */
			strcpy(MechType_Name(mech), read_desc(fp, ptr));
			break;
		case 17:				/* Jj's */
			MechJumpSpeed(mech) = atof(read_desc(fp, ptr));
			break;
		case 18:				/* Radio */
			MechRadio(mech) = atoi(read_desc(fp, ptr));
			break;
		case 19:				/* SI */
			AeroSI(mech) = AeroSIOrig(mech) = atoi(read_desc(fp, ptr));
			break;
		case 20:				/* Fuel */
			AeroFuel(mech) = AeroFuelOrig(mech) = atoi(read_desc(fp, ptr));
			break;
		case 21:				/* Comment */
			break;
		case 22:				/* Radio_freqs */
			MechRadioType(mech) = atoi(read_desc(fp, ptr));
			break;
		case 23:				/* Mech battle value */
			MechBV(mech) = atoi(read_desc(fp, ptr));
			break;
		case 24:				/* Cargospace */
			CargoSpace(mech) = atoi(read_desc(fp, ptr));
			break;
		case 25:				/* Maxsuits */
			MechMaxSuits(mech) = atoi(read_desc(fp, ptr));
			break;
		case 26:				/* Specials */
			tmpc = read_desc(fp, ptr);

			if(CheckSpecialsList(infantry_specials, 0, tmpc))
				MechInfantrySpecials(mech) |=
					BuildBitVectorNoErr(infantry_specials, tmpc);

			break;
		case 27:				/* Carmaxton */
			CarMaxTon(mech) = atoi(read_desc(fp, ptr));
			break;
		}
	}
	fclose(fp);
	MechEngineSizeV(mech) = MechEngineSizeC(mech);
#define Set(a,b) \
		if (!(a)) a = b
	if(!(MechSpecials(mech) & ICE_TECH))
		Set(MechRealNumsinks(mech), DEFAULT_HEATSINKS);
	if(MechType(mech) == CLASS_MECH)
		do_sub_magic(mech, 1);
	if(MechType(mech) == CLASS_MW)
		Startup(mech);

	if(MechType(mech) == CLASS_MECH)
		value = 8;
	else
		value = 6;

	if(mudconf.btech_parts)
		for(x = 0; x < value; x++)
			for(y = 0; y < CritsInLoc(mech, x); y++)
				if((t = GetPartType(mech, x, y))) {
					if(GetPartBrand(mech, x, y))
						continue;
					if(IsAmmo(t))
						continue;
					if(IsBomb(t))
						continue;
					SetPartBrand(mech, x, y,
								 isClan ? DEFAULT_CLPART_LEVEL :
								 DEFAULT_PART_LEVEL);
				}
	if(isClan) {
		Set(MechComputer(mech), DEFAULT_CLCOMPUTER);
		Set(MechRadio(mech), DEFAULT_CLRADIO);
	} else {
		Set(MechComputer(mech), DEFAULT_COMPUTER);
		Set(MechRadio(mech), DEFAULT_RADIO);
	}
	if(!MechRadioType(mech))
		MechRadioType(mech) = generic_radio_type(MechRadio(mech), isClan);
	if(!MechComputer(mech)) {
		Set(MechScanRange(mech), DEFAULT_SCANRANGE);
		Set(MechLRSRange(mech), DEFAULT_LRSRANGE);
		Set(MechRadioRange(mech), DEFAULT_RADIORANGE);
		Set(MechTacRange(mech), DEFAULT_TACRANGE);
	} else {
		Set(MechScanRange(mech), MechComputersScanRange(mech));
		Set(MechLRSRange(mech), MechComputersLRSRange(mech));
		Set(MechRadioRange(mech), MechComputersRadioRange(mech));
		Set(MechTacRange(mech), MechComputersTacRange(mech));
	}
#if 1							/* Don't know if we're ready for this yet - aw, what the hell :) */
	if(MechType(mech) == CLASS_MECH)
		if((GetPartType(mech, LARM, 2) != Special(LOWER_ACTUATOR)) &&
		   (GetPartType(mech, RARM, 2) != Special(LOWER_ACTUATOR)) &&
		   (GetPartType(mech, LARM, 3) != Special(HAND_OR_FOOT_ACTUATOR))
		   && (GetPartType(mech, RARM, 3) != Special(HAND_OR_FOOT_ACTUATOR)))
			MechSpecials(mech) |= FLIPABLE_ARMS;
#endif
	update_specials(mech);
	mech_int_check(mech, 1);
	x = mech_weight_sub(GOD, mech, 0);
	y = MechTons(mech) * 1024;
	if(abs(x - y) > 40)
		SendError(tprintf
				  ("Error in %s template: %.1f tons of 'stuff', yet %d ton frame.",
				   MechType_Ref(mech), x / 1024.0, y / 1024));
	update_oweight(mech, x);
	if((map = FindObjectsData(mech->mapindex)))
		UpdateConditions(mech, map);
	/* To prevent certain funny occurences.. */
	for(i = 0; i < NUM_SECTIONS; i++)
		if(!(GetSectOInt(mech, i)))
			SetSectDestroyed(mech, i);
	return 0;
}

void DumpMechSpecialObjects(dbref player)
{
	coolmenu *c;

	c = AutoCol_StringMenu("MechSpecials available", internals);
	ShowCoolMenu(player, c);
	KillCoolMenu(c);
}

static char *dumpweapon_fun(int i)
{
	static char buf[256];

	buf[0] = 0;
	if(!i)
		sprintf(buf, WDUMP_MASKS);
	else {
		i--;
		sprintf(buf, WDUMP_MASK, MechWeapons[i].name, MechWeapons[i].heat,
				MechWeapons[i].damage, MechWeapons[i].min,
				MechWeapons[i].shortrange, MechWeapons[i].medrange,
				GunRange(i), MechWeapons[i].vrt, MechWeapons[i].criticals,
				MechWeapons[i].ammoperton);
	}
	return buf;
}

void DumpWeapons(dbref player)
{
	coolmenu *c;

	c = SelCol_FunStringMenuK(1, "MechWeapons available", dumpweapon_fun,
							  num_def_weapons + 1);
	ShowCoolMenu(player, c);
	KillCoolMenu(c);
}

char *techlist_func(MECH * mech)
{
	static char buffer[MBUF_SIZE];
	char bufa[SBUF_SIZE], bufb[SBUF_SIZE];
	int i, ii, part = 0, axe = 0, mace = 0, sword = 0, saw = 0, hascase = 0;

	snprintf(bufa, SBUF_SIZE, "%s",
			 BuildBitString(specialsabrev, MechSpecials(mech)));
	snprintf(bufb, SBUF_SIZE, "%s",
			 BuildBitString(specialsabrev2, MechSpecials2(mech)));
	snprintf(buffer, MBUF_SIZE, "%s %s", bufa, bufb);

	if(!
	   (strstr(buffer, "XL") || strstr(buffer, "XXL")
		|| strstr(buffer, "LENG") || strstr(buffer, "ICE")
		|| strstr(buffer, "CENG")))
		strcat(buffer, " FUS ");
	for(i = 0; i < NUM_SECTIONS; i++)
		for(ii = 0; ii < NUM_CRITICALS; ii++) {
			part = GetPartType(mech, i, ii);
			if(part == I2Special(AXE) && !axe) {
				axe = 1;
				strcat(buffer, " AXE");
			}
			if(part == I2Special(MACE) && !mace) {
				mace = 1;
				strcat(buffer, " MACE");
			}
			if(part == I2Special(DUAL_SAW) && !saw) {
				saw = 1;
				strcat(buffer, " DUAL_SAW");
			}
			if(part == I2Special(SWORD) && !sword) {
				sword = 1;
				strcat(buffer, " SWORD");
			}
			if((MechSections(mech)[i].config & CASE_TECH) && !hascase) {
				hascase = 1;
				strcat(buffer, " CASE");
			}
		}

	if(CargoSpace(mech))
		strcat(buffer, " INFC");

	if(MechType(mech) == CLASS_VTOL)
		strcat(buffer, " VTOL");

	if(MechType(mech) == CLASS_MECH && MechMove(mech) != MOVE_QUAD) {
		if((OkayCritSectS(RARM, 3, HAND_OR_FOOT_ACTUATOR)
			&& OkayCritSectS(RARM, 0, SHOULDER_OR_HIP))
		   || (OkayCritSectS(LARM, 3, HAND_OR_FOOT_ACTUATOR)
			   && OkayCritSectS(LARM, 0, SHOULDER_OR_HIP))
		   || MechSpecials(mech) & SALVAGE_TECH)
			strcat(buffer, " MTOW");
	} else {
		if(MechSpecials(mech) & SALVAGE_TECH)
			strcat(buffer, " MTOW");
	}

	return buffer;
}

/* Function to return the payload of a unit
 * Used by the btpayload_ref() scode function
 * Dany - 06/2005 */
char *payloadlist_func(MECH * mech)
{
	static char buffer[MBUF_SIZE];

	unsigned char weaparray[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	int critical[MAX_WEAPS_SECTION];
	short ammomode;
	int temp_crit;

	int count, weap_count, ammo_count, section_loop, weap_loop, put_loop;
	char payloadbuff[120];

	unsigned short payload_items[8 * MAX_WEAPS_SECTION];
	unsigned short payload_items_count[8 * MAX_WEAPS_SECTION];

	/* Clear the buffer */
	snprintf(buffer, MBUF_SIZE, "%s", "");

	/* Count each 'unique' item */
	weap_count = 0;
	ammo_count = 0;

	/* Initialize array */
	for(put_loop = 0; put_loop < 8 * MAX_WEAPS_SECTION; put_loop++) {
		payload_items[put_loop] = 0;
		payload_items_count[put_loop] = 0;
	}

	/* Get the weapons for each sections */
	for(section_loop = 0; section_loop < NUM_SECTIONS; section_loop++) {

		/* Get all the weapons for that section */
		count =
			FindWeapons(mech, section_loop, weaparray, weapdata, critical);
		/* Check if any weapons in that section */
		if(count <= 0)
			continue;

		/* Loop through all the weapons found and store their values */
		for(weap_loop = 0; weap_loop < count; weap_loop++) {

			/* Loop to put weapons in the temp array and keep count */
			for(put_loop = 0; put_loop < 8 * MAX_WEAPS_SECTION; put_loop++) {

				/* Check to see if there is already an entry */
				if(payload_items[put_loop] == weaparray[weap_loop]) {
					payload_items_count[put_loop]++;
					break;
					/* Ok, see if there is no entry */
				} else if(payload_items[put_loop] == 0) {
					payload_items[put_loop] = weaparray[weap_loop];
					payload_items_count[put_loop]++;
					weap_count++;
					break;
				}

			}					/* End of put loop */

		}						/* End of weap count loop */

	}							/* End of section loop */

	/* Loop through all the sections */
	for(section_loop = 0; section_loop < NUM_SECTIONS; section_loop++) {

		/* Loop through all the crits in a section */
		for(count = 0; count < MAX_WEAPS_SECTION; count++) {

			/* Get the Part at that spot */
			temp_crit = GetPartType(mech, section_loop, count);

			/* Is it Ammo? */
			if(IsAmmo(temp_crit)) {

				/* Loop to put weapons in the temp array and keep count */
				for(put_loop = weap_count; put_loop < 8 * MAX_WEAPS_SECTION;
					put_loop++) {

					/* Check to see if there is already an entry */
					if(payload_items[put_loop] == temp_crit) {
						payload_items_count[put_loop]++;
						break;
						/* Ok, see if there is no entry */
					} else if(payload_items[put_loop] == 0) {
						payload_items[put_loop] = temp_crit;
						payload_items_count[put_loop]++;
						ammo_count++;
						break;
					}

				}				/* End of put loop */

			}
			/* End of is it Ammo if Statement */
		}						/* End of Crit Slot Loop */

	}							/* End of Section Loop */

	/* Final loop to print out the full payload to the buffer and return it */
	for(put_loop = 0; put_loop < (weap_count + ammo_count); put_loop++) {

		/* If its a weapon use this method of printing it out
		 * Else use the part method */
		if(put_loop < weap_count) {
			sprintf(payloadbuff, "%s:%d",
					&MechWeapons[payload_items[put_loop]].name[0],
					payload_items_count[put_loop]);
		} else {
			sprintf(payloadbuff, "%s:%d",
					partname_func(payload_items[put_loop], 'V'),
					payload_items_count[put_loop]);
		}

		/* If we are not at the end, then put a | as a spacer */
		if(put_loop < (weap_count + ammo_count - 1)) {
			strncat(payloadbuff, "|", sizeof(buffer) - strlen(buffer) - 1);
		}

		/* Adding it to the main buffer */
		strncat(buffer, payloadbuff, sizeof(buffer) - strlen(buffer) - 1);

	}							/* End of printing loop */

	return buffer;
}
