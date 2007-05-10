/*
 * Generic Value class.
 */

#ifndef BTECH_FI_VALUE_HH
#define BTECH_FI_VALUE_HH

#include <stddef.h>

#include "values.h"

#include "Serializable.hh"
#include "VocabTable.hh"

#include "Exception.hh"

namespace BTech {
namespace FI {

class DV_VocabTable;

class Value : public Serializable {
public:
	Value ()
	: value_type (FI_VALUE_AS_NULL), value_count (0), value_buf (0) {}

	virtual ~Value ();

	// Assignment.
	Value (const Value& src)
	: value_type (FI_VALUE_AS_NULL), value_count (0), value_buf (0) {
		if (!setValue(src.value_type, src.value_count, src.value_buf)) {
			// TODO: Need a more specific Exception.
			throw Exception ();
		}
	}

	Value (FI_ValueType type, size_t count, const void *buf)
	: value_type (FI_VALUE_AS_NULL), value_count (0), value_buf (0) {
		if (!setValue(type, count, buf)) {
			// TODO: Need a more specific Exception.
			throw Exception ();
		}
	}

	Value& operator = (const Value& src) {
		if (!setValue(src.value_type, src.value_count, src.value_buf)) {
			// TODO: Need a more specific Exception.
			throw Exception ();
		}

		return *this;
	}

	bool setValue (FI_ValueType type, size_t count, const void *buf);

	// Comparison.
	bool operator < (const Value& rhs) const;

	// Accessors.
	FI_ValueType getType () const {
		return value_type;
	}

	size_t getCount () const {
		return value_count;
	}

	const void *getValue () const {
		return value_buf;
	}

	// Fast Infoset serialization support.
	void setVocabTable (DV_VocabTable& new_vocab_table);

	void write (Encoder& encoder) const;
	bool read (Decoder& decoder);

private:
	FI_ValueType value_type;
	size_t value_count;
	char *value_buf;

	// Fast Infoset serialization support.
	FI_Length getEncodedSize (Encoder& encoder) const;
	size_t getDecodedSize (Decoder& decoder) const;

	void encodeOctets (Encoder& encoder, FI_Length len) const;
	bool decodeOctets (Decoder& decoder, FI_Length len);

	// C.14/C.19.
	void write_bit1 (Encoder& encoder) const;
	bool read_bit1 (Decoder& decoder);

	// C.15/C.20.
	void write_bit3 (Encoder& encoder) const;
	bool read_bit3 (Decoder& decoder);

	// TODO: Come up with a reasonable protocol for foisting all this state
	// on to the shared Encoder/Decoder objects.
	DV_VocabTable *vocab_table;
	bool add_value_to_table;

	unsigned char super_step, sub_step;
	FI_ValueType next_value_type;
	FI_Length saved_len;
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
	return static_cast<const T *>(value.getValue());
}

} // namespace FI
} // namespace BTech

// Some magic for C/C++ compatibility.
struct FI_tag_Value : public BTech::FI::Value {
	FI_tag_Value () : Value () {}

	// Cast any Value to a FI_Value.
	static const FI_Value *cast (const Value& value) {
		return static_cast<const FI_Value *>(&value);
	}
}; // FI_Value

#endif /* !BTECH_FI_VALUE_HH */
