/*
 * An abstract base class mimicking the SAX2 Attributes interface (see
 * http://www.saxproject.org/apidoc/org/xml/sax/Attributes.html), adapted to
 * handling Fast Infoset documents, and limited to the features of interest to
 * BattleTech database documents.
 *
 * For a mutable version, suitable for use with the generator API, see
 * MutableAttributes.
 */

#ifndef BTECH_FI_ATTRIBUTES_HH
#define BTECH_FI_ATTRIBUTES_HH

#include "Name.hh"
#include "Value.hh"

namespace BTech {
namespace FI {

class Attributes {
protected:
	virtual ~Attributes () {}

public:
	virtual int getLength () const = 0;

	virtual const DN_VocabTable::TypedEntryRef getName (int idx) const = 0;

	virtual const Value& getValue (int idx) const = 0;
}; // class Attributes

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_ATTRIBUTES_HH */
