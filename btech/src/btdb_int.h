/*
 * Internal btechdb declarations.
 */

#ifndef BTECH_BTDB_INT_H
#define BTECH_BTDB_INT_H

#include "sax.h"

typedef struct {
	const int idx;
	const char *const literal;
	FI_Name *cached;
} CachedName;

typedef struct {
	const int idx;
	const FI_ValueType type;
	const size_t count;
	FI_Value *cached;
} CachedVariable;

extern FI_ContentHandler *btdb_gen_handler;

extern FI_Attributes *btdb_attrs;

int init_btdb_element_names(CachedName *table);
int init_btdb_attribute_names(CachedName *table);
void fini_btdb_names(CachedName *table);

int init_btdb_variables(CachedVariable *table);
void fini_btdb_variables(CachedVariable *table);

#endif /* !BTECH_BTDB_INT_H */
