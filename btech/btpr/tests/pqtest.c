#include "pqueue.h"

#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>

typedef struct {
	struct timeval tv;
	void *data;
} example_t;

static int
comp_example(const bt_pq_item_t *a, const bt_pq_item_t *b)
{
	const example_t *real_a = (const example_t *)a;
	const example_t *real_b = (const example_t *)b;

	if (real_a->tv.tv_sec != real_b->tv.tv_sec) {
		return real_a->tv.tv_sec - real_b->tv.tv_sec;
	} else {
		return real_a->tv.tv_usec - real_b->tv.tv_usec;
	}
}

static void
verify_min_heapify(bt_pq_t *pq, int assert_size)
{
	const bt_pq_item_t *this_item;
	const bt_pq_item_t *parent_item;
	int ii;

	assert(bt_pq_nitems(pq) == assert_size);

	for (ii = 1; ii < assert_size; ii++) {
		this_item = bt_pq_get_item(pq, ii);
		parent_item = bt_pq_get_item(pq, (ii - 1) >> 1);

		assert(pq->comp_func(parent_item, this_item) <= 0);
	}
}

int
main(void)
{
	int ii;
	bt_pq_t *pq;
	/* bt_pq_item_t *pq_item; */

	example_t item1, item2, item3;

	/* Initialize example data.  */
	item1.tv.tv_sec = 10;
	item1.tv.tv_usec = 750000;
	item1.data = &item1;

	item2.tv.tv_sec = 10;
	item2.tv.tv_usec = 250000;
	item2.data = &item2;

	item3.tv.tv_sec = 15;
	item3.tv.tv_usec = 500000;
	item3.data = &item3;

	/* Test priority queue operations.  */
	if (!(pq = bt_pq_create(0, sizeof(example_t), comp_example)))
		exit(EXIT_FAILURE);

	for (ii = 0; ii < 6; ii += 3) {
		if (!bt_pq_add_item(pq, (bt_pq_item_t *)&item1))
			exit(EXIT_FAILURE);
		verify_min_heapify(pq, ii + 1);

		if (!bt_pq_add_item(pq, (bt_pq_item_t *)&item2))
			exit(EXIT_FAILURE);
		verify_min_heapify(pq, ii + 2);

		if (!bt_pq_add_item(pq, (bt_pq_item_t *)&item3))
			exit(EXIT_FAILURE);
		verify_min_heapify(pq, ii + 3);
	}

	for (ii = 5; ii >= 0; ii--) {
		bt_pq_remove_item(pq, bt_pq_peek_top(pq));
		verify_min_heapify(pq, ii);
	}

	bt_pq_destroy(pq);

	exit(EXIT_SUCCESS);
}
