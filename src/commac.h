
/* commac.h */

/* $Id: commac.h,v 1.1 2005/06/13 20:50:46 murrayma Exp $ */

#ifndef __COMMAC_H__
#define __COMMAC_H__

struct commac {
    dbref who;

    int numchannels;
    int maxchannels;
    char *alias;
    char **channels;

    int curmac;
    int macros[5];

    struct commac *next;
};

#define NUM_COMMAC 500

struct commac *commac_table[NUM_COMMAC];

void load_commac();
void save_commac();
void purge_commac();

void sort_com_aliases();
struct commac *get_commac();
struct commac *create_new_commac();
void destroy_commac();
void add_commac();
void del_commac();
void save_comsys_and_macros(char *);
void load_comsys_and_macros(char *);

#endif				/* __COMMAC_H__ */
