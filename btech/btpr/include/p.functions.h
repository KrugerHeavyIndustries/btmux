
/*
   p.functions.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Thu Mar 11 17:43:47 CET 1999 from functions.c */

#ifndef _P_FUNCTIONS_H
#define _P_FUNCTIONS_H

/* functions.c */
char *trim_space_sep(char *str, int sep);
char *next_token(char *str, int sep);
char *split_token(char **sp, int sep);
dbref match_thing(dbref player, char *name);
int list2arr(char *arr[], int maxlen, char *list, int sep);
void arr2list(char *arr[], int alen, char *list, char **bufc, int sep);
int nearby_or_control(dbref player, dbref thing);
int fn_range_check(const char *fname, int nfargs, int minargs, int maxargs,
    char *result, char **bufc);
int delim_check(char *fargs[], int nfargs, int sep_arg, char *sep,
    char *buff, char **bufc, int eval, dbref player, dbref cause,
    char *cargs[], int ncargs);
int countwords(char *str, int sep);
time_t mytime(dbref player);
int do_convtime(char *str, struct tm *ttm);
char *get_uptime_to_string(int uptime);
char *get_uptime_to_short_string(int uptime);
int xlate(char *arg);
void init_functab(void);
void do_function(dbref player, dbref cause, int key, char *fname,
    char *target);
void list_functable(dbref player);
int cf_func_access(int *vp, char *str, long extra, dbref player,
    char *cmd);

#endif				/* _P_FUNCTIONS_H */
