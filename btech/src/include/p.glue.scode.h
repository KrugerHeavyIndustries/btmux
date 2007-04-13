
/*
   p.glue.scode.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Feb 22 14:59:38 CET 1999 from glue.scode.c */

#ifndef _P_GLUE_SCODE_H
#define _P_GLUE_SCODE_H

/* glue.scode.c */
char *mechIDfunc(int mode, MECH * mech);
char *mechTypefunc(int mode, MECH * mech, char *arg);
char *mechMovefunc(int mode, MECH * mech, char *arg);
char *mechTechTimefunc(int mode, MECH * mech);
void apply_mechDamage(MECH * omech, char *buf);
char *mechDamagefunc(int mode, MECH * mech, char *arg);
char *mechCentBearingfunc(int mode, MECH * mech, char *arg);
char *mechCentDistfunc(int mode, MECH * mech, char *arg);
void fun_btsetxcodevalue(char *buff, char **bufc, dbref player,
    dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs);
void fun_btgetxcodevalue(char *buff, char **bufc, dbref player,
    dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs);
void set_xcodestuff(dbref player, void *data, char *buffer);
void list_xcodestuff(dbref player, void *data, char *buffer);
void fun_btunderrepair(char *buff, char **bufc, dbref player, dbref cause,
    char *fargs[], int nfargs, char *cargs[], int ncargs);
void fun_btstores(char *buff, char **bufc, dbref player, dbref cause,
    char *fargs[], int nfargs, char *cargs[], int ncargs);
void fun_btmapterr(char *buff, char **bufc, dbref player, dbref cause,
    char *fargs[], int nfargs, char *cargs[], int ncargs);
void fun_btmapelev(char *buff, char **bufc, dbref player, dbref cause,
    char *fargs[], int nfargs, char *cargs[], int ncargs);
void list_xcodevalues(dbref player);
void fun_btdesignex(char *buff, char **bufc, dbref player, dbref cause,
    char *fargs[], int nfargs, char *cargs[], int ncargs);
void fun_btdamages(char *buff, char **bufc, dbref player, dbref cause,
    char *fargs[], int nfargs, char *cargs[], int ncargs);
void fun_btcritstatus(char *buff, char **bufc, dbref player, dbref cause,
    char *fargs[], int nfargs, char *cargs[], int ncargs);
void fun_btarmorstatus(char *buff, char **bufc, dbref player, dbref cause,
    char *fargs[], int nfargs, char *cargs[], int ncargs);

#endif				/* _P_GLUE_SCODE_H */
