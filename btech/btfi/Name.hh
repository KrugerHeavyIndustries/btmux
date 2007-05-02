/*
 * Name class.
 */

#ifndef BTECH_FI_NAME_HH
#define BTECH_FI_NAME_HH

#include "common.h"
#include "names.h"

#include "Exception.hh"
#include "vocab.hh"

namespace BTech {
namespace FI {

/*
 * Qualified name.
 */
class Name {
public:
	Name (const VocabTable::EntryRef& local_part,
	      const VocabTable::EntryRef& nsn_part = 0,
	      const VocabTable::EntryRef& pfx_part = 0)
	: pfx_part (pfx_part), nsn_part (nsn_part), local_part (local_part) {}

	// Comparison.  Order names lexicographically as the ordered tuple
	// (prefix, namespace, local), with omitted (empty) values sorting
	// first.
	//
	// Note that this comparison is by value, not by index.
	bool operator < (const Name& rValue) const {
		if (pfx_part == 0) {
			if (rValue.pfx_part == 0) {
				// 0 == 0
			} else {
				// 0 < Rp
				return true;
			}
		} else if (rValue.pfx_part == 0) {
				// Lp > 0
				return false;
		} else if (pfx_part < rValue.pfx_part) {
			// Lp < Rp
			return true;
		} else if (rValue.pfx_part < pfx_part) {
			// Lp > Rp
			return false;
		} else {
			// Lp == Rp
		}

		if (nsn_part == 0) {
			if (rValue.nsn_part == 0) {
				// 0 == 0
			} else {
				// 0 < Rn
				return true;
			}
		} else if (rValue.nsn_part == 0) {
				// Ln > 0
				return false;
		} else if (nsn_part < rValue.nsn_part) {
			// Ln < Rn
			return true;
		} else if (rValue.nsn_part < nsn_part) {
			// Ln > Rn
			return false;
		} else {
			// Ln == Rn
		}

		// Ll ? Rl
		return local_part < rValue.local_part;
	}

	const VocabTable::EntryRef pfx_part;
	const VocabTable::EntryRef nsn_part;
	const VocabTable::EntryRef local_part;

private:
	// Convenience for getting the value of a DS_VocabTable EntryRef.
	static DS_VocabTable::const_value_ref
	getValue (const VocabTable::EntryRef& ref) {
		return DS_VocabTable::getValue(ref);
	}
}; // class Name

/*
 * Dynamic name table.
 */
class DN_VocabTable : public TypedVocabTable<Name> {
public:

protected:
	class DynamicNameEntry;

	// Create a new DynamicTypedEntry for a value.
	virtual DynamicTypedEntry *createTypedEntry (const_value_ref value) {
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

struct FI_tag_Name {
public:
	FI_tag_Name (const BTech::FI::VocabTable::EntryRef& ref)
	: name_ref (ref) {}

private:
	// Convenience for getting the Name of a DN_VocabTable EntryRef.
	static const BTech::FI::Name&
	getName (const BTech::FI::VocabTable::EntryRef& ref) {
		return BTech::FI::DN_VocabTable::getValue(ref);
	}

	// Convenience for getting the C string of a DS_VocabTable EntryRef.
	static const FI_Char *
	getString (const BTech::FI::VocabTable::EntryRef& ref) {
		return BTech::FI::DS_VocabTable::getValue(ref).c_str();
	}

	const BTech::FI::VocabTable::EntryRef name_ref;
}; // FI_Name

#endif /* !BTECH_FI_NAME_HH */
