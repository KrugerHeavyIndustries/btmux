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
 * Set the needed length directly.
 */
void
fi_set_stream_needed_length(FI_OctetStream *stream, FI_Length length)
{
	stream->needed_length = length;
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
	// Correct code shouldn't rely on clamping behavior.
	assert(length <= (stream->length - stream->cursor));

	if ((stream->length - stream->cursor) <= length) {
		/* Empty buffer.  As an optimization, reset the cursor, too.  */
		stream->cursor = 0;
		stream->length = 0;
	} else {
		stream->length -= length;
	}
}

/*
 * Advances the stream read cursor without returning the contents.  Trying to
 * advance past the end of the stream will silently clamp to the end of the
 * buffer.
 *
 * This operation may invalidate buffer pointers.
 */
void
fi_advance_stream_cursor(FI_OctetStream *stream, FI_Length length)
{
	// Correct code shouldn't rely on clamping behavior.
	assert(length <= (stream->length - stream->cursor));

	if ((stream->length - stream->cursor) <= length) {
		/* Empty buffer.  As an optimization, reset the cursor, too.  */
		stream->cursor = 0;
		stream->length = 0;
	} else {
		stream->cursor += length;
	}

	shrink_buffer(stream);
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
 * Attempt to read between min_len and max_len octets from the stream, where
 * 0 <= min_len <= max_len <= FI_MAX_LENGTH.  Returns the actual number of
 * octets available. (Note that this may be larger than max_len! The caller may
 * use the extra octets to opportunistically look ahead.)
 *
 * If all max_len octets are available, the operation is considered a success.
 * The stream cursor is advanced by min_len, and fi_get_stream_needed_length()
 * will return 0.
 *
 * If at least min_len octets are available, the operation is considered a
 * partial success.  The stream cursor is still advanced by min_len, but
 * fi_get_stream_needed_length() will return the difference between max_len and
 * the number of available octets.
 *
 * If less than min_len octets are available, the operation is considered a
 * failure.  The stream cursor will NOT be advanced, and
 * fi_get_stream_needed_length() will return the difference between max_len and
 * the number of available octets.
 *
 * In all cases, buffer_ptr (if non-NULL) will be set to point to the read
 * octets.
 *
 * Any existing buffer pointers may be invalidated.
 *
 * If min_len == max_len, this is essentially a read operation.
 *
 * If min_len == 0, this is essentially a peek operation.
 *
 * If 0 < min_len < max_len, this is a read operation with some lookahead.
 */
FI_Length
fi_try_read_stream(FI_OctetStream *stream, const FI_Octet **buffer_ptr,
                   FI_Length min_len, FI_Length max_len)
{
	FI_Length avail_len;

	assert(min_len <= max_len);

	if (buffer_ptr) {
		*buffer_ptr = stream->buffer + stream->cursor;
	}

	avail_len = stream->length - stream->cursor;

	if (avail_len < max_len) {
		stream->needed_length = max_len - avail_len;

		if (avail_len < min_len) {
			FI_SET_ERROR(stream->error_info, FI_ERROR_EOS);
			return avail_len;
		}
	} else {
		stream->needed_length = 0;
	}

	shrink_buffer(stream);

	stream->cursor += min_len;
	return avail_len;
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
