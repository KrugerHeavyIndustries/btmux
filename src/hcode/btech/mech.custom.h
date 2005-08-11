
/*
 * $Id: mech.custom.h,v 1.1.1.1 2005/01/11 21:18:14 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *       All rights reserved
 *
 * Created: Sat Feb 22 16:25:09 1997 fingon
 * Last modified: Mon Jun  1 22:29:22 1998 fingon
 *
 */

#ifndef MECH_CUSTOM_H
#define MECH_CUSTOM_H

typedef struct custom_struct {
    dbref mynum;
    int state;			/* State we are in now? */
    dbref user;
    dbref submit;		/* Who submitted the design? */
    int allow;			/* Types to allow */
    MECH new;			/* 'mech structure we use. mynum shows which
				   mech it is */
} CUSTOM;

#define STATE_MAIN   0
#define STATE_LIMB  -1
#define STATE_ARMOR -2

#define ALTER_CRIT         0
#define ALTER_ARMOR        1
#define DISCARD_CHANGES    2
#define APPLY_FOR_APPROVAL 3
#define DO_IT              4

#define ADD_WEAPON         0
#define ADD_AMMO           1
#define ADD_SPECIAL        2
#define REMOVE             3
#define ADD_NWEAPON        4
#define ADD_NAMMO          5
#define ADD_CWEAPON        6
#define ADD_CAMMO          7
#define ADD_NSPECIAL       8
#define TOGGLE_REAR        9
#define TOGGLE_TC          10
#define TOGGLE_AMMO        11
#define TOGGLE_HALFAMMO    12
#define TOGGLE_OS          13

#define FIRST_UNUSED_BIT   256	/* For the weird shit */

extern void newfreecustom(dbref key, void **data, int selector);

ECMD(custom_back);
ECMD(custom_edit);
ECMD(custom_finish);
ECMD(custom_help);
ECMD(custom_look);

ECMD(custom_weaponspecs);
ECMD(custom_critstatus);
ECMD(custom_status);

#endif				/* MECH_CUSTOM_H */
