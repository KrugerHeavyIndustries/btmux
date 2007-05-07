/*
 * Implements top-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 section 12 and annex A, and informed by annex C.
 *
 * Only enough of Fast Infoset is implemented to support saving and loading
 * BattleTech binary databases.  In particular, only the Document, Element, and
 * Attribute types are supported, and additional types as needed to support
 * processing these types.  The objective is to support saving and loading a
 * hierarchical structure, possibly with per-element attributes.
 *
 * However, to ensure backward compatibility, all valid Fast Infoset bitstreams
 * may be decoded, although possibly with the loss of information contained in
 * types beyond the basic three of Document, Element, and Attribute.
 *
 * Implementation restrictions:
 * 7.2.4: restricted-alphabets not written, must be processed.
 * 7.2.5: encoding-algorithms not written, error on read.
 * 7.2.7: additional-data not written, ignored on read.
 * 7.2.13: external-vocabulary not written, error on read.
 * 7.2.16: local-names contains all our identifier strings, while
 *         element-name-surrogates and attribute-name-surrogates contain all
 *         our name surrogates.  other-ncnames, other-uris, attribute-values,
 *         content-character-chunks, and other-strings are not written, but
 *         must be processed. (Some may be ignorable?)
 * 7.2.21: prefixes not written, must be processed. (Ignorable?)
 * 7.2.23: namespace-names contains our namespace name.
 * 7.2.24: [notations] unsupported.
 * 7.2.25: [unparsed entities] unsupported.
 * 7.2.26: [character encoding scheme] must not be set (UTF-8).
 * 7.2.27: [standalone] must not be set (???).
 * 7.2.28: [version] must not be set (???).
 * 7.2.29: [children] must only include the root element, [document element].
 *
 * Update: initial-vocabulary is currently not written, must be processed.
 */

#include "autoconf.h"

#include <cstring>
#include <cassert>

#include "stream.h"

#include "Exception.hh"
#include "Name.hh"

#include "Document.hh"


namespace BTech {
namespace FI {

namespace {

const char *const BT_NAMESPACE_URI = "http://btonline-btech.sourceforge.net";

#if 0 // defined(FI_USE_INITIAL_VOCABULARY)
bool write_ds_table(FI_OctetStream *, const DS_VocabTable&);
bool write_dn_table(FI_OctetStream *, const DN_VocabTable&);
#endif // FI_USE_INITIAL_VOCABULARY

} // anonymous namespace

Document::Document()
: start_flag (false), stop_flag (false),
  BT_NAMESPACE (vocabulary.namespace_names.getEntry(BT_NAMESPACE_URI))
{
}

void
Document::start()
{
	start_flag = true;
	stop_flag = false;
	r_state = RESET_READ_STATE;
}

void
Document::stop()
{
	start_flag = false;
	stop_flag = true;
	r_state = RESET_READ_STATE;
}

void
Document::write(FI_OctetStream *stream)
{
	if (start_flag) {
		// Ensure state is cleared.
		element_stack.clear();
		vocabulary.clear();

		// Write document header.
		write_header(stream);

#if 0 // defined(USE_FI_INITIAL_VOCABULARY)
		if (!writeVocab(stream)) {
			// TODO: Assign an exception for stream errors.
			throw Exception ();
		}
#endif // FI_USE_INITIAL_VOCABULARY

		saw_root_element = false;
	} else if (stop_flag) {
		if (!saw_root_element) {
			// Not a valid Fast Infoset.
			throw IllegalStateException ();
		}

		// Write document trailer.
		write_trailer(stream);

		// Clear state now, to save some memory.  Note that for the
		// vocabulary tables, any cached entries that still have an
		// EntryRef to them will remain interned, so they won't be
		// constantly reallocated on each run (but the indexes will be
		// reset, as intended).
		assert(!hasElements());
		element_stack.clear();
		vocabulary.clear();
	} else {
		throw IllegalStateException ();
	}
}

void
Document::read(FI_OctetStream *stream)
{
	if (start_flag) {
		switch (r_state) {
		case RESET_READ_STATE:
			// Ensure state is cleared.
			element_stack.clear();
			vocabulary.clear();

			r_state = MAIN_READ_STATE;
			r_header_state = RESET_HEADER_STATE;
			// FALLTHROUGH

		case MAIN_READ_STATE:
			// Try to read document header.
			if (!read_header(stream)) {
				return;
			}

			r_state = NEXT_PART_READ_STATE;
			// FALLTHROUGH

		case NEXT_PART_READ_STATE:
			// Determine next child type.
			read_next(stream);
			saw_root_element = false;
			break;
		}
	} else if (stop_flag) {
		switch (r_state) {
		case RESET_READ_STATE:
			if (!saw_root_element) {
				// Not a valid Fast Infoset.
				throw IllegalStateException ();
			}

			r_state = MAIN_READ_STATE;
			// FALLTHROUGH

		case MAIN_READ_STATE:
			// Try to read document trailer.
			if (!read_trailer(stream)) {
				return;
			}

			r_state = NEXT_PART_READ_STATE;
			// FALLTHROUGH

		case NEXT_PART_READ_STATE:
			// Clear state now, to save some memory.  Note that for
			// the vocabulary tables, any cached entries that still
			// have an EntryRef to them will remain interned, so
			// they won't be constantly reallocated on each run
			// (but the indexes will be reset, as intended).
			assert(!hasElements());
			element_stack.clear();
			vocabulary.clear();
			break;
		}
	} else {
		throw IllegalStateException ();
	}
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

const DN_VocabTable::TypedEntryRef
Document::getElementName(const char *name)
{
	const Name element_name (vocabulary.local_names.getEntry(name),
	                         BT_NAMESPACE);
	return vocabulary.element_names.getEntry(element_name);
}

const DN_VocabTable::TypedEntryRef
Document::getAttributeName(const char *name)
{
	const Name attribute_name (vocabulary.local_names.getEntry(name));
	return vocabulary.attribute_names.getEntry(attribute_name);
}

#if 0 // defined(FI_USE_INITIAL_VOCABULARY)
bool
Document::writeVocab(FI_OctetStream *stream)
{
	// Write padding (C.2.5: 000).
	// Write optional component presence flags (C.2.5.1: 00001 ?00000??).
	FI_Octet *w_buf = fi_get_stream_write_buffer(stream, 2);
	if (!w_buf) {
		return false;
	}

	w_buf[0] = FI_BIT_8 /* namespace-names present */;
	w_buf[1] = (local_names.size() ? FI_BIT_1 : 0)
	           | (element_name_surrogates.size() ? FI_BIT_7 : 0)
	           | (attribute_name_surrogates.size() ? FI_BIT_8 : 0);

	// Write namespace-names (C.2.5.3).
	if (!write_ds_table(stream, namespace_names)) {
		return false;
	}

	// Write local-names (C.2.5.3)?
	if (local_names.size() && !write_ds_table(stream, local_names)) {
		return false;
	}

	// Write element-name-surrogates (C.2.5.5)?
	if (element_name_surrogates.size()
	    && !write_dn_table(stream, element_name_surrogates)) {
		return false;
	}

	// Write attribute-name-surrogates (C.2.5.5)?
	if (attribute_name_surrogates.size()
	    && !write_dn_table(stream, attribute_name_surrogates)) {
		return false;
	}

	return true;
}
#endif // !FI_USE_INITIAL_VOCABULARY


/*
 * Document common subroutines.
 */

namespace {

const char *const xml_decl[] = {
	"<?xml encoding='finf'?>",
	"<?xml encoding='finf' standalone='no'?>",
	"<?xml encoding='finf' standalone='yes'?>",
	"<?xml version='1.0' encoding='finf'?>",
	"<?xml version='1.0' encoding='finf' standalone='no'?>",
	"<?xml version='1.0' encoding='finf' standalone='yes'?>",
	"<?xml version='1.1' encoding='finf'?>",
	"<?xml version='1.1' encoding='finf' standalone='no'?>",
	"<?xml version='1.1' encoding='finf' standalone='yes'?>"
}; // xml_decl[]

const char *
get_xml_decl(int version, int standalone)
{
	return xml_decl[3 * version + standalone];
}

} // anonymous namespace


/*
 * Document serialization subroutines.
 */

void
Document::write_header(FI_OctetStream *stream)
{
	// Decide which XML declaration to use.
	const char *xml_decl = get_xml_decl(0 /* no version */,
	                                    0 /* no standalone */);

	size_t xml_decl_len = strlen(xml_decl);

	// Reserve space.
	FI_Octet *w_buf = fi_get_stream_write_buffer(stream, xml_decl_len + 5);
	if (!w_buf) {
		// TODO: Assign an Exception for stream errors.
		throw Exception ();
	}

	// Write XML declaration.
	memcpy(w_buf, xml_decl, xml_decl_len);
	w_buf += xml_decl_len;

	// Write identification (12.6: 11100000 00000000).
	w_buf[0] = FI_BITS(1,1,1,0,0,0,0,0);
	w_buf[1] = FI_BITS(0,0,0,0,0,0,0,0);

	// Write Fast Infoset version number (12.7: 00000000 00000001).
	w_buf[2] = FI_BITS(0,0,0,0,0,0,0,0);
	w_buf[3] = FI_BITS(0,0,0,0,0,0,0,1);

#if 0 // defined(FI_USE_INITIAL_VOCABULARY)
	// Write padding (12.8: 0).
	w_buf[4] = FI_BITS(0, // padding

	// Write optional component presence flags (C.2.3: 0100000).
	                   0,
	                   1, // initial-vocabulary
	                   0,
	                   0,
	                   0,
	                   0,
	                   0);
#else // !FI_USE_INITIAL_VOCABULARY
	// Write padding (12.8: 0).
	w_buf[4] = FI_BITS(0, // padding

	// Write optional component presence flags (C.2.3: 0000000).
	                   0,
	                   0, // no initial-vocabulary
	                   0,
	                   0,
	                   0,
	                   0,
	                   0);
#endif // !FI_USE_INITIAL_VOCABULARY
}

void
Document::write_trailer(FI_OctetStream *stream)
{
	FI_Octet *w_buf;

	// Serialize document trailer.
	switch (fi_get_stream_num_bits(stream)) {
	case 0:
		// Write termination (C.2.12: 1111).
		// 12.11: End on the 4th bit of an octet.
		w_buf = fi_get_stream_write_buffer(stream, 1);
		if (!w_buf) {
			// TODO: Assign an Exception for stream errors.
			throw Exception ();
		}

		w_buf[0] = FI_BITS(1,1,1,1,,,,);
		break;

	case 4:
		// Write termination (C.2.12: 1111).
		// 12.11: End on the 8th bit of an octet.
		if (!fi_write_stream_bits(stream, 4, FI_BITS(1,1,1,1,,,,))) {
			// TODO: Assign an Exception for stream errors.
			throw Exception ();
		}
		break;

	default:
		// Shouldn't happen.
		throw IllegalStateException ();
	}
}

#if 0 // defined(FI_USE_INITIAL_VOCABULARY)
// C.2.5.3: Identifier string vocabulary tables.
bool
write_ds_table(FI_OctetStream *stream, const DS_VocabTable& vocab)
{
	// Write string count (C.21).
	if (!write_length_sequence_of(stream,
	                              vocab.size() - vocab.last_builtin)) {
		return false;
	}

	// Write items.
	for (FI_VocabIndex ii = vocab.first_added; ii <= vocab.size(); ii++) {
		// Write item padding (C.2.5.3: 0).
		if (!fi_write_stream_bits(stream, 1, 0)) {
			return false;
		}

		// Write item (C.22).
		if (!write_non_empty_string_bit_2(stream, vocab[ii])) {
			return false;
		}
	}

	return true;
}

// C.2.5.5: Name surrogate vocabulary tables.
bool
write_dn_table(FI_OctetStream *stream, const DN_VocabTable& vocab)
{
	// Write name surrogate count (C.21).
	if (!write_length_sequence_of(stream, vocab.size())) {
		return false;
	}

	// Write items.
	for (FI_VocabIndex ii = 1; ii <= vocab.size(); ii++) {
		// Write item padding (C.2.5.5: 000000).
		if (!fi_write_stream_bits(stream, 6, 0)) {
			return false;
		}

		// Write item (C.16).
		if (!write_name_surrogate(stream, vocab[ii])) {
			return false;
		}
	}

	return true;
}
#endif // FI_USE_INITIAL_VOCABULARY


/*
 * Document unserialization subroutines.
 */

namespace {

bool
read_xml_decl(FI_OctetStream *stream, FI_Length& r_len_state)
{
	const FI_Octet *r_buf;
	FI_Length avail_len;

	// Get at least one more octet than last time.
	avail_len = fi_try_read_stream(stream, &r_buf, 0, r_len_state + 1);
	if (avail_len <= r_len_state) {
		return false;
	}

	// Search for "?>" from r_len_state - 2 to the current position.
	FI_Length ii;

	bool saw_question_mark = false;
	for (ii = r_len_state - 1; ii < avail_len; ii++) {
		if (saw_question_mark) {
			if (r_buf[ii] == '>') {
				// Probably just saw an XML declaration.
				// TODO: Implement the full grammar.
				break;
			} else if (r_buf[ii] != '?') {
				saw_question_mark = false;
			}
		} else if (r_buf[ii] == '?') {
			saw_question_mark = true;
		}
	}

	if (ii < avail_len) {
		ii++; // advance ii to input XML declaration length
	} else {
		// Didn't find a "?>" yet.
		r_len_state = ii;

		// Just as a sanity check, give up after 192 octets or so.
		if (r_len_state > 192) {
			throw UnsupportedOperationException ();
		}

		// Mark the stream as needing at least one more byte.
		if (saw_question_mark) {
			fi_set_stream_needed_length(stream, 1);
		} else {
			fi_set_stream_needed_length(stream, 2);
		}

		return false;
	}

	// Try to match against the 9 possible XML declarations for a valid
	// Fast Infoset document (12.3).
	const int XML_DECL_MAX = sizeof(xml_decl) / sizeof(xml_decl[0]);

	int jj;

	for (jj = 0; jj < XML_DECL_MAX; jj++) {
		if (strlen(xml_decl[jj]) != ii) {
			// Not even the same length.
			continue;
		}

		if (memcmp(r_buf, xml_decl[jj], ii) == 0) {
			// Match, yay.
			break;;
		}
	}

	if (jj == XML_DECL_MAX) {
		// TODO: We're being rather strict here about what we accept,
		// since we didn't implement a full XML declaration parser.
		// This behavior is allowed by the X.891 standard, but probably
		// isn't very friendly.
		throw IllegalStateException ();
	}

	// FIXME: We don't verify that the XML declaration matches the contents
	// of the Document section.

	// Success, advance cursor.
	fi_advance_stream_cursor(stream, ii);
	return true;
}

} // anonymous namespace

bool
Document::read_header(FI_OctetStream *stream)
{
	const FI_Octet *r_buf;

redispatch:
	switch (r_header_state) {
	case RESET_HEADER_STATE:
		// A Fast Infoset may optionally begin with a UTF-8 encoded XML
		// declaration (which begins with "<?xml") (12.3).
		if (fi_try_read_stream(stream, &r_buf, 0, 4) < 4) {
			return false;
		}

		if (r_buf[0] != '<' || r_buf[1] != '?'
		    || r_buf[2] != 'x' || r_buf[3] != 'm') {
			// Try again as a Fast Infoset identification string.
			r_header_state = MAIN_HEADER_STATE;
			// XXX: Gotos are not evil, but if you really feel this
			// one is so egregious, you can replace it with a
			// continue and a loop, or some other construct.  But I
			// think that's even worse.
			goto redispatch;
		}

		r_header_state = XML_DECL_HEADER_STATE;
		r_len_state = 4;
		// FALLTHROUGH

	case XML_DECL_HEADER_STATE:
		// Try to read XML declaration.
		if (!read_xml_decl(stream, r_len_state)) {
			return false;
		}

		r_header_state = MAIN_HEADER_STATE;
		// FALLTHROUGH

	case MAIN_HEADER_STATE:
		if (fi_try_read_stream(stream, &r_buf, 5, 5) < 5) {
			return false;
		}

		// Read identification (12.6: 11100000 00000000).
		if (r_buf[0] != FI_BITS(1,1,1,0,0,0,0,0)
		    || r_buf[1] != FI_BITS(0,0,0,0,0,0,0,0)) {
			// Not a valid Fast Infoset.
			throw IllegalStateException ();
		}

		// Read Fast Infoset version number (12.7: 00000000 00000001).
		if (r_buf[2] != FI_BITS(0,0,0,0,0,0,0,0)
		    || r_buf[3] != FI_BITS(0,0,0,0,0,0,0,1)) {
			// Unsupported Fast Infoset version.
			throw UnsupportedOperationException ();
		}

		// Next comes a 0 bit of padding, then 7 bits for the optional
		// component presence flags (C.2.3).  We don't support any of
		// them right now, so we expect 0 0000000.
		if (r_buf[4] != FI_BITS(0, // padding

		                        0,
		                        0, // no initial-vocabulary
		                        0,
		                        0,
		                        0,
		                        0,
		                        0)) {
			throw UnsupportedOperationException ();
		}
		break;
	}

	return true;
}

bool
Document::read_trailer(FI_OctetStream *stream)
{
	// Read document terminator bits (C.2.12).
	const FI_Octet *r_buf;
	FI_Octet bits;

	if (fi_try_read_stream(stream, &r_buf, 1, 1) < 1) {
		return false;
	}

	switch (fi_get_stream_num_bits(stream)) {
	case 0:
		// Ended on 4th bit, has padding (1111 0000).
		bits = r_buf[0];

		if (bits != FI_BITS(1,1,1,1, 0,0,0,0)) {
			// Not a valid Fast Infoset.
			throw IllegalStateException ();
		}
		break;

	case 4:
		// Ended on 8th bit, no padding (xxxx 1111).
		bits = (r_buf[0] << 4) & FI_BITS(1,1,1,1,,,,);
		if (bits != FI_BITS(1,1,1,1,,,,)) {
			// Not a valid Fast Infoset.
			throw IllegalStateException ();
		}
		break;

	default:
		// Should never happen.
		throw IllegalStateException ();
	}

	return true;
}

// XXX: This code is also used to iterate over Element children.
void
Document::read_next(FI_OctetStream *stream)
{
	// Peek at the next octet to figure out what to do.
	const FI_Octet *r_buf;
	FI_Octet bits;

	if (fi_try_read_stream(stream, &r_buf, 0, 1) < 1) {
		return;
	}

	// Children start on the first bit of an octet, but the most recent bit
	// may have been the fourth or eight bit (C.2.11.1, C.3.7.1).
	switch (fi_get_stream_num_bits(stream)) {
	case 0:
		// Either start of child, or terminator.
		bits = r_buf[0] & FI_BITS(1,1,1,1,,,,);

		if (bits == FI_BITS(1,1,1,1,,,,)) {
			// Terminator.
			// FIXME: We only support Element/Document terminators.
			if (hasElements()) {
				next_child_type = END_ELEMENT;
			} else {
				// END_DOCUMENT
				next_child_type = NO_CHILD;
			}

			return;
		} else {
			// Child starts on this octet.
		}
		break;

	case 4:
		// Either terminator, or padding.
		bits = (r_buf[0] << 4) & FI_BITS(1,1,1,1,,,,);

		if (bits == FI_BITS(1,1,1,1,,,,)) {
			// Terminator.
			// FIXME: We only support Element/Document terminators.
			if (hasElements()) {
				next_child_type = END_ELEMENT;
			} else {
				// END_DOCUMENT
				next_child_type = NO_CHILD;
			}

			return;
		} else if (bits == FI_BITS(0,0,0,0,,,,)) {
			// Padding.  Child starts on next octet.
			fi_set_stream_num_bits(stream, 0);

			if (fi_try_read_stream(stream, &r_buf, 1, 2) < 2) {
				// We already have enough state to resume.
				return;
			}

			r_buf++;
		} else {
			// Not a valid Fast Infoset.
			throw IllegalStateException ();
		}
		break;

	default:
		// Shouldn't happen.
		throw IllegalStateException ();
	}

	// Identify the child type.
	if (!(r_buf[0] & FI_BIT_1)) {
		// Child is the start of an element (C.2.11.2, C.3.7.2).
		next_child_type = START_ELEMENT;
	} else {
		// Unsupported child type.
		throw UnsupportedOperationException ();
	}
}

} // namespace FI
} // namespace BTech
