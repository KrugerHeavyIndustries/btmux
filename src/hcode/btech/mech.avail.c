
/*
 * $Id: mech.avail.c,v 1.1.1.1 2005/01/11 21:18:11 kstevens Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Fri Nov  8 19:53:17 1996 fingon
 * Last modified: Sat Jun  6 22:11:41 1998 fingon
 *
 */

/* Code to use the availability headers made by Nim */

#include <stdio.h>

#include "mech.h"
#include "mech.avail.h"
#include "coolmenu.h"
#include "mycool.h"
#include "p.template.h"
#include "p.mech.mechref_ident.h"

#define IsMech(i) ((i) < NUM_MECHA)
#define IsAero(i) ((i) >= NUM_MECHA && (i) < (NUM_MECHA + NUM_AEROS))

#define Table(id,fi) \
(IsMech(id) ? mech_availability[id].fi : \
 IsAero(id) ? aero_availability[id - NUM_MECHA].fi : 0)

static int find_score(int table, int id)
{
	switch (table) {
#undef Map
#define Map(a,b) case a: return Table(id,b); break;
		Map(FAC_FS, FS);
		Map(FAC_LC, LC);
		Map(FAC_DC, DC);
		Map(FAC_CC, CC);
		Map(FAC_FWL, FWL);
		Map(FAC_MISC, MISC);
		Map(FAC_MERC, MERC);
	case FAC_FC:
		return (find_score(FAC_FS, id) + find_score(FAC_LC, id)) / 2;
#undef Map
	}
	return 0;
}

/* Indeksointi:
   0 - NUM_MECHA-1                   = Mechs
   NUM_MECHA - NUM_MECHA+NUM_AEROS-1 = Aeros
 */

#define STACK_SIZE NUM_MECHA + NUM_AEROS

static void mech_list_maker(dbref player, int table, int types, int tons,
							int opt, int max, int mode, char *tbuf)
{
	int mech_stack[STACK_SIZE][2];
	int i, j, k;
	int tp = 0;
	int dif;
	int count = 0, ftons = 0, otons = tons;
	coolmenu *c = NULL;
	const char *d;

	bzero(mech_stack, sizeof(mech_stack));
	for(i = 0; i < STACK_SIZE; i++) {
		if(!(types & 1) && IsMech(i))
			continue;
		if(!(types & 2) && IsAero(i))
			continue;
		j = find_score(table, i);
		if(opt) {
			if((dif = abs(Table(i, tons) - opt)) > max && max)
				continue;
			j = j * (100 - dif) * (100 - dif);
		} else
			j = j * 10000;
		if(!j)
			continue;
		tp += j;
		mech_stack[i][0] = j;
	}
	if(!tp) {
		if(!mode)
			notify(player, "No 'mechs matching that criteria!");
		else
			strcpy(tbuf, tprintf("#-1 ERROR: NO UNITS WITH FLAG %d FOUND",
								 types));
		return;
	}
	/* Ok.. We've made the table. Time to use it */
	while ((tons >= MAX(20, opt - max))) {
		i = Number(1, tp);
		for(j = 0; j < STACK_SIZE; j++) {
			i -= mech_stack[j][0];
			if(i <= 0)
				break;
		}
		if((k = Table(j, tons)) > tons)
			continue;
		mech_stack[j][1]++;
		tons -= k;
		count++;
		ftons += k;
	}
	/* We've a mech list. Time to show it :> */
	if(mode == 0) {
		addline();
		sim(tprintf("Mechs/Aeros/Whatever made for %s (%d tons)",
					side_names[table], otons), CM_ONE | CM_CENTER);
		addline();
		if(!count)
			vsi("No mechs created? (Too low tonnage?)");
		else {
			vsi(tprintf("%-4s %-10s %-20s %-6s %-s", "Tons", "Ref", "Name",
						"Chance", "Count"));
			for(i = 0; i < STACK_SIZE; i++)
				if(mech_stack[i][1]) {
					d = find_mechname_by_mechref(Table(i, name));
					vsi(tprintf("%4d %-10s %-20s %6.2f %d", Table(i, tons),
								Table(i, name), d ? d : "Unknown",
								(float) 100.0 * mech_stack[i][0] / tp,
								mech_stack[i][1]));

				}
			addline();
			vsi(tprintf("Avg weight: %.2f Total tons: %d",
						(float) ftons / count, ftons));
		}
		addline();
		ShowCoolMenu(player, c);
		KillCoolMenu(c);
	} else {
		*tbuf = 0;
		for(i = 0; i < STACK_SIZE; i++)
			for(j = 0; j < mech_stack[i][1]; j++)
				sprintf(tbuf + strlen(tbuf), "%s ", Table(i, name));
		if(*tbuf)
			tbuf[strlen(tbuf) - 1] = 0;
	}
}

void debug_makemechs(dbref player, void *data, char *buffer)
{
	char *args[7];
	int argc;
	int table;
	int tons;
	int opt = 0;
	int max = 0;
	int types = 0xff;

	argc = mech_parseattributes(buffer, args, 6);
	DOCHECK(argc < 2, "Insufficient arguments!");
	DOCHECK(argc > 5, "Too many arguments!");
	DOCHECK((table =
			 compare_array(side_names_short, args[0])) < 0,
			"Invalid faction name!");
	DOCHECK((tons = atoi(args[1])) < 20, "Invalid tonnage!");
	DOCHECK(tons > 4000, "Max of 4000 tons of mecha at once! Sowwy!");
	if(argc > 2) {
		DOCHECK(Readnum(types, args[2]), "Invalid type bitvector!");
		if(argc > 3) {
			DOCHECK((opt = atoi(args[3])) < 20, "Invalid optTonnage!");
			if(argc > 4)
				DOCHECK((max = atoi(args[4])) < 5, "Invalid MaxDifference!");
		}
	}
	mech_list_maker(player, table, types, tons, opt, max, 0, NULL);
}

#define FUNCHECK(a,b) \
if (a) { safe_tprintf_str(buff, bufc, b); return; }

void fun_btmakemechsub(char *buff, char **bufc, dbref player, dbref cause,
					 char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	/* fargs[0] = faction 
	   fargs[1] = numtons
	   fargs[2] = types
	   fargs[3] = opttons
	   fargs[4] = maxvar 
	 */
	int table;
	int tons;
	int opt = 0;
	int max = 0;
	char buf[LBUF_SIZE];
	int types = 0xff;

	FUNCHECK(nfargs < 2, "#-1 Insufficient arguments!");
	FUNCHECK(nfargs > 5, "#-1 Too many arguments!");
	FUNCHECK((table =
			  compare_array(side_names_short, fargs[0])) < 0,
			 "#-1 Invalid faction name!");
	FUNCHECK((tons = atoi(fargs[1])) < 20, "#-1 Invalid tonnage!");
	FUNCHECK(tons > 4000, "#-1 Max of 4000 tons of mecha at once! Sowwy!");
	if(nfargs > 2) {
		FUNCHECK(Readnum(types, fargs[2]), "#-1 Invalid type bitvector!");
		if(nfargs > 3) {
			FUNCHECK((opt = atoi(fargs[3])) < 20, "#-1 Invalid optTonnage!");
			if(nfargs > 4)
				FUNCHECK((max =
						  atoi(fargs[4])) < 5, "#-1 Invalid MaxDifference!");
		}
	}
	mech_list_maker(player, table, types, tons, opt, max, 1, buf);
	safe_tprintf_str(buff, bufc, "%s", buf);
}
