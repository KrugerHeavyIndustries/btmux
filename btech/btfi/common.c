/*
 * Common, shared definitions.
 */

#include "autoconf.h"

#include "common.h"


/* Some constant error strings.  */
const char *const fi_error_strings[] = {
	"No error",			/* No error set */
	"Unknown error",		/* Catch-all code; generally a bug */
	"Unsupported operation",	/* Unimplemented operation */
	"Out of memory",		/* Couldn't allocate required memory */
	"End of stream",		/* Reached end of FI_OctetStream */
	"File not found",		/* File not found */
	"Invalid argument",		/* Bad argument to function call */
	"Illegal state",		/* Unexpected state reached */
	"Check errno",			/* Error information is in errno */
	"Caught BTech::FI::Exception"	/* Error resulted from an Exception */
}; /* fi_error_strings */
