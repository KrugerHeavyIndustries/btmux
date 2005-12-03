/*
 * rbtree.h
 *
 * Copyright (c) 2004,2005 Martin Murray <mmurray@monkey.org>
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

typedef void *rbtree;

rbtree rb_init(int (*)(void *, void *, void *), void *);
void rb_destroy(rbtree);

void rb_insert(rbtree, void *key, void *data); 
void *rb_find(rbtree, void *key);
int rb_exists(rbtree, void *key);
void *rb_delete(rbtree, void *key);

void rb_walk(rbtree, int, int (*)(void *, void *, int, void *), void *);
unsigned int rb_size(rbtree);
void *rb_search(rbtree, int, void *);

#endif
