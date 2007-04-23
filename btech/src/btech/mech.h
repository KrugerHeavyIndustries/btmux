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

#ifndef MECH_H
#define MECH_H

#include "externs.h"
#include "db.h"
#include "attrs.h"
#include "powers.h"
#include "mech.stat.h"
#include "muxevent.h"
#include "p.event.h"

#include "btconfig.h"
#include "mymath.h"

#include "glue_types.h"

#define NUM_ITEMS	1024
#define NUM_ITEMS_M	512
#define NUM_BAYS	4
#define NUM_TURRETS	3
#define C3I_NETWORK_SIZE 5
#define C3_NETWORK_SIZE	11	/* Constant for the max size of the network */
#define BRANDCOUNT	5

#define LEFTSIDE	1
#define RIGHTSIDE	2
#define FRONT		3
#define BACK		4

#define STAND		1
#define FALL		0

#define TURN		30		/* 30 sec turn */
#define KPH_PER_MP	10.75
#define MP_PER_KPH	0.0930233	/* 1/KPH_PER_MP  */
#define MP_PER_UPDATE_PER_KPH 0.003100777	/* MP_PER_KPH/30 */
#define SCALEMAP	322.5		/* 1/update      */
#define HEXLEVEL	5		/* levels/hex    */
#define ZSCALE		64.5		/* scalemap/hexlevel */
#define XSCALE		0.1547		/* hex constant  */
#define YSCALE2		9.61482e-6	/* update**2     */
#define MP1		10.75		/* 2*MS_PER_MP   */
#define MP2		21.50		/* 2*MS_PER_MP   */
#define MP3		32.25		/* 3*MS_PER_MP   */
#define MP4		43.00		/* 4*MS_PER_MP   */
#define MP5		53.75		/* 5*MS_PER_MP   */
#define MP6		64.50		/* 6*MS_PER_MP   */
#define MP9		96.75		/* 9*MS_PER_MP   */
#define DELTAFACING	1440.0

#define DEFAULT_FREQS	5
#define FREQS		16

#define FREQ_DIGITAL	1
#define FREQ_MUTE	2	/* For digital transmissions */
#define FREQ_RELAY	4	/* For digital transmissions */
#define FREQ_INFO	8	/* For digital transmissions */
#define FREQ_SCAN	16
#define FREQ_REST	32

#define RADIO_RELAY	1	/* ability to relay things */
#define RADIO_INFO	2	/* ability to see where (digital) message comes from */
#define RADIO_SCAN	4	/* ability to scan for frequencies */
#define RADIO_NODIGITAL 8	/* lacks the ability to hear or set digital freqs */

#define CHTITLELEN	15

#define NOT_FOUND	-1
#define NUM_CRITICALS	12

#define ARMOR		1
#define INTERNAL	2
#define REAR		3

#define NOARC		0
#define FORWARDARC	1
#define LSIDEARC	2
#define RSIDEARC	4
#define REARARC		8
#define TURRETARC	16

/*
   Critical Types
   0       Empty
   1-192   Weapons
   193-384 Ammo
   385-394 Bombs (Aero/VTOL droppable)
   395-511 Special startings...
 */

/* Critical Types... */
#define NUM_WEAPONS	192
#define NUM_BOMBS	9

#define EMPTY			0
#define WEAPON_BASE_INDEX	1
#define AMMO_BASE_INDEX		(WEAPON_BASE_INDEX + NUM_WEAPONS)	/* 193 */
#define BOMB_BASE_INDEX		(AMMO_BASE_INDEX + NUM_WEAPONS)		/* 385 */
#define SPECIAL_BASE_INDEX	(BOMB_BASE_INDEX + NUM_BOMBS)		/* 394 */
#define OSPECIAL_BASE_INDEX	220
#define CARGO_BASE_INDEX	512

#ifdef BT_ADVANCED_ECON
#define SPECIALCOST_SIZE        (CARGO_BASE_INDEX - SPECIAL_BASE_INDEX)
#define AMMOCOST_SIZE           NUM_WEAPONS
#define WEAPCOST_SIZE           NUM_WEAPONS
#define CARGOCOST_SIZE          (NUM_ITEMS - NUM_ITEMS_M)
#define BOMBCOST_SIZE           NUM_BOMBS
#endif

#define IsAmmo(a)           ((a) >= AMMO_BASE_INDEX && (a) < BOMB_BASE_INDEX)
#define IsBomb(a)           ((a) >= BOMB_BASE_INDEX && (a) < SPECIAL_BASE_INDEX)
#define IsSpecial(a)        ((a) >= SPECIAL_BASE_INDEX && (a) < CARGO_BASE_INDEX)
#define IsCargo(a)          ((a) >= CARGO_BASE_INDEX)
#define IsActuator(a)       (IsSpecial(a) && a <= I2Special(HAND_OR_FOOT_ACTUATOR))
#define IsWeapon(a)         ((a) >= WEAPON_BASE_INDEX && (a) < AMMO_BASE_INDEX)
#define IsArtillery(a)      (MechWeapons[a].type==TARTILLERY)
#define IsMissile(a)        (MechWeapons[a].type==TMISSILE)
#define IsBallistic(a)      (MechWeapons[a].type==TAMMO)
#define IsEnergy(a)         (MechWeapons[a].type==TBEAM)

/* Fun Weapons that do affects */
#define IsFlamer(a)         (strstr(MechWeapons[a].name, "Flamer"))
#define IsCoolant(a)        (strstr(MechWeapons[a].name, "Coolant"))
#define IsAcid(a)           (strstr(MechWeapons[a].name, "Acid"))

#ifdef BT_EXILE_MW3STATS
#endif

#define GunRangeWithCheck(mech,sec,a) (SectionUnderwater(mech,sec) > 0 ? GunWaterRange(a) : IsArtillery(a)?(ARTILLERY_MAPSHEET_SIZE * MechWeapons[a].longrange):(MechWeapons[a].longrange))
#define EGunRangeWithCheck(mech,sec,a) ((SectionUnderwater(mech,sec) > 0) ? EGunWaterRange(a) : (mudconf.btech_erange && (MechWeapons[a].medrange * 2) > GunRange(a)) ? (MechWeapons[a].medrange * 2) : GunRange(a))
#define GunRange(a)	(IsArtillery(a)?(ARTILLERY_MAPSHEET_SIZE * MechWeapons[a].longrange):(MechWeapons[a].longrange))
#define EGunRange(a)	((mudconf.btech_erange && (MechWeapons[a].medrange * 2) > GunRange(a)) ? (MechWeapons[a].medrange * 2) : GunRange(a))
#define GunWaterRange(a)	(MechWeapons[a].longrange_water > 0 ? MechWeapons[a].longrange_water : MechWeapons[a].medrange_water > 0 ? MechWeapons[a].medrange_water : MechWeapons[a].shortrange_water > 0 ? MechWeapons[a].shortrange_water : 0)
#define EGunWaterRange(a)	((mudconf.btech_erange && ((MechWeapons[a].medrange_water * 2) > GunWaterRange(a)) && (MechWeapons[a].longrange_water > 0)) ? (MechWeapons[a].medrange_water * 2) : GunWaterRange(a))
#define SectionUnderwater(mech,sec) (MechZ(mech) >= 0 ? 0 : (MechZ(mech) < -1) || (Fallen(mech)) ? 1 : ((sec == LLEG) || (sec == RLEG)) || (MechIsQuad(mech) && ((sec == LARM) || (sec == RARM))) ? 1 : 0)

#define Ammo2WeaponI(a)	((a) - AMMO_BASE_INDEX)
#define Ammo2Weapon(a)	Ammo2WeaponI(a)
#define Ammo2I(a)	Ammo2Weapon(a)
#define Bomb2I(a)	((a) - BOMB_BASE_INDEX)
#define Special2I(a)	((a) - SPECIAL_BASE_INDEX)
#define Cargo2I(a)	((a) - CARGO_BASE_INDEX)
#define Weapon2I(a)	((a) - WEAPON_BASE_INDEX)
#define I2Bomb(a)	((a) + BOMB_BASE_INDEX)
#define I2Weapon(a)	((a) + WEAPON_BASE_INDEX)
#define I2Ammo(a)	((a) + AMMO_BASE_INDEX)
#define I2Special(a)	((a) + SPECIAL_BASE_INDEX)
#define I2Cargo(a)	((a) + CARGO_BASE_INDEX)
#define Special		I2Special
#define Cargo		I2Cargo

/* To define one of these-> x=SPECIAL_BASE_INDEX+SHOULDER_OR_HIP */
#define SHOULDER_OR_HIP               0
#define UPPER_ACTUATOR                1
#define LOWER_ACTUATOR                2
#define HAND_OR_FOOT_ACTUATOR         3
#define LIFE_SUPPORT                  4
#define SENSORS                       5
#define COCKPIT                       6
#define ENGINE                        7
#define GYRO                          8
#define HEAT_SINK                     9
#define JUMP_JET                     10
#define CASE                         11
#define FERRO_FIBROUS                12
#define ENDO_STEEL                   13
#define TRIPLE_STRENGTH_MYOMER       14
#define TARGETING_COMPUTER           15
#define MASC                         16
#define C3_MASTER                    17
#define C3_SLAVE                     18
#define BEAGLE_PROBE                 19
#define ARTEMIS_IV                   20
#define ECM                          21
#define AXE                          22
#define SWORD                        23
#define MACE                         24
#define CLAW                         25
#define DS_AERODOOR                  26
#define DS_MECHDOOR                  27
#define FUELTANK                     28
#define TAG                          29
#define DS_TANKDOOR                  30
#define DS_CARGODOOR                 31
#define LAMEQUIP                     32
#define CASE_II                      33
#define STEALTH_ARMOR                34
#define NULL_SIGNATURE_SYSTEM        35
#define C3I                          36
#define ANGELECM                     37
#define HVY_FERRO_FIBROUS            38
#define LT_FERRO_FIBROUS             39
#define BLOODHOUND_PROBE             40
#define PURIFIER_ARMOR               41
#define KAGE_STEALTH_UNIT            42
#define ACHILEUS_STEALTH_UNIT        43
#define INFILTRATOR_STEALTH_UNIT     44
#define INFILTRATORII_STEALTH_UNIT   45
#define SUPERCHARGER                 46
#define DUAL_SAW                     47

#define LBX2_AMMO               0
#define LBX5_AMMO               1
#define LBX10_AMMO              2
#define LBX20_AMMO              3
#define LRM_AMMO                4
#define SRM_AMMO                5
#define SSRM_AMMO               6
#define NARC_LRM_AMMO		7
#define NARC_SRM_AMMO		8
#define NARC_SSRM_AMMO		9
#define ARTEMIS_LRM_AMMO	10
#define ARTEMIS_SRM_AMMO	11
#define ARTEMIS_SSRM_AMMO       12

#define PETROLEUM		13
#define PHOSPHORUS		14
#define HYDROGEN		15
#define GOLD			16
#define NATURAL_EXTRACTS	17
#define MARIJUANA		18
#define SULFUR			19
#define SODIUM			20
#define PLUTONIUM		21
#define ORE			22
#define METAL			23
#define PLASTICS		24
#define MEDICAL_SUPPLIES	25
#define COMPUTERS		26
#define EXPLOSIVES		27

#define ES_INTERNAL		28
#define FF_ARMOR		29
#define XL_ENGINE		30
#define DOUBLE_HEAT_SINK	31
#define IC_ENGINE		32

#define S_ELECTRONIC		33
#define S_INTERNAL		34
#define S_ARMOR			35
#define S_ACTUATOR		36
#define S_AERO_FUEL		37
#define S_DS_FUEL		38
#define S_VTOL_FUEL		39

#define SWARM_LRM_AMMO		40
#define SWARM1_LRM_AMMO		41
#define INFERNO_SRM_AMMO	42

#define XXL_ENGINE		43
#define COMP_ENGINE		44

#define HD_ARMOR		45
#define RE_INTERNAL		46
#define CO_INTERNAL		47
#define MRM_AMMO		48
#define LIGHT_ENGINE		49
#define CASEII			50
#define STH_ARMOR		51
#define NULLSIGSYS		52
#define SILICON			53
#define HVY_FF_ARMOR		54
#define LT_FF_ARMOR		55

#define INARC_EXPLO_AMMO	56
#define INARC_HAYWIRE_AMMO	57
#define INARC_ECM_AMMO		58
#define INARC_NEMESIS_AMMO	59

#define AC2_AP_AMMO		60
#define AC5_AP_AMMO		61
#define AC10_AP_AMMO		62
#define AC20_AP_AMMO		63
#define LAC2_AP_AMMO		64
#define LAC5_AP_AMMO		65
#define AC2_FLECHETTE_AMMO	66
#define AC5_FLECHETTE_AMMO	67
#define AC10_FLECHETTE_AMMO	68
#define AC20_FLECHETTE_AMMO	69
#define LAC2_FLECHETTE_AMMO	70
#define LAC5_FLECHETTE_AMMO	71
#define AC2_INCENDIARY_AMMO	72
#define AC5_INCENDIARY_AMMO	73
#define AC10_INCENDIARY_AMMO	74
#define AC20_INCENDIARY_AMMO	75
#define LAC2_INCENDIARY_AMMO	76
#define LAC5_INCENDIARY_AMMO	77
#define AC2_PRECISION_AMMO	78
#define AC5_PRECISION_AMMO	79
#define AC10_PRECISION_AMMO	80
#define AC20_PRECISION_AMMO	81
#define LAC2_PRECISION_AMMO	82
#define LAC5_PRECISION_AMMO	83
#define LR_DFM_AMMO		84
#define SR_DFM_AMMO		85
#define SLRM_AMMO		86
#define ELRM_AMMO		87
#define BSUIT_SENSOR		88
#define BSUIT_LIFESUPPORT	89
#define BSUIT_ELECTRONIC	90
#define CARGO_OIL		91
#define CARGO_WATER		92
#define CARGO_EARTH		93
#define CARGO_OXYGEN		94
#define CARGO_NITROGEN		95
#define CARGO_NICKEL		96
#define CARGO_STEEL		97
#define CARGO_IRON		98
#define CARGO_BRASS		99
#define CARGO_PLATINUM		100
#define CARGO_COPPER		101
#define CARGO_ALUMINUM		102
#define CARGO_CONSUMER_GOOD	103
#define CARGO_MACHINERY		104
#define CARGO_SLAVES		105
#define CARGO_TIMBIQUI_DARK	106
#define CARGO_COCAINE		107
#define CARGO_HEROINE		108
#define CARGO_MARBLE		109
#define CARGO_GLASS		110
#define CARGO_DIAMOND		111
#define CARGO_COAL		112
#define CARGO_FOOD		113
#define CARGO_ZINC		114
#define CARGO_FABRIC		115
#define CARGO_CLOTHING		116
#define CARGO_WOOD		117
#define CARGO_PULP		118
#define CARGO_LUMBER		119
#define CARGO_RUBBER		120
#define CARGO_SEEDS		121
#define CARGO_FERTILIZER	122
#define CARGO_SALT		123
#define CARGO_LITHIUM		124
#define CARGO_HELIUM		125
#define CARGO_LARIUM		126
#define CARGO_URANIUM		127
#define CARGO_IRIDIUM		128
#define CARGO_TITANIUM		129
#define CARGO_CONCRETE		130
#define CARGO_FERROCRETE	131
#define CARGO_BUILDING_SUPPLIES	132
#define CARGO_KEVLAR		133
#define CARGO_WASTE		134
#define CARGO_LIVESTOCK		135
#define CARGO_PAPER		136
#define XL_GYRO			137
#define HD_GYRO			138
#define COMP_GYRO		139
#define COMPACT_HEAT_SINK	140
#define AMMO_LRM_STINGER	141
#define AC2_CASELESS_AMMO	142
#define AC5_CASELESS_AMMO	143
#define AC10_CASELESS_AMMO	144
#define AC20_CASELESS_AMMO	145
#define LAC2_CASELESS_AMMO	146
#define LAC5_CASELESS_AMMO	147
#define AMMO_LRM_SGUIDED	148

#ifdef BT_COMPLEXREPAIRS
#define TON_SENSORS_FIRST       149
#define TON_SENSORS_LAST        (TON_SENSORS_FIRST + 9)

#define TON_MYOMER_FIRST        (TON_SENSORS_LAST + 1)
#define TON_MYOMER_LAST         (TON_MYOMER_FIRST + 9)

#define TON_TRIPLEMYOMER_FIRST  (TON_MYOMER_LAST + 1)
#define TON_TRIPLEMYOMER_LAST   (TON_TRIPLEMYOMER_FIRST + 9)

#define TON_INTERNAL_FIRST      (TON_TRIPLEMYOMER_LAST + 1)
#define TON_INTERNAL_LAST       (TON_INTERNAL_FIRST + 9)

#define TON_ESINTERNAL_FIRST    (TON_INTERNAL_LAST + 1)
#define TON_ESINTERNAL_LAST     (TON_ESINTERNAL_FIRST + 9)

#define TON_JUMPJET_FIRST       (TON_ESINTERNAL_LAST + 1)
#define TON_JUMPJET_LAST        (TON_JUMPJET_FIRST + 9)

#define TON_ARMUPPER_FIRST      (TON_JUMPJET_LAST + 1)
#define TON_ARMUPPER_LAST       (TON_ARMUPPER_FIRST + 9)

#define TON_ARMLOWER_FIRST      (TON_ARMUPPER_LAST + 1)
#define TON_ARMLOWER_LAST       (TON_ARMLOWER_FIRST + 9)

#define TON_ARMHAND_FIRST       (TON_ARMLOWER_LAST + 1)
#define TON_ARMHAND_LAST        (TON_ARMHAND_FIRST + 9)

#define TON_LEGUPPER_FIRST      (TON_ARMHAND_LAST + 1)
#define TON_LEGUPPER_LAST       (TON_LEGUPPER_FIRST + 9)

#define TON_LEGLOWER_FIRST      (TON_LEGUPPER_LAST + 1)
#define TON_LEGLOWER_LAST       (TON_LEGLOWER_FIRST + 9)

#define TON_LEGFOOT_FIRST       (TON_LEGLOWER_LAST + 1)
#define TON_LEGFOOT_LAST        (TON_LEGFOOT_FIRST + 9)

#define TON_ENGINE_FIRST        (TON_LEGFOOT_LAST + 1)
#define TON_ENGINE_LAST         (TON_ENGINE_FIRST + 19)

#define TON_ENGINE_XL_FIRST     (TON_ENGINE_LAST + 1)
#define TON_ENGINE_XL_LAST      (TON_ENGINE_XL_FIRST + 19)

#define TON_ENGINE_ICE_FIRST    (TON_ENGINE_XL_LAST + 1)
#define TON_ENGINE_ICE_LAST     (TON_ENGINE_ICE_FIRST + 19)


#define TON_ENGINE_LIGHT_FIRST  (TON_ENGINE_ICE_LAST + 1)
#define TON_ENGINE_LIGHT_LAST   (TON_ENGINE_LIGHT_FIRST + 19)

#define TON_COINTERNAL_FIRST    (TON_ENGINE_LIGHT_LAST + 1)
#define TON_COINTERNAL_LAST     (TON_COINTERNAL_FIRST + 9)


#define TON_REINTERNAL_FIRST    (TON_COINTERNAL_LAST + 1)
#define TON_REINTERNAL_LAST     (TON_REINTERNAL_FIRST + 9)

#define TON_GYRO_FIRST          (TON_REINTERNAL_LAST + 1)
#define TON_GYRO_LAST           (TON_GYRO_FIRST + 3)

#define TON_XLGYRO_FIRST        (TON_GYRO_LAST + 1)
#define TON_XLGYRO_LAST         (TON_XLGYRO_FIRST + 3)

#define TON_HDGYRO_FIRST        (TON_XLGYRO_LAST + 1)
#define TON_HDGYRO_LAST         (TON_HDGYRO_FIRST + 3)

#define TON_CGYRO_FIRST         (TON_HDGYRO_LAST + 1)
#define TON_CGYRO_LAST          (TON_CGYRO_FIRST + 3)

#define TON_ENGINE_XXL_FIRST    (TON_CGYRO_LAST + 1)
#define TON_ENGINE_XXL_LAST     (TON_ENGINE_XXL_FIRST + 19)

#define TON_ENGINE_COMP_FIRST   (TON_ENGINE_XXL_LAST + 1)
#define TON_ENGINE_COMP_LAST    (TON_ENGINE_COMP_FIRST + 19)
#endif


/* Weapons structure and array... */
#define TBEAM		0
#define TMISSILE	1
#define TARTILLERY	2
#define TAMMO		3
#define THAND		4

/* Tic status */

#define TIC_NUM_DESTROYED	-2
#define TIC_NUM_RELOADING	-3
#define TIC_NUM_RECYCLING	-4
#define TIC_NUM_PHYSICAL	-5

/* This is the max weapons per area- assuming 12 critical location and */
 /* the smallest weapon requires 1 */
#define MAX_WEAPS_SECTION	12

struct weapon_struct {
    char *name;
    char vrt;
    char type;
    char heat;
    char damage;
    char min;
    int shortrange;
    int medrange;
    int longrange;
    char min_water;
    int shortrange_water;
    int medrange_water;
    int longrange_water;
    char criticals;
    unsigned char ammoperton;
    unsigned short weight;	/* in 1/100ths tons */
    short explosiondamage;	/* Damage done when exploding (GR/LGR/HGR) */
    long special;
    int battlevalue;
    int cost;
    int ammo_bv;
    int ammo_cost;
};

/* special weapon effects */
#define NONE		0x00000000
#define PULSE		0x00000001	/* Pulse laser */
#define LBX		0x00000002	/* LBX AC */
#define ULTRA		0x00000004	/* Ultra AC */
#define STREAK		0x00000008	/* Streak missile */
#define GAUSS		0x00000010	/* Gauss weapon */
#define NARC		0x00000020	/* NARC launcher */
#define IDF		0x00000040	/* Can be used w/ IDF */
#define DAR		0x00000080	/* Has artillery-level delay on hit (1sec/2hex) */
#define HYPER		0x00000100	/* Hyper AC */
#define A_POD		0x00000200	/* Anti-infantry Pod */
#define CLAT		0x00000400	/* Clan-tech */
#define NOSPA		0x00000800	/* Does not allow special ammo (swarm, etc) */
#define PC_HEAT		0x00001000	/* Heat-based PC weapon (laser/inferno/..) */
#define PC_IMPA		0x00002000	/* Impact (weapons) */
#define PC_SHAR		0x00004000	/* Shrapnel / slash (various kinds of weapons) */
#define AMS		0x00008000	/* AntiMissileSystem */
#define NOBOOM		0x00010000	/* No ammo boom */
#define CASELESS	0x00020000	/* Caseless AC */
#define DFM		0x00040000	/* DFM - 2 worst rolls outta 3 for missiles */
#define ELRM		0x00080000	/* ELRM - 2 worst rolls outta 3 for missiles under */
#define MRM		0x00100000	/* MRM - +1 BTH */
#define CHEAT		0x00200000	/* Can cause heat or damage */
#define HVYW		0x00400000	/* Clam HeavyWeapons (call 'm so cuz FA$A will undoubtly bring more variants to the lasers) */
#define RFAC		0x00800000	/* Rapid fire ACs */
#define GMG		0x01000000	/* Gattling MGs */
#define INARC		0x02000000	/* iNARC launcher */
#define RAC		0x04000000	/* Rotary AC */
#define HVYGAUSS	0x08000000	/* Heavy Gauss */
#define ROCKET		0x10000000	/* Rocket launchers. +1 to hit, one shot wonders */
#define SPLIT_CRITS     0x20000000	/* Certain weapons can split crits. Mark them appropriately */

#define PCOMBAT		(PC_HEAT|PC_IMPA|PC_SHAR)

#define MAX_ROLL 11
struct missile_hit_table_struct {
    char *name;
    int key;
    int num_missiles[MAX_ROLL];
};

/* Section #defs... */

/* The unusual order is related to the locations of weapons of high */

/* magnitude versus weapons of low mag */
#define LARM		0
#define RARM		1
#define LTORSO		2
#define RTORSO		3
#define CTORSO		4
#define LLEG		5
#define RLEG		6
#define HEAD		7
#define NUM_SECTIONS	8

/*  These defs are for Vehicles */
#define LSIDE		0
#define RSIDE		1
#define FSIDE		2
#define BSIDE		3
#define TURRET		4
#define ROTOR		5
#define NUM_VEH_SECTIONS 6

/* Aerofighter */
#define AERO_NOSE	0
#define AERO_LWING	1
#define AERO_RWING	2
#define AERO_AFT	3
#define NUM_AERO_SECTIONS 4

#define NUM_BSUIT_MEMBERS 8

#define DS_RWING	0	/* Right Front Side / Right Wing */
#define DS_LWING	1	/* Left Front Side / Left Wing */
#define DS_LRWING	2	/* Left Rear Side */
#define DS_RRWING	3	/* Right Rear Side / Right Wing */
#define DS_AFT		4
#define DS_NOSE		5

#define NUM_DS_SECTIONS 6
#define SpheroidDS(a) (MechType(a)==CLASS_SPHEROID_DS)
#define SpheroidToRear(mech,a) \
if (MechType(mech) == CLASS_SPHEROID_DS) \
	(a) = ((a) == DS_LWING ? DS_LRWING : DS_RRWING)


#define NUM_TICS		4
#define MAX_WEAPONS_PER_MECH	96	/* Thanks to crit limits */
#define SINGLE_TICLONG_SIZE	32
#define TICLONGS		(MAX_WEAPONS_PER_MECH / SINGLE_TICLONG_SIZE)

/* structure for each critical hit section */
struct critical_slot {
    unsigned char brand;	/* Hold brand number, and damage (upper 4 bits) */
    unsigned char data;		/* Holds information like ammo remaining, etc */
    unsigned short type;	/* Type of item that this is a critical for */
    unsigned int firemode;	/* Holds info like rear mount, ultra mode... */
    unsigned int ammomode;	/* Holds info for the special ammo type in use */
    unsigned int weapDamageFlags;	/* Holds the enhanced critical damage flags */
    short desiredAmmoLoc;	/* Location of the desired ammo bin */
//    unsigned int recycle;   /* time when it will finish recycling */
};

/* Fire modes */
#define DESTROYED_MODE          0x00000001  /* the part is destroyed */
#define DISABLED_MODE           0x00000002  /* the part is disabled */
#define BROKEN_MODE             0x00000004  /* the part is part of a destroyed weapon/item */
#define DAMAGED_MODE            0x00000008  /* the part is damaged from an enhanced critical */
#define ON_TC                   0x00000010  /* (T) Set if the wepons mounted with TC */
#define REAR_MOUNT              0x00000020  /* (R) set if weapon is rear mounted */
#define HOTLOAD_MODE            0x00000040  /* (H) Weapon's being hotloaded */
#define HALFTON_MODE            0x00000080  /* Weapon is in halfton mode */
#define OS_MODE                 0x00000100  /* (O) In weapon itself : Weapon's one-shot */
#define OS_USED                 0x00000200  /* One-shot ammo _has_ been already used */
#define ULTRA_MODE              0x00000400  /* (U) set if weapon is in Ultra firing mode */
#define RFAC_MODE               0x00000800  /* (F) the weapon is set as a rapid fire AC */
#define GATTLING_MODE           0x00001000  /* (G) For Gattling MGs */
#define RAC_TWOSHOT_MODE        0x00002000  /* (2) RAC in two shot mode */
#define RAC_FOURSHOT_MODE       0x00004000  /* (4) RAC in four shot mode */
#define RAC_SIXSHOT_MODE        0x00008000  /* (6) RAC in six shot mode */
#define HEAT_MODE               0x00010000  /* (H) Toggle a flamer into heat mode */
#define WILL_JETTISON_MODE      0x00020000  /* Set if the slot will get destroyed during a backpack jettison (BSuits) */
#define IS_JETTISONED_MODE      0x00040000  /* Set if the slot has been jettisoned (BSuits) */
#define OMNI_BASE_MODE          0x00080000  /* Set if the slot part of the base config of an omni mech */
#define ROCKET_FIRED            0x00100000  /* Set if the slot's rocket launcher has been fired */

#define RAC_MODES	(RAC_TWOSHOT_MODE|RAC_FOURSHOT_MODE|RAC_SIXSHOT_MODE)
#define FIRE_MODES	(HOTLOAD_MODE|ULTRA_MODE|RFAC_MODE|GATTLING_MODE|RAC_MODES|HEAT_MODE)

/* Ammo modes */
#define LBX_MODE                0x00000001  /* (L) set if weapon is firing LBX ammo */
#define ARTEMIS_MODE            0x00000002  /* (A) artemis compatible missiles/laucher */
#define NARC_MODE               0x00000004  /* (N) narc compatible missiles/launcher */
#define CLUSTER_MODE            0x00000008  /* (C) Set if weapon is firing cluster ammo */
#define MINE_MODE               0x00000010  /* (M) Set if weapon's firing mines */
#define SMOKE_MODE              0x00000020  /* (S) Set if weapon's firing smoke rounds */
#define INFERNO_MODE            0x00000040  /* (I) SRM's loaded with Inferno rounds (cause heat) */
#define SWARM_MODE              0x00000080  /* (W) LRM's loaded with Swarm rounds */
#define SWARM1_MODE             0x00000100  /* (1) LRM's loaded with Swarm1 rounds (FoF) */
#define INARC_EXPLO_MODE        0x00000200  /* (X) inarc launcher firing explosive pods */
#define INARC_HAYWIRE_MODE      0x00000400  /* (Y) inarc launcher firing haywire pods */
#define INARC_ECM_MODE          0x00000800  /* (E) inarc launcher firing ecm pods */
#define INARC_NEMESIS_MODE      0x00001000  /* (Z) inarc launcher firing nemesis pods */
#define AC_AP_MODE              0x00002000  /* (R) autocannon firing armor piercing rounds */
#define AC_FLECHETTE_MODE       0x00004000  /* (F) autocannon firing flechette rounds */
#define AC_INCENDIARY_MODE      0x00008000  /* (D) autocannon firing incendiary rounds */
#define AC_PRECISION_MODE       0x00010000  /* (P) autocannon firing precision rounds */
#define STINGER_MODE            0x00020000  /* (T) AntiAir LRM */
#define AC_CASELESS_MODE	0x00040000  /* (U) autocannon firing caseless rounds */
#define SGUIDED_MODE		0x00080000  /* (G) LRM's loaded with Semi-Guided rounds (benefits only if unit is lit by 'TAG' */

#define ARTILLERY_MODES		(CLUSTER_MODE|MINE_MODE|SMOKE_MODE)
#define INARC_MODES		(INARC_EXPLO_MODE|INARC_HAYWIRE_MODE|INARC_ECM_MODE|INARC_NEMESIS_MODE)
#define MISSILE_MODES		(ARTEMIS_MODE|NARC_MODE|INFERNO_MODE|SWARM_MODE|SWARM1_MODE|STINGER_MODE|SGUIDED_MODE)
#define AC_MODES		(AC_AP_MODE|AC_FLECHETTE_MODE|AC_INCENDIARY_MODE|AC_PRECISION_MODE|AC_CASELESS_MODE)
#define AMMO_MODES		(LBX_MODE|AC_MODES|MISSILE_MODES|INARC_MODES|ARTILLERY_MODES)

/* Enhanced critical damage flags */
#define WEAP_DAM_MODERATE	0x00000001	/* +1 to hit */
#define WEAP_DAM_EN_FOCUS	0x00000002	/* Energy weapons: Focus misaligned. -1 damage, +1 BTH at med and long range */
#define WEAP_DAM_EN_CRYSTAL	0x00000004	/* Energy weapons: Crystal damaged. +1 heat. Roll of 2 results in ammo like explosion */
#define WEAP_DAM_BALL_BARREL	0x00000008	/* Ballistic weapons: Barrel damaged. Roll of 2 results in weapon jam */
#define WEAP_DAM_BALL_AMMO	0x00000010	/* Ballistic weapons: Ammo feed damaged. Can not switch ammo type. Roll of 2 results in ammo explosion */
#define WEAP_DAM_MSL_RANGING	0x00000020	/* Missile weapons: Ranging system hit. +1 BTH at med and long ranges */
#define WEAP_DAM_MSL_AMMO	0x00000040	/* Missile weapons: Ammo feed damaged. Can not switch ammo type. Roll of 2 results in ammo explosion */

/* Structure for each of the 8 sections */
struct section_struct {
    unsigned char armor;	/* External armor value */
    unsigned char internal;	/* Internal armor value */
    unsigned char rear;		/* Rear armor value */
    unsigned char armor_orig;
    unsigned char internal_orig;
    unsigned char rear_orig;
    char basetohit;		/* Holds to hit modifiers for weapons in section */
    char config;		/* flags for CASE, etc. */
    char recycle;		/* after physical attack, set counter */
    unsigned short specials;	/* specials for this section, like attached NARC pods, etc... */
    struct critical_slot criticals[NUM_CRITICALS];	/* Criticals */
};

/* Section configurations */
#define CASE_TECH		0x01	/* section has CASE technology */
#define SECTION_DESTROYED	0x02	/* section has been destroyed */
#define SECTION_BREACHED	0x04	/* section has been exposed to vacuum */
#define SECTION_FLOODED		0x08	/* section has been flooded with water - Kipsta. 8/3/99 */
#define AXED			0x10	/* arm was used to axe/sword someone */
#define STABILIZERS_DESTROYED	0x20	/* vehicle only. Double attacker mod for weapons from the section */
#define CASEII_TECH		0x40	/* section has CASE II technology */

/* Section specials */
#define NARC_ATTACHED		0x00000001	/* set if mech has a NARC beacon attached. */
#define INARC_HOMING_ATTACHED	0x00000002	/* set if mech has an iNARC homing beacon attached. */
#define INARC_HAYWIRE_ATTACHED	0x00000004	/* set if mech has an iNARC haywire beacon attached. */
#define INARC_ECM_ATTACHED	0x00000008	/* set if mech has an iNARC ecm beacon attached. */
#define INARC_NEMESIS_ATTACHED	0x00000010	/* set if mech has an iNARC nemesis beacon attached. */
#define CARRYING_CLUB		0x00000020	/* carrying a club in this location */

/* ground combat types */
#define CLASS_MECH		0
#define CLASS_VEH_GROUND	1
#define CLASS_VEH_NAVAL		3

/* Air types */
#define CLASS_VTOL		2
#define CLASS_SPHEROID_DS	4	/* Spheroid DropShip */
#define CLASS_AERO		5
#define CLASS_MW		6	/* Ejected MechWarrior */
#define CLASS_DS		7	/* AeroDyne DropShip */
#define CLASS_BSUIT		8
#define CLASS_LAST		8

#define DropShip(a) ((a)==CLASS_DS || (a)==CLASS_SPHEROID_DS)
#define IsDS(m)             (DropShip(MechType(m)))

/* ground movement types */
#define MOVE_BIPED		0
#define MOVE_QUAD		8
#define MOVE_TRACK		1
#define MOVE_WHEEL		2
#define MOVE_HOVER		3
#define MOVE_HULL		5
#define MOVE_FOIL		6
#define MOVE_SUB		9

/* Air movenement types */
#define MOVE_VTOL		4
#define MOVE_FLY		7

#define MOVE_NONE		10	/* Stationary, for one reason or another */

#define MOVENEMENT_LAST		10

/* Mech Preferences list */
#define MECHPREF_PKILL          0x01    /* Kill MWs anyway */
#define MECHPREF_SLWARN         0x02    /* Warn when lit by slite */
#define MECHPREF_AUTOFALL       0x04    /* Jump off cliffs (don't try to avoid) */
#define MECHPREF_NOARMORWARN    0x08    /* Don't warn when armor is getting low */
#define MECHPREF_NOAMMOWARN     0x10    /* Don't warn when ammo is getting low */
#define MECHPREF_STANDANYWAY    0x20    /* Try to stand even when BTH too high */
#define MECHPREF_AUTOCON_SD     0x40    /* Autocon on non-started units */
#define MECHPREF_NOFRIENDLYFIRE 0x80    /* Disallow firing on teammates */
#define MECHPREF_TURNMODE       0x100   /* Tight or Normal for Maneuvering Ace */

typedef struct {
    char mech_name[31];		/* Holds the 30 char ID for the mech */
    char mech_type[15];		/* Holds the mechref for the mech */
    char type;			/* The type of this unit */
    char move;			/* The movement type of this unit */
    char tac_range;		/* Tactical range for sensors */
    char lrs_range;		/* Long range for sensors */
    char scan_range;		/* Range for scanners */
    char numsinks;		/* number of heatsinks (also engine */
    char computer;		/* Partially replaces tac/lrs/scan/radiorange */
    char radio;
    unsigned char radioinfo;
    /* crits ( - from heatsinks) ) */
    char si;			/* Structural integrity of a craft */
    char si_orig;		/* maximum struct. int */

    short radio_range;		/* Can read/write comfortably at that distance */

    struct section_struct sections[NUM_SECTIONS];	/* armor */
    int fuel;			/* Fuel left */
    int fuel_orig;		/* Fuel tank capacity */

    int tons;			/* How much I weigh */
    int walkspeed;		/* Future expansion to do speed correctly */
    int runspeed;
    float maxspeed;		/* Maxspeed (running) in KPH */
    float template_maxspeed;    /* we should read this in */

    int mechbv;			/* Fasa BattleValue of this unit */
    int cargospace;		/* Assigned cargo space * 100 for half and quarter tons */
#ifndef BT_CALCULATE_BV
    int unused[8];		/* Space for future expansion */
#else
    int mechbv_last;		/* BV caclulation cacher */
#endif
    char targcomp;		/* Targeting comp mode. */
    char unused_char[3];
    char carmaxton;             /* Max Tonnage variable for carrier sizing */
} mech_ud;

typedef struct {
    char jumptop;		/* How many MPs we've left for vertical stuff? */
    char aim;			/* section of target aimed at */
    char basetohit;		/* total to hit modifiers from critical hits */
    char pilotskillbase;	/* holds constant skills mods */
    char engineheat;		/* +5 per critical hit there */
    char masc_value;		/* MASC roll .. updated up/down as needed */
    char aim_type;		/* Type we aim at */

    char sensor[2];		/* Primary mode, secondary mode */
    byte fire_adjustment;	/* For artillery mostly */
    char vis_mod;		/* Should be in range of 0 to 100 ; basically, this
				   is used as _base_ of random element in each sensor type, altered
				   once every heat update (and when mech's sensor mode changes) */
    char chargetimer;		/* # of movement ticks since 'charge' command */
    float chargedist;		/* # of hexes moved since 'charge' command */
    char staggerstamp;		/* When in last turn this 'mech staggered */

    short mech_prefs;		/* Mech preferences */
    short jumplength;		/* in real coords (for jump and goto) */
    short goingx, goingy;	/* in map coords (for jump and goto) */
    short desiredfacing;	/* You are turning if this != facing */
    short angle;		/* For DS / Aeros */
    short jumpheading;		/* Jumping head */
    short targx, targy, targz;	/* in map coords, target squares */
    short turretfacing;		/* Jumping head */
    short turndamage;		/* holds damge taken in 5 sec interval */
    short lateral;		/* Quad lateral move mode */
    short num_seen;		/* Number of enemies seen */
    short lx, ly;


    dbref chgtarget;		/* My CHARGE target */
    dbref dfatarget;		/* My DFA target */
    dbref target;		/* My default target */
    dbref swarming;		/* Swarm target */
    dbref swarmedby;		/* Who's swarming/mounting us */
    dbref carrying;		/* Who are we lugging about? */
    dbref spotter;		/* Who's spotting for us? */

    float heat;			/* Heat index */
    float weapheat;		/* Weapon heat factor-> see manifesto */
    float plus_heat;		/* how much heat I am producing */
    float minus_heat;		/* how much heat I can dissipate */

    float startfx, startfy;	/* in real coords (for jump and goto) */
    float startfz, endfz;	/* startstuff's also aeros' speed */
    float verticalspeed;	/* VTOL vertical speed in KPH */
    float speed;		/* Speed in KPH */
    float desired_speed;	/* Desired speed in KPH */
    float jumpspeed;		/* Jumping distance or current height in km */

    int critstatus;		/* see key below */
    int status;			/* see key below */
    int status2;		/* see key below */
    int specials;		/* see key below */
    int specials2;		/* More tech specials */
    int specialsstatus;		/* status element specials, like ECM, etc... */
    int tankcritstatus;		/* status element for crits that are specific to vehicles. see key below */

    time_t last_weapon_recycle;	/* This updated only on 'as needed' basis ;
				   basically, all weapon recycling events
				   compare the current time to the
				   last_weapon_recycle, and send recycled-messages
				   for all recycled weapons. */
    int cargo_weight;		/* How much stuff do we have? */

    /* BTHRandomization stuff (rok) ;) */
    int lastrndu;
    int rnd;

    int last_ds_msg;		/* Used for DS-spam */
    int boom_start;		/* Used for Stackpole-effect */

    int maxfuel;		/* How much fuel fits to this thing anyway? */
    int lastused;		/* Idle timeout thing */
    int cocoon;			/* OOD cocoon */
    int commconv;		/* Evil magic related to commconv, p1 */
    int commconv_last;		/* Evil magic related to commconv, p2 */
    int onumsinks;		/* Original HS (?) */
    int disabled_hs;		/* Disabled (on purpose, not destroyed) HS */
    int autopilot_num;
    int heatboom_last;
    int sspin;			/* Start of aero spin */
    int can_see;
    int row;			/* _Own_ weight */
    int rcw;			/* _Carried_ weight */
    float rspd;
    int erat;
    int per;
    int wxf;
    int last_startup;		/* timestamp of last 'startup' */
    int maxsuits;		/* Maximum number of bsuits in this unit */
    int infantry_specials;	/* Infantry related specials */
    char scharge_value;		/* Supercharger roll .. updated up/down as needed */
    int staggerDamage;		/* Damage for Stagger MkII */
    int lastStaggerNotify;	/* The level that we were last notified of a stagger */
    int critstatus2;            /* Starting to fill up. More CritStatus */
    int unused[5];		/* Space for future expansion */
    float xpmod;		/* Used to modify XP values per unit. Will default loading to 1 */
} mech_rd;

typedef struct {
    char pilotstatus;		/* damage pilot has taken */
    char terrain;		/* Terrain I am in */
    char elev;			/* Elevation I am at */
    short hexes_walked;		/* Hexes walked counter */
    short facing;		/* 0-359.. */
    short x, y, z;		/* hex quantized x,y,z on the map in MP (hexes) */
    short last_x, last_y;	/* last hex entered */
    float fx, fy, fz;		/* exact x, y and z on the map */
    int team;			/* Only for internal use */
    int unusable_arcs;		/* Horrid kludge for disallowing use of some arcs' guns */
    int stall;			/* is this mech in a repair stall? */
    dbref pilot;		/* My pilot */
    dbref bay[NUM_BAYS];
    dbref turret[NUM_TURRETS];
} mech_pd;

typedef struct {
    char C3ChanTitle[CHTITLELEN + 1];	/* applies to C3 and C3i */
    dbref C3iNetwork[C3I_NETWORK_SIZE];	/* other mechs in the C3i network */
    int wC3iNetworkSize;	/* Current size of our network */
    dbref C3Network[C3_NETWORK_SIZE];	/* The whole network. We're sacrificing memory for speed. */
    int wC3NetworkSize;		/* Current size of the C3Network */
    int wTotalC3Masters;	/* How many masters are on this mech? */
    int wWorkingC3Masters;	/* How many working masters are on this mech? */
    int C3FreqMode;		/* applies to C3 and C3i */
    dbref tagTarget;		/* dbref of the target we're tagging */
    dbref taggedBy;		/* dbref of the person tagging us */
} mech_sd;

typedef struct {
    XCODE xcode;		/* XCODE base class field */

    char ID[2];			/* Only for internal use */
    char brief;			/* toggle brievity */
    char chantitle[FREQS][CHTITLELEN + 1];	/* Channel titles */
    dbref mynum;		/* My dbref */
    int mapnumber;		/* My number on the map */
    dbref mapindex;		/* 0..MAX_MAPS (dbref of map object) */
    unsigned long tic[NUM_TICS][TICLONGS];	/* tics.. */
    int freq[FREQS];		/* channel frequencies */
    int freqmodes[FREQS];	/* flags for the freq */
    mech_ud ud;			/* UnitData (mostly not bzero'able) */
    mech_pd pd;			/* PositionData(mostly not bzero'able) */
    mech_rd rd;			/* RSdata (mostly bzero'able) */
    mech_sd sd;			/* SpecialsData (mostly not bzero'able) */

} MECH;

struct spot_data {
    float tarFX;
    float tarFY;
    float mechFX;
    float mechFY;
    MECH *target;
};

struct repair_data {
    int delta;
    int time;
    int target;
    int code;
};

/* status element... */

#define LANDED                0x00000001  /* (a) For VTOL use only */
#define TORSO_RIGHT           0x00000002  /* (b) Torso heading -= 60 degrees */
#define TORSO_LEFT            0x00000004  /* (c) Torso heading += 60 degrees */
#define STARTED               0x00000008  /* (d) Mech is warmed up */
#define PARTIAL_COVER         0x00000010  /* (e) */
#define DESTROYED             0x00000020  /* (f) */
#define JUMPING               0x00000040  /* (g) Handled in UPDATE */
#define FALLEN                0x00000080  /* (h) */
#define DFA_ATTACK            0x00000100  /* (i) */
#define PERFORMING_ACTION     0x00000200  /* (j) Set if the unit is performing some sort of action. Controlled by SCode */
#define FLIPPED_ARMS          0x00000400  /* (k) */
#define AMS_ENABLED           0x00000800  /* (l) only settable if mech has ANTI-MISSILE_TECH */
#define EXPLODE_SAFE          0x00001000  /* (m) Used to prevent a unit from doing EXPLODE AMMO */
#define UNCONSCIOUS           0x00002000  /* (n) Pilot is unconscious */
#define TOWED                 0x00004000  /* (o) Someone's towing us */
#define LOCK_TARGET           0x00008000  /* (p) We mean business */
#define LOCK_BUILDING         0x00010000  /* (q) Hit building */
#define LOCK_HEX              0x00020000  /* (r) Hit hex (clear / ignite, d'pend on weapon) */
#define LOCK_HEX_IGN          0x00040000  /* (s) */
#define LOCK_HEX_CLR          0x00080000  /* (t) */
#define MASC_ENABLED          0x00100000  /* (u) Using MASC */
#define BLINDED               0x00200000  /* (v) Pilot has been blinded momentarily by something */
#define COMBAT_SAFE           0x00400000  /* (w) Can't be hurt */
#define AUTOCON_WHEN_SHUTDOWN 0x00800000  /* (x) Autocon sees it even when shutdown */
#define FIRED                 0x01000000  /* (y) Fired at something */
#define SCHARGE_ENABLED       0x02000000  /* (z) */
#define HULLDOWN              0x04000000  /* (A) */
#define UNDERSPECIAL          0x08000000  /* (B) */
#define UNDERGRAVITY          0x10000000  /* (C) */
#define UNDERTEMPERATURE      0x20000000  /* (D) */
#define UNDERVACUUM           0x40000000  /* (E) */
/* UNUSED                     0x80000000     (F) */

#define CONDITIONS	(UNDERSPECIAL | UNDERGRAVITY | UNDERTEMPERATURE | UNDERVACUUM)
#define LOCK_MODES	(LOCK_TARGET|LOCK_BUILDING|LOCK_HEX|LOCK_HEX_IGN|LOCK_HEX_CLR)

/* status2 element */

/* Specials status element */
#define ECM_ENABLED           0x00000001  /* (a) Unit ECM is enabled */
#define ECCM_ENABLED          0x00000002  /* (b) Unit ECCM is enabled */
#define ECM_DISTURBANCE       0x00000004  /* (c) Unit affected by ECM */
#define ECM_PROTECTED         0x00000008  /* (d) Unit protected by ECM */
#define ECM_COUNTERED         0x00000010  /* (e) ECM countered by ECCM.  
                                             This only happens if an enemy ECCM
                                             is within range. */
#define SLITE_ON              0x00000020  /* (f) Unit SLITE is enabled */
#define STH_ARMOR_ON          0x00000040  /* (g) Unit has steath armor enabled */
#define NULLSIGSYS_ON         0x00000080  /* (h) Unit has nullsig enabled */
#define ANGEL_ECM_ENABLED     0x00000100  /* (i) Unit Angel ECM is enabled */
#define ANGEL_ECCM_ENABLED    0x00000200  /* (j) Unit Angel ECCM is enabled */
#define ANGEL_ECM_PROTECTED   0x00000400  /* (k) Unit protected by Angel ECM */
#define ANGEL_ECM_DISTURBED   0x00000800  /* (l) Unit affected by Angel ECM */
#define PER_ECM_ENABLED       0x00001000  /* (m) Unit Personal ECM is enabled */
#define PER_ECCM_ENABLED      0x00002000  /* (n) Unit Personal ECCM is enabled */
#define AUTOTURN_TURRET       0x00004000  /* (o) Unit Auto-Turret enabled to locked target */
/* UNUSED                     0x00008000     (p) */
#define SPRINTING             0x00010000  /* (q) Unit is Sprinting */
#define EVADING               0x00020000  /* (r) Unit is Evading */
#define DODGING               0x00040000  /* (s) Unit is Dodging */
#define ATTACKEMIT_MECH       0x00080000  /* (t) Units attacks sent to MechAttackEmits channel */
#define UNIT_MOUNTED          0x00100000  /* (u) Unit has been mounted by a suit */
#define UNIT_MOUNTING         0x00200000  /* (v) Unit is mounting another unit */
/* UNUSED                     0x00400000     (w) */
/* UNUSED                     0x00800000     (x) */
/* UNUSED                     0x01000000     (y) */
/* UNUSED                     0x02000000     (z) */
/* UNUSED                     0x04000000     (A) */
/* UNUSED                     0x08000000     (B) */
/* UNUSED                     0x10000000     (C) */
/* UNUSED                     0x20000000     (D) */
/* UNUSED                     0x40000000     (E) */
/* UNUSED                     0x80000000     (F) */

/* Grouping masks */
#define MOVE_MODES      (SPRINTING|EVADING|DODGING)
#define MOVE_MODES_LOCK (SPRINTING|EVADING)

/* Flags for mode handling */
#define MODE_EVADE      0x1
#define MODE_SPRINT     0x2
#define MODE_ON         0x4
#define MODE_OFF        0x8
#define MODE_DODGE      0x10
#define MODE_DG_USED    0x20

/* MechFullRecycle check flags */
#define CHECK_WEAPS     0x1
#define CHECK_PHYS      0x2
#define CHECK_BOTH      (CHECK_WEAPS|CHECK_PHYS)

#define MechLockFire(mech) \
((MechStatus(mech) & LOCK_TARGET) && \
 !(MechStatus(mech) & (LOCK_BUILDING|LOCK_HEX|LOCK_HEX_IGN|LOCK_HEX_CLR)))

/* Macros for accessing some parts */
#define Blinded(a)	(MechStatus(a) & BLINDED)
#define Started(a)	(MechStatus(a) & STARTED)
#define Uncon(a)	(MechStatus(a) & UNCONSCIOUS)

/* critstatus element */
#define GYRO_DESTROYED		0x00000001  /* (a) */
#define SENSORS_DAMAGED		0x00000002  /* (b) */
#define TAG_DESTROYED		0x00000004  /* (c) */
#define HIDDEN			0x00000008  /* (d) */
#define GYRO_DAMAGED		0x00000010  /* (e) */
#define HIP_DAMAGED		0x00000020  /* (f) */
#define LIFE_SUPPORT_DESTROYED	0x00000040  /* (g) */
#define ANGEL_ECM_DESTROYED	0x00000080  /* (h) */
#define C3I_DESTROYED		0x00000100  /* (i) */
#define NSS_DESTROYED		0x00000200  /* (j) */
#define SLITE_DEST		0x00000400  /* (k) */
#define SLITE_LIT		0x00000800  /* (l) */
#define LOAD_OK			0x00001000  /* (m) Carried load recalculated */
#define OWEIGHT_OK		0x00002000  /* (n) Own weight recalculated */
#define SPEED_OK		0x00004000  /* (o) Total speed recalculated */
#define HEATCUTOFF		0x00008000  /* (p) */
#define TOWABLE			0x00010000  /* (q) */
#define HIP_DESTROYED		0x00020000  /* (r) */
#define TC_DESTROYED		0x00040000  /* (s) */
#define C3_DESTROYED		0x00080000  /* (t) */
#define ECM_DESTROYED		0x00100000  /* (u) */
#define BEAGLE_DESTROYED	0x00200000  /* (v) */
#define JELLIED			0x00400000  /* (w) Got inferno gel on us */
#define PC_INITIALIZED		0x00800000  /* (x) PC-initialization done already */
#define SPINNING		0x01000000  /* (y) */
#define CLAIRVOYANT		0x02000000  /* (z) See everything, regardless of blocked */
#define INVISIBLE		0x04000000  /* (A) Unable to be seen by anyone */
#define CHEAD			0x08000000  /* (B) Altered heading */
#define OBSERVATORIC		0x10000000  /* (C) */
#define BLOODHOUND_DESTROYED	0x20000000  /* (D) */
#define MECH_STUNNED            0x40000000  /* (E) Is the mech stunned (Exile stun code) */

/* tankcritstatus element */
#define TURRET_LOCKED		0x01
#define TURRET_JAMMED		0x02	/* can be fixed by player */
#define DUG_IN			0x04
#define DIGGING_IN		0x08
#define CREW_STUNNED		0x10	/* can't go over cruise speed, make any attacks at all, use radio. IE, can't do jack but turn */
#define TAIL_ROTOR_DESTROYED 	0x20

/* specials element: used to tell quickly what type of tech the mech has */
#define TRIPLE_MYOMER_TECH	0x01
#define CL_ANTI_MISSILE_TECH	0x02
#define IS_ANTI_MISSILE_TECH	0x04
#define DOUBLE_HEAT_TECH	0x08
#define MASC_TECH		0x10
#define CLAN_TECH		0x20
#define FLIPABLE_ARMS		0x40
#define C3_MASTER_TECH		0x80
#define C3_SLAVE_TECH		0x100
#define ARTEMIS_IV_TECH		0x200
#define ECM_TECH		0x400
#define BEAGLE_PROBE_TECH	0x800
#define SALVAGE_TECH		0x1000	/* 2x 'mech carrying capacity */
#define CARGO_TECH		0x2000	/* 2x cargo carrying capacity */
#define SLITE_TECH		0x4000
#define LOADER_TECH		0x8000
#define AA_TECH			0x10000
#define NS_TECH			0x20000
#define SS_ABILITY		0x40000	/* Has sixth sense */
#define FF_TECH			0x80000	/* Has ferro-fib. armor */
#define ES_TECH			0x100000	/* Has endo-steel internals */
#define XL_TECH			0x200000
#define ICE_TECH		0x400000	/* ICE engine */
#define LIFTER_TECH		0x800000
#define LE_TECH			0x1000000	/* Light engine */
#define XXL_TECH		0x2000000
#define CE_TECH			0x4000000
#define REINFI_TECH		0x8000000
#define COMPI_TECH		0x10000000
#define HARDA_TECH		0x20000000
#define CRITPROOF_TECH		0x40000000
/* 0x80000000 can not be used. */

/*critstatus2 element */
#define HDGYRO_DAMAGED          0x01        /* HDGYRO is damaged */

/* specials2 element: used to tell quickly what type of tech the mech has */
#define STEALTH_ARMOR_TECH      0x01        /* Stealth armor */
#define HVY_FF_ARMOR_TECH       0x02        /* Heavy FF. 1.24 armor multi. 21 slots. */
#define LASER_REF_ARMOR_TECH    0x04        /* Not yet implemented */
#define REACTIVE_ARMOR_TECH     0x08        /* Not yet implemented */
#define NULLSIGSYS_TECH         0x10        /* Null signature system */
#define C3I_TECH                0x20        /* Improved C3 */
#define SUPERCHARGER_TECH       0x40        /* Not yet implemented */
#define IMPROVED_JJ_TECH        0x80        /* Not yet implemented */
#define MECHANICAL_JJ_TECH      0x100       /* Not yet implemented */
#define COMPACT_HS_TECH         0x200       /* Not yet implemented */
#define LASER_HS_TECH           0x400       /* Not yet implemented */
#define BLOODHOUND_PROBE_TECH   0x800       /* BLoodhound Active Probe */
#define ANGEL_ECM_TECH          0x1000      /* Angel ECM suite */
#define WATCHDOG_TECH           0x2000      /* Not yet implemented */
#define LT_FF_ARMOR_TECH        0x4000      /* Heavy FF. 1.06 armor multi. 7 slots. */
#define TAG_TECH                0x8000      /* Target Aquisition Gear */
#define OMNIMECH_TECH           0x10000     /* Is an omni mech */
#define ARTEMISV_TECH           0x20000     /* Not yet implemented */
#define CAMO_TECH               0x40000     /* Allows any unit to 'hide' */
#define CARRIER_TECH            0x80000     /* Can be used as a carrier of mechs */
#define WATERPROOF_TECH         0x100000    /* Can the unit go underwater without problems 
                                               for use with tanks */
#define XLGYRO_TECH             0x200000
#define HDGYRO_TECH             0x400000
#define CGYRO_TECH              0x800000
#define TCOMP_TECH		0x1000000

/* Infantry specials */
#define INF_SWARM_TECH			0x01	/* Infantry/BSuits can swarm unfriendlies */
#define INF_MOUNT_TECH			0x02	/* Infantry/BSuits can mount friendlies */
#define INF_ANTILEG_TECH		0x04	/* Infantry/BSuits can make anti-leg attacks */
#define CS_PURIFIER_STEALTH_TECH	0x08	/* CS Purifier stealth technology. +3 at 0MP, +2 at 1MP, +1 at 2MP, +3 at 3+MP. */
#define DC_KAGE_STEALTH_TECH		0x10	/* DC stealth technology. +3 at med, +6 at long. No BAP. */
#define FWL_ACHILEUS_STEALTH_TECH	0x20	/* FWL stealth technology. +1 at short, +4 at medium, +7 at long. No BAP. */
#define FC_INFILTRATOR_STEALTH_TECH	0x40	/* FC stealth technology. +3 at med, +6 at long. No BAP. */
#define FC_INFILTRATORII_STEALTH_TECH	0x80	/* FC stealth II technology. +1 at short, +3 at med, +6 at long. No BAP. ECM in same hex. */
#define MUST_JETTISON_TECH		0x100	/* Special considerations for some suits. Must jettison backpack before they can jump/swarm/anti-leg */
#define CAN_JETTISON_TECH		0x200	/* Whether or not the unit can jettison its backpack */

#define STEALTH_TECH (CS_PURIFIER_STEALTH_TECH|DC_KAGE_STEALTH_TECH|FWL_ACHILEUS_STEALTH_TECH|FC_INFILTRATOR_STEALTH_TECH|FC_INFILTRATORII_STEALTH_TECH)

/* TargComp types */
#define TARGCOMP_NORMAL 0
#define TARGCOMP_SHORT  1
#define TARGCOMP_LONG   2
#define TARGCOMP_MULTI  3
#define TARGCOMP_AA     4

/*
	Notes on unimplemented stuff:
 
	- Laser Reflective Armor:
		- made for better protection against energy weapons
		- 10 crits for IS, 5 for Clan
		- 1 vehicle items for IS, 1 for Clan
		- 16 pts/ton
		- Absorbs 2 pts of damager per pt of armor from energy weapon attacks.
			- Single pt attacks still use up 1 pt of armor.
		- Due to brittleness, double damage from physical and falling
		- Does not apply to unarmored sections
 
	- Reactive Armor:
		- made for better protection against missile attacks
		- 14 crits for IS, 7 for Clan
		- 2 vehicle items for IS, 1 for Clan
		- 16 pts/ton
		- Can not be mounted on OmniMechs
		- Absorbs 2 pts of damager per pt of armor from missile weapon attacks.
			- Single pt attacks still use up 1 pt of armor.
		- Critical hits on reactive armor slots should be re-rolled, but also
			roll 2d6. On roll of 2 chain reaction occurs and destroys all remaining
			armor on section (front and back) and cases 1 point of internal damage with
			normal chance of critical hit.
		- Does not apply to unarmored sections

	- Improved JJs
		- Weigh twice as much as std JJs
		-	Take up twice as many crits slots as std JJs
		- Can jump as high as running movement
		- Heat produced by jumping is 1 pt per 2 hexes
			- Minimum of 3 heat

	- Mechanical Jump Boosters (Mattress Springs Of Doom)
		- Water does not affect them -- can jump out of water
		- System takes up all crits slots in all legs
		- Any critical hit disabled whole system
		- Weighs 5 percent of mech's tonnage * jumping range
		- Range not limited by walking/running movement
		- Mech can not turn while jumping
		- Can not DFA
		- Can mount std JJs and mechanical, but can not use both at the same time

	- Compact HS
		- Weighs 1.5 tons
		- Can fit 2 in each crit slot
		- Still gets 10 'free heatsinks'
		- Number of HS 'in engine' is doubled
		- Critical hit destroys both HS
		- IS only
		- Can not be used by vehicles

	- Laser HS
		- Not affected by water
		- Act as DHS for heat dissipation
		- Add 1 to ammo explosion roll
		- At night/dusk
			- If mech takes any action to generate heat, subtract 1 from
				night/dusk modifier
			- If mech overheats, remove night/dusk modifier
		- Can not be used by vehicles
		- Clan only

	- Watchdog System
		- Clan only
		- Acts as ECM suite and active probe
		- 1 ton, 1 crit

	- Artemis V
		- Clan only
		- Just like ArtemisIV
		- Weighs 1.5 tons and takes up 2 crit slots
		- Not compatible with ArtemisIV
		- -1 BTH
		- Add 3 to missile hit roll
		- Blocked by ECM

	- iNarc Nemesis pods
		- If friendly mech fires SemiGuided or NARC missiles...
		- If Nemesis'd unit between firer and target, resolve attack
		  as if it was fired at the Nemesis'd unit. Add +1 BTH. If attack misses
		  missiles continue on to normal target.
		- If multiple Nemesis'd units are between firer and target, resolve
		  attack on each in succession from closest unit until all shots hit
		  or attack continues on to real target
 */

/* Status stuff for common_checks function */
#define MECH_STARTED		0x1
#define MECH_PILOT		0x2
#define MECH_PILOT_CON		0x4
#define MECH_MAP		0x8
#define MECH_CONSISTENT		0x10
#define MECH_PILOTONLY		0x20
#define MECH_USUAL		(MECH_CONSISTENT|MECH_MAP|MECH_PILOT_CON|MECH_PILOT|MECH_STARTED)
#define MECH_USUALS		(MECH_CONSISTENT|MECH_MAP|MECH_PILOT_CON|MECH_PILOT)
#define MECH_USUALSP		(MECH_CONSISTENT|MECH_MAP|MECH_PILOT_CON)
#define MECH_USUALSM		(MECH_CONSISTENT|MECH_PILOT_CON|MECH_PILOT)
#define MECH_USUALM		(MECH_CONSISTENT|MECH_PILOT_CON|MECH_PILOT|MECH_STARTED)
#define MECH_USUALO		(MECH_CONSISTENT|MECH_MAP|MECH_PILOT_CON|MECH_PILOT|MECH_STARTED|MECH_PILOTONLY)
#define MECH_USUALSO		(MECH_CONSISTENT|MECH_MAP|MECH_PILOT_CON|MECH_PILOT|MECH_PILOTONLY)
#define MECH_USUALSPO		(MECH_CONSISTENT|MECH_MAP|MECH_PILOT_CON|MECH_PILOTONLY)
#define MECH_USUALSMO		(MECH_CONSISTENT|MECH_PILOT_CON|MECH_PILOT|MECH_PILOTONLY)
#define MECH_USUALMO		(MECH_CONSISTENT|MECH_PILOT_CON|MECH_PILOT|MECH_STARTED|MECH_PILOTONLY)

extern struct weapon_struct MechWeapons[];
extern struct missile_hit_table_struct MissileHitTable[];

#define TELE_ALL	1	/* Tele all, not just mortals */
#define TELE_SLAVE	2	/* Make slaves in progress (not of wizzes, though) */
#define TELE_LOUD	4	/* Loudly teleport */
#define TELE_XP		8	/* Lose 1/3 XP */

#define MINE_STEP	1	/* Someone steps to a hex */
#define MINE_LAND	2	/* Someone lands in a hex */
#define MINE_FALL	3	/* Someone falls in the hex */
#define MINE_DROP	4	/* Someone drops to ground in the hex */

extern void *FindObjectsData(dbref key);
#ifndef ECMD
#define ECMD(a) extern void a (dbref player, void *data, char *buffer)
#endif

#define destroy_object(obj)	destroy_thing(obj)
#define create_object(name)	create_obj(GOD, TYPE_THING, name, 1)

#define A_MECHREF A_MECHTYPE
#define MECH_PATH mudconf.mech_db
#define MAP_PATH mudconf.map_db

#define WSDUMP_MASK_ER	"%-24s %2d     %2d           %2d  %2d    %2d  %3d  %3d %2d"
#define WSDUMP_MASK_NOER "%-24s %2d     %2d           %2d    %2d     %2d  %3d   %2d"
#define WSDUMP_MASKS_ER	"%%cgWeapon Name             Heat  Damage  Range: Min Short Med Long Ext VRT"
#define WSDUMP_MASKS_NOER "%%cgWeapon Name             Heat  Damage  Range: Min  Short  Med  Long  VRT"

#define WDUMP_MASK	"%-24s %2d     %2d           %2d  %2d    %2d  %3d  %2d  %2d %d"
#define WDUMP_MASKS	"%%cgWeapon Name             Heat  Damage  Range: Min Short Med Long VRT C  ApT"
#include "btmacros.h"
#include "p.glue.hcode.h"
#include "map.h"
#include "map.coding.h"
#include "p.glue.h"
#include "mech.notify.h"

/* For mech.lostracer.c's TraceLOS() */
typedef struct {
    int x;
    int y;
} lostrace_info;

#endif				/* MECH_H */
