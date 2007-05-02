/*
 * Utility routines for Fast Infoset encoding/decoding.
 */

#ifndef BTECH_FI_ENCUTIL_HH
#define BTECH_FI_ENCUTIL_HH

#include "common.h"
#include "stream.h"

#include "names.h"

#include "vocab.hh"
#include "Name.hh"

namespace BTech {
namespace FI {

// C.12
bool write_namespace_attribute(FI_OctetStream *stream,
                               const VocabTable::EntryRef& namespace_name);

// C.13
bool write_identifier(FI_OctetStream *stream,
                      const VocabTable::EntryRef& id);

// C.16
bool write_name_surrogate(FI_OctetStream *stream, const FI_Name *name);

// C.18
bool write_name_bit_3(FI_OctetStream *stream,
                      const VocabTable::EntryRef& name);

// C.21
bool write_length_sequence_of(FI_OctetStream *stream, FI_Length len);

// C.22
bool write_non_empty_string_bit_2(FI_OctetStream *stream,
                                  const CharString& str);

// C.25
bool write_non_zero_uint20_bit_2(FI_OctetStream *stream, FI_UInt20 val);

// C.27
bool write_non_zero_uint20_bit_3(FI_OctetStream *stream, FI_UInt20 val);

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_ENCUTIL_HH */
