/*
 * Dynamically-castable Value class implementation.
 */
#include "autoconf.h"

#include <cstddef>
#include <cstring>
#include <cassert>

#include <memory>

#include "values.h"

#include "Codec.hh"

#include "Value.hh"
#include "Serializable.hh"

// Use auto_ptr to avoid memory leak warnings from valgrind & friends.
using std::auto_ptr;


namespace BTech {
namespace FI {

/*
 * Dynamic value table.  Holds pre-decoded values from parsing. (We don't
 * support writing values by index, at least at this time.)
 *
 * ATTRIBUTE VALUE
 * CONTENT CHARACTER CHUNK
 * OTHER STRING
 *
 * In the standard, this is just a dynamic string table (8.4).
 */

DV_VocabTable::TypedVocabTable *
DV_VocabTable::builtin_table()
{
	static DV_VocabTable builtins (true, FI_ONE_MEG);
	return &builtins;
}

DV_VocabTable::DV_VocabTable(bool read_only, FI_VocabIndex max_idx)
: DynamicTypedVocabTable<value_type> (true, max_idx)
{
	// Index 0 is the empty value.
	base_idx = 0;
	last_idx = -1;

	// Add built-in values.
	static auto_ptr<Entry>
	entry_0 (addStaticEntry (FI_VOCAB_INDEX_NULL, Value ()));
}

DV_VocabTable::DV_VocabTable()
: DynamicTypedVocabTable<value_type> (false, builtin_table())
{
}


/*
 * Dynamically-typed Value implementation.
 */

namespace {

// Get the size of a type dynamically.
size_t
get_size_of(FI_ValueType value_type)
{
	switch (value_type) {
	case FI_VALUE_AS_NULL:
		return 0;

	case FI_VALUE_AS_SHORT:
		return sizeof(FI_Int16);

	case FI_VALUE_AS_INT:
		return sizeof(FI_Int32);

	case FI_VALUE_AS_LONG:
		return sizeof(FI_Int64);

	case FI_VALUE_AS_BOOLEAN:
		return sizeof(FI_Boolean);

	case FI_VALUE_AS_FLOAT:
		return sizeof(FI_Float32);

	case FI_VALUE_AS_DOUBLE:
		return sizeof(FI_Float64);

	case FI_VALUE_AS_UUID:
		return sizeof(FI_UUID);

	case FI_VALUE_AS_UTF8:
		return sizeof(FI_UInt8);

	case FI_VALUE_AS_UTF16:
		return sizeof(FI_UInt16);

	case FI_VALUE_AS_OCTETS:
		return sizeof(FI_Octet);

	default:
		// Don't understand that value type.
		throw InvalidArgumentException ();
	}
}

void
pow2_ceil_down(size_t& pow2_size, size_t exact_size)
{
	size_t tmp_pow2_size = pow2_size >> 1;

	while (tmp_pow2_size > exact_size) {
		pow2_size = tmp_pow2_size;
		tmp_pow2_size >>= 1;
	}
}

bool
pow2_ceil_up(size_t& pow2_size, size_t exact_size)
{
	while (pow2_size < exact_size) {
		const size_t tmp_pow2_size = pow2_size << 1;

		if (tmp_pow2_size <= pow2_size) {
			// Would overflow.
			return false;
		}

		pow2_size = tmp_pow2_size;
	}

	return true;
}

} // anonymous namespace

Value::~Value()
{
	delete[] static_cast<char *>(value_buf);
}

Value::Value (const Value& src)
: value_type (src.value_type), value_count (src.value_count),
  value_size (src.value_size), value_buf_size (src.value_buf_size)
{
	if (value_type == FI_VALUE_AS_NULL) {
		// Empty buffer.
		value_buf = 0;
		return;
	}

	// Instead of re-throwing our own Exception, maybe just pass the
	// std::bad_alloc up? Either that, or we have to guarantee that we
	// never throw unexpected exceptions.
	try {
		value_buf = new char[value_buf_size];
	} catch (const std::bad_alloc& e) {
		throw OutOfMemoryException ();
	}

	memcpy(value_buf, src.value_buf, value_size);
}

Value&
Value::operator = (const Value& src)
{
	if (&src == this) {
		// Self-assignment, don't do any work.
		return *this;
	}

	if (src.value_type == FI_VALUE_AS_NULL) {
		allocateValueBuffer(0);
		return *this;
	}

	// Instead of re-throwing our own Exception, maybe just pass the
	// std::bad_alloc up? Either that, or we have to guarantee that we
	// never throw unexpected exceptions.
	try {
		allocateValueBuffer(src.value_buf_size);
	} catch (const std::bad_alloc& e) {
		throw OutOfMemoryException ();
	}

	value_type = src.value_type;
	value_count = src.value_count;
	value_size = src.value_size;

	memcpy(value_buf, src.value_buf, value_size);
	return *this;
}

// (Re)allocate value buffer.  The existing value will be deallocated.  Note
// that constructors/destructors will not be called; use POD types only.
//
// With some casting/templates/massive switch statements/polymorphism, we could
// support arbitrary types, but that's a heavyweight solution for our needs,
// which is essentially a dynamically "typed" bag of bytes.
void
Value::allocateValueBuffer(size_t new_buf_size)
{
	if (new_buf_size == 0) {
		// Null assignment.
		value_type = FI_VALUE_AS_NULL;
		value_count = 0;
		value_size = 0;

		delete[] static_cast<char *>(value_buf);
		value_buf = 0;
		value_buf_size = 0;
		return;
	}

	// Conditionally replace existing buffer.
	if (new_buf_size != value_buf_size) {
		char *new_buf = new char[new_buf_size];

		delete[] static_cast<char *>(value_buf);
		value_buf = new_buf;
		value_buf_size = new_buf_size;
	}
}

bool
Value::setBufferType(FI_ValueType type, size_t count)
{
	// Handle assignment of empty value.
	// We don't require type == FI_VALUE_AS_NULL, as a convenience.
	if (count < 1 || type == FI_VALUE_AS_NULL) {
		allocateValueBuffer(0);
		return true;
	}

	// Compute value size.
	const size_t item_size = get_size_of(type);

	if (count > ((size_t)-1) / item_size) {
		// Would overflow.
		return false;
	}

	const size_t exact_size = count * item_size;
	assert(exact_size > 0);

	// Compute value buffer size.
	size_t pow2_size;

	if (value_buf) {
		pow2_size = value_buf_size >> 2;

		if (exact_size <= pow2_size) {
			// Adjust buffer size downward when it saves >75%.
			pow2_ceil_down(pow2_size, exact_size);
		} else {
			// Adjust buffer size upward by powers of 2.
			pow2_size = value_buf_size;

			if (!pow2_ceil_up(pow2_size, exact_size)) {
				// Would overflow.
				return false;
			}
		}
	} else {
		// Set initial buffer size as a power of 2.
		pow2_size = 1;

		if (!pow2_ceil_up(pow2_size, exact_size)) {
			// Would overflow.
			return false;
		}
	}

	// Update object state.
	try {
		allocateValueBuffer(pow2_size);
	} catch (const std::bad_alloc& e) {
		return false;
	}

	value_type = type;
	value_count = count;
	value_size = exact_size;
	return true;
}

// Set the value buffer from the contents of an external buffer.
void
Value::setBufferValue(const void *src)
{
	if (!value_buf) {
		// Not really needed, but okay.
		return;
	}

	memcpy(value_buf, src, value_size);
}

// Imposes an absolute ordering on all possible values.  Note that some values
// which would otherwise be equal may compare as non-equal; this operator is
// not generally useful, except to provide ordering for containers.
bool
Value::operator < (const Value& rhs) const
{
	// Values are directly comparable if they have the same type.  If they
	// have different types, then they are ordered by increasing value of
	// the FI_ValueType enumeration.
	if (value_type < rhs.value_type) {
		return true;
	} else if (rhs.value_type > value_type) {
		return false;
	}

	// All FI_VALUE_AS_NULL values compare as equal (not-less-than).
	if (value_type == FI_VALUE_AS_NULL) {
		assert(rhs.value_type == FI_VALUE_AS_NULL);
		return false;
	}

	assert(value_count > 0);

	// Values are directly comparable if they have the same count.  If they
	// have different counts, then they are ordered by increasing count.
	if (value_count < rhs.value_count) {
		return true;
	} else if (rhs.value_count < value_count) {
		return false;
	}

	assert(value_size == rhs.value_size);

	// Compare the values.  We consider two dynamic values the same only if
	// they have the exact same bits (to deal with things like multiple NaN
	// types in IEEE 754 floating point).  Note that this requires that
	// bitwise comparison be valid, usually appropriate only for POD.
	//
	// This ordering is really only used for caching identical Value
	// objects, so we don't really suffer a penalty if some values don't
	// compare equal, other than a slight increase in space usage.
	return (memcmp(value_buf, rhs.value_buf, value_size) < 0);
}


/*
 * Fast Infoset serialization support.
 */

namespace {

// Selecting the encoding format based on value type.
EncodingFormat
choose_enc_format(FI_ValueType value_type)
{
	switch (value_type) {
	case FI_VALUE_AS_UTF8:
		return ENCODE_AS_UTF8;

	case FI_VALUE_AS_UTF16:
		return ENCODE_AS_UTF16;

	default:
		// Try to encode everything else with an algorithm.
		return ENCODE_WITH_ALGORITHM;
	}
}

// Select the encoding algorithm index based on value type.
FI_VocabIndex
choose_enc_alg(FI_ValueType value_type)
{
	switch (value_type) {
	case FI_VALUE_AS_SHORT:
		return FI_EA_SHORT;

	case FI_VALUE_AS_INT:
		return FI_EA_INT;

	case FI_VALUE_AS_LONG:
		return FI_EA_LONG;

	case FI_VALUE_AS_BOOLEAN:
		return FI_EA_BOOLEAN;

	case FI_VALUE_AS_FLOAT:
		return FI_EA_FLOAT;

	case FI_VALUE_AS_DOUBLE:
		return FI_EA_DOUBLE;

	case FI_VALUE_AS_UUID:
		return FI_EA_UUID;

	case FI_VALUE_AS_OCTETS:
		return FI_EA_BASE64;

	default:
		// Couldn't select an appropriate algorithm.
		throw UnsupportedOperationException ();
	}
}

} // anonymous namespace

void
Value::setVocabTable(DV_VocabTable& new_vocab_table)
{
	vocab_table = &new_vocab_table;
}

void
Value::write(Encoder& encoder) const
{
	switch (encoder.getBitOffset()) {
	case 0:
		// Write value starting on first bit (C.14).
		write_bit1_or_3(encoder);
		break;

	case 2:
		// Write value starting on third bit (C.15).
		write_bit1_or_3(encoder, false);
		break;

	default:
		// Should never happen.
		throw AssertionFailureException ();
	}
}

bool
Value::read(Decoder& decoder)
{
reparse:
	switch (decoder.ext_super_step) {
	case 0:
		decoder.ext_sub_step = 0;

		switch (decoder.getBitOffset()) {
		case 0:
			// On first bit.
			break;

		case 2:
			// On third bit.
			decoder.ext_super_step = 2;
			goto reparse;

		default:
			// Should never happen.
			throw AssertionFailureException ();
		}

		decoder.ext_super_step = 1;
		// FALLTHROUGH

	case 1:
		// Read value header starting on first bit (C.14).
		if (!read_bit1_or_3(decoder)) {
			return false;
		}
		break;

	case 2:
		// Read value header starting on third bit (C.15).
		if (!read_bit1_or_3(decoder, false)) {
			return false;
		}
		break;

	default:
		// Should never happen.
		throw AssertionFailureException ();
	}

	decoder.ext_super_step = 0;
	return true;
}

void
Value::encodeOctets(Encoder& encoder, FI_Length len) const
{
	FI_Octet *w_buf = encoder.getWriteBuffer(len);

	const FI_EncodingAlgorithm *enc_alg;

	switch (choose_enc_format(value_type)) {
	case ENCODE_AS_UTF8:
		// TODO: This is a bit too liberal to be UTF-8.
		memcpy(w_buf, value_buf, len);
		break;

	case ENCODE_AS_UTF16:
	case ENCODE_WITH_ALPHABET:
		// FIXME: Implementation restriction.
		throw UnsupportedOperationException ();

	case ENCODE_WITH_ALGORITHM:
		// Use encoding algorithm.
		enc_alg = *encoder.encoding_algorithm;

		if (!enc_alg->encode(&encoder.encoding_context,
		                     w_buf, FI_Value::cast(*this))) {
			// FIXME: This Exception isn't really appropriate.
			throw UnsupportedOperationException ();
		}

		encoder.encoding_algorithm = 0;
		break;
	}
}

// XXX: If this routine fails, we guarantee the Value will be left in a
// consistent state, but it may lose its original value before decoding aborts.
// In that case, the Value will become an FI_VALUE_AS_NULL, though the caller
// still shouldn't try to make use of it, except to assign a new value.
bool
Value::decodeOctets(Decoder& decoder, FI_Length len)
{
	const FI_Octet *r_buf = decoder.getReadBuffer(len);
	if (!r_buf) {
		return false;
	}

	const FI_EncodingAlgorithm *enc_alg;

	switch (saved_format) {
	case ENCODE_AS_UTF8:
		// TODO: This is a bit too liberal to be UTF-8.
		if (!setBufferType(FI_VALUE_AS_UTF8, len)) {
			// FIXME: This Exception isn't really appropriate.
			throw UnsupportedOperationException ();
		}

		setBufferValue(r_buf);
		break;

	case ENCODE_AS_UTF16:
	case ENCODE_WITH_ALPHABET:
		// FIXME: Implementation restriction.
		throw UnsupportedOperationException ();

	case ENCODE_WITH_ALGORITHM:
		// Use encoding algorithm.
		enc_alg = *decoder.encoding_algorithm;

		if (!enc_alg->decode(&decoder.encoding_context,
		                     FI_Value::cast(*this), len, r_buf)) {
			// FIXME: This Exception isn't really appropriate.
			throw UnsupportedOperationException ();
		}
		break;
	}

	if (add_value_to_table) {
		vocab_table->getEntry(*this).getNewIndex();
	}

	return true;
}

// C.14/C.19, C.15/C.20
// Note that we only choose to encode by literal, not index, except for the
// empty string/value.  Also, we never request values be added to the table,
// although we do support adding during parsing.
void
Value::write_bit1_or_3(Encoder& encoder, bool is_bit1) const
{
	assert(!is_bit1 || encoder.getBitOffset() == 0); // C.14.2
	assert(is_bit1 || encoder.getBitOffset() == 2); // C.15.2

	if (value_type == FI_VALUE_AS_NULL) {
		// Use index rules (C.14.4, C.15.5).
		if (!is_bit1) {
			// C.15 doesn't allow index 0 (null character data).
			throw AssertionFailureException ();
		}

		// Write string-index discriminant (C.14.4, C.15.4: 1).
		encoder.writeBits(1, FI_BITS(1,,,,,,,));

		// Write string-index using C.26 (C.14.4), C.28 (C.15.4).
		encoder.writeUInt21_bit2(FI_VOCAB_INDEX_NULL);
		return;
	}

	// Use literal rules (C.14.3, C.15.3).

	// Write literal-character-string discriminant (C.14.3, C.15.3: 0).
	// Write add-to-table (C.14.3.1, C.15.3.1: 0). (Not currently used.)
	if (is_bit1) {
		encoder.writeBits(2, FI_BITS(0, 0,,,,,,));
	} else {
		encoder.writeBits(2, FI_BITS(,,0, 0,,,,));
	}

	// Write character-string using C.19 (C.14.3.2), C.20 (C.15.3.2).

	// Write discriminant (C.19.3, C.20.3).
	const EncodingFormat format = choose_enc_format(value_type);

	if (is_bit1) {
		encoder.writeBits(2, format);
	} else {
		encoder.writeBits(2, (format >> 2));
	}

	// Write encoded string length using C.23.3 (C.19.4), C.24.3 (C.20.4).
	// (And potentially the alphabet/algorithm index.)
	FI_Length len;

	FI_VocabIndex encoding_idx;
	const FI_EncodingAlgorithm *enc_alg;

	switch (format) {
	case ENCODE_AS_UTF8:
		if (value_count > FI_LENGTH_MAX / 1) {
			// Overflow.
			throw UnsupportedOperationException ();
		}

		encoder.encoding_algorithm = 0;

		len = value_count * 1;
		break;

	case ENCODE_AS_UTF16:
		// TODO: We don't use the utf-16 alternative.
		if (value_count > FI_LENGTH_MAX / 2) {
			// Overflow.
			throw UnsupportedOperationException ();
		}

		encoder.encoding_algorithm = 0;

		len = value_count * 2;
		throw UnsupportedOperationException ();

	case ENCODE_WITH_ALPHABET:
		// TODO: We don't use the restricted-alphabet alternative.
		// Write alphabet index using C.29 (C.19.3.3, C.20.3.3).
		throw UnsupportedOperationException ();

	case ENCODE_WITH_ALGORITHM:
		// Write algorithm index using C.29 (C.19.3.4, C.20.3.4).
		encoder.setEncodingAlgorithm(choose_enc_alg(value_type));

		enc_alg = *encoder.encoding_algorithm;

		if (!enc_alg->encoded_size(&encoder.encoding_context,
		                           FI_Value::cast(*this))) {
			// FIXME: This Exception isn't really appropriate.
			throw UnsupportedOperationException ();
		}

		len = encoder.encoding_context.encoded_size;

		encoding_idx = encoder.encoding_algorithm.getIndex();
		assert(encoding_idx > 0 && encoding_idx <= FI_PINT8_MAX);

		encoder.writePInt8(FI_UINT_TO_PINT(encoding_idx));
		break;
	}

	// Encode octets.
	assert(len > 0 && len <= FI_UINT32_MAX);

	if (is_bit1) {
		encoder.writeNonEmptyOctets_len_bit5(FI_UINT_TO_PINT(len));
	} else {
		encoder.writeNonEmptyOctets_len_bit7(FI_UINT_TO_PINT(len));
	}

	encodeOctets(encoder, len);
}

bool
Value::read_bit1_or_3(Decoder& decoder, bool is_bit1)
{
	FI_Octet bits;

	FI_VocabIndex idx;

	FI_PInt32 len;
	FI_PInt8 encoding_idx;

reparse:
	switch (decoder.ext_sub_step) {
	case 0:
		assert(!is_bit1 || decoder.getBitOffset() == 0); // C.14.2
		assert(is_bit1 || decoder.getBitOffset() == 2); // C.15.2

		// Examine discriminator bits.
		if (is_bit1) {
			if (!decoder.readBits(1)) {
				return false;
			}

			bits = decoder.getBits();
		} else {
			decoder.readBits(1);

			bits = decoder.getBits() << 2;
		}

		if (!(bits & FI_BIT_1)) {
			// Use literal rules (C.14.3, C.15.3).

			// Read add-to-table (C.14.3.1, C.15.3.1).
			decoder.readBits(1);

			add_value_to_table = bits & FI_BIT_2;

			decoder.ext_sub_step = 2;
			goto reparse;
		}

		decoder.ext_sub_step = 1;
		// FALLTHROUGH

	case 1:
		// Read string-index using C.26 (C.14.4), C.28 (C.15.4).
		if (is_bit1) {
			FI_UInt21 tmp_idx;

			if (!decoder.readUInt21_bit2(tmp_idx)) {
				return false;
			}

			idx = tmp_idx;
		} else {
			FI_PInt20 tmp_idx;

			if (!decoder.readPInt20_bit4(tmp_idx)) {
				return false;
			}

			idx = FI_PINT_TO_UINT(tmp_idx);
		}

		// XXX: Throws IndexOutOfBoundsException on bogus index.
		*this = *(*vocab_table)[idx];
		break;

	case 2:
		// Read character-string using C.19 (C.14.3.2), C.20 (C.15.3.3).

		// Examine discriminator bits.
		decoder.readBits(2);

		if (is_bit1) {
			bits = decoder.getBits();
		} else {
			bits = decoder.getBits() << 2;
		}

		switch (bits & FI_BITS(,,1,1,,,,)) {
		case ENCODE_AS_UTF8:
			// Use UTF-8 decoding rules.
			saved_format = ENCODE_AS_UTF8;
			break;

		case ENCODE_AS_UTF16:
			// FIXME: We don't support the utf-16 alternative.
			saved_format = ENCODE_AS_UTF16;
			throw UnsupportedOperationException ();

		case ENCODE_WITH_ALPHABET:
			saved_format = ENCODE_WITH_ALPHABET;
			decoder.ext_sub_step = 5;
			goto reparse;

		case ENCODE_WITH_ALGORITHM:
			saved_format = ENCODE_WITH_ALGORITHM;
			decoder.ext_sub_step = 6;
			goto reparse;

		default:
			// This should never happen.
			throw AssertionFailureException ();
		}

		decoder.ext_sub_step = 3;
		// FALLTHROUGH

	case 3:
		// Read encoded length using C.23.3 (C.19.4), C.24.3 (C.20.4).
		if (is_bit1) {
			if (!decoder.readNonEmptyOctets_len_bit5(len)) {
				return false;
			}
		} else {
			if (!decoder.readNonEmptyOctets_len_bit7(len)) {
				return false;
			}
		}

		// XXX: FI_PINT_TO_UINT() can't overflow, because we already
		// checked in readNonEmptyOctets_len_bit{5,7}().
		decoder.ext_saved_len = FI_PINT_TO_UINT(len);

		decoder.ext_sub_step = 4;
		// FALLTHROUGH

	case 4:
		// Read encoded string octets.
		if (!decodeOctets(decoder, decoder.ext_saved_len)) {
			return false;
		}
		break;

	case 5:
		// Read alphabet index using C.29 (C.19.3.3, C.20.3.3).
		if (!decoder.readPInt8(encoding_idx)) {
			return false;
		}

		// Determine future value type.
		// FIXME: We don't support the restricted-alphabet alternative.
		throw UnsupportedOperationException ();

	case 6:
		// Read algorithm index using C.29 (C.19.3.4, C.20.3.4).
		if (!decoder.readPInt8(encoding_idx)) {
			return false;
		}

		// XXX: Throws IndexOutOfBoundsException on bogus index.
		decoder.setEncodingAlgorithm(FI_PINT_TO_UINT(encoding_idx));

		decoder.ext_sub_step = 3;
		goto reparse;

	default:
		// Should never happen.
		throw AssertionFailureException ();
	}

	decoder.ext_sub_step = 0;
	return true;
}

} // namespace FI
} // namespace BTech


/*
 * C interface.
 */

using namespace BTech::FI;

FI_Value *
fi_create_value(void)
{
	try {
		return new FI_Value ();
	} catch (const std::bad_alloc& e) {
		return 0;
	}
}

void
fi_destroy_value(FI_Value *obj)
{
	delete obj;
}

FI_ValueType
fi_get_value_type(const FI_Value *obj)
{
	return obj->getType();
}

size_t
fi_get_value_count(const FI_Value *obj)
{
	return obj->getCount();
}

const void *
fi_get_value(const FI_Value *obj)
{
	return obj->getBuffer();
}

int
fi_set_value_type(FI_Value *obj, FI_ValueType type, size_t count)
{
	return obj->setBufferType(type, count);
}

void
fi_set_value(FI_Value *obj, const void *buf)
{
	obj->setBufferValue(buf);
}

void *
fi_get_value_buffer(FI_Value *obj)
{
	return obj->getBuffer();
}
