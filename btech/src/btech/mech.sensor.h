
/*
 * $Id: mech.sensor.h,v 1.1.1.1 2005/01/11 21:18:23 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Mon Sep  2 14:48:12 1996 fingon
 * Last modified: Tue Sep  2 15:32:05 1997 fingon
 *
 */

#ifndef MECH_SENSOR_H
#define MECH_SENSOR_H

/* 
   For all scanners chance of seeing a foe is modified by:
   - Side arcs are 70% chance
   - Rear arc is 40% chance
 */

typedef struct {
    char *sensorname;
    char *matchletter;

    /* Is the sensor 360 degree with just one of them? */
    int fullvision;

    /* Longest vis this sensor brand sees */
    int maxvis;

    /* Variable factor in maxvis ; it changes by +- 1 every 30 seconds */
    int maxvvar;

    /* Function for retrieving generic chance of spotting foe with
       this scanner at the range */
    /* first int = sensor type #, second = maxrange by conditions,
       third = lightning level */
    int (*seechance_func) (MECH *, MAP *, int, float, int, int);

    /* Do we really see 'em? Mainly checks for various things that
       vary between diff. sensors (and also seechancefunc > 0) */
    int (*cansee_func) (MECH *, MECH *, MAP *, float, int);

    /* Chance of actually hitting someone */
    int (*tohitbonus_func) (MECH * mech, MECH * target, MAP *, int, int);

    /* If <0, not used */
    int min_light;
    int max_light;

    int required_special;
    int specials_set;		/* 1 if the original specials struct, 2 if the extended */

    int attributeCheck;		/* Some special checks here that are done in mech.sensor.c */

    char *range_desc;
    char *block_desc;
    char *special_desc;
} SensorStruct;

#define ESEEFUNC(a) extern int a (MECH *, MAP *, int, float, int, int);
#define SEEFUNC(a,b) \
int a (MECH *t, MAP *map, int num, float r, int c, int l) { return (int) (b); }
ESEEFUNC(vislight_see);
ESEEFUNC(liteamp_see);
ESEEFUNC(infrared_see);
ESEEFUNC(electrom_see);
ESEEFUNC(seismic_see);
ESEEFUNC(radar_see);
ESEEFUNC(bap_see);
ESEEFUNC(blood_see);

#define ECSEEFUNC(a) extern int a (MECH *, MECH *, MAP *, float, int);
#define CSEEFUNC(a,b) \
int a (MECH *m, MECH *t, MAP *map, float r, int f) { return (int) (b); }
ECSEEFUNC(vislight_csee);
ECSEEFUNC(liteamp_csee);
ECSEEFUNC(infrared_csee);
ECSEEFUNC(electrom_csee);
ECSEEFUNC(seismic_csee);
ECSEEFUNC(radar_csee);
ECSEEFUNC(bap_csee);
ECSEEFUNC(blood_csee);

#define ETOHITFUNC(a) extern int a (MECH *, MECH *, MAP *, int, int);
#define TOHITFUNC(a,b) \
int a (MECH *m, MECH *t, MAP *map, int f, int l) { return (int) (b); }

ETOHITFUNC(vislight_tohit);
ETOHITFUNC(liteamp_tohit);
ETOHITFUNC(infrared_tohit);
ETOHITFUNC(electrom_tohit);
ETOHITFUNC(seismic_tohit);
ETOHITFUNC(radar_tohit);
ETOHITFUNC(bap_tohit);
ETOHITFUNC(blood_tohit);

#define SENSOR_VIS	0
#define SENSOR_LA	1
#define SENSOR_IR	2
#define SENSOR_EM	3
#define SENSOR_SE	4
#define SENSOR_RA	5
#define SENSOR_BAP	6
#define SENSOR_BHAP	7

#define SENSOR_ATTR_NONE		0
#define SENSOR_ATTR_SEISMIC 1

#ifdef _MECH_SENSOR_C
SensorStruct sensors[] = {
    {"Vislight", "V", 0, 60, 0, vislight_see, vislight_csee,
	    vislight_tohit,
	    -1, -1, 0, 1, SENSOR_ATTR_NONE,
	    "Visual",
	    "Fire/Smoke/Obstacles, 3 pt woods, 5 underwater hexes",
	"Bad in night-fighting (BTH)"},
    {"Light-amplification", "L", 0, 60, 0, liteamp_see, liteamp_csee,
	    liteamp_tohit,
	    0, 1, 0 - NS_TECH, 1, SENSOR_ATTR_NONE,
	    "Visual (Dawn/Dusk), 2x Visual (Night)",
	    "Fire/Smoke/Obstacles, 2 pt woods, any water",
	"Somewhat harder enemy detection (than vislight), bad in forests (BTH/range)"},
    {"Infrared", "I", 1, 15, 0, infrared_see, infrared_csee,
	    infrared_tohit,
	    -1, -1, 0 - NS_TECH, 1, SENSOR_ATTR_NONE,
	    "15",
	    "Fire/Obstacles, 6 pt woods",
	"Easy to hit 'hot' targets, not very efficient in forests (BTH)"},
    {"Electromagnetic", "E", 1, 24, 8, electrom_see, electrom_csee,
	    electrom_tohit,
	    -1, -1, 0 - NS_TECH, 1, SENSOR_ATTR_NONE,
	    "16-24",
	    "Mountains/Obstacles, 8 pt woods",
	"Easy to hit heavies, good in forests (BTH), overall unreliable (chances of detection/BTH)"},
    {"Seismic", "S", 1, 8, 4, seismic_see, seismic_csee, seismic_tohit, -1,
	    -1,
	    0 - NS_TECH, 1, SENSOR_ATTR_SEISMIC,
	    "4-8",
	    "Nothing",
	"Easier heavy and/or moving object detection (although overall hard to detect with), somewhat unreliable(BTH)"},
    {"Radar", "R", 1, 180, 0, radar_see, radar_csee, radar_tohit, -1, -1,
	    AA_TECH, 1, SENSOR_ATTR_NONE,
	    "<=180",
	    "Obstacles, enemy elevation (Enemy Z >= 10, range: 180, Enemy Z < 10, range: varies)",
	"Premier anti-aircraft sensor, partially negates partial cover(BTH), doesn't see targets that are too low for detection"},

    {"Beagle ActiveProbe", "B", 1, 6, 0, bap_see, bap_csee, bap_tohit, -1,
	    -1, BEAGLE_PROBE_TECH, 1, SENSOR_ATTR_NONE, "<=6",
	    "Nothing (except range)",
	"Ultimate sensor in close-range detection (slightly varying BTH, but ignores partial/woods/water)"},

/* Don't need special see/hit. just ranges for lbap*/
    {"Light Beagle ActiveProbe", "A", 1, 3, 0, bap_see, bap_csee, bap_tohit, -1,
    	    -1, LIGHT_BAP_TECH, 1, SENSOR_ATTR_NONE, "<=3",
	    "Nothing (except range)",
	"Short range, but ultimate sensor in close-range detection (slightly varying BTH, but ignores partial/woods/water)"},

    {"Bloodhound ActiveProbe", "H", 1, 8, 0, blood_see, blood_csee,
	    blood_tohit, -1,
	    -1, BLOODHOUND_PROBE_TECH, 2, SENSOR_ATTR_NONE, "<=8",
	    "Nothing (except range)",
	"Superior version of the Beagle Active Probe (slightly varying BTH, but ignores partial/woods/water)"}
};
   
#define NUM_SENSORS (sizeof (sensors) / sizeof(sensors[0]))
#else
extern SensorStruct sensors[];
#endif
#endif				/* MECH_SENSOR_H */
