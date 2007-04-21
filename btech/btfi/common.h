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
	FI_ERROR_EOS,			/* End of stream */
	FI_ERROR_NOFILE,		/* File not found */
	FI_ERROR_INVAL,			/* Invalid argument */
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
typedef unsigned long VocabIndex;

#define FI_VOCAB_INDEX_NULL 0
#define FI_VOCAB_INDEX_MAX FI_ONE_MEG

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

typedef struct FI_NameSurrogate {
	VocabIndex prefix_idx;		/* optional */
	VocabIndex namespace_idx;	/* optional, required by prefix_idx */
	VocabIndex local_idx;		/* required */

#ifdef __cplusplus
	FI_NameSurrogate (VocabIndex local_idx,
	                  VocabIndex namespace_idx = FI_VOCAB_INDEX_NULL,
	                  VocabIndex prefix_idx = FI_VOCAB_INDEX_NULL)
	: prefix_idx(prefix_idx),
	  namespace_idx(namespace_idx),
	  local_idx(local_idx) {}

	bool operator< (const FI_NameSurrogate& rhs) const throw () {
		if (prefix_idx < rhs.prefix_idx) {
			return true;
		} else if (namespace_idx < rhs.namespace_idx) {
			return true;
		} else {
			return local_idx < rhs.local_idx;
		}
	}

	bool operator== (const FI_NameSurrogate& rhs) const throw () {
		return (local_idx == rhs.local_idx
		        && namespace_idx == rhs.namespace_idx
		        && prefix_idx == rhs.prefix_idx);
	}

	bool operator!= (const FI_NameSurrogate& rhs) const throw () {
		return !(*this == rhs);
	}
#endif /* __cplusplus */
} FI_NameSurrogate;

#ifdef __cplusplus

#include <exception>
#include <string>

namespace BTech {
namespace FI {

typedef std::basic_string<FI_Char> CharString;

class Exception : public std::exception {
}; // Exception

class IndexOutOfBoundsException : public Exception {
}; // IndexOutOfBoundsException

class InvalidArgumentException : public Exception {
}; // InvalidArgumentException

class IllegalStateException : public Exception {
}; // IllegalStateException

class UnsupportedOperationException : public Exception {
}; // UnsupportedOperationException

} // namespace FI
} // namespace BTech

#endif /* __cplusplus */

#endif /* !BTECH_FI_COMMON_H */
