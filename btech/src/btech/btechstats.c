/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 *
 */

#include "config.h"
#include <stdio.h>
#include <math.h>
#define BTECHSTATS
#include "mech.h"
#include "coolmenu.h"
#include "mycool.h"
#include "mech.events.h"
#define BTECHSTATS_C
#include "btmacros.h"
#include "btechstats.h"
#include "htab.h"
#include "create.h"
#include "muxevent.h"
#include "glue.h"
#include "p.mechfile.h"
#include "p.mech.utils.h"
#include "p.mech.partnames.h"
#include "p.mech.update.h"
#include "p.bsuit.h"
#include "p.map.obj.h"
#include "p.mech.combat.h"
#include "p.mech.combat.misc.h"
#include "p.mech.pickup.h"
#include "p.mech.tag.h"
#include "p.functions.h"

extern dbref pilot_override;

dbref cached_target_char = -1;
int cached_skill;
int cached_result;
extern char *get_uptime_to_string(int);

static int char_xp_bonus(PSTATS * s, int code);
static int char_getstatvalue(PSTATS * s, char *name);
static PSTATS *retrieve_stats(dbref player, int modes);
static void clear_player(PSTATS * s);
static void store_stats(dbref player, PSTATS * s, int modes);

char *silly_get_uptime_to_string(int i)
{
	static char buf[MBUF_SIZE];
	char *c;

	c = get_uptime_to_string(i);
	strcpy(buf, c);
	free_sbuf(c);
	return buf;
}

static int char_getskilltargetbycode_base(dbref player, PSTATS * s,
										  int code, int modifier, int use_xp);

static int char_getskilltargetbycode_noxp(dbref player, int code,
										  int modifier);

static int figure_xp_bonus(dbref player, PSTATS * s, int code)
{
	int t = char_values[code].xpthreshold;
	int tx, bon, btar;

	if(t <= 0)
		return 0;
	/* KLUDGE */
	s->xp[code] = s->xp[code] % XP_MAX;	/* reset exp modifier - this probably _was_ cached */
	btar = char_getskilltargetbycode_base(player, s, code, 0, 0);
	while (btar > 4) {
		btar--;
		t = t / 3;
	}
	while (btar < 4) {
		btar++;
		t = t * 3;
	}
	if(t < 1)
		t = 1;
	tx = s->xp[code] % XP_MAX;
	bon = 0;
	while (tx > t) {
		bon++;
		tx -= t;
		t = t * 3;
	}
	return bon;
}

static int figure_xp_to_next_level(dbref target, int code)
{
	int xpthresh = char_values[code].xpthreshold;
	int start_skill, target_skill, counter, running_total = 1;

	if(xpthresh <= 0)
		return -1;
	target_skill = char_getskilltargetbycode(target, code, 0);
	start_skill = char_getskilltargetbycode_noxp(target, code, 0);
	counter = start_skill;
	while (counter > 4) {
		counter--;
		xpthresh /= 3;
	}
	while (counter < 4) {
		counter++;
		xpthresh *= 3;
	}
	if(xpthresh < 1)
		xpthresh = 1;
	while (target_skill <= start_skill) {
		start_skill--;
		running_total += xpthresh;
		xpthresh *= 3;
	}
	return running_total;
}

/* Right now applies to only very few select skills */

static int char_xp_bonus(PSTATS * s, int code)
{
#if 0
	int count = 1;
	int xp;

	if(t <= 0)
		return 0;
	xp = s->xp[code];
	while (xp > (t * count)) {
		xp -= t * count * (mudconf.btech_ic ? 1 : 4);
		count++;
	}
	return count - 1;
#endif
	return s->xp[code] / XP_MAX;
}

/*****************************/

/*     list commands        */

/*****************************/

void list_charvaluestuff(dbref player, int flag)
{
	int found = 0, ok, type;
	int i;
	char buf[80];

	if(flag == -1)
		notify(player, "List of charvalues available:");
	if(flag >= 0) {
		notify_printf(player, "List of %s available:",
					  btech_charvaluetype_names[flag]);
	}
	buf[0] = 0;
	for(i = 0; i < NUM_CHARVALUES; i++) {
		ok = 0;
		type = char_values[i].type;
		if(flag < 0)
			ok = 1;
		else if(type == flag)
			ok = 1;
		if(ok) {
			sprintf(buf + strlen(buf), "%-23s ", char_values[i].name);
			if(!((++found) % 3)) {
				notify(player, buf);
				strcpy(buf, " ");
			}
		}
	}
	if(found % 3) {
		notify(player, buf);
	}
	notify(player, " ");
	notify_printf(player, "Total of %d things found.", found);
}

/*****************************/

/*     get code commands    */

/*****************************/

HASHTAB playervaluehash, playervaluehash2;

int char_getvaluecode(char *name)
{
	int *ip;
	char *tmpbuf, *tmpc1, *tmpc2;

	tmpbuf = alloc_sbuf("getvaluecodefind");
	for(tmpc1 = name, tmpc2 = tmpbuf; *tmpc1 &&
		((tmpbuf - tmpc2) < (SBUF_SIZE - 1)); tmpc1++, tmpc2++)
		*tmpc2 = ToLower(*tmpc1);
	*tmpc2 = 0;
	if((ip = hashfind(tmpbuf, &playervaluehash)) == NULL)
		ip = hashfind(tmpbuf, &playervaluehash2);
	free_sbuf(tmpbuf);
	return ((int) ip) - 1;
}

/********************/

/*   Roll the dice  */

/********************/

int char_rollsaving(void)
{
	int r1, r2, r3;
	int r12, r13, r23;

	r1 = char_rolld6(1);
	r2 = char_rolld6(1);
	r3 = char_rolld6(1);

	r12 = r1 + r2;
	r13 = r1 + r3;
	r23 = r2 + r3;

	if(r12 > r13) {
		if(r12 > r23)
			return r12;
		else
			return r23;
	} else {
		if(r13 > r23)
			return r13;
		else
			return r23;
	}
}

int char_rollunskilled(void)
{
	int r1, r2, r3;
	int r12, r13, r23;

	r1 = char_rolld6(1);
	r2 = char_rolld6(1);
	r3 = char_rolld6(1);

	r12 = r1 + r2;
	r13 = r1 + r3;
	r23 = r2 + r3;

	if(r12 < r13) {
		if(r12 < r23)
			return r12;
		else
			return r23;
	} else {
		if(r13 < r23)
			return r13;
		else
			return r23;
	}
}

int char_rollskilled(void)
{
	return char_rolld6(2);
}

int char_rolld6(int num)
{
	int i, total = 0;

	for(i = 0; i < num; i++)
		total = total + Number(1, 6);
	return (total);
}

/*****************************/

/*     DB access commands   */

/*****************************/

static int char_getstatvalue(PSTATS * s, char *name)
{
	return char_getstatvaluebycode(s, char_getvaluecode(name));
}

int char_getvalue(dbref player, char *name)
{
	return char_getvaluebycode(player, char_getvaluecode(name));
}

static void char_setstatvalue(PSTATS * s, char *name, int value)
{
	char_setstatvaluebycode(s, char_getvaluecode(name), value);
}

void char_setvalue(dbref player, char *name, int value)
{
	char_setvaluebycode(player, char_getvaluecode(name), value);
}

static int char_getskilltargetbycode_base(dbref player, PSTATS * s,
										  int code, int modifier, int use_xp)
{
	int val, skill;

	if(code == -1)
		return 18;
	if(char_values[code].type != CHAR_SKILL)
		return 18;
	if(use_xp && cached_target_char == player && cached_skill == code)
		return cached_result + modifier;
	if(char_values[code].flag & CHAR_ATHLETIC)
		val = char_gvalue(s, "build") + char_gvalue(s, "reflexes");
	else if(char_values[code].flag & CHAR_PHYSICAL)
		val = char_gvalue(s, "reflexes") + char_gvalue(s, "intuition");
	else if(char_values[code].flag & CHAR_MENTAL)
		val = char_gvalue(s, "intuition") + char_gvalue(s, "learn");
	else if(char_values[code].flag & CHAR_PHYSICAL)
		val = char_gvalue(s, "reflexes") + char_gvalue(s, "intuition");
	else if(char_values[code].flag & CHAR_SOCIAL)
		val = char_gvalue(s, "intuition") + char_gvalue(s, "charisma");
	else
		return 18;
	if(use_xp) {
		skill = char_getstatvaluebycode(s, code);

		if(skill == -1)
			return 18;
		cached_target_char = player;
		cached_skill = code;
		cached_result = 18 - val - skill;
		return cached_result + modifier;
	} else {
		skill = s->values[code];
		if(skill == -1)
			return (18);
		return 18 - val - skill;
	}
}

int char_getskilltargetbycode(dbref player, int code, int modifier)
{
	PSTATS *s;

	s = retrieve_stats(player, VALUES_CO);
	return char_getskilltargetbycode_base(player, s, code, modifier, 1);
}

static int char_getskilltargetbycode_noxp(dbref player, int code,
										  int modifier)
{
	PSTATS *s;

	s = retrieve_stats(player, VALUES_CO);
	return char_getskilltargetbycode_base(player, s, code, modifier, 0);
}

int char_getskilltarget(dbref player, char *name, int modifier)
{
	return char_getskilltargetbycode(player, char_getvaluecode(name),
									 modifier);
}

int char_getxpbycode(dbref player, int code)
{
	PSTATS *s;

	if(code < 0)
		return 0;
	s = retrieve_stats(player, VALUES_SKILLS);
	return s->xp[code] % XP_MAX;
}

int char_gainxpbycode(dbref player, int code, int amount)
{
	PSTATS *s;

	if(code < 0)
		return 0;
	s = retrieve_stats(player, VALUES_SKILLS | VALUES_ATTRS);
	if(!((mudstate.now > (s->last_use[code] + 30)) ||
		 (char_values[code].flag & SK_XP)))
		return 0;
	s->last_use[code] = mudstate.now;
	s->xp[code] += amount;
	s->xp[code] =
		s->xp[code] % XP_MAX + XP_MAX * figure_xp_bonus(player, s, code);
	store_stats(player, s, VALUES_SKILLS);
	return 1;
}

int char_gainxp(dbref player, char *skill, int amount)
{
	return char_gainxpbycode(player, char_getvaluecode(skill), amount);
}

int char_getskillsuccess(dbref player, char *name, int modifier, int loud)
{
	int roll, val;
	int code;

	code = char_getvaluecode(name);

	val = char_getskilltargetbycode(player, code, modifier);

	if(char_getvaluebycode(player, code) == 0)
		roll = char_rollunskilled();
	else
		roll = char_rollskilled();
	if(loud) {
		notify_printf(player, "You make a %s skill roll!", name);
		notify_printf(player, "Modified skill BTH : %d Roll : %d", val, roll);
	}

	if(roll >= val)
		return (1);				/* Success! */
	else
		return (0);				/* Failure */
}

int char_getskillmargsucc(dbref player, char *name, int modifier)
{
	int roll, val;
	int code;

	code = char_getvaluecode(name);

	val = char_getskilltargetbycode(player, code, modifier);

	if(char_getvaluebycode(player, code) == 0)
		roll = char_rollunskilled();
	else
		roll = char_rollskilled();

	return (roll - val);
}

int char_getopposedskill(dbref first, char *skill1, dbref second,
						 char *skill2)
{
	int per1, per2;

	per1 = char_getskillmargsucc(first, skill1, 0);
	per2 = char_getskillmargsucc(second, skill2, 0);

	if(per1 > per2)
		return (first);
	else if(per2 == per1)
		return (0);
	else
		return (second);
}

int char_getattrsave(dbref player, char *name)
{
	int val = char_getvalue(player, name);

	if(val == -1)
		return (-1);
	else if(val > 9)
		return 0;
	else
		return (18 - 2 * val);
}

int char_getattrsavesucc(dbref player, char *name)
{
	int roll, val = char_getattrsave(player, name);

	if(val == -1)
		return (-1);

	roll = char_rollskilled();

	if(roll >= val)
		return (1);
	else
		return (0);
}

/************************/

/*    Database Commands */

/************************/

void init_btechstats(void)
{
	char *tmpbuf, *tmpc1, *tmpc2;
	int i, j;

	hashinit(&playervaluehash, 20 * HASH_FACTOR);
	hashinit(&playervaluehash2, 20 * HASH_FACTOR);
	tmpbuf = alloc_sbuf("getvaluecode");
	for(i = 0; i < NUM_CHARVALUES; i++) {
		for(tmpc1 = char_values[i].name, tmpc2 = tmpbuf; *tmpc1;
			tmpc1++, tmpc2++)
			*tmpc2 = ToLower(*tmpc1);
		*tmpc2 = '\0';
		hashadd(tmpbuf, (int *) (i + 1), &playervaluehash);
		tmpbuf[0] = '\0';
		tmpc1 = tmpbuf;
		for(j = 0; char_values[i].name[j]; j++) {
			if(!isupper(char_values[i].name[j]))
				continue;
			strncpy(tmpc1, &char_values[i].name[j], 3);
			tmpc1 += 3;
		}
		*tmpc1 = '\0';
		if(strlen(tmpbuf) <= 3) {
			strncpy(tmpbuf, char_values[i].name, 5);
			tmpbuf[5] = '\0';
		}
		char_values_short[i] = strdup(tmpbuf);
		for(tmpc1 = tmpbuf; *tmpc1; tmpc1++)
			*tmpc1 = ToLower(*tmpc1);
		hashadd(tmpbuf, (int *) (i + 1), &playervaluehash2);
	}
	free_sbuf(tmpbuf);
}

static PSTATS *create_new_stats(void)
{
	PSTATS *s;

	Create(s, PSTATS, 1);
	s->dbref = -1;
	clear_player(s);
	return s;
}

static void clear_player(PSTATS * s)
{
	int i;

	for(i = 0; i < NUM_CHARVALUES; i++) {
		s->values[i] = (char_values[i].type == CHAR_ATTRIBUTE ? 1 : 0);
		s->xp[i] = 0;
	}
	char_slives(s, 1);
}

static void show_charstatus(dbref player, PSTATS * s, dbref thing)
{
	char *p;
	int i, j;
	int notified;
	coolmenu *c = NULL;

	if(thing) {
		addmenu(tprintf("%%cgName     %%c: %s (#%d)", Name(thing), thing));
		if(*(p = silly_atr_get(thing, A_FACTION)))
			addmenu(tprintf("%%cgFaction  %%c: %s", p));
#if 0
		if((p = get_rankname(thing)))
			addmenu(tprintf("%%cgRank     %%c: %s", p));
		if(*(p = silly_atr_get(thing, A_JOB)))
			addmenu(tprintf("%%cgJob      %%c: %s", p));
#endif
		addline();
	}
	if(thing) {
		addmenu(tprintf("%%cgBruise   %%c: %d of %d", char_gbruise(s),
						char_gmaxbruise(s)));
		addmenu(tprintf("%%cgLethal   %%c: %d of %d", char_glethal(s),
						char_gmaxlethal(s)));
	}
	addmenu(tprintf("%%cgLives    %%c: %d", char_glives(s)));
	addempty();
	addmenu("%cgAttributes");
	addmenu("Characteristics");
	addline();

	addmenu(tprintf("  %-8s%1d (%d+)", "BLD", char_gvalue(s, "build"),
					18 - (char_gvalue(s, "build") * 2)));
	addmenu(tprintf("  %-15s%2d+", "Athletic", 18 - (char_gvalue(s,
																 "build") +
													 char_gvalue(s,
																 "reflexes"))));

	addmenu(tprintf("  %-8s%1d (%d+)", "REF", char_gvalue(s, "reflexes"),
					18 - (char_gvalue(s, "reflexes") * 2)));
	addmenu(tprintf("  %-15s%2d+", "Physical", 18 - (char_gvalue(s,
																 "reflexes") +
													 char_gvalue(s,
																 "intuition"))));

	addmenu(tprintf("  %-8s%1d (%d+)                              ", "INT",
					char_gvalue(s, "intuition"), 18 - (char_gvalue(s,
																   "intuition")
													   * 2)));
	addmenu(tprintf
			("  %-15s%2d+", "Mental",
			 18 - (char_gvalue(s, "learn") + char_gvalue(s, "intuition"))));

	addmenu(tprintf("  %-8s%1d (%d+)                              ", "LRN",
					char_gvalue(s, "learn"),
					18 - (char_gvalue(s, "learn") * 2)));
	addmenu(tprintf
			("  %-15s%2d+", "Social",
			 18 - (char_gvalue(s, "charisma") +
				   char_gvalue(s, "intuition"))));

	addmenu(tprintf("  %-8s%1d (%d+)", "CHA", char_gvalue(s, "charisma"),
					18 - (char_gvalue(s, "charisma") * 2)));
	addempty();
	notified = 0;
	for(i = 0; i < NUM_CHARVALUES; i++) {
		if(char_values[i].type != CHAR_ADVANTAGE)
			continue;
		if(!(j = s->values[i]))
			continue;
		notified = 1;
	}
	if(notified) {
		addmenu("%cgAdvantages");
		addline();
		for(i = 0; i < NUM_CHARVALUES; i++) {
			if(char_values[i].type != CHAR_ADVANTAGE)
				continue;
			if(!(j = s->values[i]))
				continue;
			switch (char_values[i].flag) {
			case CHAR_ADV_BOOL:
				addmenu(tprintf("  %s", char_values[i].name));
				break;
			case CHAR_ADV_VALUE:
				addmenu(tprintf("  %s: %d", char_values[i].name, j));
				break;
			case CHAR_ADV_EXCEPT:
				if(j & CHAR_BLD)
					addmenu("  Exceptional Attribute: Build");
				if(j & CHAR_REF)
					addmenu("  Exceptional Attribute: Reflexes");
				if(j & CHAR_INT)
					addmenu("  Exceptional Attribute: Intuition");
				if(j & CHAR_LRN)
					addmenu("  Exceptional Attribute: Learn");
				if(j & CHAR_CHA)
					addmenu("  Exceptional Attribute: Charisma");
			}
			addempty();
		}
	}
	addmenu("%cgSkills");
	addline();

	notified = 0;
	for(i = 0; i < NUM_CHARVALUES; i++) {
		if(!s->values[i])
			continue;
		if(char_values[i].type != CHAR_SKILL)
			continue;
		addmenu(tprintf("  %-25.25s : %d (%d+)", char_values[i].name,
						s->values[i], char_getskilltargetbycode(thing, i,
																0)));
		notified = 1;
	}
	if(!notified)
		addmenu("  None");

/*   addempty(); */
	ShowCoolMenu(player, c);
	KillCoolMenu(c);
}

/************************/

/*      MUSE COMMANDS   */

/************************/

void do_charstatus(dbref player, dbref cause, int key, char *arg1)
{
	dbref thing;
	int dir = 0;

	PSTATS *s;

	if(WizR(player))
		dir++;

	if(arg1 && *arg1) {
		thing = char_lookupplayer(player, player, 0, arg1);

		DOCHECK(thing == NOTHING, "I don't know who that is");

		if(thing != player && !(WizR(player))) {
			notify(player,
				   "You do not have the authority to check that players stats");
			return;
		}
	} else
		thing = player;

	s = retrieve_stats(thing, VALUES_ALL);
	show_charstatus(player, s, thing);
}

void do_charclear(dbref player, dbref cause, int key, char *arg1)
{
	dbref thing;

	DOCHECK(!WizR(player),
			"Sorry, only those with the real power may clear players stats");
	DOCHECK(!arg1 || !*arg1, "Who do you want to clear the stats from?");
	thing = char_lookupplayer(player, player, 0, arg1);
	DOCHECK(thing == NOTHING, "I don't know who that is");
	silly_atr_set(thing, A_ATTRS, "");
	silly_atr_set(thing, A_SKILLS, "");
	silly_atr_set(thing, A_ADVS, "");
	silly_atr_set(thing, A_HEALTH, "");
	notify_printf(player, "Player #%d stats cleared", thing);
}

#if 0
/* Why, what the fuck? */
dbref char_lookupplayer(dbref player, dbref cause, int key, char *arg1)
{
	dbref which;

	if(!arg1 || !*arg1)
		return NOTHING;

	if(!string_compare(arg1, "me") && (Typeof(player) == TYPE_PLAYER))
		return player;
	if(arg1[0] == '#') {
		if(sscanf(arg1, "#%d", &which) == 1)
			if(which >= 0 && isPlayer(which))
				return which;
		return NOTHING;
	}
	return lookup_player(NOTHING, arg1, 0);
}
#endif

dbref char_lookupplayer(dbref player, dbref cause, int key, char *arg1)
{
	return lookup_player(player, arg1, 0);
}

static int loc_mod(int loc)
{
	switch (loc) {
	case HEAD:
		return 15;
	case CTORSO:
		return 50;
	case LTORSO:
	case RTORSO:
		return 35;
	case LARM:
	case RARM:
		return 30;
	case LLEG:
	case RLEG:
		return 35;
	}
	return 0;
}

void initialize_pc(dbref player, MECH * mech)
{
	PSTATS *s;
	int bruise, lethal, playerBLD;
	int dam, tot;
	char *c;
	int cnt;
	char buf1[MBUF_SIZE];
	char buf2[MBUF_SIZE];
	char buf3[MBUF_SIZE];
	char buf4[2];
	int ammo1;
	int ammo2;
	int i, id, brand;
	int pc_loc_to_mech_loc[] = { HEAD, CTORSO, RARM, RLEG };

	if(!(MechType(mech) == CLASS_MW &&
		 !(MechCritStatus(mech) & PC_INITIALIZED)))
		return;
	buf4[1] = 0;
	s = retrieve_stats(player, VALUES_HEALTH | VALUES_ATTRS | VALUES_SKILLS);
	playerBLD = char_gvalue(s, "build");
	MechCritStatus(mech) |= PC_INITIALIZED;
	bruise = char_gbruise(s);
	lethal = char_glethal(s);
	tot = playerBLD * 20;
	dam = bruise + lethal;
	MechMaxSpeed(mech) =
		(playerBLD + char_gvalue(s, "reflexes") + char_gvalue(s,
															  "running")) *
		MP1 / 9.0;
#define PC_LOCS 4
	for(i = 0; i < NUM_SECTIONS; i++) {
		SetSectArmor(mech, i, 0);
		SetSectOArmor(mech, i, 0);
		SetSectInt(mech, i, (loc_mod(i) * (tot - dam)) / 100 + 1);
		SetSectOInt(mech, i, (loc_mod(i) * (tot - dam)) / 100 + 1);
	}
	c = silly_atr_get(player, A_PCEQUIP);
	cnt = sscanf(c, "%s %s %s %d %d", buf1, buf2, buf3, &ammo1, &ammo2);

	switch (cnt) {
	case 5:
	case 4:
	case 3:
		if(strcmp(buf3, "-")) {
			if(!find_matching_vlong_part(buf3, NULL, &id, &brand)) {
				SendError(tprintf("Invalid PC weapon #1 for %s(#%d): %s",
								  Name(player), player, buf3));
				return;
			}
			if(IsWeapon(id)) {
				SetPartType(mech, LARM, 0, id);
				SetPartData(mech, LARM, 0, 0);
				SetPartFireMode(mech, LARM, 0, 0);
				SetPartAmmoMode(mech, LARM, 0, 0);
				if((i = MechWeapons[Weapon2I(id)].ammoperton)) {
					SetPartType(mech, LARM, 1, I2Ammo(Weapon2I(id)));
					SetPartData(mech, LARM, 1, cnt >= 5 ? ammo2 : i);
					SetPartFireMode(mech, LARM, 1, 0);
					SetPartAmmoMode(mech, LARM, 1, 0);
				}
			}
		}
	case 2:
		if(strcmp(buf2, "-")) {
			if(!find_matching_vlong_part(buf2, NULL, &id, &brand)) {
				SendError(tprintf("Invalid PC weapon #1 for %s(#%d): %s",
								  Name(player), player, buf2));
				return;
			}
			if(IsWeapon(id)) {
				SetPartType(mech, RARM, 0, id);
				SetPartData(mech, RARM, 0, 0);
				SetPartFireMode(mech, RARM, 0, 0);
				SetPartAmmoMode(mech, RARM, 0, 0);
				if((i = MechWeapons[Weapon2I(id)].ammoperton)) {
					SetPartType(mech, RARM, 1, I2Ammo(Weapon2I(id)));
					SetPartData(mech, RARM, 1, cnt >= 4 ? ammo1 : i);
					SetPartFireMode(mech, RARM, 1, 0);
					SetPartAmmoMode(mech, RARM, 1, 0);
				}
			}
		}
	case 1:
		if(strlen(buf1) != PC_LOCS) {
			SendError(tprintf("Invalid armor string for %s(#%d): %s",
							  Name(player), player, buf1));
			return;
		}
		for(i = 0; buf1[i]; i++)
			if(!isdigit(buf1[i])) {
				SendError(tprintf
						  ("Invalid armor char for %s(#%d) in %s (pos %d,%c)",
						   Name(player), player, buf1, i + 1, buf1[i]));
				return;
			}
		for(i = 0; buf1[i]; i++) {
			buf4[0] = buf1[i];
			SetSectArmor(mech, pc_loc_to_mech_loc[i], atoi(buf4));
		}
	}
}

void fix_pilotdamage(MECH * mech, dbref player)
{
	PSTATS *s;
	int bruise, lethal, playerBLD;

	s = retrieve_stats(player, VALUES_HEALTH | VALUES_ATTRS);
	bruise = char_gbruise(s);
	lethal = char_glethal(s);
	playerBLD = char_gvalue(s, "build") * 2;
	if(playerBLD < 1 || playerBLD > 100)
		playerBLD = 10;

	MechPilotStatus(mech) = (bruise + lethal) / playerBLD;
}

int PilotStatusRollNeeded[] = { 0, 3, 5, 7, 10, 11 };

#define CHDAM(val,ret) if (playerhits >= ((val))) return ret * mod;

int mw_ic_bth(MECH * mech)
{
/* Rule Reference: BMR Revised, Page 17 ( Consciousness Table ) */
/* Rule Reference: Total Warfare, Page 41-42 ( Consciousness Table ) */
/* Rule Reference: MaxTech Revised, Page 46 ( Pain Resistance = -1 ) */

	int playerBLD;
	int bruise, playerhits;
	PSTATS *s;
	int mod = 0;

	s = retrieve_stats(MechPilot(mech),
					   VALUES_ATTRS | VALUES_ADVS | VALUES_HEALTH);
	playerBLD = char_gvalue(s, "build");
	bruise = char_gbruise(s);
	playerhits = 10 * playerBLD - bruise;
	if(char_gvalue(s, "pain_resistance") == 1)
		mod = -1;
	if(playerhits >= (8 * playerBLD))
		return 3 + mod;
	else if(playerhits >= (6 * playerBLD))
		return 5 + mod;
	else if(playerhits >= (4 * playerBLD))
		return 7 + mod;
	else if(playerhits >= (2 * playerBLD))
		return 10 + mod;
	else if(playerhits >= -1)
		return 11 + mod;
	return 0;
}

int handlemwconc(MECH * mech, int initial)
{
/* Rule Reference: MechWarrior 2nd Edition RPG, Page 22 (Toughness = Best of 3D6) */
/* Rule Reference: Old Tactical Handbook, Page 51 (Use MW 2nd Edition) */
/* Rule Reference: BMR Revised, Page 17 ( >5 Bruise = Death ) */
/* Rule Reference: Total Warfare, Page 41-42 ( >5 Bruise = Death ) */

	int m, roll;

	if(In_Character(mech->mynum) && MechPilot(mech) > 0)
		m = mw_ic_bth(mech);
	else {
		if(initial)
			if(MechPilotStatus(mech) > 5) {
				mech_notify(mech, MECHPILOT,
							"You are killed from personal injuries!!");

				// This is here to avoid multi-triggers of AMECHDEST.
				if(!Destroyed(mech))
					ChannelEmitKill(mech, mech, KILL_TYPE_PILOT);

				MechPilot(mech) = -1;
				Destroy(mech);
				MechSpeed(mech) = 0.;
				MechDesiredSpeed(mech) = 0.;
				return 0;
			}
		m = PilotStatusRollNeeded[BOUNDED(0, (int) MechPilotStatus(mech), 4)];
	}
	if(initial && Uncon(mech))
		return 0;
	if(HasBoolAdvantage(MechPilot(mech), "toughness"))
		/*  Gets the saving roll for someone with toughness  */
		roll = char_rollsaving();
	else
		roll = char_rollskilled();
	if(MechPilot(mech) >= 0) {
		if(initial) {
			mech_notify(mech, MECHPILOT,
						"You attempt to keep consciousness!");
			mech_printf(mech, MECHPILOT,
						"Retain Conciousness on: %d  \tRoll: %d", abs(m),
						roll);
		} else {
			mech_notify(mech, MECHPILOT,
						"You attempt to regain consciousness!");
			mech_printf(mech, MECHPILOT,
						"Regain Consciousness on: %d  \tRoll: %d", abs(m),
						roll);
		}
	}
	if(roll < (abs(m))) {
		if(initial)
			mech_notify(mech, MECHPILOT,
						"Consciousness slips away from you as you enter a sea of darkness...");
		ProlongUncon(mech, UNCONSCIOUS_TIME);
		return 0;
	}
	return 1;
}

void headhitmwdamage(MECH * mech, MECH * attacker, int dam)
{
	PSTATS *s;
	dbref player;
	int damage, bruise, lethaldam, playerBLD;

	if(mech->mynum < 0)
		return;
	/* check to see if mech is IC */
	if(!In_Character(mech->mynum) || !GotPilot(mech)) {
		MechPilotStatus(mech) += dam;
		handlemwconc(mech, 1);
		return;
	}
	player = MechPilot(mech);

	s = retrieve_stats(player, VALUES_ATTRS | VALUES_ADVS | VALUES_HEALTH);
	/* get the player_stats structure */

	bruise = char_gbruise(s);
	/* gets the players bruise damage */

	playerBLD = char_gvalue(s, "build");
	/* get the player's BLD value */

	damage = 2 * playerBLD * dam;
	/* the damage we are due */

	bruise += damage;
	/* this part subtracts 10 from players lethal damage */

	if(bruise > playerBLD * 10) {
		lethaldam = char_glethal(s);
		lethaldam += (bruise - playerBLD * 10);
		bruise = playerBLD * 10;

		if(lethaldam >= playerBLD * 10) {
			lethaldam = playerBLD * 10;
			char_slethal(s, playerBLD * 10 - 1);
			char_sbruise(s, playerBLD * 10);
			store_stats(player, s, VALUES_HEALTH);
			if(!Destroyed(mech)) {
				DestroyMech(mech, attacker, 1, KILL_TYPE_PILOT);
			}
			KillMechContentsIfIC(mech->mynum);
			return;
		}
		char_slethal(s, lethaldam);
	}
	char_sbruise(s, bruise);
	store_stats(player, s, VALUES_HEALTH);
	handlemwconc(mech, 1);
	MechPilotStatus(mech) += dam;
}

void mwlethaldam(MECH * mech, MECH * attacker, int dam)
{
	PSTATS *s;
	dbref player;
	int bruise, lethaldam, playerBLD;

	if(mech->mynum < 0)
		return;
	/* check to see if mech is IC */
	if(!In_Character(mech->mynum) || !GotPilot(mech)) {
		MechPilotStatus(mech) += dam;
		handlemwconc(mech, 1);
		return;
	}
	player = MechPilot(mech);

	s = retrieve_stats(player, VALUES_ATTRS | VALUES_ADVS | VALUES_HEALTH);
	/* get the player_stats structure */
	bruise = char_gbruise(s);
	playerBLD = char_gvalue(s, "build");
	if(!playerBLD)
		playerBLD++;
	lethaldam = char_glethal(s);
	lethaldam += BOUNDED(10, dam * playerBLD, 40);
	if(lethaldam >= playerBLD * 10) {
		lethaldam = playerBLD * 10;
		char_slethal(s, lethaldam - 1);
		char_sbruise(s, lethaldam);
		store_stats(player, s, VALUES_HEALTH);
		if(!Destroyed(mech)) {
			DestroyMech(mech, attacker, 1, KILL_TYPE_PILOT);
		}
		KillMechContentsIfIC(mech->mynum);
		return;
	}
	char_sbruise(s, playerBLD * 10 - 5);
	char_slethal(s, lethaldam);
	store_stats(player, s, VALUES_HEALTH);
	handlemwconc(mech, 1);
	MechPilotStatus(mech) += dam;
}

void lower_xp(dbref player, int promillage)
{
	PSTATS *s;
	int i;

	s = retrieve_stats(player, VALUES_ALL);
	for(i = 0; i < NUM_CHARVALUES; i++) {
		if(!s->xp[i])
			continue;
		if(s->xp[i] < 0) {
			s->xp[i] = 0;
			continue;
		}
		s->xp[i] = (s->xp[i] % XP_MAX) * promillage / 1000;
		s->xp[i] = s->xp[i] % XP_MAX + XP_MAX * figure_xp_bonus(player, s, i);
	}
	store_stats(player, s, VALUES_ALL);
}

void AccumulateTechXP(dbref pilot, MECH * mech, int reason)
{
	char *skname;
	int xp;
	static char *techw = "technician-weapons";

	if(mech) {
		if(!(skname = FindTechSkillName(mech)))
			return;
	} else
		skname = techw;

	xp = MAX(1, reason);

	// We emit all tech XP gains to the MechTechXP channel.
	if(char_gainxp(pilot, skname, xp))
		SendTechXP(tprintf("%s gained %d %s XP (changing mech #%d)",
						   Name(pilot), xp, skname, mech ? mech->mynum : -1));
}

void AccumulateTechWeaponsXP(dbref pilot, MECH * mech, int reason)
{
	char *skname;
	int xp;
	static char *techw = "technician-weapons";

	skname = techw;
	xp = MAX(1, reason);

	// We emit all tech xp gains to MechTechXP channel.
	if(char_gainxp(pilot, skname, xp))
		SendTechXP(tprintf("%s gained %d %s XP (changing mech #%d)",
						   Name(pilot), xp, skname, mech ? mech->mynum : -1));
}

void AccumulateCommXP(dbref pilot, MECH * mech)
{
	int xp;

	xp = 1;
	if(!RGotPilot(mech))
		return;
	if(!In_Character(mech->mynum))
		return;
	if(!Connected(pilot))
		return;
	if(char_gainxp(pilot, "Comm-Conventional", xp))
		SendXP(tprintf("%s gained %d %s XP (in #%d)", Name(pilot), xp,
					   "Comm-Conventional", mech->mynum));
}

void AccumulatePilXP(dbref pilot, MECH * mech, int reason, int addanyway)
{
	char *skname;
	int xp;

	if(!In_Character(mech->mynum))
		return;

	if(!RGotPilot(mech))
		return;

	if(!(skname = FindPilotingSkillName(mech)))
		return;

	if(!addanyway) {
		if(MechLX(mech) != MechX(mech) || MechLY(mech) != MechY(mech)) {
			MechLX(mech) = MechX(mech);
			MechLY(mech) = MechY(mech);
		} else
			return;
	}
	xp = MAX(1, reason);

	/* Switching to Exile method of tracking xp, where we split
	 * Attacking and Piloting xp into two different channels
	 */
	if(char_gainxp(pilot, skname, xp))
		SendPilotXP(tprintf("%s gained %d %s XP", Name(pilot), xp, skname));
/*
    if (char_gainxp(pilot, skname, xp))
	    SendXP(tprintf("%s gained %d %s XP", Name(pilot), xp, skname));
*/
}

void AccumulateSpotXP(dbref pilot, MECH * attacker, MECH * wounded)
{
	int xp = 1;

	if(!In_Character(attacker->mynum))
		return;
	if(!RGotPilot(attacker))
		return;
	if(MechPilot(attacker) != pilot)
		return;
	if(attacker == wounded)
		return;
	if(Destroyed(wounded))
		return;
	if(MechTeam(wounded) == MechTeam(attacker))
		return;
	if(!In_Character(wounded->mynum))
		return;
	if(char_gainxp(pilot, "Gunnery-Spotting", xp))
		SendXP(tprintf("%s gained spotting XP", Name(pilot)));
}

int MadePerceptionRoll(MECH * mech, int modifier)
{
	int pilot;

	if(!In_Character(mech->mynum))
		return 0;
	if(!RGotGPilot(mech))
		return 0;
	pilot = MechPilot(mech);
	if(pilot <= 0)
		return 0;
	if(!MechPer(mech))
		MechPer(mech) = char_getskilltarget(pilot, "Perception", 2);
	if(Roll() < (MechPer(mech) + modifier))
		return 0;
	char_gainxp(pilot, "Perception", 1);
	if(char_gainxp(pilot, "Perception", 1))
		SendXP(tprintf("%s gained 1 perception XP", Name(pilot)));
	return 1;
}

void AccumulateArtyXP(dbref pilot, MECH * attacker, MECH * wounded)
{
	int xp = 1;

	/* If not in character ie: like in simulator - no xp */
	if(!In_Character(attacker->mynum))
		return;

	if(!RGotGPilot(attacker))
		return;

	if(GunPilot(attacker) != pilot)
		return;

	/* No xp for shooting yourself */
	if(attacker == wounded)
		return;

	/* No xp for shooting destroyed units */
	if(Destroyed(wounded))
		return;

	/* No xp if both on same team */
	if(MechTeam(wounded) == MechTeam(attacker))
		return;

	/* If target not in character ie: in simulator - no xp */
	if(!In_Character(wounded->mynum))
		return;

	/* Switching to Exile method of tracking xp, where we split
	 * Attacking and Piloting xp into two different channels
	 */
	if(char_gainxp(pilot, "Gunnery-Artillery", xp))
		SendAttackXP(tprintf("%s gained %d artillery XP", Name(pilot), xp));
}

void AccumulateComputerXP(dbref pilot, MECH * mech, int reason)
{
	int xp;

	if(!mech)
		return;

	if(mech && In_Character(mech->mynum) && isPlayer(pilot))
		if(char_gainxp(pilot, "computer", MAX(1, reason)))
			SendXP(tprintf
				   ("%s gained %d computer XP (mech #%d)", Name(pilot),
					reason, mech ? mech->mynum : -1));
}

int HasBoolAdvantage(dbref player, const char *name)
{
	PSTATS *s;
	char buf[SBUF_SIZE];

	strcpy(buf, name);
	s = retrieve_stats(player, VALUES_ATTRS | VALUES_ADVS | VALUES_HEALTH);
	if(char_gvalue(s, buf) == 1)
		return 1;
	else
		return 0;
}

int bth_modifier[] =			/* Starts from '3' , in 1/36's */
{
	/*  3 4 5  6  7  8  9 10 11 12 */
	1, 3, 6, 10, 15, 21, 26, 30, 33, 35, 0, 0, 0, 0	/* pad, just in case */
};

#define TonValue(mech) \
MAX(1, (MechTons(mech) / \
 ((MechType(mech) != CLASS_MECH) ? 2 : 1) / \
 ((MechMove(mech) == MOVE_NONE) ? 2 : 1)))

static int t_mod(float sp)
{
	if(sp <= MP2)
		return 0;
	if(sp <= MP4)
		return 1;
	if(sp <= MP6)
		return 2;
	if(sp <= MP9)
		return 3;
	return 4;					/* No extra mods */
}

#define MoveValue(mech) (t_mod(MMaxSpeed(mech)) + 2)
#define NewMoveValue(mech) ((int) (MechMaxSpeed(mech)/MP1))

float getPilotBVMod(MECH * mech, int weapindx)
{
	/*
	 * What we do is we get the mod as if we had a 0+ piloting (baseline)
	 * for the gun skill we want. Each '+' above zero subtracts .05 from
	 * the result. Obviously, each '+' below adds .05.
	 *
	 * The first number in the array below corresponds to a 0+ 0+ person
	 * and the last number in the array below corresponds to a 7+ 0+ person
	 * (that's <gun skill>+ <pilot skill>+)
	 */

	int zeroPilotBaseSkills[] =
		{ 2.05, 1.85, 1.65, 1.45, 1.25, 1.15, 1.05, .95 };

	int myGSkill = FindPilotGunnery(mech, weapindx);
	int myPSkill = FindPilotPiloting(mech);
	float baseMod = 0.0;

	/* First we check if we have a totally off the wall GSkill, i.e., below
	 * 0 or above 7.
	 */
	if(myGSkill < 0) {
		baseMod = zeroPilotBaseSkills[0] + (abs(myGSkill) * 0.20);
	} else if(myGSkill > 7) {
		baseMod = zeroPilotBaseSkills[7] - (myGSkill * 0.10);
	} else {
		baseMod = zeroPilotBaseSkills[myGSkill];
	}

	return (baseMod - ((0 + myPSkill) * 0.05));
}

/*
 * Routines and formula for XP gain.
 */
void AccumulateGunXP(dbref pilot, MECH * attacker, MECH * wounded,
					 int damage, float multiplier, int weapindx, int bth)
{
	int omul, xp, my_BV, th_BV, my_speed, th_speed;
	float myPilotBVMod = 1.0, theirPilotBVMod = 1.0;
	float weapTypeMod;
	char *skname;
	char buf[MBUF_SIZE];
	int damagemod;
	float vrtmod;
	int i;
	int j = NUM_SECTIONS;

	weapTypeMod = 1;

	if(mudconf.btech_oldxpsystem) {
		AccumulateGunXPold(pilot, attacker, wounded, damage,
						   multiplier, weapindx, bth);
		return;
	}

	/* No XP for zero'd mechas */
	for(i = 0; i < NUM_SECTIONS; i++)
		j -= SectIsDestroyed(wounded, i);
	
	if( j < 1)
		return;

	/* Is attacker in character ie: not in simulator */
	if(!In_Character(attacker->mynum))
		return;

	if(!RGotGPilot(attacker))
		return;

	if(GunPilot(attacker) != pilot)
		return;

	/* No xp for shooting yourself */
	if(attacker == wounded)
		return;

	/* No xp for shooting destroyed mechs */
	if(Destroyed(wounded))
		return;

	/* No xp for shooting a teammate */
	if(MechTeam(wounded) == MechTeam(attacker))
		return;

	/* Is the target in character ie: in simulators */
	if(!In_Character(wounded->mynum))
		return;

	/* No skill to match the weapon we're shooting with? */
	if(!(skname = FindGunnerySkillName(attacker, weapindx)))
		return;

	/* No xp for shooting mechwarriors if you not a mechwarrior */
	if(MechType(wounded) == CLASS_MW && MechType(attacker) != CLASS_MW)
		return;

	/* bth to high so no way to hit */
	if(!(bth <= 12))
		return;

	multiplier = multiplier * mudconf.btech_xp_modifier;

	if(mudconf.btech_xp_bthmod) {
		if(!(bth >= 3 && bth <= 12))
			return;				/* sure hits aren't interesting */
		multiplier = 2 * multiplier * bth_modifier[bth - 3] / 36;
	}

	omul = multiplier;

	/* Need to do a BV mod between the mechs */
	my_BV = MechBV(attacker);
	th_BV = MechBV(wounded);

	if(mudconf.btech_xp_usePilotBVMod) {
		myPilotBVMod = getPilotBVMod(attacker, weapindx);
		theirPilotBVMod = getPilotBVMod(wounded, weapindx);

		my_BV = my_BV * myPilotBVMod;
		th_BV = th_BV * theirPilotBVMod;

#ifdef XP_DEBUG
		SendDebug(tprintf
				  ("Using skill modified battle value for mechs %d and %d "
				   "with skill mods of %2.2f and %2.2f", attacker->mynum,
				   wounded->mynum, myPilotBVMod, theirPilotBVMod));
#endif
	}

	my_speed = NewMoveValue(attacker) + 1;
	th_speed = NewMoveValue(wounded) + 1;

	if(MechWeapons[weapindx].type == TMISSILE)
		weapTypeMod = mudconf.btech_xp_missilemod;
	else if(MechWeapons[weapindx].type == TAMMO)
		weapTypeMod = mudconf.btech_xp_ammomod;

	if(mudconf.btech_defaultweapdam > 1)
		damagemod = damage;
	else
		damagemod = 1;

	if(mudconf.btech_xp_vrtmod)
		vrtmod = (MechWeapons[weapindx].vrt <
				  30 ? sqrt((double) MechWeapons[weapindx].vrt / 30.0) : 1);
	else
		vrtmod = 1.0;

	multiplier =
		(vrtmod * weapTypeMod * multiplier * sqrt((double) (th_BV +
															1) * th_speed *
												  mudconf.
												  btech_defaultweapbv /
												  mudconf.
												  btech_defaultweapdam)) /
		(sqrt
		 ((double) (my_BV + 1) * my_speed *
		  MechWeapons[weapindx].battlevalue / damagemod));

	if(mudconf.btech_perunit_xpmod)
		multiplier = multiplier * MechXPMod(attacker); /* Per unit XP Mod. Defaults to 1 anyways */
	
	xp = BOUNDED(1, (int) (multiplier * damage / 100), 10);

	strcpy(buf, Name(wounded->mynum));

	// Emit XP gain over MechAttackXP
	if(char_gainxp(pilot, skname, (int) xp))
		SendAttackXP(tprintf
					 ("%s gained %d gun XP from feat of %f/100 difficulty "
					  "(%d damage) against %s", Name(pilot), (int) xp, multiplier,
					  damage, buf));
}								// end AccumulateGunXP()

void AccumulateGunXPold(dbref pilot, MECH * attacker, MECH * wounded,
						int numOccurences, float multiplier, int weapindx,
						int bth)
{
	int omul, xp;
	char *skname;
	char buf[MBUF_SIZE];

	/* Is the attacker in character ie: in simulators */
	if(!In_Character(attacker->mynum))
		return;

	if(!RGotGPilot(attacker))
		return;

	if(GunPilot(attacker) != pilot)
		return;

	/* No xp for shooting yourself */
	if(attacker == wounded)
		return;

	/* No xp for shooting destroyed units */
	if(Destroyed(wounded))
		return;

	/* No xp for shooting teammate */
	if(MechTeam(wounded) == MechTeam(attacker))
		return;

	/* if target is in character ie: in simulators or something */
	if(!In_Character(wounded->mynum))
		return;

	if(!(skname = FindGunnerySkillName(attacker, weapindx)))
		return;

	/* No xp for shooting a mechwarrior unless you a mechwarrior */
	if(MechType(wounded) == CLASS_MW && MechType(attacker) != CLASS_MW)
		return;

	if(!(bth >= 3 && bth <= 12))
		return;					/* sure hits aren't interesting */

	omul = multiplier;
	if(MechTons(attacker) > 0)
		multiplier = multiplier * BOUNDED(50,
										  100 * TonValue(wounded) /
										  TonValue(attacker), 150);
	else {
		/* Bring this to the attention of the admins */
		SendError(tprintf
				  ("AccumulateGunXP: Weird tonnage for IC mech #%d (%s): %d",
				   attacker->mynum, Name(attacker->mynum),
				   (short) MechTons(attacker)));
		return;
	}

	/* Hmm.. we have to figure the speed differences as well */
	{
		int my_speed = MoveValue(attacker);
		int th_speed = MoveValue(wounded);

		multiplier = multiplier * th_speed * th_speed / my_speed / my_speed;
	}

	multiplier = multiplier * bth_modifier[bth - 3] / 36;
	multiplier = multiplier * 2;	/* For average shot */
	if(mudconf.btech_perunit_xpmod)
		multiplier = multiplier * MechXPMod(attacker); /* Per unit XP Modifier. Defaults to 1 */

	if(Number(1, 50) > (multiplier * numOccurences))
		return;					/* Nothing for truly twinky stuff, occasionally */

	xp = BOUNDED(1, (int) (multiplier * numOccurences) / 100, 50);	/*Hardcoded limit */
	strcpy(buf, Name(wounded->mynum));
	/* Switching to Exile method of tracking xp, where we split
	 * Attacking and Piloting xp into two different channels
	 */
	if(char_gainxp(pilot, skname, (int) xp))
		SendAttackXP(tprintf("%s gained %d gun XP from feat of %f %% "
							 "difficulty (%d occurences) against %s",
							 Name(pilot), (int) xp, multiplier, numOccurences,
							 buf));
}

void fun_btgetcharvalue(char *buff, char **bufc, dbref player, dbref cause,
						char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = char id (#222)
	   fargs[1] = value name / value loc #
	   fargs[2] = flaggo (?) */
	dbref target;
	int targetcode, flaggo;

	FUNCHECK((target =
			  char_lookupplayer(player, cause, 0, fargs[0])) == NOTHING,
			 "#-1 INVALID TARGET");
	FUNCHECK(!Wiz(player), "#-1 PERMISSION DENIED!");
	if(Readnum(targetcode, fargs[1]))
		targetcode = char_getvaluecode(fargs[1]);
	FUNCHECK(targetcode < 0 ||
			 targetcode >= NUM_CHARVALUES, "#-1 INVALID VALUE");
	flaggo = atoi(fargs[2]);
	if(char_values[targetcode].type == CHAR_SKILL && flaggo == 4) {
		safe_tprintf_str(buff, bufc, "%d",
						 figure_xp_to_next_level(target, targetcode));
		return;
	}
	if(char_values[targetcode].type == CHAR_SKILL && flaggo == 3) {
		safe_tprintf_str(buff, bufc, "%d",
						 retrieve_stats(target,
										VALUES_SKILLS)->values[targetcode]);
		return;
	}
	if(char_values[targetcode].type == CHAR_SKILL && flaggo == 2) {
		safe_tprintf_str(buff, bufc, "%d", char_getxpbycode(target,
															targetcode));
		return;
	}
	if(char_values[targetcode].type == CHAR_SKILL && flaggo) {
		safe_tprintf_str(buff, bufc, "%d",
						 char_getskilltargetbycode(target, targetcode, 0));
		return;
	}
	safe_tprintf_str(buff, bufc, "%d", char_getvaluebycode(target,
														   targetcode));
}

void fun_btsetcharvalue(char *buff, char **bufc, dbref player, dbref cause,
						char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = char id (#222)
	   fargs[1] = value name / value loc #
	   fargs[2] = value to be set
	   fargs[3] = flaggo (?)
	 */
	dbref target;
	int targetcode, targetvalue, flaggo;

	FUNCHECK((target =
			  char_lookupplayer(player, cause, 0, fargs[0])) == NOTHING,
			 "#-1 INVALID TARGET");
	FUNCHECK(!Wiz(player), "#-1 PERMISSION DENIED!");
	if(Readnum(targetcode, fargs[1]))
		targetcode = char_getvaluecode(fargs[1]);
	FUNCHECK(targetcode < 0 ||
			 targetcode >= NUM_CHARVALUES, "#-1 INVALID VALUE");
	targetvalue = atoi(fargs[2]);
	flaggo = atoi(fargs[3]);

	/* We supposedly have everything at hand.. */
	if(flaggo) {
		FUNCHECK(char_values[targetcode].type != CHAR_SKILL,
				 "#-1 ONLY SKILLS CAN HAVE FLAG");
		if(flaggo == 1) {
			/* Need to do some evil frobbage here */
			char_setvaluebycode(target, targetcode, 0);
			targetvalue =
				char_getskilltargetbycode(target, targetcode,
										  0) - targetvalue;
		} else {
			if(flaggo != 3) {
				/* Add exp */
				char_gainxpbycode(target, targetcode, targetvalue);
				SendXP(tprintf("#%d added %d more %s XP to #%d", player,
							   targetvalue, char_values[targetcode].name,
							   target));
				safe_tprintf_str(buff, bufc, "%s gained %d more %s XP.",
								 Name(target), targetvalue,
								 char_values[targetcode].name);
			} else {
				/* Set the xp instead */
				char_gainxpbycode(target, targetcode,
								  targetvalue - char_getxpbycode(target,
																 targetcode));
				SendXP(tprintf
					   ("#%d set #%d's %s XP to %d", player, target,
						char_values[targetcode].name, targetvalue));
				safe_tprintf_str(buff, bufc, "%s's %s XP set to %d.",
								 Name(target), char_values[targetcode].name,
								 targetvalue);
			}
			return;
		}
	}
	char_setvaluebycode(target, targetcode, targetvalue);
	safe_tprintf_str(buff, bufc, "%s's %s set to %d", Name(target),
					 char_values[targetcode].name, char_getvaluebycode(target,
																	   targetcode));
}

/* ----------------------------------------------------------------------
** Syntax: btcharlist(skills|advantages|attributes[,targetplayer])
**
** Given one of the three arguments above, btcharlist returns the
** listing of each in a space delimited list.  This is basically a
** function version of +show. If the second argument is provided, only
** the skills/advantages that are learned or possessed will
** appear. For attributes the full list will be returned of since
** characters need all of them.
*/
void fun_btcharlist(char *buff, char **bufc, dbref player, dbref cause,
					char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	int i;
	int type = 0;
	int first = 1;
	dbref target = 0;
	enum {
		CHSKI,
		CHADV,
		CHATT,
	};
	static char *cmds[] = {
		"skills",
		"advantages",
		"attributes",
		NULL
	};

	if(!fn_range_check("BTCHARLIST", nfargs, 1, 2, buff, bufc))
		return;

	if(nfargs == 2) {
		target = char_lookupplayer(player, cause, 0, fargs[1]);
		if(target == NOTHING) {
			safe_str("#-1 FUNCTION (BTCHARLIST) INVALID TARGET", buff, bufc);
			return;
		}
	}

	switch (listmatch(cmds, fargs[0])) {
	case CHSKI:
		type = CHAR_SKILL;
		break;
	case CHADV:
		type = CHAR_ADVANTAGE;
		break;
	case CHATT:
		type = CHAR_ATTRIBUTE;
		break;
	default:
		safe_str("#-1 FUNCTION (BTCHARLIST) INVALID VALUE", buff, bufc);
		return;
	}

	for(i = 0; i < NUM_CHARVALUES; ++i)
		if(type == char_values[i].type) {
			if(nfargs == 2 && type != CHAR_ATTRIBUTE) {
				int targetcode = char_getvaluecode(char_values[i].name);
				if(char_getvaluebycode(target, targetcode) == 0)
					continue;
			}
			if(first)
				first = 0;
			else
				safe_str(" ", buff, bufc);
			safe_str(char_values[i].name, buff, bufc);
		}
	return;
}

#define MAX_PLAYERS_ON 10000

void debug_xptop(dbref player, void *data, char *buffer)
{
	int hm, i, j;
	dbref top[MAX_PLAYERS_ON];
	int topv[MAX_PLAYERS_ON];
	int count = 0, gt = 0;
	coolmenu *c = NULL;
	PSTATS *s;

#if 0
	notify(player, "Support discontinued. Bother a wiz if this bothers you.");
	return;
#endif

	bzero(top, sizeof(top));
	bzero(topv, sizeof(topv));
	skipws(buffer);
	DOCHECK(!*buffer, "Invalid argument!");
	DOCHECK((hm = char_getvaluecode(buffer)) < 0, "Invalid value name!");
	DOCHECK(char_values[hm].type != CHAR_SKILL,
			"Only skills have XP (for now at least)");
	DO_WHOLE_DB(i) {
		if(!isPlayer(i))
			continue;
		if(Wiz(i))
			continue;
		if(!(s = retrieve_stats(i, VALUES_SKILLS)))
			continue;
		if(!s->xp[hm])
			continue;
		top[count] = i;
		topv[count] = (s->xp[hm] % XP_MAX);
		gt += topv[count];
		count++;
	}
	for(i = 0; i < (count - 1); i++)
		for(j = i + 1; j < count; j++) {
			if(topv[j] > topv[i]) {
				topv[count] = topv[j];
				topv[j] = topv[i];
				topv[i] = topv[count];

				top[count] = top[j];
				top[j] = top[i];
				top[i] = top[count];
			}
		}
	addline();
	for(i = 0; i < MIN(16, count); i++) {
		addmenu(tprintf("%3d. %s", i + 1, Name(top[i])));
		addmenu(tprintf("%d (%.3f %%)", topv[i], (100.0 * topv[i]) / gt));
	}
	addline();
	if(gt) {
		addmenu(tprintf("Grand total: %d points", gt));
		addline();
	}
	ShowCoolMenu(player, c);
	KillCoolMenu(c);
}

static void store_health(dbref player, PSTATS * s)
{
	silly_atr_set(player, A_HEALTH, tprintf("%d,%d", char_gvalue(s,
																 "Bruise"),
											char_gvalue(s, "Lethal")));
}

static void retrieve_health(dbref player, PSTATS * s)
{
	char *c = silly_atr_get(player, A_HEALTH);
	PSTATS *s1;
	int i1, i2;

	if(sscanf(c, "%d,%d", &i1, &i2) != 2) {
		s1 = create_new_stats();
		memcpy(s, s1, sizeof(PSTATS));
		store_stats(player, s, VALUES_ALL);
		free((void *) s1);
		return;
	}
	char_svalue(s, "Bruise", i1);
	char_svalue(s, "Lethal", i2);
}

static void store_attrs(dbref player, PSTATS * s)
{
	silly_atr_set(player, A_ATTRS, tprintf("%d,%d,%d,%d,%d", char_gvalue(s,
																		 "Build"),
										   char_gvalue(s, "Reflexes"),
										   char_gvalue(s, "Intuition"),
										   char_gvalue(s, "Learn"),
										   char_gvalue(s, "Charisma")));
}

static void retrieve_attrs(dbref player, PSTATS * s)
{
	char *c = silly_atr_get(player, A_ATTRS);
	PSTATS *s1;
	int i1, i2, i3, i4, i5;

	if(sscanf(c, "%d,%d,%d,%d,%d", &i1, &i2, &i3, &i4, &i5) != 5) {
		s1 = create_new_stats();
		memcpy(s, s1, sizeof(PSTATS));
		store_stats(player, s, VALUES_ALL);
		free((void *) s1);
		return;
	}
	char_svalue(s, "Build", i1);
	char_svalue(s, "Reflexes", i2);
	char_svalue(s, "Intuition", i3);
	char_svalue(s, "Learn", i4);
	char_svalue(s, "Charisma", i5);
}

static void generic_retrieve_stuff(dbref player, PSTATS * s, int attr)
{
	char *c = silly_atr_get(player, attr), *e;
	char buf[512];
	int i1, i2, i3, sn;

	if(!*c)
		return;
	while (1) {
		i2 = i3 = 0;
		e = strchr(c, '/');
		if(sscanf(c, "%[A-Za-z_-]:%d,%d,%d", buf, &i1, &i2, &i3) < 2)
			return;
		/* Do the magic ;) */
		sn = char_getvaluecode(buf);
		if(sn >= 0) {
			s->values[sn] = i1;
			if(i2)
				s->xp[sn] = i2;
			if(i3)
				s->last_use[sn] = i3;
		}
		if(!(c = e))
			return;
		c++;
		if(!(*c))
			return;
	}
}

static void generic_store_stuff(dbref player, PSTATS * s, int attr, int flag)
{
	char buf[LBUF_SIZE];
	int i;
	char *c;

	buf[0] = 0;
	c = buf;
	for(i = 0; i < NUM_CHARVALUES; i++) {
		if(!s->values[i] && !s->xp[i])
			continue;
		if(flag) {
			if(char_values[i].type != CHAR_SKILL)
				continue;
		} else if(i != 5 && char_values[i].type != CHAR_ADVANTAGE)
			continue;
		if(s->xp[i])
			sprintf(c, "%s:%d,%d,%d/", char_values_short[i], s->values[i],
					s->xp[i], (int) s->last_use[i]);
		else
			sprintf(c, "%s:%d/", char_values_short[i], s->values[i]);
		while (*(++c));
	}
	if(*buf)
		silly_atr_set(player, attr, buf);
	else
		silly_atr_set(player, attr, "");
}

static void retrieve_skills(dbref player, PSTATS * s)
{
	generic_retrieve_stuff(player, s, A_SKILLS);
}

static void retrieve_advs(dbref player, PSTATS * s)
{
	generic_retrieve_stuff(player, s, A_ADVS);
}

static void store_skills(dbref player, PSTATS * s)
{
	generic_store_stuff(player, s, A_SKILLS, 1);
}

static void store_advs(dbref player, PSTATS * s)
{
	generic_store_stuff(player, s, A_ADVS, 0);
}

static void store_stats(dbref player, PSTATS * s, int modes)
{
	if(!isPlayer(player))
		return;
	if(modes & VALUES_HEALTH)
		store_health(player, s);
	if(modes & VALUES_ATTRS)
		store_attrs(player, s);
	if(modes & VALUES_ADVS) {
		if(player == cached_target_char)
			cached_target_char = -1;
		store_advs(player, s);
	}
	if(modes & VALUES_SKILLS) {
		if(player == cached_target_char)
			cached_target_char = -1;
		store_skills(player, s);
	}
}

static PSTATS *retrieve_stats(dbref player, int modes)
{
	static PSTATS s;

	bzero(&s, sizeof(PSTATS));
	if(modes & VALUES_HEALTH)
		retrieve_health(player, &s);
	if(modes & VALUES_ADVS)
		retrieve_advs(player, &s);
	if(modes & VALUES_ATTRS)
		retrieve_attrs(player, &s);
	if(modes & VALUES_SKILLS)
		retrieve_skills(player, &s);
	return &s;
}

void debug_setxplevel(dbref player, void *data, char *buffer)
{
	char *args[3];
	int xpt, code;

	DOCHECK(mech_parseattributes(buffer, args, 3) != 2, "Invalid arguments!");
	DOCHECK(Readnum(xpt, args[1]), "Invalid value!");
	DOCHECK(xpt < 0, "Threshold needs to be >=0 (0 = no gains possible)");
	DOCHECK((code =
			 char_getvaluecode(args[0])) < 0, "That isn't any charvalue!");
	DOCHECK(char_values[code].type != CHAR_SKILL, "That isn't any skill!");
	char_values[code].xpthreshold = xpt;
    log_error(LOG_WIZARD, "WIZ", "CHANGE", "Exp threshold for %s changed to %d by #%d", 
						 char_values[code].name, xpt, player);
}

int btthreshold_func(char *skillname)
{
	int code;

	if(!skillname || !*skillname)
		return -1;
	code = char_getvaluecode(skillname);
	if(code < 0)
		return -1;
	if(char_values[code].type != CHAR_SKILL)
		return -1;
	return char_values[code].xpthreshold;
}
