
/*
   p.mech.tag.h
*/

/* static void tag_recycle_event(MUXEVENT * e); */
void mech_tag(dbref player, void *data, char *buffer);
int isTAGDestroyed(MECH * mech);
void stopTAG(MECH * mech);
void checkTAG(MECH * mech);
