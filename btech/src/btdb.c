/*
 * EXPERIMENTAL: New btechdb.finf format.
 *
 * Consists of a series of elements, delegating to specific subdatabase types.
 *
 * Suggestion: We could use an XML schema language to specify structure layout,
 * making it easier to import our Fast Infoset files into another tool.  Or,
 * then again, maybe not.
 *
 * TODO: Merge <xcode> and <mapdynamic> sections, now that we have a fancy
 * extensible file format that can easily accommodate variable record contents.
 *
 * TODO: <repairs> should probably become part of an "events" element.
 *
 * <btechdb xmlns="http://btonline-btech.sourceforge.net" version="0">
 *     <xcode><!-- XCODE blobs go here --></xcode>
 *     <mapdynamic><!-- mapdynamic items go here --></mapdynamic>
 *     <repairs><!-- repair events go here --></repairs>
 * </btechdb>
 *
 * The old XCODE hcode.db implementation was last maintained in r1088.
 */

#include "config.h"

#include <stdio.h>

#include <assert.h>

#include "sax.h"

#include "btdb.h"
#include "btdb_int.h"

#include "xcode_io.h"


/*
 * Global state variables.
 */

static FI_Vocabulary *vocabulary = NULL;

static FI_Generator *generator = NULL;
FI_ContentHandler *btdb_gen_handler = NULL;

FI_Attributes *btdb_attrs = NULL;

/*
 * Cached top-level Fast Infoset library objects.  Each submodule is
 * responsible for acquiring its own set of objects.
 */
typedef enum {
	EN_BTECHDB,
	EN_XCODE
} ElementNameIndex;

static CachedName element_names[] = {
	{ EN_BTECHDB, "btechdb", NULL },
	{ EN_XCODE, "xcode", NULL },
	{ -1, NULL, NULL }
}; /* element_names[] */

#define GET_EN(x) (element_names[(x)].cached)

typedef enum {
	AN_VERSION
} AttrNameIndex;

static CachedName attr_names[] = {
	{ AN_VERSION, "version", NULL },
	{ -1, NULL, NULL }
}; /* attr_names[] */

#define GET_AN(x) (attr_names[(x)].cached)

typedef enum {
	VAR_INT_1
} ValueVariableIndex;

static CachedVariable value_vars[] = {
	{ VAR_INT_1, FI_VALUE_AS_INT, 1, NULL },
	{ -1, FI_VALUE_AS_NULL, 0, NULL }
}; /* values[] */

#define GET_VAR(x) (value_vars[(x)].cached)

int
init_btdb_state(size_t (*sizefunc)(GlueType))
{
	/* Construct vocabulary.  */
	assert(!vocabulary);
	vocabulary = fi_create_vocabulary();
	if (!vocabulary) {
		return 0;
	}

	if (!init_btdb_element_names(element_names)) {
		fini_btdb_state();
		return 0;
	}

	if (!init_btdb_attribute_names(attr_names)) {
		fini_btdb_state();
		return 0;
	}

	/* Generator-specific.  */
	assert(!generator);
	generator = fi_create_generator(vocabulary);
	if (!generator) {
		fini_btdb_state();
		return 0;
	}

	btdb_gen_handler = fi_getContentHandler(generator);

	assert(!btdb_attrs);
	btdb_attrs = fi_create_attributes();
	if (!btdb_attrs) {
		fini_btdb_state();
		return 0;
	}

	if (!init_btdb_variables(value_vars)) {
		fini_btdb_state();
		return 0;
	}

	/* Subelements.  */
	if (!btdb_init_xcode(sizefunc)) {
		fini_btdb_state();
		return 0;
	}

	return 1;
}

int
fini_btdb_state(void)
{
	/* Destroy vocabulary.  */
	if (vocabulary) {
		fi_destroy_vocabulary(vocabulary);
		vocabulary = NULL;
	}

	fini_btdb_names(element_names);
	fini_btdb_names(attr_names);

	/* Generator-specific.  */
	if (generator) {
		fi_destroy_generator(generator);
		generator = NULL;
	}

	if (btdb_attrs) {
		fi_destroy_attributes(btdb_attrs);
		btdb_attrs = NULL;
	}

	fini_btdb_variables(value_vars);

	/* Subelements.  */
	btdb_fini_xcode();

	return 1;
}

/* This will replace the wrapper in the final version.  */
static int
real_save_btdb(FILE *fpout)
{
	FI_Int32 version;

	if (!fi_generate_file(generator, fpout)) {
		fputs("FIXME: BTDB: fi_generate_file() error\n", stderr);
		return 0;
	}

	/* Write <btechdb> root-level element.  */
	if (!btdb_gen_handler->startDocument(btdb_gen_handler)) {
		fputs("FIXME: BTDB: startDocument() error\n", stderr);
		return 0;
	}

	fi_clear_attributes(btdb_attrs);

	version = 0;
	fi_set_value(GET_VAR(VAR_INT_1), &version);

	if (!fi_add_attribute(btdb_attrs,
	                      GET_AN(AN_VERSION), GET_VAR(VAR_INT_1))) {
		fputs("FIXME: BTDB: fi_add_attribute() error\n", stderr);
		return 0;
	}

	if (!btdb_gen_handler->startElement(btdb_gen_handler,
	                                    GET_EN(EN_BTECHDB), btdb_attrs)) {
		fputs("FIXME: BTDB: startElement() error\n", stderr);
		return 0;
	}

	/* Write subelements.  */
	if (!btdb_save_xcode(GET_EN(EN_XCODE))) {
		return 0;
	}

	/* Write </btechdb>.  */
	if (!btdb_gen_handler->endElement(btdb_gen_handler,
	                                  GET_EN(EN_BTECHDB))) {
		fputs("FIXME: BTDB: endElement() error\n", stderr);
		return 0;
	}

	if (!btdb_gen_handler->endDocument(btdb_gen_handler)) {
		fputs("FIXME: BTDB: endDocument() error\n", stderr);
		return 0;
	}

	/* Clean up.  */
	fi_clear_attributes(btdb_attrs);
	return 1;
}

/* This will replace the wrapper in the final version.  */
static int btdb_startElement(FI_ContentHandler *, const FI_Name *,
                             const FI_Attributes *);
static int btdb_endElement(FI_ContentHandler *, const FI_Name *);

static int btdb_characters(FI_ContentHandler *, const FI_Value *);

static FI_ContentHandler parser_content_handler = {
	NULL /* startDocument */,
	NULL /* endDocument */,

	btdb_startElement /* startElement */,
	btdb_endElement /* endElement */,

	btdb_characters /* characters */,

	NULL /* app_data_ptr */
}; /* btdb_content_handler */

static size_t element_depth;
static ElementNameIndex element_context;

static int has_xcode_element;

static int
real_load_btdb(FILE *fpin)
{
	FI_Parser *parser;

	parser = fi_create_parser(vocabulary);
	if (!parser) {
		return 0;
	}

	fi_setContentHandler(parser, &parser_content_handler);

	element_depth = 0;
	element_context = EN_BTECHDB;

	has_xcode_element = 0;

	/* Parse.  */
	if (!fi_parse_file(parser, fpin)) {
		fputs("FIXME: BTDB: fi_parse_file() error\n", stderr);

		btdb_load_fini_xcode();

		fi_destroy_parser(parser);
		return 0;
	}

	/* Clean up.  */
	btdb_load_fini_xcode();

	fi_destroy_parser(parser);
	return 1;
}

/* We won't open the FILE ourselves in the final version.  */
#define BTECHDB_NAME "data/btechdb.finf"

int
save_btdb(void)
{
	FILE *fpout;

	fpout = fopen(BTECHDB_NAME, "wb");
	if (!fpout) {
		fputs("FIXME: BTDB: fopen(fpout) error\n", stderr);
		return 0;
	}

	if (!real_save_btdb(fpout)) {
		fputs("FIXME: BTDB: save_btech_database() error\n", stderr);
		return 0;
	}

	if (fclose(fpout) != 0) {
		fputs("FIXME: BTDB: fclose(fpout) error\n", stderr);
		return 0;
	}

	return 1;
}

int
load_btdb(void)
{
	FILE *fpin;

	fpin = fopen(BTECHDB_NAME, "rb");
	if (!fpin) {
		fputs("FIXME: BTDB: fopen(fpin) error\n", stderr);
		return 1; /* non-essential database (FIXME later) */
	}

	if (!real_load_btdb(fpin)) {
		fputs("FIXME: BTDB: load_btech_database() error\n", stderr);
		fclose(fpin);
		return 0;
	}

	if (fclose(fpin) != 0) {
		fputs("FIXME: BTDB: fclose(fpin) error\n", stderr);
		return 0;
	}

	return 1;
}


/*
 * Parser event handlers.
 */

/* Validate <btechdb> root element.  */
static int
check_btechdb_element(const FI_Attributes *attrs)
{
	int ii;

	const FI_Value *value;
	FI_Int32 version;

	/* Parse "version" attribute.  */
	ii = fi_get_attribute_index(attrs, GET_AN(AN_VERSION));
	if (ii == -1) {
		fputs("FIXME: BTDB: <xcode> has no version\n", stderr);
		return 0;
	}

	value = fi_get_attribute_value(attrs, ii);
	version = *(const FI_Int32 *)fi_get_value(value);

	if (version != 0) {
		fprintf(stderr, "FIXME: BTDB: Version mismatch: %d != %d\n",
		        version, 0);
		return 0;
	}

	return 1;
}

static int
btdb_startElement(FI_ContentHandler *handler, const FI_Name *name,
                  const FI_Attributes *attrs)
{
	element_depth++;
	assert(element_depth > 0); /* overflow? */

	if (element_depth == 1) {
		/* Check for root element.  */
		if (!fi_names_equal(name, GET_EN(EN_BTECHDB))) {
			/* Root element must be <btechdb>.  */
			fputs("FIXME: BTDB: Expected root <btechdb>\n",
			      stderr);
			return 0;
		}

		if (!check_btechdb_element(attrs)) {
			/* Invalid <btechdb>.  */
			fputs("FIXME: BTDB: Bad <btechdb>\n", stderr);
			return 0;
		}
	} else if (element_depth == 2) {
		/* Check for subelements.  */
		if (fi_names_equal(name, GET_EN(EN_XCODE))) {
			if (has_xcode_element) {
				/* Multiple <xcode> elements.  */
				fputs("FIXME: BTDB: Multiple <xcode>\n",
				      stderr);
				return 0;
			}

			has_xcode_element = 1;

			if (!btdb_start_xcode(attrs)) {
				return 0;
			}

			element_context = EN_XCODE;
		} else {
			/* No match.  Ignore, for forward compatibility.  */
			fputs("FIXME: BTDB: Warning: Unrecognized element\n",
			      stderr);
			element_context = EN_BTECHDB;
		}
	} else {
		/* >2 level.  Delegate to subelement handlers.  */
		switch (element_context) {
		case EN_XCODE:
			if (!btdb_start_in_xcode(name, attrs)) {
				return 0;
			}
			break;

		default:
			/* Still EN_BTECHDB.  Ignore contents.  */
			break;
		}
	}

	return 1;
}

static int
btdb_endElement(FI_ContentHandler *handler, const FI_Name *name)
{
	assert(element_depth > 0);

	if (element_depth == 1) {
		/* Root element.  Parser guarantees we only see 1.  */
	} else if (element_depth == 2) {
		/* Subelement.  Parser guarantees proper nesting.  */
		switch (element_context) {
		case EN_XCODE:
			if (!btdb_end_xcode()) {
				return 0;
			}
			break;

		default:
			/* Still EN_BTECHDB.  Ignore contents.  */
			break;
		}
	} else {
		/* >2 level.  Delegate to subelement handlers.  */
		switch (element_context) {
		case EN_XCODE:
			if (!btdb_end_in_xcode(name)) {
				return 0;
			}
			break;

		default:
			/* Still EN_BTECHDB.  Ignore contents.  */
			break;
		}
	}

	element_depth--;
	return 1;
}

static int
btdb_characters(FI_ContentHandler *handler, const FI_Value *value)
{
	switch (element_context) {
	case EN_XCODE:
		if (!btdb_chars_in_xcode(value)) {
			return 0;
		}
		break;

	default:
		/* Ignore element content.  */
		break;
	}

	return 1;
}


/*
 * Routines for initializing/finalizing cached Fast Infoset library objects.
 */

int
init_btdb_element_names(CachedName *table)
{
	int ii;

	FI_Name *new_name;

	for (ii = 0; table[ii].literal; ii++) {
		assert(table[ii].idx == ii);

		new_name = fi_create_element_name(vocabulary,
		                                  table[ii].literal);
		if (!new_name) {
			fputs("FIXME: BTDB: Couldn't allocate name\n", stderr);
			fini_btdb_names(table);
			return 0;
		}

		assert(!table[ii].cached);
		table[ii].cached = new_name;
	}

	return 1;
}

int
init_btdb_attribute_names(CachedName *table)
{
	int ii;

	FI_Name *new_name;

	for (ii = 0; table[ii].literal; ii++) {
		assert(table[ii].idx == ii);

		new_name = fi_create_attribute_name(vocabulary,
		                                    table[ii].literal);
		if (!new_name) {
			fputs("FIXME: BTDB: Couldn't allocate name\n", stderr);
			fini_btdb_names(table);
			return 0;
		}

		assert(!table[ii].cached);
		table[ii].cached = new_name;
	}

	return 1;
}

void
fini_btdb_names(CachedName *table)
{
	int ii;

	for (ii = 0; table[ii].cached; ii++) {
		fi_destroy_name(table[ii].cached);
		table[ii].cached = NULL;
	}
}

int
init_btdb_variables(CachedVariable *table)
{
	int ii;

	FI_Value *new_var;

	for (ii = 0; table[ii].count; ii++) {
		assert(table[ii].idx == ii);

		new_var = fi_create_value();

		if (!new_var) {
			fputs("FIXME: BTDB: Couldn't allocate variable\n",
			      stderr);
			fini_btdb_variables(table);
			return 0;
		}

		if (!fi_set_value_type(new_var,
		                       table[ii].type, table[ii].count)) {
			fputs("FIXME: BTDB: Couldn't allocate variable\n",
			      stderr);
			fi_destroy_value(new_var);
			fini_btdb_variables(table);
			return 0;
		}

		assert(!table[ii].cached);
		table[ii].cached = new_var;
	}

	return 1;
}

void
fini_btdb_variables(CachedVariable *table)
{
	int ii;

	for (ii = 0; table[ii].cached; ii++) {
		fi_destroy_value(table[ii].cached);
		table[ii].cached = NULL;
	}
}
