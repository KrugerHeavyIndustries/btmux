
/*
 * $Id: btechstats_global.h,v 1.1.1.1 2005/01/11 21:18:03 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Thu Sep 19 22:40:49 1996 fingon
 * Last modified: Sat Jun  6 20:20:38 1998 fingon
 *
 */

#ifndef BTECHSTATS_GLOBAL_H
#define BTECHSTATS_GLOBAL_H

#define VALUES_HEALTH 1		/* In PLBRUISE / PLLETHAL */
#define VALUES_SKILLS 2		/* In PLSKILLS */
#define VALUES_ATTRS  4		/* In PLATTRS */
#define VALUES_ADVS   8		/* In PLADVS */
#define VALUES_ALL    15
#define VALUES_CO     6		/* Attr + Skill */


#define CHAR_LASTSKILLTYPE CHAR_SOCIAL

/* hmm. */

#define CHAR_VALUE     0
#define CHAR_SKILL     1
#define CHAR_ADVANTAGE 2
#define CHAR_ATTRIBUTE 3

/* 4 diff. skill types */

#define CHAR_ATHLETIC    0x0001
#define CHAR_MENTAL      0x0002
#define CHAR_PHYSICAL    0x0004
#define CHAR_SOCIAL      0x0008

/* Career-types */
#define CAREER_CAVALRY   0x0010	/* Drive + Gun-Conv */
#define CAREER_BMECH     0x0020	/* Bmech Pilot/Gun */
#define CAREER_AERO      0x0040	/* Aero Pilot/Gun */
#define CAREER_ARTILLERY 0x0080	/* Artillery-Gun */
#define CAREER_DROPSHIP  0x0100	/* Dropship Pilot/Gun */
#define CAREER_TECHMECH  0x0200
#define CAREER_TECHVEH   0x0400
#define CAREER_TECH      (CAREER_TECHMECH|CAREER_TECHVEH)
#define CAREER_MISC      0x0800
#define CAREER_ACADMISC  0x1000
#define CAREER_RECON     0x2000
#define SK_XP            0x4000	/* Always raise xp (not spammable) */

#define XP_MAX    (256*256*256)	/* Then we wrap ; tough beans */


/* 3 diff. adv types */

#define CHAR_ADV_VALUE  0
#define CHAR_ADV_BOOL   1
#define CHAR_ADV_EXCEPT 2

#define CHAR_BLD 1
#define CHAR_REF 2
#define CHAR_INT 4
#define CHAR_LRN 8
#define CHAR_CHA 16

#define GREEN 0
#define REGULAR 1
#define VETEREN 2
#define ELITE 3
#define HISTORICAL 4


#include "p.btech.h"
#include "p.btechstats.h"

#define char_gvalue char_getstatvalue
#define char_svalue char_setstatvalue

#define char_getlives(a) char_getvalue(a, "lives")
#define char_getxp(a) char_getvalue(a, "maxxp")
#define char_getxpavail(a) char_getvalue(a, "xp")
#define char_getxp(a) char_getvalue(a, "maxxp")
#define char_getxpavail(a) char_getvalue(a, "xp")
#define char_getbruise(a) char_getvalue((a), "bruise")
#define char_getmaxbruise(a) char_getvalue((a), "maxbruise")
#define char_getlethal(a) char_getvalue((a), "lethal")
#define char_getmaxlethal(a) char_getvalue((a), "maxlethal")

#define char_glives(a) char_gvalue(a, "lives")
#define char_gxp(a) char_gvalue(a, "maxxp")
#define char_gxpavail(a) char_gvalue(a, "xp")
#define char_gbruise(a) char_gvalue((a), "bruise")
#define char_gmaxbruise(a) (char_gvalue((a), "build")*10)
#define char_glethal(a) char_gvalue((a), "lethal")
#define char_gmaxlethal(a) (char_gvalue((a), "build")*10)

#define char_setlives(a,b) char_setvalue((a), "lives", (b))
#define char_setbruise(a,b) char_setvalue((a), "Bruise", (b))
#define char_setmaxbruise(a,b) char_setvalue((a), "maxbruise", (b))
#define char_setlethal(a,b) char_setvalue((a), "Lethal", (b))
#define char_setmaxlethal(a,b) char_setvalue((a), "maxlethal", (b))

#define char_slives(a,b) char_svalue((a), "lives", (b))
#define char_sbruise(a,b) char_svalue((a), "bruise", (b))
#define char_smaxbruise(a,b) char_svalue((a), "maxbruise", (b))
#define char_slethal(a,b) char_svalue((a), "lethal", (b))
#define char_smaxlethal(a,b) char_svalue((a), "maxlethal", (b))

#define EE_NUMBER    11
#define LIVES_NUMBER 5

#define char_getstatvaluebycode(s,code) (code >= 0 ? (s->values[code] + (char_values[code].type == CHAR_SKILL ? char_xp_bonus(s,code) : 0) ): -1)
#define char_getstatvaluebycode(s,code) (code >= 0 ? (s->values[code] + (char_values[code].type == CHAR_SKILL ? char_xp_bonus(s,code) : 0) ): -1)
#define char_setstatvaluebycode(s,code,value) \
if (code >= 0) \
  { if (code == EE_NUMBER) s->values[LIVES_NUMBER]+=value-s->values[code];\
  s->values[code] = value; }
#define char_getvaluebycode(player,code) \
  char_getstatvaluebycode(retrieve_stats(player, VALUES_ALL), code)
#define char_setvaluebycode(player,code,value) \
  { PSTATS *hm = retrieve_stats(player, VALUES_ALL); \
    char_setstatvaluebycode(hm, code, value); \
    store_stats(player, hm, VALUES_ALL); \
  }
#endif				/* BTECHSTATS_GLOBAL_H */
