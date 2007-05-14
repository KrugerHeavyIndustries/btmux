/*
 * Name class.
 */

#ifndef BTECH_FI_NAME_HH
#define BTECH_FI_NAME_HH

#include "names.h"

#include "VocabSimple.hh"

namespace BTech {
namespace FI {

/*
 * Qualified name.
 */
class Name {
public:
	Name (const DS_VocabTable::TypedEntryRef& local_part,
	      const NSN_DS_VocabTable::TypedEntryRef& nsn_part = 0,
	      const PFX_DS_VocabTable::TypedEntryRef& pfx_part = 0)
	: pfx_part (pfx_part), nsn_part (nsn_part), local_part (local_part) {}

	// Comparison.  Order names lexicographically as the ordered tuple
	// (prefix, namespace, local), with omitted (empty) values sorting
	// first.
	//
	// Note that this comparison is by value, not by index.
	bool operator < (const Name& rhs) const {
		if (pfx_part < rhs.pfx_part) {
			// Lp < Rp
			return true;
		} else if (rhs.pfx_part < pfx_part) {
			// Lp > Rp
			return false;
		}

		if (nsn_part < rhs.nsn_part) {
			// Ln < Rn
			return true;
		} else if (rhs.nsn_part < nsn_part) {
			// Ln > Rn
			return false;
		}

		// Ll < Rl ?
		return local_part < rhs.local_part;
	}

	// Equality.  We consider two names equivalent if they have the same
	// namespace and local part objects (after interning).
	//
	// Note that this comparison may consider some objects equal which are
	// not equal according to < (less-than), and vice versa.
	bool operator == (const Name& rhs) const {
		return nsn_part == rhs.nsn_part
		       && local_part == rhs.local_part;
	}

	const PFX_DS_VocabTable::TypedEntryRef pfx_part;
	const NSN_DS_VocabTable::TypedEntryRef nsn_part;
	const DS_VocabTable::TypedEntryRef local_part;
}; // class Name

/*
 * Dynamic name table.
 */
class DN_VocabTable : public DynamicTypedVocabTable<Name> {
public:
	DN_VocabTable ();

protected:
	class DynamicNameEntry;

	// Create a new DynamicTypedEntry for a value.
	DynamicTypedEntry *createEntryObject (const EntryPoolPtr& value_ptr) {
		return new DynamicNameEntry (*this, value_ptr);
	}

	class DynamicNameEntry : public DynamicTypedEntry {
	public:
		DynamicNameEntry (DN_VocabTable& owner,
		                  const EntryPoolPtr& value_ptr)
		: DynamicTypedEntry (owner, value_ptr) {}

	protected:
		FI_VocabIndex acquireIndex ();
	}; // class DN_VocabTable::DynamicNameEntry
}; // class DN_VocabTable

} // namespace FI
} // namespace BTech

// Some magic for C/C++ compatibility.
struct FI_tag_Name : public BTech::FI::DN_VocabTable::TypedEntryRef {
	FI_tag_Name (const TypedEntryRef& src) : TypedEntryRef (src) {}

	// Cast any DN_VocabTable::TypedEntryRef() to a FI_Name.
	static const FI_Name *cast (const TypedEntryRef& ref) {
		return static_cast<const FI_Name *>(&ref);
	}
}; // FI_Name

#endif /* !BTECH_FI_NAME_HH */
