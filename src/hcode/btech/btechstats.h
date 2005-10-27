
/*
 * $Id: btechstats.h,v 1.1.1.1 2005/01/11 21:18:03 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 *
 * Last modified: Mon Jul 13 11:10:38 1998 fingon
 *
 */

/* Function declarations / skill list for btechstats.c */

#ifndef BTECHSTATS_H
#define BTECHSTATS_H

#include "db.h"
#include "externs.h"
#include "interface.h"
#include "config.h"
#include "powers.h"
#include "btechstats_global.h"

#ifdef BTECHSTATS_C
char *btech_charvaluetype_names[] = {
    "Char_value",
    "Char_skill",
    "Char_advantage",
    "Char_attribute"
};

char *btech_charskillflag_names[] = {
    "Athletic",
    "Mental",
    "Physical",
    "Social"
};

#endif

#define EE_NUMBER 11

#ifdef BTECHSTATS

/* *INDENT-OFF* */

struct char_value {
    char *name;
    char type;
    int flag;
    int xpthreshold;
} char_values[] = {

    {"XP", CHAR_VALUE, 0, 0},
    {"MaxXP", CHAR_VALUE, 0, 0},
    {"Type", CHAR_VALUE, 0, 0},
    {"Level", CHAR_VALUE, 0, 0},
    {"Package", CHAR_VALUE, 0, 0},
    {"Lives", CHAR_VALUE, 0, 0},
    {"Bruise", CHAR_VALUE, 0, 0},
    {"Lethal", CHAR_VALUE, 0, 0},
    {"Unused1", CHAR_VALUE, 0, 0},

/* Advantages */
    {"Ambidextrous", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},
    {"Bloodname", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},
    {"Combat_Sense", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},
    {"Contact", CHAR_ADVANTAGE, CHAR_ADV_VALUE, 0},
    {"Dropship", CHAR_ADVANTAGE, CHAR_ADV_VALUE, 0},
    {"EI_Implant", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},
    {"Exceptional_Attribute", CHAR_ADVANTAGE, CHAR_ADV_EXCEPT, 0},
    {"Extra_Edge", CHAR_ADVANTAGE, CHAR_ADV_VALUE, 0},
    {"Land_Grant", CHAR_ADVANTAGE, CHAR_ADV_VALUE, 0},
    {"Reputation", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},
    {"Sixth_Sense", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},
    {"Title", CHAR_ADVANTAGE, CHAR_ADV_VALUE, 0},
    {"Toughness", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},
    {"Wealth", CHAR_ADVANTAGE, CHAR_ADV_VALUE, 0},
    {"Well-Connected", CHAR_ADVANTAGE, CHAR_ADV_VALUE, 0},
    {"Well_Equipped", CHAR_ADVANTAGE, CHAR_ADV_VALUE, 0},
    {"Dodge_Maneuver", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},
    {"Maneuvering_Ace", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},
    {"Melee_Specialist", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},
    {"Pain_Resistance", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},
    {"Speed_Demon", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},
    {"Tech_Aptitude", CHAR_ADVANTAGE, CHAR_ADV_BOOL, 0},

/* Attributes */
    {"Build", CHAR_ATTRIBUTE, 0, 0},
    {"Reflexes", CHAR_ATTRIBUTE, 0, 0},
    {"Intuition", CHAR_ATTRIBUTE, 0, 0},
    {"Learn", CHAR_ATTRIBUTE, 0, 0},
    {"Charisma", CHAR_ATTRIBUTE, 0, 0},

/* Skills themselves */
    {"Acrobatics", CHAR_SKILL, CHAR_ATHLETIC, 50},
    {"Administration", CHAR_SKILL, CHAR_MENTAL, 50},
    {"Alternate_Identity", CHAR_SKILL, CHAR_MENTAL, 50},
    {"Appraisal", CHAR_SKILL, CHAR_MENTAL, 50},
    {"Archery", CHAR_SKILL, CHAR_ATHLETIC, 50},
    {"Blade", CHAR_SKILL, CHAR_ATHLETIC | CAREER_MISC, 50},
    {"Bureaucracy", CHAR_SKILL, CHAR_SOCIAL | CAREER_MISC, 50},
    {"Climbing", CHAR_SKILL, CHAR_ATHLETIC, 50},
    {"Comm-Conventional", CHAR_SKILL, CHAR_MENTAL | CAREER_TECH, 150},
    {"Comm-Hyperpulse", CHAR_SKILL, CHAR_MENTAL | CAREER_TECH, 50},
    {"Computer", CHAR_SKILL, CHAR_MENTAL | CAREER_TECH, 50},
    {"Cryptography", CHAR_SKILL, CHAR_MENTAL | CAREER_TECH, 50},
    {"Demolitions", CHAR_SKILL, CHAR_MENTAL, 50},
    {"Disguise", CHAR_SKILL, CHAR_MENTAL | CAREER_RECON, 50},
#ifndef BT_EXILE_MW3STATS
    {"Drive", CHAR_SKILL, CHAR_PHYSICAL | CAREER_CAVALRY, 3000},
    {"Drive-Naval", CHAR_SKILL, CHAR_PHYSICAL, 3000},
#endif
    {"Engineering", CHAR_SKILL, CHAR_MENTAL | CAREER_TECH, 50},
    {"Escape_Artist", CHAR_SKILL, CHAR_PHYSICAL | CAREER_RECON, 50},
    {"Forgery", CHAR_SKILL, CHAR_MENTAL, 50},
    {"Gambling", CHAR_SKILL, CHAR_MENTAL, 50},
#ifndef BT_EXILE_MW3STATS
    {"Gunnery-Aerospace", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_AERO, 1000},
    {"Gunnery-Artillery", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_ARTILLERY, 500},
    {"Gunnery-Battlemech", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_BMECH, 3000},
    {"Gunnery-BSuit", CHAR_SKILL, SK_XP | CHAR_PHYSICAL, 500},
    {"Gunnery-Conventional", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_CAVALRY, 3000},
    {"Gunnery-Spacecraft", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_DROPSHIP, 50},
    {"Gunnery-Spotting", CHAR_SKILL, CHAR_PHYSICAL | CAREER_ARTILLERY, 50},
#else
    {"Gunnery-Artillery", CHAR_SKILL, SK_XP | CHAR_PHYSICAL, 500},
    {"Gunnery-Ballistic", CHAR_SKILL, SK_XP | CHAR_PHYSICAL, 2500},
    {"Gunnery-Flamer", CHAR_SKILL, SK_XP | CHAR_PHYSICAL, 500},
    {"Gunnery-Laser", CHAR_SKILL, SK_XP | CHAR_PHYSICAL, 2500},
    {"Gunnery-Missile", CHAR_SKILL, SK_XP | CHAR_PHYSICAL, 2500},
    {"Gunnery-Spotting", CHAR_SKILL, SK_XP | CHAR_PHYSICAL, 250},
#endif
    {"Impersonation", CHAR_SKILL, CHAR_SOCIAL, 50},
    {"Interrogation", CHAR_SKILL, CHAR_SOCIAL | CAREER_RECON, 50},
    {"Jump_Pack", CHAR_SKILL, CHAR_ATHLETIC, 50},
    {"Leadership", CHAR_SKILL, CHAR_SOCIAL | CAREER_ACADMISC, 50},
    {"Medtech", CHAR_SKILL, CHAR_MENTAL | CAREER_MISC, 300},
    {"Navigation", CHAR_SKILL, CHAR_MENTAL, 25},
    {"Negotiation", CHAR_SKILL, CHAR_SOCIAL, 25},
    {"Perception", CHAR_SKILL, CHAR_MENTAL | CAREER_RECON, 150},
#ifndef BT_EXILE_MW3STATS
    {"Piloting-Aerospace", CHAR_SKILL, CHAR_PHYSICAL | CAREER_AERO, 2500},
    {"Piloting-Battlemech", CHAR_SKILL, CHAR_PHYSICAL | CAREER_BMECH, 3000},
    {"Piloting-Battlesuit", CHAR_SKILL, CHAR_ATHLETIC, 3000},
    {"Piloting-BSuit", CHAR_SKILL, CHAR_PHYSICAL, 3000},
    {"Piloting-Spacecraft", CHAR_SKILL, CHAR_PHYSICAL | CAREER_DROPSHIP, 50},
#else
    {"Piloting-Aerospace", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_AERO, 3000},
    {"Piloting-Biped", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_BMECH, 3000},
    {"Piloting-BSuit", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_CAVALRY, 3000},
    {"Piloting-Hover", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_CAVALRY, 3000},
    {"Piloting-Naval", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_CAVALRY, 3000},
    {"Piloting-Quad", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_BMECH, 3000},
    {"Piloting-Spacecraft", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_DROPSHIP, 3000},
    {"Piloting-Tracked", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_CAVALRY, 3000},
    {"Piloting-Wheeled", CHAR_SKILL, SK_XP | CHAR_PHYSICAL | CAREER_CAVALRY, 3000},
#endif
    {"Protocol", CHAR_SKILL, CHAR_SOCIAL, 50},
    {"Quickdraw", CHAR_SKILL, CHAR_PHYSICAL, 50},
#ifdef BT_EXILE_SKILLS
    {"Research", CHAR_SKILL, CHAR_MENTAL | CAREER_TECH, 100},
#endif
    {"Running", CHAR_SKILL, SK_XP | CHAR_ATHLETIC, 100},
    {"Riding", CHAR_SKILL, CHAR_ATHLETIC, 50},
    {"Scrounge", CHAR_SKILL, CHAR_SOCIAL | CAREER_TECH, 50},
    {"Security_Systems", CHAR_SKILL, CHAR_MENTAL | CAREER_RECON, 50},
    {"Seduction", CHAR_SKILL, CHAR_SOCIAL, 50},
    {"Small_Arms", CHAR_SKILL, CHAR_PHYSICAL | CAREER_MISC, 50},
    {"Stealth", CHAR_SKILL, CHAR_PHYSICAL | CAREER_RECON, 50},
    {"Strategy", CHAR_SKILL, CHAR_MENTAL | CAREER_ACADMISC, 50},
    {"Streetwise", CHAR_SKILL, CHAR_SOCIAL, 50},
    {"Support_Weapons", CHAR_SKILL, CHAR_PHYSICAL | CAREER_MISC, 50},
    {"Survival", CHAR_SKILL, CHAR_MENTAL, 50},
    {"Swimming", CHAR_SKILL, CHAR_ATHLETIC, 50},
    {"Tactics", CHAR_SKILL, CHAR_MENTAL | CAREER_ACADMISC, 50},
    {"Technician-Aerospace", CHAR_SKILL, SK_XP | CHAR_MENTAL | CAREER_TECHVEH, 50},
    {"Technician-Battlemech", CHAR_SKILL, SK_XP | CHAR_MENTAL | CAREER_TECHMECH, 600},
    {"Technician-Battlesuit", CHAR_SKILL, SK_XP | CHAR_MENTAL, 300},
    {"Technician-Electronics", CHAR_SKILL, SK_XP | CHAR_MENTAL | CAREER_TECH, 50},
    {"Technician-Mechanic", CHAR_SKILL, SK_XP | CHAR_MENTAL | CAREER_TECHVEH, 400},
    {"Technician-Weapons", CHAR_SKILL, SK_XP | CHAR_MENTAL | CAREER_TECH, 300},
    {"Technician-Spacecraft", CHAR_SKILL, SK_XP | CHAR_MENTAL, 300},
    {"Throwing_Weapons", CHAR_SKILL, CHAR_PHYSICAL, 50},
    {"Tinker", CHAR_SKILL, CHAR_MENTAL | CAREER_TECH, 50},
    {"Tracking", CHAR_SKILL, CHAR_MENTAL | CAREER_RECON, 50},
    {"Training", CHAR_SKILL, CHAR_SOCIAL, 50},
    {"Unarmed_Combat", CHAR_SKILL, CHAR_ATHLETIC | CAREER_MISC, 50},
    {"Zero-G_Operations", CHAR_SKILL, CHAR_PHYSICAL, 50},
};

/* *INDENT-ON* */

#define NUM_CHARVALUES sizeof(char_values)/sizeof(struct char_value)

char *char_values_short[NUM_CHARVALUES];

/*************************************************************************/

char *char_levels[] = {
    "Green",
    "Regular",
    "Veteran",
    "Elite",
    "Historical"
};

#define NUM_CHARLEVELS 5

char *char_types[] = {
    "Inner_Sphere",
    "Clan_MechWarrior",
    "Clan_Aerospace",
    "Clan_Elemental",
    "Clan_Freebirth",
    "Clan_Other"
};

#define NUM_CHARTYPES 6

char *char_packages[] = {
    "None",
    "Primary_Clan_Warrior",
    "Secondary_Clan_Warrior",
    "Secondar_Clan_Pilot",
    "Clan_Elemental",
    "Basic_Academy",
    "Advanced_Academy",
    "Basic_University",
    "Advanced_University"
};


#define NUM_CHARPACKAGES 9

/* 
    XP is added only if the player is online AND
    the skill is marked SK_XP OR the last xp-gain is 30 seconds or more ago.
 */

typedef struct {
    dbref dbref;
    unsigned char values[NUM_CHARVALUES];
    time_t last_use[NUM_CHARVALUES];
    int xp[NUM_CHARVALUES];
} PSTATS;

#endif

#include "p.btechstats.h"

#endif				/* BTECHSTATS_H */
