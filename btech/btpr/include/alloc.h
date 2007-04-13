
/* alloc.h - External definitions for memory allocation subsystem */

/* $Id: alloc.h,v 1.3 2005/08/08 09:43:05 murrayma Exp $ */

#include "copyright.h"

#ifndef M_ALLOC_H
#define M_ALLOC_H

#include "btpr_alloc.h"

#define MBUF_SIZE	2048
#define SBUF_SIZE	256

#define	alloc_lbuf(s)	(char *)malloc(LBUF_SIZE)
#define	free_lbuf(b)	if (b) free(b)
#define	alloc_mbuf(s)	(char *)malloc(MBUF_SIZE)
#define	free_mbuf(b)	if (b) free(b)
#define	alloc_sbuf(s)	(char *)malloc(SBUF_SIZE)
#define	free_sbuf(b)	if (b) free(b)

#define	safe_str(s,b,p)		safe_copy_str(s,b,p,(LBUF_SIZE-1))

#endif				/* M_ALLOC_H */
