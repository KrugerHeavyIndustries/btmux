
/*
 * $Id: mech.tech.h,v 1.5 2005/06/24 04:39:08 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Fri Aug 30 15:22:08 1996 fingon
 * Last modified: Sat Jun  6 20:49:50 1998 fingon
 *
 */

#include "config.h"

#ifndef MECH_TECH_H
#define MECH_TECH_H

#include "mech.events.h"

/* In minutes */
#define MAX_TECHTIME 600
#if 1
#define TECH_TICK     60
#define TECH_UNIT "minute"
#else
#define TECH_TICK      1
#define TECH_UNIT "second"
#endif

/* Tech skill modifiers ; + = bad, - = good */
#define PARTTYPE_DIFFICULTY(a) (1)
#define WEAPTYPE_DIFFICULTY(a) ((int) (sqrt(MechWeapons[Weapon2I(a)].criticals)*1.5-1.1))
#define REPAIR_DIFFICULTY     0
#define REPLACE_DIFFICULTY     1
#define RELOAD_DIFFICULTY      1
#define FIXARMOR_DIFFICULTY    1
#define FIXINTERNAL_DIFFICULTY 2
#define REATTACH_DIFFICULTY    3
#define REMOVEG_DIFFICULTY     1
#define REMOVEP_DIFFICULTY     0
#define REMOVES_DIFFICULTY     2
#define RESEAL_DIFFICULTY    0	/* Added 8/4/99. Kipsta. */
#define REPLACESUIT_DIFFICULTY    3
#define ENHCRIT_DIFFICULTY    0

/* Times are in minutes */
#define MOUNT_BOMB_TIME  5
#define UMOUNT_BOMB_TIME 5
#define REPLACEGUN_TIME  60
#define REPLACEPART_TIME 45
#define REPAIRGUN_TIME   20
#define REPAIRENHCRIT_TIME  15
#define REPAIRPART_TIME  15
#define RELOAD_TIME      10
#define FIXARMOR_TIME    3
#define FIXINTERNAL_TIME 9
#define REATTACH_TIME    240
#define REMOVEP_TIME     40
#define REMOVEG_TIME     40
#define REMOVES_TIME     120
#define RESEAL_TIME    60	/* Added 8/4/99. Kipsta. */
#define REPLACESUIT_TIME    120

#define TECHCOMMANDH(a) \
   void a (dbref player, void * data, char * buffer)
#define TECHCOMMANDB \
 MECH *mech = (MECH *) data; \
 int loc, part, t, full, now, from, to, change, mod=2, isds=0; \
 char *c;


#define TECHCOMMANDC \
DOCHECK(!(Tech(player)),"Insufficient clearance to access the command."); \
DOCHECK(!mech, "Error has occured in techcommand ; please contact a wiz"); \
isds = DropShip(MechType(mech)); \
DOCHECK(Starting(mech) && !Wiz(player), "The mech's starting up! Please stop the sequence first."); \
DOCHECK(Started(mech) && !Wiz(player), "The mech's started up ; please shut it down first."); \
DOCHECK(!isds && !MechStall(mech) && !Wiz(player), "The 'mech isn't in a repair stall!");

#define TECHCOMMANDD \
DOCHECK(!(Tech(player)),"Insufficient clearance to access the command."); \
DOCHECK(!mech, "Error has occured in techcommand ; please contact a wiz"); \
isds = DropShip(MechType(mech)); \
DOCHECK(Starting(mech) && !Wiz(player), "The mech's starting up! Please stop the sequence first."); \
DOCHECK(Started(mech) && !Wiz(player), "The mech's started up ; please shut it down first."); \
DOCHECK(mudconf.btech_limitedrepairs && !isds && !MechStall(mech) && !Wiz(player), "The 'mech isn't in a repair stall!");

#define ETECHCOMMAND(a) \
 void a (dbref player, void *data, char *buffer)

#define LOCMAX 16
#define POSMAX 16
#define EXTMAX 256
#define PLAYERPOS (LOCMAX*POSMAX*EXTMAX)

#define TECHEVENT(a) \
   void a (MUXEVENT *e) \
     { MECH *mech = (MECH *) e->data;  \
       int earg = (int) (e->data2) % PLAYERPOS;

#define ETECHEVENT(a) \
   extern void a (MUXEVENT *e)

#define START(a) notify(player, a)
#ifndef BT_FREETECHTIME
#define FIXEVENT(time,d1,d2,fu,type) \
     muxevent_add(MAX(1, time), 0, type, fu, (void *) d1, (void *) ((d2) + player * PLAYERPOS))
#else
#define FIXEVENT(time,d1,d2,fu,type) \
    muxevent_add((mudconf.btech_freetechtime ? 2 : MAX(2, time)), 0, type, fu, (void *) d1, (void *) ((d2) + player * PLAYERPOS))
#endif
#define REPAIREVENT(time,d1,d2,fu,type) \
     FIXEVENT((time)*TECH_TICK,d1,d2,fu,type)
#define STARTREPAIR(time,d1,d2,fu,type) \
     FIXEVENT(tech_addtechtime(player, (time * mod) / 2),d1,d2,fu,type)
#define STARTIREPAIR(time,d1,d2,fu,type,amount) \
     FIXEVENT((tech_addtechtime(player, (time * mod) / 2) - (amount > 0 ? TECH_TICK * (time * (amount - 1) / (amount)) : 0)), d1, d2, fu, type)
#define FAKEREPAIR(time,type,d1,d2) \
     FIXEVENT(tech_addtechtime(player, (time * mod) / 2),d1,d2,very_fake_func,type)

/* replace gun/part, repair gun/part (loc/pos) */
#define DOTECH_LOCPOS(diff,flunkfunc,succfunc,resourcefunc,time,d1,d2,fu,type,msg,isgun)\
   if (resourcefunc(player,mech,loc,part)>=0) { START(msg); \
   if ((!isgun && tech_roll(player, mech, diff) < 0) || \
       (isgun && tech_weapon_roll(player, mech, diff) < 0)) { mod = 3;  \
   if (flunkfunc(player,mech,loc,part)<0) { FAKEREPAIR(time,type,d1,d2); return;}} \
    else \
     { if (succfunc(player,mech,loc,part)<0) return; } \
     STARTREPAIR(time,d1,d2,fu,type); }

/* reload (loc/pos/amount) */
#define DOTECH_LOCPOS_VAL(diff,flunkfunc,succfunc,resourcefunc,amo,time,d1,d2,fu,type,msg)\
   if (resourcefunc(player,mech,loc,part,amo)<0) return; \
   START(msg); \
   if (tech_roll(player, mech, diff) < 0) { mod = 3; \
   if (flunkfunc(player,mech,loc,part,amo)<0) {FAKEREPAIR(time,type,d1,d2);return;}}\
     else \
   { if (succfunc(player,mech,loc,part,amo)<0) return; } \
   STARTREPAIR(time,d1,d2,fu,type)


/* fixarmor/internal (loc/amount) */
#define DOTECH_LOC_VAL_S(diff,flunkfunc,succfunc,resourcefunc,amo,time,type,d1,d2,msg) \
   if (resourcefunc(player,mech,loc,amo)<0) return; \
   START(msg); \
   if (tech_roll(player, mech, diff) < 0) { mod = 3; \
   if (flunkfunc(player,mech,loc,amo)<0) { FAKEREPAIR(time,type,d1,d2); return; }} \
   else \
     { if (succfunc(player,mech,loc,amo)<0) return; }

#define DOTECH_LOC_VAL(diff,flunkfunc,succfunc,resourcefunc,amo,time,d1,d2,fu,type,msg) \
   if (resourcefunc(player,mech,loc,amo)<0) return; \
   START(msg); \
   if (tech_roll(player, mech, diff) < 0) { mod = 3; \
   if (flunkfunc(player,mech,loc,amo)<0) { FAKEREPAIR(time,type,d1,d2); return; }} \
   else \
     { if (succfunc(player,mech,loc,amo)<0) return; } \
   STARTREPAIR(time,d1,d2,fu,type)

/* reattach and reseal (loc) */
#define DOTECH_LOC(diff,flunkfunc,succfunc,resourcefunc,time,d1,d2,fu,type,msg) \
   if (resourcefunc(player,mech,loc)<0) return; \
   START(msg); \
   if (tech_roll(player, mech, diff) < 0) { mod = 3; \
   if (flunkfunc(player,mech,loc)<0) { FAKEREPAIR(time,type,d1,d2);return; }} \
    else \
   { if (succfunc(player,mech,loc)<0) return; } \
   STARTREPAIR(time,d1,d2,fu,type)

#define TFUNC_LOCPOS_VAL(name) \
int name (dbref player,MECH *mech,int loc,int part, int * val)
#define TFUNC_LOC_VAL(name) \
int name (dbref player, MECH *mech, int loc, int * val)
#define TFUNC_LOCPOS(name) \
int name (dbref player, MECH *mech, int loc, int part)
#define TFUNC_LOC(name) \
int name (dbref player, MECH *mech, int loc)
#define TFUNC_LOC_RESEAL(name) int name (dbref player, MECH *mech, int loc)
#define NFUNC(a) a { return 0; }



ETECHCOMMAND(tech_removegun);
ETECHCOMMAND(tech_removepart);
ETECHCOMMAND(tech_removesection);
ETECHCOMMAND(tech_replacegun);
ETECHCOMMAND(tech_repairgun);
ETECHCOMMAND(tech_fixenhcrit);
ETECHCOMMAND(tech_replacepart);
ETECHCOMMAND(tech_repairpart);
ETECHCOMMAND(tech_toggletype);
ETECHCOMMAND(tech_reload);
ETECHCOMMAND(tech_unload);
ETECHCOMMAND(tech_fixarmor);
ETECHCOMMAND(tech_fixinternal);
ETECHCOMMAND(tech_reattach);
ETECHCOMMAND(tech_checkstatus);
ETECHCOMMAND(tech_reseal);
ETECHCOMMAND(tech_replacesuit);
ECMD(show_mechs_damage);
ECMD(tech_fix);

#define PACK_LOCPOS(loc,pos)          ((loc) + (pos)*LOCMAX)
#define PACK_LOCPOS_E(loc,pos,extra)  ((loc) + (pos)*LOCMAX + (extra)*LOCMAX*POSMAX)

#define UNPACK_LOCPOS(var,loc,pos)  loc = (var % LOCMAX);pos = (var / LOCMAX) % POSMAX
#define UNPACK_LOCPOS_E(var,loc,pos,extra) UNPACK_LOCPOS(var,loc,pos);extra = var / (LOCMAX * POSMAX)

#ifndef BT_COMPLEXREPAIRS
#define ProperArmor(mech) \
(Cargo(\
       (MechSpecials(mech) & FF_TECH) ? FF_ARMOR : \
       (MechSpecials(mech) & HARDA_TECH) ? HD_ARMOR : \
       (MechSpecials2(mech) & STEALTH_ARMOR_TECH) ? STH_ARMOR : \
       (MechSpecials2(mech) & HVY_FF_ARMOR_TECH) ? HVY_FF_ARMOR : \
       (MechSpecials2(mech) & LT_FF_ARMOR_TECH) ? LT_FF_ARMOR : \
       (MechInfantrySpecials(mech) & CS_PURIFIER_STEALTH_TECH) ? PURIFIER_ARMOR : \
       S_ARMOR))

#define ProperInternal(mech) \
(Cargo(\
       (MechSpecials(mech) & ES_TECH) ? ES_INTERNAL : \
       (MechSpecials(mech) & REINFI_TECH) ? RE_INTERNAL : \
       (MechSpecials(mech) & COMPI_TECH) ? CO_INTERNAL : \
       S_INTERNAL))
#endif

#define GrabPartsM(m,a,b,c) econ_change_items(IsDS(m) ? AeroBay(m,0) : Location(m->mynum),a,b,0-c)
#define PartAvailM(m,a,b,c) (econ_find_items(IsDS(m) ? AeroBay(m,0) : Location(m->mynum),a,b)>=c)
#ifndef BT_COMPLEXREPAIRS
#define AddPartsM(m,a,b,c) econ_change_items(IsDS(m) ? AeroBay(m,0) : Location(m->mynum), alias_part(m, a) , b, c)
#else
#define AddPartsM(m,l,a,b,c) econ_change_items(IsDS(m) ? AeroBay(m,0) : Location(m->mynum), alias_part(m, a, l) , b, c)
#endif
#define AVCHECKM(m,a,b,c)    DOCHECK1(!PartAvailM(m,a,b,c), tprintf("Not enough units of %s in store! You need to have at least %d.",part_name(a,b),c));

#ifndef BT_COMPLEXREPAIRS
#define alias_part(m,t) \
  (IsActuator(t) ? Cargo(S_ACTUATOR) : \
   (t == Special(ENGINE) ? \
    ((MechSpecials(m) & XL_TECH) ? Cargo(XL_ENGINE) : \
     (MechSpecials(m) & ICE_TECH) ? Cargo(IC_ENGINE) : \
     (MechSpecials(m) & XXL_TECH) ? Cargo(XXL_ENGINE) : \
     (MechSpecials(m) & CE_TECH) ? Cargo(COMP_ENGINE) : \
     (MechSpecials(m) & LE_TECH) ? Cargo(LIGHT_ENGINE) : t) : \
   (t == Special(HEAT_SINK) && MechHasDHS(m) ? Cargo(DOUBLE_HEAT_SINK) : t)))
#endif

ETECHEVENT(muxevent_tickmech_reattach);
ETECHEVENT(muxevent_tickmech_reseal);
ETECHEVENT(muxevent_tickmech_reload);
ETECHEVENT(muxevent_tickmech_removegun);
ETECHEVENT(muxevent_tickmech_removepart);
ETECHEVENT(muxevent_tickmech_removesection);
ETECHEVENT(muxevent_tickmech_repairarmor);
ETECHEVENT(muxevent_tickmech_repairgun);
ETECHEVENT(muxevent_tickmech_repairenhcrit);
ETECHEVENT(muxevent_tickmech_repairinternal);
ETECHEVENT(muxevent_tickmech_repairpart);
ETECHEVENT(muxevent_tickmech_replacegun);
ETECHEVENT(muxevent_tickmech_mountbomb);
ETECHEVENT(muxevent_tickmech_umountbomb);
ETECHEVENT(muxevent_tickmech_replacesuit);
ETECHEVENT(very_fake_func);

void loadrepairs(FILE * f);
void saverepairs(FILE * f);
int valid_ammo_mode(MECH * mech, int loc, int part, int let);

#endif				/* MECH_TECH_H */
