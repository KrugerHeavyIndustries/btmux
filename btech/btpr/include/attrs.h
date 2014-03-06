
/* attrs.h - Attribute definitions */

/* $Id: attrs.h,v 1.3 2005/08/08 09:43:05 murrayma Exp $ */

#include "config.h"

#ifndef _ATTRS_H
#define _ATTRS_H

#include "copyright.h"

#define	A_FAIL		3	/* Invoker fail message */
#define	A_AFAIL		13	/* Failure action list */
#define	A_LOCK		42	/* Object lock */
#define A_LEAVE         50      /* Invoker leave message */
#define A_OLEAVE        51      /* Others leave message in src */
#define A_ALEAVE        52      /* Leave action list */
#define	A_LENTER	59	/* ENTER lock */
#define A_MECHPREFID	146	/* Preferred Mech ID on map */
#define A_MAPCOLOR	147	/* ANSIMAP color scheme */

/* Mecha stuff */

#define A_MECHSKILLS    214	/* Pilot's skills in using a mech */
#define A_XTYPE         215	/* Hardcode type */
#define A_TACSIZE       216	/* Tactical Size (H & W) */
#define A_LRSHEIGHT     217	/* LRS height */
#define A_CONTACTOPT    218	/* Contact options */
#define A_MECHNAME      219	/* Mech name */
#define A_MECHTYPE      220	/* Mech type */
#define A_MECHDESC      221	/* Mech extra desc (for view) */
#define A_MECHSTATUS    222	/* Mech status string. Not to be tampered. */
#define A_MWTEMPLATE    229	/* MW template to use (if any) */
#define A_FACTION       230	/* Faction */

/* BT-stats: */
#define A_HEALTH        233	/* Bruise,Lethal */
#define A_ATTRS         234	/* Attributes */

#define A_BUILDLINKS 	235	/* Links */
#define A_BUILDENTRANCE	236	/* Entrance(s) */
#define A_BUILDCOORD	237	/* X/Y coord */

/* BT-stats: */
#define A_ADVS          238	/* Advantages */
#define A_PILOTNUM      239	/* Mech's pilot # */
#define A_MAPVIS        240	/* Visibility */
#define A_TECHTIME      242	/* Time (as a time_t number) until completion */
#define A_ECONPARTS     243	/* Econ parts */

/* BT-stats: */
#define A_SKILLS        244	/* Skills */
#define A_PCEQUIP       245	/* PCombat equipment */
#define A_AMECHDEST     247
#define A_AMINETRIGGER  248
#define A_AAEROLAND	249
#define A_AOODLAND	250	/* For when an OOD finishes */
#endif
