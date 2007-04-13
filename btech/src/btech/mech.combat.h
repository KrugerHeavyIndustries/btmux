
/*
 * $Id: mech.combat.h,v 1.1.1.1 2005/01/11 21:18:13 kstevens Exp $
 *
 * Author: Cord Awtry <kipsta@mediaone.net>
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Based on work that was:
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2000 Thomas Wouters
 */

#ifndef MECH_COMBAT_H
#define MECH_COMBAT_H

#define Clustersize(weapindx) (((MechWeapons[weapindx].special & (IDF | MRM | ROCKET)) && (MechWeapons[weapindx].damage == 1))? 5 : 1)

#define Swap(val1,val2) { rtmp = val1 ; val1 = val2 ; val2 = rtmp; }

#endif				/* MECH_COMBAT_H.ECM_H */
