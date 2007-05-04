/*
 * Implements top-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 section 12.
 */

#ifndef BTECH_FI_DOCUMENT_HH
#define BTECH_FI_DOCUMENT_HH

#include <vector>

#include "stream.h"

#include "Name.hh"
#include "Vocabulary.hh"

namespace BTech {
namespace FI {

class Document : public Serializable {
	friend class Element;

public:
	static const int START_ELEMENT = 2;
	static const int END_ELEMENT = 3;

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
	int next () {
		int child_type = next_child_type;
		next_child_type = 0;
		return child_type;
	}

protected:
	// Set next child type to be returned.
	void setNext (int child_type) {
		next_child_type = child_type;
	}

	// Interact with the element stack.
	bool hasElements () const {
		return !element_stack.empty();
	}

	void pushElement (const DN_VocabTable::TypedEntryRef& name) {
		element_stack.push_back(name);
	}

	const DN_VocabTable::TypedEntryRef popElement () {
		const DN_VocabTable::TypedEntryRef ref = element_stack.back();
		element_stack.pop_back();
		return ref;
	}

private:
	void clearVocab ();
#if 0 // defined(FI_USE_INITIAL_VOCABULARY)
	bool writeVocab (FI_OctetStream *stream);
#endif // FI_USE_INITIAL_VOCABULARY

	bool start_flag;
	bool stop_flag;

	// Child handling.
	int next_child_type;

	std::vector<DN_VocabTable::TypedEntryRef> element_stack;

	// Fast Infoset vocabulary tables.  Most aren't used yet.
	RA_VocabTable restricted_alphabets;
	EA_VocabTable encoding_algorithms;

	PFX_DS_VocabTable prefixes;
	NSN_DS_VocabTable namespace_names;
	DS_VocabTable local_names;

	DS_VocabTable other_ncnames;
	DS_VocabTable other_uris;

	DS_VocabTable attribute_values;
	DS_VocabTable content_character_chunks;
	DS_VocabTable other_strings;

	DN_VocabTable element_name_surrogates;
	DN_VocabTable attribute_name_surrogates;

	// Incremental read state.

public:
	// Cached vocabulary entries.  Need to put them at the end, to be sure
	// they're constructed after the vocabulary tables.
	const NSN_DS_VocabTable::TypedEntryRef BT_NAMESPACE;
}; // class Document

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_DOCUMENT_HH */
