/*
 * Module for working with octet streams.
 */

#include "autoconf.h"

#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include <assert.h>

#include "stream.h"


/*
 * Common definitions.
 */

struct FI_tag_OctetStream {
	size_t size;			/* allocated size */
	FI_Octet *buffer;		/* allocated buffer */

	FI_Length length;		/* occupied length */
	FI_Length cursor;		/* read cursor */

	FI_ErrorInfo error_info;	/* error information */

	FI_Octet bits;			/* partial octet bits */
	int num_bits;			/* number of partial octet bits */

	FI_Length needed_length;	/* needed octets */

	void *app_data_ptr;		/* application-specific data */
}; /* FI_OctetStream */

static int grow_buffer(FI_OctetStream *, FI_Length);
static void shrink_buffer(FI_OctetStream *);


/*
 * FI_OctetStream construction/destruction.
 */

/* initial_size is only a suggestion, and could be unsatisfied.  */
FI_OctetStream *
fi_create_stream(size_t initial_size)
{
	FI_OctetStream *new_stream;

	new_stream = (FI_OctetStream *)malloc(sizeof(FI_OctetStream));
	if (!new_stream) {
		return NULL;
	}

	new_stream->size = 0;
	new_stream->buffer = NULL;

	fi_clear_stream(new_stream);

	FI_CLEAR_ERROR(new_stream->error_info);

	new_stream->app_data_ptr = NULL;

	/* XXX: initial_size is only a suggestion, so we can't fail.  */
	grow_buffer(new_stream, initial_size);

	return new_stream;
}

void
fi_destroy_stream(FI_OctetStream *stream)
{
	if (stream->buffer) {
		free(stream->buffer);
	}

	free(stream);
}

/* Get/set application data pointer.  */
void *
fi_get_stream_data(const FI_OctetStream *stream)
{
	return stream->app_data_ptr;
}

void
fi_set_stream_data(FI_OctetStream *stream, void *app_data_ptr)
{
	stream->app_data_ptr = app_data_ptr;
}

/* Clear stream buffer.  */
void
fi_clear_stream(FI_OctetStream *stream)
{
	stream->length = 0;
	stream->cursor = 0;

	stream->bits = 0;
	stream->num_bits = 0;

	stream->needed_length = 0;
}


/*
 * Error handling.
 */

const FI_ErrorInfo *
fi_get_stream_error(const FI_OctetStream *stream)
{
	return &stream->error_info;
}

void
fi_clear_stream_error(FI_OctetStream *stream)
{
	FI_CLEAR_ERROR(stream->error_info);
}


/*
 * Octet-oriented stream operations.
 */

/*
 * Returns the number of octets that can be written without needing to expand
 * the buffer.
 */
FI_Length
fi_get_stream_free_length(const FI_OctetStream *stream)
{
	return stream->size / sizeof(FI_Octet) - stream->length;
}

/*
 * Returns the number of excess octets needed by the last read operation, in
 * the event of failure due to exhausting the buffer.  Resets to 0 after any
 * successful read operation, or a fi_clear_stream().
 *
 * This is useful to allow a stream to be filled until the next read is likely
 * to succeed.  Unlike with real I/O, our in-memory streams should make heavy
 * use of minimally-sized reads, to avoid having a buffer writer try to read
 * more data than is strictly required.
 */
FI_Length
fi_get_stream_needed_length(const FI_OctetStream *stream)
{
	return stream->needed_length;
}

/*
 * Reduce the occupied length of the stream, which is useful for partial
 * writes.  Reducing by more than the number of available octets will silently
 * clamp to the number of available octets.
 *
 * This operation may invalidate buffer pointers.
 */
void
fi_reduce_stream_length(FI_OctetStream *stream, FI_Length length)
{
	const FI_Length occupied_length = stream->length - stream->cursor;

	if (occupied_length <= length) {
		/* Empty buffer.  As an optimization, reset the cursor, too.  */
		stream->cursor = 0;
		stream->length = 0;
	} else {
		stream->length = occupied_length - length;
	}
}

/*
 * Behaves like fi_try_read_stream(), except always reads all available octets.
 */
FI_Length
fi_read_stream(FI_OctetStream *stream, const FI_Octet **buffer_ptr)
{
	const FI_Length length = stream->length - stream->cursor;

	if (buffer_ptr) {
		*buffer_ptr = stream->buffer + stream->cursor;
	}

	shrink_buffer(stream);

	stream->cursor += length;
	stream->needed_length = 0;
	return length;
}

/*
 * Attempt to read a specific number of octets from the stream.  If successful,
 * returns length, and advances the stream cursor.  If unsuccessful, returns
 * the actual number of read octets, and does NOT advance the stream cursor.
 * In both cases, buffer_ptr (if non-NULL) will be set to point to the read
 * octets.
 *
 * Any existing buffer pointers may be invalidated.
 */
FI_Length
fi_try_read_stream(FI_OctetStream *stream, const FI_Octet **buffer_ptr,
                   FI_Length length)
{
	FI_Length remaining_length;

	if (buffer_ptr) {
		*buffer_ptr = stream->buffer + stream->cursor;
	}

	remaining_length = stream->length - stream->cursor;

	if (remaining_length < length) {
		FI_SET_ERROR(stream->error_info, FI_ERROR_EOS);
		stream->needed_length = length - remaining_length;
		return remaining_length;
	}

	shrink_buffer(stream);

	stream->cursor += length;
	stream->needed_length = 0;
	return length;
}

/*
 * Get a writable pointer into the main buffer for a specific number of octets.
 * The caller may use this pointer to directly write the requested number of
 * octets into the buffer.
 *
 * Any other octet stream calls may invalidate the returned pointer.
 *
 * Returns NULL on errors.
 */
FI_Octet *
fi_get_stream_write_buffer(FI_OctetStream *stream, FI_Length length)
{
	FI_Octet *write_ptr;

	if (!grow_buffer(stream, length)) {
		FI_SET_ERROR(stream->error_info, FI_ERROR_OOM);
		return NULL;
	}

	write_ptr = stream->buffer + stream->length;
	stream->length += length;
	return write_ptr;
}


/*
 * Bit-oriented stream operations.
 */

/* Returns the number of bits (ranging from 0 to 7) in the bit accumulator.  */
int
fi_get_stream_num_bits(const FI_OctetStream *stream)
{
	return stream->num_bits;
}

/*
 * Clear the bit accumulator, writing its value out to the stream.  Any missing
 * bits will be padded out with 0's.
 */
int
fi_flush_stream_bits(FI_OctetStream *stream)
{
	FI_Octet *write_ptr;

	/* Write octet.  */
	write_ptr = fi_get_stream_write_buffer(stream, 1);
	if (!write_ptr) {
		/* XXX: Error set by fi_get_stream_write_buffer.  */
		return 0;
	}

	write_ptr[0] = stream->bits; /* bits should already be padded */

	stream->bits = 0;
	stream->num_bits = 0;
	return 1;
}

/*
 * Writes some number of bits (up to 8) to the stream.  These bits will be
 * combined with any previously written bits to form a complete octet, which
 * will then be written out to the stream as a unit.  Any remaining bits will
 * accumulate until the next fi_write_stream_bits() operation.
 *
 * Octet-oriented stream operations will ignore any accumulated, uncommitted
 * bits.  It is the responsibility of the calling application to check the
 * value of fi_get_stream_num_bits(), and pad out to a full octet. (The
 * convenience function fi_flush_stream_bits() will pad out with 0's.)
 *
 * Bits are numbered from 1 (MSB) to 8 (LSB).  The FI_BIT_# macros have been
 * provided to make creating bit strings easier.
 *
 * This function should not be used for writing long sequences of bits, but
 * instead is intended for writing a final, partial octet at the end of a
 * longer, multi-octet write operation, as a prelude to further processing.
 *
 * Returns 0 on errors.
 */
int
fi_write_stream_bits(FI_OctetStream *stream, int num_bits, FI_Octet bits)
{
	FI_Octet *write_ptr;

	FI_Octet next_octet;
	int total_bits;

	if (num_bits < 1 || num_bits > 8) {
		FI_SET_ERROR(stream->error_info, FI_ERROR_INVAL);
		return 0;
	}

	/* Try to fill an octet.  */
	next_octet = stream->bits | (bits >> stream->num_bits);
	total_bits = stream->num_bits + num_bits;

	if (total_bits < 8) {
		stream->bits = next_octet;
		stream->num_bits = total_bits;
		return 1;
	}

	/* Write octet.  */
	write_ptr = fi_get_stream_write_buffer(stream, 1);
	if (!write_ptr) {
		/* XXX: Error set by fi_get_stream_write_buffer.  */
		return 0;
	}

	write_ptr[0] = next_octet;

	stream->bits = bits << (8 - stream->num_bits);
	stream->num_bits = total_bits - 8;
	return 1;
}

/*
 * Reads some number of bits (up to 8) from the stream, possibly including bits
 * leftover in the accumulator from previous fi_read_stream_bits() operations.
 *
 * Octet-oriented stream operations will ignore any accumulated bits.  It is
 * the responsibility of the calling application to check the value of
 * fi_get_stream_num_bits(), and read out to a full octet, before conducting
 * any octet-oriented stream operations.
 *
 * Bits are numbered from 1 (MSB) to 8 (LSB).  The FI_BIT_# macros have been
 * provided to make creating bit strings easier.
 *
 * This function should not be used for reading long sequences of bits, but
 * instead is intended for reading a partial octet, to determine further
 * processing.
 *
 * The read bits are returned in the FI_Octet addressed by "bits".  Only the
 * first num_bits are meaningful; the remainder may contain arbitrary data.
 * The caller is expected to mask off the appropriate bits.
 *
 * Returns 0 on errors.
 */
int
fi_read_stream_bits(FI_OctetStream *stream, int num_bits, FI_Octet *bits)
{
	const FI_Octet *read_ptr;

	FI_Octet tmp_bits, next_octet;
	int need_bits;

	if (num_bits < 1 || num_bits > 8) {
		FI_SET_ERROR(stream->error_info, FI_ERROR_INVAL);
		return 0;
	}

	/* Try to fill request from the accumulator.  */
	tmp_bits = stream->bits;
	need_bits = num_bits - stream->num_bits;

	if (need_bits <= 0) {
		stream->bits = tmp_bits << num_bits;
		stream->num_bits = -need_bits;

		*bits = tmp_bits;
		stream->needed_length = 0;
		return 1;
	}

	/* Read octet.  */
	if (fi_try_read_stream(stream, &read_ptr, 1) != 1) {
		/* XXX: Error set by fi_try_read_stream.  */
		return 0;
	}

	next_octet = read_ptr[0];

	stream->bits = next_octet << need_bits;
	stream->num_bits = 8 - need_bits;

	*bits = tmp_bits | (next_octet >> (num_bits - need_bits));
	stream->needed_length = 0;
	return 1;
}


/*
 * Buffer handling.
 */

/* Expand buffer to accommodate requested number of additional octets.  */
static int
grow_buffer(FI_OctetStream *stream, FI_Length length)
{
	FI_Octet *new_buffer;

	FI_Length new_length;
	size_t new_size, tmp_new_size;

	/* Some sanity checks.  */
	new_length = stream->length + length;

	if (new_length < stream->length) {
		/* Overflow.  */
		return 0;
	}

	new_size = new_length * sizeof(FI_Octet);

	if (new_size <= stream->size) {
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
	new_buffer = (FI_Octet *)realloc(stream->buffer, new_size);
	if (!new_buffer) {
		return 0;
	}

	stream->size = new_size;
	stream->buffer = new_buffer;
	return 1;
}

/* Reduce buffer utilization by removing already processed octets (those
 * preceding the read cursor).  Since this involves a copy operation, it will
 * only be performed if it would reduce utilization to under 50%.  */
static void
shrink_buffer(FI_OctetStream *stream)
{
	FI_Length tail_length;

	if (stream->cursor <= (stream->size >> 1)) {
		/* Less than 50% buffer utilization.  */
		return;
	}

	/* We can use memcpy(), rather than memmove(), because we're copying
	 * from second half to first half, so no overlap is possible.  */
	tail_length = stream->length - stream->cursor;

	memcpy(stream->buffer, stream->buffer + stream->cursor,
	       tail_length * sizeof(FI_Octet));

	stream->length = tail_length;
	stream->cursor = 0;
}
