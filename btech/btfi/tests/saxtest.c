#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>

#include "sax.h"

#define TEST_FILE "saxtest.test"

static void
die_fi(const char *cause, const FI_ErrorInfo *ei)
{
	fprintf(stderr, "%s: %s\n",
	        cause, ei->error_string ? ei->error_string : "???");
	exit(EXIT_FAILURE);
}

static void
write_test(void)
{
	FI_Generator *gen;
	FI_ContentHandler *handler;

	/* Set up.  */
	gen = fi_create_generator();
	if (!gen) {
		die_fi("fi_create_generator", NULL);
	}

	handler = fi_getContentHandler(gen);
	if (!handler) {
		die_fi("fi_getContentHandler", fi_get_generator_error(gen));
	}

	if (!fi_generate(gen, TEST_FILE)) {
		die_fi("fi_generate", fi_get_generator_error(gen));
	}

	/* Write document.  */
	if (!handler->startDocument(handler)) {
		die_fi("generate::startDocument", fi_get_generator_error(gen));
	}

	if (!handler->endDocument(handler)) {
		die_fi("generate::endDocument", fi_get_generator_error(gen));
	}

	/* Clean up.  */
	fi_destroy_generator(gen);
}

int
real_main(void)
{
	write_test();

	exit(EXIT_SUCCESS);
}
