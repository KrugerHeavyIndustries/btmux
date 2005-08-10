
/*
   p.autopilot_commands.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:35 CET 1999 from autopilot_commands.c */

#ifndef _P_AUTOPILOT_COMMANDS_H
#define _P_AUTOPILOT_COMMANDS_H

/* autopilot_commands.c */
int auto_valid_progline(AUTO * a, int p);
void auto_delcommand(dbref player, void *data, char *buffer);
void auto_jump(dbref player, void *data, char *buffer);
void auto_addcommand(dbref player, void *data, char *buffer);
void auto_listcommands(dbref player, void *data, char *buffer);
int AutoPilotOn(AUTO * a);
void StopAutoPilot(AUTO * a);
void ai_set_comtitle(AUTO * a, MECH * mech);
void auto_engage(dbref player, void *data, char *buffer);
void auto_disengage(dbref player, void *data, char *buffer);

#endif				/* _P_AUTOPILOT_COMMANDS_H */
