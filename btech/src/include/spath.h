
/*
 * $Id: spath.h,v 1.1 2005/06/13 20:50:53 murrayma Exp $
 *
 * Last modified: Sat Jun  6 20:57:26 1998 fingon
 *
 */

#ifndef SPATH_H
#define SPATH_H

#define HEX_BASED
#undef APPROXIMATE_ASTAR

/* note the cols should be 2 more than the screen size because if we
 * have a point at the extremes and we check to see if we can go in
 * all directions, we don't need a special case to check these. (same
 * for rows) */

typedef struct {
    int *coords;
    int coordcount;
    int iterations;
    int score;
} SPATHRESULT;

void FreePath(SPATHRESULT * res);
SPATHRESULT *CalculatePath(int x1, int y1, int x2, int y2, int errper);


#ifdef _SPATH_C


#ifdef HEX_BASED
#define NBCOUNT 6
#else
#define NBCOUNT 8
#endif

typedef struct NODETYPE {
    int x, y;
    int f, h, g;
    int NodeNum;
    struct NODETYPE *Parent;
    struct NODETYPE *Child[NBCOUNT];	/* a node may have upto NBC children. */
    struct NODETYPE *next;	/* for filing purposes */
} NODE;


#define TileNum(x,y) ((x) + ((y) << 16))

/**************************************************************************/

/*                                 STACK                                  */

/**************************************************************************/
typedef struct STACKTYPE {
    NODE *Node;
    struct STACKTYPE *Next;
} MYSTACK;

#endif
#include "p.spath.h"

#endif				/* SPATH_H */
