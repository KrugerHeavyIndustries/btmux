/*
 * Generic Value class.
 */

#ifndef BTECH_FI_VALUE_HH
#define BTECH_FI_VALUE_HH

#include <stddef.h>

#include "values.h"

#include "Exception.hh"

namespace BTech {
namespace FI {

class Value {
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

	Value& operator = (const Value& src) {
		if (!setValue(src.value_type, src.value_count, src.value_buf)) {
			// TODO: Need a more specific Exception.
			throw Exception ();
		}

		return *this;
	}

	bool setValue (FI_ValueType type, size_t count, const void *buf);

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

private:
	FI_ValueType value_type;
	size_t value_count;
	char *value_buf;
}; // class Value

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
