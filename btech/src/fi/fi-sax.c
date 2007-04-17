/*
 * Module implementing the SAX2-style parser/generator API.
 */

#include "autoconf.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "fi-common.h"

#include "fi-sax.h"


/*
 * Generator API.
 */

struct FI_tag_Generator {
	FI_ErrorInfo error_info;

	FI_ContentHandler content_handler;
}; /* FI_Generator */

FI_Generator *
fi_create_generator(void)
{
	FI_Generator *new_gen;

	new_gen = (FI_Generator *)malloc(sizeof(FI_Generator));
	if (!new_gen) {
		return NULL;
	}

	FI_CLEAR_ERROR(new_gen->error_info);

	return new_gen;
}

void
fi_destroy_generator(FI_Generator *gen)
{
	free(gen);
}

FI_ContentHandler *
fi_getContentHandler(FI_Generator *gen)
{
	return &gen->content_handler;
}

int
fi_generate(FI_Generator *gen, const char *filename)
{
	return 0;
}


/*
 * Parser API.
 */

struct FI_tag_Parser {
	FI_ErrorInfo error_info;

	FI_ContentHandler *content_handler;
}; /* FI_Parser */

FI_Parser *
fi_create_parser(void)
{
	FI_Parser *new_parser;

	new_parser = (FI_Parser *)malloc(sizeof(FI_Parser));
	if (!new_parser) {
		return NULL;
	}

	new_parser->content_handler = NULL;

	FI_CLEAR_ERROR(new_parser->error_info);

	return new_parser;
}

void
fi_destroy_parser(FI_Parser *parser)
{
	free(parser);
}

void
fi_setContentHandler(FI_Parser *parser, FI_ContentHandler *handler)
{
	parser->content_handler = handler;
}

int
fi_parse(FI_Parser *parser, const char *filename)
{
	FILE *fpin;

	/* Open stream.  */
	fpin = fopen(filename, "rb");
	if (!fpin) {
		/* TODO: Record error info.  */
		return 0;
	}

	/* Main parse loop.  */

	/* Close stream.  */
	fclose(fpin); /* don't care if fails, we got everything we wanted */
	return 1;
}
