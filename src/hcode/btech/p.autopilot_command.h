
/*
   p.autopilot_command.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:35 CET 1999 from autopilot_command.c */

#ifndef _P_AUTOPILOT_COMMAND_H
#define _P_AUTOPILOT_COMMAND_H

/* autopilot_command.c */
int auto_parse_command_sub(AUTO * a, MECH * mech, char *buffer,
    char ***gargs, int *argc_n);
void auto_reply_event(EVENT * e);
void auto_reply(MECH * mech, char *buf);
void auto_replyA(MECH * mech, char *buf);
void auto_parse_command(AUTO * a, MECH * mech, int chn, char *buffer);

#endif				/* _P_AUTOPILOT_COMMAND_H */
