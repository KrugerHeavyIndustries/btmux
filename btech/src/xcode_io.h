/*
 * XCODE I/O routines.
 */

#ifndef BTECH_XCODE_IO_H
#define BTECH_XCODE_IO_H

#include "sax.h"

#include "glue_types.h"

int btdb_init_xcode(size_t (*size_func)(GlueType));
void btdb_fini_xcode(void);

int btdb_save_xcode(const FI_Name *name_xcode);
void btdb_load_fini_xcode(void);

int btdb_start_xcode(const FI_Attributes *attrs);
int btdb_end_xcode(void);

int btdb_start_in_xcode(const FI_Name *name, const FI_Attributes *attrs);
int btdb_end_in_xcode(const FI_Name *name);
int btdb_chars_in_xcode(const FI_Value *value);

#endif /* !BTECH_XCODE_IO_H */
