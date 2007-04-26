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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Octet stream.  */
typedef struct FI_tag_OctetStream FI_OctetStream;

FI_OctetStream *fi_create_stream(size_t initial_size);
void fi_destroy_stream(FI_OctetStream *stream);

/* Stream operations.  */
void *fi_get_stream_data(const FI_OctetStream *stream);
void fi_set_stream_data(FI_OctetStream *stream, void *app_data);

void fi_clear_stream(FI_OctetStream *stream);

FI_Length fi_read_stream(FI_OctetStream *stream, const FI_Octet **buffer_ptr);

FI_Length fi_try_read_stream(FI_OctetStream *stream,
                             const FI_Octet **buffer_ptr, FI_Length length);
FI_Octet *fi_get_stream_write_buffer(FI_OctetStream *stream, FI_Length length);

/* Error handling.  */
const FI_ErrorInfo *fi_get_stream_error(const FI_OctetStream *stream);
void fi_clear_stream_error(FI_OctetStream *stream);

#ifdef __cplusplus
} // extern "C"

namespace BTech {
namespace FI {

// Pure abstract base class (interface) for objects supporting serialization
// via an FI_OctetStream.
class Serializable {
protected:
	virtual ~Serializable () {}

public:

	virtual void write (FI_OctetStream *stream) throw (Exception) = 0;
	virtual void read (FI_OctetStream *stream) throw (Exception) = 0;
}; // class Serializable

} // namespace FI
} // namespace BTech
#endif /* __cplusplus */

#endif /* !BTECH_FI_STREAM_H */
