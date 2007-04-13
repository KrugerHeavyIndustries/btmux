
/* externs.h - Prototypes for externs not defined elsewhere */

/* $Id: externs.h,v 1.6 2005/08/08 10:30:11 murrayma Exp $ */

#include "config.h"
#include "copyright.h"

#ifndef __EXTERNS__H
#define	__EXTERNS__H

#include "htab.h"

#define ToLower(C)	(((C) >= 'A' && (C) <= 'Z')? (C) - 'A' + 'a': (C))

/* From eval.c */
void exec(char *, char **, int, dbref, dbref, int, char **, char *[], int);

/* From game.c */
#define	notify(p,m)			notify_checked(p,p,m, MSG_PUP_ALWAYS|MSG_ME_ALL|MSG_F_DOWN)
void notify_printf(dbref, const char *, ...);
void notify_checked(dbref, dbref, const char *, int);

/* From log.c */
char *strip_ansi_r(char *, const char *, size_t);
void log_perror(const char *, const char *, const char *, const char *);
void log_error(int, char *, char *, char *, ...);

/* From move.c */
int move_via_teleport(dbref, dbref, dbref, int);

/* From object.c */
dbref create_obj(dbref, int, char *, int);

/* From player.c */
dbref lookup_player(dbref, char *, int);

/* From predicates.c */
char *tprintf(const char *, ...);
void safe_tprintf_str(char *, char **, const char *, ...);
int could_doit(dbref, dbref, int);
void did_it(dbref, dbref, int, const char *, int, const char *, int, char *[], int);

/* From stringutil.c */
int safe_copy_str(char *, char *, char **, int);

/* From wild.c */
int quick_wild(char *, char *);

/* from db.c */
char *Name(dbref);
char *atr_get_str(char *, dbref, int, dbref *, int *);
void atr_add_raw(dbref, int, char *);

/* Command handler keys */
#define	SET_QUIET	1	/* Don't display 'Set.' message. */

/* Evaluation directives */

#define	EV_FIGNORE	0x00000000	/* Don't look for func if () found */
#define	EV_STRIP_AROUND	0x00008000	/* Strip {} only at ends of string */
#define	EV_NOTRACE	0x00020000	/* Don't trace this call to eval */
#define EV_NO_COMPRESS  0x00040000	/* Don't compress spaces. */
#define EV_NO_LOCATION	0x00080000	/* Supresses %l */
#define EV_NOFCHECK	0x00100000	/* Do not evaluate functions! */

/* Message forwarding directives */

#define	MSG_PUP_ALWAYS	1	/* Always forward msg to puppet own */
#define	MSG_INV_L	4	/* ... only if msg passes my @listen */
#define	MSG_INV_EXITS	8	/* Forward through my audible exits */
#define	MSG_NBR_A	32	/* ... only if I am audible */
#define	MSG_NBR_EXITS_A	128	/* ... only if I am audible */
#define	MSG_LOC_A	512	/* ... only if I am audible */
#define	MSG_FWDLIST	1024	/* Forward to my fwdlist members if aud */
#define	MSG_ME		2048	/* Send to me */
#define	MSG_S_INSIDE	4096	/* Originator is inside target */
#define	MSG_S_OUTSIDE	8192	/* Originator is outside target */
#define MSG_COLORIZE    16384	/* Message needs to be given color */
#define	MSG_ME_ALL	(MSG_ME|MSG_INV_EXITS|MSG_FWDLIST)
#define	MSG_F_UP	(MSG_NBR_A|MSG_LOC_A)
#define	MSG_F_DOWN	(MSG_INV_L)

#endif				/* __EXTERNS_H */
