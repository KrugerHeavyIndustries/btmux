
/*
   p.mech.c3i.h
*/

#ifndef _P_MECH_C3I_H
#define _P_MECH_C3I_H

void mech_c3i_join_leave(dbref player, void *data, char *buffer);
void mech_c3i_message(dbref player, MECH * mech, char *buffer);
void mech_c3i_targets(dbref player, MECH * mech, char *buffer);
void mech_c3i_network(dbref player, MECH * mech, char *buffer);
int getFreeC3iNetworkPos(MECH * mech, MECH * mechToAdd);
void replicateC3iNetwork(MECH * mechSrc, MECH * mechDest);
void validateC3iNetwork(MECH * mech);
MECH *getOtherC3iMech(MECH * mech, int wIdx, int tCheckECM,
    int tCheckStarted, int tCheckUncon);
void clearC3iNetwork(MECH * mech, int tClearFromOthers);
void clearMechFromC3iNetwork(dbref refToClear, MECH * mech);
void addMechToC3iNetwork(MECH * mech, MECH * mechToAdd);

#endif
