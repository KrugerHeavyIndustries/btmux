/*
 * Module implementing the SAX2-style parser/generator API.
 */

#include "autoconf.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "stream.h"

#include "Document.hh"

#include "sax.h"


/*
 * Generator API.
 */

struct FI_tag_Generator {
	FI_ErrorInfo error_info;

	FI_ContentHandler content_handler;

	FILE *fpout;
}; /* FI_Generator */

static int gen_ch_startDocument(FI_ContentHandler *);
static int gen_ch_endDocument(FI_ContentHandler *);

FI_Generator *
fi_create_generator(void)
{
	FI_Generator *new_gen;

	new_gen = (FI_Generator *)malloc(sizeof(FI_Generator));
	if (!new_gen) {
		return NULL;
	}

	FI_CLEAR_ERROR(new_gen->error_info);

	new_gen->content_handler.startDocument = gen_ch_startDocument;
	new_gen->content_handler.endDocument = gen_ch_endDocument;

	new_gen->fpout = NULL;

	return new_gen;
}

void
fi_destroy_generator(FI_Generator *gen)
{
	if (gen->fpout) {
		/* XXX: Don't care, never finished document.  */
		fclose(gen->fpout);
	}

	free(gen);
}

const FI_ErrorInfo *
fi_get_generator_error(const FI_Generator *gen)
{
	return &gen->error_info;
}

FI_ContentHandler *
fi_getContentHandler(FI_Generator *gen)
{
	return &gen->content_handler;
}

int
fi_generate(FI_Generator *gen, const char *filename)
{
	/* Open stream.  */
	if (gen->fpout) {
		/* TODO: Warn about incomplete document? */
		fclose(gen->fpout);
	}

	gen->fpout = fopen(filename, "wb");
	if (!gen->fpout) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_NOFILE);
		return 0;
	}

	/* Wait for generator events.  */
	return 1;
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

const FI_ErrorInfo *
fi_get_parser_error(const FI_Parser *parser)
{
	return &parser->error_info;
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


/*
 * Generator subroutines.
 */

/* A fancy macro to get the corresponding FI_Generator object.  */
#define GEN_HANDLER_OFFSET (offsetof(FI_Generator, content_handler))
#define GET_GEN(h) ((FI_Generator *)((char *)(h) - GEN_HANDLER_OFFSET))

/* Helper routine to write out to buffers.  */
static int
write_buffer(FILE *fpout)
{
}

static int
gen_ch_startDocument(FI_ContentHandler *handler)
{
	FI_Generator *gen = GET_GEN(handler);

	/* Sanity checks.  */
	if (!gen->fpout) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_INVAL);
		return 0;
	}

	/* Write document header.  */

	return 1;
}

static int
gen_ch_endDocument(FI_ContentHandler *handler)
{
	FI_Generator *gen = GET_GEN(handler);

	/* Sanity checks.  */
	if (!gen->fpout) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_INVAL);
		return 0;
	}

	/* Write document trailer.  */

	/* Close out file.  */
	if (fclose(gen->fpout) != 0) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_ERRNO);
		gen->fpout = NULL;
		return 0;
	}

	gen->fpout = NULL;
	return 1;
}


/*
 * Parser subroutines.
 */
