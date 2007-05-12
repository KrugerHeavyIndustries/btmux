/*
 * Various primitive types.
 */

#ifndef BTECH_FI_FIPTYPES_H
#define BTECH_FI_FIPTYPES_H

#include <stdint.h>

/*
 * Basic integer types.
 */

#define FI_UINT32_MAX 4294967295U	/* 2^32 - 1 */

typedef uint_fast8_t FI_UInt8;		/* unsigned 8-bit integer */
typedef uint_fast16_t FI_UInt16;	/* unsigned 16-bit integer */
typedef uint_fast32_t FI_UInt32;	/* unsigned 32-bit integer */
typedef uint_fast64_t FI_UInt64;	/* unsigned 64-bit integer */

typedef int_fast8_t FI_Int8;		/* signed 8-bit integer */
typedef int_fast16_t FI_Int16;		/* signed 16-bit integer */
typedef int_fast32_t FI_Int32;		/* signed 32-bit integer */
typedef int_fast64_t FI_Int64;		/* signed 64-bit integer */

typedef uint_fast8_t FI_Boolean;	/* boolean */

/*
 * Basic floating point types.
 */

typedef float FI_Float32;		/* single-precision IEEE 754 */
typedef double FI_Float64;		/* double-precision IEEE 754 */

/*
 * Some limit constants.
 */

#define FI_ONE_MEG 1048576U /* 2^20 */
#define FI_FOUR_GIG 4294967296ULL /* 2^32 */

/*
 * Positive integer types.  These encode values from 1 to some power of 2,
 * inclusive.  To do this within the smallest possible integer type, the raw
 * value is shifted down by 1.
 *
 * In practice, this means some values aren't usable, but at least they can
 * handled.
 *
 * A companion type, FI_UInt21, provides an unsigned integer type capable of
 * holding at least 2^20. (The name "FI_UInt21" is a bit of a misnomer.)
 */

typedef uint_fast32_t FI_UInt21;	/* 0 to at least 2^20 */

#define FI_PINT8_MAX  255U		/* 2^8 in p-int encoding */
#define FI_PINT20_MAX 1048575U		/* 2^20 in p-int encoding */
#define FI_PINT32_MAX FI_UINT32_MAX	/* 2^32 in p-int encoding */

typedef FI_UInt8 FI_PInt8;		/* 1 to at least 2^8 */
typedef uint_fast32_t FI_PInt20;	/* 1 to at least 2^20 */
typedef FI_UInt32 FI_PInt32;		/* 1 to at least 2^32 */

#define FI_UINT_TO_PINT(ui) ((ui) - 1U)	/* convert from UInt to PInt range */
#define FI_PINT_TO_UINT(pi) ((pi) + 1U)	/* convert from PInt to UInt range */

/*
 * Derived types.
 *
 * TODO: Move these to more appropriate headers.  FI_Octet should probably go
 * with stream.h, and FI_VocabIndex and friends with VocabTable.hh.
 */

typedef FI_UInt8 FI_Octet;		/* octet */

/* Sections 6.5 and 6.10: Vocabulary table indexes range from 1 to 2^20.  */
typedef FI_UInt21 FI_VocabIndex;	/* vocab index */
#define FI_VOCAB_INDEX_NULL 0		/* null vocab index */

#endif /* !BTECH_FI_FIPTYPES_H */
