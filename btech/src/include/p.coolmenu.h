
/*
   p.coolmenu.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Feb 22 14:59:35 CET 1999 from coolmenu.c */

#ifndef _P_COOLMENU_H
#define _P_COOLMENU_H

/* coolmenu.c */
int number_of_entries(coolmenu * c);
int count_following_with(coolmenu * c, int num);
void display_line(char **c, int *len, coolmenu * m);
void display_string(char **c, int *len, coolmenu * m);
void display_toggle_end(char **c, coolmenu * m);
char *stringified_value(int v);
void display_number_end(char **c, coolmenu * m);
char *display_entry(char *ch, int maxlen, coolmenu * c);
void display_entries(coolmenu * c, int wnum, int num, char *text);
char **MakeCoolMenuText(coolmenu * c);
void CreateMenuEntry_Killer(coolmenu ** c, char *text, int flag, int id,
    int value, int maxvalue);
void KillCoolMenu(coolmenu * c);
void ShowCoolMenu(dbref player, coolmenu * c);
int CoolMenu_FPWBit(int number, int maxlen);
coolmenu *SelCol_Menu(int columns, char *heading, char **strings, int type,
    int max);
coolmenu *SelCol_FunStringMenuK(int columns, char *heading,
    char *(*fun) (), int last);
coolmenu *SelCol_FunStringMenu(int columns, char *heading,
    char *(*fun) ());

#endif				/* _P_COOLMENU_H */
