#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "rbtree.h"


int count = 0;

rbtree *rb_init(int (*compare_function)(void *, void *, void *), void *token) {
    rbtree *temp;

    temp = malloc(sizeof(rbtree));
    if(temp == NULL) return NULL;
    memset(temp, 0, sizeof(rbtree));
    temp->compare_function = compare_function;
    temp->token = token;
    temp->size = 0;
    return temp;
}

static rbtree_node *rb_find_minimum(rbtree_node *node) {
    rbtree_node *child;
    child = node;
    while(child->left != NULL) child = child->left;
    return child;
}

static rbtree_node *rb_find_maximum(rbtree_node *node) {
    rbtree_node *child;
    child = node;
    while(child->right != NULL) child = child->right;
    return child;
}

static rbtree_node *rb_find_successor_node(rbtree_node *node) {
    rbtree_node *child, *parent;
    if(node->right != NULL) {
        child = node->right;
        while(child->left != NULL) {
            child = child->left;
        }
        return child;
    } else {
        child = node;
        parent = node->parent;
        while(parent != NULL && child==parent->right) {
            child = parent;
            parent = child->parent;
        }
        return parent;
    }
    return NULL;
}

static rbtree_node *rb_find_predecessor_node(rbtree_node *node) {
    rbtree_node *child, *parent;
    if(node->left != NULL) {
        child = node->left;
        while(child->right != NULL) child = child->right;
        return child;
    } else {
        child = node;
        parent = node->parent;
        while(parent != NULL && child==parent->left) {
            child = parent;
            parent = parent->parent;
        }
        return parent;
    }
    return NULL;
}

void rb_destroy(rbtree *bt) {
    rbtree_node *node, *parent;
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
            else if(parent) {
                fprintf(stderr, "serious braindamage in rbtree:rb_destroy\n");
                exit(1);
            }
            free(node);
            node = parent;
        }
    }
    free(bt);
    return;
}

static rbtree_node *rb_allocate(rbtree_node *parent, void *key, void *data) {
    rbtree_node *temp;
    temp = malloc(sizeof(rbtree_node));
    memset(temp, 0, sizeof(rbtree_node));
    temp->parent = parent;
    temp->key = key;
    temp->data = data;
    return temp;
}

static void rb_rotate_right(rbtree *bt, rbtree_node *pivot) {
    rbtree_node *child;

    child = pivot->left;

    pivot->left = child->right;
    if(child->right != NULL)
        child->right->parent = pivot;

    child->parent = pivot->parent;
    
    if(pivot->parent) {
        if(pivot->parent->left == pivot)
            pivot->parent->left = child;
        else
            pivot->parent->right = child;
    } else bt->head = child;
    child->right = pivot;
    pivot->parent = child;
}

static void rb_rotate_left(rbtree *bt, rbtree_node *pivot) {
    rbtree_node *child;

    child = pivot->right;

    pivot->right = child->left;
    if(child->left != NULL)
        child->left->parent = pivot;

    child->parent = pivot->parent;

    if(pivot->parent) {
        if(pivot->parent->right == pivot)
            pivot->parent->right = child;
        else
            pivot->parent->left = child;
    } else bt->head = child;
    child->left = pivot;
    pivot->parent = child;
}



void rb_insert(rbtree *bt, void *key, void *data) {
    rbtree_node *node;
    rbtree_node *iter;
    int compare_result;

    if(!bt->head) {
        bt->head = rb_allocate(NULL, key, data);
        bt->size++;
        bt->head->color = NODE_BLACK;
        return;
    }

    node = bt->head;
    while(node != NULL) {
        compare_result = (*bt->compare_function)(key, node->key, bt->token);
        if(compare_result == 0) {
            // Key already exists, replace data.
            node->key = key;
            node->data = data;
            return;
        } else if(compare_result < 0) {
            // Go Left
            if(node->left != NULL) {
                node = node->left;
            } else {
                node->left = rb_allocate(node, key, data);
                bt->size++;
                node = node->left;
                break;
            }
        } else {
            if(node->right != NULL) {
                node = node->right;
            } else {
                node->right = rb_allocate(node, key, data);
                bt->size++;
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
                    continue;
                }
            }
        }
    }
    bt->head->color = NODE_BLACK;
}

void *rb_find(rbtree *bt, void *key) {
    rbtree_node *node;
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
    fprintf(stderr, "Serious fault in rbtree.c:rb_find!\n");
    exit(1);
}

int rb_exists(rbtree *bt, void *key) {
    rbtree_node *node;
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
    fprintf(stderr, "Serious fault in rbtree.c:rb_exists!\n");
    exit(1);
}

static void rb_rebalance(rbtree *bt, rbtree_node *node) {
    rbtree_node *iterator;
    
    // we get node from rb_delete, assume node != NULL
    while(node && node!=bt->head && node->color == NODE_BLACK) {
        // if node != bt->head, we assume node->parent != NULL
        if(node == node->parent->left) {
            iterator = node->parent->right;
            if(iterator && iterator->color == NODE_RED) {
                iterator->color = NODE_BLACK;
                iterator->parent->color = NODE_RED;
                rb_rotate_left(bt, iterator->parent);
                if(node && node->parent)
                    iterator = node->parent->right;
                else {
                    iterator = NULL;
                }
            }

            if((!iterator || !iterator->left || iterator->left->color != NODE_RED) && 
               (!iterator || !iterator->right || iterator->right->color != NODE_RED)) {
                if(iterator) iterator->color = NODE_RED;
                node = node->parent;
            } else {
                if(iterator && iterator->right && iterator->right->color == NODE_BLACK) {
                    if(iterator->left) iterator->left->color = NODE_BLACK;
                    iterator->color = NODE_RED;
                    rb_rotate_right(bt, iterator);
                    if(node->parent) iterator = node->parent->right;
                    else iterator = NULL;
                }
                if(iterator) {
                    if(iterator->parent)
                        iterator->color = iterator->parent->color;
                    else
                        iterator->color = NODE_BLACK;
                    // after the rotate's, we don't know where bt->head is.
                    if(node->parent)
                        node->parent->color = NODE_BLACK;
                    if(iterator->right)
                        iterator->right->color = NODE_BLACK;
                    if(node->parent) 
                        rb_rotate_left(bt, node->parent);
                    node = bt->head;
                }
            }
        } else {
            // node is bt->head := node != NULL
            iterator = node->parent->left;
            if(iterator && iterator->color == NODE_RED) {
                iterator->color = NODE_BLACK;
                if(node->parent) {
                    node->parent->color = NODE_RED;
                }
                rb_rotate_right(bt, node->parent);
                if(node->parent) {
                    iterator = node->parent->right;
                } else {
                    iterator = NULL;
                }
            }

            if((!iterator || !iterator->right || iterator->right->color != NODE_RED) &&
               (!iterator || !iterator->left || iterator->left->color != NODE_RED)) {
                if(iterator) iterator->color = NODE_RED;
                node = node->parent;
            } else {
                if(iterator && iterator->left && iterator->left->color == NODE_BLACK) {
                    if(iterator->right) iterator->right->color = NODE_BLACK;
                    iterator->color = NODE_RED;
                    rb_rotate_left(bt, iterator);
                    if(node->parent) iterator = node->parent->left;
                    else iterator = NULL;
                }

                if(iterator && node->parent) {
                    iterator->color = node->parent->color;
                    node->parent->color = NODE_BLACK;
                    if(iterator->left) iterator->left->color = NODE_BLACK;
                    rb_rotate_right(bt, node->parent);
                    node = bt->head;
                }
            }
        }
    }
    if(node) node->color = NODE_BLACK;
}
 

void *rb_delete(rbtree *bt, void *key) {
    rbtree_node *node, *child, *tail;
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

    if(node->left == NULL || node->right == NULL) {
        child=node;
    } else {
        child=rb_find_successor_node(node);
    }
  
    if(child->left != NULL) {
        tail = child->left;
    } else {
        tail = child->right;
    }

    if(tail) tail->parent = child->parent;

    if(child->parent == NULL) {
        bt->head = tail;
    } else {
        if(child == child->parent->left) {
            child->parent->left = tail;
        } else {
            child->parent->right = tail;
        }
    }

    if(child != node) {
        node->key = child->key;
        node->data = child->data;
    }

    if(child->color == NODE_RED && tail) {
        rb_rebalance(bt, tail);
    }
    
    bt->size--;
    free(child);
    return data;
}

void rb_walk(rbtree *bt, int how, 
        int (*callback)(void *, void *, int, void *), void *arg) {
    rbtree_node *last, *node;
    int depth = 0;
    if(!bt->head)
        return;
    last = NULL;
    node = bt->head;
    while(node != NULL) {
        if(last == node->parent) {
            if(how == WALK_PREORDER)
                if(!(*callback)(node->key, node->data, depth, arg)) return;
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
                if(!(*callback)(node->key, node->data, depth, arg)) return;
            if(node->right != NULL) {
                depth++;
                last = node;
                node = node->right;
                continue;
            }
        }
        if(how == WALK_POSTORDER)
            if(!(*callback)(node->key, node->data, depth, arg)) return;
        depth--;
        last = node;
        node = node->parent;
    }
}

unsigned int rb_size(rbtree *bt) {
    return bt->size;
}

void *rb_search(rbtree *bt, int method, void *key) {
    rbtree_node *node, *last;
    int compare_result;
    int found = 0;

    if(!bt->head) {
        return NULL;
    }

    if(method == SEARCH_FIRST) {
        node = rb_find_minimum(bt->head);
        return node->data;
    } else if(method == SEARCH_LAST) {
        node = rb_find_maximum(bt->head);
        return node->data;
    }

    node = bt->head;
    while(node != NULL) {
        last = node;
        compare_result = (*bt->compare_function)(key, node->key, bt->token);
        if(compare_result == 0) {
            found = 1;
            break;
        } else if(compare_result < 0) {
            // Go Left
            if(node->left != NULL) {
                node = node->left;
            } else {
                node = NULL;
                break;
            }
        } else {
            if(node->right != NULL) {
                node = node->right;
            } else {
                node = NULL;
                break;
            }
        }
    }

    if(found && (method == SEARCH_EQUAL || method == SEARCH_LTEQ || method == SEARCH_GTEQ)) {
        return node->data;
    }

    if(!found && (method == SEARCH_EQUAL || method == SEARCH_NEXT || method == SEARCH_PREV)) {
        return NULL;
    }

    if(method == SEARCH_GTEQ || (!found && method==SEARCH_GT)) {
        if(compare_result > 0) {
            node = rb_find_successor_node(last);
            return node->data;
        } else {
            return last->data;
        }
    }

    if(method == SEARCH_LTEQ || (!found && method == SEARCH_LT)) {
        if(compare_result < 0) {
            node = rb_find_predecessor_node(last);
            return node->data;
        } else {
            return last->data;
        }
    }

    if(method == SEARCH_NEXT || (found && method == SEARCH_GT)) {
        node = rb_find_successor_node(node);
        return node->data;
    }

    if(method == SEARCH_PREV || (found && method == SEARCH_LT)) {
        node = rb_find_predecessor_node(node);
        return node->data;
    }

    return NULL;
}
