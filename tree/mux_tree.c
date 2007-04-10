
/*
 * $Id: mux_tree.c,v 1.2 2005/06/23 22:02:14 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Mon Nov 25 11:41:43 1996 mstenber
 * Last modified: Mon Jun 22 07:27:06 1998 fingon
 *
 */

#define FATAL(msgs...) \
{ fprintf(stderr, ##msgs); exit(1); }

#define TREAD(to,len,msg) \
if (feof(f)) FATAL("ERROR READING FILE (%s): EOF.\n", msg); \
if (fread(to,1,len,f) != len) \
FATAL("ERROR READING FILE (%s): NOT ENOUGH READ!\n", msg);

#define TSAVE(from,len,msg) \
if (fwrite(from, 1, len, tree_file) != len) \
FATAL("ERROR WRITING FILE (%s): NOT ENOUGH WRITTEN(?!?)\n", msg);

/* Main aim: Attempt to provide 1-1 interface when compared to my
   old rbtc code and the new AVL code */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "mux_tree.h"

#define Create(var,typ,count) \
if (!(var = (typ *) calloc(sizeof (typ), count))) \
{ fprintf(stderr, "Error mallocating!\n"); exit(1); }

static int NodeCompare(Node * a, Node * b) {
    if (a->key < b->key)
	return -1;
    return (a->key > b->key);
}

static void NodeDelete(Node * a) {
    if (a->data)
	free((void *) a->data);
    free((void *) a);
}

void AddEntry(Tree * tree, muxkey_t key, dtype_t type, dsize_t size, void *data) {
    Node *foo;

    if (!tree)
	return;
    Create(foo, Node, 1);
    foo->key = key;
    foo->data = data;
    foo->type = type;
    foo->size = size;
    tree_add(tree, NodeCompare, foo, NodeDelete);
}

Node *FindNode(Tree tree, muxkey_t key) {
    Node foo;

    foo.key = key;
    return tree_srch(&tree, NodeCompare, &foo);
}

void DeleteEntry(Tree * tree, muxkey_t key) {
    Node foo;

    if (FindNode(*tree, key)) {
	foo.key = key;
	tree_delete(tree, NodeCompare, &foo, NodeDelete);
    }
}

static FILE *tree_file;

static int nodesave_count;

static int NodeSave(Node * n) {
    TSAVE(&n->key, sizeof(n->key), "key");
    TSAVE(&n->type, sizeof(n->type), "type");
    TSAVE(&n->size, sizeof(n->size), "size");
    if (n->size > 0)
	TSAVE(n->data, n->size, "data");
    nodesave_count++;
    return 1;
}

int SaveTree(FILE * f, Tree tree) {
    muxkey_t key;

    nodesave_count = 0;
    tree_file = f;
    tree_trav(&tree, NodeSave);
    key = -1;
    fwrite(&key, sizeof(key), 1, tree_file);
    return nodesave_count;
}

static void MyLoadTree(FILE * f, Tree * tree, int (*sizefunc)(int)) {
    muxkey_t key;
    dtype_t type;
    dsize_t osize;
    int nsize, rsize;
    void *data;

    TREAD(&key, sizeof(key), "first key");
    while (key >= 0 && !feof(f)) {
	TREAD(&type, sizeof(type), "type");
	TREAD(&osize, sizeof(osize), "size");
	if (sizefunc && (rsize = sizefunc(type)) >= 0) {
	    nsize = osize > rsize ? osize : rsize;
	} else
	    nsize = rsize = osize;
	if (nsize) {
	    if (!(data = malloc(nsize))) {
		printf("Error malloccing!\n");
		exit(1);
	    }
	    if (osize)
	    	TREAD(data, osize, "data");

	    if (nsize > osize)
	    	memset(data + osize, 0, nsize - osize);
	} else
	    data = NULL;

	/* Set node size to the real type size, rather than the read size,
	 * so that the object will get 'truncated' to the right size on the
	 * next dump.
	 */
	AddEntry(tree, key, type, rsize, data);
	TREAD(&key, sizeof(key), "new key");
    }
}


/* This is a _MONSTER_ :P */
void ClearTree(Tree * tree) {
    tree_mung(tree, NodeDelete);
}

void LoadTree(FILE * f, Tree * tree, int (*sizefunc)(int)) {
    ClearTree(tree);
    MyLoadTree(f, tree, sizefunc);
}

void UpdateTree(FILE * f, Tree * tree, int(*sizefunc)(int)) {
    MyLoadTree(f, tree, sizefunc);
}

void GoThruTree(Tree tree, int (*func) (Node *)) {
    tree_trav(&tree, func);
}

