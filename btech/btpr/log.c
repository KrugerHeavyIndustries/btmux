#include "mudconf.h"
#include "externs.h"

#include <stdio.h>
#include <string.h>

#define BROKEN fprintf(stderr, "broken: " __FILE__ ": %s()\n", __FUNCTION__)

/* Remove ANSI color code, copying to output buffer.  */
char *
strip_ansi_r(char *dest, const char *raw, size_t n)
{
	BROKEN;

	strncpy(dest, raw, n);
	dest[n - 1] = '\0';
	return dest;
}

void
log_perror(const char *primary, const char *secondary, const char *extra,
           const char *failing_object)
{
	BROKEN;

	/* Print logging header, check for recursion, etc.  */
	if (secondary && *secondary) {
		fprintf(stderr, "TIMESTAMP MUDNAME %s/%s: ",
		        primary, secondary);
	} else {
		fprintf(stderr, "TIMESTAMP MUDNAME %s: ", primary);
	}

	if(extra && *extra) {
		/* Log output, stripping ANSI codes.  */
		fprintf(stderr, "(%s) ", extra);
	}

	perror((char *) failing_object);
	fflush(stderr);
}

void
log_error(int key, char *primary, char *secondary, char *format, ...)
{
	char buffer[LBUF_SIZE];
	va_list ap;

	BROKEN;

	/* Filter log output based on key.  */

	va_start(ap, format);
	vsnprintf(buffer, LBUF_SIZE, format, ap);
	va_end(ap);

	/* Strip buffer of color codes.  */

	if (secondary && *secondary) {
		fprintf(stderr, "TIMESTAMP MUDNAME %s/%s: %s\n",
		        primary, secondary, buffer);
	} else {
		fprintf(stderr, "TIMESTAMP MUDNAME %s: %s\n", primary, buffer);
	}
}
