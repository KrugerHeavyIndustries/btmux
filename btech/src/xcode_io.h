/*
 * XCODE I/O routines.
 */

#ifndef BTECH_XCODE_IO_H
#define BTECH_XCODE_IO_H

#include <stdio.h>

#include "glue_types.h"

/* Save/load btechdb.finf.  */
int init_btech_database_parser(void);
int fini_btech_database_parser(void);
int save_btech_database(void);
int load_btech_database(size_t (*sizefunc)(GlueType));

#endif /* !BTECH_XCODE_IO_H */
