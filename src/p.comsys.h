
/*
   p.comsys.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Thu Mar 11 17:43:42 CET 1999 from comsys.c */

#ifndef _P_COMSYS_H
#define _P_COMSYS_H

/* comsys.c */
void init_chantab(void);
void send_channel(char *chan, const char *format, ...);
char *get_channel_from_alias(dbref player, char *alias);
void load_comsystem(FILE * fp);
void save_comsystem(FILE * fp);
void do_processcom(dbref player, char *arg1, char *arg2);
struct channel *select_channel(char *channel);
struct comuser *select_user(struct channel *ch, dbref player);
void do_addcom(dbref player, dbref cause, int key, char *arg1, char *arg2);
void do_delcom(dbref player, dbref cause, int key, char *arg1);
void do_delcomchannel(dbref player, char *channel);
void do_createchannel(dbref player, dbref cause, int key, char *channel);
void do_destroychannel(dbref player, dbref cause, int key, char *channel);
void do_listchannels(dbref player);
void do_comtitle(dbref player, dbref cause, int key, char *arg1,
    char *arg2);
void do_comlist(dbref player, dbref cause, int key);
void do_channelnuke(dbref player);
void do_clearcom(dbref player, dbref cause, int key);
void do_allcom(dbref player, dbref cause, int key, char *arg1);
void do_channelwho(dbref player, dbref cause, int key, char *arg1);
void do_comdisconnectraw_notify(dbref player, char *chan);
void do_comconnectraw_notify(dbref player, char *chan);
void do_comconnectchannel(dbref player, char *channel, char *alias, int i);
void do_comdisconnect(dbref player);
void do_comconnect(dbref player, DESC * d);
void do_comdisconnectchannel(dbref player, char *channel);
void do_editchannel(dbref player, dbref cause, int flag, char *arg1,
    char *arg2);
int do_comsystem(dbref who, char *cmd);
void do_chclose(dbref player, char *chan);
void do_cemit(dbref player, dbref cause, int key, char *chan, char *text);
void do_chopen(dbref player, dbref cause, int key, char *chan,
    char *object);
void do_chloud(dbref player, char *chan);
void do_chsquelch(dbref player, char *chan);
void do_chtransparent(dbref player, char *chan);
void do_chopaque(dbref player, char *chan);
void do_chboot(dbref player, dbref cause, int key, char *channel,
    char *victim);
void do_chanobj(dbref player, char *channel, char *object);
void do_chanstatus(dbref player, dbref cause, int key, char *chan);
void do_chanlist(dbref player, dbref cause, int key);


#endif				/* _P_COMSYS_H */
