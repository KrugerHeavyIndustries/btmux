/*
 * Test for the ASN.1 DER module.
 */

#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>

#include "fi/asn1der.h"

#define TEST_FILE "asn1der.test"

static void
die(const char *cause)
{
	perror(cause);
	exit(EXIT_FAILURE);
}

int
main(void)
{
	FILE *fp;

	/* Test writing.  */
	if (!(fp = fopen(TEST_FILE, "wb"))) {
		die("fopen");
	}

	if (fclose(fp) != 0) {
		die("fclose");
	}

	/* Test reading.  */
	if (!(fp = fopen(TEST_FILE, "rb"))) {
		die("fopen");
	}

	if (fclose(fp) != 0) {
		die("fclose");
	}

	remove(TEST_FILE);

	exit(EXIT_SUCCESS);
}
