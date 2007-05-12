/*
 * Generic Value class.
 */

#ifndef BTECH_FI_VALUE_HH
#define BTECH_FI_VALUE_HH

#include <stddef.h>

#include "values.h"

#include "Serializable.hh"
#include "VocabSimple.hh"

#include "Exception.hh"

namespace BTech {
namespace FI {

class DV_VocabTable;

enum EncodingFormat {
	ENCODE_AS_UTF8        = FI_BITS(,,0,0,,,,), // C.19.3.1 (C.20.3.1)
	ENCODE_AS_UTF16       = FI_BITS(,,0,1,,,,), // C.19.3.2 (C.20.3.2)
	ENCODE_WITH_ALPHABET  = FI_BITS(,,1,0,,,,), // C.19.3.3 (C.20.3.2)
	ENCODE_WITH_ALGORITHM = FI_BITS(,,1,1,,,,)  // C.19.3.4 (C.20.3.4)
}; // enum EncodingFormat

class Value : public Serializable {
public:
	// Construction.
	Value ()
	: value_type (FI_VALUE_AS_NULL), value_count (0), value_size (0),
	  value_buf (0), value_buf_size (0) {}

	virtual ~Value ();

	Value (const Value& src);

	Value (FI_ValueType type, size_t count, const void *buf)
	: value_buf (0), value_buf_size (0) {
		if (!setBufferType(type, count)) {
			// TODO: This Exception isn't specific enough.
			throw Exception ();
		}

		setBufferValue(buf);
	}

	Value& operator = (const Value& src);

	// XXX: For these purposes, "type" includes the count.
	bool setBufferType (FI_ValueType type, size_t count);

	void setBufferValue (const void *buf);

	// Comparison.
	bool operator < (const Value& rhs) const;

	// Accessors.
	FI_ValueType getType () const {
		return value_type;
	}

	size_t getCount () const {
		return value_count;
	}

	const void *getBuffer () const {
		return value_buf;
	}

	void *getBuffer () {
		return value_buf;
	}

	// Fast Infoset serialization support.
	void setVocabTable (DV_VocabTable& new_vocab_table);

	void write (Encoder& encoder) const;
	bool read (Decoder& decoder);

private:
	// Value buffer.
	void allocateValueBuffer (size_t new_buf_size);

	FI_ValueType value_type;
	size_t value_count;
	size_t value_size;

	void *value_buf;
	size_t value_buf_size;

	// Fast Infoset serialization support.
	void encodeOctets (Encoder& encoder, FI_Length len) const;
	bool decodeOctets (Decoder& decoder, FI_Length len);

	// C.14/C.19, C.15/C.20.
	void write_bit1_or_3 (Encoder& encoder, bool is_bit1 = true) const;
	bool read_bit1_or_3 (Decoder& decoder, bool is_bit1 = true);

	// TODO: Come up with a reasonable protocol for foisting all this state
	// on to the shared Encoder/Decoder objects.
	DV_VocabTable *vocab_table;
	bool add_value_to_table;

	EncodingFormat saved_format;
}; // class Value

/*
 * Dynamic value table.
 */
class DV_VocabTable : public DynamicTypedVocabTable<Value> {
public:
	DV_VocabTable ();

private:
	DV_VocabTable (bool read_only, FI_VocabIndex max_idx);

	static TypedVocabTable *builtin_table ();
}; // class DN_VocabTable

// Template function to ease casting a Value to primitive types.
// TODO: Add specializations that check the types are correct.
template<typename T>
const T *
Value_cast(const Value& value)
{
	return static_cast<const T *>(value.getBuffer());
}

} // namespace FI
} // namespace BTech

// Some magic for C/C++ compatibility.
struct FI_tag_Value : public BTech::FI::Value {
	FI_tag_Value () : Value () {}

	// Cast any Value to a FI_Value.
	static FI_Value *cast (Value& value) {
		return static_cast<FI_Value *>(&value);
	}

	static const FI_Value *cast (const Value& value) {
		return static_cast<const FI_Value *>(&value);
	}
}; // FI_Value

#endif /* !BTECH_FI_VALUE_HH */
