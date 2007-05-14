/*
 * Module implementing the SAX2-style parser/generator API.
 */

#include "autoconf.h"

#include <cstdio>
#include <cassert>

#include "stream.h"
#include "encalg.h"

#include "Vocabulary.hh"
#include "Codec.hh"
#include "Document.hh"
#include "Element.hh"
#include "MutableAttributes.hh"

#include "sax.h"

using namespace BTech::FI;

namespace {

const FI_Length DEFAULT_BUFFER_SIZE = 8192; // good as any; ~2 pages/16 sectors

// Implements generating and parsing support for CHARACTERS items.
struct Characters : public Serializable {
	Characters (DV_VocabTable& vocab_table) : vocab_table (vocab_table) {
		r_value.setVocabTable(vocab_table);
	}

	void write (Encoder& encoder) const;
	bool read (Decoder& decoder);

	const FI_Value *w_value;

	DV_VocabTable& vocab_table;
	FI_Value r_value;
}; // struct Characters

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
	FI_tag_Generator (FI_Vocabulary& vocab);
	~FI_tag_Generator ();

	FI_ContentHandler content_handler;

	FI_ErrorInfo error_info;

	FILE *fpout;				// caller-created
	FI_OctetStream *buffer;

	Encoder encoder;
	FI_Vocabulary& vocabulary;

	Document document;
	Element element;
	Characters characters;
}; // FI_Generator

FI_tag_Generator::FI_tag_Generator(FI_Vocabulary& vocab)
: fpout (0), buffer (0), vocabulary (vocab),
  element (document), characters (vocab.content_character_chunks)
{
	buffer = fi_create_stream(DEFAULT_BUFFER_SIZE);
	if (!buffer) {
		throw std::bad_alloc ();
	}

	encoder.setStream(buffer);
	encoder.setVocabulary(vocab);

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
	if (buffer) {
		fi_destroy_stream(buffer);
	}
}

FI_Generator *
fi_create_generator(FI_Vocabulary *vocab)
{
	if (!fi_init_encoding_algorithms()) {
		return 0;
	}

	try {
		return new FI_Generator (*vocab);
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
fi_generate_file(FI_Generator *gen, FILE *fpout)
{
	// Open stream.
	if (gen->fpout) {
		// TODO: Warn about incomplete document?
		fclose(gen->fpout);
	}

	gen->fpout = fpout;

	fi_clear_stream(gen->buffer);
	gen->encoder.clear();

	// Wait for generator events.
	return 1;
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

} // anonymous namespace

struct FI_tag_Parser {
	FI_tag_Parser (FI_Vocabulary& vocab);
	~FI_tag_Parser ();

	FI_ContentHandler *content_handler;

	FI_ErrorInfo error_info;

	FILE *fpin;				// caller-created
	FI_OctetStream *buffer;

	Decoder decoder;
	FI_Vocabulary& vocabulary;

	Document document;
	Element element;
	Characters characters;

	Comment comment;
}; // FI_Parser

FI_tag_Parser::FI_tag_Parser(FI_Vocabulary& vocab)
: fpin (0), buffer (0), vocabulary (vocab),
  element (document), characters (vocabulary.content_character_chunks)
{
	buffer = fi_create_stream(DEFAULT_BUFFER_SIZE);
	if (!buffer) {
		throw std::bad_alloc ();
	}

	decoder.setStream(buffer);
	decoder.setVocabulary(vocab);

	content_handler = 0;

	FI_CLEAR_ERROR(error_info);
}

FI_tag_Parser::~FI_tag_Parser()
{
	if (buffer) {
		fi_destroy_stream(buffer);
	}
}

FI_Parser *
fi_create_parser(FI_Vocabulary *vocab)
{
	if (!fi_init_encoding_algorithms()) {
		return 0;
	}

	try {
		return new FI_Parser (*vocab);
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
fi_parse_file(FI_Parser *parser, FILE *fpin)
{
	// Open stream.
	if (parser->fpin) {
		// TODO: Warn about incomplete parse?
	}

	parser->fpin = fpin;

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
		parser->fpin = 0;

		// XXX: Save some memory.
		fi_clear_stream(parser->buffer);
		parser->vocabulary.clear();
		parser->document.clearElements();
		return 0;
	}

	// Close stream.
	parser->fpin = 0;

	// XXX: Save some memory.
	fi_clear_stream(parser->buffer);
	parser->vocabulary.clear();
	assert(!parser->document.hasElements());
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
	FI_Length length = fi_get_stream_length(gen->buffer);

	const FI_Octet *r_buf = fi_get_stream_read_window(gen->buffer, length);

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

	fi_advance_stream_read_cursor(gen->buffer, length);
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
	try {
		gen->document.start();

		if (!write_object(gen, gen->document)) {
			// error_info set by write_object().
			return 0;
		}
	} catch (...) {
		// TODO: Catching all exceptions is kinda hazardous.
		FI_SET_ERROR(gen->error_info, FI_ERROR_EXCEPTION);
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
	try {
		gen->document.stop();

		if (!write_object(gen, gen->document)) {
			// error_info set by write_object().

			// XXX: Save some memory.
			fi_clear_stream(gen->buffer);
			gen->vocabulary.clear();
			gen->document.clearElements();
			return 0;
		}
	} catch (...) {
		// TODO: Catching all exceptions is kinda hazardous.
		FI_SET_ERROR(gen->error_info, FI_ERROR_EXCEPTION);
		return 0;
	}

	// Close out file.
	gen->fpout = 0;

	// XXX: Save some memory.
	fi_clear_stream(gen->buffer);
	gen->vocabulary.clear();
	assert(!gen->document.hasElements());
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
	try {
		gen->element.start(gen->vocabulary.BT_NAMESPACE);

		gen->element.setName(*name);
		gen->element.setAttributes(*attrs);

		if (!write_object(gen, gen->element)) {
			// error_info set by write_object().
			return 0;
		}
	} catch (...) {
		// TODO: Catching all exceptions is kinda hazardous.
		FI_SET_ERROR(gen->error_info, FI_ERROR_EXCEPTION);
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
	try {
		gen->element.stop();

		// Don't need to set name, because we maintain the value on a
		// stack.  Besides, we don't actually write it out anyway.
		// TODO: Should probably check they match, though?
		//gen->element.setName(*name);

		if (!write_object(gen, gen->element)) {
			// error_info set by write_object().
			return 0;
		}
	} catch (...) {
		// TODO: Catching all exceptions is kinda hazardous.
		FI_SET_ERROR(gen->error_info, FI_ERROR_EXCEPTION);
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

	if (!gen->document.hasElements()) {
		// Character data is ony allowed inside of an element.
		FI_SET_ERROR(gen->error_info, FI_ERROR_INVAL);
		return 0;
	}

	// Write character chunk.
	try {
		gen->characters.w_value = value;

		if (!write_object(gen, gen->characters)) {
			// error_info set by write_object().
			return 0;
		}
	} catch (...) {
		// TODO: Catching all exceptions is kinda hazardous.
		FI_SET_ERROR(gen->error_info, FI_ERROR_EXCEPTION);
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

	FI_Octet *w_buf = fi_get_stream_write_window(parser->buffer, len);

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
#if 0
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

		// Can still satisfy request.
	}

	fi_advance_stream_write_cursor(parser->buffer, r_len);
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
	parser->element.start(parser->vocabulary.BT_NAMESPACE);

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
	if (!parser->document.hasElements()) {
		// Character data is ony allowed inside of an element.
		throw IllegalStateException ();
	}

	while (!parser->characters.read(parser->decoder)) {
		read_file_octets(parser);
	}

	// Call content event handler.
	FI_ContentHandler *const handler = parser->content_handler;

	if (!handler || !handler->characters) {
		return;
	}

	if (!handler->characters(handler,
	                         FI_Value::cast(parser->characters.r_value))) {
		// FIXME: Set error_info to user-triggered.
		throw Exception ();
	}
}

// C.7
void
Characters::write(Encoder& encoder) const
{
	encoder.writeNext(CHARACTERS);

	w_value->write(encoder);
}

bool
Characters::read(Decoder& decoder)
{
	return r_value.read(decoder);
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
