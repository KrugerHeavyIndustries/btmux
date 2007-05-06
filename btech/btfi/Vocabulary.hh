/*
 * Module implementing the vocabulary data structure required by X.891.
 */

#ifndef BTECH_FI_VOCABULARY_HH
#define BTECH_FI_VOCABULARY_HH

#include "VocabSimple.hh"
#include "Name.hh"


namespace BTech {
namespace FI {

//
// A vocabulary consisting of the entire set of vocabulary tables.
//
class Vocabulary {
public:
	void clear ();

	const PFX_DS_VocabTable::TypedEntryRef getPrefix (const char *pfx);
	const NSN_DS_VocabTable::TypedEntryRef getNamespace (const char *nsn);
	const DS_VocabTable::TypedEntryRef getLocalName (const char *local);

	const DN_VocabTable::TypedEntryRef getElementName (const Name& name);
	const DN_VocabTable::TypedEntryRef getAttributeName (const Name& name);

private:
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
}; // class Vocabulary

} // namespace FI
} // namespace BTech

#endif // !BTECH_FI_VOCABULARY_HH
