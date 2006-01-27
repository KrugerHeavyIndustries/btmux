/*
 * db_rw.h
 */
#ifndef __DB_XDR_H__

#include "copyright.h"

struct mmdb_t {
	void *base;
	void *ppos;
	void *end;
	int length;
	int fd;
};

struct mmdb_t *_open_read(char *filename);
struct mmdb_t *_open_write(char *filename);
void mmdb_resize(struct mmdb_t *, int length);
void mmdb_close(struct mmdb_t *);
void *_read(struct mmdb_t *, void *dest, int length);
void mmdb_write(struct mmdb_t *, void *data, int length);
void mmdb_write_opaque(struct mmdb_t *, void *data, int length);

void mmdb_write_uint(struct mmdb_t *, unsigned int); /* Deprecated */
unsigned int mmdb_read_uint(struct mmdb_t *); /* Deprecated */

void mmdb_write_uint32(struct mmdb_t *, uint32_t);
uint32_t mmdb_read_uint32(struct mmdb_t *);
void mmdb_write_uint16(struct mmdb_t *, uint16_t);
uint16_t mmdb_read_uint16(struct mmdb_t *);
void mmdb_write_uint64(struct mmdb_t *, uint64_t);
uint64_t mmdb_read_uint64(struct mmdb_t *);

#endif
