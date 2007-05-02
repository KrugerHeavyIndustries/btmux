/*
 * Implements Element-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 sections 7.3, and informed by section C.3.
 */

#ifndef BTECH_FI_ELEMENT_HH
#define BTECH_FI_ELEMENT_HH

#include "common.h"
#include "stream.h"

#include "vocab.hh"

#include "Document.hh"
#include "Name.hh"
#include "Attributes.hh"

namespace BTech {
namespace FI {

class Element : public Serializable {
public:
	Element (Document& doc)
	: start_flag (false), stop_flag (false),
	  doc (doc), w_attrs (0) {}

	// Next write()/read() will be element header.
	void start (const DN_VocabTable::TypedEntryRef& name,
	            const Attributes& attrs);

	// Next write()/read() will be element trailer.
	void stop (const DN_VocabTable::TypedEntryRef& name);

	void write (FI_OctetStream *stream);
	void read (FI_OctetStream *stream);

private:
	bool start_flag;
	bool stop_flag;

	Document& doc;

	DN_VocabTable::TypedEntryRef w_name;
	const Attributes *w_attrs;
}; // class Element

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_ELEMENT_HH */
