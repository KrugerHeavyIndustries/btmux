/*
 * Module for working with octet streams.  Provides an interface suitable for
 * iteratively reading from or writing to arbitrary kinds of octet streams.
 *
 * FIXME: We're re-inventing the wheel, but hopefully we can converge on an
 * interface like this one, or at least with the capabilities of this one.
 */

#ifndef BTECH_FI_STREAM_H
#define BTECH_FI_STREAM_H

#include <stddef.h>

/* Octet type.  Must contain at least 8 bits.  */
typedef unsigned char FI_Octet;

/* Length type.  Must have a range of at least 0 to 2^32 - 1.  */
typedef unsigned long FI_Length;

/* Octet stream.  */
typedef struct FI_tag_OctetStream FI_OctetStream;

FI_OctetStream *fi_create_stream(size_t initial_size);
void fi_destroy_stream(FI_OctetStream *stream);

/* Stream operations.  */
FI_Length fi_try_read_stream(FI_OctetStream *stream, FI_Length length,
                             const FI_Octet **buffer_ptr);
FI_Octet *fi_get_stream_write_buffer(FI_OctetStream *stream, FI_Length length);

/* Error handling.  */

/* TODO: Move this to a common module.  */
typedef enum {
	FI_ERROR_NONE,			/* No error */
	FI_ERROR_OOM,			/* Out of memory */
	FI_ERROR_EOS			/* End of stream */
} FI_ErrorCode;

typedef struct {
	FI_ErrorCode error_code;	/* error code */
	const char *error_string;	/* descriptive string; read-only */
} FI_ErrorInfo;

const FI_ErrorInfo *fi_get_stream_error(const FI_OctetStream *stream);
void fi_clear_stream_error(FI_OctetStream *stream);

#endif /* !BTECH_FI_STREAM_H */
