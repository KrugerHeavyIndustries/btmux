/*
 * Mutable implementation of Attributes.
 */

#ifndef BTECH_FI_MUTABLEATTRIBUTES_HH
#define BTECH_FI_MUTABLEATTRIBUTES_HH

#include <utility>
#include <vector>

#include "Attributes.hh"

namespace BTech {
namespace FI {

class MutableAttributes : public Attributes {
private:
	typedef std::pair<Name,Value> NameValue;

public:
	// Clear all attributes from this set.
	void clear () {
		attributes.clear();
	}

	// Add an attribute to this set.
	bool add (const Name& name, const Value& value) {
		attributes.push_back(NameValue (name, value));
		return true;
	}

	//
	// Attributes implementation.
	//

	int getLength () const {
		return attributes.size();
	}

	const Name& getName (int idx) const {
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

private:
	std::vector<NameValue> attributes;
}; // class MutableAttributes

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_MUTABLEATTRIBUTES_HH */
