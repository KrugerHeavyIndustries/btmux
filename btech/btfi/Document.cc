/*
 * Implements top-level encoding/decoding for Fast Infoset documents,
 * conforming to X.891 section 12 and annex A, and informed by annex C.
 *
 * Only enough of Fast Infoset is implemented to support saving and loading
 * BattleTech binary databases.  In particular, only the Document, Element, and
 * Attribute types are supported, and additional types as needed to support
 * processing these types.  The objective is to support saving and loading a
 * hierarchical structure, possibly with per-element attributes.
 *
 * However, to ensure backward compatibility, all valid Fast Infoset bitstreams
 * may be decoded, although possibly with the loss of information contained in
 * types beyond the basic three of Document, Element, and Attribute.
 *
 * Implementation restrictions:
 * 7.2.4: restricted-alphabets not written, must be processed.
 * 7.2.5: encoding-algorithms not written, error on read.
 * 7.2.7: additional-data not written, ignored on read.
 * 7.2.13: external-vocabulary not written, error on read.
 * 7.2.16: local-names contains all our identifier strings, while
 *         element-name-surrogates and attribute-name-surrogates contain all
 *         our name surrogates.  other-ncnames, other-uris, attribute-values,
 *         content-character-chunks, and other-strings are not written, but
 *         must be processed. (Some may be ignorable?)
 * 7.2.21: prefixes not written, must be processed. (Ignorable?)
 * 7.2.23: namespace-names contains our namespace name.
 * 7.2.24: [notations] unsupported.
 * 7.2.25: [unparsed entities] unsupported.
 * 7.2.26: [character encoding scheme] must not be set (UTF-8).
 * 7.2.27: [standalone] must not be set (???).
 * 7.2.28: [version] must not be set (???).
 * 7.2.29: [children] must only include the root element, [document element].
 */

#include "autoconf.h"

#include <string>

#include "stream.h"

#include "Document.hh"


namespace BTech {
namespace FI {

namespace {

const char *const NAMESPACE = "http://btonline-btech.sourceforge.net";

bool write_header(FI_OctetStream *) throw ();
bool write_trailer(FI_OctetStream *) throw ();

} // anonymous namespace

void
Document::start() throw ()
{
	start_flag = true;
	stop_flag = false;

}

void
Document::stop() throw ()
{
	start_flag = false;
	stop_flag = true;
}

void
Document::write(FI_OctetStream *stream) throw (Exception)
{
	if (start_flag) {
		writeInit();

		if (!write_header(stream)) {
			// TODO: Assign an exception for stream errors.
			throw Exception ();
		}
	} else if (stop_flag) {
		if (!write_trailer(stream)) {
			// TODO: Assign an exception for stream errors.
			throw Exception ();
		}
	} else {
		throw IllegalStateException ();
	}
}

void
Document::read(FI_OctetStream *stream) throw (Exception)
{	
	if (start_flag) {
	} else if (stop_flag) {
	} else {
		throw IllegalStateException ();
	}

	throw UnsupportedOperationException ();
}

void
Document::writeInit() throw (Exception)
{
	/*
	 * Initialize vocabulary tables.
	 */

	prefixes.clear();
	namespace_names.clear();

	// Fast Infoset built-ins.
	prefixes.add("xml"); // 7.2.21
	namespace_names.add("http://www.w3.org/XML/1998/namespace"); // 7.2.22

	// Our namespace.  Since this will be the default namespace in
	// documents we create, we don't need (or want) to define a prefix.
	// TODO: We may want to point this at a document.
	NAMESPACE_IDX = namespace_names.add(NAMESPACE);
	if (NAMESPACE_IDX == FI_VOCAB_INDEX_NULL) {
		throw IllegalStateException ();
	}

	/*
	 * Initialize XML information set properties.
	 */

}


//
// Document serialization subroutines.
//

namespace {

bool
write_header(FI_OctetStream *stream) throw ()
{
	// Reserve space.
	FI_Octet *w_buf = fi_get_stream_write_buffer(stream, 4);
	if (!w_buf) {
		return false;
	}

	// Serialize document header.
	// 11100000 00000000 (C.1.3a: identification)
	w_buf[0] = FI_BIT_1 | FI_BIT_2 | FI_BIT_3;
	w_buf[1] = 0x00;
	// 00000000 00000001 (C.1.3b: version number)
	w_buf[2] = 0x00;
	w_buf[3] = FI_BIT_8;

	// 0 (C.1.3c: padding)
	if (!fi_write_stream_bits(stream, 1, 0)) {
		return false;
	}

	return true;
}

bool
write_trailer(FI_OctetStream *stream) throw ()
{
	// Serialize document trailer.
	switch (fi_get_stream_num_bits(stream)) {
	case 0:
		// C.1.3: Ended on the 8th bit of an octet.
		break;

	case 4:
		// C.1.3: Ended on the 4th bit of an octet.
		if (!fi_flush_stream_bits(stream)) {
			return false;
		}
		break;

	default:
		// FIXME: For debugging only.
		if (!fi_write_stream_bits(stream,
		                          8 - fi_get_stream_num_bits(stream),
		                          0xFF)) {
			return false;
		}
		break;
	}

	return true;
}

typedef struct {
	FI_Length length;
	FI_Octet *octets;
} FI_OctetString;

typedef enum {
	FI_BOOLEAN_NULL,
	FI_BOOLEAN_FALSE,
	FI_BOOLEAN_TRUE
} FI_Boolean;

typedef FI_OctetString FI_URI;

typedef struct {
	FI_URI id;
	FI_OctetString data;
} FI_AdditionalDatum;

typedef struct {
	FI_Length length;
	FI_AdditionalDatum datum;
} FI_AdditionalData;

typedef enum {
	FI_VERSION_NULL,
	FI_VERSION_1_0,
	FI_VERSION_1_1
} FI_Version;

typedef struct {
	FI_AdditionalData additional_data;

	/* Additional data.  */
	/* Initial vocabulary.  */
	/* Notations.  */
	/* Unparsed entities.  */
	/* Character encoding scheme.  */
	FI_Boolean standalone;
	FI_Version version;

	/* Children.  */
} FI_Document;


/*
 * Subroutines for the optional XML declaration, conforming to section 12.3.
 */

static const char *const xml_decl[] = {
	"<?xml encoding='finf'?>",
	"<?xml encoding='finf' standalone='no'?>",
	"<?xml encoding='finf' standalone='yes'?>",
	"<?xml version='1.0' encoding='finf'?>",
	"<?xml version='1.0' encoding='finf' standalone='no'?>",
	"<?xml version='1.0' encoding='finf' standalone='yes'?>",
	"<?xml version='1.1' encoding='finf'?>",
	"<?xml version='1.1' encoding='finf' standalone='no'?>",
	"<?xml version='1.1' encoding='finf' standalone='yes'?>"
};

#define GET_XML_DECL(v,s) (xml_decl[3 * (v) + (s)])

static int
fi_write_xml_decl()
{
}

static int
fi_read_xml_decl()
{
}


/* Section 12.6.  */
static FI_Octet const fi_header[] = { 0xE0, 0x00 };

/* Section 12.7.  */
static FI_Octet const fi_version[] = { 0x00, 0x01 };

/* 12.8: Bit 8 of following octet is 0.  */

/* ECN encoding of Document.  */

/* 12.11: 4 bits of padding to complete final octet.  */


/* Based on informational section C.2.  */

/* C.2.3: 7-bit field, indicating absence/presence of optional components.  */

/* C.2.4/C.21: additional-data: length (id uri)+. */

/* C.2.5: initial-vocabulary.  */

/* C.2.6: notations.  */
/* b110000 */
/* C.11.3: <system-identifier?> <public-identifier?> */


/* C.2.7: unparsed-entities.  */

/* C.2.8: character-encoding-scheme.  */

/* C.2.9: standalone.  0x00 = FALSE, 0x01 = TRUE.  Optional.  */

/* C.2.10/C.14: version.  */

/* C.2.11: 0 or more children. (element, processing, comment, DTD.) */

/* C.2.12: 0xF: Termination bits. (Potentially 0xF0 with padding.) */


/* C.21: Sequence-Of length.  */
//1 -- 128: 0x00 -- 0x7F
//129 -- 2^20: 0x80 0x00 0x00 -- 0x8F 0xFF 0x7F

/* C.22: NonEmptyOctetString, starting at 2nd bit.  */
/* Length + octets.  */
//1 -- 64: 0x00 -- 0x3F
//65 -- 320: 0x40 -- 0x40 0xFF
//321 -- 2^32: 0x60 0xFF 0xFF 0xFE 0xBF -- 0x60 0xFF 0xFF 0xFE 0xBF

} // anonymous namespace

} // namespace FI
} // namespace BTech
