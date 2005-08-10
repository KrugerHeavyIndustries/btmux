
/*
   p.mech.contacts.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:47 CET 1999 from mech.contacts.c */

#ifndef _P_MECH_CONTACTS_H
#define _P_MECH_CONTACTS_H

/* mech.contacts.c */
void show_brief_flags(dbref player, MECH * mech);
void mech_brief(dbref player, void *data, char *buffer);
void mech_contacts(dbref player, void *data, char *buffer);
char getWeaponArc(MECH * mech, int arc);
char *getStatusString(MECH * target, int enemy);
char getStatusChar(MECH * mech, MECH * mechTarget, int wCharNum);
#endif				/* _P_MECH_CONTACTS_H */
