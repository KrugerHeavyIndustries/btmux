
/*
   p.mech.utils.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:33:03 CET 1999 from mech.utils.c */

#include "config.h"

#ifndef _P_MECH_UTILS_H
#define _P_MECH_UTILS_H

/* mech.utils.c */
const char *mechtypename(MECH * foo);
int MNumber(MECH * mech, int low, int high);
char *MechIDS(MECH * mech, int islower);
char *MyToUpper(char *string);
int CritsInLoc(MECH * mech, int index);
int SectHasBusyWeap(MECH * mech, int sect);
MAP *ValidMap(dbref player, dbref map);
dbref FindMechOnMap(MAP *map, char *mechid);
dbref FindTargetDBREFFromMapNumber(MECH * mech, char *mapnum);
void FindComponents(float magnitude, int degrees, float *x, float *y);
void CheckEdgeOfMap(MECH * mech);
int FindZBearing(float x0, float y0, float z0, float x1, float y1, float z1);
int FindBearing(float x0, float y0, float x1, float y1);
int InWeaponArc(MECH * mech, float x, float y);
char *FindGunnerySkillName(MECH * mech, int weapindx);
char *FindPilotingSkillName(MECH * mech);
int FindPilotPiloting(MECH * mech);
int FindSPilotPiloting(MECH * mech);
int FindPilotSpotting(MECH * mech);
int FindPilotArtyGun(MECH * mech);
int FindPilotGunnery(MECH * mech, int weapindx);
char *FindTechSkillName(MECH * mech);
int FindTechSkill(dbref player, MECH * mech);
int MadePilotSkillRoll(MECH * mech, int mods);
int MechPilotSkillRoll_BTH(MECH *mech, int mods);
int MadePilotSkillRoll_Advanced(MECH * mech, int mods,
    int succeedWhenFallen);
int MadePilotSkillRoll_XP(MECH * mech, int mods,
    int succeedWhenFallen);
void FindXY(float x0, float y0, int bearing, float range, float *x1,
    float *y1);
float FindRange(float x0, float y0, float z0, float x1, float y1,
    float z1);
float FindXYRange(float x0, float y0, float x1, float y1);
float FindHexRange(float x0, float y0, float x1, float y1);
void RealCoordToMapCoord(short *hex_x, short *hex_y, float cart_x,
    float cart_y);
void MapCoordToRealCoord(int hex_x, int hex_y, float *cart_x,
    float *cart_y);
void navigate_sketch_mechs(MECH *mech, MAP *map, int x, int y,
    char buff[NAVIGATE_LINES][MBUF_SIZE]);
int FindTargetXY(MECH * mech, float *x, float *y, float *z);
int FindWeapons_Advanced(MECH * mech, int index, unsigned char *weaparray,
    unsigned char *weapdataarray, int *critical, int whine);
int FindAmmunition(MECH * mech, unsigned char *weaparray,
    unsigned short *ammoarray, unsigned short *ammomaxarray,
    unsigned int *modearray, int returnall);
int FindLegHeatSinks(MECH * mech);
int FindWeaponNumberOnMech_Advanced(MECH * mech, int number, int *section,
    int *crit, int sight);
int FindWeaponNumberOnMech(MECH * mech, int number, int *section,
    int *crit);
int FindWeaponFromIndex(MECH * mech, int weapindx, int *section,
    int *crit);
int FindWeaponIndex(MECH * mech, int number);
int findAmmoInSection(MECH * mech, int section, int type, int nogof,
    int gof);
int FullAmmo(MECH * mech, int loc, int pos);
int FindAmmoForWeapon_sub(MECH * mech, int weapSection, int weapCritical,
    int weapindx, int start, int *section, int *critical, int nogof,
    int gof);
int FindAmmoForWeapon(MECH * mech, int weapindx, int start, int *section,
    int *critical);
int CountAmmoForWeapon(MECH * mech, int weapindx);
int FindArtemisForWeapon(MECH * mech, int section, int critical);
int FindDestructiveAmmo(MECH * mech, int *section, int *critical);
int FindInfernoAmmo(MECH * mech, int *section, int *critical);
int FindRoundsForWeapon(MECH * mech, int weapindx);
char **ProperSectionStringFromType(int type, int mtype);
void ArmorStringFromIndex(int index, char *buffer, char type, char mtype);
int IsInWeaponArc(MECH * mech, float x, float y, int section,
    int critical);
int GetWeaponCrits(MECH * mech, int weapindx);
int listmatch(char **foo, char *mat);
void do_sub_magic(MECH * mech, int loud);
void do_magic(MECH * mech);
void mech_RepairPart(MECH * mech, int loc, int pos);
int no_locations_destroyed(MECH * mech);
void mech_ReAttach(MECH * mech, int loc);
void mech_ReplaceSuit(MECH * mech, int loc);
void mech_ReSeal(MECH * mech, int loc);
void mech_Detach(MECH * mech, int loc);
void mech_FillPartAmmo(MECH * mech, int loc, int pos);
int AcceptableDegree(int d);
void MarkForLOSUpdate(MECH * mech);
void multi_weap_sel(MECH * mech, dbref player, char *buffer, int bitbybit,
    int (*foo) (MECH *, dbref, int, int));
int Roll(void);
int MyHexDist(int x1, int y1, int x2, int y2, int tc);
int CountDestroyedLegs(MECH * objMech);
int IsLegDestroyed(MECH * objMech, int wLoc);
int IsMechLegLess(MECH * objMech);
int FindFirstWeaponCrit(MECH * objMech, int wLoc, int wSlot,
    int wStartSlot, int wCritType, int wMaxCrits);
int checkAllSections(MECH * mech, int specialToFind);
int checkSectionForSpecial(MECH * mech, int specialToFind, int wSec);

int getRemainingInternalPercent(MECH * mech);
int getRemainingArmorPercent(MECH * mech);
int FindObj(MECH * mech, int loc, int type);
int FindObjWithDest(MECH * mech, int loc, int type);
MECH *find_mech_in_hex(MECH * mech, MAP * mech_map, int x, int y,
    int needlos);
int FindAndCheckAmmo(MECH * mech, int weapindx, int section, int critical,
    int *ammoLoc, int *ammoCrit, int *ammoLoc1, int *ammoCrit1,
    int *wGattlingShots);
void ChannelEmitKill(MECH * mech, MECH * attacker);
void visit_neighbor_hexes(MAP *map, int x, int y,
    void (*callback)(MAP *, int, int));
int GetPartWeight(int part);
#ifdef BT_ADVANCED_ECON
unsigned long long int GetPartCost(int p);
void SetPartCost(int p, unsigned long long int cost);
unsigned long long int CalcFasaCost(MECH * mech);
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
int HeatFactor(MECH * mech);
int WeaponIsNonfunctional(MECH * mech, int section, int crit, int numcrits);
#endif				/* _P_MECH_UTILS_H */
