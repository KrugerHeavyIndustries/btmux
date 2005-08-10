
/* mguests.h */

/* $Id: mguests.h,v 1.3 2005/08/08 09:43:07 murrayma Exp $ */

#ifndef  __MGUESTS_H
#define __MGUESTS_H

#include "copyright.h"
#include "config.h"
#include "interface.h"

extern char *make_guest(DESC *);
extern dbref create_guest(char *, char *);
extern void destroy_guest(dbref);
#endif
