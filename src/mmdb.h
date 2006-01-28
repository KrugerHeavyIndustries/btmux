/*
 * db_rw.h
 */
#ifndef __MMDB_H__

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
void mmdb_resize(struct mmdb_t *, int length);
void mmdb_close(struct mmdb_t *);
void *mmdb_read(struct mmdb_t *, void *dest, int length);
void mmdb_write(struct mmdb_t *, void *data, int length);

void mmdb_write_opaque(struct mmdb_t *, void *data, int length);
void mmdb_write_string(struct mmdb_t *, char *data);

void mmdb_write_uint(struct mmdb_t *, unsigned int); /* Deprecated */
unsigned int mmdb_read_uint(struct mmdb_t *); /* Deprecated */

#define mmdb_write_uint8(db,val) mmdb_write_uint32(db, (uint32_t)val);
#define mmdb_read_uint8(db) (uint8_t)mmdb_read_uint32(db);
#define mmdb_write_uint16(db,val) mmdb_write_uint32(db, (uint32_t)val);
#define mmdb_read_uint16(db) (uint8_t)mmdb_read_uint32(db);

void mmdb_write_uint32(struct mmdb_t *, uint32_t);
uint32_t mmdb_read_uint32(struct mmdb_t *);
void mmdb_write_uint64(struct mmdb_t *, uint64_t);
uint64_t mmdb_read_uint64(struct mmdb_t *);

void mmdb_write_single(struct mmdb_t *, float);
float mmdb_read_single(struct mmdb_t *);
void mmdb_write_double(struct mmdb_t *, double);
double mmdb_read_double(struct mmdb_t *);


#endif
