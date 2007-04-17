/*
 * Module implementing the encoding algorithms of X.891, including the built-in
 * algorithms of section 10.
 */

#ifndef BTECH_FI_ENCALG_H
#define BTECH_FI_ENCALG_H

#include "common.h"
#include "stream.h"

typedef struct {
	int (*encode)(FI_OctetStream *dst, const void *src);
	int (*decode)(void *dst, FI_OctetStream *src);
} FI_EncodingAlgorithm;

extern const FI_EncodingAlgorithm fi_ea_hexadecimal;
extern const FI_EncodingAlgorithm fi_ea_base64;
extern const FI_EncodingAlgorithm fi_ea_short;
extern const FI_EncodingAlgorithm fi_ea_int;
extern const FI_EncodingAlgorithm fi_ea_long;
extern const FI_EncodingAlgorithm fi_ea_boolean;
extern const FI_EncodingAlgorithm fi_ea_float;
extern const FI_EncodingAlgorithm fi_ea_double;
extern const FI_EncodingAlgorithm fi_ea_cdata;
extern const FI_EncodingAlgorithm fi_ea_uuid;

#endif /* !BTECH_FI_ENCALG_H */
