/*
 * Implements Element-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 annex A, and informed by annex C.
 *
 * Only enough of Fast Infoset is implemented to support saving and loading
 * BattleTech binary databases.  In particular, only the Document, Element, and
 * Attribute types are supported, and additional types as needed to support
 * processing these types.  The objective is to support saving and loading a
 * hierarchical structure, possibly with per-element attributes.
 *
 * However, to ensure backward compatibility, all valid Fast Infoset bitstreams
 * may be decoded, although possibly with the loss of information contained in
 * types beyond the basic three of Document, Element, and Attribute.
 *
 * Implementation restrictions:
 * 7.3.4: [namespace attributes] is only set on [document element], and is
 *        always just our default BT namespace (index 2).
 * 7.3.5: qualified-name must always be an element name surrogate index (but
 *        must follow the specification with regards to dynamic indexes).
 * 7.3.7: Only Element-type children are supported. (Other types ignorable?)
 * 
 * TODO: It might be cleaner to make Attributes Serializable, rather than
 *       handling it here.
 *
 * 7.4.4: qualified-name must always be an attribute name surrogate index (but
 *        must follow the specification with regards to dynamic indexes).
 * 7.4.5: normalized-value must always be an octet string (but must follow the
 *        specification with regards to dynamic indexes).
 *
 * 7.12.4: The namespace declaration contains a default declaration (no
 *         prefix), corresponding to our default BT namespace (index 2).
 * 7.12.6: The namespace name will always be expressed as an index (but must be
 *         processed).
 *
 * This API currently doesn't attempt to prevent all the possible ways of
 * creating an ill-formed document; the API user is expected to be reasonable.
 */

#include "autoconf.h"

#include <cstddef>
#include <cassert>
#include <cstdio>

#include "common.h"
#include "stream.h"
#include "encutil.hh"

#include "Name.hh"
#include "Value.hh"

#include "Element.hh"


namespace BTech {
namespace FI {

namespace {

FI_VocabIndex
debug_name(const Name *name)
{
	assert(name->getType() == FI_NAME_AS_INDEX);
	return *reinterpret_cast<const FI_VocabIndex *>(name->getName());
}

const char *
debug_value(const Value& value)
{
	assert(value.getType() == FI_VALUE_AS_OCTETS);
	return reinterpret_cast<const char *>(value.getValue());
}

bool write_start(FI_OctetStream *, const Name&, const Attributes&, bool);
bool write_end(FI_OctetStream *);

bool write_namespace_attributes(FI_OctetStream *);

} // anonymous namespace

void
Element::start(const Name& name, const Attributes& attrs)
{
	start_flag = true;
	stop_flag = false;

	w_name = &name;
	w_attrs = &attrs;
}

void
Element::stop(const Name& name)
{
	start_flag = false;
	stop_flag = true;

	w_name = &name;
}

void
Element::write(FI_OctetStream *stream)
{
	if (start_flag) {
		if (!write_start(stream, *w_name, *w_attrs,
		                 doc.getDepth() == 0)) {
			// TODO: Assign an exception for stream errors.
			throw Exception ();
		}

		doc.increaseDepth();

		// XXX: Debug.
		printf("%*s<%u",
		       4 * (doc.getDepth() - 1), "", debug_name(w_name));

		for (int ii = 0; ii < w_attrs->getLength(); ii++) {
			const Name& a_name = w_attrs->getName(ii);
			const Value& a_value = w_attrs->getValue(ii);

			printf(" %u='%.*s'", debug_name(&a_name),
			       a_value.getCount(), debug_value(a_value));
		}

		printf(">\n");
	} else if (stop_flag) {
		doc.decreaseDepth();

		if (!write_end(stream)) {
			// TODO: Assign an exception for stream errors.
			throw Exception ();
		}

		// XXX: Debug.
		printf("%*s</%u>\n",
		       4 * doc.getDepth(), "", debug_name(w_name));
	} else {
		throw IllegalStateException ();
	}
}

void
Element::read(FI_OctetStream *stream)
{
	if (start_flag) {
	} else if (stop_flag) {
	} else {
		throw IllegalStateException ();
	}
}


namespace {

//
// Element serialization subroutines.
//

bool
write_start(FI_OctetStream *stream, const Name& name, const Attributes& attrs,
            bool useNamespace)
{
	// Pad out bitstream so we start on the 1st bit of an octet.
	switch (fi_get_stream_num_bits(stream)) {
	case 0:
		// C.3.2: Starting at 1st bit of this octet.
		break;

	case 4:
		// C.2.11.1, C.3.7.1: Add 0000 padding.
		// C.3.2: Starting at 1st bit of next octet.
		if (!fi_flush_stream_bits(stream)) {
			return false;
		}
		break;

	default:
		// Shouldn't happen.
		return false;
	}

	// Write identification (C.2.11.2, C.3.7.2: 0).
	// Write attributes presence flag (C.3.3).
	if (!fi_write_stream_bits(stream, 2,
	                          attrs.getLength() ? 0/*TODO:FI_BIT_2*/ : 0)) {
		return false;
	}

	// Write namespace-attributes (C.3.4).
	if (useNamespace && !write_namespace_attributes(stream)) {
		return false;
	}

	// Write qualified-name (C.18).
	if (!write_name_bit_3(stream, name)) {
		return false;
	}

	// Write attributes (C.3.6).

	// Write children (C.3.7).
	return true;
}

bool
write_end(FI_OctetStream *stream)
{
	assert(fi_get_stream_num_bits(stream) == 0
	       || fi_get_stream_num_bits(stream) == 4);

	// Write termination (C.3.8: 1111).
	if (!fi_write_stream_bits(stream, 4, FI_BITS(1,1,1,1,,,,))) {
		return false;
	}

	return true;
}

bool
write_namespace_attributes(FI_OctetStream *stream)
{
	assert(fi_get_stream_num_bits(stream) == 2);

	// Write identification (C.3.4.1: 1110, 00).
	if (!fi_write_stream_bits(stream, 6, FI_BITS(1,1,1,0,0,0,,))) {
		return false;
	}

	// Write namespace-attributes using C.12 (C.3.4.2: 110011).
	// XXX: We only have 1 namespace-attribute, which corresponds to index
	// 2, our default namespace, with no prefix.
	// FIXME: This needs to be looked up, not hardcoded.
	if (!fi_write_stream_bits(stream, 6, FI_BITS(1,1,0,0,1,1,,))) {
		return false;
	}

	if (!write_namespace_attribute(stream)) {
		return false;
	}

	// Write termination (C.3.4.3: 1111, 0000 00).
	assert(fi_get_stream_num_bits(stream) == 0);

	FI_Octet *w_buf = fi_get_stream_write_buffer(stream, 1);
	if (!w_buf) {
		return false;
	}

	w_buf[0] = FI_BITS(1,1,1,1,0,0,0,0);

	if (!fi_write_stream_bits(stream, 2, 0)) {
		return false;
	}

	return true;
}

} // anonymous namespace

} // namespace FI
} // namespace BTech
