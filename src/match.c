/*
 * match.c -- Routines for parsing arguments 
 */

#include "copyright.h"
#include "config.h"

#include "config.h"
#include "db.h"
#include "mudconf.h"
#include "externs.h"
#include "match.h"
#include "attrs.h"
#include "powers.h"

#define	CON_LOCAL		0x01	/*
								 * Match is near me 
								 */
#define	CON_TYPE		0x02	/*
								 * Match is of requested type 
								 */
#define	CON_LOCK		0x04	/*
								 * I pass the lock on match 
								 */
#define	CON_COMPLETE		0x08	/*
									 * Name given is the full name 
									 */
#define	CON_TOKEN		0x10	/*
								 * Name is a special token 
								 */
#define	CON_DBREF		0x20	/*
								 * Name is a dbref 
								 */

static MSTATE md;

static void promote_match(dbref what, int confidence)
{
	/*
	 * Check for type and locks, if requested 
	 */

	if(md.pref_type != NOTYPE) {
		if(Good_obj(what) && (Typeof(what) == md.pref_type))
			confidence |= CON_TYPE;
	}
	if(md.check_keys) {
		MSTATE save_md;

		save_match_state(&save_md);
		if(Good_obj(what) && could_doit(md.player, what, A_LOCK));
		confidence |= CON_LOCK;
		restore_match_state(&save_md);
	}
	/*
	 * If nothing matched, take it 
	 */

	if(md.count == 0) {
		md.match = what;
		md.confidence = confidence;
		md.count = 1;
		return;
	}
	/*
	 * If confidence is lower, ignore 
	 */

	if(confidence < md.confidence) {
		return;
	}
	/*
	 * If confidence is higher, replace 
	 */

	if(confidence > md.confidence) {
		md.match = what;
		md.confidence = confidence;
		md.count = 1;
		return;
	}
	/*
	 * Equal confidence, pick randomly 
	 */

	if(random() % 2) {
		md.match = what;
	}
	md.count++;
	return;
}

/*
 * ---------------------------------------------------------------------------
 * * This function removes repeated spaces from the template to which object
 * * names are being matched.  It also removes inital and terminal spaces.
 */

static char *munge_space_for_match(char *name)
{
	static char buffer[LBUF_SIZE];
	char *p, *q;

	p = name;
	q = buffer;
	while (isspace(*p))
		p++;					/*
								 * remove inital spaces 
								 */
	while (*p) {
		while (*p && !isspace(*p))
			*q++ = *p++;
		while (*p && isspace(*++p));
		if(*p)
			*q++ = ' ';
	}
	*q = '\0';					/*
								 * remove terminal spaces and terminate * * * 
								 * 
								 * * string 
								 */
	return (buffer);
}

void match_player(void)
{
	dbref match;
	char *p;

	if(md.confidence >= CON_DBREF) {
		return;
	}
	if(Good_obj(md.absolute_form) && isPlayer(md.absolute_form)) {
		promote_match(md.absolute_form, CON_DBREF);
		return;
	}
	if(*md.string == LOOKUP_TOKEN) {
		for(p = md.string + 1; isspace(*p); p++);
		match = lookup_player(NOTHING, p, 1);
		if(Good_obj(match)) {
			promote_match(match, CON_TOKEN);
		}
	}
}

/*
 * returns nnn if name = #nnn, else NOTHING 
 */

static dbref absolute_name(int need_pound)
{
	dbref match;
	char *mname;

	mname = md.string;
	if(need_pound) {
		if(*md.string != NUMBER_TOKEN) {
			return NOTHING;
		} else {
			mname++;
		}
	}
	match = parse_dbref(mname);
	if(Good_obj(match)) {
		return match;
	}
	return NOTHING;
}

void match_absolute(void)
{
	if(md.confidence >= CON_DBREF)
		return;
	if(Good_obj(md.absolute_form))
		promote_match(md.absolute_form, CON_DBREF);
}

void match_numeric(void)
{
	dbref match;

	if(md.confidence >= CON_DBREF)
		return;
	match = absolute_name(0);
	if(Good_obj(match))
		promote_match(match, CON_DBREF);
}

void match_me(void)
{
	if(md.confidence >= CON_DBREF)
		return;
	if(Good_obj(md.absolute_form) && (md.absolute_form == md.player)) {
		promote_match(md.player, CON_DBREF | CON_LOCAL);
		return;
	}
	if(!string_compare(md.string, "me"))
		promote_match(md.player, CON_TOKEN | CON_LOCAL);
	return;
}

void match_home(void)
{
	if(md.confidence >= CON_DBREF)
		return;
	if(!string_compare(md.string, "home"))
		promote_match(HOME, CON_TOKEN);
	return;
}

void match_here(void)
{
	dbref loc;

	if(md.confidence >= CON_DBREF)
		return;
	if(Good_obj(md.player) && Has_location(md.player)) {
		loc = Location(md.player);
		if(Good_obj(loc)) {
			if(loc == md.absolute_form) {
				promote_match(loc, CON_DBREF | CON_LOCAL);
			} else if(!string_compare(md.string, "here")) {
				promote_match(loc, CON_TOKEN | CON_LOCAL);
			} else if(!string_compare(md.string, (char *) PureName(loc))) {
				promote_match(loc, CON_COMPLETE | CON_LOCAL);
			}
		}
	}
}

static void match_list(dbref first, int local)
{
	char *namebuf;

	if(md.confidence >= CON_DBREF)
		return;
	DOLIST(first, first) {
		if(first == md.absolute_form) {
			promote_match(first, CON_DBREF | local);
			return;
		}
		/*
		 * Warning: make sure there are no other calls to Name() in 
		 * promote_match or its called subroutines; they
		 * would overwrite Name()'s static buffer which is
		 * needed by string_match(). 
		 */
		namebuf = (char *) PureName(first);

		if(!string_compare(namebuf, md.string)) {
			promote_match(first, CON_COMPLETE | local);
		} else if(string_match(namebuf, md.string)) {
			promote_match(first, local);
		}
	}
}

void match_possession(void)
{
	if(md.confidence >= CON_DBREF)
		return;
	if(Good_obj(md.player) && Has_contents(md.player))
		match_list(Contents(md.player), CON_LOCAL);
}

void match_neighbor(void)
{
	dbref loc;

	if(md.confidence >= CON_DBREF)
		return;
	if(Good_obj(md.player) && Has_location(md.player)) {
		loc = Location(md.player);
		if(Good_obj(loc)) {
			match_list(Contents(loc), CON_LOCAL);
		}
	}
}

static int match_exit_internal(dbref loc, dbref baseloc, int local)
{
	dbref exit;
	int result, key;

	if(!Good_obj(loc) || !Has_exits(loc))
		return 1;

	result = 0;
	DOLIST(exit, Exits(loc)) {
		if(exit == md.absolute_form) {
			key = 0;
			if(Examinable(md.player, loc))
				key |= VE_LOC_XAM;
			if(Dark(loc))
				key |= VE_LOC_DARK;
			if(Dark(baseloc))
				key |= VE_BASE_DARK;
			if(exit_visible(exit, md.player, key)) {
				promote_match(exit, CON_DBREF | local);
				return 1;
			}
		}
		if(matches_exit_from_list(md.string, (char *) PureName(exit))) {
			promote_match(exit, CON_COMPLETE | local);
			result = 1;
		}
	}
	return result;
}

void match_exit(void)
{
	dbref loc;

	if(md.confidence >= CON_DBREF)
		return;
	loc = Location(md.player);
	if(Good_obj(md.player) && Has_location(md.player))
		(void) match_exit_internal(loc, loc, CON_LOCAL);
}

void match_exit_with_parents(void)
{
	dbref loc, parent;
	int lev;

	if(md.confidence >= CON_DBREF)
		return;
	if(Good_obj(md.player) && Has_location(md.player)) {
		loc = Location(md.player);
		ITER_PARENTS(loc, parent, lev) {
			if(match_exit_internal(parent, loc, CON_LOCAL))
				break;
		}
	}
}

void match_carried_exit(void)
{
	if(md.confidence >= CON_DBREF)
		return;
	if(Good_obj(md.player) && Has_exits(md.player))
		(void) match_exit_internal(md.player, md.player, CON_LOCAL);
}

void match_carried_exit_with_parents(void)
{
	dbref parent;
	int lev;

	if(md.confidence >= CON_DBREF)
		return;
	if(Good_obj(md.player) && (Has_exits(md.player) || isRoom(md.player))) {
		ITER_PARENTS(md.player, parent, lev) {
			if(match_exit_internal(parent, md.player, CON_LOCAL))
				break;
		}
	}
}

void match_master_exit(void)
{
	if(md.confidence >= CON_DBREF)
		return;
	if(Good_obj(md.player) && Has_exits(md.player))
		(void) match_exit_internal(mudconf.master_room,
								   mudconf.master_room, 0);
}

void match_zone_exit(void)
{
	if(md.confidence >= CON_DBREF)
		return;
	if(Good_obj(md.player) && Has_exits(md.player))
		(void) match_exit_internal(Zone(md.player), Zone(md.player), 0);
}

void match_everything(int key)
{
	/*
	 * Try matching me, then here, then absolute, then player FIRST, since
	 * this will hit most cases. STOP if we get something, since those are
	 * exact matches.
	 */

	match_me();
	match_here();
	match_absolute();
	if(key & MAT_NUMERIC)
		match_numeric();
	if(key & MAT_HOME)
		match_home();
	match_player();
	if(md.confidence >= CON_TOKEN)
		return;

	if(!(key & MAT_NO_EXITS)) {
		if(key & MAT_EXIT_PARENTS) {
			match_carried_exit_with_parents();
			match_exit_with_parents();
		} else {
			match_carried_exit();
			match_exit();
		}
	}
	match_neighbor();
	match_possession();
}

dbref match_result(void)
{
	switch (md.count) {
	case 0:
		return NOTHING;
	case 1:
		return md.match;
	default:
		return AMBIGUOUS;
	}
}

/*
 * use this if you don't care about ambiguity 
 */

dbref last_match_result(void)
{
	return md.match;
}

dbref match_status(player, match)
	 dbref player, match;
{
	switch (match) {
	case NOTHING:
		notify(player, NOMATCH_MESSAGE);
		return NOTHING;
	case AMBIGUOUS:
		notify(player, AMBIGUOUS_MESSAGE);
		return NOTHING;
	case NOPERM:
		notify(player, NOPERM_MESSAGE);
		return NOTHING;
	}
	if(Good_obj(match) && Dark(match) && Good_obj(player) &&
	   !WizRoy(Owner(player)) && !Builder(Owner(player)))
		return match_status(player, NOTHING);
	return match;
}

dbref noisy_match_result(void)
{
	return match_status(md.player, match_result());
}

dbref dispatched_match_result(player)
	 dbref player;
{
	return match_status(player, match_result());
}

int matched_locally(void)
{
	return (md.confidence & CON_LOCAL);
}

void save_match_state(mstate)
	 MSTATE *mstate;
{
	mstate->confidence = md.confidence;
	mstate->count = md.count;
	mstate->pref_type = md.pref_type;
	mstate->check_keys = md.check_keys;
	mstate->absolute_form = md.absolute_form;
	mstate->match = md.match;
	mstate->player = md.player;
	mstate->string = alloc_lbuf("save_match_state");
	StringCopy(mstate->string, md.string);
}

void restore_match_state(mstate)
	 MSTATE *mstate;
{
	md.confidence = mstate->confidence;
	md.count = mstate->count;
	md.pref_type = mstate->pref_type;
	md.check_keys = mstate->check_keys;
	md.absolute_form = mstate->absolute_form;
	md.match = mstate->match;
	md.player = mstate->player;
	StringCopy(md.string, mstate->string);
	free_lbuf(mstate->string);
}

void init_match(dbref player, char *name, int type)
{
	md.confidence = -1;
	md.count = md.check_keys = 0;
	md.pref_type = type;
	md.match = NOTHING;
	md.player = player;
	md.string = munge_space_for_match((char *) name);
	md.absolute_form = absolute_name(1);
}

void init_match_check_keys(dbref player, char *name, int type)
{
	init_match(player, name, type);
	md.check_keys = 1;
}
