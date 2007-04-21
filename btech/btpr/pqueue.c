/*
 * Implements a binary heap-based priority queue.  Heaps make it easy to answer
 * the question, "What's the smallest/largest item in my heap?" but they're
 * completely useless for ordered traversal, since a smaller item can be in
 * either the left or right branch of the binary heap.
 *
 * As it turns out, I'm using a priority queue to re-implement the muxevent
 * system, which relies on libevent to execute events in order and doesn't sort
 * its lists at all, so it's all good.
 *
 * This implementation works a lot like the qsort() library function and
 * friends: You need to tell it how big your items are, and you need to provide
 * a comparison function.
 *
 * Items are stored in the heap.  Values passed to bt_pqueue_add_item() are
 * copied into the heap.  Values are also copied in the heap during swap
 * operations.  Item sizes should thus be kept as small as possible.  The
 * upshot, though, is that we get really good cache locality.
 */

#include <stdlib.h>
#include <string.h>

#include "pqueue.h"
#undef bt_pq_nitems
#undef bt_pq_peek_top
#undef bt_pq_get_item

#include <assert.h>


#define MIN_MIN_NITEMS	1 /* 16 for production code? */

#define ITEM_SIZE	(pq->item_size)
#define ITEM_STRIDE	ITEM_SIZE

/*
 * These macros rely on the queue being called "pq".
 */
#define ITEM(i)		(pq->heap + (i) * ITEM_STRIDE)
#define CONST_ITEM(i)	((const bt_pq_item_t *)ITEM(i))

#define ITEM_IDX(p)	(((p) - pq->heap) / ITEM_STRIDE)
#define HAS_ITEM(i)	((i) < pq->nitems)

#define PARENT(i)	((i - 1) >> 1)
#define LEFT(i)		(((i) << 1) + 1)
#define RIGHT(i)	(((i) << 1) + 2)

#define SET_ITEM(i,v)	(memcpy(ITEM(i), (const bt_pq_item_t *)(v), ITEM_SIZE))
#define COPY_ITEM(i,j)	(memcpy(ITEM(i), CONST_ITEM(j), ITEM_SIZE))


/* Heap size = 2^k * ITEM_SIZE.  */

static int
grow_heap(bt_pq_t *pq, int new_nitems)
{
	char *tmp_heap;
	size_t tmp_size;

	/* Try to grow by one step.  */
	/* FIXME: Handle overflow.  */
	if (pq->heap_size >= (new_nitems * ITEM_SIZE))
		return 1;

	tmp_size = pq->heap_size << 1;

	if (!(tmp_heap = (char *)realloc(pq->heap, tmp_size)))
		return 0; /* XXX */

	pq->heap = tmp_heap;
	pq->heap_size = tmp_size;
	return 1;
}

static void
shrink_heap(bt_pq_t *pq, int new_nitems)
{
	char *tmp_heap;
	size_t tmp_size;

	/* Try to shrink by two steps (to add some hysteresis).  */
	tmp_size = pq->heap_size >> 2;

	if (tmp_size < (new_nitems * ITEM_SIZE)) {
		/* Can't shrink by two steps.  */
		return;	
	}

	/* Only actually shrink by one step.  */
	tmp_size = pq->heap_size >> 1;

	if (tmp_size < (pq->min_nitems * ITEM_SIZE)) {
		/* Shrinking would violate min_nitems.  */
		return;
	}

	/* Reallocate.  */
	if (!(tmp_heap = (char *)realloc(pq->heap, tmp_size)))
		return; /* no problem */

	pq->heap = tmp_heap;
	pq->heap_size = tmp_size;
}

bt_pq_t *
bt_pq_create(int min_nitems, size_t item_size, bt_comp_func_t item_comp_func)
{
	bt_pq_t *pq;
	int tmp_nitems;

	/* Initialize priority queue object.  */
	if (!(pq = (bt_pq_t *)malloc(sizeof(bt_pq_t))))
		return NULL;

	pq->nitems = 0;
	pq->comp_func = item_comp_func;
	ITEM_SIZE = item_size; /* force a compile time error when ITEM_SIZE
	                          isn't an lvalue */

	if (min_nitems < MIN_MIN_NITEMS)
		/* Keep the user from shooting us in the foot.  */
		min_nitems = MIN_MIN_NITEMS;

	pq->min_nitems = min_nitems;

	/* Initialize heap.  */
	tmp_nitems = 1;

	while (tmp_nitems < min_nitems)
		/* FIXME: Handle overflow.  */
		tmp_nitems <<= 1;

	pq->heap_size = tmp_nitems * ITEM_SIZE;

	if (!(pq->heap = (char *)malloc(pq->heap_size))) {
		free(pq);
		return NULL;
	}

	return pq;
}

void
bt_pq_destroy(bt_pq_t *pq)
{
	free(pq->heap);
	free(pq);
}


/* Find the smallest among this node and its children to bubble here.  */

typedef enum {
	BUBBLE_NONE = 0,
	BUBBLE_LEFT = -1,
	BUBBLE_RIGHT = 1,
} bubble_t;

static bubble_t
choose_bubble_down(bt_pq_t *pq, const bt_pq_item_t *root_item, int root_ii)
{
	const int left_ii = LEFT(root_ii);
	const int right_ii = RIGHT(root_ii);

	if (!HAS_ITEM(left_ii)) {
		/* No children.  */
		return BUBBLE_NONE;
	}

	if (pq->comp_func(root_item, CONST_ITEM(left_ii)) <= 0) {
		/* <=LEFT, maybe >RIGHT.  */
		if (!HAS_ITEM(right_ii)) {
			/* <=LEFT.  */
			return BUBBLE_NONE;
		}

		if (pq->comp_func(root_item, CONST_ITEM(right_ii)) <= 0) {
			/* <=LEFT and <=RIGHT.  */
			return BUBBLE_NONE;
		} else {
			/* >RIGHT.  */
			return BUBBLE_RIGHT;
		}
	} else {
		/* >LEFT.  Is LEFT or RIGHT smaller?  */
		if (!HAS_ITEM(right_ii)) {
			/* >LEFT.  */
			return BUBBLE_LEFT;
		}

		if (pq->comp_func(CONST_ITEM(left_ii),
		                  CONST_ITEM(right_ii)) < 0) {
			/* LEFT < RIGHT.  */
			return BUBBLE_LEFT;
		} else {
			/* LEFT >= RIGHT.  */
			return BUBBLE_RIGHT;
		}
	}
}

/*
 * Bubble down:
 *
 * 1) If this > L, bubble down left.
 *    If this > R, bubble down right.
 *    If this > L and this > R, bubble down smaller of L and R.
 * 2) Otherwise done.
 * 3) Repeat from step 1 at new position.
 */
static void
bubble_down(bt_pq_t *pq, const bt_pq_item_t *this_item, int this_ii)
{
	for (;;) {
		switch (choose_bubble_down(pq, this_item, this_ii)) {
		case BUBBLE_LEFT:
			COPY_ITEM(this_ii, LEFT(this_ii));
			this_ii = LEFT(this_ii);
			break;

		case BUBBLE_RIGHT:
			COPY_ITEM(this_ii, RIGHT(this_ii));
			this_ii = RIGHT(this_ii);
			break;

		default:
			SET_ITEM(this_ii, this_item);
			return;
		}
	}
}

/*
 * Bubble up:
 *
 * 1) If this < V, bubble up.
 * 2) Otherwise done.
 * 3) Repeat from step 1 at new position.
 */
static void
bubble_up(bt_pq_t *pq, const bt_pq_item_t *this_item, int this_ii)
{
	int parent_ii;

	for (;;) {
		assert(this_ii >= 0);
		if (this_ii == 0) {
			/* No parent.  */
			SET_ITEM(this_ii, this_item);
			return;
		}

		parent_ii = PARENT(this_ii);
		if (pq->comp_func(this_item, CONST_ITEM(parent_ii)) < 0) {
			/* <PARENT.  */
			COPY_ITEM(this_ii, parent_ii);
			this_ii = parent_ii;
		} else {
			/* >=PARENT.  */
			SET_ITEM(this_ii, this_item);
			return;
		}
	}
}

/* item_value is copied into the heap.  */
int
bt_pq_add_item(bt_pq_t *pq, const bt_pq_item_t *item_value)
{
	int this_ii;

	/* Add item to heap.
	 *
	 * 1) Add this item as last leaf in heap.
	 * 2) This item has no children.  If it is smaller than its parent,
	 *    bubble up.
	 */

	/* Grow heap if necessary.  */
	if (!grow_heap(pq, pq->nitems + 1))
		return 0;

	this_ii = pq->nitems++;

	/* Bubble up.  */
	bubble_up(pq, item_value, this_ii);
	return 1;
}

/* item is inside the heap.  */
int
bt_pq_remove_item(bt_pq_t *pq, const bt_pq_item_t *item)
{
	int this_ii, root_ii;

	/* Remove item from heap.
	 *
	 * 1) Replace this item with last leaf in heap.
	 * 2) This leaf generally has no particular relationship with the new
	 *    parent/children, as it may come from a separate subheap.
	 *
	 *    If P <= this, then bubble down (and only down).
	 *    If this < P, then bubble up (and only up).
	 */

	root_ii = ITEM_IDX(item);
	if (root_ii < 0 || !HAS_ITEM(root_ii)) {
		/* Item isn't in this heap.  */
		return 0;
	}

	assert(pq->nitems > 0);
	this_ii = --pq->nitems;

	/* Bubble last leaf into position.  */
	if (root_ii == this_ii) {
		/* Last leaf was removed item.  */
	} else if (root_ii != 0
	           && pq->comp_func(ITEM(this_ii), ITEM(PARENT(root_ii))) < 0) {
		/* Bubble up.  */
		bubble_up(pq, ITEM(this_ii), root_ii);
	} else {
		/* Bubble down.  */
		bubble_down(pq, ITEM(this_ii), root_ii);
	}

	/* Shrink heap if necessary.  */
	shrink_heap(pq, pq->nitems);
	return 1;
}


int
bt_pq_nitems(const bt_pq_t *pq)
{
	return pq->nitems;
}

bt_pq_item_t *
bt_pq_peek_top(const bt_pq_t *pq)
{
	return ITEM(0);
}


/* This isn't fast for heaps in general, but it's a convenient operation for
 * this particular implementation.  Don't rely on it if the implementation is
 * going to change in the future.
 */
bt_pq_item_t *
bt_pq_get_item(const bt_pq_t *pq, int ii)
{
	return ITEM(ii);
}
