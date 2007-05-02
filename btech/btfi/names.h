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
 * corresponding vocabulary table.  For example, fi_create_element_name()
 * creates a new FI_Name in a FI_Generator's element vocabulary table.  */

void fi_destroy_name(FI_Name *);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* !BTECH_FI_NAMES_H */
