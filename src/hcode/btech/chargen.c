
/*
 * $Id: chargen.c,v 1.1.1.1 2005/01/11 21:18:04 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Wed Sep 18 00:35:56 1996 fingon
 * Last modified: Sat Jun  6 19:59:03 1998 fingon
 *
 */

/*
   Basic/Advanced Academy
   -//-       University
   (     -//-       Intelligence College)

   Basic Academy        9
   3@1, 3@2
   Advanced Academy    15
   2@1, 2@2, 2@3
   Aerospace,Infantry,Specialist(Recon/Tech),Battlemech,Cavalry

   Basic University    12
   4@1,4@2
   Advanced university 18
   3@1,3@2,2@3
   same as academy + Military Science + Leadership (2 careers)

   None                0
 */


/* Alternative, menu-based chargen */

/* Due to peculiar nature of this file, it's not compiled but included
   at end of btechstats.c */

#include "coolmenu.h"

#define BEGINSTARTS "Type 'begin' to start!"
#define BEGIN_PRI_POINTS 8

enum {
    NOTBEGUN, PRI_PICK, ADV_PICK, ATT_PICK, PACK_PICK, PACKSET_PICK,
    PACKSKI_PICK, SKI_PICK, DONE
};
enum {
    MENU_PRI, MENU_ADV, MENU_ATT, MENU_PACK, MENU_PACKSET, MENU_PACKSKI,
    NUM_MENUS
};
enum {
    ATHLETIC, MENTAL, PHYSICAL, SOCIAL, NUM_SKIMENUS
};
enum {
    ADVANTAGE, ATTRIBUTE, SKILL, NUM_PRIORITIES
};
enum {
    BASIC_ACADEMY, ADV_ACADEMY, BASIC_UNIV, ADV_UNIV, PACKAGE_NONE,
    NUM_PACKAGES
};
enum {
    CAVALRY, BMECH, AERO, ARTILLERY, DROPSHIP, TECHMECH, TECHVEH,
    NUM_CAREERS
};
int das_career_magic[NUM_PACKAGES][3] = {
    {3, 3, 0},
    {2, 2, 2},
    {4, 4, 0},
    {3, 3, 2},
    {0, 0, 0}
};
int das_career_costs[NUM_PACKAGES] = { 9, 15, 12, 18, 0 };
char *das_menus[NUM_PRIORITIES + 1] =
    { "Advantages", "Attributes", "Skills", NULL };
char *das_packages[NUM_PACKAGES + 1] =
    { "Basic Academy", "Advanced Academy",
    "Basic University", "Advanced University", "No package", NULL
};
char *das_careers[NUM_CAREERS + 1] =
    { "Cavalry", "Battlemech", "Aerospace", "Artillery", "DropShip",
    "Mech Technician",
    "Vehicle Technician", NULL
};

#define EA_NUMBER    char_getvaluecode("Exceptional_Attribute")
#define INT_NUMBER   char_getvaluecode("Intuition")
#define LEARN_NUMBER char_getvaluecode("Learn")
#define FIRST_ATT    char_getvaluecode("Build")
#define LAST_ATT     char_getvaluecode("Charisma")

#define SKIBTH(s,a)        a - st->skills[s]
#define SKIBASEBTH(a,b) \
                   18 - st->attributes[a] - st->attributes[b]
#define ATHBASEBTH SKIBASEBTH(FIRST_ATT,FIRST_ATT+1)
#define MENBASEBTH SKIBASEBTH(FIRST_ATT+2,FIRST_ATT+3)
#define PHYBASEBTH SKIBASEBTH(FIRST_ATT+1,FIRST_ATT+2)
#define SOCBASEBTH SKIBASEBTH(FIRST_ATT+2,FIRST_ATT+4)

#define ATHBTH(s)  SKIBTH(s, ATHBASEBTH)
#define MENBTH(s)  SKIBTH(s, MENBASEBTH)
#define PHYBTH(s)  SKIBTH(s, PHYBASEBTH)
#define SOCBTH(s)  SKIBTH(s, SOCBASEBTH)

#define SKILLBTH(s) \
     (char_values[s].flag & CHAR_ATHLETIC) ? ATHBTH(s) : \
     (char_values[s].flag & CHAR_MENTAL)   ? MENBTH(s) : \
     (char_values[s].flag & CHAR_PHYSICAL) ? PHYBTH(s) : SOCBTH(s)

struct chargen_struct {
    dbref player;
    /*[Begin]/Priority/Adv/Att/Pack/PackSet/PackSki/(Skills)/[Done] */
    int state;
    int skill_substate;
    int applied;
    coolmenu *cm[NUM_MENUS];
    coolmenu *sm[NUM_SKIMENUS];

    int pritotal;
    int pribought;

    int advbought;
    int advtotal;

    int attbought;
    int atttotal;
    int eacount;

    int skibought;
    int skitotal;

    int chosen_packagetype;	/* Number, 1-5 or 0 if not yet set */
    int chosen_packages;	/* Basically, a bit vector */

    int packagesbought;
    int packages[NUM_PACKAGES];
    int careersbought;
    int careerstotal;
    int careers[NUM_CAREERS];
    int prioritys[NUM_PRIORITIES];
    int advantages[NUM_CHARVALUES];
    int attributes[NUM_CHARVALUES];
    int pskills[NUM_CHARVALUES];	/* Skills from packages */
    int skills[NUM_CHARVALUES];
    int career_or_package_changed;
    struct chargen_struct *next;
};

struct chargen_struct *chargen_list = NULL;

#define State   (st=retrieve_chargen_struct(player))->state
#define Applied st->applied

struct chargen_struct *retrieve_chargen_struct(dbref player)
{
    struct chargen_struct *foo;

    for (foo = chargen_list; foo; foo = foo->next)
	if (foo->player == player)
	    return foo;
    Create(foo, struct chargen_struct, 1);
    foo->player = player;
    ADD_TO_LIST_HEAD(chargen_list, next, foo);
    return foo;
}

#include "chargen_menus.h"

static coolmenu *find_proper_menu(struct chargen_struct *c)
{
    switch (c->state) {
    case PRI_PICK:
	return c->cm[MENU_PRI];
    case ADV_PICK:
	return c->cm[MENU_ADV];
    case ATT_PICK:
	return c->cm[MENU_ATT];
    case PACK_PICK:
	return c->cm[MENU_PACK];
    case PACKSET_PICK:
	return c->cm[MENU_PACKSET];
    case PACKSKI_PICK:
	return c->cm[MENU_PACKSKI];
    case SKI_PICK:
	return c->sm[c->skill_substate];
    }
    return NULL;
}

int recursive_add(int lev)
{
    int i, t = 0;

    for (i = 1; i <= lev; i++)
	t += i;
    return t;
}

static void update_status(struct chargen_struct *st, coolmenu * c,
    coolmenu * e, int now, int max)
{
    char buf[MBUF_SIZE];
    int bt = 0;

    strcpy(buf, "");

    if (st->state == SKI_PICK) {
	switch (st->skill_substate) {
	case ATHLETIC:
	    bt = ATHBASEBTH;
	    break;
	case MENTAL:
	    bt = MENBASEBTH;
	    break;
	case PHYSICAL:
	    bt = PHYBASEBTH;
	    break;
	case SOCIAL:
	    bt = SOCBASEBTH;
	    break;
	}
	if (bt)
	    sprintf(buf, "BTH base: %d+   ", bt);
    } else if (e && (st->state == PACK_PICK || st->state == PACKSET_PICK))
	st->career_or_package_changed = 1;
    for (; c; c = c->next)
	if (c->id == -1) {
	    if (c->text)
		free((void *) c->text);
	    if (now > max)
		c->text =
		    strdup(tprintf("%s%%cr%d out of %d used - drop some?",
			buf, now, max));
	    else if (now < max)
		c->text =
		    strdup(tprintf("%s%%cc%d out of %d used (%d free)",
			buf, now, max, max - now));
	    else if (now == max)
		c->text =
		    strdup(tprintf
		    ("%s%%cgAll selected. Type 'next' to advance to next stage.",
			buf));
	}
}

static int pack_skills_status(struct chargen_struct *st, coolmenu * c)
{
    char buf[MBUF_SIZE];
    int used[3];
    int i, x, y, can_go = 1;

    for (i = 0; i < 3; i++)
	used[i] = 0;
    for (i = 0; i < NUM_CHARVALUES; i++)
	if (st->pskills[i])
	    used[st->pskills[i] - 1]++;
    strcpy(buf, "");
    for (i = 0; i < 3; i++) {
	if ((x = used[i]) != (y =
		das_career_magic[st->chosen_packagetype][i])) {
	    if (c) {
		if (x < y)
		    sprintf(buf + strlen(buf), "%s%d level %d skills left",
			buf[0] ? ", " : "", (y - x), i + 1);
		else
		    sprintf(buf + strlen(buf),
			"%sremove %d level %d skills", buf[0] ? ", " : "",
			(y - x), i + 1);
	    }
	    can_go = 0;
	}
    }
    if (can_go && c)
	sprintf(buf,
	    "%%cgAll selected. Type 'next' to advance to next stage.");
    for (; c; c = c->next)
	if (c->id == -1) {
	    if (c->text)
		free((void *) c->text);
	    c->text = strdup(buf);
	}
    return can_go;
}

int can_proceed(dbref player, struct chargen_struct *st)
{
    int x = 0, y = 0, i, j;

    switch (State) {
    case PRI_PICK:
	x = st->pribought;
	y = st->pritotal;
#define GENNOT(name) \
      if (x < y) \
	{ \
	  notify(player, tprintf("You haven't used all your %s yet!", name)); \
	  return -1; \
	} \
      else \
	if (x > y) \
	  { \
	    notify(player, tprintf("Whoa! Free some %s first.", name)); \
	    return -1; \
	  }
	GENNOT("priority points");
	return 1;
    case ADV_PICK:
	x = st->advbought;
	y = st->advtotal;
	GENNOT("advantages");
	return 1;
    case ATT_PICK:
	x = st->attbought;
	y = st->atttotal;
	GENNOT("attribute points");
	for (i = FIRST_ATT; i <= LAST_ATT; i++)
	    if (st->attributes[i] < 3) {
		notify(player, "No attributes below 3, please!");
		return -1;
	    }
	if (st->eacount != st->advantages[EA_NUMBER]) {
	    notify(player, "Invalid number of exceptional attributes!");
	    return -1;
	}
	return 1;
    case PACK_PICK:
	/* Pack picking's tricky - you can always pick just one */
	x = st->packagesbought;
	y = 1;
	GENNOT("package(s)");
	for (i = 0; i < NUM_PACKAGES; i++)
	    if (st->packages[i]) {
		if (das_career_costs[i] > st->skitotal) {
		    notify(player,
			"You can't afford it! Get more skill points!");
		    return -1;
		}
		st->chosen_packagetype = i;
	    }
	return 1;
	break;
    case PACKSET_PICK:
	x = st->careersbought;
	y = (st->chosen_packagetype >= BASIC_UNIV) ? 2 : 1;
	GENNOT("career(s)");
	j = 0;
	for (i = 0; i < NUM_CAREERS; i++)
	    if (st->careers[i])
		j += 16 * (1 << i);
	st->chosen_packages = j;
	return 1;
	break;
    case PACKSKI_PICK:
	/* This has a special status (fear) */
	if (!pack_skills_status(st, NULL)) {
	    notify(player,
		"You haven't allocated all your package skills properly yet!");
	    return -1;
	}
	return 1;
    case SKI_PICK:
	x = st->skibought;
	y = st->skitotal;
	if ((st->skill_substate + 1) < NUM_SKIMENUS)
	    return 1;
	GENNOT("skill points");
	if (st->chosen_packagetype != PACKAGE_NONE &&
	    (!st->skills[char_getvaluecode("Small_Arms")] ||
		!st->skills[char_getvaluecode("Medtech")])) {
	    notify(player,
		"All packages require at least one course on both Small_Arms and Medtech!");
	    return -1;
	}
	for (i = 0; i < NUM_CHARVALUES; i++) {
	    if ((x = st->skills[i]) > (y = st->attributes[LEARN_NUMBER])) {
		notify(player,
		    tprintf
		    ("Some of your skill(s) are above your LRN! (%d > %d",
			x, y));
		return -1;
	    } else if (x)
		if ((y = SKILLBTH(i)) < 4) {
		    notify(player,
			tprintf
			("Skills can't have better than 4+ BTH! (%s = %d+)",
			    char_values[i].name, y));
		    return -1;
		}
	    if (st->pskills[i] > st->skills[i]) {
		notify(player,
		    tprintf
		    ("Note: You can't drop skills from what you chose for them from packages. (%s: %d->%d can't be done)",
			char_values[i].name, st->pskills[i],
			st->skills[i]));
		return -1;
	    }
	}
	return 1;
    }
    return 0;
}

static void update_skillmenu_entry(struct chargen_struct *st, int id,
    int val)
{
    coolmenu *d;
    int i;

    for (i = 0; i < NUM_SKIMENUS; i++)
	if (st->sm[i])
	    for (d = st->sm[i]; d; d = d->next)
		if (d->id == id)
		    d->value = val;
}


static void chargen_recalculate_menu(dbref player, coolmenu * menu,
    coolmenu * e)
{
    struct chargen_struct *st;
    int x = 0, y = 0, i;

    switch (State) {
    case PACK_PICK:
	/* Pack picking's tricky - you can always pick just one */
	if (e) {
	    st->packagesbought += e->value - (st->packages[e->id - 1]);
	    st->packages[e->id - 1] = e->value;
	}
	x = st->packagesbought;
	y = 1;
	break;
    case PACKSET_PICK:
	if (e) {
	    st->careersbought += e->value - (st->careers[e->id - 1]);
	    st->careers[e->id - 1] = e->value;
	}
	x = st->careersbought;
	y = (st->chosen_packagetype >= BASIC_UNIV) ? 2 : 1;
	break;
    case PACKSKI_PICK:
	/* This has a special status (fear) */
	if (e) {
	    st->skills[e->id - 1] += e->value - st->pskills[e->id - 1];
	    /* Also, we have to update the damned menu entry */
	    update_skillmenu_entry(st, e->id, st->skills[e->id - 1]);
	    st->pskills[e->id - 1] = e->value;
	}
	pack_skills_status(st, menu);
	return;
    case PRI_PICK:
	if (e) {
	    st->pribought += e->value - (st->prioritys[e->id - 1]);
	    st->prioritys[e->id - 1] = e->value;
	}
	x = st->pribought;
	y = st->pritotal;
	break;
    case ADV_PICK:
	if (e) {
	    st->advbought += e->value - st->advantages[e->id - 1];
	    st->advantages[e->id - 1] = e->value;
	}
	x = st->advbought;
	y = st->advtotal;
	break;
    case ATT_PICK:
	if (e) {
	    if ((e->id - 1) == INT_NUMBER)
		st->attbought +=
		    (2 * e->value) - (2 * st->attributes[e->id - 1]);
	    else
		st->attbought += (e->value - st->attributes[e->id - 1]);

	    if (e->value > 6 && st->attributes[e->id - 1] <= 6)
		st->eacount++;
	    else if (e->value <= 6 && st->attributes[e->id - 1] > 6)
		st->eacount--;
	    st->attributes[e->id - 1] = e->value;
	}
	x = st->attbought;
	y = st->atttotal;
	break;
    case SKI_PICK:
	if (e) {
	    i = MAX(e->value, st->pskills[e->id - 1]);
	    DOCHECK(i != e->value,
		tprintf
		("Error: You can't go below the level allocated from the package skills (%d)",
		    i));
	    st->skibought +=
		recursive_add(i) - recursive_add(st->skills[e->id - 1]);
	    st->skills[e->id - 1] = i;
	}
	x = st->skibought;
	y = st->skitotal;
	break;
    }
    update_status(st, menu, e, x, y);
}


/* DasMagic is the one that figures how to get coolmenu struct */
#define DASMAGIC  \
   coolmenu *c = find_proper_menu(retrieve_chargen_struct(player))

/* DasMagic2 is name of the coolmenu struct in the function */
#define DASMAGIC2 c

/* DasMagic3 is called if value in the menu changes */

/* c = main menu, d = the changed entry */
#define DASMAGIC3 chargen_recalculate_menu(player,c,d)

/* If DasMagic4 is set, menu is re-shown every time value changes */
#undef DASMAGIC4
#include "coolmenu_interface2.h"

COMMANDSET(cm);

int can_advance_state(struct chargen_struct *st)
{
    if (st->state == DONE)
	return 0;
    if (st->state == NOTBEGUN)
	return 0;
    return 1;
}

int can_go_back_state(struct chargen_struct *st)
{
    return ((st->state != DONE || !Applied) && st->state > PRI_PICK);
}

static coolmenu *make_generic_menu(dbref player, struct chargen_struct *st,
    int type, int flag, int maxval)
{
    coolmenu *c, *d;

    c = create_menu_of_charvalues(player, NULL, type, flag, maxval);
    if (st->state == SKI_PICK) {
	for (d = c; d; d = d->next)
	    if (d->id > 0)
		d->value = st->skills[d->id - 1];
    }
    if (st->state == PACKSKI_PICK) {
	for (d = c; d; d = d->next)
	    if (d->id > 0)
		d->value = st->pskills[d->id - 1];
    }
    if (st->state == ADV_PICK) {
	for (d = c; d; d = d->next)
	    if (d->id > 0)
		d->flags |= CM_NO_HILITE;
    }
    if (st->state != NOTBEGUN) {
	CreateMenuEntry_Simple(&c, "Prev = Previous menu",
	    CM_TWO | CM_CENTER);
	CreateMenuEntry_Simple(&c, "Next = Next menu", CM_TWO | CM_CENTER);
    }
    CreateMenuEntry_Normal(&c, "Status", CM_ONE | CM_CENTER, -1, 0);
    CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
    return c;
}

static coolmenu *make_text_menu(dbref player, struct chargen_struct *st,
    char *desc, char **ch, int type, int maxval)
{
    coolmenu *c;

    c = SelCol_Menu(-1, desc, ch, type, maxval);
    CreateMenuEntry_Normal(&c, "Status", CM_ONE, -1, 0);
    CreateMenuEntry_Simple(&c, NULL, CM_ONE | CM_LINE);
    return c;
}

void recalculate_skillpoints(struct chargen_struct *st)
{
    int i = 0;
    int j;

    i += das_career_costs[st->chosen_packagetype];
    for (j = 0; j < NUM_CHARVALUES; j++)
	if (st->skills[j])
	    i += recursive_add(st->skills[j]) -
		recursive_add(st->pskills[j]);
    st->skibought = i;
}

static void advance_state(dbref player, struct chargen_struct *st)
{
    coolmenu *c;
    int i;

    switch (st->state) {
    case NOTBEGUN:
	if (!st->cm[MENU_PRI])
	    st->cm[MENU_PRI] =
		make_text_menu(player, st, "Priority Selection", das_menus,
		CM_NUMBER, 4);
	st->state = PRI_PICK;
	break;
    case PRI_PICK:
	if (!st->cm[MENU_ADV])
	    st->cm[MENU_ADV] =
		make_generic_menu(player, st, CHAR_ADVANTAGE, -1, 8);
	st->advtotal = st->prioritys[ADVANTAGE];
	st->atttotal = 18 + (st->prioritys[ATTRIBUTE]) * 3;
	st->skitotal = 8 + (st->prioritys[SKILL]) * 4;
	st->state = ADV_PICK;
	if (!st->advbought && !st->advtotal) {
	    advance_state(player, st);
	    return;
	}
	break;
    case ADV_PICK:
	if (!st->cm[MENU_ATT])
	    st->cm[MENU_ATT] =
		make_generic_menu(player, st, CHAR_ATTRIBUTE, -1, 7);
	st->state = ATT_PICK;
	if (!st->attbought && !st->atttotal) {
	    advance_state(player, st);
	    return;
	}
	break;
    case ATT_PICK:
	if (!st->cm[MENU_PACK])
	    st->cm[MENU_PACK] =
		make_text_menu(player, st, "Package Selection",
		das_packages, CM_TOGGLE, 1);
	st->state = PACK_PICK;
	break;
    case PACK_PICK:
	if (!st->cm[MENU_PACKSET])
	    st->cm[MENU_PACKSET] =
		make_text_menu(player, st, "Career Selection", das_careers,
		CM_TOGGLE, 1);
	st->state = PACKSET_PICK;
	break;
    case PACKSET_PICK:
	/* 
	   This _is_ painful part.. 
	   1) We have to ensure that pskills is empty,
	   2) Always create new menu (according to careers)
	 */
	if (st->career_or_package_changed) {
	    st->career_or_package_changed = 0;
	    if (st->cm[MENU_PACKSKI])
		free((void *) st->cm[MENU_PACKSKI]);
	    for (i = 0; i < NUM_CHARVALUES; i++)
		if (st->pskills[i]) {
		    st->skills[i] -= st->pskills[i];
		    st->pskills[i] = 0;
		}
	    st->cm[MENU_PACKSKI] = create_packskill_menu(player, st);
	    for (i = 0; i < NUM_SKIMENUS; i++)
		if (st->sm[i]) {
		    free((void *) st->sm[i]);
		    st->sm[i] = NULL;
		}
	}
	st->state = PACKSKI_PICK;
	break;
    case PACKSKI_PICK:
	/* 
	   Once we leave packskills, we _do_ have to recalculate our
	   skillpoint total used (mainly thanks to packages fudging it,
	   and some other small things
	 */
	recalculate_skillpoints(st);
	st->skill_substate = -1;
	st->state = SKI_PICK;
    case SKI_PICK:
	if ((++st->skill_substate) == NUM_SKIMENUS)
	    st->state = DONE;
	else {
	    if (!st->sm[st->skill_substate])
		st->sm[st->skill_substate] =
		    make_generic_menu(player, st, CHAR_SKILL,
		    1 << st->skill_substate, 7);
	}
	break;
    }
    c = find_proper_menu(st);
    if (State == DONE) {
	chargen_look(player, NULL, NULL);
	return;
    }
    if (!c)
	return;
    chargen_recalculate_menu(player, c, NULL);
    ShowCoolMenu(player, c);
}

void go_back_state(dbref player, struct chargen_struct *st)
{
    coolmenu *c;

    switch (st->state) {
    case ADV_PICK:
	st->state = PRI_PICK;
	break;
    case ATT_PICK:
	st->state = ADV_PICK;
	if (!st->advbought && !st->advtotal) {
	    go_back_state(player, st);
	    return;
	}
	break;
    case DONE:
	st->state = SKI_PICK;
	st->skill_substate = NUM_SKIMENUS - 1;
	break;
    case SKI_PICK:
	if ((--st->skill_substate) < 0)
	    st->state = PACKSKI_PICK;
	break;
    case PACK_PICK:
	st->state = ATT_PICK;
	if (!st->attbought && !st->atttotal) {
	    go_back_state(player, st);
	    return;
	}
	break;
    case PACKSET_PICK:
	st->state = PACK_PICK;
	break;
    case PACKSKI_PICK:
	st->state = PACKSET_PICK;
	break;
    }
    c = find_proper_menu(st);
    if (!c)
	return;
    chargen_recalculate_menu(player, c, NULL);
    ShowCoolMenu(player, c);
}


static void apply_values(dbref player, coolmenu * c, int overwrite)
{
    int v, i;

    for (; c; c = c->next)
	if (c->id > 0) {
	    i = c->id - 1;
	    v = c->value;
	    if (overwrite) {
		char_setvaluebycode(player, i, v);
	    } else {
		char_setvaluebycode(player, i, char_getvaluebycode(player,
			i) + v);
	    }
	}
}

#include "chargen_commands.h"
