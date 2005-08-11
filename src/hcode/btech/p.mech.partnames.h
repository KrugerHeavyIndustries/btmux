
/*
   p.mech.partnames.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:53 CET 1999 from mech.partnames.c */

#ifndef _P_MECH_PARTNAMES_H
#define _P_MECH_PARTNAMES_H

/* mech.partnames.c */
void list_phashstats(dbref player);
void initialize_partname_tables(void);
char *get_parts_short_name(int i, int b);
char *get_parts_long_name(int i, int b);
char *get_parts_vlong_name(int i, int b);
int find_matching_vlong_part(char *wc, int *ind, int *id, int *brand);
int find_matching_long_part(char *wc, int *i, int *id, int *brand);
int find_matching_short_part(char *wc, int *ind, int *id, int *brand);
void ListForms(dbref player, void *data, char *buffer);
char *partname_func(int index, int size);
#endif				/* _P_MECH_PARTNAMES_H */
