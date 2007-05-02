/*
 * Generic value types.
 */

#include "autoconf.h"

#include <cstddef>
#include <memory>

#include "Value.hh"

#include "generic.h"

using namespace BTech::FI;


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
