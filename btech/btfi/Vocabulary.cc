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

} // namespace FI
} // namespace BTech
