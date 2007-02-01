
/*
   p.glue.hcode.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Feb 22 14:59:37 CET 1999 from glue.hcode.c */

#ifndef _P_GLUE_HCODE_H
#define _P_GLUE_HCODE_H

/* glue.hcode.c */
int bt_get_attr(char *tbuf, int obj, char *name);
char *silly_atr_get(int id, int flag);
void silly_atr_set(int id, int flag, char *dat);
void bt_set_attr(dbref obj, char *attri, char *value);
void KillText(char **mapt);
void ShowText(char **mapt, dbref player);
float FBOUNDED(float min, float val, float max);
int BOUNDED(int min, int val, int max);
int MAX(int v1, int v2);
int MIN(int v1, int v2);
int silly_parseattributes(char *buffer, char **args, int max);
int mech_parseattributes(char *buffer, char **args, int maxargs);
int proper_parseattributes(char *buffer, char **args, int max);
int proper_explodearguments(char *buffer, char **args, int max);
void proper_freearguments(char **args, int maxargs);
int proper_parseint(char *string, int *integer);
char *first_parseattribute(char *buffer);

#endif				/* _P_GLUE_HCODE_H */
