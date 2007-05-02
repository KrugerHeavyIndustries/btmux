/*
 * Implements top-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 section 12.
 */

#ifndef BTECH_FI_DOCUMENT_HH
#define BTECH_FI_DOCUMENT_HH

#include "stream.h"

#include "vocab.hh"
#include "Name.hh"

namespace BTech {
namespace FI {

class Document : public Serializable {
public:
	Document ();

	// Next write()/read() will be document header.
	void start ();

	// Next write()/read() will be document trailer.
	void stop ();

	// Increase nesting depth.
	void decreaseDepth () {
		// TODO: Check nesting_depth >= 0.
		nesting_depth--;
	}

	// Decrease nesting depth.
	void increaseDepth () {
		// TODO: Check nesting_depth <= MAX.
		nesting_depth++;
	}

	// Get nesting depth.
	int getDepth () const {
		return nesting_depth;
	}

	// Get a vocabulary table reference for an element/attribute name in
	// the default namespace.
	VocabTable::EntryRef getElementNameRef (const char *name);
	VocabTable::EntryRef getAttributeNameRef (const char *name);

	void write (FI_OctetStream *stream);
	void read (FI_OctetStream *stream);

private:
	bool start_flag;
	bool stop_flag;

	bool is_reading;
	bool is_writing;

	void setWriting ();
	bool writeVocab (FI_OctetStream *stream);

	int nesting_depth;

	// Fast Infoset vocabulary tables.
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

	// Cached vocabulary entries.
	const VocabTable::EntryRef BT_NAMESPACE;
}; // class Document

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_DOCUMENT_HH */
