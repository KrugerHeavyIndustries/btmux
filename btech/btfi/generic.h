/*
 * Generic types.
 */

#ifndef BTECH_FI_GENERIC_H
#define BTECH_FI_GENERIC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Generic names.
 */

typedef enum {
	FI_NAME_AS_NULL,		/* no name */
	FI_NAME_AS_INDEX		/* name surrogate FI_VocabIndex */
} FI_NameType;

typedef struct FI_tag_Name FI_Name;

FI_Name *fi_create_name(void);
void fi_destroy_name(FI_Name *obj);

FI_NameType fi_get_name_type(const FI_Name *obj);
const void *fi_get_name(const FI_Name *obj);

int fi_set_name(FI_Name *obj, FI_NameType type, const void *buf);

/*
 * Generic values.
 */

typedef int_fast16_t FI_Int16;		/* signed 16-bit integer */
typedef int_fast32_t FI_Int32;		/* signed 32-bit integer */
typedef int_fast64_t FI_Int64;		/* signed 64-bit integer */
typedef unsigned char FI_Boolean;	/* boolean */
typedef float FI_Float32;		/* single-precision IEEE 754 */
typedef double FI_Float64;		/* double-precision IEEE 754 */
typedef FI_Octet FI_UUID[16];		/* 128-bit (16 octet) UUID */

typedef enum {
	FI_VALUE_AS_NULL,		/* no value */
	FI_VALUE_AS_SHORT,		/* FI_Int16 */
	FI_VALUE_AS_INT,		/* FI_Int32 */
	FI_VALUE_AS_LONG,		/* FI_Int64 */
	FI_VALUE_AS_BOOLEAN,		/* FI_Boolean */
	FI_VALUE_AS_FLOAT,		/* FI_Float32 */
	FI_VALUE_AS_DOUBLE,		/* FI_Float64 */
	FI_VALUE_AS_UUID,		/* FI_UUID */
	FI_VALUE_AS_OCTETS		/* FI_Octet[] */
} FI_ValueType;

typedef struct FI_tag_Value FI_Value;

FI_Value *fi_create_value(void);
void fi_destroy_value(FI_Value *obj);

FI_ValueType fi_get_value_type(const FI_Value *obj);
size_t fi_get_value_count(const FI_Value *obj);
const void *fi_get_value(const FI_Value *obj);

int fi_set_value(FI_Value *obj,
                 FI_ValueType type, size_t count, const void *buf);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* !BTECH_FI_GENERIC_H */
