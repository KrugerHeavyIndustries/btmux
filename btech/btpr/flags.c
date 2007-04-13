#include "mudconf.h"
#include "flags.h"

#define BROKEN fprintf(stderr, "broken: " __FILE__ ": %s()\n", __FUNCTION__)

/* Has_contents() */
int
btpr_has_contents(dbref thing)
{
	BROKEN;
	return 1;
}

extern int btshim_is_type_player(dbref);

int
btpr_is_type(dbref thing, int type_flag)
{
	switch (type_flag) {
	case TYPE_PLAYER: /* IsPlayer() */
		return btshim_is_type_player(thing);

	default:
		fprintf(stderr,
		        "warning: btpr_is_type(#%d): type 0x%X unsupported\n",
		        thing, type_flag);
		return 0;
	}
}

extern int btshim_has_flag(dbref, const char *);

#define CASE_FLAG_SIMPLE(f) \
	case f: \
		return btshim_has_flag(thing, #f);

int
btpr_has_flag(dbref thing, int flag1)
{
	switch (flag1) {
	CASE_FLAG_SIMPLE(WIZARD) /* Wizard() */
	CASE_FLAG_SIMPLE(DARK) /* Dark() */
	CASE_FLAG_SIMPLE(QUIET) /* Quiet() */

	CASE_FLAG_SIMPLE(HALT) /* Halted() */
	CASE_FLAG_SIMPLE(GOING) /* Going() */
	CASE_FLAG_SIMPLE(PUPPET) /* Puppet() */

	case INHERIT: /* Inherit() */
		/* MUX treats INHERIT on wizard-owned objects like WIZARD.
		 * Penn always requires objects to be WIZARD, so this INHERIT
		 * behavior is useless.
		 */
		//return btshim_has_flag(thing, "TRUST");
		/* This would be more efficient if we just didn't check it, but
		 * MUX allowed non-WIZARD wizard-owned objects to have WIZARD
		 * powers if they were INHERIT, and we don't want that check to
		 * always succeed.  Further analysis will be needed.
		 */
		return btshim_has_flag(thing, "WIZARD");

	CASE_FLAG_SIMPLE(ROYALTY) /* Royalty() */

	case ROBOT: /* FIXME: No Penn equivalent */
		return btshim_has_flag(thing, "BT_ROBOT");

	default:
		fprintf(stderr,
		        "warning: btpr_has_flag(#%d): flag 0x%X unsupported\n",
		        thing, flag1);
		return 0;
	}
}

int
btpr_has_flag2(dbref thing, int flag2)
{
	switch (flag2) {
	case ANSI: /* MUX ANSI = Penn ANSI with COLOR */
		/* TODO: Since PennMUSH strips out ANSI when the client doesn't
		 * support it anyway, maybe we could just always return 1?
		 */
		return btshim_has_flag(thing, "ANSI")
		       && btshim_has_flag(thing, "COLOR");

	case HARDCODE:
		return btshim_has_flag(thing, "XCODE");

	case IN_CHARACTER: /* FIXME: No Penn equivalent */
		return btshim_has_flag(thing, "BT_IN_CHARACTER");

	case ANSIMAP: /* FIXME: No Penn equivalent (unless everyone ANSIMAP */
		return btshim_has_flag(thing, "BT_ANSIMAP");

	case ZOMBIE: /* FIXME: No Penn equivalent */
		return btshim_has_flag(thing, "BT_ZOMBIE");

	case SLAVE: /* FIXME: No Penn equivalent */
		return btshim_has_flag(thing, "BT_SLAVE");

	case CONNECTED:
		/* FIXME: Does this mean the same thing on Penn? */
	default:
		fprintf(stderr,
		        "warning: btpr_has_flag2(#%d): flag 0x%X unsupported\n",
		        thing, flag2);
		return 0;
	}
}


extern void btshim_set_flag(dbref, const char *);

void
btpr_set_flag(dbref thing, int flag1)
{
	fprintf(stderr,
	        "warning: btpr_set_flag(#%d): flag 0x%X unsupported\n",
	        thing, flag1);
}

void
btpr_set_flag2(dbref thing, int flag2)
{
	switch (flag2) {
	/* TODO: We have a magic @xtype command now, and every s_Hardcode() is
	 * matched to a handle_xcode(), but there's no need to antagonize
	 * broken code.  Yet.
	 */
	case HARDCODE:
		btshim_set_flag(thing, "XCODE");
		break;

	default:
		fprintf(stderr,
		        "warning: btpr_set_flag2(#%d): flag 0x%X unsupported\n",
		        thing, flag2);
		break;
	}
}

extern void btshim_unset_flag(dbref, const char *);

void
btpr_unset_flag2(dbref thing, int flag2)
{
	switch (flag2) {
	/* TODO: We have a magic @xtype command now, and every c_Hardcode() is
	 * matched to a handle_xcode(), but there's no need to antagonize
	 * broken code.  Yet.
	 */
	case HARDCODE:
		btshim_unset_flag(thing, "XCODE");
		break;

	default:
		fprintf(stderr,
		        "warning: btpr_unset_flag2(#%d): flag 0x%X unsupported\n",
		        thing, flag2);
		break;
	}
}
