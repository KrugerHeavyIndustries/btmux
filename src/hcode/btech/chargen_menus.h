
/*
 * $Id: chargen_menus.h,v 1.1 2005/06/13 20:50:50 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Wed Sep 18 01:47:15 1996 fingon
 * Last modified: Sat Jun  6 20:01:48 1998 fingon
 *
 */

#ifndef CHARGEN_MENUS_H
#define CHARGEN_MENUS_H

int lowest_bit(int num)
{
    int i, j;

    for (i = 0;; i++) {
	j = 1 << i;
	if (j > num)
	    return -1;
	if (num & j)
	    return i;
    }
}

/* Chargen's default menu creation functions */
static coolmenu *create_menu_of_charvalues(dbref player, char *heading,
    int type, int flag, int maxval)
{
    coolmenu *c = NULL;
    int i, t, f, co = 0, wb;
    char buf[512];

    if (heading)
	strcpy(buf, heading);
    else {
	if (flag > 0) {
	    i = lowest_bit(flag);
	    sprintf(buf, "%s %ss for %s", btech_charskillflag_names[i],
		&(btech_charvaluetype_names[type][5]), Name(player));
	} else
	    sprintf(buf, "%ss for %s",
		&(btech_charvaluetype_names[type][5]), Name(player));
    }
    buf[0] = toupper(buf[0]);
    CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
    CreateMenuEntry_Simple(&c, buf, CM_ONE | CM_CENTER);
    CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
    for (i = 0; i < NUM_CHARVALUES; i++)
	if ((t = char_values[i].type) == type || type < 0)
	    if (((f = char_values[i].flag) & flag) || flag < 0)
		co++;
    wb = CoolMenu_FPWBit(co, 18);
    for (i = 0; i < NUM_CHARVALUES; i++)
	if ((t = char_values[i].type) == type || type < 0)
	    if (((f = char_values[i].flag) & flag) || flag < 0) {
		if (t == CHAR_ADVANTAGE && f == CHAR_ADV_BOOL)
		    CreateMenuEntry_Normal(&c, char_values[i].name,
			wb | CM_TOGGLE, i + 1, 1);
		else
		    CreateMenuEntry_Normal(&c, char_values[i].name,
			wb | CM_NUMBER, i + 1, maxval);
	    }
    CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
    return c;
}

static coolmenu *create_packskill_menu(dbref player,
    struct chargen_struct *st)
{
    coolmenu *c = NULL;
    int i, t, f, co = 0, wb;
    int type = CHAR_SKILL;
    int flag = st->chosen_packages + CAREER_MISC;

    if (st->chosen_packagetype >= BASIC_UNIV)
	flag |= CAREER_ACADMISC;
    CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
    CreateMenuEntry_Simple(&c, tprintf("Package Skills for %s",
	    Name(player)), CM_ONE | CM_CENTER);
    CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
    for (i = 0; i < NUM_CHARVALUES; i++)
	if ((t = char_values[i].type) == type)
	    if (((f = char_values[i].flag) & flag))
		co++;
    wb = CoolMenu_FPWBit(co, 18);
    for (i = 0; i < NUM_CHARVALUES; i++)
	if ((t = char_values[i].type) == type)
	    if (((f = char_values[i].flag) & flag))
		CreateMenuEntry_Normal(&c, char_values[i].name,
		    wb | CM_NUMBER, i + 1, 3);
    CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
    CreateMenuEntry_Simple(&c, "Prev = Previous menu", CM_TWO | CM_CENTER);
    CreateMenuEntry_Simple(&c, "Next = Next menu", CM_TWO | CM_CENTER);
    CreateMenuEntry_Normal(&c, "Status", CM_ONE | CM_CENTER, -1, 0);
    CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
    return c;
}

#endif				/* CHARGEN_MENUS_H */
