
/*
 * $Id: mech.ecm.h,v 1.1.1.1 2005/01/11 21:18:15 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Fri Mar 21 16:19:41 1997 fingon
 * Last modified: Fri Mar 21 16:19:53 1997 fingon
 *
 */

#ifndef MECH_ECM_H
#define MECH_ECM_H

/* mech.ecm.c */
void cause_ecm(MECH * from, MECH * to);
void end_ecm_check(MECH * mech);

#define ECM_NOTIFY_DISTURBED		0
#define ECM_NOTIFY_UNDISTURBED	1
#define ECM_NOTIFY_COUNTERED		2
#define ECM_NOTIFY_UNCOUNTERED	3

#endif				/* MECH.ECM_H */
