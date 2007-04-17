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

#include "common.h"

/* Octet stream.  */
typedef struct FI_tag_OctetStream FI_OctetStream;

FI_OctetStream *fi_create_stream(size_t initial_size);
void fi_destroy_stream(FI_OctetStream *stream);

/* Stream operations.  */
FI_Length fi_try_read_stream(FI_OctetStream *stream, FI_Length length,
                             const FI_Octet **buffer_ptr);
FI_Octet *fi_get_stream_write_buffer(FI_OctetStream *stream, FI_Length length);

/* Error handling.  */
const FI_ErrorInfo *fi_get_stream_error(const FI_OctetStream *stream);
void fi_clear_stream_error(FI_OctetStream *stream);

#endif /* !BTECH_FI_STREAM_H */
