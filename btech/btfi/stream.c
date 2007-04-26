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

	void *app_data;			/* application-specific data */
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

	new_stream->app_data = NULL;

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
 * Stream operations.
 */

/* Get/set app_data pointer.  */
void *
fi_get_stream_data(const FI_OctetStream *stream)
{
	return stream->app_data;
}

void
fi_set_stream_data(FI_OctetStream *stream, void *app_data)
{
	stream->app_data = app_data;
}

/* Clear stream buffer.  */
void
fi_clear_stream(FI_OctetStream *stream)
{
	stream->length = 0;
	stream->cursor = 0;
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
		return remaining_length;
	}

	shrink_buffer(stream);

	stream->cursor += length;
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
