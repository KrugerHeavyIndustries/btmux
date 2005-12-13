
/*
 * functions.c - MUX function handlers 
 */

/*
 * $Id: functions.c,v 1.11 2005/07/01 14:01:02 av1-op Exp $ 
 */

#include "copyright.h"
#include "config.h"

#include <limits.h>
#include <math.h>

#include "mudconf.h"
#include "db.h"
#include "flags.h"
#include "powers.h"
#include "attrs.h"
#include "externs.h"
#include "match.h"
#include "command.h"
#include "functions.h"
#include "misc.h"
#include "alloc.h"
#include "ansi.h"
#include "comsys.h"
#include "interface.h"

UFUN *ufun_head;

extern NAMETAB indiv_attraccess_nametab[];

extern int game_lag(void);

extern void cf_log_notfound(dbref player, char *cmd,
	const char *thingname, char *thing);

/*
 * Function definitions from funceval.c 
 */

extern void fun_btdesignex();
extern void fun_btgetcharvalue();
extern void fun_btmakemechs();
extern void fun_btmapelev();
extern void fun_btmapterr();
extern void fun_btsetcharvalue();
extern void fun_btgetxcodevalue();
extern void fun_btmakepilotroll();
extern void fun_btsetxcodevalue();
extern void fun_btstores();
extern void fun_btunderrepair();
extern void fun_btdamages();
extern void fun_btcritstatus();
extern void fun_btarmorstatus();
extern void fun_btsetarmorstatus();
extern void fun_btweaponstatus();
extern void fun_btthreshold();
extern void fun_btdamagemech();
extern void fun_bttechstatus();
extern void fun_btpartmatch();
extern void fun_btpartname();
extern void fun_btloadmap();
extern void fun_btloadmech();
extern void fun_btmechfreqs();
extern void fun_btmapunits();
extern void fun_btgetxcodevalue_ref();
extern void fun_btarmorstatus_ref();
extern void fun_btcritstatus_ref();
extern void fun_btweaponstatus_ref();
extern void fun_btid2db();
extern void fun_bthexlos();
extern void fun_btlosm2m();
extern void fun_bthexemit();

extern void fun_btgetweight();
extern void fun_btpartweight();
extern void fun_btaddstores();
extern void fun_btremovestores();
extern void fun_bttechtime();
extern void fun_btcritslot();
extern void fun_btcritslot_ref();
extern void fun_btgetrange();
extern void fun_btsetmaxspeed();
extern void fun_btgetrealmaxspeed();
extern void fun_btgetbv();
extern void fun_btgetbv_ref();
extern void fun_bttechlist();
extern void fun_bttechlist_ref();
extern void fun_btpayload_ref();
extern void fun_btshowstatus_ref();
extern void fun_btshowwspecs_ref();
extern void fun_btshowcritstatus_ref();
extern void fun_btengrate();
extern void fun_btengrate_ref();
extern void fun_btweapstat();
extern void fun_btnumrepjobs();
extern void fun_btsetxy();
extern void fun_btmapemit();
extern void fun_btparttype();
#ifdef BT_ADVANCED_ECON
extern void fun_btgetpartcost();
extern void fun_btsetpartcost();
extern void fun_btfasabasecost_ref();
#endif
extern void fun_btunitfixable();
extern void fun_btlistblz();
extern void fun_bthexinblz();
extern void fun_btcharlist();
extern void fun_cobj();
extern void fun_config();
	
extern void fun_cwho();
extern void fun_clist();
extern void fun_cemit();
extern void fun_beep();
extern void fun_ansi();
extern void fun_zone();
extern void fun_frank2num();
extern void fun_fnum2rank();
#ifdef SIDE_EFFECT_FUNCTIONS
extern void fun_link();
extern void fun_tel();
extern void fun_pemit();
extern void fun_create();
extern void fun_set();
extern void fun_setlock();
#endif
extern void fun_last();
extern void fun_matchall();
extern void fun_ports();
extern void fun_mix();
extern void fun_foreach();
extern void fun_munge();
extern void fun_visible();
extern void fun_elements();
extern void fun_grab();
extern void fun_scramble();
extern void fun_shuffle();
extern void fun_sortby();
extern void fun_default();
extern void fun_edefault();
extern void fun_udefault();
extern void fun_findable();
extern void fun_isword();
extern void fun_hasattr();
extern void fun_hasattrp();
extern void fun_zwho();
extern void fun_zplayers();
extern void fun_inzone();
extern void fun_children();
extern void fun_encrypt();
extern void fun_decrypt();
extern void fun_objeval();
extern void fun_squish();
extern void fun_stripansi();
extern void fun_zfun();
extern void fun_columns();
extern void fun_playmem();
#ifdef USE_PYTHON
extern void fun_python();
extern void fun_pythoncall();
#endif
extern void fun_objmem();
extern void fun_orflags();
extern void fun_andflags();
extern void fun_strtrunc();
extern void fun_ifelse();
extern void fun_inc();
extern void fun_dec();
extern void fun_mail();
extern void fun_mailfrom();
extern void fun_die();
extern void fun_lit();
extern void fun_shl();
extern void fun_shr();
extern void fun_vadd();
extern void fun_vsub();
extern void fun_vmul();
extern void fun_vmag();
extern void fun_vunit();
extern void fun_vdim();
extern void fun_strcat();
extern void fun_grep();
extern void fun_grepi();
extern void fun_art();
extern void fun_alphamax();
extern void fun_alphamin();
extern void fun_valid();
extern void fun_hastype();
extern void fun_lparent();
extern void fun_empty();
extern void fun_push();
extern void fun_peek();
extern void fun_pop();
extern void fun_items();
extern void fun_lstack();
extern void fun_regmatch();
extern void fun_translate();

/*
 * This is for functions that take an optional delimiter character 
 */

#define varargs_preamble(xname,xnargs)					\
	if (!fn_range_check(xname, nfargs, xnargs-1, xnargs, buff, bufc))	\
		return;							\
	if (!delim_check(fargs, nfargs, xnargs, &sep, buff, bufc, 0,	\
		player, cause, cargs, ncargs))				\
		return;

#define evarargs_preamble(xname,xnargs)					\
	if (!fn_range_check(xname, nfargs, xnargs-1, xnargs, buff, bufc))	\
		return;							\
	if (!delim_check(fargs, nfargs, xnargs, &sep, buff, bufc, 1,	\
	    player, cause, cargs, ncargs))				\
		return;

#define mvarargs_preamble(xname,xminargs,xnargs)			\
	if (!fn_range_check(xname, nfargs, xminargs, xnargs, buff, bufc))	\
		return;							\
	if (!delim_check(fargs, nfargs, xnargs, &sep, buff, bufc, 0,		\
	    player, cause, cargs, ncargs))				\
		return;

/*
 * Trim off leading and trailing spaces if the separator char is a space 
 */

char *trim_space_sep(str, sep)
char *str, sep;
{
    char *p;

    if (sep != ' ')
	return str;
    while (*str && (*str == ' '))
	str++;
    for (p = str; *p; p++);
    for (p--; *p == ' ' && p > str; p--);
    p++;
    *p = '\0';
    return str;
}

/*
 * next_token: Point at start of next token in string 
 */

char *next_token(str, sep)
char *str, sep;
{
    while (*str && (*str != sep))
	str++;
    if (!*str)
	return NULL;
    str++;
    if (sep == ' ') {
	while (*str == sep)
	    str++;
    }
    return str;
}

/*
 * split_token: Get next token from string as null-term string.  String is
 * * destructively modified.
 */

char *split_token(sp, sep)
char **sp, sep;
{
    char *str, *save;

    save = str = *sp;
    if (!str) {
	*sp = NULL;
	return NULL;
    }
    while (*str && (*str != sep))
	str++;
    if (*str) {
	*str++ = '\0';
	if (sep == ' ') {
	    while (*str == sep)
		str++;
	}
    } else {
	str = NULL;
    }
    *sp = str;
    return save;
}

dbref match_thing(player, name)
dbref player;
char *name;
{
    init_match(player, name, NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    return (noisy_match_result());
}

/*
 * ---------------------------------------------------------------------------
 * * List management utilities.
 */

#define	ALPHANUM_LIST	1
#define	NUMERIC_LIST	2
#define	DBREF_LIST	3
#define	FLOAT_LIST	4

static int autodetect_list(ptrs, nitems)
char *ptrs[];
int nitems;
{
    int sort_type, i;
    char *p;

    sort_type = NUMERIC_LIST;
    for (i = 0; i < nitems; i++) {
	switch (sort_type) {
	case NUMERIC_LIST:
	    if (!is_number(ptrs[i])) {

		/*
		 * If non-numeric, switch to alphanum sort. * 
		 * 
		 * *  * *  * * Exception: if this is the
		 * first * element * * and * it is a good
		 * dbref, * switch to a * * dbref sort. *
		 * We're a * little looser than *  * the
		 * normal * 'good  * dbref' rules, any * *
		 * number following # * the #-sign is
		 * accepted.  
		 */

		if (i == 0) {
		    p = ptrs[i];
		    if (*p++ != NUMBER_TOKEN) {
			return ALPHANUM_LIST;
		    } else if (is_integer(p)) {
			sort_type = DBREF_LIST;
		    } else {
			return ALPHANUM_LIST;
		    }
		} else {
		    return ALPHANUM_LIST;
		}
	    } else if (index(ptrs[i], '.')) {
		sort_type = FLOAT_LIST;
	    }
	    break;
	case FLOAT_LIST:
	    if (!is_number(ptrs[i])) {
		sort_type = ALPHANUM_LIST;
		return ALPHANUM_LIST;
	    }
	    break;
	case DBREF_LIST:
	    p = ptrs[i];
	    if (*p++ != NUMBER_TOKEN)
		return ALPHANUM_LIST;
	    if (!is_integer(p))
		return ALPHANUM_LIST;
	    break;
	default:
	    return ALPHANUM_LIST;
	}
    }
    return sort_type;
}

static int get_list_type(fargs, nfargs, type_pos, ptrs, nitems)
char *fargs[], *ptrs[];
int nfargs, nitems, type_pos;
{
    if (nfargs >= type_pos) {
	switch (ToLower(*fargs[type_pos - 1])) {
	case 'd':
	    return DBREF_LIST;
	case 'n':
	    return NUMERIC_LIST;
	case 'f':
	    return FLOAT_LIST;
	case '\0':
	    return autodetect_list(ptrs, nitems);
	default:
	    return ALPHANUM_LIST;
	}
    }
    return autodetect_list(ptrs, nitems);
}

int list2arr(arr, maxlen, list, sep)
char *arr[], *list, sep;
int maxlen;
{
    char *p;
    int i;

    list = trim_space_sep(list, sep);
    p = split_token(&list, sep);
    for (i = 0; p && i < maxlen; i++, p = split_token(&list, sep)) {
	arr[i] = p;
    }
    return i;
}

void arr2list(arr, alen, list, bufc, sep)
char *arr[], **bufc, *list, sep;
int alen;
{
    int i;

    for (i = 0; i < alen; i++) {
	safe_str(arr[i], list, bufc);
	safe_chr(sep, list, bufc);
    }
    if (*bufc != list)
	(*bufc)--;
}

static int dbnum(dbr)
char *dbr;
{
    if ((strlen(dbr) < 2) && (*dbr != '#'))
	return 0;
    else
	return atoi(dbr + 1);
}

/*
 * ---------------------------------------------------------------------------
 * * nearby_or_control: Check if player is near or controls thing
 */

int nearby_or_control(player, thing)
dbref player, thing;
{
    if (!Good_obj(player) || !Good_obj(thing))
	return 0;
    if (Controls(player, thing))
	return 1;
    if (!nearby(player, thing))
	return 0;
    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * * fval: copy the floating point value into a buffer and make it presentable
 */

static void fval(buff, bufc, result)
char *buff, **bufc;
double result;
{
    char *p, *buf1;

    buf1 = *bufc;
    safe_tprintf_str(buff, bufc, "%.6f", result);	/*
							 * get double val * * 
							 * into buffer 
							 */
    **bufc = '\0';
    p = (char *) rindex(buf1, '0');
    if (p == NULL) {		/*
				 * remove useless trailing 0's 
				 */
	return;
    } else if (*(p + 1) == '\0') {
	while (*p == '0') {
	    *p-- = '\0';
	}
	*bufc = p + 1;
    }
    p = (char *) rindex(buf1, '.');	/*
					 * take care of dangling '.' 
					 */
    if ((p != NULL) && (*(p + 1) == '\0')) {
	*p = '\0';
	*bufc = p;
    }
}

/*
 * ---------------------------------------------------------------------------
 * * fn_range_check: Check # of args to a function with an optional argument
 * * for validity.
 */

int fn_range_check(fname, nfargs, minargs, maxargs, result, bufc)
const char *fname;
char *result, **bufc;
int nfargs, minargs, maxargs;
{
    if ((nfargs >= minargs) && (nfargs <= maxargs))
	return 1;

    if (maxargs == (minargs + 1))
	safe_tprintf_str(result, bufc,
	    "#-1 FUNCTION (%s) EXPECTS %d OR %d ARGUMENTS", fname, minargs,
	    maxargs);
    else
	safe_tprintf_str(result, bufc,
	    "#-1 FUNCTION (%s) EXPECTS BETWEEN %d AND %d ARGUMENTS", fname,
	    minargs, maxargs);
    return 0;
}

/*
 * ---------------------------------------------------------------------------
 * * delim_check: obtain delimiter
 */

int delim_check(fargs, nfargs, sep_arg, sep, buff, bufc, eval, player,
    cause, cargs, ncargs)
char *fargs[], *cargs[], *sep, *buff, **bufc;
int nfargs, ncargs, sep_arg, eval;
dbref player, cause;
{
    char *tstr, *bp, *str;
    int tlen;

    if (nfargs >= sep_arg) {
	tlen = strlen(fargs[sep_arg - 1]);
	if (tlen <= 1)
	    eval = 0;
	if (eval) {
	    tstr = bp = alloc_lbuf("delim_check");
	    str = fargs[sep_arg - 1];
	    exec(tstr, &bp, 0, player, cause, EV_EVAL | EV_FCHECK, &str,
		cargs, ncargs);
	    *bp = '\0';
	    tlen = strlen(tstr);
	    *sep = *tstr;
	    free_lbuf(tstr);
	}
	if (tlen == 0) {
	    *sep = ' ';
	} else if (tlen != 1) {
	    safe_str("#-1 SEPARATOR MUST BE ONE CHARACTER", buff, bufc);
	    return 0;
	} else if (!eval) {
	    *sep = *fargs[sep_arg - 1];
	}
    } else {
	*sep = ' ';
    }
    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_words: Returns number of words in a string.
 * * Added 1/28/91 Philip D. Wasson
 */

int countwords(str, sep)
char *str, sep;
{
    int n;

    str = trim_space_sep(str, sep);
    if (!*str)
	return 0;
    for (n = 0; str; str = next_token(str, sep), n++);
    return n;
}

static void fun_words(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char sep;

    if (nfargs == 0) {
	safe_str("0", buff, bufc);
	return;
    }
    varargs_preamble("WORDS", 2);
    safe_tprintf_str(buff, bufc, "%d", countwords(fargs[0], sep));
}

/*
 * fun_flags: Returns the flags on an object.
 * Because @switch is case-insensitive, not quite as useful as it could be.
 */

static void fun_flags(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;
    char *buff2;

    it = match_thing(player, fargs[0]);
    if ((it != NOTHING) && (mudconf.pub_flags || Examinable(player, it) ||
	    (it == cause))) {
	buff2 = unparse_flags(player, it);
	safe_str(buff2, buff, bufc);
	free_sbuf(buff2);
    } else
	safe_str("#-1", buff, bufc);
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_rand: Return a random number from 0 to arg1-1
 */

static void fun_rand(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int num;

    num = atoi(fargs[0]);
    if (num < 1)
	safe_str("0", buff, bufc);
    else
	safe_tprintf_str(buff, bufc, "%ld", (random() % num));
}

/*
 * ---------------------------------------------------------------------------
 * * fun_abs: Returns the absolute value of its argument.
 */

static void fun_abs(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    double num;

    num = atof(fargs[0]);
    if (num == 0.0) {
	safe_str("0", buff, bufc);
    } else if (num < 0.0) {
	fval(buff, bufc, -num);
    } else {
	fval(buff, bufc, num);
    }
}

/*
 * ---------------------------------------------------------------------------
 * * fun_sign: Returns -1, 0, or 1 based on the the sign of its argument.
 */

static void fun_sign(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    double num;

    num = atof(fargs[0]);
    if (num < 0)
	safe_str("-1", buff, bufc);
    else if (num > 0)
	safe_str("1", buff, bufc);
    else
	safe_str("0", buff, bufc);
}

extern char *silly_atr_get(dbref, int);

time_t mytime(dbref player)
{
    struct timeval tv;
    struct timezone tz;
    char *tzstr;
    int hours, dst;

    tzstr = silly_atr_get(player, A_TZ);
    if (sscanf(tzstr, "%d:%d", &hours, &dst) < 2)
	return mudstate.now;
    tz.tz_minuteswest = (0 - hours) * 60;
    tz.tz_dsttime = dst;
    gettimeofday(&tv, &tz);
    return tv.tv_sec;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_time: Returns nicely-formatted time.
 */

static void fun_time(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *temp;

    temp = (char *) ctime(&mudstate.now);
    temp[strlen(temp) - 1] = '\0';
    safe_str(temp, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_time: Seconds since 0:00 1/1/70
 */

static void fun_secs(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%d", mudstate.now);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_convsecs: converts seconds to time string, based off 0:00 1/1/70
 */

static void fun_convsecs(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *temp;
    time_t tt;

    tt = atol(fargs[0]);
    temp = (char *) ctime(&tt);
    temp[strlen(temp) - 1] = '\0';
    safe_str(temp, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_convtime: converts time string to seconds, based off 0:00 1/1/70
 * *    additional auxiliary function and table used to parse time string,
 * *    since no ANSI standard function are available to do this.
 */

static const char *monthtab[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char daystab[] = { 31, 29, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

/*
 * converts time string to a struct tm. Returns 1 on success, 0 on fail.
 * * Time string format is always 24 characters long, in format
 * * Ddd Mmm DD HH:MM:SS YYYY
 */

#define	get_substr(buf, p) { \
	p = (char *)index(buf, ' '); \
	if (p) { \
		*p++ = '\0'; \
		while (*p == ' ') p++; \
	} \
}

int do_convtime(str, ttm)
char *str;
struct tm *ttm;
{
    char *buf, *p, *q;
    int i;

    if (!str || !ttm)
	return 0;
    while (*str == ' ')
	str++;
    buf = p = alloc_sbuf("do_convtime");	/*
						 * make a temp copy of arg 
						 */
    safe_sb_str(str, buf, &p);
    *p = '\0';

    get_substr(buf, p);		/*
				 * day-of-week or month 
				 */
    if (!p || strlen(buf) != 3) {
	free_sbuf(buf);
	return 0;
    }
    for (i = 0; (i < 12) && string_compare(monthtab[i], p); i++);
    if (i == 12) {
	get_substr(p, q);	/*
				 * month 
				 */
	if (!q || strlen(p) != 3) {
	    free_sbuf(buf);
	    return 0;
	}
	for (i = 0; (i < 12) && string_compare(monthtab[i], p); i++);
	if (i == 12) {
	    free_sbuf(buf);
	    return 0;
	}
	p = q;
    }
    ttm->tm_mon = i;

    get_substr(p, q);		/*
				 * day of month 
				 */
    if (!q || (ttm->tm_mday = atoi(p)) < 1 || ttm->tm_mday > daystab[i]) {
	free_sbuf(buf);
	return 0;
    }
    p = (char *) index(q, ':');	/*
				 * hours 
				 */
    if (!p) {
	free_sbuf(buf);
	return 0;
    }
    *p++ = '\0';
    if ((ttm->tm_hour = atoi(q)) > 23 || ttm->tm_hour < 0) {
	free_sbuf(buf);
	return 0;
    }
    if (ttm->tm_hour == 0) {
	while (isspace(*q))
	    q++;
	if (*q != '0') {
	    free_sbuf(buf);
	    return 0;
	}
    }
    q = (char *) index(p, ':');	/*
				 * minutes 
				 */
    if (!q) {
	free_sbuf(buf);
	return 0;
    }
    *q++ = '\0';
    if ((ttm->tm_min = atoi(p)) > 59 || ttm->tm_min < 0) {
	free_sbuf(buf);
	return 0;
    }
    if (ttm->tm_min == 0) {
	while (isspace(*p))
	    p++;
	if (*p != '0') {
	    free_sbuf(buf);
	    return 0;
	}
    }
    get_substr(q, p);		/*
				 * seconds 
				 */
    if (!p || (ttm->tm_sec = atoi(q)) > 59 || ttm->tm_sec < 0) {
	free_sbuf(buf);
	return 0;
    }
    if (ttm->tm_sec == 0) {
	while (isspace(*q))
	    q++;
	if (*q != '0') {
	    free_sbuf(buf);
	    return 0;
	}
    }
    get_substr(p, q);		/*
				 * year 
				 */
    if ((ttm->tm_year = atoi(p)) == 0) {
	while (isspace(*p))
	    p++;
	if (*p != '0') {
	    free_sbuf(buf);
	    return 0;
	}
    }
    free_sbuf(buf);
    if (ttm->tm_year > 100)
	ttm->tm_year -= 1900;
    if (ttm->tm_year < 0) {
	return 0;
    }
#define LEAPYEAR_1900(yr) ((yr)%400==100||((yr)%100!=0&&(yr)%4==0))
    return (ttm->tm_mday != 29 || i != 1 || LEAPYEAR_1900(ttm->tm_year));
#undef LEAPYEAR_1900
}

static void fun_convtime(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    struct tm *ttm;

    ttm = localtime(&mudstate.now);
    if (do_convtime(fargs[0], ttm))
	safe_tprintf_str(buff, bufc, "%d", timelocal(ttm));
    else
	safe_str("-1", buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_convuptime: Converts number to minutes/secs/days
 */

#define UPTIME_UNITS 6

struct {
    int multip;
    char *name;
    char *sname;
} uptime_unit_table[UPTIME_UNITS] = {
    {
    60 *60 * 24 * 30 * 12, "year", "y"}, {
    60 *60 * 24 * 30, "month", "m"}, {
    60 *60 * 24, "day", "d"}, {
    60 *60, "hour", "h"}, {
    60, "minute", "m"}, {
    1, "second", "s"}
};

char *get_uptime_to_string(int uptime)
{
    char *buf = alloc_sbuf("get_uptime_to_string");
    int units[UPTIME_UNITS];
    int taim = uptime;
    int ut = 0, uc = 0, foofaa;

    if (uptime <= 0) {
	strcpy(buf, "#-1 INVALID VALUE");
	return buf;
    }
    for (ut = 0; ut < UPTIME_UNITS; ut++)
	units[ut] = 0;
    ut = 0;
    buf[0] = 0;
    while (taim > 0) {
	if ((foofaa = (taim / uptime_unit_table[ut].multip)) > 0) {
	    uc++;
	    units[ut] = foofaa;
	    taim -= uptime_unit_table[ut].multip * foofaa;
	}
	ut++;
    }
    /*
     * Now, we got it..
     */
    for (ut = 0; ut < UPTIME_UNITS; ut++) {
	if (units[ut]) {
	    uc--;
	    if (units[ut] > 1)
		sprintf(buf + strlen(buf), "%d %ss", units[ut],
		    uptime_unit_table[ut].name);
	    else
		sprintf(buf + strlen(buf), "%d %s", units[ut],
		    uptime_unit_table[ut].name);
	    if (uc > 1)
		strcat(buf, ", ");
	    else if (uc > 0)
		strcat(buf, " and ");
	}
    }
    return buf;
}

char *get_uptime_to_short_string(int uptime)
{
    char *buf = alloc_sbuf("get_uptime_to_string");
    int units[UPTIME_UNITS];
    int taim = uptime;
    int ut = 0, uc = 0, foofaa;

    if (uptime <= 0) {
	strcpy(buf, "#-1 INVALID VALUE");
	return buf;
    }
    for (ut = 0; ut < UPTIME_UNITS; ut++)
	units[ut] = 0;
    ut = 0;
    buf[0] = 0;
    while (taim > 0) {
	if ((foofaa = (taim / uptime_unit_table[ut].multip)) > 0) {
	    uc++;
	    units[ut] = foofaa;
	    taim -= uptime_unit_table[ut].multip * foofaa;
	}
	ut++;
    }
    /*
     * Now, we got it..
     */
    for (ut = 0; ut < UPTIME_UNITS; ut++) {
	if (units[ut]) {
	    uc--;
	    if (units[ut] > 1)
		sprintf(buf + strlen(buf), "%d%s", units[ut],
		    uptime_unit_table[ut].sname);
	    else
		sprintf(buf + strlen(buf), "%d%s", units[ut],
		    uptime_unit_table[ut].sname);
	    if (uc > 0)
		strcat(buf, " ");
	}
    }
    return buf;
}

static void fun_convuptime(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *uptimestring = get_uptime_to_string(atoi(fargs[0]));

    safe_str(uptimestring, buff, bufc);
    free_sbuf(uptimestring);
}


#ifdef BT_ENABLED
static void fun_lag(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char buf[256];

    sprintf(buf, "%d", game_lag());
    safe_str(buf, buff, bufc);
}
#endif

/*
 * ---------------------------------------------------------------------------
 * * fun_starttime: What time did this system last reboot?
 */

static void fun_starttime(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *temp;

    temp = (char *) ctime(&mudstate.start_time);
    temp[strlen(temp) - 1] = '\0';
    safe_str(temp, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_get, fun_get_eval: Get attribute from object.
 */

int check_read_perms(player, thing, attr, aowner, aflags, buff, bufc)
dbref player, thing;
ATTR *attr;
int aowner, aflags;
char *buff, **bufc;
{
    int see_it;

    /*
     * If we have explicit read permission to the attr, return it 
     */

    if (See_attr_explicit(player, thing, attr, aowner, aflags))
	return 1;

    /*
     * If we are nearby or have examine privs to the attr and it is * * * 
     * 
     * * visible to us, return it. 
     */

    see_it = See_attr(player, thing, attr, aowner, aflags);
    if ((Examinable(player, thing) || nearby(player, thing) ||
	    See_All(player)) && see_it)
	return 1;

    /*
     * For any object, we can read its visible attributes, EXCEPT * for * 
     * 
     * *  * * descs, which are only visible if read_rem_desc is on. 
     */

    if (see_it) {
	if (!mudconf.read_rem_desc && (attr->number == A_DESC)) {
	    safe_str("#-1 TOO FAR AWAY TO SEE", buff, bufc);
	    return 0;
	} else {
	    return 1;
	}
    }
    safe_str("#-1 PERMISSION DENIED", buff, bufc);
    return 0;
}

static void fun_get(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags;
    ATTR *attr;
    char *atr_gotten;
    struct boolexp *bool;

    if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
        safe_str("#-1 NO MATCH", buff, bufc);
        return;
    }
    if (attrib == NOTHING) {
        return;
    }
    free_buffer = 1;
    attr = atr_num(attrib);	/*
                             * We need the attr's flags for this: 
                             */
    if (!attr) {
        return;
    }
    if (attr->flags & AF_IS_LOCK) {
        atr_gotten = atr_get(thing, attrib, &aowner, &aflags);
        if (Read_attr(player, thing, attr, aowner, aflags)) {
            bool = parse_boolexp(player, atr_gotten, 1);
            free_lbuf(atr_gotten);
            atr_gotten = unparse_boolexp(player, bool);
            free_boolexp(bool);
        } else {
            free_lbuf(atr_gotten);
            atr_gotten = (char *) "#-1 PERMISSION DENIED";
        }
        free_buffer = 0;
    } else {
        atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
    }

    /*
     * Perform access checks.  c_r_p fills buff with an error message * * 
     * 
     * *  * * if needed. 
     */

    if (check_read_perms(player, thing, attr, aowner, aflags, buff, bufc))
        safe_str(atr_gotten, buff, bufc);
    if (free_buffer)
        free_lbuf(atr_gotten);
    return;
}

static void fun_xget(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags;
    ATTR *attr;
    char *atr_gotten;
    struct boolexp *bool;

    if (!*fargs[0] || !*fargs[1])
	return;

    if (!parse_attrib(player, tprintf("%s/%s", fargs[0], fargs[1]), &thing,
	    &attrib)) {
	safe_str("#-1 NO MATCH", buff, bufc);
	return;
    }
    if (attrib == NOTHING) {
	return;
    }
    free_buffer = 1;
    attr = atr_num(attrib);	/*
				 * We need the attr's flags for this: 
				 */
    if (!attr) {
	return;
    }
    if (attr->flags & AF_IS_LOCK) {
	atr_gotten = atr_get(thing, attrib, &aowner, &aflags);
	if (Read_attr(player, thing, attr, aowner, aflags)) {
	    bool = parse_boolexp(player, atr_gotten, 1);
	    free_lbuf(atr_gotten);
	    atr_gotten = unparse_boolexp(player, bool);
	    free_boolexp(bool);
	} else {
	    free_lbuf(atr_gotten);
	    atr_gotten = (char *) "#-1 PERMISSION DENIED";
	}
	free_buffer = 0;
    } else {
	atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
    }

    /*
     * Perform access checks.  c_r_p fills buff with an error message * * 
     * 
     * *  * * if needed. 
     */

    if (check_read_perms(player, thing, attr, aowner, aflags, buff, bufc))
	safe_str(atr_gotten, buff, bufc);
    if (free_buffer)
	free_lbuf(atr_gotten);
    return;
}

static void fun_get_eval(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags, eval_it;
    ATTR *attr;
    char *atr_gotten, *str;
    struct boolexp *bool;

    if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
	safe_str("#-1 NO MATCH", buff, bufc);
	return;
    }
    if (attrib == NOTHING) {
	return;
    }
    free_buffer = 1;
    eval_it = 1;
    attr = atr_num(attrib);	/*
				 * We need the attr's flags for this: 
				 */
    if (!attr) {
	return;
    }
    if (attr->flags & AF_IS_LOCK) {
	atr_gotten = atr_get(thing, attrib, &aowner, &aflags);
	if (Read_attr(player, thing, attr, aowner, aflags)) {
	    bool = parse_boolexp(player, atr_gotten, 1);
	    free_lbuf(atr_gotten);
	    atr_gotten = unparse_boolexp(player, bool);
	    free_boolexp(bool);
	} else {
	    free_lbuf(atr_gotten);
	    atr_gotten = (char *) "#-1 PERMISSION DENIED";
	}
	free_buffer = 0;
	eval_it = 0;
    } else {
	atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
    }
    if (!check_read_perms(player, thing, attr, aowner, aflags, buff, bufc)) {
	if (free_buffer)
	    free_lbuf(atr_gotten);
	return;
    }
    if (eval_it) {
	str = atr_gotten;
	exec(buff, bufc, 0, thing, player, EV_FIGNORE | EV_EVAL, &str,
	    (char **) NULL, 0);
    } else {
	safe_str(atr_gotten, buff, bufc);
    }
    if (free_buffer)
	free_lbuf(atr_gotten);
    return;
}

static void fun_subeval(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *str;

    if (nfargs != 1) {
	safe_str("#-1 FUNCTION (EVALNOCOMP) EXPECTS 1 OR 2 ARGUMENTS",
	    buff, bufc);
	return;
    }

    str = fargs[0];
    exec(buff, bufc, 0, player, cause,
	EV_NO_LOCATION | EV_NOFCHECK | EV_FIGNORE | EV_NO_COMPRESS, &str,
	(char **) NULL, 0);
}

static void fun_eval(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags, eval_it;
    ATTR *attr;
    char *atr_gotten, *str;
    struct boolexp *bool;

    if ((nfargs != 1) && (nfargs != 2)) {
	safe_str("#-1 FUNCTION (EVAL) EXPECTS 1 OR 2 ARGUMENTS", buff,
	    bufc);
	return;
    }
    if (nfargs == 1) {
	str = fargs[0];
	exec(buff, bufc, 0, player, cause, EV_EVAL, &str, (char **) NULL,
	    0);
	return;
    }
    if (!*fargs[0] || !*fargs[1])
	return;

    if (!parse_attrib(player, tprintf("%s/%s", fargs[0], fargs[1]), &thing,
	    &attrib)) {
	safe_str("#-1 NO MATCH", buff, bufc);
	return;
    }
    if (attrib == NOTHING) {
	return;
    }
    free_buffer = 1;
    eval_it = 1;
    attr = atr_num(attrib);
    if (!attr) {
	return;
    }
    if (attr->flags & AF_IS_LOCK) {
	atr_gotten = atr_get(thing, attrib, &aowner, &aflags);
	if (Read_attr(player, thing, attr, aowner, aflags)) {
	    bool = parse_boolexp(player, atr_gotten, 1);
	    free_lbuf(atr_gotten);
	    atr_gotten = unparse_boolexp(player, bool);
	    free_boolexp(bool);
	} else {
	    free_lbuf(atr_gotten);
	    atr_gotten = (char *) "#-1 PERMISSION DENIED";
	}
	free_buffer = 0;
	eval_it = 0;
    } else {
	atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
    }
    if (!check_read_perms(player, thing, attr, aowner, aflags, buff, bufc)) {
	if (free_buffer)
	    free_lbuf(atr_gotten);
	return;
    }
    if (eval_it) {
	str = atr_gotten;
	exec(buff, bufc, 0, thing, player, EV_FIGNORE | EV_EVAL, &str,
	    (char **) NULL, 0);
    } else {
	safe_str(atr_gotten, buff, bufc);
    }
    if (free_buffer)
	free_lbuf(atr_gotten);
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_u and fun_ulocal:  Call a user-defined function.
 */

static void do_ufun(buff, bufc, player, cause, fargs, nfargs, cargs,
    ncargs, is_local)
char *buff, **bufc;
dbref player, cause;
char *fargs[], *cargs[];
int nfargs, ncargs, is_local;
{
    dbref aowner, thing;
    int aflags, anum, i;
    ATTR *ap;
    char *atext, *preserve[MAX_GLOBAL_REGS], *str;

    /*
     * We need at least one argument 
     */

    if (nfargs < 1) {
	safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
	return;
    }
    /*
     * Two possibilities for the first arg: <obj>/<attr> and <attr>. 
     */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
	if ((anum == NOTHING) || (!Good_obj(thing)))
	    ap = NULL;
	else
	    ap = atr_num(anum);
    } else {
	thing = player;
	ap = atr_str(fargs[0]);
    }

    /*
     * Make sure we got a good attribute 
     */

    if (!ap) {
	return;
    }
    /*
     * Use it if we can access it, otherwise return an error. 
     */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
	free_lbuf(atext);
	return;
    }
    if (!*atext) {
	free_lbuf(atext);
	return;
    }
    if (!check_read_perms(player, thing, ap, aowner, aflags, buff, bufc)) {
	free_lbuf(atext);
	return;
    }
    /*
     * If we're evaluating locally, preserve the global registers. 
     */

    if (is_local) {
	for (i = 0; i < MAX_GLOBAL_REGS; i++) {
	    if (!mudstate.global_regs[i])
		preserve[i] = NULL;
	    else {
		preserve[i] = alloc_lbuf("u_regs");
		StringCopy(preserve[i], mudstate.global_regs[i]);
	    }
	}
    }
    /*
     * Evaluate it using the rest of the passed function args 
     */

    str = atext;
    exec(buff, bufc, 0, thing, cause, EV_FCHECK | EV_EVAL, &str,
	&(fargs[1]), nfargs - 1);
    free_lbuf(atext);

    /*
     * If we're evaluating locally, restore the preserved registers. 
     */

    if (is_local) {
	for (i = 0; i < MAX_GLOBAL_REGS; i++) {
	    if (preserve[i]) {
		if (!mudstate.global_regs[i])
		    mudstate.global_regs[i] = alloc_lbuf("u_reg");
		StringCopy(mudstate.global_regs[i], preserve[i]);
		free_lbuf(preserve[i]);
	    } else {
		if (mudstate.global_regs[i])
		    *(mudstate.global_regs[i]) = '\0';
	    }
	}
    }
}

static void fun_u(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    do_ufun(buff, bufc, player, cause, fargs, nfargs, cargs, ncargs, 0);
}

static void fun_ulocal(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    do_ufun(buff, bufc, player, cause, fargs, nfargs, cargs, ncargs, 1);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_parent: Get parent of object.
 */

static void fun_parent(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (Good_obj(it) && (Examinable(player, it) || (it == cause))) {
	safe_tprintf_str(buff, bufc, "#%d", Parent(it));
    } else {
	safe_str("#-1", buff, bufc);
    }
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_parse: Make list from evaluating arg3 with each member of arg2.
 * * arg1 specifies a delimiter character to use in the parsing of arg2.
 * * NOTE: This function expects that its arguments have not been evaluated.
 */

static void fun_parse(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *curr, *objstring, *buff2, *buff3, *cp, sep;
    char *dp, *str;
    int first, number = 0;

    evarargs_preamble("PARSE", 3);
    cp = curr = dp = alloc_lbuf("fun_parse");
    str = fargs[0];
    exec(curr, &dp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str,
	cargs, ncargs);
    *dp = '\0';
    cp = trim_space_sep(cp, sep);
    if (!*cp) {
	free_lbuf(curr);
	return;
    }
    first = 1;
    while (cp) {
	if (!first)
	    safe_chr(' ', buff, bufc);
	first = 0;
	number++;
	objstring = split_token(&cp, sep);
	buff2 = replace_string(BOUND_VAR, objstring, fargs[1]);
	buff3 =
	    replace_string(LISTPLACE_VAR, tprintf("%d", number), buff2);
	str = buff3;
	exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	    &str, cargs, ncargs);
	free_lbuf(buff2);
	free_lbuf(buff3);
    }
    free_lbuf(curr);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_mid: mid(foobar,2,3) returns oba
 */

static void fun_mid(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int l, len;
    char *oldp;

    oldp = *bufc;
    l = atoi(fargs[1]);
    len = atoi(fargs[2]);
    if ((l < 0) || (len < 0) || ((len + l) > LBUF_SIZE) || ((len + 1) < 0)) {
	safe_str("#-1 OUT OF RANGE", buff, bufc);
	return;
    }
    if (l < strlen(strip_ansi(fargs[0])))
	safe_str(strip_ansi(fargs[0]) + l, buff, bufc);
    oldp[len] = 0;
    if ((oldp + len) < *bufc) {
	*bufc = oldp + len;
    }
}

/*
 * ---------------------------------------------------------------------------
 * * fun_first: Returns first word in a string
 */

static void fun_first(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *s, *first, sep;

    /*
     * If we are passed an empty arglist return a null string 
     */

    if (nfargs == 0) {
	return;
    }
    varargs_preamble("FIRST", 2);
    s = trim_space_sep(fargs[0], sep);	/*
					 * leading spaces ... 
					 */
    first = split_token(&s, sep);
    if (first) {
	safe_str(first, buff, bufc);
    }
}

/*
 * ---------------------------------------------------------------------------
 * * fun_rest: Returns all but the first word in a string 
 */


static void fun_rest(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *s, *first, sep;

    /*
     * If we are passed an empty arglist return a null string 
     */

    if (nfargs == 0) {
	return;
    }
    varargs_preamble("REST", 2);
    s = trim_space_sep(fargs[0], sep);	/*
					 * leading spaces ... 
					 */
    first = split_token(&s, sep);
    if (s) {
	safe_str(s, buff, bufc);
    }
}

/*
 * ---------------------------------------------------------------------------
 * * fun_v: Function form of %-substitution
 */

static void fun_v(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner;
    int aflags;
    char *sbuf, *sbufc, *tbuf, *str;
    ATTR *ap;

    tbuf = fargs[0];
    if (isalpha(tbuf[0]) && tbuf[1]) {

	/*
	 * Fetch an attribute from me.  First see if it exists, * * * 
	 * 
	 * * returning a null string if it does not. 
	 */

	ap = atr_str(fargs[0]);
	if (!ap) {
	    return;
	}
	/*
	 * If we can access it, return it, otherwise return a * null
	 * * * * string 
	 */

	atr_pget_info(player, ap->number, &aowner, &aflags);
	if (See_attr(player, player, ap, aowner, aflags)) {
	    tbuf = atr_pget(player, ap->number, &aowner, &aflags);
	    safe_str(tbuf, buff, bufc);
	    free_lbuf(tbuf);
	}
	return;
    }
    /*
     * Not an attribute, process as %<arg> 
     */

    sbuf = alloc_sbuf("fun_v");
    sbufc = sbuf;
    safe_sb_chr('%', sbuf, &sbufc);
    safe_sb_str(fargs[0], sbuf, &sbufc);
    *sbufc = '\0';
    str = sbuf;
    exec(buff, bufc, 0, player, cause, EV_FIGNORE, &str, cargs, ncargs);
    free_sbuf(sbuf);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_s: Force substitution to occur.
 */

static void fun_s(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *str;

    str = fargs[0];
    exec(buff, bufc, 0, player, cause, EV_FIGNORE | EV_EVAL, &str, cargs,
	ncargs);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_con: Returns first item in contents list of object/room
 */

static void fun_con(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;

    it = match_thing(player, fargs[0]);

    if ((it != NOTHING) && (Has_contents(it)) && (Examinable(player, it) ||
	    (where_is(player) == it) || (it == cause))) {
	safe_tprintf_str(buff, bufc, "#%d", Contents(it));
	return;
    }
    safe_str("#-1", buff, bufc);
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_exit: Returns first exit in exits list of room.
 */

static void fun_exit(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it, exit;
    int key;

    it = match_thing(player, fargs[0]);
    if (Good_obj(it) && Has_exits(it) && Good_obj(Exits(it))) {
	key = 0;
	if (Examinable(player, it))
	    key |= VE_LOC_XAM;
	if (Dark(it))
	    key |= VE_LOC_DARK;
	DOLIST(exit, Exits(it)) {
	    if (exit_visible(exit, player, key)) {
		safe_tprintf_str(buff, bufc, "#%d", exit);
		return;
	    }
	}
    }
    safe_str("#-1", buff, bufc);
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_next: return next thing in contents or exits chain
 */

static void fun_next(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it, loc, exit, ex_here;
    int key;

    it = match_thing(player, fargs[0]);
    if (Good_obj(it) && Has_siblings(it)) {
	loc = where_is(it);
	ex_here = Good_obj(loc) ? Examinable(player, loc) : 0;
	if (ex_here || (loc == player) || (loc == where_is(player))) {
	    if (!isExit(it)) {
		safe_tprintf_str(buff, bufc, "#%d", Next(it));
		return;
	    } else {
		key = 0;
		if (ex_here)
		    key |= VE_LOC_XAM;
		if (Dark(loc))
		    key |= VE_LOC_DARK;
		DOLIST(exit, it) {
		    if ((exit != it) && exit_visible(exit, player, key)) {
			safe_tprintf_str(buff, bufc, "#%d", exit);
			return;
		    }
		}
	    }
	}
    }
    safe_str("#-1", buff, bufc);
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_loc: Returns the location of something
 */

static void fun_loc(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (locatable(player, it, cause))
	safe_tprintf_str(buff, bufc, "#%d", Location(it));
    else
	safe_str("#-1", buff, bufc);
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_where: Returns the "true" location of something
 */

static void fun_where(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (locatable(player, it, cause))
	safe_tprintf_str(buff, bufc, "#%d", where_is(it));
    else
	safe_str("#-1", buff, bufc);
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_rloc: Returns the recursed location of something (specifying #levels)
 */

static void fun_rloc(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i, levels;
    dbref it;

    levels = atoi(fargs[1]);
    if (levels > mudconf.ntfy_nest_lim)
	levels = mudconf.ntfy_nest_lim;

    it = match_thing(player, fargs[0]);
    if (locatable(player, it, cause)) {
	for (i = 0; i < levels; i++) {
	    if (!Good_obj(it) || !Has_location(it))
		break;
	    it = Location(it);
	}
	safe_tprintf_str(buff, bufc, "#%d", it);
	return;
    }
    safe_str("#-1", buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_room: Find the room an object is ultimately in.
 */

static void fun_room(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;
    int count;

    it = match_thing(player, fargs[0]);
    if (locatable(player, it, cause)) {
	for (count = mudconf.ntfy_nest_lim; count > 0; count--) {
	    it = Location(it);
	    if (!Good_obj(it))
		break;
	    if (isRoom(it)) {
		safe_tprintf_str(buff, bufc, "#%d", it);
		return;
	    }
	}
	safe_str("#-1", buff, bufc);
    } else if (isRoom(it)) {
	safe_tprintf_str(buff, bufc, "#%d", it);
    } else {
	safe_str("#-1", buff, bufc);
    }
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_owner: Return the owner of an object.
 */

static void fun_owner(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it, aowner;
    int atr, aflags;

    if (parse_attrib(player, fargs[0], &it, &atr)) {
	if (atr == NOTHING) {
	    it = NOTHING;
	} else {
	    atr_pget_info(it, atr, &aowner, &aflags);
	    it = aowner;
	}
    } else {
	it = match_thing(player, fargs[0]);
	if (it != NOTHING)
	    it = Owner(it);
    }
    safe_tprintf_str(buff, bufc, "#%d", it);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_controls: Does x control y?
 */

static void fun_controls(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref x, y;

    x = match_thing(player, fargs[0]);
    if (x == NOTHING) {
	safe_tprintf_str(buff, bufc, "%s", "#-1 ARG1 NOT FOUND");
	return;
    }
    y = match_thing(player, fargs[1]);
    if (y == NOTHING) {
	safe_tprintf_str(buff, bufc, "%s", "#-1 ARG2 NOT FOUND");
	return;
    }
    safe_tprintf_str(buff, bufc, "%d", Controls(x, y));
}

/*
 * ---------------------------------------------------------------------------
 * * fun_fullname: Return the fullname of an object (good for exits)
 */

static void fun_fullname(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (it == NOTHING) {
	return;
    }
    if (!mudconf.read_rem_name) {
	if (!nearby_or_control(player, it) && (!isPlayer(it))) {
	    safe_str("#-1 TOO FAR AWAY TO SEE", buff, bufc);
	    return;
	}
    }
    safe_str(Name(it), buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_name: Return the name of an object
 */

static void fun_name(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;
    char *s, *temp;

    it = match_thing(player, fargs[0]);
    if (it == NOTHING) {
	return;
    }
    if (!mudconf.read_rem_name) {
	if (!nearby_or_control(player, it) && !isPlayer(it) &&
	    !Long_Fingers(player)) {
	    safe_str("#-1 TOO FAR AWAY TO SEE", buff, bufc);
	    return;
	}
    }
    temp = *bufc;
    safe_str(Name(it), buff, bufc);
    if (isExit(it)) {
	for (s = temp; (s != *bufc) && (*s != ';'); s++);
	if (*s == ';')
	    *bufc = s;
    }
}

/*
 * ---------------------------------------------------------------------------
 * * fun_match, fun_strmatch: Match arg2 against each word of arg1 returning
 * * index of first match, or against the whole string.
 */

static void fun_match(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int wcount;
    char *r, *s, sep;

    varargs_preamble("MATCH", 3);

    /*
     * Check each word individually, returning the word number of the * * 
     * 
     * *  * * first one that matches.  If none match, return 0. 
     */

    wcount = 1;
    s = trim_space_sep(fargs[0], sep);
    do {
	r = split_token(&s, sep);
	if (quick_wild(fargs[1], r)) {
	    safe_tprintf_str(buff, bufc, "%d", wcount);
	    return;
	}
	wcount++;
    } while (s);
    safe_str("0", buff, bufc);
}

static void fun_strmatch(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    /*
     * Check if we match the whole string.  If so, return 1 
     */

    if (quick_wild(fargs[1], fargs[0]))
	safe_str("1", buff, bufc);
    else
	safe_str("0", buff, bufc);
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_extract: extract words from string:
 * * extract(foo bar baz,1,2) returns 'foo bar'
 * * extract(foo bar baz,2,1) returns 'bar'
 * * extract(foo bar baz,2,2) returns 'bar baz'
 * * 
 * * Now takes optional separator extract(foo-bar-baz,1,2,-) returns 'foo-bar'
 */

static void fun_extract(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int start, len;
    char *r, *s, *t, sep;

    varargs_preamble("EXTRACT", 4);

    s = fargs[0];
    start = atoi(fargs[1]);
    len = atoi(fargs[2]);

    if ((start < 1) || (len < 1)) {
	return;
    }
    /*
     * Skip to the start of the string to save 
     */

    start--;
    s = trim_space_sep(s, sep);
    while (start && s) {
	s = next_token(s, sep);
	start--;
    }

    /*
     * If we ran of the end of the string, return nothing 
     */

    if (!s || !*s) {
	return;
    }
    /*
     * Count off the words in the string to save 
     */

    r = s;
    len--;
    while (len && s) {
	s = next_token(s, sep);
	len--;
    }

    /*
     * Chop off the rest of the string, if needed 
     */

    if (s && *s)
	t = split_token(&s, sep);
    safe_str(r, buff, bufc);
}

int xlate(arg)
char *arg;
{
    int temp;
    char *temp2;

    if (arg[0] == '#') {
	arg++;
        /* #- anything is false */
        if (arg[1] =='-') {
            return 0;
        }
	if (is_integer(arg)) {
	    temp = atoi(arg);
	    if (temp == -1)
		temp = 0;
	    return temp;
	}
	return 0;
    }
    temp2 = trim_space_sep(arg, ' ');
    if (!*temp2)
	return 0;
    if (is_integer(temp2))
	return atoi(temp2);
    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_index:  like extract(), but it works with an arbitrary separator.
 * * index(a b | c d e | f gh | ij k, |, 2, 1) => c d e
 * * index(a b | c d e | f gh | ij k, |, 2, 2) => c d e | f g h
 */

static void fun_index(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int start, end;
    char c, *s, *p;

    s = fargs[0];
    c = *fargs[1];
    start = atoi(fargs[2]);
    end = atoi(fargs[3]);

    if ((start < 1) || (end < 1) || (*s == '\0'))
	return;
    if (c == '\0')
	c = ' ';

    /*
     * move s to point to the start of the item we want 
     */

    start--;
    while (start && s && *s) {
	if ((s = (char *) index(s, c)) != NULL)
	    s++;
	start--;
    }

    /*
     * skip over just spaces 
     */

    while (s && (*s == ' '))
	s++;
    if (!s || !*s)
	return;

    /*
     * figure out where to end the string 
     */

    p = s;
    while (end && p && *p) {
	if ((p = (char *) index(p, c)) != NULL) {
	    if (--end == 0) {
		do {
		    p--;
		} while ((*p == ' ') && (p > s));
		*(++p) = '\0';
		safe_str(s, buff, bufc);
		return;
	    } else {
		p++;
	    }
	}
    }

    /*
     * if we've gotten this far, we've run off the end of the string 
     */

    safe_str(s, buff, bufc);
}


static void fun_cat(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i;

    safe_str(fargs[0], buff, bufc);
    for (i = 1; i < nfargs; i++) {
	safe_chr(' ', buff, bufc);
	safe_str(fargs[i], buff, bufc);
    }
}

static void fun_version(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_str(mudstate.version, buff, bufc);
}

static void fun_strlen(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%d",
	(int) strlen((char *) strip_ansi(fargs[0])));
}

static void fun_num(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "#%d", match_thing(player, fargs[0]));
}

static void fun_pmatch(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref thing;

    if (*fargs[0] == '#') {
	safe_tprintf_str(buff, bufc, "#%d", match_thing(player, fargs[0]));
	return;
    }
    if (!((thing = lookup_player(player, fargs[0], 1)) == NOTHING)) {
	safe_tprintf_str(buff, bufc, "#%d", thing);
	return;
    } else
	safe_str("#-1 NO MATCH", buff, bufc);
}

static void fun_gt(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%d", (atof(fargs[0]) > atof(fargs[1])));
}

static void fun_gte(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%d", (atof(fargs[0]) >= atof(fargs[1])));
}

static void fun_lt(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%d", (atof(fargs[0]) < atof(fargs[1])));
}

static void fun_lte(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%d", (atof(fargs[0]) <= atof(fargs[1])));
}

static void fun_eq(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%d", (atof(fargs[0]) == atof(fargs[1])));
}

static void fun_neq(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%d", (atof(fargs[0]) != atof(fargs[1])));
}

static void fun_and(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i, val;

    if (nfargs < 2) {
    	safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
    	return;
    }

    val = atoi(fargs[0]);
    for (i = 1; val && i < nfargs; i++)
	val = val && atoi(fargs[i]);

    safe_tprintf_str(buff, bufc, "%d", val);
}

static void fun_or(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i, val;

    if (nfargs < 2) {
    	safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
    	return;
    }

    val = atoi(fargs[0]);
    for (i = 1; !val && i < nfargs; i++)
	val = val || atoi(fargs[i]);

    safe_tprintf_str(buff, bufc, "%d", val);
}

static void fun_xor(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i, val;

    if (nfargs < 2) {
    	safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
    	return;
    }

    val = atoi(fargs[0]);
    for (i = 1; i < nfargs; i++) {
	int tval = atoi(fargs[i]);
	val = (val && !tval) || (!val && tval);
    }
    safe_tprintf_str(buff, bufc, "%d", val);
}

static void fun_not(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%d", !xlate(fargs[0]));
}

static void fun_t(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%d", !(!xlate(fargs[0])));
}

static void fun_sqrt(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    double val;

    val = atof(fargs[0]);
    if (val < 0) {
	safe_str("#-1 SQUARE ROOT OF NEGATIVE", buff, bufc);
    } else if (val == 0) {
	safe_str("0", buff, bufc);
    } else {
	fval(buff, bufc, sqrt(val));
    }
}

static void fun_add(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    double sum = 0;
    int i;

    if (!nfargs) {
    	safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
    	return;
    }
    for (i = 0; i < nfargs; i++)
	sum += atof(fargs[i]);

    fval(buff, bufc, sum);
}

static void fun_sub(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, atof(fargs[0]) - atof(fargs[1]));
}

static void fun_mul(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i;
    double prod;

    if (!nfargs) {
    	safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
    	return;
    }

    prod = atof(fargs[0]);
    for (i = 1; i < nfargs; i++)
	prod *= atof(fargs[i]);

    fval(buff, bufc, prod);
}

static void fun_floor(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%.0f", floor(atof(fargs[0])));
}

static void fun_ceil(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%.0f", ceil(atof(fargs[0])));
}

static void fun_round(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    const char *fstr;
    char *oldp;

    oldp = *bufc;

    switch (atoi(fargs[1])) {
    case 1:
	fstr = "%.1f";
	break;
    case 2:
	fstr = "%.2f";
	break;
    case 3:
	fstr = "%.3f";
	break;
    case 4:
	fstr = "%.4f";
	break;
    case 5:
	fstr = "%.5f";
	break;
    case 6:
	fstr = "%.6f";
	break;
    default:
	fstr = "%.0f";
	break;
    }
    safe_tprintf_str(buff, bufc, (char *) fstr, atof(fargs[0]));

    /* Handle bogus result of "-0" from sprintf.  Yay, cclib. */

    if (!strcmp(oldp, "-0")) {
	*oldp = '0';
	*bufc = oldp + 1;
    }
}

static void fun_trunc(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_tprintf_str(buff, bufc, "%.0f", atof(fargs[0]));
}

static void fun_div(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int bot;

    bot = atoi(fargs[1]);
    if (bot == 0) {
	safe_str("#-1 DIVIDE BY ZERO", buff, bufc);
    } else {
	safe_tprintf_str(buff, bufc, "%d", (atoi(fargs[0]) / bot));
    }
}

static void fun_fdiv(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    double bot;

    bot = atof(fargs[1]);
    if (bot == 0) {
	safe_str("#-1 DIVIDE BY ZERO", buff, bufc);
    } else {
	fval(buff, bufc, (atof(fargs[0]) / bot));
    }
}

static void fun_mod(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int bot;

    bot = atoi(fargs[1]);
    if (bot == 0)
	bot = 1;
    safe_tprintf_str(buff, bufc, "%d", atoi(fargs[0]) % bot);
}

static void fun_pi(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_str("3.141592654", buff, bufc);
}

static void fun_e(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_str("2.718281828", buff, bufc);
}

static void fun_sin(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, sin(atof(fargs[0])));
}

static void fun_cos(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, cos(atof(fargs[0])));
}

static void fun_tan(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, tan(atof(fargs[0])));
}

static void fun_exp(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, exp(atof(fargs[0])));
}

static void fun_power(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    double val1, val2;

    val1 = atof(fargs[0]);
    val2 = atof(fargs[1]);
    if (val1 < 0) {
	safe_str("#-1 POWER OF NEGATIVE", buff, bufc);
    } else {
	fval(buff, bufc, pow(val1, val2));
    }
}

static void fun_ln(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    double val;

    val = atof(fargs[0]);
    if (val > 0)
	fval(buff, bufc, log(val));
    else
	safe_str("#-1 LN OF NEGATIVE OR ZERO", buff, bufc);
}

static void fun_log(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    double val;

    val = atof(fargs[0]);
    if (val > 0) {
	fval(buff, bufc, log10(val));
    } else {
	safe_str("#-1 LOG OF NEGATIVE OR ZERO", buff, bufc);
    }
}

static void fun_asin(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    double val;

    val = atof(fargs[0]);
    if ((val < -1) || (val > 1)) {
	safe_str("#-1 ASIN ARGUMENT OUT OF RANGE", buff, bufc);
    } else {
	fval(buff, bufc, asin(val));
    }
}

static void fun_acos(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    double val;

    val = atof(fargs[0]);
    if ((val < -1) || (val > 1)) {
	safe_str("#-1 ACOS ARGUMENT OUT OF RANGE", buff, bufc);
    } else {
	fval(buff, bufc, acos(val));
    }
}

static void fun_atan(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    fval(buff, bufc, atan(atof(fargs[0])));
}

static void fun_dist2d(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int d;
    double r;

    d = atoi(fargs[0]) - atoi(fargs[2]);
    r = (double) (d * d);
    d = atoi(fargs[1]) - atoi(fargs[3]);
    r += (double) (d * d);
    d = (int) (sqrt(r) + 0.5);
    safe_tprintf_str(buff, bufc, "%d", d);
}

static void fun_dist3d(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int d;
    double r;

    d = atoi(fargs[0]) - atoi(fargs[3]);
    r = (double) (d * d);
    d = atoi(fargs[1]) - atoi(fargs[4]);
    r += (double) (d * d);
    d = atoi(fargs[2]) - atoi(fargs[5]);
    r += (double) (d * d);
    d = (int) (sqrt(r) + 0.5);
    safe_tprintf_str(buff, bufc, "%d", d);
}



/*
 * ---------------------------------------------------------------------------
 * * fun_comp: string compare.
 */

static void fun_comp(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int x;

    x = strcmp(fargs[0], fargs[1]);
    if (x > 0)
	safe_str("1", buff, bufc);
    else if (x < 0)
	safe_str("-1", buff, bufc);
    else
	safe_str("0", buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_lcon: Return a list of contents.
 */

static void fun_lcon(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref thing, it;
    char *tbuf;
    int first = 1;

    it = match_thing(player, fargs[0]);
    if ((it != NOTHING) && (Has_contents(it)) && (Examinable(player, it) ||
	    (Location(player) == it) || (it == cause))) {
	tbuf = alloc_sbuf("fun_lcon");
	DOLIST(thing, Contents(it)) {
	    if (!first)
		sprintf(tbuf, " #%d", thing);
	    else {
		sprintf(tbuf, "#%d", thing);
		first = 0;
	    }
	    safe_str(tbuf, buff, bufc);
	}
	free_sbuf(tbuf);
    } else
	safe_str("#-1", buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_lexits: Return a list of exits.
 */

static void fun_lexits(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref thing, it, parent;
    char *tbuf;
    int exam, lev, key;
    int first = 1;

    it = match_thing(player, fargs[0]);

    if (!Good_obj(it) || !Has_exits(it)) {
	safe_str("#-1", buff, bufc);
	return;
    }
    exam = Examinable(player, it);
    if (!exam && (where_is(player) != it) && (it != cause)) {
	safe_str("#-1", buff, bufc);
	return;
    }
    tbuf = alloc_sbuf("fun_lexits");

    /*
     * Return info for all parent levels 
     */

    ITER_PARENTS(it, parent, lev) {

	/*
	 * Look for exits at each level 
	 */

	if (!Has_exits(parent))
	    continue;
	key = 0;
	if (Examinable(player, parent))
	    key |= VE_LOC_XAM;
	if (Dark(parent))
	    key |= VE_LOC_DARK;
	if (Dark(it))
	    key |= VE_BASE_DARK;
	DOLIST(thing, Exits(parent)) {
	    if (exit_visible(thing, player, key)) {
		if (!first)
		    sprintf(tbuf, " #%d", thing);
		else {
		    sprintf(tbuf, "#%d", thing);
		    first = 0;
		}
		safe_str(tbuf, buff, bufc);
	    }
	}
    }
    free_sbuf(tbuf);
    return;
}

/*
 * --------------------------------------------------------------------------
 * * fun_home: Return an object's home 
 */

static void fun_home(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (!Good_obj(it) || !Examinable(player, it))
	safe_str("#-1", buff, bufc);
    else if (Has_home(it))
	safe_tprintf_str(buff, bufc, "#%d", Home(it));
    else if (Has_dropto(it))
	safe_tprintf_str(buff, bufc, "#%d", Dropto(it));
    else if (isExit(it))
	safe_tprintf_str(buff, bufc, "#%d", where_is(it));
    else
	safe_str("#-1", buff, bufc);
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_money: Return an object's value
 */

static void fun_money(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if ((it == NOTHING) || !Examinable(player, it))
	safe_str("#-1", buff, bufc);
    else
	safe_tprintf_str(buff, bufc, "%d", Pennies(it));
}

/*
 * ---------------------------------------------------------------------------
 * * fun_pos: Find a word in a string 
 */

static void fun_pos(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i = 1;
    char *s, *t, *u;

    i = 1;
    s = fargs[1];
    while (*s) {
	u = s;
	t = fargs[0];
	while (*t && *t == *u)
	    ++t, ++u;
	if (*t == '\0') {
	    safe_tprintf_str(buff, bufc, "%d", i);
	    return;
	}
	++i, ++s;
    }
    safe_str("#-1", buff, bufc);
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * ldelete: Remove a word from a string by place
 * *  ldelete(<list>,<position>[,<separator>])
 * *
 * * insert: insert a word into a string by place
 * *  insert(<list>,<position>,<new item> [,<separator>])
 * *
 * * replace: replace a word into a string by place
 * *  replace(<list>,<position>,<new item>[,<separator>])
 */

#define	IF_DELETE	0
#define	IF_REPLACE	1
#define	IF_INSERT	2

static void do_itemfuns(buff, bufc, str, el, word, sep, flag)
char *buff, **bufc, *str, *word, sep;
int el, flag;
{
    int ct, overrun;
    char *sptr, *iptr, *eptr;
    char nullb;

    /*
     * If passed a null string return an empty string, except that we * * 
     * 
     * *  * * are allowed to append to a null string. 
     */

    if ((!str || !*str) && ((flag != IF_INSERT) || (el != 1))) {
	return;
    }
    /*
     * we can't fiddle with anything before the first position 
     */

    if (el < 1) {
	safe_str(str, buff, bufc);
	return;
    }
    /*
     * Split the list up into 'before', 'target', and 'after' chunks * *
     * * * pointed to by sptr, iptr, and eptr respectively. 
     */

    nullb = '\0';
    if (el == 1) {
	/*
	 * No 'before' portion, just split off element 1 
	 */

	sptr = NULL;
	if (!str || !*str) {
	    eptr = NULL;
	    iptr = NULL;
	} else {
	    eptr = trim_space_sep(str, sep);
	    iptr = split_token(&eptr, sep);
	}
    } else {
	/*
	 * Break off 'before' portion 
	 */

	sptr = eptr = trim_space_sep(str, sep);
	overrun = 1;
	for (ct = el; ct > 2 && eptr; eptr = next_token(eptr, sep), ct--);
	if (eptr) {
	    overrun = 0;
	    iptr = split_token(&eptr, sep);
	}
	/*
	 * If we didn't make it to the target element, just return *
	 * * * * the string.  Insert is allowed to continue if we are 
	 * *  * *  * exactly at the end of the string, but replace
	 * and * delete *  *  * * are not. 
	 */

	if (!(eptr || ((flag == IF_INSERT) && !overrun))) {
	    safe_str(str, buff, bufc);
	    return;
	}
	/*
	 * Split the 'target' word from the 'after' portion. 
	 */

	if (eptr)
	    iptr = split_token(&eptr, sep);
	else
	    iptr = NULL;
    }

    switch (flag) {
    case IF_DELETE:		/*
				   * deletion 
				 */
	if (sptr) {
	    safe_str(sptr, buff, bufc);
	    if (eptr)
		safe_chr(sep, buff, bufc);
	}
	if (eptr) {
	    safe_str(eptr, buff, bufc);
	}
	break;
    case IF_REPLACE:		/*
				 * replacing 
				 */
	if (sptr) {
	    safe_str(sptr, buff, bufc);
	    safe_chr(sep, buff, bufc);
	}
	safe_str(word, buff, bufc);
	if (eptr) {
	    safe_chr(sep, buff, bufc);
	    safe_str(eptr, buff, bufc);
	}
	break;
    case IF_INSERT:		/*
				   * insertion 
				 */
	if (sptr) {
	    safe_str(sptr, buff, bufc);
	    safe_chr(sep, buff, bufc);
	}
	safe_str(word, buff, bufc);
	if (iptr) {
	    safe_chr(sep, buff, bufc);
	    safe_str(iptr, buff, bufc);
	}
	if (eptr) {
	    safe_chr(sep, buff, bufc);
	    safe_str(eptr, buff, bufc);
	}
	break;
    }
}


static void fun_ldelete(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{				/*
				 * delete a word at position X of a list 
				 */
    char sep;

    varargs_preamble("LDELETE", 3);
    do_itemfuns(buff, bufc, fargs[0], atoi(fargs[1]), NULL, sep,
	IF_DELETE);
}

static void fun_replace(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{				/*
				 * replace a word at position X of a list 
				 */
    char sep;

    varargs_preamble("REPLACE", 4);
    do_itemfuns(buff, bufc, fargs[0], atoi(fargs[1]), fargs[2], sep,
	IF_REPLACE);
}

static void fun_insert(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{				/*
				 * insert a word at position X of a list 
				 */
    char sep;

    varargs_preamble("INSERT", 4);
    do_itemfuns(buff, bufc, fargs[0], atoi(fargs[1]), fargs[2], sep,
	IF_INSERT);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_remove: Remove a word from a string
 */

static void fun_remove(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *s, *sp, *word;
    char sep;
    int first, found;

    varargs_preamble("REMOVE", 3);
    if (index(fargs[1], sep)) {
	safe_str("#-1 CAN ONLY DELETE ONE ELEMENT", buff, bufc);
	return;
    }
    s = fargs[0];
    word = fargs[1];

    /*
     * Walk through the string copying words until (if ever) we get to *
     * * * * one that matches the target word. 
     */

    sp = s;
    found = 0;
    first = 1;
    while (s) {
	sp = split_token(&s, sep);
	if (found || strcmp(sp, word)) {
	    if (!first)
		safe_chr(sep, buff, bufc);
	    safe_str(sp, buff, bufc);
	    first = 0;
	} else {
	    found = 1;
	}
    }
}

/*
 * ---------------------------------------------------------------------------
 * * fun_member: Is a word in a string
 */

static void fun_member(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int wcount;
    char *r, *s, sep;

    varargs_preamble("MEMBER", 3);
    wcount = 1;
    s = trim_space_sep(fargs[0], sep);
    do {
	r = split_token(&s, sep);
	if (!strcmp(fargs[1], r)) {
	    safe_tprintf_str(buff, bufc, "%d", wcount);
	    return;
	}
	wcount++;
    } while (s);
    safe_str("0", buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_secure, fun_escape: escape [, ], %, \, and the beginning of the string.
 */

static void fun_secure(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *s;

    s = fargs[0];
    while (*s) {
	switch (*s) {
	case '%':
	case '$':
	case '\\':
	case '[':
	case ']':
	case '(':
	case ')':
	case '{':
	case '}':
	case ',':
	case ';':
	    safe_chr(' ', buff, bufc);
	    break;
	default:
	    safe_chr(*s, buff, bufc);
	}
	s++;
    }
}

static void fun_ansi_secure(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *s;

    s = fargs[0];
    while (*s) {
	switch (*s) {
	case '\033':
	    safe_chr(*s++, buff, bufc);
	    if (*s == '[') {
		safe_chr(*s, buff, bufc);
		break;
	    }			/* FALLTHRU */
	case '%':
	case '[':
	case '$':
	case '\\':
	case ']':
	case '(':
	case ')':
	case '{':
	case '}':
	case ',':
	case ';':
	    safe_chr(' ', buff, bufc);
	    break;
	default:
	    safe_chr(*s, buff, bufc);
	}
	s++;
    }
}

static void fun_escape(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *s, *d;

    d = *bufc;
    s = fargs[0];
    while (*s) {
	switch (*s) {
	case '%':
	case '\\':
	case '[':
	case ']':
	case '{':
	case '}':
	case ';':
	    safe_chr('\\', buff, bufc);
	default:
	    if (*bufc == d)
		safe_chr('\\', buff, bufc);
	    safe_chr(*s, buff, bufc);
	}
	s++;
    }
}


/*
 * Take a character position and return which word that char is in. 
 * wordpos(<string>, <charpos>)
 */
static void fun_wordpos(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int charpos, i;
    char *cp, *tp, *xp, sep;

    varargs_preamble("WORDPOS", 3);

    charpos = atoi(fargs[1]);
    cp = fargs[0];
    if ((charpos > 0) && (charpos <= strlen(cp))) {
	tp = &(cp[charpos - 1]);
	cp = trim_space_sep(cp, sep);
	xp = split_token(&cp, sep);
	for (i = 1; xp; i++) {
	    if (tp < (xp + strlen(xp)))
		break;
	    xp = split_token(&cp, sep);
	}
	safe_tprintf_str(buff, bufc, "%d", i);
	return;
    }
    safe_str("#-1", buff, bufc);
    return;
}

static void fun_type(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (!Good_obj(it)) {
	safe_str("#-1 NOT FOUND", buff, bufc);
	return;
    }
    switch (Typeof(it)) {
    case TYPE_ROOM:
	safe_str("ROOM", buff, bufc);
	break;
    case TYPE_EXIT:
	safe_str("EXIT", buff, bufc);
	break;
    case TYPE_PLAYER:
	safe_str("PLAYER", buff, bufc);
	break;
    case TYPE_THING:
	safe_str("THING", buff, bufc);
	break;
    default:
	safe_str("#-1 ILLEGAL TYPE", buff, bufc);
    }
    return;
}

static void fun_hasflag(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (!Good_obj(it)) {
	safe_str("#-1 NOT FOUND", buff, bufc);
	return;
    }
    if (mudconf.pub_flags || Examinable(player, it) || (it == cause)) {
	if (has_flag(player, it, fargs[1]))
	    safe_str("1", buff, bufc);
	else
	    safe_str("0", buff, bufc);
    } else {
	safe_str("#-1 PERMISSION DENIED", buff, bufc);
    }
}

static void fun_haspower(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (!Good_obj(it)) {
	safe_str("#-1 NOT FOUND", buff, bufc);
	return;
    }
    if (mudconf.pub_flags || Examinable(player, it) || (it == cause)) {
	if (has_power(player, it, fargs[1]))
	    safe_str("1", buff, bufc);
	else
	    safe_str("0", buff, bufc);
    } else {
	safe_str("#-1 PERMISSION DENIED", buff, bufc);
    }
}

static void fun_delete(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *s, *temp, *bp;
    int i, start, nchars, len;

    s = fargs[0];
    start = atoi(fargs[1]);
    nchars = atoi(fargs[2]);
    len = strlen(s);
    if ((start >= len) || (nchars <= 0)) {
	safe_str(s, buff, bufc);
	return;
    }
    bp = temp = alloc_lbuf("fun_delete");
    for (i = 0; i < start; i++)
	*bp++ = (*s++);
    if ((i + nchars) < len && (i + nchars) > 0) {
	s += nchars;
	while ((*bp++ = *s++));
    } else
	*bp = '\0';

    safe_str(temp, buff, bufc);
    free_lbuf(temp);
}

static void fun_lock(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it, aowner;
    int aflags;
    char *tbuf;
    ATTR *attr;
    struct boolexp *bool;

    /*
     * Parse the argument into obj + lock 
     */

    if (!get_obj_and_lock(player, fargs[0], &it, &attr, buff, bufc))
	return;

    /*
     * Get the attribute and decode it if we can read it 
     */

    tbuf = atr_get(it, attr->number, &aowner, &aflags);
    if (Read_attr(player, it, attr, aowner, aflags)) {
	bool = parse_boolexp(player, tbuf, 1);
	free_lbuf(tbuf);
	tbuf = (char *) unparse_boolexp_function(player, bool);
	free_boolexp(bool);
	safe_str(tbuf, buff, bufc);
    } else
	free_lbuf(tbuf);
}

static void fun_elock(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref it, victim, aowner;
    int aflags;
    char *tbuf;
    ATTR *attr;
    struct boolexp *bool;

    /*
     * Parse lock supplier into obj + lock 
     */

    if (!get_obj_and_lock(player, fargs[0], &it, &attr, buff, bufc))
	return;

    /*
     * Get the victim and ensure we can do it 
     */

    victim = match_thing(player, fargs[1]);
    if (!Good_obj(victim)) {
	safe_str("#-1 NOT FOUND", buff, bufc);
    } else if (!nearby_or_control(player, victim) &&
	!nearby_or_control(player, it)) {
	safe_str("#-1 TOO FAR AWAY", buff, bufc);
    } else {
	tbuf = atr_get(it, attr->number, &aowner, &aflags);
	if ((attr->number == A_LOCK) ||
	    Read_attr(player, it, attr, aowner, aflags)) {
	    bool = parse_boolexp(player, tbuf, 1);
	    safe_tprintf_str(buff, bufc, "%d", eval_boolexp(victim, it, it,
		    bool));
	    free_boolexp(bool);
	} else {
	    safe_str("0", buff, bufc);
	}
	free_lbuf(tbuf);
    }
}

/*
 * ---------------------------------------------------------------------------
 * * fun_lwho: Return list of connected users.
 */

static void fun_lwho(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    make_ulist(player, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_nearby: Return whether or not obj1 is near obj2.
 */

static void fun_nearby(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref obj1, obj2;

    obj1 = match_thing(player, fargs[0]);
    obj2 = match_thing(player, fargs[1]);
    if (!(nearby_or_control(player, obj1) ||
	    nearby_or_control(player, obj2)))
	safe_str("0", buff, bufc);
    else if (nearby(obj1, obj2))
	safe_str("1", buff, bufc);
    else
	safe_str("0", buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_obj, fun_poss, and fun_subj: perform pronoun sub for object.
 */

static void process_sex(player, what, token, buff, bufc)
dbref player;
char *what, *buff, **bufc;
const char *token;
{
    dbref it;
    char *str;

    it = match_thing(player, what);
    if (!Good_obj(it) || (!isPlayer(it) && !nearby_or_control(player, it))) {
	safe_str("#-1 NO MATCH", buff, bufc);
    } else {
	str = (char *) token;
	exec(buff, bufc, 0, it, it, 0, &str, (char **) NULL, 0);
    }
}

static void fun_obj(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    process_sex(player, fargs[0], "%o", buff, bufc);
}

static void fun_poss(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    process_sex(player, fargs[0], "%p", buff, bufc);
}

static void fun_subj(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    process_sex(player, fargs[0], "%s", buff, bufc);
}

static void fun_aposs(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    process_sex(player, fargs[0], "%a", buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_mudname: Return the name of the mud.
 */

static void fun_mudname(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_str(mudconf.mud_name, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_lcstr, fun_ucstr, fun_capstr: Lowercase, uppercase, or capitalize str.
 */

static void fun_lcstr(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *ap;

    ap = fargs[0];
    while (*ap) {
	**bufc = ToLower(*ap);
	ap++;
	(*bufc)++;
    }
}

static void fun_ucstr(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *ap;

    ap = fargs[0];
    while (*ap) {
	**bufc = ToUpper(*ap);
	ap++;
	(*bufc)++;
    }
}

static void fun_capstr(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *s;

    s = *bufc;

    safe_str(fargs[0], buff, bufc);
    *s = ToUpper(*s);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_lnum: Return a list of numbers.
 */

static void fun_lnum(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char tbuff[10];
    int ctr, limit, llimit = 0, over;

    if (nfargs > 2 || nfargs < 1) {
	safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
	return;
    }
    over = 0;
    limit = atoi(fargs[0]);
    if (nfargs == 2) {
	llimit = atoi(fargs[0]);
	limit = atoi(fargs[1]) + 1;
    } else
	limit = atoi(fargs[0]);
    if (limit > 0 && llimit >= 0 && llimit < limit) {
	for (ctr = llimit; ctr < limit && !over; ctr++) {
	    {
		sprintf(tbuff, "%s%d", ctr != llimit ? " " : "", ctr);
		over = safe_str(tbuff, buff, bufc);
	    }
	}
    }
}

/*
 * ---------------------------------------------------------------------------
 * * fun_lattr: Return list of attributes I can see on the object.
 */

static void fun_lattr(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref thing;
    int ca, first;
    ATTR *attr;

    /*
     * Check for wildcard matching.  parse_attrib_wild checks for read
     * permission, so we don't have to.  Have p_a_w assume the slash-star if
     * it is missing.
     */

    first = 1;
    olist_push();
    if (parse_attrib_wild(player, fargs[0], &thing, 0, 0, 1)) {
	for (ca = olist_first(); ca != NOTHING; ca = olist_next()) {
	    attr = atr_num(ca);
	    if (attr) {
		if (!first)
		    safe_chr(' ', buff, bufc);
		first = 0;
		safe_str((char *) attr->name, buff, bufc);
	    }
	}
    } else {
	safe_str("#-1 NO MATCH", buff, bufc);
    }
    olist_pop();
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * do_reverse, fun_reverse, fun_revwords: Reverse things.
 */

static void do_reverse(from, to)
char *from, *to;
{
    char *tp;

    tp = to + strlen(from);
    *tp-- = '\0';
    while (*from) {
	*tp-- = *from++;
    }
}

static void fun_reverse(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    do_reverse(fargs[0], *bufc);
    *bufc += strlen(fargs[0]);
}

static void fun_revwords(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *temp, *tp, *t1, sep;
    int first;

    /*
     * If we are passed an empty arglist return a null string 
     */

    if (nfargs == 0) {
	return;
    }
    varargs_preamble("REVWORDS", 2);
    temp = alloc_lbuf("fun_revwords");

    /*
     * Reverse the whole string 
     */

    do_reverse(fargs[0], temp);

    /*
     * Now individually reverse each word in the string.  This will
     * undo the reversing of the words (so the words themselves are
     * forwards again. 
     */

    tp = temp;
    first = 1;
    while (tp) {
	if (!first)
	    safe_chr(sep, buff, bufc);
	t1 = split_token(&tp, sep);
	do_reverse(t1, *bufc);
	*bufc += strlen(t1);
	first = 0;
    }
    free_lbuf(temp);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_after, fun_before: Return substring after or before a specified string.
 */

static void fun_after(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *bp, *cp, *mp;
    int mlen;

    if (nfargs == 0) {
	return;
    }
    if (!fn_range_check("AFTER", nfargs, 1, 2, buff, bufc))
	return;
    bp = fargs[0];
    mp = fargs[1];

    /*
     * Sanity-check arg1 and arg2 
     */

    if (bp == NULL)
	bp = "";
    if (mp == NULL)
	mp = " ";
    if (!mp || !*mp)
	mp = (char *) " ";
    mlen = strlen(mp);
    if ((mlen == 1) && (*mp == ' '))
	bp = trim_space_sep(bp, ' ');

    /*
     * Look for the target string 
     */

    while (*bp) {

	/*
	 * Search for the first character in the target string 
	 */

	cp = (char *) index(bp, *mp);
	if (cp == NULL) {

	    /*
	     * Not found, return empty string 
	     */

	    return;
	}
	/*
	 * See if what follows is what we are looking for 
	 */

	if (!strncmp(cp, mp, mlen)) {

	    /*
	     * Yup, return what follows 
	     */

	    bp = cp + mlen;
	    safe_str(bp, buff, bufc);
	    return;
	}
	/*
	 * Continue search after found first character 
	 */

	bp = cp + 1;
    }

    /*
     * Ran off the end without finding it 
     */

    return;
}

static void fun_before(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *bp, *cp, *mp, *ip;
    int mlen;

    if (nfargs == 0) {
	return;
    }
    if (!fn_range_check("BEFORE", nfargs, 1, 2, buff, bufc))
	return;

    bp = fargs[0];
    mp = fargs[1];

    /*
     * Sanity-check arg1 and arg2 
     */

    if (bp == NULL)
	bp = "";
    if (mp == NULL)
	mp = " ";
    if (!mp || !*mp)
	mp = (char *) " ";
    mlen = strlen(mp);
    if ((mlen == 1) && (*mp == ' '))
	bp = trim_space_sep(bp, ' ');
    ip = bp;

    /*
     * Look for the target string 
     */

    while (*bp) {

	/*
	 * Search for the first character in the target string 
	 */

	cp = (char *) index(bp, *mp);
	if (cp == NULL) {

	    /*
	     * Not found, return entire string 
	     */

	    safe_str(ip, buff, bufc);
	    return;
	}
	/*
	 * See if what follows is what we are looking for 
	 */

	if (!strncmp(cp, mp, mlen)) {

	    /*
	     * Yup, return what follows 
	     */

	    *cp = '\0';
	    safe_str(ip, buff, bufc);
	    return;
	}
	/*
	 * Continue search after found first character 
	 */

	bp = cp + 1;
    }

    /*
     * Ran off the end without finding it 
     */

    safe_str(ip, buff, bufc);
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_max, fun_min: Return maximum (minimum) value.
 */

static void fun_max(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i, j, got_one;
    double max;

    max = 0;
    for (i = 0, got_one = 0; i < nfargs; i++) {
	if (fargs[i]) {
	    j = atof(fargs[i]);
	    if (!got_one || (j > max)) {
		got_one = 1;
		max = j;
	    }
	}
    }

    if (!got_one)
	safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
    else
	fval(buff, bufc, max);
    return;
}

static void fun_min(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i, j, got_one;
    double min;

    min = 0;
    for (i = 0, got_one = 0; i < nfargs; i++) {
	if (fargs[i]) {
	    j = atof(fargs[i]);
	    if (!got_one || (j < min)) {
		got_one = 1;
		min = j;
	    }
	}
    }

    if (!got_one) {
	safe_str("#-1 TOO FEW ARGUMENTS", buff, bufc);
    } else {
	fval(buff, bufc, min);
    }
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_search: Search the db for things, returning a list of what matches
 */

static void fun_search(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref thing;
    char *bp, *nbuf;
    SEARCH searchparm;

    /*
     * Set up for the search.  If any errors, abort. 
     */

    if (!search_setup(player, fargs[0], &searchparm)) {
	safe_str("#-1 ERROR DURING SEARCH", buff, bufc);
	return;
    }
    /*
     * Do the search and report the results 
     */

    olist_push();
    search_perform(player, cause, &searchparm);
    bp = *bufc;
    nbuf = alloc_sbuf("fun_search");
    for (thing = olist_first(); thing != NOTHING; thing = olist_next()) {
	if (bp == *bufc)
	    sprintf(nbuf, "#%d", thing);
	else
	    sprintf(nbuf, " #%d", thing);
	safe_str(nbuf, buff, bufc);
    }
    free_sbuf(nbuf);
    olist_pop();
}

/*
 * ---------------------------------------------------------------------------
 * * fun_stats: Get database size statistics.
 */

static void fun_stats(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref who;
    STATS statinfo;

    if ((!fargs[0]) || !*fargs[0] || !string_compare(fargs[0], "all")) {
	who = NOTHING;
    } else {
	who = lookup_player(player, fargs[0], 1);
	if (who == NOTHING) {
	    safe_str("#-1 NOT FOUND", buff, bufc);
	    return;
	}
    }
    if (!get_stats(player, who, &statinfo)) {
	safe_str("#-1 ERROR GETTING STATS", buff, bufc);
	return;
    }
    safe_tprintf_str(buff, bufc, "%d %d %d %d %d %d", statinfo.s_total,
	statinfo.s_rooms, statinfo.s_exits, statinfo.s_things,
	statinfo.s_players, statinfo.s_garbage);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_merge:  given two strings and a character, merge the two strings
 * *   by replacing characters in string1 that are the same as the given 
 * *   character by the corresponding character in string2 (by position).
 * *   The strings must be of the same length.
 */

static void fun_merge(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *str, *rep;
    char c;

    /*
     * do length checks first 
     */

    if (strlen(fargs[0]) != strlen(fargs[1])) {
	safe_str("#-1 STRING LENGTHS MUST BE EQUAL", buff, bufc);
	return;
    }
    if (strlen(fargs[2]) > 1) {
	safe_str("#-1 TOO MANY CHARACTERS", buff, bufc);
	return;
    }
    /*
     * find the character to look for. null character is considered * a * 
     * 
     * *  * * space 
     */

    if (!*fargs[2])
	c = ' ';
    else
	c = *fargs[2];

    /*
     * walk strings, copy from the appropriate string 
     */

    for (str = fargs[0], rep = fargs[1];
	*str && *rep && ((*bufc - buff) < LBUF_SIZE);
	str++, rep++, (*bufc)++) {
	if (*str == c)
	    **bufc = *rep;
	else
	    **bufc = *str;
    }

    /*
     * There is no need to check for overflowing the buffer since * both
     * * * * strings are LBUF_SIZE or less and the new string cannot be *
     * * any * * longer. 
     */

    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_splice: similar to MERGE(), eplaces by word instead of by character.
 */

static void fun_splice(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *p1, *p2, *q1, *q2, sep;
    int words, i, first;

    varargs_preamble("SPLICE", 4);

    /*
     * length checks 
     */

    if (countwords(fargs[2], sep) > 1) {
	safe_str("#-1 TOO MANY WORDS", buff, bufc);
	return;
    }
    words = countwords(fargs[0], sep);
    if (words != countwords(fargs[1], sep)) {
	safe_str("#-1 NUMBER OF WORDS MUST BE EQUAL", buff, bufc);
	return;
    }
    /*
     * loop through the two lists 
     */

    p1 = fargs[0];
    q1 = fargs[1];
    first = 1;
    for (i = 0; i < words; i++) {
	p2 = split_token(&p1, sep);
	q2 = split_token(&q1, sep);
	if (!first)
	    safe_chr(sep, buff, bufc);
	if (!strcmp(p2, fargs[2]))
	    safe_str(q2, buff, bufc);	/*
					 * replace 
					 */
	else
	    safe_str(p2, buff, bufc);	/*
					 * copy 
					 */
	first = 0;
    }
}

/*
 * ---------------------------------------------------------------------------
 * * fun_repeat: repeats a string
 */

static void fun_repeat(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int times, i;

    times = atoi(fargs[1]);
    if ((times < 1) || (fargs[0] == NULL) || (!*fargs[0])) {
	return;
    } else if (times == 1) {
	safe_str(fargs[0], buff, bufc);
    } else if (strlen(fargs[0]) * times >= (LBUF_SIZE - 1)) {
	safe_str("#-1 STRING TOO LONG", buff, bufc);
    } else {
	for (i = 0; i < times; i++)
	    safe_str(fargs[0], buff, bufc);
    }
}

/*
 * ---------------------------------------------------------------------------
 * * fun_iter: Make list from evaluating arg2 with each member of arg1.
 * * NOTE: This function expects that its arguments have not been evaluated.
 */

static void fun_iter(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *curr, *objstring, *buff2, *buff3, *cp, *dp, sep, *str;
    int first, number = 0;

    evarargs_preamble("ITER", 3);
    dp = cp = curr = alloc_lbuf("fun_iter");
    str = fargs[0];
    exec(curr, &dp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str,
	cargs, ncargs);
    *dp = '\0';
    cp = trim_space_sep(cp, sep);
    if (!*cp) {
	free_lbuf(curr);
	return;
    }
    first = 1;
    while (cp) {
	if (!first)
	    safe_chr(' ', buff, bufc);
	first = 0;
	number++;
	objstring = split_token(&cp, sep);
	buff2 = replace_string(BOUND_VAR, objstring, fargs[1]);
	buff3 =
	    replace_string(LISTPLACE_VAR, tprintf("%d", number), buff2);
	str = buff3;
	exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	    &str, cargs, ncargs);
	free_lbuf(buff2);
	free_lbuf(buff3);
    }
    free_lbuf(curr);
}

static void fun_list(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *curr, *objstring, *buff2, *buff3, *result, *cp, *dp, *str, sep;
    int number = 0;

    evarargs_preamble("LIST", 3);
    cp = curr = dp = alloc_lbuf("fun_list");
    str = fargs[0];
    exec(curr, &dp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL, &str,
	cargs, ncargs);
    cp = trim_space_sep(cp, sep);
    if (!*cp) {
	free_lbuf(curr);
	return;
    }
    while (cp) {
	number++;
	objstring = split_token(&cp, sep);
	buff2 = replace_string(BOUND_VAR, objstring, fargs[1]);
	buff3 =
	    replace_string(LISTPLACE_VAR, tprintf("%d", number), buff2);
	dp = result = alloc_lbuf("fun_list.2");
	str = buff3;
	exec(result, &dp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	    &str, cargs, ncargs);
	*dp = '\0';
	free_lbuf(buff2);
	free_lbuf(buff3);
	notify(cause, result);
	free_lbuf(result);
    }
    free_lbuf(curr);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_fold: iteratively eval an attrib with a list of arguments
 * *        and an optional base case.  With no base case, the first list element
 * *    is passed as %0 and the second is %1.  The attrib is then evaluated
 * *    with these args, the result is then used as %0 and the next arg is
 * *    %1 and so it goes as there are elements left in the list.  The
 * *    optinal base case gives the user a nice starting point.
 * *
 * *    > &REP_NUM object=[%0][repeat(%1,%1)]
 * *    > say fold(OBJECT/REP_NUM,1 2 3 4 5,->)
 * *    You say "->122333444455555"
 * *
 * *      NOTE: To use added list separator, you must use base case!
 */

static void fun_fold(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner, thing;
    int aflags, anum;
    ATTR *ap;
    char *atext, *result, *curr, *bp, *str, *cp, *atextbuf, *clist[2],
	*rstore, sep;

    /*
     * We need two to four arguements only 
     */

    mvarargs_preamble("FOLD", 2, 4);

    /*
     * Two possibilities for the first arg: <obj>/<attr> and <attr>. 
     */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
	if ((anum == NOTHING) || (!Good_obj(thing)))
	    ap = NULL;
	else
	    ap = atr_num(anum);
    } else {
	thing = player;
	ap = atr_str(fargs[0]);
    }

    /*
     * Make sure we got a good attribute 
     */

    if (!ap) {
	return;
    }
    /*
     * Use it if we can access it, otherwise return an error. 
     */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
	return;
    } else if (!*atext || !See_attr(player, thing, ap, aowner, aflags)) {
	free_lbuf(atext);
	return;
    }
    /*
     * Evaluate it using the rest of the passed function args 
     */

    cp = curr = fargs[1];
    atextbuf = alloc_lbuf("fun_fold");
    StringCopy(atextbuf, atext);

    /*
     * may as well handle first case now 
     */

    if ((nfargs >= 3) && (fargs[2])) {
	clist[0] = fargs[2];
	clist[1] = split_token(&cp, sep);
	result = bp = alloc_lbuf("fun_fold");
	str = atextbuf;
	exec(result, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	    &str, clist, 2);
	*bp = '\0';
    } else {
	clist[0] = split_token(&cp, sep);
	clist[1] = split_token(&cp, sep);
	result = bp = alloc_lbuf("fun_fold");
	str = atextbuf;
	exec(result, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	    &str, clist, 2);
	*bp = '\0';
    }

    rstore = result;
    result = NULL;

    while (cp) {
	clist[0] = rstore;
	clist[1] = split_token(&cp, sep);
	StringCopy(atextbuf, atext);
	result = bp = alloc_lbuf("fun_fold");
	str = atextbuf;
	exec(result, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	    &str, clist, 2);
	*bp = '\0';
	StringCopy(rstore, result);
	free_lbuf(result);
    }
    safe_str(rstore, buff, bufc);
    free_lbuf(rstore);
    free_lbuf(atext);
    free_lbuf(atextbuf);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_filter: iteratively perform a function with a list of arguments
 * *              and return the arg, if the function evaluates to TRUE using the 
 * *      arg.
 * *
 * *      > &IS_ODD object=mod(%0,2)
 * *      > say filter(object/is_odd,1 2 3 4 5)
 * *      You say "1 3 5"
 * *      > say filter(object/is_odd,1-2-3-4-5,-)
 * *      You say "1-3-5"
 * *
 * *  NOTE:  If you specify a separator it is used to delimit returned list
 */

static void fun_filter(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner, thing;
    int aflags, anum, first;
    ATTR *ap;
    char *atext, *result, *curr, *objstring, *bp, *str, *cp, *atextbuf,
	sep;

    varargs_preamble("FILTER", 3);

    /*
     * Two possibilities for the first arg: <obj>/<attr> and <attr>. 
     */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
	if ((anum == NOTHING) || (!Good_obj(thing)))
	    ap = NULL;
	else
	    ap = atr_num(anum);
    } else {
	thing = player;
	ap = atr_str(fargs[0]);
    }

    /*
     * Make sure we got a good attribute 
     */

    if (!ap) {
	return;
    }
    /*
     * Use it if we can access it, otherwise return an error. 
     */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
	return;
    } else if (!*atext || !See_attr(player, thing, ap, aowner, aflags)) {
	free_lbuf(atext);
	return;
    }
    /*
     * Now iteratively eval the attrib with the argument list 
     */

    cp = curr = trim_space_sep(fargs[1], sep);
    atextbuf = alloc_lbuf("fun_filter");
    first = 1;
    while (cp) {
	objstring = split_token(&cp, sep);
	StringCopy(atextbuf, atext);
	result = bp = alloc_lbuf("fun_filter");
	str = atextbuf;
	exec(result, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	    &str, &objstring, 1);
	*bp = '\0';
	if (!first && *result == '1')
	    safe_chr(sep, buff, bufc);
	if (*result == '1') {
	    safe_str(objstring, buff, bufc);
	    first = 0;
	}
	free_lbuf(result);
    }
    free_lbuf(atext);
    free_lbuf(atextbuf);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_map: iteratively evaluate an attribute with a list of arguments.
 * *
 * *  > &DIV_TWO object=fdiv(%0,2)
 * *  > say map(1 2 3 4 5,object/div_two)
 * *  You say "0.5 1 1.5 2 2.5"
 * *  > say map(object/div_two,1-2-3-4-5,-)
 * *  You say "0.5-1-1.5-2-2.5"
 * *
 */

static void fun_map(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref aowner, thing;
    int aflags, anum, first;
    ATTR *ap;
    char *atext, *objstring, *str, *cp, *atextbuf, sep;

    varargs_preamble("MAP", 3);

    /*
     * Two possibilities for the second arg: <obj>/<attr> and <attr>. 
     */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
	if ((anum == NOTHING) || (!Good_obj(thing)))
	    ap = NULL;
	else
	    ap = atr_num(anum);
    } else {
	thing = player;
	ap = atr_str(fargs[0]);
    }

    /*
     * Make sure we got a good attribute 
     */

    if (!ap) {
	return;
    }
    /*
     * Use it if we can access it, otherwise return an error. 
     */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
	return;
    } else if (!*atext || !See_attr(player, thing, ap, aowner, aflags)) {
	free_lbuf(atext);
	return;
    }
    /*
     * now process the list one element at a time 
     */

    cp = trim_space_sep(fargs[1], sep);
    atextbuf = alloc_lbuf("fun_map");
    first = 1;
    while (cp) {
	if (!first)
	    safe_chr(sep, buff, bufc);
	first = 0;
	objstring = split_token(&cp, sep);
	StringCopy(atextbuf, atext);
	str = atextbuf;
	exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	    &str, &objstring, 1);
    }
    free_lbuf(atext);
    free_lbuf(atextbuf);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_edit: Edit text.
 */

static void fun_edit(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *tstr;

    edit_string((char *) strip_ansi(fargs[0]), &tstr, fargs[1], fargs[2]);
    safe_str(tstr, buff, bufc);
    free_lbuf(tstr);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_locate: Search for things with the perspective of another obj.
 */

static void fun_locate(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int pref_type, check_locks, verbose, multiple;
    dbref thing, what;
    char *cp;

    pref_type = NOTYPE;
    check_locks = verbose = multiple = 0;

    /*
     * Find the thing to do the looking, make sure we control it. 
     */

    if (See_All(player))
	thing = match_thing(player, fargs[0]);
    else
	thing = match_controlled(player, fargs[0]);
    if (!Good_obj(thing)) {
	safe_str("#-1 PERMISSION DENIED", buff, bufc);
	return;
    }
    /*
     * Get pre- and post-conditions and modifiers 
     */

    for (cp = fargs[2]; *cp; cp++) {
	switch (*cp) {
	case 'E':
	    pref_type = TYPE_EXIT;
	    break;
	case 'L':
	    check_locks = 1;
	    break;
	case 'P':
	    pref_type = TYPE_PLAYER;
	    break;
	case 'R':
	    pref_type = TYPE_ROOM;
	    break;
	case 'T':
	    pref_type = TYPE_THING;
	    break;
	case 'V':
	    verbose = 1;
	    break;
	case 'X':
	    multiple = 1;
	    break;
	}
    }

    /*
     * Set up for the search 
     */

    if (check_locks)
	init_match_check_keys(thing, fargs[1], pref_type);
    else
	init_match(thing, fargs[1], pref_type);

    /*
     * Search for each requested thing 
     */

    for (cp = fargs[2]; *cp; cp++) {
	switch (*cp) {
	case 'a':
	    match_absolute();
	    break;
	case 'c':
	    match_carried_exit_with_parents();
	    break;
	case 'e':
	    match_exit_with_parents();
	    break;
	case 'h':
	    match_here();
	    break;
	case 'i':
	    match_possession();
	    break;
	case 'm':
	    match_me();
	    break;
	case 'n':
	    match_neighbor();
	    break;
	case 'p':
	    match_player();
	    break;
	case '*':
	    match_everything(MAT_EXIT_PARENTS);
	    break;
	}
    }

    /*
     * Get the result and return it to the caller 
     */

    if (multiple)
	what = last_match_result();
    else
	what = match_result();

    if (verbose)
	(void) match_status(player, what);

    safe_tprintf_str(buff, bufc, "#%d", what);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_switch: Return value based on pattern matching (ala @switch)
 * * NOTE: This function expects that its arguments have not been evaluated.
 */

static void fun_switch(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i;
    char *mbuff, *tbuff, *buf, *bp, *str;

    /*
     * If we don't have at least 2 args, return nothing 
     */

    if (nfargs < 2) {
	return;
    }
    /*
     * Evaluate the target in fargs[0] 
     */

    mbuff = bp = alloc_lbuf("fun_switch");
    str = fargs[0];
    exec(mbuff, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	&str, cargs, ncargs);
    *bp = '\0';

    /*
     * Loop through the patterns looking for a match 
     */

    for (i = 1; (i < nfargs - 1) && fargs[i] && fargs[i + 1]; i += 2) {
	tbuff = bp = alloc_lbuf("fun_switch.2");
	str = fargs[i];
	exec(tbuff, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	    &str, cargs, ncargs);
	*bp = '\0';
	if (quick_wild(tbuff, mbuff)) {
	    free_lbuf(tbuff);
	    buf = alloc_lbuf("fun_switch");
	    StringCopy(buf, fargs[i + 1]);
	    str = buf;
	    exec(buff, bufc, 0, player, cause,
		EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
	    free_lbuf(buf);
	    free_lbuf(mbuff);
	    return;
	}
	free_lbuf(tbuff);
    }
    free_lbuf(mbuff);

    /*
     * Nope, return the default if there is one 
     */

    if ((i < nfargs) && fargs[i]) {
	buf = alloc_lbuf("fun_switch");
	StringCopy(buf, fargs[i]);
	str = buf;
	exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	    &str, cargs, ncargs);
	free_lbuf(buf);
    }
    return;
}

static void fun_case(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int i;
    char *mbuff, *buf, *bp, *str;

    /*
     * If we don't have at least 2 args, return nothing 
     */

    if (nfargs < 2) {
	return;
    }
    /*
     * Evaluate the target in fargs[0] 
     */

    mbuff = bp = alloc_lbuf("fun_switch");
    str = fargs[0];
    exec(mbuff, &bp, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	&str, cargs, ncargs);
    *bp = '\0';

    /*
     * Loop through the patterns looking for a match 
     */

    for (i = 1; (i < nfargs - 1) && fargs[i] && fargs[i + 1]; i += 2) {
	if (*fargs[i] == *mbuff) {
	    buf = alloc_lbuf("fun_switch");
	    StringCopy(buf, fargs[i + 1]);
	    str = buf;
	    exec(buff, bufc, 0, player, cause,
		EV_STRIP | EV_FCHECK | EV_EVAL, &str, cargs, ncargs);
	    free_lbuf(buf);
	    free_lbuf(mbuff);
	    return;
	}
    }
    free_lbuf(mbuff);

    /*
     * Nope, return the default if there is one 
     */

    if ((i < nfargs) && fargs[i]) {
	buf = alloc_lbuf("fun_switch");
	StringCopy(buf, fargs[i]);
	str = buf;
	exec(buff, bufc, 0, player, cause, EV_STRIP | EV_FCHECK | EV_EVAL,
	    &str, cargs, ncargs);
	free_lbuf(buf);
    }
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_space: Make spaces.
 */

static void fun_space(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int num;
    char *cp;

    if (!fargs[0] || !(*fargs[0])) {
	num = 1;
    } else {
	num = atoi(fargs[0]);
    }

    if (num < 1) {

	/*
	 * If negative or zero spaces return a single space,  * * * * 
	 * -except- allow 'space(0)' to return "" for calculated * *
	 * * * padding 
	 */

	if (!is_integer(fargs[0]) || (num != 0)) {
	    num = 1;
	}
    } else if (num >= LBUF_SIZE) {
	num = LBUF_SIZE - 1;
    }
    for (cp = *bufc; num > 0; num--)
	*cp++ = ' ';
    *bufc = cp;
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * fun_idle, fun_conn: return seconds idle or connected.
 */

static void fun_idle(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref target;

    target = lookup_player(player, fargs[0], 1);
    if (Good_obj(target) && Dark(target) && !Wizard(player))
	target = NOTHING;
    safe_tprintf_str(buff, bufc, "%d", fetch_idle(target));
}

static void fun_conn(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref target;

    target = lookup_player(player, fargs[0], 1);
    if (Good_obj(target) && Dark(target) && !Wizard(player))
	target = NOTHING;
    safe_tprintf_str(buff, bufc, "%d", fetch_connect(target));
}

/*
 * ---------------------------------------------------------------------------
 * * fun_sort: Sort lists.
 */

typedef struct f_record f_rec;
typedef struct i_record i_rec;

struct f_record {
    double data;
    char *str;
};

struct i_record {
    long data;
    char *str;
};

static int a_comp(s1, s2)
const void *s1, *s2;
{
    return strcmp(*(char **) s1, *(char **) s2);
}

static int f_comp(s1, s2)
const void *s1, *s2;
{
    if (((f_rec *) s1)->data > ((f_rec *) s2)->data)
	return 1;
    if (((f_rec *) s1)->data < ((f_rec *) s2)->data)
	return -1;
    return 0;
}

static int i_comp(s1, s2)
const void *s1, *s2;
{
    if (((i_rec *) s1)->data > ((i_rec *) s2)->data)
	return 1;
    if (((i_rec *) s1)->data < ((i_rec *) s2)->data)
	return -1;
    return 0;
}

static void do_asort(s, n, sort_type)
char *s[];
int n, sort_type;
{
    int i;
    f_rec *fp;
    i_rec *ip;

    switch (sort_type) {
    case ALPHANUM_LIST:
	qsort((void *) s, n, sizeof(char *), a_comp);

	break;
    case NUMERIC_LIST:
	ip = (i_rec *) malloc(n * sizeof(i_rec));
	for (i = 0; i < n; i++) {
	    ip[i].str = s[i];
	    ip[i].data = atoi(s[i]);
	}
	qsort((void *) ip, n, sizeof(i_rec), i_comp);
	for (i = 0; i < n; i++) {
	    s[i] = ip[i].str;
	}
	free(ip);
	break;
    case DBREF_LIST:
	ip = (i_rec *) malloc(n * sizeof(i_rec));
	for (i = 0; i < n; i++) {
	    ip[i].str = s[i];
	    ip[i].data = dbnum(s[i]);
	}
	qsort((void *) ip, n, sizeof(i_rec), i_comp);
	for (i = 0; i < n; i++) {
	    s[i] = ip[i].str;
	}
	free(ip);
	break;
    case FLOAT_LIST:
	fp = (f_rec *) malloc(n * sizeof(f_rec));
	for (i = 0; i < n; i++) {
	    fp[i].str = s[i];
	    fp[i].data = atof(s[i]);
	}
	qsort((void *) fp, n, sizeof(f_rec), f_comp);
	for (i = 0; i < n; i++) {
	    s[i] = fp[i].str;
	}
	free(fp);
	break;
    }
}

static void fun_sort(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int nitems, sort_type;
    char *list, sep;
    char *ptrs[LBUF_SIZE / 2];

    /*
     * If we are passed an empty arglist return a null string 
     */

    if (nfargs == 0) {
	return;
    }
    mvarargs_preamble("SORT", 1, 3);

    /*
     * Convert the list to an array 
     */

    list = alloc_lbuf("fun_sort");
    StringCopy(list, fargs[0]);
    nitems = list2arr(ptrs, LBUF_SIZE / 2, list, sep);
    sort_type = get_list_type(fargs, nfargs, 2, ptrs, nitems);
    do_asort(ptrs, nitems, sort_type);
    arr2list(ptrs, nitems, buff, bufc, sep);
    free_lbuf(list);
}

/*
 * ---------------------------------------------------------------------------
 * * fun_setunion, fun_setdiff, fun_setinter: Set management.
 */

#define	SET_UNION	1
#define	SET_INTERSECT	2
#define	SET_DIFF	3

static void handle_sets(fargs, buff, bufc, oper, sep)
char *fargs[], *buff, **bufc, sep;
int oper;
{
    char *list1, *list2, *oldp;
    char *ptrs1[LBUF_SIZE], *ptrs2[LBUF_SIZE];
    int i1, i2, n1, n2, val, first;

    list1 = alloc_lbuf("fun_setunion.1");
    StringCopy(list1, fargs[0]);
    n1 = list2arr(ptrs1, LBUF_SIZE, list1, sep);
    do_asort(ptrs1, n1, ALPHANUM_LIST);

    list2 = alloc_lbuf("fun_setunion.2");
    StringCopy(list2, fargs[1]);
    n2 = list2arr(ptrs2, LBUF_SIZE, list2, sep);
    do_asort(ptrs2, n2, ALPHANUM_LIST);

    i1 = i2 = 0;
    first = 1;
    oldp = *bufc;
    **bufc = '\0';

    switch (oper) {
    case SET_UNION:		/*
				   * Copy elements common to both lists 
				 */

	/*
	 * Handle case of two identical single-element lists 
	 */

	if ((n1 == 1) && (n2 == 1) && (!strcmp(ptrs1[0], ptrs2[0]))) {
	    safe_str(ptrs1[0], buff, bufc);
	    break;
	}
	/*
	 * Process until one list is empty 
	 */

	while ((i1 < n1) && (i2 < n2)) {

	    /*
	     * Skip over duplicates 
	     */

	    if ((i1 > 0) || (i2 > 0)) {
		while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
		    i1++;
		while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
		    i2++;
	    }
	    /*
	     * Compare and copy 
	     */

	    if ((i1 < n1) && (i2 < n2)) {
		if (!first)
		    safe_chr(sep, buff, bufc);
		first = 0;
		oldp = *bufc;
		if (strcmp(ptrs1[i1], ptrs2[i2]) < 0) {
		    safe_str(ptrs1[i1], buff, bufc);
		    i1++;
		} else {
		    safe_str(ptrs2[i2], buff, bufc);
		    i2++;
		}
		**bufc = '\0';
	    }
	}

	/*
	 * Copy rest of remaining list, stripping duplicates 
	 */

	for (; i1 < n1; i1++) {
	    if (strcmp(oldp, ptrs1[i1])) {
		if (!first)
		    safe_chr(sep, buff, bufc);
		first = 0;
		oldp = *bufc;
		safe_str(ptrs1[i1], buff, bufc);
		**bufc = '\0';
	    }
	}
	for (; i2 < n2; i2++) {
	    if (strcmp(oldp, ptrs2[i2])) {
		if (!first)
		    safe_chr(sep, buff, bufc);
		first = 0;
		oldp = *bufc;
		safe_str(ptrs2[i2], buff, bufc);
		**bufc = '\0';
	    }
	}
	break;
    case SET_INTERSECT:	/*
				   * Copy elements not in both lists 
				 */

	while ((i1 < n1) && (i2 < n2)) {
	    val = strcmp(ptrs1[i1], ptrs2[i2]);
	    if (!val) {

		/*
		 * Got a match, copy it 
		 */

		if (!first)
		    safe_chr(sep, buff, bufc);
		first = 0;
		oldp = *bufc;
		safe_str(ptrs1[i1], buff, bufc);
		i1++;
		i2++;
		while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
		    i1++;
		while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
		    i2++;
	    } else if (val < 0) {
		i1++;
	    } else {
		i2++;
	    }
	}
	break;
    case SET_DIFF:		/*
				 * Copy elements unique to list1 
				 */

	while ((i1 < n1) && (i2 < n2)) {
	    val = strcmp(ptrs1[i1], ptrs2[i2]);
	    if (!val) {

		/*
		 * Got a match, increment pointers 
		 */

		oldp = ptrs1[i1];
		while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
		    i1++;
		while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
		    i2++;
	    } else if (val < 0) {

		/*
		 * Item in list1 not in list2, copy 
		 */

		if (!first)
		    safe_chr(sep, buff, bufc);
		first = 0;
		safe_str(ptrs1[i1], buff, bufc);
		oldp = ptrs1[i1];
		i1++;
		while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
		    i1++;
	    } else {

		/*
		 * Item in list2 but not in list1, discard 
		 */

		oldp = ptrs2[i2];
		i2++;
		while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
		    i2++;
	    }
	}

	/*
	 * Copy remainder of list1 
	 */

	while (i1 < n1) {
	    if (!first)
		safe_chr(sep, buff, bufc);
	    first = 0;
	    safe_str(ptrs1[i1], buff, bufc);
	    oldp = ptrs1[i1];
	    i1++;
	    while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
		i1++;
	}
    }
    free_lbuf(list1);
    free_lbuf(list2);
    return;
}

static void fun_setunion(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char sep;

    varargs_preamble("SETUNION", 3);
    handle_sets(fargs, buff, bufc, SET_UNION, sep);
    return;
}

static void fun_setdiff(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char sep;

    varargs_preamble("SETDIFF", 3);
    handle_sets(fargs, buff, bufc, SET_DIFF, sep);
    return;
}

static void fun_setinter(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char sep;

    varargs_preamble("SETINTER", 3);
    handle_sets(fargs, buff, bufc, SET_INTERSECT, sep);
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * rjust, ljust, center: Justify or center text, specifying fill character
 */

static void fun_ljust(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int spaces, i;
    char sep;

    varargs_preamble("LJUST", 3);
    spaces = atoi(fargs[1]) - strlen((char *) strip_ansi(fargs[0]));

    /*
     * Sanitize number of spaces 
     */

    if (spaces <= 0) {
	/*
	 * no padding needed, just return string 
	 */
	safe_str(fargs[0], buff, bufc);
	return;
    } else if (spaces > LBUF_SIZE) {
	spaces = LBUF_SIZE;
    }
    safe_str(fargs[0], buff, bufc);
    for (i = 0; i < spaces; i++)
	safe_chr(sep, buff, bufc);
}

static void fun_rjust(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int spaces, i;
    char sep;

    varargs_preamble("RJUST", 3);
    spaces = atoi(fargs[1]) - strlen((char *) strip_ansi(fargs[0]));

    /*
     * Sanitize number of spaces 
     */

    if (spaces <= 0) {
	/*
	 * no padding needed, just return string 
	 */
	safe_str(fargs[0], buff, bufc);
	return;
    } else if (spaces > LBUF_SIZE) {
	spaces = LBUF_SIZE;
    }
    for (i = 0; i < spaces; i++)
	safe_chr(sep, buff, bufc);
    safe_str(fargs[0], buff, bufc);
}

static void fun_center(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char sep;
    int i, len, lead_chrs, trail_chrs, width;

    varargs_preamble("CENTER", 3);

    width = atoi(fargs[1]);
    len = strlen((char *) strip_ansi(fargs[0]));

    if (width > LBUF_SIZE) {
	safe_str("#-1 OUT OF RANGE", buff, bufc);
	return;
    }

    if (len >= width) {
	safe_str(fargs[0], buff, bufc);
	return;
    }

    lead_chrs = (width / 2) - (len / 2) + .5;
    for (i = 0; i < lead_chrs; i++)
	safe_chr(sep, buff, bufc);
    safe_str(fargs[0], buff, bufc);
    trail_chrs = width - lead_chrs - len;
    for (i = 0; i < trail_chrs; i++)
	safe_chr(sep, buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * setq, setr, r: set and read global registers.
 */

static void fun_setq(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int regnum;

    regnum = atoi(fargs[0]);
    if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
	safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufc);
    } else {
	if (!mudstate.global_regs[regnum])
	    mudstate.global_regs[regnum] = alloc_lbuf("fun_setq");
	StringCopy(mudstate.global_regs[regnum], fargs[1]);
    }
}

static void fun_setr(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int regnum;

    regnum = atoi(fargs[0]);
    if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
	safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufc);
	return;
    } else {
	if (!mudstate.global_regs[regnum])
	    mudstate.global_regs[regnum] = alloc_lbuf("fun_setq");
	StringCopy(mudstate.global_regs[regnum], fargs[1]);
    }
    safe_str(fargs[1], buff, bufc);
}

static void fun_r(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    int regnum;

    regnum = atoi(fargs[0]);
    if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
	safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufc);
    } else if (mudstate.global_regs[regnum]) {
	safe_str(mudstate.global_regs[regnum], buff, bufc);
    }
}

/*
 * ---------------------------------------------------------------------------
 * * isnum: is the argument a number?
 */

static void fun_isnum(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_str((is_number(fargs[0]) ? "1" : "0"), buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * isdbref: is the argument a valid dbref?
 */

static void fun_isdbref(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *p;
    dbref dbitem;

    p = fargs[0];
    if (*p++ == NUMBER_TOKEN) {
	dbitem = parse_dbref(p);
	if (Good_obj(dbitem)) {
	    safe_str("1", buff, bufc);
	    return;
	}
    }
    safe_str("0", buff, bufc);
}

/*
 * ---------------------------------------------------------------------------
 * * trim: trim off unwanted white space.
 */

static void fun_trim(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    char *p, *lastchar, *q, sep;
    int trim;

    if (nfargs == 0) {
	return;
    }
    mvarargs_preamble("TRIM", 1, 3);
    if (nfargs >= 2) {
	switch (ToLower(*fargs[1])) {
	case 'l':
	    trim = 1;
	    break;
	case 'r':
	    trim = 2;
	    break;
	default:
	    trim = 3;
	    break;
	}
    } else {
	trim = 3;
    }

    if (trim == 2 || trim == 3) {
	p = lastchar = fargs[0];
	while (*p != '\0') {
	    if (*p != sep)
		lastchar = p;
	    p++;
	}
	*(lastchar + 1) = '\0';
    }
    q = fargs[0];
    if (trim == 1 || trim == 3) {
	while (*q != '\0') {
	    if (*q == sep)
		q++;
	    else
		break;
	}
    }
    safe_str(q, buff, bufc);
}

#ifdef ARBITRARY_LOGFILES
static void fun_logf(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    if (!fargs[0] || !fargs[1]) {
    	safe_str("#-1 INVALID ARGUMENTS", buff, bufc);
    }
    if (log_to_file(player, fargs[0], fargs[1]))
	return;
	
    safe_str("#-1 INVALID LOGFILE", buff, bufc);
    return;
}
#endif

/* ----------------------------------------------------------------------
** fun_doing: return @doing string for arg.
** arg is either socket or player (Code adopted from Mux2.3)
*/
static void fun_doing(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    if ( is_number( fargs[0] ) )
    {
        int s = atoi( fargs[0] );
        unsigned short bFound = 0;
        DESC *d;
        DESC_ITER_CONN(d)
        {
            if (d->descriptor == s) 
            {
                bFound = 1;
                break;
            }
        }
        if (  bFound
           && (  d->player == player
              || Wizard_Who(player)))
        {
            safe_str(d->doing, buff, bufc);
        }
        else
        {
            /* [cad] Mux2.3 returns empty string here, not sure what
               convention is used here. Just return #-1 for now */
            safe_str("#-1", buff, bufc);
        }
    }
    else
    {
        dbref victim = lookup_player(player, fargs[0], 1);
        if (victim == NOTHING)
        {
            safe_str("#-1 PLAYER DOES NOT EXIST", buff, bufc);
            return;
        }
    
        if (  Wizard_Who(player)
           || !Hidden(victim))
        {
            DESC *d;
            DESC_ITER_CONN(d)
            {
                if (d->player == victim)
                {
                    safe_str(d->doing, buff, bufc);
                    return;
                }
            }
        }
        safe_str("#-1 NOT A CONNECTED PLAYER", buff, bufc);
    }
}


/* ----------------------------------------------------------------------
** fun_poll: return @doing header
** (Code adopted from Mux2.3)
*/
static void fun_poll(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    safe_str(mudstate.doing_hdr, buff, bufc);
}

/* ----------------------------------------------------------------------
 ** fun_pairs: take an attr off an object and count the # of
 ** {[()]} in that attribute and return it as a list
 ** Modified from fun_get
 ** Dany - 06/2005
 */
static void fun_pairs(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags;
    ATTR *attr;
    char *atr_gotten;
    struct boolexp *bool;

    char *tmp_char;
    int right_brace = 0, left_brace = 0, right_square_bracket = 0,
        left_square_bracket = 0, right_parenthesis = 0,
        left_parenthesis = 0;

    if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
	    safe_str("#-1 NO MATCH", buff, bufc);
	    return;
    }
    if (attrib == NOTHING) {
	    return;
    }
    free_buffer = 1;
    attr = atr_num(attrib);	/*
				             * We need the attr's flags for this: 
				             */
    if (!attr) {
	    return;
    }
    if (attr->flags & AF_IS_LOCK) {
	    atr_gotten = atr_get(thing, attrib, &aowner, &aflags);
	    if (Read_attr(player, thing, attr, aowner, aflags)) {
	        bool = parse_boolexp(player, atr_gotten, 1);
	        free_lbuf(atr_gotten);
	        atr_gotten = unparse_boolexp(player, bool);
	        free_boolexp(bool);
	    } else {
	        free_lbuf(atr_gotten);
	        atr_gotten = (char *) "#-1 PERMISSION DENIED";
	    }
	    free_buffer = 0;
    } else {
	    atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
    }

    /*
     * Perform access checks.  c_r_p fills buff with an error message * * 
     * 
     * *  * * if needed. 
     */

    if (check_read_perms(player, thing, attr, aowner, aflags, buff, bufc)) {

        /* Scan through the attribute and count the various brackets */
        for(tmp_char = atr_gotten; *tmp_char; tmp_char++) {
       
            switch(*tmp_char) {
                case '{':
                    left_brace++;
                    break;
                case '[':
                    left_square_bracket++;
                    break;
                case '(':
                    left_parenthesis++;
                    break;
                case '}':
                    right_brace++;
                    break;
                case ']':
                    right_square_bracket++;
                    break;
                case ')':
                    right_parenthesis++;
                    break;
                default:
                    break;
            }

        }

        snprintf(atr_gotten, LBUF_SIZE, "%d %d %d %d %d %d",
                left_brace,
                left_square_bracket,
                left_parenthesis,
                right_parenthesis,
                right_square_bracket,
                right_brace);
	    safe_str(atr_gotten, buff, bufc);
    }

    if (free_buffer)
	    free_lbuf(atr_gotten);
    return;
}

/* ----------------------------------------------------------------------
 ** fun_colorpairs: take an attr off an object and color the
 ** {[()]} in that attribute and return it.
 ** Modified from fun_get
 ** Dany - 09/2005
 */
static void fun_colorpairs(char *buff, char **bufc, dbref player, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags;
    ATTR *attr;
    char *atr_gotten;
    struct boolexp *bool;

    char *tmp_char;
    char tmp_string[LBUF_SIZE];
    char tmp_piece[2];

    if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
	    safe_str("#-1 NO MATCH", buff, bufc);
	    return;
    }
    if (attrib == NOTHING) {
	    return;
    }
    free_buffer = 1;
    attr = atr_num(attrib);	/*
				             * We need the attr's flags for this: 
				             */
    if (!attr) {
	    return;
    }
    if (attr->flags & AF_IS_LOCK) {
	    atr_gotten = atr_get(thing, attrib, &aowner, &aflags);
	    if (Read_attr(player, thing, attr, aowner, aflags)) {
	        bool = parse_boolexp(player, atr_gotten, 1);
	        free_lbuf(atr_gotten);
	        atr_gotten = unparse_boolexp(player, bool);
	        free_boolexp(bool);
	    } else {
	        free_lbuf(atr_gotten);
	        atr_gotten = (char *) "#-1 PERMISSION DENIED";
	    }
	    free_buffer = 0;
    } else {
	    atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
    }

    /*
     * Perform access checks.  c_r_p fills buff with an error message * * 
     * 
     * *  * * if needed. 
     */

    if (check_read_perms(player, thing, attr, aowner, aflags, buff, bufc)) {

        /* zero temporary string */
        memset(tmp_string, 0, sizeof(tmp_string));

        /* Scan through the attribute, colorize what we want */
        for(tmp_char = atr_gotten; *tmp_char; tmp_char++) {
       
            switch(*tmp_char) {
                case '{':
                    strncat(tmp_string, ANSI_RED, LBUF_SIZE);
                    strncat(tmp_string, "{", LBUF_SIZE);
                    strncat(tmp_string, ANSI_NORMAL, LBUF_SIZE);
                    break;
                case '[':
                    strncat(tmp_string, ANSI_YELLOW, LBUF_SIZE);
                    strncat(tmp_string, "[", LBUF_SIZE);
                    strncat(tmp_string, ANSI_NORMAL, LBUF_SIZE);
                    break;
                case '(':
                    strncat(tmp_string, ANSI_GREEN, LBUF_SIZE);
                    strncat(tmp_string, "(", LBUF_SIZE);
                    strncat(tmp_string, ANSI_NORMAL, LBUF_SIZE);
                    break;
                case '}':
                    strncat(tmp_string, ANSI_RED, LBUF_SIZE);
                    strncat(tmp_string, "}", LBUF_SIZE);
                    strncat(tmp_string, ANSI_NORMAL, LBUF_SIZE);
                    break;
                case ']':
                    strncat(tmp_string, ANSI_YELLOW, LBUF_SIZE);
                    strncat(tmp_string, "]", LBUF_SIZE);
                    strncat(tmp_string, ANSI_NORMAL, LBUF_SIZE);
                    break;
                case ')':
                    strncat(tmp_string, ANSI_GREEN, LBUF_SIZE);
                    strncat(tmp_string, ")", LBUF_SIZE);
                    strncat(tmp_string, ANSI_NORMAL, LBUF_SIZE);
                    break;
                default:
                    sprintf(tmp_piece, "%c", *tmp_char);
                    strncat(tmp_string, tmp_piece, LBUF_SIZE);
                    break;
            }

        }

	    safe_str(tmp_string, buff, bufc);
    }

    if (free_buffer)
	    free_lbuf(atr_gotten);
    return;
}
/* *INDENT-OFF* */

/* ---------------------------------------------------------------------------
 * flist: List of existing functions in alphabetical order.
 */

FUN flist[] = {
{"ABS",		fun_abs,	1,  0,		CA_PUBLIC},
{"ACOS",	fun_acos,	1,  0,		CA_PUBLIC},
{"ADD",		fun_add,	0,  FN_VARARGS,	CA_PUBLIC},
{"AFTER",	fun_after,	0,  FN_VARARGS,	CA_PUBLIC},
{"ALPHAMAX",	fun_alphamax,	0,  FN_VARARGS,	CA_PUBLIC},
{"ALPHAMIN",	fun_alphamin,   0,  FN_VARARGS, CA_PUBLIC},
{"AND",		fun_and,	0,  FN_VARARGS,	CA_PUBLIC},
{"ANDFLAGS",	fun_andflags,	2,  0,		CA_PUBLIC},
{"ANSI",        fun_ansi,       2,  0,          CA_PUBLIC},
{"ANSISECURE",	fun_ansi_secure,	-1, 0,	CA_PUBLIC},
{"APOSS",	fun_aposs,	1,  0,		CA_PUBLIC},
{"ART",		fun_art,	1,  0,		CA_PUBLIC},
{"ASIN",	fun_asin,	1,  0,		CA_PUBLIC},
{"ATAN",	fun_atan,	1,  0,		CA_PUBLIC},
{"BEEP",        fun_beep,       0,  0,          CA_BUILDER},
{"BEFORE",	fun_before,	0,  FN_VARARGS,	CA_PUBLIC},
#ifdef BT_ENABLED
{"BTARMORSTATUS",fun_btarmorstatus,2,0,		CA_WIZARD},
{"BTCRITSTATUS",fun_btcritstatus,2,0,		CA_WIZARD},
{"BTDAMAGEMECH",fun_btdamagemech,7, 0, 		CA_WIZARD},
{"BTDAMAGES",	fun_btdamages,	1, 0,		CA_WIZARD},
{"BTDESIGNEX",  fun_btdesignex, 1,  0,		CA_PUBLIC},
{"BTGETCHARVALUE", fun_btgetcharvalue, 3, 0,	CA_WIZARD},
{"BTGETXCODEVALUE", fun_btgetxcodevalue, 2, 0,	CA_WIZARD},
{"BTLAG",       fun_lag,        0,  0,		CA_WIZARD},
{"BTLOADMAP",	fun_btloadmap,	2,  FN_VARARGS,	CA_WIZARD},
{"BTLOADMECH",	fun_btloadmech, 2,  0,		CA_WIZARD},
{"BTMAKEMECHS", fun_btmakemechs,0,  FN_VARARGS,	CA_WIZARD},
{"BTMAKEPILOTROLL", fun_btmakepilotroll, 3, 0,	CA_WIZARD},
{"BTMAPELEV",   fun_btmapelev,  3,  0,		CA_WIZARD},
{"BTMAPTERR",   fun_btmapterr,  3,  0,		CA_WIZARD},
{"BTMAPUNITS",  fun_btmapunits, 0,  FN_VARARGS,     CA_WIZARD},
{"BTMECHFREQS",	fun_btmechfreqs, 1, 0,		CA_WIZARD},
{"BTPARTMATCH",	fun_btpartmatch, 1, 0,		CA_WIZARD},
{"BTPARTNAME",	fun_btpartname, 2, 0,		CA_WIZARD},
{"BTSETARMORSTATUS",fun_btsetarmorstatus,4,0,	CA_WIZARD},
{"BTSETCHARVALUE", fun_btsetcharvalue, 4, 0,	CA_WIZARD},
{"BTSETXCODEVALUE", fun_btsetxcodevalue, 3, 0,	CA_WIZARD},
{"BTSTORES",    fun_btstores,   0, FN_VARARGS,	CA_WIZARD},
{"BTTECHSTATUS",fun_bttechstatus,1, 0,		CA_WIZARD},
{"BTTHRESHOLD", fun_btthreshold, 1, 0,		CA_WIZARD},
{"BTUNDERREPAIR",    fun_btunderrepair,   1, 0, CA_BUILDER},
{"BTWEAPONSTATUS",fun_btweaponstatus,0,FN_VARARGS,	CA_WIZARD},
{"BTADDSTORES",			fun_btaddstores,		3, 0,		CA_WIZARD},
{"BTARMORSTATUS_REF", 		fun_btarmorstatus_ref,		2, 0,		CA_WIZARD},
{"BTCHARLIST",			fun_btcharlist,			1, FN_VARARGS,	CA_WIZARD},
{"BTCRITSLOT",			fun_btcritslot,			0, FN_VARARGS,	CA_WIZARD},
{"BTCRITSLOT_REF",		fun_btcritslot_ref,		0, FN_VARARGS,	CA_WIZARD},
{"BTCRITSTATUS_REF", 		fun_btcritstatus_ref,		2, 0,		CA_WIZARD},
{"BTENGRATE",			fun_btengrate,			1, 0,		CA_WIZARD},
{"BTENGRATE_REF",		fun_btengrate_ref,		1, 0,		CA_WIZARD},
#ifdef BT_ADVANCED_ECON
{"BTFASABASECOST_REF",		fun_btfasabasecost_ref,		1, 0,		CA_WIZARD},
#endif
{"BTGETBV",			fun_btgetbv,			1, 0,		CA_WIZARD},
{"BTGETBV_REF",			fun_btgetbv_ref,		1, 0,		CA_WIZARD},
#ifdef BT_ADVANCED_ECON
{"BTGETPARTCOST",		fun_btgetpartcost,		1, 0,		CA_WIZARD},
#endif
{"BTGETRANGE",			fun_btgetrange,			0, FN_VARARGS,  CA_WIZARD},
{"BTGETREALMAXSPEED",		fun_btgetrealmaxspeed,		1, 0,		CA_WIZARD},
{"BTGETWEIGHT",			fun_btgetweight, 		1, 0,		CA_WIZARD},
{"BTGETXCODEVALUE_REF", 	fun_btgetxcodevalue_ref,	2, 0,		CA_WIZARD},
{"BTHEXEMIT", 			fun_bthexemit, 			4, 0, 		CA_WIZARD},
{"BTHEXINBLZ",			fun_bthexinblz,			3, 0,		CA_WIZARD},
{"BTHEXLOS", 			fun_bthexlos, 			3, 0,		CA_WIZARD},
{"BTID2DB",			fun_btid2db, 			2, 0, 		CA_WIZARD},
{"BTLISTBLZ",			fun_btlistblz,			1, 0,		CA_WIZARD},
{"BTLOSM2M", 			fun_btlosm2m, 			2, 0, 		CA_WIZARD},
{"BTMAPEMIT",			fun_btmapemit,			0, FN_VARARGS,	CA_WIZARD},
{"BTNUMREPJOBS",		fun_btnumrepjobs,		1, 0,		CA_WIZARD},
{"BTPARTTYPE",			fun_btparttype,			1, 0,		CA_WIZARD},
{"BTPARTWEIGHT",		fun_btgetweight, 		1, 0,		CA_WIZARD},
{"BTPAYLOAD_REF",       fun_btpayload_ref,      1, 0,       CA_WIZARD},
{"BTREMOVESTORES",		fun_btremovestores,		3, 0,		CA_WIZARD},
{"BTSETMAXSPEED",		fun_btsetmaxspeed,		2, 0,		CA_WIZARD},
#ifdef BT_ADVANCED_ECON
{"BTSETPARTCOST",		fun_btsetpartcost,		2, 0,		CA_WIZARD},
#endif
{"BTSETXY",			fun_btsetxy,			0, FN_VARARGS, CA_WIZARD},
{"BTSHOWCRITSTATUS_REF",	fun_btshowcritstatus_ref,	3, 0,		CA_WIZARD},
{"BTSHOWSTATUS_REF",		fun_btshowstatus_ref,		2, 0,		CA_WIZARD},
{"BTSHOWWSPECS_REF",		fun_btshowwspecs_ref,		2, 0,		CA_WIZARD},
{"BTTECHLIST",			fun_bttechlist,			1, 0,		CA_WIZARD},
{"BTTECHLIST_REF",		fun_bttechlist_ref,		1, 0,		CA_WIZARD},
{"BTTECHTIME",			fun_bttechtime, 		0, 0,           CA_WIZARD},
{"BTUNITFIXABLE",		fun_btunitfixable,		1, 0,		CA_WIZARD},
{"BTWEAPONSTATUS_REF", 		fun_btweaponstatus_ref,		0, FN_VARARGS,	CA_WIZARD},
{"BTWEAPSTAT",			fun_btweapstat,			2, 0,		CA_WIZARD},
#endif
{"CAPSTR",	fun_capstr,	-1, 0,		CA_PUBLIC},
{"CASE",	fun_case,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"CAT",		fun_cat,	0,  FN_VARARGS,	CA_PUBLIC},
{"CEIL",	fun_ceil,	1,  0,		CA_PUBLIC},
{"CENTER",	fun_center,	0,  FN_VARARGS,	CA_PUBLIC},
{"CHILDREN",    fun_children,   1,  0,          CA_PUBLIC},
{"COBJ",	fun_cobj,	1,  0,		CA_PUBLIC},
{"COLORPAIRS",  fun_colorpairs,	1,  0,		CA_PUBLIC},
{"COLUMNS",	fun_columns,	0,  FN_VARARGS, CA_PUBLIC},
{"COMP",	fun_comp,	2,  0,		CA_PUBLIC},
{"CON",		fun_con,	1,  0,		CA_PUBLIC},
{"CONFIG", 	fun_config,	1,  0,		CA_WIZARD},
{"CONN",	fun_conn,	1,  0,		CA_PUBLIC},
{"CONTROLS", 	fun_controls,	2,  0,		CA_PUBLIC},
{"CONVSECS",    fun_convsecs,   1,  0,		CA_PUBLIC},
{"CONVTIME",    fun_convtime,   1,  0,		CA_PUBLIC},
{"CONVUPTIME",  fun_convuptime, 1,  0,		CA_PUBLIC},
{"COS",		fun_cos,	1,  0,		CA_PUBLIC},
{"CREATE",      fun_create,     0,  FN_VARARGS, CA_PUBLIC},
{"CWHO",        fun_cwho,       1,  0,          CA_PUBLIC},
{"CLIST",	fun_clist,	1,  0,		CA_PUBLIC},
{"CEMIT",	fun_cemit,	2,  0,		CA_PUBLIC},
{"DEC",         fun_dec,        1,  0,          CA_PUBLIC},
{"DECRYPT",     fun_decrypt,    2,  0,          CA_PUBLIC},
{"DEFAULT",	fun_default,	2,  FN_NO_EVAL, CA_PUBLIC},
{"DELETE",	fun_delete,	3,  0,		CA_PUBLIC},
{"DIE",		fun_die,	2,  0,		CA_PUBLIC},
{"DIST2D",	fun_dist2d,	4,  0,		CA_PUBLIC},
{"DIST3D",	fun_dist3d,	6,  0,		CA_PUBLIC},
{"DIV",		fun_div,	2,  0,		CA_PUBLIC},
{"DOING",       fun_doing,      1,  0,          CA_WIZARD},
{"E",		fun_e,		0,  0,		CA_PUBLIC},
{"EDEFAULT",	fun_edefault,	2,  FN_NO_EVAL, CA_PUBLIC},
{"EDIT",	fun_edit,	3,  0,		CA_PUBLIC},
{"ELEMENTS",	fun_elements,	0,  FN_VARARGS,	CA_PUBLIC},
{"ELOCK",	fun_elock,	2,  0,		CA_PUBLIC},
{"EMPTY",	fun_empty,	0,  FN_VARARGS, CA_PUBLIC},
{"ENCRYPT",     fun_encrypt,    2,  0,          CA_PUBLIC},
{"EQ",		fun_eq,		2,  0,		CA_PUBLIC},
{"ESCAPE",	fun_escape,	-1, 0,		CA_PUBLIC},
{"EXIT",	fun_exit,	1,  0,		CA_PUBLIC},
{"EXP",		fun_exp,	1,  0,		CA_PUBLIC},
{"EXTRACT",	fun_extract,	0,  FN_VARARGS,	CA_PUBLIC},
{"EVAL",        fun_eval,       0,  FN_VARARGS, CA_PUBLIC},
{"FDIV",	fun_fdiv,	2,  0,		CA_PUBLIC},
{"FILTER",	fun_filter,	0,  FN_VARARGS,	CA_PUBLIC},
{"FINDABLE",	fun_findable,	2,  0,		CA_PUBLIC},
{"FIRST",	fun_first,	0,  FN_VARARGS,	CA_PUBLIC},
{"FLAGS",	fun_flags,	1,  0,		CA_PUBLIC},
{"FLOOR",	fun_floor,	1,  0,		CA_PUBLIC},
#if 0
{"FNUM2RANK",   fun_fnum2rank,  2,  0,          CA_PUBLIC},
{"FRANK2NUM",   fun_frank2num,  2,  0,          CA_PUBLIC},
#endif
{"FOLD",	fun_fold,	0,  FN_VARARGS,	CA_PUBLIC},
{"FOREACH",	fun_foreach,	0,  FN_VARARGS,	CA_PUBLIC},
{"FULLNAME",	fun_fullname,	1,  0,		CA_PUBLIC},
{"GET",		fun_get,	1,  0,		CA_PUBLIC},
{"GET_EVAL",	fun_get_eval,	1,  0,		CA_PUBLIC},
{"GRAB",	fun_grab,	0,  FN_VARARGS,	CA_PUBLIC},
{"GREP",	fun_grep,	3,  0,		CA_PUBLIC},
{"GREPI",	fun_grepi,	3,  0,		CA_PUBLIC},
{"GT",		fun_gt,		2,  0,		CA_PUBLIC},
{"GTE",		fun_gte,	2,  0,		CA_PUBLIC},
{"HASATTR",	fun_hasattr,	2,  0,		CA_PUBLIC},
{"HASATTRP",	fun_hasattrp,	2,  0,		CA_PUBLIC},
{"HASFLAG",	fun_hasflag,	2,  0,		CA_PUBLIC},
{"HASPOWER",    fun_haspower,   2,  0,          CA_PUBLIC},
{"HASTYPE",	fun_hastype,	2,  0,		CA_PUBLIC},
{"HOME",	fun_home,	1,  0,		CA_PUBLIC},
{"IDLE",	fun_idle,	1,  0,		CA_PUBLIC},
{"IFELSE",      fun_ifelse,     3,  FN_NO_EVAL,          CA_PUBLIC},
{"INC",         fun_inc,        1,  0,          CA_PUBLIC},
{"INDEX",	fun_index,	4,  0,		CA_PUBLIC},
{"INSERT",	fun_insert,	0,  FN_VARARGS,	CA_PUBLIC},
{"INZONE",      fun_inzone,     1,  0,          CA_PUBLIC},
{"ISDBREF",	fun_isdbref,	1,  0,		CA_PUBLIC},
{"ISNUM",	fun_isnum,	1,  0,		CA_PUBLIC},
{"ISWORD",	fun_isword,	1,  0,		CA_PUBLIC},
{"ITEMS",	fun_items,	0,  FN_VARARGS, CA_PUBLIC},
{"ITER",	fun_iter,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"LAST",	fun_last,	0,  FN_VARARGS,	CA_PUBLIC},
{"LATTR",	fun_lattr,	1,  0,		CA_PUBLIC},
{"LCON",	fun_lcon,	1,  0,		CA_PUBLIC},
{"LCSTR",	fun_lcstr,	-1, 0,		CA_PUBLIC},
{"LDELETE",	fun_ldelete,	0,  FN_VARARGS,	CA_PUBLIC},
{"LEXITS",	fun_lexits,	1,  0,		CA_PUBLIC},
{"LPARENT",	fun_lparent,	1,  0,		CA_PUBLIC}, 
{"LIST",	fun_list,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC}, 
{"LIT",		fun_lit,	1,  FN_NO_EVAL,	CA_PUBLIC},
{"LJUST",	fun_ljust,	0,  FN_VARARGS,	CA_PUBLIC},
{"LINK",	fun_link,	2,  0,		CA_PUBLIC},
{"LN",		fun_ln,		1,  0,		CA_PUBLIC},
{"LNUM",	fun_lnum,	0,  FN_VARARGS,	CA_PUBLIC},
{"LOC",		fun_loc,	1,  0,		CA_BUILDER},
{"LOCATE",	fun_locate,	3,  0,		CA_BUILDER},
{"LOCK",	fun_lock,	1,  0,		CA_PUBLIC},
{"LOG",		fun_log,	1,  0,		CA_PUBLIC},
#ifdef ARBITRARY_LOGFILES
{"LOGF",	fun_logf,	2,  0,		CA_WIZARD},
#endif
{"LSTACK",	fun_lstack,	0,  FN_VARARGS, CA_PUBLIC},
{"LT",		fun_lt,		2,  0,		CA_PUBLIC},
{"LTE",		fun_lte,	2,  0,		CA_PUBLIC},
{"LWHO",	fun_lwho,	0,  0,		CA_BUILDER},
{"MAIL",        fun_mail,       0,  FN_VARARGS, CA_PUBLIC},
{"MAILFROM",    fun_mailfrom,   0,  FN_VARARGS, CA_PUBLIC},
{"MAP",		fun_map,	0,  FN_VARARGS,	CA_PUBLIC},
{"MATCH",	fun_match,	0,  FN_VARARGS,	CA_PUBLIC},
{"MATCHALL",	fun_matchall,	0,  FN_VARARGS,	CA_PUBLIC},
{"MAX",		fun_max,	0,  FN_VARARGS,	CA_PUBLIC},
{"MEMBER",	fun_member,	0,  FN_VARARGS,	CA_PUBLIC},
{"MERGE",	fun_merge,	3,  0,		CA_PUBLIC},
{"MID",		fun_mid,	3,  0,		CA_PUBLIC},
{"MIN",		fun_min,	0,  FN_VARARGS,	CA_PUBLIC},
{"MIX",		fun_mix,	0,  FN_VARARGS,	CA_PUBLIC},
{"MOD",		fun_mod,	2,  0,		CA_PUBLIC},
{"MONEY",	fun_money,	1,  0,		CA_PUBLIC},
{"MUDNAME",	fun_mudname,	0,  0,		CA_PUBLIC},
{"MUL",		fun_mul,	0,  FN_VARARGS,	CA_PUBLIC},
{"MUNGE",	fun_munge,	0,  FN_VARARGS,	CA_PUBLIC},
{"NAME",	fun_name,	1,  0,		CA_PUBLIC},
{"NEARBY",	fun_nearby,	2,  0,		CA_PUBLIC},
{"NEQ",		fun_neq,	2,  0,		CA_PUBLIC},
{"NEXT",	fun_next,	1,  0,		CA_PUBLIC},
{"NOT",		fun_not,	1,  0,		CA_PUBLIC},
{"NUM",		fun_num,	1,  0,		CA_PUBLIC},
{"OBJ",		fun_obj,	1,  0,		CA_PUBLIC},
{"OBJEVAL",     fun_objeval,    2,  FN_NO_EVAL, CA_PUBLIC},
{"OBJMEM",	fun_objmem,	1,  0,		CA_PUBLIC},
{"OR",		fun_or,		0,  FN_VARARGS,	CA_PUBLIC},
{"ORFLAGS",	fun_orflags,	2,  0,		CA_PUBLIC},
{"OWNER",	fun_owner,	1,  0,		CA_PUBLIC},
{"PAIRS",   fun_pairs,	1,  0,		CA_PUBLIC},
{"PARENT",	fun_parent,	1,  0,		CA_PUBLIC},
{"PARSE",	fun_parse,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"PEEK",	fun_peek,	0,  FN_VARARGS, CA_PUBLIC},
{"PEMIT",	fun_pemit,	2,  0,		CA_PUBLIC},
{"PI",		fun_pi,		0,  0,		CA_PUBLIC},
{"PLAYMEM",	fun_playmem,	1,  0,		CA_PUBLIC},
{"PMATCH",	fun_pmatch,	1,  0,		CA_PUBLIC},
{"POLL",        fun_poll,       0,  0,          CA_WIZARD},
{"POP",		fun_pop,	0,  FN_VARARGS, CA_PUBLIC},
{"PORTS",	fun_ports,	1,  0,		CA_PUBLIC},
{"POS",		fun_pos,	2,  0,		CA_PUBLIC},
{"POSS",	fun_poss,	1,  0,		CA_PUBLIC},
{"POWER",	fun_power,	2,  0,		CA_PUBLIC},
{"PUSH",	fun_push,	0,  FN_VARARGS, CA_PUBLIC},
#ifdef USE_PYTHON
{"PYTHON",	fun_python,	1,  FN_NO_EVAL,	CA_WIZARD},
{"PYTHONCALL",	fun_pythoncall,	0,  FN_VARARGS,	CA_WIZARD},
#endif
{"R",		fun_r,		1,  0,		CA_PUBLIC},
{"RAND",	fun_rand,	1,  0,		CA_PUBLIC},
{"REGMATCH",    fun_regmatch,   0,  FN_VARARGS, CA_PUBLIC},
{"REMOVE",	fun_remove,	0,  FN_VARARGS,	CA_PUBLIC},
{"REPEAT",	fun_repeat,	2,  0,		CA_PUBLIC},
{"REPLACE",	fun_replace,	0,  FN_VARARGS,	CA_PUBLIC},
{"REST",	fun_rest,	0,  FN_VARARGS,	CA_PUBLIC},
{"REVERSE",	fun_reverse,	-1, 0,		CA_PUBLIC},
{"REVWORDS",	fun_revwords,	0,  FN_VARARGS,	CA_PUBLIC},
{"RJUST",	fun_rjust,	0,  FN_VARARGS,	CA_PUBLIC},
{"RLOC",	fun_rloc,	2,  0,		CA_PUBLIC},
{"ROOM",	fun_room,	1,  0,		CA_PUBLIC},
{"ROUND",	fun_round,	2,  0,		CA_PUBLIC},
{"S",		fun_s,		-1, 0,		CA_PUBLIC},
{"SCRAMBLE",	fun_scramble,	1,  0,		CA_PUBLIC},
{"SEARCH",	fun_search,	-1, 0,		CA_PUBLIC},
{"SECS",	fun_secs,	0,  0,		CA_PUBLIC},
{"SECURE",	fun_secure,	-1, 0,		CA_PUBLIC},
{"SET",		fun_set,	2,  0,		CA_PUBLIC},
{"SETDIFF",	fun_setdiff,	0,  FN_VARARGS,	CA_PUBLIC},
{"SETINTER",	fun_setinter,	0,  FN_VARARGS,	CA_PUBLIC},
{"SETLOCK",	fun_setlock,	3,  0,		CA_WIZARD},
{"SETQ",	fun_setq,	2,  0,		CA_PUBLIC},
{"SETR",	fun_setr,	2,  0,		CA_PUBLIC},
{"SETUNION",	fun_setunion,	0,  FN_VARARGS,	CA_PUBLIC},
{"SHL",		fun_shl,	2,  0,		CA_PUBLIC},
{"SHR",		fun_shr,	2,  0,		CA_PUBLIC},
{"SHUFFLE",	fun_shuffle,	0,  FN_VARARGS,	CA_PUBLIC},
{"SIGN",	fun_sign,	1,  0,		CA_PUBLIC},
{"SIN",		fun_sin,	1,  0,		CA_PUBLIC},
{"SORT",	fun_sort,	0,  FN_VARARGS,	CA_PUBLIC},
{"SORTBY",	fun_sortby,	0,  FN_VARARGS, CA_PUBLIC},
{"SPACE",	fun_space,	1,  0,		CA_PUBLIC},
{"SPLICE",	fun_splice,	0,  FN_VARARGS,	CA_PUBLIC},
{"SQRT",	fun_sqrt,	1,  0,		CA_PUBLIC},
{"SQUISH",	fun_squish,	1,  0,		CA_PUBLIC},
{"STARTTIME",	fun_starttime,	0,  0,		CA_PUBLIC},
{"STATS",	fun_stats,	1,  0,		CA_PUBLIC},
{"STRCAT",	fun_strcat,	0,  FN_VARARGS,	CA_PUBLIC},
{"STRIPANSI",	fun_stripansi,	1,  0,		CA_PUBLIC},
{"STRLEN",	fun_strlen,	-1, 0,		CA_PUBLIC},
{"STRMATCH",	fun_strmatch,	2,  0,		CA_PUBLIC},
{"STRTRUNC",    fun_strtrunc,   2,  0,          CA_PUBLIC},
{"SUB",		fun_sub,	2,  0,		CA_PUBLIC},
{"SUBEVAL",  	fun_subeval,	1,  0,		CA_PUBLIC},
{"SUBJ",	fun_subj,	1,  0,		CA_PUBLIC},
{"SWITCH",	fun_switch,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"TAN",		fun_tan,	1,  0,		CA_PUBLIC},
{"T",           fun_t,          1,  0,          CA_PUBLIC},
{"TEL",		fun_tel,	2,  0,		CA_PUBLIC},
{"TIME",	fun_time,	0,  0,		CA_PUBLIC},
{"TRANSLATE",   fun_translate,  2,  0,          CA_PUBLIC},
{"TRIM",	fun_trim,	0,  FN_VARARGS,	CA_PUBLIC},
{"TRUE",        fun_t,          1,  0,          CA_PUBLIC},
{"TRUNC",	fun_trunc,	1,  0,		CA_PUBLIC},
{"TYPE",	fun_type,	1,  0,		CA_PUBLIC},
{"U",		fun_u,		0,  FN_VARARGS,	CA_PUBLIC},
{"UCSTR",	fun_ucstr,	-1, 0,		CA_PUBLIC},
{"UDEFAULT",	fun_udefault,	0,  FN_VARARGS|FN_NO_EVAL,
						CA_PUBLIC},
{"ULOCAL",	fun_ulocal,	0,  FN_VARARGS,	CA_PUBLIC},
{"V",		fun_v,		1,  0,		CA_PUBLIC},
{"VADD",	fun_vadd,	0,  FN_VARARGS,	CA_PUBLIC},
{"VALID",	fun_valid,	2,  FN_VARARGS, CA_PUBLIC},
{"VDIM",	fun_vdim,	0,  FN_VARARGS,	CA_PUBLIC},
{"VERSION",	fun_version,	0,  0,		CA_PUBLIC},
{"VISIBLE",	fun_visible,	2,  0,		CA_PUBLIC},
{"VMAG",	fun_vmag,	0,  FN_VARARGS,	CA_PUBLIC},
{"VMUL",	fun_vmul,	0,  FN_VARARGS,	CA_PUBLIC},
{"VSUB",	fun_vsub,	0,  FN_VARARGS,	CA_PUBLIC},
{"VUNIT",	fun_vunit,	0,  FN_VARARGS,	CA_PUBLIC},
{"WHERE",	fun_where,	1,  0,		CA_PUBLIC},
{"WORDPOS",     fun_wordpos,    0,  FN_VARARGS,	CA_PUBLIC},
{"WORDS",	fun_words,	0,  FN_VARARGS,	CA_PUBLIC},
{"XGET",	fun_xget,	2,  0,		CA_PUBLIC},
{"XOR",		fun_xor,	0,  FN_VARARGS,	CA_PUBLIC},
{"ZFUN",	fun_zfun,	0,  FN_VARARGS,	CA_PUBLIC},
{"ZONE",        fun_zone,       1,  0,          CA_PUBLIC},
{"ZPLAYERS",    fun_zplayers,   1,  0,          CA_PUBLIC},
{"ZWHO",        fun_zwho,       1,  0,          CA_PUBLIC},
{NULL,		NULL,		0,  0,		0}};

/* *INDENT-ON* */





void init_functab() {
    FUN *fp;
    char *buff, *cp, *dp;

    buff = alloc_sbuf("init_functab");
    hashinit(&mudstate.func_htab, 100 * HASH_FACTOR);
    for (fp = flist; fp->name; fp++) {
	cp = (char *) fp->name;
	dp = buff;
	while (*cp) {
	    *dp = ToLower(*cp);
	    cp++;
	    dp++;
	}
	*dp = '\0';
	hashadd(buff, (int *) fp, &mudstate.func_htab);
    }
    free_sbuf(buff);
    ufun_head = NULL;
    hashinit(&mudstate.ufunc_htab, 11);
}

void do_function(player, cause, key, fname, target)
dbref player, cause;
int key;
char *fname, *target;
{
    UFUN *ufp, *ufp2;
    ATTR *ap;
    char *np, *bp;
    int atr, aflags;
    dbref obj, aowner;

    /*
     * Make a local uppercase copy of the function name 
     */

    bp = np = alloc_sbuf("add_user_func");
    safe_sb_str(fname, np, &bp);
    *bp = '\0';
    for (bp = np; *bp; bp++)
	*bp = ToLower(*bp);

    /*
     * Verify that the function doesn't exist in the builtin table 
     */

    if (hashfind(np, &mudstate.func_htab) != NULL) {
	notify_quiet(player,
	    "Function already defined in builtin function table.");
	free_sbuf(np);
	return;
    }
    /*
     * Make sure the target object exists 
     */

    if (!parse_attrib(player, target, &obj, &atr)) {
	notify_quiet(player, "I don't see that here.");
	free_sbuf(np);
	return;
    }
    /*
     * Make sure the attribute exists 
     */

    if (atr == NOTHING) {
	notify_quiet(player, "No such attribute.");
	free_sbuf(np);
	return;
    }
    /*
     * Make sure attribute is readably by me 
     */

    ap = atr_num(atr);
    if (!ap) {
	notify_quiet(player, "No such attribute.");
	free_sbuf(np);
	return;
    }
    atr_get_info(obj, atr, &aowner, &aflags);
    if (!See_attr(player, obj, ap, aowner, aflags)) {
	notify_quiet(player, "Permission denied.");
	free_sbuf(np);
	return;
    }
    /*
     * Privileged functions require you control the obj.  
     */

    if ((key & FN_PRIV) && !Controls(player, obj)) {
	notify_quiet(player, "Permission denied.");
	free_sbuf(np);
	return;
    }
    /*
     * See if function already exists.  If so, redefine it 
     */

    ufp = (UFUN *) hashfind(np, &mudstate.ufunc_htab);

    if (!ufp) {
	ufp = (UFUN *) malloc(sizeof(UFUN));
	ufp->name = strsave(np);
	for (bp = (char *) ufp->name; *bp; bp++)
	    *bp = ToUpper(*bp);
	ufp->obj = obj;
	ufp->atr = atr;
	ufp->perms = CA_PUBLIC;
	ufp->next = NULL;
	if (!ufun_head) {
	    ufun_head = ufp;
	} else {
	    for (ufp2 = ufun_head; ufp2->next; ufp2 = ufp2->next);
	    ufp2->next = ufp;
	}
	hashadd(np, (int *) ufp, &mudstate.ufunc_htab);
    }
    ufp->obj = obj;
    ufp->atr = atr;
    ufp->flags = key;
    free_sbuf(np);
    if (!Quiet(player))
	notify_quiet(player, tprintf("Function %s defined.", fname));
}

/*
 * ---------------------------------------------------------------------------
 * * list_functable: List available functions.
 */

void list_functable(dbref player)
{
    FUN *fp;
    UFUN *ufp;
    char *buf, *bp, *cp;

    buf = alloc_lbuf("list_functable");
    bp = buf;

    /* Hardcoded Functions */
    for (cp = (char *) "Functions:"; *cp; cp++)
	*bp++ = *cp;
    for (fp = flist; fp->name; fp++) {
	if (check_access(player, fp->perms)) {
	    *bp++ = ' ';
	    for (cp = (char *) (fp->name); *cp; cp++)
		*bp++ = *cp;
	}
    }
    *bp = '\0';
    notify(player, buf);

    /* User-Defined functions (via @function) */
    bp = buf;
    safe_str("User-Functions:", buf, &bp);

    for (ufp = ufun_head; ufp; ufp = ufp->next) {
	if (check_access(player, ufp->perms)) {
	    *bp++ = ' ';
	    for (cp = (char *) (ufp->name); *cp; cp++)
		*bp++ = *cp;
	}
    }
    *bp = '\0';
    notify(player, buf);
    free_lbuf(buf);
}

/*
 * ---------------------------------------------------------------------------
 * * cf_func_access: set access on functions
 */

int cf_func_access(int *vp, char *str, long extra, dbref player, char *cmd)
{
    FUN *fp;
    UFUN *ufp;
    char *ap;

    for (ap = str; *ap && !isspace(*ap); ap++);
    if (*ap)
	*ap++ = '\0';

    for (fp = flist; fp->name; fp++) {
	if (!string_compare(fp->name, str)) {
	    return (cf_modify_bits(&fp->perms, ap, extra, player, cmd));
	}
    }
    for (ufp = ufun_head; ufp; ufp = ufp->next) {
	if (!string_compare(ufp->name, str)) {
	    return (cf_modify_bits(&ufp->perms, ap, extra, player, cmd));
	}
    }
    cf_log_notfound(player, cmd, "Function", str);
    return -1;
}
