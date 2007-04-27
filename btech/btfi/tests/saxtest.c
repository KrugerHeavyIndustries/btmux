#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "sax.h"

#define TEST_FILE "saxtest.test"

static FI_Generator *gen;
static FI_ContentHandler *handler;

static void
die(const char *cause)
{
	fprintf(stderr, "%s\n", cause);
	exit(EXIT_FAILURE);
}

static void
die_gen(const char *cause)
{
	const FI_ErrorInfo *ei = fi_get_generator_error(gen);

	fprintf(stderr, "%s: %s\n",
	        cause, ei->error_string ? ei->error_string : "???");
	exit(EXIT_FAILURE);
}

static FI_Attributes *attributes;

static void
start_element(FI_VocabIndex name, ...)
{
	va_list ap;

	FI_Name e_name, a_name;
	FI_Value a_value;

	/* Collect attribute values.  */
	fi_clear_attributes(attributes);

	//a_name.type = FI_NAME_SURROGATE;
	//a_name.as.surrogate.prefix_idx = FI_VOCAB_INDEX_NULL;
	//a_name.as.surrogate.namespace_idx = 2; /* FIXME: get namespace index */

	va_start(ap, name);

	while (0) { //(a_name.as.surrogate.local_idx = va_arg(ap, FI_VocabIndex))) {
		//a_value = va_arg(ap, const char *)
		if (!fi_add_attribute(attributes, &a_name, &a_value)) {
			die("fi_add_attribute");
		}
	}

	va_end(ap);

	/* Begin element.  */
	//e_name.type = FI_NAME_SURROGATE;
	//e_name.as.surrogate.prefix_idx = FI_VOCAB_INDEX_NULL;
	//e_name.as.surrogate.namespace_idx = 2; /* FIXME: get namespace index */
	//e_name.as.surrogate.local_idx = name;

	if (!handler->startElement(handler, &e_name, attributes)) {
		die_gen("generate::startElement");
	}
}

static void
end_element(FI_VocabIndex name)
{
	FI_Name e_name;

	/* End element.  */
	//e_name.type = FI_NAME_SURROGATE;
	//e_name.as.surrogate.prefix_idx = FI_VOCAB_INDEX_NULL;
	//e_name.as.surrogate.namespace_idx = 2; /* FIXME: get namespace index */
	//e_name.as.surrogate.local_idx = name;

	if (!handler->endElement(handler, &e_name)) {
		die_gen("generate::endElement");
	}
}

static void
write_test(void)
{
	FI_VocabIndex en_how, en_now, an_brown, an_cow, an_now;

	/* Set up.  */
	gen = fi_create_generator();
	if (!gen) {
		die("fi_create_generator");
	}

	attributes = fi_create_attributes();
	if (!attributes) {
		die("fi_create_attributes");
	}

	handler = fi_getContentHandler(gen);
	if (!handler) {
		die_gen("fi_getContentHandler");
	}

	if (!fi_generate(gen, TEST_FILE)) {
		die_gen("fi_generate");
	}

	/* Add initial vocabulary entries.  */
	en_how = fi_add_element_name(gen, "how");
	en_now = fi_add_element_name(gen, "now");

	if (en_how == FI_VOCAB_INDEX_NULL || en_now == FI_VOCAB_INDEX_NULL) {
		die_gen("fi_add_element_name");
	}

	an_brown = fi_add_attribute_name(gen, "brown");
	an_cow = fi_add_attribute_name(gen, "cow");
	an_now = fi_add_attribute_name(gen, "now");

	if (an_brown == FI_VOCAB_INDEX_NULL
	    || an_cow == FI_VOCAB_INDEX_NULL
	    || an_now == FI_VOCAB_INDEX_NULL) {
		die_gen("fi_add_attribute_name");
	}

	/*
	 * Write this document as a Fast Infoset:
	 *
	 * <?xml encoding='utf-8'?>
	 * <how cow='moo'>
	 *     <now brown='1' now='3.14'>
	 *         <how brown='oops' />
	 *     </now>
	 *     <now cow='moo' />
	 * </how>
	 */
	if (!handler->startDocument(handler)) {
		die_gen("generate::startDocument");
	}

	start_element(en_how, an_cow, "moo", 0);

		start_element(en_now, an_brown, "1", an_now, "3.14", 0);

			start_element(en_how, an_brown, "oops", 0);
			end_element(en_how);

		end_element(en_now);

		start_element(en_now, an_now, "cow", 0);
		end_element(en_now);

	end_element(en_how);

	if (!handler->endDocument(handler)) {
		die_gen("generate::endDocument");
	}

	/* Clean up.  */
	fi_destroy_attributes(attributes);
	fi_destroy_generator(gen);
}

int
real_main(void)
{
	write_test();

	exit(EXIT_SUCCESS);
}
