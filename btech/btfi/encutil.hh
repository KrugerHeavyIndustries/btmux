/*
 * Utility routines for Fast Infoset encoding/decoding.
 */

#ifndef BTECH_FI_ENCUTIL_HH
#define BTECH_FI_ENCUTIL_HH

#include "common.h"
#include "stream.h"

#include "Name.hh"
#include "Value.hh"
#include "Vocabulary.hh"

namespace BTech {
namespace FI {

// C.4
bool write_attribute(FI_OctetStream *stream,
                     const DN_VocabTable::TypedEntryRef& name,
                     const Value& value);

// C.12
bool write_namespace_attribute(FI_OctetStream *stream,
                               const NSN_DS_VocabTable::TypedEntryRef& ns_name);

bool read_namespace_attribute(FI_OctetStream *stream, FI_Length& adv_len,
                              Vocabulary& vocabulary,
                              NSN_DS_VocabTable::TypedEntryRef& ns_name);

// C.13
bool write_identifier(FI_OctetStream *stream,
                      const DS_VocabTable::TypedEntryRef& id);

bool read_identifier(FI_OctetStream *stream, FI_Length& adv_len,
                     DS_VocabTable& string_table,
                     DS_VocabTable::TypedEntryRef& id);

// C.14
bool write_value_bit_1(FI_OctetStream *stream, const Value& value);

#if 0 // defined(FI_USE_INITIAL_VOCABULARY)
// C.16
bool write_name_surrogate(FI_OctetStream *stream, const FI_Name *name);
#endif // FI_USE_INITIAL_VOCABULARY

// C.17
bool write_name_bit_2(FI_OctetStream *stream,
                      const DN_VocabTable::TypedEntryRef& name);

// C.18
bool write_name_bit_3(FI_OctetStream *stream,
                      const DN_VocabTable::TypedEntryRef& name);

bool read_name_bit_3(FI_OctetStream *stream, FI_Length& av_len,
                     Vocabulary& vocabulary,
                     DN_VocabTable::TypedEntryRef& name);

// C.19
bool write_encoded_bit_3(FI_OctetStream *stream, const Value& value);

#if 0 // defined(FI_USE_INITIAL_VOCABULARY)
// C.21
bool write_length_sequence_of(FI_OctetStream *stream, FI_PInt20 len);
#endif // FI_USE_INITIAL_VOCABULARY

// C.22
FI_Octet *write_non_empty_octets_bit_2(FI_OctetStream *stream, FI_PInt32 len);

const FI_Octet *read_non_empty_octets_bit_2(FI_OctetStream *stream,
                                            FI_Length& adv_len,
                                            FI_PInt32& len);

// C.23
FI_Octet *write_non_empty_octets_bit_5(FI_OctetStream *stream, FI_PInt32 len);

// C.25
bool write_pint20_bit_2(FI_OctetStream *stream, FI_PInt20 val);

bool read_pint20_bit_2(FI_OctetStream *stream, FI_Length& adv_len,
                       FI_PInt20& val);

// C.27
bool write_pint20_bit_3(FI_OctetStream *stream, FI_PInt20 val);

bool read_pint20_bit_3(FI_OctetStream *stream, FI_Length& adv_len,
                       FI_PInt20& val);

// C.29
bool write_pint8(FI_OctetStream *stream, FI_PInt8 val);

bool read_pint8(FI_OctetStream *stream, FI_Length& adv_len, FI_PInt8& val);

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_ENCUTIL_HH */
