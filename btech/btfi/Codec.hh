/*
 * Fast Infoset encoder/decoder classes.
 */

#ifndef BTECH_FI_CODEC_HH
#define BTECH_FI_CODEC_HH

#include "fiptypes.h"
#include "stream.h"
#include "encalg.h"

#include "Vocabulary.hh"

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

// TODO: Codec isn't a very accurate name.  I'll probably end up borrowing
// something from the StAX API or something, like XMLStream or something.
class Codec {
public:
	void clear ();

	void setStream (FI_OctetStream *out_stream) {
		stream = out_stream;
	}

	void setVocabulary (Vocabulary& new_vocabulary) {
		vocabulary = &new_vocabulary;
	}

	// Bit-level I/O routines.
	unsigned int getBitOffset () const {
		return num_partial_bits;
	}

	FI_Octet getBits () const {
		return partial_octet;
	}

	// Encoding algorithm support.
	void setEncodingAlgorithm (FI_VocabIndex idx);

	EA_VocabTable::TypedEntryRef encoding_algorithm;
	FI_EncodingContext encoding_context;

protected:
	Codec ();
	~Codec ();

	// Byte-level I/O.
	FI_OctetStream *stream;

	// Bit-level I/O.
	unsigned int num_partial_bits;
	FI_Octet partial_octet;

	Vocabulary *vocabulary;
}; // class Codec

class Encoder : public Codec {
public:
	// Byte-level I/O routines.
	FI_Octet *getWriteBuffer (FI_Length length);

	// Bit-level I/O routines.
	void writeBits (unsigned int num_bits, FI_Octet octet_mask);

	// High-level encoding routines.
	void writeNext (ChildType child_type);

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
}; // class Encoder

class Decoder : public Codec {
public:
	void clear ();

	// Byte-level I/O routines.
	const FI_Octet *getReadBuffer (FI_Length length);

	bool skipLength (FI_Length& length);

	// Bit-level I/O routines.
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

	// These state variables are accessible to external clients of Decoder.
	// Other than initially setting the ext_*_step variables to 0, the
	// Decoder makes no use of them.  Their main purpose is to avoid
	// replicating similar state variables on every client that needs them.
	//
	// To aid in cooperation, only one Decoder client should use these
	// variables at any one time, and the owning client should always set
	// at least the ext_super_step variable back to 0 after parsing
	// completes successfully. (In the case of errors, it is presumed that
	// the parsing loop will call clear(), which will take care of this.)
	//
	// TODO: We could add a Serializable pointer to track which object is
	// the current client, but the extra safety probably isn't needed.
	//
	// TODO: Having continuations would be a neat way to avoid having
	// these state variables to begin with, but it's a bit heavy-handed for
	// a one-off task.
	int ext_super_step, ext_sub_step, ext_sub_sub_step;
	FI_Length ext_saved_len;

private:
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
