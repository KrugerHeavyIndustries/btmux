
/*
 * $Id: scen.c,v 1.1 2005/06/13 20:50:52 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry 
 *       All rights reserved
 *
 * Created: Sun Oct 19 19:44:29 1997 fingon
 * Last modified: Sat Jun  6 22:25:54 1998 fingon
 *
 */

#include "create.h"
#include "mech.h"
#include "glue.h"
#include "scen.h"

#include "coolmenu.h"
#include "mycool.h"
#include "p.mech.utils.h"

void do_destroy(dbref player, dbref cause, int key, char *what);
dbref match_thing(dbref player, char *name);
char *get_uptime_to_string(int uptime);

dbref scen_map_ref(SCEN * s)
{
    char buf3[LBUF_SIZE];
    dbref d;

    if (!bt_get_attr(buf3, s->mynum, "MAP_REF"))
	return -1;
    d = match_thing(GOD, buf3);
    return d;

}

dbref scen_weather_ref(SCEN * s)
{
    char buf3[LBUF_SIZE];
    dbref d;

    if (!bt_get_attr(buf3, s->mynum, "WEATHER_REF"))
	return -1;
    d = match_thing(GOD, buf3);
    return d;

}

MAP *scen_map(SCEN * s)
{
    return FindObjectsData(scen_map_ref(s));
}

#define MySafe(i) (Flags((i)) & SAFE)

void newfreescen(dbref key, void **data, int sel)
{
    SCEN *new;
    SSIDE *si;
    SSOBJ *ob;
    SSINS *in;
    SSEXT *ex;
    MAP *map, *ma;
    mapobj *o;
    MECH *mech, *tm;

    int d, d1, d2, d3, d4;

    /* Make things go *bang* ;-) */

    if (sel == SPECIAL_FREE) {
	new = *data;

	if ((map = scen_map(new))) {
	    /* Destroy everything on map, and (try to) destroy hangars
	       involved as well. Therefore marking non-stupid hangars
	       'safe' is a wise move ;-) */
	    LOOP_MAP_MECHS(mech, map, d4) {
		if (!MySafe(mech->mynum)) {
		    LOOP_DS_BAYS(d, mech, d2) {
			ma = FindObjectsData(d);
			LOOP_MAP_MECHS(tm, ma, d3)
			    do_destroy(GOD, GOD, 0, tprintf("#%d",
				tm->mynum));
			do_destroy(GOD, GOD, 0, tprintf("#%d", ma->mynum));
		    }
		    do_destroy(GOD, GOD, 0, tprintf("#%d", mech->mynum));
		}
	    }

	    LOOP_MAP_MAPLINKS_REF(d3, map, o) {
		if (!MySafe(d3)) {
		    ma = FindObjectsData(d3);
		    LOOP_MAP_MECHS(mech, ma, d4)
			do_destroy(GOD, GOD, 0, tprintf("#%d",
			    mech->mynum));
		    do_destroy(GOD, GOD, 0, tprintf("#%d", d3));
		}
	    }
	    do_destroy(GOD, GOD, 0, tprintf("#%d", map->mynum));
	}
	if ((d = scen_weather_ref(new)))
	    do_destroy(GOD, GOD, 0, tprintf("#%d", d));
	LOOP_THRU_SIDES(si, new, d1) {
	    LOOP_THRU_OBJECTIVES(ob, si, d2)
		do_destroy(GOD, GOD, 0, tprintf("#%d", ob->mynum));
	    LOOP_THRU_INSERTIONS(in, si, d2)
		do_destroy(GOD, GOD, 0, tprintf("#%d", in->mynum));
	    LOOP_THRU_EXTRACTIONS(ex, si, d2)
		do_destroy(GOD, GOD, 0, tprintf("#%d", ex->mynum));
	    do_destroy(GOD, GOD, 0, tprintf("#%d", si->mynum));
	}
    }
}

SCEN *Map_in_Valid_SO(MAP * map)
{
    dbref d;
    SCEN *s;

    if (!map)
	return NULL;
    d = Location(map->mynum);
    if (Hardcode(d))
	if (WhichSpecial(d) == GTYPE_SCEN) {
	    s = FindObjectsData(d);
	    if (s->state != 1)
		return NULL;
	    return s;
	}
    return NULL;
}


void scen_set_osucc(SSOBJ * o, int val)
{
    char buf[LBUF_SIZE];
    char buf2[LBUF_SIZE];

    if (o->state != val) {
	strcpy(buf, Name(Location(Location(o->mynum))));
	strcpy(buf2, Name(Location(o->mynum)));
	ScenStatus("In %s/%s/%s - %d => %d", buf, buf2, Name(o->mynum),
	    o->state, val);
	o->state = val;
    }
}

static int location_matches_xy(SSOBJ * ob, int x, int y)
{
    char buf[LBUF_SIZE];
    int myx, myy;

    if (bt_get_attr(buf, ob->mynum, "LOCATION"))
	if (sscanf(buf, "%d %d", &myx, &myy) >= 2)
	    if (myx == x && myy == y)
		return 1;
    return 0;
}


void scen_trigger_mine(MAP * map, MECH * mech, int x, int y)
{
    SCEN *s;
    SSIDE *si;
    SSOBJ *ob;
    char buf[LBUF_SIZE];
    dbref d1, d2;

    if (!(s = Map_in_Valid_SO(map)))
	return;
    LOOP_THRU_SIDES(si, s, d1)
	LOOP_THRU_OBJECTIVES(ob, si, d2) {
	if (!bt_get_attr(buf, ob->mynum, "TYPE"))
	    continue;
	if (strcmp(buf, "hex"))
	    continue;
	if (!location_matches_xy(ob, x, y))
	    continue;
	if (!bt_get_attr(buf, ob->mynum, "GOAL"))
	    continue;
	if (!strcmp(buf, "recon"))
	    if (scen_mech_in_side(mech, si))
		scen_set_osucc(ob, 100);
	if (!strcmp(buf, "capture"))
	    scen_set_osucc(ob, scen_mech_in_side(mech, si) ? 100 : 0);
	if (!strcmp(buf, "defend"))
	    if (!scen_mech_in_side(mech, si))
		scen_set_osucc(ob, 0);

    }
}

void scen_base_generic(MAP * map, MECH * mech, mapobj * o, char *type,
    int val, int sside)
{
    SCEN *s;
    SSIDE *si;
    SSOBJ *ob;
    char buf[LBUF_SIZE];
    dbref d1, d2;

    if (!(s = Map_in_Valid_SO(map)))
	return;
    LOOP_THRU_SIDES(si, s, d1)
	LOOP_THRU_OBJECTIVES(ob, si, d2) {
	if (!bt_get_attr(buf, ob->mynum, "TYPE"))
	    continue;
	if (strcmp(buf, "base"))
	    continue;
	if (!location_matches_xy(ob, o->x, o->y))
	    continue;
	if (!bt_get_attr(buf, ob->mynum, "GOAL"))
	    continue;
	if (!strcmp(buf, type))
	    if (!sside || scen_mech_in_side(mech, si))
		scen_set_osucc(ob, val);
    }
}

void scen_see_base(MAP * map, MECH * mech, mapobj * o)
{
    scen_base_generic(map, mech, o, "recon", 100, 1);
}

void scen_damage_base(MAP * map, MECH * mech, mapobj * o)
{
    scen_base_generic(map, mech, o, "damage", 100, 1);
}

void scen_destroy_base(MAP * map, MECH * mech, mapobj * o)
{
    scen_base_generic(map, mech, o, "destroy", 100, 1);
    scen_base_generic(map, mech, o, "defend", 0, 0);
}


/* Unimplemented or unfinished */

void scen_start_oods(SCEN * s)
{

}

void scen_update_enemy(int *now, int *best, int mode, MECH * ds,
    MECH * mech)
{
    /* For now, we just handle the 0 = damage, 1 = destroy */
    /* 0 scores are calculated by present / maximum int+armor (if not dest'ed, otherwise max int+armor for both) */
    int i, j, k;

    if (mode == 0) {
	for (i = 0; i < NUM_SECTIONS; i++) {
	    j = GetSectOArmor(mech, i) + GetSectORArmor(mech,
		i) + GetSectOInt(mech, i);
	    k = GetSectArmor(mech, i) + GetSectRArmor(mech,
		i) + GetSectInt(mech, i);
	    if (Destroyed(mech))
		k = j;
	    else
		k = j - k;
	    *best += j;
	    *now += k;
	}
	return;
    }
    if (mode == 1) {
	(*best)++;
	if (Destroyed(mech))
	    (*now)++;
	return;
    }
}

int scen_update_enemies(SSIDE * si, MAP * map, int dest)
{
    int i, i2, i3;
    dbref d;
    MECH *mech, *tm;
    int score_n = 0, score_m = 0;
    MAP *ma;
    mapobj *o;

    if (!map)
	return 1;
    LOOP_MAP_MECHS(mech, map, i) {
	if (IsDS(mech)) {
	    /* Do not count DS itself, but count stuff inside DS */
	    LOOP_DS_BAYS(d, mech, i2) {
		ma = FindObjectsData(d);
		LOOP_MAP_MECHS(tm, ma, i3)
		    scen_update_enemy(&score_n, &score_m, dest, mech, tm);
	    }
	}
    }
    LOOP_MAP_MAPLINKS_REF(d, map, o) {
	ma = FindObjectsData(d);
	LOOP_MAP_MECHS(tm, ma, i3)
	    scen_update_enemy(&score_n, &score_m, dest, NULL, tm);
    }
    if (!score_m)
	return 100;
    return 100 * score_n / score_m;
}

void scen_update_goal(SCEN * s, SSIDE * si, SSOBJ * ob)
{
    char buf[LBUF_SIZE];
    char buf2[LBUF_SIZE];
    char buf3[LBUF_SIZE];
    dbref d;
    MECH *mech;
    int i, j;

    if (!bt_get_attr(buf, ob->mynum, "TYPE"))
	return;
    if (!bt_get_attr(buf2, ob->mynum, "GOAL"))
	return;
    if (!strcmp(buf, "unit") || !strcmp(buf, "existing unit")) {
	/* Base success on what has happened to the unit */
	/* Note: You HAVE to !claim mech for capture objective to work */
	if (!bt_get_attr(buf3, s->mynum, "REF"))
	    return;
	d = match_thing(GOD, buf3);
	if (d <= 0)
	    return;
	if (!(mech = getMech(d)))
	    return;
	if (!strcmp(buf2, "capture")) {
	    if (!Destroyed(mech))
		scen_set_osucc(ob, scen_mech_in_side(mech, si) ? 100 : 0);
	    return;
	}
	if (!strcmp(buf2, "damage")) {
	    i = j = 0;
	    scen_update_enemy(&i, &j, 0, NULL, mech);
	    scen_set_osucc(ob, j > 0 ? 100 * i / j : 0);
	    return;
	}
	if (!strcmp(buf2, "destroy")) {
	    if (Destroyed(mech))
		scen_set_osucc(ob, 100);
	    return;
	}
	if (!strcmp(buf2, "defend")) {
	    if (Destroyed(mech))
		scen_set_osucc(ob, 0);
	    return;
	}
	return;
    }
    if (!strcmp(buf, "enemies")) {
	/* Enemy 'location' :
	   on main map
	   in any hangar on map
	   in any DS on map
	 */

	if ((d = scen_map_ref(s)) <= 0)
	    return;
	if (!strcmp(buf2, "capture"))
	    scen_set_osucc(ob, scen_update_enemies(si, FindObjectsData(d),
		    -1));
	if (!strcmp(buf2, "damage"))
	    scen_set_osucc(ob, scen_update_enemies(si, FindObjectsData(d),
		    0));
	if (!strcmp(buf2, "destroy"))
	    scen_set_osucc(ob, scen_update_enemies(si, FindObjectsData(d),
		    1));
    }
}



void scen_start(dbref player, void *data, char *buffer)
{
    SCEN *s = (SCEN *) data;

    if (!s)
	return;
    DOCHECK(s->state > 0, "This scenario has been already started.");
    ScenStatus("Scenario #%d (%s) has been engaged by #%d", s->mynum,
	Name(s->mynum), player);
    s->state = 1;
    s->start_t = mudstate.now;
    scen_start_oods(s);
}

void scen_tport_players(dbref from, int death)
{
    dbref i, tmpnext;
    int to;

    SAFE_DOLIST(i, tmpnext, Contents(from))
	if (!Wiz(i))
	if (isPlayer(i)) {
	    to = death ? mudconf.afterlife_dbref : mudconf.afterscen_dbref;
	    hush_teleport(i, to);
	}
}

void scen_handle_mech_extraction(SCEN * s, MAP * map, MECH * mech)
{
    SSIDE *si;
    dbref d;
    int d1, d2, d3, d4;
    int succ = 0;
    char buf[LBUF_SIZE];
    MECH *ds;
    float x1, y1;
    int x, y, r;
    SSEXT *ex;

    /* First off, figure side we're on */
    LOOP_THRU_SIDES(si, s, d1)
	if (scen_mech_in_side(mech, si)) {
	/* Extraction blues */
	LOOP_THRU_EXTRACTIONS(ex, si, d2) {
	    if (bt_get_attr(buf, ex->mynum, "TYPE")) {
		if (!strcmp(buf, "dropship pickup")) {
		    /* Basically, we should be onboard a DS */
		    if (bt_get_attr(buf, ex->mynum, "DBREF")) {
			d = match_thing(GOD, buf);
			if (d >= 0 && (ds = FindObjectsData(d))) {
			    LOOP_DS_BAYS(d3, ds, d4) {
				if (d3 == mech->mapindex)
				    succ = 1;
				break;
			    }
			}
		    }
		} else if (!strcmp(buf, "base")) {
		    if (bt_get_attr(buf, ex->mynum, "DBREF")) {
			d = match_thing(GOD, buf);
			if (mech->mapindex == d)
			    succ = 1;
		    }
		} else if (!strcmp(buf, "leaving map")) {
		    /* Determine whether we're on the main map or not */
		    if (scen_map_ref(s) == mech->mapindex) {
			if (bt_get_attr(buf, ex->mynum, "LOCATION"))
			    if (sscanf(buf, "%d %d %d", &x, &y, &r) == 3) {
				/* Hm, calculate distance to the hex,
				   and judge succ by it */
				MapCoordToRealCoord(x, y, &x1, &y1);
				if (FindRange(MechFX(mech), MechFY(mech),
					MechFZ(mech), x1, y1, 0) < r)
				    succ = 1;
			    }
		    }
		}
	    }
	    if (succ)
		break;
	}
	break;
    }
    /* Hm, for now we don't do a thing to mechs with failed extraction
       (except for sending the pilots to limbo, muah) */
    scen_tport_players(mech->mynum, !succ);
}

static void ext_check(SCEN * s)
{
    MAP *map = scen_map(s), *ma;
    MECH *mech, *tm;
    dbref d;
    int i1, i2, i3;
    mapobj *o;

    if (!map)
	return;
    LOOP_MAP_MECHS(mech, map, i1) {
	if (IsDS(mech)) {
	    LOOP_DS_BAYS(d, mech, i2) {
		ma = FindObjectsData(d);
		LOOP_MAP_MECHS(tm, ma, i3)
		    scen_handle_mech_extraction(s, ma, tm);
		scen_tport_players(d, 0);
	    }
	    scen_tport_players(mech->mynum, 0);
	    s_Zombie(mech->mynum);
	} else
	    scen_handle_mech_extraction(s, map, mech);
    }
    LOOP_MAP_MAPLINKS_REF(d, map, o) {
	ma = FindObjectsData(d);
	LOOP_MAP_MECHS(tm, ma, i3)
	    scen_handle_mech_extraction(s, ma, tm);
	scen_tport_players(d, 0);
    }
}

void scen_end(dbref player, void *data, char *buffer)
{
    SCEN *s = (SCEN *) data;
    SSIDE *si;
    SSOBJ *ob;
    dbref d1, d2;

    if (!s)
	return;
    DOCHECK(!s->state, "This scenario hasn't been even started yet.");
    DOCHECK(s->state == 2, "This scenario has already ended.");
    ScenStatus("Scenario #%d (%s) has been ended by #%d", s->mynum,
	Name(s->mynum), player);
    ext_check(s);
    LOOP_THRU_SIDES(si, s, d1)
	LOOP_THRU_OBJECTIVES(ob, si, d2)
	scen_update_goal(s, si, ob);
    s->end_t = mudstate.now;
    s->state = 2;
}

void show_goals_side(coolmenu * c, SCEN * s, SSIDE * si)
{
    SSOBJ *ob;
    dbref d2;
    char buf[LBUF_SIZE];
    int wscore = 0, score = 0, wgoals = 0, goals = 0, pri, iswin;

    LOOP_THRU_OBJECTIVES(ob, si, d2) {
	scen_update_goal(s, si, ob);
	if (!bt_get_attr(buf, ob->mynum, "TYPE"))
	    continue;
	if (!goals)
	    vsi(tprintf("Objective status for side %s",
		    si->slet ? si->slet : "?"));
	addmenu4(tprintf(" %s", buf));
	bt_get_attr(buf, ob->mynum, "GOAL");
	addmenu4(buf);
	bt_get_attr(buf, ob->mynum, "LOCATION");
	addmenu4(buf);
	addmenu4(tprintf("%d", s->state));

	goals++;
	bt_get_attr(buf, ob->mynum, "PRIORITY");
	pri = 1;
	iswin = 1;
	if (!strcmp(buf, "secondary"))
	    pri = 2;
	else if (!strcmp(buf, "optional")) {
	    pri = 3;
	    iswin = 0;
	}
	if (iswin) {
	    wgoals++;
	    wscore += s->state / pri;
	}
	score += s->state / pri;
    }
    if (wgoals) {
	addmenu4(tprintf("WSc:%d(%d)", wscore, wgoals));
	addmenu4(tprintf("WScW:%d", wscore / wgoals));
    }
    if (goals && wgoals != goals) {
	addmenu4(tprintf("Sc:%d(%d)", score, goals));
	addmenu4(tprintf("ScW:%d", score / goals));
    }
}

void show_goals(coolmenu * c, SCEN * s, char *side)
{
    SSIDE *si;
    dbref d1;

    LOOP_THRU_SIDES(si, s, d1) {
	if (side && *side) {
	    if (!strcmp(side, si->slet))
		show_goals_side(c, s, si);
	} else
	    show_goals_side(c, s, si);
    }
}

void scen_status(dbref player, void *data, char *buffer)
{
    SCEN *s = (SCEN *) data;
    char *temp;
    char *scen_states[] = { "Initialized", "Running", "Ended" };
    coolmenu *c = NULL;

    if (!s)
	return;
    addline();
    cent(tprintf("Status for %s", Name(s->mynum)));
    addline();
    vsi(tprintf("State of this scenario: %s", scen_states[s->state]));
    if (s->state == 1)
	vsi(tprintf("Uptime: %s",
		get_uptime_to_string(mudstate.now - s->start_t)));
    if (s->state == 2) {
	temp = (char *) ctime(&s->start_t);
	temp[strlen(temp) - 1] = '\0';
	addmenu(tprintf("Started: %s", temp));
	temp = (char *) ctime(&s->end_t);
	temp[strlen(temp) - 1] = '\0';
	addmenu(tprintf("Ended: %s", temp));
	vsi(tprintf("Duration: %s",
		get_uptime_to_string(s->end_t - s->start_t)));
    }
    addline();
    if (*buffer)
	show_goals(c, s, buffer);
    else
	show_goals(c, s, NULL);
    addline();
    ShowCoolMenu(player, c);
    KillCoolMenu(c);
}
