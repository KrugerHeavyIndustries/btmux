#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "map.h"

typedef int /* unsigned char */ MagicType;

static const MagicType DYNAMIC_MAGIC = 0x67134269 /* 42 */;

static void
die(const char *reason)
{
	perror(reason);
	exit(EXIT_FAILURE);
}

static const char *const type_strings[] = {
	"MECH", "DEBUG", "MECHREP", "MAP", "AUTO", "TURRET"
};

typedef struct MapList {
	MAP map;
	struct MapList *next;
} MapList;

static MapList *list_head = NULL;
static MapList **list_tail = &list_head;

static void
add_map(MAP *next_map)
{
	MapList *new_item;

	if (!(new_item = (MapList *)malloc(sizeof(MapList))))
		die("malloc(MapList)");

	memcpy(&new_item->map, next_map, sizeof(MAP));
	new_item->next = NULL;

	*list_tail = new_item;
	list_tail = &new_item->next;
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	long offset;
	int ibuf;

	int key, type;
	size_t size;

	const MapList *cursor;
	MagicType magic;

	if (argc != 2) {
		fputs("Usage: hcode <hcode.db>\n", stderr);
		exit(EXIT_FAILURE);
	}

	fp = fopen(argv[1], "rb");

	if (fread(&ibuf, sizeof(ibuf), 1, fp) != 1)
		die("fread(magic)");

	/* Parse XCODE tree.  */
	for (;;) {
		offset = ftell(fp);

		if (fread(&key, sizeof(key), 1, fp) != 1)
			die("fread(key)");

		if (key < 0)
			break;

		if (fread(&type, sizeof(type), 1, fp) != 1)
			die("fread(type)");

		if (fread(&size, sizeof(size), 1, fp) != 1)
			die("fread(size)");

		if (1/*type == GTYPE_MAP*/) {
			printf("[%08lX] #%4d: %s, %u bytes\n",
			       offset, key, type_strings[type], size);
		}

		if (type == GTYPE_MAP) {
			assert(size == sizeof(MAP));

			MAP tmp_map;

			if (fread(&tmp_map, sizeof(MAP), 1, fp) != 1)
				die("fread(data:MAP)");

			add_map(&tmp_map);

			printf("\tfirst_free: %d\n", tmp_map.first_free);
		} else {
			if (fseek(fp, size, SEEK_CUR) != 0)
				die("fseek(data)");
		}
	}

	printf("[%08lX] END XCODE TREE\n", offset);

	/* Parse mapdynamic.  */
	for (cursor = list_head; cursor; cursor = cursor->next) {
		const int count = cursor->map.first_free;
		long seek_size;

		printf("[%08lX] #%4d: first_free = %d\n",
		       ftell(fp), cursor->map.mynum, count);

		seek_size = count * sizeof(cursor->map.mechsOnMap[0]);
		printf("\tskipping %ld bytes...\n", seek_size);
		if (fseek(fp, seek_size, SEEK_CUR) != 0)
			die("fseek(mechsOnMap)");

		seek_size = count * sizeof(cursor->map.mechflags[0]);
		printf("\tskipping %ld bytes...\n", seek_size);
		if (fseek(fp, count * sizeof(char), SEEK_CUR) != 0)
			die("fseek(mechflags)");

		seek_size = count * count * sizeof(cursor->map.LOSinfo[0][0]);
		printf("\tskipping %ld bytes...\n", seek_size);
		if (fseek(fp, seek_size, SEEK_CUR) != 0)
			die("fseek(LOSinfo)");

		if (fread(&magic, sizeof(magic), 1, fp) != 1)
			die("fread(DYNAMIC_MAGIC)");

		if (magic != DYNAMIC_MAGIC) {
			fprintf(stderr, "magic mismatch: 0x%08X\n", magic);
			exit(EXIT_FAILURE);
		}
	}

	printf("[%08lX] END MAPDYNAMIC\n", ftell(fp));

	/* Repairs.  */

	fclose(fp);

	exit(EXIT_SUCCESS);
}
