/* mudconf.h */

#include "config.h"

#ifndef __MUDCONF_H
#define __MUDCONF_H

/* Neither of these two definitions are used here, but some files only include
 * mudconf.h.
 */
#include "alloc.h" /* used for LBUF_SIZE */
#include "flags.h" /* used for GOD */

#include "btpr_mudconf.h" /* common mudconf.h items */

extern dbref btpr_db_top(void) __attribute__ ((pure));
#define MUDSTATE_DB_TOP (btpr_db_top())

#define LOG_WIZARD	0x00002000	/* Log dangerous things */

#endif
