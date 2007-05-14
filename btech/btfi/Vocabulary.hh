/*
 * Module implementing the vocabulary data structure required by X.891.
 */

#ifndef BTECH_FI_VOCABULARY_HH
#define BTECH_FI_VOCABULARY_HH

#include "errors.h"
#include "vocab.h"

#include "VocabSimple.hh"
#include "Name.hh"
#include "Value.hh"


namespace BTech {
namespace FI {

//
// A vocabulary consisting of the entire set of vocabulary tables.
//
class Vocabulary {
public:
	void clear ();

	// There's no real need to protect these from sane API users.
	RA_VocabTable restricted_alphabets;
	EA_VocabTable encoding_algorithms;

	PFX_DS_VocabTable prefixes;
	NSN_DS_VocabTable namespace_names;
	DS_VocabTable local_names;

	//DS_VocabTable other_ncnames;
	//DS_VocabTable other_uris;

	DV_VocabTable attribute_values;
	DV_VocabTable content_character_chunks;
	//DV_VocabTable other_strings;

	DN_VocabTable element_names;
	DN_VocabTable attribute_names;
}; // class Vocabulary

} // namespace FI
} // namespace BTech

// Some magic for C/C++ compatibility.
struct FI_tag_Vocabulary : public BTech::FI::Vocabulary {
	FI_tag_Vocabulary ();

	FI_ErrorInfo error_info;

	const BTech::FI::NSN_DS_VocabTable::TypedEntryRef BT_NAMESPACE;
}; // FI_Vocabulary

#endif // !BTECH_FI_VOCABULARY_HH
