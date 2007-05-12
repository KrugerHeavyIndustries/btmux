/*
 * XCODE I/O routines.
 */

#ifndef BTECH_XCODE_IO_H
#define BTECH_XCODE_IO_H

#include <stdio.h>

#include "glue_types.h"

/* Returns a count of the number of items saved, -1 if error.  */
int save_xcode_tree(FILE *f);

/* Returns a count of the number of items loaded, -1 if error.  */
int load_xcode_tree(FILE *f, size_t (*sizefunc)(GlueType));

/* EXPERIMENTAL: Save/load hcode.db in the new format as btechdb.finf.  */
int init_btech_database_parser(void);
int fini_btech_database_parser(void);
int save_btech_database(void);
int load_btech_database(void);

#endif /* !BTECH_XCODE_IO_H */
