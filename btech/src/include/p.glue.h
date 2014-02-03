
/*
   p.glue.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Feb 22 14:59:36 CET 1999 from glue.c */

#ifndef _P_GLUE_H
#define _P_GLUE_H

/* glue.c */
int HandledCommand_sub(dbref player, dbref location, char *command);
int HandledCommand(dbref player, dbref loc, char *command);
void mech_remove_from_all_maps(MECH * mech);
void mech_remove_from_all_maps_except(MECH * mech, int num);
void zap_unneccessary_hcode(void);
void LoadSpecialObjects(void);
void ChangeSpecialObjects(int i);
void SaveSpecialObjects(int i);
void UpdateSpecialObjects(void);
void *NewSpecialObject(long id, int type);
void CreateNewSpecialObject(dbref player, dbref key);
void DisposeSpecialObject(dbref player, dbref key);
void Dump_Mech(dbref player, int type, char *typestr);
void DumpMechs(dbref player);
void DumpMaps(dbref player);
int WhichSpecial(dbref key);
int IsMech(dbref num);
int IsAuto(dbref num);
int IsMap(dbref num);
void *FindObjectsData(dbref key);
char *center_string(char *c, int len);
void InitSpecialHash(int which);
void handle_xcode(dbref player, dbref obj, int from, int to);
void initialize_colorize(void);
char *colorize(dbref player, char *from);
void mecha_notify(dbref player, char *msg);
void mecha_notify_except(dbref loc, dbref player, dbref exception,
    char *msg);
void list_chashstats(dbref player);
void ResetSpecialObjects(void);
MAP *getMap(dbref d);
MECH *getMech(dbref d);

#endif				/* _P_GLUE_H */
