#include "autoconf.h"

#include <cstddef>
#include <cassert>
#include <cstdio>

#include "common.h"
#include "stream.h"

#include "Name.hh"
#include "Value.hh"

#include "Element.hh"


namespace BTech {
namespace FI {

namespace {

FI_VocabIndex
debug_name(const Name *name) throw ()
{
	assert(name->getType() == FI_NAME_AS_INDEX);
	return *reinterpret_cast<const FI_VocabIndex *>(name->getName());
}

const char *
debug_value(const Value& value) throw ()
{
	assert(value.getType() == FI_VALUE_AS_OCTETS);
	return reinterpret_cast<const char *>(value.getValue());
}

} // anonymous namespace

void
Element::start(const Name& name, const Attributes& attrs) throw ()
{
	start_flag = true;
	stop_flag = false;

	w_name = &name;
	w_attrs = &attrs;

	doc.increaseDepth();
}

void
Element::stop(const Name& name) throw ()
{
	start_flag = false;
	stop_flag = true;

	w_name = &name;

	doc.decreaseDepth();
}

void
Element::write(FI_OctetStream *stream) throw (Exception)
{
	if (start_flag) {
		printf("%*s<%lu",
		       4 * (doc.getDepth() - 1), "", debug_name(w_name));

		for (int ii = 0; ii < w_attrs->getLength(); ii++) {
			const Name& a_name = w_attrs->getName(ii);
			const Value& a_value = w_attrs->getValue(ii);

			printf(" %lu='%.*s'", debug_name(&a_name),
			       a_value.getCount(), debug_value(a_value));
		}

		printf(">\n");
	} else if (stop_flag) {
		printf("%*s</%lu>\n",
		       4 * doc.getDepth(), "", debug_name(w_name));
	} else {
		throw IllegalStateException ();
	}
}

void
Element::read(FI_OctetStream *stream) throw (Exception)
{
	if (start_flag) {
	} else if (stop_flag) {
	} else {
		throw IllegalStateException ();
	}
}


//
// Element serialization subroutines.
//

namespace {



} // anonymous namespace

} // namespace FI
} // namespace BTech
