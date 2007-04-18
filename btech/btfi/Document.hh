/*
 * Implements top-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 section 12.
 */

#ifndef BTECH_FI_DOCUMENT_HH
#define BTECH_FI_DOCUMENT_HH

#include "stream.h"

namespace BTech {
namespace FI {

class Document : public Serializable {
public:
	Document () : start_flag(false), stop_flag(false) {}
	~Document () {}

	// Next write()/read() will be document header.
	void start () throw ();

	// Next write()/read() will be document trailer.
	void stop () throw ();

	void write (FI_OctetStream *stream) throw (Exception);
	void read (FI_OctetStream *stream) throw (Exception);

private:
	bool start_flag;
	bool stop_flag;
}; // class Document

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_DOCUMENT_HH */
