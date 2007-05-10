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

#ifdef __cplusplus
	bool operator < (const FI_UUID& rhs) const {
		for (size_t ii = 0; ii < sizeof(octets); ii++) {
			if (octets[ii] < rhs.octets[ii]) {
				return true;
			} else if (rhs.octets[ii] < octets[ii]) {
				return false;
			}
		}

		return false;
	}
#endif /* __cplusplus */
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

#endif /* !BTECH_FI_VALUES_H */
