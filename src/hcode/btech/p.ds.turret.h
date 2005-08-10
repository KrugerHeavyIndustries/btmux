
/*
   p.ds.turret.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:39 CET 1999 from ds.turret.c */

#ifndef _P_DS_TURRET_H
#define _P_DS_TURRET_H

/* ds.turret.c */
void turret_addtic(dbref player, void *data, char *buffer);
void turret_deltic(dbref player, void *data, char *buffer);
void turret_listtic(dbref player, void *data, char *buffer);
void turret_cleartic(dbref player, void *data, char *buffer);
void turret_firetic(dbref player, void *data, char *buffer);
void turret_bearing(dbref player, void *data, char *buffer);
void turret_eta(dbref player, void *data, char *buffer);
void turret_findcenter(dbref player, void *data, char *buffer);
void turret_fireweapon(dbref player, void *data, char *buffer);
void turret_settarget(dbref player, void *data, char *buffer);
void turret_lrsmap(dbref player, void *data, char *buffer);
void turret_navigate(dbref player, void *data, char *buffer);
void turret_range(dbref player, void *data, char *buffer);
void turret_sight(dbref player, void *data, char *buffer);
void turret_tacmap(dbref player, void *data, char *buffer);
void turret_contacts(dbref player, void *data, char *buffer);
void turret_critstatus(dbref player, void *data, char *buffer);
void turret_report(dbref player, void *data, char *buffer);
void turret_scan(dbref player, void *data, char *buffer);
void turret_status(dbref player, void *data, char *buffer);
void turret_weaponspecs(dbref player, void *data, char *buffer);
void newturret(dbref key, void **data, int selector);
void turret_initialize(dbref player, void *data, char *buffer);
void turret_deinitialize(dbref player, void *data, char *buffer);

#endif				/* _P_DS_TURRET_H */
