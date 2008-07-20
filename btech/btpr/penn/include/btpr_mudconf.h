#ifndef BTPR__MUDCONF_H
#define BTPR__MUDCONF_H

typedef struct confdata CONFDATA;

struct confdata {
    char hcode_db[128];		/* Hardcode stuff */
#ifdef BT_ADVANCED_ECON
    char econ_db[128];		/* Hardcode Econ stuff */
#endif /* BT_ADVANCED_ECON */
    char mech_db[128];		/* Mecha templates */
    char map_db[128];		/* Map templates */
    int btech_explode_reactor;	/* Allow or disallow explode reactor */
    int btech_explode_ammo;	/* Allow or disallow explode ammo */
    int btech_explode_stop;	/* Allow or disallow explode stop */
    int btech_explode_time;	/* Number of tics self-destruction takes */
    int btech_ic;		/* Allow or disallow MechWarrior embark/disembark */
    int btech_parts;
    int btech_vcrit;		/* If below 2, vehicles don't crit at all. */
    int btech_slowdown;
    int btech_fasaturn;
    int btech_dynspeed;
    int btech_stackpole;        /* Mechs mushroom as a result of triple engine crits */
    int btech_erange;
    int btech_hit_arcs;		/* hit arc rules (see FindAreaHitGroup()) */
    int btech_phys_use_pskill;  /* Use piloting skills for physical attacks */
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
    int btech_newcharge;
    int btech_tl3_charge;
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
    int btech_digbonus;
    int btech_dig_only_fs;
    int btech_xploss;
    int btech_critlevel;	/* percentage of armor left before TAC occurs */
    int btech_tankshield;
    int btech_newstagger;	/* For the new round based stagger */
    int btech_extendedmovemod;	/* Whether to use MaxTech's extended target movement modifiers */
    int btech_stacking;		/* Whether to check for stacking, and how to penalize */
    int btech_stackdamage;	/* Damage modifier for btech_stacking=2 */
    int btech_mw_losmap;	/* Whether MechWarriors always get a losmap */
    int btech_seismic_see_stopped; /* Whether you see stopped mechs on seismic */
    int btech_exile_stun_code;      /* Should we use the Exile Head Hit Stun code */
    int btech_roll_on_backwalk;     /* wheter a piloting roll should be made to walk backwards over elevation changes */
    int btech_usedmechstore;	/* DBref for the dead mechs to spool into upon IC death */
    int btech_idf_requires_spotter; /* Requires spotter for IDF firing */
    int btech_vtol_ice_causes_fire; /* VTOL ICE engines cause fire on crash/explosion */
    int btech_glancing_blows; /* 0=Don't, 1=maxtech (BTH) , 2= Exile (BTH-1) */
    int btech_inferno_penalty;    /* FASA Inferno Ammo penalty (+30 heat, ammo explode) */
    int btech_perunit_xpmod;	/* Allow per unit xp modifications */
    int btech_tsm_tow_bonus;	/* Give bonus to TSM units when towing, similiar to salvage tech (1=Yes, 0=No) */
    int btech_tsm_sprint_bonus;	/* 0= sprint and tsm don't stack 1= stack sprint and tsm */
    int btech_heatcutoff;       /* 0= Don't allow use of the heatcutoff command */
    int btech_sprint_bth;	/* set to appropriate BTH on sprinting units (-4 is default) */
    int btech_cost_debug;	/* 1= Send info for btfasabasecost to MechDebugInfo channel */
#ifdef HUDINFO_SUPPORT
    int hudinfo_show_mapinfo;	/* What kind of info we are willing to give */
#endif
    int afterlife_dbref;
}; /* struct confdata */

extern CONFDATA mudconf;


typedef struct statedata STATEDATA;

struct statedata {
    time_t now;			/* What time is it now? */
    time_t restart_time;	/* When was MUX (re-)started */

    char *debug_cmd;		/* FIXME: Just a dummy variable, for now */
}; /* struct statedata */

extern STATEDATA mudstate;

#endif /* undef BTPR__MUDCONF_H */
