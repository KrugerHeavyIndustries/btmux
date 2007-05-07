/*
 * Implements Element-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 sections 7.3, and informed by section C.3.
 */

#ifndef BTECH_FI_ELEMENT_HH
#define BTECH_FI_ELEMENT_HH

#include "common.h"
#include "stream.h"

#include "Document.hh"
#include "Name.hh"
#include "MutableAttributes.hh"

namespace BTech {
namespace FI {

class Element : public Serializable {
public:
	Element (Document& doc)
	: start_flag (false), stop_flag (false),
	  doc (doc), w_attrs (0) {}

	// Next write()/read() will be element header.
	void start ();

	// Next write()/read() will be element trailer.
	void stop ();

	void write (FI_OctetStream *stream);
	void read (FI_OctetStream *stream);

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
	bool start_flag;
	bool stop_flag;

	Document& doc;

	DN_VocabTable::TypedEntryRef name;

	// Write subroutines.
	void write_start (FI_OctetStream *stream);
	void write_end (FI_OctetStream *stream);
	void write_namespace_attributes (FI_OctetStream *stream);
	void write_attributes (FI_OctetStream *stream);

	const Attributes *w_attrs;

	// Read subroutines.
	bool read_start (FI_OctetStream *stream);
	bool read_end (FI_OctetStream *stream);
	bool read_namespace_attributes (FI_OctetStream *stream);
	bool read_attributes (FI_OctetStream *stream);

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

	FI_Length r_len_state;
}; // class Element

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_ELEMENT_HH */
