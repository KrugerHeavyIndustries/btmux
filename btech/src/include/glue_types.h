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
 * FIXME: The last modify timestamp is blatant lie.  One of these days, we need
 * to settle on consistent boilerplate, and slap it on everything.
 */

#ifndef GLUE_TYPES_H
#define GLUE_TYPES_H

#include <stddef.h>

typedef enum {
	GTYPE_MECH,
	GTYPE_DEBUG,
	GTYPE_MECHREP,
	GTYPE_MAP,
	GTYPE_AUTO,
	GTYPE_TURRET,
	GTYPE_UNUSED1 /* placeholder for old chargen object */
} GlueType;

/*
 * Base "class" for all XCODE objects.  Every XCODE object should start with a
 * field of this type, called 'xcode' by convention.
 */
typedef struct {
	GlueType type;	/* XCODE object type */
	size_t size;	/* object size */
} XCODE;

#endif /* !GLUE_TYPES_H */
