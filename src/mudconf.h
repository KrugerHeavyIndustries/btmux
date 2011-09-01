/* mudconf.h */

#include "config.h"

#ifndef __MUDCONF_H
#define __MUDCONF_H

#include <netinet/in.h>
#include "config.h"
#include "htab.h"
#include "alloc.h"
#include "flags.h"
#include "mail.h"
#include "db.h"
#include "pcache.h"
#include "cque.h"
#include "rbtree.h"

/* CONFDATA:	runtime configurable parameters */

typedef unsigned char Uchar;

typedef struct confdata CONFDATA;
struct confdata {
    int cache_trim;		/* Should cache be shrunk to original size */
    int cache_steal_dirty;	/* Should cache code write dirty attrs */
    int cache_depth;		/* Number of entries in each cache cell */
    int cache_width;		/* Number of cache cells */
    int cache_names;		/* Should object names be cached separately */
    char indb[128];		/* database file name */
    char outdb[128];		/* checkpoint the database to here */
    char crashdb[128];		/* write database here on crash */
    char gdbm[128];		/* use this gdbm file if we need one */
    char mail_db[128];		/* name of the @mail database */
    char commac_db[128];	/* name of the comsys and macros db */
    char hcode_db[128];		/* Hardcode stuff */
#ifdef BT_ADVANCED_ECON
    char econ_db[128];		/* Hardcode Econ stuff */
#endif
    char mech_db[128];		/* Mecha templates */
    char map_db[128];		/* Map templates */
    char config_file[128];	/* name of config file, used by @restart */
    int compress_db;		/* should we use compress */
    char compress[128];		/* program to run to compress */
    char uncompress[128];	/* program to run to uncompress */
    char status_file[128];	/* Where to write arg to @shutdown */
    int have_specials;		/* Should the special hcode be active? */
    int have_comsys;		/* Should the comsystem be active? */
    int have_macros;		/* Should the macro system be active? */
    int have_mailer;		/* Should @mail be active? */
    int have_zones;		/* Should zones be active? */
    int port;			/* user port */
    int conc_port;		/* concentrator port */
    int init_size;		/* initial db size */
    int have_guest;		/* Do we wish to allow a GUEST character? */
    int guest_char;		/* player num of prototype GUEST character */
    int guest_nuker;		/* Wiz who nukes the GUEST characters. */
    int number_guests;		/* number of guest characters allowed */
    char guest_prefix[32];	/* Prefix for the guest char's name */
    char guest_file[32];	/* display if guest connects */
    char conn_file[32];		/* display on connect if no registration */
    char conn_dir[32];		/* display on connect if no registration */
    char creg_file[32];		/* display on connect if registration */
    char regf_file[32];		/* display on (failed) create if reg is on */
    char motd_file[32];		/* display this file on login */
    char wizmotd_file[32];	/* display this file on login to wizards */
    char quit_file[32];		/* display on quit */
    char down_file[32];		/* display this file if no logins */
    char full_file[32];		/* display when max users exceeded */
    char site_file[32];		/* display if conn from bad site */
    char crea_file[32];		/* display this on login for new users */
    char help_file[32];		/* HELP text file */
    char help_indx[32];		/* HELP index file */
    char news_file[32];		/* NEWS text file */
    char news_indx[32];		/* NEWS index file */
    char whelp_file[32];	/* Wizard help text file */
    char whelp_indx[32];	/* Wizard help index file */
    char plushelp_file[32];	/* +help text file */
    char plushelp_indx[32];	/* +help index file */
    char wiznews_file[32];	/*  wiznews text file */
    char wiznews_indx[32];	/*  wiznews index file */
    char motd_msg[4096];	/* Wizard-settable login message */
    char wizmotd_msg[4096];	/* Login message for wizards only */
    char downmotd_msg[4096];	/* Settable 'logins disabled' message */
    char fullmotd_msg[4096];	/* Settable 'Too many players' message */
    char dump_msg[128];		/* Message displayed when @dump-ing */
    char postdump_msg[128];	/* Message displayed after @dump-ing */
    char fixed_home_msg[128];	/* Message displayed when going home and FIXED */
    char fixed_tel_msg[128];	/* Message displayed when teleporting and FIXED */
    char public_channel[32];	/* Name of public channel */
    char guests_channel[32];	/* Name of guests channel */
    int allow_unloggedwho;	/* Wether or not to allow unlogged-in clients to use WHO, DOING and SESSION */
    int btech_explode_reactor;	/* Allow or disallow explode reactor */
    int btech_explode_ammo;	/* Allow or disallow explode ammo */
    int btech_explode_stop;	/* Allow or disallow explode stop */
    int btech_explode_time;	/* Number of tics self-destruction takes */
    int btech_ic;		/* Allow or disallow MechWarrior embark/disembark */
    int btech_parts;		/* If 1, enable part brands. */
    int btech_vcrit;		/* If below 2, vehicles don't crit at all. */
    int btech_slowdown;		/* If 2, new slowdown code based on desired/current heading. 1 is a flat reduction. 0 is no slowdown. */
    int btech_fasaturn;
    int btech_dynspeed;		/* Factor in section loss and gravity for mech speed. */
    int btech_stackpole;        /* Mechs mushroom as a result of triple engine crits */
    int btech_erange;		/* 1= Enable extended, extended weapon ranges. */
    int btech_hit_arcs;		/* hit arc rules (see FindAreaHitGroup()) */
    int btech_phys_use_pskill;  /* Use piloting skills for physical attacks */
    int registeredonly;		/* Only all logins with the REGISTERED flag. Middle ground between @disable logins and Full Allow */
    int namechange_days;
    int allow_chanlurking;	/* 'last' and 'who' on 'off' channels ? */
    int btech_newterrain;	/* fasa terrain restrictions for wheeled */
    int btech_fasacrit;		/* fasa critsystem */
    int btech_fasaadvvtolcrit;	/* L3 FASA VTOL crits and hitlocs */
    int btech_fasaadvvhlcrit;	/* L3 FASA ground vehicle crits and hitlocs */
    int btech_fasaadvvhlfire;	/* L3 FASA ground vehicle fire rules */
    int btech_divrotordamage;	/* amount to divide damage to vtol rotors by. Instead of just 1 pt per hit, we'll divide the damage by something */
    int btech_moddamagewithrange;	/* For energy weapons: +1 damage at <=1 hex, -1 damage at long range */
    int btech_moddamagewithwoods;	/* Occupied woods do not add to BTH but lessen damage. -2 for light, -4 for heavy, chance to clear on each shot. Intervening woods don't change. */
    int btech_hotloadaddshalfbthmod;	/* Mod the BTH for hotloaded LRMs. The BTH mod is half what it is if not hotloaded */
    int btech_nofusionvtolfuel;	/* Fusion engine'd VTOLs don't use fuel */
    int btech_vhltacthreshold;	/* threshold for vehicle TACs. If it's <= 0, we ignore any armor level and do normal tacs, else we only TAC if the armor percent is <= to the value. */
    int btech_mechtacthreshold;	/* threshold for Mech TACs. If it's <= 0, we ignore any armor level and do normal tacs, else we only TAC if the armor percent is <= to the value. */
    int btech_newcharge;	/* 1= use distance counter for charge damage. */
    int btech_tl3_charge;	/* 1= New TL3 damage formula. Takes direction and speed into account. */
    int btech_tankfriendly;	/* Some tank friendly changes if fasacrit is too harsh */
    int btech_skidcliff;	/* skidroll to check for cliffs and falldamage for mechs  */
    int btech_xp_bthmod;	/* Use bth modifier in new xp code */
    int btech_xp_missilemod;	/* Modifier for missile weapons */
    int btech_xp_ammomod;	/* modifier for ammo weapons (not missiles ) */
    int btech_defaultweapdam;	/* modifier to default weapon BV */
    int btech_xp_usePilotBVMod;	/* use the pilot's skills to modify the BV of the unit */
    int btech_xp_modifier;	/* modifier to increase or decrease xp-gain */
    int btech_defaultweapbv;	/* Weapons with BVs higher than this give less xp, lower give more */
    int btech_oldxpsystem;	/* Uses old xp system if 1 */
    int btech_xp_vrtmod;	/* Modifier for VRT weapons used if !0 */
    int btech_limitedrepairs;	/* If on then armor fixes and reloads in stalls only */
    int btech_digbonus;		/* If shot would land on the FS hitgroup of a tank at >= your elevation, add this number to BTH. */
    int btech_dig_only_fs;	/* Make the bonus apply to only shots that would hit the tank's FS hitgroup. */
    int btech_xploss;		/* Percentage of XP to lose on dying. 1000 = none, 0 = all? */
    int btech_critlevel;	/* percentage of armor left before TAC occurs */
    int btech_tankshield;	/* ???: Not really sure. */
    int btech_newstagger;	/* For the new round based stagger */
    int btech_newstaggertons;   /* For the new stagger tonnage mod (0 turns off stagger based on tonnage) */
    int btech_newstaggertime;  	/* Time between stagger checks */
    int btech_extendedmovemod;	/* Whether to use MaxTech's extended target movement modifiers */
    int btech_stacking;		/* Whether to check for stacking, and how to penalize */
    int btech_stackdamage;	/* Damage modifier for btech_stacking=2 */
    int btech_mw_losmap;	/* Whether MechWarriors always get a losmap */
    int btech_seismic_see_stopped; /* Whether you see stopped mechs on seismic */
    int btech_exile_stun_code;      /* Should we use the Exile Head Hit Stun code. Set to 2 to not actually stun, but reassign headhits */
    int btech_roll_on_backwalk;     /* wheter a piloting roll should be made to walk backwards over elevation changes */
    int btech_usedmechstore;	/* DBref for the dead mechs to spool into upon IC death */
    int btech_ooc_comsys;	/* Enable bypassing of CA_NO_IC command checks for IC location blocking */
    int btech_idf_requires_spotter; /* Requires spotter for IDF firing */
    int btech_vtol_ice_causes_fire; /* VTOL ICE engines cause fire on crash/explosion */
    int btech_glancing_blows; /* 0=Don't, 1=maxtech (BTH) , 2= Exile (BTH-1) */
    int btech_inferno_penalty;    /* FASA Inferno Ammo penalty (+30 heat, ammo explode) */
    int btech_perunit_xpmod;	/* Allow per unit xp modifications */
    int btech_tsm_tow_bonus;    /* Give bonus to TSM units when towing, similiar to salvage tech (1=Yes, 0=No) */
    int btech_tsm_sprint_bonus; /* 0= sprint and tsm don't stack 1= stack sprint and tsm */
    int btech_heatcutoff;	/* 0= The 'heatcutoff' command is disabled. */
    int btech_sprint_bth;	/* BTH to give for attacks against sprinting units (default is -4)*/
    int btech_cost_debug;	/* 1= Send info from btfasabasecost to MechDebugInfo */
    int btech_noisy_xpgain;	/* 1 = Send XP Gain info to MechXP */
    int btech_xpgain_cap;	/* Cap for Weapons XP Gain */
    int btech_transported_unit_death; /* 1=Destroy units in a transport automatically. (Via AMECHDEST) 0=Don't. */
    int btech_mwpickup_action;  /* 0 = 3030 style (Slave MW, @unlock, quiet @tel), 1 = TELE_LOUD (Triggers aenter), no slave, no @unlock */
    int btech_standcareful;     /* 0 = Don't allow (FASA rules), 1 Allow (-2 BTH to stand, double time if successful */
    int btech_maxtechtime;      /* Max Tech Time allowed, in minutes */
    int btech_blzmapmode;    /* 0 = Regular tacmap for non-aeros <blzs with terrain blockouts> (Default and usual), 1 = Just BLZs <no terrain blockers> */
    int btech_extended_piloting; /* 0 = No (Drive, Piloting-Bmech, etc) 1 = Yes (Piloting-Tracked, Piloting-Biped, etc.) */
    int btech_extended_gunnery; /* 0 = No (Gunnery-Bmech, Gunnery-Aero, etc) 1 = Yes (Gunnery-Laser, Gunnery-Missile, etc.) */
    int btech_xploss_for_mw;	 /* 0 = No (Mechwarrior death !=XP Loss.) 1 = Yes (Default. MW death = XPLOSS if IC) */
    int btech_variable_techtime; /* 0 = No (Normal Techtime) 1 = Yes (Techtime is modded by btech_techtime_mod per roll above tofix) */
    int btech_techtime_mod; /* BTH: 6, Roll 7 would be 1 * techtime_mod , BTH 6, Roll 8, would 2 be 2 *, etc. Lowers techtime based on 'good' rolls */
    int btech_statengine_obj; /* Object to send stats on hits/crits to. Defaults to -1 (off) */
#ifdef BT_FREETECHTIME
    int btech_freetechtime;	/* Near instant repair times */
#endif
#ifdef BT_COMPLEXREPAIRS
    int btech_complexrepair;
#endif
#ifdef HUDINFO_SUPPORT
    int hudinfo_show_mapinfo;	/* What kind of info we are willing to give */
    int hudinfo_enabled;	/* At runtime turn HUD on and off */
#endif
    int afterlife_dbref;
    int indent_desc;		/* Newlines before and after descs? */
    int name_spaces;		/* allow player names to have spaces */
    int show_unfindable_who;    /* should players set UNFINDABLE appear on who? */
    int site_chars;		/* where to truncate site name */
    int fork_dump;		/* perform dump in a forked process */
    int fork_vfork;		/* use vfork to fork */
    int sig_action;		/* What to do with fatal signals */
    int paranoid_alloc;		/* Rigorous buffer integrity checks */
    int max_players;		/* Max # of connected players */
    int dump_interval;		/* interval between ckp dumps in seconds */
    int check_interval;		/* interval between db check/cleans in secs */
    int events_daily_hour;	/* At what hour should @daily be executed? */
    int dump_offset;		/* when to take first checkpoint dump */
    int check_offset;		/* when to perform first check and clean */
    int idle_timeout;		/* Boot off players idle this long in secs */
    int conn_timeout;		/* Allow this long to connect before booting */
    int idle_interval;		/* when to check for idle users */
    int retry_limit;		/* close conn after this many bad logins */
    int output_limit;		/* Max # chars queued for output */
    int paycheck;		/* players earn this much each day connected */
    int paystart;		/* new players start with this much money */
    int paylimit;		/* getting money gets hard over this much */
    int start_quota;		/* Quota for new players */
    int payfind;		/* chance to find a penny with wandering */
    int digcost;		/* cost of @dig command */
    int linkcost;		/* cost of @link command */
    int opencost;		/* cost of @open command */
    int createmin;		/* default (and minimum) cost of @create cmd */
    int createmax;		/* max cost of @create command */
    int killmin;		/* default (and minimum) cost of kill cmd */
    int killmax;		/* max cost of kill command */
    int killguarantee;		/* cost of kill cmd that guarantees success */
    int robotcost;		/* cost of @robot command */
    int pagecost;		/* cost of @page command */
    int searchcost;		/* cost of commands that search the whole DB */
    int waitcost;		/* cost of @wait (refunded when finishes) */
    int mail_expiration;	/* Number of days to wait to delete mail */
    int use_http;		/* Should we allow http access? */
    int queuemax;		/* max commands a player may have in queue */
    int queue_chunk;		/* # cmds to run from queue when idle */
    int active_q_chunk;		/* # cmds to run from queue when active */
    int machinecost;		/* One in mc+1 cmds costs 1 penny (POW2-1) */
    int room_quota;		/* quota needed to make a room */
    int exit_quota;		/* quota needed to make an exit */
    int thing_quota;		/* quota needed to make a thing */
    int player_quota;		/* quota needed to make a robot player */
    int sacfactor;		/* sacrifice earns (obj_cost/sfactor) + sadj */
    int sacadjust;		/* ... */
    int clone_copy_cost;	/* Does @clone copy value? */
    int use_hostname;		/* TRUE = use machine NAME rather than quad */
    int quotas;			/* TRUE = have building quotas */
    int ex_flags;		/* TRUE = show flags on examine */
    int robot_speak;		/* TRUE = allow robots to speak */
    int pub_flags;		/* TRUE = flags() works on anything */
    int quiet_look;		/* TRUE = don't see attribs when looking */
    int exam_public;		/* Does EXAM show public attrs by default? */
    int read_rem_desc;		/* Can the DESCs of nonlocal objs be read? */
    int read_rem_name;		/* Can the NAMEs of nonlocal objs be read? */
    int sweep_dark;		/* Can you sweep dark places? */
    int player_listen;		/* Are AxHEAR triggered on players? */
    int quiet_whisper;		/* Can others tell when you whisper? */
    int dark_sleepers;		/* Are sleeping players 'dark'? */
    int see_own_dark;		/* Do you see your own dark stuff? */
    int idle_wiz_dark;		/* Do idling wizards get set dark? */
    int pemit_players;		/* Can you @pemit to faraway players? */
    int pemit_any;		/* Can you @pemit to ANY remote object? */
    int match_mine;		/* Should you check yourself for $-commands? */
    int match_mine_pl;		/* Should players check selves for $-cmds? */
    int switch_df_all;		/* Should @switch match all by default? */
    int fascist_tport;		/* Src of teleport must be owned/JUMP_OK */
    int terse_look;		/* Does manual look obey TERSE */
    int terse_contents;		/* Does TERSE look show exits */
    int terse_exits;		/* Does TERSE look show obvious exits */
    int terse_movemsg;		/* Show move msgs (SUCC/LEAVE/etc) if TERSE? */
    int trace_topdown;		/* Is TRACE output top-down or bottom-up? */
    int trace_limit;		/* Max lines of trace output if top-down */
    int stack_limit;		/* How big can stacks get? */
    int safe_unowned;		/* Are objects not owned by you safe? */
    int space_compress;		/* Convert multiple spaces into one space */
    int start_room;		/* initial location and home for players */
    int start_home;		/* initial HOME for players */
    int default_home;		/* HOME when home is inaccessable */
    int master_room;		/* Room containing default cmds/exits/etc */
    FLAGSET player_flags;	/* Flags players start with */
    FLAGSET room_flags;		/* Flags rooms start with */
    FLAGSET exit_flags;		/* Flags exits start with */
    FLAGSET thing_flags;	/* Flags things start with */
    FLAGSET robot_flags;	/* Flags robots start with */
    int vattr_flags;		/* Attr flags for all user-defined attrs */
    char mud_name[32];		/* Name of the mud */
    char one_coin[32];		/* name of one coin (ie. "penny") */
    char many_coins[32];	/* name of many coins (ie. "pennies") */
    int timeslice;		/* How often do we bump people's cmd quotas? */
    int cmd_quota_max;		/* Max commands at one time */
    int cmd_quota_incr;		/* Bump #cmds allowed by this each timeslice */
    int control_flags;		/* Global runtime control flags */
    int log_options;		/* What gets logged */
    int log_info;		/* Info that goes into log entries */
    Uchar markdata[8];		/* Masks for marking/unmarking */
    int func_nest_lim;		/* Max nesting of functions */
    int func_invk_lim;		/* Max funcs invoked by a command */
    int ntfy_nest_lim;		/* Max nesting of notifys */
    int lock_nest_lim;		/* Max nesting of lock evals */
    int parent_nest_lim;	/* Max levels of parents */
    int zone_nest_lim;		/* Max nesting of zones */
#ifdef SQL_SUPPORT
    int sqlDB_init_A;
    char sqlDB_type_A[128];     /* Currently only 'MySQL' */
    char sqlDB_hostname_A[128]; /* Usually localhost */
    char sqlDB_username_A[128]; /* SQL user */
    char sqlDB_password_A[128]; /* SQL user's password */
    char sqlDB_dbname_A[128];   /* Name of the DB to use */
    int sqlDB_init_B;
    char sqlDB_type_B[128];
    char sqlDB_hostname_B[128];
    char sqlDB_username_B[128];
    char sqlDB_password_B[128];
    char sqlDB_dbname_B[128];
    int sqlDB_init_C;
    char sqlDB_type_C[128];
    char sqlDB_hostname_C[128];
    char sqlDB_username_C[128];
    char sqlDB_password_C[128];
    char sqlDB_dbname_C[128];
    int sqlDB_init_D;
    char sqlDB_type_D[128];
    char sqlDB_hostname_D[128];
    char sqlDB_username_D[128];
    char sqlDB_password_D[128];
    char sqlDB_dbname_D[128];
    int sqlDB_init_E;
    char sqlDB_type_E[128];
    char sqlDB_hostname_E[128];
    char sqlDB_username_E[128];
    char sqlDB_password_E[128];
    char sqlDB_dbname_E[128];
    char sqlDB_sqlite_dbdir[128];
    char sqlDB_mysql_socket[128];
    int sqlDB_max_queries;
#endif
    int room_parent;
    int exit_parent;
    int player_parent;
    int player_zone;
};

extern CONFDATA mudconf;

typedef struct site_data SITE;
struct site_data {
    struct site_data *next;	/* Next site in chain */
    struct in_addr address;	/* Host or network address */
    struct in_addr mask;	/* Mask to apply before comparing */
    int flag;			/* Value to return on match */
};

typedef struct objlist_block OBLOCK;
struct objlist_block {
    struct objlist_block *next;
    dbref data[(LBUF_SIZE - sizeof(OBLOCK *)) / sizeof(dbref)];
};

#define OBLOCK_SIZE ((LBUF_SIZE - sizeof(OBLOCK *)) / sizeof(dbref))

typedef struct objlist_stack OLSTK;
struct objlist_stack {
        struct objlist_stack *next;	/* Next object list in stack */
        OBLOCK  *head;		/* Head of object list */
        OBLOCK  *tail;		/* Tail of object list */
        OBLOCK  *cblock;	/* Current block for scan */
        int     count;		/* Number of objs in last obj list block */
        int     citm;		/* Current item for scan */
};

typedef struct markbuf MARKBUF;
struct markbuf {
    char chunk[5000];
};

typedef struct alist ALIST;
struct alist {
    char *data;
    int len;
    struct alist *next;
};

typedef struct badname_struc BADNAME;
struct badname_struc {
    char *name;
    struct badname_struc *next;
};

typedef struct forward_list FWDLIST;
struct forward_list {
    int count;
    int data[1000];
};

typedef struct statedata STATEDATA;
struct statedata {
    int record_players;		/* The maximum # of player logged on */
    int initializing;		/* are we reading config file at startup? */
    int panicking;		/* are we in the middle of dying horribly? */
    int restarting;		/* Are we restarting? */
    int dumping;		/* Are we dumping? */
    int logging;		/* Are we in the middle of logging? */
    int epoch;			/* Generation number for dumps */
    int generation;		/* DB global generation number */
    dbref curr_enactor;		/* Who initiated the current command */
    dbref curr_player;		/* Who is running the current command */
    int alarm_triggered;	/* Has periodic alarm signal occurred? */
    time_t now;			/* What time is it now? */
    time_t dump_counter;	/* Countdown to next db dump */
    time_t check_counter;	/* Countdown to next db check */
    time_t idle_counter;	/* Countdown to next idle check */
    time_t mstats_counter;	/* Countdown to next mstats snapshot */
    time_t events_counter;	/* Countdown to next events check */
    int events_flag;		/* Flags for check_events */
    int events_lasthour;	/* Last hour we ran hourly maintenance */

    int shutdown_flag;		/* Should interface be shut down? */
    char version[256];		/* MUX version string */
    time_t start_time;		/* When was MUX started */
    time_t restart_time;	/* When was MUX (re-)started */
    char buffer[256];		/* A buffer for holding temp stuff */
    char *debug_cmd;		/* The command we are executing (if any) */
    char doing_hdr[41];		/* Doing column header in the WHO display */
    SITE *access_list;		/* Access states for sites */
    SITE *suspect_list;		/* Sites that are suspect */
    HASHTAB command_htab;	/* Commands hashtable */
    HASHTAB macro_htab;		/* Macro command hashtable */
    HASHTAB channel_htab;	/* Channels hashtable */
    NHSHTAB mail_htab;		/* Mail players hashtable */
    HASHTAB logout_cmd_htab;	/* Logged-out commands hashtable (WHO, etc) */
    HASHTAB func_htab;		/* Functions hashtable */
    HASHTAB ufunc_htab;		/* Local functions hashtable */
    HASHTAB powers_htab;	/* Powers hashtable */
    HASHTAB flags_htab;		/* Flags hashtable */
    HASHTAB attr_name_htab;	/* Attribute names hashtable */
    HASHTAB vattr_name_htab;	/* User attribute names hashtable */
    HASHTAB player_htab;	/* Player name->number hashtable */
    rbtree desctree;
    NHSHTAB fwdlist_htab;	/* Room forwardlists */
    NHSHTAB parent_htab;	/* Parent $-command exclusion */
#ifdef PARSE_TREES
    NHSHTAB tree_htab;		/* Parse trees for evaluation */
#endif
    HASHTAB news_htab;		/* News topics hashtable */
    HASHTAB help_htab;		/* Help topics hashtable */
    HASHTAB wizhelp_htab;	/* Wizard help topics hashtable */
    HASHTAB plushelp_htab;	/* +help topics hashtable */
    HASHTAB wiznews_htab;	/* wiznews topics hashtable */
    int attr_next;		/* Next attr to alloc when freelist is empty */
    OBJQE *qhead;        /* Per Object Queue Entries */
    OBJQE *qtail;        
    BQUE *qwait;		/* Head of wait queue */
    BQUE *qsemfirst;		/* Head of semaphore queue */
    BQUE *qsemlast;		/* Tail of semaphore queue */
    BADNAME *badname_head;	/* List of disallowed names */
    int mstat_ixrss[2];		/* Summed shared size */
    int mstat_idrss[2];		/* Summed private data size */
    int mstat_isrss[2];		/* Summed private stack size */
    int mstat_secs[2];		/* Time of samples */
    int mstat_curr;		/* Which sample is latest */
    ALIST iter_alist;		/* Attribute list for iterations */
    char *mod_alist;		/* Attribute list for modifying */
    int mod_size;		/* Length of modified buffer */
    dbref mod_al_id;		/* Where did mod_alist come from? */
    OLSTK *olist;		/* Stack of object lists for nested searches */
    dbref freelist;		/* Head of object freelist */
    int min_size;		/* Minimum db size (from file header) */
    int db_top;			/* Number of items in the db */
    int db_size;		/* Allocated size of db structure */
    int db_revision;     /* database revision */
    int mail_freelist;		/* The next free mail number */
    int mail_db_top;		/* Like db_top */
    int mail_db_size;		/* Like db_size */
    MENT *mail_list;		/* The mail database */
    int *guest_free;		/* Table to keep track of free guests */
    MARKBUF *markbits;		/* temp storage for marking/unmarking */
    int func_nest_lev;		/* Current nesting of functions */
    int func_invk_ctr;		/* Functions invoked so far by this command */
    int ntfy_nest_lev;		/* Current nesting of notifys */
    int lock_nest_lev;		/* Current nesting of lock evals */
    char *global_regs[MAX_GLOBAL_REGS];	/* Global registers */
    int zone_nest_num;		/* Global current zone nest position */
    int inpipe;			/* Boolean flag for command piping */
    char *pout;			/* The output of the pipe used in %| */
    char *poutnew;		/* The output being build by the current command */
    char *poutbufc;		/* Buffer position for poutnew */
    dbref poutobj;		/* Object doing the piping */
    char *executable_path;
};

extern STATEDATA mudstate;

/* Global flags */

/* Game control flags in mudconf.control_flags */

#define	CF_LOGIN	0x0001	/* Allow nonwiz logins to the mux */
#define CF_BUILD	0x0002	/* Allow building commands */
#define CF_INTERP	0x0004	/* Allow object triggering */
#define CF_CHECKPOINT	0x0008	/* Perform auto-checkpointing */
#define	CF_DBCHECK	0x0010	/* Periodically check/clean the DB */
#define CF_IDLECHECK	0x0020	/* Periodically check for idle users */

/* empty		0x0040 */

/* empty		0x0080 */
#define CF_DEQUEUE	0x0100	/* Remove entries from the queue */
#define CF_EVENTCHECK   0x0200	/* Allow events checking */

/* Host information codes */

#define H_REGISTRATION	0x0001	/* Registration ALWAYS on */
#define H_FORBIDDEN	0x0002	/* Reject all connects */
#define H_SUSPECT	0x0004	/* Notify wizards of connects/disconnects */

/* Event flags, for noting when an event has taken place */

#define ET_DAILY	0x00000001	/* Daily taken place? */

/* Logging options */

#define LOG_ALLCOMMANDS	0x00000001	/* Log all commands */
#define LOG_ACCOUNTING	0x00000002	/* Write accounting info on logout */
#define LOG_BADCOMMANDS	0x00000004	/* Log bad commands */
#define LOG_BUGS	0x00000008	/* Log program bugs found */
#define LOG_DBSAVES	0x00000010	/* Log database dumps */
#define LOG_CONFIGMODS	0x00000020	/* Log changes to configuration */
#define LOG_PCREATES	0x00000040	/* Log character creations */
#define LOG_KILLS	0x00000080	/* Log KILLs */
#define LOG_LOGIN	0x00000100	/* Log logins and logouts */
#define LOG_NET		0x00000200	/* Log net connects and disconnects */
#define LOG_SECURITY	0x00000400	/* Log security-related events */
#define LOG_SHOUTS	0x00000800	/* Log shouts */
#define LOG_STARTUP	0x00001000	/* Log nonfatal errors in startup */
#define LOG_WIZARD	0x00002000	/* Log dangerous things */
#define LOG_ALLOCATE	0x00004000	/* Log alloc/free from buffer pools */
#define LOG_PROBLEMS	0x00008000	/* Log runtime problems */
#define LOG_SUSPECTCMDS	0x00010000	/* Log commands by people set SUSPECT */
#define LOG_ALWAYS	0x80000000	/* Always log it */

#define LOGOPT_FLAGS		0x01	/* Report flags on object */
#define LOGOPT_LOC		0x02	/* Report loc of obj when requested */
#define LOGOPT_OWNER		0x04	/* Report owner of obj if not obj */
#define LOGOPT_TIMESTAMP	0x08	/* Timestamp log entries */

#define HIDDEN_IDLESECS		600	/* Show people idle for less as 0s idle */

#endif
