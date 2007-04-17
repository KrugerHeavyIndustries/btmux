/*
 * Common, shared definitions.
 */

#ifndef BTECH_FI_COMMON_H
#define BTECH_FI_COMMON_H

#include <stddef.h>

/*
 * Constants.
 */

#define FI_ONE_MEG 1048576

/*
 * Common types.
 */

typedef unsigned char FI_Octet;		/* 8-bit octet */
typedef char FI_Char;			/* UTF-8 character component */

typedef unsigned long FI_Length;	/* 0 to at least 2^32 - 1 */

/*
 * Error handling.
 */

typedef enum {
	FI_ERROR_NONE,			/* No error */
	FI_ERROR_OOM,			/* Out of memory */
	FI_ERROR_EOS			/* End of stream */
} FI_ErrorCode;

typedef struct {
	FI_ErrorCode error_code;	/* error code */
	const char *error_string;	/* descriptive string; read-only */
} FI_ErrorInfo;

#define FI_CLEAR_ERROR(ei) \
	do { \
		(ei).error_code = FI_ERROR_NONE; \
		(ei).error_string = NULL; \
	} while (0)

extern const char *const fi_error_strings[];

#endif /* !BTECH_FI_COMMON_H */
