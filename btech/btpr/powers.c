#include "mudconf.h"
#include "powers.h"

extern int btshim_has_power(dbref, const char *);

int
btpr_has_power2(dbref thing, int power2)
{
	switch (power2) {
	case POW_MECH:
		/* FIXME */
	case POW_SECURITY:
		/* FIXME */
	case POW_MECHREP:
		/* FIXME */
	case POW_MAP:
		/* FIXME */
	case POW_TEMPLATE:
		/* FIXME */
	case POW_TECH:
		/* FIXME */
	default:
		fprintf(stderr,
		        "warning: btpr_has_power2(#%d): power 0x%X unsupported\n",
		        thing, power2);
		return 0;
	}
}
