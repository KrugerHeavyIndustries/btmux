/*
 * Mutable implementation of Attributes.
 */

#include "autoconf.h"

#include "attribs.h"

#include "Name.hh"
#include "Value.hh"

#include "MutableAttributes.hh"


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
	attrs->clear();
}

int
fi_add_attribute(FI_Attributes *attrs,
                 const FI_Name *name, const FI_Value *value)
{
	return attrs->add(*name, *value);
}

int
fi_get_attributes_length(const FI_Attributes *attrs)
{
	return attrs->getLength();
}

const FI_Name *
fi_get_attribute_name(const FI_Attributes *attrs, int idx)
{
	return attrs->getCName(idx);
}

const FI_Value *
fi_get_attribute_value(const FI_Attributes *attrs, int idx)
{
	return attrs->getCValue(idx);
}
