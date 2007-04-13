/*
 * $Id: hudinfo.h,v 1.1.1.1 2005/01/11 21:18:07 kstevens Exp $
 *
 * Copyright (c) 2002 Thomas Wouters <thomas@xs4all.net>
 *
 * HUDINFO support.
 */

#define HUD_PROTO_VERSION "0.8"

static void hud_generalstatus(DESC *, MECH *, char *, char *);
static void hud_weapons(DESC *, MECH *, char *, char *);
static void hud_weaponlist(DESC *, MECH *, char *, char *);
static void hud_limbstatus(DESC *, MECH *, char *, char *);
static void hud_ammostatus(DESC *, MECH *, char *, char *);
static void hud_templateinfo(DESC *, MECH *, char *, char *);
static void hud_templatearmor(DESC *, MECH *, char *, char *);
static void hud_armorstatus(DESC *, MECH *, char *, char *);
static void hud_contacts(DESC *, MECH *, char *, char *);
static void hud_building_contacts(DESC *, MECH *, char *, char *);
static void hud_armorscan(DESC *, MECH *, char *, char *);
static void hud_weapscan(DESC *, MECH *, char *, char *);
static void hud_tactical(DESC *, MECH *, char *, char *);
static void hud_conditions(DESC *, MECH *, char *, char *);

typedef struct hudinfo_command_struct HUDCMD;

#define HUDCMD_HASARG	0x00001
#define HUDCMD_NEEDMECH	0x00002
#define HUDCMD_STARTED	0x00004
#define HUDCMD_NONDEST	0x00008
#define HUDCMD_AWAKE	0x00016

#define HUDCMD_ACTIVE	(HUDCMD_NEEDMECH | HUDCMD_STARTED | HUDCMD_NONDEST \
			 | HUDCMD_AWAKE)
			
#define HUDCMD_PASSIVE	(HUDCMD_NEEDMECH)

#define HUDCMD_INFO	(0)

struct hudinfo_command_struct {
    char *cmd;
    char *msgclass;
    int flag;
    void (*handler)(DESC *, MECH *, char *, char *);
} hudinfo_cmds[] = {
    { "gs", "GS", HUDCMD_PASSIVE, hud_generalstatus },
    { "we", "WE", HUDCMD_PASSIVE, hud_weapons },
    { "wl", "WL", HUDCMD_INFO, hud_weaponlist },
    { "li", "LI", HUDCMD_PASSIVE, hud_limbstatus },
    { "am", "AM", HUDCMD_PASSIVE, hud_ammostatus },
    { "sgi", "SGI", HUDCMD_PASSIVE, hud_templateinfo },
    { "oas", "OAS", HUDCMD_PASSIVE, hud_templatearmor },
    { "as", "AS", HUDCMD_PASSIVE, hud_armorstatus },
    { "c", "C", HUDCMD_ACTIVE, hud_contacts },
    { "cb", "CB", HUDCMD_ACTIVE, hud_building_contacts },
    { "asc", "ASC", HUDCMD_ACTIVE | HUDCMD_HASARG, hud_armorscan },
    { "wsc", "WSC", HUDCMD_ACTIVE | HUDCMD_HASARG, hud_weapscan },
    { "t", "T", HUDCMD_ACTIVE | HUDCMD_HASARG, hud_tactical },
    { "con", "CON", HUDCMD_ACTIVE, hud_conditions },
    { "co", "CO", HUDCMD_ACTIVE, hud_conditions },
    { NULL, NULL, 0, NULL },
};

extern const int num_def_weapons;
extern struct weapon_struct MechWeapons[];
