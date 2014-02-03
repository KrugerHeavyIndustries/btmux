
/*
 * player.c 
 */

#include "copyright.h"
#include "config.h"

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "externs.h"
#include "alloc.h"
#include "attrs.h"
#include "powers.h"
#include "command.h"

#define	NUM_GOOD	4			/*
								 * # of successful logins to save data for 
								 */
#define NUM_BAD		3			/*
								 * # of failed logins to save data for 
								 */

typedef struct hostdtm HOSTDTM;
struct hostdtm {
	char *host;
	char *dtm;
};

typedef struct logindata LDATA;
struct logindata {
	HOSTDTM good[NUM_GOOD];
	HOSTDTM bad[NUM_BAD];
	int tot_good;
	int tot_bad;
	int new_bad;
};

extern char *crypt(const char *, const char *);
extern time_t time(time_t *);

/**
 * Decode login info.
 */
static void decrypt_logindata(char *atrbuf, LDATA * info)
{
	int i;
	char *tmpc;

	info->tot_good = 0;
	info->tot_bad = 0;
	info->new_bad = 0;
	for(i = 0; i < NUM_GOOD; i++) {
		info->good[i].host = NULL;
		info->good[i].dtm = NULL;
	}
	for(i = 0; i < NUM_BAD; i++) {
		info->bad[i].host = NULL;
		info->bad[i].dtm = NULL;
	}

	if(*atrbuf == '#') {
		atrbuf++;
		if(!(tmpc = grabto(&atrbuf, ';')))
			return;
		info->tot_good = atoi(tmpc);
		for(i = 0; i < NUM_GOOD; i++) {
			if(!(tmpc = grabto(&atrbuf, ';')))
				return;
			info->good[i].host = tmpc;
			if(!(tmpc = grabto(&atrbuf, ';')))
				return;
			info->good[i].dtm = tmpc;
		}
		if(!(tmpc = grabto(&atrbuf, ';')))
			return;
		info->new_bad = atoi(tmpc);
		if(!(tmpc = grabto(&atrbuf, ';')))
			return;
		info->tot_bad = atoi(tmpc);
		for(i = 0; i < NUM_BAD; i++) {
			if(!(tmpc = grabto(&atrbuf, ';')))
				return;
			info->bad[i].host = tmpc;
			if(!(tmpc = grabto(&atrbuf, ';')))
				return;
			info->bad[i].dtm = tmpc;
		}
	}
}

/**
 * Encode login info.
 */
static void encrypt_logindata(char *atrbuf, LDATA * info)
{
	char *bp, nullc;
	int i;

	/*
	 * Make sure the SPRINTF call tracks NUM_GOOD and NUM_BAD for the * * 
	 * 
	 * *  * * number of host/dtm pairs of each type. 
	 */

	nullc = '\0';
	for(i = 0; i < NUM_GOOD; i++) {
		if(!info->good[i].host)
			info->good[i].host = &nullc;
		if(!info->good[i].dtm)
			info->good[i].dtm = &nullc;
	}
	for(i = 0; i < NUM_BAD; i++) {
		if(!info->bad[i].host)
			info->bad[i].host = &nullc;
		if(!info->bad[i].dtm)
			info->bad[i].dtm = &nullc;
	}
	bp = alloc_lbuf("encrypt_logindata");
	sprintf(bp, "#%d;%s;%s;%s;%s;%s;%s;%s;%s;%d;%d;%s;%s;%s;%s;%s;%s;",
			info->tot_good, info->good[0].host, info->good[0].dtm,
			info->good[1].host, info->good[1].dtm, info->good[2].host,
			info->good[2].dtm, info->good[3].host, info->good[3].dtm,
			info->new_bad, info->tot_bad, info->bad[0].host, info->bad[0].dtm,
			info->bad[1].host, info->bad[1].dtm, info->bad[2].host,
			info->bad[2].dtm);
	StringCopy(atrbuf, bp);
	free_lbuf(bp);
}

/**
 * Record successful or failed login attempt.
 * If successful, report last successful login and number of failures since
 * last successful login.
 */
void record_login(dbref player, int isgood, char *ldate, char *lhost,
				  char *lusername)
{
	LDATA login_info;
	char *atrbuf;
	dbref aowner;
	long aflags; int i;

	atrbuf = atr_get(player, A_LOGINDATA, &aowner, &aflags);
	decrypt_logindata(atrbuf, &login_info);
	if(isgood) {
		if(login_info.new_bad > 0) {
			notify(player, "");
			notify_printf(player,
						  "**** %d failed connect%s since your last successful connect. ****",
						  login_info.new_bad,
						  (login_info.new_bad == 1 ? "" : "s"));
			notify_printf(player,
						  "Most recent attempt was from %s on %s.",
						  login_info.bad[0].host, login_info.bad[0].dtm);
			notify(player, "");
			login_info.new_bad = 0;
		}
		if(login_info.good[0].host && *login_info.good[0].host &&
		   login_info.good[0].dtm && *login_info.good[0].dtm) {
			notify_printf(player, "Last connect was from %s on %s.",
						  login_info.good[0].host, login_info.good[0].dtm);
		}
		if(mudconf.have_mailer)
			check_mail(player, 0, 0);
		for(i = NUM_GOOD - 1; i > 0; i--) {
			login_info.good[i].dtm = login_info.good[i - 1].dtm;
			login_info.good[i].host = login_info.good[i - 1].host;
		}
		login_info.good[0].dtm = ldate;
		login_info.good[0].host = lhost;
		login_info.tot_good++;
		if(*lusername)
			atr_add_raw(player, A_LASTSITE, tprintf("%s@%s", lusername,
													lhost));
		else
			atr_add_raw(player, A_LASTSITE, lhost);
	} else {
		for(i = NUM_BAD - 1; i > 0; i--) {
			login_info.bad[i].dtm = login_info.bad[i - 1].dtm;
			login_info.bad[i].host = login_info.bad[i - 1].host;
		}
		login_info.bad[0].dtm = ldate;
		login_info.bad[0].host = lhost;
		login_info.tot_bad++;
		login_info.new_bad++;
	}
	encrypt_logindata(atrbuf, &login_info);
	atr_add_raw(player, A_LOGINDATA, atrbuf);
	free_lbuf(atrbuf);
}

/**
 * Test a password to see if it is correct.
 */
int check_pass(dbref player, const char *password)
{
	dbref aowner;
	long aflags;
	char *target;
	char *hashed;

	target = atr_get(player, A_PASS, &aowner, &aflags);
	hashed = crypt(password, "XX");
	if(*target && strcmp(target, password) && strcmp(hashed, target)) {
		free_lbuf(target);
		return 0;
	}
	free_lbuf(target);

	/*
	 * This is needed to prevent entering the raw encrypted password from
	 * working. Do it better if you like, but it's needed. 
	 *
	 * Not really, you should just not really allow unencrypted passwords.
	 * -Hag
	 */

	if((strlen(password) == 13) && (password[0] == 'X') &&
	   (password[1] == 'X'))
		return 0;

	return 1;
}

/**
 * Try to connect to an existing player.
 */
dbref connect_player(char *name, char *password, char *host, char *username)
{
	dbref player, aowner;
	long aflags;
	time_t tt;
	char *time_str, *player_last, *allowance;

	time(&tt);
	time_str = ctime(&tt);
	time_str[strlen(time_str) - 1] = '\0';

	if((player = lookup_player(NOTHING, name, 0)) == NOTHING)
		return NOTHING;
	if(!check_pass(player, password)) {
		record_login(player, 0, time_str, host, username);
		return NOTHING;
	}
	time(&tt);
	time_str = ctime(&tt);
	time_str[strlen(time_str) - 1] = '\0';

	/*
	 * compare to last connect see if player gets salary 
	 */
	player_last = atr_get(player, A_LAST, &aowner, &aflags);
	if(strncmp(player_last, time_str, 10) != 0) {
		allowance = atr_pget(player, A_ALLOWANCE, &aowner, &aflags);
		if(*allowance == '\0')
			giveto(player, mudconf.paycheck);
		else
			giveto(player, atoi(allowance));
		free_lbuf(allowance);
	}
	atr_add_raw(player, A_LAST, time_str);
	free_lbuf(player_last);
	return player;
}

/**
 * Create a new player.
 */
dbref create_player(char *name, char *password, dbref creator, int isrobot,
					int isguest)
{
	dbref player;
	char *pbuf;

	/*
	 * Make sure the password is OK.  Name is checked in create_obj 
	 */

	pbuf = trim_spaces(password);
	if(!ok_password(pbuf)) {
		free_lbuf(pbuf);
		return NOTHING;
	}
	/*
	 * If so, go create him 
	 */

	player = create_obj(creator, TYPE_PLAYER, name, isrobot);
	if(player == NOTHING) {
		free_lbuf(pbuf);
		return NOTHING;
	}
	/*
	 * initialize everything 
	 */
	if(isguest) {
		if(*mudconf.guests_channel)
			do_addcom(player, player, 0, "g", mudconf.guests_channel);
	} else {
		if(*mudconf.public_channel)
			do_addcom(player, player, 0, "pub", mudconf.public_channel);
	}

	s_Pass(player, crypt(pbuf, "XX"));
	s_Home(player, start_home());
	s_Fixed(player);
	free_lbuf(pbuf);
	return player;
}

/**
 * Change the password for a player
 */
void do_password(dbref player, dbref cause, int key, char *oldpass,
				 char *newpass)
{
	dbref aowner;
	long aflags;
	char *target;

	target = atr_get(player, A_PASS, &aowner, &aflags);
	if(!*target || !check_pass(player, oldpass)) {
		notify(player, "Sorry.");
	} else if(!ok_password(newpass)) {
		notify(player, "Bad new password.");
	} else {
		atr_add_raw(player, A_PASS, crypt(newpass, "XX"));
		notify(player, "Password changed.");
	}
	free_lbuf(target);
}

/**
 * Display login history data.
 */
static void disp_from_on(dbref player, char *dtm_str, char *host_str)
{
	if(dtm_str && *dtm_str && host_str && *host_str) {
		notify_printf(player, "     From: %s   On: %s", dtm_str, host_str);
	}
}

void do_last(dbref player, dbref cause, int key, char *who)
{
	dbref target, aowner;
	LDATA login_info;
	char *atrbuf;
	int i; long aflags;

	if(!who || !*who) {
		target = Owner(player);
	} else if(!(string_compare(who, "me"))) {
		target = Owner(player);
	} else {
		target = lookup_player(player, who, 1);
	}

	if(target == NOTHING) {
		notify(player, "I couldn't find that player.");
	} else if(!Controls(player, target)) {
		notify(player, "Permission denied.");
	} else {
		atrbuf = atr_get(target, A_LOGINDATA, &aowner, &aflags);
		decrypt_logindata(atrbuf, &login_info);

		notify_printf(player, "Total successful connects: %d",
					  login_info.tot_good);
		for(i = 0; i < NUM_GOOD; i++) {
			disp_from_on(player, login_info.good[i].host,
						 login_info.good[i].dtm);
		}
		notify_printf(player, "Total failed connects: %d",
					  login_info.tot_bad);
		for(i = 0; i < NUM_BAD; i++) {
			disp_from_on(player, login_info.bad[i].host,
						 login_info.bad[i].dtm);
		}
		free_lbuf(atrbuf);
	}
}

/*
 * add_player_name, delete_player_name, lookup_player:
 * Manage playername->dbref mapping
 */
int add_player_name(dbref player, char *name)
{
	int stat;
	dbref *p;
	char *temp, *tp;

	/*
	 * Convert to all lowercase 
	 */

	tp = temp = alloc_lbuf("add_player_name");
	safe_str(name, temp, &tp);
	*tp = '\0';
	for(tp = temp; *tp; tp++)
		*tp = ToLower(*tp);

	p = (long *) hashfind(temp, &mudstate.player_htab);
	if(p) {

		/*
		 * Entry found in the hashtable.  If a player, succeed if the
		 * * * numbers match (already correctly in the hash table),
		 * fail * * if they don't.  Fail if the name is a disallowed
		 * name * * (value AMBIGUOUS). 
		 */

		if(*p == AMBIGUOUS) {
			free_lbuf(temp);
			return 0;
		}
		if(Good_obj(*p) && (Typeof(*p) == TYPE_PLAYER)) {
			free_lbuf(temp);
			if(*p == player) {
				return 1;
			} else {
				return 0;
			}
		}
		/*
		 * It's an alias (or an incorrect entry).  Clobber it 
		 */
		free(p);
		p = (dbref *) malloc(sizeof(int));

		*p = player;
		stat = hashrepl(temp, p, &mudstate.player_htab);
		free_lbuf(temp);
	} else {
		p = (dbref *) malloc(sizeof(int));

		*p = player;
		stat = hashadd(temp, p, &mudstate.player_htab);
		free_lbuf(temp);
		stat = (stat < 0) ? 0 : 1;
	}
	return stat;
}

int delete_player_name(dbref player, char *name)
{
	dbref *p;
	char *temp, *tp;

	tp = temp = alloc_lbuf("delete_player_name");
	safe_str(name, temp, &tp);
	*tp = '\0';
	for(tp = temp; *tp; tp++)
		*tp = ToLower(*tp);

	p = (long *) hashfind(temp, &mudstate.player_htab);
	if(!p || (*p == NOTHING) || ((player != NOTHING) && (*p != player))) {
		free_lbuf(temp);
		return 0;
	}
	free(p);
	hashdelete(temp, &mudstate.player_htab);
	free_lbuf(temp);
	return 1;
}

dbref lookup_player(dbref doer, char *name, int check_who)
{
	dbref *p, thing;
	char *temp, *tp;

	if(!string_compare(name, "me"))
		return doer;

	if(*name == NUMBER_TOKEN) {
		name++;
		if(!is_number(name))
			return NOTHING;
		thing = atoi(name);
		if(!Good_obj(thing))
			return NOTHING;
		if(!((Typeof(thing) == TYPE_PLAYER) || God(doer)))
			thing = NOTHING;
		return thing;
	}
	tp = temp = alloc_lbuf("lookup_player");
	safe_str(name, temp, &tp);
	*tp = '\0';
	for(tp = temp; *tp; tp++)
		*tp = ToLower(*tp);
	p = (long *) hashfind(temp, &mudstate.player_htab);
	free_lbuf(temp);
	if(!p) {
		if(check_who) {
			thing = find_connected_name(doer, name);
			if(Dark(thing))
				thing = NOTHING;
		} else
			thing = NOTHING;
	} else if(!Good_obj(*p)) {
		thing = NOTHING;
	} else
		thing = *p;

	return thing;
}

void load_player_names(void)
{
	dbref i, j, aowner;
	long aflags;
	char *alias;

	j = 0;
	DO_WHOLE_DB(i) {
		if(Typeof(i) == TYPE_PLAYER) {
			add_player_name(i, Name(i));
		}
	}
	alias = alloc_lbuf("load_player_names");
	j = 0;
	DO_WHOLE_DB(i) {
		if(Typeof(i) == TYPE_PLAYER) {
			alias = atr_pget_str(alias, i, A_ALIAS, &aowner, &aflags);
			if(*alias)
				add_player_name(i, alias);
		}
	}
	free_lbuf(alias);
}

/**
 * badname_add, badname_check, badname_list: Add/look for/display bad names.
 */
void badname_add(char *bad_name)
{
	BADNAME *bp;

	/*
	 * Make a new node and link it in at the top 
	 */

	bp = (BADNAME *) XMALLOC(sizeof(BADNAME), "badname.struc");
	bp->name = XMALLOC(strlen(bad_name) + 1, "badname.name");
	bp->next = mudstate.badname_head;
	mudstate.badname_head = bp;
	StringCopy(bp->name, bad_name);
}

void badname_remove(char *bad_name)
{
	BADNAME *bp, *backp;

	/*
	 * Look for an exact match on the bad name and remove if found 
	 */

	backp = NULL;
	for(bp = mudstate.badname_head; bp; backp = bp, bp = bp->next) {
		if(!string_compare(bad_name, bp->name)) {
			if(backp)
				backp->next = bp->next;
			else
				mudstate.badname_head = bp->next;
			XFREE(bp->name, "badname.name");
			XFREE(bp, "badname.struc");
			return;
		}
	}
}

int badname_check(char *bad_name)
{
	BADNAME *bp;

	/*
	 * Walk the badname list, doing wildcard matching.  If we get a hit * 
	 * 
	 * *  * *  * * then return false.  If no matches in the list, return
	 * true.  
	 */

	for(bp = mudstate.badname_head; bp; bp = bp->next) {
		if(quick_wild(bp->name, bad_name))
			return 0;
	}
	return 1;
}

void badname_list(dbref player, const char *prefix)
{
	BADNAME *bp;
	char *buff, *bufp;

	/*
	 * Construct an lbuf with all the names separated by spaces 
	 */

	buff = bufp = alloc_lbuf("badname_list");
	safe_str((char *) prefix, buff, &bufp);
	for(bp = mudstate.badname_head; bp; bp = bp->next) {
		safe_chr(' ', buff, &bufp);
		safe_str(bp->name, buff, &bufp);
	}
	*bufp = '\0';

	/*
	 * Now display it 
	 */

	notify(player, buff);
	free_lbuf(buff);
}
