/*
 * Fast Infoset encoder/decoder classes.
 */

#ifndef BTECH_FI_CODEC_HH
#define BTECH_FI_CODEC_HH

#include "fiptypes.h"
#include "stream.h"

#include "Vocabulary.hh"

// Section 5.5: Fast Infoset numbers bits from 1 (MSB) to 8 (LSB).
#define FI_BIT_1 0x80
#define FI_BIT_2 0x40
#define FI_BIT_3 0x20
#define FI_BIT_4 0x10
#define FI_BIT_5 0x08
#define FI_BIT_6 0x04
#define FI_BIT_7 0x02
#define FI_BIT_8 0x01

#define FI_BITS(a,b,c,d,e,f,g,h) \
	(  ((0||(a+0)) << 7) | ((0||(b+0)) << 6) \
	 | ((0||(c+0)) << 5) | ((0||(d+0)) << 4) \
	 | ((0||(e+0)) << 3) | ((0||(f+0)) << 2) \
	 | ((0||(g+0)) << 1) | ((0||(h+0))     ))

namespace BTech {
namespace FI {

enum XMLVersion {
	XML_VERSION_NONE,
	XML_VERSION_1_0,
	XML_VERSION_1_1
}; // enum XMLVersion

enum ChildType {
	NEXT_UNKNOWN,			// bad state

	END_CHILD,			// end of child (doc, el, dtd)

	START_ELEMENT,			// start of element (doc, el)
	PROCESSING_INSTRUCTION,		// processing instruction (doc, el, dtd)
	ENTITY_REFERENCE,		// unexpanded entity reference (el)
	CHARACTERS,			// character chunk (el)
	COMMENT,			// comment (doc, el)
	DTD				// document type declaration (doc)
}; // enum ChildType

enum EncodingFormat {
	ENCODE_AS_UTF8        = FI_BITS(,,0,0,,,,), // C.19.3.1
	ENCODE_AS_UTF16       = FI_BITS(,,0,1,,,,), // C.19.3.2
	ENCODE_WITH_ALPHABET  = FI_BITS(,,1,0,,,,), // C.19.3.3
	ENCODE_WITH_ALGORITHM = FI_BITS(,,1,1,,,,)  // C.19.3.4
}; // enum EncodingFormat

class Encoder {
public:
	Encoder ();

	void clear ();

	void setStream (FI_OctetStream *out_stream) {
		stream = out_stream;
	}

	void setVocabulary (Vocabulary& new_vocabulary) {
		vocabulary = &new_vocabulary;
	}

	// Byte-level I/O routines.
	FI_Octet *getWriteBuffer (FI_Length length);

	// Bit-level I/O routines.
	unsigned int getBitOffset () const {
		return num_unwritten_bits;
	}

	FI_Octet getBits () const {
		return partial_octet;
	}

	void writeBits (unsigned int num_bits, FI_Octet octet_mask);

	// High-level encoding routines.
	void writeXMLDecl (XMLVersion version, FI_Ternary standalone); // 12.3

	void writeElementName (const DN_VocabTable::TypedEntryRef& name) {
		writeName_bit3(name);
	} // C.3.5

	void writeAttribute (const DN_VocabTable::TypedEntryRef& name,
	                     const Value& value); // C.4

	void writeNSAttribute
	     (const NSN_DS_VocabTable::TypedEntryRef& ns_name); // C.12

	// Less primitive encoding routines.
	void writeName_bit2 (const DN_VocabTable::TypedEntryRef& name); // C.17
	void writeName_bit3 (const DN_VocabTable::TypedEntryRef& name); // C.18

	// Primitive encoding routines.
	void writeIdentifier (const DS_VocabTable::TypedEntryRef& id); // C.13

	void writeNonEmptyOctets_len_bit2 (FI_PInt32 len); // C.22.3
	void writeNonEmptyOctets_len_bit5 (FI_PInt32 len); // C.23.3
	void writeNonEmptyOctets_len_bit7 (FI_PInt32 len); // C.24.3

	void writePInt20_bit2 (FI_PInt20 val); // C.25
	void writeUInt21_bit2 (FI_UInt21 val); // C.26
	void writePInt20_bit3 (FI_PInt20 val); // C.27
	void writePInt20_bit4 (FI_PInt20 val); // C.28

	void writePInt8 (FI_PInt8 val); // C.29

private:
	FI_OctetStream *stream;

	// Bit-level I/O.
	unsigned int num_unwritten_bits;
	FI_Octet partial_octet;

	Vocabulary *vocabulary;
}; // class Encoder

class Decoder {
public:
	Decoder ();

	void clear ();

	void setStream (FI_OctetStream *out_stream) {
		stream = out_stream;
	}

	void setVocabulary (Vocabulary& new_vocabulary) {
		vocabulary = &new_vocabulary;
	}

	// Byte-level I/O routines.
	const FI_Octet *getReadBuffer (FI_Length length);

	bool skipLength (FI_Length& length);

	// Bit-level I/O routines.
	unsigned int getBitOffset () const {
		return num_read_bits;
	}

	FI_Octet getBits () const {
		return partial_octet;
	}

	bool readBits (unsigned int num_bits);

	// High-level decoding routines.  May use super_step.
	bool readNext (ChildType& child_type);

	bool readXMLDecl (); // 12.3

	bool readElementName (DN_VocabTable::TypedEntryRef& name) {
		return readName_bit3(vocabulary->element_names, name);
	} // C.3.5

	bool readAttribute (DN_VocabTable::TypedEntryRef& name,
	                    Value& value); // C.4

	bool readNSAttribute
	     (NSN_DS_VocabTable::TypedEntryRef& ns_name); // C.12

	// Less primitive decoding routines.  May use sub_step.
	bool readName_bit2 (DN_VocabTable& name_table,
	                    DN_VocabTable::TypedEntryRef& name); // C.17
	bool readName_bit3 (DN_VocabTable& name_table,
	                    DN_VocabTable::TypedEntryRef& name); // C.18

	// Primitive decoding routines.  May use sub_sub_step.
	bool readIdentifier (DS_VocabTable& string_table,
	                     DS_VocabTable::TypedEntryRef& id); // C.13

	// None of the following use sub_sub_step, but instead repeatedly
	// reparse a (small) set of discriminator bits.  They're short and
	// don't recurse, so it's not a big deal.
	bool readNonEmptyOctets_len_bit2 (FI_PInt32& len); // C.22.3
	bool readNonEmptyOctets_len_bit5 (FI_PInt32& len); // C.23.3
	bool readNonEmptyOctets_len_bit7 (FI_PInt32& len); // C.24.3

	bool readPInt20_bit2 (FI_PInt20& val); // C.25
	bool readUInt21_bit2 (FI_UInt21& val); // C.26
	bool readPInt20_bit3 (FI_PInt20& val); // C.27
	bool readPInt20_bit4 (FI_PInt20& val); // C.28

	bool readPInt8 (FI_PInt8& val); // C.29

private:
	FI_OctetStream *stream;

	// Bit-level I/O.
	unsigned int num_read_bits;
	FI_Octet partial_octet;

	Vocabulary *vocabulary;

	int super_step, sub_step, sub_sub_step;

	FI_Length saved_len;

	DN_VocabTable::TypedEntryRef saved_name;

	PFX_DS_VocabTable::TypedEntryRef saved_pfx_part;
	NSN_DS_VocabTable::TypedEntryRef saved_nsn_part;
	DS_VocabTable::TypedEntryRef saved_local_part;
}; // class Decoder

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_CODEC_HH */
