/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Mon Sep 16 23:02:01 1996 fingon
 * Last modified: Sat Jun  6 19:48:06 1998 fingon
 *
 */

/* Functions for toggling / changing values / changing strings */

/* These are the generic form ones */
static coolmenu *retrieve_matching_letter(coolmenu * c, char l)
{
    l = tolower(l);
    for (; c; c = c->next)
	if (c->letter == l)
	    return c;
    return NULL;
}

#if 0
static coolmenu *retrieve_matching_id(coolmenu * c, int i)
{
    for (; c; c = c->next)
	if (c->id == i)
	    return c;
    return NULL;
}
#endif
#ifdef DASMAGIC4
#define MAYBESHOW ShowCoolMenu(player, c)
#else
#define MAYBESHOW
#endif

static void update_entry(dbref player, coolmenu * c, char l, int val)
{
    int o;
    coolmenu *d = retrieve_matching_letter(c, l);

    DOCHECK(!d, "Invalid letter!");
    DOCHECK(c->flags & CM_NUMBER, "Invalid type of field!");
    o = d->value;
    d->value += val;
    if (d->value < 0) {
	val = 0 - o;
	d->value = 0;
    }
    if (d->value > d->maxvalue) {
	val = d->maxvalue - o;
	d->value = d->maxvalue;
    }
    DOCHECK(!val,
	"Uh.. You think about changing something and then don't.");
    if (val > 0)
	notify_printf(player, "%s increased by %d to %d!", d->text, val,
		d->value);
    else
	notify_printf(player, "%s decreased by %d to %d!", d->text,
		0 - val, d->value);
    DASMAGIC3;
    MAYBESHOW;
}

static void update_entry_toggle(dbref player, coolmenu * c, char l)
{
    coolmenu *d = retrieve_matching_letter(c, l);

    DOCHECK(!d, "Invalid letter!");
    DOCHECK(!(d->flags & CM_TOGGLE), "Invalid type of field!");
#ifndef REAL_SNEAKY_SET
    if (d->value)
	notify_printf(player, "%s set off!", d->text);
    else
	notify_printf(player, "%s set on!", d->text);
#endif
    d->value = !d->value;
    DASMAGIC3;
    MAYBESHOW;
}

static void update_entry_set(dbref player, coolmenu * c, char l,
    char *buffer)
{
    coolmenu *d = retrieve_matching_letter(c, l);
    int i;

    DOCHECK(!d, "Invalid letter!");
    DOCHECK(!(d->flags & (CM_STRING | CM_NUMBER)),
	"Invalid type of field!");
    if (d->flags & CM_STRING) {
	if (d->text)
	    free((void *) d->text);
	d->text = strdup(buffer);
    } else {
	i = atoi(buffer);
	if (i > d->maxvalue)
	    i = d->maxvalue;
	DOCHECK(i < 0,
	    "You consider a negative value, and then forget about it.");
	notify_printf(player, "%s set to %d!", d->text, i);
	d->value = i;
    }
    DASMAGIC3;
    MAYBESHOW;
}



#define CMD(a) void a (dbref player, void *data, char *buffer)


#define COMMAND_ADD(fname,letter,mod) \
CMD(fname) \
{ DASMAGIC; DOCHECK(!c, "Huh?"); if (buffer && (strlen(buffer) > 1 || (buffer[0] && buffer[0] != ' '))) \
{ if (atoi(buffer) > 0) \
   update_entry(player, DASMAGIC2, letter, mod*atoi(buffer)); \
 else \
   notify(player, "Invalid argument!"); \
} else update_entry(player, DASMAGIC2, letter, mod*1); }

#define COMMAND_TOGGLE(fname,letter) \
CMD(fname) \
{ DASMAGIC; DOCHECK(!c, "Huh?"); if (buffer && (strlen(buffer) > 1 || (buffer[0] && buffer[0] != ' ')))  notify(player, "Invalid argument!"); \
 else update_entry_toggle(player, DASMAGIC2, letter); }

#define COMMAND_SET(fname,letter) \
CMD(fname) \
{ DASMAGIC; DOCHECK(!c, "Huh?"); if (!(buffer && (strlen(buffer) > 1 || (buffer[0] && buffer[0] != ' ')))) notify(player, "Lack argument(s)!"); \
 else update_entry_set(player, DASMAGIC2, letter, buffer); }

#define COMMANDS(bname,letter) \
COMMAND_ADD(bname ## _add,letter,1); \
COMMAND_ADD(bname ## _minus,letter,-1); \
COMMAND_TOGGLE(bname ## _toggle,letter); \
COMMAND_SET(bname ## _set,letter);

#define COMMANDSET(name) \
COMMANDS(name ## _a,'a'); \
COMMANDS(name ## _b,'b'); \
COMMANDS(name ## _c,'c'); \
COMMANDS(name ## _d,'d'); \
COMMANDS(name ## _e,'e'); \
COMMANDS(name ## _f,'f'); \
COMMANDS(name ## _g,'g'); \
COMMANDS(name ## _h,'h'); \
COMMANDS(name ## _i,'i'); \
COMMANDS(name ## _j,'j'); \
COMMANDS(name ## _k,'k'); \
COMMANDS(name ## _l,'l'); \
COMMANDS(name ## _m,'m'); \
COMMANDS(name ## _n,'n'); \
COMMANDS(name ## _o,'o'); \
COMMANDS(name ## _p,'p'); \
COMMANDS(name ## _q,'q'); \
COMMANDS(name ## _r,'r'); \
COMMANDS(name ## _s,'s'); \
COMMANDS(name ## _t,'t'); \
COMMANDS(name ## _u,'u'); \
COMMANDS(name ## _v,'v'); \
COMMANDS(name ## _w,'w'); \
COMMANDS(name ## _x,'x'); \
COMMANDS(name ## _y,'y'); \
COMMANDS(name ## _z,'z');
