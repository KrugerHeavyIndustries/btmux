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

#include "fiptypes.h"
#include "errors.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define FI_LENGTH_MAX ((size_t)-1)

typedef size_t FI_Length; /* FI_LENGTH_MAX must be <= SIZE_MAX, for sanity */

/* Octet stream.  */
typedef struct FI_tag_OctetStream FI_OctetStream;

FI_OctetStream *fi_create_stream(size_t initial_size);
void fi_destroy_stream(FI_OctetStream *stream);

void fi_clear_stream(FI_OctetStream *stream);

/* Error handling.  */
const FI_ErrorInfo *fi_get_stream_error(const FI_OctetStream *stream);
void fi_clear_stream_error(FI_OctetStream *stream);

/* Octet-oriented stream operations.  */
FI_Length fi_get_stream_length(const FI_OctetStream *stream);
FI_Length fi_get_stream_needed_length(const FI_OctetStream *stream);
FI_Length fi_get_stream_free_length(const FI_OctetStream *stream);

void fi_advance_stream_write_cursor(FI_OctetStream *stream, FI_Length length);
void fi_advance_stream_read_cursor(FI_OctetStream *stream, FI_Length length);

const FI_Octet *fi_get_stream_read_window(FI_OctetStream *stream,
                                          FI_Length length);
FI_Octet *fi_get_stream_write_window(FI_OctetStream *stream,
                                     FI_Length length);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !BTECH_FI_STREAM_H */
