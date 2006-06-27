/*
 * comsys.c 
 */

#include <ctype.h>
#include <sys/types.h>

#include "copyright.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "attrs.h"
#include "match.h"
#include "config.h"
#include "externs.h"
#include "flags.h"
#include "powers.h"

#include "comsys.h"
#include "p.comsys.h"
#include "p.functions.h"
#include "create.h"

/* Static functions */
static void do_save_com(chmsg *);
static void do_show_com(chmsg *);
static void do_comlast(dbref, struct channel *);
static void do_comsend(struct channel *, char *);
static void do_comprintf(struct channel *, char *, ...);
extern void do_joinchannel(dbref, struct channel *);
static void do_leavechannel(dbref, struct channel *);
static void do_comwho(dbref, struct channel *);
static void do_setnewtitle(dbref, struct channel *, char *);
extern void sort_users(struct channel *);
static int do_test_access(dbref, long, struct channel *);

/*
 * This is the hash table for channel names 
 */

void init_chantab(void)
{
	hashinit(&mudstate.channel_htab, 30 * HASH_FACTOR);
}

void send_channel(char *chan, const char *format, ...)
{
	struct channel *ch;
	char buf[LBUF_SIZE];
	char data[LBUF_SIZE];
	char *newline;
	va_list ap;

	if(!(ch = select_channel(chan)))
		return;
	va_start(ap, format);
	vsnprintf(data, LBUF_SIZE, format, ap);
	va_end(ap);

	snprintf(buf, LBUF_SIZE-1, "[%s] %s", chan, data);
	while ((newline = strchr(buf, '\n')))
		*newline = ' ';
	do_comsend(ch, buf);
}

char *get_channel_from_alias(dbref player, char *alias)
{
	struct commac *c;
	int first, last, current = 0;
	int dir;

	c = get_commac(player);

	first = 0;
	last = c->numchannels - 1;
	dir = 1;

	while (dir && (first <= last)) {
		current = (first + last) / 2;
		dir = strcasecmp(alias, c->alias + 6 * current);
		if(dir < 0)
			last = current - 1;
		else
			first = current + 1;
	}

	if(!dir)
		return c->channels[current];
	else
		return "";
}

void load_comsystem(FILE * fp)
{
	int i, j, k, dummy;
	int nc, new = 0;
	struct channel *ch;
	struct comuser *user;
	char temp[LBUF_SIZE];
	char buf[8];

	num_channels = 0;

	fgets(buf, sizeof(buf), fp);
	if(!strncmp(buf, "+V2", 3)) {
		new = 2;
		fscanf(fp, "%d\n", &nc);
	} else if(!strncmp(buf, "+V1", 3)) {
		new = 1;
		fscanf(fp, "%d\n", &nc);
	} else
		nc = atoi(buf);

	num_channels = nc;

	for(i = 0; i < nc; i++) {
		ch = (struct channel *) malloc(sizeof(struct channel));

		fscanf(fp, "%[^\n]\n", temp);

		strncpy(ch->name, temp, CHAN_NAME_LEN);
		ch->name[CHAN_NAME_LEN - 1] = '\0';
		ch->on_users = NULL;

		hashadd(ch->name, (int *) ch, &mudstate.channel_htab);

		ch->last_messages = NULL;

		if(new) {				/* V1 or higher */

			if(fscanf(fp,
					  new ==
					  1 ? "%d %d %d %d %d %d %d %d\n" :
					  "%d %d %d %d %d %d %d %d %d\n", &(ch->type),
					  &(ch->temp1), &(ch->temp2), &(ch->charge),
					  &(ch->charge_who), &(ch->amount_col),
					  &(ch->num_messages), &(ch->chan_obj), &dummy) >= 9 &&
			   new > 1) {
				/* Do things with 'dummy' */
				if(dummy > 0) {
					for(j = 0; j < dummy; j++) {
						chmsg *c;

						fscanf(fp, "%d %[^\n]\n", &k, temp);
						Create(c, chmsg, 1);
						c->msg = strdup(temp);
						c->time = k;
						myfifo_push(&ch->last_messages, c);
					}
				}
			}
		} else {
			fscanf(fp, "%d %d %d %d %d %d %d %d %d %d\n", &(ch->type),
				   &(dummy), &(ch->temp1), &(ch->temp2), &(dummy),
				   &(ch->charge), &(ch->charge_who), &(ch->amount_col),
				   &(ch->num_messages), &(ch->chan_obj));
		}

		fscanf(fp, "%d\n", &(ch->num_users));
		ch->max_users = ch->num_users;
		if(ch->num_users > 0) {
			ch->users = (struct comuser **)
				calloc(ch->max_users, sizeof(struct comuser *));

			for(j = 0; j < ch->num_users; j++) {
				user = (struct comuser *) malloc(sizeof(struct comuser));

				ch->users[j] = user;

				if(new) {
					fscanf(fp, "%d %d\n", &(user->who), &(user->on));
				} else {
					fscanf(fp, "%d %d %d", &(user->who), &(dummy), &(dummy));
					fscanf(fp, "%d\n", &(user->on));
				}

				fscanf(fp, "%[^\n]\n", temp);

				user->title = strdup(temp + 2);

				if(!(isPlayer(user->who)) && !(Going(user->who) &&
											   (God(Owner(user->who))))) {
					do_joinchannel(user->who, ch);
					user->on_next = ch->on_users;
					ch->on_users = user;
				} else {
					user->on_next = ch->on_users;
					ch->on_users = user;
				}
			}
			sort_users(ch);
		} else
			ch->users = NULL;
	}
}

static FILE *temp_file;

static void do_save_com(chmsg * d)
{
	fprintf(temp_file, "%d %s\n", (int) d->time, d->msg);
}

void save_comsystem(FILE * fp)
{
	struct channel *ch;
	struct comuser *user;
	int j, k, player_users;

	fprintf(fp, "+V2\n");
	fprintf(fp, "%d\n", num_channels);
	for(ch = (struct channel *) hash_firstentry(&mudstate.channel_htab);
		ch; ch = (struct channel *) hash_nextentry(&mudstate.channel_htab)) {
		fprintf(fp, "%s\n", ch->name);
		k = myfifo_length(&ch->last_messages);
		/*            1  2  3  4  5  6  7  8  9 */
		fprintf(fp, "%d %d %d %d %d %d %d %d %d\n", ch->type, ch->temp1,
				ch->temp2, ch->charge, ch->charge_who, ch->amount_col,
				ch->num_messages, ch->chan_obj, k);

		if(k) {
			temp_file = fp;
			myfifo_trav_r(&ch->last_messages, do_save_com);
		}
		player_users = 0;
		for(j = 0; j < ch->num_users; j++)
			if(isPlayer(ch->users[j]->who) || isRobot(ch->users[j]->who))
				player_users++;

		fprintf(fp, "%d\n", player_users);
		for(j = 0; j < ch->num_users; j++) {
			user = ch->users[j];
			if(!isPlayer(user->who) && !isRobot(user->who))
				continue;
			fprintf(fp, "%d %d\n", user->who, user->on);
			if(strlen(user->title))
				fprintf(fp, "t:%s\n", user->title);
			else
				fprintf(fp, "t:\n");
		}
	}
}

static dbref cheat_player;

static void do_show_com(chmsg * d)
{
	struct tm *t;
	int day;
	char buf[LBUF_SIZE];

	t = localtime(&mudstate.now);
	day = t->tm_mday;
	t = localtime(&d->time);
	if(day == t->tm_mday) {
		sprintf(buf, "[%02d:%02d] %s", t->tm_hour, t->tm_min, d->msg);
	} else
		sprintf(buf, "[%02d.%02d / %02d:%02d] %s", t->tm_mon + 1,
				t->tm_mday, t->tm_hour, t->tm_min, d->msg);
	notify(cheat_player, buf);
}

static void do_comlast(dbref player, struct channel *ch)
{
	if(!myfifo_length(&ch->last_messages)) {
		notify_printf(player, "There haven't been any messages on %s.",
					  ch->name);
		return;
	}
	cheat_player = player;
	myfifo_trav_r(&ch->last_messages, do_show_com);
}

void do_processcom(dbref player, char *arg1, char *arg2)
{
	struct channel *ch;
	struct comuser *user;

	if((strlen(arg1) + strlen(arg2)) > LBUF_SIZE / 2) {
		arg2[LBUF_SIZE / 2 - strlen(arg1)] = '\0';
	}
	if(!*arg2) {
		raw_notify(player, "No message.");
		return;
	}

	if(!Wizard(player) && In_IC_Loc(player)) {
		raw_notify(player, "Permission denied.");
		return;
	}

	if(!(ch = select_channel(arg1))) {
		notify_printf(player, "Unknown channel %s.", arg1);
		return;
	}
	if(!(user = select_user(ch, player))) {
		raw_notify(player,
				   "You are not listed as on that channel.  Delete this alias and re-add.");
		return;
	}
	if(!strcasecmp(arg2, "on")) {
		do_joinchannel(player, ch);
	} else if(!strcasecmp(arg2, "off")) {
		do_leavechannel(player, ch);
	} else if(!user->on && !Wizard(player) && !mudconf.allow_chanlurking) {
		notify_printf(player, "You must be on %s to do that.", arg1);
		return;
	} else if(!strcasecmp(arg2, "who")) {
		do_comwho(player, ch);
	} else if(!strcasecmp(arg2, "last")) {
		do_comlast(player, ch);
	} else if(!user->on) {
		notify_printf(player, "You must be on %s to do that.", arg1);
		return;
	} else if(!do_test_access(player, CHANNEL_TRANSMIT, ch)) {
		raw_notify(player, "That channel type cannot be transmitted on.");
		return;
	} else {
		if(!payfor(player, Guest(player) ? 0 : ch->charge)) {
			notify_printf(player, "You don't have enough %s.",
						  mudconf.many_coins);
			return;
		} else {
			ch->amount_col += ch->charge;
			giveto(ch->charge_who, ch->charge);
		}

		if((*arg2) == ':')
			do_comprintf(ch, "[%s] %s %s", arg1, Name(player), arg2 + 1);
		else if((*arg2) == ';')
			do_comprintf(ch, "[%s] %s%s", arg1, Name(player), arg2 + 1);
		else if(strlen(user->title))
			do_comprintf(ch, "[%s] %s: <%s> %s", arg1, Name(player), user->title, arg2);
		else
			do_comprintf(ch, "[%s] %s: %s", arg1, Name(player), arg2);
	}
}

static void do_comsend(struct channel *ch, char *mess)
{
	struct comuser *user;
	chmsg *c;

	ch->num_messages++;
	for(user = ch->on_users; user; user = user->on_next) {
		if(user->on && do_test_access(user->who, CHANNEL_RECIEVE, ch) &&
		   (Wizard(user->who) || !In_IC_Loc(user->who))) {
			if(Typeof(user->who) == TYPE_PLAYER && Connected(user->who))
				raw_notify(user->who, mess);
			else
				notify(user->who, mess);
		}
	}
	/* Also, add it to the history of channel */
	if(myfifo_length(&ch->last_messages) >= CHANNEL_HISTORY_LEN) {
		c = myfifo_pop(&ch->last_messages);
		free((void *) c->msg);
	} else
		Create(c, chmsg, 1);
	c->msg = strdup(mess);
	c->time = mudstate.now;
	myfifo_push(&ch->last_messages, c);
}

static void do_comprintf(struct channel *ch, char *messfmt, ...)
{
	struct comuser *user;
	chmsg *c;
    va_list ap;
    char buffer[LBUF_SIZE];
    memset(buffer, 0, LBUF_SIZE);
    va_start(ap, messfmt);
    vsnprintf(buffer, LBUF_SIZE-1, messfmt, ap);
    va_end(ap);

    ch->num_messages++;
	for(user = ch->on_users; user; user = user->on_next) {
		if(user->on && do_test_access(user->who, CHANNEL_RECIEVE, ch) &&
		   (Wizard(user->who) || !In_IC_Loc(user->who))) {
			if(Typeof(user->who) == TYPE_PLAYER && Connected(user->who))
				raw_notify(user->who, buffer);
			else
				notify(user->who, buffer);
		}
	}
	/* Also, add it to the history of channel */
	if(myfifo_length(&ch->last_messages) >= CHANNEL_HISTORY_LEN) {
		c = myfifo_pop(&ch->last_messages);
		free((void *) c->msg);
	} else
		Create(c, chmsg, 1);
	c->msg = strdup(buffer);
	c->time = mudstate.now;
	myfifo_push(&ch->last_messages, c);
}

extern void do_joinchannel(dbref player, struct channel *ch)
{
	struct comuser *user;
	int i;

	user = select_user(ch, player);

	if(!user) {
		ch->num_users++;
		if(ch->num_users >= ch->max_users) {
			ch->max_users += 10;
			ch->users = realloc(ch->users, sizeof(struct comuser *) *
								ch->max_users);
		}
		user = (struct comuser *) malloc(sizeof(struct comuser));

		for(i = ch->num_users - 1;
			i > 0 && ch->users[i - 1]->who > player; i--)
			ch->users[i] = ch->users[i - 1];
		ch->users[i] = user;

		user->who = player;
		user->on = 1;
		user->title = strdup("");

		if(UNDEAD(player)) {
			user->on_next = ch->on_users;
			ch->on_users = user;
			}
	} else if(!user->on) {
		user->on = 1;
	} else {
		notify_printf(player, "You are already on channel %s.", ch->name);
		return;
	}

	/* Trigger AENTER of any channel objects on the channel */
	for(i = ch->num_users - 1; i > 0; i--) {
		if(Typeof(ch->users[i]->who) == TYPE_THING)
			did_it(player, ch->users[i]->who, 0, NULL, 0, NULL, A_AENTER,
				   (char **) NULL, 0);
	}

	notify_printf(player, "You have joined channel %s.", ch->name);

	if(!Dark(player)) {
		do_comprintf(ch, "[%s] %s has joined this channel.", ch->name, Name(player));
	}
}

static void do_leavechannel(dbref player, struct channel *ch)
{
	struct comuser *user;
	int i;

	user = select_user(ch, player);

	if(!user)
		return;

	/* Trigger ALEAVE of any channel objects on the channel */
	for(i = ch->num_users - 1; i > 0; i--) {
		if(Typeof(ch->users[i]->who) == TYPE_THING)
			did_it(player, ch->users[i]->who, 0, NULL, 0, NULL, A_ALEAVE,
				   (char **) NULL, 0);
	}

	notify_printf(player, "You have left channel %s.", ch->name);

	if((user->on) && (!Dark(player))) {
		char *c = Name(player);

		if(c && *c) {
			do_comprintf(ch,"[%s] %s has left this channel.", ch->name, c);
		}
	}
	user->on = 0;
}

static void do_comwho(dbref player, struct channel *ch)
{
	struct comuser *user;
	char *buff;

	raw_notify(player, "-- Players --");
	for(user = ch->on_users; user; user = user->on_next) {
		if(Typeof(user->who) == TYPE_PLAYER && user->on &&
		   Connected(user->who) && (!Hidden(user->who) ||
									((ch->type & CHANNEL_TRANSPARENT)
									 && !Dark(user->who))
									|| Wizard_Who(player))
		   && (!In_IC_Loc(user->who) || Wizard(user->who))) {

			int i = fetch_idle(user->who);

			buff = unparse_object(player, user->who, 0);
			if(i > 30) {
				char *c = get_uptime_to_string(i);

				notify_printf(player, "%s [idle %s]", buff, c);
				free_sbuf(c);
			} else
				notify_printf(player, "%s", buff);
			free_lbuf(buff);
		}
	}

	raw_notify(player, "-- Objects --");
	for(user = ch->on_users; user; user = user->on_next) {
		if(Typeof(user->who) != TYPE_PLAYER && user->on &&
		   !(Going(user->who) && God(Owner(user->who)))) {
			buff = unparse_object(player, user->who, 0);
			notify_printf(player, "%s", buff);
			free_lbuf(buff);
		}
	}
	notify_printf(player, "-- %s --", ch->name);
}

struct channel *select_channel(char *channel)
{
	return (struct channel *) hashfind(channel, &mudstate.channel_htab);
}

struct comuser *select_user(struct channel *ch, dbref player)
{
	int last, current;
	int dir = 1, first = 0;

	if(!ch)
		return NULL;

	last = ch->num_users - 1;
	current = (first + last) / 2;

	while (dir && (first <= last)) {
		current = (first + last) / 2;
		if(ch->users[current] == NULL) {
			last--;
			continue;
		}
		if(ch->users[current]->who == player)
			dir = 0;
		else if(ch->users[current]->who < player) {
			dir = 1;
			first = current + 1;
		} else {
			dir = -1;
			last = current - 1;
		}
	}

	if(!dir)
		return ch->users[current];
	else
		return NULL;
}

void do_addcom(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
	char channel[200];
	char title[100];
	struct channel *ch;
	char *s;
	int where;
	struct commac *c;

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	if(!*arg1) {
		raw_notify(player, "You need to specify an alias.");
		return;
	}
	if(!*arg2) {
		raw_notify(player, "You need to specify a channel.");
		return;
	}

	s = strchr(arg2, ',');

	if(s) {
		/* channelname,title */
		if(s >= arg2 + 200) {
			raw_notify(player, "Channel name too long.");
			return;
		}
		strncpy(channel, arg2, s - arg2);
		channel[s - arg2] = '\0';
		strncpy(title, s + 1, 100);
		title[99] = '\0';
	} else {
		/* just channelname */
		if(strlen(arg2) >= 200) {
			raw_notify(player, "Channel name too long.");
			return;
		}
		strcpy(channel, arg2);
		title[0] = '\0';
	}

	if(strchr(channel, ' ')) {
		raw_notify(player, "Channel name cannot contain spaces.");
		return;
	}

	if(!(ch = select_channel(channel))) {
		notify_printf(player, "Channel %s does not exist yet.", channel);
		return;
	}
	if(!do_test_access(player, CHANNEL_JOIN, ch)) {
		raw_notify(player,
				   "Sorry, this channel type does not allow you to join.");
		return;
	}
	if(select_user(ch, player)) {
		raw_notify(player,
				   "Warning: you are already listed on that channel.");
	}
	c = get_commac(player);
	for(where = 0; where < c->numchannels &&
		(strcasecmp(arg1, c->alias + where * 6) > 0); where++);
	if(where < c->numchannels && !strcasecmp(arg1, c->alias + where * 6)) {
		notify_printf(player, "That alias is already in use for channel %s.",
					  c->channels[where]);
		return;
	}
	if(c->numchannels >= c->maxchannels) {
		c->maxchannels += 10;
		c->alias = realloc(c->alias, sizeof(char) * 6 * c->maxchannels);
		c->channels = realloc(c->channels, sizeof(char *) * c->maxchannels);
	}
	if(where < c->numchannels) {
		memmove(c->alias + 6 * (where + 1), c->alias + 6 * where,
				6 * (c->numchannels - where));
		memmove(c->channels + where + 1, c->channels + where,
				sizeof(c->channels) * (c->numchannels - where));
	}

	c->numchannels++;

	strncpy(c->alias + 6 * where, arg1, 5);
	c->alias[where * 6 + 5] = '\0';
	c->channels[where] = strdup(channel);

	do_joinchannel(player, ch);
	do_setnewtitle(player, ch, title);

	if(title[0])
		notify_printf(player, "Channel %s added with alias %s and title %s.",
					  channel, arg1, title);
	else
		notify_printf(player, "Channel %s added with alias %s.",
					  channel, arg1);
}

static void do_setnewtitle(dbref player, struct channel *ch, char *title)
{
	struct comuser *user;
	char *new;

	user = select_user(ch, player);

	/* Make sure there can be no embedded newlines from %r */

	if(!ch || !user)
		return;

	new = replace_string("\r\n", "", title);
	if(user->title)
		free(user->title);
	user->title = strdup(new);	/* strdup so we can free() safely */
	free_lbuf(new);
}

void do_delcom(dbref player, dbref cause, int key, char *arg1)
{
	int i;
	struct commac *c;

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	if(!arg1) {
		raw_notify(player, "Need an alias to delete.");
		return;
	}
	c = get_commac(player);

	for(i = 0; i < c->numchannels; i++) {
		if(!strcasecmp(arg1, c->alias + i * 6)) {
			do_delcomchannel(player, c->channels[i]);
			notify_printf(player, "Channel %s deleted.", c->channels[i]);
			free(c->channels[i]);

			c->numchannels--;
			if(i < c->numchannels) {
				memmove(c->alias + 6 * i, c->alias + 6 * (i + 1),
						6 * (c->numchannels - i));
				memmove(c->channels + i, c->channels + i + 1,
						sizeof(c->channels) * (c->numchannels - i));
			}
			return;
		}
	}
	raw_notify(player, "Unable to find that alias.");
}

void do_delcomchannel(dbref player, char *channel)
{
	struct channel *ch;
	struct comuser *user;
	int i;

	if(!(ch = select_channel(channel))) {
		notify_printf(player, "Unknown channel %s.", channel);
	} else {

		/* Trigger ALEAVE of any channel objects on the channel */
		for(i = ch->num_users - 1; i > 0; i--) {
			if(Typeof(ch->users[i]->who) == TYPE_THING)
				did_it(player, ch->users[i]->who, 0, NULL, 0, NULL, A_ALEAVE,
					   (char **) NULL, 0);
		}

		for(i = 0; i < ch->num_users; i++) {
			user = ch->users[i];
			if(user->who == player) {
				do_comdisconnectchannel(player, channel);
				if(user->on && !Dark(player)) {
					char *c = Name(player);

					if(c && *c)
						do_comprintf(ch, "[%s] %s has left this channel.", channel, c);
				}
				notify_printf(player, "You have left channel %s.", channel);

				if(user->title)
					free(user->title);
				free(user);
				ch->num_users--;
				if(i < ch->num_users)
					memmove(ch->users + i, ch->users + i + 1,
							sizeof(ch->users) * (ch->num_users - i));
			}
		}
	}
}

void do_createchannel(dbref player, dbref cause, int key, char *channel)
{
	struct channel *newchannel;

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	if(select_channel(channel)) {
		notify_printf(player, "Channel %s already exists.", channel);
		return;
	}
	if(!*channel) {
		raw_notify(player, "You must specify a channel to create.");
		return;
	}
	if(!(Comm_All(player))) {
		raw_notify(player, "You do not have permission to do that.");
		return;
	}
	newchannel = (struct channel *) malloc(sizeof(struct channel));

	strncpy(newchannel->name, channel, CHAN_NAME_LEN - 1);
	newchannel->name[CHAN_NAME_LEN - 1] = '\0';
	newchannel->last_messages = NULL;
	newchannel->type = 127;
	newchannel->temp1 = 0;
	newchannel->temp2 = 0;
	newchannel->charge = 0;
	newchannel->charge_who = player;
	newchannel->amount_col = 0;
	newchannel->num_users = 0;
	newchannel->max_users = 0;
	newchannel->users = NULL;
	newchannel->on_users = NULL;
	newchannel->chan_obj = NOTHING;
	newchannel->num_messages = 0;

	num_channels++;

	hashadd(newchannel->name, (int *) newchannel, &mudstate.channel_htab);

	notify_printf(player, "Channel %s created.", channel);
}

void do_destroychannel(dbref player, dbref cause, int key, char *channel)
{
	struct channel *ch;
	int j;

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	ch = (struct channel *) hashfind(channel, &mudstate.channel_htab);

	if(!ch) {
		notify_printf(player, "Could not find channel %s.", channel);
		return;
	} else if(!(Comm_All(player)) && (player != ch->charge_who)) {
		raw_notify(player, "You do not have permission to do that. ");
		return;
	}
	num_channels--;
	hashdelete(channel, &mudstate.channel_htab);

	for(j = 0; j < ch->num_users; j++) {
		free(ch->users[j]);
	}
	free(ch->users);
	free(ch);
	notify_printf(player, "Channel %s destroyed.", channel);
}

void do_listchannels(dbref player)
{
	struct channel *ch;
	int perm;

	if(!(perm = Comm_All(player))) {
		raw_notify(player,
				   "Warning: Only public channels and your channels will be shown.");
	}
	raw_notify(player,
			   "** Channel             --Flags--  Obj   Own   Charge  Balance  Users   Messages");

	for(ch = (struct channel *) hash_firstentry(&mudstate.channel_htab);
		ch; ch = (struct channel *) hash_nextentry(&mudstate.channel_htab))
		if(perm || (ch->type & CHANNEL_PUBLIC)
		   || ch->charge_who == player) {

			notify_printf(player,
						  "%c%c %-20.20s %c%c%c/%c%c%c %5d %5d %8d %8d %6d %10d",
						  (ch->type & (CHANNEL_PUBLIC)) ? 'P' : '-',
						  (ch->type & (CHANNEL_LOUD)) ? 'L' : '-', ch->name,
						  (ch->type & (CHANNEL_PL_MULT *
									   CHANNEL_JOIN)) ? 'J' : '-',
						  (ch->type & (CHANNEL_PL_MULT *
									   CHANNEL_TRANSMIT)) ? 'X' : '-',
						  (ch->type & (CHANNEL_PL_MULT *
									   CHANNEL_RECIEVE)) ? 'R' : '-',
						  (ch->type & (CHANNEL_OBJ_MULT *
									   CHANNEL_JOIN)) ? 'j' : '-',
						  (ch->type & (CHANNEL_OBJ_MULT *
									   CHANNEL_TRANSMIT)) ? 'x' : '-',
						  (ch->type & (CHANNEL_OBJ_MULT *
									   CHANNEL_RECIEVE)) ? 'r' : '-',
						  (ch->chan_obj != NOTHING) ? ch->chan_obj : -1,
						  ch->charge_who, ch->charge, ch->amount_col,
						  ch->num_users, ch->num_messages);
		}
	raw_notify(player, "-- End of list of Channels --");
}

void do_comtitle(dbref player, dbref cause, int key, char *arg1, char *arg2)
{
	struct channel *ch;
	char channel[100];

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	if(!*arg1) {
		raw_notify(player, "Need an alias to do comtitle.");
		return;
	}
	strncpy(channel, get_channel_from_alias(player, arg1), 100);
	channel[99] = '\0';

	if(!*channel) {
		raw_notify(player, "Unknown alias");
		return;
	}
	if((ch = select_channel(channel)) && select_user(ch, player)) {
		notify_printf(player, "Title set to '%s' on channel %s.",
					  arg2, channel);
		do_setnewtitle(player, ch, arg2);
	}
	if(!ch) {
		raw_notify(player, "Invalid comsys alias, please delete.");
		return;
	}
}

void do_comlist(dbref player, dbref cause, int key)
{
	struct comuser *user;
	struct commac *c;
	int i;

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	c = get_commac(player);

	raw_notify(player,
			   "Alias     Channel             Title                                   Status");

	for(i = 0; i < c->numchannels; i++) {
		if((user = select_user(select_channel(c->channels[i]), player))) {
			notify_printf(player, "%-9.9s %-19.19s %-39.39s %s",
						  c->alias + i * 6, c->channels[i],
						  user->title, (user->on ? "on" : "off"));
		} else {
			notify_printf(player, "Bad Comsys Alias: %s for Channel: %s",
						  c->alias + i * 6, c->channels[i]);
		}
	}
	raw_notify(player, "-- End of comlist --");
}

void do_channelnuke(dbref player)
{
	struct channel *ch;
	int j;

	for(ch = (struct channel *) hash_firstentry(&mudstate.channel_htab);
		ch; ch = (struct channel *) hash_nextentry(&mudstate.channel_htab)) {
		if(ch->charge_who == player) {
			num_channels--;
			hashdelete(ch->name, &mudstate.channel_htab);

			for(j = 0; j < ch->num_users; j++)
				free(ch->users[j]);
			free(ch->users);
			free(ch);
		}
	}
}

void do_clearcom(dbref player, dbref cause, int key)
{
	int i;
	struct commac *c;

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	c = get_commac(player);

	for(i = (c->numchannels) - 1; i > -1; --i) {
		do_delcom(player, player, 0, c->alias + i * 6);
	}
}

void do_allcom(dbref player, dbref cause, int key, char *arg1)
{
	int i;
	struct commac *c;

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	c = get_commac(player);

	if((strcasecmp(arg1, "who") != 0) && (strcasecmp(arg1, "on") != 0) &&
	   (strcasecmp(arg1, "off") != 0)) {
		raw_notify(player, "Only options available are: on, off and who.");
		return;
	}
	for(i = 0; i < c->numchannels; i++) {
		do_processcom(player, c->channels[i], arg1);
		if(strcasecmp(arg1, "who") == 0)
			raw_notify(player, "");
	}

}

extern void sort_users(struct channel *ch)
{
	int i;
	int nu;
	int done;
	struct comuser *user;

	nu = ch->num_users;
	done = 0;
	while (!done) {
		done = 1;
		for(i = 0; i < (nu - 1); i++) {
			if(ch->users[i]->who > ch->users[i + 1]->who) {
				user = ch->users[i];
				ch->users[i] = ch->users[i + 1];
				ch->users[i + 1] = user;
				done = 0;
			}
		}
	}
}

void do_channelwho(dbref player, dbref cause, int key, char *arg1)
{
	struct channel *ch;
	struct comuser *user;
	char channel[100];
	int flag = 0;
	char *cp;
	int i;
	char ansibuffer[LBUF_SIZE];
	char outputbuffer[LBUF_SIZE];

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	cp = strchr(arg1, '/');
	if(!cp) {
		strncpy(channel, arg1, 100);
		channel[99] = '\0';
	} else {
		/* channelname/all */
		if(cp - arg1 >= 100) {
			raw_notify(player, "Channel name too long.");
			return;
		}
		strncpy(channel, arg1, cp - arg1);
		channel[cp - arg1] = '\0';
		if(*++cp == 'a')
			flag = 1;
	}

	if(!(ch = select_channel(channel))) {
		notify_printf(player, "Unknown channel \"%s\".", channel);
		return;
	}
	if(!((Comm_All(player)) || (player == ch->charge_who))) {
		raw_notify(player, "You do not have permission to do that.");
		return;
	}
	notify_printf(player, "-- %s --", ch->name);
	notify_printf(player, "%-29.29s %-6.6s %-6.6s", "Name", "Status",
				  "Player");
	for(i = 0; i < ch->num_users; i++) {
		user = ch->users[i];
		if((flag || UNDEAD(user->who)) && (!Hidden(user->who) ||
										   ((ch->type & CHANNEL_TRANSPARENT)
											&& !Dark(user->who))
										   || Wizard_Who(player))) {
			cp = unparse_object(player, user->who, 0);
			strip_ansi_r(ansibuffer, cp, LBUF_SIZE);
			notify_printf(player, "%-29.29s %-6.6s %-6.6s", ansibuffer,
						  ((user->on) ? "on " : "off"),
						  (Typeof(user->who) == TYPE_PLAYER) ? "yes" : "no ");
			free_lbuf(cp);
		}
	}
	notify_printf(player, "-- %s --", ch->name);
}

void do_comdisconnectraw_notify(dbref player, char *chan)
{
	struct channel *ch;
	struct comuser *cu;

	if(!(ch = select_channel(chan)))
		return;
	if(!(cu = select_user(ch, player)))
		return;

	if((ch->type & CHANNEL_LOUD) && (cu->on) && (!Dark(player))) {
		do_comprintf(ch, "[%s] %s has disconnected.", ch->name, Name(player));
	}
}

void do_comconnectraw_notify(dbref player, char *chan)
{
	struct channel *ch;
	struct comuser *cu;

	if(!(ch = select_channel(chan)))
		return;
	if(!(cu = select_user(ch, player)))
		return;

	if((ch->type & CHANNEL_LOUD) && (cu->on) && (!Dark(player))) {
		do_comprintf(ch, "[%s] %s has connected.", ch->name, Name(player));
	}
}

void do_comconnectchannel(dbref player, char *channel, char *alias, int i)
{
	struct channel *ch;
	struct comuser *user;

	if((ch = select_channel(channel))) {
		for(user = ch->on_users; user && user->who != player;
			user = user->on_next);

		if(!user) {
			if((user = select_user(ch, player))) {
				user->on_next = ch->on_users;
				ch->on_users = user;
			} else
				notify_printf(player, "Bad Comsys Alias: %s for Channel: %s",
							  alias + i * 6, channel);
		}
	} else
		notify_printf(player, "Bad Comsys Alias: %s for Channel: %s",
					  alias + i * 6, channel);
}

void do_comdisconnect(dbref player)
{
	int i;
	struct commac *c;

	c = get_commac(player);

	for(i = 0; i < c->numchannels; i++) {
		do_comdisconnectchannel(player, c->channels[i]);
		do_comdisconnectraw_notify(player, c->channels[i]);
	}
	send_channel("MUXConnections", "* %s has disconnected *", Name(player));
}

void do_comconnect(dbref player, DESC * d)
{
	struct commac *c;
	int i;
	char *lsite;

	c = get_commac(player);

	for(i = 0; i < c->numchannels; i++) {
		do_comconnectchannel(player, c->channels[i], c->alias, i);
		do_comconnectraw_notify(player, c->channels[i]);
	}
	lsite = d->addr;
	if(lsite && *lsite)
		send_channel("MUXConnections","* %s has connected from %s *", Name(player), lsite);
	else
		send_channel("MUXConnections","* %s has connected from somewhere *", Name(player));
}

void do_comdisconnectchannel(dbref player, char *channel)
{
	struct comuser *user, *prevuser = NULL;
	struct channel *ch;

	if(!(ch = select_channel(channel)))
		return;
	for(user = ch->on_users; user;) {
		if(user->who == player) {
			if(prevuser)
				prevuser->on_next = user->on_next;
			else
				ch->on_users = user->on_next;
			return;
		} else {
			prevuser = user;
			user = user->on_next;
		}
	}
}

void do_editchannel(dbref player, dbref cause, int flag, char *arg1,
					char *arg2)
{
	char *s;
	struct channel *ch;
	int add_remove = 1;

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	if(!(ch = select_channel(arg1))) {
		notify_printf(player, "Unknown channel %s.", arg1);
		return;
	}
	if(!((Comm_All(player)) || (player == ch->charge_who))) {
		raw_notify(player, "Permission denied.");
		return;
	}
	s = arg2;
	if(*s == '!') {
		add_remove = 0;
		s++;
	}
	switch (flag) {
	case 0:
		if(lookup_player(player, arg2, 1) != NOTHING) {
			ch->charge_who = lookup_player(player, arg2, 1);
			raw_notify(player, "Set.");
			return;
		} else {
			raw_notify(player, "Invalid player.");
			return;
		}
	case 1:
		ch->charge = atoi(arg2);
		raw_notify(player, "Set.");
		return;
	case 3:
		if(strcasecmp(s, "join") == 0) {
			add_remove ? (ch->type |=
						  (CHANNEL_PL_MULT * CHANNEL_JOIN)) : (ch->type &=
															   ~
															   (CHANNEL_PL_MULT
																*
																CHANNEL_JOIN));
			raw_notify(player,
					   (add_remove) ? "@cpflags: Set." :
					   "@cpflags: Cleared.");
			return;
		}
		if(strcasecmp(s, "receive") == 0) {
			add_remove ? (ch->type |=
						  (CHANNEL_PL_MULT * CHANNEL_RECIEVE)) : (ch->type &=
																  ~
																  (CHANNEL_PL_MULT
																   *
																   CHANNEL_RECIEVE));
			raw_notify(player,
					   (add_remove) ? "@cpflags: Set." :
					   "@cpflags: Cleared.");
			return;
		}
		if(strcasecmp(s, "transmit") == 0) {
			add_remove ? (ch->type |=
						  (CHANNEL_PL_MULT * CHANNEL_TRANSMIT)) : (ch->type &=
																   ~
																   (CHANNEL_PL_MULT
																	*
																	CHANNEL_TRANSMIT));
			raw_notify(player,
					   (add_remove) ? "@cpflags: Set." :
					   "@cpflags: Cleared.");
			return;
		}
		raw_notify(player, "@cpflags: Unknown Flag.");
		break;
	case 4:
		if(strcasecmp(s, "join") == 0) {
			add_remove ? (ch->type |=
						  (CHANNEL_OBJ_MULT * CHANNEL_JOIN)) : (ch->type &=
																~
																(CHANNEL_OBJ_MULT
																 *
																 CHANNEL_JOIN));
			raw_notify(player,
					   (add_remove) ? "@coflags: Set." :
					   "@coflags: Cleared.");
			return;
		}
		if(strcasecmp(s, "receive") == 0) {
			add_remove ? (ch->type |=
						  (CHANNEL_OBJ_MULT * CHANNEL_RECIEVE)) : (ch->type &=
																   ~
																   (CHANNEL_OBJ_MULT
																	*
																	CHANNEL_RECIEVE));
			raw_notify(player,
					   (add_remove) ? "@coflags: Set." :
					   "@coflags: Cleared.");
			return;
		}
		if(strcasecmp(s, "transmit") == 0) {
			add_remove ? (ch->type |=
						  (CHANNEL_OBJ_MULT *
						   CHANNEL_TRANSMIT)) : (ch->type &=
												 ~(CHANNEL_OBJ_MULT *
												   CHANNEL_TRANSMIT));
			raw_notify(player,
					   (add_remove) ? "@coflags: Set." :
					   "@coflags: Cleared.");
			return;
		}
		raw_notify(player, "@coflags: Unknown Flag.");
		break;
	}
	return;
}

static int do_test_access(dbref player, long access, struct channel *chan)
{
	long flag_value = access;

	if(Comm_All(player))
		return (1);

	/*
	 * Channel objects allow custom locks for channels.  The normal
	 * lock is used to see if they can join that channel.  The
	 * enterlock is checked to see if they can receive messages on
	 * it. The Uselock is checked to see if they can transmit on
	 * it. Note: These checks do not supercede the normal channel
	 * flags. If a channel is set JOIN for players, ALL players can
	 * join the channel, whether or not they pass the lock.  Same for
	 * all channel object locks.
	 */

	if((flag_value & CHANNEL_JOIN) && !((chan->chan_obj == NOTHING) ||
										(chan->chan_obj == 0))) {
		if(could_doit(player, chan->chan_obj, A_LOCK))
			return (1);
	}
	if((flag_value & CHANNEL_TRANSMIT) && !((chan->chan_obj == NOTHING) ||
											(chan->chan_obj == 0))) {
		if(could_doit(player, chan->chan_obj, A_LUSE))
			return (1);
	}
	if((flag_value & CHANNEL_RECIEVE) && !((chan->chan_obj == NOTHING) ||
										   (chan->chan_obj == 0))) {
		if(could_doit(player, chan->chan_obj, A_LENTER))
			return (1);
	}
	if(Typeof(player) == TYPE_PLAYER)
		flag_value *= CHANNEL_PL_MULT;
	else
		flag_value *= CHANNEL_OBJ_MULT;
	flag_value &= 0xFF;			/*
								 * Mask out CHANNEL_PUBLIC and CHANNEL_LOUD
								 * just to be paranoid. 
								 */

	return (((long) chan->type & flag_value));
}

int do_comsystem(dbref who, char *cmd)
{
	char *t;
	char *ch;
	char *alias;
	char *s;

	alias = alloc_lbuf("do_comsystem");
	s = alias;
	for(t = cmd; *t && *t != ' '; *s++ = *t++)
		/* nothing */ ;

	*s = '\0';

	if(*t)
		t++;

	ch = get_channel_from_alias(who, alias);
	if(ch && *ch) {
		do_processcom(who, ch, t);
		free_lbuf(alias);
		return 0;
	}
	free_lbuf(alias);
	return 1;

}

void do_chclose(dbref player, char *chan)
{
	struct channel *ch;

	if(!(ch = select_channel(chan))) {
		notify_printf(player, "@cset: Channel %s does not exist.", chan);
		return;
	}
	if((player != ch->charge_who) && (!Comm_All(player))) {
		raw_notify(player, "@cset: Permission denied.");
		return;
	}
	ch->type &= (~(CHANNEL_PUBLIC));
	notify_printf(player,
				  "@cset: Channel %s taken off the public listings.", chan);
	return;
}

void do_cemit(dbref player, dbref cause, int key, char *chan, char *text)
{
	struct channel *ch;

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	if(!(ch = select_channel(chan))) {
		notify_printf(player, "Channel %s does not exist.", chan);
		return;
	}
	if((player != ch->charge_who) && (!Comm_All(player))) {
		raw_notify(player, "Permission denied.");
		return;
	}
	if(key == CEMIT_NOHEADER)
		do_comsend(ch, text);
	else
		do_comprintf(ch, "[%s] %s", chan, text);
}

void do_chopen(dbref player, dbref cause, int key, char *chan, char *object)
{
	struct channel *ch;

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	switch (key) {
	case CSET_PRIVATE:
		do_chclose(player, chan);
		return;
	case CSET_LOUD:
		do_chloud(player, chan);
		return;
	case CSET_QUIET:
		do_chsquelch(player, chan);
		return;
	case CSET_LIST:
		do_chanlist(player, NOTHING, 1);
		return;
	case CSET_OBJECT:
		do_chanobj(player, chan, object);
		return;
	case CSET_STATUS:
		do_chanstatus(player, NOTHING, 1, chan);
		return;
	case CSET_TRANSPARENT:
		do_chtransparent(player, chan);
		return;
	case CSET_OPAQUE:
		do_chopaque(player, chan);
		return;
	}

	if(!(ch = select_channel(chan))) {
		notify_printf(player, "@cset: Channel %s does not exist.", chan);
		return;
	}
	if((player != ch->charge_who) && (!Comm_All(player))) {
		raw_notify(player, "@cset: Permission denied.");
		return;
	}
	ch->type |= (CHANNEL_PUBLIC);
	notify_printf(player, "@cset: Channel %s placed on the public listings.",
				  chan);
	return;
}

void do_chloud(dbref player, char *chan)
{
	struct channel *ch;

	if(!(ch = select_channel(chan))) {
		notify_printf(player, "@cset: Channel %s does not exist.", chan);
		return;
	}
	if((player != ch->charge_who) && (!Comm_All(player))) {
		raw_notify(player, "@cset: Permission denied.");
		return;
	}
	ch->type |= (CHANNEL_LOUD);
	notify_printf(player,
				  "@cset: Channel %s now sends connect/disconnect msgs.",
				  chan);
	return;
}

void do_chsquelch(dbref player, char *chan)
{
	struct channel *ch;

	if(!(ch = select_channel(chan))) {
		notify_printf(player, "@cset: Channel %s does not exist.", chan);
		return;
	}
	if((player != ch->charge_who) && (!Comm_All(player))) {
		raw_notify(player, "@cset: Permission denied.");
		return;
	}
	ch->type &= ~(CHANNEL_LOUD);
	notify_printf(player,
				  "@cset: Channel %s connect/disconnect msgs muted.", chan);
	return;
}

void do_chtransparent(dbref player, char *chan)
{
	struct channel *ch;

	if(!(ch = select_channel(chan))) {
		notify_printf(player, "@cset: Channel %s does not exist.", chan);
		return;
	}
	if((player != ch->charge_who) && (!Comm_All(player))) {
		raw_notify(player, "@cset: Permission denied.");
		return;
	}
	ch->type |= CHANNEL_TRANSPARENT;
	notify_printf(player,
				  "@cset: Channel %s now shows all listeners to everyone.",
				  chan);
	return;
}

void do_chopaque(dbref player, char *chan)
{
	struct channel *ch;

	if(!(ch = select_channel(chan))) {
		notify_printf(player, "@cset: Channel %s does not exist.", chan);
		return;
	}
	if((player != ch->charge_who) && (!Comm_All(player))) {
		raw_notify(player, "@cset: Permission denied.");
		return;
	}
	ch->type &= ~CHANNEL_TRANSPARENT;
	notify_printf(player,
				  "@cset: Channel %s now does not show all listeners to everyone.",
				  chan);
	return;
}

void do_chboot(dbref player, dbref cause, int key, char *channel,
			   char *victim)
{
	struct comuser *user;
	struct channel *ch;
	struct comuser *vu;
	dbref thing;

	/*
	 * * I sure hope it's not going to be that *
	 * *  * *  * *  * * long.  
	 */

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	if(!(ch = select_channel(channel))) {
		raw_notify(player, "@cboot: Unknown channel.");
		return;
	}
	if(!(user = select_user(ch, player))) {
		raw_notify(player, "@cboot: You are not on that channel.");
		return;
	}
	if(!((ch->charge_who == player) || Comm_All(player))) {
		raw_notify(player, "Permission denied.");
		return;
	}
	thing = match_thing(player, victim);

	if(thing == NOTHING) {
		return;
	}
	if(!(vu = select_user(ch, thing))) {
		notify_printf(player, "@cboot: %s in not on the channel.",
					  Name(thing));
		return;
	}
	/*
	 * We should be in the clear now. :) 
	 */
	do_comprintf(ch, "[%s] %s boots %s off the channel.", ch->name,
               unparse_object_numonly(player), unparse_object_numonly(thing));
	do_delcomchannel(thing, channel);

}

void do_chanobj(dbref player, char *channel, char *object)
{
	struct channel *ch;
	dbref thing;
	char *buff;

	init_match(player, object, NOTYPE);
	match_everything(0);
	thing = match_result();

	if(!(ch = select_channel(channel))) {
		raw_notify(player, "That channel does not exist.");
		return;
	}
	if(thing == NOTHING) {
		ch->chan_obj = NOTHING;
		raw_notify(player, "Set.");
		return;
	}
	if(!(ch->charge_who == player) && !Comm_All(player)) {
		raw_notify(player, "Permission denied.");
		return;
	}
	ch->chan_obj = thing;
	buff = unparse_object(player, thing, 0);
	notify_printf(player,
				  "Channel %s is now using %s as channel object.", ch->name,
				  buff);
	free_lbuf(buff);
}

void do_chanlist(dbref player, dbref cause, int key)
{
	dbref owner;
	struct channel *ch;
	int flags;
	char *temp;
	char *buf;
	char *atrstr;

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}
	flags = (int) NULL;

	if(key & CLIST_FULL) {
		do_listchannels(player);
		return;
	}
	temp = alloc_mbuf("do_chanlist_temp");
	buf = alloc_mbuf("do_chanlist_buf");

	raw_notify(player, "** Channel       Owner           Description");

	for(ch = (struct channel *) hash_firstentry(&mudstate.channel_htab);
		ch; ch = (struct channel *) hash_nextentry(&mudstate.channel_htab)) {
		if(Comm_All(player) || (ch->type & CHANNEL_PUBLIC) ||
		   ch->charge_who == player) {

			atrstr = atr_pget(ch->chan_obj, A_DESC, &owner, &flags);
			if((ch->chan_obj == NOTHING) || !*atrstr)
				sprintf(buf, "%s", "No description.");
			else
				sprintf(buf, "%-54.54s", atrstr);

			free_lbuf(atrstr);
			sprintf(temp, "%c%c %-13.13s %-15.15s %-45.45s",
					(ch->type & (CHANNEL_PUBLIC)) ? 'P' : '-',
					(ch->type & (CHANNEL_LOUD)) ? 'L' : '-', ch->name,
					Name(ch->charge_who), buf);

			raw_notify(player, temp);
		}
	}
	free_mbuf(temp);
	free_mbuf(buf);
	raw_notify(player, "-- End of list of Channels --");
}

void do_chanstatus(dbref player, dbref cause, int key, char *chan)
{
	dbref owner;
	struct channel *ch;
	int flags;
	char *temp;
	char *buf;
	char *atrstr;

	if(!mudconf.have_comsys) {
		raw_notify(player, "Comsys disabled.");
		return;
	}

	if(key & CSTATUS_FULL) {
		struct channel *ch;
		int perm;

		if(!(perm = Comm_All(player))) {
			raw_notify(player,
					   "Warning: Only public channels and your channels will be shown.");
		}
		raw_notify(player,
				   "** Channel             --Flags--  Obj   Own   Charge  Balance  Users   Messages");

		if(!(ch = select_channel(chan))) {
			notify_printf(player,
						  "@cstatus: Channel %s does not exist.", chan);
			return;
		}
		if(perm || (ch->type & CHANNEL_PUBLIC) || ch->charge_who == player) {

			notify_printf(player,
						  "%c%c %-20.20s %c%c%c/%c%c%c %5d %5d %8d %8d %6d %10d",
						  (ch->type & (CHANNEL_PUBLIC)) ? 'P' : '-',
						  (ch->type & (CHANNEL_LOUD)) ? 'L' : '-', ch->name,
						  (ch->
						   type & (CHANNEL_PL_MULT *
								   CHANNEL_JOIN)) ? 'J' : '-',
						  (ch->
						   type & (CHANNEL_PL_MULT *
								   CHANNEL_TRANSMIT)) ? 'X' : '-',
						  (ch->
						   type & (CHANNEL_PL_MULT *
								   CHANNEL_RECIEVE)) ? 'R' : '-',
						  (ch->
						   type & (CHANNEL_OBJ_MULT *
								   CHANNEL_JOIN)) ? 'j' : '-',
						  (ch->
						   type & (CHANNEL_OBJ_MULT *
								   CHANNEL_TRANSMIT)) ? 'x' : '-',
						  (ch->
						   type & (CHANNEL_OBJ_MULT *
								   CHANNEL_RECIEVE)) ? 'r' : '-',
						  (ch->chan_obj != NOTHING) ? ch->chan_obj : -1,
						  ch->charge_who, ch->charge, ch->amount_col,
						  ch->num_users, ch->num_messages);
		}
		raw_notify(player, "-- End of list of Channels --");
		return;
	}
	temp = alloc_mbuf("do_chanstatus_temp");
	buf = alloc_mbuf("do_chanstatus_buf");

	raw_notify(player, "** Channel       Owner           Description");
	if(!(ch = select_channel(chan))) {
		notify_printf(player, "@cstatus: Channel %s does not exist.", chan);
		return;
	}
	if(Comm_All(player) || (ch->type & CHANNEL_PUBLIC) ||
	   ch->charge_who == player) {

		atrstr = atr_pget(ch->chan_obj, A_DESC, &owner, &flags);
		if((ch->chan_obj == NOTHING) || !*atrstr)
			sprintf(buf, "%s", "No description.");
		else
			sprintf(buf, "%-54.54s", atrstr);

		free_lbuf(atrstr);
		snprintf(temp, MBUF_SIZE, "%c%c %-13.13s %-15.15s %-45.45s",
				 (ch->type & (CHANNEL_PUBLIC)) ? 'P' : '-',
				 (ch->type & (CHANNEL_LOUD)) ? 'L' : '-', ch->name,
				 Name(ch->charge_who), buf);

		raw_notify(player, temp);
	}
	free_mbuf(temp);
	free_mbuf(buf);
	raw_notify(player, "-- End of list of Channels --");
}

void fun_cemit(char *buff, char **bufc, dbref player, dbref cause,
			   char *fargs[], char *nfargs[], int cargs, int ncargs)
{
	struct channel *ch;
	static char smbuf[LBUF_SIZE];

	if(!(ch = select_channel(fargs[0]))) {
		safe_str("#-1 CHANNEL NOT FOUND", buff, bufc);
		return;
	}

	if(!mudconf.have_comsys
	   || (!Comm_All(player) && (player != ch->charge_who))) {
		safe_str("#-1 NO PERMISSION TO USE", buff, bufc);
		return;
	}

	do_comprintf(ch, "[%s] %s", fargs[0], fargs[1]);
	*buff = '\0';
}
