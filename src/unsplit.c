
/*
 * unsplit.c -- filter for re-combining continuation lines 
 */

/*
 * $Id: unsplit.c,v 1.1 2005/06/13 20:50:48 murrayma Exp $ 
 */

#include "copyright.h"

#include <stdio.h>
#include <ctype.h>

int main(argc, argv)
int argc;
char **argv;
{
    int c, numcr;

    while ((c = getchar()) != EOF) {
	if (c == '\\') {
	    numcr = 0;
	    do {
		c = getchar();
		if (c == '\n')
		    numcr++;
	    } while ((c != EOF) && isspace(c));
	    if (numcr > 1)
		putchar('\n');
	    ungetc(c, stdin);
	} else {
	    putchar(c);
	}
    }
    fflush(stdout);
    return 0;
}
