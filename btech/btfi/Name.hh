/*
 * Generic Name class.
 */

#ifndef BTECH_FI_NAME_HH
#define BTECH_FI_NAME_HH

#include "common.h"
#include "generic.h"

namespace BTech {
namespace FI {

// Forward declaration.
class Name;

} // namespace FI
} // namespace BTech

struct FI_tag_Name {
	FI_tag_Name (BTech::FI::Name& parent) : parent (parent) {}

	BTech::FI::Name& parent;
}; // FI_Name

namespace BTech {
namespace FI {

class Name {
public:
	Name () : proxy (*this), name_type (FI_NAME_AS_NULL), name_buf (0) {}

	~Name ();

	// Assignment.
	Name (const Name& src)
	: proxy (*this), name_type(FI_NAME_AS_NULL), name_buf (0) {
		if (!setName(src.name_type, src.name_buf)) {
			// TODO: Need a more specific Exception.
			throw Exception ();
		}
	}

	Name& operator= (const Name& src) {
		if (!setName(src.name_type, src.name_buf)) {
			// TODO: Need a more specific Exception.
			throw Exception ();
		}

		return *this;
	}

	bool setName (FI_NameType type, const void *buf);

	// Accessors.
	FI_NameType getType () const {
		return name_type;
	}

	const void *getName () const {
		return name_buf;
	}

	// Proxy handling.
	const FI_Name *getProxy () const {
		return &proxy;
	}

	FI_Name *getProxy () {
		return &proxy;
	}

private:
	FI_Name proxy;

	FI_NameType name_type;
	char *name_buf;
}; // class Name

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_NAME_HH */
