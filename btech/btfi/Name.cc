/*
 * Generic Name class implementation.
 */

#include "autoconf.h"

#include <memory>

#include "common.h"
#include "generic.h"

#include "Name.hh"


namespace BTech {
namespace FI {

namespace {

// Replace value of existing type.
template<typename T>
void
set_name_buf(char *name_buf, const void *buf)
{
	*reinterpret_cast<T *>(name_buf) = *reinterpret_cast<const T *>(buf);
}

// Create value of different type.
template<typename T>
char *
new_name_buf(const void *buf)
{
	char *new_buf;

	try {
		new_buf = new char[sizeof(T)];
	} catch (const std::bad_alloc& e) {
		return 0;
	}

	set_name_buf<T>(new_buf, buf);

	return new_buf;
}

} // anonymous namespace

Name::~Name()
{
	delete[] name_buf;
}

bool
Name::setName(FI_NameType type, const void *buf)
{
	char *new_buf = 0;

	// Create/assign the new name.
	switch (type) {
	case FI_NAME_AS_INDEX:
		if (name_type == FI_NAME_AS_INDEX) {
			set_name_buf<FI_VocabIndex>(name_buf, buf);
		} else {
			new_buf = new_name_buf<FI_VocabIndex>(buf);
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
		delete[] name_buf;

		name_type = type;
		name_buf = new_buf;
	}

	return true;
}

} // namespace FI
} // namespace BTech
