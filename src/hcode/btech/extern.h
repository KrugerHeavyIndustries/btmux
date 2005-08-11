
/*
 * $Id: extern.h,v 1.1.1.1 2005/01/11 21:18:06 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Tue Oct  8 22:18:31 1996 fingon
 * Last modified: Tue Oct  8 22:19:51 1996 fingon
 *
 */

#ifndef EXTERN_H
#define EXTERN_H

#define	FUNCTION(x)	\
	void x(buff, bufc, player, cause, fargs, nfargs, cargs, ncargs) \
	char *buff, **bufc; \
	dbref player, cause; \
	char *fargs[], *cargs[]; \
	int nfargs, ncargs;

#endif				/* EXTERN_H */
