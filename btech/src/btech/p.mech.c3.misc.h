
/*
   p.mech.c3.misc.h
*/

#ifndef _P_MECH_C3_MISC_H
#define _P_MECH_C3_MISC_H

MECH *getMechInTempNetwork(int wIdx, dbref * myNetwork, int networkSize);
MECH *getOtherMechInNetwork(MECH * mech, int wIdx, int tCheckECM,
    int tCheckStarted, int tCheckUncon, int tIsC3);
void buildTempNetwork(MECH * mech, dbref * myNetwork, int *networkSize,
    int tCheckECM, int tCheckStarted, int tCheckUncon, int tIsC3);
void sendNetworkMessage(dbref player, MECH * mech, char *msg, int tIsC3);
void showNetworkTargets(dbref player, MECH * mech, int tIsC3);
void showNetworkData(dbref player, MECH * mech, int tIsC3);
int mechSeenByNetwork(MECH * mech, MECH * mechTarget, int isC3);
float findC3Range(MECH * mech, MECH * mechTarget, float realRange,
    dbref * c3Ref, int tIsC3);
float findC3RangeWithNetwork(MECH * mech, MECH * mechTarget,
    float realRange, dbref * myNetwork, int networkSize, dbref * c3Ref);
void debugC3(char *msg);

#endif
