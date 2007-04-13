#include "externs.h"

/**
 * Copy buffers, watching for overflows.
 */
int
safe_copy_str(char *src, char *buff, char **bufp, int max)
{
	int len = 0;

	char *tp;

	if (!src)
		return 0;

	tp = *bufp;
	while (*src && len++ < max)
		*tp++ = *src++;
	*bufp = tp;

	return len;
}
