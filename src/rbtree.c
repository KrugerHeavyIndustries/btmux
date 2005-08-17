/* 
 * rbtree.c - a redblack tree implementation
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "rbtree.h"

#ifdef assert
#undef assert
#endif

#define assert(x) if(!(x)) do { fprintf(stderr, "%s (%s:%d)] failed assert. aborting.\n", \
        __FUNCTION__, __FILE__, __LINE__); abort(); } while (0)


rb_tree *rb_init(int (*compare_function)(void *, void *, void *), void *token) {
    rb_tree *temp;

    temp = malloc(sizeof(rb_tree));
    if(temp == NULL) return NULL;
    memset(temp, 0, sizeof(rb_tree));
    temp->compare_function = compare_function;
    temp->token = token;
    return temp;
}

static rb_tree_node *rb_find_minimum(rb_tree_node *node) {
    rb_tree_node *child;
    assert(node != NULL);
    child = node;
    while(child->left != NULL) child = child->left;
    return child;
}

static rb_tree_node *rb_find_maximum(rb_tree_node *node) {
    rb_tree_node *child;
    assert(node != NULL);
    child = node;
    while(child->right != NULL) child = child->right;
    return child;
}

static rb_tree_node *rb_find_successor_node(rb_tree_node *node) {
    rb_tree_node *child;
    assert(node!=NULL);
    if(node->left == NULL && node->right == NULL) return NULL;
    if(node->right == NULL) return node->left;
    child = node->right;
    while(child->left != NULL) child = child->left;
    return child;
}

static rb_tree_node *rb_find_predessor_node(rb_tree_node *node) {
    rb_tree_node *child, *parent;
    assert(node!=NULL);
    if(node->left != NULL) {
        child = node->left;
        while(child != NULL) child = child->right;
        return child;
    } else {
        child = node;
        parent = node->parent;
        while(parent != NULL) {
            if(parent->right == child) return child;
            child = parent;
            parent = child->parent;
        }
    }
    return NULL;
}

void rb_destroy(rb_tree *bt) {
    rb_tree_node *node, *parent;
    node = bt->head;
    while(node != NULL) {
        if(node->left != NULL) {
            node = node->left;
            continue;
        } else if(node->right != NULL) {
            node = node->right;
            continue;
        } else {
            parent = node->parent;
            if(parent && parent->left == node) parent->left = NULL;
            else if(parent && parent->right == node) parent->right = NULL;
            else assert(!parent); 
            free(node);
            node = parent;
        }
    }
    free(bt);
    return;
}

static rb_tree_node *rb_allocate(rb_tree_node *parent, void *key, void *data) {
    rb_tree_node *temp;
    temp = malloc(sizeof(rb_tree_node));
    memset(temp, 0, sizeof(rb_tree_node));
    temp->parent = parent;
    temp->key = key;
    temp->data = data;
    return temp;
}

static void rb_rotate_right(rb_tree *bt, rb_tree_node *pivot) {
    rb_tree_node *child, *parent, *subchild;
    parent = pivot->parent;
    child = pivot->left;

    pivot->left = child->right;
    if(pivot->left != NULL)
        pivot->left->parent = pivot;

    pivot->parent = child;
    child->parent = parent;
    child->right = pivot;

    if(parent) {
        if(parent->left == pivot)
            parent->left = child;
        else
            parent->right = child;
    } else bt->head = child;
}

static void rb_rotate_left(rb_tree *bt, rb_tree_node *pivot) {
    rb_tree_node *child, *parent, *subchild;
    parent = pivot->parent;
    child = pivot->right;

    pivot->right = child->left;
    if(pivot->right != NULL)
        pivot->right->parent = pivot;

    pivot->parent = child;
    child->parent = parent;
    child->left = pivot;

    if(parent) {
        if(parent->right == pivot)
            parent->right = child;
        else
            parent->left = child;
    } else bt->head = child;
}



void rb_insert(rb_tree *bt, void *key, void *data) {
    rb_tree_node *node;
    rb_tree_node *iter;
    int compare_result;

    if(!bt->head) {
        bt->head = rb_allocate(NULL, key, data);
        bt->head->color = NODE_BLACK;
        return;
    }

    node = bt->head;
    while(node != NULL) {
        compare_result = (*bt->compare_function)(key, node->key, bt->token);
        if(compare_result == 0) {
            // Key already exists, replace data.
            node->data = data;
            return;
        } else if(compare_result < 0) {
            // Go Left
            if(node->left != NULL) {
                node = node->left;
            } else {
                node->left = rb_allocate(node, key, data);
                node = node->left;
                break;
            }
        } else {
            if(node->right != NULL) {
                node = node->right;
            } else {
                node->right = rb_allocate(node, key, data);
                node = node->right;
                break;
            }
        }
    }
    node->color = NODE_RED;
    if(node->parent->color == NODE_RED) {
        iter = node;
        while(iter != bt->head && iter->parent->color == NODE_RED) {
            bt->head->color = NODE_BLACK;

            if(iter->parent == iter->parent->parent->left) {
                // left child
                if(iter->parent->parent->right == NULL ||
                        iter->parent->parent->right->color == NODE_BLACK) {
                    // black uncle.
                    if(iter == iter->parent->right) {
                        iter = iter->parent;
                        rb_rotate_left(bt, iter);
                    }
                    iter->parent->color = NODE_BLACK;
                    iter->parent->parent->color = NODE_RED;
                    rb_rotate_right(bt, iter->parent->parent);
                    continue;
                } else {
                    // red uncle.
                    iter->parent->color = NODE_BLACK;
                    iter->parent->parent->right->color = NODE_BLACK;
                    iter->parent->parent->color = NODE_RED;
                    iter = iter->parent->parent;
                    bt->head->color = NODE_BLACK;
                    continue;
                }
            } else {
                if(iter->parent->parent->left == NULL ||
                        iter->parent->parent->left->color == NODE_BLACK) {
                    // black uncle.
                    if(iter == iter->parent->left) {
                        iter = iter->parent;
                        rb_rotate_right(bt, iter);
                    }
                    iter->parent->color = NODE_BLACK;
                    iter->parent->parent->color = NODE_RED;
                    rb_rotate_left(bt, iter->parent->parent);
                    continue;
                } else {
                    // red uncle.
                    iter->parent->color = NODE_BLACK;
                    iter->parent->parent->left->color = NODE_BLACK;
                    iter->parent->parent->color = NODE_RED;
                    iter = iter->parent->parent;
                    bt->head->color = NODE_BLACK;
                    continue;
                }
            }
            iter = iter->parent;
        }
    }
    bt->head->color = NODE_BLACK;
}

void *rb_find(rb_tree *bt, void *key) {
    rb_tree_node *node;
    int compare_result;

    if(!bt->head) {
        return NULL;
    }
    node = bt->head;
    while(node != NULL) {
        compare_result = (*bt->compare_function)(key, node->key, bt->token);
        if(compare_result == 0) {
            return node->data;
        } else if(compare_result < 0) {
            // Go Left
            if(node->left != NULL) {
                node = node->left;
            } else {
                return NULL;
            }
        } else {
            if(node->right != NULL) {
                node = node->right;
            } else {
                return NULL;
            }
        }
    }
    /* Shouldn't happen. */
    assert(0);
}

int rb_exists(rb_tree *bt, void *key) {
    rb_tree_node *node;
    int compare_result;
    if(!bt->head) {
        return 0;
    }
    node = bt->head;
    while(node != NULL) {
        compare_result = (*bt->compare_function)(key, node->key, bt->token);
        if(compare_result == 0) {
            return 1;
        } else if(compare_result < 0) {
            // Go Left
            if(node->left != NULL) {
                node = node->left;
            } else {
                return 0;
            }
        } else {
            if(node->right != NULL) {
                node = node->right;
            } else {
                return 0;
            }
        }
    }
    /* Shouldn't happen. */
    assert(0);
}


void *rb_delete(rb_tree *bt, void *key) {
    rb_tree_node *node, *child;
    void *data;
    int compare_result;
    if(!bt->head) {
        return NULL;
    }
    node = bt->head;
    while(node != NULL) {
        compare_result = (*bt->compare_function)(key, node->key, bt->token);
        if(compare_result == 0) {
            break;
        } else if(compare_result < 0) {
            if(node->left != NULL) {
                node = node->left;
            } else {
                return NULL;
            }
        } else {
            if(node->right != NULL) {
                node = node->right;
            } else {
                return NULL;
            }
        }
    }
    data = node->data;
    if(node->left == NULL && node->right == NULL) {
        if(node->parent)
            if(node->parent->left == node) 
                node->parent->left = NULL;
            else 
                node->parent->right = NULL;
        else
            bt->head = NULL;
        free(node);
        return data;
    } else if(node->left == NULL) {
        if(node->parent)
            if(node->parent->left == node)
                node->parent->left = node->right;
            else
                node->parent->right = node->right;
        else 
            bt->head = node->right;
        free(node);
        return data;
    } else if(node->right == NULL) {
        if(node->parent)
            if(node->parent->left == node)
                node->parent->left = node->left;
            else
                node->parent->right = node->left;
        else
            bt->head = node->left;
        free(node);
        return data;
    } else {
        // complicated case. 

        child = rb_find_successor_node(node);
        assert(child != NULL);

        if(child == node->right) {
            node->right = child->right;
        } else {
            child->parent->left = child->right;
        }

        if(child->right) {
            child->right->parent = child->parent;
        }

        node->data = child->data;
        node->key = child->key;
        free(child);
        return data;
    }
}

void rb_walk(rb_tree *bt, int how, 
        void (*callback)(void *, void *, int, void *), void *arg) {
    rb_tree_node *last, *node, *end;
    int depth = 0;
    if(!bt->head)
        return;
    last = NULL;
    node = bt->head;
    while(node != NULL) {
        if(last == node->parent) {
            if(how == WALK_PREORDER)
                (*callback)(node->key, node->data, depth, arg);
            if(node->left != NULL) {
                depth++;
                last = node;
                node = node->left;
                continue;
            }
        }
        if(last == node->left || 
                (last == node->parent && node->left == NULL)) {
            if(how == WALK_INORDER)
                (*callback)(node->key, node->data, depth, arg);
            if(node->right != NULL) {
                depth++;
                last = node;
                node = node->right;
                continue;
            }
        }
        if(how == WALK_POSTORDER)
            (*callback)(node->key, node->data, depth, arg);
        depth--;
        last = node;
        node = node->parent;
    }
}
