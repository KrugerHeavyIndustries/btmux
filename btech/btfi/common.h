/*
 * Some common type definitions.
 */

#ifndef BTECH_FI_COMMON_H
#define BTECH_FI_COMMON_H

/*
 * Common types.
 */

typedef enum FI_tag_Ternary {
	FI_TERNARY_FALSE = -1,		/* ternary false */
	FI_TERNARY_UNKNOWN = 0,		/* ternary unknown */
	FI_TERNARY_TRUE = 1		/* ternary true */
} FI_Ternary; /* (balanced) ternary logic */

/*
 * Vocabulary index types.
 *
 * TODO: Move these out into their own vocabulary header.
 */

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
