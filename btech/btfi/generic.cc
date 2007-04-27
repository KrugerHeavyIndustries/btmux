/*
 * Generic types.
 */

#include "autoconf.h"

#include <cstddef>
#include <memory>

#include "Name.hh"
#include "Value.hh"

#include "generic.h"

using namespace BTech::FI;


/*
 * Generic names.
 */

FI_Name *
fi_create_name(void)
{
	try {
		return (new Name ())->getProxy();
	} catch (const std::bad_alloc& e) {
		return 0;
	}
}

void
fi_destroy_name(FI_Name *obj)
{
	delete &obj->parent;
}

FI_NameType
fi_get_name_type(const FI_Name *obj)
{
	return obj->parent.getType();
}

const void *
fi_get_name(const FI_Name *obj)
{
	return obj->parent.getName();
}

int
fi_set_name(FI_Name *obj, FI_NameType type, const void *buf)
{
	return obj->parent.setName(type, buf);
}


/*
 * Generic values.
 */

FI_Value *
fi_create_value(void)
{
	try {
		return (new Value ())->getProxy();
	} catch (const std::bad_alloc& e) {
		return 0;
	}
}

void
fi_destroy_value(FI_Value *obj)
{
	delete &obj->parent;
}

FI_ValueType
fi_get_value_type(const FI_Value *obj)
{
	return obj->parent.getType();
}

size_t
fi_get_value_count(const FI_Value *obj)
{
	return obj->parent.getCount();
}

const void *
fi_get_value(const FI_Value *obj)
{
	return obj->parent.getValue();
}

int
fi_set_value(FI_Value *obj, FI_ValueType type, size_t count, const void *buf)
{
	return obj->parent.setValue(type, count, buf);
}
