/*
 * Utility routines for Fast Infoset encoding/decoding.
 */

#include "autoconf.h"

#include <string>
#include <cassert>

#include "stream.h"

#include "Name.hh"

#include "encutil.hh"


namespace BTech {
namespace FI {

//
// TODO: Although the spec talks about bits, we can work just in octets, but we
// may need to pass around the values of some bits if they're not jsut padding.
//

// C.12
// FIXME: The namespace index is currently hardcoded to 2, since we only use
// this to encode our default namespace index on the document element.
bool
write_namespace_attribute(FI_OctetStream *stream) throw ()
{
	assert(fi_get_stream_num_bits(stream) == 6); // C.12.2

	// No prefix (C.12.3: 0).
	// namespace-name (C.12.4: 1).
	if (!fi_write_stream_bits(stream, 2, FI_BITS(0,1,,,,,,))) {
		return false;
	}

	// No prefix to encode using C.13 (C.12.5).
	// namespace-name encoded using C.13 (C.12.6).
	const FI_VocabIndex namespace_idx = 2;

	Name namespace_name;
	if (!namespace_name.setName(FI_NAME_AS_INDEX, &namespace_idx)) {
		return false;
	}

	if (!write_identifier(stream, namespace_name)) {
		return false;
	}

	return true;
}

// C.13
bool
write_identifier(FI_OctetStream *stream, const Name& id) throw ()
{
	assert(fi_get_stream_num_bits(stream) == 0); // C.13.2

	FI_VocabIndex idx;

	switch (id.getType()) {
	case FI_NAME_AS_INDEX:
		// Write '1' + C.25 (C.13.4).
		idx = *reinterpret_cast<const FI_VocabIndex *>(id.getName());

		if (!fi_write_stream_bits(stream, 1, FI_BIT_1)) {
			return false;
		}

		if (!write_non_zero_uint20_bit_2(stream, idx)) {
			return false;
		}
		break;

	default:
		// FIXME: Use '0' + C.22 (C.13.3).
		return false;
	}

	return true;
}

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

// C.18
bool
write_name_bit_3(FI_OctetStream *stream, const Name& name) throw ()
{
	assert(fi_get_stream_num_bits(stream) == 2); // C.18.2

	FI_VocabIndex idx;

	switch (name.getType()) {
	case FI_NAME_AS_INDEX:
		// Write name-surrogate-index using C.27 (C.18.4).
		idx = *reinterpret_cast<const FI_VocabIndex *>(name.getName());
		if (!write_non_zero_uint20_bit_3(stream, idx)) {
			return false;
		}
		break;

	default: // FIXME: support other types
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

// C.27
bool
write_non_zero_uint20_bit_3(FI_OctetStream *stream, FI_UInt20 val) throw ()
{
	assert(val > 0 && val <= FI_ONE_MEG);
	assert(fi_get_stream_num_bits(stream) == 2); // C.27.1

	FI_Octet *w_buf;

	if (val <= 32) {
		// [1,32] (C.27.2)
		val -= 1;

		if (!fi_write_stream_bits(stream, 6, val << 2)) {
			return false;
		}
	} else if (val <= 2080) {
		// [33,2080] (C.27.3)
		val -= 33;

		if (!fi_write_stream_bits(stream, 6,
		                          FI_BITS(1,0,0,,,,,) | val >> 6)) {
			return false;
		}

		w_buf = fi_get_stream_write_buffer(stream, 1);
		if (!w_buf) {
			return false;
		}

		w_buf[0] = val;
	} else if (val <= 526368) {
		// [2081,526368] (C.27.4)
		val -= 2081;

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
		val -= 526369;

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

} // namespace FI
} // namespace BTech
