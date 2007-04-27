/*
 * Generic Value class implementation.
 */

#include "autoconf.h"

#include <cstddef>
#include <memory>

#include "common.h"
#include "generic.h"

#include "Value.hh"


namespace BTech {
namespace FI {

namespace {

// Replace value of existing type.
template<typename T>
void
set_value_buf(char *value_buf, size_t count, const void *buf) throw ()
{
	const T *const src_buf = reinterpret_cast<const T *>(buf);
	T *const dst_buf = reinterpret_cast<T *>(value_buf);

	for (size_t ii = 0; ii < count; ii++) {
		dst_buf[ii] = src_buf[ii];
	}
}

// Create value of different type.
template<typename T>
char *
new_value_buf(size_t count, const void *buf) throw ()
{
        char *new_buf;

        try {
                new_buf = new char[count * sizeof(T)];
        } catch (const std::bad_alloc& e) {
                return 0;
        }

        set_value_buf<T>(new_buf, count, buf);

        return new_buf;
}

} // anonymous namespace

Value::~Value() throw ()
{
	delete[] value_buf;
}

bool
Value::setValue(FI_ValueType type, size_t count, const void *buf) throw ()
{
	char *new_buf = 0;

	// Create/assign the new value.
	switch (type) {
	case FI_VALUE_AS_SHORT:
		if (value_type == type && value_count == count) {
			set_value_buf<FI_Int16>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Int16>(count, buf);
			if (!new_buf) {
				return false;
			}
		}
		break;

	case FI_VALUE_AS_INT:
		if (value_type == type && value_count == count) {
			set_value_buf<FI_Int32>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Int32>(count, buf);
			if (!new_buf) {
				return false;
			}
		}
		break;

	case FI_VALUE_AS_LONG:
		if (value_type == type && value_count == count) {
			set_value_buf<FI_Int64>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Int64>(count, buf);
			if (!new_buf) {
				return false;
			}
		}
		break;

	case FI_VALUE_AS_BOOLEAN:
		if (value_type == type && value_count == count) {
			set_value_buf<FI_Boolean>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Boolean>(count, buf);
			if (!new_buf) {
				return false;
			}
		}
		break;

	case FI_VALUE_AS_FLOAT:
		if (value_type == type && value_count == count) {
			set_value_buf<FI_Float32>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Float32>(count, buf);
			if (!new_buf) {
				return false;
			}
		}
		break;

	case FI_VALUE_AS_DOUBLE:
		if (value_type == type && value_count == count) {
			set_value_buf<FI_Float64>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Float64>(count, buf);
			if (!new_buf) {
				return false;
			}
		}
		break;

	case FI_VALUE_AS_UUID:
		count *= sizeof(FI_UUID); // FIXME: check for overflow

		if (value_type == type && value_count == count) {
			set_value_buf<FI_Octet>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Octet>(count, buf);
			if (!new_buf) {
				return false;
			}
		}
		break;

	case FI_VALUE_AS_OCTETS:
		if (value_type == type && value_count == count) {
			set_value_buf<FI_Octet>(value_buf, count, buf);
		} else {
			new_buf = new_value_buf<FI_Octet>(count, buf);
			if (!new_buf) {
				return false;
			}
		}
		break;

	default:
		return false;
	}

	// Perform the swap.
	if (new_buf) {
		delete[] value_buf;

		value_type = type;
		value_count = count;
		value_buf = new_buf;
	}

	return true;
}

} // namespace FI
} // namespace BTech
