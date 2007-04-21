
/*
	 p.crit.h

	 Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
	 Protomaker is actually only a wrapper script for cproto, but well.. I like
	 fancy headers and stuff :)
	 */

/* Generated at Wed Feb 17 23:36:30 CET 1999 from crit.c */

#ifndef _P_CRIT_H
#define _P_CRIT_H

/* crit.c */
void correct_speed(MECH * mech);
void explode_unit(MECH * wounded, MECH * attacker);
int handleWeaponCrit(MECH * attacker, MECH * wounded, int hitloc,
    int critHit, int critType, int LOS);
void HandleVTOLCrit(MECH * wounded, MECH * attacker, int LOS, int hitloc,
    int num);
void DestroyMainWeapon(MECH * mech);
void JamMainWeapon(MECH * mech);
void pickRandomWeapon(MECH * objMech, int wLoc, int *critNum,
    int wIgnoreJams);
void limitSpeedToCruise(MECH * objMech);
void DoVehicleStablizerCrit(MECH * objMech, int wLoc);
void DoTurretJamCrit(MECH * objMech);
void DoWeaponJamCrit(MECH * objMech, int wLoc);
void DoTurretLockCrit(MECH * objMech);
void DoWeaponDestroyedCrit(MECH * objAttacker, MECH * objMech, int wLoc,
    int LOS);
void DoTurretBlownOffCrit(MECH * objMech, MECH * objAttacker, int LOS);
void DoAmmunitionCrit(MECH * objMech, MECH * objAttacker, int wLoc,
    int LOS);
void DoCargoInfantryCrit(MECH * objMech, int wLoc);
void DoVehicleEngineHit(MECH * objMech, MECH * objAttacker);
void DoVehicleFuelTankCrit(MECH * objMech, MECH * objAttacker);
void DoVehicleCrewStunnedCrit(MECH * objMech);
void DoVehicleDriverCrit(MECH * objMech);
void DoVehicleSensorCrit(MECH * objMech);
void DoVehicleCommanderHit(MECH * objMech);
void DoVehicleCrewKilledCrit(MECH * objMech, MECH * objAttacker);
void DoVTOLCoPilotCrit(MECH * objMech);
void DoVTOLPilotHit(MECH * objMech);
void DoVTOLRotorDamagedCrit(MECH * objMech);
void DoVTOLTailRotorDamagedCrit(MECH * objMech);
void DoVTOLRotorDestroyedCrit(MECH * objMech, MECH * objAttacker, int LOS);
void StartVTOLCrash(MECH * objMech);
void HandleAdvFasaVehicleCrit(MECH * wounded, MECH * attacker, int LOS,
    int hitloc, int num);
void HandleFasaVehicleCrit(MECH * wounded, MECH * attacker, int LOS,
    int hitloc, int num);
void HandleVehicleCrit(MECH * wounded, MECH * attacker, int LOS,
    int hitloc, int num);
int HandleMechCrit(MECH * wounded, MECH * attacker, int LOS, int hitloc,
    int critHit, int critType, int critData);
void HandleCritical(MECH * wounded, MECH * attacker, int LOS, int hitloc,
    int num);
void NormalizeArmActuatorCrits(MECH * objMech, int wLoc, int wCritType);
void NormalizeLegActuatorCrits(MECH * objMech, int wLoc, int wCritType);
void NormalizeLocActuatorCrits(MECH * objMech, int wLoc);
void NormalizeAllActuatorCrits(MECH * objMech);

#endif				/* _P_CRIT_H */
