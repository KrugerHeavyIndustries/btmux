
/*
 * $Id: autopilot.h,v 1.4 2005/08/03 21:40:54 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Wed Oct 30 20:43:42 1996 fingon
 * Last modified: Sat Jun  6 19:56:42 1998 fingon
 *
 */

#ifndef AUTOPILOT_H
#define AUTOPILOT_H

#include "mech.events.h"
#include "dllist.h"

#define AUTOPILOT_VERSION       "BTMux AI 3.0 - Zeke"

/* Status Flags for the AI */
#define AUTOPILOT_ON            1       /* Is the AI even on? */
#define AUTOPILOT_SETUP         2       /* AI has been setup? */
#define AUTOPILOT_GUNNING       4       /* Should the AI be shooting stuff */

#define IsAutoOn(a)             ((a)->status & AUTOPILOT_ON)
#define AutoOn(a)               ((a)->status |= AUTOPILOT_ON)
#define AutoOff(a)              ((a)->status &= ~(AUTOPILOT_ON))

#define IsAutoSetup(a)          ((a)->status & AUTOPILOT_SETUP)
#define AutoSetup(a)            ((a)->status & AUTOPILOT_SETUP)
#define ClearAutoSetup(a)       ((a)->status &= ~(AUTOPILOT_SETUP))

#define IsAutoGunning(a)        ((a)->status & AUTOPILOT_GUNNING)
#define AutoGunning(a)          ((a)->status |= AUTOPILOT_GUNNING)
#define StopAutoGunning(a)      ((a)->status &= ~(AUTOPILOT_GUNNING))

/* Stuff specifically for autogun */
#define AUTO_PROFILE_TICK               180     /* How often to update the weapon profile 
                                                   of the AI */
#define AUTO_PROFILE_MAX_SIZE           30      /* Size of the profile array */

/* Stuff for Auto Sensor */
#define AUTO_SENSOR_TICK                30      /* Every 30 seconds or so */

/* Old stuff delete it */

#define AUTOPILOT_MEMORY        100     /* Number of command slots available to AI */
#define AUTOPILOT_MAX_ARGS      5       /* Max number of arguments for a given AI Command
                                           Includes the command as the first argument */

/* The various flags for the AI */
#define AUTOPILOT_AUTOGUN           1       /* Is autogun enabled, ie: shoot what AI wants to */
#define AUTOPILOT_GUNZOMBIE         2
#define AUTOPILOT_PILZOMBIE         4
#define AUTOPILOT_ROAM              8       /* Are we roaming around */
#define AUTOPILOT_LSENS             16      /* Should change sensors or user set them */
#define AUTOPILOT_CHASETARG         32      /* Should chase the target */
#define AUTOPILOT_WAS_CHASE_ON      64      /* Was chasetarg on, for use with movement stuff */
#define AUTOPILOT_SWARMCHARGE       128 
#define AUTOPILOT_ASSIGNED_TARGET   256     /* We given a specific target ? */

/* Various delays for the autopilot */
#define AUTOPILOT_NC_DELAY              1       /* Generic command wait time before executing */

#define AUTOPILOT_GOTO_TICK             3       /* How often to check any GOTO event */
#define AUTOPILOT_LEAVE_TICK            5       /* How often to check if we've left 
                                                   the bay/hangar */
#define AUTOPILOT_WAITFOE_TICK          4
#define AUTOPILOT_PURSUE_TICK           3

#define AUTOPILOT_FOLLOW_TICK           3
#define AUTOPILOT_FOLLOW_UPDATE_TICK    10  /* When should we update the target hex */

#define AUTOPILOT_CHASETARG_UPDATE_TICK 30  /* When should we update chasetarg */

#define AUTOPILOT_STARTUP_TICK  STARTUP_TIME + AUTOPILOT_NC_DELAY   /* Delay for startup */

/* Defines for the autogun/autosensor stuff */
#define AUTO_GUN_TICK                   1       /* Every second */
#define AUTO_GUN_MAX_HEAT               6.0     /* Last heat we let heat go to */
#define AUTO_GUN_MAX_TARGETS            100     /* Don't really use this one */
#define AUTO_GUN_MAX_RANGE              30      /* Max range to look for targets */
#define AUTO_GUN_UPDATE_TICK            30      /* When to look for a new target */
#define AUTO_GUN_IDLE_TICK              10      /* How often to call autogun when in idle mode */
#define AUTO_GUN_PHYSICAL_RANGE_MIN     3.0     /* Min range at which to physically attack 
                                                   other targets if our main target is beyond
                                                   this distance */

#define Gunning(a)              ((a)->flags & AUTOPILOT_AUTOGUN)
#define StartGun(a)             (a)->flags |= AUTOPILOT_AUTOGUN
#define StopGun(a)              (a)->flags &= ~(AUTOPILOT_AUTOGUN|AUTOPILOT_GUNZOMBIE)

/* Do we have an assigned target */
#define AssignedTarget(a)       ((a)->flags & AUTOPILOT_ASSIGNED_TARGET)
#define AssignTarget(a)         (a)->flags |= AUTOPILOT_ASSIGNED_TARGET
#define UnassignTarget(a)       (a)->flags &= ~(AUTOPILOT_ASSIGNED_TARGET)

/* Chase Target stuff for use with auto_set_chasetarget_mode */
#define AUTO_CHASETARGET_ON             1       /* Turns it on and resets the values */
#define AUTO_CHASETARGET_OFF            2       /* Turns it off */
#define AUTO_CHASETARGET_REMEMBER       3       /* Turns it on only if the AI remembers it 
                                                   being on */
#define AUTO_CHASETARGET_SAVE           4       /* Turns it off and saves that it was on */

/* Is chasetarget mode on */
#define ChasingTarget(a)        ((a)->flags & AUTOPILOT_CHASETARG)
#define StartChasingTarget(a)   (a)->flags |= AUTOPILOT_CHASETARG
#define StopChasingTarget(a)    (a)->flags &= ~(AUTOPILOT_CHASETARG)

/* Was chasetarget mode on ? */
#define WasChasingTarget(a)         ((a)->flags & AUTOPILOT_WAS_CHASE_ON)
#define RememberChasingTarget(a)    (a)->flags |= AUTOPILOT_WAS_CHASE_ON
#define ForgetChasingTarget(a)      (a)->flags &= ~(AUTOPILOT_WAS_CHASE_ON) 

/* Roam Stuff */
/* Types of ROAMing */
#define AUTO_ROAM_MAP                   1       /* Roaming the whole map */
#define AUTO_ROAM_SPOT                  2       /* Roaming a single area */

/* Tick values etc.. */
#define AUTO_ROAM_TICK                  3       /* How often to update */
#define AUTO_ROAM_NEW_HEX_TICK          100     /* How often to pick a new hex */
#define AUTO_ROAM_MAX_RADIUS            30      /* Max distance a person can make AI
                                                   radius roam */
#define AUTO_ROAM_MAX_MAP_DISTANCE      50      /* Max distance an AI will try to roam
                                                   at a given time if its roaming the
                                                   whole map */
#define AUTO_ROAM_MAX_ITERATIONS        3       /* Max number of times AI will look
                                                   for a new roam hex */

/*! \todo {Not sure what these are look into it} */
#define AUTO_GOET               15
#define AUTO_GOTT               240

#define DoStartGun(a) \
    do { \
        StartGun(a); \
        a->flags &= ~AUTOPILOT_GUNZOMBIE; \
    } while (0)

#define DoStopGun(a)       StopGun(a) 

#define Zombify(a) \
    do { a->flags |= AUTOPILOT_GUNZOMBIE; StopGun(a); } while (0)

#define PilZombify(a) \
    do { a->flags |= AUTOPILOT_PILZOMBIE; } while (0)

#define UnZombify(a) \
    do { if (a->flags & AUTOPILOT_GUNZOMBIE) { StartGun(a);\
        a->flags &= ~AUTOPILOT_GUNZOMBIE; } } while (0)

#define UnZombifyAuto(a) \
    do { if (Gunning(a)) UnZombify(a); if (a->flags & AUTOPILOT_PILZOMBIE) { \
        a->flags &= ~AUTOPILOT_PILZOMBIE ;\
        AUTOEVENT(a, EVENT_AUTOCOM, auto_com_event, 1, 0); } } while (0)

#define UnZombifyMech(mech) \
    do { AUTO *au; if (MechAuto(mech) > 0 && \
        (au=FindObjectsData(MechAuto(mech)))) UnZombifyAuto(au); } while (0)

/*! \todo {Get rid of these} */

#define GVAL(a,b) \
    (((a->program_counter + (b)) < a->first_free) ? \
    a->commands[(a->program_counter+(b))] : -1)

#define CCLEN(a) \
       (1 + acom[a->commands[a->program_counter]].argcount)

#define PG(a) \
       a->program_counter

/* Command Macros */

/* Basic checks for the autopilot */
#define AUTO_CHECKS(a) \
    if (Location(a->mynum) != a->mymechnum) return; \
    if (Destroyed(mech)) return;

/* Shortcut to the auto_com_event function */
#define AUTO_COM(a,n) \
        AUTOEVENT(a, EVENT_AUTOCOM, auto_com_event, (n), 0);

/*! \todo {Get rid of this once we're done} */
#define ADVANCE_PG(a) \
        PG(a) += CCLEN(a); REDO(a,AUTOPILOT_NC_DELAY)

/* Force the unit to start up */
#define AUTO_PSTART(a,mech) \
        if (!Started(mech)) { auto_command_startup(a, mech); return; }

/* Force the unit to startup as well as try to stand */
#define AUTO_GSTART(a,mech) \
    AUTO_PSTART(a,mech); \
    if (MechType(mech) == CLASS_MECH && Fallen(mech) && \
            !(CountDestroyedLegs(mech) > 0)) { \
        if (!Standing(mech)) mech_stand(a->mynum, mech, ""); \
        AUTO_COM(a, AUTOPILOT_NC_DELAY); return; \
    }; \
    if (MechType(mech) == CLASS_VTOL && Landed(mech) && \
            !SectIsDestroyed(mech, ROTOR)) { \
        if (!TakingOff(mech)) aero_takeoff(a->mynum, mech, ""); \
        AUTO_COM(a, AUTOPILOT_NC_DELAY); return; \
    }

/* Macros for quick access to bitfield code */
#define HexOffSet(x, y) (x * MAPY + y)
#define HexOffSetNode(node) (HexOffSet(node->x, node->y))

#define WhichByte(hex_offset) ((hex_offset) >> 3)
#define WhichBit(hex_offset) ((hex_offset) & 7)

#define CheckHexBit(array, offset) (array[WhichByte(offset)] & (1 << WhichBit(offset)) ? 1 : 0)
#define SetHexBit(array, offset) do { array[offset >> 3] = \
        array[offset >> 3] | (1 << (offset & 7)); } while (0)
#define ClearHexBit(array, offset) do { array[offset >> 3] = \
        array[offset >> 3] & ~(1 << (offset & 7)); } while (0)

#ifdef DEBUG_AUTOGUN
    #define print_autogun_log(autopilot, args...) \
    do { \
        fprintf(stderr, "AI: %d AUTOGUN ", autopilot->mynum); \
        fprintf(stderr, args); \
        fprintf(stderr, "\n"); \
    } while(0)
#else
    #define print_autogun_log(autopilot, args...)
#endif

/*
 * Profile node structure
 */
typedef struct profile_node_t {
    int damage;
    int heat;
    rbtree weaplist;
} profile_node;

/*
 * The Autopilot Structure
 */
typedef struct {
    dbref mynum;                    /* The AI's dbref number */
    dbref mymechnum;                /* the dbref of the AI's mech */
    MECH *mymech;                   /* The AI's unit */
    int mapindex;                   /* The map the AI is currently on */
    unsigned short speed;           /* % of speed (1-100) that the AI should drive at */
    int ofsx, ofsy;                 /* ? */

    unsigned char verbose_level;    /* How talkative should the AI be */

    dbref target;                   /* The AI's current target */
    int target_score;               /* Current score of the AI's target */
    int target_threshold;           /* Threshold at which to change to another target */
    int target_update_tick;         /* What autogun tick we currently at and should we update */

    dbref chase_target;             /* Current target we are chasing */
    int chasetarg_update_tick;      /* When should we update chasetarg */

    int follow_update_tick;         /* When should we update follow */

    /* Special AI flags */
    int status;                     /* Status of the AI - Main flags here */
    unsigned short flags;

    /* The autopilot's command list */
    dllist *commands;

    /* AI A* pathfinding stuff */
    dllist *astar_path;

    /* The AI's Weaplist for use with autogun */
    dllist *weaplist;

    /* Range Profile Array - for use with autogun */
    rbtree profile[AUTO_PROFILE_MAX_SIZE];

    /* Max Range of AI's mech's weapons */
    int mech_max_range;

    /* Roam Stuff */
    unsigned char roam_type;            /* What type of ROAM are we doing */
    int roam_update_tick;               /* When should we update roam info */
    short roam_target_hex_x;            /* Target Hex - X Coord */
    short roam_target_hex_y;            /* Target Hex - Y Coord */
    short roam_anchor_hex_x;            /* Anchor Hex - X Coord */
    short roam_anchor_hex_y;            /* Anchor Hex - Y Coord */
    short roam_anchor_distance;         /* Distance (in hexes) allowed from anchor hex */

    /*! \todo {Figure out if we need to keep these variables} */
    /* Temporary AI-pathfind data variables */
    int ahead_ok;
    int auto_cmode;     /* 0 = Flee ; 1 = Close ; 2 = Maintain distances if possible */
    int auto_cdist;     /* Distance we're trying to maintain */
    int auto_goweight;
    int auto_fweight;
    int auto_nervous;

    int b_msc, w_msc, b_bsc, w_bsc, b_dan, w_dan, last_upd;

} AUTO;

/* command_node struct to store AI 
 * commands for the AI command list */
typedef struct command_node_t {
    char *args[AUTOPILOT_MAX_ARGS];         /* Store arguements - At most will ever have 5 */
    unsigned char argcount;                 /* Number of arguments */
    int command_enum;                       /* The ENUM value for the command */
    int (*ai_command_function)(AUTO *);     /* Function Pointer */
} command_node;

/* A structure to store info about the various AI commands */
typedef struct {
    char *name;
    int argcount;
    int command_enum;
    int (*ai_command_function)(AUTO *);
} ACOM;

/* astar node Structure for the A star pathfinding */
typedef struct astar_node_t {
    short x;
    short y;
    short x_parent;
    short y_parent;
    int g_score;
    int h_score;
    int f_score;
    int hexoffset;
} astar_node;

/* Weaplist node for storing info about weapons on the mech */
typedef struct weapon_node_t {
    short weapon_number;
    short weapon_db_number;
    short section;
    short critical;
    int range_scores[AUTO_PROFILE_MAX_SIZE];
} weapon_node;

/* Target node for storing target data */
typedef struct target_node_t {
    int target_score;
    dbref target_dbref;
} target_node;

/* Quick flags for use with the various autopilot
 * commands.  Check the ACOM array in autopilot_commands.c */
enum {
    GOAL_CHASETARGET,   /* An extension of follow for chasetarget */
    GOAL_DUMBFOLLOW,
    GOAL_DUMBGOTO,
    GOAL_ENTERBASE,     /* Revamp of enterbase so it keeps trying */
    GOAL_FOLLOW,        /* Uses the new Astar system */
    GOAL_GOTO,          /* Uses the new Astar system */ 
    GOAL_LEAVEBASE,
    GOAL_OLDGOTO,       /* Old implementation of goto */
    GOAL_ROAM,          /* New version using Astar */
    GOAL_WAIT,          /* unimplemented */

    COMMAND_ATTACKLEG,  /* unimplemented */
    COMMAND_AUTOGUN,    /* New version that has 3 options 'on', 'off' and 'target dbref' */
    COMMAND_CHASEMODE,  /* unimplemented */
    COMMAND_CMODE,      /* unimplemented */
    COMMAND_DROPOFF, 
    COMMAND_EMBARK,
    COMMAND_ENTERBAY,   /* unimplemented */
    COMMAND_JUMP,       /* unimplemented */
    COMMAND_LOAD,       /* unimplemented */
    COMMAND_PICKUP,
    COMMAND_REPORT,     /* unimplemented */
    COMMAND_ROAMMODE,   /* unimplemented */
    COMMAND_SHUTDOWN,
    COMMAND_SPEED,
    COMMAND_STARTUP,
    COMMAND_STOPGUN,    /* unimplemented */
    COMMAND_SWARM,      /* unimplemented */
    COMMAND_SWARMMODE,  /* unimplemented */
    COMMAND_UDISEMBARK,
    COMMAND_UNLOAD,     /* unimplemented */
    AUTO_NUM_COMMANDS
};

/* Function Prototypes will go here */

/* From autopilot_core.c */
void auto_destroy_command_node(command_node *node);
void auto_save_commands(FILE *file, AUTO *autopilot);
void auto_load_commands(FILE *file, AUTO *autopilot);
void auto_delcommand(dbref player, void *data, char *buffer);
void auto_addcommand(dbref player, void *data, char *buffer);
void auto_listcommands(dbref player, void *data, char *buffer);
void auto_eventstats(dbref player, void *data, char *buffer);
void auto_set_comtitle(AUTO *autopilot, MECH * mech);
void auto_init(AUTO *autopilot, MECH *mech);
void auto_engage(dbref player, void *data, char *buffer);
void auto_disengage(dbref player, void *data, char *buffer);
void auto_goto_next_command(AUTO *autopilot, int time);
char *auto_get_command_arg(AUTO *autopilot, int command_number, int arg_number);
int auto_get_command_enum(AUTO *autopilot, int command_number);
void auto_newautopilot(dbref key, void **data, int selector);

/* From autopilot_commands.c */
void auto_cal_mapindex(MECH *mech);
void auto_set_chasetarget_mode(AUTO *autopilot, int mode);
void auto_command_startup(AUTO *autopilot, MECH *mech);
void auto_command_shutdown(AUTO *autopilot, MECH *mech);
void auto_command_pickup(AUTO *autopilot, MECH *mech);
void auto_command_dropoff(MECH *mech);
void auto_command_speed(AUTO *autopilot);
void auto_com_event(MUXEVENT *muxevent);
void auto_astar_goto_event(MUXEVENT *muxevent);
void auto_astar_follow_event(MUXEVENT *muxevent);
void auto_dumbgoto_event(MUXEVENT *muxevent);
void auto_dumbfollow_event(MUXEVENT *muxevent);
void auto_leave_event(MUXEVENT *muxevent);
void auto_enter_event(MUXEVENT *muxevent);
void auto_command_roam(AUTO *autopilot, MECH *mech);
void auto_astar_roam_event(MUXEVENT *muxevent);

/* From autopilot_ai.c */
int auto_astar_generate_path(AUTO *autopilot, MECH *mech, short end_x, short end_y);
void auto_destroy_astar_path(AUTO *autopilot);

/* From autopilot_autogun.c */
void auto_update_profile(AUTO *autopilot, MECH *mech);
int SearchLightInRange(MECH *mech, MAP *map);
int PrefVisSens(MECH *mech, MAP *map, int slite, MECH *target);
void auto_sensor_event(AUTO * muxevent);
void auto_gun(AUTO *AUTOPILOT, MECH *mech, MAP *map);
void auto_destroy_weaplist(AUTO *autopilot);

/* From autopilot_radio.c */
void auto_reply_event(MUXEVENT *muxevent);
void auto_reply(MECH *mech, char *buf);
void auto_parse_command(AUTO *autopilot, MECH *mech, int chn, char *buffer);

void auto_radio_command_autogun(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_chasetarg(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_dfollow(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_dgoto(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_dropoff(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_embark(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_enterbase(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_follow(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_goto(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_heading(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_help(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_hide(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_jumpjet(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_leavebase(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_ogoto(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_pickup(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_position(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_prone(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_report(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_reset(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_sensor(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_shutdown(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_speed(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_stand(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_startup(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_stop(AUTO *autopilot, MECH *mech, 
        char **args, int argc, char *mesg);
void auto_radio_command_sweight(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);
void auto_radio_command_target(AUTO *autopilot, MECH *mech,
        char **args, int argc, char *mesg);

#include "p.autopilot.h"
#include "p.ai.h"
#include "p.autopilot_command.h"
#include "p.autopilot_commands.h"
#include "p.autogun.h"

#endif                  /* AUTOPILOT_H */
