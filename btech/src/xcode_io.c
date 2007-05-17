/*
 * EXPERIMENTAL: New btechdb.finf format.
 *
 * <xcode> element:
 *
 * Right now, it's a bunch of blobs with a dbref # "id" attribute and a magic
 * integer "class" attribute.  Eventually, each basic type should get its own
 * element type, and be filled with type-appropriate info.
 *
 * <btechdb xmlns="http://btonline-btech.sourceforge.net">
 *     <xcode>
 *         <blob id="<dbref #>" class="<GlueType #>">Base64 binary blob</blob>
 *         <!-- more blobs -->
 *     </xcode>
 * </btechdb>
 *
 * (Note that Fast Infoset doesn't store actual Base64-encoded binary, but the
 * octets.  Base64 is simply the canonical textual representation, for
 * exporting to traditional XML files.)
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <assert.h>

/*#include "create.h" -- temporarily not used */

#include "glue_types.h"
#include "rbtree.h"

#include "sax.h"

#include "xcode_io.h"
#include "btdb_int.h"

extern rbtree xcode_tree;


/*
 * Global state variables.
 */

typedef enum {
	EN_BLOB
} ElementNameIndex;

static CachedName element_names[] = {
	{ EN_BLOB, "blob", NULL },
	{ -1, NULL, NULL }
}; /* element_names[] */

#define GET_EN(x) (element_names[(x)].cached)

typedef enum {
	AN_ID,
	AN_CLASS
} AttrNameIndex;

static CachedName attr_names[] = {
	{ AN_ID, "id", NULL },
	{ AN_CLASS, "class", NULL },
	{ -1, NULL, NULL }
}; /* attr_names[] */

#define GET_AN(x) (attr_names[(x)].cached)

/*
 * TODO: Future enhancement.  We only dump 6 or so different kinds of objects,
 * each of fixed size.  So we can create the same number of FI_Value objects,
 * pre-initialized to those sizes, and eliminate a lot of potential memory
 * reallocation.
 */
typedef enum {
	VAR_ANY,
	VAR_INT_1
} ValueVariableIndex;

static CachedVariable value_vars[] = {
	{ VAR_ANY, FI_VALUE_AS_OCTETS, 1, NULL },
	{ VAR_INT_1, FI_VALUE_AS_INT, 1, NULL },
	{ -1, FI_VALUE_AS_NULL, 0, NULL }
}; /* values[] */

#define GET_VAR(x) (value_vars[(x)].cached)

static size_t (*btdb_size_func)(GlueType);

int
btdb_init_xcode(size_t (*size_func)(GlueType))
{
	btdb_size_func = size_func;

	if (!init_btdb_element_names(element_names)) {
		return 0;
	}

	if (!init_btdb_attribute_names(attr_names)) {
		return 0;
	}

	if (!init_btdb_variables(value_vars)) {
		return 0;
	}

	return 1;
}

void
btdb_fini_xcode(void)
{
	fini_btdb_names(element_names);
	fini_btdb_names(attr_names);
	fini_btdb_variables(value_vars);
}


/*
 * Generating routines.
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
	if (!fi_set_value_type(GET_VAR(VAR_ANY),
	                       FI_VALUE_AS_OCTETS, blob_size)) {
		fputs("FIXME: BTDB: fi_set_value_type() error\n", stderr);
		return 0;
	}

	fi_set_value(GET_VAR(VAR_ANY), blob); /* TODO: optimize this */

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

int
btdb_save_xcode(const FI_Name *name_xcode)
{
	/* Write <xcode>.  */
	fi_clear_attributes(btdb_attrs);

	if (!btdb_gen_handler->startElement(btdb_gen_handler,
	                                    name_xcode, btdb_attrs)) {
		fputs("FIXME: BTDB: startElement() error\n", stderr);
		return 0;
	}

	/* Write <blob>s.  */
	if (!rb_walk(xcode_tree, WALK_INORDER, save_blob_walker, NULL)) {
		fputs("FIXME: BTDB: Failed to write blobs\n", stderr);
		return 0;
	}

	/* Write </xcode>.  */
	if (!btdb_gen_handler->endElement(btdb_gen_handler, name_xcode)) {
		fputs("FIXME: BTDB: endElement() error\n", stderr);
		return 0;
	}

	return 1;
}


/*
 * Parsing routines.
 */

static size_t subelement_depth = 0;

static int in_blob_element;

static dbref blob_key;
static GlueType blob_type;

static XCODE *blob_content = NULL;
static char *blob_content_cursor;
static size_t blob_content_size;

void
btdb_load_fini_xcode(void)
{
	if (blob_content) {
		free(blob_content);
		blob_content = NULL;
	}
}

int
btdb_start_xcode(const FI_Attributes *attrs)
{
	assert(subelement_depth == 0);
	subelement_depth = 1;

	assert(!blob_content);

	in_blob_element = 0;
	return 1;
}

int
btdb_end_xcode(void)
{
	assert(subelement_depth == 1 && !in_blob_element);
	subelement_depth = 0;
	return 1;
}

int
btdb_start_in_xcode(const FI_Name *name, const FI_Attributes *attrs)
{
	subelement_depth++;
	assert(subelement_depth > 0);

	if (subelement_depth == 2) {
		/* Check for subelements.  */
		if (fi_names_equal(name, GET_EN(EN_BLOB))) {
			/* Start of blob (<blob>).  */
			int ii;

			const FI_Value *value;

			assert(!in_blob_element && !blob_content);

			/* Parse "id" attribute (dbref).  */
			ii = fi_get_attribute_index(attrs, GET_AN(AN_ID));
			if (ii == -1) {
				fputs("FIXME: BTDB: <blob> has no id\n",
				      stderr);
				return 0;
			}

			value = fi_get_attribute_value(attrs, ii);
			blob_key = *(const FI_Int32 *)fi_get_value(value);

			/* Parse "class" attribute (GlueType).  */
			ii = fi_get_attribute_index(attrs, GET_AN(AN_CLASS));
			if (ii == -1) {
				fputs("FIXME: BTDB: <blob> has no class\n",
				      stderr);
				return 0;
			}

			value = fi_get_attribute_value(attrs, ii);
			blob_type = *(const FI_Int32 *)fi_get_value(value);

			/* Create the initial blob content.  */
			assert(!blob_content);

			blob_content_size = btdb_size_func(blob_type);

			if (blob_content_size < sizeof(XCODE)) {
				fputs("FIXME: BTDB: Invalid <blob> class\n",
				      stderr);
				return 0;
			}

			/*
			 * FIXME: We don't use Create() (and friends) because
			 * it calls exit() on failures, which isn't what we
			 * want.  This is fine, because Free() is just a
			 * wrapper around free(), but that's not guaranteed in
			 * the future.  So we need to fix one or the other.
			 */
			blob_content_cursor = calloc(blob_content_size, 1);
			if (!blob_content_cursor) {
				fputs("FIXME: BTDB: Couldn't allocate XCODE\n",
				      stderr);
				return 0;
			}

			blob_content = (XCODE *)blob_content_cursor;

			in_blob_element = 1;
		} else {
			/* No match.  Ignore, for forward compatibility.  */
			fputs("FIXME: BTDB: Warning: Unrecognized element\n",
			      stderr);
			assert(!in_blob_element);
		}
	} else {
		/* Ignorable subelement.  */
	}

	return 1;
}

int
btdb_end_in_xcode(const FI_Name *name)
{
	assert(subelement_depth > 1);

	if (in_blob_element && subelement_depth == 2) {
		/* End of blob (</blob>).  */
		assert(fi_names_equal(name, GET_EN(EN_BLOB)));

		assert(blob_content);

		/* Success, proceed to add to tree.  */
		if ((blob_content_cursor - (char *)blob_content)
		    < blob_content_size) {
			fputs("FIXME: BTDB: Warning: Short blob content\n",
			      stderr);
		}

		if (blob_content->type != blob_type) {
			fputs("FIXME: BTDB: Warning: Blob type mismatch\n",
			      stderr);
		}

		blob_content->type = blob_type;
		blob_content->size = blob_content_size;

		/* TODO: rb_insert() could conceivably fail.  */
		rb_insert(xcode_tree, (void *)blob_key, blob_content);

		blob_content = NULL;

		in_blob_element = 0;
	} else {
		/* Ignorable subelement.  */
	}

	subelement_depth--;
	return 1;
}

int
btdb_chars_in_xcode(const FI_Value *value)
{
	if (in_blob_element && subelement_depth == 2) {
		/* <blob> content.  */
		size_t diff_size, added_size;

		if (fi_get_value_type(value) != FI_VALUE_AS_OCTETS) {
			/* Oops, not a blob after all.  */
			fputs("FIXME: BTDB: Expected binary in <blob>\n",
			      stderr);
			return 0;
		}

		added_size = fi_get_value_count(value);
		assert(added_size > 0); /* parser prevents empty characters */

		diff_size = blob_content_size
		            - (blob_content_cursor - (char *)blob_content);

		if (diff_size < added_size) {
			/* Silently drop excess data.  */
			fputs("FIXME: BTDB: Warning: Excess blob content\n",
			      stderr);
			added_size = diff_size;
		}

		memcpy(blob_content_cursor, fi_get_value(value), added_size);
		blob_content_cursor += added_size;
	} else {
		/* Ignorable character content.  */
	}

	return 1;
}
