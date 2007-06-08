/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 */

#include "mech.h"
#include "mech.events.h"
#include "p.mech.utils.h"
#include "p.mech.combat.h"
#include "p.mech.damage.h"
#include "p.aero.bomb.h"
#include "p.mech.update.h"
#include "p.crit.h"

#define CHECK_ZERO_LOC(mech,a,b) ( GetSectInt(mech, a) > 0 ? a : b )

int FindPunchLocation(MECH *target, int hitGroup) {

    int roll = Number(1, 6);

    /* New tables from Total Warfare - pg 147 (and back of book)
     * - Dany 01/2007 */
    if (MechIsQuad(target)) {

        switch (hitGroup) {
            case LEFTSIDE:
                switch (roll) {
                    case 1:
                    case 2:
                        return LTORSO; 
                    case 3:
                        return CTORSO;
                    case 4:
                        /* Front Left Leg */
                        return LARM; 
                    case 5:
                        /* Rear Left Leg */
                        return LLEG;
                    case 6:
                        return HEAD;
                }
                break;

            case RIGHTSIDE:
                switch (roll) {
                    case 1:
                    case 2:
                        return RTORSO;
                    case 3:
                        return CTORSO;
                    case 4:
                        /* Front Right Leg */
                        return RARM;
                    case 5:
                        /* Rear Right Leg */
                        return RLEG;
                    case 6:
                        return HEAD;
                }
                break;

            case BACK:
            case FRONT:
                switch (roll) {
                    case 1:
                        if (hitGroup == BACK) {
                            /* Rear Left Leg */
                            return LLEG;
                        } else {
                            /* Front Left Leg */
                            return LARM;
                        }
                    case 2:
                        return LTORSO;
                    case 3:
                        return CTORSO;
                    case 4:
                        return RTORSO;
                    case 5:
                        if (hitGroup == BACK) {
                            /* Rear Right Leg */
                            return RLEG;
                        } else {
                            /* Front Right Leg */
                            return RARM;
                        }
                    case 6:
                        return HEAD;
                }
                break;
        }

    } else {

        /* Biped Mech */
        switch (hitGroup) {
            case LEFTSIDE:
                switch (roll) {
                    case 1:
                    case 2:
                        return LTORSO;
                    case 3:
                        return CTORSO;
                    case 4:
                    case 5:
                        return LARM;
                    case 6:
                        return HEAD;
                }
                break;

            case BACK:
            case FRONT:
                switch (roll) {
                    case 1:
                        return LARM;
                    case 2:
                        return LTORSO;
                    case 3:
                        return CTORSO;
                    case 4:
                        return RTORSO;
                    case 5:
                        return RARM;
                    case 6:
                        return HEAD;
                }
                break;

            case RIGHTSIDE:
                switch (roll) {
                    case 1:
                    case 2:
                        return RTORSO;
                    case 3:
                        return CTORSO;
                    case 4:
                    case 5:
                        return RARM;
                    case 6:
                        return HEAD;
                }
                break;
        }
    }

    /* Should never reach this point unless
     * someone uses this function wrong */
    return -1;
}

int FindKickLocation(MECH *target, int hitGroup) {

    int roll = Number(1, 6);

    /* New tables from Total Warfare for quads */
    if (MechIsQuad(target)) {

        switch (hitGroup) {
            case LEFTSIDE:
                switch (roll) {
                    case 1:
                    case 2:
                    case 3:
                        /* Left Front Leg */
                        return LARM;
                    case 4:
                    case 5:
                    case 6:
                        /* Left Rear Leg */
                        return LLEG;
                }
                break;

            case RIGHTSIDE:
                switch (roll) {
                    case 1:
                    case 2:
                    case 3:
                        /* Right Front Leg */
                        return RARM;
                    case 4:
                    case 5:
                    case 6:
                        /* Right Rear Leg */
                        return RLEG;
                }
                break;

            case BACK:
            case FRONT:
                switch (roll) {
                    case 1:
                    case 2:
                    case 3:
                        if (hitGroup == BACK) {
                            /* Right Rear Leg */
                            return RLEG;
                        } else {
                            /* Right Front Leg */
                            return RARM;
                        }
                    case 4:
                    case 5:
                    case 6:
                        if (hitGroup == BACK) {
                            /* Left Rear Leg */
                            return LLEG;
                        } else {
                            /* Left Front Leg */
                            return LARM;
                        }
                }
        }

    } else {

        switch (hitGroup) {
            case LEFTSIDE:
                return LLEG;
            case BACK:
            case FRONT:
                switch (roll) {
                    case 1:
                    case 2:
                    case 3:
                        return RLEG;
                    case 4:
                    case 5:
                    case 6:
                        return LLEG;
                }
            case RIGHTSIDE:
                return RLEG;
        }
    }

    /* Should never get to this point but will include this
     * as a safeguard.  Should probably have the value
     * returned from this function checked anyways. */
    return -1;
}

/*
 * Exile stun code - Used when a mech takes a hit to the head
 * instead of doing damage to the head it stuns the pilot
 * and re-rolls the location
 */
int ModifyHeadHit(int hitGroup, MECH *mech) {

    int newloc = FindPunchLocation(mech, hitGroup);

    if (MechType(mech) != CLASS_MECH) {
        return newloc;
    }

    if (newloc != HEAD) {

        mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
        mech_notify(mech, MECHALL,
                "The cockpit violently shakes from a grazing blow! "
                "You are momentarily stunned!");

        if (CrewStunning(mech)) {
            StopCrewStunning(mech);
        }

        MechLOSBroadcast(mech,
                "significantly slows down and starts wobbling!");

        MechCritStatus(mech) |= MECH_STUNNED;

        if (MechSpeed(mech) > WalkingSpeed(MechMaxSpeed(mech))) {
            MechDesiredSpeed(mech) = WalkingSpeed(MechMaxSpeed(mech));
        }

        MECHEVENT(mech, EVENT_CREWSTUN, mech_crewstun_event, MECHSTUN_TICK, 0);
    }
    return newloc;
}

int get_bsuit_hitloc(MECH * mech)
{
	int i;
	int table[NUM_BSUIT_MEMBERS];
	int last = 0;

	for(i = 0; i < NUM_BSUIT_MEMBERS; i++)
		if(GetSectInt(mech, i))
			table[last++] = i;
	if(!last)
		return -1;
	return table[Number(0, last - 1)];
}

int TransferTarget(MECH * mech, int hitloc)
{
	switch (MechType(mech)) {
	case CLASS_BSUIT:
		return get_bsuit_hitloc(mech);
	case CLASS_AERO:
	case CLASS_MECH:
	case CLASS_MW:
		switch (hitloc) {
		case RARM:
		case RLEG:
			return RTORSO;
			break;
		case LARM:
		case LLEG:
			return LTORSO;
			break;
		case RTORSO:
		case LTORSO:
			return CTORSO;
			break;
		}
		break;
	}
	return -1;
}

int FindSwarmHitLocation(int *iscritical, int *isrear)
{
	*isrear = 0;
	*iscritical = 1;

	switch (Roll()) {
	case 12:
	case 2:
		return HEAD;
	case 3:
	case 11:
		*isrear = 1;
		return CTORSO;
	case 4:
		*isrear = 1;
	case 5:
		return RTORSO;
	case 10:
		*isrear = 1;
	case 9:
		return LTORSO;
	case 6:
		return RARM;
	case 8:
		return LARM;
	case 7:
		return CTORSO;
	}
	return HEAD;
}

/*
 * Determines whether a section is crittable.
 * tres = armor percentage threshhold
 */
int crittable(MECH * mech, int loc, int tres)
{
	int d;

	if(MechSpecials(mech) & CRITPROOF_TECH)
		return 0;
	/* Towers and Stationary Objectives should not crit */
	if(MechMove(mech) == MOVE_NONE)
		return 0;
	if(!GetSectOArmor(mech, loc))
		return 1;
	if(MechType(mech) != CLASS_MECH && mudconf.btech_vcrit <= 1)
		return 0;

	/* Calculate percentage of armor remaining */
	d = (100 * GetSectArmor(mech, loc)) / GetSectOArmor(mech, loc);

	/* Are we below the threshold? */
	if(d < tres)
		return 1;
	if(d == 100) {
		if(Number(1, 71) == 23)
			return 1;
		return 0;
	}
	if(d < (100 - ((100 - tres) / 2)))
		if(Number(1, 11) == 6)
			return 1;
	return 0;
}								/* end crittable() */

int FindFasaHitLocation(MECH * mech, int hitGroup, int *iscritical,
						int *isrear)
{
	int roll, hitloc = 0;
	int side;

	*iscritical = 0;
	roll = Roll();

	if(MechStatus(mech) & COMBAT_SAFE)
		return 0;

	if(MechDugIn(mech) && GetSectOInt(mech, TURRET) && Number(1, 100) >= 42)
		return TURRET;

	rollstat.hitrolls[roll - 2]++;
	rollstat.tothrolls++;
	switch (MechType(mech)) {
	case CLASS_BSUIT:
		if((hitloc = get_bsuit_hitloc(mech)) < 0)
			return Number(0, NUM_BSUIT_MEMBERS - 1);
	case CLASS_MW:
	case CLASS_MECH:
		switch (hitGroup) {
		case LEFTSIDE:
			switch (roll) {
			case 2:
				*iscritical = 1;
				return LTORSO;
			case 3:
				return LLEG;
			case 4:
			case 5:
				return LARM;
			case 6:
				return LLEG;
			case 7:
				return LTORSO;
			case 8:
				return CTORSO;
			case 9:
				return RTORSO;
			case 10:
				return RARM;
			case 11:
				return RLEG;
			case 12:
				if(mudconf.btech_exile_stun_code)
					return ModifyHeadHit(hitGroup, mech);
				return HEAD;
			}
		case RIGHTSIDE:
			switch (roll) {
			case 2:
				*iscritical = 1;
				return RTORSO;
			case 3:
				return RLEG;
			case 4:
			case 5:
				return RARM;
			case 6:
				return RLEG;
			case 7:
				return RTORSO;
			case 8:
				return CTORSO;
			case 9:
				return LTORSO;
			case 10:
				return LARM;
			case 11:
				return LLEG;
			case 12:
				if(mudconf.btech_exile_stun_code)
					return ModifyHeadHit(hitGroup, mech);
				return HEAD;
			}
		case FRONT:
		case BACK:
			switch (roll) {
			case 2:
				*iscritical = 1;
				return CTORSO;
			case 3:
			case 4:
				return RARM;
			case 5:
				return RLEG;
			case 6:
				return RTORSO;
			case 7:
				return CTORSO;
			case 8:
				return LTORSO;
			case 9:
				return LLEG;
			case 10:
			case 11:
				return LARM;
			case 12:
				if(mudconf.btech_exile_stun_code)
					return ModifyHeadHit(hitGroup, mech);
				return HEAD;
			}
		}
		break;
	case CLASS_VEH_GROUND:
		switch (hitGroup) {

		case LEFTSIDE:
			switch (roll) {
			case 2:
				/* A Roll on Determining Critical Hits Table */
				*iscritical = 1;
				return LSIDE;
			case 3:
				if(mudconf.btech_tankfriendly) {
					if(!Fallen(mech)) {
						mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
						switch (MechMove(mech)) {
						case MOVE_TRACK:
							mech_notify(mech, MECHALL,
										"One of your tracks is seriously damaged!");
							break;
						case MOVE_WHEEL:
							mech_notify(mech, MECHALL,
										"One of your wheels is seriously damaged!");
							break;
						case MOVE_HOVER:
							mech_notify(mech, MECHALL,
										"Your air skirt is seriously damaged!");
							break;
						case MOVE_HULL:
						case MOVE_SUB:
						case MOVE_FOIL:
							mech_notify(mech, MECHALL,
										"Your craft lurches and suddenly loses a lot of speed!");
							break;
						}
						LowerMaxSpeed(mech, MP2);
					}
					return LSIDE;
				}
				/* Cripple tank */
				if(!Fallen(mech)) {
					mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
					switch (MechMove(mech)) {
					case MOVE_TRACK:
						mech_notify(mech, MECHALL,
									"One of your tracks is destroyed, immobilizing your vehicle!");
						break;
					case MOVE_WHEEL:
						mech_notify(mech, MECHALL,
									"One of your wheels is destroyed, immobilizing your vehicle!");
						break;
					case MOVE_HOVER:
						mech_notify(mech, MECHALL,
									"Your lift fan is destroyed, immobilizing your vehicle!");
						break;
					case MOVE_HULL:
					case MOVE_SUB:
					case MOVE_FOIL:
						mech_notify(mech, MECHALL,
									"Your engines cut out and you drift to a halt!");
					}
					SetMaxSpeed(mech, 0.0);

					MakeMechFall(mech);
				}
				return LSIDE;
			case 4:
			case 5:
				/* MP -1 */
				if(!Fallen(mech)) {
					mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
					switch (MechMove(mech)) {
					case MOVE_TRACK:
						mech_notify(mech, MECHALL,
									"One of your tracks is damaged!");
						break;
					case MOVE_WHEEL:
						mech_notify(mech, MECHALL,
									"One of your wheels is damaged!");
						break;
					case MOVE_HOVER:
						mech_notify(mech, MECHALL,
									"Your air skirt is damaged!");
						break;
					case MOVE_HULL:
					case MOVE_SUB:
					case MOVE_FOIL:
						mech_notify(mech, MECHALL, "Your craft suddenly slows!");
						break;
					}
					LowerMaxSpeed(mech, MP1);
				}
				return LSIDE;
				break;
			case 6:
			case 7:
			case 8:
			case 9:
				/* MP -1 if hover */
				return LSIDE;
			case 10:
				return (GetSectInt(mech, TURRET)) ? TURRET : LSIDE;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					if(!(MechTankCritStatus(mech) & TURRET_LOCKED)) {
						mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
						MechTankCritStatus(mech) |= TURRET_LOCKED;
						mech_notify(mech, MECHALL,
									"Your turret takes a direct hit and locks up!");
					}
					return TURRET;
				} else
					return LSIDE;
			case 12:
				/* A Roll on Determining Critical Hits Table */
				*iscritical = 1;
				return LSIDE;
			}
			break;
		case RIGHTSIDE:
			switch (roll) {
			case 2:
				*iscritical = 1;
				return RSIDE;
			case 3:
				if(mudconf.btech_tankfriendly) {
					if(!Fallen(mech)) {
						mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
						switch (MechMove(mech)) {
						case MOVE_TRACK:
							mech_notify(mech, MECHALL,
										"One of your tracks is seriously damaged!");
							break;
						case MOVE_WHEEL:
							mech_notify(mech, MECHALL,
										"One of your wheels is seriously damaged!");
							break;
						case MOVE_HOVER:
							mech_notify(mech, MECHALL,
										"Your air skirt is seriously damaged!");
							break;
						case MOVE_HULL:
						case MOVE_SUB:
						case MOVE_FOIL:
							mech_notify(mech, MECHALL,
										"Your craft lurches and suddenly loses a lot of speed!");
							break;
						}
						LowerMaxSpeed(mech, MP2);
					}
					return RSIDE;
				}
				/* Cripple Tank */
				if(!Fallen(mech)) {
					mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
					switch (MechMove(mech)) {
					case MOVE_TRACK:
						mech_notify(mech, MECHALL,
									"One of your tracks is destroyed, immobilizing your vehicle!");
						break;
					case MOVE_WHEEL:
						mech_notify(mech, MECHALL,
									"One of your wheels is destroyed, immobilizing your vehicle!");
						break;
					case MOVE_HOVER:
						mech_notify(mech, MECHALL,
									"Your lift fan is destroyed, immobilizing your vehicle!");
						break;
					case MOVE_HULL:
					case MOVE_SUB:
					case MOVE_FOIL:
						mech_notify(mech, MECHALL,
									"Your engines cut out and you drift to a halt!");
					}
					SetMaxSpeed(mech, 0.0);

					MakeMechFall(mech);
				}
				return RSIDE;
			case 4:
			case 5:
				/* MP -1 */
				if(!Fallen(mech)) {
					mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
					switch (MechMove(mech)) {
					case MOVE_TRACK:
						mech_notify(mech, MECHALL,
									"One of your tracks is damaged!");
						break;
					case MOVE_WHEEL:
						mech_notify(mech, MECHALL,
									"One of your wheels is damaged!");
						break;
					case MOVE_HOVER:
						mech_notify(mech, MECHALL,
									"Your air skirt is damaged!");
						break;
					case MOVE_HULL:
					case MOVE_SUB:
					case MOVE_FOIL:
						mech_notify(mech, MECHALL, "Your craft suddenly slows!");
						break;
					}
					LowerMaxSpeed(mech, MP1);
				}
				return RSIDE;
			case 6:
			case 7:
			case 8:
				return RSIDE;
			case 9:
				/* MP -1 if hover */
				if(!Fallen(mech)) {
					if(MechMove(mech) == MOVE_HOVER) {
						mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
						mech_notify(mech, MECHALL,
									"Your air skirt is damaged!");
						LowerMaxSpeed(mech, MP1);
					}
				}
				return RSIDE;
			case 10:
				return (GetSectInt(mech, TURRET)) ? TURRET : RSIDE;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					if(!(MechTankCritStatus(mech) & TURRET_LOCKED)) {
						mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
						MechTankCritStatus(mech) |= TURRET_LOCKED;
						mech_notify(mech, MECHALL,
									"Your turret takes a direct hit and locks up!");
					}
					return TURRET;
				} else
					return RSIDE;
			case 12:
				/* A Roll on Determining Critical Hits Table */
				*iscritical = 1;
				return RSIDE;
			}
			break;

		case FRONT:
		case BACK:
			side = (hitGroup == FRONT ? FSIDE : BSIDE);
			switch (roll) {
			case 2:
				/* A Roll on Determining Critical Hits Table */
				*iscritical = 1;
				return side;
			case 3:
				if(mudconf.btech_tankshield) {
					if(mudconf.btech_tankfriendly) {
						if(!Fallen(mech)) {
							mech_notify(mech, MECHALL,
										"%ch%cyCRITICAL HIT!%c");
							switch (MechMove(mech)) {
							case MOVE_TRACK:
								mech_notify(mech, MECHALL,
											"One of your tracks is seriously damaged!");
								break;
							case MOVE_WHEEL:
								mech_notify(mech, MECHALL,
											"One of your wheels is seriously damaged!");
								break;
							case MOVE_HOVER:
								mech_notify(mech, MECHALL,
											"Your air skirt is seriously damaged!");
								break;
							case MOVE_HULL:
							case MOVE_SUB:
							case MOVE_FOIL:
								mech_notify(mech, MECHALL,
											"Your craft lurches and suddenly loses a lot of speed!");
								break;
							}
							LowerMaxSpeed(mech, MP2);
						}
						return side;
					}
					/* Cripple tank */
					if(!Fallen(mech)) {
						mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
						switch (MechMove(mech)) {
						case MOVE_TRACK:
							mech_notify(mech, MECHALL,
										"One of your tracks is destroyed, immobilizing your vehicle!");
							break;
						case MOVE_WHEEL:
							mech_notify(mech, MECHALL,
										"One of your wheels is destroyed, immobilizing your vehicle!");
							break;
						case MOVE_HOVER:
							mech_notify(mech, MECHALL,
										"Your lift fan is destroyed, immobilizing your vehicle!");
							break;
						case MOVE_HULL:
						case MOVE_SUB:
						case MOVE_FOIL:
							mech_notify(mech, MECHALL,
										"Your engines cut out and you drift to a halt!");
						}
						SetMaxSpeed(mech, 0.0);

						MakeMechFall(mech);
					}
				}
				return side;
			case 4:
				/* MP -1 */
				if(mudconf.btech_tankshield) {
					if(!Fallen(mech)) {
						mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
						switch (MechMove(mech)) {
						case MOVE_TRACK:
							mech_notify(mech, MECHALL,
										"One of your tracks is damaged!");
							break;
						case MOVE_WHEEL:
							mech_notify(mech, MECHALL,
										"One of your wheels is damaged!");
							break;
						case MOVE_HOVER:
							mech_notify(mech, MECHALL,
										"Your air skirt is damaged!");
							break;
						case MOVE_HULL:
						case MOVE_SUB:
						case MOVE_FOIL:
							mech_notify(mech, MECHALL,
										"Your craft suddenly slows!");
							break;
						}
						LowerMaxSpeed(mech, MP1);
					}
				}
				return side;
			case 5:
				/* MP -1 if Hovercraft */
				if(!Fallen(mech)) {
					if(MechMove(mech) == MOVE_HOVER) {
						mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
						mech_notify(mech, MECHALL,
									"Your air skirt is damaged!");
						LowerMaxSpeed(mech, MP1);
					}
				}
				return side;
			case 6:
			case 7:
			case 8:
			case 9:
				return side;
			case 10:
				return (GetSectInt(mech, TURRET)) ? TURRET : side;
			case 11:
				*iscritical = 1;
				/* Lock turret into place */
				if(GetSectInt(mech, TURRET)) {
					if(!(MechTankCritStatus(mech) & TURRET_LOCKED)) {
						mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");
						MechTankCritStatus(mech) |= TURRET_LOCKED;
						mech_notify(mech, MECHALL,
									"Your turret takes a direct hit and locks up!");
					}
					return TURRET;
				} else
					return side;
			case 12:
				/* A Roll on Determining Critical Hits Table */
				if(crittable(mech, (GetSectInt(mech,
											   TURRET)) ? TURRET : side,
							 mudconf.btech_critlevel))
					*iscritical = 1;
				return (GetSectInt(mech, TURRET)) ? TURRET : side;
			}
		}
		break;
	case CLASS_AERO:
		switch (hitGroup) {
		case FRONT:
			switch (roll) {
				case 2:
					// Nose/Weapons
					return AERO_NOSE;
				case 3:
					// Nose/Sensors
					return AERO_NOSE;
				case 4:
					// Right Wing/Heat Sink
					return AERO_RWING;
				case 5:
					// Right Wing/Weapon
					return AERO_RWING;
				case 6:
					// Nose/Avionics
					return AERO_NOSE;
				case 7:
					// Nose/Control
					return AERO_NOSE;
				case 8:
					// Nose/FCS
					return AERO_NOSE;
				case 9:
					// Left Wing/Weapon
					return AERO_LWING;
				case 10:
					// Left Wing/Heat Sink
					return AERO_LWING;
				case 11:
					// Nose/Gear
					if(crittable(mech, AERO_NOSE, 90))
						LoseWeapon(mech, AERO_NOSE);
					return AERO_NOSE;
				case 12:
					// Nose/Weapon
					return AERO_NOSE;
			}
			break;
		case LEFTSIDE:
			switch (roll) {
				case 2:
					// Nose/Weapon
					return AERO_NOSE;
				case 3:
					// Wing/Gear
					return AERO_LWING;
				case 4:
					// Nose/Senors
					return AERO_NOSE;
				case 5:
					// Nose/Crew
					return AERO_NOSE;
				case 6:
					// Wing/Weapons
					return AERO_LWING;
				case 7:
					// Wing/Avionics
					return AERO_LWING;
				case 8:
					// Wing/Bomb
					return AERO_LWING;
				case 9:
					// Aft/Control
					return AERO_AFT;
				case 10:
					// Aft/Engine
					return AERO_AFT;
				case 11:
					// Wing/Gear
					return AERO_LWING;
				case 12:
					// Aft/Weapon
					return AERO_AFT;
			}
		case RIGHTSIDE:
			switch (roll) {
				case 2:
					// Nose/Weapon
					return AERO_NOSE;
				case 3:
					// Wing/Gear
					return AERO_RWING;
				case 4:
					// Nose/Sensors
					return AERO_NOSE;
				case 5:
					// Nose/Crew
					return AERO_NOSE;
				case 6:
					// Wing/Weapons
					return AERO_RWING;
				case 7:
					// Wing/Avionics
					return AERO_RWING;
				case 8:
					// Wing/Bomb
					return AERO_RWING;
				case 9:
					// Aft/Control
					return AERO_AFT;
				case 10:
					// Aft/Engine
					return AERO_AFT;
				case 11:
					// Wing/Gear
					return AERO_RWING;
				case 12:
					// Aft/Weapon
					return AERO_AFT;
			}
			break;
		case BACK:
			switch (roll) {
				case 2:
					// Aft/Weapon
					return AERO_AFT;
				case 3:
					// Aft/Heat Sink
					return AERO_AFT;
				case 4:
					// Right Wing/Fuel
					return AERO_RWING;
				case 5:
					// Right Wing/Weapon
					return AERO_RWING;
				case 6:
					// Aft/Engine
					return AERO_AFT;
				case 7:
					// Aft/Control
					return AERO_AFT;
				case 8:
					// Aft/Engine
					return AERO_AFT;
				case 9:
					// Left Wing/Weapon
					return AERO_LWING;
				case 10:
					// Left Wing/Fuel
					return AERO_LWING;
				case 11:
					// Aft/Heat Sink
					return AERO_AFT;
				case 12:
					// Aft/Weapon
					return AERO_AFT;
			}
		}
		break;
	case CLASS_DS:
	case CLASS_SPHEROID_DS:
		switch (hitGroup) {
		case FRONT:
			switch (roll) {
			case 2:
			case 12:
				if(crittable(mech, DS_NOSE, 30))
					ds_BridgeHit(mech);
				return DS_NOSE;
			case 3:
			case 11:
				if(crittable(mech, DS_NOSE, 50))
					LoseWeapon(mech, DS_NOSE);
				return DS_NOSE;
			case 5:
				return DS_RWING;
			case 6:
			case 7:
			case 8:
				return DS_NOSE;
			case 9:
				return DS_LWING;
			case 4:
			case 10:
				return (Number(1, 2)) == 1 ? DS_LWING : DS_RWING;
			}
		case LEFTSIDE:
		case RIGHTSIDE:
			side = (hitGroup == LEFTSIDE) ? DS_LWING : DS_RWING;
			if(Number(1, 2) == 2)
				SpheroidToRear(mech, side);
			switch (roll) {
			case 2:
				if(crittable(mech, DS_NOSE, 30))
					ds_BridgeHit(mech);
				return DS_NOSE;
			case 3:
			case 11:
				if(crittable(mech, side, 60))
					LoseWeapon(mech, side);
				return side;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 10:
				return side;
			case 9:
				return DS_NOSE;
			case 12:
				if(crittable(mech, side, 60))
					*iscritical = 1;
				return side;
			}
		case BACK:
			switch (roll) {
			case 2:
			case 12:
				if(crittable(mech, DS_AFT, 60))
					*iscritical = 1;
				return DS_AFT;
			case 3:
			case 11:
				return DS_AFT;
			case 4:
			case 7:
			case 10:
				if(crittable(mech, DS_AFT, 60))
					DestroyHeatSink(mech, DS_AFT);
				return DS_AFT;
			case 5:
				hitloc = DS_RWING;
				SpheroidToRear(mech, hitloc);
				return hitloc;
			case 6:
			case 8:
				return DS_AFT;
			case 9:
				hitloc = DS_LWING;
				SpheroidToRear(mech, hitloc);
				return hitloc;
			}
		}
		break;
	case CLASS_VTOL:
		switch (hitGroup) {
		case LEFTSIDE:
			switch (roll) {
			case 2:
				hitloc = ROTOR;
				*iscritical = 1;
				DoVTOLRotorDestroyedCrit(mech, NULL, 1);
				break;
			case 3:
				*iscritical = 1;
				break;
			case 4:
			case 5:
				hitloc = ROTOR;
				DoVTOLRotorDamagedCrit(mech);
				break;
			case 6:
			case 7:
			case 8:
				hitloc = LSIDE;
				break;
			case 9:
				/*  Destroy Main Weapon but do not destroy armor */
				DestroyMainWeapon(mech);
				hitloc = 0;
				break;
			case 10:
			case 11:
				hitloc = ROTOR;
				DoVTOLRotorDamagedCrit(mech);
				break;
			case 12:
				hitloc = ROTOR;
				*iscritical = 1;
				DoVTOLRotorDamagedCrit(mech);
				break;
			}
			break;

		case RIGHTSIDE:
			switch (roll) {
			case 2:
				hitloc = ROTOR;
				*iscritical = 1;
				DoVTOLRotorDestroyedCrit(mech, NULL, 1);
				break;
			case 3:
				*iscritical = 1;
				break;
			case 4:
			case 5:
				hitloc = ROTOR;
				DoVTOLRotorDamagedCrit(mech);
				break;
			case 6:
			case 7:
			case 8:
				hitloc = RSIDE;
				break;
			case 9:
				/* Destroy Main Weapon but do not destroy armor */
				DestroyMainWeapon(mech);
				break;
			case 10:
			case 11:
				hitloc = ROTOR;
				DoVTOLRotorDamagedCrit(mech);
				break;
			case 12:
				hitloc = ROTOR;
				*iscritical = 1;
				DoVTOLRotorDamagedCrit(mech);
				break;
			}
			break;

		case FRONT:
		case BACK:
			side = (hitGroup == FRONT ? FSIDE : BSIDE);

			switch (roll) {
			case 2:
				hitloc = ROTOR;
				*iscritical = 1;
				DoVTOLRotorDestroyedCrit(mech, NULL, 1);
				break;
			case 3:
				hitloc = ROTOR;
				DoVTOLRotorDestroyedCrit(mech, NULL, 1);
				break;
			case 4:
			case 5:
				hitloc = ROTOR;
				DoVTOLRotorDamagedCrit(mech);
				break;
			case 6:
			case 7:
			case 8:
			case 9:
				hitloc = side;
				break;
			case 10:
			case 11:
				hitloc = ROTOR;
				DoVTOLRotorDamagedCrit(mech);
				break;
			case 12:
				hitloc = ROTOR;
				*iscritical = 1;
				DoVTOLRotorDamagedCrit(mech);
				break;
			}
			break;
		}

		break;
	case CLASS_VEH_NAVAL:
		switch (hitGroup) {
		case LEFTSIDE:
			switch (roll) {
			case 2:
				hitloc = LSIDE;
				if(crittable(mech, hitloc, 40))
					*iscritical = 1;
				break;
			case 3:
			case 4:
			case 5:
				hitloc = LSIDE;
				break;
			case 9:
				hitloc = LSIDE;
				break;
			case 10:
				if(GetSectInt(mech, TURRET))
					hitloc = TURRET;
				else
					hitloc = LSIDE;
				break;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					hitloc = TURRET;
					if(crittable(mech, hitloc, 40))
						*iscritical = 1;
				} else
					hitloc = LSIDE;
				break;
			case 12:
				hitloc = LSIDE;
				*iscritical = 1;
				break;
			}
			break;

		case RIGHTSIDE:
			switch (roll) {
			case 2:
			case 12:
				hitloc = RSIDE;
				if(crittable(mech, hitloc, 40))
					*iscritical = 1;
				break;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				hitloc = RSIDE;
				break;
			case 10:
				if(GetSectInt(mech, TURRET))
					hitloc = TURRET;
				else
					hitloc = RSIDE;
				break;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					hitloc = TURRET;
					if(crittable(mech, hitloc, 40))
						*iscritical = 1;
				} else
					hitloc = RSIDE;
				break;
			}
			break;

		case FRONT:
		case BACK:
			side = (hitGroup == FRONT ? FSIDE : BSIDE);
			switch (roll) {
			case 2:
			case 12:
				hitloc = side;
				if(crittable(mech, hitloc, 40))
					*iscritical = 1;
				break;
			case 3:
				hitloc = side;
				break;
			case 4:
				hitloc = side;
				break;
			case 5:
				hitloc = side;
				break;
			case 6:
			case 7:
			case 8:
			case 9:
				hitloc = side;
				break;
			case 10:
				if(GetSectInt(mech, TURRET))
					hitloc = TURRET;
				else
					hitloc = side;
				break;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					hitloc = TURRET;
					*iscritical = 1;
				} else
					hitloc = side;
				break;
			}
			break;
		}
		break;
	}
	return (hitloc);
}

/* Do L3 FASA motive system crits */
void DoMotiveSystemHit(MECH * mech, int wRollMod)
{
	int wRoll;
	char strVhlTypeName[30];

	wRoll = Roll() + wRollMod;

	switch (MechMove(mech)) {
	case MOVE_TRACK:
		strcpy(strVhlTypeName, "tank");
		break;
	case MOVE_WHEEL:
		strcpy(strVhlTypeName, "vehicle");
		wRoll += 2;
		break;
	case MOVE_HOVER:
		strcpy(strVhlTypeName, "hovercraft");
		wRoll += 4;
		break;
	case MOVE_HULL:
		strcpy(strVhlTypeName, "ship");
		break;
	case MOVE_FOIL:
		strcpy(strVhlTypeName, "hydrofoil");
		wRoll += 4;
		break;
	case MOVE_SUB:
		strcpy(strVhlTypeName, "submarine");
		break;
	default:
		strcpy(strVhlTypeName, "weird unidentifiable toy (warn a wizard!)");
		break;
	}

	if(wRoll < 8)				/* no effect */
		return;

	mech_notify(mech, MECHALL, "%ch%cyCRITICAL HIT!%c");

	if(wRoll < 10) {			/* minor effect */
		MechPilotSkillBase(mech) += 1;

		if(Fallen(mech))
			mech_notify(mech, MECHALL,
						"%cr%chYour destroyed motive system takes another hit!%cn");
		else
			mech_printf(mech, MECHALL,
						"%%cr%%chYour motive system takes a minor hit, making it harder to control your %s!%%cn",
						strVhlTypeName);

		if(MechSpeed(mech) != 0.0)
			MechLOSBroadcast(mech, "wobbles slightly.");
	} else if(wRoll < 12) {		/* moderate effect */
		MechPilotSkillBase(mech) += 2;

		if(Fallen(mech))
			mech_notify(mech, MECHALL,
						"%cr%chYour destroyed motive system takes another hit!%cn");
		else
			mech_printf(mech, MECHALL,
						"%%cr%%chYour motive system takes a moderate hit, slowing you down and making it harder to control your %s!%%cn",
						strVhlTypeName);

		if(MechSpeed(mech) != 0.0)
			MechLOSBroadcast(mech, "wobbles violently.");

		LowerMaxSpeed(mech, MP1);
		correct_speed(mech);
	} else {
		if(Fallen(mech))
			mech_notify(mech, MECHALL,
						"%cr%chYour destroyed motive system takes another hit!%cn");
		else
			mech_printf(mech, MECHALL,
						"%%cr%%chYour motive system is destroyed! Your %s can no longer move!%%cn",
						strVhlTypeName);

		if(MechSpeed(mech) > 0)
			MechLOSBroadcast(mech,
							 "shakes violently then begins to slow down.");

		SetMaxSpeed(mech, 0.0);
		MakeMechFall(mech);
		correct_speed(mech);
	}

}

int FindAdvFasaVehicleHitLocation(MECH * mech, int hitGroup,
								  int *iscritical, int *isrear)
{
	int roll, hitloc = 0;
	int side;

	*iscritical = 0;
	roll = Roll();

	if(MechStatus(mech) & COMBAT_SAFE)
		return 0;

	if(MechDugIn(mech) && GetSectInt(mech, TURRET) && Number(1, 100) >= 42)
		return TURRET;

	rollstat.hitrolls[roll - 2]++;
	rollstat.tothrolls++;

	switch (MechType(mech)) {
	case CLASS_VEH_GROUND:
		switch (hitGroup) {
		case LEFTSIDE:
		case RIGHTSIDE:
			side = (hitGroup == LEFTSIDE ? LSIDE : RSIDE);

			switch (roll) {
			case 2:
				hitloc = side;
				*iscritical = 1;
				break;
			case 3:
				hitloc = side;
				if(crittable(mech, hitloc, mudconf.btech_critlevel))
					DoMotiveSystemHit(mech, 0);
				break;
			case 4:
				hitloc = side;
				break;
			case 5:
				hitloc = FSIDE;
				break;
			case 6:
			case 7:
			case 8:
				hitloc = side;
				break;
			case 9:
				hitloc = BSIDE;
				break;
			case 10:
			case 11:
				hitloc = CHECK_ZERO_LOC(mech, TURRET, side);
				break;
			case 12:
				hitloc = CHECK_ZERO_LOC(mech, TURRET, side);
				*iscritical = 1;
				break;
			}
			break;

		case FRONT:
		case BACK:
			side = (hitGroup == FRONT ? FSIDE : BSIDE);

			switch (roll) {
			case 2:
				hitloc = side;
				*iscritical = 1;
				break;
			case 3:
				hitloc = side;

				if(crittable(mech, hitloc, mudconf.btech_critlevel))
					DoMotiveSystemHit(mech, 0);
				break;
			case 4:
				hitloc = side;
				break;
			case 5:
				hitloc = (hitGroup == FRONT ? RSIDE : LSIDE);
				break;
			case 6:
			case 7:
			case 8:
				hitloc = side;
				break;
			case 9:
				hitloc = (hitGroup == FRONT ? LSIDE : RSIDE);
				break;
			case 10:
			case 11:
				hitloc =
					CHECK_ZERO_LOC(mech, TURRET,
								   (hitGroup == FRONT ? LSIDE : RSIDE));
				break;
			case 12:
				hitloc =
					CHECK_ZERO_LOC(mech, TURRET,
								   (hitGroup == FRONT ? LSIDE : RSIDE));
				*iscritical = 1;
				break;
			}
			break;
		}
		break;

	case CLASS_VTOL:
		switch (hitGroup) {
		case LEFTSIDE:
		case RIGHTSIDE:
			side = (hitGroup == LEFTSIDE ? LSIDE : RSIDE);

			switch (roll) {
			case 2:
				hitloc = side;
				*iscritical = 1;
				break;
			case 3:
			case 4:
				hitloc = side;
				break;
			case 5:
				hitloc = FSIDE;
				break;
			case 6:
			case 7:
			case 8:
				hitloc = side;
				break;
			case 9:
				hitloc = BSIDE;
				break;
			case 10:
			case 11:
				hitloc = ROTOR;
				break;
			case 12:
				hitloc = ROTOR;
				*iscritical = 1;
				break;
			}
			break;

		case FRONT:
		case BACK:
			side = (hitGroup == FRONT ? FSIDE : BSIDE);

			switch (roll) {
			case 2:
				hitloc = side;
				*iscritical = 1;
				break;
			case 3:
				hitloc = side;
				break;
			case 4:
				hitloc = side;
				break;
			case 5:
				hitloc = (hitGroup == FRONT ? RSIDE : LSIDE);
				break;
			case 6:
			case 7:
			case 8:
				hitloc = side;
				break;
			case 9:
				hitloc = (hitGroup == FRONT ? LSIDE : RSIDE);
				break;
			case 10:
			case 11:
				hitloc = ROTOR;
				break;
			case 12:
				hitloc = ROTOR;
				*iscritical = 1;
				break;
			}
			break;
		}
		break;
	}

	if(!crittable(mech, hitloc, mudconf.btech_critlevel))
		*iscritical = 0;

	return hitloc;
}

/* Use this when the unit is CRITPROOF because the other
 * hitlocation functions are screwy */
int FindHitLocation_CritProof(MECH * mech, int hitGroup, int *iscritical,
							  int *isrear)
{
	int roll, hitloc = 0;
	int side;

	roll = Roll();

	/* Since we're crit proof set this to 0 */
	*iscritical = 0;

	if(MechStatus(mech) & COMBAT_SAFE)
		return 0;

	if(MechDugIn(mech) && GetSectOInt(mech, TURRET) && Number(1, 100) >= 42)
		return TURRET;

	rollstat.hitrolls[roll - 2]++;
	rollstat.tothrolls++;
	switch (MechType(mech)) {
	case CLASS_BSUIT:
		if((hitloc = get_bsuit_hitloc(mech)) < 0)
			return Number(0, NUM_BSUIT_MEMBERS - 1);
	case CLASS_MW:
	case CLASS_MECH:
		switch (hitGroup) {
		case LEFTSIDE:
			switch (roll) {
			case 2:
				return LTORSO;
			case 3:
				return LLEG;
			case 4:
			case 5:
				return LARM;
			case 6:
				return LLEG;
			case 7:
				return LTORSO;
			case 8:
				return CTORSO;
			case 9:
				return RTORSO;
			case 10:
				return RARM;
			case 11:
				return RLEG;
			case 12:
				if(mudconf.btech_exile_stun_code)
					return ModifyHeadHit(hitGroup, mech);
				return HEAD;
			}
		case RIGHTSIDE:
			switch (roll) {
			case 2:
				return RTORSO;
			case 3:
				return RLEG;
			case 4:
			case 5:
				return RARM;
			case 6:
				return RLEG;
			case 7:
				return RTORSO;
			case 8:
				return CTORSO;
			case 9:
				return LTORSO;
			case 10:
				return LARM;
			case 11:
				return LLEG;
			case 12:
				if(mudconf.btech_exile_stun_code)
					return ModifyHeadHit(hitGroup, mech);
				return HEAD;
			}
		case FRONT:
		case BACK:
			switch (roll) {
			case 2:
				return CTORSO;
			case 3:
			case 4:
				return RARM;
			case 5:
				return RLEG;
			case 6:
				return RTORSO;
			case 7:
				return CTORSO;
			case 8:
				return LTORSO;
			case 9:
				return LLEG;
			case 10:
			case 11:
				return LARM;
			case 12:
				if(mudconf.btech_exile_stun_code)
					return ModifyHeadHit(hitGroup, mech);
				return HEAD;
			}
		}
		break;
	case CLASS_VEH_GROUND:
		switch (hitGroup) {
		case LEFTSIDE:
			switch (roll) {
			case 2:
			case 12:
				return LSIDE;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				return LSIDE;
			case 10:
				return (GetSectInt(mech, TURRET)) ? TURRET : LSIDE;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					return TURRET;
				} else
					return LSIDE;
			}
			break;
		case RIGHTSIDE:
			switch (roll) {
			case 2:
			case 12:
				return RSIDE;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				return RSIDE;
			case 10:
				return (GetSectInt(mech, TURRET)) ? TURRET : RSIDE;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					return TURRET;
				} else
					return RSIDE;
				break;
			}
			break;

		case FRONT:
		case BACK:
			side = (hitGroup == FRONT ? FSIDE : BSIDE);
			switch (roll) {
			case 2:
			case 12:
				return side;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				return side;
			case 10:
				return (GetSectInt(mech, TURRET)) ? TURRET : side;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					return TURRET;
				} else
					return side;
			}
		}
		break;
	case CLASS_AERO:
		switch (hitGroup) {
		case FRONT:
			switch (roll) {
			case 2:
			case 12:
			case 3:
			case 11:
				return AERO_NOSE;
			case 4:
			case 10:
			case 5:
				return AERO_RWING;
			case 9:
				return AERO_LWING;
			case 6:
			case 7:
			case 8:
				return AERO_NOSE;
			}
			break;
		case LEFTSIDE:
		case RIGHTSIDE:
			side = ((hitGroup == LEFTSIDE) ? AERO_LWING : AERO_RWING);
			switch (roll) {
			case 2:
			case 12:
				return AERO_AFT;
			case 3:
			case 11:
				return side;
			case 4:
			case 10:
				return AERO_AFT;
			case 5:
			case 9:
				return AERO_NOSE;
			case 6:
			case 8:
				return side;
			case 7:
			    return side;
			}
			break;
		case BACK:
			switch (roll) {
			case 2:
			case 12:
				return AERO_AFT;
			case 3:
			case 11:
			case 4:
			case 7:
			case 10:
			case 5:
				return AERO_RWING;
			case 9:
				return AERO_LWING;
			case 6:
			case 8:
				return AERO_AFT;
			}
		}
		break;
	case CLASS_DS:
	case CLASS_SPHEROID_DS:
		switch (hitGroup) {
		case FRONT:
			switch (roll) {
			case 2:
			case 12:
				return DS_NOSE;
			case 3:
			case 11:
				return DS_NOSE;
			case 5:
				return DS_RWING;
			case 6:
			case 7:
			case 8:
				return DS_NOSE;
			case 9:
				return DS_LWING;
			case 4:
			case 10:
				return (Number(1, 2)) == 1 ? DS_LWING : DS_RWING;
			}
		case LEFTSIDE:
		case RIGHTSIDE:
			side = (hitGroup == LEFTSIDE) ? DS_LWING : DS_RWING;
			if(Number(1, 2) == 2)
				SpheroidToRear(mech, side);
			switch (roll) {
			case 2:
				return DS_NOSE;
			case 3:
			case 11:
				return side;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 10:
				return side;
			case 9:
				return DS_NOSE;
			case 12:
				return side;
			}
		case BACK:
			switch (roll) {
			case 2:
			case 12:
				return DS_AFT;
			case 3:
			case 11:
				return DS_AFT;
			case 4:
			case 7:
			case 10:
				return DS_AFT;
			case 5:
				hitloc = DS_RWING;
				SpheroidToRear(mech, hitloc);
				return hitloc;
			case 6:
			case 8:
				return DS_AFT;
			case 9:
				hitloc = DS_LWING;
				SpheroidToRear(mech, hitloc);
				return hitloc;
			}
		}
		break;
	case CLASS_VTOL:
		switch (hitGroup) {
		case LEFTSIDE:
			switch (roll) {
			case 2:
				hitloc = ROTOR;
				break;
			case 3:
			case 4:
				hitloc = ROTOR;
				break;
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				hitloc = LSIDE;
				break;
			case 10:
			case 11:
				hitloc = ROTOR;
				break;
			case 12:
				hitloc = ROTOR;
				break;
			}
			break;

		case RIGHTSIDE:
			switch (roll) {
			case 2:
				hitloc = ROTOR;
				break;
			case 3:
			case 4:
				hitloc = ROTOR;
				break;
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				hitloc = RSIDE;
				break;
			case 10:
			case 11:
				hitloc = ROTOR;
				break;
			case 12:
				hitloc = ROTOR;
				break;
			}
			break;

		case FRONT:
		case BACK:
			side = (hitGroup == FRONT ? FSIDE : BSIDE);
			switch (roll) {
			case 2:
				hitloc = ROTOR;
				break;
			case 3:
			case 4:
				hitloc = ROTOR;
				break;
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				hitloc = side;
				break;
			case 10:
			case 11:
				hitloc = ROTOR;
				break;
			case 12:
				hitloc = ROTOR;
				break;
			}
			break;
		}
		break;
	case CLASS_VEH_NAVAL:
		switch (hitGroup) {
		case LEFTSIDE:
			switch (roll) {
			case 2:
				hitloc = LSIDE;
				break;
			case 3:
			case 4:
			case 5:
				hitloc = LSIDE;
				break;
			case 9:
				hitloc = LSIDE;
				break;
			case 10:
				if(GetSectInt(mech, TURRET))
					hitloc = TURRET;
				else
					hitloc = LSIDE;
				break;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					hitloc = TURRET;
				} else
					hitloc = LSIDE;
				break;
			case 12:
				hitloc = LSIDE;
				break;
			}
			break;

		case RIGHTSIDE:
			switch (roll) {
			case 2:
			case 12:
				hitloc = RSIDE;
				break;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				hitloc = RSIDE;
				break;
			case 10:
				if(GetSectInt(mech, TURRET))
					hitloc = TURRET;
				else
					hitloc = RSIDE;
				break;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					hitloc = TURRET;
				} else
					hitloc = RSIDE;
				break;
			}
			break;

		case FRONT:
		case BACK:
			switch (roll) {
			case 2:
			case 12:
				hitloc = FSIDE;
				break;
			case 3:
				hitloc = FSIDE;
				break;
			case 4:
				hitloc = FSIDE;
				break;
			case 5:
				hitloc = FSIDE;
				break;
			case 6:
			case 7:
			case 8:
			case 9:
				hitloc = FSIDE;
				break;
			case 10:
				if(GetSectInt(mech, TURRET))
					hitloc = TURRET;
				else
					hitloc = FSIDE;
				break;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					hitloc = TURRET;
				} else
					hitloc = FSIDE;
				break;
			}
			break;
		}
		break;
	}
	return (hitloc);
}

int FindHitLocation(MECH * mech, int hitGroup, int *iscritical, int *isrear)
{
	int roll, hitloc = 0;
	int side;

	roll = Roll();

	/* We have a varying set of crit charts we can use, so let's see what's been config'd */
	/* We call the FindHitLocation_CritProof after the adv fasa hit loc function because
	 * it already has a check built in for critproof (lookup crittable), the others don't */
	switch (MechType(mech)) {
	case CLASS_VTOL:
		if(mudconf.btech_fasaadvvtolcrit)
			return FindAdvFasaVehicleHitLocation(mech, hitGroup,
												 iscritical, isrear);
		else if(MechSpecials(mech) & CRITPROOF_TECH)
			return FindHitLocation_CritProof(mech, hitGroup, iscritical,
											 isrear);
		else if(mudconf.btech_fasacrit)
			return FindFasaHitLocation(mech, hitGroup, iscritical, isrear);
		break;
	case CLASS_VEH_GROUND:
		if(mudconf.btech_fasaadvvhlcrit)
			return FindAdvFasaVehicleHitLocation(mech, hitGroup,
												 iscritical, isrear);
		else if(MechSpecials(mech) & CRITPROOF_TECH)
			return FindHitLocation_CritProof(mech, hitGroup, iscritical,
											 isrear);
		else if(mudconf.btech_fasacrit)
			return FindFasaHitLocation(mech, hitGroup, iscritical, isrear);
		break;
	default:
		if(MechSpecials(mech) & CRITPROOF_TECH)
			return FindHitLocation_CritProof(mech, hitGroup, iscritical,
											 isrear);
		else if(mudconf.btech_fasacrit)
			return FindFasaHitLocation(mech, hitGroup, iscritical, isrear);
		break;
	}

	if(MechStatus(mech) & COMBAT_SAFE)
		return 0;
	if(MechDugIn(mech) && GetSectOInt(mech, TURRET) && Number(1, 100) >= 42)
		return TURRET;
	rollstat.hitrolls[roll - 2]++;
	rollstat.tothrolls++;
	switch (MechType(mech)) {
	case CLASS_BSUIT:
		if((hitloc = get_bsuit_hitloc(mech)) < 0)
			return Number(0, NUM_BSUIT_MEMBERS - 1);
	case CLASS_MW:
	case CLASS_MECH:
		switch (hitGroup) {
		case LEFTSIDE:
			switch (roll) {
			case 2:
				if(crittable(mech, LTORSO, 60))
					*iscritical = 1;
				return LTORSO;
			case 3:
				return LLEG;
			case 4:
			case 5:
				return LARM;
			case 6:
				return LLEG;
			case 7:
				return LTORSO;
			case 8:
				return CTORSO;
			case 9:
				return RTORSO;
			case 10:
				return RARM;
			case 11:
				return RLEG;
			case 12:
				if(mudconf.btech_exile_stun_code)
					return ModifyHeadHit(hitGroup, mech);
				return HEAD;
			}
		case RIGHTSIDE:
			switch (roll) {
			case 2:
				if(crittable(mech, RTORSO, 60))
					*iscritical = 1;
				return RTORSO;
			case 3:
				return RLEG;
			case 4:
			case 5:
				return RARM;
			case 6:
				return RLEG;
			case 7:
				return RTORSO;
			case 8:
				return CTORSO;
			case 9:
				return LTORSO;
			case 10:
				return LARM;
			case 11:
				return LLEG;
			case 12:
				if(mudconf.btech_exile_stun_code)
					return ModifyHeadHit(hitGroup, mech);
				return HEAD;
			}
		case FRONT:
		case BACK:
			switch (roll) {
			case 2:
				if(crittable(mech, CTORSO, 60))
					*iscritical = 1;
				return CTORSO;
			case 3:
			case 4:
				return RARM;
			case 5:
				return RLEG;
			case 6:
				return RTORSO;
			case 7:
				return CTORSO;
			case 8:
				return LTORSO;
			case 9:
				return LLEG;
			case 10:
			case 11:
				return LARM;
			case 12:
				if(mudconf.btech_exile_stun_code)
					return ModifyHeadHit(hitGroup, mech);
				return HEAD;
			}
		}
		break;
	case CLASS_VEH_GROUND:
		switch (hitGroup) {
		case LEFTSIDE:
			switch (roll) {
			case 2:
			case 12:
				if(crittable(mech, LSIDE, 40))
					*iscritical = 1;
				return LSIDE;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				return LSIDE;
			case 10:
				return (GetSectInt(mech, TURRET)) ? TURRET : LSIDE;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					if(crittable(mech, TURRET, 50))
						*iscritical = 1;
					return TURRET;
				} else
					return LSIDE;
			}
			break;
		case RIGHTSIDE:
			switch (roll) {
			case 2:
			case 12:
				if(crittable(mech, RSIDE, 40))
					*iscritical = 1;
				return RSIDE;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				return RSIDE;
			case 10:
				return (GetSectInt(mech, TURRET)) ? TURRET : RSIDE;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					if(crittable(mech, TURRET, 50))
						*iscritical = 1;
					return TURRET;
				} else
					return RSIDE;
				break;
			}
			break;

		case FRONT:
		case BACK:
			side = (hitGroup == FRONT ? FSIDE : BSIDE);
			switch (roll) {
			case 2:
			case 12:
				if(crittable(mech, FSIDE, 40))
					*iscritical = 1;
				return side;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				return side;
			case 10:
				return (GetSectInt(mech, TURRET)) ? TURRET : side;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					if(crittable(mech, TURRET, 50))
						*iscritical = 1;
					return TURRET;
				} else
					return side;
			}
		}
		break;
	case CLASS_AERO:
		switch (hitGroup) {
		case FRONT:
			switch (roll) {
			case 2:
			case 12:
			case 3:
			case 11:
				if(crittable(mech, AERO_NOSE, 90))
					LoseWeapon(mech, AERO_NOSE);
				return AERO_NOSE;
			case 4:
			case 10:
			case 5:
				return AERO_RWING;
			case 9:
				return AERO_LWING;
			case 6:
			case 7:
			case 8:
				return AERO_NOSE;
			}
			break;
		case LEFTSIDE:
		case RIGHTSIDE:
			side = ((hitGroup == LEFTSIDE) ? AERO_LWING : AERO_RWING);
			switch (roll) {
			case 2:
			case 12:
				if(crittable(mech, AERO_AFT, 99))
					*iscritical = 1;
				return AERO_AFT;
			case 3:
			case 11:
				if(crittable(mech, side, 99))
					LoseWeapon(mech, side);
				return side;
			case 4:
			case 10:
				if(crittable(mech, AERO_AFT, 90))
					DestroyHeatSink(mech, AERO_AFT);
				return AERO_AFT;
			case 5:
			case 9:
				return AERO_NOSE;
			case 6:
			case 8:
				return side;
			case 7:
			    return side;
			}
			break;
		case BACK:
			switch (roll) {
			case 2:
			case 12:
				if(crittable(mech, AERO_AFT, 90))
					*iscritical = 1;
				return AERO_AFT;
			case 3:
			case 11:
			case 4:
			case 7:
			case 10:
			case 5:
				return AERO_RWING;
			case 9:
				return AERO_LWING;
			case 6:
			case 8:
				return AERO_AFT;
			}
		}
		break;
	case CLASS_DS:
	case CLASS_SPHEROID_DS:
		switch (hitGroup) {
		case FRONT:
			switch (roll) {
			case 2:
			case 12:
				if(crittable(mech, DS_NOSE, 30))
					ds_BridgeHit(mech);
				return DS_NOSE;
			case 3:
			case 11:
				if(crittable(mech, DS_NOSE, 50))
					LoseWeapon(mech, DS_NOSE);
				return DS_NOSE;
			case 5:
				return DS_RWING;
			case 6:
			case 7:
			case 8:
				return DS_NOSE;
			case 9:
				return DS_LWING;
			case 4:
			case 10:
				return (Number(1, 2)) == 1 ? DS_LWING : DS_RWING;
			}
		case LEFTSIDE:
		case RIGHTSIDE:
			side = (hitGroup == LEFTSIDE) ? DS_LWING : DS_RWING;
			if(Number(1, 2) == 2)
				SpheroidToRear(mech, side);
			switch (roll) {
			case 2:
				if(crittable(mech, DS_NOSE, 30))
					ds_BridgeHit(mech);
				return DS_NOSE;
			case 3:
			case 11:
				if(crittable(mech, side, 60))
					LoseWeapon(mech, side);
				return side;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 10:
				return side;
			case 9:
				return DS_NOSE;
			case 12:
				if(crittable(mech, side, 60))
					*iscritical = 1;
				return side;
			}
		case BACK:
			switch (roll) {
			case 2:
			case 12:
				if(crittable(mech, DS_AFT, 60))
					*iscritical = 1;
				return DS_AFT;
			case 3:
			case 11:
				return DS_AFT;
			case 4:
			case 7:
			case 10:
				if(crittable(mech, DS_AFT, 60))
					DestroyHeatSink(mech, DS_AFT);
				return DS_AFT;
			case 5:
				hitloc = DS_RWING;
				SpheroidToRear(mech, hitloc);
				return hitloc;
			case 6:
			case 8:
				return DS_AFT;
			case 9:
				hitloc = DS_LWING;
				SpheroidToRear(mech, hitloc);
				return hitloc;
			}
		}
		break;
	case CLASS_VTOL:
		switch (hitGroup) {
		case LEFTSIDE:
			switch (roll) {
			case 2:
				hitloc = ROTOR;
				*iscritical = 1;
				DoVTOLRotorDestroyedCrit(mech, NULL, 1);
				break;
			case 3:
			case 4:
				hitloc = ROTOR;
				DoVTOLRotorDamagedCrit(mech);
				break;
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				hitloc = LSIDE;
				break;
			case 10:
			case 11:
				hitloc = ROTOR;
				DoVTOLRotorDamagedCrit(mech);
				break;
			case 12:
				hitloc = ROTOR;
				*iscritical = 1;
				DoVTOLRotorDamagedCrit(mech);
				break;
			}
			break;

		case RIGHTSIDE:
			switch (roll) {
			case 2:
				hitloc = ROTOR;
				*iscritical = 1;
				DoVTOLRotorDestroyedCrit(mech, NULL, 1);
				break;
			case 3:
			case 4:
				hitloc = ROTOR;
				DoVTOLRotorDamagedCrit(mech);
				break;
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				hitloc = RSIDE;
				break;
			case 10:
			case 11:
				hitloc = ROTOR;
				DoVTOLRotorDamagedCrit(mech);
				break;
			case 12:
				hitloc = ROTOR;
				*iscritical = 1;
				DoVTOLRotorDamagedCrit(mech);
				break;
			}
			break;

		case FRONT:
		case BACK:
			side = (hitGroup == FRONT ? FSIDE : BSIDE);
			switch (roll) {
			case 2:
				hitloc = ROTOR;
				*iscritical = 1;
				DoVTOLRotorDestroyedCrit(mech, NULL, 1);
				break;
			case 3:
			case 4:
				hitloc = ROTOR;
				DoVTOLRotorDamagedCrit(mech);
				break;
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				hitloc = side;
				break;
			case 10:
			case 11:
				hitloc = ROTOR;
				DoVTOLRotorDamagedCrit(mech);
				break;
			case 12:
				hitloc = ROTOR;
				*iscritical = 1;
				DoVTOLRotorDamagedCrit(mech);
				break;
			}
			break;
		}
		break;
	case CLASS_VEH_NAVAL:
		switch (hitGroup) {
		case LEFTSIDE:
			switch (roll) {
			case 2:
				hitloc = LSIDE;
				if(crittable(mech, hitloc, 40))
					*iscritical = 1;
				break;
			case 3:
			case 4:
			case 5:
				hitloc = LSIDE;
				break;
			case 9:
				hitloc = LSIDE;
				break;
			case 10:
				if(GetSectInt(mech, TURRET))
					hitloc = TURRET;
				else
					hitloc = LSIDE;
				break;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					hitloc = TURRET;
					if(crittable(mech, hitloc, 40))
						*iscritical = 1;
				} else
					hitloc = LSIDE;
				break;
			case 12:
				hitloc = LSIDE;
				*iscritical = 1;
				break;
			}
			break;

		case RIGHTSIDE:
			switch (roll) {
			case 2:
			case 12:
				hitloc = RSIDE;
				if(crittable(mech, hitloc, 40))
					*iscritical = 1;
				break;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				hitloc = RSIDE;
				break;
			case 10:
				if(GetSectInt(mech, TURRET))
					hitloc = TURRET;
				else
					hitloc = RSIDE;
				break;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					hitloc = TURRET;
					if(crittable(mech, hitloc, 40))
						*iscritical = 1;
				} else
					hitloc = RSIDE;
				break;
			}
			break;

		case FRONT:
		case BACK:
			switch (roll) {
			case 2:
			case 12:
				hitloc = FSIDE;
				if(crittable(mech, hitloc, 40))
					*iscritical = 1;
				break;
			case 3:
				hitloc = FSIDE;
				break;
			case 4:
				hitloc = FSIDE;
				break;
			case 5:
				hitloc = FSIDE;
				break;
			case 6:
			case 7:
			case 8:
			case 9:
				hitloc = FSIDE;
				break;
			case 10:
				if(GetSectInt(mech, TURRET))
					hitloc = TURRET;
				else
					hitloc = FSIDE;
				break;
			case 11:
				if(GetSectInt(mech, TURRET)) {
					hitloc = TURRET;
					*iscritical = 1;
				} else
					hitloc = FSIDE;
				break;
			}
			break;
		}
		break;
	}
	return (hitloc);
}

/*
 * Total Warfare, p. 119:
 *
 * Mechs have the following attack direction diagram:
 *
 *    FS
 * FS    FS
 * LS    RS
 *    AS
 *
 * Vehicles have the following attack direction diagram (p. 192):
 *
 *    FS
 * LS    RS
 * LS    RS
 *    AS
 *
 * Aerospace units use a diagram similar to that of vehicles.  The full rules
 * start on p. 237, with additional rules for grounded units on p. 249.
 *
 * ProtoMechs, infantry, and buildings do not use attack direction.
 *
 * In a change from the previous implementation, we use the bearing from the
 * target to the attacker, since this uses angles which seem more intuitive.
 *
 * Note that these arcs haven't been traditional in BTMUX usage.  Instead, the
 * following rules had been adopted:
 *
 * FS: front 90 degrees
 * AS: back 90 degrees
 * LS: left 90 degrees
 * RS: right 90 degree
 *
 * They had also previously claimed some "official" BT arcs which don't match
 * the BMR.  We'll use these arcs for our "old" BT option instead:
 *
 *    FS
 * FS    FS
 * LS    RS
 *    AS
 *
 * (The main difference between BMR and TW is that vehicles use the same arcs
 * as mechs do in BMR.)
 *
 * The particular rules desired may be set using the "btech_hit_arcs"
 * configuration option.  0 = TW rules (default), 1 = "classic" BTMUX rules,
 * 2 = "official" BT rules.
 *
 * The main problem with the traditional BTMUX arcs is that they tend to
 * over-emphasize rear area shots, and under-emphasize frontal shots, relative
 * to what the original designs had been intended for.  This tended to
 * over-emphasize the weakness of the rear area, relative to the TBS rules.
 *
 * The new arcs reduce the rear area on both mechs and vehicles by 33%, and
 * reduce the front arc on vehicles also by 33%, while increasing the front arc
 * on mechs by a whopping 100%.  Side arcs on vehicles will increase by 33%,
 * while decreasing by 33% on mechs, and shifting towards the rear.
 *
 * I'm guessing the overall effect of these changes will be to lengthen
 * battles, since more shots will tend to spread themselves out over the front
 * side, at least on mechs.  This will also make it a bit harder for defenders
 * to sponge specific arcs, as well as for attackers to target specific arcs.
 *
 * For vehicles, the reduction in rear arc vulnerability will probably be a net
 * positive, but the front arc tends to be strongest, and that will also be
 * reduced slightly in size.
 *
 * Note that this function is for hit location arcs, not firing arcs.  The
 * current firing arc rules are correct, to the best of my knowledge.
 *
 * --Codicus Unitus (cu5)
 */
int
FindAreaHitGroup(MECH *mech, MECH *target)
{
	int m_fs_hw, fs_hw;
	int m_as_hw, as_hw;

	int ad;

	/*
	 * Select rule set.  We compute this every time, to allow for dynamic
	 * configuration changes (although we could hook the configuration
	 * option instead and just do it once per change).
	 *
	 * The front side center is always 0 degrees, and the aft side center
	 * is always 180 degrees.  We merely configure how wide those arcs are,
	 * by setting the size of the half arc to each side of the center (the
	 * "half-width").
	 *
	 * The left side and right side are simply the leftovers, and are
	 * determined by whether an arc is less than or greater than 180.
	 */
	switch (mudconf.btech_hit_arcs) {
	case 0: /* TW rules */
	default:
		m_fs_hw = 90;
		m_as_hw = 30;

		fs_hw = 30;
		as_hw = 30;
		break;

	case 1: /* classic BTMUX rules */
		m_fs_hw = 45;
		m_as_hw = 45;

		fs_hw = m_fs_hw;
		as_hw = m_as_hw;
		break;

	case 2: /* BMR rules */
		m_fs_hw = 90;
		m_as_hw = 30;

		fs_hw = m_fs_hw;
		as_hw = m_as_hw;
		break;
	}

	/* Compute attack direction.  */
	ad = AcceptableDegree(FindBearing(MechFX(target), MechFY(target),
	                                  MechFX(mech), MechFY(mech))
	                      - MechFacing(target));

	/* Determine hit group.  */
	switch (MechType(target)) {
	case CLASS_MECH:
		/* Mech rules.  */
		if (ad >= (360 - m_fs_hw) || ad <= (0 + m_fs_hw)) {
			return FRONT;
		} else if (ad >= (180 - m_as_hw) && ad <= (180 + m_as_hw)) {
			return BACK;
		} else if (ad >= 180) {
			return LEFTSIDE;
		} else {
			return RIGHTSIDE;
		}
		break;

	default:
		/*
		 * TODO: This function shouldn't really do anything at all for
		 * most other types.  As a sensible default, use the vehicle
		 * rules, which is no more broken than the old code.
		 */
		/* FALLTHROUGH */

	case CLASS_VEH_GROUND:
	case CLASS_VEH_NAVAL:
	case CLASS_VTOL:
		/* Vehicle rules.  */
		if (ad >= (360 - fs_hw) || ad <= (0 + fs_hw)) {
			return FRONT;
		} else if (ad >= (180 - as_hw) && ad <= (180 + as_hw)) {
			return BACK;
		} else if (ad >= 180) {
			return LEFTSIDE;
		} else {
			return RIGHTSIDE;
		}
		break;
	}
}

int FindTargetHitLoc(MECH *mech, MECH *target, int *isrear, int *iscritical) {

    int hitGroup;

    *iscritical = 0;
    hitGroup = FindAreaHitGroup(mech, target);

    if (hitGroup == BACK) {
        *isrear = 1;
    }

    if (MechType(target) == CLASS_MECH && (MechStatus(target) & PARTIAL_COVER)) {
        return FindPunchLocation(target, hitGroup);
    }

    if (MechType(mech) == CLASS_MW && MechType(target) == CLASS_MECH &&
            MechZ(mech) <= MechZ(target)) {
        return FindKickLocation(target, hitGroup);
    }

    if (MechType(target) == CLASS_MECH && ((MechType(mech) == CLASS_BSUIT &&
              MechSwarmTarget(mech) == target->mynum))) {
        return FindSwarmHitLocation(iscritical, isrear);
    }

    return FindHitLocation(target, hitGroup, iscritical, isrear);
}

int findNARCHitLoc(MECH * mech, MECH * hitMech, int *tIsRearHit)
{
	int tIsRear = 0;
	int tIsCritical = 0;
	int wHitLoc = FindTargetHitLoc(mech, hitMech, &tIsRear, &tIsCritical);

	while (GetSectInt(hitMech, wHitLoc) <= 0) {
		wHitLoc = TransferTarget(hitMech, wHitLoc);
		if(wHitLoc < 0)
			return -1;
	}

	*tIsRearHit = 0;
	if(tIsRear) {
		if(MechType(hitMech) == CLASS_MECH)
			*tIsRearHit = 1;
		else if(wHitLoc == FSIDE)
			wHitLoc = BSIDE;
	}

	return wHitLoc;
}

int FindTCHitLoc(MECH * mech, MECH * target, int *isrear, int *iscritical)
{
	int hitGroup;

	*isrear = 0;
	*iscritical = 0;
	hitGroup = FindAreaHitGroup(mech, target);
	if(hitGroup == BACK)
		*isrear = 1;
	if(MechAimType(mech) == MechType(target) && Number(1, 6) >= 3)
		switch (MechType(target)) {
		case CLASS_MECH:
		case CLASS_MW:
			switch (MechAim(mech)) {
			case RARM:
				if(hitGroup != LEFTSIDE)
					return RARM;
				break;
			case LARM:
				if(hitGroup != RIGHTSIDE)
					return LARM;
				break;
			case RLEG:
				if(hitGroup != LEFTSIDE &&
				   !(MechStatus(target) & PARTIAL_COVER))
					return RLEG;
				break;
			case LLEG:
				if(hitGroup != RIGHTSIDE &&
				   !(MechStatus(target) & PARTIAL_COVER))
					return LLEG;
				break;
			case RTORSO:
				if(hitGroup != LEFTSIDE)
					return RTORSO;
				break;
			case LTORSO:
				if(hitGroup != RIGHTSIDE)
					return LTORSO;
				break;
			case CTORSO:

/*        if (hitGroup != LEFTSIDE && hitGroup != RIGHTSIDE) */
				return CTORSO;
			case HEAD:
				if(Immobile(target))
					return HEAD;
			}
			break;
		case CLASS_AERO:
		case CLASS_DS:
			switch (MechAim(mech)) {
			case AERO_NOSE:
				if(hitGroup != BACK)
					return AERO_NOSE;
				break;
			case AERO_LWING:
				if(hitGroup != RIGHTSIDE)
					return AERO_LWING;
				break;
			case AERO_RWING:
				if(hitGroup != LEFTSIDE)
					return AERO_RWING;
				break;
			case AERO_AFT:
				if(hitGroup != FRONT)
					return AERO_AFT;
				break;
			}
		case CLASS_VEH_GROUND:
		case CLASS_VEH_NAVAL:
		case CLASS_VTOL:
			switch (MechAim(mech)) {
			case RSIDE:
				if(hitGroup != LEFTSIDE)
					return (RSIDE);
				break;
			case LSIDE:
				if(hitGroup != RIGHTSIDE)
					return (LSIDE);
				break;
			case FSIDE:
				if(hitGroup != BACK)
					return (FSIDE);
				break;
			case BSIDE:
				if(hitGroup != FRONT)
					return (BSIDE);
				break;
			case TURRET:
				return (TURRET);
				break;
			}
			break;
		}
	if(MechType(target) == CLASS_MECH && (MechStatus(target) & PARTIAL_COVER))
		return FindPunchLocation(target, hitGroup);
	return FindHitLocation(target, hitGroup, iscritical, isrear);
}

int FindAimHitLoc(MECH * mech, MECH * target, int *isrear, int *iscritical)
{
	int hitGroup;

	*isrear = 0;
	*iscritical = 0;
	hitGroup = FindAreaHitGroup(mech, target);
	if(hitGroup == BACK)
		*isrear = 1;
	if(MechType(target) == CLASS_MECH || MechType(target) == CLASS_MW)
		switch (MechAim(mech)) {
		case RARM:
			if(hitGroup != LEFTSIDE)
				return (RARM);
			break;
		case LARM:
			if(hitGroup != RIGHTSIDE)
				return (LARM);
			break;
		case RLEG:
			if(hitGroup != LEFTSIDE && !(MechStatus(target) & PARTIAL_COVER))
				return (RLEG);
			break;
		case LLEG:
			if(hitGroup != RIGHTSIDE && !(MechStatus(target) & PARTIAL_COVER))
				return (LLEG);
			break;
		case RTORSO:
			if(hitGroup != LEFTSIDE)
				return (RTORSO);
			break;
		case LTORSO:
			if(hitGroup != RIGHTSIDE)
				return (LTORSO);
			break;
		case CTORSO:
			return (CTORSO);
		case HEAD:
			return (HEAD);
	} else if(is_aero(target))
		return MechAim(mech);
	else
		switch (MechAim(mech)) {
		case RSIDE:
			if(hitGroup != LEFTSIDE)
				return (RSIDE);
			break;
		case LSIDE:
			if(hitGroup != RIGHTSIDE)
				return (LSIDE);
			break;
		case FSIDE:
			if(hitGroup != BACK)
				return (FSIDE);
			break;
		case BSIDE:
			if(hitGroup != FRONT)
				return (BSIDE);
			break;
		case TURRET:
			return (TURRET);
			break;
		}

	if(MechType(target) == CLASS_MECH && (MechStatus(target) & PARTIAL_COVER))
		return FindPunchLocation(target, hitGroup);
	return FindHitLocation(target, hitGroup, iscritical, isrear);
}
