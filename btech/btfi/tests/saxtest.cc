#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>

extern "C" {

void
die(const char *cause)
{
	fprintf(stderr, "%s\n", cause);
	exit(EXIT_FAILURE);
}

void write_test(const char *);
void read_test(const char *);

} // extern "C"

int
main()
{
	const char *const TEST_FILE = "saxtest.finf";

	write_test(TEST_FILE);
	read_test(TEST_FILE);

	if (remove(TEST_FILE) != 0) {
		die("remove");
	}

	return 0;
}
