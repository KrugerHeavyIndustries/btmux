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

#include "sax.h"

namespace BTech {
namespace FI {

class Attributes {
protected:
	virtual ~Attributes () {}

public:
	virtual int getLength () const throw () = 0;

	virtual const FI_Name *getName (int idx) const throw () = 0;

	virtual const FI_Value *getValue (int idx) const throw () = 0;
}; // class Attributes

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_ATTRIBUTES_HH */
