
/*
 * $Id: scen.h,v 1.1 2005/06/13 20:50:52 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Sun Oct 19 19:42:37 1997 fingon
 * Last modified: Tue Apr 28 21:48:19 1998 fingon
 *
 */

#ifndef SCEN_H
#define SCEN_H

ECMD(scen_start);
ECMD(scen_end);
ECMD(scen_status);

typedef struct {
    dbref mynum;
    int state;			/* 0 : Not started, 1: Started, 2: Ended */
    time_t start_t;
    time_t end_t;
} SCEN;

typedef struct {
    dbref mynum;
    char slet[10];		/* Side letters, if any, identifying mechs in this side */
} SSIDE;

typedef struct {
    dbref mynum;
    int state;
} SSOBJ;

typedef struct {
    dbref mynum;
} SSINS;

typedef struct {
    dbref mynum;
} SSEXT;

void scen_trigger_mine(MAP * map, MECH * mech, int x, int y);

void scen_see_base(MAP * map, MECH * mech, mapobj * o);
void scen_damage_base(MAP * map, MECH * mech, mapobj * o);
void scen_destroy_base(MAP * map, MECH * mech, mapobj * o);
MAP *scen_map(SCEN * s);


#define LOOP_THRU_SIDES(side,scen,i) \
  DOLIST(i,  Contents(scen->mynum)) \
    if (Hardcode(i)) \
      if (WhichSpecial(i) == GTYPE_SSIDE) \
	if ((side = (SSIDE *) FindObjectsData(i)))

#define LOOP_THRU_OBJECTIVES(ob,side,i) \
  DOLIST(i,  Contents(side->mynum)) \
    if (Hardcode(i)) \
      if (WhichSpecial(i) == GTYPE_SSOBJ) \
	if ((ob = (SSOBJ *) FindObjectsData(i)))

#define LOOP_THRU_INSERTIONS(ob,side,i) \
  DOLIST(i,  Contents(side->mynum)) \
    if (Hardcode(i)) \
      if (WhichSpecial(i) == GTYPE_SSINS) \
	if ((ob = (SSINS *) FindObjectsData(i)))

#define LOOP_THRU_EXTRACTIONS(ob,side,i) \
  DOLIST(i,  Contents(side->mynum)) \
    if (Hardcode(i)) \
      if (WhichSpecial(i) == GTYPE_SSEXT) \
	if ((ob = (SSEXT *) FindObjectsData(i)))

#define LOOP_MAP_MECHS(mech,map,i) \
  for (i = 0 ; i < map->first_free ; i++) \
    if ((mech = FindObjectsData(map->mechsOnMap[i])))

#define LOOP_DS_BAYS(d, mech, i) \
  for (i = 0 ; i < NUM_BAYS ; i++) \
    if ((d = AeroBay(mech, i)) > 0) \
      if (Hardcode(d)) \
        if (WhichSpecial(d) == GTYPE_MAP)

#define LOOP_MAP_MAPLINKS(map, o) \
  for (o = first_mapobj(map, TYPE_BUILD) ; o ; o = next_mapobj(o))

#define LOOP_MAP_MAPLINKS_REF(ob,map, o) \
  LOOP_MAP_MAPLINKS(map,o) \
    if ((ob = o->obj) > 0) \
      if (Hardcode(ob)) \
	if (WhichSpecial(ob) == GTYPE_MAP)

#define scen_mech_in_side(m,s) \
(!s->slet[0] || !strncmp(s->slet, silly_atr_get(m->mynum, A_MECHNAME), strlen(s->slet)))




#endif				/* SCEN_H */
