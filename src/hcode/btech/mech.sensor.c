/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 */

#include "mech.h"
#include "map.h"
#define _MECH_SENSOR_C
#include "p.mech.ecm.h"
#include "p.mech.tag.h"
#include "mech.events.h"
#include "mech.sensor.h"
#include "autopilot.h"
#include "p.mech.utils.h"
#include "p.mech.lite.h"
#include "p.mech.los.h"
#include "p.btechstats.h"

#define SEE_SENSOR_PRIMARY        1
#define SEE_SENSOR_SECONDARY      2

int Sensor_ToHitBonus(MECH * mech, MECH * target, int flag, int maplight,
					  float range, int wAmmoMode)
{
	int bth1, bth2;
	int wLightMod = (wAmmoMode & AC_INCENDIARY_MODE) ? 1 : 0;
	MAP *map = getMap(mech->mapindex);

	maplight = maplight + wLightMod;

	if(maplight < 0)
		maplight = 0;

	if(maplight > 2)
		maplight = 2;

	if(!(flag & (MECHLOSFLAG_SEESP | MECHLOSFLAG_SEESS)))
		return 10000;
	if(!(flag & MECHLOSFLAG_SEESP)) {
		bth2 = 1 + sensors[(int) MechSensor(mech)[1]].tohitbonus_func(mech,
																	  target,
																	  map,
																	  flag,
																	  maplight);
#ifdef SENSOR_BTH_DEBUG
		SendDebug(tprintf("%d: BTH S+%d", mech->mynum, bth2));
#endif
		return bth2;
	}
	if(!(flag & MECHLOSFLAG_SEESS) ||
	   (MechSensor(mech)[0] == MechSensor(mech)[1])) {
		bth1 = sensors[(int) MechSensor(mech)[0]].tohitbonus_func(mech,
																  target, map,
																  flag,
																  maplight);
#ifdef SENSOR_BTH_DEBUG
		SendDebug(tprintf("%d: BTH P+%d", mech->mynum, bth1));
#endif
		return bth1;
	}
	bth1 = sensors[(int) MechSensor(mech)[0]].tohitbonus_func(mech, target,
															  map, flag,
															  maplight);
	bth2 =
		1 + sensors[(int) MechSensor(mech)[1]].tohitbonus_func(mech, target,
															   map, flag,
															   maplight);
#ifdef SENSOR_BTH_DEBUG
	SendDebug(tprintf("%d: BTH +%d/+%d", mech->mynum, bth1, bth2));
#endif
	return MIN(bth1, bth2);
}

int Sensor_CanSee(MECH * mech, MECH * target, int *flag, int arc,
				  float range, int mapvis, int maplight, int cloudbase)
{
	int i, j = 0, sn;
	MAP *map = getMap(mech->mapindex);

	if(!(*flag & MECHLOSFLAG_SEEC2))
		return 0;
	if(target && (MechCritStatus(target) & INVISIBLE))
		return 0;
	/* Ok.. s'pose we can, at that. */
	if(MechSensor(mech)[0] != MechSensor(mech)[1]) {
#define MaxVis(mech,sensor) ((sensors[sensor].maxvis*(((sensor)>=2 && MechMove(mech)==MOVE_NONE)?140:100)) / 100)
#define MaxVar(mech,sensor) sensors[sensor].maxvvar
#define MAXVR(mech,sensor) MaxVis(mech,sensor) - MaxVar(mech, sensor)
		/* Check both seperately */
		for(i = 0; i < 2; i++) {
			sn = MechSensor(mech)[i];
			/* No chance */

			if(!sensors[sn].fullvision && !(arc & (FORWARDARC | TURRETARC)) &&
			   !Sees360(mech))
				continue;
			if(MaxVis(mech, sn) < range)
				continue;

			/* Okay, so this is a horrible, horrible hack that'll break when new
			 * sensors get added. Thankfully it's not as ugly as the rest of the
			 * constructs in this area, so i dont really care. -Foo */

			if(target) {
				if(cloudbase && sn < 3 && ((MechZ(mech) < cloudbase)
										   ? (MechZ(target) >=
											  cloudbase) : (MechZ(target)
															< cloudbase)))
					continue;
			} else {
				if(cloudbase && MechZ(mech) > cloudbase)
					continue;
			}

			if(!sensors[sn].cansee_func(mech, target, map, range, *flag))
				continue;

			if(!sensors[sn].seechance_func(target, map, sn, range, mapvis,
										   maplight))
				continue;

			if(MaxVar(mech, sn) && (MAXVR(mech, sn)) < range)
				if(((MAXVR(mech, sn)) + (MechVisMod(mech) * (MaxVar(mech,
																	sn) +
															 1)) / 100) <
				   range)
					continue;
			j += (i + 1);
		}
		return j;
	}
	sn = MechSensor(mech)[0];
	if(MaxVis(mech, sn) < range)
		return 0;
	if(cloudbase && target && sn < 3 &&
	   ((MechZ(mech) < cloudbase) ? (MechZ(target) >= cloudbase)
		: (MechZ(target) < cloudbase)))
		return 0;
	if(!sensors[sn].cansee_func(mech, target, map, range, *flag))
		return 0;
	if(!sensors[sn].seechance_func(target, map, sn, range, mapvis, maplight))
		return 0;
	if(MaxVar(mech, sn) && (MAXVR(mech, sn)) < range)
		if(((MAXVR(mech, sn)) + MechVisMod(mech) * (MaxVar(mech,
														   sn) + 1) / 100) <
		   range)
			return 0;
	return 3;
}

int Sensor_ArcBaseChance(int type, int arc)
{
	int base = 100;

	switch (type) {
	case CLASS_MW:
		if(arc & (LSIDEARC | RSIDEARC | REARARC))
			return 0;
		break;
	case CLASS_BSUIT:
	case CLASS_MECH:
		if(arc & (LSIDEARC | RSIDEARC))
			base = 70;
		else if(arc & REARARC)
			base = 40;
		break;
	default:
		if(arc & (LSIDEARC | RSIDEARC))
			base = 80;
		else if(arc & REARARC)
			base = 50;
		if(arc & TURRETARC)
			base += 15;
		break;
	}
	return base;
}

extern int bth_modifier[];

/* Slow, but sacrifices we make for sake of playability.. :-) */
int Sensor_DriverBaseChance(MECH * mech)
{
	int i = 1;

	if(MechPer(mech) <= 2)
		i = 36;
	else if(MechPer(mech) >= 12)
		i = 1;
	else
		i = (36 - bth_modifier[MechPer(mech) - 2]);
	return 64 + i;				/* Padded a bit */
}

int Sensor_Sees(MECH * mech, MECH * target, int f, int arc, float range,
				int snum, int chance_divisor, int mapvis, int maplight)
{
	MAP *map = getMap(mech->mapindex);
	int chance = (Sensor_ArcBaseChance(MechType(mech), arc) * ((target &&
																MechTeam(mech)
																!=
																MechTeam
																(target)) ?
															   Sensor_DriverBaseChance
															   (mech) : 100))
		/ 100;
	int ch2 = sensors[snum].seechance_func(target, map, snum, range,
										   mapvis, maplight);

	if(!ch2 || !sensors[snum].cansee_func(mech, target, map, range, f))
		return 0;
	if(target && IsDS(target))
		chance = chance * 4;
	if(target && MechCritStatus(target) & HIDDEN &&
	   MechTeam(mech) != MechTeam(target)) {

		if(sensors[snum].matchletter[0] == 'B' &&
		   (MechInfantrySpecials(target) & STEALTH_TECH) &&
		   (!(MechInfantrySpecials(target) & CS_PURIFIER_STEALTH_TECH)))
			return 0;

		if(ch2 <= 100) {
			if(range > 5)
				return 0;
			chance = chance / 4;
		}
	}
	if(range < 3 || Number(1, 10000) < ((chance * ch2) / chance_divisor)) {
		if(target && In_Character(mech->mynum) &&
		   In_Character(target->mynum) && MechTeam(mech) != MechTeam(target))
			if(!Number(0, 5))
				MadePerceptionRoll(mech, -2);
		return 1;
	}
	return 0;
}

/* Main idea: If primary & secondary scanner differ,
   check both scanners for chance (with secondary at 1/2 chance).
   Also, if non-360degree scanners are used, check only forward arc for
   them. 

   If both same, multiply chance by 1.1
 */
int Sensor_SeesNow(MECH * mech, MECH * target, int f, int arc, float range,
				   int mapvis, int maplight)
{
	int i, sn;

	if(MechSensor(mech)[0] != MechSensor(mech)[1]) {
		/* Check both seperately */
		for(i = 0; i < 2; i++) {
			sn = MechSensor(mech)[i];
			/* No chance */
			if(!sensors[sn].fullvision && !(arc & (FORWARDARC | TURRETARC)) &&
			   !Sees360(mech))
				continue;
			if(MaxVis(mech, sn) < range)
				continue;
			if(Sensor_Sees(mech, target, f, arc, range, sn, i + 1, mapvis,
						   maplight))
				return (i + 1);
		}
		return 0;
	}
	sn = MechSensor(mech)[0];
	if(MaxVis(mech, sn) < range)
		return 0;
	return (Sensor_Sees(mech, target, f, arc, range, sn, 1, mapvis,
						maplight));
	return 3;
}

char *my_dump_flag(int i)
{
	int j;
	static char buf[MBUF_SIZE];

	strcpy(buf, "");
	for(j = 0; j < 32; j++)
		if(i & (1 << j)) {
			if(buf[0] == 0)
				sprintf(buf, "%d", j);
			else
				sprintf(buf + strlen(buf), ",%d", j);
		}
	return buf;
}

#define AUTOCON_LONG		0x01
#define AUTOCON_WARN		0x02
#define AUTOCON_SHORT		0x04

static int valid_to_notice(MECH * mech, MECH * targ, int los)
{
	int bf = (mech->brief / 4);
	int foe;

	if((los < 0 && MechSeemsFriend(mech, targ)) ||
	   (los > 0 && MechTeam(mech) == MechTeam(targ)))
		foe = 0;
	else
		foe = AUTOCON_WARN;

	switch (bf) {
	case 0:
		return AUTOCON_LONG | foe;
	case 1:
		return AUTOCON_SHORT | foe;
	case 2:
		return foe ? (AUTOCON_LONG | foe) : 0;
	case 3:
		return foe ? (AUTOCON_SHORT | foe) : 0;
	case 4:
		return AUTOCON_SHORT;
	case 5:
		return foe ? AUTOCON_SHORT : 0;
	case 6:
	default:
		return 0;
	}
}

void Sensor_DoWeSeeNow(MECH * mech, unsigned short *fl, float range, int x,
					   int y, MECH * target, int mapvis, int maplight,
					   int cloudbase, int seeanew, int wlf)
{
	int arc;
	float x1, y1;
	int sc, sl, st;
	int f = *fl;
	char buf[MBUF_SIZE];

	if(!Started(mech))
		return;
	if(target) {
		x1 = MechFX(target);
		y1 = MechFY(target);
		if(MechZ(mech) >= ATMO_Z && MechZ(target) >= ATMO_Z)
			range = range / 3;
	} else
		MapCoordToRealCoord(x, y, &x1, &y1);
	arc = InWeaponArc(mech, x1, y1);
	if(f & MECHLOSFLAG_SEEN) {
		if((sl =
			Sensor_CanSee(mech, target, &f, arc, range, mapvis,
						  maplight, cloudbase))) {
			if(sl & 1)
				f |= MECHLOSFLAG_SEESP;
			if(sl & 2)
				f |= MECHLOSFLAG_SEESS;
		}
		if(!(f & (MECHLOSFLAG_SEESP | MECHLOSFLAG_SEESS))) {
			if(MechTeam(mech) != MechTeam(target))
				MechNumSeen(mech) = MAX(0, MechNumSeen(mech) - 1);
			f &= ~MECHLOSFLAG_SEEN;
			if(!MechIsObservator(mech)
			   && (Started(target) || SeeWhenShutdown(target)
				   || MechAutoconSD(mech))
			   && (st = valid_to_notice(mech, target, wlf)) && seeanew < 3) {
				if(st & AUTOCON_WARN)
					strcpy(buf, "%cy");
				else
					buf[0] = 0;
				if(st & AUTOCON_SHORT)
					sprintf(buf + strlen(buf), "Lost: %s, %s arc.",
							GetMechToMechID_base(mech, target, wlf),
							GetArcID(mech, arc));
				else
					sprintf(buf,
							"You have lost %s from your scanners. It was last in your %s arc.",
							GetMechToMechID_base(mech, target, wlf),
							GetArcID(mech, arc));
				if(st & AUTOCON_WARN)
					strcat(buf, "%cn");
				mech_notify(mech, MECHALL, buf);
			}
			if((MechStatus(mech) & LOCK_TARGET) &&
			   target->mynum == MechTarget(mech)) {
				mech_notify(mech, MECHALL,
							"Weapon system reports the lock has been lost.");
				LoseLock(mech);
			}
#ifdef SENSOR_DEBUG
			sprintf(buf, "Notice: #%d lost #%d (Sensor: %d, Flag: %s)",
					mech->mynum, target->mynum,
					(f & (MECHLOSFLAG_SEESP | MECHLOSFLAG_SEESS)),
					my_dump_flag(f));
			SendSensor(buf);
#endif
			MechPNumSeen(mech)++;
		}
		*fl = f;
		return;
	}
	if((sc =
		Sensor_CanSee(mech, target, &f, arc, range, mapvis, maplight,
					  cloudbase))) {
		if(!seeanew) {
			MechPNumSeen(mech)++;	/* Whee, something we _could_ see */
			*fl = f;
			return;
		}
		if((sl =
			Sensor_SeesNow(mech, target, f, arc, range, mapvis, maplight))) {
			if(sc & 1)
				f |= MECHLOSFLAG_SEESP;
			if(sc & 2)
				f |= MECHLOSFLAG_SEESS;
		}
		if((f & (MECHLOSFLAG_SEESP | MECHLOSFLAG_SEESS))) {
			if(MechTeam(mech) != MechTeam(target)) {
				MechNumSeen(mech)++;
				UnZombifyMech(mech);
			}
			f |= MECHLOSFLAG_SEEN;
			*fl = f;
			if(!MechIsObservator(mech)
			   && (Started(target) || SeeWhenShutdown(target)
				   || MechAutoconSD(mech))
			   && (st = valid_to_notice(mech, target, -1)) && seeanew < 2) {
				if(st & AUTOCON_WARN)
					strcpy(buf, "%cr");
				else
					buf[0] = 0;
				if(st & AUTOCON_SHORT)
					sprintf(buf + strlen(buf), "Seen: %s, %s arc.",
							GetMechToMechID(mech, target), GetArcID(mech,
																	arc));
				else
					sprintf(buf + strlen(buf),
							"You notice %s in your %s arc.",
							GetMechToMechID(mech, target), GetArcID(mech,
																	arc));
				if(st & AUTOCON_WARN)
					strcat(buf, "%cn");
				mech_notify(mech, MECHALL, buf);
			}
			if(MechTeam(mech) != MechTeam(target))
				StopHiding(target);
#ifdef SENSOR_DEBUG
			sprintf(buf, "Notice: #%d saw #%d (Sensor: %d, Flag: %s C:%d)",
					mech->mynum, target->mynum,
					(f & (MECHLOSFLAG_SEESP | MECHLOSFLAG_SEESS)),
					my_dump_flag(f), seeanew);
			SendSensor(buf);
#endif
		} else
			MechPNumSeen(mech)++;
	}
	*fl = f;
}

void update_LOSinfo(dbref obj, MAP * map)
{
	int i, j, fl;
	int mapvis = map->mapvis;
	int maplight = map->maplight;
	MECH *mech, *target;
	float range;
	int wlf;

	/* First, for all moved mechs, calculate new LOS flags */
	for(i = 0; i < map->first_free; i++) {
		mech = getMech(map->mechsOnMap[i]);
		if(!mech)
			continue;
		if(!Started(mech))
			continue;
		for(j = i + 1; j < map->first_free; j++)
			if(map->mechflags[i] || map->mechflags[j]) {
				target = getMech(map->mechsOnMap[j]);
				if(!target)
					continue;
				range = FaMechRange(mech, target);
				if(ECMEnabled(mech) || ECCMEnabled(mech) ||
				   AngelECMEnabled(mech) || AngelECCMEnabled(mech))
					if(range < ECM_RANGE)
						checkECM(target);
				if(TAGTarget(mech) > 0)
					checkTAG(mech);
				if(MechStatus2(mech) & SLITE_ON)
					if(range < LITE_RANGE)
						cause_lite(mech, target);
				if(range > map->maxvis && MechZ(target) < 11 &&
				   MechZ(mech) < 11) {
					map->LOSinfo[i][j] = MECHLOSFLAG_BLOCK;
					continue;
				}
				wlf = !(map->LOSinfo[i][j] & MECHLOSFLAG_BLOCK) &&
					((map->LOSinfo[i][j] & MECHLOSFLAG_SEESP) ||
					 (map->LOSinfo[i][j] & MECHLOSFLAG_SEESS));
				map->LOSinfo[i][j] = fl =
					CalculateLOSFlag(mech, target, map, MechX(target),
									 MechY(target), map->LOSinfo[i]
									 [j], (float) range);
				/* Then, we update the SEES* */
#ifdef ADVANCED_LOS
				Sensor_DoWeSeeNow(mech, &map->LOSinfo[i][j], range, -1, -1,
								  target, mapvis, maplight, map->cloudbase, 0,
								  wlf);
#endif
			}
	}
	for(i = 1; i < map->first_free; i++) {
		if(map->mechsOnMap[i] > 0);
		mech = getMech(map->mechsOnMap[i]);
		if(!mech)
			continue;
		if(!Started(mech))
			continue;
		for(j = 0; j < i; j++)
			if(map->mechflags[i] || map->mechflags[j]) {
				target = getMech(map->mechsOnMap[j]);
				if(!target)
					continue;
				range = FaMechRange(mech, target);
				if(ECMEnabled(mech) || ECCMEnabled(mech) ||
				   AngelECMEnabled(mech) || AngelECCMEnabled(mech))
					if(range < ECM_RANGE)
						checkECM(target);
				if(TAGTarget(mech) > 0)
					checkTAG(mech);
				if(MechStatus2(mech) & SLITE_ON)
					if(range < LITE_RANGE)
						cause_lite(mech, target);
/* for now, commenting out this if(Started... section 
 * it was causing problems with bap. so we update los a bit more often now
 */
/*				if(Started(target))
					if(map->LOSinfo[j][i] & MECHLOSFLAG_BLOCK) {
						wlf = !(map->LOSinfo[i][j] & MECHLOSFLAG_BLOCK) &&
							((map->LOSinfo[i][j] & MECHLOSFLAG_SEESP) ||
							 (map->LOSinfo[i][j] & MECHLOSFLAG_SEESS));
						map->LOSinfo[i][j] =
							MECHLOSFLAG_BLOCK | (map->LOSinfo[i][j] &
												 (MECHLOSFLAG_SEEN |
												  MECHLOSFLAG_SEEC2));

#ifdef ADVANCED_LOS
						Sensor_DoWeSeeNow(mech, &map->LOSinfo[i][j], range,
										  -1, -1, target, mapvis, maplight,
										  map->cloudbase, 0, wlf);
#endif
						continue;
					}
*/
				if(range > map->maxvis && MechZ(target) < 11 &&
				   MechZ(mech) < 11) {
					map->LOSinfo[i][j] = MECHLOSFLAG_BLOCK;
					continue;
				}
				/* Then, we update the SEES* */
				wlf = !(map->LOSinfo[i][j] & MECHLOSFLAG_BLOCK) &&
					((map->LOSinfo[i][j] & MECHLOSFLAG_SEESP) ||
					 (map->LOSinfo[i][j] & MECHLOSFLAG_SEESS));
				map->LOSinfo[i][j] = fl =
					CalculateLOSFlag(mech, target, map, MechX(target),
									 MechY(target), map->LOSinfo[i]
									 [j], (float) range);
				Sensor_DoWeSeeNow(mech, &map->LOSinfo[i][j], range, -1, -1,
								  target, mapvis, maplight, map->cloudbase, 0,
								  wlf);
			}
	}
	for(i = 0; i < map->first_free; i++)
		map->mechflags[i] = 0;
}

void add_sensor_info(char *buf, MECH * mech, int sn, int verbose)
{
	if(!verbose)
		sprintf(buf + strlen(buf), "(R:%s)", sensors[sn].range_desc);
	else {
		sprintf(buf + strlen(buf), "\n\tRange:      %s\n\tBlocked by: %s",
				sensors[sn].range_desc, sensors[sn].block_desc);
		if(sensors[sn].special_desc)
			sprintf(buf + strlen(buf), "\n\tNotes:      %s",
					sensors[sn].special_desc);
	}
}

static char *sensor_mode_name(MECH * mech, int sn, int full, int verbose)
{
	static char buf[MBUF_SIZE];

	if(sn < 0 || sn >= NUM_SENSORS)
		return "None";

	if(sensors[sn].fullvision) {
		sprintf(buf, "%s ", sensors[sn].sensorname);
		add_sensor_info(buf, mech, sn, verbose);
	} else {
		if(full || Sees360(mech))
			sprintf(buf, "%s in 360 degree scanning mode ",
					sensors[sn].sensorname);
		else
			sprintf(buf, "%s in 120 degree scanning mode (Forward arc) ",
					sensors[sn].sensorname);
		add_sensor_info(buf, mech, sn, verbose);
	}
	return buf;
}

static void sensor_mode(MECH * mech, char *msg, dbref player, int p, int s,
						int verbose)
{
	char buf[MBUF_SIZE];
	int i;

	if(p != s) {
		for(i = 0; i < strlen(msg); i++)
			buf[i] = '-';
		buf[strlen(msg)] = 0;
		notify(player, msg);
		notify(player, buf);
		notify_printf(player, "Primary:   %s", sensor_mode_name(mech, p,
																0, verbose));
		notify_printf(player, "Secondary: %s", sensor_mode_name(mech, s,
																0, verbose));
	} else
		notify_printf(player, "%s: %s", msg, sensor_mode_name(mech, p, 1,
															  verbose));
}

static int tmp_prim;
static int tmp_sec;
static int tmp_found;

static void sensor_check(MUXEVENT * e)
{
	int d = ((int) e->data2);

	tmp_prim = d / NUM_SENSORS;
	tmp_sec = d % NUM_SENSORS;
	tmp_found = 1;
}

static char SensorInf[] = "vliesrbVLIESRB";

char *mechSensorInfo(int mode, MECH * mech, char *arg)
{
	static char buffer[5];

	tmp_found = 0;
	buffer[0] = SensorInf[(short) MechSensor(mech)[0]];
	buffer[1] = SensorInf[(short) MechSensor(mech)[1]];
	if(SensorChange(mech)) {
		muxevent_gothru_type_data(EVENT_SCHANGE, (void *) mech, sensor_check);
		if(tmp_found) {
			buffer[2] = SensorInf[tmp_prim + NUM_SENSORS];
			buffer[3] = SensorInf[tmp_sec + NUM_SENSORS];
			buffer[4] = '\0';
			return buffer;
		}
	}
	buffer[2] = '\0';
	return buffer;
}

static void show_sensor(dbref player, MECH * mech, int verbose)
{

	tmp_found = 0;
	sensor_mode(mech, "Sensors", player, MechSensor(mech)[0],
				MechSensor(mech)[1], verbose);
	if(SensorChange(mech)) {
		muxevent_gothru_type_data(EVENT_SCHANGE, (void *) mech, sensor_check);
		if(tmp_found)
			sensor_mode(mech, "Wanted", player, tmp_prim, tmp_sec, 0);
	}
}

static void mech_sensorchange_event(MUXEVENT * e)
{
	int d = (int) e->data2;
	MECH *mech = (MECH *) e->data;
	int prim = d / NUM_SENSORS;
	int sec = d % NUM_SENSORS;

	if(!Started(mech))
		return;
	MechSensor(mech)[0] = prim;
	MechSensor(mech)[1] = sec;
	mech_notify(mech, MECHALL, "As your sensors change, your lock clears.");
	MechTarget(mech) = -1;
	MarkForLOSUpdate(mech);
}

int CanChangeTo(MECH * mech, int s)
{
	MAP *map;
	int i;

	if(!(map = getMap(mech->mapindex))) {
		mech_notify(mech, MECHALL, "Where are you? ;-)");
		return 0;
	}
	/* < 0, means you you don't have the sensors if you _do_ have the bit
	   > 0, means you have the sensors if you have the bit */
	if((i = sensors[s].required_special)) {
		/* original specials struct */
		if(sensors[s].specials_set == 1) {
			if((i > 0) == ((!(MechSpecials(mech) & abs(i))) != 0)) {
				mech_printf(mech, MECHALL,
							"You lack the %s sensors!",
							sensors[s].sensorname);
				return 0;
			}
		} else {
			if((i > 0) == ((!(MechSpecials2(mech) & abs(i))) != 0)) {
				mech_printf(mech, MECHALL,
							"You lack the %s sensors!",
							sensors[s].sensorname);
				return 0;
			}
		}
	}

	if(sensors[s].min_light >= 0 && sensors[s].min_light > map->maplight) {
		if(!Destroyed(mech) && Started(mech))
			mech_printf(mech, MECHALL,
						"It's now too dark to use %s!",
						sensors[s].sensorname);
		return 0;
	}
	if(sensors[s].max_light >= 0 && sensors[s].max_light < map->maplight) {
		if(!Destroyed(mech) && Started(mech))
			mech_printf(mech, MECHALL,
						"The light's kinda too bright now to use %s!",
						sensors[s].sensorname);
		return 0;
	}

	switch (sensors[s].attributeCheck) {
	case SENSOR_ATTR_SEISMIC:
		if((MechType(mech) == CLASS_MW) ||
		   (MechType(mech) == CLASS_BSUIT) ||
		   (MechType(mech) == CLASS_VEH_NAVAL) ||
		   (MechMove(mech) == MOVE_HOVER)) {
			mech_printf(mech, MECHALL,
						"You lack the %s sensors!", sensors[s].sensorname);
			return 0;
		}

		break;
	}

	return 1;
}

void sensor_light_availability_check(MECH * mech)
{
	int p = MechSensor(mech)[0], s = MechSensor(mech)[1];
	int same = (p == s);

	if(sensors[p].min_light >= 0 || sensors[p].max_light >= 0)
		if(!CanChangeTo(mech, p))
			MechSensor(mech)[0] = 0;

	if(!same && (sensors[s].min_light >= 0 || sensors[s].max_light >= 0))
		if(!CanChangeTo(mech, s))
			MechSensor(mech)[1] = 0;
}

static int set_sensor(MECH * mech, char ps, char ss)
{
	int prim = -1, sec = -1;
	int i;

	if(!Started(mech))
		return 0;
	for(i = 0; i < NUM_SENSORS; i++) {
		if(sensors[i].matchletter[0] == ps)
			prim = i;
		if(sensors[i].matchletter[0] == ss)
			sec = i;
	}
	if(prim < 0 || sec < 0)
		return -1;
	if(prim != MechSensor(mech)[0] || sec != MechSensor(mech)[1]) {
		if(!CanChangeTo(mech, prim))
			return -1;
		if(!CanChangeTo(mech, sec))
			return -1;
		StopSensorChange(mech);
		MECHEVENT(mech, EVENT_SCHANGE, mech_sensorchange_event,
				  SCHANGE_TICK, ((prim * NUM_SENSORS) + sec));
	}
	return 0;
}

void mech_sensor(dbref player, void *data, char *buffer)
{
	MECH *mech = (MECH *) data;
	char *args[3];
	int argc;

	if(!mech)
		return;
	DOCHECK(MechType(mech) == CLASS_MW,
			"You're using your eyes, and nothing you can do changes that!");
	argc = mech_parseattributes(buffer, args, 2);
	DOCHECK(argc > 2, "Invalid number of arguments!");
	switch (argc) {
	case 0:
		show_sensor(player, mech, 0);
		break;
	case 1:
		show_sensor(player, mech, 1);
		break;
	case 2:
		DOCHECK(set_sensor(mech, toupper(args[0][0]), toupper(args[1][0]))
				< 0, "Invalid arguments!");
		show_sensor(player, mech, 0);
		break;
	}
}

void possibly_see_mech(MECH * mech)
{
	MAP *map = getMap(mech->mapindex);
	int i, j;
	MECH *seer;
	int mapvis;
	int maplight;
	float range;
	int num = mech->mapnumber;

	if(!map)
		return;
	mapvis = map->mapvis;
	maplight = map->maplight;
	/* This is quiet ; no message for noticing foe etc */
	/* Basically, this is a 'bonus' effect in addition to the movement-caused
	   effects, but just done once / move of the guy */
	for(i = 0; i < map->first_free; i++)
		if(i != num && (j = map->mechsOnMap[i]) >= 0) {
			if(!(seer = getMech(j)))
				continue;
			if(seer->mapindex != map->mynum) {
				SendError(tprintf("Mech #%d was on map #%d but with "
								  "incorrect mapindex (%d)", seer->mynum,
								  map->mynum, seer->mapindex));
				map->mechsOnMap[i] = -1;
				continue;
			}
			range = FaMechRange(seer, mech);
			map->LOSinfo[i][num] =
				CalculateLOSFlag(seer, mech, map, MechX(mech), MechY(mech),
								 map->LOSinfo[i][num], (float) range);
			/* Then, we update the SEES* */
			/* seeanew used to be 2 ; I want them to know they notice
			   it first not to bug me 'bout it, though */
#ifdef ADVANCED_LOS
			Sensor_DoWeSeeNow(seer, &map->LOSinfo[i][num], range, -1, -1,
							  mech, mapvis, maplight, map->cloudbase, 2, 0);
#endif
		}
}

static void mech_unblind_event(MUXEVENT * e)
{
	MECH *m = (MECH *) e->data;

	MechStatus(m) &= ~BLINDED;
	if(!Uncon(m))
		mech_notify(m, MECHALL, "Your sight recovers.");
}

void ScrambleInfraAndLiteAmp(MECH * mech, int time, int chance,
							 char *inframsg, char *liteampmsg)
{
	MAP *mech_map = getMap(mech->mapindex);
	int i;
	MECH *tempMech;

	possibly_see_mech(mech);
	for(i = 0; i < mech_map->first_free; i++)
		if(mech_map->mechsOnMap[i] != -1 &&
		   mech_map->mechsOnMap[i] != mech->mynum)
			if((tempMech = getMech(mech_map->mechsOnMap[i])))
				if(InLineOfSight(tempMech, mech, MechX(mech), MechY(mech),
								 FaMechRange(tempMech, mech))) {
					if(Blinded(tempMech) || Uncon(tempMech))
						continue;
					if(sensors[(int)
							   MechSensor(tempMech)[0]].matchletter[0] == 'I'
					   || sensors[(int)
								  MechSensor(tempMech)
								  [0]].matchletter[1] == 'I') {
						if(chance)
							if(Number(1, 100) > chance)
								continue;
						/* Infra effect */
						mech_notify(tempMech, MECHALL, inframsg);
					} else if(sensors[(int)
									  MechSensor(tempMech)[0]].matchletter[0]
							  == 'L' || sensors[(int)
												MechSensor(tempMech)
												[0]].matchletter[1]
							  == 'L') {
						if(chance)
							if(Number(1, 100) > chance)
								continue;
						/* Liteamp effect */
						mech_notify(tempMech, MECHALL, liteampmsg);
					} else
						continue;
					MechStatus(tempMech) |= BLINDED;
					MECHEVENT(tempMech, EVENT_BLINDREC, mech_unblind_event,
							  time, 0);
				}
}
