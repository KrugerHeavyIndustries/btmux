/*
 * Mutable implementation of Attributes.
 */

#include "autoconf.h"

#include "Name.hh"
#include "Value.hh"

#include "MutableAttributes.hh"

namespace BTech {
namespace FI {

} // namespace FI
} // namespace BTech


/*
 * C interface.
 */

using namespace BTech::FI;


/*
 * Attributes API.
 */

FI_Attributes *
fi_create_attributes(void)
{
	return new FI_Attributes ();
}

void
fi_destroy_attributes(FI_Attributes *attrs)
{
	delete attrs;
}

void
fi_clear_attributes(FI_Attributes *attrs)
{
	attrs->impl.clear();
}

int
fi_add_attribute(FI_Attributes *attrs,
                 const FI_Name *name, const FI_Value *value)
{
	return attrs->impl.add(name->getNameRef(), value->parent);
}

int
fi_get_attributes_length(const FI_Attributes *attrs)
{
	return attrs->impl.getLength();
}

const FI_Name *
fi_get_attribute_name(const FI_Attributes *attrs, int idx)
{
	return attrs->impl.getCName(idx);
}

const FI_Value *
fi_get_attribute_value(const FI_Attributes *attrs, int idx)
{
	return attrs->impl.getCValue(idx);
}
