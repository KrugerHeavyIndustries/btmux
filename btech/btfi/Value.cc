/*
 * Dynamically-castable Value class implementation.
 */
#include "autoconf.h"

#include <cstddef>
#include <climits>
#include <cassert>

#include <memory>

#include "common.h"
#include "values.h"

#include "Value.hh"

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
 * Value implementation.
 */

namespace {

// Replace value of existing type.
template<typename T>
void
set_value_buf(char *value_buf, size_t count, const void *buf)
{
	const T *const src_buf = static_cast<const T *>(buf);
	T *const dst_buf = reinterpret_cast<T *>(value_buf);

	for (size_t ii = 0; ii < count; ii++) {
		dst_buf[ii] = src_buf[ii];
	}
}

// Create value of different type.
template<typename T>
char *
new_value_buf(size_t count, const void *buf)
{
	char *new_buf;

	if (count > ((size_t)-1) / sizeof(T)) {
		// Overflow.
		return 0;
	}

	try {
		new_buf = new char[count * sizeof(T)];
	} catch (const std::bad_alloc& e) {
		return 0;
	}

	set_value_buf<T>(new_buf, count, buf);

	return new_buf;
}

// Test whether one value of a given type is less than another of the same
// type.  Strings of values are compared lexicographically.
//
// The type involved must naturally be LessThanComparable.
template<typename T>
bool
less_value_buf(size_t count, const void *lhs, const void *rhs)
{
	const T *const lhs_buf = static_cast<const T *>(lhs);
	const T *const rhs_buf = static_cast<const T *>(rhs);

	for (size_t ii = 0; ii < count; ii++) {
		if (lhs_buf[ii] < rhs_buf[ii]) {
			// lhs < rhs
			return true;
		} else if (rhs_buf[ii] < lhs_buf[ii]) {
			// lhs > rhs
			return false;
		}
	}

	// lhs == rhs
	return false;
}

} // anonymous namespace

Value::~Value()
{
	delete[] value_buf;
}

bool
Value::setValue(FI_ValueType type, size_t count, const void *buf)
{
	// Empty values are always of type FI_VALUE_AS_NULL.
	if (count == 0) {
		delete[] value_buf;

		value_type = FI_VALUE_AS_NULL;
		value_count = 0;
		value_buf = 0;
		return true;
	}

	// Create/assign the new value.
	const bool in_place = (value_type == type && value_count == count);

	char *new_buf;

	switch (type) {
	case FI_VALUE_AS_SHORT:
		if (in_place) {
			set_value_buf<FI_Int16>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Int16>(count, buf);
		}
		break;

	case FI_VALUE_AS_INT:
		if (in_place) {
			set_value_buf<FI_Int32>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Int32>(count, buf);
		}
		break;

	case FI_VALUE_AS_LONG:
		if (in_place) {
			set_value_buf<FI_Int64>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Int64>(count, buf);
		}
		break;

	case FI_VALUE_AS_BOOLEAN:
		if (in_place) {
			set_value_buf<FI_Boolean>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Boolean>(count, buf);
		}
		break;

	case FI_VALUE_AS_FLOAT:
		if (in_place) {
			set_value_buf<FI_Float32>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Float32>(count, buf);
		}
		break;

	case FI_VALUE_AS_DOUBLE:
		if (in_place) {
			set_value_buf<FI_Float64>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Float64>(count, buf);
		}
		break;

	case FI_VALUE_AS_UUID:
		if (in_place) {
			set_value_buf<FI_UUID>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_UUID>(count, buf);
		}
		break;

	case FI_VALUE_AS_OCTETS:
		if (in_place) {
			set_value_buf<FI_Octet>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Octet>(count, buf);
		}
		break;

	default:
		// Unknown type requested.
		return false;
	}

	// Perform the swap.
	if (!in_place) {
		if (!new_buf) {
			return false;
		}

		delete[] value_buf;

		value_type = type;
		value_count = count;
		value_buf = new_buf;
	}

	return true;
}

// Imposes an absolute ordering on all possible values.
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

	// Compare the values.
	switch (value_type) {
	case FI_VALUE_AS_SHORT:
		return less_value_buf<FI_Int16>(value_count,
		                                value_buf, rhs.value_buf);

	case FI_VALUE_AS_INT:
		return less_value_buf<FI_Int32>(value_count,
		                                value_buf, rhs.value_buf);

	case FI_VALUE_AS_LONG:
		return less_value_buf<FI_Int64>(value_count,
		                                value_buf, rhs.value_buf);

	case FI_VALUE_AS_BOOLEAN:
		return less_value_buf<FI_Boolean>(value_count,
		                                  value_buf, rhs.value_buf);

	case FI_VALUE_AS_FLOAT:
		return less_value_buf<FI_Float32>(value_count,
		                                  value_buf, rhs.value_buf);

	case FI_VALUE_AS_DOUBLE:
		return less_value_buf<FI_Float64>(value_count,
		                                  value_buf, rhs.value_buf);

	case FI_VALUE_AS_UUID:
		return less_value_buf<FI_UUID>(value_count,
		                               value_buf, rhs.value_buf);

	case FI_VALUE_AS_OCTETS:
		return less_value_buf<FI_Octet>(value_count,
		                                value_buf, rhs.value_buf);

	default:
		// Should never happen.
		assert(false);
		return false;
	}
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
	return obj->getValue();
}

int
fi_set_value(FI_Value *obj, FI_ValueType type, size_t count, const void *buf)
{
	return obj->setValue(type, count, buf);
}
