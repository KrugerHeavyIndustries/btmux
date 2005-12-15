
/*
 * $Id: mine.h,v 1.1.1.1 2005/01/11 21:18:29 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Tue Oct 22 18:32:22 1996 fingon
 * Last modified: Tue Oct 21 18:35:36 1997 fingon
 *
 */

#ifndef MINE_H
#define MINE_H

#define MINE_LOW  1
#define MINE_HIGH 5
#define MINE_STANDARD 1
#define MINE_INFERNO  2
#define MINE_COMMAND  3
#define MINE_VIBRA    4
#define MINE_TRIGGER  5		/* Same as vibra, except shows _no_ message,
				   and doesn't get destroyed */
#define VIBRO(a)      (a == MINE_VIBRA || a == MINE_TRIGGER)

#endif				/* MINE_H */
