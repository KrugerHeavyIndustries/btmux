
/*
 * $Id: glue.h,v 1.4 2005/08/08 09:43:10 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 *
 * Created: Thu Sep 19 22:02:48 1996 fingon
 * Last modified: Thu Dec 10 21:45:10 1998 fingon
 *
 */

/*
  Header for special command rooms...
  Based on the original by MUSE folks
*/

#include "config.h"

/* Parameter to the save/load function */
#ifndef _GLUE_H
#define _GLUE_H

#define VERIFY 0
#define SAVE 1
#define LOAD 2

#define XCODE_VERSION 2

#define SPECIAL_FREE 0
#define SPECIAL_ALLOC 1

#define GFLAG_ALL 0
#define GFLAG_MECH 1
#define GFLAG_GROUNDVEH 2
#define GFLAG_AERO 4
#define GFLAG_DS 8
#define GFLAG_VTOL 16
#define GFLAG_NAVAL 32
#define GFLAG_BSUIT 64
#define GFLAG_MW 128

#include "glue_types.h"

#define Have_MechPower(a,b) (((Powers2((Owner(a))) & (b)) || Wizard(Owner(a))) && Inherits((a)))

typedef struct CommandsStruct {
    int flag;
    char *name;
    char *helpmsg;
    void (*func) ();
} CommandsStruct;

typedef struct SpecialObjectStruct {
    char *type;			/* Type of the object */
    CommandsStruct *commands;	/* Commands array */
    long datasize;		/* Size of private buffer */
    void (*allocfreefunc) ();
    int updateTime;		/* Amount of time between updates */
    /* (secs) */
    void (*updatefunc) ();	/* called for every */
    /* object at every */
    /* update */
    int power_needed;		/* WHat power is needed to do */
    /* restricted commands */
} SpecialObjectStruct;

#ifdef _GLUE_C

#include "p.mech.move.h"
#include "p.debug.h"
#include "turret.h"
#include "p.aero.move.h"
#include "p.mech.maps.h"
#include "p.ds.bay.h"
#include "p.mech.notify.h"
#include "p.mech.utils.h"
#include "p.mech.combat.h"
#include "p.mech.update.h"
#include "p.mechrep.h"
#include "p.mech.restrict.h"
#include "p.mech.advanced.h"
#include "p.mech.tic.h"
#include "p.ds.turret.h"
#include "p.mech.contacts.h"
#include "p.mech.status.h"
#include "p.mech.scan.h"
#include "p.mech.sensor.h"
#include "p.map.h"
#include "p.mech.pickup.h"
#include "p.eject.h"
#include "p.mech.c3.h"
#include "p.bsuit.h"
#include "p.mech.startup.h"
#include "p.mech.consistency.h"
#include "p.mech.physical.h"
#include "mech.tech.h"
#include "p.mech.tech.repairs.h"
#include "p.glue.scode.h"
#include "mechrep.h"
#include "p.mine.h"
#include "mech.custom.h"
#include "p.mech.custom.h"
#include "scen.h"
#include "p.btechstats.h"
#include "autopilot.h"
#include "p.events.h"
#include "p.mech.tag.h"
#include "p.mech.c3i.h"
#include "p.mech.fire.h"
#include "p.mech.enhanced.criticals.h"
#include "p.mech.spot.h"
#include "p.mech.ammodump.h"
#include "p.mech.damage.h"

void newturret(dbref, void **, int);
void newfreemech(dbref, void **, int);

ECMD(f_mapblock_set);
ECMD(f_mapblock_setxy);
ECMD(ListForms);
ECMD(initiate_ood);
ECMD(mech_Raddstuff);
ECMD(mech_Rfixstuff);
ECMD(mech_Rremovestuff);
ECMD(mech_Rresetstuff);
ECMD(mech_bomb);
ECMD(mech_loadcargo);
ECMD(mech_losemit);
ECMD(mech_manifest);
ECMD(mech_stores);
ECMD(mech_domystuff);
ECMD(mech_unloadcargo);
ECMD(tech_magic);
ECMD(mech_inferno);
ECMD(mech_swarm);
ECMD(mech_swarm1);
ECMD(mech_dig);
ECMD(mech_vector);

ECMD(f_map_loadmap);

ECMD(f_draw);
ECMD(f_sheath);
ECMD(f_hold);
ECMD(f_put);

ECMD(f_shout);
ECMD(f_emote);
ECMD(f_say);
ECMD(f_whisper);

/* Flag: 0 = all, 1=mech, 2=groundveh, 4=aero, 8=ds, 16=vtol */

/* Categories:
   - Movement
   - Radio
   - Weapons
   - Physical
   - Status
   - Navigation
   - Repairing
   - Special
   - Information
   - TICs
   */


#define SHEADER(a,b) \
{ a, b, b, NULL }
#define HEADER(a) SHEADER(0,a)

CommandsStruct mechcommands[] = {
    /* Movement */
    HEADER("Movement"),

    {0, "HEADING [num]",
	    "Shows/Changes your heading to <NUM> (<NUM> in degrees)",
	mech_heading},
    {0, "SPEED [num | walk | run | stop | back | flank | cruise]",
	    "Without arguments shows your present speed, otherwise changes your speed to <NUM> or the specified speed (run/cruise = 1x maxspeed, walk/flank = 2/3x maxspeed, stop = 0, back = -2/3x maxspeed)",
	mech_speed},
    {48, "VERTICAL [num]", "Shows/Changes your vertical speed to <NUM>.",
	mech_vertical},

    {12, "CLIMB [angle]", "Shows/Changes the climbing angle to <NUM>.",
	aero_climb},
    {12, "DIVE [angle]", "Shows/Changes the diving angle to <NUM>.",
	aero_dive},
    {12, "THRUST [num]", "Shows/Changes the thrust to <NUM>.",
	aero_thrust},
    {1, "LATERAL [fl|fr|rl|rr|-]",
	    "Change your lateral movement mode (quad only). fl/fr/rl/rr = Directions, - = Disable lateral movement.",
	mech_lateral},
    {129, "STAND", "Stand up after a fall or dropping prone.", mech_stand},
    {1, "PRONE", "Force your 'mech to drop prone where it is.", mech_drop},
    {1, "THRASH", "Thrash around and try to kill nearby battle suits.",
	mech_thrash},
    {65, "JUMP [<TARGET-ID> | <BEARING> <RANGE>]",
	    "Jump on default target / given target / bearing + range.",
	mech_jump},
    {1, "HULLDOWN [- | STOP]",
	    "ALlows a QUAD to go hull down behind a hill to gain better protection.",
	mech_hulldown},
    {0, "ENTERBASE [N|W|S|E]",
	    "Enters base/hangar/whatnever from selected dir.",
	mech_enterbase},

    {0, "ENTERBAY [REF]",
	    "Enters bay of a moving(?) hangar (DropShip for example). Ref is target ref, and it is optional.",
	mech_enterbay},
    {1, "BOOTLEGGER [R|L]",
	    "Performs a bootlegger turn. This will turn you instantly 90 degrees in the desired direction, but requires a pilot roll. Roll BTH is based on tonnage and speed. Legs must not be recycling.",
	mech_bootlegger},
#ifdef BT_MOVEMENT_MODES
    {195, "SPRINT",
	    "Toggles sprinting mode. While sprinting you are easier to hit, cannot attack, but can move 2xWalkSpeed.",
	mech_sprint},
    {195, "EVADE",
	    "Toggles evasion mode. While evading you are harder to hit, but cannot attack.",
	mech_evade},
    {1, "DODGE",
	    "Toggles dodge mode on. You must have Dodge_Maneuver advantage. While dodging you can counter physical attack rolls. One per turn.",
	mech_dodge},
#endif
    /* Radio */
    HEADER("Radio"),
    {0, "LISTCHANNELS", "Lists set frequencies + comtitles for them.",
	mech_list_freqs},
    {0, "SENDCHANNEL <LETTER> = <STRING>",
	"Sends <string> on channel <letter>'s freq.", mech_sendchannel},
    {0, "RADIO <ID> = <STRING>",
	"Radioes (LOS) <ID> with <STRING>", mech_radio},
    {0, "SETCHANNELFREQ <LETTER> = <NUMBER>",
	    "Sets channel <letter> to frequency <number>.",
	mech_set_channelfreq},
    {0, "SETCHANNELMODE <LETTER> = <STRING>",
	    "Sets channel <letter> mode <string> (available letters: DIUES, color codes).",
	mech_set_channelmode},
    {0, "SETCHANNELTITLE <LETTER> = <STRING>",
	    "Sets channel <letter> comtitle to <string>.",
	mech_set_channeltitle},

    /* Weapons */
    HEADER("Weapons"),
    {0, "LOCK [<TARGET-ID> | <X> <Y> | <X> <Y> <B|H|I|C> | -]",
	    "Sets the target to the (3rd argument :  B = building, C = clear, I = ignite, H = hex (clear/ignite/break ice/destroy bridge)) /, - = Clears present lock.",
	mech_settarget},
    {0, "SIGHT <WEAPNUM> [<TARGET-ID> | <X> <Y>]",
	"Computes base-to-hit for given weapon and target.", mech_sight},
    {0, "FIRE <WEAPNUM> [<TARGET-ID> | <X> <Y>]",
	    "Fires weapon <weapnum> at def. target or specified target.",
	mech_fireweapon},
    {0, "TARGET <SECTION | ->",
	    "Sets your aimed shot target / Disables targetting.",
	mech_target},
    {0, "TAG [ID|-]",
	    "Lights an enemy unit with your TAG / Disables current TAG.",
	mech_tag},
    /* Weapon mode funcs */

    {0, "AMS <weapnum>", "Toggles Anti-Missile System on and off.",
	mech_ams},
    {0, "AP <weapnum>",
	    "Sets/Unsets the autocannon to fire armor piercing rounds.",
	mech_armorpiercing},
    {0, "ARTEMIS <weapnum>", "Sets Weapon to and from ARTEMIS Mode.",
	mech_artemis},
    {0, "EXPLOSIVE <weapnum>", "Toggles between explosive/normal rounds",
	mech_explosive},
    {0, "FIRECLUSTER <weapnum>",
	    "Sets/unsets artillery weapon to fire cluster rounds.",
	mech_cluster},
    {0, "FIREMINE <weapnum>",
	"Sets/unsets artillery weapon to fire mine rounds.", mech_mine},
    {0, "FIRESMOKE <weapnum>",
	    "Sets/unsets artillery weapon to fire smoke rounds.",
	mech_smoke},
    {0, "FIRESWARM <weapnum>",
	    "Sets/Unsets the LRM launcher to shoot swarm missiles",
	mech_swarm},
    {0, "FIRESWARM1 <weapnum>",
	    "Sets/Unsets the LRM launcher to shoot swarm missiles",
	mech_swarm1},
    {0, "FLECHETTE <weapnum>",
	    "Sets/Unsets the autocannon to fire flechette rounds.",
	mech_flechette},
    {0, "GATTLING <weapnum>",
	    "Sets weapon to and from Gattling Mode (machineguns only).",
	mech_gattling},
    {0, "HEAT <weapnum>", "Sets a flamer to and from heat mode`.",
	mech_flamerheat},
    {0, "HOTLOAD <weapnum>",
	    "Sets/Unsets the LRM launcher to hotload missiles, removing short-range penalties, but adding to chance of jamming.",
	mech_hotload},

    {0, "INARC <weapnum> <-|X|Y|E>",
	    "Sets the type of ammo to fire from your iNarc weapon. '-': standard Homing, 'X': Explosive, 'Y': Haywire, 'E': ECM",
	mech_inarc_ammo_toggle},
    {0, "INCENDIARY <weapnum>",
	    "Sets/Unsets the autocannon to fire incendiary rounds.",
	mech_incendiary},
    {0, "INFERNO <weapnum>",
	    "Sets/Unsets the SRM launcher to shoot inferno missiles",
	mech_inferno},
    {0, "LBX <weapnum>", "Sets weapon to and from LBX Mode.", mech_lbx},
    {0, "NARC <weapnum>", "Sets weapon to and from NARC Mode.", mech_narc},
    {0, "STINGER <weaponum", "Sets weapon to and from Stinger Mode.", 
        mech_stinger},
    {0, "PRECISION <weapnum>",
	    "Sets/Unsets the autocannon to fire precision rounds.",
	mech_precision},
    {0, "RAC <weapnum> <-/2/4/6>",
	    "Sets the Rotary AutoCannon to fire either 1, 2, 4 or 6 shots at a time.",
	mech_rac},
    {0, "RAPIDFIRE <weapnum>",
	    "Sets weapon to and from Rapid Fire Mode (std and light ACs only).",
	mech_rapidfire},
    {0, "ULTRA <weapnum>", "Sets weapon to and from Ultra Mode.",
	mech_ultra},
    {0, "DISABLE <weapnum>", "Disables the weapon (Gauss only).",
	mech_disableweap},
    {0, "UNJAM <weapnum>", "Fixes ammo loader jams.",
	mech_unjamammo},
    {0, "USEBIN <weapnum> <location>",
	    "Draw ammo from <location> first for <weapnum>.",
	mech_usebin},

    /* TIC Support */
    {0, "ADDTIC  <NUM> <WEAPNUM | LOWNUM-HIGHNUM>",
	    "Adds weapon <weapnum>, or weapons from <lownum> to <highnum> to TIC <num>.",
	mech_addtic},
    {0, "CLEARTIC <NUM>", "Clears the TIC <num>.", mech_cleartic},

    {0, "DELTIC <NUM> <WEAPNUM>",
	    "Deletes weapon number <weapnum> from TIC <num>.",
	mech_deltic},

    {0, "FIRETIC <NUM> [<TARGET> or <X Y>]",
	    "Fires the weapons in TIC <num>.",
	mech_firetic},
    {0, "LISTTIC <NUM>", "Lists weapons in the TIC <num>.", mech_listtic},

    /* Information */
    HEADER("Information"),

    {0, "BRIEF [<LTR> <VAL>]",
	    "Shows brief status / Sets brief for <ltr> to <val>.",
	mech_brief},
    {0, "CONTACTS [<Prefix> | <TARGET-ID>]", "List all current contacts",
	mech_contacts},

    {0, "CRITSTATUS <SECTION>", "Shows the Critical hits status",
	mech_critstatus},
    {0, "REPORT [<TARGET-ID> | <X Y>]",
	"Information on default target, num, or x,y", mech_report},
    {0, "SCAN [<TARGET-ID> | <X Y> | <X Y> <B|H>]",
	    "Scans the default target, chosen target, or hex",
	mech_scan},

    {0, "SENSOR [LONG | [<V|L|I|E|S> <V|L|I|E|S>]]",
	    "Shows/Changes your sensor mode (1 argument: Long, otherwise Short description about sensor mode)",
	mech_sensor},

    {0, "STATUS [A(rmor) | I(nfo)] | W(eapons)]",
	    "Prints the mech's status",
	mech_status},
    {0, "VIEW [<TARGET-ID>]", "View the war painting on the target",
	mech_view},
    {0, "WEAPONSPECS", "Shows the specifications for your weapons",
	mech_weaponspecs},
    {0, "WEAPONSTATUS", "Shows the status of all your weapons",
	mech_weaponstatus},

    /* Navigation */
    HEADER("Navigation"),
    {0, "BEARING [<X Y>] [<X Y>]", "Same format as range.", mech_bearing},

    {0, "ETA [<X> <Y>]", "Estimates time to target (/default target)",
	mech_eta},
    {0, "FINDCENTER", "Shows distance/bearing to center of hex.",
	mech_findcenter},
    {0, "NAVIGATE", "Shows the hex and surroundings graphically",
	mech_navigate},
    {0, "RANGE [<X Y>] [<X Y>]",
	    "Range to def. target / range to x y / range to x,y from x,y",
	mech_range},
    {0,
	    "LRS <M|T|E|L|S|H|C> [<BEARING> <RANGE> | <TARGET-ID>]",
	"Shows the (Mech/Terrain/Elevation/LOS/Sensors/Height/Combined) long range map", mech_lrsmap},
    {0, "TACTICAL [C | T | L] [<BEARING> <RANGE> | <TARGET-ID>]",
	    "Shows the tactical map at the mech's location / at bearing and range / around chosen target",
	mech_tacmap},
    {0, "VECTOR [<X Y> <X Y>]", "Same format as range.",mech_vector},

    /* Special */
    HEADER("Special"),

    {12, "CHECKLZ", "Checks if the landing-zone is good for a landing",
	aero_checklz},
    {0, "@OOD <X> <Y> [Z]",
	    "@Initiates OOD drop at the orbit altitude to <X> <Y> (optional Z altitude to start from)",
	initiate_ood},
    {0, "@LOSEMIT <MESSAGE>",
	    "@Sends message to everyone seeing the 'mech right now",
	mech_losemit},
    {0, "@DAMAGE <NUM> <CLUSTERSIZE> <ISREAR> <ISCRITICAL>",
	    "@Causes <NUM> pt of damage to be done to 'mech in <CLUSTERSIZE> point clusters (if <ISREAR> is 1, damage is done to rear arc ; if <ISCRITICAL> is 1, damage does crit-thru-armor)",
	mech_damage},
    {0, "@WEIGHT", "@Checks the weight allocated in the mech",
	mech_weight},
    {4, "BOMB [list | drop <num> | aim]",
	    "Lists bombs / drops bomb <num> / aims where a bomb would fall.",
	mech_bomb},
    {2, "DIG", "Starts burrowing for cover [non-hovers only].", mech_dig},

    {2, "FIXTURRET",
	    "Starts to fix a turret. Only works on jammed turrets, not locked turrets.",
	mech_fixturret},

    {0, "EXPLODE <AMMO|REACTOR|STOP>",
	    "<AMMO|REACTOR> specifies which to ignite ; ammo causes all ammo on your mech to go *bang* (in no particular order), reactor disables control systems. Do note that neither are instant. STOP allows you to stop existing countdown.",
	mech_explode},
    {0, "SAFETY [ON|OFF]",
	    "Enable/Disable Safeties against killing MechWarriors.",
	mech_safety},
    {0, "MECHPREFS [SETTING [ON|OFF]]", "Toggle the mechpref setting of SETTING",
    	mech_mechprefs},
    {51, "TURNMODE [TIGHT | NORMAL]", "Sets turnmode.", mech_turnmode},

    /* Vengy's pickup/dropoff */
    {0, "DROPOFF", "Drops the mech you are carrying.", mech_dropoff},
    {0, "PICKUP [ID]", "Picks up [ID].", mech_pickup},
    {128, "ATTACHCABLES <ID1> <ID2>",
	    "Attaches tow cables so that [ID1] can tow [ID2].",
	    mech_attachcables},
    {128, "DETACHCABLES <ID>", "Detaches the tow cables from [ID].",
	    mech_detachcables},


    {0, "DUMP <WEAPNUM|LOCATION|ALL|STOP> [<CRIT>]",
	    "Dumps the ammunition for the weapon / in the location [ crit ] / all ammunition in the 'mech / stops all dumping in progress.",
	mech_dump},
    {1, "FLIPARMS",
	    "Flips the arms to and from the rear arcs, if possible.",
	mech_fliparms},

    {0, "ECM",
	    "Toggles the ECM status of your Guardian ECM suite (only applicable if you have one)",
	mech_ecm},
    {0, "ECCM",
	    "Toggles the ECCM status of your Guardian ECM suite (only applicable if you have one)",
	mech_eccm},
    {0, "ANGELECM",
	    "Toggles the ECM status of your Angel ECM suite (only applicable if you have one)",
	mech_angelecm},
    {0, "ANGELECCM",
	    "Toggles the ECCM status of your Angel ECM suite (only applicable if you have one)",
	mech_angeleccm},

    {0, "PERECM",
	    "Toggles the ECM status of your Personal ECM suite (only applicable if you have one)",
	mech_perecm},
    {0, "PERECCM",
	    "Toggles the ECCM status of your Personal ECM suite (only applicable if you have one)",
	mech_pereccm},

    {0, "STEALTH",
	    "Toggles status of Stealth Armor for those mechs equipped with it.",
	mech_stealtharmor},
    {0, "NSS",
	    "Toggles status of the Null Signature System for those mechs equipped with it.",
	mech_nullsig},
    /* Ejection */

    {0, "DISEMBARK", "Gets the hell out of the 'mech / vehicle.",
	mech_disembark},
    {0, "UDISEMBARK", "Get the unit out of it's carrier.", mech_udisembark},
    {0, "EMBARK", "Climb into a 'mech / vehicle", mech_embark},
    {1, "MASC", "Toggles MASC on and off", mech_masc},
    {0, "SCHARGE", "Toggles Supercharger on and off", mech_scharge},
    /* DS / VTOL */
    {-2, "LAND", "Terminate your jump or land a VTOL/Aero/DS", mech_land},
    {-35, "TAKEOFF", "VTOL/Aero take off command", aero_takeoff},
    {1, "ROTTORSO <L(eft) | R(ight) | C(enter)>",
	"Rotates the torso 60 degrees right or left.", mech_rotatetorso},
    /* Nim's IDF things */
    {0, "SLITE", "Turns your searchlight on/off", mech_slite},

    {0, "SPOT [ID|-|OWNID]",
	    "Sets someone as your spotter / makes you stop spotting / sets you as a spotter.",
	mech_spot},
    {0, "STARTUP [OVERRIDE]", "Commences startup cycle.", mech_startup},
    {0, "SHUTDOWN", "Shuts down the mech.", mech_shutdown},
    {34, "TURRET", "Set the turret facing.", mech_turret},
    {34, "AUTOTURRET",
	    "Forces your turret to stay facing the locked target.",
	mech_auto_turret},
    {18, "EXTINGUISH",
	    "Puts out the fires on your vehicle. You must be shut down to do this.",
	vehicle_extinquish_fire},

#ifdef C3_SUPPORT
    /* C3 */

    {0, "C3 [ID|-]",
	    "Joins/Leaves a C3 network which the target mech is in. You will be assigned to a master computer within the network.",
	mech_c3_join_leave},
    {0, "C3MESSAGE <MSG>",
	    "Sends a message to all others in your C3 network",
	mech_c3_message},
    {0, "C3TARGETS", "Shows available C3 targeting information",
	mech_c3_targets},
    {0, "C3NETWORK", "Displays information about your C3 network",
	mech_c3_network},

    {0, "C3I [ID|-]",
	    "Joins/Leaves the C3i network connected to the target",
	mech_c3i_join_leave},
    {0, "C3IMESSAGE <MSG>",
	    "Sends a message to all others in your C3i network",
	mech_c3i_message},
    {0, "C3ITARGETS", "Shows available C3i targeting information",
	mech_c3i_targets},
    {0, "C3INETWORK", "Displays information about your C3i network",
	mech_c3i_network},
#endif

/* Heat stuff */

    {0, "HEATCUTOFF",
	    "Sets your heat dissipation so that you wont go under 9 heat for TSM",
	heat_cutoff},
    {0, "PODS",
	    "Shows the location of NARC and iNARC pods that attached to you",
	show_narc_pods},
    {1, "REMOVEPOD <LOCATION> <TYPE>",
	    "Remove one of the pods from the selected location. Possible types are: 'H' - Homing, 'Y' - Haywire, 'E' - ECM",
	remove_inarc_pods_mech},
    {18, "REMOVEPODS",
	    "Removes all iNARC pods from the unit.",
	remove_inarc_pods_tank},

    /* Physical */
    SHEADER(1, "Physical"),
    {1, "AXE [R | L | B] [<TARGET-ID>]", "Axes a target", mech_axe},

    {3, "CHARGE [<TARGET-ID> | - ]",
	"Charges a target. '-' removes charge command.", mech_charge},
    {1, "CHOP [R | L | B] [<TARGET-ID>]", "Chops target with a sword",
	mech_sword},
    {1, "CLUB [<TARGET-ID>]", "Clubs a target with a tree", mech_club},
    {1, "KICK [R | L] [<TARGET-ID>]", "Kicks a target", mech_kick},
    {1, "PUNCH [R | L | B] [<TARGET-ID>]", "Punches a target", mech_punch},
    {1, "GRABCLUB [R | L | -]",
	"Grabs a tree and carries it around as a club", mech_grabclub},

    {64, "ATTACKLEG [<TARGET-ID>]",
	"Attacks legs of the target battlemech", bsuit_attackleg},
    {0, "HIDE",
	    "Attempts to hide your team ; doesn't work if any hostiles have their eyeballs on you",
	bsuit_hide},
    {64, "SWARM [<TARGET-ID> | -]",
	"Swarms the target / drop off target (-)", bsuit_swarm},
    {64, "JETTISON",
	"Jettison your backpack", JettisonPacks},

    /* Repairing */
    HEADER("Repair"),
    {0, "CHECKSTATUS", "Checks mech's techstatus", tech_checkstatus},
    {0, "DAMAGES", "Shows the mech's damages",
	show_mechs_damage},
    {0, "FIX [<NUM> | <LOW-HI>]", "Fixes entry <NUM> from mech's damages",
	tech_fix},
    {0, "FIXARMOR <LOC>", "Repairs armor in <loc>", tech_fixarmor},
    {0, "FIXINTERNAL <LOC>", "Repairs internals in <loc>",
	tech_fixinternal},
    {0, "REATTACH <LOC>", "Reattaches the limb",
	tech_reattach},
    {64, "REPLACESUIT <SUIT>", "Replaces the missing suit",
	tech_replacesuit},
    {0, "RESEAL <LOC>", "Reseals the limb",
	tech_reseal},

    {0, "RELOAD <LOC> <POS> [TYPE]",
	    "Reloads the ammo compartment in <loc>/<pos> (optionally with [type])",
	tech_reload},
    {0, "TOGGLETYPE <loc> <pos> <type>",
	    "Set the type of ammo in ammobin <loc>/<pos> to type <type>",
	tech_toggletype},
    {0, "REMOVEGUN <NUM>", "Removes the gun", tech_removegun},
    {0, "REMOVEPART <LOC> <POS>", "Removes the part", tech_removepart},
    {0, "REMOVESECTION <LOC>", "Removes the section", tech_removesection},


    {0, "REPLACEGUN [<NUM> | <LOC> <POS>] [ITEM]",
	    "Replaces the gun in the position (optionally with [ITEM], like Martell.MediumLaser)",
	tech_replacegun},

    {0, "REPAIRGUN [<NUM> | <LOC> <POS>]",
	    "Repairs the gun in the position",
	tech_repairgun},
    {0, "REPLACEPART <LOC> <POS>", "Replaces the part in the position",
	tech_replacepart},
    {0, "REPAIRPART <LOC> <POS>", "Repairs the part in the position",
	tech_repairpart},
    {0, "REPAIRS", "Shows repairs/scrapping in progress", tech_repairs},
    {0, "UNLOAD <LOC> <POS>",
	    "Unloads the ammo compartment in <loc>/<pos>",
	tech_unload},

    {0, "@MAGIC", "@Fixes the unfixable - skirt crits etc (wiz-only)",
	tech_magic},

    /* Cargo */
    HEADER("Cargo"),

    {0, "LOADCARGO <NAME> <COUNT>",
	    "Loads up <COUNT> <NAME>s from the bay.",
	mech_loadcargo},
    {0, "MANIFEST", "Lists stuff carried by mech.", mech_manifest},
    {0, "STORES", "Lists stuff in the bay.", mech_stores},
    {0, "UNLOADCARGO <NAME> <COUNT>",
	    "Unloads <COUNT> <NAME>s to the bay.",
	mech_unloadcargo},

    /* Restricted commands */
    HEADER("@Restricted"),
    {0, "@CREATEBAYS [.. list of DBrefs, seperated by space]",
	"@Creates / Disables bays on a DS", mech_createbays},
    {0, "@SETMECH <NAME> <VALUE|DATA>", "@Sets xcode value on object",
	set_xcodestuff},
    {0, "@SETXCODE <NAME> <VALUE|DATA>", "@Sets xcode value on object",
	set_xcodestuff},
    {0, "@VIEWXCODE", "@Views xcode values on object", list_xcodestuff},

    {0, "SNIPE <ID> <WEAPON>",
	    "@Lets you 'snipe' (=shoot artillery weapons with movement prediction)",
	mech_snipe},
    {0, "ADDSTUFF <NAME> <COUNT>",
	    "@Adds <COUNT> <NAME> to mech's inventory",
	mech_Raddstuff},

    {0, "FIXSTUFF", "@Fixes consistency errors in econ data",
	mech_Rfixstuff},
    {0, "CLEARSTUFF", "@Removes all stuff from 'mech", mech_Rresetstuff},

    {0, "REMOVESTUFF <NAME> <COUNT>",
	    "@Removes <COUNT> <NAME> from mech's inventory",
	mech_Rremovestuff},
    {0, "SETMAPINDX <NUM>", "@Sets the mech's map index to num.",
	mech_Rsetmapindex},
    {0, "SETTEAM <NUM>", "@Sets the teams.", mech_Rsetteam},
    {0, "SETXY <X> <Y>", "@Sets the x & y value of the mech.",
	mech_Rsetxy},
    {0, NULL, NULL, NULL}
};

ECMD(map_addice);
ECMD(map_delice);
ECMD(map_setconditions);

CommandsStruct mapcommands[] = {
    {0, "@VIEWXCODE", "@Views xcode values on object", list_xcodestuff},
    {0, "@SETXCODE <NAME> <VALUE|DATA>", "@Sets xcode value on object",
	set_xcodestuff},
    {0, "@SETMAP <NAME> <VALUE|DATA>", "@Sets xcode value on object",
	set_xcodestuff},

    {0, "ADDICE <NUMBER>",
	    "@Adds ice (<NUMBER> percent chance for each watery hex connected to land/ice)",
	map_addice},
    {0, "DELICE <NUMBER>",
	"@Deletes first-melting ices at <NUMBER> chance", map_delice},
    {0, "PATHFIND <X1> <Y1> <X2> <Y2> [OPTFACT]",
	    "@Finds shortest path from x1,y1 to x2,y2 using A* approx algorithm (using optfact optimization factor, 0-100, smaller = slower, more accurate)",
	map_pathfind},
    {0, "SETCOND <GRAV> <TEMP> [CLOUDBASE [VACUUM]]",
	    "@Sets the map attributes (gravity: in 1/100'ths of Earth gravity, temperature: in Celsius, vacuum: optional, number (0 or 1)",
	map_setconditions},
    {0, "VIEW <X> <Y>", "@Shows the map centered at X,Y", map_view},

    {0, "ADDBLOCK <X> <Y> <DIST> [TEAM#_TO_ALLOW]",
	    "@Adds no-landings zone of DIST hexes to X Y",
	map_add_block},
    {0, "ADDMINE <X> <Y> <TYPE> <STRENGTH> [OPT]", "@Adds mine to X,Y",
	map_add_mine},
    {0, "ADDHEX <X> <Y> <TERRAIN> <ELEV>",
	    "@Changes the terrain and elevation of the given hex",
	map_addhex},
    {0, "SETLINKED", "@Sets the map linked", map_setlinked},
    {0, "@MAPEMIT <MESSAGE>", "@Emits stuff to the map", map_mapemit},
    {0, "FIXMAP", "@Fixes inconsistencies in map", debug_fixmap},
    {0, "LOADMAP <NAME>", "@Loads the named map", map_loadmap},
    {0, "SAVEMAP <NAME>", "@Saves the map as name", map_savemap},
    {0, "SETMAPSIZE <X> <Y>", "@Sets x and y size of map", map_setmapsize},

    {0, "LIST [MECHS | OBJS]", "@Lists mechs/objects on the map",
	map_listmechs},
    {0, "CLEARMECHS [DBNUM]", "@Clears mechs from the map",
	map_clearmechs},
    {0, "ADDFIRE [X] [Y] [DURATION]",
	"@Adds fire that lasts <duration> secs", map_addfire},
    {0, "ADDSMOKE [X] [Y] [DURATION]",
	"@Adds smoke that lasts <duration> secs", map_addsmoke},
    {0, "DELOBJ [[TYPE] | [X] [Y] | [TYPE] [X] [Y]]",
	"@Deletes objects of either type or at x/y", map_delobj},
    {0, "UPDATELINKS",
	    "@Updates CodeLinks from the database objs (recursive)",
	map_updatelinks},

    /* Cargo things */
    {0, "STORES", "Lists stuff in the hangar.", mech_manifest},
    {0, "ADDSTUFF <NAME> <COUNT>", "@Adds <COUNT> <NAME> to map",
	mech_Raddstuff},

    {0, "FIXSTUFF", "@Fixes consistency errors in econ data",
	mech_Rfixstuff},
    {0, "REMOVESTUFF <NAME> <COUNT>", "@Removes <COUNT> <NAME> from map",
	mech_Rremovestuff},
    {0, "CLEARSTUFF", "@Removes all stuff from map", mech_Rresetstuff},
    {0, NULL, NULL, NULL}
};


CommandsStruct mechrepcommands[] = {
    {0, "SETTARGET <NUM>", "@Sets the mech to be repaired/built to num",
	mechrep_Rsettarget},

    {0, "LOADNEW <TYPENAME>", "@Loads a new mech template.",
	mechrep_Rloadnew},
    {0, "RESTORE", "@Completely repairs and reloads mech. ",
	mechrep_Rrestore},

/* {0,"SAVENEW <TYPENAME>","@Saves the mech as a template.", mechrep_Rsavetemp}, */
    {0, "SAVENEW <TYPENAME>", "@Saves the mech as a new-type template.",
	mechrep_Rsavetemp2},
    {0, "SETARMOR <LOC> <AVAL> <IVAL> <RVAL>",
	    "@Sets the armor, int. armor, and rear armor.",
	mechrep_Rsetarmor},
    {0, "ADDWEAP <NAME> <LOC> <CRIT SECS> [R|T|O]",
	    "@Adds weapon to the mech, using given loc, crits, and flags.",
	mechrep_Raddweap},

    {0, "RESETCRITS", "@Resets criticals of the toy to base of type.",
	mechrep_Rresetcrits},
    {0, "REPAIR <LOC> <TYPE> <[VAL | SUBSECT]>", "@Repairs the mech.",
	mechrep_Rrepair},
    {0, "RELOAD <NAME> <LOC> <SUBSECT> [L|A|N(|C|M|S)]",
	    "@Reloads weapon in location and critical subsection.",
	mechrep_Rreload},
    {0, "ADDSP <ITEM> <LOC> <SUBSECT> [<DATA>]",
	    "@Adds a special item in location & critical subsection.",
	mechrep_Raddspecial},
    {0, "DISPLAY <LOC>", "@Displays all the items in the location.",
	mechrep_Rdisplaysection},
    {0, "SHOWTECH", "@Shows the advanced technology of the mech.",
	mechrep_Rshowtech},
    {0, "ADDTECH <TYPE>",
	"@Adds the advanced technology to the mech.", mechrep_Raddtech},
    {0, "DELTECH <ALL or [<TECH>]>",
	    "@Deletes all or one advanced technologies on the mech.",
	mechrep_Rdeltech},
    {0, "ADDINFTECH <TYPE>",
	    "@Adds the advanced infantry technology to the mech.",
	mechrep_Raddinftech},
    {0, "DELINFTECH",
	    "@Deletes the advanced infantry technology of the mech.",
	mechrep_Rdelinftech},
    {0, "SETTONS <NUM>", "@Sets the mech tonnage", mechrep_Rsettons},
    {0, "SETTYPE <MECH | GROUND | VTOL | NAVAL | AERO | DS | SPHEROIDDS | BSUIT >",
	"@Sets the mech type", mechrep_Rsettype},
    {0, "SETMOVE <TRACK | WHEEL | HOVER | VTOL | HULL | FOIL | FLY>",
	"@Sets the mech movement type", mechrep_Rsetmove},
    {0, "SETMAXSPEED <NUM>", "@Sets the max speed of the mech.",
	mechrep_Rsetspeed},
    {0, "SETHEATSINKS <NUM>", "@Sets the number of heat sinks.",
	mechrep_Rsetheatsinks},
    {0, "SETJUMPSPEED <NUM>", "@Sets the jump speed of the mech.",
	mechrep_Rsetjumpspeed},
    {0, "SETLRSRANGE <NUM>", "@Sets the lrs range of the mech.",
	mechrep_Rsetlrsrange},
    {0, "SETTACRANGE <NUM>", "@Sets the tactical range of the mech.",
	mechrep_Rsettacrange},
    {0, "SETSCANRANGE <NUM>", "@Sets the scan range of the mech.",
	mechrep_Rsetscanrange},
    {0, "SETRADIORANGE <NUM>", "@Sets the radio range of the mech.",
	mechrep_Rsetradiorange},
    {0, "SETCARGOSPACE <VAL> <MAXTON>",
	"Sets cargospace and max cargo tonnage", mechrep_setcargospace},
    {0, NULL, NULL, NULL}
};

#ifdef MENU_CUSTOMIZE
#include "coolmenu_interface.h"
ECOMMANDSET(cu);
#endif

CommandsStruct customcommands[] = {
#ifdef MENU_CUSTOMIZE
    GCOMMANDSET(cu) {0, "@SETXCODE <NAME> <VALUE|DATA>",
	    "@Sets xcode value on object",
	set_xcodestuff},
    {0, "@VIEWXCODE", "@Views xcode values on object", list_xcodestuff},

    {0, "@WEIGHT", "@Checks the weight allocated in the new mech",
	custom_weight1},
    {0, "@WEIGHTO", "@Checks the weight allocated in the old mech",
	custom_weight2},
    {0, "EDIT <ref>", "Alters <ref>", custom_edit},
    {0, "FINISH", "Quit editing mode", custom_finish},
    {0, "Z", "Back", custom_back},
    {0, "L", "Shows menu", custom_look},
    {0, "LO", "Shows menu", custom_look},
    {0, "LOO", "Shows menu", custom_look},
    {0, "LOOK", "Shows menu", custom_look},
    {0, "HELP", "Shows help for customization", custom_help},

    {0, "CRITSTATUS <SECTION>", "Shows the Critical hits status",
	custom_critstatus},
    {0, "STATUS [A(rmor) | I(nfo)] | W(eapons)]",
	    "Prints the mech's status",
	custom_status},
    {0, "WEAPONSPECS", "Shows the specifications for your weapons",
	custom_weaponspecs},
    {0, NULL, NULL, NULL}
#endif
};

#ifdef MENU_CHARGEN
#include "coolmenu_interface.h"
ECOMMANDSET(cm);
#endif

CommandsStruct chargencommands[] = {
#ifdef MENU_CHARGEN
    GCOMMANDSET(cm) {0, "DONE", "Finishes your chargen (permanent)",
	chargen_done},
    {0, "BEGIN", "Starts chargen", chargen_begin},
    {0, "NEXT", "Goes to next stage of chargen", chargen_next},
    {0, "PREV", "Goes to previous stage of chargen", chargen_prev},

    {0, "APPLY",
	    "Applies the values to your character (fixes them, only reset/done can be done after)",
	chargen_apply},
    {0, "RESET", "Resets your stats and lets you begin again",
	chargen_reset},
    {0, "L", "Shows menu", chargen_look},
    {0, "LO", "Shows menu", chargen_look},
    {0, "LOO", "Shows menu", chargen_look},
    {0, "LOOK", "Shows menu", chargen_look},
    {0, "STATUS", "Shows menu", chargen_look},
    {0, "HELP", "Shows help for chargen", chargen_help},
#endif
    {0, NULL, NULL, NULL}
};

CommandsStruct autopilotcommands[] = {
    {0, "ENGAGE", "Engages the autopilot", auto_engage},
    {0, "DISENGAGE", "Disengages the autopilot", auto_disengage},

    {0, "ADDCOMMAND <NAME> [ARGS]", "Adds a command to queue",
	auto_addcommand},
    {0, "DELCOMMAND <NUM>", "Removes command <NUM> from queue (-1 = all)",
	auto_delcommand},
    {0, "LISTCOMMANDS", "Lists whole command queue of the autopilot",
	auto_listcommands},
    {0, "JUMP <NUM>", "Sets current instruction to <NUM>", auto_jump},
    {0, "EVENTSTATS", "Lists current events for this AI", auto_eventstats},
    {0, NULL, NULL, NULL}
};

CommandsStruct turretcommands[] = {
    {0, "@SETTURRET <NAME> <VALUE|DATA>", "@Sets xcode value on object",
	set_xcodestuff},
    {0, "@SETXCODE <NAME> <VALUE|DATA>", "@Sets xcode value on object",
	set_xcodestuff},
    {0, "@VIEWXCODE", "@Views xcode values on object", list_xcodestuff},
    {0, "DEINITIALIZE", "De-initializes you as gunner",
	turret_deinitialize},
    {0, "INITIALIZE", "Sets you as the gunner", turret_initialize},

    {0, "ADDTIC  <NUM> <WEAPNUM | LOWNUM-HIGHNUM>",
	"Adds weapnum, or lownum-highnum to given TIC", turret_addtic},
    {0, "BEARING [<X Y>] [<X Y>]", "Same format as range.",
	turret_bearing},
    {0, "CLEARTIC <NUM>", "Clears the TIC number given ", turret_cleartic},
    {0, "CONTACTS [<Prefix> | <TARGET-ID>]", "List all current contacts",
	turret_contacts},

    {0, "CRITSTATUS <SECTION>", "Shows the Critical hits status",
	turret_critstatus},
    {0, "DELTIC <NUM> <WEAPNUM>", "Deletes weapnum from given TIC",
	turret_deltic},
    {0, "ETA [<X> <Y>]", "Estimates time to target (/default target)",
	turret_eta},
    {0, "FINDCENTER", "Shows distance/bearing to center of hex.",
	turret_findcenter},
    {0, "FIRE <WEAPNUM> [<TARGET-ID> | <X> <Y>]",
	    "Fires Weap at loc at def. target or specified target.",
	turret_fireweapon},
    {0, "FIRETIC <NUM> [<TARGET> or <X Y>]", "Fires the given TIC",
	turret_firetic},
    {0, "LISTTIC <NUM>", "Lists weapons in the given TIC", turret_listtic},

    {0, "LOCK [<TARGET-ID> | <X> <Y> | <X> <Y> <B|H> | -]",
	    "Sets the target to the arg (in 3rd, B = building, H = hex (clear/ignite)) / Clears lock (-)",
	turret_settarget},
    {0,
	    "LRS <M(ech) | T(errain) | E(lev)> [<BEARING> <RANGE> | <TARGET-ID>]",
	"Shows the long range map", turret_lrsmap},
    {0, "NAVIGATE", "Shows the hex and surroundings graphically",
	turret_navigate},
    {0, "RANGE [<X Y>] [<X Y>]",
	    "Range to def. target / range to x y / range to x,y from x,y",
	turret_range},
    {0, "REPORT [<TARGET-ID> | <X Y>]",
	"Information on default target, num, or x,y", turret_report},
    {0, "SCAN [<TARGET-ID> | <X Y> | <X Y> <B|H>]",
	    "Scans the default target, chosen target, or hex",
	turret_scan},

    {0, "SIGHT <WEAPNUM> [<TARGET-ID> | <X> <Y>]",
	    "Computes base-to-hit for given weapon and target.",
	turret_sight},
    {0, "STATUS [A(rmor)|I(nfo)]|W(eapons)|S(hort)]",
	    "Prints the mech's status",
	turret_status},

    {0, "TACTICAL [<BEARING> <RANGE> | <TARGET-ID>]",
	    "Shows the tactical map at the mech's location / at bearing and range / around chosen target",
	turret_tacmap},
    {0, "WEAPONSPECS", "Shows the specifications for your weapons",
	turret_weaponspecs},
    {0, NULL, NULL, NULL}
};

CommandsStruct scencommands[] = {
    {0, "@SETXCODE <NAME> <VALUE|DATA>", "@Sets xcode value on object",
	set_xcodestuff},
    {0, "@VIEWXCODE", "@Views xcode values on object", list_xcodestuff},
    {0, "ENGAGE", "Starts the scenario", scen_start},
    {0, "END", "Ends the scenario", scen_end},

    {0, "STATUS [SIDE]",
	"Reports status of the scenario [/for one side]", scen_status},
    {0, NULL, NULL, NULL}
};


ECMD(debug_makemechs);
ECMD(debug_memory);
ECMD(debug_setvrt);
ECMD(debug_setxplevel);

CommandsStruct debugcommands[] = {
    {0, "EVENTSTATS", "@Shows event statistics", debug_EventTypes},
    {0, "MEMSTATS [LONG]",
	    "@Shows memory statistics (optionally in long form)",
	debug_memory},
    {0, "SAVEDB", "@Saves the SpecialObject DB", debug_savedb},
    {0,
	    "MAKEMECHS <FACTION> <TONS> [<TYPES> [<OPT_TONNAGE> [<MAX_VARIATION>]]]",
	    "@Makes list of 'mechs of <faction> with max tonnage of <tons>, and optimum tonnage for each mech <opt_tonnage> (optional)",
	debug_makemechs},
    {0, "LISTFORMS", "@Shows forms", ListForms},

    {0, "SETVRT <WEAPON> <NUM>",
	    "@Sets the VariableRecycleTime for weapon <WEAPON> to <NUM>",
	debug_setvrt},
    {0, "SETXPLEVEL <SKILL> <NUM>",
	    "@Sets the XP threshold for skill <skill> to <num>",
	debug_setxplevel},
    {0, "SHUTDOWN <MAP#>", "@Shutdown all mechs on the map and clear it.",
	debug_shutdown},

    {0, "XPTOP <SKILL>", "@Shows list of people champ in the <SKILL>",
	debug_xptop},
    {0, NULL, NULL, NULL}
};

CommandsStruct sscommands[] = {
    {0, "@SETXCODE <NAME> <VALUE|DATA>", "@Sets xcode value on object",
	set_xcodestuff},
    {0, "@VIEWXCODE", "@Views xcode values on object", list_xcodestuff},
    {0, NULL, NULL, NULL}
};

#define LINEB(txt,cmd,str,func,upd,updfunc,power) \
{ txt, cmd, str, func, upd, updfunc, power }
#define LINE(txt,cmd,str,func,upd,updfunc,power) \
LINEB(txt,cmd,sizeof(str),func,upd,updfunc,power)

/* Own init func, no update func */
#define LINE_NU(txt,cmd,str,fu,power) \
LINE(txt,cmd,str,fu,0,NULL,power)

/* No data, no update */
#define LINE_ND(txt,cmd,power) \
LINEB(txt,cmd,0,NULL,0,NULL,power)

/* Just data, no special init, no update func */
#define LINE_NFS(txt,cmd,t,power) \
LINEB(txt,cmd,sizeof(t),NULL,0,NULL,power)

SpecialObjectStruct SpecialObjects[] = {
    LINE("MECH", mechcommands, MECH, newfreemech, HEAT_TICK, mech_update, POW_MECH),
    LINE_ND("DEBUG", debugcommands, POW_SECURITY),
    LINE_NU("MECHREP", mechrepcommands, struct mechrep_data, newfreemechrep, POW_MECHREP),
    LINE("MAP", mapcommands, MAP, newfreemap, LOS_TICK, map_update, POW_MAP),
    LINE_ND("CHARGEN", chargencommands, POW_SECURITY),
    LINE_NU("AUTOPILOT", autopilotcommands, AUTO, auto_newautopilot, POW_SECURITY),
    LINE_NU("TURRET", turretcommands, TURRET_T, newturret, POW_SECURITY),
    LINE_NU("CUSTOM", customcommands, struct custom_struct, newfreecustom, POW_MECHREP),
    LINE_NFS("SCEN", scencommands, SCEN, POW_SECURITY),
    LINE_NFS("SSIDE", sscommands, SSIDE, POW_SECURITY),
    LINE_NFS("SSOBJ", sscommands, SSOBJ, POW_SECURITY),
    LINE_NFS("SSINS", sscommands, SSINS, POW_SECURITY),
    LINE_NFS("SSEXT", sscommands, SSEXT, POW_SECURITY)
};

#define NUM_SPECIAL_OBJECTS \
   ((sizeof(SpecialObjects))/(sizeof(struct SpecialObjectStruct)))

#undef HEADER


#endif

/* Something about [new] Linux gcc is braindead.. I just don't know
   what, but this allows the code to link [bleah] */
#ifdef memcpy
#undef memcpy
#endif

void send_channel(char *, const char *, ...);

#endif
