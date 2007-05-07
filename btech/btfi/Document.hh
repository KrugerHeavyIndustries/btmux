/*
 * Implements top-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 section 12.
 */

#ifndef BTECH_FI_DOCUMENT_HH
#define BTECH_FI_DOCUMENT_HH

#include <vector>

#include "common.h"
#include "stream.h"

#include "Exception.hh"
#include "Vocabulary.hh"

namespace BTech {
namespace FI {

class Document : public Serializable {
	friend class Element;

public:
	enum ChildType {
		NO_CHILD,

		START_ELEMENT,
		END_ELEMENT
	}; // enum ChildType

	Document ();

	// Next write()/read() will be document header.
	void start ();

	// Next write()/read() will be document trailer.
	void stop ();

	void write (FI_OctetStream *stream);
	void read (FI_OctetStream *stream);

	// Get a vocabulary table reference for an element/attribute name in
	// the default namespace.
	const DN_VocabTable::TypedEntryRef getElementName (const char *name);
	const DN_VocabTable::TypedEntryRef getAttributeName (const char *name);

	// Does the document have any more children?
	bool hasNext () const {
		return next_child_type != 0;
	}

	// Get the type of the next child.
	ChildType next () {
		ChildType child_type = next_child_type;
		next_child_type = NO_CHILD;
		return child_type;
	}

protected:
	// Interact with the element stack.
	bool hasElements () const {
		return !element_stack.empty();
	}

	void pushElement (const DN_VocabTable::TypedEntryRef& name) {
		if (!hasElements()) {
			if (r_saw_root_element) {
				// Not a valid Fast Infoset.
				throw IllegalStateException ();
			} else {
				r_saw_root_element = true;
			}
		}

		element_stack.push_back(name);
	}

	const DN_VocabTable::TypedEntryRef popElement () {
		// TODO: Throw our own kind of exception here.
		const DN_VocabTable::TypedEntryRef ref = element_stack.back();
		element_stack.pop_back();
		return ref;
	}

private:
#if 0 // defined(FI_USE_INITIAL_VOCABULARY)
	bool writeVocab (FI_OctetStream *stream);
#endif // FI_USE_INITIAL_VOCABULARY

	bool start_flag;
	bool stop_flag;

	// Child handling.
	ChildType next_child_type;

	std::vector<DN_VocabTable::TypedEntryRef> element_stack;

	Vocabulary vocabulary;

	// Write subroutines.
	void write_header (FI_OctetStream *stream);
	void write_trailer (FI_OctetStream *stream);

	// Read subroutines.
	bool read_header (FI_OctetStream *stream);
	bool read_trailer (FI_OctetStream *stream);

	void read_next (FI_OctetStream *stream);

	enum {
		RESET_READ_STATE,
		MAIN_READ_STATE,
		NEXT_PART_READ_STATE
	} r_state;

	enum {
		RESET_HEADER_STATE,
		XML_DECL_HEADER_STATE,
		MAIN_HEADER_STATE
	} r_header_state;

	bool r_saw_root_element;

	FI_Length r_len_state;

public:
	// Cached vocabulary entries.  Need to put them at the end, to be sure
	// they're constructed after the vocabulary tables.
	const NSN_DS_VocabTable::TypedEntryRef BT_NAMESPACE;
}; // class Document

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_DOCUMENT_HH */
