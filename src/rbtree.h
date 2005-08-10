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

typedef struct rb_tree_node_t {
    struct rb_tree_node_t *left, *right, *parent;
    void *key;
    void *data;
    int color;
} rb_tree_node;

typedef struct rb_tree_head_t {
    struct rb_tree_node_t *head;
    int (*compare_function)(void *, void *, void *);
    void *token;
} rb_tree;

rb_tree *rb_init(int (*)(void *, void *, void *), void *);
void rb_destroy(rb_tree *);

void rb_insert(rb_tree *, void *key, void *data); 
void *rb_find(rb_tree *, void *key);
int rb_exists(rb_tree *, void *key);
void *rb_delete(rb_tree *, void *key);

void rb_walk(rb_tree *, int, void (*)(void *, void *, int, void *), 
        void *);

#endif
