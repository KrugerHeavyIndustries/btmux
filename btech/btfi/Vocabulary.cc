/*
 * Module implementing the vocabulary data structure required by X.891.
 */

#include "autoconf.h"

#include "errors.h"
#include "vocab.h"

#include "Name.hh"

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

	//other_ncnames.clear();
	//other_uris.clear();

	attribute_values.clear();
	content_character_chunks.clear();
	//other_strings.clear();

	element_names.clear();
	attribute_names.clear();
}

} // namespace FI
} // namespace BTech


/*
 * C interface.
 */

using namespace BTech::FI;

namespace {

const char *const BT_NAMESPACE_URI = "http://btonline-btech.sourceforge.net";

} // anonymous namespace

FI_tag_Vocabulary::FI_tag_Vocabulary()
: BT_NAMESPACE (namespace_names.getEntry(BT_NAMESPACE_URI))
{
	FI_CLEAR_ERROR(error_info);
}

FI_Vocabulary *
fi_create_vocabulary(void)
{
	try {
		return new FI_Vocabulary ();
	} catch (const std::bad_alloc& e) {
		return 0;
	}
}

void
fi_destroy_vocabulary(FI_Vocabulary *vocab)
{
	delete vocab;
}

// Namespaces in XML 1.0 (Second Edition)
// http://www.w3.org/TR/2006/REC-xml-names-20060816
//
// Section 6.2, paragraph 2:
//
// A default namespace declaration applies to all unprefixed element names
// within its scope. Default namespace declarations do not apply directly to
// attribute names; the interpretation of unprefixed attributes is determined
// by the element on which they appear.

FI_Name *
fi_create_element_name(FI_Vocabulary *vocab, const char *name)
{
	try {
		const Name e_name (vocab->local_names.getEntry(name),
		                   vocab->BT_NAMESPACE);

		return new FI_Name (vocab->element_names.getEntry(e_name));
	} catch (const Exception& e) {
		// TODO: We haven't exposed FI_Vocabulary's error_info.
		FI_SET_ERROR(vocab->error_info, FI_ERROR_EXCEPTION);
		return FI_VOCAB_INDEX_NULL;
	} // FIXME: Catch all exceptions (at all C/C++ boundaries in all APIs)
}

FI_Name *
fi_create_attribute_name(FI_Vocabulary *vocab, const char *name)
{
	try {
		const Name a_name (vocab->local_names.getEntry(name));

		return new FI_Name (vocab->attribute_names.getEntry(a_name));
	} catch (const Exception& e) {
		// TODO: We haven't exposed FI_Vocabulary's error_info.
		FI_SET_ERROR(vocab->error_info, FI_ERROR_EXCEPTION);
		return FI_VOCAB_INDEX_NULL;
	} // FIXME: Catch all exceptions (at all C/C++ boundaries in all APIs)
}
