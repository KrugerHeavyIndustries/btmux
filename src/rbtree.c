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
 * $Id $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
	int count;
} rbtree_node;

typedef struct rbtree_head_t {
	struct rbtree_node_t *head;
	int (*compare_function) (void *, void *, void *);
	void *token;
	unsigned int size;
} *rbtree;

rbtree rb_init(int (*)(void *, void *, void *), void *);
void rb_destroy(rbtree);

void rb_insert(rbtree, void *key, void *data);
void *rb_find(rbtree, void *key);
int rb_exists(rbtree, void *key);
void *rb_delete(rbtree, void *key);

void rb_walk(rbtree, int, int (*)(void *, void *, int, void *), void *);
unsigned int rb_size(rbtree);
void *rb_search(rbtree, int, void *);

rbtree rb_init(int (*compare_function) (void *, void *, void *), void *token)
{
	rbtree temp;

	temp = malloc(sizeof(struct rbtree_head_t));
	if(temp == NULL)
		return NULL;
	memset(temp, 0, sizeof(struct rbtree_head_t));
	temp->compare_function = compare_function;
	temp->token = token;
	temp->size = 0;
	return temp;
}

static rbtree_node *rb_find_minimum(rbtree_node * node)
{
	rbtree_node *child;
	child = node;
	if(!node)
		return NULL;
	while (child->left != NULL)
		child = child->left;
	return child;
}

static rbtree_node *rb_find_maximum(rbtree_node * node)
{
	rbtree_node *child;
	child = node;
	if(!node)
		return NULL;
	while (child->right != NULL)
		child = child->right;
	return child;
}

static rbtree_node *rb_find_successor_node(rbtree_node * node)
{
	rbtree_node *child, *parent;
	if(!node)
		return NULL;
	if(node->right != NULL) {
		child = node->right;
		while (child->left != NULL) {
			child = child->left;
		}
		return child;
	} else {
		child = node;
		parent = node->parent;
		while (parent != NULL && child == parent->right) {
			child = parent;
			parent = child->parent;
		}
		return parent;
	}
	return NULL;
}

static rbtree_node *rb_find_predecessor_node(rbtree_node * node)
{
	rbtree_node *child, *parent;
	if(!node)
		return NULL;
	if(node->left != NULL) {
		child = node->left;
		while (child->right != NULL)
			child = child->right;
		return child;
	} else {
		child = node;
		parent = node->parent;
		while (parent != NULL && child == parent->left) {
			child = parent;
			parent = parent->parent;
		}
		return parent;
	}
	return NULL;
}

void rb_release(rbtree bt, void (*release) (void *, void *, void *),
				void *arg)
{
	rbtree_node *node, *parent;
	node = bt->head;

	if(bt->head) {
		while (node != NULL) {
			if(node->left != NULL) {
				node = node->left;
				continue;
			} else if(node->right != NULL) {
				node = node->right;
				continue;
			} else {
				parent = node->parent;
				if(parent && parent->left == node)
					parent->left = NULL;
				else if(parent && parent->right == node)
					parent->right = NULL;
				else if(parent) {
					fprintf(stderr, "serious braindamage.\n");
					exit(1);
				}
				release(node->key, node->data, arg);
				free(node);
				node = parent;
			}
		}
	}
	free(bt);
	return;
}

void rb_destroy(rbtree bt)
{
	rbtree_node *node, *parent;
	node = bt->head;

	if(bt->head) {
		while (node != NULL) {
			if(node->left != NULL) {
				node = node->left;
				continue;
			} else if(node->right != NULL) {
				node = node->right;
				continue;
			} else {
				parent = node->parent;
				if(parent && parent->left == node)
					parent->left = NULL;
				else if(parent && parent->right == node)
					parent->right = NULL;
				else if(parent) {
					fprintf(stderr, "serious braindamage.\n");
					exit(1);
				}
				free(node);
				node = parent;
			}
		}
	}
	free(bt);
	return;
}

static rbtree_node *rb_allocate(rbtree_node * parent, void *key, void *data)
{
	rbtree_node *temp;
	temp = malloc(sizeof(struct rbtree_node_t));
	memset(temp, 0, sizeof(struct rbtree_node_t));
	temp->parent = parent;
	temp->key = key;
	temp->data = data;
	temp->count = 1;
	return temp;
}

static void rb_rotate_right(rbtree bt, rbtree_node * pivot)
{
	rbtree_node *child;

	if(!pivot || !pivot->left)
		return;
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
	} else
		bt->head = child;
	child->right = pivot;
	pivot->parent = child;
	child->count = pivot->count;
	pivot->count =
		1 + (pivot->left ? pivot->left->count : 0) +
		(pivot->right ? pivot->right->count : 0);
}

static void rb_rotate_left(rbtree bt, rbtree_node * pivot)
{
	rbtree_node *child;

	if(!pivot || !pivot->right)
		return;
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
	} else
		bt->head = child;
	child->left = pivot;
	pivot->parent = child;
	child->count = pivot->count;
	pivot->count =
		1 + (pivot->left ? pivot->left->count : 0) +
		(pivot->right ? pivot->right->count : 0);
}

void rb_insert(rbtree bt, void *key, void *data)
{
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
	while (node != NULL) {
		compare_result = (*bt->compare_function) (key, node->key, bt->token);
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

	iter = node->parent;
	while (iter) {
		iter->count++;
		iter = iter->parent;
	}

	node->color = NODE_RED;
	if(node->parent && node->parent->color == NODE_RED) {
		iter = node;
		while (iter != bt->head && iter->parent->parent
			   && iter->parent->color == NODE_RED) {
			bt->head->color = NODE_BLACK;
			if(iter->parent == iter->parent->parent->left) {
				// parent is left child of grandparent
				if(iter->parent->parent->right != NULL &&
				   iter->parent->parent->right->color == NODE_RED) {
					// Case 1:
					// The current node has a red uncle and it's parent is parent node is a 
					// red left child. 
					iter->parent->color = NODE_BLACK;
					iter->parent->parent->color = NODE_RED;
					if(iter->parent->parent->right)
						iter->parent->parent->right->color = NODE_BLACK;
					iter = iter->parent->parent;
					continue;
				} else {
					// Case 2 or 3:
					// The current node has a black uncle.
					if(iter->parent->right == iter) {
						// Case 2:
						// The current node has a black uncle and is the right child
						// of the parent. The parent is the red left child. The parent's
						// sibling, the current node's uncle, is black.
						rb_rotate_left(bt, iter->parent);
						iter = iter->left;
					}
					// Case 3:
					// The current node is a left child. It's parent is a red left child
					// and has a black sibling. 
					iter->parent->color = NODE_BLACK;
					iter->parent->parent->color = NODE_RED;
					rb_rotate_right(bt, iter->parent->parent);
					break;
				}
			} else {
				// parent is right child of grandparent
				if(iter->parent->parent->left != NULL &&
				   iter->parent->parent->left->color == NODE_RED) {
					// Case 1:
					// The current node has a red uncle and it's parent is parent node is a 
					// red right child. 
					iter->parent->color = NODE_BLACK;
					iter->parent->parent->color = NODE_RED;
					if(iter->parent->parent->left)
						iter->parent->parent->left->color = NODE_BLACK;
					iter = iter->parent->parent;
					continue;
				} else {
					// Case 2 or 3:
					// The current node has a black uncle.
					if(iter->parent->left == iter) {
						// Case 2:
						// The current node has a black uncle and is the left child
						// of the parent. The parent is the red right child. The parent's
						// sibling, the current node's uncle, is black.
						rb_rotate_right(bt, iter->parent);
						iter = iter->right;
					}
					// Case 3:
					// The current node is a right child. It's parent is a red right child
					// and has a black sibling. 
					iter->parent->color = NODE_BLACK;
					iter->parent->parent->color = NODE_RED;
					rb_rotate_left(bt, iter->parent->parent);
					continue;
				}
			}
		}
	}
	bt->head->color = NODE_BLACK;
}

void *rb_find(rbtree bt, void *key)
{
	rbtree_node *node;
	int compare_result;

	if(!bt->head) {
		return NULL;
	}
	node = bt->head;
	while (node != NULL) {
		compare_result = (*bt->compare_function) (key, node->key, bt->token);
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

int rb_exists(rbtree bt, void *key)
{
	rbtree_node *node;
	int compare_result;
	if(!bt->head) {
		return 0;
	}
	node = bt->head;
	while (node != NULL) {
		compare_result = (*bt->compare_function) (key, node->key, bt->token);
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

#define rbann(format, args...) printf("%d: " format "\n", __LINE__, ##args)
#define rbfail(format, args...) do { printf("%d: " format "\n", __LINE__, ##args); abort(); } while (0)

static void rb_unlink_leaf(rbtree bt, rbtree_node * leaf)
{
	rbtree_node *sibling = NULL, *node;

	node = leaf;

	if(node->color == NODE_RED) {
		// if node is red and has at most one child, then it has no child.
		if(node->parent->left == node) {
			node->parent->left = NULL;
		} else {
			node->parent->right = NULL;
		}
		node->parent = NULL;
		return;
	}
	// node is black so it has only one red child, two black children, or no children.
	// If it had two children, we would've handled that in rb_delete()
	if(node->left) {
		if(node == bt->head) {
			bt->head = node->left;
			node->left->parent = NULL;
		} else if(node->parent->left == node) {
			node->parent->left = node->left;
			node->left->parent = node->parent;
		} else {
			node->parent->right = node->left;
			node->left->parent = node->parent;
		}
		if(node->color == NODE_BLACK) {
			if(node->left->color == NODE_RED) {
				node->left->color = NODE_BLACK;
			} else {
				rbfail("shit.");
			}
		}
		node->parent = NULL;
		node->left = NULL;
		return;
	}

	if(node->right) {
		if(node == bt->head) {
			bt->head = node->right;
			node->right->parent = NULL;
		} else if(node->parent->right == node) {
			node->parent->right = node->right;
			node->right->parent = node->parent;
		} else {
			node->parent->left = node->right;
			node->right->parent = node->parent;
		}
		if(node->color == NODE_BLACK) {
			if(node->right->color == NODE_RED) {
				node->right->color = NODE_BLACK;
			} else {
				rbfail("shit.");
			}
		}
		node->right = NULL;
		node->left = NULL;
		return;
	}
	// node is black and has no children, if it had two children, then rb_delete
	// would have handled the situation. Since the node is black and has no
	// children, things get complicated.

	while (node != bt->head) {
		// First we loop through the Case 2a situations.
		// 
		if(node->parent->left == node) {
			sibling = node->parent->right;
		} else {
			sibling = node->parent->left;
		}
		// if the parent is black, it has two black children, or no children.
		// since we are a child, we're guaranteed a sibling.
		if(!sibling)			// Sanity Check
			rbfail
				("serious braindamage: black child of black parent has no sibling.");
		if(node->parent->color == NODE_BLACK && sibling->color == NODE_BLACK
		   && (!sibling->right || sibling->right->color == NODE_BLACK)
		   && (!sibling->left || sibling->left->color == NODE_BLACK)) {
			sibling->color = NODE_RED;
			node = node->parent;
			continue;
		}
		break;
	}

	if(node == bt->head) {
		node->color = NODE_BLACK;
		goto done;
	}

	if(node->parent->left == node) {
		sibling = node->parent->right;
	} else {
		sibling = node->parent->left;
	}

	if(node->parent->color == NODE_BLACK && sibling
	   && sibling->color == NODE_RED && (!sibling->right
										 || sibling->right->color ==
										 NODE_BLACK) && (!sibling->left
														 || sibling->left->
														 color ==
														 NODE_BLACK)) {
		node->parent->color = NODE_RED;
		sibling->color = NODE_BLACK;
		if(node->parent->left == node) {
			rb_rotate_left(bt, node->parent);
			sibling = node->parent->right;
		} else {
			rb_rotate_right(bt, node->parent);
			sibling = node->parent->left;
		}
	}

	if(!sibling && node->parent->color == NODE_RED) {
		node->parent->color = NODE_BLACK;
		goto done;
	}

	if(node->parent->color == NODE_RED && sibling->color == NODE_BLACK &&
	   (!sibling->right || sibling->right->color == NODE_BLACK) &&
	   (!sibling->left || sibling->left->color == NODE_BLACK)) {

		sibling->color = NODE_RED;
		node->parent->color = NODE_BLACK;
		goto done;
	}

	if(node->parent->left == node) {

		if(sibling->color == NODE_BLACK &&
		   (sibling->left && sibling->left->color == NODE_RED) &&
		   (!sibling->right || sibling->right->color == NODE_BLACK)) {
			sibling->color = NODE_RED;
			sibling->left->color = NODE_BLACK;
			rb_rotate_right(bt, sibling);
			sibling = sibling->parent;
		}

		if(sibling->color == NODE_BLACK &&
		   (sibling->right && sibling->right->color == NODE_RED)) {
			sibling->right->color = NODE_BLACK;
			sibling->color = sibling->parent->color;
			sibling->parent->color = NODE_BLACK;
			rb_rotate_left(bt, sibling->parent);
		}
	} else {

		if(sibling->color == NODE_BLACK &&
		   (sibling->right && sibling->right->color == NODE_RED) &&
		   (!sibling->left || sibling->left->color == NODE_BLACK)) {
			sibling->color = NODE_RED;
			sibling->right->color = NODE_BLACK;
			rb_rotate_left(bt, sibling);
			sibling = sibling->parent;
		}

		if(sibling->color == NODE_BLACK &&
		   (sibling->left && sibling->left->color == NODE_RED)) {
			sibling->left->color = NODE_BLACK;
			sibling->color = sibling->parent->color;
			sibling->parent->color = NODE_BLACK;
			rb_rotate_right(bt, sibling->parent);
		}
	}

  done:
	if(leaf->parent->left == leaf) {
		leaf->parent->left = NULL;
	} else if(leaf->parent->right == leaf) {
		leaf->parent->right = NULL;
	} else {
		rbfail("major braindamage.");
	}
	return;
}

void *rb_delete(rbtree bt, void *key)
{
	rbtree_node *node = NULL, *child = NULL, *tail;
	void *data;
	int compare_result;

	if(!bt->head) {
		return NULL;
	}

	node = bt->head;
	while (node != NULL) {
		compare_result = (*bt->compare_function) (key, node->key, bt->token);
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

	if(node == NULL) {
		return node;
	}

	data = node->data;
	bt->size--;

	// XXX: handle deleting the head.

	if(node == bt->head && node->left == NULL && node->right == NULL) {
		bt->head = NULL;
		free(node);
		return data;
	}

	/* 
	 * PROPERTY 3 OF RED BLACK TREES STATES:
	 * 
	 * Any two paths from a given node v down to a leaf node contain
	 * the same number of black nodes.
	 *
	 * MEANING:
	 * That all paths to all leaf nodes should contain the same
	 * number of black nodes. Thus, we need to handle deleting a
	 * black node in every situation, even if it is a leaf.
	 */

	// our child has at most one child (or none.)
	if(node->left == NULL || node->right == NULL) {
		tail = node;
		while (tail) {
			tail->count--;
			tail = tail->parent;
		}
		rb_unlink_leaf(bt, node);
		free(node);
		return data;
	}
	// If we have full children, then we're guaranteed a successor
	// without empty children.

	child = rb_find_successor_node(node);
	tail = child;
	while (tail) {
		tail->count--;
		tail = tail->parent;
	}
	rb_unlink_leaf(bt, child);

	node->data = child->data;
	node->key = child->key;

	// XXX: finish delete

	free(child);
	return data;
}

void rb_walk(rbtree bt, int how,
			 int (*callback) (void *, void *, int, void *), void *arg)
{
	rbtree_node *last, *node;
	int depth = 0;
	if(!bt->head)
		return;
	last = NULL;
	node = bt->head;
	while (node != NULL) {
		if(last == node->parent) {
			if(how == WALK_PREORDER)
				if(!(*callback) (node->key, node->data, depth, arg))
					return;
			if(node->left != NULL) {
				depth++;
				last = node;
				node = node->left;
				continue;
			}
		}
		if(last == node->left || (last == node->parent && node->left == NULL)) {
			if(how == WALK_INORDER)
				if(!(*callback) (node->key, node->data, depth, arg))
					return;
			if(node->right != NULL) {
				depth++;
				last = node;
				node = node->right;
				continue;
			}
		}
		if(how == WALK_POSTORDER)
			if(!(*callback) (node->key, node->data, depth, arg))
				return;
		depth--;
		last = node;
		node = node->parent;
	}
}

unsigned int rb_size(rbtree bt)
{
	return bt->size;
}

void *rb_search(rbtree bt, int method, void *key)
{
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
	while (node != NULL) {
		last = node;
		compare_result = (*bt->compare_function) (key, node->key, bt->token);
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

	if(found
	   && (method == SEARCH_EQUAL || method == SEARCH_LTEQ
		   || method == SEARCH_GTEQ)) {
		if(node)
			return node->data;
		else
			return NULL;
	}

	if(!found
	   && (method == SEARCH_EQUAL || method == SEARCH_NEXT
		   || method == SEARCH_PREV)) {
		return NULL;
	}

	if(method == SEARCH_GTEQ || (!found && method == SEARCH_GT)) {
		if(compare_result > 0) {
			node = rb_find_successor_node(last);
			if(node)
				return node->data;
			else
				return node;
		} else {
			if(last)
				return last->data;
			else
				return last;
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
		if(node)
			return node->data;
		else
			return node;
	}

	if(method == SEARCH_PREV || (found && method == SEARCH_LT)) {
		node = rb_find_predecessor_node(node);
		if(node)
			return node->data;
		else
			return node;
	}

	return NULL;
}

void *rb_index(rbtree bt, int index)
{
	rbtree_node *iter;
	int leftcount;
	iter = bt->head;

	while (iter) {
		leftcount = (iter->left ? iter->left->count : 0);

		if(index == leftcount) {
			return iter->data;
		}
		if(index < leftcount) {
			iter = iter->left;
		} else {
			index -= leftcount + 1;
			iter = iter->right;
		}
	}
	rbfail("major braindamage.");
}
