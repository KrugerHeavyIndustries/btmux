/*
 * A SAX2-style event-based API for Fast Infoset.  This only implements enough
 * as is useful for parsing/generating BattleTech database infosets.
 *
 * And yes, SAX2 is technically just a parser API, but we model our generating
 * API on the same sorts of ideas, run in reverse.
 */

#ifndef BTECH_FI_SAX_H
#define BTECH_FI_SAX_H

#include <stddef.h>

#include "common.h"
#include "generic.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Attributes.
 */

typedef struct FI_tag_Attributes FI_Attributes;

FI_Attributes *fi_create_attributes(void);
void fi_destroy_attributes(FI_Attributes *attrs);

void fi_clear_attributes(FI_Attributes *attrs);
int fi_add_attribute(FI_Attributes *attrs,
                     const FI_Name *name, const FI_Value *value);

int fi_get_attributes_length(const FI_Attributes *attrs);
const FI_Name *fi_get_attribute_name(const FI_Attributes *attrs, int idx);
const FI_Value *fi_get_attribute_value(const FI_Attributes *attrs, int idx);

/*
 * Event handlers.
 */

typedef struct FI_tag_ContentHandler FI_ContentHandler;

struct FI_tag_ContentHandler {
	/* Document start/stop events.  */
	int (*startDocument)(FI_ContentHandler *handler);
	int (*endDocument)(FI_ContentHandler *handler);

	/* Element start/stop events.  */
	int (*startElement)(FI_ContentHandler *handler,
	                    const FI_Name *name, const FI_Attributes *attrs);
	int (*endElement)(FI_ContentHandler *handler, const FI_Name *name);

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

FI_VocabIndex fi_add_element_name(FI_Generator *gen, const char *name);
FI_VocabIndex fi_add_attribute_name(FI_Generator *gen, const char *name);

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
