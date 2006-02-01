
/*
 *
 * Original author: unknown
 *
 * Copyright (c) 1996-2002 Markus Stenberg
 * Copyright (c) 1998-2002 Thomas Wouters 
 * Copyright (c) 2000-2002 Cord Awtry 
 *
 */

/*************************************************************
****  LOS code  **********************************************
**************************************************************

What happens in the code now:
(pretty close, anyway -- I haven't spent THAT much time studying it.

Right now, the code just finds a line between the hexes and steps
along it in units of 1 hex, checking each hex it enters.  This is,
well, wrong.  It makes no allowance whatsoever that the LOS line might
"clip" the edge of a hex, and will occasionally skip hexes:
 __    __    __
/# \__/  \__/# \
\__/  \__/  \__/  is possible, when it SHOULD be:
 __    __    __
/# \__/  \__/# \  A 1 hex wide wall in the first case would be
\__/  \__/  \__/  skipped entirely.
   \__/  \__/   

Now my, (correct, but tougher) algorithm:

OK, first FASA rules: "The LOS is checked by laying a straightedge
... from the center of the attackers's hex to the center of the
target's hex.  Any hex that the straightedge crosses is in the LOS.
If the straightedge passes directly between two hexes, the defender
chooses which hex ist passes through." -- Compendium, p21

It is fairly easy to convince yourself that the case of the line lying
directly between two hexes is a well-defined special case: it happens
only when the hexes lie exactly on a 30, 90, 150, 210, 270, or 330
line.  This is easy to check for, so we'll put that check in as the
first stage of the routine.  We'll assume from then on that the line
is not a special case  (which is good -- the numbers explode to
infinity for 90 degree lines).

Now we iterate as the current muse code does.  We start at the hex at
one end of the line, find the next one, and repeat until we get to the
end.  Here is the algorithm for finding the "next" hex:

First, we need only check the 6 adjacent hexes (obviously -- but the
current code ignores this and sometimes returns "next" hexes that are
not adjacent).

In addition, an eligible hex muse be "on the line."  That is, the
imaginary line drawn between the endpoints must pass through the hex.
Figuring this out is the "meat" of this algorithm.  The best way to
determine this (that I have discovered, anyway) is to find the
(absolute, cartesian, floating point) distance between the center
point of the hex in question and the line (i.e. the length of a line
drawn from the center of hex, perpendicular to the LOS line). If this
distance is less than the "effective radius" of the hex, then the line
lies inside the hex.  By "effective radius" I mean the width of the
hex along a line perpendicular to the LOS line.  This will vary
(depending on the angle of the line) between 0.5 (if the hex is lying
flat along the line -- i.e. a line pointing at 30, 90, 150, 210, 270,
or 330 degrees) and (1/sqrt(3)) (if the line passes only through the
point of the hex -- i.e. a line pointing at 0, 60, 120, 180, 240, or
300)

  _Effective_Radius_
  _____        _____
 /     \ |    /     \   /     (Hope this diagram makes sense!
/       \|   /   *_  \ /       the *'s are the line representing
\       /|   \     * //        the effective radius)
 \_____/ |    \_____// 
    *****|          /

(1/sqrt(3))     0.5

For other angles, the effective radius varies as a cosine.  Since the
whole thing is periodic with intervals of 60 deg, we can "modulo" it
by subtracting 60 deg until we get the angle to within a -30 < angle <
30 degrees region and then taking the cosine.

Hoof!  That's the worst of it.  Now, from among the adjacent, eligible
hexes, the "next" one is the one that is closest "along the line" to
the one we are in. (But is still closer to the endpoint than the
current hex.  We have to check here to make sure we don't hop back
into the hex we came from!)

Now performance issues:

The main calculations here are the distance-to-line determination, and
the effective radius calculation.  Each of these must be performed 5
times (6 adjacent hexes, but we don't have to check the one we came
from).  Distance-to-line is best found by using a matrix to rotate the
coordinates to a frame where the line is horizontal and taking the
difference in y coords between the hex and any point on the line (one
of the endpoints will do).  The angle to rotate is -(slope of the LOS
line), which is -atan(y1-y2/x1-x2).  The atan function need only be
called once, since the slope of the line is unchanging.  That leaves
us with the matrix:   |cos(u) -sin(u)|
                      |sin(u)  cos(u)|   where u=-atan(y1-y2/x1-x2)

This contains four more trigonometric functions, but again each need
only be called once (and there are really only two anyway, sin(u) and
cos(u)).

The effective radius involves a cos() function.  Again, though, since
the slope of the line is unchanging, the effective radius of the hex
is a constant throughout the iterations, and need only be calculated
once.

So now we have one time costs:

*Calculate matrix coefficients:  1 atan(), 1 cos(), 1 sin()
                                 2 fp subtractions, 1 fp divide.

*Find effective radius:          1 cos(),
                                <6 fp subtractions (to get -30<u<30)

And per-hex-travelled costs:

*Test 5 hexes if on line:       10 fp mults, 5 fp additions
                                 5 fp compares

*Test for closest forward hex:  <5 fp compares

This is actually, I would think, not much more than the current LOS
code, which makes many calls to RealCoordToMapCoord() (a costly
function) and is generally messy, with lots of repeated mults by
constants such as ZSCALE (which would optimize out, if we did, but we
don't :)

**********************************************************************

OK, enough of this.  Let's get on to the code.  
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mech.h"
#include "btmacros.h"

#define DEG60 1.0471976			/* In radians, of course ;) */
#define DEG30 0.5235988
#define ROOT3 1.7320508
#define TRACESCALEMAP 1

typedef enum { N, NE, SE, S, SW, NW } hexdir;

#define Store_Hex(store_x,store_y) \
do { found_coords[found_count].x = store_x; \
     found_coords[found_count++].y = store_y; } \
while (0)

#define Store_BestOf(x1,y1,x2,y2) \
if ( ((x1) < 0) || ((x1) >= map->width) || \
     ((y1) < 0) || ((y1) >= map->height) ) \
    { Store_Hex((x2), (y2)); } \
else if ( ((x2) < 0) || ((x2) >= map->width) || \
          ((y2) < 0) || ((y2) >= map->height) ) \
       { Store_Hex((x1), (y1)); }\
else if ((Elevation(map,(x1),(y1))) > (Elevation(map, (x2), (y2)))) \
    { Store_Hex((x1),(y1)); } \
else { Store_Hex((x2),(y2)); }

#define LOS_MapCoordToRealCoord(x,y,cx,cy) \
do { cx = (float)x * ROOT3 / 2 * TRACESCALEMAP; \
  cy = ((float)y - 0.5*(x%2))*TRACESCALEMAP;} while (0)

static void GetAdjHex(int currx, int curry, int nexthex, int *x, int *y)
{
	switch (nexthex) {
	case N:
		*x = currx;
		*y = curry - 1;
		break;
	case NE:
		*x = currx + 1;
		*y = curry - (currx % 2);
		break;
	case SE:
		*x = currx + 1;
		*y = curry - (currx % 2) + 1;
		break;
	case S:
		*x = currx;
		*y = curry + 1;
		break;
	case SW:
		*x = currx - 1;
		*y = curry - (currx % 2) + 1;
		break;
	case NW:
		*x = currx - 1;
		*y = curry - (currx % 2);
		break;
	default:					/* Mostly there to satisfy gcc */
		fprintf(stderr, "XXX ARGH: TraceLos doesn't know where to go!\n");
		*x = currx + 1;			/* Just grab some values that aren't x/y */
		*y = curry + 1;			/* so we can break out of the loop */
	}
}

int TraceLOS(MAP * map, int ax, int ay, int bx, int by,
			 lostrace_info ** result)
{

	int i;						/* Generic counter */
	float acx, acy, bcx, bcy;	/* endpoints CARTESIAN coords */
	float currcx, currcy;		/* current hex CARTESIAN coords */
	int currx, curry;			/* current hex being worked from */
	int nextx, nexty;			/* x & y coords of next hex */
	int bestx = 0, besty = 0;	/* best found so far */
	int lastx, lasty;			/* Holding place for final intervening hex */
	int xmul, ymul;				/* Used in 30/150/210/330 special case */
	hexdir nexthex;				/* potential next hex being examined */
	float nextcx, nextcy;		/* Next hex's CARTESIAN coords */
	float slope;				/* slope of line */
	float uangle;				/* angle of line (in STD CARTESIAN FORM!) */
	float sinu;					/* sin of -uangle */
	float cosu;					/* cos of same */
	float liney;				/* TRANSFORMED y coord of the line */
	float tempangle;			/* temporary uangle used for effrad calc */
	float effrad;				/* effective radius of hex */
	float currdist;				/* distance along line of current hex */
	float nextdist;				/* distance along the line of potential hex */
	float bestdist;				/* "best" (not furthest) distance tried */
	float enddist;				/* distance along at end of line */
	static lostrace_info found_coords[4000];
	int found_count = 0;

	/* Before doing anything, let's check for special circumstances, this */
	/* means vertical lines (which work using the current code, but depend */
	/* on atan returning proper vaules for atan(-Inf) -- which is probably */
	/* slow and may break on non-ANSI systems; and also lines at 30, 90 */
	/* etc.. degrees which contain 'ties' between hexes.  FASA rules here */
	/* say that the 'best' hex for the defender (the one that breaks LOS, */
	/* or gives a greater BTH penalty) should be used. */

	/* THE base case */
	if((ax == bx) && (ay == by)) {
		Store_Hex(bx, by);
		*result = found_coords;
		return found_count;
	}
	/* Is it vertical? */
	if(ax == bx) {
		if(ay > by)
			for(i = ay - 1; i > by; i--)
				Store_Hex(ax, i);
		else
			for(i = ay + 1; i < by; i++)
				Store_Hex(ax, i);
		Store_Hex(bx, by);
		*result = found_coords;
		return found_count;
	}

	/* Does it lie along a 90 degree 'tie' direction? */
	/* IF(even-number-of-cols apart AND same-y-coord) */
	if(!((bx - ax) % 2) && ay == by) {
		currx = ax;
		i = bx > ax ? 1 : -1;
		while (currx != bx) {
			/* Do best of (currx+1,by-currx%2)   */
			/*         or (currx+1,by-currx%2+1) */
			Store_BestOf(currx + 1 * i, by - currx % 2, currx + 1 * i,
						 by - currx % 2 + 1);

			if(currx != bx)
				Store_Hex(currx + 2 * i, by);

			currx += 2 * i;
		}

/* 	Store_Hex(bx,by); */
		*result = found_coords;
		return found_count;
	}

	/* Does it lie along a 30, 150, 210, 330 degree 'tie' direction? */
	/* This expression is messy, but it just means that a hex is along */
	/* 30 degrees if the y distance is (the integer part of) 3/2 */
	/* times the x distance, plus 1 if the x difference is odd, AND */
	/* the original hex was in an even column and heads in the +y  */
	/* direction, or odd and goes -y.  It works, try it :) */
	if(abs(by - ay) ==
	   (3 * abs(bx - ax) / 2) + abs((bx - ax) % 2) * abs((by <
														  ay) ? (ax %
																 2) : (1 -
																	   ax %
																	   2))) {

		/* First get the x and y 'multipliers' -- either 1 or -1 */
		/* they determine the direction of the movement */
		if(bx > ax)
			if(by > ay) {
				xmul = 1;
				ymul = 1;
			} else {
				xmul = 1;
				ymul = -1;
		} else if(by > ay) {
			xmul = -1;
			ymul = 1;
		} else {
			xmul = -1;
			ymul = -1;
		}

		currx = ax;
		curry = ay;
		while ((currx != bx) && (curry != by)) {

			Store_BestOf(currx, curry + ymul, currx + xmul,
						 ymul ==
						 1 ? curry + 1 - currx % 2 : curry - currx % 2);

			curry += (ymul == 1) ? (2 - currx % 2) : (-1 - currx % 2);
			currx += xmul;

			if(currx == bx && curry == by)
				continue;

			Store_Hex(currx, curry);
		}
		Store_Hex(currx, curry);
		*result = found_coords;
		return found_count;
	}

/****************************************************************************/

/****  OK, now we know it's not a special case ******************************/

/****************************************************************************/

/* First get the necessary constants set up */

	LOS_MapCoordToRealCoord(ax, ay, acx, acy);
	LOS_MapCoordToRealCoord(bx, by, bcx, bcy);

	slope = (float) (acy - bcy) / (float) (acx - bcx);

	uangle = -atan(slope);

	sinu = sin(uangle);
	cosu = cos(uangle);

	liney = acx * sinu + acy * cosu;	/* we could just as */
	/* correctly use bx, by */

	enddist = bcx * cosu - bcy * sinu;

	tempangle = fabs(uangle);
	while (tempangle > DEG60)
		tempangle -= DEG60;
	effrad = cos(tempangle - DEG30) * TRACESCALEMAP / ROOT3;

	/*****************************************************************/

	/**  OK, setup over, here's the loop:                            */

	/*****************************************************************/

	currx = ax;
	curry = ay;
	LOS_MapCoordToRealCoord(currx, curry, currcx, currcy);
	currdist = currcx * cosu - currcy * sinu;
	bestdist = enddist;			/* set this to the endpoint, the worst */
	/* possible point to go to  */

	while (!(currx == bx && curry == by)) {

		for(nexthex = N; nexthex <= NW; nexthex++) {

			GetAdjHex(currx, curry, nexthex, &nextx, &nexty);
			LOS_MapCoordToRealCoord(nextx, nexty, nextcx, nextcy);

			/* Is it on the line? */
			if(fabs((nextcx * sinu + nextcy * cosu) - liney) > effrad)
				continue;

			/* Where is it?  Find the transformed x coord */
			nextdist = nextcx * cosu - nextcy * sinu;

			/* is it forward of the current hex? */
			if(fabs(enddist - nextdist) > fabs(enddist - currdist))
				continue;

			/* Is it better than what we have? */
			if(fabs(enddist - nextdist) >= fabs(enddist - bestdist)) {
				bestdist = nextdist;
				bestx = nextx;
				besty = nexty;
			}
		}

		if(bestx == bx && besty == by) {	/* If we've found the last hex, record */
			lastx = currx;		/* the current hex as the last */
			lasty = curry;		/* intervening hex, save currx/y, */
			currx = bestx;		/* and jump to the end of the loop */
			curry = besty;
			continue;
		}

		/* ********************************************************* */
		/* HERE is where you put the test code for intervening hexes */
		/* ********************************************************* */
		Store_Hex(bestx, besty);
		/* ********************************************************* */

		currx = bestx;			/* Reset the curr hex for the next iteration */
		curry = besty;
		currdist = bestdist;
		bestdist = enddist;		/* reset to worst possible value */

	}

	/* **************************************************** */
	/* HERE is where you put the test code for the LAST hex */
	/* **************************************************** */
	Store_Hex(currx, curry);
	/* ********************************************************* */
	*result = found_coords;
	return found_count;
}
