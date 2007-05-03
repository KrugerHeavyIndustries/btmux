/*
 * Name class.
 */

#ifndef BTECH_FI_NAME_HH
#define BTECH_FI_NAME_HH

#include "common.h"
#include "names.h"

#include "Vocabulary.hh"

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
	bool operator < (const Name& rValue) const {
		if (pfx_part < rValue.pfx_part) {
			// Lp < Rp
			return true;
		} else if (rValue.pfx_part < pfx_part) {
			// Lp > Rp
			return false;
		}

		if (nsn_part < rValue.nsn_part) {
			// Ln < Rn
			return true;
		} else if (rValue.nsn_part < nsn_part) {
			// Ln > Rn
			return false;
		}

		// Ll < Rl ?
		return local_part < rValue.local_part;
	}

	const PFX_DS_VocabTable::TypedEntryRef pfx_part;
	const NSN_DS_VocabTable::TypedEntryRef nsn_part;
	const DS_VocabTable::TypedEntryRef local_part;
}; // class Name

/*
 * Dynamic name table.
 */
class DN_VocabTable : public TypedVocabTable<Name> {
protected:
	class DynamicNameEntry;

	// Create a new DynamicTypedEntry for a value.
	DynamicTypedEntry *createTypedEntry (const_value_ref value) {
		return new DynamicNameEntry (*this, value);
	}

	class DynamicNameEntry : public DynamicTypedEntry {
	public:
		DynamicNameEntry (DN_VocabTable& table, const_value_ref value)
		: DynamicTypedEntry (table, value) {}

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
