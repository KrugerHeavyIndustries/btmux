/*
 * Module implementing the encoding algorithms of X.891, including the built-in
 * algorithms of section 10.
 */

#ifndef BTECH_FI_ENCALG_H
#define BTECH_FI_ENCALG_H

#include <stddef.h>

#include "stream.h" /* only for FI_Length? */
#include "values.h"
#include "errors.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Encoding algorithms can use this structure to pass state.  */
typedef struct {
	/*
	 * Identify this context as belonging to a particular encoding
	 * algorithm.  The initial value will be 0.  Encoding algorithms may
	 * assign a non-zero value here to indicate whether the rest of the
	 * structure is compatible with it.
	 *
	 * There's currently no mechanism in place for uniquely assigning these
	 * identifiers, as we only support the built-in algorithms.  These use
	 * their fixed vocabulary table indexes as their identifiers, so the
	 * values 0-31 are reserved for these internal uses.
	 */
	int identifier;

	/* State required by all encoding algorithms.  */
	FI_Length encoded_size;

	/*
	 * Error information.
	 * TODO: Not currently doing anything with this.
	 */
	FI_ErrorInfo error_info;

	/*
	 * This pointer is reserved for allowing encoding algorithms to
	 * maintain additional state.  It will initially be set to 0.
	 *
	 * If the context is dynamically allocated, free_context must point to
	 * a function that can deallocate the context.  Otherwise, free_context
	 * must be set to 0.
	 *
	 * If the context is replaced, the replacer is responsible for calling
	 * free_context before replacement.
	 */
	void *context;
	void (*free_context)(void *context);
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
