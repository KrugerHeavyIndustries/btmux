#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "sax.h"

extern void die(const char *);


static FI_Generator *gen;
static FI_ContentHandler *handler;

static void
die_gen(const char *cause)
{
	const FI_ErrorInfo *ei = fi_get_generator_error(gen);

	fprintf(stderr, "%s: %s\n",
	        cause, ei->error_string ? ei->error_string : "???");
	exit(EXIT_FAILURE);
}

static FI_Value *a_value;
static FI_Attributes *attributes;

static void
start_element(const FI_Name *e_name, ...)
{
	const FI_Name *a_name;

	va_list ap;

	/* Collect attribute values.  */
	fi_clear_attributes(attributes);

	va_start(ap, e_name);

	while ((a_name = va_arg(ap, const FI_Name *))) {
		const char *const a_buf = va_arg(ap, const char *);

		if (!fi_set_value(a_value,
		                  FI_VALUE_AS_UTF8, strlen(a_buf), a_buf)) {
			die("fi_set_value");
		}

		if (!fi_add_attribute(attributes, a_name, a_value)) {
			die("fi_add_attribute");
		}
	}

	va_end(ap);

	/* Begin element.  */
	if (!handler->startElement(handler, e_name, attributes)) {
		die_gen("generate::startElement");
	}
}

static void
end_element(const FI_Name *e_name)
{
	/* End element.  */
	if (!handler->endElement(handler, e_name)) {
		die_gen("generate::endElement");
	}
}

static void
characters(const char *text)
{
	/* Characters.  */
	if (!fi_set_value(a_value, FI_VALUE_AS_UTF8, strlen(text), text)) {
		die("fi_set_value");
	}

	if (!handler->characters(handler, a_value)) {
		die_gen("generate::characters");
	}
}

void
write_test(const char *const TEST_FILE)
{
	FI_Name *en_how, *en_now;
	FI_Name *an_brown, *an_cow, *an_now;

	/* Set up.  */
	gen = fi_create_generator();
	if (!gen) {
		die("fi_create_generator");
	}

	a_value = fi_create_value();
	if (!a_value) {
		die("fi_create_value");
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

	/* Get all our FI_Name handles.  */
	en_how = fi_create_element_name(gen, "how");
	en_now = fi_create_element_name(gen, "now");

	if (!en_how || !en_now) {
		die_gen("fi_create_element_name");
	}

	an_brown = fi_create_attribute_name(gen, "brown");
	an_cow = fi_create_attribute_name(gen, "cow");
	an_now = fi_create_attribute_name(gen, "now");

	if (!an_brown || !an_cow || !an_now) {
		die_gen("fi_create_attribute_name");
	}

	/*
	 * Write this document as a Fast Infoset:
	 *
	 * <?xml encoding='utf-8'?>
	 * <how xmlns="http://btonline-btech.sourceforge.net" cow='moo'>
	 *     <now brown='1' now='3.14'>
	 *         <how brown='oops' />
	 *     </now>
	 *     <now now='cow' cow='' brown='moo' />
	 * </how>
	 */
	if (!handler->startDocument(handler)) {
		die_gen("generate::startDocument");
	}

	start_element(en_how, an_cow, "moo", 0);

	characters("\n\t");

		start_element(en_now, an_brown, "1", an_now, "3.14", 0);

	characters("\n\t\t");

			start_element(en_how, an_brown, "oops", 0);
			end_element(en_how);

	characters("\n\t");

		end_element(en_now);

	characters("\n\t");

		start_element(en_now, an_now, "cow", an_cow, "",
		              an_brown, "moo", 0);
		end_element(en_now);

	characters("\n");

	end_element(en_how);

	if (!handler->endDocument(handler)) {
		die_gen("generate::endDocument");
	}

	/* Clean up.  */
	fi_destroy_name(en_how);
	fi_destroy_name(en_now);

	fi_destroy_name(an_brown);
	fi_destroy_name(an_cow);
	fi_destroy_name(an_now);

	fi_destroy_attributes(attributes);
	fi_destroy_value(a_value);

	fi_destroy_generator(gen);
}
