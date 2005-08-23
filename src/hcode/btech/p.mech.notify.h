
/*
   p.mech.notify.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:52 CET 1999 from mech.notify.c */

#ifndef _P_MECH_NOTIFY_H
#define _P_MECH_NOTIFY_H

/* mech.notify.c */
const char *GetAmmoDesc_Model_Mode(int model, int mode);
char GetWeaponAmmoModeLetter_Model_Mode(int model, int mode);
char GetWeaponFireModeLetter_Model_Mode(int model, int mode);
char GetWeaponAmmoModeLetter(MECH * mech, int loop, int crit);
char GetWeaponFireModeLetter(MECH * mech, int loop, int crit);
const char *GetMoveTypeID(int movetype);
void Mech_ShowFlags(dbref player, MECH * mech, int spaces, int level);
const char *GetArcID(MECH * mech, int arc);
const char *GetMechToMechID_base(MECH * see, MECH * mech, int i);
const char *GetMechToMechID(MECH * see, MECH * mech);
const char *GetMechID(MECH * mech);
void mech_set_channelfreq(dbref player, void *data, char *buffer);
void mech_set_channeltitle(dbref player, void *data, char *buffer);
void mech_set_channelmode(dbref player, void *data, char *buffer);
void mech_list_freqs(dbref player, void *data, char *buffer);
void mech_sendchannel(dbref player, void *data, char *buffer);
void ScrambleMessage(char *buffo, int range, int sendrange, int recvrrange,
    char *handle, char *msg, int bth, int *isxp, int under_ecm,
    int digmode);
int common_checks(dbref player, MECH * mech, int flag);
void recursive_commlink(int i, int dep);
void nonrecursive_commlink(int i);
int findCommLink(MAP * map, MECH * from, MECH * to, int freq);
void sendchannelstuff(MECH * mech, int freq, char *msg);
void mech_radio(dbref player, void *data, char *buffer);
void MechBroadcast(MECH * mech, MECH * target, MAP * mech_map,
    char *buffer);
void MechLOSBroadcast(MECH * mech, char *message);
int MechSeesHexF(MECH * mech, MAP * map, float x, float y, int ix, int iy);
int MechSeesHex(MECH * mech, MAP * map, int x, int y);
void HexLOSBroadcast(MAP * mech_map, int x, int y, char *message);
void MechLOSBroadcasti(MECH * mech, MECH * target, char *message);
void MapBroadcast(MAP * map, char *message);
void MechFireBroadcast(MECH * mech, MECH * target, int x, int y,
    MAP * mech_map, char *weapname, int IsHit);
void mech_notify(MECH * mech, int type, char *buffer);
void mech_printf(MECH * mech, int type, char *format, ...);

#endif				/* _P_MECH_NOTIFY_H */
