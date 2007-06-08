#include "config.h"

#include "mudconf.h"

#include <string.h>

CONFDATA mudconf;
STATEDATA mudstate;

CONFDATA *
btpr_get_mudconf_ptr(void)
{
	return &mudconf;
}

STATEDATA *
btpr_get_mudstate_ptr(void)
{
	return &mudstate;
}

void
btpr_cf_init(void)
{
	/* Fake default configuration.  */
	strcpy(mudconf.hcode_db, "data/hcode.db");
#ifdef BT_ADVANCED_ECON
	strcpy(mudconf.econ_db, "data/econ.db");
#endif /* BT_ADVANCED_ECON */
	strcpy(mudconf.mech_db, "mechs");
	strcpy(mudconf.map_db, "maps");
	mudconf.btech_explode_reactor = 1;
	mudconf.btech_explode_ammo = 1;
	mudconf.btech_explode_stop = 0;
	mudconf.btech_explode_time = 120;
	mudconf.btech_ic = 1;
	mudconf.btech_parts = 1;
	mudconf.btech_vcrit = 2;
	mudconf.btech_slowdown = 2;
	mudconf.btech_fasaturn = 1;
	mudconf.btech_dynspeed = 1;
	mudconf.btech_stackpole = 1;
	mudconf.btech_erange = 1;
	mudconf.btech_hit_arcs = 0;
	mudconf.btech_phys_use_pskill = 1;
	mudconf.btech_newterrain = 0;
	mudconf.btech_fasacrit = 0;
	mudconf.btech_fasaadvvtolcrit = 0;
	mudconf.btech_fasaadvvhlcrit = 0;
	mudconf.btech_fasaadvvhlfire = 0;
	mudconf.btech_divrotordamage = 0;
	mudconf.btech_moddamagewithrange = 0;
	mudconf.btech_moddamagewithwoods = 0;
	mudconf.btech_hotloadaddshalfbthmod = 0;
	mudconf.btech_nofusionvtolfuel = 0;
	mudconf.btech_tankfriendly = 0;
	mudconf.btech_skidcliff = 0;
	mudconf.btech_xp_bthmod = 0;
	mudconf.btech_xp_missilemod = 100;
	mudconf.btech_xp_ammomod = 100;
	mudconf.btech_defaultweapdam = 5;
	mudconf.btech_xp_modifier = 100;
	mudconf.btech_defaultweapbv = 120;
	mudconf.btech_oldxpsystem = 1;
	mudconf.btech_xp_vrtmod = 0;
	mudconf.btech_limitedrepairs = 0;
	mudconf.btech_digbonus = 3;
	mudconf.btech_dig_only_fs = 0;
	mudconf.btech_xploss = 666;
	mudconf.btech_critlevel = 100;
	mudconf.btech_tankshield = 0;
	mudconf.btech_newstagger = 0;
	mudconf.btech_extendedmovemod = 1;
	mudconf.btech_stacking = 2;
	mudconf.btech_stackdamage = 100;
	mudconf.btech_mw_losmap = 1;
	mudconf.btech_seismic_see_stopped = 0;
	mudconf.btech_exile_stun_code = 0;
	mudconf.btech_roll_on_backwalk = 1;
	mudconf.btech_usedmechstore = 0;
	mudconf.btech_idf_requires_spotter = 1;
	mudconf.btech_vtol_ice_causes_fire = 1;
	mudconf.btech_glancing_blows = 1;
	mudconf.btech_inferno_penalty = 0;
	mudconf.btech_perunit_xpmod = 1;
	mudconf.btech_tsm_tow_bonus = 1;
	mudconf.btech_tsm_sprint_bonus = 1;
	mudconf.btech_sprint_bth = -4;
	mudconf.btech_cost_debug = 0;
#ifdef HUDINFO_SUPPORT
	mudconf.hudinfo_show_mapinfo = 0;
#endif /* HUDINFO_SUPPORT */
	mudconf.afterlife_dbref = 220;
}
