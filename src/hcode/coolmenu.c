
/*
 * $Id: coolmenu.c,v 1.1 2005/06/13 20:50:49 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Mon Sep 16 20:38:36 1996 fingon
 * Last modified: Wed Jun 24 22:41:40 1998 fingon
 *
 */

#include <stdio.h>
#include <string.h>
#include "db.h"

void KillText(char **mapt);
void ShowText(char **mapt, dbref player);

/* 
   Simple menu system for cool menus ;-)
   */
#include "db.h"
#include "coolmenu.h"
#include "create.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

int BOUNDED(int, int, int);

int number_of_entries(coolmenu * c)
{
	if(c->flags & CM_ONE)
		return 1;
	if(c->flags & CM_TWO)
		return 2;

	if(c->flags & CM_THREE)
		return 3;
	if(c->flags & CM_FOUR)
		return 4;
	return 1;
}

int count_following_with(coolmenu * c, int num)
{
	int count = 0;

	for(; c && number_of_entries(c) >= num && count < num; c = c->next)
		count++;
	return count;
}

void display_line(char **c, int *len, coolmenu * m)
{
	char *ch = *c;
	int i;

	sprintf(ch, "%%cb");
	ch += strlen(ch);
	for(i = 0; i < *len; i++)
		*(ch++) = '-';
	sprintf(ch, "%%c");
	ch += strlen(ch);
	*len = 0;
	*c = ch;
}

static int compute_length(char *s)
{
	int l = strlen(s);
	char *c;

	for(c = s; *c; c++) {
		if(*c == '%')
			if(*(c + 1) == 'c') {
				if(isalpha(*(c + 2))) {
					c += 2;
					l -= 3;
				} else {
					c += 1;
					l -= 2;
				}
			}
	}
	return l;
}

void display_string(char **c, int *len, coolmenu * m)
{
	int l = strlen(m->text), lo;
	int p, e;
	int i;

	if(m->flags & CM_NOCUT) {
		*len = 1;
		strcpy(*c, m->text);
		*c += strlen(*c);
		return;
	}
	if(m->flags & CM_CENTER) {
		p = MAX(*len / 2 - l / 2, 0);
		e = MIN(*len - 1, p + l);
		for(i = 0; i < p; i++)
			(*c)[i] = ' ';
		*c += p;
		sprintf(*c, "%%ch%%cb");
		*c += strlen(*c);
		strncpy(*c, m->text, (e - p) + 1);
		*c += (e - p);
		sprintf(*c, "%%c");
		*c += strlen(*c);
		**c = 0;
		*len -= e;
	} else {
		lo = l - compute_length(m->text);
		l = MIN(*len - 1 + lo, l);
		strncpy(*c, m->text, l);
		(*c)[l] = 0;
		*len -= l - lo;
		*c = &((*c)[l]);
	}
}

void display_toggle_end(char **c, coolmenu * m)
{
	if(m->value)
		sprintf(*c, " %s<%%cbX%%c%%ch>%%c",
				!(m->flags & CM_NO_HILITE) ? "%ch" : "");
	else
		sprintf(*c, " < >");
	*c += strlen(*c);
}

/* Turn value into equivalent with kilo, mega, giga, tera, peta, exa, zetta
   or yotta postfix. */
char *stringified_value(int v)
{
	char foo[] = "KMGTPEZY";
	int i = -1;
	static char buf[5];

	if(v > 999) {
		do {
			i++;
			v /= 1000;
		} while (v > 999 && foo[i]);

		if(!foo[i])
			i--;
		sprintf(buf, "%d%c", BOUNDED(0, v, 999), foo[i]);
	} else
		sprintf(buf, "%d", BOUNDED(0, v, 999));
	return buf;
}

void display_number_end(char **c, coolmenu * m)
{
	if(m->value >= 0) {
		sprintf(*c, " %%cg%s%4s%%c", (m->value > 0 &&
									  !(m->
										flags & CM_NO_HILITE)) ? "%ch" : "",
				stringified_value(m->value));
	} else
		sprintf(*c, " ____");
	*c += strlen(*c);
}

char *display_entry(char *ch, int maxlen, coolmenu * c)
{
	int i, j = 0, t = 0;

	/* returns: number of characters to forward the main pointer with.
	   basically: strlen(ouradditions) */
	if((c->flags & (LETTERFIRST)) && !(c->flags & CM_NOTOG)) {
		if(c->flags & CM_NUMBER)
			maxlen -= 5;
		else
			maxlen -= 4;
		t = ((c->flags & (CM_TOGGLE | CM_NUMBER)) && c->value);
		sprintf(ch, "%s[%c]%s ", (t &&
								  !(c->
									flags & CM_NO_HILITE)) ? "%ch%cr" : "%cr",
				t ? (c->letter + 'A' - 'a') : c->letter, "%c");
		ch += strlen(ch);
	}
	if(c->flags & (RIGHTEDGES) && !(c->flags & CM_NORIGHT)) {
		if(c->flags & CM_NUMBER)
			maxlen -= 6;
		else
			maxlen -= 5;
		j = 1;
	}
	if(t && !(c->flags & (CM_NO_HILITE))) {
		sprintf(ch, "%%ch");
		ch += strlen(ch);
	}
	if(c->flags & CM_LINE)
		display_line(&ch, &maxlen, c);
	else
		display_string(&ch, &maxlen, c);
	if(t && !(c->flags & (CM_NO_HILITE))) {
		sprintf(ch, "%%c");
		ch += strlen(ch);
	}
	if(maxlen > 0 && !(c->flags & CM_NOCUT)) {
		for(i = 0; i < maxlen; i++)
			*(ch++) = ' ';
	}
	if(j) {
		if(c->flags & CM_TOGGLE)
			display_toggle_end(&ch, c);
		else if(c->flags & CM_NUMBER)
			display_number_end(&ch, c);
		*(ch++) = ' ';
	}
	*ch = 0;
	return ch;
}

void display_entries(coolmenu * c, int wnum, int num, char *text)
{
	int i;
	char *ch = text;
	int single_length = (MENU_CHAR_WIDTH / wnum);

	for(i = 0; i < num; i++) {
		ch = display_entry(ch, single_length, c);
		c = c->next;
	}
}

char **MakeCoolMenuText(coolmenu * c)
{
	char **m;
	int pos = 0;
	int n, rn;

	Create(m, char *, MAX_MENU_LENGTH + 1);

	/* Whole whopping menu is ready to be written at.. */
	while (c)
		if((n = number_of_entries(c)))
			if((rn = count_following_with(c, n))) {
				Create(m[pos], char, MAX_MENU_WIDTH);

/* 	  display_entries(c,rn,m[pos++]); */
				display_entries(c, n, rn, m[pos++]);
				while (rn > 0 && c) {
					rn--;
					c = c->next;
				}
			}
	return m;
}

void CreateMenuEntry_Killer(coolmenu ** c, char *text, int flag, int id,
							int value, int maxvalue)
{
	coolmenu *d, *e;
	char first = 'a';

	if(!*c) {
		Create(*c, coolmenu, 1);
		d = *c;
	} else {
		for(d = *c; d->next; d = d->next);
		Create(d->next, coolmenu, 1);
		d = d->next;
	}
	if(text)
		d->text = strdup(text);
	d->flags = flag;
	if((flag & LETTERFIRST) && !(flag & CM_NOTOG)) {
		/* gasp, s'pose we need a letter for this thingy */
		for(e = *c; e; e = e->next)
			if(e->letter)
				if(e->letter >= first)
					first = e->letter + 1;
		d->letter = first;
	}
	d->id = id;
	d->value = value;
	d->maxvalue = maxvalue;
}

void KillCoolMenu(coolmenu * c)
{
	coolmenu *d;

	for(; c; c = d) {
		d = c->next;
		if(c->text)
			free((void *) c->text);
		free((void *) c);
	}
}

void ShowCoolMenu(dbref player, coolmenu * c)
{
	char **ch;

	ch = MakeCoolMenuText(c);
	ShowText(ch, player);
	KillText(ch);
}

int CoolMenu_FPWBit(int number, int maxlen)
{
	if(number <= maxlen)
		return CM_ONE;
	if(number <= (maxlen * 2))
		return CM_TWO;
	if(number <= (maxlen * 3))
		return CM_THREE;
	return CM_FOUR;
}

coolmenu *SelCol_Menu(int columns, char *heading, char **strings, int type,
					  int max)
{
	coolmenu *c = NULL;
	int i, co = 0;
	char buf[LBUF_SIZE];

	strcpy(buf, heading);
	buf[0] = toupper(buf[0]);
	CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
	CreateMenuEntry_Simple(&c, buf, CM_ONE | CM_CENTER);
	CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
	for(co = 0; strings[co]; co++);
	if(columns < 0)
		columns = CoolMenu_FPWBit(co, 18);
	for(i = 0; i < co; i++)
		CreateMenuEntry_Normal(&c, strings[i], columns | type, i + 1, max);
	CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
	return c;
}

coolmenu *SelCol_FunStringMenuK(int columns, char *heading,
								char *(*fun) (), int last)
{
	coolmenu *c = NULL;
	int i;
	char buf[LBUF_SIZE];
	int sick = 0;

	strcpy(buf, heading);
	buf[0] = toupper(buf[0]);
	CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
	CreateMenuEntry_Simple(&c, buf, CM_ONE | CM_CENTER);
	if(fun(0)[0] == '%') {
		CreateMenuEntry_Normal(&c, fun(0), columns, 1, 0);
		sick = 1;
	}
	CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
	if(columns < 0)
		columns = CoolMenu_FPWBit(last, 18);
	for(i = sick; i < last; i++)
		CreateMenuEntry_Normal(&c, fun(i), columns, i + 1 - sick, 0);
	CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
	return c;
}

coolmenu *SelCol_FunStringMenu(int columns, char *heading, char *(*fun) ())
{
	int co;

	for(co = 0; fun(co); co++);
	return SelCol_FunStringMenuK(columns, heading, fun, co);
}
