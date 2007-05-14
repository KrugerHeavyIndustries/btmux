/*
 * Vocabulary table.
 */

#ifndef BTECH_FI_VOCAB_H
#define BTECH_FI_VOCAB_H

#include "names.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct FI_tag_Vocabulary FI_Vocabulary;

FI_Vocabulary *fi_create_vocabulary(void);
void fi_destroy_vocabulary(FI_Vocabulary *vocab);

FI_Name *fi_create_element_name(FI_Vocabulary *vocab, const char *name);
FI_Name *fi_create_attribute_name(FI_Vocabulary *vocab, const char *name);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !BTECH_FI_VOCAB_H */
