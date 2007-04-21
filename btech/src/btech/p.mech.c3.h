
/* p.mech.c3.h */

#ifndef _P_MECH_C3_H
#define _P_MECH_C3_H

int getC3MasterSize(MECH * mech);
int isPartOfWorkingC3Master(MECH * mech, int section, int slot);
int countWorkingC3MastersOnMech(MECH * mech);
int countTotalC3MastersOnMech(MECH * mech);
int countMaxC3Units(MECH * mech, dbref * myTempNetwork,
    int tempNetworkSize, MECH * targMech);
int trimC3Network(MECH * mech, dbref * myTempNetwork, int tempNetworkSize);
int getFreeC3NetworkPos(MECH * mech, MECH * mechToAdd);
void replicateC3Network(MECH * mechSrc, MECH * mechDest);
void addMechToC3Network(MECH * mech, MECH * mechToAdd);
void clearMechFromC3Network(dbref refToClear, MECH * mech);
void clearC3Network(MECH * mech, int tClearFromOthers);
void validateC3Network(MECH * mech);
void mech_c3_join_leave(dbref player, void *data, char *buffer);
void mech_c3_message(dbref player, MECH * mech, char *buffer);
void mech_c3_targets(dbref player, MECH * mech, char *buffer);
void mech_c3_network(dbref player, MECH * mech, char *buffer);

#endif				/* _P_MECH_C3_H */
