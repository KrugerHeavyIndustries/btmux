/*
 * Mutable implementation of Attributes.
 */

#ifndef BTECH_FI_MUTABLEATTRIBUTES_HH
#define BTECH_FI_MUTABLEATTRIBUTES_HH

#include <utility>
#include <vector>

#include "attribs.h"

#include "Exception.hh"
#include "Name.hh"

#include "Attributes.hh"

namespace BTech {
namespace FI {

class MutableAttributes : public Attributes {
private:
	typedef std::pair<DN_VocabTable::TypedEntryRef,Value> NameValue;

public:
	// Clear all attributes from this set.
	void clear () {
		attributes.clear();
	}

	// Add an attribute to this set.
	bool add (const DN_VocabTable::TypedEntryRef& name,
	          const Value& value) {
		attributes.push_back(NameValue (name, value));
		return true;
	}

	//
	// Attributes implementation.
	//

	int getLength () const {
		return attributes.size();
	}

	const DN_VocabTable::TypedEntryRef getName (int idx) const {
		if (idx < 0 || idx >= getLength()) {
			throw IndexOutOfBoundsException ();
		}

		return attributes[idx].first;
	}

	const Value& getValue (int idx) const {
		if (idx < 0 || idx >= getLength()) {
			throw IndexOutOfBoundsException ();
		}

		return attributes[idx].second;
	}

	// Note that STL modifications may change the value of this pointer, so
	// add() (and obviously clear()) may invalidate it.
	//
	// We could fix this by using pointers in the STL container, but there
	// isn't actually a problem in practice, as Attributes are meant to be
	// filled in once, then passed around as if constant.
	const FI_Name *getCName (int idx) const {
		if (idx < 0 || idx >= getLength()) {
			return 0;
		}

		return FI_Name::cast(attributes[idx].first);
	}

	// Note that STL modifications may change the value of this pointer, so
	// add() (and obviously clear()) may invalidate it.
	//
	// We could fix this by using pointers in the STL container, but there
	// isn't actually a problem in practice, as Attributes are meant to be
	// filled in once, then passed around as if constant.
	const FI_Value *getCValue (int idx) const {
		if (idx < 0 || idx >= getLength()) {
			return 0;
		}

		return FI_Value::cast(attributes[idx].second);
	}

private:
	std::vector<NameValue> attributes;
}; // class MutableAttributes

} // namespace FI
} // namespace BTech

// Some magic for C/C++ compatibility.
struct FI_tag_Attributes : public BTech::FI::MutableAttributes {
	// Cast any Attributes to an FI_Attributes.  Actually, this is only
	// technically correct for MutableAttributes, not Attributes in
	// general, but it's of no practical consequence.
	static const FI_Attributes *cast (const Attributes& ref) {
		return static_cast<const FI_Attributes *>(&ref);
	}
}; // FI_Attributes

#endif /* !BTECH_FI_MUTABLEATTRIBUTES_HH */
