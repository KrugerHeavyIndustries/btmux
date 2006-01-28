/*
 * db_rw.c 
 */
#include "copyright.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <event.h>
#include <string.h>
#include <sys/file.h>
#include <stdint.h>

#include "debug.h"

struct mmdb_t {
	void *base;
	void *ppos;
	void *end;
	int length;
	int fd;
};

struct mmdb_t *mmdb_open_read(char *filename)
{
	struct stat statbuf;
	struct mmdb_t *mmdb;
	int fd, length;

	dperror(stat(filename, &statbuf) < 0);
	mmdb = malloc(sizeof(struct mmdb_t));

	fd = open(filename, O_RDONLY);
	mmdb->fd = fd;
	mmdb->length = (statbuf.st_size + 0x3FF) & ~(0x3FF);
	mmdb->base =
		mmap(NULL, mmdb->length, PROT_READ, MAP_SHARED | MAP_POPULATE,
			 mmdb->fd, 0);
	dperror(mmdb->base == NULL);
	mmdb->end = mmdb->base + mmdb->length;

	mmdb->ppos = mmdb->base;
	return mmdb;
}

struct mmdb_t *mmdb_open_write(char *filename)
{
	struct stat statbuf;
	struct mmdb_t *mmdb;
	int fd, length;

	mmdb = malloc(sizeof(struct mmdb_t));
	dperror((fd = open(filename, O_RDWR | O_CREAT, 0644)) < 0);

	mmdb->fd = fd;
	mmdb->length = 0x400 * 16;
	mmdb->base = NULL;
	mmdb->ppos = NULL;
	dprintk("mmdb->length %d", mmdb->length);
	ftruncate(mmdb->fd, mmdb->length);
	fsync(mmdb->fd);
	mmdb->base =
		mmap(NULL, mmdb->length, PROT_READ | PROT_WRITE, MAP_SHARED, mmdb->fd,
			 0);
	dperror(mmdb->base == MAP_FAILED);
	mmdb->end = mmdb->base + mmdb->length;
	mmdb->ppos = mmdb->base;
	return mmdb;
}

void mmdb_resize(struct mmdb_t *mmdb, int length)
{
	int offset = 0;
	if(mmdb->base) {
		offset = mmdb->ppos - mmdb->base;
		msync(mmdb->base, mmdb->length, MS_ASYNC);
		munmap(mmdb->base, mmdb->length);
		mmdb->base = NULL;
		mmdb->ppos = NULL;
	}
	mmdb->length = (length + 0x3FF) & ~(0x3FF);
	ftruncate(mmdb->fd, mmdb->length);
	mmdb->base =
		mmap(NULL, mmdb->length, PROT_READ | PROT_WRITE, MAP_SHARED, mmdb->fd,
			 0);
	dperror(mmdb->base == NULL);
	mmdb->end = mmdb->base + mmdb->length;
	mmdb->ppos = mmdb->base + offset;
}

void mmdb_close(struct mmdb_t *mmdb)
{
	msync(mmdb->base, mmdb->length, MS_SYNC);
	munmap(mmdb->base, mmdb->length);
	ftruncate(mmdb->fd, mmdb->ppos - mmdb->base);
	dprintk("truncating to %d bytes.", mmdb->ppos - mmdb->base);
	close(mmdb->fd);
	mmdb->fd = 0;
	memset(mmdb, 0, sizeof(struct mmdb_t));
	free(mmdb);
}

void mmdb_write(struct mmdb_t *mmdb, void *data, int length)
{
	if(mmdb->end - mmdb->ppos < length) {
		mmdb_resize(mmdb, mmdb->length + length);
	}
	memcpy(mmdb->ppos, data, length);
	mmdb->ppos += length;
}

void mmdb_write_uint32(struct mmdb_t *mmdb, uint32_t data)
{
	data = htonl(data);
	mmdb_write(mmdb, &data, sizeof(uint32_t));
}

uint32_t mmdb_read_uint32(struct mmdb_t *mmdb)
{
	uint32_t data = *((uint32_t *) (mmdb->ppos));
	mmdb->ppos += sizeof(uint32_t);
	return ntohl(data);
}

unsigned int mmdb_read_uint(struct mmdb_t *mmdb) {
    printk("This function is deprecated, please use mmdb_read_uint32 instead.\n");
    return (unsigned int)mmdb_read_uint32(mmdb);
}

void mmdb_write_uint(struct mmdb_t *mmdb, unsigned int val) {
    printk("This function is deprecated, please use mmdb_write_uint32 instead.\n");
    mmdb_write_uint32(mmdb, (uint32_t)val);
    return;
}

void mmdb_write_uint64(struct mmdb_t *mmdb, uint64_t data)
{
    dprintk("technically, this is not 64 bit safe yet.");
	data = htonl(data);
	mmdb_write(mmdb, &data, sizeof(uint64_t));
}

uint64_t mmdb_read_uint64(struct mmdb_t *mmdb)
{
    dprintk("technically, this is not 64 bit safe yet.");
	uint64_t data = *((uint64_t *) (mmdb->ppos));
	mmdb->ppos += sizeof(uint64_t);
	return ntohl(data);
}

void mmdb_write_single(struct mmdb_t *mmdb, float data)
{
	data = htonl(data);
	mmdb_write(mmdb, &data, sizeof(float));
}

float mmdb_read_single(struct mmdb_t *mmdb)
{
	float data = *((float *) (mmdb->ppos));
	mmdb->ppos += sizeof(float);
	return ntohl(data);
}

void mmdb_write_double(struct mmdb_t *mmdb, double data)
{
	data = htonl(data);
	mmdb_write(mmdb, &data, sizeof(double));
}

double mmdb_read_double(struct mmdb_t *mmdb)
{
	double data = *((double *) (mmdb->ppos));
	mmdb->ppos += sizeof(double);
	return ntohl(data);
}


void *mmdb_read(struct mmdb_t *mmdb, void *dest, int length)
{
	if((mmdb->end - mmdb->ppos) < length)
		return NULL;
	memcpy(dest, mmdb->ppos, length);
	mmdb->ppos += length;
	if(length & 3)
		mmdb->ppos += 4 - (length & 3);
	return dest;
}

void mmdb_write_opaque(struct mmdb_t *mmdb, void *data, int length)
{
	unsigned char *pad = (unsigned char *)"\x00\x00\x00\x00";
	mmdb_write_uint(mmdb, length);
	mmdb_write(mmdb, data, length);
	if((length & 3) > 0)
		mmdb_write(mmdb, pad, 4 - (length & 3));
}

void mmdb_write_string(struct mmdb_t *mmdb, void *data) {
    if(data == NULL) {
        mmdb_write_uint32(mmdb, 0);
    } else {
        mmdb_write_opaque(mmdb, data, strlen(data)+1);
    }
}
