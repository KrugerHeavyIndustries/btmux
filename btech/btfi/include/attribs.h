/*
 * Attribute name-value list type.
 */

#ifndef BTECH_FI_ATTRIBS_H
#define BTECH_FI_ATTRIBS_H

#include "names.h"
#include "values.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Attributes.
 */

typedef struct FI_tag_Attributes FI_Attributes;

FI_Attributes *fi_create_attributes(void);
void fi_destroy_attributes(FI_Attributes *attrs);

void fi_clear_attributes(FI_Attributes *attrs);
int fi_add_attribute(FI_Attributes *attrs,
                     const FI_Name *name, const FI_Value *value);

int fi_get_attributes_length(const FI_Attributes *attrs);

int fi_get_attribute_index(const FI_Attributes *attrs, const FI_Name *name);

const FI_Name *fi_get_attribute_name(const FI_Attributes *attrs, int idx);
const FI_Value *fi_get_attribute_value(const FI_Attributes *attrs, int idx);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !BTECH_FI_ATTRIBS_H */
