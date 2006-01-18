/*
 * htab.c - table hashing routines 
 */

#include "copyright.h"
#include "config.h"
#include "db.h"
#include "externs.h"
#include "htab.h"
#include "alloc.h"
#include "mudconf.h"

static int nhrbtab_compare(int left, int right, void *arg)
{
	return (right - left);
}

void nhashinit(RBTAB * htab, int size)
{
	memset(htab, 0, sizeof(RBTAB));
	htab->tree = rb_init((void *) nhrbtab_compare, NULL);
	htab->last = NULL;
}

void nhashreset(RBTAB * htab)
{
	htab->checks = 0;
	htab->scans = 0;
	htab->hits = 0;
};

/*
 * ---------------------------------------------------------------------------
 * * hashfind: Look up an entry in a hash table and return a pointer to its
 * * hash data.
 */

void *nhashfind(int val, RBTAB * htab)
{
	htab->checks++;
	return rb_find(htab->tree, (void *) val);
}

/*
 * ---------------------------------------------------------------------------
 * * hashadd: Add a new entry to a hash table.
 */

int nhashadd(int val, void *hashdata, RBTAB * htab)
{
	if(rb_exists(htab->tree, (void *) val))
		return (-1);
	rb_insert(htab->tree, (void *) val, hashdata);
	return 0;

}

/*
 * ---------------------------------------------------------------------------
 * * hashdelete: Remove an entry from a hash table.
 */

void nhashdelete(int val, RBTAB * htab)
{
	rb_delete(htab->tree, (void *) val);
	return;
}

/*
 * ---------------------------------------------------------------------------
 * * hashflush: free all the entries in a hashtable.
 */

void nhashflush(RBTAB * htab, int size)
{
	rb_destroy(htab->tree);
	htab->tree = rb_init((void *)nhrbtab_compare, NULL);
	htab->last = NULL;
}

/*
 * ---------------------------------------------------------------------------
 * * hashrepl: replace the data part of a hash entry.
 */

int nhashrepl(int val, void *hashdata, RBTAB * htab)
{
	struct int_dict_entry *ent;

	rb_insert(htab->tree, (void *) val, hashdata);
}
