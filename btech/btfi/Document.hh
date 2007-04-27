/*
 * Implements top-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 section 12.
 */

#ifndef BTECH_FI_DOCUMENT_HH
#define BTECH_FI_DOCUMENT_HH

#include "stream.h"
#include "vocab.hh"

#include "Element.hh"

namespace BTech {
namespace FI {

class Document : public Serializable {
public:
	Document ()
	: start_flag (false), stop_flag (false),
	  is_reading (false), is_writing (false),
	  prefixes (1), namespace_names (1) {}

	// Next write()/read() will be document header.
	void start () throw ();

	// Next write()/read() will be document trailer.
	void stop () throw ();

	// Add an element/attribute name to the initial vocabulary.
	FI_VocabIndex addElementName (const char *name) throw (Exception);
	FI_VocabIndex addAttributeName (const char *name) throw (Exception);

	void write (FI_OctetStream *stream) throw (Exception);
	void read (FI_OctetStream *stream) throw (Exception);

private:
	bool start_flag;
	bool stop_flag;

	bool is_reading;
	bool is_writing;

	void setWriting () throw (Exception);
	FI_VocabIndex addName (DN_VocabTable& table, const char *name)
	                      throw (Exception);
	bool writeVocab (FI_OctetStream *stream) throw ();

	// Fast Infoset vocabulary tables.
	RA_VocabTable restricted_alphabets;
	EA_VocabTable encoding_algorithms;

	DS_VocabTable prefixes;
	DS_VocabTable namespace_names;
	DS_VocabTable local_names;

	DS_VocabTable other_ncnames;
	DS_VocabTable other_uris;

	DS_VocabTable attribute_values;
	DS_VocabTable content_character_chunks;
	DS_VocabTable other_strings;

	DN_VocabTable element_name_surrogates;
	DN_VocabTable attribute_name_surrogates;

	// Cached vocabulary indexes.
	FI_VocabIndex NAMESPACE_IDX;
}; // class Document

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_DOCUMENT_HH */
