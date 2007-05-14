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
 * Right now, it's a bunch of blobs with a dbref # "id" attribute and a magic
 * integer "class" attribute.  Eventually, each basic type should get its own
 * element type, and be filled with type-appropriate info.
 *
 * Suggestion: We could use an XML schema language for our structure spec,
 * making it easier to import our Fast Infoset files into another tool.  Or,
 * then again, maybe not.
 *
 * <xcode xmlns="http://btonline-btech.sourceforge.net">
 * 	<blob id="<dbref #>" class="<GlueType #>">Base64 binary blob</blob>
 * 	<!-- more blobs -->
 * </xcode>
 */

#include "sax.h"

static FI_Vocabulary *btdb_vocabulary = NULL;

static FI_Generator *btdb_generator = NULL;
static FI_ContentHandler *btdb_gen_handler = NULL;

static FI_Attributes *btdb_attrs = NULL;

typedef enum {
	EN_XCODE,
	EN_BLOB
} ElementNameIndex;

static struct {
	const ElementNameIndex idx;
	const char *const literal;
	FI_Name *cached;
} element_names[] = {
	{ EN_XCODE, "xcode", NULL },
	{ EN_BLOB, "blob", NULL },
	{ -1, NULL, NULL }
}; /* element_names[] */

#define GET_EN(x) (element_names[(x)].cached)

typedef enum {
	AN_VERSION,
	AN_ID,
	AN_CLASS
} AttrNameIndex;

static struct {
	const AttrNameIndex idx;
	const char *const literal;
	FI_Name *cached;
} attr_names[] = {
	{ AN_VERSION, "version", NULL },
	{ AN_ID, "id", NULL },
	{ AN_CLASS, "class", NULL },
	{ -1, NULL, NULL }
}; /* attr_names[] */

#define GET_AN(x) (attr_names[(x)].cached)

/*
 * TODO: Future enhancement.  We only dump 6 or so different kinds of objects,
 * each of fixed size.  So we can create the same number of FI_Value objects,
 * pre-initialized to those sizes, and eliminate a lot of potential memory
 * reallocation.  However, that would involve pulling in a lot of headers I'm
 * not ready to pull in yet, to get the object sizes, or changes to interfaces
 * in this file that I'm also not ready to implement yet. :-)
 */
typedef enum {
	VAR_ANY,
	VAR_INT_1
} ValueVariableIndex;

static struct {
	const ValueVariableIndex idx;
	const FI_ValueType type;
	const size_t count;
	FI_Value *cached;
} value_vars[] = {
	{ VAR_ANY, FI_VALUE_AS_OCTETS, 1, NULL },
	{ VAR_INT_1, FI_VALUE_AS_INT, 1, NULL },
	{ -1, FI_VALUE_AS_NULL, 0, NULL }
}; /* values[] */

#define GET_VAR(x) (value_vars[(x)].cached)

int
init_btech_database_parser(void)
{
	int ii;
	FI_Name *new_name;
	FI_Value *new_var;

	assert(!btdb_vocabulary);
	btdb_vocabulary = fi_create_vocabulary();
	if (!btdb_vocabulary) {
		return 0;
	}

	/* Generator-specific.  */
	assert(!btdb_generator);
	btdb_generator = fi_create_generator(btdb_vocabulary);
	if (!btdb_generator) {
		fini_btech_database_parser();
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

		new_name = fi_create_element_name(btdb_vocabulary,
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

		new_name = fi_create_attribute_name(btdb_vocabulary,
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

int
fini_btech_database_parser(void)
{
	int ii;

	if (btdb_vocabulary) {
		fi_destroy_vocabulary(btdb_vocabulary);
		btdb_vocabulary = NULL;
	}

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
static int save_blob_walker(void *, void *, int, void *);

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

	/* Write <xcode>.  */
	fi_clear_attributes(btdb_attrs);

	version = 0;
	fi_set_value(GET_VAR(VAR_INT_1), &version);

	if (!fi_add_attribute(btdb_attrs,
	                      GET_AN(AN_VERSION), GET_VAR(VAR_INT_1))) {
		fputs("FIXME: BTDB: fi_add_attribute() error\n", stderr);
		return 0;
	}

	if (!btdb_gen_handler->startElement(btdb_gen_handler,
	                                    GET_EN(EN_XCODE), btdb_attrs)) {
		fputs("FIXME: BTDB: startElement() error\n", stderr);
		return 0;
	}

	/* Write <blob>s.  */
	if (!rb_walk(xcode_tree, WALK_INORDER, save_blob_walker, NULL)) {
		fputs("FIXME: BTDB: Failed to write blobs\n", stderr);
		return 0;
	}

	/* Write </xcode>.  */
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
static int btdb_startElement(FI_ContentHandler *, const FI_Name *,
                                 const FI_Attributes *);
static int btdb_endElement(FI_ContentHandler *, const FI_Name *);

static int btdb_characters(FI_ContentHandler *, const FI_Value *);

static FI_ContentHandler btdb_content_handler = {
	NULL /* startDocument */,
	NULL /* endDocument */,

	btdb_startElement /* startElement */,
	btdb_endElement /* endElement */,

	btdb_characters /* characters */,

	NULL /* app_data_ptr */
}; /* btdb_content_handler */

static size_t depth_other_element;

static int in_xcode_element;
static int in_blob_element;
static XCODE *blob_content;
static size_t blob_content_offset;

static int
real_load_btech_database(FILE *fpin)
{
	FI_Parser *parser;

	parser = fi_create_parser(btdb_vocabulary);
	if (!parser) {
		return 0;
	}

	fi_setContentHandler(parser, &btdb_content_handler);

	depth_other_element = 0;

	in_xcode_element = 0;
	in_blob_element = 0;
	blob_content = NULL;
	blob_content_offset = 0;

	if (!fi_parse_file(parser, fpin)) {
		fputs("FIXME: BTDB: fi_parse_file() error\n", stderr);

		if (blob_content) {
			free(blob_content);
		}

		fi_destroy_parser(parser);
		return 0;
	}

	if (blob_content) {
		free(blob_content);
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
static size_t (*btdb_size_func)(GlueType);

int
load_btech_database(size_t (*sizefunc)(GlueType))
{
	FILE *fpin;

	assert(sizefunc);
	btdb_size_func = sizefunc;

	fpin = fopen(BTECHDB_NAME, "rb");
	if (!fpin) {
		fputs("FIXME: BTDB: fopen(fpin) error\n", stderr);
		return 1; /* non-essential database (FIXME later) */
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

static int check_xcode_element(const FI_Attributes *);
static int init_blob_element(const FI_Attributes *);
static int append_to_blob(const FI_Value *);
static int end_of_blob(void);

static int
btdb_startElement(FI_ContentHandler *handler, const FI_Name *name,
                      const FI_Attributes *attrs)
{
	if (depth_other_element) {
		/* Ignore contents of other elements.  */
		assert(depth_other_element < (size_t)-1);
		depth_other_element++;
		return 1;
	}

	if (!in_xcode_element) {
		if (!fi_names_equal(name, GET_EN(EN_XCODE))) {
			/* Root element must be <xcode>.  */
			fputs("FIXME: BTDB: Expected root <xcode>\n", stderr);
			return 0;
		}

		if (!check_xcode_element(attrs)) {
			/* Invalid <xcode>.  */
			fputs("FIXME: BTDB: Bad <xcode>\n", stderr);
			return 0;
		}

		/* Saw root-level <xcode>.  */
		in_xcode_element = 1;
		return 1;
	}

	if (fi_names_equal(name, GET_EN(EN_BLOB))) {
		if (in_blob_element) {
			/* Nested <blob> not allowed.  */
			fputs("FIXME: BTDB: Nested <blob>\n", stderr);
			return 0;
		}

		if (!init_blob_element(attrs)) {
			/* Couldn't init <blob> state.  */
			fputs("FIXME: BTDB: <blob>\n", stderr);
			return 0;
		}

		/* Saw <blob>.  */
		in_blob_element = 1;
		return 1;
	}

	/* Ignore other non-root elements, for forward compatibility.  */
	fputs("FIXME: BTDB: Warning: Unsupported element\n", stderr);

	assert(depth_other_element < (size_t)-1);
	depth_other_element++;
	return 1;
}

static int
btdb_endElement(FI_ContentHandler *handler, const FI_Name *name)
{
	if (depth_other_element) {
		depth_other_element--;
		return 1;
	}

	assert(in_xcode_element); /* <xcode> is our root element */

	if (fi_names_equal(name, GET_EN(EN_XCODE))) {
		/* Note the parser guarantees we only see 1 root element.  */
		in_xcode_element = 0;
		return 1;
	}

	if (!in_blob_element || !fi_names_equal(name, GET_EN(EN_BLOB))) {
		/* Not a </blob>, and we can't handle anything else.  */
		fputs("FIXME: BTDB: Expected </blob>\n", stderr);
		return 0;
	}

	/* Add completely parsed blob to XCODE tree.  */
	if (!end_of_blob()) {
		fputs("FIXME: BTDB: </blob>\n", stderr);
		return 0;
	}

	in_blob_element = 0;
	return 1;
}

static int
btdb_characters(FI_ContentHandler *handler, const FI_Value *value)
{
	if (depth_other_element) {
		/* Ignore contents of other elements.  */
		return 1;
	}

	/* The parser should guarantee we only see characters inside of an
	 * element; documents aren't allowed to have outside content.  */
	assert(in_xcode_element);

	if (!in_blob_element) {
		/* Ignore top-level character data.  */
		return 1;
	}

	/* Append content to current blob.  */
	if (fi_get_value_type(value) != FI_VALUE_AS_OCTETS) {
		/* Oops, not a blob after all.  */
		fputs("FIXME: BTDB: Expected binary in <blob>\n", stderr);
		return 0;
	}

	if (!append_to_blob(value)) {
		/* Append failed.  */
		fputs("FIXME: BTDB: Couldn't append blob contents\n", stderr);
		return 0;
	}

	return 1;
}


/*
 * Parsing/generating subroutines.
 */

/* <blob> generation.  */
static int
save_blob_walker(void *key, void *data, int depth, void *arg)
{
	const FI_Int32 id_val = (dbref)key;
	const XCODE *const xcode_obj = data;

	const FI_Int32 class_val = xcode_obj->type;
	const size_t blob_size = xcode_obj->size;
	const FI_Octet *const blob = (FI_Octet *)xcode_obj;

	/* Write <blob>.  */
	fi_clear_attributes(btdb_attrs);

	fi_set_value(GET_VAR(VAR_INT_1), &id_val);

	if (!fi_add_attribute(btdb_attrs,
	                      GET_AN(AN_ID), GET_VAR(VAR_INT_1))) {
		fputs("FIXME: BTDB: fi_add_attribute() error\n", stderr);
		return 0;
	}

	fi_set_value(GET_VAR(VAR_INT_1), &class_val);

	if (!fi_add_attribute(btdb_attrs,
	                      GET_AN(AN_CLASS), GET_VAR(VAR_INT_1))) {
		fputs("FIXME: BTDB: fi_add_attribute() error\n", stderr);
		return 0;
	}

	if (!btdb_gen_handler->startElement(btdb_gen_handler,
	                                    GET_EN(EN_BLOB), btdb_attrs)) {
		fputs("FIXME: BTDB: startElement() error\n", stderr);
		return 0;
	}

	/* Write blob content.  */
	/*fprintf(stderr, "OCTETS[%u]\n", blob_size);*/

	if (!fi_set_value_type(GET_VAR(VAR_ANY),
	                       FI_VALUE_AS_OCTETS, blob_size)) {
		fputs("FIXME: BTDB: fi_set_value_type() error\n", stderr);
		return 0;
	}

	fi_set_value(GET_VAR(VAR_ANY), blob);

	if (!btdb_gen_handler->characters(btdb_gen_handler,
	                                  GET_VAR(VAR_ANY))) {
		fputs("FIXME: BTDB: characters() error\n", stderr);
		return 0;
	}

	/* Write </blob>.  */
	if (!btdb_gen_handler->endElement(btdb_gen_handler,
	                                  GET_EN(EN_BLOB))) {
		fputs("FIXME: BTDB: startElement() error\n", stderr);
		return 0;
	}

	return 1;
}

/* */
static int
check_xcode_element(const FI_Attributes *attrs)
{
	const int attrs_len = fi_get_attributes_length(attrs);

	int ii;

	for (ii = 0; ii < attrs_len; ii++) {
		if (fi_names_equal(fi_get_attribute_name(attrs, ii),
		                   GET_AN(AN_VERSION))) {
			/* Parse version attribute.  */
			const FI_Value *value;

			FI_Int32 version;

			value = fi_get_attribute_value(attrs, ii);
			version = *(const FI_Int32 *)fi_get_value(value);

			if (version != 0) {
				fprintf(stderr, "FIXME: BTDB: Version mismatch: %d != %d\n",
				        version, 0);
				return 0;
			}

			break;
		}
	}

	fputs("<xcode version=0>\n", stderr);
	return 1;
}

/* */
static int
init_blob_element(const FI_Attributes *attrs)
{
	const int attrs_len = fi_get_attributes_length(attrs);

	int ii;

	FI_Int32 id_val = -1;
	FI_Int32 class_val = -1;

	/* TODO: The parser should ensure unique attributes.  */
	for (ii = 0; ii < attrs_len; ii++) {
		const FI_Name *const a_name = fi_get_attribute_name(attrs, ii);
		const FI_Value *a_value;

		if (fi_names_equal(a_name, GET_AN(AN_ID))) {
			/* Parse "id" attribute (dbref).  */
			a_value = fi_get_attribute_value(attrs, ii);

			id_val = *(const FI_Int32 *)fi_get_value(a_value);

			if (id_val < 0) {
				fprintf(stderr, "FIXME: BTDB: Invalid value for 'id': %d\n", id_val);
				return 0;
			}
		} else if (fi_names_equal(a_name, GET_AN(AN_CLASS))) {
			/* Parse "class" attribute (GlueType).  */
			a_value = fi_get_attribute_value(attrs, ii);

			class_val = *(const FI_Int32 *)fi_get_value(a_value);

			if (class_val < 0) {
				fprintf(stderr, "FIXME: BTDB: Invalid value for 'class': %d\n", class_val);
				return 0;
			}
		}
	}

	if (id_val < 0) {
		fputs("FIXME: BTDB: <blob> missing 'id' attribute\n", stderr);
		return 0;
	}

	if (class_val < 0) {
		fputs("FIXME: BTDB: <blob> missing 'class' attribute\n", stderr);
		return 0;
	}

	fprintf(stderr, "\t<blob id=%d class=%d>", id_val, class_val);

	/*Create(blob_content, char, mem_size);*/
	return 1;
}

/* */
static int
append_to_blob(const FI_Value *value)
{
	fputs("[blob]", stderr);
	return 1;
}

/* */
static int
end_of_blob(void)
{
	/* Success, proceed to add to tree.  */
	/* TODO: rb_insert() could conceivably fail.  */
	/*rb_insert(xcode_tree, (void *)blob_content->key, blob_content);*/
	fputs("</blob>\n", stderr);
	return 1;
}

static int
tmp_load_xcode_tree(void)
{
	dbref key;
	GlueType type;
	size_t file_size, mem_size;
	XCODE *xcode_obj;

	for (;;) {
		/* Get key for next XCODE object.  */

		/* Get type of next XCODE object.  We have a separate field,
		 * rather than using the one from the XCODE object, so that we
		 * aren't dependent on the size of the XCODE type.  */


		/* Get XCODE object.  */
		mem_size = btdb_size_func(type);

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

		if (mem_size < file_size) {
			/* Read part of saved data.  */
		} else {
			/* Read saved data.  */

			/* Remaining part zero'd by Create().  */
		}

		xcode_obj->type = type;
		xcode_obj->size = mem_size;
	}

	return 0;
}
