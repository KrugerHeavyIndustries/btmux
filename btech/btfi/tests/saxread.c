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
	puts("(END DOCUMENT)");
	return 1;
}

static int is_open = 0;

static int level = 0;

static void
print_indent(void)
{
	int ii;

	for (ii = 0; ii < level; ii++) {
		putchar('\t');
	}
}

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
print_value(const FI_Value *value)
{
	switch (fi_get_value_type(value)) {
	case FI_VALUE_AS_NULL:
		fputs("\"\"", stdout);
		break;

	case FI_VALUE_AS_OCTETS:
		// Not quite right...
		printf("\"%.*s\"",
		       fi_get_value_count(value),
		       (const char *)fi_get_value(value));
		break;

	default:
		printf("(unknown type #%d)", fi_get_value_type(value));
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
		fputs(">\n", stdout);
	}

	print_indent();
	level++;

	putchar('<');
	print_name(name);

	max_ii = fi_get_attributes_length(attrs);
	for (ii = 0; ii < max_ii; ii++) {
		putchar(' ');
		print_name(fi_get_attribute_name(attrs, ii));
		putchar('=');
		print_value(fi_get_attribute_value(attrs, ii));
	}

	is_open = 1;
	return 1;
}

static int
endElement(FI_ContentHandler *handler, const FI_Name *name)
{
	level--;

	if (is_open) {
		fputs(" />\n", stdout);
		is_open = 0;
	} else {
		print_indent();

		fputs("</", stdout);
		print_name(name);
		fputs(">\n", stdout);
	}

	return 1;
}

static int
characters(FI_ContentHandler *handler, const FI_Octet *ch, int length)
{
	/* TODO: Not implemented.  */
	die("FI_ContentHandler::characters");
	return 0;
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
	 *     <now brown='1' now='3.14'>
	 *         <how brown='oops' />
	 *     </now>
	 *     <now now='cow' cow='' />
	 * </how>
	 */
	if (!fi_parse(parser, TEST_FILE)) {
		die_parser("fi_parse");
	}

	/* Clean up.  */
	fi_destroy_parser(parser);
}
