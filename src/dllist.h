/*
 * Doubly Linked List
 */

/* Doubly Linked List Node */
typedef struct dllist_node_t {
    struct dllist_node_t *next, *prev;
    void *data;
} dllist_node;

/* DLLIST - Doesn't store data just
 * size of list and has pointer to
 * head node and tail node */
typedef struct dllist_t {
    struct dllist_node_t *head, *tail;
    unsigned int size;
} dllist;

/* The various create functions */
dllist *dllist_create_list();
dllist_node *dllist_create_node(void *data);

/* The different destroy functions
 * destroy_node returns the data */
int dllist_destroy_list(dllist *ddlist);
void *dllist_destroy_node(dllist_node *node);   /* Shouldn't include this one but whatever */

/* The various insert functions */
void dllist_insert_after(dllist *dllist, dllist_node *node, dllist_node *newnode);
void dllist_insert_before(dllist *dllist, dllist_node *node, dllist_node *newnode);
void dllist_insert_beginning(dllist *dllist, dllist_node *newnode);
void dllist_insert_end(dllist *dllist, dllist_node *newnode);

/* Remove nodes and return the data */
void *dllist_remove(dllist *dllist, dllist_node *node);
void *dllist_remove_node_at_pos(dllist *dllist, int pos);

/* Utility functions */
dllist_node *dllist_head(dllist *dllist);
dllist_node *dllist_tail(dllist *dllist);
dllist_node *dllist_next(dllist_node *node);
dllist_node *dllist_prev(dllist_node *node);

void *dllist_data(dllist_node *node);
int dllist_size(dllist *dllist);
void *dllist_get_node(dllist *dllist, int pos);
