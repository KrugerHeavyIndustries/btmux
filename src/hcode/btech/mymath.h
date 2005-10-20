
/*
 * $Id: mymath.h,v 1.1.1.1 2005/01/11 21:18:29 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Wed Oct  9 10:49:02 1996 fingon
 * Last modified: Wed Oct  9 10:49:13 1996 fingon
 *
 */

#ifndef MYMATH_H
#define MYMATH_H

#include <math.h>

#ifdef fcos
#undef fcos
#endif
#define fcos cos
#ifdef fsin
#undef fsin
#endif
#define fsin sin
#ifdef fatan
#undef fatan
#endif
#define fatan atan
#ifdef MAX
#undef MAX
#endif
#ifdef MIN
#undef MIN
#endif
#define TWOPIOVER360 0.0174533
#define PI 3.141592654

#endif				/* MYMATH_H */
