/*
 * Module implementing the SAX2-style parser/generator API.
 */

#include "autoconf.h"

#include <cstdio>
#include <cassert>

#include "stream.h"

#include "Codec.hh"
#include "Document.hh"
#include "Element.hh"
#include "MutableAttributes.hh"

#include "sax.h"

using namespace BTech::FI;

namespace {

const char *const BT_NAMESPACE_URI = "http://btonline-btech.sourceforge.net";

const FI_Length DEFAULT_BUFFER_SIZE = 8192; // good as any; ~2 pages/16 sectors

} // anonymous namespace


/*
 * Generator API.
 */

namespace {

int gen_ch_startDocument(FI_ContentHandler *);
int gen_ch_endDocument(FI_ContentHandler *);

int gen_ch_startElement(FI_ContentHandler *, const FI_Name *,
                        const FI_Attributes *);
int gen_ch_endElement(FI_ContentHandler *, const FI_Name *);

int gen_ch_characters(FI_ContentHandler *, const FI_Value *);

} // anonymous namespace

struct FI_tag_Generator {
	FI_tag_Generator ();
	~FI_tag_Generator ();

	FI_ContentHandler content_handler;

	FI_ErrorInfo error_info;

	FILE *fpout;
	FI_OctetStream *buffer;

	Encoder encoder;
	Vocabulary vocabulary;

	Document document;
	Element element;

	const NSN_DS_VocabTable::TypedEntryRef BT_NAMESPACE;
}; // FI_Generator

FI_tag_Generator::FI_tag_Generator()
: fpout (0), buffer (0), element (document),
  BT_NAMESPACE (vocabulary.namespace_names.getEntry(BT_NAMESPACE_URI))
{
	buffer = fi_create_stream(DEFAULT_BUFFER_SIZE);
	if (!buffer) {
		throw std::bad_alloc ();
	}

	encoder.setStream(buffer);
	encoder.setVocabulary(vocabulary);

	content_handler.startDocument = gen_ch_startDocument;
	content_handler.endDocument = gen_ch_endDocument;

	content_handler.startElement = gen_ch_startElement;
	content_handler.endElement = gen_ch_endElement;

	content_handler.characters = gen_ch_characters;

	content_handler.app_data_ptr = this;

	FI_CLEAR_ERROR(error_info);
}

FI_tag_Generator::~FI_tag_Generator()
{
	if (fpout) {
		// XXX: Don't care, never finished document.
		fclose(fpout);
	}

	if (buffer) {
		fi_destroy_stream(buffer);
	}
}

FI_Generator *
fi_create_generator(void)
{
	try {
		return new FI_Generator ();
	} catch (const std::bad_alloc& e) {
		return 0;
	}
}

void
fi_destroy_generator(FI_Generator *gen)
{
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
	gen->encoder.clear();

	// Wait for generator events.
	return 1;
}

// Namespaces in XML 1.0 (Second Edition)
// http://www.w3.org/TR/2006/REC-xml-names-20060816
//
// Section 6.2, paragraph 2:
//
// A default namespace declaration applies to all unprefixed element names
// within its scope. Default namespace declarations do not apply directly to
// attribute names; the interpretation of unprefixed attributes is determined
// by the element on which they appear.

FI_Name *
fi_create_element_name(FI_Generator *gen, const char *name)
{
	try {
		const Name
		element_name (gen->vocabulary.local_names.getEntry(name),
		              gen->BT_NAMESPACE);

		const DN_VocabTable::TypedEntryRef element_ref
		= gen->vocabulary.element_names.getEntry(element_name);

		return new FI_Name (element_ref);
	} catch (const Exception& e) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_EXCEPTION);
		return FI_VOCAB_INDEX_NULL;
	} // FIXME: Catch all exceptions (at all C/C++ boundaries in API)
}

FI_Name *
fi_create_attribute_name(FI_Generator *gen, const char *name)
{
	try {
		const Name
		attr_name (gen->vocabulary.local_names.getEntry(name));

		const DN_VocabTable::TypedEntryRef attr_ref
		= gen->vocabulary.attribute_names.getEntry(attr_name);

		return new FI_Name (attr_ref);
	} catch (const Exception& e) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_EXCEPTION);
		return FI_VOCAB_INDEX_NULL;
	} // FIXME: Catch all exceptions (at all C/C++ boundaries in API)
}


//
// Parser API.
//

namespace {

void parse_document(FI_Parser *);

void parse_startDocument(FI_Parser *);
void parse_endDocument(FI_Parser *);

void parse_startElement(FI_Parser *);
void parse_endElement(FI_Parser *);

void parse_characters(FI_Parser *);

// Implements parsing support for COMMENT items.
struct Comment : public Serializable {
	void write (Encoder& encoder) const {
		throw UnsupportedOperationException ();
	}

	bool read (Decoder& decoder);

	int sub_step;
	FI_Length left_len;
}; // struct Comment

} // anonymous namespace

struct FI_tag_Parser {
	FI_tag_Parser ();
	~FI_tag_Parser ();

	FI_ContentHandler *content_handler;

	FI_ErrorInfo error_info;

	FILE *fpin;
	FI_OctetStream *buffer;

	Decoder decoder;
	Vocabulary vocabulary;

	Document document;
	Element element;
	Value characters;

	Comment comment;

	const NSN_DS_VocabTable::TypedEntryRef BT_NAMESPACE;
}; // FI_Parser

FI_tag_Parser::FI_tag_Parser()
: fpin (0), buffer (0), element (document),
  BT_NAMESPACE (vocabulary.namespace_names.getEntry(BT_NAMESPACE_URI))
{
	buffer = fi_create_stream(DEFAULT_BUFFER_SIZE);
	if (!buffer) {
		throw std::bad_alloc ();
	}

	decoder.setStream(buffer);
	decoder.setVocabulary(vocabulary);

	content_handler = 0;

	FI_CLEAR_ERROR(error_info);
}

FI_tag_Parser::~FI_tag_Parser()
{
	if (fpin) {
		// XXX: Don't care, never finished parsing.
		fclose(fpin);
	}

	if (buffer) {
		fi_destroy_stream(buffer);
	}
}

FI_Parser *
fi_create_parser(void)
{
	try {
		return new FI_Parser ();
	} catch (const std::bad_alloc& e) {
		return 0;
	}
}

void
fi_destroy_parser(FI_Parser *parser)
{
	delete parser;
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
	// Open stream.
	if (parser->fpin) {
		// TODO: Warn about incomplete parse?
		fclose(parser->fpin);
	}

	parser->fpin = fopen(filename, "rb");
	if (!parser->fpin) {
		FI_SET_ERROR(parser->error_info, FI_ERROR_NOFILE);
		return 0;
	}

	fi_clear_stream(parser->buffer);
	parser->decoder.clear();

	// Parse document from start to finish.
	bool parse_failed = false;

	try {
		parse_document(parser);
	} catch (const IllegalStateException& e) {
		// FIXME: Extract FI_ErrorInfo data from Exception.
		FI_SET_ERROR(parser->error_info, FI_ERROR_ILLEGAL);
		parse_failed = true;
	} catch (const UnsupportedOperationException& e) {
		// FIXME: Extract FI_ErrorInfo data from Exception.
		FI_SET_ERROR(parser->error_info, FI_ERROR_UNSUPPORTED);
		parse_failed = true;
	} catch (const Exception& e) {
		// FIXME: Extract FI_ErrorInfo data from Exception.
		FI_SET_ERROR(parser->error_info, FI_ERROR_EXCEPTION);
		parse_failed = true;
	} // TODO: Catch all other exceptions.

	if (parse_failed) {
		fclose(parser->fpin); // XXX: Don't care, parsing failed
		parser->fpin = 0;
		parser->vocabulary.clear(); // XXX: Save some memory
		parser->document.clearElements(); // XXX: Save some memory
		return 0;
	}

	// Close stream.
	fclose(parser->fpin); // XXX: Don't care, finished parsing
	parser->fpin = 0;
	parser->vocabulary.clear(); // XXX: Save some memory
	return 1;
}


namespace {

/*
 * Generator subroutines.
 */

// Get FI_Generator from FI_ContentHandler.
FI_Generator *
GET_GEN(FI_ContentHandler *handler)
{
	return static_cast<FI_Generator *>(handler->app_data_ptr);
}

// Helper routine to write out to buffers.
bool
write_object(FI_Generator *gen, const Serializable& object)
{
	// Serialize object.
	try {
		object.write(gen->encoder);
	} catch (const Exception& e) {
		// FIXME: Extract FI_ErrorInfo data from Exception.
		FI_SET_ERROR(gen->error_info, FI_ERROR_EXCEPTION);
		return false;
	}

	// Write to file.
	const FI_Octet *r_buf;

	FI_Length length = fi_read_stream(gen->buffer, &r_buf);

	if (length > 0) {
		// fwrite() returns length, unless there's an I/O error
		// (because we're using blocking I/O).
		size_t w_len = fwrite(r_buf, sizeof(FI_Octet), length,
		                      gen->fpout);
		if (w_len != length) {
			FI_SET_ERROR(gen->error_info, FI_ERROR_ERRNO);
			return false;
		}
	}

	return true;
}

int
gen_ch_startDocument(FI_ContentHandler *handler)
{
	FI_Generator *const gen = GET_GEN(handler);

	// Sanity checks.
	if (!gen->fpout) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_INVAL);
		return 0;
	}

	// Write document header.
	gen->document.start();

	if (!write_object(gen, gen->document)) {
		// error_info set by write_object().
		return 0;
	}

	return 1;
}

int
gen_ch_endDocument(FI_ContentHandler *handler)
{
	FI_Generator *const gen = GET_GEN(handler);

	// Sanity checks.
	if (!gen->fpout) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_INVAL);
		return 0;
	}

	// Write document trailer.
	gen->document.stop();

	if (!write_object(gen, gen->document)) {
		// error_info set by write_object().
		gen->vocabulary.clear(); // XXX: Save some memory
		gen->document.clearElements(); // XXX: Save some memory
		return 0;
	}

	// Close out file.
	if (fclose(gen->fpout) != 0) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_ERRNO);
		gen->fpout = 0;
		gen->vocabulary.clear(); // XXX: Save some memory
		return 0;
	}

	gen->fpout = 0;
	gen->vocabulary.clear(); // XXX: Save some memory
	return 1;
}

int
gen_ch_startElement(FI_ContentHandler *handler,
                    const FI_Name *name, const FI_Attributes *attrs)
{
	FI_Generator *const gen = GET_GEN(handler);

	// Sanity checks.
	if (!gen->fpout) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_INVAL);
		return 0;
	}

	// Write element header.
	gen->element.start(gen->BT_NAMESPACE);

	gen->element.setName(*name);
	gen->element.setAttributes(*attrs);

	if (!write_object(gen, gen->element)) {
		// error_info set by write_object().
		return 0;
	}

	return 1;
}

int
gen_ch_endElement(FI_ContentHandler *handler, const FI_Name *name)
{
	FI_Generator *const gen = GET_GEN(handler);

	// Sanity checks.
	if (!gen->fpout) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_INVAL);
		return 0;
	}

	// Write element trailer.
	gen->element.stop();

	// Don't need to set name, because we maintain the value on a stack.
	// Besides, we don't actually write it out anyway.
	// TODO: Should probably check they match, though?
	//gen->element.setName(*name);

	if (!write_object(gen, gen->element)) {
		// error_info set by write_object().
		return 0;
	}

	return 1;
}

int
gen_ch_characters(FI_ContentHandler *handler, const FI_Value *value)
{
	FI_Generator *const gen = GET_GEN(handler);

	// Sanity checks.
	if (!gen->fpout) {
		FI_SET_ERROR(gen->error_info, FI_ERROR_INVAL);
		return 0;
	}

	// Write character chunk.
	if (!write_object(gen, *value)) {
		// error_info set by write_object().
		return 0;
	}

	return 1;
}


/*
 * Parser subroutines.
 */

// Helper routine to read from file to buffer.
void
read_file_octets(FI_Parser *parser)
{
	// Prepare buffer for read from file.
	FI_Length min_len = fi_get_stream_needed_length(parser->buffer);

	if (min_len == 0 && feof(parser->fpin)) {
		// Nothing needs to or can be read.
		return;
	}

	FI_Length len = fi_get_stream_free_length(parser->buffer);

	if (len < min_len) {
		// Always try to read at least min_len.
		len = min_len;
	}

	if (len == 0) {
		// Nothing needs to be read.
		return;
	}

	FI_Octet *w_buf = fi_get_stream_write_buffer(parser->buffer, len);
	if (!w_buf) {
		// FIXME: Set error information in IOException.
		throw IOException ();
	}

	// Read into buffer.
	//
	// fread() returns len, unless there's an I/O error or end of file.
	// Since we optimistically read more data than we know is necessary, a
	// premature EOF is not an error, unless we are unable to satisfy our
	// minimum read length.
#if 1
	// XXX: Verify that the parser code can handle the worst case of being
	// forced to parse 1 octet at a time.
	min_len = 1;
	size_t r_len = fread(w_buf, sizeof(FI_Octet), 1, parser->fpin);
#else // 0
	size_t r_len = fread(w_buf, sizeof(FI_Octet), len, parser->fpin);
#endif // 0
	if (r_len < len) {
		if (ferror(parser->fpin)) {
			// FIXME: Set error information in IOException.
			throw IOException ();
		}

		// EOF.
		if (r_len < min_len) {
			// File isn't long enough.  Give up.
			// FIXME: Set error information in IOException.
			throw IOException ();
		}

		// Can still satisfy request.  Adjust write size.
		fi_reduce_stream_length(parser->buffer, len - r_len);
	}
}

void
parse_document(FI_Parser *parser)
{
	// Pre-fill the buffer.
	read_file_octets(parser);

	// Parse document header.
	parse_startDocument(parser);

	// Parse document children.
	for (;;) {
		ChildType next_child_type;

		while (!parser->decoder.readNext(next_child_type)) {
			read_file_octets(parser);
		}

		switch (next_child_type) {
		case END_CHILD:
			if (!parser->document.hasElements()) {
				// Interpret as end of document.
				goto break_top;
			} else {
				// Interpret as end of element.
				parse_endElement(parser);
			}
			break;

		case START_ELEMENT:
			// Element start.
			parse_startElement(parser);
			break;

		case CHARACTERS:
			// Character chunk.
			parse_characters(parser);
			break;

		case COMMENT:
			// Comment.  We silently consume these.
			parser->comment.sub_step = 0;

			while (!parser->comment.read(parser->decoder)) {
				read_file_octets(parser);
			}
			break;

		default:
			// FIXME: Unsupported child type.
			throw IllegalStateException ();
		}
	}

break_top:
	// Parse document trailer.
	parse_endDocument(parser);
}

void
parse_startDocument(FI_Parser *parser)
{
	parser->document.start();

	while (!parser->document.read(parser->decoder)) {
		read_file_octets(parser);
	}

	// Call content event handler.
	FI_ContentHandler *const handler = parser->content_handler;

	if (!handler || !handler->startDocument) {
		return;
	}

	if (!handler->startDocument(handler)) {
		// FIXME: Set error_info to user-triggered.
		throw Exception ();
	}
}

void
parse_endDocument(FI_Parser *parser)
{
	parser->document.stop();

	while (!parser->document.read(parser->decoder)) {
		read_file_octets(parser);
	}

	// Call content event handler.
	FI_ContentHandler *const handler = parser->content_handler;

	if (!handler || !handler->endDocument) {
		return;
	}

	if (!handler->endDocument(handler)) {
		// FIXME: Set error_info to user-triggered.
		throw Exception ();
	}
}

void
parse_startElement(FI_Parser *parser)
{
	parser->element.start(parser->BT_NAMESPACE);

	while (!parser->element.read(parser->decoder)) {
		read_file_octets(parser);
	}

	// Call content event handler.
	FI_ContentHandler *const handler = parser->content_handler;

	if (!handler || !handler->startElement) {
		return;
	}

	const DN_VocabTable::TypedEntryRef& name = parser->element.getName();
	const Attributes& attrs = parser->element.getAttributes();

	if (!handler->startElement(handler,
	                           FI_Name::cast(name),
	                           FI_Attributes::cast(attrs))) {
		// FIXME: Set error_info to user-triggered.
		throw Exception ();
	}
}

void
parse_endElement(FI_Parser *parser)
{
	parser->element.stop();

	while (!parser->element.read(parser->decoder)) {
		read_file_octets(parser);
	}

	// Call content event handler.
	FI_ContentHandler *const handler = parser->content_handler;

	if (!handler || !handler->endElement) {
		return;
	}

	const DN_VocabTable::TypedEntryRef& name = parser->element.getName();

	if (!handler->endElement(handler, FI_Name::cast(name))) {
		// FIXME: Set error_info to user-triggered.
		throw Exception ();
	}
}

void
parse_characters(FI_Parser *parser)
{
	parser->characters.setVocabTable(parser->vocabulary.content_character_chunks);

	while (!parser->characters.read(parser->decoder)) {
		read_file_octets(parser);
	}

	// Call content event handler.
	FI_ContentHandler *const handler = parser->content_handler;

	if (!handler || !handler->characters) {
		return;
	}

	if (!handler->characters(handler,
	                         FI_Value::cast(parser->characters))) {
		// FIXME: Set error_info to user-triggered.
		throw Exception ();
	}
}

// C.8
bool
Comment::read(Decoder& decoder)
{
	// The rules essentially follow those for reading values (C.14), except
	// we simply discard the actual value.
	//
	// XXX: We don't verify whether indexed comments actually exist or not.
	FI_UInt21 idx;
	FI_PInt32 tmp_len;

reparse:
	switch (sub_step) {
	case 0:
		assert(decoder.getBitOffset() == 0); // C.14.2

		// Examine discriminator bits.
		if (!decoder.readBits(1)) {
			return false;
		}

		if (!(decoder.getBits() & FI_BIT_1)) {
			// Use literal rules (C.14.3).

			// Ignore add-to-table (C.14.3.1).
			// Ignore discriminator (C.19.3).
			switch (decoder.getBits() & FI_BITS(,,1,1,,,,)) {
			case ENCODE_AS_UTF8:
			case ENCODE_AS_UTF16:
				// 1 bit add-to-table + 2 bits discriminator.
				decoder.readBits(3);
				sub_step = 3;
				break;

			case ENCODE_WITH_ALPHABET:
			case ENCODE_WITH_ALGORITHM:
				// 1 bit add-to-table + 2 bits discriminator.
				// 4(+4) bits of alphabet/algorithm index.
				decoder.readBits(7);
				sub_step = 2;
				break;
			}
			goto reparse;
		}

		sub_step = 1;
		// FALLTHROUGH

	case 1:
		// Read string-index using C.26 (C.14.4).
		if (!decoder.readUInt21_bit2(idx)) {
			return false;
		}

		// XXX: Ignore index.
		break;

	case 2:
		// Ignore last 4 bits of alphabet/algorithm index.
		if (!decoder.readBits(4)) {
			return false;
		}

		sub_step = 3;
		// FALLTHROUGH

	case 3:
		// Read character-string length using C.23.3 (C.19.4).
		if (!decoder.readNonEmptyOctets_len_bit5(tmp_len)) {
			return false;
		}

		// XXX: readNonEmptyOctets_len_bit5() checks for overflow.
		left_len = FI_PINT_TO_UINT(tmp_len);

		sub_step = 4;
		// FALLTHROUGH

	case 4:
		// Ignore value.
		if (!decoder.skipLength(left_len)) {
			return false;
		}
		break;

	default:
		// Should never happen.
		throw AssertionFailureException ();
	}

	sub_step = 0;
	return true;
}

} // anonymous namespace
