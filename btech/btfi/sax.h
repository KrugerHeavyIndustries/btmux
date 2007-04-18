/*
 * A SAX2-style event-based API for Fast Infoset.  This only implements enough
 * as is useful for parsing/generating BattleTech database infosets.
 *
 * And yes, SAX2 is technically just a parser API, but we model our generating
 * API on the same sorts of ideas, run in reverse.
 */

#ifndef BTECH_FI_SAX_H
#define BTECH_FI_SAX_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct FI_tag_ContentHandler FI_ContentHandler;

/* TODO: Types like Attributes, etc. should be defined elsewhere.  */
typedef struct FI_tag_Name FI_Name;
typedef struct FI_tag_Attributes FI_Attributes;

struct FI_tag_ContentHandler {
	/* Document start/stop events.  */
	int (*startDocument)(FI_ContentHandler *handler);
	int (*endDocument)(FI_ContentHandler *handler);

	/* Element start/stop events.  */
	int (*startElement)(FI_ContentHandler *handler,
	                    FI_Name *name, FI_Attributes *atts);
	int (*endElement)(FI_ContentHandler *handler, FI_Name *name);

	/* "Character" chunks. */
	int (*characters)(FI_Octet ch[], int start, int length);
}; /* FI_ContentHandler */

/*
 * Generator.
 */

typedef struct FI_tag_Generator FI_Generator;

FI_Generator *fi_create_generator(void);
void fi_destroy_generator(FI_Generator *gen);

const FI_ErrorInfo *fi_get_generator_error(const FI_Generator *gen);

FI_ContentHandler *fi_getContentHandler(FI_Generator *gen);

int fi_generate(FI_Generator *gen, const char *filename);

/*
 * Parser.
 */

typedef struct FI_tag_Parser FI_Parser;

FI_Parser *fi_create_parser(void);
void fi_destroy_parser(FI_Parser *parser);

const FI_ErrorInfo *fi_get_parser_error(const FI_Parser *parser);

void fi_setContentHandler(FI_Parser *parser, FI_ContentHandler *handler);

int fi_parse(FI_Parser *parser, const char *filename);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* !BTECH_FI_SAX_H */
