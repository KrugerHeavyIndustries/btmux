/*
 * rob.c -- Commands dealing with giving/taking/killing things or money 
 */

#include "copyright.h"
#include "config.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "alloc.h"
#include "attrs.h"
#include "powers.h"
#include "p.comsys.h"

/*
 * ---------------------------------------------------------------------------
 * * give_thing, give_money, do_give: Give away money or things.
 */

static void give_thing(dbref giver, dbref recipient, int key, char *what)
{
	dbref thing;
	char *str, *sp;

	init_match(giver, what, TYPE_THING);
	match_possession();
	match_me();
	thing = match_result();

	switch (thing) {
	case NOTHING:
		notify(giver, "You don't have that!");
		return;
	case AMBIGUOUS:
		notify(giver, "I don't know which you mean!");
		return;
	}

	if(thing == giver) {
		notify(giver, "You can't give yourself away!");
		return;
	}
	if(((Typeof(thing) != TYPE_THING) && (Typeof(thing) != TYPE_PLAYER))
	   || !(Enter_ok(recipient) || controls(giver, recipient))) {
		notify(giver, "Permission denied.");
		return;
	}
	if(!could_doit(giver, thing, A_LGIVE)) {
		sp = str = alloc_lbuf("do_give.gfail");
		safe_str((char *) "You can't give ", str, &sp);
		safe_str(Name(thing), str, &sp);
		safe_str((char *) " away.", str, &sp);
		*sp = '\0';

		did_it(giver, thing, A_GFAIL, str, A_OGFAIL, NULL, A_AGFAIL,
			   (char **) NULL, 0);
		free_lbuf(str);
		return;
	}
	if(!could_doit(thing, recipient, A_LRECEIVE)) {
		sp = str = alloc_lbuf("do_give.rfail");
		safe_str(Name(recipient), str, &sp);
		safe_str((char *) " doesn't want ", str, &sp);
		safe_str(Name(thing), str, &sp);
		safe_chr('.', str, &sp);
		*sp = '\0';

		did_it(giver, recipient, A_RFAIL, str, A_ORFAIL, NULL, A_ARFAIL,
			   (char **) NULL, 0);
		free_lbuf(str);
		return;
	}
	move_via_generic(thing, recipient, giver, 0);
	divest_object(thing);
	if(!(key & GIVE_QUIET)) {
		str = alloc_lbuf("do_give.thing.ok");
		StringCopy(str, Name(giver));
		notify_with_cause(recipient, giver, tprintf("%s gave you %s.", str,
													Name(thing)));
		notify(giver, "Given.");
		notify_with_cause(thing, giver, tprintf("%s gave you to %s.", str,
												Name(recipient)));
		free_lbuf(str);
	}
	did_it(giver, thing, A_DROP, NULL, A_ODROP, NULL, A_ADROP,
		   (char **) NULL, 0);
	did_it(recipient, thing, A_SUCC, NULL, A_OSUCC, NULL, A_ASUCC,
		   (char **) NULL, 0);
}

static void give_money(dbref giver, dbref recipient, int key, int amount)
{
	dbref aowner;
	int cost, aflags;
	char *str;

	/*
	 * do amount consistency check 
	 */

	if(amount < 0 && !Steal(giver)) {
		notify_printf(giver,
					  "You look through your pockets. Nope, no negative %s.",
					  mudconf.many_coins);
		return;
	}
	if(!amount) {
		notify_printf(giver, "You must specify a positive number of %s.",
					  mudconf.many_coins);
		return;
	}
	if(!Wizard(giver)) {
		if((Typeof(recipient) == TYPE_PLAYER) &&
		   (Pennies(recipient) + amount > mudconf.paylimit)) {
			notify_printf(giver, "That player doesn't need that many %s!",
						  mudconf.many_coins);
			return;
		}
		if(!could_doit(giver, recipient, A_LUSE)) {
			notify_printf(giver, "%s won't take your money.",
						  Name(recipient));
			return;
		}
	}
	/*
	 * try to do the give 
	 */

	if(!payfor(giver, amount)) {
		notify_printf(giver, "You don't have that many %s to give!",
					  mudconf.many_coins);
		return;
	}
	/*
	 * Find out cost if an object 
	 */

	if(Typeof(recipient) == TYPE_THING) {
		str = atr_pget(recipient, A_COST, &aowner, &aflags);
		cost = atoi(str);
		free_lbuf(str);

		/*
		 * Can't afford it? 
		 */

		if(amount < cost) {
			notify(giver, "Feeling poor today?");
			giveto(giver, amount);
			return;
		}
		/*
		 * Negative cost 
		 */

		if(cost < 0) {
			return;
		}
	} else {
		cost = amount;
	}

	if(!(key & GIVE_QUIET)) {
		if(amount == 1) {
			notify_printf(giver, "You give a %s to %s.", mudconf.one_coin,
						  Name(recipient));
			notify_with_cause(recipient, giver,
							  tprintf("%s gives you a %s.", Name(giver),
									  mudconf.one_coin));
		} else {
			notify_printf(giver, "You give %d %s to %s.", amount,
						  mudconf.many_coins, Name(recipient));
			notify_with_cause(recipient, giver,
							  tprintf("%s gives you %d %s.", Name(giver),
									  amount, mudconf.many_coins));
		}
	}
	/*
	 * Report change given 
	 */

	if((amount - cost) == 1) {
		notify_printf(giver, "You get 1 %s in change.", mudconf.one_coin);
		giveto(giver, 1);
	} else if(amount != cost) {
		notify_printf(giver, "You get %d %s in change.", (amount - cost),
					  mudconf.many_coins);
		giveto(giver, (amount - cost));
	}
	/*
	 * Transfer the money and run PAY attributes 
	 */

	giveto(recipient, cost);
	did_it(giver, recipient, A_PAY, NULL, A_OPAY, NULL, A_APAY,
		   (char **) NULL, 0);
	return;
}

void do_give(dbref player, dbref cause, int key, char *who, char *amnt)
{
	dbref recipient;

	/*
	 * check recipient 
	 */

	init_match(player, who, TYPE_PLAYER);
	match_neighbor();
	match_possession();
	match_me();
	if(Long_Fingers(player)) {
		match_player();
		match_absolute();
	}
	recipient = match_result();
	switch (recipient) {
	case NOTHING:
		notify(player, "Give to whom?");
		return;
	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		return;
	}

	if(Guest(recipient)) {
		notify(player, "Guest really doesn't need money or anything.");
		return;
	}
	if(is_number(amnt)) {
		give_money(player, recipient, key, atoi(amnt));
	} else {
		give_thing(player, recipient, key, amnt);
	}
}
