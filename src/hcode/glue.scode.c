
/*
 * $Id: glue.scode.c,v 1.5 2005/08/08 09:43:09 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters 
 *  Copyright (c) 2000-2002 Cord Awtry 
 *  Copyright (c) 1999-2005 Kevin Stevens
 *       All rights reserved
 *
 * Created: Wed Oct  9 19:13:52 1996 fingon
 * Last modified: Tue Sep  8 10:00:29 1998 fingon
 *
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include "mech.h"
#include "mech.events.h"
#include "glue.h"
#include "coolmenu.h"
#include "mycool.h"
#include "turret.h"
#include "p.template.h"
#include "p.mech.tech.h"
#include "p.mech.utils.h"
#include "p.mech.partnames.h"
#include "p.econ.h"
#include "p.map.obj.h"
#include "p.mech.tech.damages.h"
#include "p.mech.status.h"
#include "p.mech.sensor.h"
#include "p.btechstats.h"
#include "p.mech.combat.h"
#include "p.mech.damage.h"
#include "p.mechrep.h"
#include "p.mech.move.h"
#include "p.mech.los.h"
#include "p.event.h"
#include "p.mech.restrict.h"
#include "mech.partnames.h"
#include "p.functions.h"
#include "p.mech.tech.commands.h"
#include "p.mech.consistency.h"

extern SpecialObjectStruct SpecialObjects[];
dbref match_thing(dbref player, char *name);
char *mechref_path(char *id);
char *setarmorstatus_func(MECH * mech, char *sectstr, char *typestr,
						  char *valuestr);

typedef struct {
	int gtype;
	char *name;
	void *rel_addr;
	int type;
	int size;
} GMV;

static MECH tmpm;
static MAP tmpmap;
static TURRET_T tmpturret;

enum { TYPE_STRING, TYPE_CHAR, TYPE_SHORT, TYPE_INT, TYPE_FLOAT,
	TYPE_DBREF, TYPE_STRFUNC, TYPE_STRFUNC_S, TYPE_BV,
	TYPE_STRFUNC_BD, TYPE_CBV, TYPE_CHAR_RO, TYPE_SHORT_RO, TYPE_INT_RO,
	TYPE_FLOAT_RO, TYPE_DBREF_RO, TYPE_LAST_TYPE
};

/* INDENT OFF */
static int scode_in_out[TYPE_LAST_TYPE] =
/* st ch sh in fl db sf sfs bv sfb cbv ro-ch ro-sh ro-in ro-fl ro-db*/
{ 3, 3, 3, 3, 3, 3, 1, 2, 3, 3, 3, 1, 1, 1, 1, 1 };
/* INDENT ON */

#define Uglie(dat) ((void *) &dat((MECH *)0))
#define UglieV(dat,val) ((void *) &dat((MECH *)0,val))

#define MeEntry(Name,Func,Type) {GTYPE_MECH,Name,Uglie(Func),Type,0}

#define MeEntryS(Name,Func,Type,Size)  {GTYPE_MECH,Name,Uglie(Func),Type,Size}

#define MeVEntry(Name,Func,Val,Type) {GTYPE_MECH,Name,UglieV(Func,Val),Type,0}

#define UglieM(dat) ((void *)&((MAP *)0)->dat)
#define MaEntry(Name,Func,Type) {GTYPE_MAP,Name,UglieM(Func),Type,0}
#define MaEntryS(Name,Func,Type,Size)  {GTYPE_MAP,Name,UglieM(Func),Type,Size}

#define UglieT(dat) (void *)&((TURRET_T *)0)->dat

#define TuEntry(Name,Func,Type)  {GTYPE_TURRET,Name,UglieT(Func),Type,0}
#define TuEntryS(Name,Func,Type,Size)  {GTYPE_TURRET,Name,UglieT(Func),Type,Size}

char *mechIDfunc(int mode, MECH * mech)
{
	static char buf[3];

	buf[0] = MechID(mech)[0];
	buf[1] = MechID(mech)[1];
	buf[2] = 0;
	return buf;
}

static char *mech_getset_ref(int mode, MECH * mech, char *data)
{
	if(mode) {
		strncpy(MechType_Ref(mech), data, 14);
		MechType_Ref(mech)[14] = '\0';
		return NULL;
	} else
		return MechType_Ref(mech);
}

extern char *mech_types[];
extern char *move_types[];

char *mechTypefunc(int mode, MECH * mech, char *arg)
{
	int i;

	if(!mode)
		return mech_types[(short) MechType(mech)];
	/* Should _alter_ mechtype.. weeeel. */
	if((i = compare_array(mech_types, arg)) >= 0)
		MechType(mech) = i;
	return NULL;
}

char *mechMovefunc(int mode, MECH * mech, char *arg)
{
	int i;

	if(!mode)
		return move_types[(short) MechMove(mech)];
	if((i = compare_array(move_types, arg)) >= 0)
		MechMove(mech) = i;
	return NULL;
}

char *mechTechTimefunc(int mode, MECH * mech)
{
	static char buf[MBUF_SIZE];
	int n = figure_latest_tech_event(mech);

	sprintf(buf, "%d", n);
	return buf;
}

void apply_mechDamage(MECH * omech, char *buf)
{
	MECH mek;
	MECH *mech = &mek;
	int i, j, i1, i2, i3;
	char *s;
	int do_mag = 0;

	memcpy(mech, omech, sizeof(MECH));
	for(i = 0; i < NUM_SECTIONS; i++) {
		SetSectInt(mech, i, GetSectOInt(mech, i));
		SetSectArmor(mech, i, GetSectOArmor(mech, i));
		SetSectRArmor(mech, i, GetSectORArmor(mech, i));
		for(j = 0; j < NUM_CRITICALS; j++)
			if(GetPartType(mech, i, j) && !IsCrap(GetPartType(mech, i, j))) {
				if(PartIsDestroyed(mech, i, j))
					UnDestroyPart(mech, i, j);
				if(IsAmmo(GetPartType(mech, i, j)))
					SetPartData(mech, i, j, FullAmmo(mech, i, j));
				else
					SetPartTempNuke(mech, i, j, 0);
			}
	}
	s = buf;
	while (*s) {
		while (*s && (*s == ' ' || *s == ','))
			s++;
		if(!(*s))
			break;
		/* Parse the keyword ; it's one of the many known types */
		if(sscanf(s, "A:%d/%d", &i1, &i2) == 2) {
			/* Ordinary armor damage */
			if(i1 >= 0 && i1 < NUM_SECTIONS)
				SetSectArmor(mech, i1, GetSectOArmor(mech, i1) - i2);
		} else if(sscanf(s, "A(R):%d/%d", &i1, &i2) == 2) {
			/* Ordinary rear armor damage */
			if(i1 >= 0 && i1 < NUM_SECTIONS)
				SetSectRArmor(mech, i1, GetSectORArmor(mech, i1) - i2);
		} else if(sscanf(s, "I:%d/%d", &i1, &i2) == 2) {
			/* Ordinary int damage */
			if(i1 >= 0 && i1 < NUM_SECTIONS)
				SetSectInt(mech, i1, GetSectOInt(mech, i1) - i2);
		} else if(sscanf(s, "C:%d/%d", &i1, &i2) == 2) {
			/* Dest'ed crit */
			if(i1 >= 0 && i1 < NUM_SECTIONS)
				DestroyPart(mech, i1, i2);
		} else if(sscanf(s, "G:%d/%d(%d)", &i1, &i2, &i3) == 3) {
			/* Glitch */
			if(i1 >= 0 && i1 < NUM_SECTIONS)
				if(i2 >= 0 && i2 < NUM_CRITICALS)
					SetPartTempNuke(mech, i1, i2, i3);
		} else if(sscanf(s, "R:%d/%d(%d)", &i1, &i2, &i3) == 3) {
			/* Reload */
			if(i1 >= 0 && i1 < NUM_SECTIONS)
				if(i2 >= 0 && i2 < NUM_CRITICALS)
					SetPartData(mech, i1, i2, FullAmmo(mech, i1, i2) - i3);
		}
		while (*s && (*s != ' ' && *s != ','))
			s++;
	}
	for(i = 0; i < NUM_SECTIONS; i++) {
		if(GetSectInt(mech, i) != GetSectInt(omech, i))
			SetSectInt(omech, i, GetSectInt(mech, i));
		if(GetSectArmor(mech, i) != GetSectArmor(omech, i))
			SetSectArmor(omech, i, GetSectArmor(mech, i));
		if(GetSectRArmor(mech, i) != GetSectRArmor(omech, i))
			SetSectRArmor(omech, i, GetSectRArmor(mech, i));
		for(j = 0; j < NUM_CRITICALS; j++)
			if(GetPartType(mech, i, j) && !IsCrap(GetPartType(mech, i, j))) {
				if(PartIsDestroyed(mech, i, j) &&
				   !PartIsDestroyed(omech, i, j)) {
					/* Blast a part */
					DestroyPart(omech, i, j);
					do_mag = 1;
				} else if(!PartIsDestroyed(mech, i, j) &&
						  PartIsDestroyed(omech, i, j)) {
					mech_RepairPart(omech, i, j);
					SetPartTempNuke(omech, i, j, 0);
					do_mag = 1;
				}
				if(IsAmmo(GetPartType(mech, i, j))) {
					if(GetPartData(mech, i, j) != GetPartData(omech, i, j))
						SetPartData(omech, i, j, GetPartData(mech, i, j));
				} else {
					if(PartTempNuke(mech, i, j) != PartTempNuke(omech, i, j))
						SetPartTempNuke(omech, i, j,
										PartTempNuke(mech, i, j));

				}
			}
	}
	if(do_mag && MechType(omech) == CLASS_MECH)
		do_magic(omech);
}

#define ADD(foo...) { if (count++) strcat(buf,","); sprintf(buf+strlen(buf), foo); }

char *mechDamagefunc(int mode, MECH * mech, char *arg)
{
	/* Lists damage in form:
	   A:LOC/num[,LOC/num[,LOC(R)/num]],I:LOC/num
	   C:LOC/num,R:LOC/num(num),G:LOC/num(num) */
	int i, j;
	static char buf[LBUF_SIZE];
	int count = 0;

	if(mode) {
		apply_mechDamage(mech, arg);
		return "?";
	};
	buf[0] = 0;
	for(i = 0; i < NUM_SECTIONS; i++)
		if(GetSectOInt(mech, i)) {
			if(GetSectArmor(mech, i) != GetSectOArmor(mech, i))
				ADD("A:%d/%d", i, GetSectOArmor(mech,
												i) - GetSectArmor(mech, i));
			if(GetSectRArmor(mech, i) != GetSectORArmor(mech, i))
				ADD("A(R):%d/%d", i, GetSectORArmor(mech,
													i) - GetSectRArmor(mech,
																	   i));
		}
	for(i = 0; i < NUM_SECTIONS; i++)
		if(GetSectOInt(mech, i))
			if(GetSectInt(mech, i) != GetSectOInt(mech, i))
				ADD("I:%d/%d", i, GetSectOInt(mech, i) - GetSectInt(mech, i));
	for(i = 0; i < NUM_SECTIONS; i++)
		for(j = 0; j < CritsInLoc(mech, i); j++) {
			if(GetPartType(mech, i, j) && !IsCrap(GetPartType(mech, i, j))) {
				if(PartIsDestroyed(mech, i, j)) {
					ADD("C:%d/%d", i, j);
				} else {
					if(IsAmmo(GetPartType(mech, i, j))) {
						if(GetPartData(mech, i, j) != FullAmmo(mech, i, j))
							ADD("R:%d/%d(%d)", i, j, FullAmmo(mech,
															  i,
															  j) -
								GetPartData(mech, i, j));
					} else if(PartTempNuke(mech, i, j))
						ADD("G:%d/%d(%d)", i, j, PartTempNuke(mech, i, j));
				}
			}
		}
	return buf;
}

char *mechCentBearingfunc(int mode, MECH * mech, char *arg)
{
	int x = MechX(mech);
	int y = MechY(mech);
	float fx, fy;
	static char buf[SBUF_SIZE];

	MapCoordToRealCoord(x, y, &fx, &fy);
	sprintf(buf, "%d", FindBearing(MechFX(mech), MechFY(mech), fx, fy));
	return buf;
}

char *mechCentDistfunc(int mode, MECH * mech, char *arg)
{
	int x = MechX(mech);
	int y = MechY(mech);
	float fx, fy;
	static char buf[SBUF_SIZE];

	MapCoordToRealCoord(x, y, &fx, &fy);
	sprintf(buf, "%.2f", FindHexRange(fx, fy, MechFX(mech), MechFY(mech)));
	return buf;
}

/* Mode:
   0 = char -> bit field
   1 = bit field -> char
   */

static int bv_val(int in, int mode)
{
	int p = 0;

	if(mode == 0) {
		if(in >= 'a' && in <= 'z')
			return 1 << (in - 'a');
		return 1 << ('z' - 'a' + 1 + (in - 'A'));
	}
	while (in > 0) {
		p++;
		in >>= 1;
	}
	/* Hmm. */
	p--;
	if(p > ('z' - 'a'))
		return 'A' + (p - ('z' - 'a' + 1));
	return 'a' + p;
}

static int text2bv(char *text)
{
	char *c;
	int j = 0;
	int mode_not = 0;

	if(!Readnum(j, text))
		return j;				/* Allow 'old style' as well */

	/* Valid bitvector letters are: a-z (=27), A-Z (=27 more) */
	for(c = text; *c; c++) {
		if(*c == '!') {
			mode_not = 1;
			c++;
		};
		if((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z')) {
			int k = bv_val(*c, 0);

			if(k) {
				if(mode_not)
					j &= ~k;
				else
					j |= k;
			}
		}
		mode_not = 0;
	}
	return j;
}

static char *bv2text(int i)
{
	static char buf[SBUF_SIZE];
	int p = 1;
	char *c = buf;

	while (i > 0) {
		if(i & 1)
			*(c++) = bv_val(p, 1);
		i >>= 1;
		p <<= 1;
	}
	if(c == buf)
		*(c++) = '-';
	*c = 0;
	return buf;
}

#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) (void *)__compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) (void *)((size_t) &((TYPE *)0)->MEMBER)
#endif

static GMV xcode_data[] = {
	{GTYPE_MECH, "mapindex", offsetof(MECH, mapindex), TYPE_DBREF_RO},
	{GTYPE_MECH, "id", mechIDfunc, TYPE_STRFUNC},
	MeEntryS("mechname", MechType_Name, TYPE_STRING, 31),
	MeEntry("maxspeed", MechMaxSpeed, TYPE_FLOAT),
	MeEntry("pilotnum", MechPilot, TYPE_DBREF),
	MeEntry("pilotdam", MechPilotStatus, TYPE_CHAR),
	MeEntry("si", AeroSI, TYPE_CHAR),
	MeEntry("si_orig", AeroSIOrig, TYPE_CHAR),
	MeEntry("speed", MechSpeed, TYPE_FLOAT),
	MeEntry("basewalkspeed", MechBaseWalk, TYPE_INT),
	MeEntry("baserunspeed", MechBaseRun, TYPE_INT),
	MeEntry("heading", MechRFacing, TYPE_SHORT),
	MeEntry("stall", MechStall, TYPE_INT),
	MeEntry("status", MechStatus, TYPE_BV),
	MeEntry("status2", MechStatus2, TYPE_BV),
	MeEntry("critstatus", MechCritStatus, TYPE_BV),
	MeEntry("critstatus2", MechCritStatus2, TYPE_BV),
	MeEntry("tankcritstatus", MechTankCritStatus, TYPE_BV),
	MeEntry("target", MechTarget, TYPE_DBREF),
	MeEntry("team", MechTeam, TYPE_INT),
	MeEntry("tons", MechTons, TYPE_INT),
	MeEntry("towing", MechCarrying, TYPE_INT_RO),
	MeEntry("heat", MechPlusHeat, TYPE_FLOAT),
	MeEntry("disabled_hs", MechDisabledHS, TYPE_INT_RO),
	MeEntry("overheat", MechHeat, TYPE_FLOAT),
	MeEntry("dissheat", MechMinusHeat, TYPE_FLOAT),
	MeEntry("heatsinks", MechRealNumsinks, TYPE_CHAR_RO),
	MeEntry("last_startup", MechLastStartup, TYPE_INT),
	MeEntry("C3iNetworkSize", MechC3iNetworkSize, TYPE_INT_RO),
	MeEntry("MaxSuits", MechMaxSuits, TYPE_INT),
	MeEntry("realweight", MechRTonsV, TYPE_INT),
	MeEntry("StaggerDamage", StaggerDamage, TYPE_INT_RO),
	MeEntry("MechPrefs", MechPrefs, TYPE_BV),

	{GTYPE_MECH, "mechtype", mechTypefunc, TYPE_STRFUNC_BD},
	{GTYPE_MECH, "mechmovetype", mechMovefunc, TYPE_STRFUNC_BD},
	{GTYPE_MECH, "mechdamage", mechDamagefunc, TYPE_STRFUNC_BD},
	{GTYPE_MECH, "techtime", mechTechTimefunc, TYPE_STRFUNC},
	{GTYPE_MECH, "centdist", mechCentDistfunc, TYPE_STRFUNC},
	{GTYPE_MECH, "centbearing", mechCentBearingfunc, TYPE_STRFUNC},
	{GTYPE_MECH, "sensors", mechSensorInfo, TYPE_STRFUNC},
	{GTYPE_MECH, "mechref", mech_getset_ref, TYPE_STRFUNC_BD},

	MeEntry("fuel", AeroFuel, TYPE_INT),
	MeEntry("fuel_orig", AeroFuelOrig, TYPE_INT),
	MeEntry("cocoon", MechCocoon, TYPE_INT_RO),
	MeEntry("numseen", MechNumSeen, TYPE_SHORT),

	MeEntry("fx", MechFX, TYPE_FLOAT),
	MeEntry("fy", MechFY, TYPE_FLOAT),
	MeEntry("fz", MechFZ, TYPE_FLOAT),
	MeEntry("x", MechX, TYPE_SHORT),
	MeEntry("y", MechY, TYPE_SHORT),
	MeEntry("z", MechZ, TYPE_SHORT),

	MeEntry("targcomp", MechTargComp, TYPE_CHAR),
	MeEntry("lrsrange", MechLRSRange, TYPE_CHAR),
	MeEntry("radiorange", MechRadioRange, TYPE_SHORT),
	MeEntry("scanrange", MechScanRange, TYPE_CHAR),
	MeEntry("tacrange", MechTacRange, TYPE_CHAR),
	MeEntry("radiotype", MechRadioType, TYPE_CHAR),
	MeEntry("bv", MechBV, TYPE_INT),
	MeEntry("cargospace", CargoSpace, TYPE_INT),
	MeEntry("carmaxton", CarMaxTon, TYPE_CHAR_RO),

	MeVEntry("bay0", AeroBay, 0, TYPE_DBREF),
	MeVEntry("bay1", AeroBay, 1, TYPE_DBREF),
	MeVEntry("bay2", AeroBay, 2, TYPE_DBREF),
	MeVEntry("bay3", AeroBay, 3, TYPE_DBREF),

	MeVEntry("turret0", AeroTurret, 0, TYPE_DBREF),
	MeVEntry("turret1", AeroTurret, 1, TYPE_DBREF),
	MeVEntry("turret2", AeroTurret, 2, TYPE_DBREF),

	MeEntry("unusablearcs", AeroUnusableArcs, TYPE_INT_RO),
	MeEntry("maxjumpspeed", MechJumpSpeed, TYPE_FLOAT),
	MeEntry("jumpheading", MechJumpHeading, TYPE_SHORT),
	MeEntry("jumplength", MechJumpLength, TYPE_SHORT),

	MaEntry("buildflag", buildflag, TYPE_CHAR),
	MaEntry("buildonmap", onmap, TYPE_DBREF_RO),
	MaEntry("cf", cf, TYPE_SHORT),
	MaEntry("cfmax", cfmax, TYPE_SHORT),
	MaEntry("gravity", grav, TYPE_CHAR),
	MaEntry("firstfree", first_free, TYPE_CHAR_RO),
	MaEntry("mapheight", map_height, TYPE_SHORT_RO),
	MaEntry("maplight", maplight, TYPE_CHAR),
	MaEntryS("mapname", mapname, TYPE_STRING, 30),
	MaEntry("mapvis", mapvis, TYPE_CHAR),
	MaEntry("mapwidth", map_width, TYPE_SHORT_RO),
	MaEntry("maxvis", maxvis, TYPE_SHORT),
	MaEntry("temperature", temp, TYPE_CHAR),
	MaEntry("winddir", winddir, TYPE_SHORT),
	MaEntry("windspeed", windspeed, TYPE_SHORT),
	MaEntry("cloudbase", cloudbase, TYPE_SHORT),
	MaEntry("flags", flags, TYPE_CBV),
	MaEntry("sensorflags", sensorflags, TYPE_BV),

	TuEntry("arcs", arcs, TYPE_INT),
	TuEntry("parent", parent, TYPE_DBREF),
	TuEntry("gunner", gunner, TYPE_DBREF),
	TuEntry("target", target, TYPE_DBREF),
	TuEntry("targx", target, TYPE_SHORT),
	TuEntry("targy", target, TYPE_SHORT),
	TuEntry("targz", target, TYPE_SHORT),
	TuEntry("lockmode", lockmode, TYPE_INT),

	MeEntry("radio", MechRadio, TYPE_CHAR),
	MeEntry("computer", MechComputer, TYPE_CHAR),
	MeEntry("perception", MechPer, TYPE_INT),

	{-1, NULL, 0, TYPE_STRING}
};

void fun_btsetxcodevalue(char *buff, char **bufc, dbref player, dbref cause,
						 char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = id of the mech
	   fargs[1] = name of the value
	   fargs[2] = what the value's to be set as
	 */
	dbref it;
	int i, spec;
	void *foo;
	void *bar;
	void *(*tempfun) ();

	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1");
	spec = WhichSpecial(it);
	FUNCHECK(!(foo = FindObjectsData(it)), "#-1");
	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	for(i = 0; xcode_data[i].name; i++)
		if(!strcasecmp(fargs[1], xcode_data[i].name) &&
		   xcode_data[i].gtype == spec &&
		   (scode_in_out[xcode_data[i].type] & 2)) {
			bar = (void *) ((int) foo + xcode_data[i].rel_addr);
			switch (xcode_data[i].type) {
			case TYPE_STRFUNC_BD:
			case TYPE_STRFUNC_S:
				tempfun = (void *) xcode_data[i].rel_addr;
				tempfun(1, (MECH *) foo, (char *) fargs[2]);
				break;
			case TYPE_STRING:
				strncpy((char *) bar, fargs[2], xcode_data[i].size - 1);
				((char *) bar)[xcode_data[i].size - 1] = '\0';
				break;
			case TYPE_DBREF:
				*((dbref *) bar) = atoi(fargs[2]);
				break;
			case TYPE_CHAR:
				*((char *) bar) = atoi(fargs[2]);
				break;
			case TYPE_SHORT:
				*((short *) bar) = atoi(fargs[2]);
				break;
			case TYPE_INT:
				*((int *) bar) = atoi(fargs[2]);
				break;
			case TYPE_FLOAT:
				*((float *) bar) = atof(fargs[2]);
				break;
			case TYPE_BV:
				*((int *) bar) = text2bv(fargs[2]);
				break;
			case TYPE_CBV:
				*((byte *) bar) = (byte) text2bv(fargs[2]);
				break;
			}
			safe_tprintf_str(buff, bufc, "1");
			return;
		}
	safe_tprintf_str(buff, bufc, "#-1");
	return;
}

static char *RetrieveValue(void *data, int i)
{
	void *bar = (void *) ((int) data + xcode_data[i].rel_addr);
	static char buf[LBUF_SIZE];
	char *(*tempfun) ();

	switch (xcode_data[i].type) {
	case TYPE_STRFUNC_BD:
	case TYPE_STRFUNC:
		tempfun = (void *) xcode_data[i].rel_addr;
		sprintf(buf, "%s", (char *) tempfun(0, (MECH *) data));
		break;
	case TYPE_STRING:
		sprintf(buf, "%s", (char *) bar);
		break;
	case TYPE_DBREF:
	case TYPE_DBREF_RO:
		sprintf(buf, "%d", (dbref) * ((dbref *) bar));
		break;
	case TYPE_CHAR:
	case TYPE_CHAR_RO:
		sprintf(buf, "%d", (char) *((char *) bar));
		break;
	case TYPE_SHORT:
	case TYPE_SHORT_RO:
		sprintf(buf, "%d", (short) *((short *) bar));
		break;
	case TYPE_INT:
	case TYPE_INT_RO:
		sprintf(buf, "%d", (int) *((int *) bar));
		break;
	case TYPE_FLOAT:
	case TYPE_FLOAT_RO:
		sprintf(buf, "%.2f", (float) *((float *) bar));
		break;
	case TYPE_BV:
		strcpy(buf, bv2text((int) *((int *) bar)));
		break;
	case TYPE_CBV:
		strcpy(buf, bv2text((int) *((char *) bar)));
		break;
	}
	return buf;
}

void fun_btgetxcodevalue(char *buff, char **bufc, dbref player, dbref cause,
						 char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = id of the mech
	   fargs[1] = name of the value
	 */
	dbref it;
	int i;
	void *foo;
	int spec;

	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1");
	spec = WhichSpecial(it);
	FUNCHECK(!(foo = FindObjectsData(it)), "#-1");
	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	for(i = 0; xcode_data[i].name; i++)
		if(!strcasecmp(fargs[1], xcode_data[i].name) &&
		   xcode_data[i].gtype == spec &&
		   (scode_in_out[xcode_data[i].type] & 1)) {
			safe_tprintf_str(buff, bufc, "%s", RetrieveValue(foo, i));
			return;
		}
	safe_tprintf_str(buff, bufc, "#-1");
	return;
}

void fun_btgetxcodevalue_ref(char *buff, char **bufc, dbref player,
							 dbref cause, char *fargs[], int nfargs,
							 char *cargs[], int ncargs)
{
	/* fargs[0] = mech ref
	   fargs[1] = name of the value
	 */
	int i;
	MECH *foo;
	int spec;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK((foo = load_refmech(fargs[0])) == NULL, "#-1 NO SUCH MECH");
	spec = GTYPE_MECH;
	for(i = 0; xcode_data[i].name; i++)
		if(!strcasecmp(fargs[1], xcode_data[i].name) &&
		   xcode_data[i].gtype == spec &&
		   (scode_in_out[xcode_data[i].type] & 1)) {
			safe_tprintf_str(buff, bufc, "%s", RetrieveValue(foo, i));
			return;
		}
	safe_tprintf_str(buff, bufc, "#-1");
	return;
}

void set_xcodestuff(dbref player, void *data, char *buffer)
{
	char *args[2];
	int t, i;
	void *bar;
	void *(*tempfun) ();

	memset(args, 0, sizeof(char *) * 2);

	DOCHECK(silly_parseattributes(buffer, args, 2) != 2,
			"Invalid arguments!");
	t = WhichSpecial(Location(player));
	for(i = 0; xcode_data[i].name; i++)
		if(xcode_data[i].gtype == t)
			break;
	DOCHECK(!xcode_data[i].name,
			"Error: No xcode values for this type of object found.");
	for(i = 0; xcode_data[i].name; i++)
		if(!strcasecmp(args[0], xcode_data[i].name) &&
		   xcode_data[i].gtype == t && (scode_in_out[xcode_data[i].type] & 2))
			break;
	DOCHECK(!xcode_data[i].name,
			"Error: No matching xcode value for this type of object found.");
	bar =
		(void *) ((int) FindObjectsData(Location(player)) +
				  xcode_data[i].rel_addr);
	switch (xcode_data[i].type) {
	case TYPE_STRFUNC_BD:
	case TYPE_STRFUNC_S:
		tempfun = (void *) xcode_data[i].rel_addr;
		tempfun(1, getMech(Location(player)), (char *) args[1]);
		break;
	case TYPE_STRING:
		strncpy((char *) bar, args[1], xcode_data[i].size - 1);
		((char *) bar)[xcode_data[i].size - 1] = '\0';
		break;
	case TYPE_DBREF:
		*((dbref *) bar) = atoi(args[1]);
		break;
	case TYPE_CHAR:
		*((char *) bar) = atoi(args[1]);
		break;
	case TYPE_SHORT:
		*((short *) bar) = atoi(args[1]);
		break;
	case TYPE_INT:
		*((int *) bar) = atoi(args[1]);
		break;
	case TYPE_FLOAT:
		*((float *) bar) = atof(args[1]);
		break;
	case TYPE_BV:
		*((int *) bar) = text2bv(args[1]);
		break;
	case TYPE_CBV:
		*((byte *) bar) = (byte) text2bv(args[1]);
	}
}

void list_xcodestuff(dbref player, void *data, char *buffer)
{
	int t, i, flag = CM_TWO, se_len = 37;
	coolmenu *c = NULL;

	t = WhichSpecial(Location(player));
	for(i = 0; xcode_data[i].name; i++)
		if(xcode_data[i].gtype == t && (scode_in_out[xcode_data[i].type] & 1))
			break;
	DOCHECK(!xcode_data[i].name,
			"Error: No xcode values for this type of object found.");
	addline();
	cent(tprintf("Data for %s (%s)", Name(Location(player)),
				 SpecialObjects[t].type));
	addline();
	if(*buffer == '1') {
		flag = CM_ONE;
		se_len = se_len * 2;
	};
	if(*buffer == '4') {
		flag = CM_FOUR;
		se_len = se_len / 2;
	};
	if(*buffer == '1' || *buffer == '4')
		buffer++;
	for(i = 0; xcode_data[i].name; i++) {
		if(xcode_data[i].gtype == t && (scode_in_out[xcode_data[i].type] & 1)) {
			/* 1/3(left) = name, 2/3(right)=value */
			char mask[SBUF_SIZE];
			char lab[SBUF_SIZE];

			if(*buffer)
				if(strncasecmp(xcode_data[i].name, buffer, strlen(buffer)))
					continue;
			strcpy(lab, xcode_data[i].name);
			lab[se_len / 3] = 0;
			sprintf(mask, "%%-%ds%%%ds", se_len / 3, se_len * 2 / 3);
			sim(tprintf(mask, lab, RetrieveValue(data, i)), flag);
		}
	}
	addline();
	ShowCoolMenu(player, c);
	KillCoolMenu(c);
}

void fun_btunderrepair(char *buff, char **bufc, dbref player, dbref cause,
					   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = ref of the mech to be checked */
	int n;
	MECH *mech;
	dbref it;

	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1");
	FUNCHECK(!IsMech(it), "#-2");
	mech = FindObjectsData(it);
	n = figure_latest_tech_event(mech);
	safe_tprintf_str(buff, bufc, "%d", n > 0);
}

void fun_btstores(char *buff, char **bufc, dbref player, dbref cause,
				  char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = id of the bay/mech */
	/* fargs[1] = (optional) name of the part */
	dbref it;
	int i = -1, x = 0;
	int p, b;
	int pile[BRANDCOUNT + 1][NUM_ITEMS];
	char *t;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK(nfargs < 1 || nfargs > 2,
			 "#-1 FUNCTION (BTSTORES) EXPECTS 1 OR 2 ARGUMENTS");
	it = match_thing(player, fargs[0]);
	FUNCHECK(!Good_obj(it), "#-1 INVALID TARGET");
	if(nfargs > 1) {
		i = -1;
		if(!find_matching_long_part(fargs[1], &i, &p, &b)) {
			i = -1;
			FUNCHECK(!find_matching_vlong_part(fargs[1], &i, &p, &b),
					 "#-1 INVALID PART NAME");
		}
		safe_tprintf_str(buff, bufc, "%d", econ_find_items(it, p, b));
	} else {
		memset(pile, 0, sizeof(pile));
		t = silly_atr_get(it, A_ECONPARTS);
		while (*t) {
			if(*t == '[')
				if((sscanf(t, "[%d,%d,%d]", &i, &p, &b)) == 3)
					pile[p][i] += b;
			t++;
		}
		for(i = 0; i < object_count; i++) {
			UNPACK_PART(short_sorted[i]->index, p, b);
			if(pile[b][p]) {
				if(x)
					safe_str("|", buff, bufc);
				x = pile[b][p];
				safe_tprintf_str(buff, bufc, "%s:%d",
								 part_name_long(p, b), x);
			}
		}
	}
}

void fun_btmapterr(char *buff, char **bufc, dbref player, dbref cause,
				   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = reference of map
	   fargs[1] = x
	   fargs[2] = y
	 */
	dbref it;
	MAP *map;
	int x, y;
	int spec;
	char terr;

	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1");
	spec = WhichSpecial(it);
	FUNCHECK(spec != GTYPE_MAP, "#-1");
	FUNCHECK(!(map = FindObjectsData(it)), "#-1");
	FUNCHECK(Readnum(x, fargs[1]), "#-2");
	FUNCHECK(Readnum(y, fargs[2]), "#-2");
	FUNCHECK(x < 0 || y < 0 || x >= map->map_width ||
			 y >= map->map_height, "?");
	terr = GetTerrain(map, x, y);
	if(terr == GRASSLAND)
		terr = '.';

	safe_tprintf_str(buff, bufc, "%c", terr);
}

void fun_btmapelev(char *buff, char **bufc, dbref player, dbref cause,
				   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = reference of map
	   fargs[1] = x
	   fargs[2] = y
	 */
	dbref it;
	int i;
	MAP *map;
	int x, y;
	int spec;

	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1");
	spec = WhichSpecial(it);
	FUNCHECK(spec != GTYPE_MAP, "#-1");
	FUNCHECK(!(map = FindObjectsData(it)), "#-1");
	FUNCHECK(Readnum(x, fargs[1]), "#-2");
	FUNCHECK(Readnum(y, fargs[2]), "#-2");
	FUNCHECK(x < 0 || y < 0 || x >= map->map_width
			 || y >= map->map_height, "?");
	i = Elevation(map, x, y);
	if(i < 0)
		safe_tprintf_str(buff, bufc, "-%c", '0' + -i);
	else
		safe_tprintf_str(buff, bufc, "%c", '0' + i);
}

void list_xcodevalues(dbref player)
{
	int i;

	notify(player, "Xcode attributes accessible thru get/setxcodevalue:");
	for(i = 0; xcode_data[i].name; i++)
		notify(player, tprintf("\t%d\t%s", xcode_data[i].gtype,
							   xcode_data[i].name));
}

/* Glue functions for easy scode interface to ton of hcode stuff */

void fun_btdesignex(char *buff, char **bufc, dbref player, dbref cause,
					char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char *id = fargs[0];

	if(mechref_path(id)) {
		safe_tprintf_str(buff, bufc, "1");
	} else
		safe_tprintf_str(buff, bufc, "0");
}

void fun_btdamages(char *buff, char **bufc, dbref player, dbref cause,
				   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = id of the mech
	 */
	dbref it;
	char *damstr;
	MECH *mech;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(it), "#-1 NOT A MECH");
	FUNCHECK(!(mech = FindObjectsData(it)), "#-1");
	damstr = damages_func(mech);
	safe_tprintf_str(buff, bufc, damstr ? damstr : "#-1 ERROR");
}

void fun_btcritstatus(char *buff, char **bufc, dbref player, dbref cause,
					  char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = id of the mech
	 * fargs[1] = location to show
	 */
	dbref it;
	char *critstr;
	MECH *mech;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(it), "#-1 NOT A MECH");
	FUNCHECK(!(mech = FindObjectsData(it)), "#-1");
	critstr = critstatus_func(mech, fargs[1]);	/* fargs[1] unguaranteed ! */
	safe_tprintf_str(buff, bufc, critstr ? critstr : "#-1 ERROR");
}

void fun_btarmorstatus(char *buff, char **bufc, dbref player, dbref cause,
					   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = id of the mech
	 * fargs[1] = location to show
	 */
	dbref it;
	char *infostr;
	MECH *mech;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(it), "#-1 NOT A MECH");
	FUNCHECK(!(mech = FindObjectsData(it)), "#-1");
	infostr = armorstatus_func(mech, fargs[1]);	/* fargs[1] unguaranteed ! */
	safe_tprintf_str(buff, bufc, infostr ? infostr : "#-1 ERROR");
}

void fun_btweaponstatus(char *buff, char **bufc, dbref player, dbref cause,
						char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = id of the mech
	 * fargs[1] = location to show
	 */
	dbref it;
	char *infostr;
	MECH *mech;

	FUNCHECK(nfargs < 1 || nfargs > 2,
			 "#-1 FUNCTION (BTWEAPONSTATUS) EXPECTS 1 OR 2 ARGUMENTS");

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(it), "#-1 NOT A MECH");
	FUNCHECK(!(mech = FindObjectsData(it)), "#-1");
	infostr = weaponstatus_func(mech, nfargs == 2 ? fargs[1] : NULL);
	safe_tprintf_str(buff, bufc, infostr ? infostr : "#-1 ERROR");
}

void fun_btcritstatus_ref(char *buff, char **bufc, dbref player, dbref cause,
						  char *fargs[], int nfargs, char *cargs[],
						  int ncargs)
{
	/* fargs[0] = ref of the mech
	 * fargs[1] = location to show
	 */
	char *critstr;
	MECH *mech;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK((mech = load_refmech(fargs[0])) == NULL, "#-1 NO SUCH MECH");
	critstr = critstatus_func(mech, fargs[1]);	/* fargs[1] unguaranteed ! */
	safe_tprintf_str(buff, bufc, critstr ? critstr : "#-1 ERROR");
}

void fun_btarmorstatus_ref(char *buff, char **bufc, dbref player, dbref cause,
						   char *fargs[], int nfargs, char *cargs[],
						   int ncargs)
{
	/* fargs[0] = ref of the mech
	 * fargs[1] = location to show
	 */
	char *infostr;
	MECH *mech;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK((mech = load_refmech(fargs[0])) == NULL, "#-1 NO SUCH MECH");
	infostr = armorstatus_func(mech, fargs[1]);	/* fargs[1] unguaranteed ! */
	safe_tprintf_str(buff, bufc, infostr ? infostr : "#-1 ERROR");
}

void fun_btweaponstatus_ref(char *buff, char **bufc, dbref player,
							dbref cause, char *fargs[], int nfargs,
							char *cargs[], int ncargs)
{
	/* fargs[0] = ref of the mech
	 * fargs[1] = location to show
	 */
	char *infostr;
	MECH *mech;

	FUNCHECK(nfargs < 1 || nfargs > 2,
			 "#-1 FUNCTION (BTWEAPONREF) EXPECTS 1 OR 2 ARGUMENTS");

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK((mech = load_refmech(fargs[0])) == NULL, "#-1 NO SUCH MECH");
	infostr = weaponstatus_func(mech, nfargs == 2 ? fargs[1] : NULL);
	safe_tprintf_str(buff, bufc, infostr ? infostr : "#-1 ERROR");
}

void fun_btsetarmorstatus(char *buff, char **bufc, dbref player, dbref cause,
						  char *fargs[], int nfargs, char *cargs[],
						  int ncargs)
{
	/* fargs[0] = id of the mech
	 * fargs[1] = location to set
	 * fargs[2] = what to change
	 * fargs[3] = value to change to.
	 */
	dbref it;
	char *infostr;
	MECH *mech;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(it), "#-1 NOT A MECH");
	FUNCHECK(!(mech = FindObjectsData(it)), "#-1");
	infostr = setarmorstatus_func(mech, fargs[1], fargs[2], fargs[3]);	/* fargs[1] unguaranteed ! */
	safe_tprintf_str(buff, bufc, infostr ? infostr : "#-1 ERROR");
}

void fun_btthreshold(char *buff, char **bufc, dbref player, dbref cause,
					 char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
	 * fargs[0] = skill to query
	 */
	int xpth;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	xpth = btthreshold_func(fargs[0]);
	safe_tprintf_str(buff, bufc, xpth < 0 ? "#%d ERROR" : "%d", xpth);
}

void fun_btdamagemech(char *buff, char **bufc, dbref player, dbref cause,
					  char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
	 * fargs[0] = dbref of MECH object
	 * fargs[1] = total amount of damage
	 * fargs[2] = clustersize
	 * fargs[3] = direction of 'attack'
	 * fargs[4] = (try to) force crit
	 * fargs[5] = message to send to damaged 'mech
	 * fargs[6] = message to MechLOSBroadcast, prepended by mech name
	 */

	int totaldam, clustersize, direction, iscrit;
	MECH *mech;
	dbref it;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(it), "#-1 NOT A MECH");
	FUNCHECK(!(mech = FindObjectsData(it)), "#-1 UNABLE TO GET MECHDATA");
	FUNCHECK(Readnum(totaldam, fargs[1]) || totaldam < 1
			 || totaldam > 1000, "#-1 INVALID 2ND ARG");
	FUNCHECK(Readnum(clustersize, fargs[2])
			 || clustersize < 1, "#-1 INVALID 3RD ARG");
	FUNCHECK(Readnum(direction, fargs[3]), "#-1 INVALID 4TH ARG");
	FUNCHECK(Readnum(iscrit, fargs[4]), "#-1 INVALID 5TH ARG");
	safe_tprintf_str(buff, bufc, "%d", dodamage_func(player, mech,
													 totaldam, clustersize,
													 direction, iscrit,
													 fargs[5], fargs[6]));
}

void fun_bttechstatus(char *buff, char **bufc, dbref player, dbref cause,
					  char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
	 * fargs[0] = dbref of MECH object
	 */

	dbref it;
	MECH *mech;
	char *infostr;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(it), "#-1 NOT A MECH");
	FUNCHECK(!(mech = FindObjectsData(it)), "#-1 UNABLE TO GET MECHDATA");
	infostr = techstatus_func(mech);
	safe_tprintf_str(buff, bufc, "%s", infostr ? infostr : "#-1 ERROR");
}

void fun_bthexemit(char *buff, char **bufc, dbref player, dbref cause,
				   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = mapref
	   fargs[1] = x coordinate
	   fargs[2] = y coordinate
	   fargs[3] = message
	 */
	MAP *map;
	int x = -1, y = -1;
	char *msg = fargs[3];
	dbref mapnum;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	while (msg && *msg && isspace(*msg))
		msg++;
	FUNCHECK(!msg || !*msg, "#-1 INVALID MESSAGE");

	mapnum = match_thing(player, fargs[0]);
	FUNCHECK(mapnum < 0, "#-1 INVALID MAP");
	map = getMap(mapnum);
	FUNCHECK(!map, "#-1 INVALID MAP");

	x = atoi(fargs[1]);
	y = atoi(fargs[2]);
	FUNCHECK(x < 0 || x > map->map_width || y < 0 || y > map->map_height,
			 "#-1 INVALID COORDINATES");
	HexLOSBroadcast(map, x, y, msg);
	safe_tprintf_str(buff, bufc, "1");
}

void fun_btmakepilotroll(char *buff, char **bufc, dbref player, dbref cause,
						 char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = mechref
	   fargs[1] = roll modifier
	   fargs[2] = damage modifier
	 */

	MECH *mech;
	int rollmod = 0, dammod = 0;
	dbref mechnum;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	mechnum = match_thing(player, fargs[0]);
	FUNCHECK(mechnum == NOTHING || !Examinable(player, mechnum),
			 "#-1 INVALID MECH");
	FUNCHECK(!IsMech(mechnum), "#-1 INVALID MECH");
	FUNCHECK(!(mech = FindObjectsData(mechnum)), "#-1 INVALID MECH");

	/* No checking on rollmod/dammod, they're assumed to be 0 if invalid. */
	rollmod = atoi(fargs[1]);
	dammod = atoi(fargs[2]);

	if(MadePilotSkillRoll(mech, rollmod)) {
		safe_tprintf_str(buff, bufc, "1");
	} else {
		MechFalls(mech, dammod, 1);
		safe_tprintf_str(buff, bufc, "0");
	}
}

void fun_btid2db(char *buff, char **bufc, dbref player, dbref cause,
				 char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = mech 
	   fargs[1] = target ID */
	MECH *target;
	MECH *mech = NULL;
	dbref mechnum;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	mechnum = match_thing(player, fargs[0]);
	FUNCHECK(mechnum == NOTHING || !Examinable(player, mechnum),
			 "#-1 INVALID MECH/MAP");
	FUNCHECK(strlen(fargs[1]) != 2, "#-1 INVALID TARGETID");
	if(IsMech(mechnum)) {
		FUNCHECK(!(mech = getMech(mechnum)), "#-1 INVALID MECH");
		mechnum = FindTargetDBREFFromMapNumber(mech, fargs[1]);
	} else if(IsMap(mechnum)) {
		MAP *map;
		FUNCHECK(!(map = getMap(mechnum)), "#-1 INVALID MAP");
		mechnum = FindMechOnMap(map, fargs[1]);
	} else {
		safe_str("#-1 INVALID MECH/MAP", buff, bufc);
		return;
	}
	FUNCHECK(mechnum < 0, "#-1 INVALID TARGETID");
	if(mech) {
		FUNCHECK(!(target = getMech(mechnum)), "#-1 INVALID TARGETID");
		FUNCHECK(!InLineOfSight_NB(mech, target, MechX(target), MechY(target),
								   FlMechRange(getMap(mech->mapindex), mech,
											   target)),
				 "#-1 INVALID TARGETID");
	}
	safe_tprintf_str(buff, bufc, "#%d", (int) mechnum);
}

void fun_bthexlos(char *buff, char **bufc, dbref player, dbref cause,
				  char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = mech 
	   fargs[1] = x
	   fargs[2] = y
	 */

	MECH *mech;
	MAP *map;
	int x = -1, y = -1, mechnum;
	float fx, fy;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	mechnum = match_thing(player, fargs[0]);
	FUNCHECK(mechnum == NOTHING || !Examinable(player, mechnum),
			 "#-1 INVALID MECH");
	FUNCHECK(!IsMech(mechnum), "#-1 INVALID MECH");
	FUNCHECK(!(mech = getMech(mechnum)), "#-1 INVALID MECH");
	FUNCHECK(!(map = getMap(mech->mapindex)), "#-1 INTERNAL ERROR");

	x = atoi(fargs[1]);
	y = atoi(fargs[2]);
	FUNCHECK(x < 0 || x > map->map_width || y < 0 || y > map->map_height,
			 "#-1 INVALID COORDINATES");
	MapCoordToRealCoord(x, y, &fx, &fy);
	if(InLineOfSight_NB(mech, NULL, x, y,
						FindHexRange(MechFX(mech), MechFY(mech), fx, fy)))
		safe_tprintf_str(buff, bufc, "1");
	else
		safe_tprintf_str(buff, bufc, "0");
}

void fun_btlosm2m(char *buff, char **bufc, dbref player, dbref cause,
				  char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = mech
	   fargs[1] = target
	 */

	int mechnum;
	MECH *mech, *target;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	mechnum = match_thing(player, fargs[0]);
	FUNCHECK(mechnum == NOTHING || !Examinable(player, mechnum),
			 "#-1 INVALID MECH");
	FUNCHECK(!IsMech(mechnum), "#-1 INVALID MECH");
	FUNCHECK(!(mech = getMech(mechnum)), "#-1 INVALID MECH");

	mechnum = match_thing(player, fargs[1]);
	FUNCHECK(mechnum == NOTHING || !Examinable(player, mechnum),
			 "#-1 INVALID MECH");
	FUNCHECK(!IsMech(mechnum), "#-1 INVALID MECH");
	FUNCHECK(!(target = getMech(mechnum)), "#-1 INVALID MECH");

	if(InLineOfSight_NB(mech, target, MechX(mech), MechY(mech),
						FlMechRange(getmap(mech->mapindex), mech, target)))
		safe_tprintf_str(buff, bufc, "1");
	else
		safe_tprintf_str(buff, bufc, "0");
}

/*
 * btaddstores(<MapDB>, <PartName>, <Amount>)
 *
 * Adds the specified parts/commodities to a map. The maximum value for <PartName> is the define, ADDSTORES_MAX.
 */
void fun_btaddstores(char *buff, char **bufc, dbref player, dbref cause,
					 char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = mech/map
	   fargs[1] = partname
	   fargs[2] = quantity
	 */
	int loc;
	int index = -1, id = 0, brand = 0, count;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	loc = match_thing(player, fargs[0]);
	FUNCHECK(!Good_obj(loc), "#-1 INVALID TARGET");

	FUNCHECK(strlen(fargs[1]) >= MBUF_SIZE, "#-1 PARTNAME TOO LONG");

	FUNCHECK(!fargs[1], "#-1 NEED PARTNAME");

	/* Add a limit to the number of parts you can add at once to prevent reaching the integer limits. */
	count = atoi(fargs[2]);
	if(count > ADDSTORES_MAX) {
		count = ADDSTORES_MAX;
	}

	FUNCHECK(!count, "1");
	FUNCHECK(!find_matching_short_part(fargs[1], &index, &id, &brand) &&
			 !find_matching_vlong_part(fargs[1], &index, &id, &brand) &&
			 !find_matching_long_part(fargs[1], &index, &id, &brand), "0");
	econ_change_items(loc, id, brand, count);
	SendEcon(tprintf("#%d added %d %s to #%d", player, count,
					 get_parts_vlong_name(id, brand), loc));
	safe_tprintf_str(buff, bufc, "1");
}								/* end btaddstores() */

extern int xlate(char *);

void fun_btloadmap(char *buff, char **bufc, dbref player, dbref cause,
				   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = mapobject
	   fargs[1] = mapname
	   fargs[2] = clear or not to clear
	 */
	int mapdbref;
	MAP *map;

	FUNCHECK(nfargs < 2 || nfargs > 3,
			 "#-1 BTLOADMAP TAKES 2 OR 3 ARGUMENTS");
	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	mapdbref = match_thing(player, fargs[0]);
	FUNCHECK(!Good_obj(mapdbref), "#-1 INVALID TARGET");
	map = getMap(mapdbref);
	FUNCHECK(!map, "#-1 INVALID TARGET");
	switch (map_load(map, fargs[1])) {
	case -1:
		safe_str("#-1 MAP NOT FOUND", buff, bufc);
		return;
	case -2:
		safe_str("#-1 INVALID MAP", buff, bufc);
		return;
	case 0:
		break;
	default:
		safe_str("#-1 UNKNOWN ERROR", buff, bufc);
		return;
	}
	if(nfargs > 2 && xlate(fargs[2]))
		map_clearmechs(player, (void *) map, "");
	safe_str("1", buff, bufc);
}

void fun_btloadmech(char *buff, char **bufc, dbref player, dbref cause,
					char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = mechobject
	   fargs[1] = mechref
	 */
	int mechdbref;
	MECH *mech;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	mechdbref = match_thing(player, fargs[0]);
	FUNCHECK(!Good_obj(mechdbref), "#-1 INVALID TARGET");
	mech = getMech(mechdbref);
	FUNCHECK(!mech, "#-1 INVALID TARGET");
	if(mech_loadnew(player, mech, fargs[1]) == 1) {
		muxevent_remove_data((void *) mech);
		clear_mech_from_LOS(mech);
		safe_str("1", buff, bufc);
	} else {
		safe_str("#-1 UNABLE TO LOAD TEMPLATE", buff, bufc);
	}
}

extern char radio_colorstr[];

void fun_btmechfreqs(char *buff, char **bufc, dbref player, dbref cause,
					 char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = mechobject
	 */
	int mechdbref;
	MECH *mech;
	int i;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	mechdbref = match_thing(player, fargs[0]);
	FUNCHECK(!Good_obj(mechdbref), "#-1 INVALID TARGET");
	mech = getMech(mechdbref);
	FUNCHECK(!mech, "#-1 INVALID TARGET");

	for(i = 0; i < MFreqs(mech); i++) {
		if(i)
			safe_str(",", buff, bufc);
		safe_tprintf_str(buff, bufc, "%d|%d|%s",
						 i + 1, mech->freq[i],
						 bv2text(mech->freqmodes[i] % FREQ_REST));
		if(mech->freqmodes[i] / FREQ_REST) {
			safe_tprintf_str(buff, bufc, "|%c",
							 radio_colorstr[mech->freqmodes[i] / FREQ_REST -
											1]);
		} else {
			safe_str("|-", buff, bufc);
		}
	}
}

void fun_btgetweight(char *buff, char **bufc, dbref player, dbref cause,
					 char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
	   fargs[0] = stringname of part
	 */
	float sw = 0;
	int i = -1, p, b;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	if(!find_matching_long_part(fargs[0], &i, &p, &b)) {
		i = -1;
		FUNCHECK(!find_matching_vlong_part(fargs[0], &i, &p, &b),
				 "#-1 INVALID PART NAME");
	}
	sw = GetPartWeight(p);
	if(sw <= 0)
		sw = (1024 * 100);
	safe_tprintf_str(buff, bufc, tprintf("%.3f", (float) sw / 1024));
}

void fun_btremovestores(char *buff, char **bufc, dbref player, dbref cause,
						char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = id of the bay */
	/* fargs[1] = name of the part */
	/* fargs[2] = amount */
	dbref it;
	int i = -1, spec;
	int num = 0;
	void *foo;
	int p, b;

	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1");
	spec = WhichSpecial(it);
	FUNCHECK(!(foo = FindObjectsData(it)), "#-1");
	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK(Readnum(num, fargs[2]), "#-2 Illegal Value");
	if(!find_matching_long_part(fargs[1], &i, &p, &b)) {
		i = -1;
		FUNCHECK(!find_matching_vlong_part(fargs[1], &i, &p, &b),
				 "#-1 INVALID PART NAME");
	}
	econ_change_items(it, p, b, 0 - num);
	safe_tprintf_str(buff, bufc, "%d", econ_find_items(it, p, b));
}

void fun_bttechtime(char *buff, char **bufc, dbref player, dbref cause,
					char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	time_t old;
	char *olds = silly_atr_get(player, A_TECHTIME);
	char buf[MBUF_SIZE];

	if(olds) {
		old = (time_t) atoi(olds);
		if(old < mudstate.now) {
			strcpy(buf, "00:00.00");
		} else {
			old -= mudstate.now;
			sprintf(buf, "%02ld:%02d.%02d", (long) (old / 3600),
					(int) ((old / 60) % 60), (int) (old % 60));
		}
	} else {
		strcpy(buf, "00:00.00");
	}

	notify(player, buf);
}

void fun_btcritslot(char *buff, char **bufc, dbref player, dbref cause,
					char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = id of the mech
	   fargs[1] = location name
	   fargs[2] = critslot
	   fargs[3] = partname type flag, 0 template name, 1 repair part name (differentiate Ammo types basically)
	 */
	dbref it;
	MECH *mech;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	if(!fn_range_check("BTCRITSLOT", nfargs, 3, 4, buff, bufc))
		return;

	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(it), "#-1 NOT A MECH");
	FUNCHECK(!(mech = FindObjectsData(it)), "#-1 INVALID MECH");

	safe_tprintf_str(buff, bufc, "%s",
					 critslot_func(mech, fargs[1], fargs[2], fargs[3]));
}

void fun_btcritslot_ref(char *buff, char **bufc, dbref player, dbref cause,
						char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = ref
	   fargs[1] = location name
	   fargs[2] = critslot
	   fargs[3] = partname type flag, 0 template name, 1 repair part name (differentiate Ammo types basically)
	 */
	MECH *mech;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	if(!fn_range_check("BTCRITSLOT_REF", nfargs, 3, 4, buff, bufc))
		return;
	FUNCHECK((mech = load_refmech(fargs[0])) == NULL, "#-1 NO SUCH MECH");

	safe_tprintf_str(buff, bufc, "%s",
					 critslot_func(mech, fargs[1], fargs[2], fargs[3]));
}

#define NUMBERS ".0123456789"

void fun_btgetrange(char *buff, char **bufc, dbref player, dbref cause,
					char *fargs[], int nfargs, char *cargs[], int ncargs)
{
/* fargs[0] - [4] Combos of XY or DBref */
	dbref mechAdb, mechBdb, mapdb;
	MECH *mechA, *mechB;
	MAP *map;
	float fxA, fyA, fzA, fxB, fyB, fzB;
	int xA, yA, zA, xB, yB, zB;

	FUNCHECK(!WizR(player), "#=1 PERMISSION DENIED");

	if(!fn_range_check("BTGETRANGE", nfargs, 3, 7, buff, bufc))
		return;

	mapdb = match_thing(player, fargs[0]);
	FUNCHECK(mapdb == NOTHING
			 || !Examinable(player, mapdb), "#-1 INVALID MAPDB");
	FUNCHECK(!IsMap(mapdb), "#-1 OBJECT NOT MAP");
	FUNCHECK(!(map = getMap(mapdb)), "#-1 INVALID MAP");

	switch (nfargs) {
	case 3:
		mechAdb = match_thing(player, fargs[1]);
		FUNCHECK(mechAdb == NOTHING
				 || !Examinable(player, mechAdb), "#-1 INVALID MECHDBREF");
		mechBdb = match_thing(player, fargs[2]);
		FUNCHECK(mechBdb == NOTHING
				 || !Examinable(player, mechBdb), "#-1 INVALID MECHDBREF");
		FUNCHECK(!IsMech(mechAdb) || !IsMech(mechBdb), "#-1 INVALID MECH");
		FUNCHECK(!(mechA = getMech(mechAdb))
				 || !(mechB = getMech(mechBdb)), "#-1 INVALID MECH");
		FUNCHECK(mechA->mapindex != mapdb
				 || mechB->mapindex != mapdb, "#-1 MECH NOT ON MAP");
		safe_tprintf_str(buff, bufc, "%f", FaMechRange(mechA, mechB));
		return;
	case 4:
		if(strspn(fargs[1], NUMBERS) < 1) {
			mechAdb = match_thing(player, fargs[1]);
			FUNCHECK(strspn(fargs[2], NUMBERS) < 1, "#-1 INVALID COORDS");
			xA = atoi(fargs[2]);
			FUNCHECK(strspn(fargs[3], NUMBERS) < 1, "#-1 INVALID COORDS");
			yA = atoi(fargs[3]);
		} else {
			FUNCHECK(strspn(fargs[1], NUMBERS) < 1, "#-1 INVALID COORDS");
			xA = atoi(fargs[1]);
			FUNCHECK(strspn(fargs[2], NUMBERS) < 1, "#-1 INVALID COORDS");
			yA = atoi(fargs[2]);
			mechAdb = match_thing(player, fargs[3]);
		}
		FUNCHECK(mechAdb == NOTHING
				 || !Examinable(player, mechAdb), "#-1 INVALID MECHDBREF");
		FUNCHECK(!IsMech(mechAdb), "#-1 INVALID MECH");
		FUNCHECK(!(mechA = getMech(mechAdb)), "#-1 INVALID MECH");
		FUNCHECK(mechA->mapindex != mapdb, "#-1 MECH NOT ON MAP");
		FUNCHECK(xA < 0 || yA < 0 || xA > map->map_width
				 || yA > map->map_height, "#-1 INVALID COORDS");
		MapCoordToRealCoord(xA, yA, &fxA, &fyA);
		safe_tprintf_str(buff, bufc, "%f",
						 FindRange(MechFX(mechA), MechFY(mechA),
								   MechFZ(mechA), fxA, fyA, Elevation(map, xA,
																	  yA) *
								   ZSCALE));
		return;
	case 5:
		if(strspn(fargs[1], NUMBERS) < 1 || strspn(fargs[4], NUMBERS) < 1) {
			// this is the (map, mech, x, y, z) or (map, x, y, z, mech) condition
			if(strspn(fargs[1], NUMBERS) < 1) {
				// mech first
				mechAdb = match_thing(player, fargs[1]);
				FUNCHECK(strspn(fargs[2], NUMBERS) < 1, "#-1 INVALID COORDS");
				xA = atoi(fargs[2]);
				FUNCHECK(strspn(fargs[3], NUMBERS) < 1, "#-1 INVALID COORDS");
				yA = atoi(fargs[3]);
				FUNCHECK(strspn(fargs[4], NUMBERS) < 1, "#-1 INVALID COORDS");
				zA = atoi(fargs[4]);
			} else {
				FUNCHECK(strspn(fargs[1], NUMBERS) < 1, "#-1 INVALID COORDS");
				xA = atoi(fargs[1]);
				FUNCHECK(strspn(fargs[2], NUMBERS) < 1, "#-1 INVALID COORDS");
				yA = atoi(fargs[2]);
				FUNCHECK(strspn(fargs[3], NUMBERS) < 1, "#-1 INVALID COORDS");
				zA = atoi(fargs[3]);
				mechAdb = match_thing(player, fargs[4]);
			}
			FUNCHECK(mechAdb == NOTHING
					 || !Examinable(player, mechAdb),
					 "#-1 INVALID MECHDBREF");
			FUNCHECK(!IsMech(mechAdb), "#-1 INVALID MECH");
			FUNCHECK(!(mechA = getMech(mechAdb)), "#-1 INVALID MECH");
			FUNCHECK(mechA->mapindex != mapdb, "#-1 MECH NOT ON MAP");
			FUNCHECK(xA < 0 || yA < 0 || xA > map->map_width
					 || yA > map->map_height, "#-1 INVALID COORDS");
			MapCoordToRealCoord(xA, yA, &fxA, &fyA);
			safe_tprintf_str(buff, bufc, "%f",
							 FindRange(MechFX(mechA), MechFY(mechA),
									   MechFZ(mechA), fxA, fyA, zA * ZSCALE));
			return;
		}
		// tihs is the (map, x1, y1, x2, y2) condition
		FUNCHECK(strspn(fargs[1], NUMBERS) < 1, "#-1 INVALID COORDS");
		xA = atoi(fargs[1]);
		FUNCHECK(strspn(fargs[2], NUMBERS) < 1, "#-1 INVALID COORDS");
		yA = atoi(fargs[2]);
		FUNCHECK(xA < 0 || yA < 0 || xA > map->map_width
				 || yA > map->map_height, "#-1 INVALID COORDS");
		FUNCHECK(strspn(fargs[3], NUMBERS) < 1, "#-1 INVALID COORDS");
		xB = atoi(fargs[3]);
		FUNCHECK(strspn(fargs[4], NUMBERS) < 1, "#-1 INVALID COORDS");
		yB = atoi(fargs[4]);
		FUNCHECK(xB < 0 || yB < 0 || xB > map->map_width
				 || yB > map->map_height, "#-1 INVALID COORDS");
		MapCoordToRealCoord(xA, yA, &fxA, &fyA);
		MapCoordToRealCoord(xB, yB, &fxB, &fyB);
		safe_tprintf_str(buff, bufc, "%f",
						 FindRange(fxA, fyA, Elevation(map, xA, yA) * ZSCALE,
								   fxB, fyB, Elevation(map, xB, yB)));
		return;
	case 7:
		FUNCHECK(strspn(fargs[1], NUMBERS) < 1, "#-1 INVALID COORDS");
		xA = atoi(fargs[1]);
		FUNCHECK(strspn(fargs[2], NUMBERS) < 1, "#-1 INVALID COORDS");
		yA = atoi(fargs[2]);
		FUNCHECK(strspn(fargs[3], NUMBERS) < 1, "#-1 INVALID COORDS");
		zA = atoi(fargs[3]);
		FUNCHECK(strspn(fargs[4], NUMBERS) < 1, "#-1 INVALID COORDS");
		xB = atoi(fargs[4]);
		FUNCHECK(strspn(fargs[5], NUMBERS) < 1, "#-1 INVALID COORDS");
		yB = atoi(fargs[5]);
		FUNCHECK(strspn(fargs[6], NUMBERS) < 1, "#-1 INVALID COORDS");
		zB = atoi(fargs[6]);
		MapCoordToRealCoord(xA, yA, &fxA, &fyA);
		MapCoordToRealCoord(xB, yB, &fxB, &fyB);
		safe_tprintf_str(buff, bufc, "%f",
						 FindRange(fxA, fyA, zA * ZSCALE, fxB, fyB,
								   zB * ZSCALE));
		return;
	default:
		safe_tprintf_str(buff, bufc, "#-1 INVALID ARGUMENTS");
		return;
	}
}

extern void correct_speed(MECH *);

void fun_btsetmaxspeed(char *buff, char **bufc, dbref player, dbref cause,
					   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = id of the mech
	   fargs[1] = what the new maxspeed should be set too
	 */
	dbref it;
	MECH *mech;
	float newmaxspeed = atof(fargs[1]);

	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(it), "#-1 NOT A MECH");
	FUNCHECK(!(mech = FindObjectsData(it)), "#-1");
	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	MechMaxSpeed(mech) = newmaxspeed;
	correct_speed(mech);

	safe_tprintf_str(buff, bufc, "1");
}

void fun_btgetrealmaxspeed(char *buff, char **bufc, dbref player, dbref cause,
						   char *fargs[], int nfargs, char *cargs[],
						   int ncargs)
{
	dbref it;
	MECH *mech;
	float speed;

	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(it), "#-1 NOT A MECH");
	FUNCHECK(!(mech = FindObjectsData(it)), "#-1");
	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	speed = MechCargoMaxSpeed(mech, MechMaxSpeed(mech));

	safe_tprintf_str(buff, bufc, tprintf("%f", speed));
}

void fun_btgetbv(char *buff, char **bufc, dbref player, dbref cause,
				 char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it;
	MECH *mech;
	int bv;

	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(it), "#-1 NOT A MECH");
	FUNCHECK(!(mech = FindObjectsData(it)), "#-1");
	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	bv = CalculateBV(mech, 100, 100);
	MechBV(mech) = bv;
	safe_tprintf_str(buff, bufc, tprintf("%d", bv));
}

void fun_btgetbv_ref(char *buff, char **bufc, dbref player, dbref cause,
					 char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	MECH *mech;
	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK((mech = load_refmech(fargs[0])) == NULL, "#-1 NO SUCH MECH");

	MechBV(mech) = CalculateBV(mech, 4, 5);
	safe_tprintf_str(buff, bufc, "%d", MechBV(mech));
}

void fun_bttechlist(char *buff, char **bufc, dbref player, dbref cause,
					char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it;
	MECH *mech;
	int i, ii, part = 0, axe = 0, sword = 0, mace = 0, hascase = 0;
	char *infostr;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(it), "#-1 NOT A MECH");
	FUNCHECK(!(mech = FindObjectsData(it)), "#-1");
	infostr = techlist_func(mech);
	safe_tprintf_str(buff, bufc, infostr ? infostr : " ");
}

void fun_bttechlist_ref(char *buff, char **bufc, dbref player, dbref cause,
						char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref it;
	MECH *mech;
	int i, ii, part = 0, axe = 0, sword = 0, mace = 0, hascase = 0;
	char *infostr;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK((mech = load_refmech(fargs[0])) == NULL, "#-1 NO SUCH MECH");

	infostr = techlist_func(mech);
	safe_tprintf_str(buff, bufc, infostr ? infostr : "#-1");
}

/* Function to return the 'payload' of a unit
 * ie: the Guns and Ammo
 * in a list format like <item_1> <# of 1>|...|<item_n> <# of n>
 * Dany - 06/2005 */
void fun_btpayload_ref(char *buff, char **bufc, dbref player, dbref cause,
					   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	MECH *mech;
	char *infostr;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK((mech = load_refmech(fargs[0])) == NULL, "#-1 NO SUCH MECH");

	infostr = payloadlist_func(mech);
	safe_tprintf_str(buff, bufc, infostr ? infostr : "#-1");
}

void fun_btshowstatus_ref(char *buff, char **bufc, dbref player, dbref cause,
						  char *fargs[], int nfargs, char *cargs[],
						  int ncargs)
{
	dbref outplayer;
	MECH *mech;
	char *infostr;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK((mech = load_refmech(fargs[0])) == NULL, "#-1 NO SUCH MECH");
	outplayer = match_thing(player, fargs[1]);
	FUNCHECK(outplayer == NOTHING || !Examinable(player, outplayer)
			 || !isPlayer(outplayer), "#-1");

	mech_status(outplayer, (void *) mech, "R");
	safe_tprintf_str(buff, bufc, "1");
}

void fun_btshowwspecs_ref(char *buff, char **bufc, dbref player, dbref cause,
						  char *fargs[], int nfargs, char *cargs[],
						  int ncargs)
{
	dbref outplayer;
	MECH *mech;
	char *infostr;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK((mech = load_refmech(fargs[0])) == NULL, "#-1 NO SUCH MECH");
	outplayer = match_thing(player, fargs[1]);
	FUNCHECK(outplayer == NOTHING || !Examinable(player, outplayer)
			 || !isPlayer(outplayer), "#-1");

	mech_weaponspecs(outplayer, (void *) mech, "");
	safe_tprintf_str(buff, bufc, "1");
}

void fun_btshowcritstatus_ref(char *buff, char **bufc, dbref player,
							  dbref cause, char *fargs[], int nfargs,
							  char *cargs[], int ncargs)
{
	dbref outplayer;
	MECH *mech;
	char *infostr;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK((mech = load_refmech(fargs[0])) == NULL, "#-1 NO SUCH MECH");
	outplayer = match_thing(player, fargs[1]);
	FUNCHECK(outplayer == NOTHING || !Examinable(player, outplayer)
			 || !isPlayer(outplayer), "#-1");

	mech_critstatus(outplayer, (void *) mech, fargs[2]);
	safe_tprintf_str(buff, bufc, "1");
}

void fun_btengrate(char *buff, char **bufc, dbref player, dbref cause,
				   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref mechdb;
	MECH *mech;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	mechdb = match_thing(player, fargs[0]);
	FUNCHECK(mechdb == NOTHING
			 || !Examinable(player, mechdb), "#-1 NOT A MECH");
	FUNCHECK(!IsMech(mechdb), "#-1 NOT A MECH");
	FUNCHECK(!(mech = getMech(mechdb)), "#-1");

	safe_tprintf_str(buff, bufc, "%d %d", MechEngineSize(mech),
					 susp_factor(mech));
}

void fun_btengrate_ref(char *buff, char **bufc, dbref player, dbref cause,
					   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	MECH *mech;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK(!(mech = load_refmech(fargs[0])), "#-1 INVALID REF");

	safe_tprintf_str(buff, bufc, "%d %d", MechEngineSize(mech),
					 susp_factor(mech));
}

void fun_btfasabasecost_ref(char *buff, char **bufc, dbref player,
							dbref cause, char *fargs[], int nfargs,
							char *cargs[], int ncargs)
{
#ifdef BT_ADVANCED_ECON
	MECH *mech;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	FUNCHECK(!(mech = load_refmech(fargs[0])), "#-1 INVALID REF");

	safe_tprintf_str(buff, bufc, "%lld", CalcFasaCost(mech));
#else
	safe_tprintf_str(buff, bufc, "#-1 NO ECONDB SUPPORT");
#endif
}

void fun_btweapstat(char *buff, char **bufc, dbref player, dbref cause,
					char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = weapon name
	 * fargs[1] = stat type
	 */

	int i = -1, p, weapindx, val = -1, b;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	if(!find_matching_long_part(fargs[0], &i, &p, &b)) {
		i = -1;
		FUNCHECK(!find_matching_vlong_part(fargs[0], &i, &p, &b),
				 "#-1 INVALID PART NAME");
	}
	if(!IsWeapon(p)) {
		safe_tprintf_str(buff, bufc, "#-1 NOT A WEAPON");
		return;
	}
	weapindx = Weapon2I(p);
	if(strcasecmp("VRT", fargs[1]) == 0)
		val = MechWeapons[weapindx].vrt;
	else if(strcasecmp("TYPE", fargs[1]) == 0)
		val = MechWeapons[weapindx].type;
	else if(strcasecmp("HEAT", fargs[1]) == 0)
		val = MechWeapons[weapindx].heat;
	else if(strcasecmp("DAMAGE", fargs[1]) == 0)
		val = MechWeapons[weapindx].damage;
	else if(strcasecmp("MIN", fargs[1]) == 0)
		val = MechWeapons[weapindx].min;
	else if(strcasecmp("SR", fargs[1]) == 0)
		val = MechWeapons[weapindx].shortrange;
	else if(strcasecmp("MR", fargs[1]) == 0)
		val = MechWeapons[weapindx].medrange;
	else if(strcasecmp("LR", fargs[1]) == 0)
		val = MechWeapons[weapindx].longrange;
	else if(strcasecmp("CRIT", fargs[1]) == 0)
		val = MechWeapons[weapindx].criticals;
	else if(strcasecmp("AMMO", fargs[1]) == 0)
		val = MechWeapons[weapindx].ammoperton;
	else if(strcasecmp("WEIGHT", fargs[1]) == 0)
		val = MechWeapons[weapindx].weight;
	else if(strcasecmp("BV", fargs[1]) == 0)
		val = MechWeapons[weapindx].battlevalue;
#if 0
	else if(strcasecmp("ABV", fargs[1]) == 0)
		val = MechWeapons[weapindx].abattlevalue;
	else if(strcasecmp("REP", fargs[1]) == 0)
		val = MechWeapons[weapindx].reptime;
	else if(strcasecmp("WCLASS", fargs[1]) == 0)
		val = MechWeapons[weapindx].class;
#endif
	if(val == -1)
		safe_tprintf_str(buff, bufc, "#-1");
	safe_tprintf_str(buff, bufc, "%d", val);
}

void fun_btnumrepjobs(char *buff, char **bufc, dbref player, dbref cause,
					  char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	extern int damage_last;
	MECH *mech;
	dbref it;

	it = match_thing(player, fargs[0]);
	FUNCHECK(it == NOTHING || !Examinable(player, it), "#-1");
	FUNCHECK(!IsMech(it), "#-2");
	mech = FindObjectsData(it);

	if(unit_is_fixable(mech))
		make_damage_table(mech);
	else
		make_scrap_table(mech);

	safe_tprintf_str(buff, bufc, "%d", damage_last);
}

void fun_btsetxy(char *buff, char **bufc, dbref player, dbref cause,
				 char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
	   fargs[0] = mech
	   fargs[1] = map
	   fargs[2] = x
	   fargs[3] = y
	   fargs[4] = z

	 */
	dbref mechdb, mapdb;
	int x, y, z;
	MECH *mech;
	MAP *map;
	char buffer[MBUF_SIZE];

	FUNCHECK(nfargs < 4 || nfargs > 5, "#-1 INVALID ARGUMENT");
	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	mechdb = match_thing(player, fargs[0]);
	FUNCHECK(!Good_obj(mechdb), "#-1 INVALID TARGET");
	mech = getMech(mechdb);
	FUNCHECK(!mech, "#-1 INVALID TARGET");

	mapdb = match_thing(player, fargs[1]);
	FUNCHECK(mapdb == NOTHING
			 || !Examinable(player, mapdb), "#-1 INVALID MAP");
	FUNCHECK(!IsMap(mapdb), "#-1 INVALID MAP");
	FUNCHECK(!(map = getMap(mapdb)), "#-1 INVALID MAP");

	x = atoi(fargs[2]);
	y = atoi(fargs[3]);
	FUNCHECK(x < 0 || x > map->map_width, "#-1 X COORD");
	FUNCHECK(y < 0 || y > map->map_height, "#-1 Y COORD");

	if(nfargs == 5) {
		z = atoi(fargs[4]);
		FUNCHECK(z < 0 || z > 10000, "#-1 Z COORD");
	}

	snprintf(buffer, MBUF_SIZE, "%d", mapdb);
	mech_Rsetmapindex(GOD, (void *) mech, buffer);
	if(nfargs == 5) {
		snprintf(buffer, MBUF_SIZE, "%d %d %d", x, y, z);
	} else {
		snprintf(buffer, MBUF_SIZE, "%d %d", x, y);
	}
	mech_Rsetxy(GOD, (void *) mech, buffer);
	safe_tprintf_str(buff, bufc, "1");
}

void fun_btmapunits(char *buff, char **bufc, dbref player, dbref cause,
					char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
	 * fargs[0] = mapref
	 *
	 * OR
	 *
	 * fargs[0] = mapref
	 * fargs[1] = x
	 * fargs[2] = y
	 * fargs[3] = range
	 *
	 * OR
	 *
	 * fargs[0] = mapref
	 * fargs[1] = x
	 * fargs[2] = y
	 * fargs[3] = z
	 * fargs[4] = range
	 */

	MAP *map;
	float x, y, z, range, realX, realY;
	MECH *mech;
	int loop;
	dbref mapnum;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	mapnum = match_thing(player, fargs[0]);
	FUNCHECK(mapnum < 0, "#-1 INVALID MAP");
	map = getMap(mapnum);
	FUNCHECK(!map, "#-1 INVALID MAP");

	switch (nfargs) {
	case 1:
		for(loop = 0; loop < map->first_free; loop++) {
			if(map->mechsOnMap[loop] < 0)
				continue;
			mech = getMech(map->mechsOnMap[loop]);

			if(mech)
				safe_tprintf_str(buff, bufc, "#%d ", map->mechsOnMap[loop]);
		}
		break;
	case 4:
		x = atof(fargs[1]);
		y = atof(fargs[2]);
		range = atof(fargs[3]);
		FUNCHECK(x < 0 || x > map->map_width, "#-1 INVALID X COORD");
		FUNCHECK(y < 0 || y > map->map_height, "#-1 INVALID Y COORD");
		FUNCHECK(range < 0, "#-1 INVALID RANGE");
		MapCoordToRealCoord(x, y, &realX, &realY);
		for(loop = 0; loop < map->first_free; loop++) {
			if(map->mechsOnMap[loop] < 0)
				continue;
			mech = getMech(map->mechsOnMap[loop]);
			if(mech
			   && FindXYRange(realX, realY, MechFX(mech),
							  MechFY(mech)) <= range)
				safe_tprintf_str(buff, bufc, "#%d ", map->mechsOnMap[loop]);
		}
		break;
	case 5:
		x = atof(fargs[1]);
		y = atof(fargs[2]);
		z = atof(fargs[3]);
		range = atof(fargs[4]);
		FUNCHECK(x < 0 || x > map->map_width, "#-1 INVALID X COORD");
		FUNCHECK(y < 0 || y > map->map_height, "#-1 INVALID Y COORD");
		FUNCHECK(range < 0, "#-1 INVALID RANGE");
		MapCoordToRealCoord(x, y, &realX, &realY);
		for(loop = 0; loop < map->first_free; loop++) {
			if(map->mechsOnMap[loop] < 0)
				continue;
			mech = getMech(map->mechsOnMap[loop]);

			if(mech
			   && FindRange(realX, realY, z * ZSCALE, MechFX(mech),
							MechFY(mech), MechFZ(mech)) <= range)
				safe_tprintf_str(buff, bufc, "#%d ", map->mechsOnMap[loop]);
		}
		break;
	default:
		safe_tprintf_str(buff, bufc, "#-1 INVALID ARGUMENTS");
		break;
	}

	return;
}

int MapLimitedBroadcast3d(MAP * map, float x, float y, float z, float range,
						  char *message);
int MapLimitedBroadcast2d(MAP * map, float x, float y, float range,
						  char *message);

void fun_btmapemit(char *buff, char **bufc, dbref player, dbref cause,
				   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = mapref
	   fargs[1] = message

	   OR

	   fargs[0] = mapref
	   fargs[1] = x
	   fargs[2] = y
	   fargs[3] = range
	   fargs[4] = message

	   OR

	   fargs[0] = mapref
	   fargs[1] = x
	   fargs[2] = y
	   fargs[3] = z
	   fargs[4] = range
	   fargs[5] = message

	 */

	MAP *map;
	dbref mapnum;
	float x, y, realX, realY, z, range;

	FUNCHECK(nfargs < 2, "#-1 TOO FEW ARGUMENTS");
	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	mapnum = match_thing(player, fargs[0]);
	FUNCHECK(mapnum < 0, "#-1 INVALID MAP");
	map = getMap(mapnum);
	FUNCHECK(!map, "#-1 INVALID MAP");

	switch (nfargs) {
	case 2:
		FUNCHECK(!fargs[1] || !*fargs[1], "#-1 INVALID MESSAGE");
		MapBroadcast(map, fargs[1]);
		safe_tprintf_str(buff, bufc, "1");
		break;
	case 5:
		x = atof(fargs[1]);
		y = atof(fargs[2]);
		range = atof(fargs[3]);
		FUNCHECK(x < 0 || x > map->map_width, "#-1 ILLEGAL X COORD");
		FUNCHECK(y < 0 || y > map->map_height, "#-1 ILLEGAL Y COORD");
		FUNCHECK(range < 0, "#-1 ILLEGAL RANGE");
		FUNCHECK(!fargs[4] || !*fargs[4], "#-1 INVALID MESSAGE");
		MapCoordToRealCoord(x, y, &realX, &realY);
		safe_tprintf_str(buff, bufc, "%d",
						 MapLimitedBroadcast2d(map, realX, realY, range,
											   fargs[4]));
		break;
	case 6:
		x = atof(fargs[1]);
		y = atof(fargs[2]);
		z = atof(fargs[3]);
		range = atof(fargs[4]);
		FUNCHECK(x < 0 || x > map->map_width, "#-1 ILLEGAL X COORD");
		FUNCHECK(y < 0 || y > map->map_height, "#-1 ILLEGAL Y COORD");
		FUNCHECK(z < 0 || z > 100000, "#-1 ILLEGAL Z COORD");	// XXX: Is this accurate?
		FUNCHECK(range < 0, "#-1 ILLEGAL RANGE");
		FUNCHECK(!fargs[5] || !*fargs[5], "#-1 INVALID MESSAGE");
		MapCoordToRealCoord(x, y, &realX, &realY);	// XXX: should we deal with z?
		safe_tprintf_str(buff, bufc, "%d",
						 MapLimitedBroadcast3d(map, realX, realY, z * ZSCALE,
											   range, fargs[5]));
		break;
	default:
		safe_tprintf_str(buff, bufc, "#-1 INVALID ARGUMENTS");
		return;
	}

	return;
}

void fun_btparttype(char *buff, char **bufc, dbref player, dbref cause,
					char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/*
	   fargs[0] = stringname of part
	 */
	int i = -1, p, b;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	if(!find_matching_long_part(fargs[0], &i, &p, &b)) {
		i = -1;
		FUNCHECK(!find_matching_vlong_part(fargs[0], &i, &p, &b),
				 "#-1 INVALID PART NAME");
	}
	if(strstr(fargs[0], "Sword") && !strstr(fargs[0], "PC."))
		p = I2Special(SWORD);
	if(IsWeapon(p)) {
		safe_tprintf_str(buff, bufc, "WEAP");
		return;
	} else if(IsAmmo(p) || strstr(fargs[0], "Ammo_")) {
		safe_tprintf_str(buff, bufc, "AMMO");
		return;
	} else if(IsBomb(p)) {
		safe_tprintf_str(buff, bufc, "BOMB");
		return;
	} else if(IsSpecial(p)) {
		safe_tprintf_str(buff, bufc, "PART");
		return;
#ifdef BT_COMPLEXREPAIRS
	} else if(mudconf.btech_complexrepair && IsCargo(p)
			  && Cargo2I(p) >= TON_SENSORS_FIRST
			  && Cargo2I(p) <= TON_ENGINE_COMP_LAST) {
		safe_tprintf_str(buff, bufc, "PART");
		return;
#endif
	} else if(IsCargo(p)) {
		safe_tprintf_str(buff, bufc, "CARG");
		return;
	} else {
		safe_tprintf_str(buff, bufc, "OTHER");
		return;
	}
}

void fun_btgetpartcost(char *buff, char **bufc, dbref player, dbref cause,
					   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
#ifdef BT_ADVANCED_ECON
	int i = -1, p, index, b;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	if(!find_matching_long_part(fargs[0], &i, &p, &b)) {
		i = -1;
		FUNCHECK(!find_matching_vlong_part(fargs[0], &i, &p, &b),
				 "#-1 INVALID PART NAME");
	}
	if(strstr(fargs[0], "Sword") && !strstr(fargs[0], "PC."))
		p = I2Special(SWORD);

	safe_tprintf_str(buff, bufc, "%lld", GetPartCost(p));
#else
	safe_tprintf_str(buff, bufc, "#-1 NO ECONDB SUPPORT");
#endif
}

void fun_btsetpartcost(char *buff, char **bufc, dbref player, dbref cause,
					   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
#ifdef BT_ADVANCED_ECON
	int i = -1, p, index, b;
	unsigned long long int cost;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	if(!find_matching_long_part(fargs[0], &i, &p, &b)) {
		i = -1;
		FUNCHECK(!find_matching_vlong_part(fargs[0], &i, &p, &b),
				 "#-1 INVALID PART NAME");
	}
	if(strstr(fargs[0], "Sword") && !strstr(fargs[0], "PC."))
		p = I2Special(SWORD);
	cost = atoll(fargs[1]);
	/* since we're using an unsigned long long, lets check before we push it to unsigned status */
	if(atoll(fargs[1]) < 0) {
		safe_tprintf_str(buff, bufc, "#-1 COST ERROR");
		return;
	}
	SetPartCost(p, cost);
	safe_tprintf_str(buff, bufc, "%lld", cost);
#else
	safe_tprintf_str(buff, bufc, "#-1 NO ECONDB SUPPORT");
#endif
}

void fun_btunitfixable(char *buff, char **bufc, dbref player, dbref cause,
					   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	MECH *mech;
	dbref mechdb;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");
	mechdb = match_thing(player, fargs[0]);
	FUNCHECK(!Good_obj(mechdb), "#-1 INVALID TARGET");
	mech = getMech(mechdb);
	FUNCHECK(!mech, "#-1 INVALID TARGET");

	safe_tprintf_str(buff, bufc, "%d", unit_is_fixable(mech));
}

void fun_btlistblz(char *buff, char **bufc, dbref player, dbref cause,
				   char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char buf[MBUF_SIZE] = { '\0' };
	dbref mapdb;
	MAP *map;
	mapobj *tmp;
	int i, count = 0, strcount = 0;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	mapdb = match_thing(player, fargs[0]);
	FUNCHECK(!Good_obj(mapdb), "#-1 INVALID MAP");
	FUNCHECK(!(map = getMap(mapdb)), "#-1 INVALID MAP");

	for(i = 0; i < NUM_MAPOBJTYPES; i++)
		for(tmp = first_mapobj(map, i); tmp; tmp = next_mapobj(tmp))
			if(i == TYPE_B_LZ) {
				count++;
				if(count == 1)
					strcount +=
						snprintf(buf + strcount, MBUF_SIZE - strcount,
								 "%d %d %d", tmp->x, tmp->y, tmp->datai);
				else
					strcount +=
						snprintf(buf + strcount, MBUF_SIZE - strcount,
								 "|%d %d %d", tmp->x, tmp->y, tmp->datai);
			}
	safe_tprintf_str(buff, bufc, buf);
}

void fun_bthexinblz(char *buff, char **bufc, dbref player, dbref cause,
					char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	dbref mapdb;
	MAP *map;
	mapobj *o;
	int x, y, bl = 0;
	float fx, fy, tx, ty;

	FUNCHECK(!WizR(player), "#-1 PERMISSION DENIED");

	mapdb = match_thing(player, fargs[0]);
	FUNCHECK(!Good_obj(mapdb), "#-1 INVALID MAP");
	FUNCHECK(!(map = getMap(mapdb)), "#-1 INVALID MAP");
	x = atoi(fargs[1]);
	y = atoi(fargs[2]);
	FUNCHECK(x < 0 || y < 0 || x > map->map_width
			 || y > map->map_height, "#-1 INVALID COORDS");
	MapCoordToRealCoord(x, y, &fx, &fy);

	for(o = first_mapobj(map, TYPE_B_LZ); o; o = next_mapobj(o)) {
		if(abs(x - o->x) > o->datai || abs(y - o->y) > o->datai)
			continue;
		MapCoordToRealCoord(o->x, o->y, &tx, &ty);
		if(FindHexRange(fx, fy, tx, ty) <= o->datai) {
			bl = 1;
			break;
		}
	}
	safe_tprintf_str(buff, bufc, "%d", bl);
}
