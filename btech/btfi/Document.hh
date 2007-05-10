/*
 * Implements top-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 section 12.
 */

#ifndef BTECH_FI_DOCUMENT_HH
#define BTECH_FI_DOCUMENT_HH

#include <vector>

#include "Serializable.hh"

#include "Exception.hh"
#include "Vocabulary.hh"

namespace BTech {
namespace FI {

class Document : public Serializable {
	friend class Element;

public:
	Document ();

	// Next write()/read() will be document header.
	void start ();

	// Next write()/read() will be document trailer.
	void stop ();

	void write (Encoder& encoder) const;
	bool read (Decoder& decoder);

	// Interact with the element stack.
	bool hasElements () const {
		return !element_stack.empty();
	}

	void clearElements () {
		element_stack.clear();
	}

protected:
	void pushElement (const DN_VocabTable::TypedEntryRef& name) {
		if (!hasElements()) {
			if (saw_root_element) {
				// Not a valid Fast Infoset.
				throw IllegalStateException ();
			} else {
				saw_root_element = true;
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
	enum {
		SERIALIZE_NONE,

		SERIALIZE_HEADER,
		SERIALIZE_TRAILER
	} serialize_mode;

	std::vector<DN_VocabTable::TypedEntryRef> element_stack;

	// Write subroutines.
	void write_header (Encoder& encoder) const;
	void write_trailer (Encoder& encoder) const;

	// Read subroutines.
	bool read_header (Decoder& decoder);

	enum {
		RESET_READ_STATE,
		MAIN_READ_STATE,
	} r_state;

	enum {
		RESET_HEADER_STATE,
		MAIN_HEADER_STATE
	} r_header_state;

	bool saw_root_element;
}; // class Document

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_DOCUMENT_HH */
