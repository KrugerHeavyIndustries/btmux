#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "sax.h"

extern void die(const char *);


static const FI_Octet DINNER[6] = { 0x0C, 0x40, 0x03, 0x04, 0x41, 0x05 };
static const FI_Int32 TERNARY[3] = { -1, 0, 1 };
static const FI_Float32 PIE = 3.14f;

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

/*
 * Variable arguments are of the form:
 *
 * (FI_Name *a_name, FI_ValueType a_type, size_t a_count, const void *a_buf)
 *
 * terminated by a_name = 0.
 */
static void
start_element(const FI_Name *e_name, ...)
{
	const FI_Name *a_name;

	va_list ap;

	/* Collect attribute values.  */
	fi_clear_attributes(attributes);

	va_start(ap, e_name);

	while ((a_name = va_arg(ap, const FI_Name *))) {
		const FI_ValueType a_type = va_arg(ap, FI_ValueType);
		const size_t a_count = va_arg(ap, size_t);
		const void *const a_buf = va_arg(ap, const void *);

		if (!fi_set_value(a_value, a_type, a_count, a_buf)) {
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
characters(FI_ValueType type, size_t count, const void *buf)
{
	/* Characters.  */
	if (!fi_set_value(a_value, type, count, buf)) {
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
	 *     <now brown={-1, 0, 1} now={3.14}>
	 *         <how brown='oops'>[Base64(DINNER)]</how>
	 *     </now>
	 *     <now now='cow' cow='' brown='moo' />
	 * </how>
	 *
	 * Note that this isn't an exhaustive test of encoding algorithms.
	 */
	if (!handler->startDocument(handler)) {
		die_gen("generate::startDocument");
	}

	start_element(en_how,
	              an_cow, FI_VALUE_AS_UTF8, 3, "moo",
	              0);

	characters(FI_VALUE_AS_UTF8, 2, "\n\t");

		start_element(en_now,
		              an_brown, FI_VALUE_AS_INT, 3, TERNARY,
		              an_now, FI_VALUE_AS_FLOAT, 1, &PIE,
		              0);

	characters(FI_VALUE_AS_UTF8, 3, "\n\t\t");

			start_element(en_how,
			              an_brown, FI_VALUE_AS_UTF8, 4, "oops",
			              0);

			characters(FI_VALUE_AS_OCTETS, 6, DINNER);

			end_element(en_how);

	characters(FI_VALUE_AS_UTF8, 2, "\n\t");

		end_element(en_now);

	characters(FI_VALUE_AS_UTF8, 2, "\n\t");

		start_element(en_now,
		              an_now, FI_VALUE_AS_UTF8, 3, "cow",
		              an_cow, FI_VALUE_AS_UTF8, 0, "",
		              an_brown, FI_VALUE_AS_UTF8, 3, "moo", 0);
		end_element(en_now);

	characters(FI_VALUE_AS_UTF8, 1, "\n");

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
