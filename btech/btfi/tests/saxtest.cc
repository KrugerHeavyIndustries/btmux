#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>

#include "Vocabulary.hh"

extern "C" {

void
die(const char *cause)
{
	fprintf(stderr, "%s\n", cause);
	exit(EXIT_FAILURE);
}

void write_test(FI_Vocabulary *, const char *);
void read_test(FI_Vocabulary *, const char *);

} // extern "C"

int
main()
{
	const char *const TEST_FILE = "saxtest.finf";
	const char *const THIRD_PARTY_FILE = "tests/thirdparty.finf";

	FI_Vocabulary *vocab = fi_create_vocabulary();

	write_test(vocab, TEST_FILE);
	read_test(vocab, TEST_FILE);

	if (remove(TEST_FILE) != 0) {
		die("remove");
	}

	read_test(vocab, THIRD_PARTY_FILE);

	fi_destroy_vocabulary(vocab);

	return 0;
}
