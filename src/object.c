/*
 * object.c - low-level object manipulation routines 
 */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "mudconf.h"
#include "command.h"
#include "externs.h"
#include "flags.h"
#include "powers.h"
#include "attrs.h"
#include "alloc.h"

#define IS_CLEAN(i)	(IS(i, TYPE_GARBAGE, GOING) && \
			 (Location(i) == NOTHING) && \
			 (Contents(i) == NOTHING) && (Exits(i) == NOTHING) && \
			 (Next(i) == NOTHING) && (Owner(i) == GOD))

#define ZAP_LOC(i)	{ s_Location(i, NOTHING); s_Next(i, NOTHING); }

static int check_type;
extern int boot_off(dbref player, char *message);
extern void do_mail_clear(dbref player, char *msglist);

/**
 * Log_pointer_err, Log_header_err, Log_simple_damage: Write errors to the
 * log file.
 */
static void Log_pointer_err(dbref prior, dbref obj, dbref loc, dbref ref,
							const char *reftype, const char *errtype)
{
	STARTLOG(LOG_PROBLEMS, "OBJ", "DAMAG") {
		log_type_and_name(obj);
		if(loc != NOTHING) {
			log_text((char *) " in ");
			log_type_and_name(loc);
		}
		log_text((char *) ": ");
		if(prior == NOTHING) {
			log_text((char *) reftype);
		} else {
			log_text((char *) "Next pointer");
		}
		log_text((char *) " ");
		log_type_and_name(ref);
		log_text((char *) " ");
		log_text((char *) errtype);
		ENDLOG;
}}

static void Log_header_err(dbref obj, dbref loc, dbref val, int is_object,
						   const char *valtype, const char *errtype)
{
	STARTLOG(LOG_PROBLEMS, "OBJ", "DAMAG") {
		log_type_and_name(obj);
		if(loc != NOTHING) {
			log_text((char *) " in ");
			log_type_and_name(loc);
		}
		log_text((char *) ": ");
		log_text((char *) valtype);
		log_text((char *) " ");
		if(is_object)
			log_type_and_name(val);
		else
			log_number(val);
		log_text((char *) " ");
		log_text((char *) errtype);
		ENDLOG;
}}

static void Log_simple_err(dbref obj, dbref loc, const char *errtype)
{
	STARTLOG(LOG_PROBLEMS, "OBJ", "DAMAG") {
		log_type_and_name(obj);
		if(loc != NOTHING) {
			log_text((char *) " in ");
			log_type_and_name(loc);
		}
		log_text((char *) ": ");
		log_text((char *) errtype);
		ENDLOG;
}}

/**
 * start_home, default_home, can_set_home, new_home, clone_home:
 * Routines for validating and determining homes.
 */
dbref start_home(void)
{
	if(mudconf.start_home != NOTHING)
		return mudconf.start_home;
	return mudconf.start_room;
}

dbref default_home(void)
{
	if(mudconf.default_home != NOTHING)
		return mudconf.default_home;
	if(mudconf.start_home != NOTHING)
		return mudconf.start_home;
	return mudconf.start_room;
}

int can_set_home(dbref player, dbref thing, dbref home)
{
	if(!Good_obj(player) || !Good_obj(home) || (thing == home))
		return 0;

	switch (Typeof(home)) {
	case TYPE_PLAYER:
	case TYPE_ROOM:
	case TYPE_THING:
		if(Going(home))
			return 0;
		if(Controls(player, home) || Abode(home))
			return 1;
	}
	return 0;
}

dbref new_home(dbref player)
{
	dbref loc;

	loc = Location(player);
	if(can_set_home(Owner(player), player, loc))
		return loc;
	loc = Home(Owner(player));
	if(can_set_home(Owner(player), player, loc))
		return loc;
	return default_home();
}

dbref clone_home(dbref player, dbref thing)
{
	dbref loc;

	loc = Home(thing);
	if(can_set_home(Owner(player), player, loc))
		return loc;
	return new_home(player);
}

/**
 * Build a freelist
 */
static void make_freelist(void)
{
	dbref i;

	mudstate.freelist = NOTHING;
	DO_WHOLE_DB_REV(i) {
		if(IS_CLEAN(i)) {
			s_Link(i, mudstate.freelist);
			mudstate.freelist = i;

		}
	}
}

/**
 * Create an object of the indicated type IF the player can
 * afford it.
 */
dbref create_obj(dbref player, int objtype, char *name, int cost)
{
	dbref obj, owner;
	int quota, okname = 0, value, self_owned, require_inherit;
	FLAG f1, f2, f3;
	time_t tt;
	char *buff;
	const char *tname;

	value = 0;
	quota = 0;
	self_owned = 0;
	require_inherit = 0;

	switch (objtype) {
	case TYPE_ROOM:
		cost = mudconf.digcost;
		quota = mudconf.room_quota;
		f1 = mudconf.room_flags.word1;
		f2 = mudconf.room_flags.word2;
		f3 = mudconf.room_flags.word3;
		okname = ok_name(name);
		tname = "a room";
		break;
	case TYPE_THING:
		if(cost < mudconf.createmin)
			cost = mudconf.createmin;
		if(cost > mudconf.createmax)
			cost = mudconf.createmax;
		quota = mudconf.thing_quota;
		f1 = mudconf.thing_flags.word1;
		f2 = mudconf.thing_flags.word2;
		f3 = mudconf.thing_flags.word3;
		value = OBJECT_ENDOWMENT(cost);
		okname = ok_name(name);
		tname = "a thing";
		break;
	case TYPE_EXIT:
		cost = mudconf.opencost;
		quota = mudconf.exit_quota;
		f1 = mudconf.exit_flags.word1;
		f2 = mudconf.exit_flags.word2;
		f3 = mudconf.exit_flags.word3;
		okname = ok_name(name);
		tname = "an exit";
		break;
	case TYPE_PLAYER:
		if(cost) {
			cost = mudconf.robotcost;
			quota = mudconf.player_quota;
			f1 = mudconf.robot_flags.word1;
			f2 = mudconf.robot_flags.word2;
			f3 = mudconf.robot_flags.word3;
			value = 0;
			tname = "a robot";
			require_inherit = 1;
		} else {
			cost = 0;
			quota = 0;
			f1 = mudconf.player_flags.word1;
			f2 = mudconf.player_flags.word2;
			f3 = mudconf.player_flags.word3;
			value = mudconf.paystart;
			quota = mudconf.start_quota;
			self_owned = 1;
			tname = "a player";
		}
		buff = munge_space(name);
		if(!badname_check(buff)) {
			notify(player, "That name is not allowed.");
			free_lbuf(buff);
			return NOTHING;
		}
		if(*buff) {
			okname = ok_player_name(buff);
			if(!okname) {
				notify(player, "That's a silly name for a player.");
				free_lbuf(buff);
				return NOTHING;
			}
		}
		if(okname) {
			okname = (lookup_player(NOTHING, buff, 0) == NOTHING);
			if(!okname) {
				notify_printf(player, "The name %s is already taken.", name);
				free_lbuf(buff);
				return NOTHING;
			}
		}
		free_lbuf(buff);
		break;
	default:
		LOG_SIMPLE(LOG_BUGS, "BUG", "OTYPE",
				   tprintf("Bad object type in create_obj: %d.", objtype));
		return NOTHING;
	}

	if(!self_owned) {
		if(!Good_obj(player))
			return NOTHING;
		owner = Owner(player);
		if(!Good_obj(owner))
			return NOTHING;
	} else {
		owner = NOTHING;
	}

	if(require_inherit) {
		if(!Inherits(player)) {
			notify(player, "Permission denied.");
			return NOTHING;
		}
	}
	/*
	 * Make sure the creator can pay for the object. 
	 */

	if((player != NOTHING) && !canpayfees(player, player, cost, quota))
		return NOTHING;

	/*
	 * Get the first object from the freelist. If the object is not 
	 * clean, discard the remainder of the freelist and go get a
	 * completely new object. 
	 */

	obj = NOTHING;
	if(mudstate.freelist != NOTHING) {
		obj = mudstate.freelist;
		if(Good_obj(obj) && IS_CLEAN(obj)) {
			mudstate.freelist = Link(obj);
		} else {
			LOG_SIMPLE(LOG_PROBLEMS, "FRL", "DAMAG",
					   tprintf("Freelist damaged, bad object #%d.", obj));
			obj = NOTHING;
			mudstate.freelist = NOTHING;
		}
	}
	if(obj == NOTHING) {
		obj = mudstate.db_top;
		db_grow(mudstate.db_top + 1);
	}
	atr_free(obj);				// Just in case...

	/*
	 * Set things up according to the object type 
	 */

	s_Location(obj, NOTHING);
	s_Contents(obj, NOTHING);
	s_Exits(obj, NOTHING);
	s_Next(obj, NOTHING);
	s_Link(obj, NOTHING);

	if(objtype == TYPE_ROOM && mudconf.room_parent > 0)
		s_Parent(obj, mudconf.room_parent);
	else if(objtype == TYPE_EXIT && mudconf.exit_parent > 0)
		s_Parent(obj, mudconf.exit_parent);
	else if(objtype == TYPE_PLAYER && mudconf.player_parent > 0)
		s_Parent(obj, mudconf.player_parent);
	else
		s_Parent(obj, NOTHING);

	if (objtype == TYPE_PLAYER && mudconf.player_zone > 0)
		s_Zone(obj, mudconf.player_zone);
	else
		s_Zone(obj, Zone(player));

	s_Flags(obj, objtype | f1);
	s_Flags2(obj, f2);
	s_Flags3(obj, f3);
	s_Owner(obj, (self_owned ? obj : owner));
	s_Pennies(obj, value);
	Unmark(obj);
	buff = munge_space((char *) name);
	s_Name(obj, buff);
	free_lbuf(buff);

	if(objtype == TYPE_PLAYER) {
		time(&tt);
		buff = (char *) ctime(&tt);
		buff[strlen(buff) - 1] = '\0';
		atr_add_raw(obj, A_LAST, buff);

		buff = alloc_sbuf("create_obj.quota");
		sprintf(buff, "%d", quota);
		atr_add_raw(obj, A_QUOTA, buff);
		atr_add_raw(obj, A_RQUOTA, buff);
		add_player_name(obj, Name(obj));
		free_sbuf(buff);
		s_Pennies(obj, 1000); /* Give the player some starting cash */
	}
	make_freelist();
	return obj;
}


/**
 * Destroy an object. Assumes it has already been removed from
 * all lists and has no contents or exits.
 */
void destroy_obj(dbref player, dbref obj)
{
	dbref owner;
	int good_owner, val, quota;
	STACK *sp, *next;
	char *tname;

	if(!Good_obj(obj))
		return;

	/*
	 * Validate the owner 
	 */

	owner = Owner(obj);
	good_owner = Good_owner(owner);

	/*
	 * Halt any pending commands (waiting or semaphore) 
	 */
	if(halt_que(NOTHING, obj) > 0) {
		if(good_owner && !Quiet(obj) && !Quiet(owner)) {
			notify(owner, "Halted.");
		}
	}
	nfy_que(obj, 0, NFY_DRAIN, 0);

	/*
	 * Compensate the owner for the object 
	 */

	val = 1;
	quota = 1;
	if(good_owner && (owner != obj)) {
		switch (Typeof(obj)) {
		case TYPE_ROOM:
			val = mudconf.digcost;
			quota = mudconf.room_quota;
			break;
		case TYPE_THING:
			val = OBJECT_DEPOSIT(Pennies(obj));
			quota = mudconf.thing_quota;
			break;
		case TYPE_EXIT:
			val = mudconf.opencost;
			quota = mudconf.exit_quota;
			break;
		case TYPE_PLAYER:
			if(Robot(obj))
				val = mudconf.robotcost;
			else
				val = 0;
			quota = mudconf.player_quota;
		}
		giveto(owner, val);
		if(mudconf.quotas)
			add_quota(owner, quota);

		if(!Quiet(owner) && !Quiet(obj))
			notify_printf(owner,
						  "You get back your %d %s deposit for %s(#%d).",
						  val, mudconf.one_coin, Name(obj), obj);
	}

	if((player != NOTHING) && !Quiet(player)) {
		if(good_owner && Owner(player) != owner) {
			if(owner == obj) {
				notify_printf(player, "Destroyed. %s(#%d)", Name(obj), obj);
			} else {
				tname = alloc_sbuf("destroy_obj");
				StringCopy(tname, Name(owner));
				notify_printf(player, "Destroyed. %s's %s(#%d)", tname,
							  Name(obj), obj);
				free_sbuf(tname);
			}
		} else if(!Quiet(obj)) {
			notify(player, "Destroyed.");
		}
	}

	atr_free(obj);
	s_Name(obj, "Garbage");
	s_Flags(obj, (TYPE_GARBAGE | GOING));
	s_Flags2(obj, 0);
	s_Flags3(obj, 0);
	s_Powers(obj, 0);
	s_Powers2(obj, 0);
	s_Location(obj, NOTHING);
	s_Contents(obj, NOTHING);
	s_Exits(obj, NOTHING);
	s_Next(obj, NOTHING);
	s_Link(obj, NOTHING);
	s_Owner(obj, GOD);
	s_Pennies(obj, 0);
	s_Parent(obj, NOTHING);
	s_Zone(obj, NOTHING);

	/*
	 * Clear the stack 
	 */
	for(sp = Stack(obj); sp != NULL; sp = next) {
		next = sp->next;
		free_lbuf(sp->data);
		free(sp);
	}

	s_Stack(obj, NULL);

	if(mudconf.have_comsys)
		toast_player(obj);

	make_freelist();
	return;
}

/**
 * Get rid of KEY contents of object.
 */
void divest_object(dbref thing)
{
	dbref curr, temp;

	SAFE_DOLIST(curr, temp, Contents(thing)) {
		if(!Controls(thing, curr) && Has_location(curr) && Key(curr)) {
			move_via_generic(curr, HOME, NOTHING, 0);
		}
	}
}

/**
 * Empties the contents of a GOING object.
 */
void empty_obj(dbref obj)
{
	dbref targ, next;

	/*
	 * Send the contents home 
	 */

	SAFE_DOLIST(targ, next, Contents(obj)) {
		if(!Has_location(targ)) {
			Log_simple_err(targ, obj,
						   "Funny object type in contents list of GOING location. Flush terminated.");
			break;
		} else if(Location(targ) != obj) {
			Log_header_err(targ, obj, Location(targ), 1, "Location",
						   "indicates object really in another location during cleanup of GOING location.  Flush terminated.");
			break;
		} else {
			ZAP_LOC(targ);
			if(Home(targ) == obj) {
				s_Home(targ, new_home(targ));
			}
			move_via_generic(targ, HOME, NOTHING, 0);
			divest_object(targ);
		}
	}

	/*
	 * Destroy the exits 
	 */

	SAFE_DOLIST(targ, next, Exits(obj)) {
		if(!isExit(targ)) {
			Log_simple_err(targ, obj,
						   "Funny object type in exit list of GOING location. Flush terminated.");
			break;
		} else if(Exits(targ) != obj) {
			Log_header_err(targ, obj, Exits(targ), 1, "Location",
						   "indicates exit really in another location during cleanup of GOING location.  Flush terminated.");
			break;
		} else {
			destroy_obj(NOTHING, targ);
		}
	}
}

/**
 * Destroys an exit.
 */
void destroy_exit(dbref exit)
{
	dbref loc;

	loc = Exits(exit);
	s_Exits(loc, remove_first(Exits(loc), exit));
	destroy_obj(NOTHING, exit);
}

/**
 * Destroys a thing.
 */
void destroy_thing(dbref thing)
{
	move_via_generic(thing, NOTHING, Owner(thing), 0);
	empty_obj(thing);
	destroy_obj(NOTHING, thing);
}

/** 
 * Destroys a player.
 */
void destroy_player(dbref victim)
{
	dbref aowner, player;
	int count; long aflags;
	char *buf;

	/*
	 * Bye bye... 
	 */
	player = (dbref) atoi(atr_get_raw(victim, A_DESTROYER));
	toast_player(victim);
	boot_off(victim, (char *) "You have been destroyed!");
	halt_que(victim, NOTHING);
	count = chown_all(victim, player);

	/*
	 * Remove the name from the name hash table 
	 */

	delete_player_name(victim, Name(victim));
	buf = atr_pget(victim, A_ALIAS, &aowner, &aflags);
	delete_player_name(victim, buf);
	free_lbuf(buf);

	move_via_generic(victim, NOTHING, player, 0);
	do_mail_clear(victim, NULL);
	do_mail_purge(victim);
	destroy_obj(NOTHING, victim);
	notify_quiet(player, tprintf("(%d objects @chowned to you)", count));
}

/**
 * Purges a GOING object. 
 */
static void purge_going(void)
{
	dbref i;

	DO_WHOLE_DB(i) {
		if(!Going(i))
			continue;

		switch (Typeof(i)) {
		case TYPE_PLAYER:
			destroy_player(i);
			break;
		case TYPE_ROOM:

			/*
			 * Room scheduled for destruction... do it 
			 */

			empty_obj(i);
			destroy_obj(NOTHING, i);
			break;
		case TYPE_THING:
			destroy_thing(i);
			break;
		case TYPE_EXIT:
			destroy_exit(i);
			break;
		case TYPE_GARBAGE:
			break;
		default:

			/*
			 * Something else... How did this happen? 
			 */

			Log_simple_err(i, NOTHING,
						   "GOING object with unexpected type.  Destroyed.");
			destroy_obj(NOTHING, i);
		}
	}
}

/**
 * Look for references to GOING or illegal objects.
 */
static void check_pennies(dbref thing, int limit, const char *qual)
{
	int j;

	if(Going(thing))
		return;
	j = Pennies(thing);
	if(isRoom(thing) || isExit(thing)) {
		if(j) {
			Log_header_err(thing, NOTHING, j, 0, qual, "is strange.  Reset.");
			s_Pennies(j, 0);
		}
	} else if(j == 0) {
		Log_header_err(thing, NOTHING, j, 0, qual, "is zero.");
	} else if(j < 0) {
		Log_header_err(thing, NOTHING, j, 0, qual, "is negative.");
	} else if(j > limit) {
		Log_header_err(thing, NOTHING, j, 0, qual, "is excessive.");
	}
}

static void check_dead_refs(void)
{
	dbref targ, owner, i, j;
	long aflags; int dirty;
	char *str;
	FWDLIST *fp;

	DO_WHOLE_DB(i) {

		/*
		 * Check the parent 
		 */

		targ = Parent(i);
		if(Good_obj(targ)) {
			if(Going(targ)) {
				s_Parent(i, NOTHING);
				owner = Owner(i);

				if(Good_owner(owner) && !Quiet(i) && !Quiet(owner)) {
					notify_printf(owner, "Parent cleared on %s(#%d)",
								  Name(i), i);
				}
			}
		} else if(targ != NOTHING) {
			Log_header_err(i, Location(i), targ, 1, "Parent",
						   "is invalid.  Cleared.");
			s_Parent(i, NOTHING);
		}
		/*
		 * Check the zone 
		 */

		targ = Zone(i);
		if(Good_obj(targ)) {
			if(Going(targ)) {
				s_Zone(i, NOTHING);
				owner = Owner(i);
				if(Good_owner(owner) && !Quiet(i) && !Quiet(owner)) {
					notify_printf(owner, "Zone cleared on %s(#%d)",
								  Name(i), i);
				}
			}
		} else if(targ != NOTHING) {
			Log_header_err(i, Location(i), targ, 1, "Zone",
						   "is invalid. Cleared.");
			s_Zone(i, NOTHING);
		}
		switch (Typeof(i)) {
		case TYPE_PLAYER:
		case TYPE_THING:

			if(Going(i))
				break;

			/*
			 * Check the home 
			 */

			targ = Home(i);
			if(Good_obj(targ)) {
				if(Going(targ)) {
					s_Home(i, new_home(i));
					owner = Owner(i);
					if(Good_owner(owner) && !Quiet(i) && !Quiet(owner)) {
						notify_printf(owner, "Home reset on %s(#%d)",
									  Name(i), i);
					}
				}
			} else if(targ != NOTHING) {
				Log_header_err(i, Location(i), targ, 1, "Home",
							   "is invalid.  Cleared.");
				s_Home(i, new_home(i));
			}
			/*
			 * Check the location 
			 */

			targ = Location(i);
			if(!Good_obj(targ)) {
				Log_pointer_err(NOTHING, i, NOTHING, targ, "Location",
								"is invalid.  Moved to home.");
				ZAP_LOC(i);
				move_object(i, HOME);
			}
			/*
			 * Check for self-referential Next() 
			 */

			if(Next(i) == i) {
				Log_simple_err(i, NOTHING,
							   "Next points to self.  Next cleared.");
				s_Next(i, NOTHING);
			}
			if(check_type & DBCK_FULL) {

				/*
				 * Check wealth or value 
				 */

				targ = OBJECT_ENDOWMENT(mudconf.createmax);
				if(OwnsOthers(i)) {
					targ += mudconf.paylimit;
					check_pennies(i, targ, "Wealth");
				} else {
					check_pennies(i, targ, "Value");
				}
			}
			break;
		case TYPE_ROOM:

			/*
			 * Check the dropto 
			 */

			targ = Dropto(i);
			if(Good_obj(targ)) {
				if(Going(targ)) {
					s_Dropto(i, NOTHING);
					owner = Owner(i);
					if(Good_owner(owner) && !Quiet(i) && !Quiet(owner)) {
						notify_printf(owner,
									  "Dropto removed from %s(#%d)", Name(i),
									  i);
					}
				}
			} else if((targ != NOTHING) && (targ != HOME)) {
				Log_header_err(i, NOTHING, targ, 1, "Dropto",
							   "is invalid.  Cleared.");
				s_Dropto(i, NOTHING);
			}
			if(check_type & DBCK_FULL) {

				/*
				 * NEXT should be null 
				 */

				if(Next(i) != NOTHING) {
					Log_header_err(i, NOTHING, Next(i), 1, "Next pointer",
								   "should be NOTHING.  Reset.");
					s_Next(i, NOTHING);
				}
				/*
				 * LINK should be null 
				 */

				if(Link(i) != NOTHING) {
					Log_header_err(i, NOTHING, Link(i), 1, "Link pointer ",
								   "should be NOTHING.  Reset.");
					s_Link(i, NOTHING);
				}
				/*
				 * Check value 
				 */

				check_pennies(i, 1, "Value");
			}
			break;
		case TYPE_EXIT:

			/*
			 * If it points to something GOING, set it going 
			 */

			targ = Location(i);
			if(Good_obj(targ)) {
				if(Going(targ)) {
					s_Going(i);
				}
			} else if(targ == HOME) {
				/*
				 * null case, HOME is always valid 
				 */
			} else if(targ != NOTHING) {
				Log_header_err(i, Exits(i), targ, 1, "Destination",
							   "is invalid.  Exit destroyed.");
				s_Going(i);
			} else {
				if(!Has_contents(targ)) {
					Log_header_err(i, Exits(i), targ, 1, "Destination",
								   "is not a valid type.  Exit destroyed.");
					s_Going(i);
				}
			}

			/*
			 * Check for self-referential Next() 
			 */

			if(Next(i) == i) {
				Log_simple_err(i, NOTHING,
							   "Next points to self.  Next cleared.");
				s_Next(i, NOTHING);
			}
			if(check_type & DBCK_FULL) {

				/*
				 * CONTENTS should be null 
				 */

				if(Contents(i) != NOTHING) {
					Log_header_err(i, Exits(i), Contents(i), 1, "Contents",
								   "should be NOTHING.  Reset.");
					s_Contents(i, NOTHING);
				}
				/*
				 * LINK should be null 
				 */

				if(Link(i) != NOTHING) {
					Log_header_err(i, Exits(i), Link(i), 1, "Link",
								   "should be NOTHING.  Reset.");
					s_Link(i, NOTHING);
				}
				/*
				 * Check value 
				 */

				check_pennies(i, 1, "Value");
			}
			break;
		case TYPE_GARBAGE:
			break;
		default:

			/*
			 * Funny object type, destroy it 
			 */

			Log_simple_err(i, NOTHING, "Funny object type.  Destroyed.");
			destroy_obj(NOTHING, i);
		}

		/*
		 * Check forwardlist 
		 */

		dirty = 0;
		fp = fwdlist_get(i);
		if(fp) {
			for(j = 0; j < fp->count; j++) {
				targ = fp->data[j];
				if(Good_obj(targ) && Going(targ)) {
					fp->data[j] = NOTHING;
					dirty = 1;
				} else if(!Good_obj(targ) && (targ != NOTHING)) {
					fp->data[j] = NOTHING;
					dirty = 1;
				}
			}
		}
		if(dirty) {
			str = alloc_lbuf("purge_going");
			(void) fwdlist_rewrite(fp, str);
			atr_get_info(i, A_FORWARDLIST, &owner, &aflags);
			atr_add(i, A_FORWARDLIST, str, owner, aflags);
			free_lbuf(str);
		}
		/*
		 * Check owner 
		 */

		owner = Owner(i);
		if(!Good_obj(owner)) {
			Log_header_err(i, NOTHING, owner, 1, "Owner",
						   "is invalid.  Set to GOD.");
			owner = GOD;
			s_Owner(i, owner);
			halt_que(NOTHING, i);
			s_Halted(i);
		} else if(check_type & DBCK_FULL) {
			if(Going(owner)) {
				Log_header_err(i, NOTHING, owner, 1, "Owner",
							   "is set GOING.  Set to GOD.");
				s_Owner(i, owner);
				halt_que(NOTHING, i);
				s_Halted(i);
			} else if(!OwnsOthers(owner)) {
				Log_header_err(i, NOTHING, owner, 1, "Owner",
							   "is not a valid owner type.");
			} else if(isPlayer(i) && (owner != i)) {
				Log_header_err(i, NOTHING, owner, 1, "Player",
							   "is the owner instead of the player.");
			}
		}
		if(check_type & DBCK_FULL) {

			/*
			 * Check for wizards 
			 */

			if(Wizard(i)) {
				if(isPlayer(i)) {
					Log_simple_err(i, NOTHING, "Player is a WIZARD.");
				}
				if(!Wizard(Owner(i))) {
					Log_header_err(i, NOTHING, Owner(i), 1, "Owner",
								   "of a WIZARD object is not a wizard");
				}
			}
		}
	}
}

/**
 * check_loc_exits, check_exit_chains: Validate the exits chains
 * of objects and attempt to correct problems. The following errors are
 * found and corrected:
 *       Location not in database                        - skip it.
 *       Location GOING                                  - skip it.
 *       Location not a PLAYER, ROOM, or THING           - skip it.
 *       Location already visited                        - skip it.
 *       Exit/next pointer not in database               - NULL it.
 *       Member is not an EXIT                           - terminate chain.
 *       Member is GOING                                 - destroy exit.
 *       Member already checked (is in another list)     - terminate chain.
 *       Member in another chain (recursive check)       - terminate chain.
 *       Location of member is not specified location    - reset it.
 */
static void check_loc_exits(dbref loc)
{
	dbref exit, back, temp, exitloc, dest;

	if(!Good_obj(loc))
		return;

	/*
	 * Only check players, rooms, and things that aren't GOING 
	 */

	if(isExit(loc) || Going(loc))
		return;

	/*
	 * If marked, we've checked here already 
	 */

	if(Marked(loc))
		return;
	Mark(loc);

	/*
	 * Check all the exits 
	 */

	back = NOTHING;
	exit = Exits(loc);
	while (exit != NOTHING) {

		exitloc = NOTHING;
		dest = NOTHING;

		if(Good_obj(exit)) {
			exitloc = Exits(exit);
			dest = Location(exit);
		}
		if(!Good_obj(exit)) {

			/*
			 * A bad pointer - terminate chain 
			 */

			Log_pointer_err(back, loc, NOTHING, exit, "Exit list",
							"is invalid.  List nulled.");
			if(back != NOTHING) {
				s_Next(back, NOTHING);
			} else {
				s_Exits(loc, NOTHING);
			}
			exit = NOTHING;
		} else if(!isExit(exit)) {

			/*
			 * Not an exit - terminate chain 
			 */

			Log_pointer_err(back, loc, NOTHING, exit, "Exitlist member",
							"is not an exit.  List terminated.");
			if(back != NOTHING) {
				s_Next(back, NOTHING);
			} else {
				s_Exits(loc, NOTHING);
			}
			exit = NOTHING;
		} else if(Going(exit)) {

			/*
			 * Going - silently filter out 
			 */

			temp = Next(exit);
			if(back != NOTHING) {
				s_Next(back, temp);
			} else {
				s_Exits(loc, temp);
			}
			destroy_obj(NOTHING, exit);
			exit = temp;
			continue;
		} else if(Marked(exit)) {

			/*
			 * Already in another list - terminate chain 
			 */

			Log_pointer_err(back, loc, NOTHING, exit, "Exitlist member",
							"is in another exitlist.  Cleared.");
			if(back != NOTHING) {
				s_Next(back, NOTHING);
			} else {
				s_Exits(loc, NOTHING);
			}
			exit = NOTHING;
		} else if(!Good_obj(dest) && (dest != HOME) && (dest != NOTHING)) {

			/*
			 * Destination is not in the db.  Null it. 
			 */

			Log_pointer_err(back, loc, NOTHING, exit, "Destination",
							"is invalid.  Cleared.");
			s_Location(exit, NOTHING);

		} else if(exitloc != loc) {

			/*
			 * Exit thinks it's in another place. Check the
			 * exitlist there and see if it contains this
			 * exit. If it does, then our exitlist
			 * somehow pointed into the middle of their
			 * exitlist. If not, assume we own the exit. 
			 */

			check_loc_exits(exitloc);
			if(Marked(exit)) {

				/*
				 * It's in the other list, give it up 
				 */

				Log_pointer_err(back, loc, NOTHING, exit, "",
								"is in another exitlist.  List terminated.");
				if(back != NOTHING) {
					s_Next(back, NOTHING);
				} else {
					s_Exits(loc, NOTHING);
				}
				exit = NOTHING;
			} else {

				/*
				 * Not in the other list, assume in ours 
				 */

				Log_header_err(exit, loc, exitloc, 1,
							   "Not on chain for location", "Reset.");
				s_Exits(exit, loc);
			}
		}
		if(exit != NOTHING) {

			/*
			 * All OK (or all was made OK) 
			 */

			if(check_type & DBCK_FULL) {

				/*
				 * Make sure exit owner owns at least one of
				 * * * * * the source or destination.  Just * 
				 * warn * if * * he doesn't. 
				 */

				temp = Owner(exit);
				if((temp != Owner(loc)) && (temp != Owner(Location(exit)))) {
					Log_header_err(exit, loc, temp, 1, "Owner",
								   "does not own either the source or destination.");
				}
			}
			Mark(exit);
			back = exit;
			exit = Next(exit);
		}
	}
	return;
}

static void check_exit_chains(void)
{
	dbref i;

	Unmark_all(i);
	DO_WHOLE_DB(i)
		check_loc_exits(i);
	DO_WHOLE_DB(i) {
		if(isExit(i) && !Marked(i)) {
			Log_simple_err(i, NOTHING, "Disconnected exit.  Destroyed.");
			destroy_obj(NOTHING, i);
		}
	}
}

/**
 * check_misplaced_obj, check_loc_contents, check_contents_chains: Validate
 * the contents chains of objects and attempt to correct problems.  The
 * following errors are found and corrected:
 *       Location not in database                        - skip it.
 *       Location GOING                                  - skip it.
 *       Location not a PLAYER, ROOM, or THING           - skip it.
 *       Location already visited                        - skip it.
 *       Contents/next pointer not in database           - NULL it.
 *       Member is not an PLAYER or THING                - terminate chain.
 *       Member is GOING                                 - destroy exit.
 *       Member already checked (is in another list)     - terminate chain.
 *       Member in another chain (recursive check)       - terminate chain.
 *       Location of member is not specified location    - reset it.
 */

static void check_loc_contents(dbref);

static void check_misplaced_obj(dbref * obj, dbref back, dbref loc)
{
	/*
	 * Object thinks it's in another place. Check the contents list
	 * there and see if it contains this object. If it does, then
	 * our contents list somehow pointed into the middle of their 
	 * contents list and we should truncate our list. If not,
	 * assume we own the object. 
	 */

	if(!Good_obj(*obj))
		return;
	loc = Location(*obj);
	Unmark(*obj);
	if(Good_obj(loc)) {
		check_loc_contents(loc);
	}
	if(Marked(*obj)) {

		/*
		 * It's in the other list, give it up 
		 */

		Log_pointer_err(back, loc, NOTHING, *obj, "",
						"is in another contents list.  Cleared.");
		if(back != NOTHING) {
			s_Next(back, NOTHING);
		} else {
			s_Contents(loc, NOTHING);
		}
		*obj = NOTHING;
	} else {
		/*
		 * Not in the other list, assume in ours 
		 */

		Log_header_err(*obj, loc, Contents(*obj), 1, "Location",
					   "is invalid.  Reset.");
		s_Contents(*obj, loc);
	}
	return;
}

static void check_loc_contents(dbref loc)
{
	dbref obj, back, temp;

	if(!Good_obj(loc))
		return;

	/*
	 * Only check players, rooms, and things that aren't GOING 
	 */

	if(isExit(loc) || Going(loc))
		return;

	/*
	 * Check all the exits 
	 */

	back = NOTHING;
	obj = Contents(loc);
	while (obj != NOTHING) {
		if(!Good_obj(obj)) {

			/*
			 * A bad pointer - terminate chain 
			 */

			Log_pointer_err(back, loc, NOTHING, obj, "Contents list",
							"is invalid.  Cleared.");
			if(back != NOTHING) {
				s_Next(back, NOTHING);
			} else {
				s_Contents(loc, NOTHING);
			}
			obj = NOTHING;
		} else if(!Has_location(obj)) {

			/*
			 * Not a player or thing - terminate chain 
			 */

			Log_pointer_err(back, loc, NOTHING, obj, "",
							"is not a player or thing.  Cleared.");
			if(back != NOTHING) {
				s_Next(back, NOTHING);
			} else {
				s_Contents(loc, NOTHING);
			}
			obj = NOTHING;
		} else if(Going(obj) && (Typeof(obj) == TYPE_GARBAGE)) {

			/*
			 * Going - silently filter out 
			 */

			temp = Next(obj);
			if(back != NOTHING) {
				s_Next(back, temp);
			} else {
				s_Contents(loc, temp);
			}
			destroy_obj(NOTHING, obj);
			obj = temp;
			continue;
		} else if(Marked(obj)) {

			/*
			 * Already visited - either truncate or ignore 
			 */

			if(Location(obj) != loc) {

				/*
				 * Location wrong - either truncate or fix 
				 */

				check_misplaced_obj(&obj, back, loc);
			} else {

				/*
				 * Location right - recursive contents 
				 */
			}
		} else if(Location(obj) != loc) {

			/*
			 * Location wrong - either truncate or fix 
			 */

			check_misplaced_obj(&obj, back, loc);
		}
		if(obj != NOTHING) {

			/*
			 * All OK (or all was made OK) 
			 */

			if(check_type & DBCK_FULL) {

				/*
				 * Check for wizard command-handlers inside * 
				 * 
				 * *  * *  * * nonwiz. Just warn if we find
				 * one. 
				 */

				if(Wizard(obj) && !Wizard(loc)) {
					if(Commer(obj)) {
						Log_simple_err(obj, loc,
									   "Wizard command handling object inside nonwizard.");
					}
				}
				/*
				 * Check for nonwizard objects inside wizard
				 * * * * * objects. 
				 */

				if(Wizard(loc) && !Wizard(obj) && !Wizard(Owner(obj))) {
					Log_simple_err(obj, loc,
								   "Nonwizard object inside wizard.");
				}
			}
			Mark(obj);
			back = obj;
			obj = Next(obj);
		}
	}
	return;
}

static void check_contents_chains(void)
{
	dbref i;

	Unmark_all(i);
	DO_WHOLE_DB(i)
		check_loc_contents(i);
	DO_WHOLE_DB(i)
		if(!Going(i) && !Marked(i) && Has_location(i)) {
		Log_simple_err(i, Location(i), "Orphaned object, moved home.");
		ZAP_LOC(i);
		move_via_generic(i, HOME, NOTHING, 0);
	}
}

/**
 * mark_place, check_floating: Look for floating rooms not set FLOATING.
 */
static void mark_place(dbref loc)
{
	dbref exit;

	/*
	 * If already marked, exit.  Otherwise set marked. 
	 */

	if(!Good_obj(loc))
		return;
	if(Marked(loc))
		return;
	Mark(loc);

	/*
	 * Visit all places you can get to via exits from here. 
	 */

	for(exit = Exits(loc); exit != NOTHING; exit = Next(exit)) {
		if(Good_obj(Location(exit)))
			mark_place(Location(exit));
	}
}

static void check_floating(void)
{
	dbref owner, i;

	/*
	 * Mark everyplace you can get to via exits from the starting room 
	 */

	Unmark_all(i);
	mark_place(mudconf.start_room);

	/*
	 * Look for rooms not marked and not set FLOATING 
	 */

	DO_WHOLE_DB(i) {
		if(isRoom(i) && !Floating(i) && !Going(i) && !Marked(i)) {
			owner = Owner(i);
			if(Good_owner(owner)) {
				notify_printf(owner, "You own a floating room: %s(#%d)",
							  Name(i), i);
			}
		}
	}
}

/**
 * Perform a database consistency check and clean up damage.
 */
void do_dbck(dbref player, dbref cause, int key)
{
	check_type = key;
	make_freelist();
	check_dead_refs();
	check_exit_chains();
	check_contents_chains();
	check_floating();
	purge_going();

	if(player != NOTHING) {
		if(!Quiet(player))
			notify(player, "Done.");
	}
}
