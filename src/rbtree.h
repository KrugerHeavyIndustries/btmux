/*
 * rbtree.h
 *
 * Copyright (c) 2004,2005 Martin Murray <mmurray@mon.org>
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.      
 *
 * $Id$
 */

#ifndef __RB_TREE__
#define __RB_TREE__

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

#ifndef DEBUG
typedef void *rbtree;
#else
typedef struct rbtree_node_t {
    struct rbtree_node_t *left, *right, *parent;
    void *key;
    void *data;
    int color;
    int count;
} rbtree_node;

typedef struct rbtree_head_t {
    struct rbtree_node_t *head;
    int (*compare_function) (void *, void *, void *);
    void *token;
    unsigned int size;
} *rbtree;
#endif

rbtree rb_init(int (*)(void *, void *, void *), void *);
void rb_destroy(rbtree);

void rb_insert(rbtree, void *, void *); 
void *rb_find(rbtree, void *);
int rb_exists(rbtree, void *);
void *rb_delete(rbtree, void *);
void *rb_release(rbtree, void (*)(void *, void *, void *), void *);

void rb_walk(rbtree, int, int (*)(void *, void *, int, void *), void *);
unsigned int rb_size(rbtree);
void *rb_search(rbtree, int, void *);
void *rb_index(rbtree, int);
#endif
