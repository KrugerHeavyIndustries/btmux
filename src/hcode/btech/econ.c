
/*
 * $Id: econ.c,v 1.1.1.1 2005/01/11 21:18:06 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Sat Oct  5 14:06:02 1996 fingon
 * Last modified: Sat Apr 19 13:54:56 1997 fingon
 *
 */

/* Idea:
   Store the parts in an attribute on object

   Format:
   [id,brand,count]{,[id,brand,count],..}
 */

#include "mech.h"

extern char *silly_atr_get(int id, int flag);
extern void silly_atr_set(int id, int flag, char *dat);

/* entry = pointer to [ */
static void remove_entry(char *alku, char *entry)
{
	char *j;

	if(!(j = strstr(entry, "]")))
		return;
	j++;
	if(*j) {
		/* Move the remainder of the string, including the terminating NUL,
		   but not including the separating comma */
		j++;
		memmove(entry, j, strlen(j) + 1);
	} else {
		if(entry == alku)
			*alku = '\0';
		else
			*(entry - 1) = '\0';
	}
}

static void add_entry(char *to, char *data)
{
	if(*to)
		sprintf(to + strlen(to), ",[%s]", data);
	else
		sprintf(to, "[%s]", data);
}

static char *find_entry(char *s, int i, int b)
{
	char buf[MBUF_SIZE];

	sprintf(buf, "[%d,%d,", i, b);
	return strstr(s, buf);
}

extern char *get_parts_short_name(int, int);

void econ_change_items(dbref d, int id, int brand, int num)
{
	char *t, *u;
	int base = 0, i1, i2, i3;

	if(!Good_obj(d))
		return;
	if(brand)
		if(get_parts_short_name(id, brand) == get_parts_short_name(id, 0))
			brand = 0;
	t = silly_atr_get(d, A_ECONPARTS);
	if((u = find_entry(t, id, brand))) {
		if(sscanf(u, "[%d,%d,%d]", &i1, &i2, &i3) == 3)
			base += i3;
		remove_entry(t, u);
	}
	base += num;
	if(base <= 0) {
		if(u)
			silly_atr_set(d, A_ECONPARTS, t);
		return;
	}
	if(!(IsActuator(id)))
		add_entry(t, tprintf("%d,%d,%d", id, brand, base));
	silly_atr_set(d, A_ECONPARTS, t);
	if(IsActuator(id))
		econ_change_items(d, Cargo(S_ACTUATOR), brand, base);
	/* Successfully changed */
}

int econ_find_items(dbref d, int id, int brand)
{
	char *t, *u;
	int i1, i2, i3;

	if(!Good_obj(d))
		return 0;
	if(brand)
		if(get_parts_short_name(id, brand) == get_parts_short_name(id, 0))
			brand = 0;
	t = silly_atr_get(d, A_ECONPARTS);
	if((u = find_entry(t, id, brand)))
		if(sscanf(u, "[%d,%d,%d]", &i1, &i2, &i3) == 3)
			return i3;
	return 0;
}

void econ_set_items(dbref d, int id, int brand, int num)
{
	int i;

	if(!Good_obj(d))
		return;
	i = econ_find_items(d, id, brand);
	if(i != num)
		econ_change_items(d, id, brand, num - i);
}
