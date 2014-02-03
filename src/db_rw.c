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

extern const char *getstring_noalloc(FILE *, int);
extern void putstring(FILE *, const char *);
extern void db_grow(dbref);

extern struct object *db;

static int g_version;
static int g_format;
static int g_flags;

/*
 * ---------------------------------------------------------------------------
 * * getboolexp1: Get boolean subexpression from file.
 */

static BOOLEXP *getboolexp1(FILE * f)
{
	BOOLEXP *b;
	char *buff, *s;
	int c, d, anum;

	c = getc(f);
	switch (c) {
	case '\n':
		ungetc(c, f);
		return TRUE_BOOLEXP;
		/*
		 * break; 
		 */
	case EOF:
		abort();				/*
								 * unexpected EOF in boolexp 
								 */
		break;
	case '(':
		b = alloc_bool("getboolexp1.openparen");
		switch (c = getc(f)) {
		case NOT_TOKEN:
			b->type = BOOLEXP_NOT;
			b->sub1 = getboolexp1(f);
			if((d = getc(f)) == '\n')
				d = getc(f);
			if(d != ')')
				goto error;
			return b;
		case INDIR_TOKEN:
			b->type = BOOLEXP_INDIR;
			b->sub1 = getboolexp1(f);
			if((d = getc(f)) == '\n')
				d = getc(f);
			if(d != ')')
				goto error;
			return b;
		case IS_TOKEN:
			b->type = BOOLEXP_IS;
			b->sub1 = getboolexp1(f);
			if((d = getc(f)) == '\n')
				d = getc(f);
			if(d != ')')
				goto error;
			return b;
		case CARRY_TOKEN:
			b->type = BOOLEXP_CARRY;
			b->sub1 = getboolexp1(f);
			if((d = getc(f)) == '\n')
				d = getc(f);
			if(d != ')')
				goto error;
			return b;
		case OWNER_TOKEN:
			b->type = BOOLEXP_OWNER;
			b->sub1 = getboolexp1(f);
			if((d = getc(f)) == '\n')
				d = getc(f);
			if(d != ')')
				goto error;
			return b;
		default:
			ungetc(c, f);
			b->sub1 = getboolexp1(f);
			if((c = getc(f)) == '\n')
				c = getc(f);
			switch (c) {
			case AND_TOKEN:
				b->type = BOOLEXP_AND;
				break;
			case OR_TOKEN:
				b->type = BOOLEXP_OR;
				break;
			default:
				goto error;
			}
			b->sub2 = getboolexp1(f);
			if((d = getc(f)) == '\n')
				d = getc(f);
			if(d != ')')
				goto error;
			return b;
		}
	case '-':					/*
								 * obsolete NOTHING key, eat it 
								 */
		while ((c = getc(f)) != '\n')
			if(c == EOF)
				abort();		/*
								 * unexp EOF 
								 */
		ungetc(c, f);
		return TRUE_BOOLEXP;
		break;
	case '"':
		ungetc(c, f);
		buff = alloc_lbuf("getboolexp_quoted");
		StringCopy(buff, getstring_noalloc(f, 1));
		c = fgetc(f);
		if(c == EOF) {
			free_lbuf(buff);
			return TRUE_BOOLEXP;
		}

		b = alloc_bool("getboolexp1_quoted");
		anum = mkattr(buff);
		if(anum <= 0) {
			free_bool(b);
			free_lbuf(buff);
			goto error;
		}
		free_lbuf(buff);
		b->thing = anum;

		/*
		 * if last character is : then this is an attribute lock. A 
		 * last character of / means an eval lock 
		 */

		if((c == ':') || (c == '/')) {
			if(c == '/')
				b->type = BOOLEXP_EVAL;
			else
				b->type = BOOLEXP_ATR;
			buff = alloc_lbuf("getboolexp1.attr_lock");
			StringCopy(buff, getstring_noalloc(f, 1));
			b->sub1 = (BOOLEXP *) strsave(buff);
			free_lbuf(buff);
		}
		return b;
	default:					/*
								 * dbref or attribute 
								 */
		ungetc(c, f);
		b = alloc_bool("getboolexp1.default");
		b->type = BOOLEXP_CONST;
		b->thing = 0;

		/*
		 * This is either an attribute, eval, or constant lock.
		 * Constant locks are of the form <num>, while
		 * attribute * and * * * * eval locks are of the form
		 * <anam-or-anum>:<string> or
		 * <aname-or-anum>/<string> respectively. The
		 * characters <nl>, |, and & terminate the string. 
		 */

		if(isdigit(c)) {
			while (isdigit(c = getc(f))) {
				b->thing = b->thing * 10 + c - '0';
			}
		} else if(isalpha(c)) {
			buff = alloc_lbuf("getboolexp1.atr_name");
			for(s = buff;
				((c = getc(f)) != EOF) && (c != '\n') && (c != ':') &&
				(c != '/'); *s++ = c);
			if(c == EOF) {
				free_lbuf(buff);
				free_bool(b);
				goto error;
			}
			*s = '\0';

			/*
			 * Look the name up as an attribute.  If not found,
			 * create a new attribute. 
			 */

			anum = mkattr(buff);
			if(anum <= 0) {
				free_bool(b);
				free_lbuf(buff);
				goto error;
			}
			free_lbuf(buff);
			b->thing = anum;
		} else {
			free_bool(b);
			goto error;
		}

		/*
		 * if last character is : then this is an attribute lock. A 
		 * last character of / means an eval lock 
		 */

		if((c == ':') || (c == '/')) {
			if(c == '/')
				b->type = BOOLEXP_EVAL;
			else
				b->type = BOOLEXP_ATR;
			buff = alloc_lbuf("getboolexp1.attr_lock");
			for(s = buff;
				((c = getc(f)) != EOF) && (c != '\n') && (c != ')') &&
				(c != OR_TOKEN) && (c != AND_TOKEN); *s++ = c);
			if(c == EOF)
				goto error;
			*s++ = 0;
			b->sub1 = (BOOLEXP *) strsave(buff);
			free_lbuf(buff);
		}
		ungetc(c, f);
		return b;
	}

  error:
	abort();					/*
								 * bomb out 
								 */
	return TRUE_BOOLEXP;
}

/*
 * ---------------------------------------------------------------------------
 * * getboolexp: Read a boolean expression from the flat file.
 */

static BOOLEXP *getboolexp(FILE * f)
{
	BOOLEXP *b;
	char c;

	b = getboolexp1(f);
	if(getc(f) != '\n')
		abort();				/*
								 * parse error, we lose 
								 */

	/*
	 * MUSH (except for PernMUSH) and MUSE can have an extra CR, * MUD *
	 * * * * * does not. 
	 */

	if(((g_format == F_MUSH) && (g_version != 2)) || (g_format == F_MUSE)
	   || (g_format == F_MUX)) {
		if((c = getc(f)) != '\n')
			ungetc(c, f);
	}
	return b;
}

/*
 * ---------------------------------------------------------------------------
 * * get_list: Read attribute list from flat file.
 */

static int get_list(FILE * f, dbref i, int new_strings)
{
	dbref atr;
	int c;
	char *buff;

	buff = alloc_lbuf("get_list");
	while (1) {
		switch (c = getc(f)) {
		case '>':				/*
								 * read # then string 
								 */
			atr = getref(f);
			if(atr > 0) {
				/*
				 * Store the attr 
				 */

				atr_add_raw(i, atr, (char *) getstring_noalloc(f,
															   new_strings));
			} else {
				/*
				 * Silently discard 
				 */

				getstring_noalloc(f, new_strings);
			}
			break;
		case '\n':				/*
								 * ignore newlines. They're due to v(r). 
								 */
			break;
		case '<':				/*
								 * end of list 
								 */
			free_lbuf(buff);
			c = getc(f);
			if(c != '\n') {
				ungetc(c, f);
				fprintf(stderr, "No line feed on object %ld\n", i);
				return 1;
			}
			return 1;
		default:
			fprintf(stderr,
					"Bad character '%c' when getting attributes on object %ld\n",
					c, i);
			/*
			 * We've found a bad spot.  I hope things aren't * *
			 * * * * * too bad. 
			 */

			(void) getstring_noalloc(f, new_strings);
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * * putbool_subexp: Write a boolean sub-expression to the flat file.
 */
static void putbool_subexp(FILE * f, BOOLEXP * b)
{
	ATTR *va;

	switch (b->type) {
	case BOOLEXP_IS:
		putc('(', f);
		putc(IS_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;
	case BOOLEXP_CARRY:
		putc('(', f);
		putc(CARRY_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;
	case BOOLEXP_INDIR:
		putc('(', f);
		putc(INDIR_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;
	case BOOLEXP_OWNER:
		putc('(', f);
		putc(OWNER_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;
	case BOOLEXP_AND:
		putc('(', f);
		putbool_subexp(f, b->sub1);
		putc(AND_TOKEN, f);
		putbool_subexp(f, b->sub2);
		putc(')', f);
		break;
	case BOOLEXP_OR:
		putc('(', f);
		putbool_subexp(f, b->sub1);
		putc(OR_TOKEN, f);
		putbool_subexp(f, b->sub2);
		putc(')', f);
		break;
	case BOOLEXP_NOT:
		putc('(', f);
		putc(NOT_TOKEN, f);
		putbool_subexp(f, b->sub1);
		putc(')', f);
		break;
	case BOOLEXP_CONST:
		fprintf(f, "%ld", b->thing);
		break;
	case BOOLEXP_ATR:
		va = atr_num(b->thing);
		if(va) {
			fprintf(f, "%s:%s", va->name, (char *) b->sub1);
		} else {
			fprintf(f, "%ld:%s\n", b->thing, (char *) b->sub1);
		}
		break;
	case BOOLEXP_EVAL:
		va = atr_num(b->thing);
		if(va) {
			fprintf(f, "%s/%s\n", va->name, (char *) b->sub1);
		} else {
			fprintf(f, "%ld/%s\n", b->thing, (char *) b->sub1);
		}
		break;
	default:
		fprintf(stderr, "Unknown boolean type in putbool_subexp: %d\n",
				b->type);
	}
}

/*
 * ---------------------------------------------------------------------------
 * * putboolexp: Write boolean expression to the flat file.
 */

static void putboolexp(FILE * f, BOOLEXP * b)
{
	if(b != TRUE_BOOLEXP) {
		putbool_subexp(f, b);
	}
	putc('\n', f);
}

dbref db_read(FILE * f, int *db_format, int *db_version, int *db_flags)
{
	dbref i, anum;
	char ch;
	const char *tstr;
	int header_gotten, size_gotten, nextattr_gotten;
	int read_attribs, read_name, read_zone, read_link, read_key, read_parent;
	int read_extflags, read_3flags, read_money, read_timestamps,
		read_new_strings;
	int read_powers, read_powers_player, read_powers_any;
	int deduce_version, deduce_name, deduce_zone, deduce_timestamps;
	int aflags, f1, f2, f3;
	BOOLEXP *tempbool;

	header_gotten = 0;
	size_gotten = 0;
	nextattr_gotten = 0;
	g_format = F_UNKNOWN;
	g_version = 0;
	g_flags = 0;
	read_attribs = 1;
	read_name = 1;
	read_zone = 0;
	read_link = 0;
	read_key = 1;
	read_parent = 0;
	read_money = 1;
	read_extflags = 0;
	read_3flags = 0;
	read_timestamps = 0;
	read_new_strings = 0;
	read_powers = 0;
	read_powers_player = 0;
	read_powers_any = 0;
	deduce_version = 1;
	deduce_zone = 1;
	deduce_name = 1;
	deduce_timestamps = 1;
	db_free();
	for(i = 0;; i++) {

		switch (ch = getc(f)) {
		case '-':				/* Misc tag */
			switch (ch = getc(f)) {
			case 'R':			/* Record number of players */
				mudstate.record_players = getref(f);
				break;
			default:
				(void) getstring_noalloc(f, 0);
			}
			break;
		case '+':				/*
								 * MUX and MUSH header 
								 */
			switch (ch = getc(f)) {	/*
									 * 2nd char selects 
									 * type 
									 */
			case 'X':			/*
								 * MUX VERSION 
								 */
				if(header_gotten) {
					fprintf(stderr,
							"\nDuplicate MUX version header entry at object %ld, ignored.\n",
							i);
					tstr = getstring_noalloc(f, 0);
					break;
				}
				header_gotten = 1;
				deduce_version = 0;
				g_format = F_MUX;
				g_version = getref(f);

				/*
				 * Otherwise extract feature flags 
				 */

				if(g_version & V_GDBM) {
					read_attribs = 0;
					read_name = !(g_version & V_ATRNAME);
				}
				read_zone = (g_version & V_ZONE);
				read_link = (g_version & V_LINK);
				read_key = !(g_version & V_ATRKEY);
				read_parent = (g_version & V_PARENT);
				read_money = !(g_version & V_ATRMONEY);
				read_extflags = (g_version & V_XFLAGS);
				read_3flags = (g_version & V_3FLAGS);
				read_powers = (g_version & V_POWERS);
				read_new_strings = (g_version & V_QUOTED);
				g_flags = g_version & ~V_MASK;

				g_version &= V_MASK;
				deduce_name = 0;
				deduce_version = 0;
				deduce_zone = 0;
				break;
			case 'S':			/*
								 * SIZE 
								 */
				if(size_gotten) {
					fprintf(stderr,
							"\nDuplicate size entry at object %ld, ignored.\n",
							i);
					tstr = getstring_noalloc(f, 0);
				} else {
					mudstate.min_size = getref(f);
				}
				size_gotten = 1;
				break;
			case 'A':			/*
								 * USER-NAMED ATTRIBUTE 
								 */
				anum = getref(f);
				tstr = getstring_noalloc(f, read_new_strings);
				if(isdigit(*tstr)) {
					aflags = 0;
					while (isdigit(*tstr))
						aflags = (aflags * 10) + (*tstr++ - '0');
					tstr++;		/*
								 * skip ':' 
								 */
				} else {
					aflags = mudconf.vattr_flags;
				}
				vattr_define((char *) tstr, anum, aflags);
				break;
			case 'F':			/*
								 * OPEN USER ATTRIBUTE SLOT 
								 */
				anum = getref(f);
				break;
			case 'N':			/*
								 * NEXT ATTR TO ALLOC WHEN NO
								 * FREELIST 
								 */
				if(nextattr_gotten) {
					fprintf(stderr,
							"\nDuplicate next free vattr entry at object %ld, ignored.\n",
							i);
					tstr = getstring_noalloc(f, 0);
				} else {
					mudstate.attr_next = getref(f);
					nextattr_gotten = 1;
				}
				break;
			default:
				fprintf(stderr,
						"\nUnexpected character '%c' in MUX header near object #%ld, ignored.\n",
						ch, i);
				tstr = getstring_noalloc(f, 0);
			}
			break;
		case '!':				/*
								 * MUX entry/MUSH entry/MUSE non-zoned entry 
								 */
			if(deduce_version) {
				g_format = F_MUX;
				g_version = 1;
				deduce_name = 0;
				deduce_zone = 0;
				deduce_version = 0;
			} else if(deduce_zone) {
				deduce_zone = 0;
				read_zone = 0;
			}
			i = getref(f);
			db_grow(i + 1);

			if(read_name) {
				tstr = getstring_noalloc(f, read_new_strings);
				if(deduce_name) {
					if(isdigit(*tstr)) {
						read_name = 0;
						s_Location(i, atoi(tstr));
					} else {
						s_Name(i, (char *) tstr);
						s_Location(i, getref(f));
					}
					deduce_name = 0;
				} else {
					s_Name(i, (char *) tstr);
					s_Location(i, getref(f));
				}
			} else {
				s_Location(i, getref(f));
			}

			/*
			 * ZONE on MUSE databases and some others 
			 */

			if(read_zone)
				s_Zone(i, getref(f));

			/*
			 * else
			 * * s_Zone(i, NOTHING); 
			 */

			/*
			 * CONTENTS and EXITS 
			 */

			s_Contents(i, getref(f));
			s_Exits(i, getref(f));

			/*
			 * LINK 
			 */

			if(read_link)
				s_Link(i, getref(f));
			else
				s_Link(i, NOTHING);

			/*
			 * NEXT 
			 */

			s_Next(i, getref(f));

			/*
			 * LOCK
			 */

			if(read_key) {
				tempbool = getboolexp(f);
				atr_add_raw(i, A_LOCK, unparse_boolexp_quiet(1, tempbool));
				free_boolexp(tempbool);
			}
			/*
			 * OWNER 
			 */

			s_Owner(i, getref(f));

			/*
			 * PARENT: PennMUSH uses this field for ZONE
			 * (which we  use as PARENT if we
			 * didn't already read in a  
			 * non-NOTHING parent. 
			 */

			if(read_parent) {
				s_Parent(i, getref(f));
			} else {
				s_Parent(i, NOTHING);
			}

			/*
			 * PENNIES 
			 */

			if(read_money)		/*
								 *  if not fix in
								 * unscraw_foreign  
								 */
				s_Pennies(i, getref(f));

			/*
			 * FLAGS 
			 */

			f1 = getref(f);
			if(read_extflags)
				f2 = getref(f);
			else
				f2 = 0;

			if(read_3flags)
				f3 = getref(f);
			else
				f3 = 0;

			s_Flags(i, f1);
			s_Flags2(i, f2);
			s_Flags3(i, f3);

			if(read_powers) {
				f1 = getref(f);
				f2 = getref(f);
				s_Powers(i, f1);
				s_Powers2(i, f2);
			}

			/*
			 * ATTRIBUTES 
			 */

			if(read_attribs) {
				if(!get_list(f, i, read_new_strings)) {
					fprintf(stderr,
							"\nError reading attrs for object #%ld\n", i);
					return -1;
				}
			}
			/*
			 * check to see if it's a player 
			 */

			if(Typeof(i) == TYPE_PLAYER) {
				c_Connected(i);
			}
			break;
		case '*':				/*
								 * EOF marker 
								 */
			tstr = getstring_noalloc(f, 0);
			if(strcmp(tstr, "**END OF DUMP***")) {
				fprintf(stderr, "\nBad EOF marker at object #%ld\n", i);
				return -1;
			} else {
				/*
				 * Fix up bizarro foreign DBs 
				 */

				*db_version = g_version;
				*db_format = g_format;
				*db_flags = g_flags;
				load_player_names();
				return mudstate.db_top;
			}
		default:
			fprintf(stderr, "\nIllegal character '%c' near object #%ld\n",
					ch, i);
			return -1;
		}

	}

}

static int db_write_object(FILE * f, dbref i, int db_format, int flags)
{
	ATTR *a;
	char *got, *as;
	dbref aowner;
	int ca, save, j; long aflags;
	BOOLEXP *tempbool;

	if(!(flags & V_ATRNAME))
		putstring(f, Name(i));
	putref(f, Location(i));
	if(flags & V_ZONE)
		putref(f, Zone(i));
	putref(f, Contents(i));
	putref(f, Exits(i));
	if(flags & V_LINK)
		putref(f, Link(i));
	putref(f, Next(i));
	if(!(flags & V_ATRKEY)) {
		got = atr_get(i, A_LOCK, &aowner, &aflags);
		tempbool = parse_boolexp(GOD, got, 1);
		free_lbuf(got);
		putboolexp(f, tempbool);
		if(tempbool)
			free_bool(tempbool);
	}
	putref(f, Owner(i));
	if(flags & V_PARENT)
		putref(f, Parent(i));
	if(!(flags & V_ATRMONEY))
		putref(f, Pennies(i));
	putref(f, Flags(i));
	if(flags & V_XFLAGS)
		putref(f, Flags2(i));
	if(flags & V_3FLAGS)
		putref(f, Flags3(i));
	if(flags & V_POWERS) {
		putref(f, Powers(i));
		putref(f, Powers2(i));
	}
	/*
	 * write the attribute list 
	 */

	if((!(flags & V_GDBM)) || (mudstate.panicking == 1)) {
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
				got = atr_get_raw(i, j);
				fprintf(f, ">%d\n", j);
				putstring(f, got);
			}
		}
		fprintf(f, "<\n");
	}
	return 0;
}

dbref db_write(FILE * f, int format, int version)
{
	dbref i;
	int flags;
	VATTR *vp;

	switch (format) {
	case F_MUX:
		flags = version;
		break;
	default:
		fprintf(stderr, "Can only write MUX format.\n");
		return -1;
	}
	i = mudstate.attr_next;
	fprintf(f, "+X%d\n+S%d\n+N%ld\n", flags, mudstate.db_top, i);
	fprintf(f, "-R%d\n", mudstate.record_players);

	/*
	 * Dump user-named attribute info 
	 */

	vp = vattr_first();
	while (vp != NULL) {
		if(!(vp->flags & AF_DELETED))
			fprintf(f, "+A%d\n\"%d:%s\"\n", vp->number, vp->flags, vp->name);
		vp = vattr_next(vp);
	}

	DO_WHOLE_DB(i) {

		if(!(Going(i))) {
			fprintf(f, "!%ld\n", i);
			db_write_object(f, i, format, flags);
		}
	}
	fputs("***END OF DUMP***\n", f);
	fflush(f);
	return (mudstate.db_top);
}
