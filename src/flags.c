/*
 * flags.c - flag manipulation routines 
 */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "mudconf.h"
#include "externs.h"
#include "command.h"
#include "flags.h"
#include "alloc.h"
#include "powers.h"

/**
 * Sets or clears the indicated bit, no security checking.
 * @param target The target object for setting/unsetting
 * @param player The object who is setting/unsetting
 * @param fflags ??
 * @param reset If 1, we're resetting the flag 
 */
int fh_any(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (fflags & FLAG_WORD3) {
	if (reset)
	    s_Flags3(target, Flags3(target) & ~flag);
	else
	    s_Flags3(target, Flags3(target) | flag);
    } else if (fflags & FLAG_WORD2) {
	if (reset)
	    s_Flags2(target, Flags2(target) & ~flag);
	else
	    s_Flags2(target, Flags2(target) | flag);
    } else {
	if (reset)
	    s_Flags(target, Flags(target) & ~flag);
	else
	    s_Flags(target, Flags(target) | flag);
    }
    return 1;
} /* end fh_and() */

/**
 * Function to block out non-GOD for setting or clearing a bit.
 * @param target Target object for setting/unsetting
 * @param player The object that is setting/unsetting
 * @param flag The flag to be manipulated
 * @param fflags ??
 * @param reset If 1, we're resetting the flag 
 */
int fh_god(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!God(player))
	return 0;

    return (fh_any(target, player, flag, fflags, reset));
} /* end fh_god() */

/**
 * Blocks out non-WIZARDS setting or clearing a bit.
 * @param target Target object for setting/unsetting
 * @param player The object that is setting/unsetting
 * @param flag The flag to be manipulated
 * @param fflags ??
 * @param reset If 1, we're resetting the flag
 */
int fh_wiz(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!Wizard(player) && !God(player))
	return 0;

    return (fh_any(target, player, flag, fflags, reset));
} /* end fh_wiz() */

/**
 * Only allows the bit to be set on players by WIZARDS.
 * @param target Target object for setting/unsetting
 * @param player The object that is setting/unsetting
 * @param flag The flag to be manipulated
 * @param fflags ??
 * @param reset If 1, we're resetting the flag
 */
int fh_fixed(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (isPlayer(target))
	if (!Wizard(player) && !God(player))
	    return 0;

    return (fh_any(target, player, flag, fflags, reset));
} /* end fh_fixed() */

/**
 * Only allows WIZARDS, ROYALTY, (or GOD) to set or clear the bit.
 * @param target Target object for setting/unsetting
 * @param player The object that is setting/unsetting
 * @param flag The flag to be manipulated
 * @param fflags ??
 * @param reset If 1, we're resetting the flag
 */

int fh_wizroy(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!WizRoy(player) && !God(player))
	return 0;

    return (fh_any(target, player, flag, fflags, reset));
} /* end fh_wizroy() */

/**
 * Only allows players to set or clear this bit.
 * @param target Target object for setting/unsetting
 * @param player The object that is setting/unsetting
 * @param flag The flag to be manipulated
 * @param fflags ??
 * @param reset If 1, we're resetting the flag
 */
int fh_inherit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!Inherits(player))
	return 0;

    return (fh_any(target, player, flag, fflags, reset));
} /* end fh_inherit() */

/**
 * Only allows GOD to set/clear this bit. Used for WIZARD flag.
 * @param target Target object for setting/unsetting
 * @param player The object that is setting/unsetting
 * @param flag The flag to be manipulated
 * @param fflags ??
 * @param reset If 1, we're resetting the flag
 */
int fh_wiz_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!God(player))
	return 0;
    if (God(target) && reset) {
	notify(player, "You cannot make yourself mortal.");
	return 0;
    }

    return (fh_any(target, player, flag, fflags, reset));
} /* end fh_wiz_bit() */

/**
 * Manipulates the dark bit. Non-Wizards may not set on players.
 * @param target Target object for setting/unsetting
 * @param player The object that is setting/unsetting
 * @param flag The flag to be manipulated
 * @param fflags ??
 * @param reset If 1, we're resetting the flag
 */
int fh_dark_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (!reset && isPlayer(target) && !((target == player) &&
	    Can_Hide(player)) && (!Wizard(player) && !God(player)))
	return 0;

    return (fh_any(target, player, flag, fflags, reset));
} /* end fh_dark_bit() */

/**
 * Manipulates the going bit.
 * @param target Target object for setting/unsetting
 * @param player The object that is setting/unsetting
 * @param flag The flag to be manipulated
 * @param fflags ??
 * @param reset If 1, we're resetting the flag
 */
int fh_going_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    if (Going(target) && reset && (Typeof(target) != TYPE_GARBAGE)) {
	notify(player, "Your object has been spared from destruction.");
	return (fh_any(target, player, flag, fflags, reset));
    }

    if (!God(player))
	return 0;

    return (fh_any(target, player, flag, fflags, reset));
} /* end fh_going_bit() */

/**
 * Sets or clears bits that affect hearing.
 * @param target Target object for setting/unsetting
 * @param player The object that is setting/unsetting
 * @param flag The flag to be manipulated
 * @param fflags ??
 * @param reset If 1, we're resetting the flag
 */
int fh_hear_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    int could_hear;

    if (isPlayer(target) && (flag & MONITOR)) {
	if (Can_Monitor(player))
	    fh_any(target, player, flag, fflags, reset);
	else
	    return 0;
    }

    could_hear = Hearer(target);
    fh_any(target, player, flag, fflags, reset);
    handle_ears(target, could_hear, Hearer(target));

    return 1;
} /* end fh_hear_bit() */


/**
 * Sets or clears bits that affect xcode in glue.h.
 * @param target Target object for setting/unsetting
 * @param player The object that is setting/unsetting
 * @param flag The flag to be manipulated
 * @param fflags ??
 * @param reset If 1, we're resetting the flag
 */
int fh_xcode_bit(dbref target, dbref player, FLAG flag, int fflags, int reset)
{
    int got_xcode;
    int new_xcode;

    got_xcode = Hardcode(target);
    fh_wiz(target, player, flag, fflags, reset);
    new_xcode = Hardcode(target);
    handle_xcode(player, target, got_xcode, new_xcode);

    return 1;
} /* end fh_xcode_bit() */

/** 
 * Alphabetized flag listing 
 * 0 = Flag's visible name
 * 1 = Flag's bit representation
 * 2 = Flag's letter alias
 * 3 = Flag's wordspace
 * 4 = Who may see the flag (0 = all) 
 * 5 = Permissions
 */
FLAGENT gen_flags[] = { 
{"ABODE",		ABODE,		'A',
	FLAG_WORD2,	0,			fh_any},
{"ANSI",                ANSI,           'X',   
        FLAG_WORD2,     0,                      fh_any},
{"ANSIMAP",             ANSIMAP,        'P',   
        FLAG_WORD2,     0,                      fh_any},
{"AUDIBLE",		HEARTHRU,	'a',
	0,		0,			fh_hear_bit},
{"AUDITORIUM",		AUDITORIUM,	'b',
	FLAG_WORD2,	0,			fh_any},
{"COMPRESS",		COMPRESS,	'.',
	FLAG_WORD2, 	0,			fh_any},
{"CONNECTED",		CONNECTED,	'c',
	FLAG_WORD2,	CA_NO_DECOMP,		fh_god},
{"CHOWN_OK",		CHOWN_OK,	'C',
	0,		0,			fh_any},
{"DARK",		DARK,		'D',
	0,		0,			fh_dark_bit},
{"DESTROY_OK",		DESTROY_OK,	'd',
	0,		0,			fh_any},
{"ENTER_OK",		ENTER_OK,	'e',
	0,		0,			fh_any},
{"FIXED",               FIXED,          'f',
        FLAG_WORD2,     0,                      fh_fixed}, 
{"FLOATING",		FLOATING,	'F',
	FLAG_WORD2,	0,			fh_any},
{"GAGGED", 		GAGGED,		'j',
	FLAG_WORD2,	0,			fh_wiz},
{"GOING",		GOING,		'G',
	0,		CA_NO_DECOMP,		fh_going_bit},
{"HALTED",		HALT,		'h',
	0,		0,			fh_any},
{"HAS_DAILY",		HAS_DAILY,	'*',
	FLAG_WORD2,	CA_GOD|CA_NO_DECOMP,	fh_god},
{"HAS_FORWARDLIST",	HAS_FWDLIST,	'&',
	FLAG_WORD2,	CA_GOD|CA_NO_DECOMP,	fh_god},
{"HAS_HOURLY",		HAS_HOURLY,	'*',
	FLAG_WORD2,	CA_GOD|CA_NO_DECOMP,	fh_god},
{"HAS_LISTEN",		HAS_LISTEN,	'@',
	FLAG_WORD2,	CA_GOD|CA_NO_DECOMP,	fh_god},
{"HAS_STARTUP",		HAS_STARTUP,	'+',
	0,		CA_GOD|CA_NO_DECOMP,	fh_god},
{"HAVEN",		HAVEN,		'H',
	0,		0,			fh_any},
{"HEAD",                HEAD_FLAG,      '?',
        FLAG_WORD2,     0,                      fh_wiz},
{"HTML", 		HTML,           '(',
	FLAG_WORD2,     0,                      fh_any},
{"IMMORTAL",		IMMORTAL,	'i',
	0,		0,			fh_wiz},
{"IN_CHARACTER",        IN_CHARACTER,   '#',
        FLAG_WORD2,     0,                      fh_wiz},
{"INHERIT",		INHERIT,	'I',
	0,		0,			fh_inherit},
{"JUMP_OK",		JUMP_OK,	'J',
	0,		0,			fh_any},
{"KEY",			KEY,		'K',
	FLAG_WORD2,	0,			fh_any},
{"LIGHT",		LIGHT,		'l',
	FLAG_WORD2,	0,			fh_any},
{"LINK_OK",		LINK_OK,	'L',
	0,		0,			fh_any},
{"MONITOR",		MONITOR,	'M',
	0,		0,			fh_hear_bit},
{"MULTIOK",		MULTIOK,	'y',
	FLAG_WORD2,	CA_WIZARD,		fh_wiz},
{"MYOPIC",		MYOPIC,		'm',
	0,		0,			fh_any},
{"NOBLEED",             NOBLEED,         '-',
        FLAG_WORD2,     0,                      fh_any},
{"NO_COMMAND",          NO_COMMAND,      'n',
        FLAG_WORD2,     0,                      fh_any},
{"NOSPOOF",		NOSPOOF,	'N',
	0,		0,			fh_any},
{"OPAQUE",		OPAQUE,		'O',
	0,		0,			fh_any},
{"PARENT_OK",		PARENT_OK,	'Y',
	FLAG_WORD2,	0,			fh_any},
{"PLAYER_MAILS",	PLAYER_MAILS,	'B',
	FLAG_WORD2,	CA_GOD|CA_NO_DECOMP,	fh_god},
{"PUPPET",		PUPPET,		'p',
	0,		0,			fh_hear_bit},
{"QUIET",		QUIET,		'Q',
	0,		0,			fh_any},
{"ROBOT",		ROBOT,		'r',
	0,		0,			fh_any},
{"ROYALTY",             ROYALTY,        'Z',    
        0,	        0,                      fh_wiz},
{"SAFE",		SAFE,		's',
	0,		0,			fh_any},
{"SLAVE",		SLAVE,		'x',
	FLAG_WORD2,	CA_WIZARD,		fh_wiz},
{"STAFF",		STAFF,		'w',
	FLAG_WORD2,     0,			fh_wiz},
{"STICKY",		STICKY,		'S',
	0,		0,			fh_wiz},
{"SUSPECT",		SUSPECT,	'u',
	FLAG_WORD2,	CA_WIZARD,		fh_wiz},
{"TERSE",		TERSE,		'q',
	0,		0,			fh_any},
{"TRACE",		TRACE,		'T',
	0,		0,			fh_any},
{"TRANSPARENT",		SEETHRU,	't',
	0,		0,			fh_any},
{"UNFINDABLE",		UNFINDABLE,	'U',
	FLAG_WORD2,	0,			fh_any},
{"UNINSPECTED",         UNINSPECTED,     'g',
        FLAG_WORD2,     0,                      fh_wizroy},
{"VERBOSE",		VERBOSE,	'v',
	0,		0,			fh_any},
{"VISUAL",		VISUAL,		'V',
	0,		0,			fh_any},
{"VACATION",		VACATION,	'|',
	FLAG_WORD2,	0,			fh_fixed},
{"WIZARD",		WIZARD,		'W',
	0,		0,			fh_wiz_bit},
{"XCODE",               HARDCODE,       'X',
        FLAG_WORD2,     0,                      fh_xcode_bit},
{"ZOMBIE",		ZOMBIE,		'z',
	FLAG_WORD2,	CA_WIZARD,		fh_wiz},
{ NULL,			0,		' ',
	0,		0,			NULL}};

/** 
 * Listing of valid object types 
 */
OBJENT object_types[8] = {
{"ROOM",	'R', CA_PUBLIC,	OF_CONTENTS|OF_EXITS|OF_DROPTO|OF_HOME},
{"THING",	' ', CA_PUBLIC,
	OF_CONTENTS|OF_LOCATION|OF_EXITS|OF_HOME|OF_SIBLINGS},
{"EXIT",	'E', CA_PUBLIC,	OF_SIBLINGS},
{"PLAYER",	'P', CA_PUBLIC,
	OF_CONTENTS|OF_LOCATION|OF_EXITS|OF_HOME|OF_OWNER|OF_SIBLINGS},
{"TYPE5",	'+', CA_GOD,	0},
{"GARBAGE",	'-', CA_PUBLIC,
	OF_CONTENTS|OF_LOCATION|OF_EXITS|OF_HOME|OF_SIBLINGS},
{"GARBAGE",	'#', CA_GOD,	0}};


/**
 * Initializes flag hash tables.
 */
void init_flagtab(void)
{
    FLAGENT *fp;
    char *nbuf, *np, *bp;

    hashinit(&mudstate.flags_htab, 100 * HASH_FACTOR);
    nbuf = alloc_sbuf("init_flagtab");

    for (fp = gen_flags; fp->flagname; fp++) {
	for (np = nbuf, bp = (char *) fp->flagname; *bp; np++, bp++)
	    *np = ToLower(*bp);
	*np = '\0';
	hashadd(nbuf, (int *) fp, &mudstate.flags_htab);
    }

    free_sbuf(nbuf);
} /* end init_flagtab() */

/**
 * Displays available flags. Used in @list flags.
 */
void display_flagtab(dbref player)
{
    char *buf, *bp;
    FLAGENT *fp;

    bp = buf = alloc_lbuf("display_flagtab");
    safe_str((char *) "Flags:", buf, &bp);

    for (fp = gen_flags; fp->flagname; fp++) {
	if ((fp->listperm & CA_WIZARD) && !Wizard(player))
	    continue;
	if ((fp->listperm & CA_GOD) && !God(player))
	    continue;
	safe_chr(' ', buf, &bp);
	safe_str((char *) fp->flagname, buf, &bp);
	safe_chr('(', buf, &bp);
	safe_chr(fp->flaglett, buf, &bp);
	safe_chr(')', buf, &bp);
    }

    *bp = '\0';
    notify(player, buf);
    free_lbuf(buf);
} /* end display_flagtab() */

/**
 * ??
 */
FLAGENT *find_flag(dbref thing, char *flagname)
{
    char *cp;

    /* Make sure the flag name is valid */
    for (cp = flagname; *cp; cp++)
	*cp = ToLower(*cp);

    return (FLAGENT *) hashfind(flagname, &mudstate.flags_htab);
} /* end find_flag() */

/**
 * Sets or clears a specified flag on an object. 
 * @param target Target object
 * @param player The object doing the setting
 * @paran flag The flag being set/unset
 * @param key Are we @set/quiet'in?
 */
void flag_set(dbref target, dbref player, char *flag, int key)
{
    FLAGENT *fp;
    int negate, result;

    /*
     * Trim spaces, and handle the negation character 
     */

    negate = 0;
    while (*flag && isspace(*flag))
	flag++;
    if (*flag == '!') {
	negate = 1;
	flag++;
    }
    while (*flag && isspace(*flag))
	flag++;

    /*
     * Make sure a flag name was specified 
     */

    if (*flag == '\0') {
	if (negate)
	    notify(player, "You must specify a flag to clear.");
	else
	    notify(player, "You must specify a flag to set.");
	return;
    }
    fp = find_flag(target, flag);
    if (fp == NULL) {
	notify(player, "I don't understand that flag.");
	return;
    }
    /*
     * Invoke the flag handler, and print feedback 
     */

    result =
	fp->handler(target, player, fp->flagvalue, fp->flagflag, negate);
    if (!result)
	notify(player, "Permission denied.");
    else if (!(key & SET_QUIET) && !Quiet(player))
	notify(player, (negate ? "Cleared." : "Set."));
    return;
} /* end flag_set() */

/**
 * Converts a flags word into corresponding letters.
 * @param player The invoking object
 * @param flagword ??
 * @param flag2word ??
 * @param flag3word ??
 */
char *decode_flags(dbref player, FLAG flagword, FLAG flag2word, FLAG flag3word)
{
    char *buf, *bp;
    FLAGENT *fp;
    int flagtype;
    FLAG fv;

    buf = bp = alloc_sbuf("decode_flags");
    *bp = '\0';

    if (!Good_obj(player)) {
	StringCopy(buf, "#-2 ERROR");
	return buf;
    }

    flagtype = (flagword & TYPE_MASK);
    if (object_types[flagtype].lett != ' ')
	safe_sb_chr(object_types[flagtype].lett, buf, &bp);

    for (fp = gen_flags; fp->flagname; fp++) {
	if (fp->flagflag & FLAG_WORD3)
	    fv = flag3word;
	else if (fp->flagflag & FLAG_WORD2)
	    fv = flag2word;
	else
	    fv = flagword;
	if (fv & fp->flagvalue) {
	    if ((fp->listperm & CA_WIZARD) && !Wizard(player))
		continue;
	    if ((fp->listperm & CA_GOD) && !God(player))
		continue;
	    /*
	     * don't show CONNECT on dark wizards to mortals 
	     */
	    if ((flagtype == TYPE_PLAYER) && (fp->flagvalue == CONNECTED)
		&& ((flagword & (WIZARD | DARK)) == (WIZARD | DARK)) &&
		!Wizard(player))
		continue;
	    safe_sb_chr(fp->flaglett, buf, &bp);
	}
    }

    *bp = '\0';
    return buf;
} /* end decode_flags() */

/**
 * Does object have flag visible to player?
 * @param player The player we're looking for
 * @param target The object with the flag
 * @param flagname The flag in question
 */
int has_flag(dbref player, dbref target, char *flagname)
{
    FLAGENT *fp;
    FLAG fv;

    fp = find_flag(target, flagname);
    if (fp == NULL)
	return 0;

    if (fp->flagflag & FLAG_WORD3)
	fv = Flags3(target);
    else if (fp->flagflag & FLAG_WORD2)
	fv = Flags2(target);
    else
	fv = Flags(target);

    if (fv & fp->flagvalue) {
	if ((fp->listperm & CA_WIZARD) && !Wizard(player))
	    return 0;
	if ((fp->listperm & CA_GOD) && !God(player))
	    return 0;
	/*
	 * don't show CONNECT on dark wizards to mortals 
	 */
	if (isPlayer(target) && (fp->flagvalue == CONNECTED) &&
	    ((Flags(target) & (WIZARD | DARK)) == (WIZARD | DARK)) &&
	    !Wizard(player))
	    return 0;
	return 1;
    }
    return 0;
} /* end has_flag() */

/**
 * Returns an mbuf containing the type and flags on thing.
 * @param player The player to send to
 * @param target The object whose flags we're checking
 */
char *flag_description(dbref player, dbref target)
{
    char *buff, *bp;
    FLAGENT *fp;
    int otype;
    FLAG fv;

    /*
     * Allocate the return buffer 
     */

    otype = Typeof(target);
    bp = buff = alloc_mbuf("flag_description");

    /*
     * Store the header strings and object type 
     */

    safe_mb_str((char *) "Type: ", buff, &bp);
    safe_mb_str((char *) object_types[otype].name, buff, &bp);
    safe_mb_str((char *) " Flags:", buff, &bp);
    if (object_types[otype].perm != CA_PUBLIC) {
	*bp = '\0';
	return buff;
    }
    /*
     * Store the type-invariant flags 
     */

    for (fp = gen_flags; fp->flagname; fp++) {
	if (fp->flagflag & FLAG_WORD3)
	    fv = Flags3(target);
	else if (fp->flagflag & FLAG_WORD2)
	    fv = Flags2(target);
	else
	    fv = Flags(target);
	if (fv & fp->flagvalue) {
	    if ((fp->listperm & CA_WIZARD) && !Wizard(player))
		continue;
	    if ((fp->listperm & CA_GOD) && !God(player))
		continue;
	    /*
	     * don't show CONNECT on dark wizards to mortals 
	     */
	    if (isPlayer(target) && (fp->flagvalue == CONNECTED) &&
		((Flags(target) & (WIZARD | DARK)) == (WIZARD | DARK)) &&
		!Wizard(player))
		continue;
	    safe_mb_chr(' ', buff, &bp);
	    safe_mb_str((char *) fp->flagname, buff, &bp);
	}
    }

    /*
     * Terminate the string, and return the buffer to the caller 
     */

    *bp = '\0';
    return buff;
} /* end flag_description() */

/**
 * Returns an lbuf containing the name and number of an object.
 * @param target The target object
 */
char *unparse_object_numonly(dbref target)
{
    char *buf;

    buf = alloc_lbuf("unparse_object_numonly");
    if (target == NOTHING) {
	StringCopy(buf, "*NOTHING*");
    } else if (target == HOME) {
	StringCopy(buf, "*HOME*");
    } else if (!Good_obj(target)) {
	sprintf(buf, "*ILLEGAL*(#%d)", target);
    } else {
	sprintf(buf, "%s(#%d)", Name(target), target);
    }
    return buf;
} /* end unparse_object_numonly() */

/**
 * Returns an lbuf pointing to the object name and possibly the db# and flags.
 */
char *unparse_object(dbref player, dbref target, int obey_myopic)
{
    char *buf, *fp;
    int exam;

    buf = alloc_lbuf("unparse_object");
    if (target == NOTHING) {
	StringCopy(buf, "*NOTHING*");
    } else if (target == HOME) {
	StringCopy(buf, "*HOME*");
    } else if (!Good_obj(target)) {
	sprintf(buf, "*ILLEGAL*(#%d)", target);
    } else {
	if (obey_myopic)
	    exam = MyopicExam(player, target);
	else
	    exam = Examinable(player, target);
	if (exam ||
	    (Flags(target) & (CHOWN_OK | JUMP_OK | LINK_OK | DESTROY_OK))
	    || (Flags2(target) & ABODE)) {

	    /*
	     * show everything 
	     */
	    fp = unparse_flags(player, target);
	    sprintf(buf, "%s(#%d%s%s)", Name(target), target,
		*fp ? ":" : "", fp);
	    free_sbuf(fp);
	} else {
	    /*
	     * show only the name. 
	     */
	    StringCopy(buf, Name(target));
	}
    }
    return buf;
} /* end unparse_object() */

/**
 * Converts a list of flag letters into its bit pattern.
 * Also set the type qualifier if specified and not already set.
 * @param player The evoking object
 * @param flaglist The list of flags to conver to bit pattern
 * @param fset ??
 * @param p_type ?? 
 */
int convert_flags(dbref player, char *flaglist, FLAGSET *fset, FLAG *p_type)
{
    int i, handled;
    char *s;
    FLAG flag1mask, flag2mask, flag3mask, type;
    FLAGENT *fp;

    flag1mask = flag2mask = flag3mask = 0;
    type = NOTYPE;

    for (s = flaglist; *s; s++) {
	handled = 0;

	/*
	 * Check for object type 
	 */

	for (i = 0; (i <= 7) && !handled; i++) {
	    if ((object_types[i].lett == *s) &&
		!(((object_types[i].perm & CA_WIZARD) && !Wizard(player))
		    || ((object_types[i].perm & CA_GOD) && !God(player)))) {
		if ((type != NOTYPE) && (type != i)) {
		    notify(player,
			tprintf("%c: Conflicting type specifications.",
			    *s));
		    return 0;
		}
		type = i;
		handled = 1;
	    }
	}

	/*
	 * Check generic flags 
	 */

	if (handled)
	    continue;
	for (fp = gen_flags; (fp->flagname) && !handled; fp++) {
	    if ((fp->flaglett == *s) && !(((fp->listperm & CA_WIZARD) &&
			!Wizard(player)) || ((fp->listperm & CA_GOD) &&
			!God(player)))) {
		if (fp->flagflag & FLAG_WORD3)
		    flag3mask |= fp->flagvalue;
		else if (fp->flagflag & FLAG_WORD2)
		    flag2mask |= fp->flagvalue;
		else
		    flag1mask |= fp->flagvalue;
		handled = 1;
	    }
	}

	if (!handled) {
	    notify(player,
		tprintf
		("%c: Flag unknown or not valid for specified object type",
		    *s));
	    return 0;
	}
    }

    /*
     * return flags to search for and type 
     */

    (*fset).word1 = flag1mask;
    (*fset).word2 = flag2mask;
    (*fset).word3 = flag3mask;
    *p_type = type;
    return 1;
} /* end convert_flags() */

/**
 * Produces commands to set flags on target.
 * @param player The evoking object
 * @param thing The target object
 * @param thingname ??
 */
void decompile_flags(dbref player, dbref thing, char *thingname)
{
    FLAG f1, f2, f3;
    FLAGENT *fp;

    /*
     * Report generic flags 
     */

    f1 = Flags(thing);
    f2 = Flags2(thing);
    f3 = Flags3(thing);

    for (fp = gen_flags; fp->flagname; fp++) {

	/*
	 * Skip if we shouldn't decompile this flag 
	 */

	if (fp->listperm & CA_NO_DECOMP)
	    continue;

	/*
	 * Skip if this flag is not set 
	 */

	if (fp->flagflag & FLAG_WORD3) {
	    if (!(f3 & fp->flagvalue))
		continue;
	} else if (fp->flagflag & FLAG_WORD2) {
	    if (!(f2 & fp->flagvalue))
		continue;
	} else {
	    if (!(f1 & fp->flagvalue))
		continue;
	}

	/*
	 * Skip if we can't see this flag 
	 */

	if (!check_access(player, fp->listperm))
	    continue;

	/*
	 * We made it this far, report this flag 
	 */

	notify(player, tprintf("@set %s=%s", strip_ansi(thingname),
		fp->flagname));
    }
} /* end decompile_flags() */
