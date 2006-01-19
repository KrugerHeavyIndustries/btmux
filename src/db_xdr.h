/*
 * db_rw.h
 */
#include "copyright.h"

struct mmdb_t {
	void *base;
	void *ppos;
	void *end;
	int length;
	int fd;
};

struct mmdb_t *mmdb_open_read(char *filename);

struct mmdb_t *mmdb_open_write(char *filename);

void mmdb_resize(struct mmdb_t *mmdb, int length);

void mmdb_close(struct mmdb_t *mmdb);

void mmdb_write(struct mmdb_t *mmdb, void *data, int length);

void mmdb_write_uint(struct mmdb_t *mmdb, unsigned int data);

unsigned int mmdb_read_uint(struct mmdb_t *mmdb);

void *mmdb_read(struct mmdb_t *mmdb, void *dest, int length);

void mmdb_write_opaque(struct mmdb_t *mmdb, void *data, int length);

void mmdb_write_object(struct mmdb_t *mmdb, dbref object);

struct string_dict_entry {
	char *key;
	char *data;
};

static int mmdb_write_vattr(void *key, void *data, int depth, void *arg);

void mmdb_db_write(char *filename);

int mmdb_db_read(char *filename);
