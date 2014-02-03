
/*
 * $Id: glue.hcode.c,v 1.1 2005/06/13 20:50:49 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1997 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters 
 *  Copyright (c) 2000-2002 Cord Awtry 
 *       All rights reserved
 *
 * Created: Mon Oct 20 18:53:30 1997 fingon
 * Last modified: Sat Jun  6 18:43:51 1998 fingon
 *
 */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "externs.h"
#include "mech.h"
#include "macros.h"
#include "p.glue.h"

char *silly_atr_get(int id, int flag)
{
	long i, j;
	static char buf[LBUF_SIZE];

	atr_get_str(buf, id, flag, &i, &j);
	return buf;
#if 0							/* This would waste memory, so.. :P */
	return atr_pget(id, flag, &i, &j);
#endif
}

void silly_atr_set(int id, int flag, char *dat)
{
	atr_add_raw(id, flag, dat);
}

void KillText(char **mapt)
{
	int i;

	for(i = 0; mapt[i]; i++)
		free(mapt[i]);
	free(mapt);
}

void ShowText(char **mapt, dbref player)
{
	int i;

	for(i = 0; mapt[i]; i++)
		notify(player, mapt[i]);
}

int BOUNDED(int min, int val, int max)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	return val;
}

float FBOUNDED(float min, float val, float max)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	return val;
}

int MAX(int v1, int v2)
{
	if(v1 > v2)
		return v1;
	return v2;
}

int MIN(int v1, int v2)
{
	if(v1 < v2)
		return v1;
	return v2;
}

/*
 * Gets the first parameter from a string
 * and returns it.
 */
char *first_parseattribute(char *buffer)
{

	int length;
	char *start, *first;

	/* Look for the first parameter */
	start = buffer;
	length = strcspn(start, " \t=");

	/* If the first parameter is to big set the size */
	if(length > SBUF_SIZE)
		length = SBUF_SIZE;

	/* Make it and return it */
	first = (char *) strndup(start, length);

	return first;

}

/*
 * proper_parseattributes
 *
 * Split user input on whitespace delimeters, including '='.
 *
 *
 * This function is designed to be used for commands of the nature
 *
 * COMMAND KEY=VALUE
 *
 * It expects the null terminated source character buffer as the firs
 * argument, buffer.
 *
 * It will duplicate each, space, tab, or equal-sign delimeted field
 * and assign a pointer to it in the string pointer array, args, in
 * ascending order for up to max-1 fields.
 *
 * All remaining input, if any, will be duplicated and the address of
 * duplicate string will be assigned as the last entry in args.
 *
 * For the above example, the result would be a set of strings with
 * the contents:
 *
 * "COMMAND", "=", "KEY", "VALUE"
 *
 * NOTE: it is the caller's responsibility to free the duplicated
 * strings.
 */

int proper_parseattributes(char *buffer, char **args, int max)
{
	int count = 0, iterator = 0, length;
	char *start, *finish;

	memset(args, 0, sizeof(char *) * max);

	start = buffer;
	while (count < (max - 1) && *start) {
		if(*start == '=') {
			args[count++] = strndup(start, 1);
			start++;
			continue;
		}
		length = strcspn(start, " \t=");
		args[count++] = strndup(start, length);
		start += length;
		if(*start != '=' && *start != '\x0')
			start++;
	}
	if(*start) {
		args[max - 1] = strdup(start);
		count++;
	}
	return count;
}

int silly_parseattributes(char *buffer, char **args, int max)
{
	char bufferi[LBUF_SIZE], foobuff[LBUF_SIZE];
	char *a, *b;
	int count = 0;
	char *parsed = buffer;
	int num_args = 0;

	memset(args, 0, sizeof(char *) * max);

	b = bufferi;
	for(a = buffer; *a && a; a++)
		if(*a == '=') {
			*(b++) = ' ';
			*(b++) = '=';
			*(b++) = ' ';
		} else
			*(b++) = *a;
	*b = 0;
	/* Got da silly string in bufferi variable */

	while ((count < max) && parsed) {
		if(!count) {
			/* first time through */
			parsed = strtok(bufferi, " \t");
		} else {
			parsed = strtok(NULL, " \t");
		}
		args[count] = parsed;	/* Set the args pointer */
		if(parsed)
			num_args++;			/* Actual count of arguments */
		count++;				/* Loop to make sure we don't overrun our */
		/* buffer */
	}
	/* Hrm. Now all we gotta do is append -rest- of data to end of _last_ arg */
	if(args[max - 1] && args[max - 1][0]) {
		strcpy(foobuff, args[max - 1]);
		while ((parsed = strtok(NULL, " \t")))
			sprintf(foobuff + strlen(foobuff), " %s", parsed);
		args[max - 1] = foobuff;
	}
	return num_args;
}

/*
 * proper_explodearguments
 *
 * Split user input on whitespace delimeters.
 *
 *
 * This function is designed to be used for commands of the nature
 *
 * COMMAND VALUE1 VALUE2 VALUE3
 *
 * It expects the null terminated source character buffer as the firs
 * argument, buffer.
 *
 * It will duplicate each space or tab delimeted field and assign a 
 * pointer to it in the string pointer array, args, in ascending order 
 * for up to max-1 fields.
 *
 * All remaining input, if any, will be duplicated and the address of
 * duplicate string will be assigned as the last entry in args.
 *
 * For the above example, the result would be a set of strings with
 * the contents:
 *
 * "COMMAND", "VALUE1", "VALUE2", "VALUE3"
 *
 * NOTE: it is the caller's responsibility to free the duplicated
 * strings.
 */


int proper_explodearguments(char *buffer, char **args, int max)
{
	int count = 0, iterator = 0, length;
	char *start, *finish;

	memset(args, 0, sizeof(char *) * max);

	start = buffer;
	while (count < max - 1 && *start) {
		length = strcspn(start, " \t");
		args[count++] = strndup(start, length);
		start += length;
		if(*start != '\x0')
			start++;
	}
	if(*start) {
		args[max - 1] = strdup(start);
		count++;
	}
	return count;
}

int mech_parseattributes(char *buffer, char **args, int maxargs)
{
	int count = 0;
	char *parsed = buffer;
	int num_args = 0;

	memset(args, 0, sizeof(char *) * maxargs);

	while ((count < maxargs) && parsed) {
		parsed = strtok(!count ? buffer : NULL, " \t");
		args[count] = parsed;	/* Set the args pointer */
		if(parsed)
			num_args++;			/* Actual count of arguments */
		count++;				/* Loop to make sure we don't overrun our */
		/* buffer */
	}
	return num_args;
}
