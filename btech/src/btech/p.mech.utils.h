#include "config.h"

#ifndef _P_MECH_UTILS_H
#define _P_MECH_UTILS_H

/* mech.utils.c */
/* Misc Functions */
const char *mechtypename(MECH * foo);
int MNumber(MECH * mech, int low, int high);
char *MechIDS(MECH * mech, int islower);
char *MyToUpper(char *string);
void MarkForLOSUpdate(MECH * mech);

/* Self-inflicted kill types. (But flood might be accidental/intentional.) */
#define KILL_TYPE_SELF_DESTRUCT "SELF-DESTRUCT"
#define KILL_TYPE_EJECT 	"EJECT"
#define KILL_TYPE_FLOOD 	"FLOOD" 	/* includes vacuum */
/* Accidental kill types. (But ice/heat might be intentional.) */
#define KILL_TYPE_ICE 		"FLOOD-ICE"
#define KILL_TYPE_HEAT 		"HEAT"
/* Intentional kill types.  */
#define KILL_TYPE_NORMAL 	"DESTROYED" 	/* all other kills; includes carrier destruction */
#define KILL_TYPE_PILOT 	"PILOT" 	/* Only happens on vehicles. Mainly crew death */
#define KILL_TYPE_MWDAMAGE 	"MWDAMAGE" 	/* Failed MW Conc rolls once too many or one too many head hit */
#define KILL_TYPE_BEHEADED 	"BEHEADED"
#define KILL_TYPE_XLENGINE 	"XLENGINE"
#define KILL_TYPE_FUELTANK 	"FUELTANK" 	/* Fuel Tank Crit Death */
#define KILL_TYPE_COCKPIT 	"COCKPIT" 	/* Alot different than Pilot death. Cockpit Crit death */
#define KILL_TYPE_POWERPLANT 	"POWERPLANT" 	/* Vehicle powerplant crit death */
#define KILL_TYPE_SCHARGE 	"SCHARGE"  	/* Super Charger overload */
#define KILL_TYPE_TRANSPORT 	"TRANSPORT" 	/* The Transport containing the unit died */
#define KILL_TYPE_ENGINE 	"ENGINE" 	/* Unit engined, standard fusion death */
#define KILL_TYPE_HEAD_TARGET	"HEAD-TARGET"	/* Head was taken off, using TARGET */

void ChannelEmitKill(MECH * mech, MECH * attacker, const char *reason);

MAP *ValidMap(dbref player, dbref map);
dbref FindMechOnMap(MAP *map, char *mechid);
MECH *find_mech_in_hex(MECH * mech, MAP * mech_map, int x, int y, int needlos);
dbref FindTargetDBREFFromMapNumber(MECH * mech, char *mapnum);

/* Map Math */
int AcceptableDegree(int d);
void FindXY(float x0, float y0, int bearing, float range, float *x1, float *y1);
float FindRange(float x0, float y0, float z0, float x1, float y1, float z1);
int MyHexDist(int x1, int y1, int x2, int y2, int tc);
float FindXYRange(float x0, float y0, float x1, float y1);
float FindHexRange(float x0, float y0, float x1, float y1);
void RealCoordToMapCoord(short *hex_x, short *hex_y, float cart_x, float cart_y);
void MapCoordToRealCoord(int hex_x, int hex_y, float *cart_x, float *cart_y);
void visit_neighbor_hexes(MAP *map, int x, int y, void (*callback)(MAP *, int, int));
void FindComponents(float magnitude, int degrees, float *x, float *y);
void CheckEdgeOfMap(MECH * mech);
int FindZBearing(float x0, float y0, float z0, float x1, float y1, float z1);
int FindBearing(float x0, float y0, float x1, float y1);
int InWeaponArc(MECH * mech, float x, float y);
int IsInWeaponArc(MECH * mech, float x, float y, int section, int critical);
void navigate_sketch_mechs(MECH *mech, MAP *map, int x, int y,
   char buff[NAVIGATE_LINES][MBUF_SIZE]);
int FindTargetXY(MECH * mech, float *x, float *y, float *z);

/* Skill lookups */
char *FindGunnerySkillName(MECH * mech, int weapindx);
char *FindPilotingSkillName(MECH * mech);
int FindPilotPiloting(MECH * mech);
int FindSPilotPiloting(MECH * mech);
int FindPilotSpotting(MECH * mech);
int FindPilotArtyGun(MECH * mech);
int FindPilotGunnery(MECH * mech, int weapindx);
char *FindTechSkillName(MECH * mech);
int FindTechSkill(dbref player, MECH * mech);

/* Skill rolls */
int MadePilotSkillRoll(MECH * mech, int mods);
int MechPilotSkillRoll_BTH(MECH *mech, int mods);
int MadePilotSkillRoll_Advanced(MECH * mech, int mods, int succeedWhenFallen);
int MadePilotSkillRoll_NoXP(MECH * mech, int mods, int succeedWhenFallen);
int Roll(void);

/* Section/Crit Functions */
int CritsInLoc(MECH * mech, int index);
int SectHasBusyWeap(MECH * mech, int sect);
int FindWeapons_Advanced(MECH * mech, int index, unsigned char *weaparray,
    unsigned char *weapdataarray, int *critical, int whine);
int FindAmmunition(MECH * mech, unsigned char *weaparray,
    unsigned short *ammoarray, unsigned short *ammomaxarray,
    unsigned int *modearray, int returnall);
int FindLegHeatSinks(MECH * mech);
int FindWeaponNumberOnMech_Advanced(MECH * mech, int number, int *section,
    int *crit, int sight);
int FindWeaponNumberOnMech(MECH * mech, int number, int *section, int *crit);
int FindWeaponFromIndex(MECH * mech, int weapindx, int *section, int *crit);
int FindWeaponIndex(MECH * mech, int number);
int findAmmoInSection(MECH * mech, int section, int type, int nogof, int gof);
int FullAmmo(MECH * mech, int loc, int pos);
int FindAmmoForWeapon_sub(MECH * mech, int weapSection, int weapCritical,
    int weapindx, int start, int *section, int *critical, int nogof, int gof);
int FindAmmoForWeapon(MECH * mech, int weapindx, int start, int *section,
    int *critical);
int CountAmmoForWeapon(MECH * mech, int weapindx);
int FindArtemisForWeapon(MECH * mech, int section, int critical);
int ReverseSplitCritLoc(MECH * mech, int sect, int crit);
int FindSplitCrits(MECH * mech, int sect, int type, int crit);
int GetSplitData(MECH * mech, int sect, int data, int *ssect, int *scrit, int *stype);
int FindDestructiveAmmo(MECH * mech, int *section, int *critical);
int FindInfernoAmmo(MECH * mech, int *section, int *critical);
int FindRoundsForWeapon(MECH * mech, int weapindx);
int HeatFactor(MECH * mech);
int WeaponIsNonfunctional(MECH * mech, int section, int crit, int numcrits);
char **ProperSectionStringFromType(int type, int mtype);
void ArmorStringFromIndex(int index, char *buffer, char type, char mtype);
int GetWeaponCrits(MECH * mech, int weapindx);
int listmatch(char **foo, char *mat);
int GetPartWeight(int part);
void multi_weap_sel(MECH * mech, dbref player, char *buffer, int bitbybit,
    int (*foo) (MECH *, dbref, int, int));
int MechNumHeatsinksInEngine(MECH * mech);

/* Tech/Repair functions */
void do_sub_magic(MECH * mech, int loud);
void do_magic(MECH * mech);
void do_fixextra(MECH * mech);
void mech_RepairPart(MECH * mech, int loc, int pos);
int no_locations_destroyed(MECH * mech);
void mech_ReAttach(MECH * mech, int loc);
void mech_ReplaceSuit(MECH * mech, int loc);
void mech_ReSeal(MECH * mech, int loc);
void mech_Detach(MECH * mech, int loc);
void mech_FillPartAmmo(MECH * mech, int loc, int pos);

int CountDestroyedLegs(MECH * objMech);
int IsLegDestroyed(MECH * objMech, int wLoc);
int IsMechLegLess(MECH * objMech);
int FindFirstWeaponCrit(MECH * objMech, int wLoc, int wSlot, int wStartSlot, 
   int wCritType, int wMaxCrits);
int checkAllSections(MECH * mech, int specialToFind);
int checkSectionForSpecial(MECH * mech, int specialToFind, int wSec);
int getRemainingInternalPercent(MECH * mech);
int getRemainingArmorPercent(MECH * mech);
int FindObj(MECH * mech, int loc, int type);
int FindObjWithDest(MECH * mech, int loc, int type);
int FindAndCheckAmmo(MECH * mech, int weapindx, int section, int critical,
   int *ammoLoc, int *ammoCrit, int *ammoLoc1, int *ammoCrit1,
   int *wGattlingShots);

#ifdef BT_ADVANCED_ECON
unsigned long long int GetPartCost(int p);
void SetPartCost(int p, unsigned long long int cost);
void CalcFasaCost_DoLegMath(MECH * mech, int loc, float * total);
void CalcFasaCost_DoArmMath(MECH * mech, int loc, float * total);
void CalcFasaCost_AddPrice(float *total, char * desc, float value);
void Calc_AddOffBV(float *offbv, char * desc, float value);
void Calc_AddDefBV(float *defbv, char * desc, float value);
void Calc_SubDefBV(float *defbv, char * desc, float value);
float Calculate_Defensive_BV(MECH *mech, int gunstat, int pilstat);
unsigned long long int CalcFasaCost(MECH * mech);
char *UnitPartsList(MECH * mech, int mode);
int mech_armorpoints(MECH * mech);
int mech_intpoints(MECH * mech);
#endif
#ifdef BT_CALCULATE_BV
int FindAverageGunnery(MECH * mech);
int CalculateBV(MECH *mech, int gunstat, int pilstat);
#endif
int MechFullNoRecycle(MECH * mech, int num);
#ifdef BT_COMPLEXREPAIRS
int GetPartMod(MECH * mech, int t);
int ProperArmor(MECH* mech);
int ProperInternal(MECH * mech);
int alias_part(MECH * mech, int t, int loc);
int ProperMyomer(MECH * mech);
#endif
#endif				/* _P_MECH_UTILS_H */
