
/*
 * $Id: glue_types.h,v 1.1 2005/06/13 20:50:52 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1998 Markus Stenberg
 *       All rights reserved
 *
 * Created: Mon May 18 19:45:10 1998 fingon
 * Last modified: Mon May 18 19:45:28 1998 fingon
 *
 */

#ifndef GLUE_TYPES_H
#define GLUE_TYPES_H

#define GTYPE_MECH    0
#define GTYPE_DEBUG   1
#define GTYPE_MECHREP 2
#define GTYPE_MAP     3
#define GTYPE_CHARGEN 4
#define GTYPE_AUTO    5
#define GTYPE_TURRET  6
#define GTYPE_CUSTOM  7

#define GTYPE_SCEN    8		/* Main scenario thing */
#define GTYPE_SSIDE   9		/* Scenario: Per-side object */
#define GTYPE_SSOBJ  10		/* Scenario: Side-specific objective */
#define GTYPE_SSINS  11		/* Scenario: Side-specific insertion */
#define GTYPE_SSEXT  12		/* Scenario: Side-specific extraction */

#endif				/* GLUE_TYPES_H */
