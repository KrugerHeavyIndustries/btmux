/*
 * db.c 
 */

#include "copyright.h"
#include "config.h"

#include <sys/file.h>
#include <sys/stat.h>

#define __DB_C
#include "mudconf.h"
#include "config.h"
#include "externs.h"
#include "db.h"
#include "attrs.h"
#include "vattr.h"
#include "match.h"
#include "alloc.h"
#include "powers.h"
#include "interface.h"
#include "flags.h"
#include "p.comsys.h"
#include "mmdb.h"

#ifndef O_ACCMODE
#define O_ACCMODE	(O_RDONLY|O_WRONLY|O_RDWR)
#endif

/*
 * Restart definitions 
 */
#define RS_CONCENTRATE		0x00000002
#define RS_RECORD_PLAYERS	0x00000004
#define RS_NEW_STRINGS		0x00000008
#define RS_HUDKEY		0x00000010

OBJ *db = NULL;
NAME *names = NULL;
NAME *purenames = NULL;

int corrupt;

extern void desc_addhash(DESC *);
extern void del_commac(dbref);
extern void do_clear_macro(dbref player, char *s);

#ifdef TEST_MALLOC
int malloc_count = 0;

#endif /*
	    * * TEST_MALLOC  
	    */

extern VATTR *vattr_rename(char *, char *);

typedef struct atrcount ATRCOUNT;
struct atrcount {
	dbref thing;
	int count;
};

/*
 * #define GNU_MALLOC_TEST 1 
 */

#ifdef GNU_MALLOC_TEST
extern unsigned int malloc_sbrk_used;	/* Amount of data space used now */
#endif

/*
 * Check routine forward declaration. 
 */
extern int fwdlist_ck(int, dbref, dbref, int, char *);

extern void pcache_reload(dbref);
extern void desc_reload(dbref);

// Macro for the flags for character stats/skills attributes.
#define PLSTAT_MODE (AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL)

/*
 * list of attributes 
 */
ATTR attr[] = {
	{"Aahear", A_AAHEAR, AF_ODARK, NULL},
	{"Aclone", A_ACLONE, AF_ODARK, NULL},
	{"Aconnect", A_ACONNECT, AF_ODARK, NULL},
	{"Adesc", A_ADESC, AF_ODARK, NULL},
	{"Adfail", A_ADFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Adisconnect", A_ADISCONNECT, AF_ODARK, NULL},
	{"Adrop", A_ADROP, AF_ODARK, NULL},
	{"Aefail", A_AEFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Aenter", A_AENTER, AF_ODARK, NULL},
	{"Afail", A_AFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Agfail", A_AGFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Ahear", A_AHEAR, AF_ODARK, NULL},
	{"Akill", A_AKILL, AF_ODARK, NULL},
	{"Aleave", A_ALEAVE, AF_ODARK, NULL},
	{"Alfail", A_ALFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Alias", A_ALIAS, AF_NOPROG | AF_NOCMD | AF_GOD, NULL},
	{"Allowance", A_ALLOWANCE, AF_MDARK | AF_NOPROG | AF_WIZARD, NULL},
	{"Amail", A_AMAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Amhear", A_AMHEAR, AF_ODARK, NULL},
	{"Amove", A_AMOVE, AF_ODARK, NULL},
	{"Apay", A_APAY, AF_ODARK, NULL},
	{"Arfail", A_ARFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Asucc", A_ASUCC, AF_ODARK, NULL},
	{"Atfail", A_ATFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Atport", A_ATPORT, AF_ODARK | AF_NOPROG, NULL},
	{"Atofail", A_ATOFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Aufail", A_AUFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Ause", A_AUSE, AF_ODARK, NULL},
	{"Away", A_AWAY, AF_ODARK | AF_NOPROG, NULL},
	{"Buildcoord", A_BUILDCOORD, AF_MDARK | AF_WIZARD, NULL},
	{"Buildentrance", A_BUILDENTRANCE, AF_MDARK | AF_WIZARD, NULL},
	{"Buildlinks", A_BUILDLINKS, AF_MDARK | AF_WIZARD, NULL},
	{"Charges", A_CHARGES, AF_ODARK | AF_NOPROG, NULL},
	{"Comment", A_COMMENT, AF_MDARK | AF_WIZARD, NULL},
	{"Contactoptions", A_CONTACTOPT, AF_ODARK, NULL},
	{"Cost", A_COST, AF_ODARK, NULL},
	{"Daily", A_DAILY, AF_ODARK, NULL},
	{"HHourly", A_HOURLY, AF_MDARK, NULL},
	{"Desc", A_DESC, AF_NOPROG, NULL},
	{"DefaultLock", A_LOCK, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
	 NULL},
	{"Destroyer", A_DESTROYER, AF_MDARK | AF_WIZARD | AF_NOPROG, NULL},
	{"Dfail", A_DFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Drop", A_DROP, AF_ODARK | AF_NOPROG, NULL},
	{"DropLock", A_LDROP, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
	 NULL},
	{"Ealias", A_EALIAS, AF_ODARK | AF_NOPROG, NULL},
	{"Efail", A_EFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Enter", A_ENTER, AF_ODARK, NULL},
	{"EnterLock", A_LENTER, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
	 NULL},
	{"Faction", A_FACTION, AF_MDARK | AF_WIZARD, NULL},
	{"Fail", A_FAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Filter", A_FILTER, AF_ODARK | AF_NOPROG, NULL},
	{"Forwardlist", A_FORWARDLIST, AF_ODARK | AF_NOPROG, fwdlist_ck},
	{"Gfail", A_GFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"GiveLock", A_LGIVE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
	{"Idesc", A_IDESC, AF_ODARK | AF_NOPROG, NULL},
	{"Idle", A_IDLE, AF_ODARK | AF_NOPROG, NULL},
	{"Infilter", A_INFILTER, AF_ODARK | AF_NOPROG, NULL},
	{"Inprefix", A_INPREFIX, AF_ODARK | AF_NOPROG, NULL},
	{"Job", A_JOB, AF_MDARK | AF_WIZARD, NULL},
	{"Kill", A_KILL, AF_ODARK, NULL},
	{"Lalias", A_LALIAS, AF_ODARK | AF_NOPROG, NULL},
	{"Last", A_LAST, AF_WIZARD | AF_NOCMD | AF_NOPROG, NULL},
	{"Lastname", A_LASTNAME, AF_WIZARD | AF_NOPROG | AF_MDARK, NULL},
	{"Lastpage", A_LASTPAGE, AF_INTERNAL | AF_NOCMD | AF_NOPROG | AF_GOD | AF_PRIVATE, NULL},
	{"Lastsite", A_LASTSITE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_GOD, NULL},
	{"Leave", A_LEAVE, AF_ODARK, NULL},
	{"LeaveLock", A_LLEAVE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
	{"Lfail", A_LFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"LinkLock", A_LLINK, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
	{"Listen", A_LISTEN, AF_ODARK, NULL},

	{"Logindata", A_LOGINDATA, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
	{"LRSheight", A_LRSHEIGHT, AF_ODARK, NULL},
	{"Unused1", A_UNUSED1, AF_WIZARD | AF_MDARK, NULL},
	{"Mailcurf", A_MAILCURF, AF_MDARK | AF_WIZARD | AF_NOPROG, NULL},
	{"Mailflags", A_MAILFLAGS, AF_MDARK | AF_WIZARD | AF_NOPROG, NULL},
	{"Mailfolders", A_MAILFOLDERS, AF_MDARK | AF_WIZARD | AF_NOPROG, NULL},
	{"Mailmsg", A_MAILMSG, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
	{"Mailsub", A_MAILSUB, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
	{"Mailsucc", A_MAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Mailto", A_MAILTO, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
	{"Mapcolor", A_MAPCOLOR, AF_ODARK, NULL},
	{"Mapvis", A_MAPVIS, AF_MDARK | AF_WIZARD, NULL},
	{"Mechdesc", A_MECHDESC, AF_MDARK, NULL},
	{"Mechname", A_MECHNAME, AF_MDARK, NULL},
	{"Mechstatus", A_MECHSTATUS, AF_MDARK | AF_WIZARD, NULL},
	{"Mechtype", A_MECHTYPE, AF_MDARK, NULL},
	{"MechPrefID", A_MECHPREFID, AF_MDARK | AF_WIZARD, NULL},
	{"Mechskills", A_MECHSKILLS, AF_MDARK, NULL},
	{"Mwtemplate", A_MWTEMPLATE, AF_MDARK, NULL},
	{"Move", A_MOVE, AF_ODARK, NULL},
	{"Name", A_NAME, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
	{"Odesc", A_ODESC, AF_ODARK | AF_NOPROG, NULL},
	{"Odfail", A_ODFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Odrop", A_ODROP, AF_ODARK | AF_NOPROG, NULL},
	{"Oefail", A_OEFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Oenter", A_OENTER, AF_ODARK, NULL},
	{"Ofail", A_OFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Ogfail", A_OGFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Okill", A_OKILL, AF_ODARK, NULL},
	{"Amechdest", A_AMECHDEST, AF_MDARK, NULL},
	{"Aminetrigger", A_AMINETRIGGER, AF_MDARK, NULL},
	{"Oleave", A_OLEAVE, AF_ODARK, NULL},
	{"Olfail", A_OLFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Omove", A_OMOVE, AF_ODARK, NULL},
	{"Opay", A_OPAY, AF_ODARK, NULL},
	{"Orfail", A_ORFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Osucc", A_OSUCC, AF_ODARK | AF_NOPROG, NULL},
	{"Otfail", A_OTFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Otport", A_OTPORT, AF_ODARK | AF_NOPROG, NULL},
	{"Otofail", A_OTOFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Oufail", A_OUFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Ouse", A_OUSE, AF_ODARK, NULL},
	{"Oxenter", A_OXENTER, AF_ODARK, NULL},
	{"Oxleave", A_OXLEAVE, AF_ODARK, NULL},
	{"Oxtport", A_OXTPORT, AF_ODARK | AF_NOPROG, NULL},
	{"PageLock", A_LPAGE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
	{"ParentLock", A_LPARENT, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
	{"Pay", A_PAY, AF_ODARK, NULL},
	{"PCequip", A_PCEQUIP, AF_MDARK, NULL},
	{"Pilot", A_PILOTNUM, AF_MDARK, NULL},
	{"Prefix", A_PREFIX, AF_ODARK | AF_NOPROG, NULL},
	{"ProgCmd", A_PROGCMD, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL, NULL},
	{"QueueMax", A_QUEUEMAX, AF_MDARK | AF_WIZARD | AF_NOPROG, NULL},
	{"Quota", A_QUOTA, AF_MDARK | AF_NOPROG | AF_GOD | AF_NOCMD, NULL},
	{"Ranknum", A_RANKNUM, AF_MDARK | AF_WIZARD, NULL},
	{"ReceiveLock", A_LRECEIVE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK, NULL},
	{"Reject", A_REJECT, AF_ODARK | AF_NOPROG, NULL},
	{"Rfail", A_RFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Rquota", A_RQUOTA, AF_MDARK | AF_NOPROG | AF_GOD | AF_NOCMD, NULL},
	{"Runout", A_RUNOUT, AF_ODARK, NULL},

	{"Semaphore", A_SEMAPHORE,
	 AF_ODARK | AF_NOPROG | AF_WIZARD | AF_NOCMD, NULL},
	{"Sex", A_SEX, AF_NOPROG, NULL},
	{"Signature", A_SIGNATURE, AF_ODARK | AF_NOPROG, NULL},
	{"SpeechLock", A_LSPEECH, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
	 NULL},
	{"Startup", A_STARTUP, AF_ODARK, NULL},
	{"Succ", A_SUCC, AF_ODARK | AF_NOPROG, NULL},
	{"Tacsize", A_TACSIZE, AF_ODARK, NULL},
	{"TeloutLock", A_LTELOUT, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
	 NULL},
	{"Tfail", A_TFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Timeout", A_TIMEOUT, AF_MDARK | AF_NOPROG | AF_WIZARD, NULL},
	{"Tport", A_TPORT, AF_ODARK | AF_NOPROG, NULL},
	{"TportLock", A_LTPORT, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
	 NULL},
	{"Tofail", A_TOFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Tz", A_TZ, AF_NOPROG, NULL},
	{"Ufail", A_UFAIL, AF_ODARK | AF_NOPROG, NULL},
	{"Use", A_USE, AF_ODARK, NULL},
	{"UseLock", A_LUSE, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
	 NULL},
	{"UserLock", A_LUSER, AF_ODARK | AF_NOPROG | AF_NOCMD | AF_IS_LOCK,
	 NULL},
	{"VA", A_VA, AF_ODARK, NULL},
	{"VB", A_VA + 1, AF_ODARK, NULL},
	{"VC", A_VA + 2, AF_ODARK, NULL},
	{"VD", A_VA + 3, AF_ODARK, NULL},
	{"VE", A_VA + 4, AF_ODARK, NULL},
	{"VF", A_VA + 5, AF_ODARK, NULL},
	{"VG", A_VA + 6, AF_ODARK, NULL},
	{"VH", A_VA + 7, AF_ODARK, NULL},
	{"VI", A_VA + 8, AF_ODARK, NULL},
	{"VJ", A_VA + 9, AF_ODARK, NULL},
	{"VK", A_VA + 10, AF_ODARK, NULL},
	{"VL", A_VA + 11, AF_ODARK, NULL},
	{"VM", A_VA + 12, AF_ODARK, NULL},
	{"VN", A_VA + 13, AF_ODARK, NULL},
	{"VO", A_VA + 14, AF_ODARK, NULL},
	{"VP", A_VA + 15, AF_ODARK, NULL},
	{"VQ", A_VA + 16, AF_ODARK, NULL},
	{"VR", A_VA + 17, AF_ODARK, NULL},
	{"VS", A_VA + 18, AF_ODARK, NULL},
	{"VT", A_VA + 19, AF_ODARK, NULL},
	{"VU", A_VA + 20, AF_ODARK, NULL},
	{"VV", A_VA + 21, AF_ODARK, NULL},
	{"VW", A_VA + 22, AF_ODARK, NULL},
	{"VX", A_VA + 23, AF_ODARK, NULL},
	{"VY", A_VA + 24, AF_ODARK, NULL},
	{"VZ", A_VA + 25, AF_ODARK, NULL},
	{"Xtype", A_XTYPE, AF_MDARK | AF_WIZARD, NULL},
	{"*Password", A_PASS, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL,
	 NULL},
	{"*Privileges", A_PRIVS, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL,
	 NULL},
	{"*Money", A_MONEY, AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL,
	 NULL},
	{"Techtime", A_TECHTIME, AF_MDARK | AF_WIZARD,
	 NULL},
	{"*EconParts", A_ECONPARTS,
	 AF_DARK | AF_NOPROG | AF_NOCMD | AF_INTERNAL,
	 NULL},
	{"PLHEALTH", A_HEALTH, PLSTAT_MODE, NULL},
	{"PLATTRS", A_ATTRS, PLSTAT_MODE, NULL},
	{"PLADVS", A_ADVS, PLSTAT_MODE, NULL},
	{"PLSKILLS", A_SKILLS, PLSTAT_MODE, NULL},

	{NULL, 0, 0, NULL}
};

/*
 * ---------------------------------------------------------------------------
 * * fwdlist_set, fwdlist_clr: Manage cached forwarding lists
 */
void fwdlist_set(dbref thing, FWDLIST * ifp)
{
	FWDLIST *fp, *xfp;
	int i;

	/*
	 * If fwdlist is null, clear 
	 */

	if(!ifp || (ifp->count <= 0)) {
		fwdlist_clr(thing);
		return;
	}
	/*
	 * Copy input forwardlist to a correctly-sized buffer 
	 */

	fp = (FWDLIST *) XMALLOC(sizeof(int) * ((ifp->count) + 1), "fwdlist_set");

	for(i = 0; i < ifp->count; i++) {
		fp->data[i] = ifp->data[i];
	}
	fp->count = ifp->count;

	/*
	 * Replace an existing forwardlist, or add a new one 
	 */

	xfp = fwdlist_get(thing);
	if(xfp) {
		XFREE(xfp, "fwdlist_set");
		nhashrepl(thing, (int *) fp, &mudstate.fwdlist_htab);
	} else {
		nhashadd(thing, (int *) fp, &mudstate.fwdlist_htab);
	}
}

void fwdlist_clr(dbref thing)
{
	FWDLIST *xfp;

	/*
	 * If a forwardlist exists, delete it 
	 */

	xfp = fwdlist_get(thing);
	if(xfp) {
		XFREE(xfp, "fwdlist_clr");
		nhashdelete(thing, &mudstate.fwdlist_htab);
	}
}

/*
 * ---------------------------------------------------------------------------
 * * fwdlist_load: Load text into a forwardlist.
 */
int fwdlist_load(FWDLIST * fp, dbref player, char *atext)
{
	dbref target;
	char *tp, *bp, *dp;
	int count, errors, fail;

	count = 0;
	errors = 0;
	bp = tp = alloc_lbuf("fwdlist_load.str");
	StringCopy(tp, atext);
	
	do {
		for(; *bp && isspace(*bp); bp++);	/*
							 * skip spaces 
							 */
		for(dp = bp; *bp && !isspace(*bp); bp++);	/*
								 * remember string
								 */
		if(*bp)
			*bp++ = '\0';		                /*
								 * terminate string 
								 */
		if((*dp++ == '#') && isdigit(*dp)) {
			target = atoi(dp);
			fail = (!Good_obj(target) || (!God(player) &&
			    !controls(player, target) && (!Link_ok(target) || !could_doit(player, target,
			    A_LLINK))));
			if(fail) {
				notify_printf(player,
				    "Cannot forward to #%d: Permission denied.", target);
				errors++;
			} else {
				fp->data[count++] = target;
			}
		}
	} while (*bp);
	
	free_lbuf(tp);
	fp->count = count;
	return errors;
}

/*
 * ---------------------------------------------------------------------------
 * * fwdlist_rewrite: Generate a text string from a FWDLIST buffer.
 */
int fwdlist_rewrite(FWDLIST * fp, char *atext)
{
	char *tp, *bp;
	int i, count;

	if(fp && fp->count) {
		count = fp->count;
		tp = alloc_sbuf("fwdlist_rewrite.errors");
		bp = atext;
		for(i = 0; i < fp->count; i++) {
			if(Good_obj(fp->data[i])) {
				sprintf(tp, "#%d ", fp->data[i]);
				safe_str(tp, atext, &bp);
			} else {
				count--;
			}
		}
		*bp = '\0';
		free_sbuf(tp);
	} else {
		count = 0;
		*atext = '\0';
	}
	return count;
}

/*
 * ---------------------------------------------------------------------------
 * * fwdlist_ck:  Check a list of dbref numbers to forward to for AUDIBLE
 */

int fwdlist_ck(int key, dbref player, dbref thing, int anum, char *atext)
{
	FWDLIST *fp;
	int count;

	count = 0;

	if(atext && *atext) {
		fp = (FWDLIST *) alloc_lbuf("fwdlist_ck.fp");
		fwdlist_load(fp, player, atext);
	} else {
		fp = NULL;
	}

	/*
	 * Set the cached forwardlist 
	 */

	fwdlist_set(thing, fp);
	count = fwdlist_rewrite(fp, atext);
	if(fp)
		free_lbuf(fp);
	return ((count > 0) || !atext || !*atext);
}

FWDLIST *fwdlist_get(dbref thing)
{
	FWDLIST *fp;

	fp = ((FWDLIST *) nhashfind(thing, &mudstate.fwdlist_htab));

	return fp;
}

static char *set_string(char **ptr, char *new)
{
	/*
	 * if pointer not null unalloc it 
	 */

	if(*ptr)
		XFREE(*ptr, "set_string");

	/*
	 * if new string is not null allocate space for it and copy it 
	 */

	if(!new)					/*
								 * * || !*new  
								 */
		return (*ptr = NULL);	/*
								 * Check with GAC about this 
								 */
	*ptr = (char *) XMALLOC(strlen(new) + 1, "set_string");
	StringCopy(*ptr, new);
	return (*ptr);
}

/*
 * ---------------------------------------------------------------------------
 * * Name, s_Name: Get or set an object's name.
 */
INLINE char *Name(dbref thing)
{
	dbref aowner;
	int aflags;
	char *buff;
	static char *tbuff[MBUF_SIZE];
	char buffer[MBUF_SIZE];

	if(mudconf.cache_names) {
		if(thing > mudstate.db_top || thing < 0) {
			return "#-1 INVALID DBREF";
		}
		if(!purenames[thing]) {
			buff = atr_get(thing, A_NAME, &aowner, &aflags);
			strip_ansi_r(buffer, buff, MBUF_SIZE);
			set_string(&purenames[thing], buffer);
			free_lbuf(buff);
		}
	}

	atr_get_str((char *) tbuff, thing, A_NAME, &aowner, &aflags);
	return ((char *) tbuff);
}

INLINE char *PureName(dbref thing)
{
	dbref aowner;
	int aflags;
	char *buff;
	static char *tbuff[LBUF_SIZE];
	char new[LBUF_SIZE];

	if(mudconf.cache_names) {
		if(thing > mudstate.db_top || thing < 0) {
			return "#-1 INVALID DBREF";
		}
		if(!purenames[thing]) {
			buff = atr_get(thing, A_NAME, &aowner, &aflags);
			strncpy(new, buff, LBUF_SIZE-1);
			set_string(&purenames[thing], strip_ansi_r(new,buff,strlen(buff)));
			free_lbuf(buff);
		}
		return purenames[thing];
	}

	atr_get_str((char *) tbuff, thing, A_NAME, &aowner, &aflags);
	strncpy(new, (char *) tbuff,LBUF_SIZE-1);
	return (strip_ansi_r(new,(char *) tbuff,strlen((char *) tbuff)));
}

INLINE void s_Name(dbref thing, char *s)
{
	char new[MBUF_SIZE];
	/* Truncate the name if we have to */

	strncpy(new,s,MBUF_SIZE-1);
	if(s && (strlen(s) > MBUF_SIZE))
		s[MBUF_SIZE] = '\0';

	atr_add_raw(thing, A_NAME, (char *) s);

	if(mudconf.cache_names) {

		set_string(&purenames[thing], strip_ansi_r(new,s,strlen(s)));
	}
}

void s_Pass(dbref thing, const char *s)
{
	atr_add_raw(thing, A_PASS, (char *) s);
}

/*
 * ---------------------------------------------------------------------------
 * * do_attrib: Manage user-named attributes.
 */

extern NAMETAB attraccess_nametab[];

void do_attribute(dbref player, dbref cause, int key, char *aname,
				  char *value)
{
	int success, negate, f;
	char *buff, *sp, *p, *q;
	VATTR *va;
	ATTR *va2;

	/*
	 * Look up the user-named attribute we want to play with 
	 */

	buff = alloc_sbuf("do_attribute");
	for(p = buff, q = aname; *q && ((p - buff) < (SBUF_SIZE - 1)); p++, q++)
		*p = ToUpper(*q);
	*p = '\0';

	va = (VATTR *) vattr_find(buff);
	if(!va) {
		notify(player, "No such user-named attribute.");
		free_sbuf(buff);
		return;
	}
	switch (key) {
	case ATTRIB_ACCESS:

		/*
		 * Modify access to user-named attribute 
		 */

		for(sp = value; *sp; sp++)
			*sp = ToUpper(*sp);
		sp = strtok(value, " ");
		success = 0;
		while (sp != NULL) {

			/*
			 * Check for negation 
			 */

			negate = 0;
			if(*sp == '!') {
				negate = 1;
				sp++;
			}
			/*
			 * Set or clear the appropriate bit 
			 */

			f = search_nametab(player, attraccess_nametab, sp);
			if(f > 0) {
				success = 1;
				if(negate)
					va->flags &= ~f;
				else
					va->flags |= f;
			} else {
				notify_printf(player, "Unknown permission: %s.", sp);
			}

			/*
			 * Get the next token 
			 */

			sp = strtok(NULL, " ");
		}
		if(success && !Quiet(player))
			notify(player, "Attribute access changed.");
		break;

	case ATTRIB_RENAME:

		/*
		 * Make sure the new name doesn't already exist 
		 */

		va2 = atr_str(value);
		if(va2) {
			notify(player, "An attribute with that name already exists.");
			free_sbuf(buff);
			return;
		}
		if(vattr_rename(va->name, value) == NULL)
			notify(player, "Attribute rename failed.");
		else
			notify(player, "Attribute renamed.");
		break;

	case ATTRIB_DELETE:

		/*
		 * Remove the attribute 
		 */

		vattr_delete(buff);
		notify(player, "Attribute deleted.");
		break;
	}
	free_sbuf(buff);
	return;
}

/*
 * ---------------------------------------------------------------------------
 * * do_fixdb: Directly edit database fields
 */

void do_fixdb(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
	dbref thing, res;

	init_match(player, arg1, NOTYPE);
	match_everything(0);
	thing = noisy_match_result();
	if(thing == NOTHING)
		return;

	res = NOTHING;
	switch (key) {
	case FIXDB_OWNER:
	case FIXDB_LOC:
	case FIXDB_CON:
	case FIXDB_EXITS:
	case FIXDB_NEXT:
		init_match(player, arg2, NOTYPE);
		match_everything(0);
		res = noisy_match_result();
		break;
	case FIXDB_PENNIES:
		res = atoi(arg2);
		break;
	}

	switch (key) {
	case FIXDB_OWNER:
		s_Owner(thing, res);
		if(!Quiet(player))
			notify_printf(player, "Owner set to #%d", res);
		break;
	case FIXDB_LOC:
		s_Location(thing, res);
		if(!Quiet(player))
			notify_printf(player, "Location set to #%d", res);
		break;
	case FIXDB_CON:
		s_Contents(thing, res);
		if(!Quiet(player))
			notify_printf(player, "Contents set to #%d", res);
		break;
	case FIXDB_EXITS:
		s_Exits(thing, res);
		if(!Quiet(player))
			notify_printf(player, "Exits set to #%d", res);
		break;
	case FIXDB_NEXT:
		s_Next(thing, res);
		if(!Quiet(player))
			notify_printf(player, "Next set to #%d", res);
		break;
	case FIXDB_PENNIES:
		s_Pennies(thing, res);
		if(!Quiet(player))
			notify_printf(player, "Pennies set to %d", res);
		break;
	case FIXDB_NAME:
		if(Typeof(thing) == TYPE_PLAYER) {
			if(!ok_player_name(arg2)) {
				notify(player, "That's not a good name for a player.");
				return;
			}
			if(lookup_player(NOTHING, arg2, 0) != NOTHING) {
				notify(player, "That name is already in use.");
				return;
			}
			STARTLOG(LOG_SECURITY, "SEC", "CNAME") {
				log_name(thing), log_text((char *) " renamed to ");
				log_text(arg2);
				ENDLOG;
			} if(Suspect(player)) {
				send_channel("Suspect", "%s renamed to %s",
							 Name(thing), arg2);
			}
			delete_player_name(thing, Name(thing));

			s_Name(thing, arg2);
			add_player_name(thing, arg2);
		} else {
			if(!ok_name(arg2)) {
				notify(player, "Warning: That is not a reasonable name.");
			}
			s_Name(thing, arg2);
		}
		if(!Quiet(player))
			notify_printf(player, "Name set to %s", arg2);
		break;
	}
}

/*
 * ---------------------------------------------------------------------------
 * * init_attrtab: Initialize the attribute hash tables.
 */

void init_attrtab(void)
{
	ATTR *a;
	char *buff, *p, *q;

	hashinit(&mudstate.attr_name_htab, 512);
	buff = alloc_sbuf("init_attrtab");
	for(a = attr; a->number; a++) {
		anum_extend(a->number);
		anum_set(a->number, a);
		for(p = buff, q = (char *) a->name; *q; p++, q++)
			*p = ToUpper(*q);
		*p = '\0';
		hashadd(buff, (int *) a, &mudstate.attr_name_htab);
	}
	free_sbuf(buff);
}

/*
 * ---------------------------------------------------------------------------
 * * atr_str: Look up an attribute by name.
 */

ATTR *atr_str(char *s)
{
	char *buff, *p, *q;
	ATTR *a;
	VATTR *va;
	static ATTR tattr;

	if(!s || !*s) {
		return (NULL);
	}

	/*
	 * Convert the buffer name to lowercase 
	 */

	buff = alloc_sbuf("atr_str");
	for(p = buff, q = s; *q && ((p - buff) < (SBUF_SIZE - 1)); p++, q++)
		*p = ToUpper(*q);
	*p = '\0';

	/*
	 * Look for a predefined attribute 
	 */

	a = (ATTR *) hashfind(buff, &mudstate.attr_name_htab);
	if(a != NULL) {
		free_sbuf(buff);
		return a;
	}
	/*
	 * Nope, look for a user attribute 
	 */

	va = (VATTR *) vattr_find(buff);
	free_sbuf(buff);

	/*
	 * If we got one, load tattr and return a pointer to it. 
	 */

	if(va != NULL) {
		tattr.name = va->name;
		tattr.number = va->number;
		tattr.flags = va->flags;
		tattr.check = NULL;
		return &tattr;
	}
	/*
	 * All failed, return NULL 
	 */

	return NULL;
}

/*
 * ---------------------------------------------------------------------------
 * * anum_extend: Grow the attr num lookup table.
 */

ATTR **anum_table = NULL;
int anum_alc_top = 0;

void anum_extend(int newtop)
{
	ATTR **anum_table2;
	int delta, i;

	delta = mudconf.init_size;

	if(newtop <= anum_alc_top)
		return;
	if(newtop < anum_alc_top + delta)
		newtop = anum_alc_top + delta;
	if(anum_table == NULL) {
		anum_table = (ATTR **) malloc((newtop + 1) * sizeof(ATTR *));
		for(i = 0; i <= newtop; i++)
			anum_table[i] = NULL;
	} else {
		anum_table2 = (ATTR **) malloc((newtop + 1) * sizeof(ATTR *));
		for(i = 0; i <= anum_alc_top; i++)
			anum_table2[i] = anum_table[i];
		for(i = anum_alc_top + 1; i <= newtop; i++)
			anum_table2[i] = NULL;
		free((char *) anum_table);
		anum_table = anum_table2;
	}
	anum_alc_top = newtop;
}

/*
 * ---------------------------------------------------------------------------
 * * atr_num: Look up an attribute by number.
 */

ATTR *atr_num(int anum)
{
	VATTR *va;
	static ATTR tattr;

	/*
	 * Look for a predefined attribute 
	 */

	if(anum < A_USER_START)
		return anum_get(anum);

	if(anum >= anum_alc_top)
		return NULL;

	/*
	 * It's a user-defined attribute, we need to copy data 
	 */

	va = (VATTR *) anum_get(anum);
	if(va != NULL) {
		tattr.name = va->name;
		tattr.number = va->number;
		tattr.flags = va->flags;
		tattr.check = NULL;
		return &tattr;
	}
	/*
	 * All failed, return NULL 
	 */

	return NULL;
}

/*
 * ---------------------------------------------------------------------------
 * * mkattr: Lookup attribute by name, creating if needed.
 */

int mkattr(char *buff)
{
	ATTR *ap;
	VATTR *va;

	if(!(ap = atr_str(buff))) {

		/*
		 * Unknown attr, create a new one 
		 */

		va = vattr_alloc(buff, mudconf.vattr_flags);
		if(!va || !(va->number))
			return -1;
		return va->number;
	}
	if(!(ap->number))
		return -1;
	return ap->number;
}

/*
 * ---------------------------------------------------------------------------
 * * Commer: check if an object has any $-commands in its attributes.
 */

int Commer(dbref thing)
{
	char *s, *as, c;
	int attr, aflags;
	dbref aowner;
	ATTR *ap;

	for(attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
		ap = atr_num(attr);
		if(!ap || (ap->flags & AF_NOPROG))
			continue;

		s = atr_get(thing, attr, &aowner, &aflags);
		c = *s;
		free_lbuf(s);
		if((c == '$') && !(aflags & AF_NOPROG)) {
			return 1;
		}
	}
	return 0;
}

/*
 * routines to handle object attribute lists 
 */

/*
 * ---------------------------------------------------------------------------
 * * atr_encode: Encode an attribute string.
 */
static char *atr_encode(char *iattr, dbref thing, dbref owner, int flags,
						int atr, char *dest_buffer)
{

	/*
	 * If using the default owner and flags (almost all attributes will),
	 * * * * * * * just store the string. 
	 */

	if(((owner == Owner(thing)) || (owner == NOTHING)) && !flags) {
        memset(dest_buffer, 0, LBUF_SIZE);
        strncpy(dest_buffer, iattr, LBUF_SIZE-1);
		return dest_buffer;
    }

	/*
	 * Encode owner and flags into the attribute text 
	 */

	if(owner == NOTHING)
		owner = Owner(thing);
    memset(dest_buffer, 0, LBUF_SIZE);
    snprintf(dest_buffer, LBUF_SIZE - 1, "%c%d:%d:%s", ATR_INFO_CHAR, owner, flags, iattr);
    return dest_buffer;
}

/*
 * ---------------------------------------------------------------------------
 * * atr_decode: Decode an attribute string.
 */

static void atr_decode(char *iattr, char *oattr, dbref thing, dbref * owner,
					   int *flags, int atr)
{
	char *cp;
	int neg;

	/*
	 * See if the first char of the attribute is the special character 
	 */

	if(*iattr == ATR_INFO_CHAR) {

		/*
		 * It is, crack the attr apart 
		 */

		cp = &iattr[1];

		/*
		 * Get the attribute owner 
		 */

		*owner = 0;
		neg = 0;
		if(*cp == '-') {
			neg = 1;
			cp++;
		}
		while (isdigit(*cp)) {
			*owner = (*owner * 10) + (*cp++ - '0');
		}
		if(neg)
			*owner = 0 - *owner;

		/*
		 * If delimiter is not ':', just return attribute 
		 */

		if(*cp++ != ':') {
			*owner = Owner(thing);
			*flags = 0;
			if(oattr) {
				StringCopy(oattr, iattr);
			}
			return;
		}
		/*
		 * Get the attribute flags 
		 */

		*flags = 0;
		while (isdigit(*cp)) {
			*flags = (*flags * 10) + (*cp++ - '0');
		}

		/*
		 * If delimiter is not ':', just return attribute 
		 */

		if(*cp++ != ':') {
			*owner = Owner(thing);
			*flags = 0;
			if(oattr) {
				StringCopy(oattr, iattr);
			}
		}
		/*
		 * Get the attribute text 
		 */

		if(oattr) {
			StringCopy(oattr, cp);
		}
		if(*owner == NOTHING)
			*owner = Owner(thing);
	} else {

		/*
		 * Not the special character, return normal info 
		 */

		*owner = Owner(thing);
		*flags = 0;
		if(oattr) {
			StringCopy(oattr, iattr);
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * * atr_clr: clear an attribute in the list.
 */

void atr_clr(dbref thing, int atr)
{
	ATRLIST *list;
	int hi, lo, mid;

	if(!db[thing].at_count || !db[thing].ahead)
		return;

	if(db[thing].at_count < 0)
		abort();

	/*
	 * Binary search for the attribute. 
	 */
	lo = 0;
	hi = db[thing].at_count - 1;
	list = db[thing].ahead;
	while (lo <= hi) {
		mid = ((hi - lo) >> 1) + lo;
		if(list[mid].number == atr) {
			free(list[mid].data);
			db[thing].at_count -= 1;
			if(mid != db[thing].at_count)
				bcopy((char *) (list + mid + 1), (char *) (list + mid),
					  (db[thing].at_count - mid) * sizeof(ATRLIST));
			break;
		} else if(list[mid].number > atr) {
			hi = mid - 1;
		} else {
			lo = mid + 1;
		}
	}

	switch (atr) {
	case A_STARTUP:
		s_Flags(thing, Flags(thing) & ~HAS_STARTUP);
		break;
	case A_DAILY:
		s_Flags2(thing, Flags2(thing) & ~HAS_DAILY);
		break;
	case A_HOURLY:
		s_Flags2(thing, Flags2(thing) & ~HAS_HOURLY);
		break;
	case A_FORWARDLIST:
		s_Flags2(thing, Flags2(thing) & ~HAS_FWDLIST);
		break;
	case A_LISTEN:
		s_Flags2(thing, Flags2(thing) & ~HAS_LISTEN);
		break;
	case A_TIMEOUT:
		desc_reload(thing);
		break;
	case A_QUEUEMAX:
		pcache_reload(thing);
		break;
	}
}

/*
 * ---------------------------------------------------------------------------
 * * atr_add_raw, atr_add: add attribute of type atr to list
 */

void atr_add_raw(dbref thing, int atr, char *buff)
{
	ATRLIST *list;
	char *text;
	int found = 0;
	int hi, lo, mid;

	if(!buff || !*buff) {
		atr_clr(thing, atr);
		return;
	}
	if(strlen(buff) >= LBUF_SIZE) {
		buff[LBUF_SIZE - 1] = '\0';
	}
	if((text = (char *) malloc(strlen(buff) + 1)) == NULL) {
		return;
	}
	StringCopy(text, buff);

	if(!db[thing].ahead) {
		if((list = (ATRLIST *) malloc(sizeof(ATRLIST))) == NULL) {
			free(text);
			return;
		}
		db[thing].ahead = list;
		db[thing].at_count = 1;
		list[0].number = atr;
		list[0].data = text;
		list[0].size = strlen(text) + 1;
		found = 1;
	} else {

		/*
		 * Binary search for the attribute 
		 */
		lo = 0;
		hi = db[thing].at_count - 1;

		list = db[thing].ahead;
		while (lo <= hi) {
			mid = ((hi - lo) >> 1) + lo;
			if(list[mid].number == atr) {
				free(list[mid].data);
				list[mid].data = text;
				list[mid].size = strlen(text) + 1;
				found = 1;
				break;
			} else if(list[mid].number > atr) {
				hi = mid - 1;
			} else {
				lo = mid + 1;
			}
		}

		if(!found) {
			/*
			 * If we got here, we didn't find it, so lo = hi + 1, 
			 * and the attribute should be inserted between them. 
			 */

			list =
				(ATRLIST *) realloc(db[thing].ahead,
									(db[thing].at_count +
									 1) * sizeof(ATRLIST));

			if(!list)
				return;

			/*
			 * Move the stuff upwards one slot 
			 */
			if(lo < db[thing].at_count)
				bcopy((char *) (list + lo), (char *) (list + lo + 1),
					  (db[thing].at_count - lo) * sizeof(ATRLIST));

			list[lo].data = text;
			list[lo].number = atr;
			list[lo].size = strlen(text) + 1;
			db[thing].at_count++;
			db[thing].ahead = list;
		}
	}

	switch (atr) {
	case A_STARTUP:
		s_Flags(thing, Flags(thing) | HAS_STARTUP);
		break;
	case A_DAILY:
		s_Flags2(thing, Flags2(thing) | HAS_DAILY);
		break;
	case A_HOURLY:
		s_Flags2(thing, Flags2(thing) | HAS_HOURLY);
		break;
	case A_FORWARDLIST:
		s_Flags2(thing, Flags2(thing) | HAS_FWDLIST);
		break;
	case A_LISTEN:
		s_Flags2(thing, Flags2(thing) | HAS_LISTEN);
		break;
	case A_TIMEOUT:
		desc_reload(thing);
		break;
	case A_QUEUEMAX:
		pcache_reload(thing);
		break;
	}
}

void atr_add(dbref thing, int atr, char *buff, dbref owner, int flags)
{
	char *tbuff;
    char buffer[LBUF_SIZE];

	if(!buff || !*buff) {
		atr_clr(thing, atr);
	} else {
		tbuff = atr_encode(buff, thing, owner, flags, atr, buffer);
		atr_add_raw(thing, atr, tbuff);
	}
}

void atr_set_owner(dbref thing, int atr, dbref owner)
{
	dbref aowner;
	int aflags;
	char *buff;

	buff = atr_get(thing, atr, &aowner, &aflags);
	atr_add(thing, atr, buff, owner, aflags);
	free_lbuf(buff);
}

void atr_set_flags(dbref thing, int atr, dbref flags)
{
	dbref aowner;
	int aflags;
	char *buff;

	buff = atr_get(thing, atr, &aowner, &aflags);
	atr_add(thing, atr, buff, aowner, flags);
	free_lbuf(buff);
}

/*
 * ---------------------------------------------------------------------------
 * * get_atr,atr_get_raw, atr_get_str, atr_get: Get an attribute from the database.
 */

int get_atr(char *name)
{
	ATTR *ap;

	if(!(ap = atr_str(name)))
		return 0;
	if(!(ap->number))
		return -1;
	return ap->number;
}

char *atr_get_raw(dbref thing, int atr)
{
	int lo, mid, hi;
	ATRLIST *list;

	if(thing < 0)
		return NULL;

	/*
	 * Binary search for the attribute 
	 */
	lo = 0;
	hi = db[thing].at_count - 1;
	list = db[thing].ahead;
	if(!list)
		return NULL;

	while (lo <= hi) {
		mid = ((hi - lo) >> 1) + lo;
		if(list[mid].number == atr) {

			return list[mid].data;
		} else if(list[mid].number > atr) {
			hi = mid - 1;
		} else {
			lo = mid + 1;
		}
	}
	return NULL;
}

char *atr_get_str(char *s, dbref thing, int atr, dbref * owner, int *flags)
{
	char *buff;

	buff = atr_get_raw(thing, atr);
	if(!buff) {
		*owner = Owner(thing);
		*flags = 0;
		*s = '\0';
	} else {
		atr_decode(buff, s, thing, owner, flags, atr);
	}
	return s;
}

char *atr_get(dbref thing, int atr, dbref * owner, int *flags)
{
	char *buff;

	buff = alloc_lbuf("atr_get");
	return atr_get_str(buff, thing, atr, owner, flags);
}

int atr_get_info(dbref thing, int atr, dbref * owner, int *flags)
{
	char *buff;

	buff = atr_get_raw(thing, atr);
	if(!buff) {
		*owner = Owner(thing);
		*flags = 0;
		return 0;
	}
	atr_decode(buff, NULL, thing, owner, flags, atr);
	return 1;
}

char *atr_pget_str(char *s, dbref thing, int atr, dbref * owner, int *flags)
{
	char *buff;
	dbref parent;
	int lev;

	ATTR *ap;

	ITER_PARENTS(thing, parent, lev) {
		buff = atr_get_raw(parent, atr);
		if(buff && *buff) {
			atr_decode(buff, s, thing, owner, flags, atr);
			if((lev == 0) || !(*flags & AF_PRIVATE))
				return s;
		}
		if((lev == 0) && Good_obj(Parent(parent))) {
			ap = atr_num(atr);
			if(!ap || ap->flags & AF_PRIVATE)
				break;
		}
	}
	*owner = Owner(thing);
	*flags = 0;
	*s = '\0';
	return s;
}

char *atr_pget(dbref thing, int atr, dbref * owner, int *flags)
{
	char *buff;

	buff = alloc_lbuf("atr_pget");
	return atr_pget_str(buff, thing, atr, owner, flags);
}

int atr_pget_info(dbref thing, int atr, dbref * owner, int *flags)
{
	char *buff;
	dbref parent;
	int lev;
	ATTR *ap;

	ITER_PARENTS(thing, parent, lev) {
		buff = atr_get_raw(parent, atr);
		if(buff && *buff) {
			atr_decode(buff, NULL, thing, owner, flags, atr);
			if((lev == 0) || !(*flags & AF_PRIVATE))
				return 1;
		}
		if((lev == 0) && Good_obj(Parent(parent))) {
			ap = atr_num(atr);
			if(!ap || ap->flags & AF_PRIVATE)
				break;
		}
	}
	*owner = Owner(thing);
	*flags = 0;
	return 0;
}

/*
 * ---------------------------------------------------------------------------
 * * atr_free: Return all attributes of an object.
 */

void atr_free(dbref thing)
{
	free(db[thing].ahead);
	db[thing].ahead = NULL;
}

/*
 * garbage collect an attribute list 
 */
void atr_collect(dbref thing)
{
	/*
	 * Nada.  gdbm takes care of us.  I hope ;-) 
	 */
}

/*
 * ---------------------------------------------------------------------------
 * * atr_cpy: Copy all attributes from one object to another.  Takes the
 * * player argument to ensure that only attributes that COULD be set by
 * * the player are copied.
 */

void atr_cpy(dbref player, dbref dest, dbref source)
{
	int attr, aflags;
	dbref owner, aowner;
	char *as, *buf;
	ATTR *at;

	owner = Owner(dest);
	for(attr = atr_head(source, &as); attr; attr = atr_next(&as)) {
		buf = atr_get(source, attr, &aowner, &aflags);
		if(!(aflags & AF_LOCK))
			aowner = owner;		/*
								 * chg owner 
								 */
		at = atr_num(attr);
		if(attr && at) {
			if(Write_attr(owner, dest, at, aflags))
				/*
				 * Only set attrs that owner has perm to set 
				 */
				atr_add(dest, attr, buf, aowner, aflags);
		}
		free_lbuf(buf);
	}
}

/*
 * ---------------------------------------------------------------------------
 * * atr_chown: Change the ownership of the attributes of an object to the
 * * current owner if they are not locked.
 */

void atr_chown(dbref obj)
{
	int attr, aflags;
	dbref owner, aowner;
	char *as, *buf;

	owner = Owner(obj);
	for(attr = atr_head(obj, &as); attr; attr = atr_next(&as)) {
		buf = atr_get(obj, attr, &aowner, &aflags);
		if((aowner != owner) && !(aflags & AF_LOCK))
			atr_add(obj, attr, buf, owner, aflags);
		free_lbuf(buf);
	}
}

/*
 * ---------------------------------------------------------------------------
 * * atr_next: Return next attribute in attribute list.
 */

int atr_next(char **attrp)
{
	ATRCOUNT *atr;

	if(!attrp || !*attrp) {
		return 0;
	} else {
		atr = (ATRCOUNT *) * attrp;
		if(atr->count > db[atr->thing].at_count) {
			free(atr);
			return 0;
		}
		atr->count++;
		return db[atr->thing].ahead[atr->count - 2].number;
	}
}

/*
 * ---------------------------------------------------------------------------
 * * atr_head: Returns the head of the attr list for object 'thing'
 */

int atr_head(dbref thing, char **attrp)
{
	ATRCOUNT *atr;

	if(db[thing].at_count) {
		atr = (ATRCOUNT *) malloc(sizeof(ATRCOUNT));
		atr->thing = thing;
		atr->count = 2;
		*attrp = (char *) atr;
		return db[thing].ahead[0].number;
	}
	return 0;
}

/*
 * ---------------------------------------------------------------------------
 * * db_grow: Extend the struct database.
 */

#define	SIZE_HACK	1			/*
								 * * So mistaken refs to #-1 won't die.  
								 */

void initialize_objects(dbref first, dbref last)
{
	dbref thing;

	for(thing = first; thing < last; thing++) {
        memset(&db[thing], 0, sizeof(db[0]));
		s_Owner(thing, GOD);
		s_Flags(thing, (TYPE_GARBAGE | GOING));
		s_Flags2(thing, 0);
		s_Flags3(thing, 0);
		s_Powers(thing, 0);
		s_Powers2(thing, 0);
		s_Location(thing, NOTHING);
		s_Contents(thing, NOTHING);
		s_Exits(thing, NOTHING);
		s_Link(thing, NOTHING);
		s_Next(thing, NOTHING);
		s_Zone(thing, NOTHING);
		s_Parent(thing, NOTHING);
		s_Stack(thing, NULL);
		db[thing].ahead = NULL;
		db[thing].at_count = 0;
	}
}

void db_grow(dbref newtop)
{
	int newsize, marksize, delta, i;
	MARKBUF *newmarkbuf;
	OBJ *newdb;
	NAME *newpurenames;

	char *cp;

	delta = mudconf.init_size;

	/*
	 * Determine what to do based on requested size, current top and  * * 
	 * 
	 * *  * *  * *  * * size.  Make sure we grow in reasonable-sized
	 * chunks to * * prevent *  * *  * frequent reallocations of the db
	 * array. 
	 */

	/*
	 * If requested size is smaller than the current db size, ignore it 
	 */

	if(newtop <= mudstate.db_top) {
		return;
	}
	/*
	 * If requested size is greater than the current db size but smaller
	 * * * * * * * than the amount of space we have allocated, raise the
	 * db  * *  * size * * and * initialize the new area. 
	 */

	if(newtop <= mudstate.db_size) {
		for(i = mudstate.db_top; i < newtop; i++) {
			if(mudconf.cache_names)
				purenames[i] = NULL;
		}
		initialize_objects(mudstate.db_top, newtop);
		mudstate.db_top = newtop;
		return;
	}
	/*
	 * Grow by a minimum of delta objects 
	 */

	if(newtop <= mudstate.db_size + delta) {
		newsize = mudstate.db_size + delta;
	} else {
		newsize = newtop;
	}

	/*
	 * Enforce minimumdatabase size 
	 */

	if(newsize < mudstate.min_size)
		newsize = mudstate.min_size + delta;;

	/*
	 * Grow the name tables 
	 */

	if(mudconf.cache_names) {
		newpurenames =
			(NAME *) XMALLOC((newsize + SIZE_HACK) * sizeof(NAME),
							 "db_grow.purenames");

		if(!newpurenames) {
			LOG_SIMPLE(LOG_ALWAYS, "ALC", "DB",
					   tprintf
					   ("Could not allocate space for %d item name cache.",
						newsize));
			abort();
		}
		bzero((char *) newpurenames, (newsize + SIZE_HACK) * sizeof(NAME));

		if(purenames) {

			/*
			 * An old name cache exists.  Copy it. 
			 */

			purenames -= SIZE_HACK;
			bcopy((char *) purenames, (char *) newpurenames,
				  (newtop + SIZE_HACK) * sizeof(NAME));
			cp = (char *) purenames;
			XFREE(cp, "db_grow.purename");
		} else {

			/*
			 * Creating a brand new struct database.  Fill in the
			 * 'reserved' area in case it gets referenced.  
			 */

			purenames = newpurenames;
			for(i = 0; i < SIZE_HACK; i++) {
				purenames[i] = NULL;
			}
		}
		purenames = newpurenames + SIZE_HACK;
		newpurenames = NULL;
	}
	/*
	 * Grow the db array 
	 */

	newdb = (OBJ *)
		XMALLOC((newsize + SIZE_HACK) * sizeof(OBJ), "db_grow.db");
	if(!newdb) {

		LOG_SIMPLE(LOG_ALWAYS, "ALC", "DB",
				   tprintf
				   ("Could not allocate space for %d item struct database.",
					newsize));
		abort();
	}
	if(db) {

		/*
		 * An old struct database exists.  Copy it to the new buffer 
		 */

		db -= SIZE_HACK;
		bcopy((char *) db, (char *) newdb,
			  (mudstate.db_top + SIZE_HACK) * sizeof(OBJ));
		cp = (char *) db;
		XFREE(cp, "db_grow.db");
	} else {

		/*
		 * Creating a brand new struct database.  Fill in the * * * * 
		 * 
		 * *  * * 'reserved' area in case it gets referenced. 
		 */

		db = newdb;
		for(i = 0; i < SIZE_HACK; i++) {
			s_Owner(i, GOD);
			s_Flags(i, (TYPE_GARBAGE | GOING));
			s_Powers(i, 0);
			s_Powers2(i, 0);
			s_Location(i, NOTHING);
			s_Contents(i, NOTHING);
			s_Exits(i, NOTHING);
			s_Link(i, NOTHING);
			s_Next(i, NOTHING);
			s_Zone(i, NOTHING);
			s_Parent(i, NOTHING);
			s_Stack(i, NULL);
			db[i].ahead = NULL;
			db[i].at_count = 0;
		}
	}
	db = newdb + SIZE_HACK;
	newdb = NULL;

	for(i = mudstate.db_top; i < newtop; i++) {
		if(mudconf.cache_names) {
			purenames[i] = NULL;
		}
	}
	initialize_objects(mudstate.db_top, newtop);
	mudstate.db_top = newtop;
	mudstate.db_size = newsize;

	/*
	 * Grow the db mark buffer 
	 */

	marksize = (newsize + 7) >> 3;
	newmarkbuf = (MARKBUF *) XMALLOC(marksize, "db_grow");
	bzero((char *) newmarkbuf, marksize);
	if(mudstate.markbits) {
		marksize = (newtop + 7) >> 3;
		bcopy((char *) mudstate.markbits, (char *) newmarkbuf, marksize);
		cp = (char *) mudstate.markbits;
		XFREE(cp, "db_grow");
	}
	mudstate.markbits = newmarkbuf;
}

void db_free(void)
{
	char *cp;

	if(db != NULL) {
		db -= SIZE_HACK;
		cp = (char *) db;
		XFREE(cp, "db_grow");
		db = NULL;
	}
	mudstate.db_top = 0;
	mudstate.db_size = 0;
	mudstate.freelist = NOTHING;
}

void db_make_minimal(void)
{
	dbref obj;

	db_free();
	db_grow(1);
	s_Name(0, "Limbo");
	s_Flags(0, TYPE_ROOM);
	s_Powers(0, 0);
	s_Powers2(0, 0);
	s_Location(0, NOTHING);
	s_Exits(0, NOTHING);
	s_Link(0, NOTHING);
	s_Parent(0, NOTHING);
	s_Zone(0, NOTHING);
	s_Pennies(0, 1);
	s_Owner(0, 1);
	db[0].ahead = NULL;
	db[0].at_count = 0;
	/*
	 * should be #1 
	 */
	load_player_names();
	obj =
		create_player((char *) "Wizard", (char *) "potrzebie", NOTHING, 0, 0);
	s_Flags(obj, Flags(obj) | WIZARD);
	s_Powers(obj, 0);
	s_Powers2(obj, 0);
	s_Pennies(obj, 1000);

	/*
	 * Manually link to Limbo, just in case 
	 */
	s_Location(obj, 0);
	s_Next(obj, NOTHING);
	s_Contents(0, obj);
	s_Link(obj, 0);
}

dbref parse_dbref(const char *s)
{
	const char *p;
	int x;

	/*
	 * Enforce completely numeric dbrefs 
	 */

	for(p = s; *p; p++) {
		if(!isdigit(*p))
			return NOTHING;
	}

	x = atoi(s);
	return ((x >= 0) ? x : NOTHING);
}

void putref(FILE * f, dbref ref)
{
	fprintf(f, "%d\n", ref);
}

void putstring(FILE * f, const char *s)
{
	putc('"', f);

	while (s && *s) {
		switch (*s) {
		case '\\':
		case '"':
			putc('\\', f);
		default:
			putc(*s, f);
		}
		s++;
	}
	putc('"', f);
	putc('\n', f);
}

const char *getstring_noalloc(FILE * f, int new_strings)
{
	static char buf[LBUF_SIZE];
	char *p;
	int c, lastc;

	p = buf;
	c = fgetc(f);
	if(!new_strings || (c != '"')) {
		ungetc(c, f);
		c = '\0';
		for(;;) {
			lastc = c;
			c = fgetc(f);

			/*
			 * If EOF or null, return 
			 */

			if(!c || (c == EOF)) {
				*p = '\0';
				return buf;
			}
			/*
			 * If a newline, return if prior char is not a cr. *
			 * * * Otherwise * keep on truckin' 
			 */

			if((c == '\n') && (lastc != '\r')) {
				*p = '\0';
				return buf;
			}
			safe_chr(c, buf, &p);
		}
	} else {
		for(;;) {
			c = fgetc(f);
			if(c == '"') {
				if((c = fgetc(f)) != '\n')
					ungetc(c, f);
				*p = '\0';
				return buf;
			} else if(c == '\\') {
				c = fgetc(f);
			}
			if((c == '\0') || (c == EOF)) {
				*p = '\0';
				return buf;
			}
			safe_chr(c, buf, &p);
		}
	}
}

dbref getref(FILE * f)
{
	static char buf[SBUF_SIZE];

	fgets(buf, sizeof(buf), f);
	return (atoi(buf));
}

void free_boolexp(BOOLEXP * b)
{
	if(b == TRUE_BOOLEXP)
		return;

	switch (b->type) {
	case BOOLEXP_AND:
	case BOOLEXP_OR:
		free_boolexp(b->sub1);
		free_boolexp(b->sub2);
		free_bool(b);
		break;
	case BOOLEXP_NOT:
	case BOOLEXP_CARRY:
	case BOOLEXP_IS:
	case BOOLEXP_OWNER:
	case BOOLEXP_INDIR:
		free_boolexp(b->sub1);
		free_bool(b);
		break;
	case BOOLEXP_CONST:
		free_bool(b);
		break;
	case BOOLEXP_ATR:
	case BOOLEXP_EVAL:
		free((char *) b->sub1);
		free_bool(b);
		break;
	}
}

BOOLEXP *dup_bool(BOOLEXP * b)
{
	BOOLEXP *r;

	if(b == TRUE_BOOLEXP)
		return (TRUE_BOOLEXP);

	r = alloc_bool("dup_bool");
	switch (r->type = b->type) {
	case BOOLEXP_AND:
	case BOOLEXP_OR:
		r->sub2 = dup_bool(b->sub2);
	case BOOLEXP_NOT:
	case BOOLEXP_CARRY:
	case BOOLEXP_IS:
	case BOOLEXP_OWNER:
	case BOOLEXP_INDIR:
		r->sub1 = dup_bool(b->sub1);
	case BOOLEXP_CONST:
		r->thing = b->thing;
		break;
	case BOOLEXP_EVAL:
	case BOOLEXP_ATR:
		r->thing = b->thing;
		r->sub1 = (BOOLEXP *) strsave((char *) b->sub1);
		break;
	default:
		fprintf(stderr, "bad bool type!!\n");
		return (TRUE_BOOLEXP);
	}
	return (r);
}

void clone_object(dbref a, dbref b)
{
	bcopy((char *) &db[b], (char *) &db[a], sizeof(struct object));
}

/*
 * check_zone - checks back through a zone tree for control 
 */
int check_zone(dbref player, dbref thing)
{
	mudstate.zone_nest_num++;

	if(!mudconf.have_zones || (Zone(thing) == NOTHING) ||
	   (mudstate.zone_nest_num == mudconf.zone_nest_lim) ||
	   (isPlayer(thing))) {
		mudstate.zone_nest_num = 0;
		return 0;
	}

	/*
	 * If the zone doesn't have an enterlock, DON'T allow control. 
	 */

	if(atr_get_raw(Zone(thing), A_LENTER) &&
	   could_doit(player, Zone(thing), A_LENTER)) {
		mudstate.zone_nest_num = 0;
		return 1;
	} else {
		return check_zone(player, Zone(thing));
	}

}

int check_zone_for_player(dbref player, dbref thing)
{
	mudstate.zone_nest_num++;

	if(!mudconf.have_zones || (Zone(thing) == NOTHING) ||
	   (mudstate.zone_nest_num == mudconf.zone_nest_lim) ||
	   !(isPlayer(thing))) {
		mudstate.zone_nest_num = 0;
		return 0;
	}

	if(atr_get_raw(Zone(thing), A_LENTER) &&
	   could_doit(player, Zone(thing), A_LENTER)) {
		mudstate.zone_nest_num = 0;
		return 1;
	} else {
		return check_zone(player, Zone(thing));
	}

}

void toast_player(dbref player)
{
	do_clearcom(player, player, 0);
	do_channelnuke(player);
	del_commac(player);
	do_clear_macro(player, NULL);
}

/*
 * ---------------------------------------------------------------------------
 * * dump_restart_db: Writes out socket information.
 */


void dump_restart_db(void)
{
	FILE *f;
	DESC *d;
    OBJQE *obq;
	int version = 0;

	/* We maintain a version number for the restart database,
	   so we can restart even if the format of the restart db
	   has been changed in the new executable. */

	version |= RS_RECORD_PLAYERS;
	version |= RS_NEW_STRINGS;
	version |= RS_HUDKEY;

	f = fopen("restart.db", "w");
	fprintf(f, "+V%d\n", version);
	putref(f, mudstate.start_time);
	putstring(f, mudstate.doing_hdr);
	putref(f, mudstate.record_players);
	DESC_ITER_ALL(d) {
		putref(f, d->descriptor);
		putref(f, d->flags);
		putref(f, d->connected_at);
		putref(f, d->command_count);
		putref(f, d->timeout);
		putref(f, d->host_info);
		putref(f, d->player);
		putref(f, d->last_time);
		putstring(f, d->output_prefix);
		putstring(f, d->output_suffix);
		putstring(f, d->addr);
		putstring(f, d->doing);
		putstring(f, d->username);
		putstring(f, d->hudkey);
	}
	putref(f, 0);
	fclose(f);
}

#define RESTART_MAGIC 0x0001D1ED
void dump_restart_db_xdr(void)
{
	struct mmdb_t *mmdb;
    DESC *d;
	int version = 0;

	/* We maintain a version number for the restart database,
	   so we can restart even if the format of the restart db
	   has been changed in the new executable. */

	version |= RS_RECORD_PLAYERS;
	version |= RS_NEW_STRINGS;
	version |= RS_HUDKEY;

   
    mmdb = mmdb_open_write("restart.xdr");
    mmdb_write_uint32(mmdb, RESTART_MAGIC);
    mmdb_write_uint32(mmdb, 1);
    mmdb_write_uint32(mmdb, version);
    mmdb_write_uint32(mmdb, mudstate.start_time);
    mmdb_write_string(mmdb, mudstate.doing_hdr);
	mmdb_write_uint32(mmdb, mudstate.record_players);
	DESC_ITER_ALL(d) {
		mmdb_write_uint32(mmdb, d->descriptor);
		mmdb_write_uint32(mmdb, d->flags);
		mmdb_write_uint32(mmdb, d->connected_at);
		mmdb_write_uint32(mmdb, d->command_count);
		mmdb_write_uint32(mmdb, d->timeout);
		mmdb_write_uint32(mmdb, d->host_info);
		mmdb_write_uint32(mmdb, d->player);
		mmdb_write_uint32(mmdb, d->last_time);
		mmdb_write_string(mmdb, d->output_prefix);
		mmdb_write_string(mmdb, d->output_suffix);
		mmdb_write_string(mmdb, d->addr);
		mmdb_write_string(mmdb, d->doing);
		mmdb_write_string(mmdb, d->username);
		mmdb_write_string(mmdb, d->hudkey);
	}
    mmdb_write_uint32(mmdb, 0);
    cque_dump_restart(mmdb); 
    mmdb_close(mmdb);
}

void accept_client_input(int fd, short event, void *arg);

void bsd_write_callback(struct bufferevent *bufev, void *arg);
void bsd_read_callback(struct bufferevent *bufev, void *arg);
void bsd_error_callback(struct bufferevent *bufev, short whut, void *arg);

void load_restart_db()
{
	FILE *f;
	DESC *d;
	DESC *p;
    struct stat statbuffer;

	int val, version, new_strings = 0;
	char *temp, buf[8];

	f = fopen("restart.db", "r");
	if(!f) {
		mudstate.restarting = 0;
		return;
	}
	mudstate.restarting = 1;

	fgets(buf, 3, f);
	if(strncmp(buf, "+V", 2)) {
		abort();
	}
	version = getref(f);

	if(version & RS_NEW_STRINGS)
		new_strings = 1;

	mudstate.start_time = getref(f);
	time(&mudstate.restart_time);
	strcpy(mudstate.doing_hdr, getstring_noalloc(f, new_strings));

	if(version & RS_RECORD_PLAYERS) {
		mudstate.record_players = getref(f);
	}

	while ((val = getref(f)) != 0) {
		d = malloc(sizeof(DESC));
		memset(d, 0, sizeof(DESC));
		d->descriptor = val;
		d->flags = getref(f);
		d->connected_at = getref(f);
		d->command_count = getref(f);
		d->timeout = getref(f);
		d->host_info = getref(f);
		d->player = getref(f);
		d->last_time = getref(f);
		temp = (char *) getstring_noalloc(f, new_strings);
		if(*temp) {
			d->output_prefix = alloc_lbuf("set_userstring");
			strcpy(d->output_prefix, temp);
		} else {
			d->output_prefix = NULL;
		}
		temp = (char *) getstring_noalloc(f, new_strings);
		if(*temp) {
			d->output_suffix = alloc_lbuf("set_userstring");
			strcpy(d->output_suffix, temp);
		} else {
			d->output_suffix = NULL;
		}

		strcpy(d->addr, getstring_noalloc(f, new_strings));
		strcpy(d->doing, getstring_noalloc(f, new_strings));
		strcpy(d->username, getstring_noalloc(f, new_strings));

		if(version & RS_HUDKEY)
			strncpy(d->hudkey, getstring_noalloc(f, new_strings), HUDKEYLEN);
		else
			d->hudkey[0] = '\0';

		d->output_size = 0;
		d->output_tot = 0;
		d->output_lost = 0;
		d->input_size = 0;
		d->input_tot = 0;
		d->input_lost = 0;
		memset(d->input, 0, sizeof(d->input));
        d->input_tail = 0;
		d->quota = mudconf.cmd_quota_max;
		d->program_data = NULL;
		d->hashnext = NULL;
        d->refcount = 1;

		d->saddr_len = sizeof(d->saddr);
		getpeername(d->descriptor, (struct sockaddr *) &d->saddr,
					(socklen_t *) & d->saddr_len);
		d->outstanding_dnschild_query = dnschild_request(d);
        
        d->next = descriptor_list;
        descriptor_list = d->next;

        d->sock_buff = bufferevent_new(d->descriptor, bsd_write_callback,
									   bsd_read_callback, bsd_error_callback,
									   NULL);

		bufferevent_disable(d->sock_buff, EV_READ);
		bufferevent_enable(d->sock_buff, EV_WRITE);

		event_set(&d->sock_ev, d->descriptor, EV_READ | EV_PERSIST,
				  accept_client_input, d);
		event_add(&d->sock_ev, NULL);

		desc_addhash(d);
		if(isPlayer(d->player))
			s_Flags2(d->player, Flags2(d->player) | CONNECTED);
	}

    DESC_ITER_CONN(d) {
        if(!isPlayer(d->player) || fstat(d->descriptor, &statbuffer) < 0) {
            dprintk("dropping descriptor %d.\n", d->descriptor);
            shutdownsock(d, R_QUIT);
        }

    }

	fclose(f);
	remove("restart.db");
	raw_broadcast(0, "Game: Restart finished.");
}

int load_restart_db_xdr()
{
	struct mmdb_t *mmdb;
    DESC *d;
	DESC *p;
    struct stat statbuffer;

	int val, version, new_strings = 0;
	char *temp, buf[8];

	mmdb = mmdb_open_read("restart.xdr");
    if(!mmdb) return 0;

    val = mmdb_read_uint32(mmdb); // RESTART_MAGIC
    if(val != RESTART_MAGIC) {
        printk("restart database had invalid magic.");
        printk("read %08x expected %08x.", val, RESTART_MAGIC);
        return 0;
    }
    mmdb_read_uint32(mmdb); // VERSION
    mmdb_read_uint32(mmdb); // VERSION
    mudstate.start_time = mmdb_read_uint32(mmdb);
    mmdb_read_opaque(mmdb, mudstate.doing_hdr, sizeof(mudstate.doing_hdr));
    mudstate.record_players = mmdb_read_uint32(mmdb);

	mudstate.restarting = 1;

    while( (val = mmdb_read_uint32(mmdb)) != 0) {
        dprintk("loading descriptor %d\n", val);
		d = malloc(sizeof(DESC));
		memset(d, 0, sizeof(DESC));
		d->descriptor = val;
		d->flags = mmdb_read_uint32(mmdb);
		d->connected_at = mmdb_read_uint32(mmdb);
		d->command_count = mmdb_read_uint32(mmdb);
		d->timeout = mmdb_read_uint32(mmdb);
		d->host_info = mmdb_read_uint32(mmdb);
		d->player = mmdb_read_uint32(mmdb);
		d->last_time = mmdb_read_uint32(mmdb);
	
        d->output_prefix = mmdb_read_string(mmdb);
        d->output_suffix = mmdb_read_string(mmdb);
        mmdb_read_opaque(mmdb, d->addr, sizeof(d->addr));
        mmdb_read_opaque(mmdb, d->doing, sizeof(d->doing));
        mmdb_read_opaque(mmdb, d->username, sizeof(d->username));
        mmdb_read_opaque(mmdb, d->hudkey, sizeof(d->hudkey));

		d->output_size = 0;
		d->output_tot = 0;
		d->output_lost = 0;
		d->input_size = 0;
		d->input_tot = 0;
		d->input_lost = 0;
		memset(d->input, 0, sizeof(d->input));
        d->input_tail = 0;
		d->quota = mudconf.cmd_quota_max;
		d->program_data = NULL;
		d->hashnext = NULL;
        d->refcount = 1;

		d->saddr_len = sizeof(d->saddr);
		getpeername(d->descriptor, (struct sockaddr *) &d->saddr,
					(socklen_t *) & d->saddr_len);
		d->outstanding_dnschild_query = dnschild_request(d);

        d->next = descriptor_list;
        descriptor_list = d;

        d->sock_buff = bufferevent_new(d->descriptor, bsd_write_callback,
									   bsd_read_callback, bsd_error_callback,
									   NULL);
		bufferevent_disable(d->sock_buff, EV_READ);
		bufferevent_enable(d->sock_buff, EV_WRITE);

		event_set(&d->sock_ev, d->descriptor, EV_READ | EV_PERSIST,
				  accept_client_input, d);
		event_add(&d->sock_ev, NULL);

		desc_addhash(d);
		if(isPlayer(d->player))
			s_Flags2(d->player, Flags2(d->player) | CONNECTED);
	}

	DESC_ITER_CONN(d) {
		if(!isPlayer(d->player) || fstat(d->descriptor, &statbuffer) < 0) {
            dprintk("dropping descriptor %d.\n", d->descriptor);
			shutdownsock(d, R_QUIT);
		}

	}
    cque_load_restart(mmdb);
    mmdb_close(mmdb);
    remove("restart.xdr");
    remove("restart.db");
	raw_broadcast(0, "Game: Restart finished.");
    return 1;
}
