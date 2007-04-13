
/*
 * $Id: coolmenu.h,v 1.1 2005/06/13 20:50:52 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Mon Sep 16 20:38:54 1996 fingon
 * Last modified: Wed Jun 24 22:37:38 1998 fingon
 *
 */

#ifndef COOLMENU_H
#define COOLMENU_H

/* #define MAX_MENU_LENGTH 24 */
#define MAX_MENU_LENGTH 400
#define MAX_MENU_WIDTH  240
#define MENU_CHAR_WIDTH 78

/*
non-ticked toggle
   [a] ....    < >
ticked toggle
   [A] ....    <X>
value
   [a] ....    ___
string that can be changed
   [a] ...
string
   ...
   */

#define CM_ONE        0x001	/* Just one / line */
#define CM_TWO        0x002	/* Two / line */
#define CM_THREE      0x004	/* Three / line */
#define CM_FOUR       0x008	/* Four / line */
#define CM_CENTER     0x010	/* Stuff's centered, not left-edge */
#define CM_TOGGLE     0x020	/* Field that can be toggled */
#define CM_NUMBER     0x040	/* Field with number in it (add/lower) */
#define CM_LINE       0x080	/* No text, just blank line */
#define CM_STRING     0x100	/* String with letter ahead of it */
#define CM_NO_HILITE  0x200	/* No extra highlight */
#define CM_NOTOG      0x400	/* Not really toggleable */
#define CM_NORIGHT    0x800	/* No right-end field */
#define CM_NOCUT      0x1000	/* Turn off cutoff */

#define LETTERFIRST (CM_TOGGLE|CM_NUMBER|CM_STRING)
#define RIGHTEDGES  (CM_TOGGLE|CM_NUMBER)

typedef struct coolmenu_type {
    int id;			/* Used for some purposes by external agency */
    char *text;			/* Text (varies) */
    int value;			/* toggle = 0/1, number=0-999 */
    int maxvalue;		/* if maxvalue's < 999 */
    char letter;		/* Letter allocated to this entry */
    int flags;			/* This entry's flags */
    struct coolmenu_type *next;
} coolmenu;

#define CreateMenuEntry_VSimple(c,text) \
     CreateMenuEntry_Normal(c, text, CM_ONE, 0, 999)
#define CreateMenuEntry_Simple(c,text,flag) \
     CreateMenuEntry_Normal(c, text, flag, 0, 999)
#define CreateMenuEntry_Normal(c,text,flag,id,mv) \
     CreateMenuEntry_Killer(c, text, flag, id, 0, mv)
void CreateMenuEntry_Killer(coolmenu ** c, char *text, int flag, int id,
    int value, int maxvalue);

void KillCoolMenu(coolmenu * c);
void ShowCoolMenu(dbref player, coolmenu * c);
char **MakeCoolMenuText(coolmenu * c);
int CoolMenu_FPWBit(int number, int maxlen);

/* Automated 'nice' looking menus: */
coolmenu *SelCol_Menu(int columns, char *heading, char **strings, int type,
    int max);

/* last = how many entries we have */
coolmenu *SelCol_FunStringMenuK(int columns, char *heading,
    char *(*fun) (), int last);

/* Same, except we dunno how many entries we got */
coolmenu *SelCol_FunStringMenu(int columns, char *heading,
    char *(*fun) ());

#define AutoCol_Menu(hea,stri,typ) SelCol_Menu(-1,hea,stri,typ,0)
#define AutoCol_StringMenu(head,str)    AutoCol_Menu(head,str,0)
#define AutoCol_FunStringMenuK(hea,fun,las) \
   SelCol_FunStringMenuK(-1,hea,fun,las)
#define AutoCol_FunStringMenu(hea,fun) \
   SelCol_FunStringMenuK(-1,hea,fun)
#define SelCol_StringMenu(col,head,str) SelCol_Menu(col,head,str,0,0)

#endif				/* COOLMENU_H */
