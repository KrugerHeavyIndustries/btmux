/*
 * Utility routines for Fast Infoset encoding/decoding.
 */

#include "autoconf.h"

#include <string>
#include <cassert>

#include "stream.h"

#include "Name.hh"
#include "Value.hh"
#include "Vocabulary.hh"

#include "encutil.hh"


namespace BTech {
namespace FI {

//
// TODO: Although the spec talks about bits, with some careful analysis, we can
// work just in octets, but we may need to pass around the values of some bits
// if they're not just padding.
//

// C.4
// XXX: This only encodes values as literals, not by index.
bool
write_attribute(FI_OctetStream *stream,
                const DN_VocabTable::TypedEntryRef& name, const Value& value)
{
	assert(fi_get_stream_num_bits(stream) == 1); // C.4.2

	// Write qualified-name using C.17 (C.4.3).
	if (!write_name_bit_2(stream, name)) {
		return false;
	}

	// Write normalized-value using C.14 (C.4.4).
	if (!write_value_bit_1(stream, value)) {
		return false;
	}

	return true;
}

// C.12
// TODO: Fix this to work with arbitrary namespace attributes, not just the
// default BT_NAMESPACE.
bool
write_namespace_attribute(FI_OctetStream *stream,
                          const NSN_DS_VocabTable::TypedEntryRef& ns_name)
{
	assert(fi_get_stream_num_bits(stream) == 6); // C.12.2

	// No prefix (C.12.3: 0).
	// namespace-name (C.12.4: 1).
	if (!fi_write_stream_bits(stream, 2, FI_BITS(0,1,,,,,,))) {
		return false;
	}

	// No prefix to encode using C.13 (C.12.5).
	// namespace-name encoded using C.13 (C.12.6).
	if (!write_identifier(stream, ns_name)) {
		return false;
	}

	return true;
}

// C.13
bool
write_identifier(FI_OctetStream *stream,
                 const DS_VocabTable::TypedEntryRef& id_str)
{
	assert(fi_get_stream_num_bits(stream) == 0); // C.13.2

	const bool has_idx = id_str.hasIndex();

	if (has_idx) {
		const FI_VocabIndex idx = id_str.getIndex();
		if (idx != FI_VOCAB_INDEX_NULL) {
			// Use index rules (C.13.4).
			// Write '1' + C.25.
			if (!fi_write_stream_bits(stream, 1, FI_BIT_1)) {
				return false;
			}

			if (!write_pint20_bit_2(stream, FI_UINT_TO_PINT(idx))) {
				return false;
			}

			return true;
		}
	}

	// Use literal rules (C.13.3).
	const CharString& value = id_str.getValue();

	// Write '0' + C.22.
	if (!fi_write_stream_bits(stream, 1, 0)) {
		return false;
	}

	const FI_Length buf_len = value.size();

	if (buf_len < 1) {
		return false;
	}

	const FI_PInt32 len = FI_UINT_TO_PINT(buf_len);

	FI_Octet *w_buf = write_non_empty_octets_bit_2(stream, len);
	if (!w_buf) {
		return false;
	}

	memcpy(w_buf, value.data(), buf_len);

	// Enter identifying string into vocabulary table (7.13.7).
	if (!has_idx) {
		id_str.getIndex();
	}

	return true;
}

// C.14
// TODO: This only encodes values as literals, not by index.
bool
write_value_bit_1(FI_OctetStream *stream, const Value& value)
{
	assert(fi_get_stream_num_bits(stream) == 0); // C.14.2

	// Write literal-character-string discriminant (C.14.3: 0); the
	// string-index alternative is not currently supported (and thus
	// add-to-table will also be FALSE).
	// Write add-to-table (C.14.3.1: 0).
	if (!fi_write_stream_bits(stream, 2, FI_BITS(0, 0,,,,,,))) {
		return false;
	}

	// Write character-string using C.19 (C.14.3.2).
	if (!write_encoded_bit_3(stream, value)) {
		return false;
	}

	return true;
}

#if 0 // defined(FI_USE_INITIAL_VOCABULARY)
// C.16
bool
write_name_surrogate(FI_OctetStream *stream, const VocabTable::EntryRef& name)
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
#endif // 0

// C.17
bool
write_name_bit_2(FI_OctetStream *stream,
                 const DN_VocabTable::TypedEntryRef& name)
{
	assert(fi_get_stream_num_bits(stream) == 1); // C.17.2

	const bool has_idx = name.hasIndex();

	if (has_idx) {
		const FI_VocabIndex idx = name.getIndex();
		if (idx != FI_VOCAB_INDEX_NULL) {
			// Use name-surrogate-index (C.17.4).
			// Write name-surrogate-index using C.25.
			if (!write_pint20_bit_2(stream, FI_UINT_TO_PINT(idx))) {
				return false;
			}

			return true;
		}
	}

	// Use literal-qualified-name (C.17.3).
	const Name& literal = name.getValue();

	// Write identification (C.17.3: 1111 0).
	// Write prefix and namespace-name presence (C.17.3.1).
	const bool has_pfx = literal.pfx_part.isValid();
	const bool has_nsn = literal.nsn_part.isValid();
	if (!fi_write_stream_bits(stream, 7,
	                          FI_BITS(1,1,1,1, 0, has_pfx, has_nsn,))) {
		return false;
	}

	// Write optional prefix using C.13 (C.17.3.2).
	if (has_pfx && !write_identifier(stream, literal.pfx_part)) {
		return false;
	}

	// Write optional namespace-name using C.13 (C.17.3.3).
	if (has_nsn && !write_identifier(stream, literal.nsn_part)) {
		return false;
	}

	// Write local-name using C.13 (C.17.3.4).
	if (!write_identifier(stream, literal.local_part)) {
		return false;
	}

	// Enter name surrogate into vocabulary table (7.16.7.5).
	if (!has_idx) {
		name.getIndex();
	}

	return true;
}

// C.18
bool
write_name_bit_3(FI_OctetStream *stream,
                 const DN_VocabTable::TypedEntryRef& name)
{
	assert(fi_get_stream_num_bits(stream) == 2); // C.18.2

	const bool has_idx = name.hasIndex();

	if (has_idx) {
		const FI_VocabIndex idx = name.getIndex();
		if (idx != FI_VOCAB_INDEX_NULL) {
			// Use name-surrogate-index (C.18.4).
			// Write name-surrogate-index using C.27.
			if (!write_pint20_bit_3(stream, FI_UINT_TO_PINT(idx))) {
				return false;
			}

			return true;
		}
	}

	// Use literal-qualified-name (C.18.3).
	const Name& literal = name.getValue();

	// Write identification (C.18.3: 1111).
	// Write prefix and namespace-name presence (C.18.3.1).
	const bool has_pfx = literal.pfx_part.isValid();
	const bool has_nsn = literal.nsn_part.isValid();
	if (!fi_write_stream_bits(stream, 6,
	                          FI_BITS(1,1,1,1, has_pfx, has_nsn,,))) {
		return false;
	}

	// Write optional prefix using C.13 (C.18.3.2).
	if (has_pfx && !write_identifier(stream, literal.pfx_part)) {
		return false;
	}

	// Write optional namespace-name using C.13 (C.18.3.3).
	if (has_nsn && !write_identifier(stream, literal.nsn_part)) {
		return false;
	}

	// Write local-name using C.13 (C.18.3.4).
	if (!write_identifier(stream, literal.local_part)) {
		return false;
	}

	// Enter name surrogate into vocabulary table (7.16.7.5).
	if (!has_idx) {
		name.getIndex();
	}

	return true;
}

bool
read_name_bit_3(FI_OctetStream *stream,
                const DN_VocabTable::TypedEntryRef& name,
                FI_Length& r_len_state)
{
	// FIXME
	return false;
}

// C.19
bool
write_encoded_bit_3(FI_OctetStream *stream, const Value& value)
{
	assert(fi_get_stream_num_bits(stream) == 2); // C.19.2

	// Decide which encoding to use based on value type.
	enum {
		ENCODE_AS_UTF8        = FI_BITS(0,0,,,,,,), // C.19.3.1
		ENCODE_AS_UTF16       = FI_BITS(0,1,,,,,,), // C.19.3.2
		ENCODE_WITH_ALPHABET  = FI_BITS(1,0,,,,,,), // C.19.3.3
		ENCODE_WITH_ALGORITHM = FI_BITS(1,1,,,,,,)  // C.19.3.4
	} mode;

	switch (value.getType()) {
	case FI_VALUE_AS_OCTETS:
		// Encode as octets of UTF-8.
		// FIXME: This needs to be actual UTF-8, not just arbitrary
		// octets.  Arbitrary octets will require an encoding algorithm
		// of their own.
		mode = ENCODE_AS_UTF8;
		break;

	default:
		// Unsupported value type.
		return false;
	}

	// Apply the encoding.
	FI_Length buf_len;
	FI_PInt32 len;
	FI_Octet *w_buf;

	switch (mode) {
	case ENCODE_AS_UTF8:
		// Write discriminant (C.19.3.1).
		if (!fi_write_stream_bits(stream, 2, ENCODE_AS_UTF8)) {
			return false;
		}

		// Write octets using C.23 (C.19.4).
		buf_len = value.getCount();

		if (buf_len < 1) {
			return false;
		}

		len = FI_UINT_TO_PINT(buf_len);

		w_buf = write_non_empty_octets_bit_5(stream, len);
		if (!w_buf) {
			return false;
		}

		memcpy(w_buf, value.getValue(), buf_len);
		break;

	case ENCODE_AS_UTF16:
		// TODO: We don't use the utf-16 alternative.
		// Write discriminant (C.19.3.2).
		// Write octets using C.23 (C.19.4).
		return false;

	case ENCODE_WITH_ALPHABET:
		// TODO: We don't use the restricted-alphabet alternative.
		// Write discriminant (C.19.3.3).
		// Write restricted-alphabet index using C.29 (C.19.3.3).
		// Write octets using C.23 (C.19.4).
		return false;

	case ENCODE_WITH_ALGORITHM:
		// Write discriminant (C.19.3.4).
		if (!fi_write_stream_bits(stream, 2, ENCODE_WITH_ALGORITHM)) {
			return false;
		}

		// Write encoding-algorithm index using C.29 (C.19.3.4).
		// Write octets using C.23 (C.19.4).
		// TODO: We don't support encoding algorithms yet.
		return false;

	default:
		// Should never happen.
		return false;
	}

	return true;
}

#if 0 // defined(FI_USE_INITIAL_VOCABULARY)
// C.21
bool
write_length_sequence_of(FI_OctetStream *stream, FI_PInt20 len)
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

		w_buf[0] = FI_BITS(1,0,0,0,,,,) | (len >> 16);
		w_buf[1] = len >> 8;
		w_buf[2] = len;
	}

	return true;
}
#endif // FI_USE_INITIAL_VOCABULARY

// C.22
// Note that this doesn't actually write the octets, but leaves that up to the
// caller to do in the buffer provided.
FI_Octet *
write_non_empty_octets_bit_2(FI_OctetStream *stream, FI_PInt32 len)
{
	assert(len <= FI_PINT32_MAX);
	assert(fi_get_stream_num_bits(stream) == 1);  // C.22.2

	// Write string length (C.22.3).
	if (len > FI_UINT_TO_PINT(FI_LENGTH_MAX)) {
		// Will overflow.
		return 0;
	}

	const FI_Length buf_len = FI_PINT_TO_UINT(len);

	FI_Octet *w_buf;

	if (len <= FI_UINT_TO_PINT(64)) {
		// [1,64] (C.22.3.1)
		len -= FI_UINT_TO_PINT(1);

		if (!fi_write_stream_bits(stream, 7,
		                          FI_BITS(0,,,,,,,) | len << 1)) {
			return 0;
		}
	} else if (len <= FI_UINT_TO_PINT(320)) {
		// [65,320] (C.22.3.2)
		len -= FI_UINT_TO_PINT(65);

		if (!fi_write_stream_bits(stream, 7,
		                          FI_BITS(1,0, 0,0,0,0,0,))) {
			return 0;
		}

		w_buf = fi_get_stream_write_buffer(stream, 1);
		if (!w_buf) {
			return 0;
		}

		w_buf[0] = len;
	} else {
		// [321,2^32] (C.22.3.3)
		len -= FI_UINT_TO_PINT(321);

		if (!fi_write_stream_bits(stream, 7,
		                          FI_BITS(1,1, 0,0,0,0,0,))) {
			return 0;
		}

		w_buf = fi_get_stream_write_buffer(stream, 4);
		if (!w_buf) {
			return 0;
		}

		w_buf[0] = len >> 24;
		w_buf[1] = len >> 16;
		w_buf[2] = len >> 8;
		w_buf[3] = len;
	}

	// Write string octets (C.22.4). (Or rather, provide the buffer to.)
	return fi_get_stream_write_buffer(stream, buf_len);
}

// C.23
// Note that this doesn't actually write the octets, but leaves that up to the
// caller to do in the buffer provided.
FI_Octet *
write_non_empty_octets_bit_5(FI_OctetStream *stream, FI_PInt32 len)
{
	assert(len <= FI_PINT32_MAX);
	assert(fi_get_stream_num_bits(stream) == 4);  // C.23.2

	// Write string length (C.23.3).
	if (len > FI_UINT_TO_PINT(FI_LENGTH_MAX)) {
		// Will overflow.
		return 0;
	}

	const FI_Length buf_len = FI_PINT_TO_UINT(len);

	FI_Octet *w_buf;

	if (len <= FI_UINT_TO_PINT(8)) {
		// [1,8] (C.23.3.1)
		len -= FI_UINT_TO_PINT(1);

		if (!fi_write_stream_bits(stream, 4,
		                          FI_BITS(0,,,,,,,) | len << 4)) {
			return 0;
		}
	} else if (len <= FI_UINT_TO_PINT(264)) {
		// [9,264] (C.23.3.2)
		len -= FI_UINT_TO_PINT(9);

		if (!fi_write_stream_bits(stream, 4, FI_BITS(1,0, 0,0,,,,))) {
			return 0;
		}

		w_buf = fi_get_stream_write_buffer(stream, 1);
		if (!w_buf) {
			return 0;
		}

		w_buf[0] = len;
	} else {
		// [265,2^32] (C.23.3.3)
		len -= FI_UINT_TO_PINT(265);

		if (!fi_write_stream_bits(stream, 4, FI_BITS(1,1, 0,0,,,,))) {
			return 0;
		}

		w_buf = fi_get_stream_write_buffer(stream, 4);
		if (!w_buf) {
			return 0;
		}

		w_buf[0] = len >> 24;
		w_buf[1] = len >> 16;
		w_buf[2] = len >> 8;
		w_buf[3] = len;
	}

	// Write string octets (C.23.4). (Or rather, provide the buffer to.)
	return fi_get_stream_write_buffer(stream, buf_len);
}

// C.25
bool
write_pint20_bit_2(FI_OctetStream *stream, FI_PInt20 val)
{
	assert(val <= FI_PINT20_MAX);
	assert(fi_get_stream_num_bits(stream) == 1); // C.25.1

	FI_Octet *w_buf;

	if (val <= FI_UINT_TO_PINT(64)) {
		// [1,64] (C.25.2)
		val -= FI_UINT_TO_PINT(1);

		if (!fi_write_stream_bits(stream, 7,
		                          FI_BITS(0,,,,,,,) | val << 1)) {
			return false;
		}
	} else if (val <= FI_UINT_TO_PINT(8256)) {
		// [65,8256] (C.25.3)
		val -= FI_UINT_TO_PINT(65);

		if (!fi_write_stream_bits(stream, 7,
		                          FI_BITS(1,0,,,,,,) | val >> 7)) {
			return false;
		}

		w_buf = fi_get_stream_write_buffer(stream, 1);
		if (!w_buf) {
			return false;
		}

		w_buf[0] = val;
	} else {
		// [8257,2^20] (C.25.4)
		val -= FI_UINT_TO_PINT(8257);

		if (!fi_write_stream_bits(stream, 7,
		                          FI_BITS(1,1,,,,,,) | val >> 15)) {
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

// C.27
bool
write_pint20_bit_3(FI_OctetStream *stream, FI_PInt20 val)
{
	assert(val <= FI_PINT20_MAX);
	assert(fi_get_stream_num_bits(stream) == 2); // C.27.1

	FI_Octet *w_buf;

	if (val <= FI_UINT_TO_PINT(32)) {
		// [1,32] (C.27.2)
		val -= FI_UINT_TO_PINT(1);

		if (!fi_write_stream_bits(stream, 6,
		                          FI_BITS(0,,,,,,,) | val << 2)) {
			return false;
		}
	} else if (val <= FI_UINT_TO_PINT(2080)) {
		// [33,2080] (C.27.3)
		val -= FI_UINT_TO_PINT(33);

		if (!fi_write_stream_bits(stream, 6,
		                          FI_BITS(1,0,0,,,,,) | val >> 6)) {
			return false;
		}

		w_buf = fi_get_stream_write_buffer(stream, 1);
		if (!w_buf) {
			return false;
		}

		w_buf[0] = val;
	} else if (val <= FI_UINT_TO_PINT(526368)) {
		// [2081,526368] (C.27.4)
		val -= FI_UINT_TO_PINT(2081);

		if (!fi_write_stream_bits(stream, 6,
		                          FI_BITS(1,0,1,,,,,) | val >> 14)) {
			return false;
		}

		w_buf = fi_get_stream_write_buffer(stream, 2);
		if (!w_buf) {
			return false;
		}

		w_buf[0] = val >> 8;
		w_buf[1] = val;
	} else {
		// [526369,2^20] (C.27.5)
		val -= FI_UINT_TO_PINT(526369);

		if (!fi_write_stream_bits(stream, 6, FI_BITS(1,1,0,0,0,0,,))) {
			return false;
		}

		w_buf = fi_get_stream_write_buffer(stream, 3);
		if (!w_buf) {
			return false;
		}

		// Padding '0000'
		w_buf[0] = val >> 16;
		w_buf[1] = val >> 8;
		w_buf[2] = val;
	}

	return true;
}

// C.29
bool
write_pint8(FI_OctetStream *stream, FI_PInt8 val)
{
	assert(val <= FI_PINT8_MAX);
	assert(fi_get_stream_num_bits(stream) == 4
	       || fi_get_stream_num_bits(stream) == 6); // C.29.1

	// [1,256] (C.29.2)
	val -= FI_UINT_TO_PINT(1);

	return fi_write_stream_bits(stream, 8, val);
}

} // namespace FI
} // namespace BTech
