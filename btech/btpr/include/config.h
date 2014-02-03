/* config.h */

#ifndef CONFIG_H
#define CONFIG_H

#include "copyright.h"
#include "autoconf.h"

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#ifdef STDC_HEADERS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "debug.h"

typedef long    dbref;
typedef long	FLAG;
typedef int     POWER;

/* TEST_MALLOC:	Defining this makes a malloc that keeps track of the number
 *		of blocks allocated.  Good for testing for Memory leaks.
 */

/* Compile time options */

/* #define TEST_MALLOC *//* Keep track of block allocs */

#define HASH_FACTOR		16	/* How much hashing you want. */

#ifdef BRAIN_DAMAGE		/* a kludge to get it to work on a mutant
				 * DENIX system */
#undef toupper
#endif

#ifdef TEST_MALLOC
extern int malloc_count;

#define XMALLOC(x,y) (fprintf(stderr,"Malloc: %s\n", (y)), malloc_count++, \
                    (char *)malloc((x)))
#define XFREE(x,y) (fprintf(stderr, "Free: %s\n", (y)), \
                    ((x) ? malloc_count--, free((x)), (x)=NULL : (x)))
#else
#define XMALLOC(x,y) (char *)malloc((x))
#define XFREE(x,y) (free((x)), (x) = NULL)
#endif				/* TEST_MALLOC */

#ifndef HAVE_STRNDUP
char *strndup(const char *s, size_t n);
#endif

#endif				/* CONFIG_H */
