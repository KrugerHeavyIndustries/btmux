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

namespace BTech {
namespace FI {

typedef struct FI_tag_Name FI_Name; // FIXME: define elsewhere
typedef struct FI_tag_Value FI_Value; // FIXME: define elsewhere

class Attributes {
protected:
	virtual ~Attributes () {}

public:
	virtual int getIndex (const FI_Name *name) const throw () = 0;

	virtual int getLength () const throw () = 0;

	virtual const FI_Value *getValue (int index) const throw () = 0;

	// Convenience method for getting a value directly from a name.
	virtual const FI_Value *getValue (const FI_Name *name) const throw () {
		const int idx = getIndex(name);

		return (idx == -1) ? 0 : getValue(idx);
	}
}; // class Attributes

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_ATTRIBUTES_HH */
