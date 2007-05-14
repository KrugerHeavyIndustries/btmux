/*
 * Qualified name type.
 */

#ifndef BTECH_FI_NAMES_H
#define BTECH_FI_NAMES_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct FI_tag_Name FI_Name;

/* There is no fi_create_name(), because FI_Name objects are created from a
 * corresponding FI_Vocabulary.  For example, fi_create_element_name() creates
 * a new element name.  */

void fi_destroy_name(FI_Name *name);

int fi_names_equal(const FI_Name *name1, const FI_Name *name2);

const char *fi_get_name_prefix(const FI_Name *name);
const char *fi_get_name_namespace_name(const FI_Name *name);
const char *fi_get_name_local_name(const FI_Name *name);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !BTECH_FI_NAMES_H */
