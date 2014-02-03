/*
 * walkdb.c -- Support for commands that walk the entire db 
 */

#include "copyright.h"
#include "config.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "command.h"
#include "flags.h"
#include "powers.h"
#include "misc.h"
#include "alloc.h"

#ifdef MCHECK
#endif

/**
 * Bind occurances of the universal var in ACTION to ARG, then run ACTION.
 * Cmds run in low-prio Q after a 1 sec delay for the first one. 
 */
static void bind_and_queue(dbref player, dbref cause, char *action,
						   char *argstr, char *cargs[], int ncargs,
						   int number)
{
	char *command, *command2;	/*

								 * allocated by replace_string  
								 */

	command = replace_string(BOUND_VAR, argstr, action), command2 =
		replace_string(LISTPLACE_VAR, tprintf("%d", number), command);
	wait_que(player, cause, 0, NOTHING, 0, command2, cargs, ncargs,
			 mudstate.global_regs);
	free_lbuf(command);
	free_lbuf(command2);
}

/**
 * Iterates through a delimited string. Used in @dolist. 
 */
void do_dolist(dbref player, dbref cause, int key, char *list, char *command,
			   char *cargs[], int ncargs)
{
	char *curr, *objstring, delimiter = ' ';
	int number = 0;

	if(!list || *list == '\0') {
		notify(player,
			   "That's terrific, but what should I do with the list?");
		return;
	}
	curr = list;

	if(key == DOLIST_DELIMIT) {
		char *tempstr;

		if(strlen((tempstr = parse_to(&curr, ' ', EV_STRIP))) > 1) {
			notify(player, "The delimiter must be a single character!");
			return;
		}
		delimiter = *tempstr;
	}
	while (curr && *curr) {
		while (*curr == delimiter)
			curr++;
		if(*curr) {
			number++;
			objstring = parse_to(&curr, delimiter, EV_STRIP);
			bind_and_queue(player, cause, command, objstring, cargs,
						   ncargs, number);
		}
	}
}

/**
 * Regular @find command 
 */
void do_find(dbref player, dbref cause, int key, char *name)
{
	dbref i, low_bound, high_bound;
	char *buff;

	if(!payfor(player, mudconf.searchcost)) {
		notify_quiet(player, tprintf("You don't have enough %s.",
									 mudconf.many_coins));
		return;
	}
	parse_range(&name, &low_bound, &high_bound);
	for(i = low_bound; i <= high_bound; i++) {
		if((Typeof(i) != TYPE_EXIT) && controls(player, i) && (!*name ||
															   string_match
															   (PureName(i),
																name))) {
			buff = unparse_object(player, i, 0);
			notify(player, buff);
			free_lbuf(buff);
		}
	}
	notify(player, "***End of List***");
}

/**
 * Get counts of items in the db.
 */
int get_stats(dbref player, dbref who, STATS * info)
{
	dbref i;

	info->s_total = 0;
	info->s_rooms = 0;
	info->s_exits = 0;
	info->s_things = 0;
	info->s_players = 0;
	info->s_garbage = 0;

	/*
	 * Do we have permission? 
	 */

	if(Good_obj(who) && !Controls(player, who) && !Stat_Any(player)) {
		notify(player, "Permission denied.");
		return 0;
	}
	/*
	 * Can we afford it? 
	 */

	if(!payfor(player, mudconf.searchcost)) {
		notify_printf(player, "You don't have enough %s.",
					  mudconf.many_coins);
		return 0;
	}
	DO_WHOLE_DB(i) {
		if((who == NOTHING) || (who == Owner(i))) {
			info->s_total++;
			if(Going(i) && (Typeof(i) != TYPE_ROOM)) {
				info->s_garbage++;
				continue;
			}
			switch (Typeof(i)) {
			case TYPE_ROOM:
				info->s_rooms++;
				break;
			case TYPE_EXIT:
				info->s_exits++;
				break;
			case TYPE_THING:
				info->s_things++;
				break;
			case TYPE_PLAYER:
				info->s_players++;
				break;
			default:
				info->s_garbage++;
			}
		}
	}
	return 1;
}

/*
 * Get counts of items in the db.
 */
void do_stats(dbref player, dbref cause, int key, char *name)
{
	dbref owner;
	STATS statinfo;

	switch (key) {
	case STAT_ALL:
		owner = NOTHING;
		break;
	case STAT_ME:
		owner = Owner(player);
		break;
	case STAT_PLAYER:
		if(!(name && *name)) {
			notify_printf(player, "The universe contains %d objects.",
						  mudstate.db_top);
			return;
		}
		owner = lookup_player(player, name, 1);
		if(owner == NOTHING) {
			notify(player, "Not found.");
			return;
		}
		break;
	default:
		notify(player, "Illegal combination of switches.");
		return;
	}

	if(!get_stats(player, owner, &statinfo))
		return;
	notify_printf(player,
				  "%d objects = %d rooms, %d exits, %d things, %d players. (%d garbage)",
				  statinfo.s_total, statinfo.s_rooms, statinfo.s_exits,
				  statinfo.s_things, statinfo.s_players, statinfo.s_garbage);

#ifdef TEST_MALLOC
	if(Wizard(player))
		notify_printf(player, "Malloc count = %d.", malloc_count);
#endif /*
	    * TEST_MALLOC 
	    */
#ifdef MCHECK
	if(Wizard(player)) {
		struct mstats mval;

		mval = mstats();
		notify_printf(player, "Total size of the heap: %d", mval.bytes_total);
		notify_printf(player,
					  "Chunks allocated: %d -- Total size of allocated chunks: %d",
					  mval.chunks_used, mval.bytes_used);
		notify_printf(player,
					  "Chunks free: %d -- Total size of free chunks: %d",
					  mval.chunks_free, mval.bytes_free);
	}
#endif /*
	    * MCHECK 
	    */
}

/**
 * Transfers ownership of all a player's objects to another player.
 */
int chown_all(dbref from_player, dbref to_player)
{
	int i, count, quota_out, quota_in;

	if(Typeof(from_player) != TYPE_PLAYER)
		from_player = Owner(from_player);
	if(Typeof(to_player) != TYPE_PLAYER)
		to_player = Owner(to_player);
	count = 0;
	quota_out = 0;
	quota_in = 0;
	DO_WHOLE_DB(i) {
		if((Owner(i) == from_player) && (Owner(i) != i)) {
			switch (Typeof(i)) {
			case TYPE_PLAYER:
				s_Owner(i, i);
				quota_out += mudconf.player_quota;
				break;
			case TYPE_THING:
				s_Owner(i, to_player);
				quota_out += mudconf.thing_quota;
				quota_in -= mudconf.thing_quota;
				break;
			case TYPE_ROOM:
				s_Owner(i, to_player);
				quota_out += mudconf.room_quota;
				quota_in -= mudconf.room_quota;
				break;
			case TYPE_EXIT:
				s_Owner(i, to_player);
				quota_out += mudconf.exit_quota;
				quota_in -= mudconf.exit_quota;
				break;
			default:
				s_Owner(i, to_player);
			}
			s_Flags(i, (Flags(i) & ~(CHOWN_OK | INHERIT)) | HALT);
			count++;
		}
	}
	add_quota(from_player, quota_out);
	add_quota(to_player, quota_in);
	return count;
}

/**
 * Transfers ownership of all a player's objects to another player.
 * Used in @chownall
 */
void do_chownall(dbref player, dbref cause, int key, char *from, char *to)
{
	int count;
	dbref victim, recipient;

	init_match(player, from, TYPE_PLAYER);
	match_neighbor();
	match_absolute();
	match_player();
	if((victim = noisy_match_result()) == NOTHING)
		return;

	if((to != NULL) && *to) {
		init_match(player, to, TYPE_PLAYER);
		match_neighbor();
		match_absolute();
		match_player();
		if((recipient = noisy_match_result()) == NOTHING)
			return;
	} else {
		recipient = player;
	}

	count = chown_all(victim, recipient);
	if(!Quiet(player)) {
		notify_printf(player, "%d objects @chowned.", count);
	}
}

#define ANY_OWNER -2

void er_mark_disabled(dbref player)
{
	notify(player,
		   "The mark commands are not allowed while DB cleaning is enabled.");
	notify(player,
		   "Use the '@disable cleaning' command to disable automatic cleaning.");
	notify(player,
		   "Remember to '@unmark_all' before re-enabling automatic cleaning.");
}

/**
 * Walk the db reporting various things (or setting/clearing
 * mark bits)
 */
int search_setup(dbref player, char *searchfor, SEARCH * parm)
{
	char *pname, *searchtype, *t;
	int err;

	/*
	 * Crack arg into <pname> <type>=<targ>,<low>,<high> 
	 */

	pname = parse_to(&searchfor, '=', EV_STRIP_TS);
	if(!pname || !*pname) {
		pname = (char *) "me";
	} else
		for(t = pname; *t; t++) {
			if(isupper(*t))
				*t = tolower(*t);
		}

	if(searchfor && *searchfor) {
		searchtype = (char *) rindex(pname, ' ');
		if(searchtype) {
			*searchtype++ = '\0';
		} else {
			searchtype = pname;
			pname = (char *) "";
		}
	} else {
		searchtype = (char *) "";
	}

	/*
	 * If the player name is quoted, strip the quotes 
	 */

	if(*pname == '\"') {
		err = strlen(pname) - 1;
		if(pname[err] == '"') {
			pname[err] = '\0';
			pname++;
		}
	}
	/*
	 * Strip any range arguments 
	 */

	parse_range(&searchfor, &parm->low_bound, &parm->high_bound);

	/*
	 * set limits on who we search 
	 */

	parm->s_owner = Owner(player);
	parm->s_wizard = Search(player);
	parm->s_rst_owner = NOTHING;
	if(!*pname) {
		parm->s_rst_owner = parm->s_wizard ? ANY_OWNER : player;
	} else if(pname[0] == '#') {
		parm->s_rst_owner = atoi(&pname[1]);
		if(!Good_obj(parm->s_rst_owner))
			parm->s_rst_owner = NOTHING;
		else if(Typeof(parm->s_rst_owner) != TYPE_PLAYER)
			parm->s_rst_owner = NOTHING;

	} else if(strcmp(pname, "me") == 0) {
		parm->s_rst_owner = player;
	} else {
		parm->s_rst_owner = lookup_player(player, pname, 1);
	}

	if(parm->s_rst_owner == NOTHING) {
		notify_printf(player, "%s: No such player", pname);
		return 0;
	}
	/*
	 * set limits on what we search for 
	 */

	err = 0;
	parm->s_rst_name = NULL;
	parm->s_rst_eval = NULL;
	parm->s_rst_type = NOTYPE;
	parm->s_parent = NOTHING;
	parm->s_zone = NOTHING;
	parm->s_fset.word1 = 0;
	parm->s_fset.word2 = 0;
	parm->s_fset.word3 = 0;
	parm->s_pset.word1 = 0;
	parm->s_pset.word2 = 0;

	switch (searchtype[0]) {
	case '\0':					/*
								 * the no class requested class  :)  
								 */
		break;
	case 'e':
		if(string_prefix("exits", searchtype)) {
			parm->s_rst_name = searchfor;
			parm->s_rst_type = TYPE_EXIT;
		} else if(string_prefix("evaluate", searchtype)) {
			parm->s_rst_eval = searchfor;
		} else if(string_prefix("eplayer", searchtype)) {
			parm->s_rst_type = TYPE_PLAYER;
			parm->s_rst_eval = searchfor;
		} else if(string_prefix("eroom", searchtype)) {
			parm->s_rst_type = TYPE_ROOM;
			parm->s_rst_eval = searchfor;
		} else if(string_prefix("eobject", searchtype)) {
			parm->s_rst_type = TYPE_THING;
			parm->s_rst_eval = searchfor;
		} else if(string_prefix("ething", searchtype)) {
			parm->s_rst_type = TYPE_THING;
			parm->s_rst_eval = searchfor;
		} else if(string_prefix("eexit", searchtype)) {
			parm->s_rst_type = TYPE_EXIT;
			parm->s_rst_eval = searchfor;
		} else {
			err = 1;
		}
		break;
	case 'f':
		if(string_prefix("flags", searchtype)) {

			/*
			 * convert_flags ignores previous values of flag_mask
			 * * * * * and s_rst_type while setting them 
			 */

			if(!convert_flags(player, searchfor, &parm->s_fset,
							  &parm->s_rst_type))
				return 0;
		} else {
			err = 1;
		}
		break;
	case 'n':
		if(string_prefix("name", searchtype)) {
			parm->s_rst_name = searchfor;
		} else {
			err = 1;
		}
		break;
	case 'o':
		if(string_prefix("objects", searchtype)) {
			parm->s_rst_name = searchfor;
			parm->s_rst_type = TYPE_THING;
		} else {
			err = 1;
		}
		break;
	case 'p':
		if(string_prefix("players", searchtype)) {
			parm->s_rst_name = searchfor;
			parm->s_rst_type = TYPE_PLAYER;
			if(!*pname)
				parm->s_rst_owner = ANY_OWNER;
		} else if(string_prefix("parent", searchtype)) {
			parm->s_parent = match_controlled(player, searchfor);
			if(!Good_obj(parm->s_parent))
				return 0;
			if(!*pname)
				parm->s_rst_owner = ANY_OWNER;
		} else if(string_prefix("power", searchtype)) {
			if(!decode_power(player, searchfor, &parm->s_pset))
				return 0;
		} else {
			err = 1;

		}
		break;
	case 'r':
		if(string_prefix("rooms", searchtype)) {
			parm->s_rst_name = searchfor;
			parm->s_rst_type = TYPE_ROOM;
		} else {
			err = 1;
		}
		break;
	case 't':
		if(string_prefix("type", searchtype)) {
			if(searchfor[0] == '\0')
				break;
			if(string_prefix("rooms", searchfor))
				parm->s_rst_type = TYPE_ROOM;
			else if(string_prefix("exits", searchfor))
				parm->s_rst_type = TYPE_EXIT;
			else if(string_prefix("objects", searchfor))
				parm->s_rst_type = TYPE_THING;
			else if(string_prefix("things", searchfor))
				parm->s_rst_type = TYPE_THING;
			else if(string_prefix("garbage", searchfor))
				parm->s_rst_type = TYPE_GARBAGE;
			else if(string_prefix("players", searchfor)) {
				parm->s_rst_type = TYPE_PLAYER;
				if(!*pname)
					parm->s_rst_owner = ANY_OWNER;
			} else {
				notify_printf(player, "%s: unknown type", searchfor);
				return 0;
			}
		} else if(string_prefix("things", searchtype)) {
			parm->s_rst_name = searchfor;
			parm->s_rst_type = TYPE_THING;
		} else {
			err = 1;
		}
		break;
	case 'z':
		if(string_prefix("zone", searchtype)) {
			parm->s_zone = match_controlled(player, searchfor);
			if(!Good_obj(parm->s_zone))
				return 0;
			if(!*pname)
				parm->s_rst_owner = ANY_OWNER;
		} else {
			err = 1;

		}
		break;
	default:
		err = 1;
	}

	if(err) {
		notify_printf(player, "%s: unknown class", searchtype);
		return 0;
	}
	/*
	 * Make sure player is authorized to do the search 
	 */

	if(!parm->s_wizard && (parm->s_rst_type != TYPE_PLAYER) &&
	   (parm->s_rst_owner != player) && (parm->s_rst_owner != ANY_OWNER)) {
		notify(player, "You need a search warrant to do that!");
		return 0;
	}
	/*
	 * make sure player has money to do the search 
	 */

	if(!payfor(player, mudconf.searchcost)) {
		notify_printf(player,
					  "You don't have enough %s to search. (You need %d)",
					  mudconf.many_coins, mudconf.searchcost);
		return 0;
	}
	return 1;
}

void search_perform(dbref player, dbref cause, SEARCH * parm)
{
	FLAG thing1flags, thing2flags, thing3flags;
	POWER thing1powers, thing2powers;
	dbref thing;
	char *buff, *buff2, *result, *bp, *str;
	int save_invk_ctr;

	buff = alloc_sbuf("search_perform.num");
	save_invk_ctr = mudstate.func_invk_ctr;

	for(thing = parm->low_bound; thing <= parm->high_bound; thing++) {
		mudstate.func_invk_ctr = save_invk_ctr;

		/*
		 * Check for matching type 
		 */

		if((parm->s_rst_type != NOTYPE) &&
		   (parm->s_rst_type != Typeof(thing)))
			continue;

		/*
		 * Check for matching owner 
		 */

		if((parm->s_rst_owner != ANY_OWNER) &&
		   (parm->s_rst_owner != Owner(thing)))
			continue;

		/*
		 * Check for matching parent 
		 */

		if((parm->s_parent != NOTHING) && (parm->s_parent != Parent(thing)))
			continue;

		/*
		 * Check for matching zone 
		 */

		if((parm->s_zone != NOTHING) && (parm->s_zone != Zone(thing)))
			continue;

		/*
		 * Check for matching flags 
		 */

		thing3flags = Flags3(thing);
		thing2flags = Flags2(thing);
		thing1flags = Flags(thing);
		if((thing1flags & parm->s_fset.word1) != parm->s_fset.word1)
			continue;
		if((thing2flags & parm->s_fset.word2) != parm->s_fset.word2)
			continue;
		if((thing3flags & parm->s_fset.word3) != parm->s_fset.word3)
			continue;

		/*
		 * Check for matching power 
		 */

		thing1powers = Powers(thing);
		thing2powers = Powers2(thing);
		if((thing1powers & parm->s_pset.word1) != parm->s_pset.word1)
			continue;
		if((thing2powers & parm->s_pset.word2) != parm->s_pset.word2)
			continue;

		/*
		 * Check for matching name 
		 */

		if(parm->s_rst_name != NULL) {
			if(!string_prefix((char *) PureName(thing), parm->s_rst_name))
				continue;
		}
		/*
		 * Check for successful evaluation 
		 */

		if(parm->s_rst_eval != NULL) {
			if(Typeof(thing) == TYPE_GARBAGE)
				continue;
			sprintf(buff, "#%ld", thing);
			buff2 = replace_string(BOUND_VAR, buff, parm->s_rst_eval);
			result = bp = alloc_lbuf("search_perform");
			str = buff2;
			exec(result, &bp, 0, player, cause,
				 EV_FCHECK | EV_EVAL | EV_NOTRACE, &str, (char **) NULL, 0);
			*bp = '\0';
			free_lbuf(buff2);
			if(!*result || !xlate(result)) {
				free_lbuf(result);
				continue;
			}
			free_lbuf(result);
		}
		/*
		 * It passed everything.  Amazing. 
		 */

		olist_add(thing);
	}
	free_sbuf(buff);
	mudstate.func_invk_ctr = save_invk_ctr;
}

void do_search(dbref player, dbref cause, int key, char *arg)
{
	int flag, destitute;
	int rcount, ecount, tcount, pcount, gcount;
	char *buff, *outbuf, *bp;
	dbref thing, from, to;
	SEARCH searchparm;

	if(!search_setup(player, arg, &searchparm))
		return;
	olist_push();
	search_perform(player, cause, &searchparm);
	destitute = 1;

	outbuf = alloc_lbuf("do_search.outbuf");

	rcount = ecount = tcount = pcount = gcount = 0;

	/*
	 * room search 
	 */
	if(searchparm.s_rst_type == TYPE_ROOM || searchparm.s_rst_type == NOTYPE) {
		flag = 1;
		for(thing = olist_first(); thing != NOTHING; thing = olist_next()) {
			if(Typeof(thing) != TYPE_ROOM)
				continue;
			if(flag) {
				flag = 0;
				destitute = 0;
				notify(player, "\nROOMS:");
			}
			buff = unparse_object(player, thing, 0);
			notify(player, buff);
			free_lbuf(buff);
			rcount++;
		}
	}
	/*
	 * exit search 
	 */
	if(searchparm.s_rst_type == TYPE_EXIT || searchparm.s_rst_type == NOTYPE) {
		flag = 1;
		for(thing = olist_first(); thing != NOTHING; thing = olist_next()) {
			if(Typeof(thing) != TYPE_EXIT)
				continue;
			if(flag) {
				flag = 0;
				destitute = 0;
				notify(player, "\nEXITS:");
			}
			from = Exits(thing);
			to = Location(thing);

			bp = outbuf;
			buff = unparse_object(player, thing, 0);
			safe_str(buff, outbuf, &bp);
			free_lbuf(buff);

			safe_str((char *) " [from ", outbuf, &bp);
			buff = unparse_object(player, from, 0);
			safe_str(((from == NOTHING) ? "NOWHERE" : buff), outbuf, &bp);
			free_lbuf(buff);

			safe_str((char *) " to ", outbuf, &bp);
			buff = unparse_object(player, to, 0);
			safe_str(((to == NOTHING) ? "NOWHERE" : buff), outbuf, &bp);
			free_lbuf(buff);

			safe_chr(']', outbuf, &bp);
			*bp = '\0';
			notify(player, outbuf);
			ecount++;
		}
	}
	/*
	 * object search 
	 */
	if(searchparm.s_rst_type == TYPE_THING || searchparm.s_rst_type == NOTYPE) {
		flag = 1;
		for(thing = olist_first(); thing != NOTHING; thing = olist_next()) {
			if(Typeof(thing) != TYPE_THING)
				continue;
			if(flag) {
				flag = 0;
				destitute = 0;
				notify(player, "\nOBJECTS:");
			}
			bp = outbuf;
			buff = unparse_object(player, thing, 0);
			safe_str(buff, outbuf, &bp);
			free_lbuf(buff);

			safe_str((char *) " [owner: ", outbuf, &bp);
			buff = unparse_object(player, Owner(thing), 0);
			safe_str(buff, outbuf, &bp);
			free_lbuf(buff);

			safe_chr(']', outbuf, &bp);
			*bp = '\0';
			notify(player, outbuf);
			tcount++;
		}
	}
	/*
	 * garbage search 
	 */
	if(searchparm.s_rst_type == TYPE_GARBAGE ||
	   searchparm.s_rst_type == NOTYPE) {
		flag = 1;
		for(thing = olist_first(); thing != NOTHING; thing = olist_next()) {
			if(Typeof(thing) != TYPE_GARBAGE)
				continue;
			if(flag) {
				flag = 0;
				destitute = 0;
				notify(player, "\nGARBAGE:");
			}
			bp = outbuf;
			buff = unparse_object(player, thing, 0);
			safe_str(buff, outbuf, &bp);
			free_lbuf(buff);

			safe_str((char *) " [owner: ", outbuf, &bp);
			buff = unparse_object(player, Owner(thing), 0);
			safe_str(buff, outbuf, &bp);
			free_lbuf(buff);

			safe_chr(']', outbuf, &bp);
			*bp = '\0';
			notify(player, outbuf);
			gcount++;
		}
	}
	/*
	 * player search 
	 */
	if(searchparm.s_rst_type == TYPE_PLAYER ||
	   searchparm.s_rst_type == NOTYPE) {
		flag = 1;
		for(thing = olist_first(); thing != NOTHING; thing = olist_next()) {
			if(Typeof(thing) != TYPE_PLAYER)
				continue;
			if(flag) {
				flag = 0;
				destitute = 0;
				notify(player, "\nPLAYERS:");
			}
			bp = outbuf;
			buff = unparse_object(player, thing, 0);
			safe_str(buff, outbuf, &bp);
			free_lbuf(buff);
			if(searchparm.s_wizard) {
				safe_str((char *) " [location: ", outbuf, &bp);
				buff = unparse_object(player, Location(thing), 0);
				safe_str(buff, outbuf, &bp);
				free_lbuf(buff);
				safe_chr(']', outbuf, &bp);
			}
			*bp = '\0';
			notify(player, outbuf);
			pcount++;
		}
	}
	/*
	 * if nothing found matching search criteria 
	 */

	if(destitute) {
		notify(player, "Nothing found.");
	} else {
		sprintf(outbuf,
				"\nFound:  Rooms...%d  Exits...%d  Objects...%d  Players...%d  Garbage...%d",
				rcount, ecount, tcount, pcount, gcount);
		notify(player, outbuf);
	}
	free_lbuf(outbuf);
	olist_pop();
}

/**
 * Object list management routines:
 * olist_push, olist_pop, olist_add, olist_first, olist_next
 */

/**
 * Create a new object list at the top of the object list stack
 */
void olist_push(void)
{
	OLSTK *ol;

	ol = (OLSTK *) XMALLOC(sizeof(OLSTK), "olist_push");
	ol->next = mudstate.olist;
	mudstate.olist = ol;

	ol->head = NULL;
	ol->tail = NULL;
	ol->cblock = NULL;
	ol->count = 0;
	ol->citm = 0;
}

/**
 * Pop one entire list off the object list stack
 */
void olist_pop(void)
{
	OLSTK *ol;
	OBLOCK *op, *onext;

	ol = mudstate.olist->next;

	for(op = mudstate.olist->head; op != NULL; op = onext) {
		onext = op->next;
		free_lbuf(op);
	}
	XFREE(mudstate.olist, "olist_pop");
	mudstate.olist = ol;
}

/**
 * Add an entry to the object list 
 */
void olist_add(dbref item)
{
	OBLOCK *op;

	if(!mudstate.olist->head) {
		op = (OBLOCK *) alloc_lbuf("olist_add.first");
		mudstate.olist->head = mudstate.olist->tail = op;
		mudstate.olist->count = 0;
		op->next = NULL;
	} else if(mudstate.olist->count >= OBLOCK_SIZE) {
		op = (OBLOCK *) alloc_lbuf("olist_add.next");
		mudstate.olist->tail->next = op;
		mudstate.olist->tail = op;
		mudstate.olist->count = 0;
		op->next = NULL;
	} else {
		op = mudstate.olist->tail;
	}
	op->data[mudstate.olist->count++] = item;
}

/**
 * Return the first entry in the object list 
 */
dbref olist_first(void)
{
	if(!mudstate.olist->head)
		return NOTHING;
	if((mudstate.olist->head == mudstate.olist->tail) &&
	   (mudstate.olist->count == 0))
		return NOTHING;
	mudstate.olist->cblock = mudstate.olist->head;
	mudstate.olist->citm = 0;
	return mudstate.olist->cblock->data[mudstate.olist->citm++];
}

dbref olist_next(void)
{
	dbref thing;

	if(!mudstate.olist->cblock)
		return NOTHING;
	if((mudstate.olist->cblock == mudstate.olist->tail) &&
	   (mudstate.olist->citm >= mudstate.olist->count))
		return NOTHING;
	thing = mudstate.olist->cblock->data[mudstate.olist->citm++];
	if(mudstate.olist->citm >= OBLOCK_SIZE) {
		mudstate.olist->cblock = mudstate.olist->cblock->next;
		mudstate.olist->citm = 0;
	}
	return thing;
}
