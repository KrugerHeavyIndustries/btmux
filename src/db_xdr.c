/*
 * db_xdr.c
 */
#include "copyright.h"
#include "config.h"

#include "mudconf.h"
#include "config.h"
#include "externs.h"
#include "db.h"
#include "vattr.h"
#include "attrs.h"
#include "alloc.h"
#include "powers.h"
#include "mmdb.h"
#include "debug.h"


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

struct string_dict_entry {
	char *key;
	char *data;
};

static int mmdb_write_vattr(void *key, void *data, int depth, void *arg)
{
	struct mmdb_t *mmdb = (struct mmdb_t *) arg;
	struct string_dict_entry *ent = data;
	VATTR *vp = (VATTR *) ent->data;

	mmdb_write_opaque(mmdb, vp->name, strlen(vp->name) + 1);
	mmdb_write_uint(mmdb, vp->number);
	mmdb_write_uint(mmdb, vp->flags);
	return 1;
}

void mmdb_db_write(char *filename)
{
	struct mmdb_t *mmdb;
	uint32_t xid[5], i;
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
	uint32_t xid[5], i;
	uint32_t magic, version, revision;
	uint32_t object;
	uint32_t vattr_count, object_count;
	uint32_t vattr_len, vattr_number, vattr_flags;
	struct timeval tv;
	rbtree vattr_htab = mudstate.vattr_name_htab.tree;
	char buffer[4096];

	mmdb = mmdb_open_read(filename);
	magic = mmdb_read_uint32(mmdb);
	dassert(magic == DB_MAGIC);
	version = mmdb_read_uint32(mmdb);
	dassert(version == DB_VERSION);

	tv.tv_sec = mmdb_read_uint32(mmdb);
	tv.tv_usec = mmdb_read_uint32(mmdb);

	mudstate.db_revision = revision = mmdb_read_uint32(mmdb);

	dprintk("Loading database revision %d, created at %s.", revision,
			asctime(localtime(&tv.tv_sec)));
	for(i = 0; i < 5; i++) {
		xid[i] = mmdb_read_uint32(mmdb);
	}

	dprintk("database XID: %08x%08x%08x%08x%08x", xid[0],
			xid[1], xid[2], xid[3], xid[4]);
	db_free();
	vattr_count = mmdb_read_uint32(mmdb);
	anum_extend(vattr_count);
	dprintk("reading in %d vattrs", vattr_count);
	for(int i = 0; i < vattr_count; i++) {
		vattr_len = mmdb_read_uint32(mmdb);
		mmdb_read(mmdb, buffer, vattr_len);
		vattr_number = mmdb_read_uint32(mmdb);
		vattr_flags = mmdb_read_uint32(mmdb);
		vattr_define(buffer, vattr_number, vattr_flags);
	}
	dprintk("... done.");

	object_count = mmdb_read_uint32(mmdb);
	db_grow(object_count);
	dprintk("reading in %d objects", object_count);
	for(int i = 0; i < object_count; i++) {
		object = mmdb_read_uint32(mmdb);
		vattr_len = mmdb_read_uint32(mmdb);
		mmdb_read(mmdb, buffer, vattr_len);
		s_Name(object, buffer);
		s_Location(object, mmdb_read_uint32(mmdb));
		s_Zone(object, mmdb_read_uint32(mmdb));
		s_Contents(object, mmdb_read_uint32(mmdb));
		s_Exits(object, mmdb_read_uint32(mmdb));
		s_Link(object, mmdb_read_uint32(mmdb));
		s_Next(object, mmdb_read_uint32(mmdb));
		s_Owner(object, mmdb_read_uint32(mmdb));
		s_Parent(object, mmdb_read_uint32(mmdb));
		s_Pennies(object, mmdb_read_uint32(mmdb));
		s_Flags(object, mmdb_read_uint32(mmdb));
		s_Flags2(object, mmdb_read_uint32(mmdb));
		s_Flags3(object, mmdb_read_uint32(mmdb));
		s_Powers(object, mmdb_read_uint32(mmdb));
		s_Powers2(object, mmdb_read_uint32(mmdb));
		vattr_count = mmdb_read_uint32(mmdb);
		for(int j = 0; j < vattr_count; j++) {
			vattr_len = mmdb_read_uint32(mmdb);
			mmdb_read(mmdb, buffer, vattr_len);
			vattr_number = mmdb_read_uint32(mmdb);
			atr_add_raw(object, vattr_number, buffer);
		}
	}
	load_player_names();
	return object_count;
}
