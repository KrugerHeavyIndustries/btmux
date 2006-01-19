/*
 * db_rw.c 
 */
#include "copyright.h"
#include "config.h"
#include "db_xdr.h"

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

#include "mudconf.h"
#include "config.h"
#include "externs.h"
#include "db.h"
#include "vattr.h"
#include "attrs.h"
#include "alloc.h"
#include "powers.h"
#include "debug.h"

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
		mmap(NULL, mmdb->length, PROT_READ, MAP_SHARED,
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

void mmdb_write_uint(struct mmdb_t *mmdb, unsigned int data)
{
	data = htonl(data);
	mmdb_write(mmdb, &data, sizeof(unsigned int));
}

unsigned int mmdb_read_uint(struct mmdb_t *mmdb)
{
	unsigned int data = *((unsigned int *) (mmdb->ppos));
	mmdb->ppos += sizeof(unsigned int);
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
	unsigned char *pad = "\x00\x00\x00\x00";
	mmdb_write_uint(mmdb, length);
	mmdb_write(mmdb, data, length);
	if((length & 3) > 0)
		mmdb_write(mmdb, pad, 4 - (length & 3));
}

extern OBJ *db;

void mmdb_write_object(struct mmdb_t *mmdb, dbref object)
{
	ATRLIST *atrlist;

	mmdb_write_uint(mmdb, object);
	mmdb_write_opaque(mmdb, Name(object), strlen(Name(object)) + 1);
	mmdb_write_uint(mmdb, Location(object));
	mmdb_write_uint(mmdb, Zone(object));
	mmdb_write_uint(mmdb, Contents(object));
	mmdb_write_uint(mmdb, Exits(object));
	mmdb_write_uint(mmdb, Link(object));
	mmdb_write_uint(mmdb, Next(object));
	mmdb_write_uint(mmdb, Owner(object));
	mmdb_write_uint(mmdb, Parent(object));
	mmdb_write_uint(mmdb, Pennies(object));
	mmdb_write_uint(mmdb, Flags(object));
	mmdb_write_uint(mmdb, Flags2(object));
	mmdb_write_uint(mmdb, Flags3(object));
	mmdb_write_uint(mmdb, Powers(object));
	mmdb_write_uint(mmdb, Powers2(object));
	mmdb_write_uint(mmdb, db[object].at_count);
	atrlist = db[object].ahead;
	for(int i = 0; i < db[object].at_count; i++) {
		mmdb_write_opaque(mmdb, atrlist[i].data, atrlist[i].size);
		mmdb_write_uint(mmdb, atrlist[i].number);
	}
}

#define DB_MAGIC 0x4841475A
#define DB_VERSION 3

static int mmdb_write_vattr(void *key, void *data, int depth, void *arg)
{
	struct mmdb_t *mmdb = (struct mmdb_t *) arg;
	struct string_dict_entry *ent = data;
	VATTR *vp = ent->data;

	mmdb_write_opaque(mmdb, vp->name, strlen(vp->name) + 1);
	mmdb_write_uint(mmdb, vp->number);
	mmdb_write_uint(mmdb, vp->flags);
	return 1;
}

void mmdb_db_write(char *filename)
{
	struct mmdb_t *mmdb;
	unsigned int xid[5], i;
	struct timeval tv;
	rbtree vattr_htab = mudstate.vattr_name_htab.tree;

	for(i = 0; i < 5; i++) {
		xid[i] = rand();
	}

	gettimeofday(&tv, NULL);

	mmdb = mmdb_open_write(filename);
	mmdb_write_uint(mmdb, DB_MAGIC);
	mmdb_write_uint(mmdb, DB_VERSION);
	mmdb_write_uint(mmdb, tv.tv_sec);
	mmdb_write_uint(mmdb, tv.tv_usec);
	mmdb_write_uint(mmdb, mudstate.db_revision++);
	for(i = 0; i < 5; i++) {
		mmdb_write_uint(mmdb, xid[i]);
	}
	mmdb_write_uint(mmdb, rb_size(vattr_htab));
	rb_walk(vattr_htab, WALK_INORDER, mmdb_write_vattr, mmdb);
	mmdb_write_uint(mmdb, mudstate.db_top);
	DO_WHOLE_DB(i) {
		mmdb_write_object(mmdb, i);
	}
	mmdb_close(mmdb);
}

int mmdb_db_read(char *filename)
{
	struct mmdb_t *mmdb;
	unsigned int xid[5], i;
	unsigned int magic, version, revision;
	unsigned int object;
	unsigned int vattr_count, object_count;
	unsigned int vattr_len, vattr_number, vattr_flags;
	struct timeval tv;
	rbtree vattr_htab = mudstate.vattr_name_htab.tree;
	char buffer[4096];

	mmdb = mmdb_open_read(filename);
	magic = mmdb_read_uint(mmdb);
	dassert(magic == DB_MAGIC);
	version = mmdb_read_uint(mmdb);
	dassert(version == DB_VERSION);

	tv.tv_sec = mmdb_read_uint(mmdb);
	tv.tv_usec = mmdb_read_uint(mmdb);

	mudstate.db_revision = revision = mmdb_read_uint(mmdb);

	dprintk("Loading database revision %d, created at %s.", revision,
			asctime(localtime(&tv.tv_sec)));
	for(i = 0; i < 5; i++) {
		xid[i] = mmdb_read_uint(mmdb);
	}

	dprintk("database XID: %08x%08x%08x%08x%08x", xid[0],
			xid[1], xid[2], xid[3], xid[4]);
	db_free();
	vattr_count = mmdb_read_uint(mmdb);
	anum_extend(vattr_count);
	dprintk("reading in %d vattrs", vattr_count);
	for(int i = 0; i < vattr_count; i++) {
		vattr_len = mmdb_read_uint(mmdb);
		mmdb_read(mmdb, buffer, vattr_len);
		vattr_number = mmdb_read_uint(mmdb);
		vattr_flags = mmdb_read_uint(mmdb);
		vattr_define(buffer, vattr_number, vattr_flags);
	}
	dprintk("... done.");

	object_count = mmdb_read_uint(mmdb);
	db_grow(object_count);
	dprintk("reading in %d objects", object_count);
	for(int i = 0; i < object_count; i++) {
		object = mmdb_read_uint(mmdb);
		vattr_len = mmdb_read_uint(mmdb);
		mmdb_read(mmdb, buffer, vattr_len);
		s_Name(object, buffer);
		s_Location(object, mmdb_read_uint(mmdb));
		s_Zone(object, mmdb_read_uint(mmdb));
		s_Contents(object, mmdb_read_uint(mmdb));
		s_Exits(object, mmdb_read_uint(mmdb));
		s_Link(object, mmdb_read_uint(mmdb));
		s_Next(object, mmdb_read_uint(mmdb));
		s_Owner(object, mmdb_read_uint(mmdb));
		s_Parent(object, mmdb_read_uint(mmdb));
		s_Pennies(object, mmdb_read_uint(mmdb));
		s_Flags(object, mmdb_read_uint(mmdb));
		s_Flags2(object, mmdb_read_uint(mmdb));
		s_Flags3(object, mmdb_read_uint(mmdb));
		s_Powers(object, mmdb_read_uint(mmdb));
		s_Powers2(object, mmdb_read_uint(mmdb));
		vattr_count = mmdb_read_uint(mmdb);
		for(int j = 0; j < vattr_count; j++) {
			vattr_len = mmdb_read_uint(mmdb);
			mmdb_read(mmdb, buffer, vattr_len);
			vattr_number = mmdb_read_uint(mmdb);
			atr_add_raw(object, vattr_number, buffer);
		}
	}
	load_player_names();
	return object_count;
}
