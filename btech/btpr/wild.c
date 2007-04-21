#include "copyright.h"
#include "config.h"

#include "externs.h"

#define FIXCASE(a) (ToLower(a))
#define EQUAL(a,b) ((a == b) || (FIXCASE(a) == FIXCASE(b)))
#define NOTEQUAL(a,b) ((a != b) && (FIXCASE(a) != FIXCASE(b)))

/**
 * Do a wildcard match, without remembering the wild data.
 * This routine will cause crashes if fed NULLs instead of strings.
 */
int
quick_wild(char *tstr, char *dstr)
{
	while (*tstr != '*') {
		switch (*tstr) {
		case '?':
			/*
			 * Single character match.  Return false if at end of
			 * data.
			 */
			if(!*dstr)
				return 0;
			break;
		case '\\':
			/*
			 * Escape character.  Move up, and force literal match
			 * of next character.
			 */
			tstr++;
			/*
			 * FALL THROUGH 
			 */
		default:
			/*
			 * Literal character.  Check for a match.  If matching
			 * end of data, return true. 
			 */
			if(NOTEQUAL(*dstr, *tstr))
				return 0;
			if(!*dstr)
				return 1;
		}
		tstr++;
		dstr++;
	}

	/*
	 * Skip over '*'. 
	 */

	tstr++;

	/*
	 * Return true on trailing '*'. 
	 */

	if(!*tstr)
		return 1;

	/*
	 * Skip over wildcards. 
	 */

	while ((*tstr == '?') || (*tstr == '*')) {
		if(*tstr == '?') {
			if(!*dstr)
				return 0;
			dstr++;
		}
		tstr++;
	}

	/*
	 * Skip over a backslash in the pattern string if it is there. 
	 */

	if(*tstr == '\\')
		tstr++;

	/*
	 * Return true on trailing '*'. 
	 */

	if(!*tstr)
		return 1;

	/*
	 * Scan for possible matches. 
	 */

	while (*dstr) {
		if(EQUAL(*dstr, *tstr) && quick_wild(tstr + 1, dstr + 1))
			return 1;
		dstr++;
	}
	return 0;
}
