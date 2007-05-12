/*
 * Generic value type.
 */

#ifndef BTECH_FI_VALUES_H
#define BTECH_FI_VALUES_H

#include <stddef.h>

#include "fiptypes.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct FI_tag_UUID FI_UUID;	/* 128-bit (16 octet) UUID */

struct FI_tag_UUID {
	FI_Octet octets[16];
}; /* FI_UUID */

typedef enum {
	FI_VALUE_AS_NULL,		/* no value */

	FI_VALUE_AS_SHORT,		/* FI_Int16 */
	FI_VALUE_AS_INT,		/* FI_Int32 */
	FI_VALUE_AS_LONG,		/* FI_Int64 */
	FI_VALUE_AS_BOOLEAN,		/* FI_Boolean */
	FI_VALUE_AS_FLOAT,		/* FI_Float32 */
	FI_VALUE_AS_DOUBLE,		/* FI_Float64 */
	FI_VALUE_AS_UUID,		/* FI_UUID */

	FI_VALUE_AS_UTF8,		/* FI_UInt8[] (as valid UTF-8) */
	FI_VALUE_AS_UTF16,		/* FI_UInt16[] (not supported) */

	FI_VALUE_AS_OCTETS		/* FI_Octet[] (standard is Base64) */
} FI_ValueType;

typedef struct FI_tag_Value FI_Value;

FI_Value *fi_create_value(void);
void fi_destroy_value(FI_Value *obj);

FI_ValueType fi_get_value_type(const FI_Value *obj);
size_t fi_get_value_count(const FI_Value *obj);

const void *fi_get_value(const FI_Value *obj);

int fi_set_value_type(FI_Value *obj, FI_ValueType type, size_t count);
void fi_set_value(FI_Value *obj, const void *buf);
void *fi_get_value_buffer(FI_Value *obj);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* !BTECH_FI_VALUES_H */
