
/*
   p.mech.sensor.functions.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:56 CET 1999 from mech.sensor.functions.c */

#ifndef _P_MECH_SENSOR_FUNCTIONS_H
#define _P_MECH_SENSOR_FUNCTIONS_H

/* mech.sensor.functions.c */
int vislight_see(MECH * t, int num, float r, int c, int l);
int liteamp_see(MECH * t, int num, float r, int c, int l);
int infrared_see(MECH * t, int num, float r, int c, int l);
int electrom_see(MECH * t, int num, float r, int c, int l);
int seismic_see(MECH * t, int num, float r, int c, int l);
int radar_see(MECH * t, int num, float r, int c, int l);
int bap_see(MECH * t, int num, float r, int c, int l);
int blood_see(MECH * t, int num, float r, int c, int l);
int vislight_csee(MECH * m, MECH * t, float r, int f);
int liteamp_csee(MECH * m, MECH * t, float r, int f);
int infrared_csee(MECH * m, MECH * t, float r, int f);
int electrom_csee(MECH * m, MECH * t, float r, int f);
int seismic_csee(MECH * m, MECH * t, float r, int f);
int radar_csee(MECH * m, MECH * t, float r, int f);
int bap_csee(MECH * m, MECH * t, float r, int f);
int blood_csee(MECH * m, MECH * t, float r, int f);
int vislight_tohit(MECH * m, MECH * t, int f, int l);
int liteamp_tohit(MECH * m, MECH * t, int f, int l);
int infrared_tohit(MECH * m, MECH * t, int f, int l);
int electrom_tohit(MECH * m, MECH * t, int f, int l);
int seismic_tohit(MECH * m, MECH * t, int f, int l);
int bap_tohit(MECH * m, MECH * t, int f, int l);
int blood_tohit(MECH * m, MECH * t, int f, int l);
int radar_tohit(MECH * m, MECH * t, int f, int l);

#endif				/* _P_MECH_SENSOR_FUNCTIONS_H */
