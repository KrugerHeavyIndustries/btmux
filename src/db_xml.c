/*
 * db_rw.c 
 */

#include "copyright.h"
#include "config.h"

#include <sys/file.h>

#include "mudconf.h"
#include "config.h"
#include "externs.h"
#include "db.h"
#include "vattr.h"
#include "attrs.h"
#include "alloc.h"
#include "powers.h"

#define MUX_REPRESENTATION
// #define REASONABLE_REPRESENTATION

extern const char *getstring_noalloc(FILE *, int);
extern void putstring(FILE *, const char *);
extern void db_grow(dbref);

extern struct object *db;

static int g_version;
static int g_format;
static int g_flags;

static int xml_putescaped(FILE * f, const char *string)
{
	char emit_buffer[LBUF_SIZE * 7];
	char *r, *s;
	s = emit_buffer;
	memset(emit_buffer, 0, sizeof(emit_buffer));
	for(r = (char *)string; *r; r++) {
		switch (*r) {
		case '"':
			s = stpcpy(s, "&quot;");
			break;
		case '\'':
			s = stpcpy(s, "&apos;");
			break;
		case '&':
			s = stpcpy(s, "&amp;");
		case '<':
			s = stpcpy(s, "&lt;");
			break;
		case '>':
			s = stpcpy(s, "&gt;");
			break;
		case '\\':
			s = stpcpy(s, "\\\\");
			break;
		default:
			*s++ = *r;
		}
	}
	*s = '\0';
	return fprintf(f, "%s", emit_buffer);
}

static int xml_putobjstring(FILE * f, const char *name, const char *value)
{
	fprintf(f, "\t\t<%s>", name);
	xml_putescaped(f, value);
	fprintf(f, "</%s>\n", name);
	return 1;
}

static int xml_putobjref(FILE * f, const char *name, long value)
{
	return fprintf(f, "\t\t<%s>%ld</%s>\n", name, value, name);
}

static int xml_putattr(FILE * f, const char *name, const char *value, long owner,
					   long flags)
{
	fprintf(f, "\t\t<Attribute name=\"");
	xml_putescaped(f, name);
	fprintf(f, "\" owner=\"%ld\" flags=\"%ld\">", owner, flags);
	xml_putescaped(f, value);
	fprintf(f, "</Attribute>\n");
	return 1;
}

static int xml_db_write_object(FILE * f, dbref i, int db_format, int flags)
{
	ATTR *a;
	char *got, *as;
	dbref aowner;
	long sub;
	int ca, save, j; long aflags;
	BOOLEXP *tempbool;

	fprintf(f, "\t<Object dbref=\"%ld\">\n", (long) i);
	xml_putobjstring(f, "Name", Name(i));
	xml_putobjref(f, "Location", Location(i));
	xml_putobjref(f, "Zone", Zone(i));
	if(Contents(i) == -1) {
		fprintf(f, "\t\t<Contents/>\n");
	} else {
		fprintf(f, "\t\t<Contents>\n");
		sub = Contents(i);
		while (sub != -1) {
			fprintf(f, "\t\t\t<DBREF dbref=\"%ld\"/>\n", sub);
			sub = Next(sub);
		}
		fprintf(f, "\t\t</Contents>\n");
	}
	if(Exits(i) == -1) {
		fprintf(f, "\t\t<Exits/>\n");
	} else {
		fprintf(f, "\t\t<Exits>\n");
		sub = Exits(i);
		while (sub != -1) {
			fprintf(f, "\t\t\t<DBREF dbref=\"%ld\"/>\n", sub);
			sub = Next(sub);
		}
		fprintf(f, "\t\t</Exits>\n");
	}
	got = atr_get(i, A_LOCK, &aowner, &aflags);
	xml_putobjstring(f, "Lock", got);
	free_lbuf(got);

	xml_putobjref(f, "Link", Link(i));
	xml_putobjref(f, "Owner", Owner(i));
	xml_putobjref(f, "Parent", Parent(i));
	xml_putobjref(f, "Pennies", Pennies(i));
	xml_putobjref(f, "Flags", Flags(i));
	xml_putobjref(f, "Flags2", Flags2(i));
	xml_putobjref(f, "Flags3", Flags3(i));
	xml_putobjref(f, "Powers", Powers(i));
	xml_putobjref(f, "Powers2", Powers2(i));

	for(ca = atr_head(i, &as); ca; ca = atr_next(&as)) {
		save = 0;
		a = atr_num(ca);
		if(a)
			j = a->number;
		else
			j = -1;

		if(j > 0) {
			switch (j) {
			case A_NAME:
				if(flags & V_ATRNAME)
					save = 1;
				break;
			case A_LOCK:
				if(flags & V_ATRKEY)
					save = 1;
				break;
			case A_LIST:
			case A_MONEY:
				break;
			default:
				save = 1;
			}
		}
		if(save) {
			got = atr_get(i, j, &aowner, &aflags);
			xml_putattr(f, a->name, got, aowner, aflags);
		}
	}
	fprintf(f, "\t</Object>\n");
	return 0;
}

static int xml_db_write_mux(FILE * f, dbref i, int db_format, int flags)
{
	ATTR *a;
	char *got, *as;
	dbref aowner;
	long sub;
	int ca, save, j; long aflags;
	BOOLEXP *tempbool;

	fprintf(f, "\t<Object dbref=\"%ld\">\n", (long) i);
	xml_putobjstring(f, "Name", Name(i));
	xml_putobjref(f, "Location", Location(i));
	xml_putobjref(f, "Zone", Zone(i));
	xml_putobjref(f, "Contents", Contents(i));
	xml_putobjref(f, "Exits", Exits(i));
	got = atr_get(i, A_LOCK, &aowner, &aflags);
	xml_putobjstring(f, "Lock", got);
	free_lbuf(got);

	xml_putobjref(f, "Link", Link(i));
	xml_putobjref(f, "Owner", Owner(i));
	xml_putobjref(f, "Parent", Parent(i));
	xml_putobjref(f, "Pennies", Pennies(i));
	xml_putobjref(f, "Flags", Flags(i));
	xml_putobjref(f, "Flags2", Flags2(i));
	xml_putobjref(f, "Flags3", Flags3(i));
	xml_putobjref(f, "Powers", Powers(i));
	xml_putobjref(f, "Powers2", Powers2(i));
	for(ca = atr_head(i, &as); ca; ca = atr_next(&as)) {
		save = 0;
		a = atr_num(ca);
		if(a)
			j = a->number;
		else
			j = -1;

		if(j > 0) {
			switch (j) {
			case A_NAME:
				if(flags & V_ATRNAME)
					save = 1;
				break;
			case A_LOCK:
				if(flags & V_ATRKEY)
					save = 1;
				break;
			case A_LIST:
			case A_MONEY:
				break;
			default:
				save = 1;
			}
		}
		if(save) {
			got = atr_get(i, j, &aowner, &aflags);
			xml_putattr(f, a->name, got, aowner, aflags);
		}
	}
	fprintf(f, "\t</Object>\n");
	return 0;
}

dbref xml_db_write(FILE * f, int format, int version)
{
	dbref i;
	int flags;
	VATTR *vp;

	fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n");
	fprintf(f, "<TinyMUXDataBase dumptime=\"%lu\">\n", mudstate.now);
	DO_WHOLE_DB(i) {
		if(!(Going(i))) {
#ifdef MUX_REPRESENTATION
			xml_db_write_mux(f, i, format, flags);
#else
			xml_db_write_object(f, i, format, flags);
#endif
		}
	}
	fprintf(f, "</TinyMUXDataBase>\n");
	fflush(f);
	return (mudstate.db_top);
}
