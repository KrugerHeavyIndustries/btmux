
/*
 * $Id: create.h,v 1.1 2005/06/13 20:50:45 murrayma Exp $
 *
 * Author: Markus "iDLari" Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Thu Aug 29 09:51:22 1996 fingon
 * Last modified: Fri Nov 27 20:15:14 1998 fingon
 *
 */

#ifndef CREATE_H
#define CREATE_H

#define Create(a,b,c) \
if (!((a) = ( b * ) calloc(sizeof( b ), c ) )) \
{ printf ("Unable to malloc!\n"); exit(1); }

#define MyReCreate(a,b,c) \
if (!((a) = ( b * ) realloc((void *) a, sizeof( b ) * (c) ) )) \
{ printf ("Unable to realloc!\n"); exit(1); }

#define ReCreate(a,b,c) \
if (a) { MyReCreate(a,b,c); } else { Create(a,b,c); }

#define Free(a) if (a) {free(a);a=0;}

#endif				/* CREATE_H */
