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

#include <cassert>

#include "Codec.hh"

#include "Exception.hh"
#include "Name.hh"

#include "Document.hh"


namespace BTech {
namespace FI {

Document::Document()
: serialize_mode (SERIALIZE_NONE)
{
}

void
Document::start()
{
	serialize_mode = SERIALIZE_HEADER;
	r_state = RESET_READ_STATE;

	element_stack.clear();
	saw_root_element = false;
}

void
Document::stop()
{
	serialize_mode = SERIALIZE_TRAILER;
	r_state = RESET_READ_STATE;

	if (hasElements() || !saw_root_element) {
		// Not a valid Fast Infoset.
		throw IllegalStateException ();
	}
}

void
Document::write(Encoder& encoder) const
{
	switch (serialize_mode) {
	case SERIALIZE_HEADER:
		// Write document header.
		write_header(encoder);

#if 0 // defined(USE_FI_INITIAL_VOCABULARY)
		if (!writeVocab(stream)) {
			// TODO: Assign an exception for stream errors.
			throw Exception ();
		}
#endif // FI_USE_INITIAL_VOCABULARY
		break;

	case SERIALIZE_TRAILER:
		// Write document trailer.
		write_trailer(encoder);
		break;

	default:
		// Didn't call start()/stop() first.
		throw IllegalStateException ();
	}
}

bool
Document::read(Decoder& decoder)
{
	switch (serialize_mode) {
	case SERIALIZE_HEADER:
		switch (r_state) {
		case RESET_READ_STATE:
			// Ensure state is cleared.
			r_state = MAIN_READ_STATE;
			r_header_state = RESET_HEADER_STATE;
			// FALLTHROUGH

		case MAIN_READ_STATE:
			// Try to read document header.
			if (!read_header(decoder)) {
				return false;
			}
			break;
		}
		break;

	case SERIALIZE_TRAILER:
		switch (r_state) {
		case RESET_READ_STATE:
			if (decoder.getBitOffset() == 4) {
				// Ended on 4th bit, has padding (1111 0000).
				if (decoder.getBits()
				    != FI_BITS(1,1,1,1, 0,0,0,0)) {
					// Not a valid Fast Infoset.
					throw IllegalStateException ();
				}
			} else {
				assert(decoder.getBitOffset() == 0);
			}
			break;

		default:
			// Should never happen.
			throw AssertionFailureException ();
		}
		break;

	default:
		// Didn't call start()/stop() first.
		throw IllegalStateException ();
	}

	return true;
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
 * Document serialization subroutines.
 */

void
Document::write_header(Encoder& encoder) const
{
	// Write XML declaration.
	XMLVersion version = XML_VERSION_NONE;
	FI_Ternary standalone = FI_TERNARY_UNKNOWN;

	encoder.writeXMLDecl(version, standalone);

	// Write identification (12.6: 11100000 00000000).
	FI_Octet *w_buf = encoder.getWriteBuffer(5);

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
Document::write_trailer(Encoder& encoder) const
{
	FI_Octet *w_buf;

	// Serialize document trailer.
	switch (encoder.getBitOffset()) {
	case 0:
		// Write termination (C.2.12: 1111).
		// 12.11: End on the 4th bit of an octet.
		w_buf = encoder.getWriteBuffer(1);

		w_buf[0] = FI_BITS(1,1,1,1, 0,0,0,0);
		break;

	case 4:
		// Write termination (C.2.12: 1111).
		// 12.11: End on the 8th bit of an octet.
		encoder.writeBits(4, FI_BITS(,,,,1,1,1,1));
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

bool
Document::read_header(Decoder& decoder)
{
	const FI_Octet *r_buf;

	switch (r_header_state) {
	case RESET_HEADER_STATE:
		// A Fast Infoset may optionally begin with a UTF-8 encoded XML
		// declaration (which begins with "<?xml") (12.3).
		if (!decoder.readXMLDecl()) {
			return false;
		}

		r_header_state = MAIN_HEADER_STATE;
		// FALLTHROUGH

	case MAIN_HEADER_STATE:
		r_buf = decoder.getReadBuffer(5);
		if (!r_buf) {
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

} // namespace FI
} // namespace BTech
