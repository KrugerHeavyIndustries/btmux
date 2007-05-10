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

#include <cassert>

#include "Codec.hh"

#include "Vocabulary.hh"
#include "Name.hh"
#include "Value.hh"
#include "MutableAttributes.hh"

#include "Element.hh"


namespace BTech {
namespace FI {

Element::Element(Document& doc)
: serialize_mode (SERIALIZE_NONE), doc (doc), w_attrs (0)
{
}

void
Element::start(const NSN_DS_VocabTable::TypedEntryRef& ns_name)
{
	serialize_mode = SERIALIZE_START;
	r_state = RESET_READ_STATE;
	saved_ns_name = ns_name;
}

void
Element::stop()
{
	serialize_mode = SERIALIZE_END;
	r_state = RESET_READ_STATE;
}

void
Element::write(Encoder& encoder) const
{
	switch (serialize_mode) {
	case SERIALIZE_START:
		// Write element start.
		write_start(encoder);

		doc.pushElement(name);
		break;

	case SERIALIZE_END:
		// Write element end.
		if (!doc.hasElements()) {
			// Tried to pop too many elements.
			throw IllegalStateException ();
		}

		doc.popElement(); // XXX: don't need to restore name...

		write_end(encoder);
		break;

	default:
		// start()/stop() wasn't called.
		throw IllegalStateException ();
	}
}

bool
Element::read(Decoder& decoder)
{
	switch (serialize_mode) {
	case SERIALIZE_START:
		switch (r_state) {
		case RESET_READ_STATE:
			r_state = MAIN_READ_STATE;
			r_element_state = RESET_ELEMENT_STATE;
			// FALLTHROUGH

		case MAIN_READ_STATE:
			// Try to read element start.
			if (!read_start(decoder)) {
				return false;
			}

			doc.pushElement(name);
			break;
		}
		break;

	case SERIALIZE_END:
		switch (r_state) {
		case RESET_READ_STATE:
			if (!doc.hasElements()) {
				// Tried to pop too many elements.
				throw IllegalStateException ();
			}

			name = doc.popElement(); // need name for SAX API
			break;

		default:
			// Should never happen.
			throw AssertionFailureException ();
		}
		break;

	default:
		// start()/stop() wasn't called.
		throw IllegalStateException ();
	}

	return true;
}


/*
 * Element serialization subroutines.
 */

void
Element::write_start(Encoder& encoder) const
{
	// Pad out bitstream so we start on the 1st bit of an octet.
	switch (encoder.getBitOffset()) {
	case 0:
		// C.3.2: Starting at 1st bit of this octet.
		break;

	case 4:
		// C.2.11.1, C.3.7.1: Add 0000 padding.
		// C.3.2: Starting at 1st bit of next octet.
		encoder.writeBits(4, FI_BITS(,,,,0,0,0,0));
		break;

	default:
		// Shouldn't happen.
		throw IllegalStateException ();
	}

	// Write identification (C.2.11.2, C.3.7.2: 0).
	// Write attributes presence flag (C.3.3).
	encoder.writeBits(2, FI_BITS(0, w_attrs->getLength(),,,,,,));

	// Write namespace-attributes (C.3.4).
	write_namespace_attributes(encoder);

	// Write qualified-name (C.18).
	encoder.writeElementName(name);

	// Write attributes (C.3.6).
	write_attributes(encoder);

	// Write children (C.3.7).  This is handled by the respective child
	// serialization routines, so we don't need to do anything here.
}

void
Element::write_end(Encoder& encoder) const
{
	// Write termination (C.3.8: 1111).
	switch (encoder.getBitOffset()) {
	case 0:
		encoder.writeBits(4, FI_BITS(1,1,1,1,,,,));
		break;

	case 4:
		encoder.writeBits(4, FI_BITS(,,,,1,1,1,1));
		break;

	default:
		// Not a valid Fast Infoset.
		throw IllegalStateException ();
	}
}

void
Element::write_namespace_attributes(Encoder& encoder) const
{
	assert(encoder.getBitOffset() == 2);

	if (doc.hasElements()) {
		// We only write the BT_NAMESPACE default namespace at root.
		return;
	}

	// Write identification (C.3.4.1: 1110, 00).
	encoder.writeBits(6, FI_BITS(,,1,1,1,0, 0,0));

	// Write namespace-attributes using C.12 (C.3.4.2: 110011).
	// XXX: We only have 1 namespace-attribute, which corresponds to index
	// 2, our default namespace, with no prefix.
	// FIXME: This needs to be looked up, not hardcoded.
	encoder.writeBits(6, FI_BITS(1,1,0,0,1,1,,));

	encoder.writeNSAttribute(saved_ns_name);

	// Write termination (C.3.4.3: 1111, 0000 00).
	assert(encoder.getBitOffset() == 0);

	FI_Octet *w_buf = encoder.getWriteBuffer(1);

	w_buf[0] = FI_BITS(1,1,1,1, 0,0,0,0);

	encoder.writeBits(2, FI_BITS(0,0,,,,,,));
}

void
Element::write_attributes(Encoder& encoder) const
{
	if (!w_attrs->getLength()) {
		// No attributes.
		return;
	}

	assert(encoder.getBitOffset() == 0);

	for (int ii = 0; ii < w_attrs->getLength(); ii++) {
		// Write identification (C.3.6.1: 0).
		encoder.writeBits(1, FI_BITS(0,,,,,,,));

		// Write attribute using C.4 (C.3.6.1).
		encoder.writeAttribute(w_attrs->getName(ii),
		                       w_attrs->getValue(ii));
	}

	// Write termination (C.3.6.2).
	assert(encoder.getBitOffset() == 0); // C.4.2

	encoder.writeBits(4, FI_BITS(1,1,1,1,,,,));
}


/*
 * Element unserialization subroutines.
 */

bool
Element::read_start(Decoder& decoder)
{
	FI_Octet bits;

reparse:
	switch (r_element_state) {
	case RESET_ELEMENT_STATE:
		assert(decoder.getBitOffset() == 1);

		// Peek at the second bit to check if we have attributes.
		r_attrs.clear();

		r_has_attrs = decoder.getBits() & FI_BIT_2;

		decoder.readBits(1);

		// Check if we have namespace attributes (C.3.4.1: 1110 00).
		bits = decoder.getBits() & FI_BITS(,,1,1,1,1,1,1);

		if (bits != FI_BITS(,,1,1,1,0, 0,0)) {
			if (!doc.hasElements()) {
				// Require BT_NAMESPACE on root element.
				// FIXME: Implementation restriction.
				throw UnsupportedOperationException ();
			}

			// Skip directly to NAME_ELEMENT_STATE.
			r_element_state = NAME_ELEMENT_STATE;
			goto reparse; // already commented on in Document
		}

		decoder.readBits(6);

		// FIXME: As an implementation restriction, we only allow
		// namespace attributes at the root level.
		if (doc.hasElements()) {
			throw UnsupportedOperationException ();
		}

		r_element_state = NS_DECL_ELEMENT_STATE;
		r_attr_state = RESET_ATTR_STATE;
		// FALLTHROUGH

	case NS_DECL_ELEMENT_STATE:
		// Parse namespace attributes.
		if (!read_namespace_attributes(decoder)) {
			return false;
		}

		r_element_state = NAME_ELEMENT_STATE;
		// FALLTHROUGH

	case NAME_ELEMENT_STATE:
		// Parse element name.
		if (!decoder.readElementName(name)) {
			return false;
		}

		if (!r_has_attrs) {
			// Proceed directly to parsing children.
			break;
		}

		r_element_state = ATTRS_ELEMENT_STATE;
		r_attr_state = RESET_ATTR_STATE;
		r_saw_an_attribute = false;
		// FALLTHROUGH

	case ATTRS_ELEMENT_STATE:
		// Parse attributes.
		if (!read_attributes(decoder)) {
			return false;
		}

		if (!r_saw_an_attribute) {
			// Not a valid Fast Infoset.
			throw IllegalStateException ();
		}
		break;
	}

	return true;
}

bool
Element::read_namespace_attributes(Decoder& decoder)
{
	if (doc.hasElements()) {
		// We only allow a BT_NAMESPACE default namespace at root.
		throw UnsupportedOperationException ();
	}

	// Read next NamespaceAttribute item using C.12.
	NSN_DS_VocabTable::TypedEntryRef ns_name;

	for (;;) {
		switch (r_attr_state) {
		case RESET_ATTR_STATE:
			if (!decoder.readBits(6)) {
				return false;
			}

			switch (decoder.getBits() & FI_BITS(1,1,1,1,1,1,,)) {
			case FI_BITS(1,1,0,0,1,1,,):
				// NamespaceAttribute (C.3.4.2: 110011).
				break;

			case FI_BITS(1,1,1,1, 0,0,/*0*/,/*0*/):
				// Terminator + padding (C.3.4.3: 1111 000000).
				if (decoder.getBits()
				    != FI_BITS(1,1,1,1, 0,0,0,0)) {
					// Not a valid Fast Infoset.
					throw IllegalStateException ();
				}

				decoder.readBits(2);

				r_attr_state = END_ATTR_STATE;
				continue;

			default:
				// Not a valid Fast Infoset.
				throw IllegalStateException ();
			}

			r_attr_state = MAIN_ATTR_STATE;
			// FALLTHROUGH

		case MAIN_ATTR_STATE:
			// Read NamespaceAttribute using C.12 (C.3.4.2: 110011).
			if (!decoder.readNSAttribute(ns_name)) {
				return false;
			}

			// FIXME: This routine is currently hard-coded to check
			// that the NamespaceAttribute == saved_ns_name.
			if (*ns_name != *saved_ns_name) {
				// FIXME: Implementation restriction.
				throw UnsupportedOperationException ();
			}

			r_attr_state = RESET_ATTR_STATE;
			break;

		case END_ATTR_STATE:
			// Read terminator and padding (C.3.4.3: 1111 000000).
			if (!decoder.readBits(2)) {
				return false;
			}

			if ((decoder.getBits() & FI_BIT_1)
			    || (decoder.getBits() & FI_BIT_2)) {
				// Not a valid Fast Infoset.
				throw IllegalStateException ();
			}
			return true;

		default:
			// Should never happen.
			throw AssertionFailureException ();
		}
	}
}

bool
Element::read_attributes(Decoder& decoder)
{
	// Read attributes (C.3.6).
	DN_VocabTable::TypedEntryRef name;
	Value value;

	FI_Octet bits;

	for (;;) {
		switch (r_attr_state) {
		case RESET_ATTR_STATE:
			assert(decoder.getBitOffset() == 0);

			if (!decoder.readBits(1)) {
				return false;
			}

			if (decoder.getBits() & FI_BIT_1) {
				// Check for terminator (C.3.6.2).
				decoder.readBits(3);

				bits = decoder.getBits() & FI_BITS(1,1,1,1,,,,);

				if (bits != FI_BITS(1,1,1,1,,,,)) {
					// Not a valid Fast Infoset.
					throw IllegalStateException ();
				}
				return true;
			}

			r_attr_state = MAIN_ATTR_STATE;
			// FALLTHROUGH

		case MAIN_ATTR_STATE:
			// Identified as attribute (C.3.6.1).
			if (!decoder.readAttribute(name, value)) {
				return false;
			}

			r_attrs.add(name, value);
			r_saw_an_attribute = true;

			r_attr_state = RESET_ATTR_STATE;
			break;

		default:
			// Should never happen.
			throw AssertionFailureException ();
		}
	}
}

} // namespace FI
} // namespace BTech
