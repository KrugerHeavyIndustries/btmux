/*
 * Generic Value class.
 */

#ifndef BTECH_FI_VALUE_HH
#define BTECH_FI_VALUE_HH

#include <stddef.h>

#include "common.h"
#include "values.h"

#include "Exception.hh"

namespace BTech {
namespace FI {

// Forward declaration.
class Value;

} // namespace FI
} // namespace BTech

struct FI_tag_Value {
	FI_tag_Value (BTech::FI::Value& parent) : parent (parent) {}

	BTech::FI::Value& parent;
}; // FI_Value

namespace BTech {
namespace FI {

class Value {
public:
	Value ()
	: proxy (*this),
	  value_type (FI_VALUE_AS_NULL), value_count (0), value_buf (0) {}

	~Value ();

	// Assignment.
	Value (const Value& src)
	: proxy (*this),
	  value_type (FI_VALUE_AS_NULL), value_count (0), value_buf (0) {
		if (!setValue(src.value_type, src.value_count, src.value_buf)) {
			// TODO: Need a more specific Exception.
			throw Exception ();
		}
	}

	Value& operator= (const Value& src) {
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

	// Proxy handling.
	const FI_Value *getProxy () const {
		return &proxy;
	}

	FI_Value *getProxy () {
		return &proxy;
	}

private:
	FI_Value proxy;

	FI_ValueType value_type;
	size_t value_count;
	char *value_buf;
}; // class Value

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_VALUE_HH */
