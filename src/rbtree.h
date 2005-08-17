/*
 * rbtree.h
 *
 * Copyright (c) 2004,2005 Martin Murray <mmurray@monkey.org>
 * All rights reserved.
 *
 * $Id$
 */

#ifndef __FOREST_RB_TREE__
#define __FOREST_RB_TREE__

#define NODE_RED 0 
#define NODE_BLACK 1

#define SEARCH_EQUAL    0x1
#define SEARCH_GTEQ     0x2
#define SEARCH_LTEQ     0x3
#define SEARCH_GT       0x4
#define SEARCH_LT       0x5
#define SEARCH_NEXT     0x6
#define SEARCH_PREV     0x7
#define SEARCH_FIRST    0x8
#define SEARCH_LAST     0x9

#define WALK_PREORDER   0x100
#define WALK_INORDER    0x101
#define WALK_POSTORDER  0x102

typedef struct rbtree_node_t {
    struct rbtree_node_t *left, *right, *parent;
    void *key;
    void *data;
    int color;
} rbtree_node;

typedef struct rbtree_head_t {
    struct rbtree_node_t *head;
    int (*compare_function)(void *, void *, void *);
    void *token;
} rbtree;

rbtree *rb_init(int (*)(void *, void *, void *), void *);
void rb_destroy(rbtree *);

void rb_insert(rbtree *, void *key, void *data); 
void *rb_find(rbtree *, void *key);
int rb_exists(rbtree *, void *key);
void *rb_delete(rbtree *, void *key);

void rb_walk(rbtree *, int, void (*)(void *, void *, int, void *), 
        void *);

#endif
