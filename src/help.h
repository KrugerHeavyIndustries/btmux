
/* help.h */

/* $Id: help.h,v 1.1 2005/06/13 20:50:47 murrayma Exp $ */

#ifndef _HELP_H
#define _HELP_H
#ifndef MKINDX
void help_write(dbref, char *, HASHTAB *, char *, int);
int helpindex_read(HASHTAB *, char *);
#endif
#define  LINE_SIZE		400

#define  TOPIC_NAME_LEN		30

typedef struct {
    long pos;			/* index into help file */
    int len;			/* length of help entry */
    char topic[TOPIC_NAME_LEN + 1];	/* topic of help entry */
} help_indx;
#endif
