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
	puts("START DOCUMENT");
	return 1;
}

static int
endDocument(FI_ContentHandler *handler)
{
	puts("END DOCUMENT");
	return 1;
}

static int
startElement(FI_ContentHandler *handler,
             const FI_Name *name, const FI_Attributes *attrs)
{
	puts("START ELEMENT");
	return 1;
}

static int
endElement(FI_ContentHandler *handler, const FI_Name *name)
{
	puts("END ELEMENT");
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
	 *     <now now='cow' />
	 * </how>
	 */
	if (!fi_parse(parser, TEST_FILE)) {
		die_parser("fi_parse");
	}

	/* Clean up.  */
	fi_destroy_parser(parser);
}
