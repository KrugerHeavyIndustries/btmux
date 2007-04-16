/*
 * ASN.1 DER encoder/decoder, sufficient for encoding BTech databases as Fast
 * Infoset documents.  See ITU X.690 for the full specification of ASN.1 DER.
 */

#include "autoconf.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <assert.h>

#include "asn1der.h"


#define SET_ERROR(state, code) \
	do { \
		(state)->error_state.error_code = (code); \
		(state)->error_state.error_string = error_strings[(code)]; \
	} while (0)

static void asn1_init_buffer(ASN1_Buffer *);
static void asn1_free_buffer(ASN1_Buffer *);
static void asn1_clear_buffer(ASN1_Buffer *);
static void asn1_trim_buffer(ASN1_Buffer *);

static const char *const error_strings[] = {
	"No error",
	"Out of buffer space",
	"Out of octets",
	"Invalid input",
	"Implementation restriction"
};

static const ASN1_Length ASN1_MAX_LENGTH = -1;
static ASN1_Length ASN1_MAX_LENGTH_OCTETS;

/*
 * Initializes global runtime constants.
 */
static void
asn1_init(void)
{
	static int asn1_was_initialized = 0;

	ASN1_Length ii, tmp_length;

	if (asn1_was_initialized) {
		return;
	} else {
		asn1_was_initialized = 1;
	}

	/* Compute constants related to maximum length.  */
	tmp_length = 0x80;

	for (ii = 0; tmp_length; ii++) {
		tmp_length <<= 8;
	}

	ASN1_MAX_LENGTH_OCTETS = ii;

	assert(ASN1_MAX_LENGTH_OCTETS <= 0x7F); /* exceeding would be silly */
}

/*
 * Reset an ASN1_Error_State object.
 */
void
asn1_error_clear(ASN1_Error_State *error_state)
{
	error_state->error_code = ASN1_ERROR_NONE;
	error_state->error_string = NULL;
}


/*
 * Encoder routines.
 */

static int asn1_write_tag(ASN1_Encoder_State *);
static int asn1_write_length(ASN1_Encoder_State *);

void
asn1_init_encoder(ASN1_Encoder_State *state)
{
	asn1_init();

	asn1_error_clear(&state->error_state);
	asn1_init_buffer(&state->buffer);
}

void
asn1_free_encoder(ASN1_Encoder_State *state)
{
	asn1_free_buffer(&state->buffer);
}

int
asn1_encode_tlv(ASN1_Encoder_State *state)
{
	asn1_clear_buffer(&state->buffer);

	if (!asn1_write_tag(state)) {
		return 0;
	}

	if (!asn1_write_length(state)) {
		return 0;
	}

	/* Writing the contents is the responsibility of the caller.  */
	return 1;
}


/*
 * Decoder routines.
 */

static int asn1_read_tag(ASN1_Decoder_State *);
static int asn1_read_length(ASN1_Decoder_State *);
static int asn1_read_contents(ASN1_Decoder_State *);

void
asn1_init_decoder(ASN1_Decoder_State *state)
{
	asn1_init();

	asn1_error_clear(&state->error_state);
	asn1_init_buffer(&state->buffer);

	state->parser_state = ASN1_DECODE_TAG;
}

void
asn1_free_decoder(ASN1_Decoder_State *state)
{
	asn1_free_buffer(&state->buffer);
}

int
asn1_decode_tlv(ASN1_Decoder_State *state)
{
	/* Parse value.  */
	switch (state->parser_state) {
	case ASN1_DECODE_TAG:
		if (!asn1_read_tag(state)) {
			return 0;
		}

		state->parser_state = ASN1_DECODE_LENGTH;
		/* FALLTHROUGH */
	case ASN1_DECODE_LENGTH:
		if (!asn1_read_length(state)) {
			return 0;
		}

		state->parser_state = ASN1_DECODE_CONTENTS;
		/* FALLTHROUGH */
	case ASN1_DECODE_CONTENTS:
		if (!asn1_read_contents(state)) {
			return 0;
		}

		state->parser_state = ASN1_DECODE_TAG;
		break;

	default:
		/* XXX: Invalid state.  */
		assert(0);
		return 0;
	}

	/* Present value.  */
	asn1_trim_buffer(&state->buffer);

	state->current_field.contents = asn1_get_buffer_contents(&state->buffer);
	state->buffer.cursor += state->current_field.length;
	return 1;
}


/*
 * Encoder/decoder subroutines.
 */

#define RESET_OCTET(buf) do { (buf).cursor = 0; } while (0)
#define PREV_OCTET(buf) ((buf).cursor--)
#define NEXT_OCTET(buf) ((buf).cursor++)

#define HAS_OCTET(buf) ((buf).cursor < (buf).length)
#define PEEK_OCTET(buf) ((buf).buffer[(buf).cursor])
#define GET_OCTET(buf) ((buf).buffer[(buf).cursor++])

/*
 * Subroutines for working with ASN.1 tags, as defined in X.690 section 8.1.2.
 *
 * Tags in ASN.1 are a combination of tag class and class-specific number.  For
 * efficiency and ease of implementation, we only support single octet tags in
 * the UNIVERSAL class.
 */

#define GOOD_TAG(tag) ((tag) > 0 && (tag) <= 30)

static int
asn1_write_tag(ASN1_Encoder_State *state)
{
	const ASN1_Tag_Universal tag_number = state->current_field.tag_number;

	if (!GOOD_TAG(tag_number)) {
		/* Tag number out of range.  */
		SET_ERROR(state, ASN1_ERROR_IMPL);
		return 0;
	}

	/* Bits 8-7 are 0 because tag class is universal. (8.1.2.4.1a) */
	/* Bit 6 is 0 because encoding (in DER) is primitive. (8.1.2.5) */
	/* Bits 5-1 are just the tag number. (8.1.2.4.1c) */
	if (!asn1_write_octet(&state->buffer, (ASN1_Octet)tag_number)) {
		SET_ERROR(state, ASN1_ERROR_OOB);
		return 0;
	}

	return 1;
}

static int
asn1_read_tag(ASN1_Decoder_State *state)
{
	ASN1_Tag_Universal tag_number;

	/* Get tag octet.  */
	if (!HAS_OCTET(state->buffer)) {
		SET_ERROR(state, ASN1_ERROR_OOO);
		state->min_octets = 1;
		return 0;
	}

	tag_number = (ASN1_Tag_Universal)GET_OCTET(state->buffer);

	/* Parse tag octet.  */
	if (!GOOD_TAG(tag_number)) {
		/* Tag number out of range.  */
		SET_ERROR(state, ASN1_ERROR_IMPL);
		return 0;
	}

	state->current_field.tag_number = tag_number;
	return 1;
}

/*
 * Subroutines for working with lengths, as defined in X.690 section 8.1.3.
 *
 * We're using DER, so we use the definite form, defined in 8.1.3.3, and always
 * encode in the smallest possible number of octets.  This means using the
 * short form when possible, and the shortest possible long form when
 * necessary.
 *
 * We define ASN1_Length as an unsigned long, so as an implementation
 * restriction, we don't support lengths greater than ULONG_MAX.
 */

static int
asn1_write_length(ASN1_Encoder_State *state)
{
	const ASN1_Length length = state->current_field.length;

	if (length > ASN1_MAX_LENGTH) {
		/* Exceeds maximum supported length.  */
		SET_ERROR(state, ASN1_ERROR_IMPL);
		return 0;
	} else if (length > 0x7F) {
		/* Use long form (8.1.3.5).  */
		ASN1_Length ii;

		for (ii = 1; ii < ASN1_MAX_LENGTH_OCTETS; ii++) {
			if (!(length >> (8 * ii))) {
				/* Remaining octets are 0.  */
				break;
			}
		}

		if (!asn1_write_octet(&state->buffer, 0x80 | (ASN1_Octet)ii)) {
			SET_ERROR(state, ASN1_ERROR_OOB);
			return 0;
		}

		ii = 8 * ii;

		do {
			ASN1_Octet tmp_octet;

			ii -= 8;

			tmp_octet = (length >> ii) & 0xFF;

			if (!asn1_write_octet(&state->buffer,
			                      (ASN1_Octet)tmp_octet)) {
				SET_ERROR(state, ASN1_ERROR_OOB);
				return 0;
			}
		} while (ii > 0);
	} else {
		/* Use short form (8.1.3.4).  */
		if (!asn1_write_octet(&state->buffer, (ASN1_Octet)length)) {
			SET_ERROR(state, ASN1_ERROR_OOB);
			return 0;
		}
	}

	return 1;
}

static int
asn1_read_length(ASN1_Decoder_State *state)
{
	ASN1_Length length, left_octets;

	ASN1_Octet tmp_octet;

	/* Determine length form.  */
	if (!HAS_OCTET(state->buffer)) {
		SET_ERROR(state, ASN1_ERROR_OOO);
		state->min_octets = 1;
		return 0;
	}

	tmp_octet = GET_OCTET(state->buffer);

	if (tmp_octet & 0x80) {
		/* Use the long form.  */
		if (tmp_octet == 0xFF) {
			/* 0xFF is reserved (8.1.3.5c).  */
			SET_ERROR(state, ASN1_ERROR_BAD);
			return 0;
		}

		tmp_octet &= 0x7F;

		left_octets = asn1_get_buffer_length(&state->buffer);

		if (left_octets < tmp_octet) {
			/* Not enough octets to parse.  */
			SET_ERROR(state, ASN1_ERROR_OOO);
			state->min_octets = tmp_octet - left_octets;

			PREV_OCTET(state->buffer);
			return 0;
		} else if (tmp_octet > ASN1_MAX_LENGTH_OCTETS) {
			/* Exceeds maximum supported length.  */
			SET_ERROR(state, ASN1_ERROR_IMPL);
			return 0;
		}

		/* Concatenate length octets.  */
		length = 0;

		for (left_octets = tmp_octet; left_octets > 0; left_octets--) {
			length = (length << 8) | GET_OCTET(state->buffer);
		}
	} else {
		/* Use the short form.  */
		length = tmp_octet;
	}

	state->current_field.length = length;
	return 1;
}

/*
 * Subroutines for working with contents, as defined in X.690 section 8.1.4.
 *
 * Nothing to do here except ensure we have the entire contents (when reading).
 * Higher-level interpretation of contents is left to another module.
 */

static int
asn1_read_contents(ASN1_Decoder_State *state)
{
	const ASN1_Length wanted_octets = state->current_field.length;
	const ASN1_Length left_octets = asn1_get_buffer_length(&state->buffer);

	if (wanted_octets > left_octets) {
		SET_ERROR(state, ASN1_ERROR_OOO);
		state->min_octets = wanted_octets - left_octets;
		return 0;
	}

	return 1;
}


/*
 * ASN1_Buffer subroutines.
 */

/*
 * Initialize an ASN1_Buffer object.
 */
static void
asn1_init_buffer(ASN1_Buffer *buffer)
{
	buffer->size = 0;
	buffer->buffer = NULL;

	asn1_clear_buffer(buffer);
}

/*
 * Free memory allocated for an ASN1_Buffer.
 */
static void
asn1_free_buffer(ASN1_Buffer *buffer)
{
	if (buffer->buffer) {
		free(buffer->buffer);

		asn1_init_buffer(buffer);
	}
}

/*
 * Expand an ASN1_Buffer to accommodation the additional requested length.
 */
static int
asn1_grow_buffer(ASN1_Buffer *buffer, ASN1_Length length)
{
	ASN1_Octet *new_buffer;

	ASN1_Length new_length;
	size_t new_size, tmp_new_size;

	/* Some sanity checks.  */
	new_length = buffer->length + length;

	if (new_length < buffer->length) {
		/* Overflow.  */
		return 0;
	}

	new_size = new_length * sizeof(ASN1_Octet);

	if (new_size <= buffer->size) {
		return 1;
	}

	/* Always allocate in powers of 2.  */
	for (tmp_new_size = 1; new_size > tmp_new_size; tmp_new_size <<= 1) {
		if ((tmp_new_size << 1) <= tmp_new_size) {
			/* Overflow.  */
			return 0;
		}
	}

	new_size = tmp_new_size;

	/* Allocate buffer.  */
	new_buffer = (ASN1_Octet *)realloc(buffer->buffer, new_size);
	if (!new_buffer) {
		return 0;
	}

	buffer->size = new_size;
	buffer->buffer = new_buffer;
	return 1;
}

/*
 * Clear contents of the buffer.
 */
static void
asn1_clear_buffer(ASN1_Buffer *buffer)
{
	buffer->length = 0;
	buffer->cursor = 0;
}

/*
 * Periodically reduce the length of the buffer, by trimming content preceding
 * the cursor.  Content will only be trimmed if it would save at least 50% of
 * the current buffer size.
 */
static void
asn1_trim_buffer(ASN1_Buffer *buffer)
{
	ASN1_Length tail_length;

	if (buffer->cursor <= (buffer->size >> 1)) {
		/* Less than 50% buffer utilization.  */
		return;
	}

	/* memcpy() is safe because we're copying from second half to first
	 * half, so no overlap is possible.  */
	tail_length = buffer->length - buffer->cursor;

	memcpy(buffer->buffer, buffer->buffer + buffer->cursor,
	       tail_length * sizeof(ASN1_Octet));

	buffer->length = tail_length;
	buffer->cursor = 0;
}

const ASN1_Octet *
asn1_get_buffer_contents(const ASN1_Buffer *buffer)
{
	return buffer->buffer + buffer->cursor;
}

ASN1_Length
asn1_get_buffer_length(const ASN1_Buffer *buffer)
{
	return buffer->length - buffer->cursor;
}

/*
 * Write a single octet to an ASN1_Buffer.
 */
int
asn1_write_octet(ASN1_Buffer *buffer, ASN1_Octet octet)
{
	if (!asn1_grow_buffer(buffer, 1)) {
		return 0;
	}

	buffer->buffer[buffer->length++] = octet;
	return 1;
}

/*
 * Write multiple octets to an ASN1_Buffer.
 */
int
asn1_write_octets(ASN1_Buffer *buffer,
                  ASN1_Length length, const ASN1_Octet *contents)
{
	if (!asn1_grow_buffer(buffer, length)) {
		return 0;
	}

	memcpy(buffer->buffer + buffer->length, contents,
	       length * sizeof(ASN1_Octet));
	buffer->length += length;
	return 1;
}

/*
 * Return a pointer to reserved buffer space.  The caller can use this pointer
 * to directly write the specified amount of data into the buffer.  Any other
 * calls may invalidate the pointer.
 *
 * Returns NULL on errors.
 */
ASN1_Octet *
asn1_write_reserve(ASN1_Buffer *buffer, ASN1_Length length)
{
	ASN1_Octet *write_ptr;

	if (!asn1_grow_buffer(buffer, length)) {
		return NULL;
	}

	write_ptr = buffer->buffer + buffer->length;
	buffer->length += length;
	return write_ptr;
}
