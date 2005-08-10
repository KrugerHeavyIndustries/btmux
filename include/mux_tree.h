
/*
 * $Id: mux_tree.h,v 1.1 2005/06/13 20:50:45 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Mon Nov 25 11:42:40 1996 mstenber
 * Last modified: Mon Jun 22 07:27:17 1998 fingon
 *
 */

#ifndef MUX_TREE_H
#define MUX_TREE_H

#include "tree.h"

typedef int muxkey_t;
typedef unsigned char dtype_t;
typedef unsigned short dsize_t;

#define NodeKey(n)  n->key
#define NodeData(n) n->data
#define NodeSize(n) n->size
#define NodeType(n) n->type

typedef struct rbtc_node_type {
    muxkey_t key;
    dtype_t type;
    dsize_t size;
    void *data;
} Node;

typedef tree *Tree;

#include "p.mux_tree.h"

Node *FindNode(Tree tree, muxkey_t key);
#endif				/* MUX_TREE_H */
