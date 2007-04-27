/*
 * Module implementing the SAX2-style parser/generator API.
 */

#include "autoconf.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include "common.h"
#include "stream.h"

#include <new>
#include <memory>

#include "Document.hh"
#include "MutableAttributes.hh"

#include "sax.h"

using std::auto_ptr;

using namespace BTech::FI;


/*
 * Attributes API.
 */

// C-compatible wrapper around the MutableAttributes class.
struct FI_tag_Attributes {
	MutableAttributes impl;
}; // FI_Attributes

FI_Attributes *
fi_create_attributes(void)
{
	return new FI_Attributes ();
}

void
fi_destroy_attributes(FI_Attributes *attrs)
{
	delete attrs;
}

void
fi_clear_attributes(FI_Attributes *attrs)
{
	attrs->impl.clear();
}

int
fi_add_attribute(FI_Attributes *attrs,
                 const FI_Name *name, const FI_Value *value)
{
	return attrs->impl.add(name, value);
}

int
fi_get_attributes_length(const FI_Attributes *attrs)
{
	return attrs->impl.getLength();
}

const FI_Name *
fi_get_attribute_name(const FI_Attributes *attrs, int idx)
{
	return attrs->impl.getName(idx);
}

const FI_Value *
fi_get_attribute_value(const FI_Attributes *attrs, int idx)
{
	return attrs->impl.getValue(idx);
}


/*
 * Generator API.
 */

#define DEFAULT_BUFFER_SIZE 8192 // good as any; generally 2 pages/16 sectors

// This must stay POD (plain old data) in order for offsetof() to be valid.
struct FI_tag_Generator {
	FI_ErrorInfo error_info;

	FI_ContentHandler content_handler;

	FILE *fpout;
	FI_OctetStream *buffer;

	Document *document;
}; // FI_Generator

namespace {

int gen_ch_startDocument(FI_ContentHandler *) throw ();
int gen_ch_endDocument(FI_ContentHandler *) throw ();

int gen_ch_startElement(FI_ContentHandler *, const FI_Name *,
                        const FI_Attributes *) throw ();
int gen_ch_endElement(FI_ContentHandler *, const FI_Name *) throw ();

} // anonymous namespace

FI_Generator *
fi_create_generator(void)
{
	auto_ptr<FI_Generator> new_gen;
	auto_ptr<Document> new_doc;

	try {
		new_gen.reset(new FI_Generator);
		new_doc.reset(new Document);
	} catch (const std::bad_alloc& e) {
		// Yay, auto_ptr magic.
		return NULL;
	}

	FI_CLEAR_ERROR(new_gen->error_info);

	new_gen->content_handler.startDocument = gen_ch_startDocument;
	new_gen->content_handler.endDocument = gen_ch_endDocument;

	new_gen->content_handler.startElement = gen_ch_startElement;
	new_gen->content_handler.endElement = gen_ch_endElement;

	new_gen->fpout = NULL;
	new_gen->buffer = fi_create_stream(DEFAULT_BUFFER_SIZE);

	if (!new_gen->buffer) {
		// Yay, auto_ptr magic.
		return NULL;
	}

	new_gen->document = new_doc.release();
	return new_gen.release();
}

void
fi_destroy_generator(FI_Generator *gen)
{
	if (gen->fpout) {
		// XXX: Don't care, never finished document.
		fclose(gen->fpout);
	}

	fi_destroy_stream(gen->buffer);

	delete gen->document;
	delete gen;
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
	// Open stream.
	if (gen->fpout) {
		// TODO: Warn about incomplete document?
		fclose(gen->fpout);
	}

	gen->fpout = fopen(filename, "wb");
	if (!gen->fpout) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_NOFILE);
		return 0;
	}

	fi_clear_stream(gen->buffer);

	// Wait for generator events.
	return 1;
}

FI_VocabIndex
fi_add_element_name(FI_Generator *gen, const char *name)
{
	try {
		return gen->document->addElementName(name);
	} catch (const Exception& e) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_EXCEPTION);
		return FI_VOCAB_INDEX_NULL;
	}
}

FI_VocabIndex
fi_add_attribute_name(FI_Generator *gen, const char *name)
{
	try {
		return gen->document->addAttributeName(name);
	} catch (const Exception& e) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_EXCEPTION);
		return FI_VOCAB_INDEX_NULL;
	}
}


//
// Parser API.
//

struct FI_tag_Parser {
	FI_ErrorInfo error_info;

	FI_ContentHandler *content_handler;
}; // FI_Parser

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

	// Open stream.
	fpin = fopen(filename, "rb");
	if (!fpin) {
		// TODO: Record error info.
		return 0;
	}

	// Main parse loop.

	// Close stream.
	fclose(fpin); // don't care if fails, we got everything we wanted
	return 1;
}


//
// Generator subroutines.
//

// A fancy macro to get the corresponding FI_Generator object.
#define GEN_HANDLER_OFFSET (offsetof(FI_Generator, content_handler))
#define GET_GEN(h) ((FI_Generator *)((char *)(h) - GEN_HANDLER_OFFSET))

namespace {

// Helper routine to write out to buffers.
bool
write_object(FI_Generator *gen, Serializable *object)
{
	// Serialize object.
	try {
		object->write(gen->buffer);
	} catch (const Exception& e) {
		// FIXME: Extract FI_ErrorInfo data from Exception.
		FI_SET_ERROR(gen->error_info, FI_ERROR_EXCEPTION);
		return false;
	}

	// Write to file.
	const FI_Octet *buffer_ptr;

	FI_Length length = fi_read_stream(gen->buffer, &buffer_ptr);

	if (length > 0) {
		// fwrite() returns length, unless there's an I/O error.
		size_t w_len = fwrite(buffer_ptr, sizeof(FI_Octet), length,
		                      gen->fpout);
		if (w_len != length) {
			FI_SET_ERROR(gen->error_info, FI_ERROR_ERRNO);
			return false;
		}
	}

	return true;
}

int
gen_ch_startDocument(FI_ContentHandler *handler) throw ()
{
	FI_Generator *gen = GET_GEN(handler);

	// Sanity checks.
	if (!gen->fpout) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_INVAL);
		return 0;
	}

	// Write document header.
	gen->document->start();

	if (!write_object(gen, gen->document)) {
		// error_info set by write_object().
		return 0;
	}

	return 1;
}

int
gen_ch_endDocument(FI_ContentHandler *handler) throw ()
{
	FI_Generator *gen = GET_GEN(handler);

	// Sanity checks.
	if (!gen->fpout) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_INVAL);
		return 0;
	}

	// Write document trailer.
	gen->document->stop();

	if (!write_object(gen, gen->document)) {
		// error_info set by write_object().
		return 0;
	}

	// Close out file.
	if (fclose(gen->fpout) != 0) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_ERRNO);
		gen->fpout = NULL;
		return 0;
	}

	gen->fpout = NULL;
	return 1;
}

int
gen_ch_startElement(FI_ContentHandler *handler, const FI_Name *name,
                    const FI_Attributes *attrs) throw ()
{
	return 1;
}

int
gen_ch_endElement(FI_ContentHandler *handler, const FI_Name *name) throw ()
{
	return 1;
}

} // anonymous namespace


//
// Parser subroutines.
//
