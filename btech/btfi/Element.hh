/*
 * Implements Element-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 sections 7.3, and informed by section C.3.
 */

#ifndef BTECH_FI_ELEMENT_HH
#define BTECH_FI_ELEMENT_HH

#include "Attributes.hh"

namespace BTech {
namespace FI {

class Element {
public:

private:
	const FI_Name *qualified_name;
	Attributes *attributes;
}; // class Element

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_ELEMENT_HH */
