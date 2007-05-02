/*
 * Mutable implementation of Attributes.
 */

#include "autoconf.h"

#include "names.h"
#include "values.h"
#include "attribs.h"

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

// C-compatible wrapper around the MutableAttributes class.
struct FI_tag_Attributes {
	MutableAttributes impl;
}; // FI_Attributes

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
	//return attrs->impl.add(name->parent, value->parent);
}

int
fi_get_attributes_length(const FI_Attributes *attrs)
{
	//return attrs->impl.getLength();
}

const FI_Name *
fi_get_attribute_name(const FI_Attributes *attrs, int idx)
{
	try {
		//return attrs->impl.getName(idx).getProxy();
	} catch (const IndexOutOfBoundsException& e) {
		return 0;
	}
}

const FI_Value *
fi_get_attribute_value(const FI_Attributes *attrs, int idx)
{
	try {
		return attrs->impl.getValue(idx).getProxy();
	} catch (const IndexOutOfBoundsException& e) {
		return 0;
	}
}
