
/* interface.h */

/* $Id: interface.h,v 1.7 2005/08/08 10:30:11 murrayma Exp $ */

#include "copyright.h"

#ifndef __INTERFACE__H
#define __INTERFACE__H

#include "config.h" /* for dbref */

#include "btpr_interface.h"

typedef BT_DESC DESC;

extern void hudinfo_notify(DESC *, const char *, const char *, const char *);

#endif
