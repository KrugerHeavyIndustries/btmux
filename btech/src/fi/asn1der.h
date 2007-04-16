/*
 * ASN.1 DER encoder/decoder, sufficient for encoding BTech databases as Fast
 * Infoset documents.  See ITU X.690 for the full specification of ASN.1 DER,
 * and ITU X.680 for the full specification of ASN.1.
 *
 * This implementation is tightly coupled to our Fast Infoset implementation,
 * and should not be used as a general-purpose ASN.1 implementation.
 */

#ifndef BTECH_ASN1DER_H
#define BTECH_ASN1DER_H

#include <stddef.h>


/*
 * ASN.1 tag classes, as defined in ITU X.680, section 8.1.  We don't currently
 * use any of these, but we reserve the namespace for future use.
 */
typedef enum {
	ASN1_TAG_UNIVERSAL = 0x0,	/* universal class */
	ASN1_TAG_APPLICATION = 0x1,	/* application class */
	ASN1_TAG_CONTEXT = 0x2,		/* context-specific class */
	ASN1_TAG_PRIVATE = 0x3		/* private class */
} ASN1_Tag_Class;

/*
 * ASN.1 universal tags, as defined in ITU X.680, section 8.4.  Since these are
 * the basic universal ASN.1 tags, we don't use a class-specific prefix.
 */
typedef enum {
	/* 0 reserved for encoding rules */
	ASN1_TAG_BOOLEAN = 1,		/* boolean */
	ASN1_TAG_INTEGER = 2,		/* integer */
	ASN1_TAG_BITS = 3,		/* bitstring */
	ASN1_TAG_OCTETS = 4,		/* octetstring */
	ASN1_TAG_NULL = 5,		/* null */
	ASN1_TAG_OID = 6,		/* object identifier */
	ASN1_TAG_ODESC = 7,		/* object descriptor */
	ASN1_TAG_EXTERNAL = 8,		/* external and instance-of */
	ASN1_TAG_REAL = 9,		/* real */
	ASN1_TAG_ENUM = 10,		/* enumerated */
	ASN1_TAG_PDV = 11,		/* embedded-pdv */
	ASN1_TAG_UTF8 = 12,		/* UTF-8 string */
	ASN1_TAG_ROID = 13,		/* relative object identifier */
	/* 14-15 reserved for future editions */
	ASN1_TAG_SEQ = 16,		/* sequence and sequence-of */
	ASN1_TAG_SET = 17,		/* set and set-of */
	/* 18-22 are various character string types */
	/* 23-24 are various time types */
	/* 25-30 are more character string types */
	/* 31- reserved for future editions */
} ASN1_Tag_Universal;


/* Type for ASN.1 octet.  */
typedef unsigned char ASN1_Octet;

/* Type for ASN.1 length value.  */
typedef unsigned long ASN1_Length;

/* Type for a complete ASN.1 TLV unit.  */
typedef struct {
	ASN1_Tag_Universal tag_number;
	ASN1_Length length;
	const ASN1_Octet *contents;
} ASN1_TLV;


/* I/O buffer.  Use this type as if it were opaque.  */
typedef struct {
	size_t size;			/* allocated size */
	ASN1_Octet *buffer;		/* allocated buffer */

	ASN1_Length length;		/* used length */
	ASN1_Length cursor;		/* read cursor */
} ASN1_Buffer;

const ASN1_Octet *asn1_get_buffer_contents(const ASN1_Buffer *);
ASN1_Length asn1_get_buffer_length(const ASN1_Buffer *);

int asn1_write_octet(ASN1_Buffer *, ASN1_Octet);
int asn1_write_octets(ASN1_Buffer *, ASN1_Length, const ASN1_Octet *);
ASN1_Octet *asn1_write_reserve(ASN1_Buffer *, ASN1_Length);


/* Error codes.  */
typedef enum {
	ASN1_ERROR_NONE,		/* No error */
	ASN1_ERROR_OOB,			/* Out of buffer space */
	ASN1_ERROR_OOO,			/* Out of octets */
	ASN1_ERROR_BAD,			/* Invalid input */
	ASN1_ERROR_IMPL			/* Implementation restriction */
} ASN1_Error_Code;

/* Error state.  */
typedef struct {
	ASN1_Error_Code error_code;	/* error code */
	const char *error_string;	/* descriptive string; read-only */
} ASN1_Error_State;

void asn1_error_clear(ASN1_Error_State *);

/* ASN.1 encoder state.  */
typedef struct {
	ASN1_Error_State error_state;

	ASN1_Buffer buffer;

	ASN1_TLV current_field;
} ASN1_Encoder_State;

void asn1_init_encoder(ASN1_Encoder_State *);
void asn1_free_encoder(ASN1_Encoder_State *);

int asn1_encode_tlv(ASN1_Encoder_State *);

/* ASN.1 decoder state.  */
typedef struct {
	ASN1_Error_State error_state;

	ASN1_Buffer buffer;
	ASN1_Length min_octets;

	ASN1_TLV current_field;

	enum {
		ASN1_DECODE_TAG,
		ASN1_DECODE_LENGTH,
		ASN1_DECODE_CONTENTS
	} parser_state;
} ASN1_Decoder_State;

void asn1_init_decoder(ASN1_Decoder_State *);
void asn1_free_decoder(ASN1_Decoder_State *);

int asn1_decode_tlv(ASN1_Decoder_State *);

#endif /* !BTECH_ASN1DER_H */
