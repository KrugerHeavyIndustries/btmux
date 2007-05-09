/*
 * Implements Element-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 sections 7.3, and informed by section C.3.
 */

#ifndef BTECH_FI_ELEMENT_HH
#define BTECH_FI_ELEMENT_HH

#include "Codec.hh"

#include "Document.hh"
#include "Vocabulary.hh"
#include "Name.hh"
#include "MutableAttributes.hh"

namespace BTech {
namespace FI {

class Element : public Serializable {
public:
	Element (Document& doc);

	// Next write()/read() will be element header.
	void start (const NSN_DS_VocabTable::TypedEntryRef& ns_name);

	// Next write()/read() will be element trailer.
	void stop ();

	void write (Encoder& encoder);
	void read (Decoder& decoder);

	// Set element properties.
	void setName (const DN_VocabTable::TypedEntryRef& name) {
		this->name = name;
	}

	void setAttributes (const Attributes& attrs) {
		w_attrs = &attrs;
	}

	// Get element properties.
	const DN_VocabTable::TypedEntryRef& getName () const {
		return name;
	}

	const Attributes& getAttributes () const {
		return r_attrs;
	}

private:
	enum {
		SERIALIZE_NONE,

		SERIALIZE_START,
		SERIALIZE_END,
	} serialize_mode;

	Document& doc;

	NSN_DS_VocabTable::TypedEntryRef saved_ns_name;
	DN_VocabTable::TypedEntryRef name;

	// Write subroutines.
	void write_start (Encoder& encoder);
	void write_end (Encoder& encoder);
	void write_namespace_attributes (Encoder& encoder);
	void write_attributes (Encoder& encoder);

	const Attributes *w_attrs;

	// Read subroutines.
	bool read_start (Decoder& decoder);
	bool read_namespace_attributes (Decoder& decoder);
	bool read_attributes (Decoder& decoder);

	MutableAttributes r_attrs;

	enum {
		RESET_READ_STATE,
		MAIN_READ_STATE,
		NEXT_PART_READ_STATE
	} r_state;

	enum {
		RESET_ELEMENT_STATE,
		NS_DECL_ELEMENT_STATE,
		NAME_ELEMENT_STATE,
		ATTRS_ELEMENT_STATE
	} r_element_state;

	bool r_has_attrs;

	bool r_saw_an_attribute;
}; // class Element

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_ELEMENT_HH */
