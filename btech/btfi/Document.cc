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
 */

#include "autoconf.h"

#include <cstring>
#include <string>

#include "stream.h"

#include "Document.hh"


namespace BTech {
namespace FI {

namespace {

const char *const NAMESPACE = "http://btonline-btech.sourceforge.net";

bool write_header(FI_OctetStream *) throw ();
bool write_trailer(FI_OctetStream *) throw ();

bool write_ds_table(FI_OctetStream *, const DS_VocabTable&) throw ();
bool write_dn_table(FI_OctetStream *, const DN_VocabTable&) throw ();

} // anonymous namespace

void
Document::start() throw ()
{
	start_flag = true;
	stop_flag = false;

}

void
Document::stop() throw ()
{
	start_flag = false;
	stop_flag = true;
}

void
Document::write(FI_OctetStream *stream) throw (Exception)
{
	if (start_flag) {
		initWrite();

		if (!write_header(stream)) {
			// TODO: Assign an exception for stream errors.
			throw Exception ();
		}

		if (!writeVocab(stream)) {
			// TODO: Assign an exception for stream errors.
			throw Exception ();
		}
	} else if (stop_flag) {
		if (!write_trailer(stream)) {
			// TODO: Assign an exception for stream errors.
			throw Exception ();
		}
	} else {
		throw IllegalStateException ();
	}
}

void
Document::read(FI_OctetStream *stream) throw (Exception)
{	
	if (start_flag) {
	} else if (stop_flag) {
	} else {
		throw IllegalStateException ();
	}

	throw UnsupportedOperationException ();
}

void
Document::initWrite() throw (Exception)
{
	/*
	 * Initialize vocabulary tables.
	 */

	prefixes.clear();
	namespace_names.clear();

	// Fast Infoset built-ins.
	prefixes.add("xml"); // 7.2.21
	namespace_names.add("http://www.w3.org/XML/1998/namespace"); // 7.2.22

	// Our namespace.  Since this will be the default namespace in
	// documents we create, we don't need (or want) to define a prefix.
	// TODO: We may want to point this at a document.
	NAMESPACE_IDX = namespace_names.add(NAMESPACE);
	if (NAMESPACE_IDX == FI_VOCAB_INDEX_NULL) {
		throw IllegalStateException ();
	}

	// TODO: Add initial names.
}

bool
Document::writeVocab(FI_OctetStream *stream) throw ()
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


//
// Document serialization subroutines.
//

namespace {

// Subsubroutine prototypes.
bool write_name_surrogate(FI_OctetStream *, const FI_NameSurrogate&) throw ();
bool write_length_sequence_of(FI_OctetStream *, FI_Length) throw ();
bool write_non_empty_string_bit_2(FI_OctetStream *, const CharString&) throw ();
bool write_non_zero_uint20_bit_2(FI_OctetStream *, FI_UInt20) throw ();

const char *
get_xml_decl(int version, int standalone)
{
	static const char *const xml_decl[] = {
		"<?xml encoding='finf'?>",
		"<?xml encoding='finf' standalone='no'?>",
		"<?xml encoding='finf' standalone='yes'?>",
		"<?xml version='1.0' encoding='finf'?>",
		"<?xml version='1.0' encoding='finf' standalone='no'?>",
		"<?xml version='1.0' encoding='finf' standalone='yes'?>",
		"<?xml version='1.1' encoding='finf'?>",
		"<?xml version='1.1' encoding='finf' standalone='no'?>",
		"<?xml version='1.1' encoding='finf' standalone='yes'?>"
	};

	return xml_decl[3 * version + standalone];
}

bool
write_header(FI_OctetStream *stream) throw ()
{
	// Decide which XML declaration to use.
	const char *xml_decl = get_xml_decl(0 /* no version */,
	                                    0 /* no standalone */);

	size_t xml_decl_len = strlen(xml_decl);

	// Reserve space.
	FI_Octet *w_buf = fi_get_stream_write_buffer(stream, xml_decl_len + 5);
	if (!w_buf) {
		return false;
	}

	// Write XML declaration.
	memcpy(w_buf, xml_decl, xml_decl_len);
	w_buf += xml_decl_len;

	// Write identification (12.6: 11100000 00000000).
	w_buf[0] = FI_BIT_1 | FI_BIT_2 | FI_BIT_3;
	w_buf[1] = 0x00;

	// Write Fast Infoset version number (12.7: 00000000 00000001).
	w_buf[2] = 0x00;
	w_buf[3] = FI_BIT_8;

	// Write padding (12.8: 0).
	// Write optional component presence flags (C.2.3: 0100000).
	w_buf[4] = FI_BIT_3 /* initial-vocabulary present */;

	return true;
}

bool
write_trailer(FI_OctetStream *stream) throw ()
{
	FI_Octet *w_buf;

	// Serialize document trailer.
	switch (fi_get_stream_num_bits(stream)) {
	case 0:
		// Write termination (C.2.12: 1111).
		// 12.11: Ended on the 4th bit of an octet.
		w_buf = fi_get_stream_write_buffer(stream, 1);
		if (!w_buf) {
			return false;
		}

		w_buf[0] = FI_BIT_1 | FI_BIT_2 | FI_BIT_3 | FI_BIT_4;
		break;

	case 4:
		// Write termination (C.2.12: 1111).
		// 12.11: Ended on the 8th bit of an octet.
		if (!fi_write_stream_bits(stream, 4, FI_BIT_1 | FI_BIT_2
		                                     | FI_BIT_3 | FI_BIT_4)) {
			return false;
		}
		break;

	default:
		// FIXME: For debugging only.
		if (!fi_write_stream_bits(stream,
		                          8 - fi_get_stream_num_bits(stream),
		                          0xFF)) {
			return false;
		}
		break;
	}

	return true;
}

// C.2.5.3: Identifier string vocabulary tables.
bool
write_ds_table(FI_OctetStream *stream, const DS_VocabTable& vocab) throw ()
{
	// Write string count (C.21).
	if (!write_length_sequence_of(stream,
	                              vocab.size() - vocab.last_builtin)) {
		return false;
	}

	// Write items.
	for (VocabIndex ii = vocab.first_added; ii <= vocab.size(); ii++) {
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
write_dn_table(FI_OctetStream *stream, const DN_VocabTable& vocab) throw ()
{
	// Write name surrogate count (C.21).
	if (!write_length_sequence_of(stream, vocab.size())) {
		return false;
	}

	// Write items.
	for (VocabIndex ii = 1; ii <= vocab.size(); ii++) {
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


//
// Document serialization subsubroutines.
// TODO: These are probably useful for more than just the Document type, and
// probably need to be moved into their own module.
// TODO: Although the spec talks about bits, we can work just in octets, but we
// may need to pass around the values of some bits if they're not jsut padding.
//

#include <cassert>

// C.16
bool
write_name_surrogate(FI_OctetStream *stream, const FI_NameSurrogate& name)
                    throw ()
{
	assert(fi_get_stream_num_bits(stream) == 6); // C.16.2

	FI_Octet presence_flags = 0;

	// Set prefix-string-index presence flag (C.16.3).
	if (name.prefix_idx) {
		presence_flags |= FI_BIT_1;
	}

	// Set namespace-name-string-index presence flag (C.16.4).
	if (name.namespace_idx) {
		presence_flags |= FI_BIT_2;
	}

	if (!fi_write_stream_bits(stream, 2, presence_flags)) {
		return false;
	}

	// If prefix-string-index, '0' + C.25 (C.16.5).
	if (name.prefix_idx) {
		if (!fi_write_stream_bits(stream, 1, 0)) {
			return false;
		}

		if (!write_non_zero_uint20_bit_2(stream, name.prefix_idx)) {
			return false;
		}
	}

	// If namespace-name-string-index, '0' + C.25 (C.16.6).
	if (name.namespace_idx) {
		if (!fi_write_stream_bits(stream, 1, 0)) {
			return false;
		}

		if (!write_non_zero_uint20_bit_2(stream, name.namespace_idx)) {
			return false;
		}
	}

	// For local-name-string-index, '0' + C.25 (C.16.7).
	if (!fi_write_stream_bits(stream, 1, 0)) {
		return false;
	}

	if (!write_non_zero_uint20_bit_2(stream, name.local_idx)) {
		return false;
	}

	return true;
}

// C.21
bool
write_length_sequence_of(FI_OctetStream *stream, FI_Length len) throw ()
{
	assert(len > 0 && len <= FI_ONE_MEG);
	assert(fi_get_stream_num_bits(stream) == 0); // C.21.1

	FI_Octet *w_buf;

	if (len <= 128) {
		// [1,128] (C.21.2)
		w_buf = fi_get_stream_write_buffer(stream, 1);
		if (!w_buf) {
			return false;
		}

		w_buf[0] = len - 1;
	} else {
		// [129,2^20] (C.21.3)
		w_buf = fi_get_stream_write_buffer(stream, 3);
		if (!w_buf) {
			return false;
		}

		len -= 129;

		w_buf[0] = FI_BIT_1 /* 1000 */ | (len >> 16);
		w_buf[1] = len >> 8;
		w_buf[2] = len;
	}

	return true;
}

// C.22
bool
write_non_empty_string_bit_2(FI_OctetStream *stream, const CharString& str)
                            throw ()
{
	assert(str.size() > 0 && str.size() <= FI_FOUR_GIG);
	assert(fi_get_stream_num_bits(stream) == 1);  // C.22.1

	// Write string length (C.22.3).
	FI_Length len;
	FI_Octet *w_buf;

	if (str.size() <= 64) {
		// [1,64] (C.22.3.1)
		len = str.size() - 1;

		if (!fi_write_stream_bits(stream, 7, len << 1)) {
			return false;
		}
	} else if (str.size() <= 320) {
		// [65,320] (C.22.3.2)
		len = str.size() - 65;

		if (!fi_write_stream_bits(stream, 7, FI_BIT_1 /* 10 00000 */)) {
			return false;
		}

		w_buf = fi_get_stream_write_buffer(stream, 1);
		if (!w_buf) {
			return false;
		}

		w_buf[0] = len;
	} else {
		// [321,2^32] (C.22.3.3)
		len = str.size() - 321;

		if (!fi_write_stream_bits(stream, 7,
		                          FI_BIT_1 | FI_BIT_2 /* 11 00000 */)) {
			return false;
		}

		w_buf = fi_get_stream_write_buffer(stream, 4);
		if (!w_buf) {
			return false;
		}

		w_buf[0] = len >> 24;
		w_buf[1] = len >> 16;
		w_buf[2] = len >> 8;
		w_buf[3] = len;
	}

	// Write string octets (C.22.4).
	w_buf = fi_get_stream_write_buffer(stream, str.size());
	if (!w_buf) {
		return false;
	}

	str.copy(reinterpret_cast<FI_Char *>(w_buf), str.size());

	return true;
}

// C.25
bool
write_non_zero_uint20_bit_2(FI_OctetStream *stream, FI_UInt20 val) throw ()
{
	assert(val > 0 && val <= FI_ONE_MEG);
	assert(fi_get_stream_num_bits(stream) == 1); // C.25.1

	FI_Octet *w_buf;

	if (val <= 64) {
		// [1,64] (C.25.2)
		val -= 1;

		if (!fi_write_stream_bits(stream, 7, val << 1)) {
			return false;
		}
	} else if (val <= 8256) {
		// [65,8256] (C.25.3)
		val -= 65;

		if (!fi_write_stream_bits(stream, 7, FI_BIT_1 | val >> 7)) {
			return false;
		}

		w_buf = fi_get_stream_write_buffer(stream, 1);
		if (!w_buf) {
			return false;
		}

		w_buf[0] = val;
	} else {
		// [8257,2^20] (C.25.4)
		val -= 8257;

		if (!fi_write_stream_bits(stream, 7,
		                          FI_BIT_1 | FI_BIT_2 | val >> 15)) {
			return false;
		}

		w_buf = fi_get_stream_write_buffer(stream, 2);
		if (!w_buf) {
			return false;
		}

		w_buf[0] = val >> 8;
		w_buf[1] = val;
	}

	return true;
}

} // anonymous namespace

} // namespace FI
} // namespace BTech
