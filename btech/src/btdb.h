/*
 * btechdb routines.
 */

#ifndef BTECH_BTDB_H
#define BTECH_BTDB_H

#include <stddef.h>
#include <stdio.h>

#include "glue_types.h"

/* Initialize/finalize btechdb system state.  */
int init_btdb_state(size_t (*sizefunc)(GlueType));
int fini_btdb_state(void);

/* Save/load btechdb.finf.  */
int save_btdb(void);
int load_btdb(void);

#endif /* !BTECH_BTDB_H */
