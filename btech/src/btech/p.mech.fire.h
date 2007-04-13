
/*
   p.mech.fire.h
*/

void inferno_burn(MECH * mech, int time);
void vehicle_start_burn(MECH * objMech, MECH * objAttacker);
void checkVehicleInFire(MECH * objMech, int fromHexFire);
void vehicle_extinquish_fire_event(MUXEVENT * e);
void vehicle_extinquish_fire(dbref player, MECH * mech, char *buffer);
void water_extinguish_inferno(MECH * mech);
