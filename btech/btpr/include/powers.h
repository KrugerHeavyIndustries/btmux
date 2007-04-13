
/* powers.h - object powers */

/* $Id: powers.h,v 1.3 2005/06/23 02:59:58 murrayma Exp $ */

#include "copyright.h"

#ifndef __POWERS_H
#define	__POWERS_H

/* Second word of powers */

/* Mech stuff: */
#define POW_MECH        0x00000002	/* access to mech cmd set */
#define POW_SECURITY    0x00000004	/* 'admin' - debug/comp */
#define POW_MECHREP     0x00000008	/* access to mechrep cmd set */
#define POW_MAP         0x00000010	/* map modifying powers */
#define POW_TEMPLATE    0x00000020	/* templating powers */
#define POW_TECH        0x00000040	/* can do the IC tech commands */

/* ---------------------------------------------------------------------------
 * POWERENT: Information about object powers.
 */

extern int btpr_has_power2(dbref, int) __attribute__ ((pure));

/* Mecha */
#define Security(c)		(WizRoy(c) || btpr_has_power2((c), POW_SECURITY))
#define Tech(c)			(WizRoy(c) || btpr_has_power2((c), POW_TECH))
#define Template(c)		(WizRoy(c) || btpr_has_power2((c), POW_TEMPLATE))

/* End Mecha */
#endif				/* POWERS_H */
