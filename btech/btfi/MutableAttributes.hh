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
	typedef std::pair<FI_Name,Value> NameValue;

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

	const DN_VocabTable::TypedEntryRef& getNameRef (int idx) const {
		if (idx < 0 || idx >= getLength()) {
			throw IndexOutOfBoundsException ();
		}

		return attributes[idx].first.getNameRef();
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

		return &attributes[idx].first;
	}

	const FI_Value *getCValue (int idx) const {
		if (idx < 0 || idx >= getLength()) {
			return 0;
		}

		return attributes[idx].second.getProxy();
	}

private:
	std::vector<NameValue> attributes;
}; // class MutableAttributes

} // namespace FI
} // namespace BTech

struct FI_tag_Attributes {
	BTech::FI::MutableAttributes impl;
}; // FI_Attributes

#endif /* !BTECH_FI_MUTABLEATTRIBUTES_HH */
