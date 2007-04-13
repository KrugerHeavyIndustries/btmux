#include "db.h"
#include "attrs.h"

#define BROKEN fprintf(stderr, "broken: " __FILE__ ": %s()\n", __FUNCTION__)


extern const char *btshim_Name(dbref);

char *
Name(dbref thing)
{
	const char *tmp_name;

	tmp_name = btshim_Name(thing);

	if (!tmp_name)
		tmp_name = "#-1 INVALID DBREF";

	/* FIXME: Nobody would be stupid enough to try to write into this
	 * memory location... right?
	 */
	return (char *)tmp_name;
}


extern const char *btshim_atr_get_str(dbref, const char *);

/* FIXME: Secure attribute permissions in PennMUSH.  */
char *
atr_get_str(char *s, dbref thing, int atr, dbref *owner, int *flags)
{
	const char *tmp_s;

	/* TODO: Might want to use a look-up table for this instead.  */
	switch (atr) {
	/* FIXME: A_FAIL/A_AFAIL/A_LOCK/A_LENTER are used for locks, so simple
	 * attribute access could be wrong.
	 */

#define CASE_SIMPLE(attr_name) \
	case A_##attr_name: \
		tmp_s = btshim_atr_get_str(thing, #attr_name); \
		break;

	CASE_SIMPLE(MECHPREFID)
	CASE_SIMPLE(MAPCOLOR)

	CASE_SIMPLE(MECHSKILLS)
	CASE_SIMPLE(XTYPE)
	CASE_SIMPLE(TACSIZE)
	CASE_SIMPLE(LRSHEIGHT)

	case A_CONTACTOPT:
		tmp_s = btshim_atr_get_str(thing, "CONTACTOPTIONS");
		break;

	CASE_SIMPLE(MECHNAME)
	CASE_SIMPLE(MECHTYPE) /* MECHREF */
	CASE_SIMPLE(MECHDESC)
	CASE_SIMPLE(MECHSTATUS)
	CASE_SIMPLE(MWTEMPLATE)
	CASE_SIMPLE(FACTION)

#define CASE_STATS(attr_name) \
	case A_##attr_name: \
		tmp_s = btshim_atr_get_str(thing, "PL" #attr_name); \
		break;

	CASE_STATS(HEALTH)
	CASE_STATS(ATTRS)

	CASE_SIMPLE(BUILDLINKS)
	CASE_SIMPLE(BUILDENTRANCE)
	CASE_SIMPLE(BUILDCOORD)

	CASE_STATS(ADVS)

	case A_PILOTNUM:
		tmp_s = btshim_atr_get_str(thing, "PILOT");
		break;

	CASE_SIMPLE(MAPVIS)
	CASE_SIMPLE(TECHTIME)
	CASE_SIMPLE(ECONPARTS)

	CASE_STATS(SKILLS)

	CASE_SIMPLE(PCEQUIP)

	/* A_AMECHDEST and A_AMINETRIGGER are only used by did_it().  */

	default:
		fprintf(stderr,
		        "warning: atr_get_str(#%d): attribute %d unsupported\n",
		        thing, atr);
		tmp_s = "";
		break;

#undef CASE_SIMPLE
#undef CASE_STATS
	}

	strncpy(s, tmp_s, LBUF_SIZE - 1);
	s[LBUF_SIZE - 1] = '\0';

	/* silly_atr_get() ignores owner, flags, and return value, so we set
	 * these to garbage values in case someone tries to use them.
	 */
	*owner = NOTHING;
	*flags = -1;
	return NULL;
}

extern void btshim_atr_add_raw(dbref, const char *, const char *);

/* FIXME: Secure attribute permissions in PennMUSH.  */
void
atr_add_raw(dbref thing, int atr, char *buff)
{
	/* TODO: Might want to use a look-up table for this instead.  */
	switch (atr) {
	/* FIXME: A_LOCK is used for locks, so simple attribute access could be
	 * wrong.
	 */

#define CASE_SIMPLE(attr_name) \
	case A_##attr_name: \
		btshim_atr_add_raw(thing, #attr_name, buff); \
		break;

	CASE_SIMPLE(XTYPE)
	CASE_SIMPLE(MECHNAME)
	CASE_SIMPLE(MECHTYPE) /* MECHREF */

#define CASE_STATS(attr_name) \
	case A_##attr_name: \
		btshim_atr_add_raw(thing, "PL" #attr_name, buff); \
		break;

	CASE_STATS(HEALTH)
	CASE_STATS(ATTRS)

	CASE_STATS(ADVS)

	CASE_SIMPLE(TECHTIME)
	CASE_SIMPLE(ECONPARTS)

	default:
		fprintf(stderr,
		        "warning: atr_add_raw(#%d): attribute %d unsupported\n",
		        thing, atr);
		break;

#undef CASE_SIMPLE
#undef CASE_STATS
	}
}


extern void handle_xcode(dbref, dbref, int, int);

int
btpr_update_xtype(dbref player, dbref thing, const char *xtype)
{
	/* FIXME: handle_xcode() does some stupid stuff with just setting the
	 * XCODE flag around special object disposal.  In fact, it seems a bit
	 * redundant to have from/to and s_Hardcode()/c_Hardcode() right before
	 * handle_xcode().
	 *
	 * handle_xcode() has three basic cases:
	 *
	 * from  to   action
	 * 0     0    Not XCODE, do nothing.
	 * 0     1    Create XCODE object.
	 * 1     0    Destroy XCODE object.
	 * 1     1    Already XCODE, do nothing.
	 *
	 * By the way, using the btshim_atr_add_raw() would be more efficient
	 * than using the MUX interfaces, but it's more platform consistent.
	 */
	if (!xtype || !*xtype) {
		/* Clear XTYPE.  */
		if (!Hardcode(thing)) {
			/* Not XCODE object.  */
			return 0;
		} else {
			/* Clear XCODE flag.  */
			/* handle_xcode() sets then clears XCODE.  */
		}

		handle_xcode(player, thing, 1, 0);
		atr_add_raw(thing, A_XTYPE, NULL);
		return 1; /* TODO: clearing always works? */
	} else {
		/* Set XTYPE.  */
		if (Hardcode(thing)) {
			/* Clear old XTYPE.  */
			handle_xcode(player, thing, 1, 0);
		}

		s_Hardcode(thing);
		atr_add_raw(thing, A_XTYPE, (char *)xtype);
		handle_xcode(player, thing, 0, 1);

		if (!Hardcode(thing)) {
			/* Looks like we failed.  */
			/* XXX: We don't need to clean up, but meh.  */
			atr_add_raw(thing, A_XTYPE, NULL);
			return -1;
		}
		return 1; /* FIXME: report errors */
	}
}
