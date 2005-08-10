
/*
 * powers.c - power manipulation routines 
 */

/*
 * $Id: powers.c,v 1.3 2005/08/08 09:43:07 murrayma Exp $ 
 */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "mudconf.h"
#include "externs.h"
#include "command.h"
#include "powers.h"
#include "alloc.h"

#ifndef STANDALONE

/*
 * ---------------------------------------------------------------------------
 * * ph_any: set or clear indicated bit, no security checking
 */

int ph_any(target, player, power, fpowers, reset)
dbref target, player;
POWER power;
int fpowers, reset;
{
    if (fpowers & POWER_EXT) {
	if (reset)
	    s_Powers2(target, Powers2(target) & ~power);
	else
	    s_Powers2(target, Powers2(target) | power);
    } else {
	if (reset)
	    s_Powers(target, Powers(target) & ~power);
	else
	    s_Powers(target, Powers(target) | power);
    }
    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * * ph_god: only GOD may set or clear the bit
 */

int ph_god(target, player, power, fpowers, reset)
dbref target, player;
POWER power;
int fpowers, reset;
{
    if (!God(player))
	return 0;
    return (ph_any(target, player, power, fpowers, reset));
}

/*
 * ---------------------------------------------------------------------------
 * * ph_wiz: only WIZARDS (or GOD) may set or clear the bit
 */

int ph_wiz(target, player, power, fpowers, reset)
dbref target, player;
POWER power;
int fpowers, reset;
{
    if (!Wizard(player) & !God(player))
	return 0;
    return (ph_any(target, player, power, fpowers, reset));
}

/*
 * ---------------------------------------------------------------------------
 * * ph_wizroy: only WIZARDS, ROYALTY, (or GOD) may set or clear the bit
 */

int ph_wizroy(target, player, power, fpowers, reset)
dbref target, player;
POWER power;
int fpowers, reset;
{
    if (!WizRoy(player) & !God(player))
	return 0;
    return (ph_any(target, player, power, fpowers, reset));
}

/*
 * ---------------------------------------------------------------------------
 * * ph_inherit: only players may set or clear this bit.
 */

int ph_inherit(target, player, power, fpowers, reset)
dbref target, player;
POWER power;
int fpowers, reset;
{
    if (!Inherits(player))
	return 0;
    return (ph_any(target, player, power, fpowers, reset));
}
/* *INDENT-OFF* */
POWERENT gen_powers[] =
{
{(char *)"quota",		POW_CHG_QUOTAS,	0, 0,	ph_wiz},
{(char *)"chown_anything", 	POW_CHOWN_ANY,  0, 0,	ph_wiz},
{(char *)"announce", 		POW_ANNOUNCE,	0, 0,	ph_wiz},
{(char *)"boot",		POW_BOOT,	0, 0,	ph_wiz},
{(char *)"halt",		POW_HALT,	0, 0,	ph_wiz},
{(char *)"control_all",		POW_CONTROL_ALL,0, 0,	ph_god},
{(char *)"expanded_who",	POW_WIZARD_WHO, 0, 0,	ph_wiz},
{(char *)"see_all",		POW_EXAM_ALL,	0, 0,	ph_wiz},
{(char *)"prog",		POW_PROG,	0, 0,	ph_wiz},
{(char *)"find_unfindable",	POW_FIND_UNFIND,0, 0,	ph_wiz},
{(char *)"free_money",		POW_FREE_MONEY, 0, 0,	ph_wiz},
{(char *)"free_quota",		POW_FREE_QUOTA, 0, 0,	ph_wiz},
{(char *)"hide",		POW_HIDE,	0, 0,	ph_wiz},
{(char *)"idle",		POW_IDLE, 	0, 0,	ph_wiz},
{(char *)"search",		POW_SEARCH,	0, 0,	ph_wiz},
{(char *)"long_fingers",	POW_LONGFINGERS,0, 0,	ph_wiz},
{(char *)"comm_all",		POW_COMM_ALL,	0, 0,	ph_wiz},
{(char *)"see_queue",		POW_SEE_QUEUE,	0, 0,	ph_wiz},
{(char *)"see_hidden",		POW_SEE_HIDDEN, 0, 0,	ph_wiz},
{(char *)"monitor",		POW_MONITOR,	0, 0,	ph_wiz},
{(char *)"poll",		POW_POLL,	0, 0,	ph_wiz},
{(char *)"no_destroy",		POW_NO_DESTROY, 0, 0,	ph_wiz},
{(char *)"guest",		POW_GUEST,	0, 0,	ph_god},
{(char *)"stat_any",		POW_STAT_ANY,	0, 0,	ph_wiz},
{(char *)"steal_money",		POW_STEAL,	0, 0,	ph_wiz},
{(char *)"tel_anywhere",	POW_TEL_ANYWHR, 0, 0,	ph_wiz},
{(char *)"tel_anything",	POW_TEL_UNRST,	0, 0,	ph_wiz},
{(char *)"unkillable",		POW_UNKILLABLE, 0, 0,	ph_wiz},
{(char *)"pass_locks",		POW_PASS_LOCKS, 0, 0,   ph_wiz},
{(char *)"builder",		POW_BUILDER,	POWER_EXT, 0,	ph_wiz},
/* mecha stuff */
{(char *)"mech",                POW_MECH,       POWER_EXT, 0,   ph_wiz},
{(char *)"security",            POW_SECURITY,   POWER_EXT, 0,   ph_wiz},
{(char *)"mechrep",             POW_MECHREP,    POWER_EXT, 0,   ph_wiz},
{(char *)"map",                 POW_MAP,        POWER_EXT, 0,   ph_wiz},
{(char *)"tech",                POW_TECH,       POWER_EXT, 0,   ph_wiz},
{(char *)"template",            POW_TEMPLATE,   POWER_EXT, 0,   ph_wiz},

/* mecha stuff end */
{NULL,				0,		0, 0,	0}};

#endif /* STANDALONE */
/* *INDENT-ON* */




#ifndef STANDALONE

/*
 * ---------------------------------------------------------------------------
 * * init_powertab: initialize power hash tables.
 */

void init_powertab(void)
{
    POWERENT *fp;
    char *nbuf, *np, *bp;

    hashinit(&mudstate.powers_htab, 15 * HASH_FACTOR);
    nbuf = alloc_sbuf("init_powertab");
    for (fp = gen_powers; fp->powername; fp++) {
	for (np = nbuf, bp = (char *) fp->powername; *bp; np++, bp++)
	    *np = ToLower(*bp);
	*np = '\0';
	hashadd(nbuf, (int *) fp, &mudstate.powers_htab);
    }
    free_sbuf(nbuf);
}

/*
 * ---------------------------------------------------------------------------
 * * display_powers: display available powers.
 */

void display_powertab(player)
dbref player;
{
    char *buf, *bp;
    POWERENT *fp;

    bp = buf = alloc_lbuf("display_powertab");
    safe_str((char *) "Powers:", buf, &bp);
    for (fp = gen_powers; fp->powername; fp++) {
	if ((fp->listperm & CA_WIZARD) && !Wizard(player))
	    continue;
	if ((fp->listperm & CA_GOD) && !God(player))
	    continue;
	safe_chr(' ', buf, &bp);
	safe_str((char *) fp->powername, buf, &bp);
    }
    *bp = '\0';
    notify(player, buf);
    free_lbuf(buf);
}

POWERENT *find_power(thing, powername)
dbref thing;
char *powername;
{
    char *cp;

    /*
     * Make sure the power name is valid 
     */

    for (cp = powername; *cp; cp++)
	*cp = ToLower(*cp);
    return (POWERENT *) hashfind(powername, &mudstate.powers_htab);
}

int decode_power(player, powername, pset)
dbref player;
char *powername;
POWERSET *pset;
{
    POWERENT *pent;

    pset->word1 = 0;
    pset->word2 = 0;

    pent = (POWERENT *) hashfind(powername, &mudstate.powers_htab);
    if (!pent) {
	notify(player, tprintf("%s: Power not found.", powername));
	return 0;
    }
    if (pent->powerpower & POWER_EXT)
	pset->word2 = pent->powervalue;
    else
	pset->word1 = pent->powervalue;

    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * * power_set: Set or clear a specified power on an object. 
 */

void power_set(target, player, power, key)
dbref target, player;
char *power;
int key;
{
    POWERENT *fp;
    int negate, result;

    /*
     * Trim spaces, and handle the negation character 
     */

    negate = 0;
    while (*power && isspace(*power))
	power++;
    if (*power == '!') {
	negate = 1;
	power++;
    }
    while (*power && isspace(*power))
	power++;

    /*
     * Make sure a power name was specified 
     */

    if (*power == '\0') {
	if (negate)
	    notify(player, "You must specify a power to clear.");
	else
	    notify(player, "You must specify a power to set.");
	return;
    }
    fp = find_power(target, power);
    if (fp == NULL) {
	notify(player, "I don't understand that power.");
	return;
    }
    /*
     * Invoke the power handler, and print feedback 
     */

    result =
	fp->handler(target, player, fp->powervalue, fp->powerpower,
	negate);
    if (!result)
	notify(player, "Permission denied.");
    else if (!(key & SET_QUIET) && !Quiet(player))
	notify(player, (negate ? "Cleared." : "Set."));
    return;
}


/*
 * ---------------------------------------------------------------------------
 * * has_power: does object have power visible to player?
 */

int has_power(player, it, powername)
dbref player, it;
char *powername;
{
    POWERENT *fp;
    POWER fv;

    fp = find_power(it, powername);
    if (fp == NULL)
	return 0;

    if (fp->powerpower & POWER_EXT)
	fv = Powers2(it);
    else
	fv = Powers(it);

    if (fv & fp->powervalue) {
	if ((fp->listperm & CA_WIZARD) && !Wizard(player))
	    return 0;
	if ((fp->listperm & CA_GOD) && !God(player))
	    return 0;
	return 1;
    }
    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * * power_description: Return an mbuf containing the type and powers on thing.
 */

char *power_description(player, target)
dbref player, target;
{
    char *buff, *bp;
    POWERENT *fp;
    int otype;
    POWER fv;

    /*
     * Allocate the return buffer 
     */

    otype = Typeof(target);
    bp = buff = alloc_mbuf("power_description");

    /*
     * Store the header strings and object type 
     */

    safe_mb_str((char *) "Powers:", buff, &bp);

    for (fp = gen_powers; fp->powername; fp++) {
	if (fp->powerpower & POWER_EXT)
	    fv = Powers2(target);
	else
	    fv = Powers(target);
	if (fv & fp->powervalue) {
	    if ((fp->listperm & CA_WIZARD) && !Wizard(player))
		continue;
	    if ((fp->listperm & CA_GOD) && !God(player))
		continue;
	    safe_mb_chr(' ', buff, &bp);
	    safe_mb_str((char *) fp->powername, buff, &bp);
	}
    }

    /*
     * Terminate the string, and return the buffer to the caller 
     */

    *bp = '\0';
    return buff;
}


/*
 * ---------------------------------------------------------------------------
 * * decompile_powers: Produce commands to set powers on target.
 */

void decompile_powers(player, thing, thingname)
dbref player, thing;
char *thingname;
{
    POWER f1, f2;
    POWERENT *fp;

    /*
     * Report generic powers 
     */

    f1 = Powers(thing);
    f2 = Powers2(thing);

    for (fp = gen_powers; fp->powername; fp++) {

	/*
	 * Skip if we shouldn't decompile this power 
	 */

	if (fp->listperm & CA_NO_DECOMP)
	    continue;

	/*
	 * Skip if this power is not set 
	 */

	if (fp->powerpower & POWER_EXT) {
	    if (!(f2 & fp->powervalue))
		continue;
	} else {
	    if (!(f1 & fp->powervalue))
		continue;
	}

	/*
	 * Skip if we can't see this power 
	 */

	if (!check_access(player, fp->listperm))
	    continue;

	/*
	 * We made it this far, report this power 
	 */

	notify(player, tprintf("@power %s=%s", strip_ansi(thingname),
		fp->powername));
    }
}

#endif				/*
				 * STANDALONE 
				 */
