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
 * 7.3.7: Only Element-type children are supported. (Other types ignorable?)
 * 
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
#include <cstdio> // XXX: DEBUG

#include "common.h"
#include "stream.h"
#include "encutil.hh"

#include "Name.hh"
#include "Value.hh"
#include "MutableAttributes.hh"

#include "Element.hh"


namespace BTech {
namespace FI {

void
Element::start()
{
	start_flag = true;
	stop_flag = false;
	r_state = RESET_READ_STATE;
}

void
Element::stop()
{
	start_flag = false;
	stop_flag = true;
	r_state = RESET_READ_STATE;
}

void
Element::write(FI_OctetStream *stream)
{
	if (start_flag) {
		// Write element start.
		write_start(stream);

		doc.pushElement(name);
	} else if (stop_flag) {
		// Write element end.
		if (!doc.hasElements()) {
			// Tried to pop too many elements.
			throw IllegalStateException ();
		}

		name = doc.popElement(); // XXX: don't need to restore name...

		write_end(stream);
	} else {
		throw IllegalStateException ();
	}
}

void
Element::read(FI_OctetStream *stream)
{
	if (start_flag) {
		switch (r_state) {
		case RESET_READ_STATE:
			r_state = MAIN_READ_STATE;
			r_element_state = RESET_ELEMENT_STATE;
			// FALLTHROUGH

		case MAIN_READ_STATE:
			// Try to read element start.
			if (!read_start(stream)) {
				return;
			}

			doc.pushElement(name);

			r_state = NEXT_PART_READ_STATE;
			// FALLTHROUGH

		case NEXT_PART_READ_STATE:
			// Determine next child type.
			doc.read_next(stream);
			break;
		}
	} else if (stop_flag) {
		switch (r_state) {
		case RESET_READ_STATE:
			if (!doc.hasElements()) {
				// Tried to pop too many elements.
				throw IllegalStateException ();
			}

			name = doc.popElement(); // need name for SAX API

			r_state = MAIN_READ_STATE;
			// FALLTHROUGH

		case MAIN_READ_STATE:
			// Try to read element end.
			if (!read_end(stream)) {
				return;
			}

			r_state = NEXT_PART_READ_STATE;
			// FALLTHROUGH

		case NEXT_PART_READ_STATE:
			// Determine next child type.
			doc.read_next(stream);
			break;
		}
	} else {
		throw IllegalStateException ();
	}
}


/*
 * Element serialization subroutines.
 */

void
Element::write_start(FI_OctetStream *stream)
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
			// TODO: Assign an exception for stream errors.
			throw Exception ();
		}
		break;

	default:
		// Shouldn't happen.
		throw IllegalStateException ();
	}

	// Write identification (C.2.11.2, C.3.7.2: 0).
	// Write attributes presence flag (C.3.3).
	if (!fi_write_stream_bits(stream, 2,
	                          w_attrs->getLength() ? FI_BIT_2 : 0)) {
		// TODO: Assign an exception for stream errors.
		throw Exception ();
	}

	// Write namespace-attributes (C.3.4).
	write_namespace_attributes(stream);

	// Write qualified-name (C.18).
	if (!write_name_bit_3(stream, name)) {
		// TODO: Assign an exception for stream errors.
		throw Exception ();
	}

	// Write attributes (C.3.6).
	write_attributes(stream);

	// Write children (C.3.7).  This is handled by the respective child
	// serialization routines, so we don't need to do anything here.
}

void
Element::write_end(FI_OctetStream *stream)
{
	assert(fi_get_stream_num_bits(stream) == 0
	       || fi_get_stream_num_bits(stream) == 4);

	// Write termination (C.3.8: 1111).
	if (!fi_write_stream_bits(stream, 4, FI_BITS(1,1,1,1,,,,))) {
		// TODO: Assign an exception for stream errors.
		throw Exception ();
	}
}

void
Element::write_namespace_attributes(FI_OctetStream *stream)
{
	assert(fi_get_stream_num_bits(stream) == 2);

	if (doc.hasElements()) {
		// We only write the BT_NAMESPACE default namespace at root.
		return;
	}

	// Write identification (C.3.4.1: 1110, 00).
	if (!fi_write_stream_bits(stream, 6, FI_BITS(1,1,1,0, 0,0,,))) {
		// TODO: Assign an exception for stream errors.
		throw Exception ();
	}

	// Write namespace-attributes using C.12 (C.3.4.2: 110011).
	// XXX: We only have 1 namespace-attribute, which corresponds to index
	// 2, our default namespace, with no prefix.
	// FIXME: This needs to be looked up, not hardcoded.
	if (!fi_write_stream_bits(stream, 6, FI_BITS(1,1,0,0,1,1,,))) {
		// TODO: Assign an exception for stream errors.
		throw Exception ();
	}

	if (!write_namespace_attribute(stream, doc.BT_NAMESPACE)) {
		// TODO: Assign an exception for stream errors.
		throw Exception ();
	}

	// Write termination (C.3.4.3: 1111, 0000 00).
	assert(fi_get_stream_num_bits(stream) == 0);

	FI_Octet *w_buf = fi_get_stream_write_buffer(stream, 1);
	if (!w_buf) {
		// TODO: Assign an exception for stream errors.
		throw Exception ();
	}

	w_buf[0] = FI_BITS(1,1,1,1, 0,0,0,0);

	if (!fi_write_stream_bits(stream, 2, FI_BITS(0,0,,,,,,))) {
		// TODO: Assign an exception for stream errors.
		throw Exception ();
	}
}

void
Element::write_attributes(FI_OctetStream *stream)
{
	if (!w_attrs->getLength()) {
		// No attributes.
		return;
	}

	for (int ii = 0; ii < w_attrs->getLength(); ii++) {
		// Write identification (C.3.6.1: 0).
		if (!fi_write_stream_bits(stream, 1, 0)) {
			// TODO: Assign an exception for stream errors.
			throw Exception ();
		}

		// Write attribute using C.4 (C.3.6.1).
		if (!write_attribute(stream,
		                     w_attrs->getName(ii),
		                     w_attrs->getValue(ii))) {
			// TODO: Assign an exception for stream errors.
			throw Exception ();
		}
	}

	// Write termination (C.3.6.2).
	assert(fi_get_stream_num_bits(stream) == 0); // C.4.2

	if (!fi_write_stream_bits(stream, 4, FI_BITS(1,1,1,1,,,,))) {
		// TODO: Assign an exception for stream errors.
		throw Exception ();
	}
}


/*
 * Element unserialization subroutines.
 */

bool
Element::read_start(FI_OctetStream *stream)
{
	const FI_Octet *r_buf;
	FI_Octet bits;

redispatch:
	switch (r_element_state) {
	case RESET_ELEMENT_STATE:
		// Peek at the second bit to check if we have attributes.
		r_attrs.clear();

		if (fi_try_read_stream(stream, &r_buf, 0, 1) < 1) {
			return false;
		}

		r_has_attrs = r_buf[0] & FI_BIT_2;

		// Check if we have namespace attributes (C.3.4.1: 1110 00).
		bits = r_buf[0] & FI_BITS(,,1,1,1,1,1,1);

		if (bits != FI_BITS(,,1,1,1,0, 0,0)) {
			if (!doc.hasElements()) {
				// Require BT_NAMESPACE on root element.
				// FIXME: Implementation restriction.
				throw UnsupportedOperationException ();
			}

			// Skip directly to NAME_ELEMENT_STATE.
			r_element_state = NAME_ELEMENT_STATE;
			goto redispatch; // already commented on in Document
		}

		// FIXME: As an implementation restriction, we only allow
		// namespace attributes at the root level.
		if (doc.hasElements()) {
			throw UnsupportedOperationException ();
		}

		// Consume namespace attributes presence bits (C.3.4.1).
		fi_advance_stream_cursor(stream, 1);

		r_element_state = NS_DECL_ELEMENT_STATE;
		// FALLTHROUGH

	case NS_DECL_ELEMENT_STATE:
		// Parse namespace attributes.
		if (!read_namespace_attributes(stream)) {
			return false;
		}

		r_element_state = NAME_ELEMENT_STATE;
		// FALLTHROUGH

	case NAME_ELEMENT_STATE:
		// Parse element name.
		if (!read_name_bit_3(stream, name)) {
			return false;
		}

		if (!r_has_attrs) {
			// Proceed directly to parsing children.
			break;
		}

		r_element_state = ATTRS_ELEMENT_STATE;
		// FALLTHROUGH

	case ATTRS_ELEMENT_STATE:
		// Parse attributes.
		if (!read_attributes(stream)) {
			return false;
		}
		break;
	}

	return true;
}

bool
Element::read_end(FI_OctetStream *stream)
{
	// Read element terminator bits (C.3.8).
	const FI_Octet *r_buf;
	FI_Octet bits;

	if (fi_try_read_stream(stream, &r_buf, 0, 1) < 1) {
		return false;
	}

	switch (fi_get_stream_num_bits(stream)) {
	case 0:
		// Ended on 4th bit (C.3.2).
		bits = r_buf[0] & FI_BITS(1,1,1,1,,,,);

		if (bits != FI_BITS(1,1,1,1,,,,)) {
			// Not a valid Fast Infoset.
			throw IllegalStateException ();
		}

		fi_set_stream_num_bits(stream, 4);
		// Next 4 bits will be either terminator or padding.
		break;

	case 4:
		// Ended on 8th bit (C.3.2).
		bits = (r_buf[0] << 4) & FI_BITS(1,1,1,1,,,,);

		if (bits != FI_BITS(1,1,1,1,,,,)) {
			// Not a valid Fast Infoset.
			throw IllegalStateException ();
		}

		fi_advance_stream_cursor(stream, 1);
		break;

	default:
		// Should never happen.
		throw IllegalStateException ();
	}

	return true;
}

bool
Element::read_namespace_attributes(FI_OctetStream *stream)
{
	if (doc.hasElements()) {
		// We only allow a BT_NAMESPACE default namespace at root.
		throw UnsupportedOperationException ();
	}

	// Read next NamespaceAttribute item using C.12.
	NSN_DS_VocabTable::TypedEntryRef ns_name;

	const FI_Octet *r_buf;
	FI_Octet bits;

	if (fi_try_read_stream(stream, &r_buf, 0, 1) < 1) {
		return false;
	}

	switch (r_buf[0] & FI_BITS(1,1,1,1,1,1,,)) {
	case FI_BITS(1,1,0,0,1,1,,):
		// Read NamespaceAttribute using C.12 (C.3.4.2: 110011).
		r_len_state = 0;

		if (!read_namespace_attribute(stream, r_len_state,
		                              doc.vocabulary, ns_name)) {
			return false;
		}

		// FIXME: This routine is currently hard-coded to check that
		// the NamespaceAttribute is doc.BT_NAMESPACE, with no prefix.
		if (*ns_name != *doc.BT_NAMESPACE
		    || !ns_name.hasIndex() || !doc.BT_NAMESPACE.hasIndex()
		    || ns_name.getIndex() != doc.BT_NAMESPACE.getIndex()
		    || ns_name.getIndex() != 2) {
			// FIXME: Implementation restriction.
			throw UnsupportedOperationException ();
		}

		fi_advance_stream_cursor(stream, r_len_state);
		break;

	case FI_BITS(1,1,1,1, 0,0,0,0):
		// Read terminator and padding (C.3.4.3: 1111 000000).
		if (fi_try_read_stream(stream, &r_buf, 0, 2) < 2) {
			return false;
		}

		bits = r_buf[1] & FI_BITS(1,1,,,,,,);
		if (bits != FI_BITS(0,0,,,,,,)) {
			// Not a valid Fast Infoset.
			throw IllegalStateException ();
		}

		fi_advance_stream_cursor(stream, 1);
		return true;

	default:
		// Not a valid Fast Infoset.
		throw IllegalStateException ();
	}

	// Look for more NamespaceAttribute items, until we reach terminator.
	return false;
}

bool
Element::read_attributes(FI_OctetStream *stream)
{
	// FIXME
	fputs("reading attributes\n", stderr);
	return false;
}

} // namespace FI
} // namespace BTech
