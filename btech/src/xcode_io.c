/*
 * The database format is as follows:
 *
 * XCODE_MAGIC
 *
 * 1st record:
 * 	dbref key
 * 	GlueType type
 * 	size_t size
 * 	char data[size]
 *
 * Nth record:
 * 	same form as 1st record
 *
 * last record:
 * 	dbref key = -1
 *
 * <non-XCODE records>
 *
 * Each record holds a single XCODE object.
 *
 * Based on:
 *
 * $Id: mux_tree.c,v 1.2 2005/06/23 22:02:14 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Mon Nov 25 11:41:43 1996 mstenber
 * Last modified: Mon Jun 22 07:27:06 1998 fingon
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "create.h"

#include "glue_types.h"
#include "rbtree.h"

#include "xcode_io.h"

extern rbtree xcode_tree;


/*
 * I/O utilities.
 */

static int
do_write(FILE *f, const void *buf, size_t size, const char *comment)
{
	if (size == 0) {
		return 1;
	}

	if (fwrite(buf, size, 1, f) != 1) {
		/* Always an error.  */
		fprintf(stderr, "save_xcode_tree(): while writing %s: %s\n",
		        comment, strerror(errno)); /* don't need strerror_r */
		return -1;
	}

	return 1;
}

static int
do_read(FILE *f, void *buf, size_t size, const char *comment)
{
	if (size == 0) {
		return 1;
	}

	if (fread(buf, size, 1, f) != 1) {
		if (!ferror(f)) {
			/* End of file.  */
			return 0;
		}

		/* Error.  */
		fprintf(stderr, "load_xcode_tree(): while reading %s: %s\n",
		        comment, strerror(errno)); /* don't need strerror_r */
		return -1;
	}

	return 1;
}

static int
do_skip(FILE *f, long amount, const char *comment)
{
	assert(amount >= 0);

	if (amount == 0) {
		return 1;
	}

	if (fseek(f, amount, SEEK_CUR) != 0) {
		/* Always an error.  */
		fprintf(stderr, "load_xcode_tree(): while seeking in %s: %s\n",
		        comment, strerror(errno)); /* don't need strerror_r */
		return -1;
	}

	return 1;
}


/*
 * Save tree.
 */

typedef struct {
	FILE *f;
	int write_count;
} save_walker_state;

static int
save_node_walker(void *key, void *data, int depth, void *arg)
{
	const dbref key_val = (dbref)key;
	const XCODE *const xcode_obj = data;
	save_walker_state *const state = arg;

	/* Write key.  */
	if (do_write(state->f, &key_val, sizeof(key_val), "key") < 1) {
		/* Write failed.  */
		state->write_count = -1;
		return 0;
	}

	/* Write type.  */
	if (do_write(state->f, &xcode_obj->type, sizeof(xcode_obj->type),
	             "type") < 1) {
		/* Write failed.  */
		state->write_count = -1;
		return 0;
	}

	/* Write size.  */
	if (do_write(state->f, &xcode_obj->size, sizeof(xcode_obj->size),
	             "size") < 1) {
		/* Write failed.  */
		state->write_count = -1;
		return 0;
	}

	/* Write data.  */
	if (do_write(state->f, xcode_obj, xcode_obj->size, "data") < 1) {
		/* Write failed.  */
		state->write_count = -1;
		return 0;
	}

	state->write_count++;
	return 1;
}

int
save_xcode_tree(FILE *f)
{
	const dbref end_key = -1;

	save_walker_state state;

	state.f = f;
	state.write_count = 0;

	/* Dump XCODE objects.  */
	rb_walk(xcode_tree, WALK_INORDER, save_node_walker, &state);
	if (state.write_count < 0) {
		/* A write failed.  */
		return -1;
	}

	/* Write terminator.  */
	if (do_write(f, &end_key, sizeof(end_key), "terminator") < 1) {
		/* Write failed.  */
		return -1;
	}

	return state.write_count;
}


/*
 * Read tree.
 */

int
load_xcode_tree(FILE *f, size_t (*sizefunc)(GlueType))
{
	int read_count = 0; /* TODO: worry about overflow? */

	dbref key;
	GlueType type;
	size_t file_size, mem_size;
	XCODE *xcode_obj;

	for (;;) {
		/* Get key for next XCODE object.  */
		if (do_read(f, &key, sizeof(key), "key") < 1) {
			/* Read failed.  */
			return -1;
		}

		if (key < 0) {
			/* End marker.  */
			break;
		}

		/* Get type of next XCODE object.  We have a separate field,
		 * rather than using the one from the XCODE object, so that we
		 * aren't dependent on the size of the XCODE type.  */
		if (do_read(f, &type, sizeof(type), "type") < 1) {
			/* Read failed.  */
			return -1;
		}

		/* Get size of XCODE record.  We have a separate field, rather
		 * than using the one from the XCODE object, so that we aren't
		 * dependent on the size of the XCODE type.  */
		if (do_read(f, &file_size, sizeof(file_size), "size") < 1) {
			/* Read failed.  */
			return -1;
		}

		/* Get XCODE object.  */
		mem_size = sizefunc(type);

		if (mem_size < 0) {
			/* Encountered object of unknown type.  Bail out to
			 * avoid data corruption.  */
			return -1;
		}

		if (mem_size != file_size) {
			/* FIXME: If there's a size mismatch, we should just
			 * give up; the data structures are likely to be
			 * incompatible anyway.  But whatever.  */
			fprintf(stderr, "load_xcode_tree(): warning: #%d: size mismatch\n",
			        key);
		}

		Create(xcode_obj, char, mem_size);

		if (mem_size < file_size) {
			/* Read part of saved data.  */
			if (do_read(f, xcode_obj, mem_size, "data") < 1) {
				/* Read failed.  */
				Free(xcode_obj);
				return -1;
			}

			/* Skip remaining part.  */
			if (do_skip(f, file_size - mem_size, "data") < 1) {
				/* Seek failed.  */
				Free(xcode_obj);
				return -1;
			}
		} else {
			/* Read saved data.  */
			if (do_read(f, xcode_obj, mem_size, "data") < 1) {
				/* Read failed.  */
				Free(xcode_obj);
				return -1;
			}

			/* Remaining part zero'd by Create().  */
		}

		xcode_obj->type = type;
		xcode_obj->size = mem_size;

		/* Success, proceed to add to tree.  */
		/* TODO: rb_insert() could conceivably fail.  */
		rb_insert(xcode_tree, (void *)key, xcode_obj);

		read_count++;
	}

	return read_count;
}


/*
 * EXPERIMENTAL: New btechdb.finf format.
 *
 * <xcode xmlns="http://btonline-btech.sourceforge.net">
 * 	stuff goes here
 * </xcode>
 */

#include "sax.h"

static FI_Generator *btdb_generator = NULL;
static FI_ContentHandler *btdb_gen_handler = NULL;

static FI_Attributes *btdb_attrs = NULL;

typedef enum {
	EN_XCODE
} ElementNameIndex;

static struct {
	const ElementNameIndex idx;
	const char *const literal;
	FI_Name *cached;
} element_names[] = {
	{ EN_XCODE, "xcode", NULL },
	{ -1, NULL, NULL }
}; /* element_names[] */

#define GET_EN(x) (element_names[(x)].cached)

typedef enum {
	AN_VERSION
} AttrNameIndex;

static struct {
	const AttrNameIndex idx;
	const char *const literal;
	FI_Name *cached;
} attr_names[] = {
	{ AN_VERSION, "version", NULL },
	{ -1, NULL, NULL }
}; /* attr_names[] */

#define GET_AN(x) (attr_names[(x)].cached)

typedef enum {
	VAR_INT_1
} ValueVariableIndex;

static struct {
	const ValueVariableIndex idx;
	const FI_ValueType type;
	const size_t count;
	FI_Value *cached;
} value_vars[] = {
	{ VAR_INT_1, FI_VALUE_AS_INT, 1, NULL },
	{ -1, FI_VALUE_AS_NULL, 0, NULL }
}; /* values[] */

#define GET_VAR(x) (value_vars[(x)].cached)

/* This actually initializes the generator, not the parser.  Ironic.  */
int
init_btech_database_parser(void)
{
	int ii;
	FI_Name *new_name;
	FI_Value *new_var;

	assert(!btdb_generator);
	btdb_generator = fi_create_generator();
	if (!btdb_generator) {
		return 0;
	}

	btdb_gen_handler = fi_getContentHandler(btdb_generator);

	assert(!btdb_attrs);
	btdb_attrs = fi_create_attributes();
	if (!btdb_attrs) {
		fini_btech_database_parser();
		return 0;
	}

	/* Cache element names.  */
	for (ii = 0; element_names[ii].literal; ii++) {
		assert(element_names[ii].idx == ii);

		new_name = fi_create_element_name(btdb_generator,
		                                  element_names[ii].literal);
		if (!new_name) {
			fini_btech_database_parser();
			return 0;
		}

		assert(!element_names[ii].cached);
		element_names[ii].cached = new_name;
	}

	/* Cache attribute names.  */
	for (ii = 0; attr_names[ii].literal; ii++) {
		assert(attr_names[ii].idx == ii);

		new_name = fi_create_attribute_name(btdb_generator,
		                                    attr_names[ii].literal);
		if (!new_name) {
			fini_btech_database_parser();
			return 0;
		}

		assert(!attr_names[ii].cached);
		attr_names[ii].cached = new_name;
	}

	/* Cache common variable types.  */
	for (ii = 0; value_vars[ii].count; ii++) {
		assert(value_vars[ii].idx == ii);

		new_var = fi_create_value();

		if (!new_var) {
			fini_btech_database_parser();
			return 0;
		}

		if (!fi_set_value_type(new_var,
		                       value_vars[ii].type,
		                       value_vars[ii].count)) {
			fi_destroy_value(new_var);
			fini_btech_database_parser();
			return 0;
		}

		assert(!value_vars[ii].cached);
		value_vars[ii].cached = new_var;
	}

	return 1;
}

/* This actually finalizes the generator, not the parser.  Ironic.  */
int
fini_btech_database_parser(void)
{
	int ii;

	if (btdb_generator) {
		fi_destroy_generator(btdb_generator);
		btdb_generator = NULL;
	}

	if (btdb_attrs) {
		fi_destroy_attributes(btdb_attrs);
		btdb_attrs = NULL;
	}

	/* Free any cached element names.  */
	for (ii = 0; element_names[ii].cached; ii++) {
		fi_destroy_name(element_names[ii].cached);
		element_names[ii].cached = NULL;
	}

	/* Free any cached attribute names.  */
	for (ii = 0; attr_names[ii].cached; ii++) {
		fi_destroy_name(attr_names[ii].cached);
		attr_names[ii].cached = NULL;
	}

	/* Free any cached value variables.  */
	for (ii = 0; value_vars[ii].cached; ii++) {
		fi_destroy_value(value_vars[ii].cached);
		value_vars[ii].cached = NULL;
	}

	return 1;
}

/* This will replace the wrapper in the final version.  */
static int
real_save_btech_database(FILE *fpout)
{
	FI_Int32 version;

	if (!fi_generate_file(btdb_generator, fpout)) {
		fputs("FIXME: BTDB: fi_generate_file() error\n", stderr);
		return 0;
	}

	if (!btdb_gen_handler->startDocument(btdb_gen_handler)) {
		fputs("FIXME: BTDB: startDocument() error\n", stderr);
		return 0;
	}

	fi_clear_attributes(btdb_attrs);

	version = 0;
	fi_set_value(GET_VAR(VAR_INT_1), &version);

	if (!fi_add_attribute(btdb_attrs,
	                      GET_AN(AN_VERSION), GET_VAR(VAR_INT_1))) {
		fputs("FIXME: BTDB: fi_add_attributes() error\n", stderr);
		return 0;
	}

	if (!btdb_gen_handler->startElement(btdb_gen_handler,
	                                    GET_EN(EN_XCODE), btdb_attrs)) {
		fputs("FIXME: BTDB: startElement() error\n", stderr);
		return 0;
	}                                       

	if (!btdb_gen_handler->endElement(btdb_gen_handler,
	                                  GET_EN(EN_XCODE))) {
		fputs("FIXME: BTDB: startElement() error\n", stderr);
		return 0;
	}                                       

	if (!btdb_gen_handler->endDocument(btdb_gen_handler)) {
		fputs("FIXME: BTDB: endDocument() error\n", stderr);
		return 0;
	}

	return 1;
}

/* This will replace the wrapper in the final version.  */
static int btech_db_startElement(FI_ContentHandler *, const FI_Name *,
                                 const FI_Attributes *);
static int btech_db_endElement(FI_ContentHandler *, const FI_Name *);

static int btech_db_characters(FI_ContentHandler *, const FI_Value *);

static FI_ContentHandler btech_db_content_handler = {
	NULL /* startDocument */,
	NULL /* endDocument */,

	btech_db_startElement /* startElement */,
	btech_db_endElement /* endElement */,

	btech_db_characters /* characters */,

	NULL /* app_data_ptr */
}; /* btech_db_content_handler */

static int
real_load_btech_database(FILE *fpin)
{
	FI_Parser *parser;

	parser = fi_create_parser();
	if (!parser) {
		return 0;
	}

	fi_setContentHandler(parser, &btech_db_content_handler);

	if (!fi_parse_file(parser, fpin)) {
		fputs("FIXME: BTDB: fi_parse_file() error\n", stderr);
		fi_destroy_parser(parser);
		return 0;
	}

	fi_destroy_parser(parser);
	return 1;
}

/* We won't open the FILE ourselves in the final version.  */
#define BTECHDB_NAME "data/btechdb.finf"

int
save_btech_database(void)
{
	FILE *fpout;

	fpout = fopen(BTECHDB_NAME, "wb");
	if (!fpout) {
		fputs("FIXME: BTDB: fopen(fpout) error\n", stderr);
		return 0;
	}

	if (!real_save_btech_database(fpout)) {
		fputs("FIXME: BTDB: save_btech_database() error\n", stderr);
		return 0;
	}

	if (fclose(fpout) != 0) {
		fputs("FIXME: BTDB: fclose(fpout) error\n", stderr);
		return 0;
	}

	return 1;
}

/* We won't open the FILE ourselves in the final version.  */
int
load_btech_database(void)
{
	FILE *fpin;

	fpin = fopen(BTECHDB_NAME, "rb");
	if (!fpin) {
		fputs("FIXME: BTDB: fopen(fpin) error\n", stderr);
		return 0;
	}

	if (!real_load_btech_database(fpin)) {
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

static int
btech_db_startElement(FI_ContentHandler *handler, const FI_Name *name,
                      const FI_Attributes *attrs)
{
	fputs("FIXME: BTDB: START ELEMENT\n", stderr);
	return 1;
}

static int
btech_db_endElement(FI_ContentHandler *handler, const FI_Name *name)
{
	fputs("FIXME: BTDB: END ELEMENT\n", stderr);
	return 1;
}

static int
btech_db_characters(FI_ContentHandler *handler, const FI_Value *value)
{
	fputs("FIXME: BTDB: CHARACTERS\n", stderr);
	return 1;
}
