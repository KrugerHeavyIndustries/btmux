#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "sax.h"

extern void die(const char *);


static FI_Parser *parser;
static FI_ContentHandler handler;

static void
die_parser(const char *cause)
{
	const FI_ErrorInfo *ei = fi_get_parser_error(parser);

	fprintf(stderr, "%s: %s\n",
	        cause, ei->error_string ? ei->error_string : "???");
	exit(EXIT_FAILURE);
}

/*
 * Content handlers.
 */

static int
startDocument(FI_ContentHandler *handler)
{
	puts("(START DOCUMENT)");
	return 1;
}

static int
endDocument(FI_ContentHandler *handler)
{
	puts("\n(END DOCUMENT)");
	return 1;
}

static int is_open = 0;

static void
print_name(const FI_Name *name)
{
	const char *pfx_str = fi_get_name_prefix(name);
	/*const char *nsn_str = fi_get_name_namespace_name(name);*/
	const char *local_str = fi_get_name_local_name(name);

	if (pfx_str) {
		fputs(pfx_str, stdout);
		putchar(':');
	}

	fputs(local_str, stdout);
}

static void
print_value_content(const FI_Value *value)
{
	const size_t count = fi_get_value_count(value);
	const FI_ValueType type = fi_get_value_type(value);
	const char *data_ptr = (const char *)fi_get_value(value);

	size_t ii;

	switch (type) {
	case FI_VALUE_AS_NULL:
		die("print_value_content: Unexpected NULL value");
		break;

	case FI_VALUE_AS_UTF8:
		printf("%.*s", count, data_ptr);
		break;

	case FI_VALUE_AS_OCTETS:
		fputs("[Base64(", stdout);

		for (ii = 0; ii < count; ii++) {
			if (ii != 0) {
				putchar(' ');
			}

			printf("%02X", ((FI_Octet *)data_ptr)[ii]);
		}

		fputs(")]", stdout);
		break;

	default:
		for (ii = 0; ii < count; ii++) {
			if (ii != 0) {
				fputs(", ", stdout);
			}

			switch (type) {
			case FI_VALUE_AS_INT:
				printf("%d", ((FI_Int32 *)data_ptr)[ii]);
				break;

			case FI_VALUE_AS_FLOAT:
				printf("%g", ((FI_Float32 *)data_ptr)[ii]);
				break;

			default:
				printf("(unknown type #%d)", type);
			}
		}
		break;
	}
}

static void
print_attr_value(const FI_Value *value)
{
	switch (fi_get_value_type(value)) {
	case FI_VALUE_AS_NULL:
		fputs("\"\"", stdout);
		break;

	case FI_VALUE_AS_UTF8:
		putchar('"');
		print_value_content(value);
		putchar('"');
		break;

	default:
		putchar('{');
		print_value_content(value);
		putchar('}');
		break;
	}
}

static int
startElement(FI_ContentHandler *handler,
             const FI_Name *name, const FI_Attributes *attrs)
{
	int ii, max_ii;

	if (is_open) {
		/* Close the most recent element.  */
		fputs(">", stdout);
	}

	putchar('<');
	print_name(name);

	max_ii = fi_get_attributes_length(attrs);
	for (ii = 0; ii < max_ii; ii++) {
		putchar(' ');
		print_name(fi_get_attribute_name(attrs, ii));
		putchar('=');
		print_attr_value(fi_get_attribute_value(attrs, ii));
	}

	is_open = 1;
	return 1;
}

static int
endElement(FI_ContentHandler *handler, const FI_Name *name)
{
	if (is_open) {
		fputs(" />", stdout);
		is_open = 0;
	} else {
		fputs("</", stdout);
		print_name(name);
		fputs(">", stdout);
	}

	return 1;
}

static int
characters(FI_ContentHandler *handler, const FI_Value *value)
{
	if (is_open) {
		/* Close the most recent element.  */
		fputs(">", stdout);
		is_open = 0;
	}

#if 0
	fputs("<![CDATA[", stdout);
#endif // 0

	if (fi_get_value_type(value) == FI_VALUE_AS_NULL) {
		die("FI_ContentHandler::characters: empty CDATA");
	} else {
		print_value_content(value);
	}

#if 0
	fputs("]]>", stdout);
#endif // 0

	return 1;
}

void
read_test(const char *const TEST_FILE)
{
	/* Set up.  */
	parser = fi_create_parser();
	if (!parser) {
		die("fi_create_parser");
	}

	fi_setContentHandler(parser, &handler);

	handler.startDocument = startDocument;
	handler.endDocument = endDocument;

	handler.startElement = startElement;
	handler.endElement = endElement;

	handler.characters = characters;

	/*
	 * Read this document as a Fast Infoset:
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
	if (!fi_parse(parser, TEST_FILE)) {
		die_parser("fi_parse");
	}

	/* Clean up.  */
	fi_destroy_parser(parser);
}
