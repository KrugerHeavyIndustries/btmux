
/*
   p.mech.sensor.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Mar 22 10:40:21 CET 1999 from mech.sensor.c */

#ifndef _P_MECH_SENSOR_H
#define _P_MECH_SENSOR_H

/* mech.sensor.c */
int Sensor_ToHitBonus(MECH * mech, MECH * target, int flag, int maplight,
    float range, int wAmmoMode);
int Sensor_CanSee(MECH * mech, MECH * target, int *flag, int arc,
    float range, int mapvis, int maplight, int cloudbase);
int Sensor_ArcBaseChance(int type, int arc);
int Sensor_DriverBaseChance(MECH * mech);
int Sensor_Sees(MECH * mech, MECH * target, int f, int arc, float range,
    int snum, int chance_divisor, int mapvis, int maplight);
int Sensor_SeesNow(MECH * mech, MECH * target, int f, int arc, float range,
    int mapvis, int maplight);
char *my_dump_flag(int i);
void Sensor_DoWeSeeNow(MECH * mech, unsigned short *fl, float range, int x,
    int y, MECH * target, int mapvis, int maplight, int cloudbase,
    int seeanew, int wlf);
void update_LOSinfo(dbref obj, MAP * map);
void add_sensor_info(char *buf, MECH * mech, int sn, int verbose);
char *mechSensorInfo(int mode, MECH * mech, char *arg);
int CanChangeTo(MECH * mech, int s);
void sensor_light_availability_check(MECH * mech);
void mech_sensor(dbref player, void *data, char *buffer);
void possibly_see_mech(MECH * mech);
void ScrambleInfraAndLiteAmp(MECH * mech, int time, int chance,
    char *inframsg, char *liteampmsg);

#endif				/* _P_MECH_SENSOR_H */
