
/*
 * $Id: chargen_commands.h,v 1.1 2005/06/13 20:50:50 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Wed Sep 18 10:40:06 1996 fingon
 * Last modified: Thu May 14 20:50:26 1998 fingon
 *
 */

#ifndef CHARGEN_COMMANDS_H
#define CHARGEN_COMMANDS_H

#include "help.h"

void chargen_look(dbref player, void *data, char *buffer)
{
    struct chargen_struct *st;
    coolmenu *c;

    DOCHECK(State == NOTBEGUN, BEGINSTARTS);
    DOCHECK(State == DONE &&
	Applied == 2, "Type 'leave' to leave the booth!");
    c = find_proper_menu(st);
    if (State == DONE) {
	notify(player, "Options:");
	if (!Applied) {
	    notify(player, "\tApply Sets your plstats (no Prev after it)");
	    notify(player, "\tPrev  Go back to previous menu(s)");
	}
	notify(player, "\tDone  Finalizes your char creation");
	notify(player, "\tReset Lets you start chargen from beginning");
	return;
    }
    DOCHECK(!c, "Hrm.. no menu, are you sure you're 'k?");
    ShowCoolMenu(player, c);
}

void chargen_begin(dbref player, void *data, char *buffer)
{
    PSTATS *s;
    struct chargen_struct *st;

    DOCHECK(State,
	"You have begun already! Type 'reset' to cancel your present values.");
    DOCHECK(atoi(silly_atr_get(player, A_RANKNUM)) != INITIAL_RANK &&
	!Wiz(player), "There is no going back to chargen! Get out.");
    s = retrieve_stats(player, VALUES_ALL);
    clear_player(s);
    store_stats(player, s, VALUES_ALL);
    st->pritotal = 8;
    advance_state(player, st);	/* To picking priorities */
}

void chargen_apply(dbref player, void *data, char *buffer)
{
    /* Apply values in the N menus into character */
    struct chargen_struct *st;
    int i, j;

    DOCHECK(State != DONE, "You aren't yet done with your chargen!");
    DOCHECK(Applied,
	"Duhh.. Even we aren't that stupid. Be content doing it just once.");
    /* Whee.. looks like we've a valid character who wants their
       stats 'on character' */
    apply_values(player, st->cm[MENU_ADV], 1);
    apply_values(player, st->cm[MENU_ATT], 1);

/*   apply_values(player, st->cm[MENU_PACKSKI], 1); */
    if (st->eacount > 0) {
	j = 0;
	for (i = FIRST_ATT; i <= LAST_ATT; i++)
	    if (st->attributes[i] > 6)
		j |= 1 << (i - FIRST_ATT);
	char_setvaluebycode(player, EA_NUMBER, j);
    }
    for (i = 0; i < NUM_SKIMENUS; i++)
	apply_values(player, st->sm[i], 1);
    notify(player, "Your stats are now set! Enjoy!");
    Applied = 1;
}

void chargen_done(dbref player, void *data, char *buffer)
{
    struct chargen_struct *st, *t;
    int i;

    DOCHECK(State != DONE, "You aren't yet done with your chargen!");
    if (!Applied)
	chargen_apply(player, data, buffer);
    Applied = 2;
    notify(player,
	"Your chargen is now finished! You may leave the booth now.");
    if (!Wiz(player)) {
	silly_atr_set(player, A_JOB, "Looking for a faction to join");
	silly_atr_set(player, A_RANKNUM, tprintf("%d", FINAL_RANK));
    }
    if (chargen_list == st)
	chargen_list = st->next;
    else {
	for (t = chargen_list; t->next != st; t = t->next);
	t->next = st->next;
    }
    for (i = 0; i < NUM_MENUS; i++)
	if (st->cm[i])
	    free((void *) st->cm[i]);
    for (i = 0; i < NUM_SKIMENUS; i++)
	if (st->sm[i])
	    free((void *) st->sm[i]);
    free((void *) st);
}



void chargen_next(dbref player, void *data, char *buffer)
{
    struct chargen_struct *st;
    int i;

    DOCHECK(State == NOTBEGUN, BEGINSTARTS);
    if (can_advance_state(st)) {
	if ((i = can_proceed(player, st)) > 0)
	    advance_state(player, st);
	else if (!i) {
	    notify(player, "Checking data..");
	    notify(player,
		"Syntax error at line 217: Illegal information!");
	}
    } else
	notify(player, "Uh.. where do you want to go next, anyway?");
}

void chargen_prev(dbref player, void *data, char *buffer)
{
    struct chargen_struct *st;

    DOCHECK(State == NOTBEGUN, BEGINSTARTS);
    if (can_go_back_state(st))
	go_back_state(player, st);
    else
	notify(player, "Well.. we all want to go back, don't we?");
}


void chargen_reset(dbref player, void *data, char *buffer)
{
    struct chargen_struct *st;

    DOCHECK(State == NOTBEGUN,
	"You haven't even started yet! Type 'begin' instead.");
    bzero(st, sizeof(struct chargen_struct));

    chargen_begin(player, data, buffer);
}

void chargen_help(dbref player, void *data, char *buffer)
{
    char buf[MBUF_SIZE];

    strcpy(buf, "chargen");
    help_write(player, buf, &mudstate.news_htab, mudconf.news_file, 0);
}

#endif				/* CHARGEN_COMMANDS_H */
