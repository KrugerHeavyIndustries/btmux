/*
 * Common, shared definitions.
 */

#ifndef BTECH_FI_COMMON_H
#define BTECH_FI_COMMON_H

#include <stddef.h>

/*
 * Constants.
 */

#define FI_ONE_MEG 1048576U /* 2^20 */
#define FI_FOUR_GIG 4294967296ULL /* 2^32 */

/*
 * Common types.
 */

typedef unsigned char FI_Octet;		/* 8-bit octet */
typedef char FI_Char;			/* UTF-8 character component */

typedef unsigned int FI_UInt20;		/* 0 to at least 2^20 */
typedef unsigned int FI_Length;		/* 0 to at least 2^32 - 1 */

/*
 * Error handling.
 */

typedef enum {
	FI_ERROR_NONE,			/* No error */
	FI_ERROR_OOM,			/* Out of memory */
	FI_ERROR_EOS,			/* End of stream */
	FI_ERROR_NOFILE,		/* File not found */
	FI_ERROR_INVAL,			/* Invalid argument */
	FI_ERROR_ILLEGAL,		/* Illegal state */
	FI_ERROR_ERRNO,			/* Check errno */
	FI_ERROR_EXCEPTION		/* Caught Exception */
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

#define FI_SET_ERROR(ei,ec) \
	do { \
		(ei).error_code = (ec); \
		(ei).error_string = fi_error_strings[(ec)]; \
	} while (0)

#define FI_COPY_ERROR(lhs,rhs) \
	do { \
		(lhs).error_code = (rhs).error_code; \
		(lhs).error_stirng = (rhs).error_string; \
	} while (0)

extern const char *const fi_error_strings[];

/*
 * Vocabulary index types.
 */

/* Sections 6.5 and 6.10: Vocabulary table indexes range from 1 to 2^20.  */
typedef unsigned int FI_VocabIndex;

#define FI_VOCAB_INDEX_NULL 0

/* Convenience constants for built-in restricted alphabets.  */
typedef enum {
	FI_RA_NUMERIC = 1,		/* numeric (section 9.1) */
	FI_RA_DATE_AND_TIME		/* date and time (section 9.2) */
} FI_RA_VocabIndex;

/* Convenience constants for built-in encoding algorithms.  */
typedef enum {
	FI_EA_HEXADECIMAL = 1,		/* hexadecimal (section 10.2) */
	FI_EA_BASE64,			/* Base64 (section 10.3) */
	FI_EA_SHORT,			/* 16-bit integer (section 10.4) */
	FI_EA_INT,			/* 32-bit integer (section 10.5) */
	FI_EA_LONG,			/* 64-bit integer (section 10.6) */
	FI_EA_BOOLEAN,			/* boolean (section 10.7) */
	FI_EA_FLOAT,			/* single precision (section 10.8) */
	FI_EA_DOUBLE,			/* double precision (section 10.9) */
	FI_EA_UUID,			/* UUID (section 10.10) */
	FI_EA_CDATA			/* CDATA (section 10.11) */
} FI_EA_VocabIndex;

/* Convenience constants for built-in prefixes.  */
typedef enum {
	FI_PFX_XML = 1			/* xml (section 7.2.21) */
} FI_PFX_VocabIndex;

typedef enum {
	FI_NSN_XML = 1			/* http://www.w3.org/XML/1998/namespace (section 7.2.22) */
} FI_NSN_VocabIndex;

#ifdef __cplusplus

#include <string>

namespace BTech {
namespace FI {

typedef std::string CharString;

} // namespace FI
} // namespace BTech

#endif /* __cplusplus */

#endif /* !BTECH_FI_COMMON_H */
