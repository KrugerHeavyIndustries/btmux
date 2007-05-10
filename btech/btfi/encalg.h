/*
 * Module implementing the encoding algorithms of X.891, including the built-in
 * algorithms of section 10.
 */

#ifndef BTECH_FI_ENCALG_H
#define BTECH_FI_ENCALG_H

#include <stddef.h>

#include "stream.h"
#include "values.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Encoding algorithms can use this structure to pass state.  */
typedef struct {
	void *context;

	FI_Length encoded_size;
	size_t decoded_size;
} FI_EncodingContext;

/* The encoding algorithm interface.  */
typedef struct {
	/* Structure version number = 0.  */
	int version;

	/* encoded_size() must compute the size of the output buffer.  */
	int (*encoded_size)(FI_EncodingContext *context,
	                    const FI_Value *src);

	/* encode() must write out the encoded value of src.  */
	int (*encode)(FI_EncodingContext *context, FI_Octet *dst,
	              const FI_Value *src);

	/* decoded_size() must compute the count of the output Value.  */
	int (*decoded_size)(FI_EncodingContext *context,
	                    FI_Length src_len, const FI_Octet *src);

	/* decode() must write out the decoded value of src.  */
	int (*decode)(FI_EncodingContext *context, FI_Value *dst,
	              FI_Length src_len, const FI_Octet *src);
} FI_EncodingAlgorithm;

/* Built-in encoding algorithm initialization.  Call this at least once before
 * using any of the built-in encoding algorithms.  Returns 0 on failure, !0 on
 * success.  */
int fi_init_encoding_algorithms(void);

/* The built-in encoding algorithms.  */
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

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !BTECH_FI_ENCALG_H */
