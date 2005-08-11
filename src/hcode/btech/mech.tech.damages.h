
/*
 * $Id: mech.tech.damages.h,v 1.1.1.1 2005/01/11 21:18:25 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Mon Dec  2 19:58:48 1996 fingon
 * Last modified: Mon Dec  2 22:02:54 1996 fingon
 *
 */

#ifndef MECH_TECH_DAMAGES_H
#define MECH_TECH_DAMAGES_H

/* Added RESEAL to repair flooded sections
 * -Kipsta
 * 8/4/99
 */

enum damage_type {
    REATTACH, REPAIRP, REPAIRP_T, ENHCRIT_MISC, ENHCRIT_FOCUS,
    ENHCRIT_CRYSTAL, ENHCRIT_BARREL, ENHCRIT_AMMOB, ENHCRIT_RANGING,
    ENHCRIT_AMMOM, REPAIRG, RELOAD, FIXARMOR, FIXARMOR_R,
    FIXINTERNAL, DETACH, SCRAPP, SCRAPG, UNLOAD, RESEAL, REPLACESUIT,
    NUM_DAMAGE_TYPES
};

/* Reattachs / fixints / fixarmors, repair / reload */
#define MAX_DAMAGES (3 * NUM_SECTIONS + 2 * NUM_SECTIONS * NUM_CRITICALS)

#endif				/* MECH_TECH_DAMAGES_H */
