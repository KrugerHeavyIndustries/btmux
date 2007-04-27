/*
 * Utility routines for Fast Infoset encoding/decoding.
 */

#ifndef BTECH_FI_ENCUTIL_HH
#define BTECH_FI_ENCUTIL_HH

#include "common.h"
#include "stream.h"

namespace BTech {
namespace FI {

// C.16
bool write_name_surrogate(FI_OctetStream *stream, const FI_NameSurrogate& name)
                         throw ();

// C.21
bool write_length_sequence_of(FI_OctetStream *stream, FI_Length len)
                             throw ();

// C.22
bool write_non_empty_string_bit_2(FI_OctetStream *stream, const CharString& str)
                                 throw ();

// C.25
bool write_non_zero_uint20_bit_2(FI_OctetStream *stream, FI_UInt20 val)
                                throw ();

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_ENCUTIL_HH */
