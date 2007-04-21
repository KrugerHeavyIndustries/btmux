#include "config.h"
#include "alloc.h"
#include "p.functions.h"


extern int btshim_match_thing(dbref, const char *);

dbref
match_thing(dbref player, char *name)
{
	return btshim_match_thing(player, name);
}


int
fn_range_check(const char *fname, int nfargs, int minargs, int maxargs,
               char *result, char **bufc)
{
	/* nfargs range checking is handled by the PennMUSH parser.  */
	return 1;
}


extern int btshim_xlate(const char *arg);

int
xlate(char *arg)
{
	return btshim_xlate(arg);
}


/* Taken pretty much directly from mux/src/functions.c.  */

/**
 * Converts number to minutes/secs/days
 */

#define UPTIME_UNITS 6

static struct {
	int multip;
	char *name;
	char *sname;
} uptime_unit_table[UPTIME_UNITS] = {
	{
	60 *60 * 24 * 30 * 12, "year", "y"}, {
	60 *60 * 24 * 30, "month", "m"}, {
	60 *60 * 24, "day", "d"}, {
	60 *60, "hour", "h"}, {
	60, "minute", "m"}, {
	1, "second", "s"}
};

char *
get_uptime_to_string(int uptime)
{
	char *buf = alloc_sbuf("get_uptime_to_string");
	int units[UPTIME_UNITS];
	int taim = uptime;
	int ut = 0, uc = 0, foofaa;

	if (uptime <= 0) {
		strcpy(buf, "#-1 INVALID VALUE");
		return buf;
	}

	for (ut = 0; ut < UPTIME_UNITS; ut++)
		units[ut] = 0;

	ut = 0;
	buf[0] = 0;

	while (taim > 0) {
		if ((foofaa = (taim / uptime_unit_table[ut].multip)) > 0) {
			uc++;
			units[ut] = foofaa;
			taim -= uptime_unit_table[ut].multip * foofaa;
		}

		ut++;
	}

	/*
	 * Now, we got it..
	 */
	for (ut = 0; ut < UPTIME_UNITS; ut++) {
		if (units[ut]) {
			uc--;
			if (units[ut] > 1)
				sprintf(buf + strlen(buf), "%d %ss", units[ut],
				        uptime_unit_table[ut].name);
			else
				sprintf(buf + strlen(buf), "%d %s", units[ut],
				        uptime_unit_table[ut].name);

			if (uc > 1)
				strcat(buf, ", ");
			else if (uc > 0)
				strcat(buf, " and ");
		}
	}

	return buf;
}
