/*
 * Generic Name class implementation.
 */

#include "autoconf.h"

#include <memory>

#include "names.h"

#include "Exception.hh"
#include "Name.hh"


namespace BTech {
namespace FI {

/*
 * 8.5: Dynamic names: Name surrogates.  Dynamic.  Each document has 2:
 *
 * ELEMENT NAME
 * ATTRIBUTE NAME
 *
 * Name surrogates are triplets of indices into the PREFIX, NAMESPACE NAME,
 * and LOCAL NAME tables, representing qualified names.  Only the LOCAL NAME
 * index is mandatory.  The PREFIX index requires the NAMESPACE NAME index.
 *
 * The meaning of the three possible kinds of name surrogates is defined in
 * section 8.5.3.  Processing rules are defined in section 7.15 and 7.16.
 */

DN_VocabTable::DN_VocabTable()
: DynamicTypedVocabTable<value_type> (false, FI_ONE_MEG)
{
}

// Resolving a qualified name to a name surrogate index is a somewhat
// complicated process, described in section 7.16.
FI_VocabIndex
DN_VocabTable::DynamicNameEntry::acquireIndex()
{
	// Check if the name components have indexes.  It is the responsibility
	// of the caller to create indexes, if possible, before trying to get
	// an index for a name surrogate.
	//
	// A name surrogate is always created if the components have indexes,
	// and if there is sufficient space in the corresponding name surrogate
	// table.
	//
	// Note that in the case of a parser, it doesn't matter what exact
	// indexes are used by the name surrogate, as long as they have the
	// same literal values. (Such indexes are not visible in any way.)

	if (value.pfx_part
	    && (!value.pfx_part.hasIndex()
	        || value.pfx_part.getIndex() == FI_VOCAB_INDEX_NULL)) {
		return FI_VOCAB_INDEX_NULL;
	}

	if (value.nsn_part
	    && (!value.nsn_part.hasIndex()
	        || value.nsn_part.getIndex() == FI_VOCAB_INDEX_NULL)) {
		return FI_VOCAB_INDEX_NULL;
	}

	if (!value.local_part
	    || value.local_part.getIndex() == FI_VOCAB_INDEX_NULL) {
		return FI_VOCAB_INDEX_NULL;
	}

	return DynamicTypedEntry::acquireIndex();
}

} // namespace FI
} // namespace BTech


/*
 * C interface.
 */

using namespace BTech::FI;

void
fi_destroy_name(FI_Name *name)
{
	delete name;
}

int
fi_names_equal(const FI_Name *name1, const FI_Name *name2)
{
	return **name1 == **name2;
}

const char *
fi_get_name_prefix(const FI_Name *name)
{
	return (*name)->pfx_part ? (*name)->pfx_part->c_str() : 0;
}

const char *
fi_get_name_namespace_name(const FI_Name *name)
{
	return (*name)->nsn_part ? (*name)->nsn_part->c_str() : 0;
}

const char *
fi_get_name_local_name(const FI_Name *name)
{
	return (*name)->local_part->c_str();
}
