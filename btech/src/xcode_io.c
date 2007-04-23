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
