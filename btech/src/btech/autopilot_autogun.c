
/*
 * $Id: autogun.c,v 1.5 2005/08/03 21:40:54 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Sun Nov 17 13:23:20 1996 fingon
 * Last modified: Sun Jun 14 16:29:44 1998 fingon
 *
 */

/*
 * AI's sensor and targeting system
 *
 * Gives the AI the ability to choose what to shoot as well as
 * what sensor to use when it has found a target
 */

/*! \todo { Add more code to the sensor system so the AI can be
 * more aware of its terrain } */

#include "mech.h"
#include "mech.events.h"
#include "autopilot.h"
#include "failures.h"
#include "mech.sensor.h"
#include "p.autogun.h"
#include "p.bsuit.h"
#include "p.glue.h"
#include "p.mech.sensor.h"
#include "p.mech.utils.h"
#include "p.mech.physical.h"
#include "p.mech.combat.h"
#include "p.mech.advanced.h"
#include "p.mech.bth.h"

#if 0
static char *my2string(const char *old)
{
	static char new[64];

	strncpy(new, old, 63);
	new[63] = '\0';
	return new;
}
#endif

/* Function to determine if there are any slites affecting the AI */
int SearchLightInRange(MECH * mech, MAP * map)
{

	MECH *target;
	int i;

	/* Make sure theres a valid mech or map */
	if(!mech || !map)
		return 0;

	/* Loop through all the units on the map */
	for(i = 0; i < map->first_free; i++) {

		/* No units on the map */
		if(!(target = FindObjectsData(map->mechsOnMap[i])))
			continue;

		/* The unit doesn't have slite on */
		if(!(MechSpecials(target) & SLITE_TECH)
		   || MechCritStatus(mech) & SLITE_DEST)
			continue;

		/* Is the mech close enough to be affected by the slite */
		if(FaMechRange(target, mech) < LITE_RANGE) {

			/* Returning true, but let's differentiate also between being in-arc. */
			if((MechStatus(target) & SLITE_ON) &&
			   InWeaponArc(target, MechFX(mech), MechFY(mech)) & FORWARDARC) {

				/* Make sure its in los */
				if(!
				   (map->
					LOSinfo[target->mapnumber][mech->
											   mapnumber] &
					MECHLOSFLAG_BLOCK))

					/* Slite on and, arced, and LoS to you */
					return 3;
				else
					/* Slite on, arced, but LoS blocked */
					return 4;

			} else if(!MechStatus(target) & SLITE_ON &&
					  InWeaponArc(target, MechFX(mech),
								  MechFY(mech)) & FORWARDARC) {

				if(!
				   (map->
					LOSinfo[target->mapnumber][mech->
											   mapnumber] &
					MECHLOSFLAG_BLOCK))

					/* Slite off, arced, and LoS to you */
					return 5;

				else
					/* Slite off, arced, and LoS blocked */
					return 6;
			}

			/* Slite is in range of you, but apparently not arced on you. 
			 * Return tells wether on or off */
			return (MechStatus(target) & SLITE_ON ? 1 : 2);
		}

	}
	return 0;
}

/* Function to determine if the AI should use V or L sensor */
int PrefVisSens(MECH * mech, MAP * map, int slite, MECH * target)
{

	/* No map or mech so use default till we get put somewhere */
	if(!mech || !map)
		return SENSOR_VIS;

	/* Ok the AI is lit or using slite so use V */
	if(MechStatus(mech) & SLITE_ON || MechCritStatus(mech) & SLITE_LIT)
		return SENSOR_VIS;

	/* The target is lit so use V */
	if(target && MechCritStatus(target) & SLITE_LIT)
		return SENSOR_VIS;

	/* Ok if its night/dawn/dusk and theres no slite use L */
	if(map->maplight <= 1 && slite != 3 && slite != 5)
		return SENSOR_LA;

	/* Default sensor */
	return SENSOR_VIS;
}

/*
 * AI event to let the AI decide what sensors to use for a given
 * target and situation
 */
/*! \todo {Improve this so it knows more about the terrain} */
void auto_sensor_event(AUTO *autopilot)
{
	MECH *target = NULL;
	MAP *map;
	char buf[16];
	int wanted_s[2];
	int rvis;
	int slite, prefvis;
	float trng;
	int set = 0;

	if (!Good_obj(autopilot->mymechnum)) {
		dprintk("mymechnum is bad!");
		return;
	}
	if (!Good_obj(autopilot->mynum)) {
		dprintk("mynum is bad!");
		return;
	}
			 
	
	MECH *mech = (MECH *) autopilot->mymech;
	
	/* Make sure its a MECH Xcode Object and the AI is
	 * an AUTOPILOT Xcode Object */
        /* Basic checks */
	if(!mech) {
                dprintk("mech is bad!");
                return;
        }
        if(!autopilot) {
                dprintk("ai is bad!");
                return;
        }
			
	if(!IsMech(mech->mynum) || !IsAuto(autopilot->mynum))
		return;

	/* Mech is dead so stop trying to shoot things */
	if(Destroyed(mech)) {
		DoStopGun(autopilot);
		return;
	}

	/* Mech isn't started */
	if(!Started(mech)) {
		Zombify(autopilot);
		return;
	}

	/* The mech is using user defined sensors so don't try
	 * and change them */
	if(autopilot->flags & AUTOPILOT_LSENS)
		return;

	/* Get the map */
	if(!(map = getMap(mech->mapindex))) {

		/* Bad Map */
		Zombify(autopilot);
		return;
	}
	
	/* Get the target if there is one */
	if(MechTarget(mech) > 0)
		target = getMech(MechTarget(mech));

	/* Checks to see if there is slite, and what types of vis
	 * and which visual sensor (V or L) to use */
	slite = (map->mapvis != 2 ? SearchLightInRange(mech, map) : 0);
	rvis = (map->maplight ? (map->mapvis) : (map->mapvis * (slite ? 1 : 3)));
	prefvis = PrefVisSens(mech, map, slite, target);

	/* Is there a target */
	if(target) {

		/* Range to target */
		trng = FaMechRange(mech, target);

		/* Actually not gonna bother with this */
		/* If the target is running hot and is close switch to IR */
		if(!set && HeatFactor(target) > 35 && (int) trng < 15) {
			//wanted_s[0] = SENSOR_IR;
			//wanted_s[1] = ((MechTons(target) >= 60) ? SENSOR_EM : prefvis);
			//set++;
		}

		/* If the target is BIG and close enough, use EM */
		if(!set && MechTons(target) >= 60 && (int) trng <= 20) {
			wanted_s[0] = SENSOR_EM;
			wanted_s[1] = SENSOR_IR;
			set++;
		}

		/* If the target is flying switch to Radar */
		if(!set && !Landed(target) && FlyingT(target)) {
			wanted_s[0] = SENSOR_RA;
			wanted_s[1] = prefvis;
			set++;
		}

		/* If the target is really close and the unit has BAP, use it */
		if(!set && (int) trng <= 4 && MechSpecials(mech) & BEAGLE_PROBE_TECH
		   && !(MechCritStatus(mech) & BEAGLE_DESTROYED)) {
			wanted_s[0] = SENSOR_BAP;
			wanted_s[1] = SENSOR_BAP;
			set++;
		}

		/* If the target is really close and the unit has Bloodhound, use it */
		if(!set && (int) trng <= 8
		   && MechSpecials2(mech) & BLOODHOUND_PROBE_TECH
		   && !(MechCritStatus(mech) & BLOODHOUND_DESTROYED)) {
			wanted_s[0] = SENSOR_BHAP;
			wanted_s[1] = SENSOR_BHAP;
			set++;
		}

		/* Didn't stop at any of the others so use selected visual sensors */
		if(!set) {
			wanted_s[0] = prefvis;
			wanted_s[1] = (rvis <= 15 ? SENSOR_EM : prefvis);
			set++;
		}

	}

	/* Ok no target and no sensors set yet so lets go for defaults */
	if(!set) {
		if(rvis <= 15) {
			/* Vis is less then or equal to 15 so go to E I for longer range */
			wanted_s[0] = SENSOR_EM;
			wanted_s[1] = SENSOR_IR;
		} else {
			/* Ok lets go with default visual sensors */
			wanted_s[0] = prefvis;
			wanted_s[1] = prefvis;
		}
	}

	/* Check to make sure valid sensors are selected and then set them */
	if(wanted_s[0] >= SENSOR_VIS && wanted_s[0] <= SENSOR_BHAP &&
	   wanted_s[1] >= SENSOR_VIS && wanted_s[1] <= SENSOR_BHAP &&
	   (MechSensor(mech)[0] != wanted_s[0]
		|| MechSensor(mech)[1] != wanted_s[1])) {

		wanted_s[0] = BOUNDED(SENSOR_VIS, wanted_s[0], SENSOR_BHAP);
		wanted_s[1] = BOUNDED(SENSOR_VIS, wanted_s[1], SENSOR_BHAP);

		MechSensor(mech)[0] = wanted_s[0];
		MechSensor(mech)[1] = wanted_s[1];
		mech_notify(mech, MECHALL, "As your sensors change, your lock clears.");
		MechTarget(mech) = -1;
		MarkForLOSUpdate(mech);
	}
}

/*
 * Create a weapon_list node
 */
weapon_node *auto_create_weapon_node(short weapon_number,
									 short weapon_db_number, short section,
									 short critical)
{

	weapon_node *temp;

	temp = malloc(sizeof(weapon_node));

	if(temp == NULL) {
		return NULL;
	}

	memset(temp, 0, sizeof(weapon_node));

	temp->weapon_number = weapon_number;
	temp->weapon_db_number = weapon_db_number;
	temp->section = section;
	temp->critical = critical;

	return temp;

}

/*
 * Destroy weapon node
 */
void auto_destroy_weapon_node(weapon_node * victim)
{

	free(victim);
	return;
}

/*
 * Create a target node for the target list
 */
target_node *auto_create_target_node(int target_score, dbref target_dbref)
{

	target_node *temp;

	temp = malloc(sizeof(target_node));

	if(temp == NULL) {
		return NULL;
	}

	memset(temp, 0, sizeof(target_node));

	temp->target_score = target_score;
	temp->target_dbref = target_dbref;

	return temp;

}

/*
 * Destroy a target node
 */
void auto_destroy_target_node(target_node * victim)
{

	free(victim);
	return;

}

/*
 * Destroy autopilot's weaplist
 */
void auto_destroy_weaplist(AUTO * autopilot)
{

	weapon_node *temp_weapon_node;

	/* Check to make sure there is a weapon list */
	if(!(autopilot->weaplist))
		return;

	/* There is a weapon list - lets kill it */
	if(dllist_size(autopilot->weaplist) > 0) {

		while (dllist_size(autopilot->weaplist)) {
			temp_weapon_node =
				(weapon_node *) dllist_remove_node_at_pos(autopilot->weaplist,
														  1);
			auto_destroy_weapon_node(temp_weapon_node);
		}

	}

	/* Finally destroying the list */
	dllist_destroy_list(autopilot->weaplist);
	autopilot->weaplist = NULL;

}

/*
 * Callback function to destroy target list
 */
static int auto_targets_callback(void *key, void *data, int depth, void *arg)
{

	target_node *temp;

	temp = (target_node *) data;
	auto_destroy_target_node(temp);

	return 1;

}

/*
 * rbtree generic compare function
 */
static int auto_generic_compare(void *a, void *b, void *token)
{

	int *one, *two;

	one = (int *) a;
	two = (int *) b;

	return (*one - *two);
}

/*
 * How we score a given weapon based on range, heat and damage
 */
int auto_calc_weapon_score(int weapon_db_number, int range)
{

	int weapon_score;
	int range_score;
	int damage_score;
	int heat_score;
	int minrange_score;

	int weapon_damage;
	int weapon_heat;

	int i;

	/* Simple Calc */

	/* For the modifiers I assumed best was approx 550
	 *
	 * So for SR, chance of hitting is roughly 92% which is 506 rounded to 500 
	 * For MR, its 72%, so 390 and LR its 41% its 225 */

	/* Assume default values */
	weapon_score = 0;
	weapon_damage = 0;
	range_score = 500;			/* Since by default we assume its SR */
	damage_score = 0;
	heat_score = 0;
	minrange_score = 0;

	/* Don't bother trying to set a value if its outside its range */
	if(range >= MechWeapons[weapon_db_number].longrange) {
		return weapon_score;
	}

	/* Are we at LR ? */
	if(range >= MechWeapons[weapon_db_number].medrange) {
		range_score = 215;
	}

	/* Are we at MR ? */
	if(range >= MechWeapons[weapon_db_number].shortrange &&
	   range < MechWeapons[weapon_db_number].medrange) {
		range_score = 390;
	}

	/* Check min range */
	/* Use a polynomial equation here because at 2 under min its equiv to MR, at
	 * 4 under its equiv to LR, so we want it to balance out the range score */
	/* score = -12.5(min - range)^2 - 25 * (min - range) */
	if(range < MechWeapons[weapon_db_number].min) {
		minrange_score =
			-12.5 * (float) ((MechWeapons[weapon_db_number].min - range) *
							 (MechWeapons[weapon_db_number].min - range)) -
			25.0 * (float) (MechWeapons[weapon_db_number].min - range);
	}

	/* Get the damage for the weapon */
	if(IsMissile(weapon_db_number)) {

		/* Its a missile weapon so lookup in the Missile table get the max
		 * number of missiles it can hit with, and multiply by the damage
		 * per missile */
		/* To make it more fair going to use the avg # of missile hits
		 * which is when they would roll a 7, which becomes slot #
		 * 5 */
		for(i = 0; MissileHitTable[i].key != -1; i++) {
			if(MissileHitTable[i].key == weapon_db_number) {
				weapon_damage = MissileHitTable[i].num_missiles[5] *
					MechWeapons[weapon_db_number].damage;
				break;
			}
		}

	} else {
		weapon_damage = MechWeapons[weapon_db_number].damage;
	}

	/* Get the damage score */
	/* Straight linear plot */
	damage_score = 50 * weapon_damage;

	/* Get the heat */
	weapon_heat = MechWeapons[weapon_db_number].heat;

	/* Get the heat score */
	/* Straight inverse linear plot - more heat bad... */
	heat_score = -25 * weapon_heat + 250;

	/* Final calc */
	weapon_score = range_score + damage_score + heat_score + minrange_score;

	return weapon_score;

}

/*
 * AI profiling event
 *
 * Every so often updates the profile for the AI's weapons
 */
void auto_update_profile_event(AUTO *autopilot) {
	MECH *mech = (MECH *) autopilot->mymech;

	weapon_node *temp_weapon_node;
	dllist_node *temp_dllist_node;

	int section;
	int weapon_count_section;
	unsigned char weaparray[MAX_WEAPS_SECTION];
	unsigned char weapdata[MAX_WEAPS_SECTION];
	int critical[MAX_WEAPS_SECTION];

	int range;

	int weapon_count;
	int weapon_number;

	/* Basic checks */
	/* some accounting checks. try to prevent some race stuff */

	if (!Good_obj(autopilot->mymechnum)) {
		/* most commonly, the mech is a bad memory space.
		 * lets not try to access it
		 */
		dprintk("ap mymechnum is bad");
		StopGun(autopilot);
		return;
	}

	if(!mech) {
		dprintk("mech is bad!");
		return;
	}
	if(!autopilot) {
		dprintk("ai is bad!");
		return;
	}
	if(!IsMech(mech->mynum) || !IsAuto(autopilot->mynum))
		return;

	/* Ok our mech is dead we're done */
	if(Destroyed(mech)) {
		return;
	}
    
	/* Log Message */
	print_autogun_log(autopilot, "Profiling Unit #%d", mech->mynum);

	/* Destroy the arrays first, don't worry about the weap
	 * structures because we can clear them with the ddlist
	 * weaplist */

	/* Zero the array of rbtree stuff */
	for(range = 0; range < AUTO_PROFILE_MAX_SIZE; range++) {

		if(autopilot->profile[range]) {

			/* Destroy rbtree */
			rb_destroy(autopilot->profile[range]);
		}
		autopilot->profile[range] = NULL;
	}

	/* Check to see if the weaplist exists */
	if(autopilot->weaplist != NULL) {

		/* Destroy the list */
		auto_destroy_weaplist(autopilot);
	}

	/* List doesn't exist so lets build it */
	autopilot->weaplist = dllist_create_list();

	/* Reset the AI's max range value for its mech */
	autopilot->mech_max_range = 0;

	/* Set our counter */
	weapon_count = -1;

	/* Now loop through the weapons building a list */
	for(section = 0; section < NUM_SECTIONS; section++) {

		/* Find all the weapons for a given section */
		weapon_count_section = FindWeapons(mech, section,
										   weaparray, weapdata, critical);

		/* No weapons here */
		if(weapon_count_section <= 0)
			continue;

		/* loop through the possible weapons */
		for(weapon_number = 0; weapon_number < weapon_count_section;
			weapon_number++) {

			/* Count it even if its not a valid weapon like AMS */
			/* This is so when we go to fire the weapon we know
			 * which one to send in the command */
			weapon_count++;

			if(IsAMS(weaparray[weapon_number]))
				continue;

			/* Does it work? */
			if(WeaponIsNonfunctional(mech, section, critical[weapon_number],
									 GetWeaponCrits(mech,
													Weapon2I(weaparray
															 [weapon_number])))
			   > 0)
				continue;

			/* Ok made it this far, lets add it to our list */
			temp_weapon_node = auto_create_weapon_node(weapon_count,
													   weaparray
													   [weapon_number],
													   section,
													   critical
													   [weapon_number]);

			temp_dllist_node = dllist_create_node(temp_weapon_node);
			dllist_insert_end(autopilot->weaplist, temp_dllist_node);

			/* Check the max range */
			if(autopilot->mech_max_range <
			   MechWeapons[weaparray[weapon_number]].longrange) {
				autopilot->mech_max_range =
					MechWeapons[weaparray[weapon_number]].longrange;
			}

		}

	}

	/* Now build the profile array, basicly loop through 
	 * all the current avail weapons, get its max range,
	 * then loop through ranges and for each range add it
	 * to profile */

	/* Our counter */
	weapon_number = 1;

	while (weapon_number <= dllist_size(autopilot->weaplist)) {

		/* Get the weapon */
		temp_weapon_node =
			(weapon_node *) dllist_get_node(autopilot->weaplist,
											weapon_number);

		for(range = 0; range <
			MechWeapons[temp_weapon_node->weapon_db_number].longrange;
			range++) {

			/* Out side the the range of AI's profile system */
			if(range >= AUTO_PROFILE_MAX_SIZE) {
				break;
			}

			/* Score the weapon */
			temp_weapon_node->range_scores[range] =
				auto_calc_weapon_score(temp_weapon_node->weapon_db_number,
									   range);

			/* If rbtree for this range doesn't exist, create it */
			if(autopilot->profile[range] == NULL) {
				autopilot->profile[range] =
					rb_init(&auto_generic_compare, NULL);
			}

			/* Check to see if the score exists in the tree
			 * if so alter it slightly so we don't have
			 * overlaping keys */
			while (1) {

				if(rb_exists(autopilot->profile[range],
							 &temp_weapon_node->range_scores[range])) {
					temp_weapon_node->range_scores[range]++;
				} else {
					break;
				}

			}

			/* Add it to tree */
			rb_insert(autopilot->profile[range],
					  &temp_weapon_node->range_scores[range],
					  temp_weapon_node);

		}

		/* Increment */
		weapon_number++;

	}

	/* Log Message */
	print_autogun_log(autopilot, "Finished Profiling");

}

/*
 * Function to calculate a score based on a target and
 * its range to the AI
 */
int auto_calc_target_score(AUTO * autopilot, MECH * mech, MECH * target,
						   MAP * map)
{

	int target_score;
	float range;
	float target_speed;
	int target_bv;

	int total_armor_current;
	int total_armor_original;
	int total_internal_current;
	int total_internal_original;

	int section;

	float damage_score;
	float bv_score;
	float speed_score;
	float range_score;
	float status_score;

	/* Default Values */
	target_score = 0;

	total_armor_current = 0;
	total_armor_original = 0;
	total_internal_current = 0;
	total_internal_original = 0;

	damage_score = 0.0;
	bv_score = 0.0;
	speed_score = 0.0;
	range_score = 0.0;
	status_score = 0.0;

	/* Here is the meat of the function, basicly I gave each
	 * part a maximum score, then fit a linear plot from the
	 * max to a min value and score.  Then I just summed
	 * all the pieces together, very linear but should
	 * give us a good starting point */

	/* Is the target dead? */
	if(Destroyed(target))
		return target_score;

	/* If target is combat safe don't even try to shoot it */
	if(MechStatus(target) & COMBAT_SAFE)
		return target_score;

	/* Compare Teams - for now we won't try to shoot a guy on our team */
	if(MechTeam(target) == MechTeam(mech))
		return target_score;

	/* Are we in los of the target - not sure really what to do about this
	 * one, since we want the AI to be smart and all, for now, lets have
	 * it be all seeing */

	/* Range to target */
	range = FindHexRange(MechFX(mech), MechFY(mech), MechFX(target),
						 MechFY(target));

	/* Our we outside the range of the AI's System */
	if((range >= (float) AUTO_GUN_MAX_RANGE)) {
		return target_score;
	}

	/* Range score calc */
	/* Min range is 0, max range is 30, so score goes from 300 to 0 */
	range_score = -10.0 * range + 300.0;

	/* Get the Speed of the target */
	target_speed = MechSpeed(target);

	/* Speed score calc */
	/* Min speed is 0, max is 150 (can go higher tho), and score goes from
	 * 300 to 0 (can go negative if the target is faster then 150) */
	/*! \todo {Check to see what happens when the target is backing} */
	speed_score = -2.0 * target_speed + 300.0;

	/* Get the BV of the target */
	target_bv = MechBV(target);

	/* BV score calc */
	/* Min bv is 0, max is around 2000 (can go higher), and score goes from
	 * 0 to 100 (can go higher but we don't care much about bv) */
	bv_score = 0.05 * ((float) target_bv);

	/* Get the damage of the target by cycling through all the sections
	 * and adding up the current and original values */
	for(section = 0; section < NUM_SECTIONS; section++) {

		/* Total the current armor and original armor */
		total_armor_current += GetSectArmor(target, section) +
			GetSectRArmor(target, section);
		total_armor_original += GetSectOArmor(target, section) +
			GetSectORArmor(target, section);

		/* Total the current internal and original internal */
		total_internal_current += GetSectInt(target, section);
		total_internal_original += GetSectOInt(target, section);

	}

	/* Ok like above, we set a min and max, for armor was 100% to 0%
	 * and scored from 0 to 300.  For internal was 100% to 0% and
	 * scored from 0 to 200. But we have to take care not to divide
	 * by zero. */

	/* Check the totals before we divide so no Divide by zeros */
	if(total_internal_original == 0 && total_armor_original == 0) {

		/* Both values are zero, not going to try and shoot it */
		return target_score;

	} else if(total_internal_original == 0) {

		/* Just use armor part of the calc */
		damage_score = -3.0 * ((float) total_armor_current /
							   (float) total_armor_original) + 300.0;

	} else if(total_armor_original == 0) {

		/* Just use internal part of the calc */
		damage_score = -2.0 * ((float) total_internal_current /
							   (float) total_internal_original) + 200.0;

	} else {

		/* Use the whole thing */
		damage_score = -3.0 * ((float) total_armor_current /
							   (float) total_armor_original) + 300.0
			- 2.0 * ((float) total_internal_current /
					 (float) total_internal_original) + 200.0;

	}

	/* Get the 'state' ie: shutdown, prone whatever */
	if(!Started(target))
		status_score += 100.0;

	if(Uncon(target))
		status_score += 100.0;

/* Since the max bv is somewhat around 2000, lets put mechs in LOS on an even scale */
	if(MechToMech_LOSFlag(map, mech, target) & MECHLOSFLAG_SEEN)
		status_score += 2000.0;

	/* Add the individual scores and return the value */
	target_score = (int) floor(range_score + speed_score + bv_score +
							   damage_score + status_score);

	return target_score;

}

/*
 * The main targeting/firing event for the AI
 *
 * Loops through all the cons around it, scoring them and deciding
 * what to shoot and what weapons to shoot at it
 */
void auto_gun_event(AUTO *autopilot)
{
	MECH *mech = (MECH *) autopilot->mymech;	/* Its Mech */
	MAP *map;					/* The current Map */
	MECH *target;				/* Our current target */
	MECH *physical_target;		/* Our physical target */
	rbtree targets;			    /* all the targets we're looking at */
	target_node *temp_target_node;	/* temp target node struct */
	weapon_node *temp_weapon_node;	/* temp weapon node struct */

	char buffer[LBUF_SIZE];		/* General use buffer */

	int target_score;			/* variable to store temp score */
	int threshold_score;		/* The score to beat to switch targets */

	/* Stuff for Physical attacks */
	int elevation_diff;			/* Whats the elevation difference between
								   the target and the mech */
	int what_arc;				/* What arc is the target in */
	int new_arc;				/* What arc now that we've twisted
								   our torso */
	int relative_bearing;		/* Int to figure out which part of
								   the rear arc hes in */
	int is_section_destroyed[4];	/* Array of the different possible
									   sections that could be destroyed */
	int section_hasbusyweap[4];	/* Array to show if a section has a
								   cycling weapon in it */
	int rleg_bth, lleg_bth;		/* To help us decide which leg to kick
								   with */
	int is_rarm_ready, is_larm_ready;	/* To help us decide which arms to
										   punch with */

	/* Stuff for Weapon Attacks */
	int accumulate_heat;		/* How much heat we're building up */
	int i, j;

	float range;				/* General variable for range */
	float maxspeed;				/* So we know how fast our guy is going */

	/* For use with chase targ */
	short x, y;
	float fx, fy;
	char do_chasetarget;		/* Flag should we do chasetarget
								   based on certain conditions */

	/* Basic checks */
	if(!mech || !autopilot)
		return;

	if(!IsMech(mech->mynum) || !IsAuto(autopilot->mynum))
		return;

	/* Ok our mech is dead we're done */
	if(Destroyed(mech)) {
		DoStopGun(autopilot);
		return;
	}

	/*! \todo {Need to change this incase the AI shuts down while fighting} */
	if(!Started(mech)) {
		Zombify(autopilot);
		return;
	}

	/* Not on map - so lets calm down */
	if(!(map = getMap(mech->mapindex))) {
		Zombify(autopilot);
		return;
	}

	/* Log it */
	print_autogun_log(autopilot, "Autogun Event Started");

	/* check for a gun profile. */
    if(autopilot->weaplist == NULL) {
		print_autogun_log(autopilot, "Autogun Event Finished");
		return;
	}

	/* OODing so don't shoot any guns */
	if(OODing(mech)) {
		/* Log It */
		print_autogun_log(autopilot, "Autogun Event Finished");
		return;
	}

	/* First check to make sure we have a valid current target */
	if(autopilot->target > -1) {

		if(!(target = getMech(autopilot->target))) {

			/* ok its not a valid target reset */
			autopilot->target = -1;
			autopilot->target_score = 0;

		} else if(Destroyed(target) || (target->mapindex != mech->mapindex)) {

			/* Target is either dead or not on the map anymore */
			autopilot->target = -1;
			autopilot->target_score = 0;

		} else {

			/* Will keep on an assigned target even if its to far
			 * away */

			/* Get range from mech to current target */
			range = FindHexRange(MechFX(mech), MechFY(mech),
								 MechFX(target), MechFY(target));

			if((range >= (float) AUTO_GUN_MAX_RANGE) &&
			   !AssignedTarget(autopilot)) {

				/* Target is to far away */
				autopilot->target = -1;
				autopilot->target_score = 0;

			}

		}

	}

	/* Were we given a target and its no longer there? */
	if(AssignedTarget(autopilot) && autopilot->target == -1) {

		/* Ok we had an assigned target but its gone now */
		UnassignTarget(autopilot);

		/*! \todo {Possibly add a radio message saying target destroyed} */
	}

	/* Do we need to look for a new target */
	if(autopilot->target == -1 ||
	   (autopilot->target_update_tick >= AUTO_GUN_UPDATE_TICK &&
		!AssignedTarget(autopilot))) {

		/* Ok looking for a new target */

		/* Log It */
		print_autogun_log(autopilot, "Autogun - Looking for new target");

		/* Reset the update ticker */
		autopilot->target_update_tick = 0;

		/* Setup the rbtree */
		targets = rb_init(&auto_generic_compare, NULL);

		/* Cycle through possible targets and pick something to shoot */
		for(i = 0; i < map->first_free; i++) {

			/* Make sure its on the right map */
			if(i != mech->mapnumber && (j = map->mechsOnMap[i]) > 0) {

				/* Is it a valid unit ? */
				if(!(target = getMech(j)))
					continue;

				/* Score the target */
				target_score =
					auto_calc_target_score(autopilot, mech, target, map);

				/* Log It */
				print_autogun_log(autopilot,
								  "Autogun - Possible target #%d with score %d",
								  target->mynum, target_score);

				/* If target has a score add it to rbtree */
				if(target_score > 0) {

					/* Create target node and fill with proper values */
					temp_target_node = auto_create_target_node(target_score,
															   target->mynum);

					/*! \todo {should add check incase it returns a NULL struct} */

					/* Add it to list but first make sure it doesn't overlap
					 * with a current score */
					while (1) {

						if(rb_exists
						   (targets, &temp_target_node->target_score)) {
							temp_target_node->target_score++;
						} else {
							break;
						}

					}

					/* Add it */
					rb_insert(targets, &temp_target_node->target_score,
							  temp_target_node);

				}

				/* Check to see if its our current target */
				if(autopilot->target == target->mynum) {

					/* Save the new score */
					autopilot->target_score = target_score;

				}

			}

		}						/* End of for loop */

		/* Check to see if we couldn't find ANY targets within range,
		 * if not, cycle autogun and set the update tick to 20, so we
		 * check again in 10 seconds */
		if(!(rb_size(targets) > 0)) {

			/* Have the AI look for a new target 10 seconds from now */
			/*! \todo {Possibly change this since this gives an attacker who
			 * appears somehow very quickly 10 seconds to hose the AI} */
			autopilot->target = -1;
			autopilot->target_score = 0;
			autopilot->target_update_tick = 20;

			/* Don't need the target list any more so lets destroy it */
			rb_walk(targets, WALK_INORDER, &auto_targets_callback, NULL);
			rb_destroy(targets);

            /* Log It */
			print_autogun_log(autopilot, "Autogun in idle mode");
			print_autogun_log(autopilot, "Autogun Event Finished");
			return;
		}

		/* Now if we have a current target, compare it to best target from
		 * the new list.  If better then threshold, lock new target, else
		 * stay on target */

		/* Best target */
		temp_target_node =
			(target_node *) rb_search(targets, SEARCH_LAST, NULL);

		/* Log It */
		print_autogun_log(autopilot,
						  "Autogun - Best target #%d with score %d",
						  temp_target_node->target_dbref,
						  temp_target_node->target_score);
		print_autogun_log(autopilot,
						  "Autogun - Current target #%d with score %d",
						  autopilot->target, autopilot->target_score);

		if(autopilot->target > -1 && autopilot->target_score > 0) {

			/* Check to see if its our current target */
			if(autopilot->target != temp_target_node->target_dbref) {

				/* Calc the threshold score to beat */
				threshold_score =
					((100.0 +
					  (float) autopilot->target_threshold) / 100.0) *
					autopilot->target_score;

				if(temp_target_node->target_score > threshold_score) {

					/* Change targets */
					autopilot->target = temp_target_node->target_dbref;
					autopilot->target_score = temp_target_node->target_score;

					print_autogun_log(autopilot, "Switching Target to #%d",
									  autopilot->target);

				}

				/* Else: Don't switch targets */

			}

			/* Else: Don't need to swtich targets */

		} else {

			/* Don't have a good current target so lock this one */
			autopilot->target = temp_target_node->target_dbref;
			autopilot->target_score = temp_target_node->target_score;

		}						/* End of choosing new target */

		/* Don't need the target list any more so lets destroy it */
		rb_walk(targets, WALK_INORDER, &auto_targets_callback, NULL);
		rb_destroy(targets);

	} else {

		/* Ok didn't need to look for a new target so update the ticker */
		autopilot->target_update_tick++;

	}

	/* End of picking a new target */

	/* Log It */
	print_autogun_log(autopilot, "Autogun - Current target #%d with score %d",
					  autopilot->target, autopilot->target_score);

	/* Setup the current target */
	if(!(target = getMech(autopilot->target))) {

		/* There were no valid targets so
		 * rerun autogun */

		/* Reset the AI */
		autopilot->target = -1;
		autopilot->target_score = 0;

        /* Log It */
		print_autogun_log(autopilot, "Autogun - No valid current targets");
		print_autogun_log(autopilot, "Autogun Event Finished");
		return;

	}

	/* Check to see if we need to (re)lock our target */
	if(MechTarget(mech) != autopilot->target) {

		/* Lock Him */
		snprintf(buffer, LBUF_SIZE, "%c%c", MechID(target)[0],
				 MechID(target)[1]);
		mech_settarget(autopilot->mynum, mech, buffer);

		/* Log It */
		print_autogun_log(autopilot, "Autogun - Locking target #%d",
						  autopilot->target);

	}
	
	/* Primary target isn't in LOS. Let's re-run in 5s */
	if(!(MechToMech_LOSFlag(map, mech, target) & MECHLOSFLAG_SEEN)) {
		autopilot->target = -1;
		autopilot->target_update_tick = 25;
		return;
	}

	/* Update autosensor */

	/* Now lets get physical */

	/* Log It */
	print_autogun_log(autopilot, "Autogun - Start Physical Attack Stage");

	/* Get range from mech to current target */
	range =
		FindHexRange(MechFX(mech), MechFY(mech), MechFX(target),
					 MechFY(target));

	/* First check our range to our target, if within range attack it, else
	 * check to see if its outside our range threshold and if so pick a target
	 * close and attack that */

	/*! \todo {Might need to add in here something incase the target is a bsuit} */
	if(range < 1.0) {

		/* We're beating on our main target */
		physical_target = target;

	} else if(range > AUTO_GUN_PHYSICAL_RANGE_MIN) {

		/* Try and find a target */

		physical_target = NULL;

		/* Cycle through possible targets and pick something to beat on */
		for(i = 0; i < map->first_free; i++) {

			/* Make sure its on the right map */
			if(i != mech->mapnumber && (j = map->mechsOnMap[i]) > 0) {

				/* Is it a valid unit ? */
				if(!(target = getMech(j)))
					continue;

				if(Destroyed(target))
					continue;

				if(MechStatus(target) & COMBAT_SAFE)
					continue;

				if(MechTeam(target) == MechTeam(mech))
					continue;

				/* Check its range */
				range = FindHexRange(MechFX(mech), MechFY(mech),
									 MechFX(target), MechFY(target));

				/* Just go for first one , can always add scoring later */
				if(range < 1.0) {
					physical_target = target;
					break;
				}

			}

		}

	} else {

		/* Our target is close so dont try and physically attack anyone */
		physical_target = NULL;

	}

	/* Now nail it with a physical attack but only if we see it */
	if(physical_target &&
	   (MechToMech_LOSFlag(map, mech, physical_target) & MECHLOSFLAG_SEEN)) {

		/* Log It */
		print_autogun_log(autopilot,
						  "Autogun - Attempting physical attack against"
						  " target #%d", physical_target->mynum);

		/* Calculate elevation difference */
		elevation_diff = MechZ(mech) - MechZ(target);

		/* Are we a biped Mech */
		if((MechType(mech) == CLASS_MECH) && (MechMove(mech) == MOVE_BIPED)) {

			/* Center the torso */
			MechStatus(mech) &= ~(TORSO_RIGHT | TORSO_LEFT);

			if(MechSpecials(mech) & FLIPABLE_ARMS) {

				/* Center the arms if need be */
				MechStatus(mech) &= ~(FLIPPED_ARMS);
			}

			/* Find direction of bad guy */
			what_arc =
				InWeaponArc(mech, MechFX(physical_target),
							MechFY(physical_target));

			/* Rotate if we need to */
			if(what_arc & LSIDEARC) {

				/* Rotate Left */
				MechStatus(mech) |= TORSO_LEFT;

			} else if(what_arc & RSIDEARC) {

				/* Rotate Right */
				MechStatus(mech) |= TORSO_RIGHT;

			} else if(what_arc & REARARC) {

				/* Find out if it would be better to
				 * rotate left or right */
				relative_bearing =
					MechFacing(mech) - FindBearing(MechFX(mech), MechFY(mech),
												   MechFX(physical_target),
												   MechFY(physical_target));

				if(relative_bearing > 120 && relative_bearing < 180) {

					/* Rotate Right */
					MechStatus(mech) |= TORSO_RIGHT;

				} else if(relative_bearing > 180 && relative_bearing < 240) {

					/* Rotate Left */
					MechStatus(mech) |= TORSO_LEFT;

				}

				/* ELSE: Hes directly behind us so we can't do anything */

			}

			/* Calculate the new arc */
			new_arc =
				InWeaponArc(mech, MechFX(physical_target),
							MechFY(physical_target));

			/* Check to see what sections are destroyed */
			memset(is_section_destroyed, 0, sizeof(is_section_destroyed));
			is_section_destroyed[0] = SectIsDestroyed(mech, RARM);
			is_section_destroyed[1] = SectIsDestroyed(mech, LARM);
			is_section_destroyed[2] = SectIsDestroyed(mech, RLEG);
			is_section_destroyed[3] = SectIsDestroyed(mech, LLEG);

			/* Check to see if the sections have a busy weapon */
			memset(section_hasbusyweap, 0, sizeof(section_hasbusyweap));
			section_hasbusyweap[0] = SectHasBusyWeap(mech, RARM);
			section_hasbusyweap[1] = SectHasBusyWeap(mech, LARM);
			section_hasbusyweap[2] = SectHasBusyWeap(mech, RLEG);
			section_hasbusyweap[3] = SectHasBusyWeap(mech, LLEG);

			/* Try weapon physical attacks */

			/* Right Arm */
			if(!is_section_destroyed[0] && !section_hasbusyweap[0] &&
			   !AnyLimbsRecycling(mech) && ((new_arc & FORWARDARC) ||
											(new_arc & RSIDEARC)) &&
			   (elevation_diff == 0 || elevation_diff == -1)) {

				snprintf(buffer, LBUF_SIZE, "r %c%c",
						 MechID(target)[0], MechID(target)[1]);

				if(have_axe(mech, RARM))
					mech_axe(autopilot->mynum, mech, buffer);
				/* else if (have_mace(mech, RARM)) */
				/*! \todo {Add in mace code here} */
				else if(have_sword(mech, RARM))
					mech_sword(autopilot->mynum, mech, buffer);

			}

			/* Left Arm */
			if(!is_section_destroyed[1] && !section_hasbusyweap[1] &&
			   !AnyLimbsRecycling(mech) && ((new_arc & FORWARDARC) ||
											(new_arc & LSIDEARC)) &&
			   (elevation_diff == 0 || elevation_diff == -1)) {

				snprintf(buffer, LBUF_SIZE, "l %c%c",
						 MechID(target)[0], MechID(target)[1]);

				if(have_axe(mech, LARM))
					mech_axe(autopilot->mynum, mech, buffer);
				/* else if (have_mace(mech, RARM)) */
				/*! \todo {Add in mace code here} */
				else if(have_sword(mech, LARM))
					mech_sword(autopilot->mynum, mech, buffer);

			}

			/* Try and kick but only if we got two legs, one of them
			 * doesn't have a cycling weapon and the target is in the
			 * front arc */
			if((!section_hasbusyweap[2] || !section_hasbusyweap[3]) &&
			   !is_section_destroyed[2] && !is_section_destroyed[3] &&
			   (what_arc & FORWARDARC) && !AnyLimbsRecycling(mech) &&
			   (elevation_diff == 0 || elevation_diff == 1)) {

				rleg_bth = 0;
				lleg_bth = 0;

				/* Check the RLEG for any crits or weaps cycling */
				if(!section_hasbusyweap[2]) {
					if(!OkayCritSectS(RLEG, 0, SHOULDER_OR_HIP))
						rleg_bth += 3;
					if(!OkayCritSectS(RLEG, 1, UPPER_ACTUATOR))
						rleg_bth++;
					if(!OkayCritSectS(RLEG, 2, LOWER_ACTUATOR))
						rleg_bth++;
					if(!OkayCritSectS(RLEG, 3, HAND_OR_FOOT_ACTUATOR))
						rleg_bth++;
				} else {
					rleg_bth = 99;
				}

				/* Check the LLEG for any crits or weaps cycling */
				if(!section_hasbusyweap[3]) {
					if(!OkayCritSectS(LLEG, 0, SHOULDER_OR_HIP))
						lleg_bth += 3;
					if(!OkayCritSectS(LLEG, 1, UPPER_ACTUATOR))
						lleg_bth++;
					if(!OkayCritSectS(LLEG, 2, LOWER_ACTUATOR))
						lleg_bth++;
					if(!OkayCritSectS(LLEG, 3, HAND_OR_FOOT_ACTUATOR))
						lleg_bth++;
				} else {
					rleg_bth = 99;
				}

				/* Now kick depending on which one would be better 
				 * to kick with */
				if(rleg_bth <= lleg_bth) {
					snprintf(buffer, LBUF_SIZE, "r %c%c",
							 MechID(physical_target)[0],
							 MechID(physical_target)[1]);
				} else {
					snprintf(buffer, LBUF_SIZE, "l %c%c",
							 MechID(physical_target)[0],
							 MechID(physical_target)[1]);
				}
				mech_kick(autopilot->mynum, mech, buffer);
			}

			/* Finally try to punch */
			if(((!is_section_destroyed[0] && !section_hasbusyweap[0]) ||
				(!is_section_destroyed[1] && !section_hasbusyweap[1]))
			   && !AnyLimbsRecycling(mech) &&
			   (elevation_diff == 0 || elevation_diff == -1)) {

				is_rarm_ready = 0;
				is_larm_ready = 0;

				if(!is_section_destroyed[0] &&
				   !section_hasbusyweap[0] &&
				   ((new_arc & FORWARDARC) || (new_arc & RSIDEARC))) {

					/* We can use the right arm */
					is_rarm_ready = 1;
				}

				if(!is_section_destroyed[1] &&
				   !section_hasbusyweap[1] &&
				   ((new_arc & FORWARDARC) || (new_arc & LSIDEARC))) {

					/* We can use the left arm */
					is_larm_ready = 1;
				}

				if(is_rarm_ready == 1 && is_larm_ready == 1) {
					snprintf(buffer, LBUF_SIZE, "b %c%c",
							 MechID(target)[0], MechID(target)[1]);
				} else if(is_rarm_ready == 1) {
					snprintf(buffer, LBUF_SIZE, "r %c%c",
							 MechID(target)[0], MechID(target)[1]);
				} else {
					snprintf(buffer, LBUF_SIZE, "l %c%c",
							 MechID(target)[0], MechID(target)[1]);
				}

				/* Now punch */
				mech_punch(autopilot->mynum, mech, buffer);

			}

		} else if((MechType(mech) == CLASS_MECH) &&
				  (MechMove(mech) == MOVE_QUAD)) {

			/* Quad Mech - Right now only supporting kicking front style for quad */
			/* Remember, the RARM becomes the Front RLEG and the LARM becomes the
			 * Front LLEG */

			/* Find direction of bad guy */
			what_arc =
				InWeaponArc(mech, MechFX(physical_target),
							MechFY(physical_target));

			/* Check to see what sections are destroyed */
			memset(is_section_destroyed, 0, sizeof(is_section_destroyed));
			is_section_destroyed[0] = SectIsDestroyed(mech, RARM);
			is_section_destroyed[1] = SectIsDestroyed(mech, LARM);
			is_section_destroyed[2] = SectIsDestroyed(mech, RLEG);
			is_section_destroyed[3] = SectIsDestroyed(mech, LLEG);

			/* Check to see if the sections have a busy weapon */
			memset(section_hasbusyweap, 0, sizeof(section_hasbusyweap));
			section_hasbusyweap[0] = SectHasBusyWeap(mech, RARM);
			section_hasbusyweap[1] = SectHasBusyWeap(mech, LARM);
			//section_hasbusyweap[2] = SectHasBusyWeap(mech, RLEG);
			//section_hasbusyweap[3] = SectHasBusyWeap(mech, LLEG);

			/* Try and kick but only if we got two legs, one of them
			 * doesn't have a cycling weapon and the target is in the
			 * front arc */
			if((!section_hasbusyweap[0] || !section_hasbusyweap[0]) &&
			   !is_section_destroyed[0] && !is_section_destroyed[1] &&
			   !is_section_destroyed[2] && !is_section_destroyed[3] &&
			   (what_arc & FORWARDARC) && !AnyLimbsRecycling(mech) &&
			   (elevation_diff == 0 || elevation_diff == 1)) {

				rleg_bth = 0;
				lleg_bth = 0;

				/* Check the Front Right Leg for any crits or weaps cycling */
				if(!section_hasbusyweap[0]) {
					if(!OkayCritSectS(RARM, 0, SHOULDER_OR_HIP))
						rleg_bth += 3;
					if(!OkayCritSectS(RARM, 1, UPPER_ACTUATOR))
						rleg_bth++;
					if(!OkayCritSectS(RARM, 2, LOWER_ACTUATOR))
						rleg_bth++;
					if(!OkayCritSectS(RARM, 3, HAND_OR_FOOT_ACTUATOR))
						rleg_bth++;
				} else {
					rleg_bth = 99;
				}

				/* Check the Front Left Leg for any crits or weaps cycling */
				if(!section_hasbusyweap[1]) {
					if(!OkayCritSectS(LARM, 0, SHOULDER_OR_HIP))
						lleg_bth += 3;
					if(!OkayCritSectS(LARM, 1, UPPER_ACTUATOR))
						lleg_bth++;
					if(!OkayCritSectS(LARM, 2, LOWER_ACTUATOR))
						lleg_bth++;
					if(!OkayCritSectS(LARM, 3, HAND_OR_FOOT_ACTUATOR))
						lleg_bth++;
				} else {
					rleg_bth = 99;
				}

				/* Now kick depending on which one would be better 
				 * to kick with */
				if(rleg_bth <= lleg_bth) {
					snprintf(buffer, LBUF_SIZE, "r %c%c",
							 MechID(physical_target)[0],
							 MechID(physical_target)[1]);
				} else {
					snprintf(buffer, LBUF_SIZE, "l %c%c",
							 MechID(physical_target)[0],
							 MechID(physical_target)[1]);
				}
				mech_kick(autopilot->mynum, mech, buffer);
			}

		} else if(MechType(mech) == CLASS_BSUIT) {

			/* Are we a BSuit */

		} else {

			/* Eventually add code here maybe for other physical attacks for
			 * tanks perhaps */

		}

	}

	/* End of physical attack */
	/* Log It */
	print_autogun_log(autopilot, "Autogun - End Physical Attack Stage");

	/* Now we mow down our target */

	/* Get our current target */
	/* Check to make sure we didn't kill it with physical attack
	 * or something */
	if(!(target = getMech(autopilot->target))) {

		/* There were no valid targets so
		 * rerun autogun */

		/* Reset the AI */
		autopilot->target = -1;
		autopilot->target_score = 0;

        /* Log It */
		print_autogun_log(autopilot, "Autogun - No valid current target");
		print_autogun_log(autopilot, "Autogun Event Finished");
		return;

	} else if(Destroyed(target) || (target->mapindex != mech->mapindex)) {

		/* Target is either dead or not on the map anymore */
		autopilot->target = -1;
		autopilot->target_score = 0;

        /* Log it */
		print_autogun_log(autopilot, "Autogun - Target Gone");
		print_autogun_log(autopilot, "Autogun Event Finished");
		return;
	}

	/* Log It */
	print_autogun_log(autopilot, "Autogun - Starting Weapon Attack Phase");

	/* Get range from mech to current target */
	range = FindHexRange(MechFX(mech), MechFY(mech),
						 MechFX(target), MechFY(target));

	/* This probably unnecessary but since it doesn't
	 * take much to calc range it should be ok for
	 * testing for now */
	if((range >= (float) AUTO_GUN_MAX_RANGE) && !AssignedTarget(autopilot)) {

		/* Target is to far - reset */
		autopilot->target = -1;
		autopilot->target_score = 0;


		/* Log it */
		print_autogun_log(autopilot, "Autogun - Target out of range");
		print_autogun_log(autopilot, "Autogun Event Finished");
		return;
	}

	/* Cycle through Guns while watching the heat */
	if((range < (float) AUTO_GUN_MAX_RANGE)
	   && autopilot->profile[(int) range]) {

		/* Ok we got weapons lets use them */

		/* Reset heat counter to current heat */
		accumulate_heat = MechWeapHeat(mech);

		/* If the unit is moving need to account for the heat of that as well */
		if((MechType(mech) == CLASS_MECH) && (fabs(MechSpeed(mech)) > 0.0)) {

			maxspeed = MMaxSpeed(mech);
			if(IsRunning(MechDesiredSpeed(mech), maxspeed))
				accumulate_heat += 2;
			else
				accumulate_heat += 1;
		}

		/* Get first weapon */
		temp_weapon_node =
			(weapon_node *) rb_search(autopilot->profile[(int) range],
									  SEARCH_LAST, NULL);

		while (temp_weapon_node) {

			/* Check to see if the weapon even works */
			if(WeaponIsNonfunctional(mech, temp_weapon_node->section,
									 temp_weapon_node->critical,
									 GetWeaponCrits(mech,
													Weapon2I
													(temp_weapon_node->
													 weapon_db_number))) >
			   0) {

				/* Weapon Doesn't work so go to next one */
				temp_weapon_node =
					(weapon_node *) rb_search(autopilot->profile[(int) range],
											  SEARCH_PREV,
											  &temp_weapon_node->
											  range_scores[(int) range]);

				continue;
			}

			/* Check to see if its cycling */
			if(WpnIsRecycling
			   (mech, temp_weapon_node->section,
				temp_weapon_node->critical)) {

				/* Go to the next one */
				temp_weapon_node =
					(weapon_node *) rb_search(autopilot->profile[(int) range],
											  SEARCH_PREV,
											  &temp_weapon_node->
											  range_scores[(int) range]);

				continue;
			}

			if(IsAMS(temp_weapon_node->weapon_db_number)) {

				/* Ok its an AMS so go to next weapon */
				temp_weapon_node =
					(weapon_node *) rb_search(autopilot->profile[(int) range],
											  SEARCH_PREV,
											  &temp_weapon_node->
											  range_scores[(int) range]);
				continue;
			}
			
			/* No sense trying to fire Stinger missiles if the target isn't airborne/jumping */

			if((GetPartAmmoMode(mech, temp_weapon_node->section, temp_weapon_node->critical) & STINGER_MODE) && target
				&& !(Jumping(target) || OODing(target) || (FlyingT(target) && !Landed(target)))) {

				temp_weapon_node =
					(weapon_node *) rb_search(autopilot->profile[(int) range],
											SEARCH_PREV,
											&temp_weapon_node->
											range_scores[(int) range]);
				continue;
			}
	

			/* Check heat levels, since the heat isn't updated untill we're done
			 * we have to manage the heat ourselves */
			/*! \todo {Add a check also for aeros} */
			if((MechType(mech) == CLASS_MECH) && (((float) accumulate_heat +
												   (float)
												   MechWeapons
												   [temp_weapon_node->
													weapon_db_number].heat -
												   (float)
												   MechMinusHeat(mech)) >
												  AUTO_GUN_MAX_HEAT)) {

				/* Would make ourselves to hot to fire this gun */
				temp_weapon_node =
					(weapon_node *) rb_search(autopilot->profile[(int) range],
											  SEARCH_PREV,
											  &temp_weapon_node->
											  range_scores[(int) range]);

				continue;
			}

			/* Ok passed the checks now setup the arcs and see if we can fire it */

			/* Ok the rest depends on what type of unit we driving */
			if((MechType(mech) == CLASS_MECH) &&
			   (MechMove(mech) == MOVE_BIPED)) {

				/* Center ourself and get target arc */
				MechStatus(mech) &= ~(TORSO_RIGHT | TORSO_LEFT);
				if(MechSpecials(mech) & FLIPABLE_ARMS) {

					/* Center the arms if need be */
					MechStatus(mech) &= ~(FLIPPED_ARMS);
				}

				/* Get Target Arc */
				what_arc = InWeaponArc(mech, MechFX(target), MechFY(target));

				/* Now go through the various arcs and see if we
				 * need to flip arm or rotorso or something */
				if(what_arc & REARARC) {

					if(temp_weapon_node->section == LARM ||
					   temp_weapon_node->section == RARM) {

						/* First see if we can flip arms */
						if(MechSpecials(mech) & FLIPABLE_ARMS) {

							/* Flip the arms */
							MechStatus(mech) |= FLIPPED_ARMS;

						} else {

							/* Now see if we can rotatorso */

							/* Find out if it would be better to
							 * rotate left or right */
							relative_bearing = MechFacing(mech) -
								FindBearing(MechFX(mech), MechFY(mech),
											MechFX(target), MechFY(target));

							if(relative_bearing > 120
							   && relative_bearing < 180
							   && temp_weapon_node->section == RARM) {

								/* Rotate Right */
								MechStatus(mech) |= TORSO_RIGHT;

							} else if(relative_bearing > 180
									  && relative_bearing < 240
									  && temp_weapon_node->section == LARM) {

								/* Rotate Left */
								MechStatus(mech) |= TORSO_LEFT;

							} else {

								/* Can't do anything so go to next weapon */
								temp_weapon_node =
									(weapon_node *) rb_search(autopilot->
															  profile[(int)
																	  range],
															  SEARCH_PREV,
															  &temp_weapon_node->
															  range_scores[(int) range]);

								continue;

							}

						}

					} else
						if(!
						   (GetPartFireMode
							(mech, temp_weapon_node->section,
							 temp_weapon_node->critical) & REAR_MOUNT)) {

						/* Weapon is forward torso or leg mounted weapon
						 * so no way to shoot with */
						temp_weapon_node =
							(weapon_node *) rb_search(autopilot->
													  profile[(int) range],
													  SEARCH_PREV,
													  &temp_weapon_node->
													  range_scores[(int)
																   range]);

						continue;

					}

					/* ELSE: Weapon is rear mounted so don't need to 
					 * do anything */

				} else if(what_arc & LSIDEARC) {

					if(temp_weapon_node->section == RLEG ||
					   temp_weapon_node->section == LLEG) {

						/* No way can we hit him with leg mounted
						 * weapons so lets go to next one */
						temp_weapon_node =
							(weapon_node *) rb_search(autopilot->
													  profile[(int) range],
													  SEARCH_PREV,
													  &temp_weapon_node->
													  range_scores[(int)
																   range]);

						continue;

					}

					/* Rotate torso left */
					MechStatus(mech) |= TORSO_LEFT;

				} else if(what_arc & RSIDEARC) {

					if(temp_weapon_node->section == RLEG ||
					   temp_weapon_node->section == LLEG) {

						/* No way can we hit him with leg mounted
						 * weapons so lets go to next one */
						temp_weapon_node =
							(weapon_node *) rb_search(autopilot->
													  profile[(int) range],
													  SEARCH_PREV,
													  &temp_weapon_node->
													  range_scores[(int)
																   range]);

						continue;

					}

					/* Rotate torso right */
					MechStatus(mech) |= TORSO_RIGHT;

				} else {

					if(GetPartFireMode(mech, temp_weapon_node->section,
									   temp_weapon_node->
									   critical) & REAR_MOUNT) {

						/* No way can we hit the guy with a rear
						 * gun so lets go to next one */
						temp_weapon_node =
							(weapon_node *) rb_search(autopilot->
													  profile[(int) range],
													  SEARCH_PREV,
													  &temp_weapon_node->
													  range_scores[(int)
																   range]);

						continue;

					}

				}

			} else if((MechType(mech) == CLASS_MECH) &&
					  (MechMove(mech) == MOVE_QUAD)) {

				/* Get Target Arc */
				what_arc = InWeaponArc(mech, MechFX(target), MechFY(target));

				if(what_arc & REARARC) {

					if(!(GetPartFireMode(mech, temp_weapon_node->section,
										 temp_weapon_node->
										 critical) & REAR_MOUNT)) {

						/* Weapon is not rear mounted so skip it and
						 * go to the next weapon */
						temp_weapon_node =
							(weapon_node *) rb_search(autopilot->
													  profile[(int) range],
													  SEARCH_PREV,
													  &temp_weapon_node->
													  range_scores[(int)
																   range]);

						continue;

					}

				} else if(what_arc & FORWARDARC) {

					if(GetPartFireMode(mech, temp_weapon_node->section,
									   temp_weapon_node->
									   critical) & REAR_MOUNT) {

						/* Weapon is rear mounted so skip it and
						 * go to the next weapon */
						temp_weapon_node =
							(weapon_node *) rb_search(autopilot->
													  profile[(int) range],
													  SEARCH_PREV,
													  &temp_weapon_node->
													  range_scores[(int)
																   range]);

						continue;

					}

				} else {

					/* The attacker is in a zone we can't possibly
					 * shoot into, so just go to next weapon */
					temp_weapon_node =
						(weapon_node *) rb_search(autopilot->
												  profile[(int) range],
												  SEARCH_PREV,
												  &temp_weapon_node->
												  range_scores[(int) range]);

					continue;

				}

			} else if((MechType(mech) == CLASS_VEH_GROUND) ||
					  (MechType(mech) == CLASS_VEH_NAVAL)) {

				/* Get Target Arc */
				what_arc = InWeaponArc(mech, MechFX(target), MechFY(target));

				/* Check if turret exists and weapon is there */
				if(GetSectInt(mech, TURRET)
				   && temp_weapon_node->section == TURRET) {

					/* Rotate Turret and nail the guy */
					if(!(MechTankCritStatus(mech) & TURRET_JAMMED) &&
					   !(MechTankCritStatus(mech) & TURRET_LOCKED) &&
					   (AcceptableDegree
						(MechTurretFacing(mech) + MechFacing(mech)) !=
						FindBearing(MechFX(mech), MechFY(mech),
									MechFX(target), MechFY(target)))) {

						snprintf(buffer, LBUF_SIZE, "%d",
								 FindBearing(MechFX(mech), MechFY(mech),
											 MechFX(target), MechFY(target)));
						mech_turret(autopilot->mynum, mech, buffer);
					}

				} else {

					/* Check if in arc of weapon */
					if(!IsInWeaponArc(mech, MechFX(target), MechFY(target),
									  temp_weapon_node->section,
									  temp_weapon_node->critical)) {

						/* Not in the arc so lets go to the next weapon */
						temp_weapon_node =
							(weapon_node *) rb_search(autopilot->
													  profile[(int) range],
													  SEARCH_PREV,
													  &temp_weapon_node->
													  range_scores[(int)
																   range]);

						continue;

					}

				}

			} else {

				/* We're either an aero, ds, bsuit, mechwarrior or vtol
				 *
				 * Still need to add code for them */

			}

			/* Done moving around, fire the weapon */
			snprintf(buffer, LBUF_SIZE, "%d",
					 temp_weapon_node->weapon_number);
			mech_fireweapon(autopilot->mynum, mech, buffer);

			/* Log It */
			print_autogun_log(autopilot, "Autogun - Fired Weapon #%d "
							  "at target #%d",
							  temp_weapon_node->weapon_number,
							  autopilot->target);

			/* Ok check to see if weapon was fired if so account for the
			 * heat */
			if(WpnIsRecycling
			   (mech, temp_weapon_node->section,
				temp_weapon_node->critical)) {
				accumulate_heat +=
					MechWeapons[temp_weapon_node->weapon_db_number].heat;
			}

			/* Ok go to the next weapon */
			temp_weapon_node =
				(weapon_node *) rb_search(autopilot->profile[(int) range],
										  SEARCH_PREV,
										  &temp_weapon_node->
										  range_scores[(int) range]);

		}						/* End of cycling through weapons */

	}

	/* Log It */
	print_autogun_log(autopilot, "Autogun - End Weapon Attack Phase");

	/* Setup chasetarg 
	 * 
	 * Since chasetarget uses follow but we don't want follow getting
	 * messed up if its following a normal target.  We define our own
	 * follow (basicly its follow but with the command name chasetarget */

	/* Get the first command, if its chasetarget or there is no command
	 * check to see if we need to update chasetarget, otherwise means
	 * AI is doing something else important so don't try to chase
	 * anything */

	if(ChasingTarget(autopilot)) {

		/* Reset the flag */
		do_chasetarget = 0;

		/* Get the first command's enum and check it */
		switch (auto_get_command_enum(autopilot, 1)) {

		case GOAL_CHASETARGET:

			/* Ok its our command so we can change it */
			do_chasetarget = 1;
			break;

		case -1:

			/* No current commands so we can do our chasetarget */
			do_chasetarget = 1;
			break;

			/* ALl the other stuff we don't want to mess with */
		default:

			/* Reset the chase values */
			autopilot->chase_target = -10;
			autopilot->chasetarg_update_tick =
				AUTOPILOT_CHASETARG_UPDATE_TICK;
			autopilot->follow_update_tick = AUTOPILOT_FOLLOW_UPDATE_TICK;
			break;

		}

		/* Check the flag */
		if(do_chasetarget) {

			/* Ok lets chase the guy */

			/* First see if we need to update */
			if((autopilot->target != autopilot->chase_target) ||
			   (autopilot->chasetarg_update_tick >=
				AUTOPILOT_CHASETARG_UPDATE_TICK)) {

				/* Tell the AI to follow its target */
				/* Basicly remove all the commands, add in
				 * autogun and follow and engage */

				/* Let the AI know we chasing this guy */
				autopilot->chase_target = autopilot->target;

				/* Reset the tickers */
				autopilot->chasetarg_update_tick = 0;
				autopilot->follow_update_tick = AUTOPILOT_FOLLOW_UPDATE_TICK;

				/* Reset the AI */
				auto_disengage(autopilot->mynum, autopilot, "");
				auto_delcommand(autopilot->mynum, autopilot, "-1");

				/* Add in autogun and follow and engage */
				if(AssignedTarget(autopilot) && autopilot->target != -1) {
					snprintf(buffer, LBUF_SIZE, "autogun target %ld",
							 autopilot->target);
				} else {
					snprintf(buffer, LBUF_SIZE, "autogun on");
				}

				auto_addcommand(autopilot->mynum, autopilot, buffer);
				snprintf(buffer, LBUF_SIZE, "chasetarget %ld",
						 autopilot->target);
				auto_addcommand(autopilot->mynum, autopilot, buffer);
				auto_engage(autopilot->mynum, autopilot, "");

				/* Log it */
				print_autogun_log(autopilot, "Autogun Event Finished");

				return;

			} else {

				/* Update the ticker */
				autopilot->chasetarg_update_tick++;

				/* Check to see if we need to turn to face the guy by
				 * generating our target hex and seeing if we are in that
				 * hex then face the bad guy */
				if((target = getMech(autopilot->target)) &&
				   (!Destroyed(target)
					&& (target->mapindex == mech->mapindex))) {

					/* Generate the target hex */
					/*! \todo {Instead of calcing this all the time, possibly add
					 * variables to the AI to remember it} */
					FindXY(MechFX(target), MechFY(target),
						   MechFacing(target) + autopilot->ofsx,
						   autopilot->ofsy, &fx, &fy);

					RealCoordToMapCoord(&x, &y, fx, fy);

					/* Make sure the hex is sane */
					if(x < 0 || y < 0 || x >= map->map_width
					   || y >= map->map_height) {

						/* Bad Target Hex */

						/* Reset the hex to the Target's current hex */
						x = MechX(target);
						y = MechY(target);

					}

					/* Are we in the target hex and is the target not moving */
					if((MechX(mech) == x) && (MechY(mech) == y)
					   && (MechSpeed(target) < 0.5)) {

						/* Get his bearing and face him */
						MapCoordToRealCoord(x, y, &fx, &fy);

						/* If we're not facing him, turn towards him */
						if(MechDesiredFacing(mech) !=
						   FindBearing(MechFX(mech), MechFY(mech), fx, fy)) {

							snprintf(buffer, LBUF_SIZE, "%d",
									 FindBearing(MechFX(mech), MechFY(mech),
												 fx, fy));
							mech_heading(autopilot->mynum, mech, buffer);

						}
						/* Turn towards him */
					}
					/* Is he moving and are we in target hex */
				}
				/* Do we need to turn towards him */
			}					/* Do we need to update */

		}
		/* Do we run chasetarget */
	}

	/* Is chasetarget on */
	/* Make sure multiple instances of autogun aren't running */

	/* Log it */
	print_autogun_log(autopilot, "Autogun Event Finished");

	/* The End */
}
