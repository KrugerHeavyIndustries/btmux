/*
 * htab.c - table hashing routines 
 */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "externs.h"
#include "htab.h"
#include "alloc.h"

#include "mudconf.h"


/*
 * ---------------------------------------------------------------------------
 * * search_nametab: Search a name table for a match and return the flag value.
 */
int search_nametab(dbref player, NAMETAB *ntab, char *flagname)
{
    NAMETAB *nt;

    for (nt = ntab; nt->name; nt++) {
        if (minmatch(flagname, nt->name, nt->minlen)) {
            if (check_access(player, nt->perm)) {
                return nt->flag;
            } else
                return -2;
        }
    }
    return -1;
}

/*
 * ---------------------------------------------------------------------------
 * * find_nametab_ent: Search a name table for a match and return a pointer to it.
 */

NAMETAB *find_nametab_ent(dbref player, NAMETAB *ntab, char *flagname) {
    NAMETAB *nt;

    for (nt = ntab; nt->name; nt++) {
        if (minmatch(flagname, nt->name, nt->minlen)) {
            if (check_access(player, nt->perm)) {
                return nt;
            }
        }
    }
    return NULL;
}

/*
 * ---------------------------------------------------------------------------
 * * display_nametab: Print out the names of the entries in a name table.
 */

void display_nametab(dbref player, NAMETAB *ntab, char *prefix, 
    int list_if_none)
{
    char *buf, *bp, *cp;
    NAMETAB *nt;
    int got_one;

    buf = alloc_lbuf("display_nametab");
    bp = buf;
    got_one = 0;
    for (cp = prefix; *cp; cp++)
        *bp++ = *cp;
    for (nt = ntab; nt->name; nt++) {
        if (God(player) || check_access(player, nt->perm)) {
            *bp++ = ' ';
            for (cp = nt->name; *cp; cp++)
                *bp++ = *cp;
            got_one = 1;
        }
    }
    *bp = '\0';
    if (got_one || list_if_none)
        notify(player, buf);
    free_lbuf(buf);
}



/*
 * ---------------------------------------------------------------------------
 * * interp_nametab: Print values for flags defined in name table.
 */

void interp_nametab(dbref player, NAMETAB *ntab, int flagword, char *prefix, 
    char *true_text, char *false_text)
{
    char *buf, *bp, *cp;
    NAMETAB *nt;

    buf = alloc_lbuf("interp_nametab");
    bp = buf;
    for (cp = prefix; *cp; cp++)
        *bp++ = *cp;
    nt = ntab;
    while (nt->name) {
        if (God(player) || check_access(player, nt->perm)) {
            *bp++ = ' ';
            for (cp = nt->name; *cp; cp++)
                *bp++ = *cp;
            *bp++ = '.';
            *bp++ = '.';
            *bp++ = '.';
            if ((flagword & nt->flag) != 0)
                cp = true_text;
            else
                cp = false_text;
            while (*cp)
                *bp++ = *cp++;
            if ((++nt)->name)
                *bp++ = ';';
        }
    }
    *bp = '\0';
    notify(player, buf);
    free_lbuf(buf);
}

/*
 * ---------------------------------------------------------------------------
 * * listset_nametab: Print values for flags defined in name table.
 */

void listset_nametab(dbref player, NAMETAB *ntab, int flagword, char *prefix, 
    int list_if_none)
{
    char *buf, *bp, *cp;
    NAMETAB *nt;
    int got_one;

    buf = bp = alloc_lbuf("listset_nametab");
    for (cp = prefix; *cp; cp++)
        *bp++ = *cp;
    nt = ntab;
    got_one = 0;
    while (nt->name) {
        if (((flagword & nt->flag) != 0) && (God(player) ||
                    check_access(player, nt->perm))) {
            *bp++ = ' ';
            for (cp = nt->name; *cp; cp++)
                *bp++ = *cp;
            got_one = 1;
        }
        nt++;
    }
    *bp = '\0';
    if (got_one || list_if_none)
        notify(player, buf);
    free_lbuf(buf);
}

extern void cf_log_notfound(dbref, char *, const char *, char *);

int cf_ntab_access(int *vp, char *str, long extra, dbref player, char *cmd)
{
    NAMETAB *np;
    char *ap;

    for (ap = str; *ap && !isspace(*ap); ap++);
    if (*ap)
        *ap++ = '\0';
    while (*ap && isspace(*ap))
        ap++;
    for (np = (NAMETAB *) vp; np->name; np++) {
        if (minmatch(str, np->name, np->minlen)) {
            return cf_modify_bits(&(np->perm), ap, extra, player, cmd);
        }
    }
    cf_log_notfound(player, cmd, "Entry", str);
    return -1;
}

