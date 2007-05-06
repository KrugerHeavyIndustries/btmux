/*
 * Module implementing the vocabulary data structure required by X.891.
 */

#include "autoconf.h"

#include "Vocabulary.hh"


namespace BTech {
namespace FI {

/*
 * Vocabulary definitions.
 */

void
Vocabulary::clear ()
{
	restricted_alphabets.clear();
	encoding_algorithms.clear();

	prefixes.clear();
	namespace_names.clear();
	local_names.clear();

	other_ncnames.clear();
	other_uris.clear();

	attribute_values.clear();
	content_character_chunks.clear();
	other_strings.clear();

	element_name_surrogates.clear();
	attribute_name_surrogates.clear();
}

const PFX_DS_VocabTable::TypedEntryRef
Vocabulary::getPrefix(const char *pfx)
{
	return prefixes.getEntry(pfx);
}

const NSN_DS_VocabTable::TypedEntryRef
Vocabulary::getNamespace(const char *nsn)
{
	return namespace_names.getEntry(nsn);
}

const DS_VocabTable::TypedEntryRef
Vocabulary::getLocalName(const char *local)
{
	return local_names.getEntry(local);
}

const DN_VocabTable::TypedEntryRef
Vocabulary::getElementName(const Name& name)
{
	return element_name_surrogates.getEntry(name);
}

const DN_VocabTable::TypedEntryRef
Vocabulary::getAttributeName(const Name& name)
{
	return attribute_name_surrogates.getEntry(name);
}

} // namespace FI
} // namespace BTech
