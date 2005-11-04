
/*
   p.mech.advanced.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Wed Feb 17 23:36:31 CET 1999 from mech.advanced.c */

#ifndef _P_MECH_ADVANCED_H
#define _P_MECH_ADVANCED_H

/* mech.advanced.c */
void mech_ecm(dbref player, MECH * mech, char *buffer);
void mech_eccm(dbref player, MECH * mech, char *buffer);
void mech_perecm(dbref player, MECH * mech, char *buffer);
void mech_pereccm(dbref player, MECH * mech, char *buffer);
void mech_angelecm(dbref player, MECH * mech, char *buffer);
void mech_angeleccm(dbref player, MECH * mech, char *buffer);
void mech_stinger(dbref player, MECH * mech, char *buffer);
void mech_slite(dbref player, MECH * mech, char *buffer);
void mech_ams(dbref player, void *data, char *buffer);
void mech_fliparms(dbref player, void *data, char *buffer);
void mech_flamerheat(dbref player, void *data, char *buffer);
void mech_ultra(dbref player, void *data, char *buffer);
void mech_rac(dbref player, void *data, char *buffer);
void mech_rapidfire(dbref player, void *data, char *buffer);
void mech_unjamammo(dbref player, void *data, char *buffer);
void mech_gattling(dbref player, void *data, char *buffer);
void mech_inarc_ammo_toggle(dbref player, void *data, char *buffer);
void mech_explosive(dbref player, void *data, char *buffer);
void mech_lbx(dbref player, void *data, char *buffer);
void mech_armorpiercing(dbref player, void *data, char *buffer);
void mech_flechette(dbref player, void *data, char *buffer);
void mech_incendiary(dbref player, void *data, char *buffer);
void mech_precision(dbref player, void *data, char *buffer);
void mech_artemis(dbref player, void *data, char *buffer);
void mech_narc(dbref player, void *data, char *buffer);
void mech_swarm(dbref player, void *data, char *buffer);
void mech_swarm1(dbref player, void *data, char *buffer);
void mech_inferno(dbref player, void *data, char *buffer);
void mech_hotload(dbref player, void *data, char *buffer);
void mech_cluster(dbref player, void *data, char *buffer);
void mech_smoke(dbref player, void *data, char *buffer);
void mech_mine(dbref player, void *data, char *buffer);
void mech_masc(dbref player, void *data, char *buffer);
void mech_scharge(dbref player, void *data, char *buffer);
void mech_explode(dbref player, void *data, char *buffer);
void mech_dig(dbref player, void *data, char *buffer);
void mech_fixturret(dbref player, void *data, char *buffer);
void mech_disableweap(dbref player, void *data, char *buffer);
int FindMainWeapon(MECH * mech, int (*callback) (MECH *, int, int, int,
	int));
void mech_stealtharmor(dbref player, MECH * mech, char *buffer);
void mech_nullsig(dbref player, MECH * mech, char *buffer);
void show_narc_pods(dbref player, MECH * mech, char *buffer);
void remove_inarc_pods_mech(dbref player, MECH * mech, char *buffer);
void remove_inarc_pods_tank(dbref player, MECH * mech, char *buffer);
void mech_auto_turret(dbref player, MECH * mech, char *buffer);
void mech_usebin(dbref player, MECH * mech, char *buffer);
void mech_safety(dbref player, void *data, char *buffer);
void mech_mechprefs(dbref player, void *data, char *buffer);

#endif				/* _P_MECH_ADVANCED_H */
