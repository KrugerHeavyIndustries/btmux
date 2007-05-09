/*
 * Fast Infoset encoder/decoder classes.
 */

#ifndef BTECH_FI_CODEC_HH
#define BTECH_FI_CODEC_HH

#include "common.h"
#include "stream.h"

#include "Name.hh"
#include "Value.hh"
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

protected:
	// Less primitive encoding routines.
	void writeValue_bit1 (const Value& value); // C.14

	void writeName_bit2 (const DN_VocabTable::TypedEntryRef& name); // C.17
	void writeName_bit3 (const DN_VocabTable::TypedEntryRef& name); // C.18

	// Primitive encoding routines.
	void writeIdentifier (const DS_VocabTable::TypedEntryRef& id); // C.13

	void writeEncoded_bit3 (const Value& value); // C.19

	void writeNonEmptyOctets_len_bit2 (FI_PInt32 len); // C.22.3
	void writeNonEmptyOctets_len_bit5 (FI_PInt32 len); // C.23.3

	void writePInt20_bit2 (FI_PInt20 val); // C.25
	void writeUInt21_bit2 (FI_UInt21 val); // C.26
	void writePInt20_bit3 (FI_PInt20 val); // C.27

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

	// Child type iteration.

	// TODO: Move these constants somewhere common.
	enum ChildType {
		NEXT_UNKNOWN,		// bad state

		END_CHILD,		// end of child (doc, el, dtd)

		START_ELEMENT,		// start of element (doc, el)
		PROCESSING_INSTRUCTION,	// processing instruction (doc, el, dtd)
		ENTITY_REFERENCE,	// unexpanded entity reference (el)
		CHARACTERS,		// character chunk (el)
		COMMENT,		// comment (doc, el)
		DTD			// document type declaration (doc)
	}; // enum ChildType

	ChildType next () {
		ChildType child_type = next_child_type;
		next_child_type = NEXT_UNKNOWN;
		return child_type;
	}

	// Byte-level I/O routines.
	const FI_Octet *getReadBuffer (FI_Length length);

	// Bit-level I/O routines.
	unsigned int getBitOffset () const {
		return num_read_bits;
	}

	FI_Octet getBits () const {
		return partial_octet;
	}

	bool readBits (unsigned int num_bits);

	// High-level decoding routines.  May use super_step.
	bool readNext ();

	bool readXMLDecl (); // 12.3

	bool readElementName (DN_VocabTable::TypedEntryRef& name) {
		return readName_bit3(vocabulary->element_names, name);
	} // C.3.5

	bool readAttribute (DN_VocabTable::TypedEntryRef& name,
	                    Value& value); // C.4

	bool readNSAttribute
	     (NSN_DS_VocabTable::TypedEntryRef& ns_name); // C.12

protected:
	// Less primitive decoding routines.  May use sub_step.
	bool readValue_bit1 (DV_VocabTable& value_table, Value& value); // C.14

	bool readName_bit2 (DN_VocabTable& name_table,
	                    DN_VocabTable::TypedEntryRef& name); // C.17
	bool readName_bit3 (DN_VocabTable& name_table,
	                    DN_VocabTable::TypedEntryRef& name); // C.18

	// Primitive decoding routines.  May use sub_sub_step.
	bool readIdentifier (DS_VocabTable& string_table,
	                     DS_VocabTable::TypedEntryRef& id); // C.13

	bool readEncoded_bit3 (Value& value); // C.19

	// None of the following use sub_sub_step, but instead repeatedly
	// reparse a (small) set of discriminator bits.  They're short and
	// don't recurse, so it's not a big deal.
	bool readNonEmptyOctets_len_bit2 (FI_PInt32& len); // C.22.3
	bool readNonEmptyOctets_len_bit5 (FI_PInt32& len); // C.23.3

	bool readPInt20_bit2 (FI_PInt20& val); // C.25
	bool readUInt21_bit2 (FI_UInt21& val); // C.26
	bool readPInt20_bit3 (FI_PInt20& val); // C.27

	bool readPInt8 (FI_PInt8& val); // C.29

private:
	FI_OctetStream *stream;

	ChildType next_child_type;

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

// Abstract base class for objects supporting serialization via an
// Encoder/Decoder.
class Serializable {
protected:
	virtual ~Serializable () {}

public:
	virtual void write (Encoder& encoder) = 0;
	virtual void read (Decoder& decoder) = 0;
}; // class Serializable

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_CODEC_HH */
