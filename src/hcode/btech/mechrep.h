
/*
 * $Id: mechrep.h,v 1.1.1.1 2005/01/11 21:18:29 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Last modified: Sat Jun  6 20:56:10 1998 fingon
 *
 */

#include "mech.h"

#ifndef _MECHREP_H
#define _MECHREP_H

/* This is the silly structure that I use for the repair stuff */
typedef struct mechrep_data {
    dbref mynum;
    dbref current_target;
} MECHREP;

/* Mech repair/type commands */
ECMD(mechrep_Raddspecial);
ECMD(mechrep_Raddtech);
ECMD(mechrep_Raddinftech);
ECMD(mechrep_Raddweap);
ECMD(mechrep_Rdeltech);
ECMD(mechrep_Rdelinftech);
ECMD(mechrep_Rdisplaysection);
ECMD(mechrep_Rloadnew);
ECMD(mechrep_Rloadnew2);
ECMD(mechrep_Rreload);
ECMD(mechrep_Rfiremode);
ECMD(mechrep_Rrepair);
ECMD(mechrep_Rresetcrits);
ECMD(mechrep_Rrestore);
ECMD(mechrep_Rsavetemp);
ECMD(mechrep_Rsavetemp2);
ECMD(mechrep_Rsetarmor);
ECMD(mechrep_Rsetheatsinks);
ECMD(mechrep_Rsetjumpspeed);
ECMD(mechrep_Rsetlrsrange);
ECMD(mechrep_Rsetmove);
ECMD(mechrep_Rsetradio);
ECMD(mechrep_Rsetradiorange);
ECMD(mechrep_Rsetscanrange);
ECMD(mechrep_Rsetspeed);
ECMD(mechrep_Rsettacrange);
ECMD(mechrep_Rsettarget);
ECMD(mechrep_Rsettech);
ECMD(mechrep_Rsettons);
ECMD(mechrep_Rsettype);
ECMD(mechrep_Rshowtech);

/* Mem alloc/free routines */
void newfreemechrep(dbref key, void **data, int selector);

#endif
