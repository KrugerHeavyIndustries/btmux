/* alloc.h - External definitions for memory allocation subsystem */

#include "copyright.h"

#ifndef M_ALLOC_H
#define M_ALLOC_H

#define POOL_SBUF   0
#define POOL_MBUF   1
#define POOL_LBUF   2
#define POOL_BOOL   3
#define POOL_DESC   4
#define POOL_QENTRY 5
#define POOL_PCACHE 6
#define NUM_POOLS   7


#define LBUF_SIZE   16384
#define MBUF_SIZE   2048
#define SBUF_SIZE   256

#define alloc_lbuf(s)       (char *)malloc(LBUF_SIZE)
#define free_lbuf(b)        if (b) free(b)
#define alloc_mbuf(s)       (char *)malloc(MBUF_SIZE)
#define free_mbuf(b)        if (b) free(b)
#define alloc_sbuf(s)       (char *)malloc(SBUF_SIZE)
#define free_sbuf(b)        if (b) free(b)
#define alloc_bool(s)       (struct boolexp *)malloc(sizeof(struct boolexp))
#define free_bool(b)        if (b) free(b)
#define alloc_qentry(s)     (BQUE *)malloc(sizeof(BQUE))
#define free_qentry(b)      if (b) free(b)
#define alloc_pcache(s)     (PCACHE *)malloc(sizeof(PCACHE)
#define free_pcache(b)      if (b) free(b)

#define safe_str(s,b,p)     safe_copy_str(s,b,p,(LBUF_SIZE-1))
#define safe_chr(c,b,p)     safe_copy_chr(c,b,p,(LBUF_SIZE-1))
#define safe_sb_str(s,b,p)  safe_copy_str(s,b,p,(SBUF_SIZE-1))
#define safe_sb_chr(c,b,p)  safe_copy_chr(c,b,p,(SBUF_SIZE-1))
#define safe_mb_str(s,b,p)  safe_copy_str(s,b,p,(MBUF_SIZE-1))
#define safe_mb_chr(c,b,p)  safe_copy_chr(c,b,p,(MBUF_SIZE-1))

#endif              /* M_ALLOC_H */
