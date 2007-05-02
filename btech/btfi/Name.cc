/*
 * Generic Name class implementation.
 */

#include "autoconf.h"

#include <memory>

#include "common.h"
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

// TODO:
//
// 1) Set it up so that name surrogates are derived automatically from the
//    related name tables.
// 2) Expand the FI_NameSurrogate type to cover all possibilities.
// 3) Add a mechanism to test if a table is full before trying to add new
//    entries to it.
// 4) Extend method signatures to handle the more complex type.

// Resolving a qualified name to a name surrogate index is a somewhat
// complicated process, described in section 7.16.
FI_VocabIndex
DN_VocabTable::DynamicNameEntry::acquireIndex()
{
	// Attempt to get a vocabulary index for each component of the name.
	//
	// It's OK if some indexes are created, while others are not, as this
	// simply determines whether the strings are encoded as literals or
	// indexes (see section 7.13).
	//
	// The decoding process (section 7.16.8) is carefully defined so that
	// indexes will be created whenever it is possible to create an index;
	// index creation is not optional, so there's no need to decide whether
	// or not to invoke getIndex() on a name component.
	const bool has_pfx = (value.pfx_part != 0);
	const FI_VocabIndex pfx_idx = has_pfx
	                              ? value.pfx_part.getIndex()
	                              : FI_VOCAB_INDEX_NULL;

	const bool has_nsn = (value.nsn_part != 0);
	const FI_VocabIndex nsn_idx = has_nsn
	                              ? value.nsn_part.getIndex()
	                              : FI_VOCAB_INDEX_NULL;

	const FI_VocabIndex local_idx = value.local_part.getIndex();

	// However, we only create a name surrogate if we can get indexes for
	// all present components (section 7.16.8.2b).
	//
	// Note that names check their components on creation, so there's no
	// need to ensure here that every prefix has a namespace name.
	if ((has_pfx && pfx_idx == FI_VOCAB_INDEX_NULL)
	    || (has_nsn && nsn_idx == FI_VOCAB_INDEX_NULL)
	    || (local_idx == FI_VOCAB_INDEX_NULL)) {
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

#if 0
FI_Name *
fi_create_name(void)
{
	try {
		return (new Name ())->getProxy();
	} catch (const std::bad_alloc& e) {
		return 0;
	}
}

void
fi_destroy_name(FI_Name *obj)
{
	delete &obj->parent;
}

FI_NameType
fi_get_name_type(const FI_Name *obj)
{
	return obj->parent.getType();
}

const void *
fi_get_name(const FI_Name *obj)
{
	return obj->parent.getName();
}

int
fi_set_name(FI_Name *obj, FI_NameType type, const void *buf)
{
	return obj->parent.setName(type, buf);
}

#endif // 0
