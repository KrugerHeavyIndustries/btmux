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

#define FI_LENGTH_MAX ((size_t)-1) /* size_t must be unsigned, of course */

typedef size_t FI_Length;

/* Octet stream.  */
typedef struct FI_tag_OctetStream FI_OctetStream;

FI_OctetStream *fi_create_stream(size_t initial_size);
void fi_destroy_stream(FI_OctetStream *stream);

void *fi_get_stream_data(const FI_OctetStream *stream);
void fi_set_stream_data(FI_OctetStream *stream, void *app_data);

void fi_clear_stream(FI_OctetStream *stream);

/* Error handling.  */
const FI_ErrorInfo *fi_get_stream_error(const FI_OctetStream *stream);
void fi_clear_stream_error(FI_OctetStream *stream);

/* Octet-oriented stream operations.  */
FI_Length fi_read_stream(FI_OctetStream *stream, const FI_Octet **buffer_ptr);

FI_Length fi_try_read_stream(FI_OctetStream *stream,
                             const FI_Octet **buffer_ptr, FI_Length length);
FI_Octet *fi_get_stream_write_buffer(FI_OctetStream *stream, FI_Length length);

/* Bit-oriented stream operations.  */

/* Section 5.5: Fast Infoset numbers bits from 1 (MSB) to 8 (LSB).  */
#define FI_BIT_1 0x80
#define FI_BIT_2 0x40
#define FI_BIT_3 0x20
#define FI_BIT_4 0x10
#define FI_BIT_5 0x08
#define FI_BIT_6 0x04
#define FI_BIT_7 0x02
#define FI_BIT_8 0x01

#define FI_BITS(a,b,c,d,e,f,g,h) \
	(((a+0) << 7) | ((b+0) << 6) | ((c+0) << 5) | ((d+0) << 4) \
	 | ((e+0) << 3) | ((f+0) << 2) | ((g+0) << 1) | (h+0))

int fi_get_stream_num_bits(const FI_OctetStream *stream);
FI_Octet fi_get_stream_bits(const FI_OctetStream *stream);
int fi_set_stream_bits(FI_OctetStream *stream, int num_bits, FI_Octet bits);

int fi_flush_stream_bits(FI_OctetStream *stream);
int fi_write_stream_bits(FI_OctetStream *stream, int num_bits, FI_Octet bits);

#ifdef __cplusplus
} // extern "C"

namespace BTech {
namespace FI {

// Abstract base class for objects supporting serialization via an
// FI_OctetStream.
class Serializable {
protected:
	virtual ~Serializable () {}

public:
	virtual void write (FI_OctetStream *stream) = 0;
	virtual void read (FI_OctetStream *stream) = 0;
}; // class Serializable

} // namespace FI
} // namespace BTech
#endif /* __cplusplus */

#endif /* !BTECH_FI_STREAM_H */
