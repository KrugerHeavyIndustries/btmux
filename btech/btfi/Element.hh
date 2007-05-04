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
	const Attributes *w_attrs;
	MutableAttributes r_attrs;

	// Incremental read state.
}; // class Element

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_ELEMENT_HH */
