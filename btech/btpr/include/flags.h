
/* flags.h - object flags */

/* $Id: flags.h,v 1.3 2005/06/23 02:59:58 murrayma Exp $ */

#include "copyright.h"

#ifndef __FLAGS_H
#define	__FLAGS_H

/* Object types */
#define	TYPE_THING 	0x1
#define	TYPE_PLAYER 	0x3

/* First word of flags */
#define	WIZARD		0x00000010	/* gets automatic control */
#define	DARK		0x00000040	/* Don't show contents or presence */
#define	QUIET		0x00000800	/* Prevent 'feelgood' messages */
#define	HALT		0x00001000	/* object cannot perform actions */
#define	GOING		0x00004000	/* object is available for recycling */
#define	PUPPET		0x00020000	/* Relays ALL messages to owner */
#define	INHERIT		0x02000000	/* Gets owner's privs. (i.e. Wiz) */
#define	ROBOT		0x08000000	/* Player is a ROBOT */
#define ROYALTY		0x20000000	/* Sees like a wiz, but ca't modify */

/* Second word of flags */
#define ANSI            0x00000200

#define HARDCODE        0x00080000
#define IN_CHARACTER    0x00100000
#define ANSIMAP         0x00200000	/* Player uses ANSI maps */

#define ZOMBIE          0x08000000	/* Hardcode object is a zombie */

#define	CONNECTED	0x40000000	/* Player is connected */
#define	SLAVE		0x80000000	/* Disallow most commands */

/* ---------------------------------------------------------------------------
 * OBJENT: Fundamental object types
 */

#define	GOD ((dbref) 1)

/* ---------------------- Object Permission/Attribute Macros */

extern int btpr_is_good_object(dbref) __attribute__ ((pure));

extern int btpr_is_type(dbref, int) __attribute__ ((pure));

extern int btpr_has_flag(dbref, int) __attribute__ ((pure));
extern int btpr_has_flag2(dbref, int) __attribute__ ((pure));
extern void btpr_set_flag(dbref, int);
extern void btpr_set_flag2(dbref, int);
extern void btpr_unset_flag2(dbref, int);

extern int btpr_has_contents(dbref) __attribute__ ((pure));
extern int btpr_can_examine(dbref, dbref) __attribute__ ((pure));

#define isRobot(x)		(btpr_has_flag((x), ROBOT))
#define Alive(x)		(isPlayer(x) || (Puppet(x) && Has_contents(x)))
#define Has_contents		btpr_has_contents
#define isPlayer(x)		(btpr_is_type((x), TYPE_PLAYER))

#define Good_obj		btpr_is_good_object

#define Royalty(x)		(btpr_has_flag((x), ROYALTY))
#define WizRoy(x)		(Royalty(x) || Wizard(x))
#define Ansi(x)			(btpr_has_flag2((x), ANSI))
#define Ansimap(x)		(btpr_has_flag2((x), ANSIMAP))

#define Wizard(x)		(btpr_has_flag((x), WIZARD) \
				 || btpr_has_flag(Owner(x), WIZARD) \
				    && Inherits(x))
#define Quiet(x)		(btpr_has_flag((x), QUIET))
#define Halted(x)		(btpr_has_flag((x), HALT))
#define Going(x)		(btpr_has_flag((x), GOING))
#define Puppet(x)		(btpr_has_flag((x), PUPPET))
#define Inherits(x)		(btpr_has_flag((x), INHERIT) \
				 || btpr_has_flag(Owner(x), INHERIT) \
				 || ((x) == Owner(x)))
#define Hardcode(x)		(btpr_has_flag2((x), HARDCODE))
#define Zombie(x)		(btpr_has_flag2((x), ZOMBIE))
#define In_Character(x)		(btpr_has_flag2((x), IN_CHARACTER))
#define Connected(x)		(isPlayer(x) && btpr_has_flag2((x), CONNECTED))
#define Slave(x)		(btpr_has_flag2((x), SLAVE))

#define s_Slave(x)		(btpr_set_flag2((x), SLAVE))
#define s_Going(x)		(btpr_set_flag((x), GOING))
#define s_Hardcode(x)		(btpr_set_flag2((x), HARDCODE))
#define c_Hardcode(x)		(btpr_unset_flag2((x), HARDCODE))
#define s_Zombie(x)		(btpr_set_flag2((x), ZOMBIE))
#define s_In_Character(x)	(btpr_set_flag2((x), IN_CHARACTER))

#define Examinable		btpr_can_examine

#define s_Dark(x)		(btpr_set_flag((x), DARK))

#endif
