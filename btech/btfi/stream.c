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

	FI_Length w_cursor;		/* write cursor */
	FI_Length r_cursor;		/* read cursor */

	FI_ErrorInfo error_info;	/* error information */

	FI_Length needed_length;	/* needed octets */
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

/* Clear stream buffer.  */
void
fi_clear_stream(FI_OctetStream *stream)
{
	stream->w_cursor = 0;
	stream->r_cursor = 0;

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
 * Returns the number of unread octets currently in the stream.
 */
FI_Length
fi_get_stream_length(const FI_OctetStream *stream)
{
	return stream->w_cursor - stream->r_cursor;
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
 * Returns the number of octets that can be written without needing to expand
 * the buffer.
 */
FI_Length
fi_get_stream_free_length(const FI_OctetStream *stream)
{
	return stream->size / sizeof(FI_Octet) - stream->w_cursor;
}

/*
 * Advances the stream write cursor.  Trying to advance past the end of the
 * stream will silently clamp to the end of the buffer.
 *
 * This operation is guaranteed not to invalidate write cursors obtained since
 * the previous call to fi_advance_stream_write_cursor().
 */
void
fi_advance_stream_write_cursor(FI_OctetStream *stream, FI_Length length)
{
	// Correct code shouldn't rely on clamping behavior.
	// TODO: We should probably enforce not advancing farther than the last
	// write window, either.
	assert(fi_get_stream_free_length(stream) >= length);

	if (fi_get_stream_free_length(stream) <= length) {
		stream->w_cursor = stream->size;
	} else {
		stream->w_cursor += length;
	}
}

/*
 * Advances the stream read cursor without returning the contents.  Trying to
 * advance past the end of the stream will silently clamp to the end of the
 * buffer.
 *
 * This operation is guaranteed not to invalidate read cursors obtained since
 * the previous call to fi_advance_stream_read_cursor().
 */
void
fi_advance_stream_read_cursor(FI_OctetStream *stream, FI_Length length)
{
	// Correct code shouldn't rely on clamping behavior.
	assert(fi_get_stream_length(stream) >= length);

	shrink_buffer(stream);

	if (fi_get_stream_length(stream) <= length) {
		/* Empty buffer.  As an optimization, reset the cursor, too.  */
		stream->w_cursor = 0;
		stream->r_cursor = 0;
	} else {
		stream->r_cursor += length;
	}
}

/*
 * Get a read-only window on the stream buffer.  If the requested window would
 * require more octets than currently available, returns a truncated window and
 * sets the needed length (returned by fi_get_stream_needed_length()) to the
 * number of octets required to complete the request.
 *
 * To obtain the size of a truncated window, use fi_get_stream_length().
 *
 * This operation may invalidate buffer pointers.
 *
 * Return NULL on errors.
 */
const FI_Octet *
fi_get_stream_read_window(FI_OctetStream *stream, FI_Length length)
{
	const FI_Length avail_len = fi_get_stream_length(stream);

	if (avail_len < length) {
		stream->needed_length = length - avail_len;

		FI_SET_ERROR(stream->error_info, FI_ERROR_EOS);
	} else {
		stream->needed_length = 0;
	}

	return &stream->buffer[stream->r_cursor];
}

/*
 * Get a writable window on the stream buffer.  The caller may use this pointer
 * to directly write the requested number of octets into the buffer.
 *
 * This operation may invalidate buffer pointers.
 *
 * Return NULL on errors.
 */
FI_Octet *
fi_get_stream_write_window(FI_OctetStream *stream, FI_Length length)
{
	if (!grow_buffer(stream, length)) {
		FI_SET_ERROR(stream->error_info, FI_ERROR_OOM);
		return NULL;
	}

	return &stream->buffer[stream->w_cursor];
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
	new_length = stream->w_cursor + length;

	if (new_length < stream->w_cursor) {
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

	if (stream->r_cursor <= (stream->size >> 1)) {
		/* Less than 50% buffer utilization.  */
		return;
	}

	/* We can use memcpy(), rather than memmove(), because we're copying
	 * from second half to first half, so no overlap is possible.  */
	tail_length = fi_get_stream_length(stream);

	memcpy(stream->buffer, stream->buffer + stream->r_cursor,
	       tail_length * sizeof(FI_Octet));

	stream->w_cursor = tail_length;
	stream->r_cursor = 0;
}
