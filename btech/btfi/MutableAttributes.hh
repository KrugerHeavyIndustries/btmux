/*
 * Mutable implementation of Attributes.
 */

#ifndef BTECH_FI_MUTABLEATTRIBUTES_HH
#define BTECH_FI_MUTABLEATTRIBUTES_HH

#include <utility>
#include <vector>

#include "common.h"

#include "Attributes.hh"

namespace BTech {
namespace FI {

class MutableAttributes : public Attributes {
private:
	typedef std::pair<FI_Name,FI_Value> AttributePair;

public:
	// Clear all attributes from this set.
	void clear () throw () {
		attributes.clear();
	}

	// Add an attribute to this set.
	bool add (const FI_Name *name, const FI_Value *value)
	         throw (Exception) {
		attributes.push_back(AttributePair (*name, *value));
		return true;
	}

	//
	// Attributes implementation.
	//

	int getLength () const throw () {
		return attributes.size();
	}

	const FI_Name *getName (int idx) const throw () {
		if (idx < 0 || idx >= getLength()) {
			return 0;
		}

		return &attributes[idx].first;
	}

	const FI_Value *getValue (int idx) const throw () {
		if (idx < 0 || idx >= getLength()) {
			return 0;
		}

		return &attributes[idx].second;
	}

private:
	std::vector<AttributePair> attributes;
}; // class MutableAttributes

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_MUTABLEATTRIBUTES_HH */
