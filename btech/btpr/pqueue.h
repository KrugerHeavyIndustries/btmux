#ifndef BT_PQUEUE_H
#define BT_PQUEUE_H

#include <stddef.h>

/* bt_pq_item_t is defined as a single char.  The actual item size is variable.
 * Using bt_pq_item_t * instead of char * or void * makes it easier to convert
 * this code to a fixed bt_pq_item_t type, simply by redefining bt_pq_item_t
 * and removing some of the pointer arithmetic.
 */

typedef struct bt_tag_pq bt_pq_t;
typedef char bt_pq_item_t;

/* -1 if a < b, 0 if a == b, and 1 if a > b */
typedef int (*bt_comp_func_t)(const bt_pq_item_t *, const bt_pq_item_t *);

struct bt_tag_pq {
	bt_pq_item_t *heap;		/* heap of items */
	int nitems;			/* number of items on heap */

	bt_comp_func_t comp_func;	/* item comparison function */
	int min_nitems;			/* minimum heap size (in items) */

	size_t heap_size;		/* heap size (in bytes) */

	size_t item_size;		/* item size (in bytes) */
}; /* struct bt_tag_pq */


extern bt_pq_t *bt_pq_create(int, size_t, bt_comp_func_t);
extern void bt_pq_destroy(bt_pq_t *);


extern int bt_pq_add_item(bt_pq_t *, const bt_pq_item_t *);
extern int bt_pq_remove_item(bt_pq_t *, const bt_pq_item_t *);


extern int bt_pq_nitems(const bt_pq_t *) __attribute__ ((pure));
#define bt_pq_nitems(q) ((q)->nitems)

extern bt_pq_item_t *bt_pq_peek_top(const bt_pq_t *) __attribute__ ((pure));
#define bt_pq_peek_top(q) ((q)->heap)


/* XXX: bt_pq_get_item() macro evaluates q multiple times.  */
extern bt_pq_item_t *bt_pq_get_item(const bt_pq_t *, int) __attribute__ ((pure));
#define bt_pq_get_item(q,i) ((q)->heap + (i) * (q)->item_size)


#endif /* undef BT_PQUEUE_H */
