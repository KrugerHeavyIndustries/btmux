
/*
 * $Id: pcombat.h,v 1.1.1.1 2005/01/11 21:18:31 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Sun Mar 23 20:11:57 1997 fingon
 * Last modified: Thu Aug 14 17:34:27 1997 fingon
 *
 */

#ifndef PCOMBAT_H
#define PCOMBAT_H

/* pcombat.c */
int pc_to_dam_conversion(MECH * target, int weapindx, int dam);
int dam_to_pc_conversion(MECH * target, int weapindx, int dam);
int armor_effect(MECH * wounded, int cause, int hitloc, int intDamage,
    int id);

#endif				/* PCOMBAT_H */
