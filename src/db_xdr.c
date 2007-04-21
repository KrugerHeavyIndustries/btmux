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

/* adds for commac */
#include "macro.h"
#include "commac.h"
#include "comsys.h"
#include "myfifo.h"
#include "create.h"

extern OBJ *db;

static void do_save_com_xdr(chmsg * d,struct mmdb_t *mmdb)
{
        mmdb_write_uint32(mmdb, (int) d->time);

        mmdb_write_opaque(mmdb, d->msg, strlen(d->msg)+1);

}

void myfifo_trav_r_xdr(myfifo ** foo, struct mmdb_t *mmdb, void (*func) ())
{
        myfifo_e *tmp;


        for (tmp = (*foo)->last; tmp !=NULL; tmp = tmp->prev)
                func(tmp->data, mmdb);
}

void mmdb_write_object(struct mmdb_t *mmdb, dbref object)
{
	ATRLIST *atrlist;
	int ii;

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
	for (ii = 0; ii < db[object].at_count; ii++) {
		mmdb_write_opaque(mmdb, atrlist[ii].data, atrlist[ii].size);
		mmdb_write_uint(mmdb, atrlist[ii].number);
	}
}

#define DB_MAGIC 0x4841475A
#define DB_VERSION 3
#define COMMAC_MAGIC   0x434f4d5a  /* COMZ */
#define COMMAC_VERSION 1

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
/* commac specific */
	int j, k, np, player_users;
	struct commac *c;
	struct channel *ch;
	struct comuser *user;
	struct macros *m;

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

	/* START COMMAC */
	mmdb_write_uint32(mmdb, COMMAC_MAGIC);
	mmdb_write_uint32(mmdb, COMMAC_VERSION);
	
	purge_commac();
	np = 0;
	
	for(i = 0; i < NUM_COMMAC; i++) {
		c = commac_table[i];
		while (c) {
			np++;
			c = c->next;
		}
	}

	mmdb_write_uint32(mmdb, np);

	for(i = 0; i < NUM_COMMAC; i++) {
		c = commac_table[i];
		while (c) {
			mmdb_write_uint32(mmdb, c->who);
			mmdb_write_uint32(mmdb, c->numchannels);
			mmdb_write_uint32(mmdb, c->macros[0]);
			mmdb_write_uint32(mmdb, c->macros[1]);
			mmdb_write_uint32(mmdb, c->macros[2]);
			mmdb_write_uint32(mmdb, c->macros[3]);
			mmdb_write_uint32(mmdb, c->macros[4]);
			mmdb_write_uint32(mmdb, c->curmac);
			if(c->numchannels > 0) {
				for(j = 0; j <c->numchannels; j++) {
					mmdb_write_opaque(mmdb, c->alias +j *6, strlen(c->alias +j * 6)+1);
					mmdb_write_opaque(mmdb, c->channels[j], strlen(c->channels[j])+1);
				}
			}
			c = c->next;
		}
	}
	/* END COMMAC */
	/* START COMSYS */
	mmdb_write_uint32(mmdb, num_channels);
	for(ch = (struct channel *) hash_firstentry(&mudstate.channel_htab); ch; ch = (struct channel *) hash_nextentry(&mudstate.channel_htab)) {
		mmdb_write_opaque(mmdb, ch->name, strlen(ch->name)+1);
		mmdb_write_uint32(mmdb, ch->type);
		mmdb_write_uint32(mmdb, ch->charge);
		mmdb_write_uint32(mmdb, ch->charge_who);
		mmdb_write_uint32(mmdb, ch->amount_col);
		mmdb_write_uint32(mmdb, ch->num_messages);
		mmdb_write_uint32(mmdb, ch->chan_obj);
		k = myfifo_length(&ch->last_messages);
		mmdb_write_uint32(mmdb, k);

		if (k)
			myfifo_trav_r_xdr(&ch->last_messages,mmdb,do_save_com_xdr);

		player_users = 0;
		for(j = 0; j < ch->num_users; j++) 
			if(isPlayer(ch->users[j]->who) || isRobot(ch->users[j]->who))
				player_users++;

		mmdb_write_uint32(mmdb,player_users);
		for(j = 0; j < ch->num_users; j++) {
			user = ch->users[j];
			if(!isPlayer(user->who) && !isRobot(user->who))
				continue;
			mmdb_write_uint32(mmdb, user->who);
			mmdb_write_uint32(mmdb, user->on);
			if(strlen(user->title)) {
				mmdb_write_uint32(mmdb,1);
				mmdb_write_opaque(mmdb,user->title,strlen(user->title)+1);
			} else
				mmdb_write_uint32(mmdb,0);
		}
	}

	/* END COMSYS */
	/* START MACRO */
	mmdb_write_uint32(mmdb, nummacros);

        for(i = 0; i <nummacros; i++) {
                m = macros[i];
                mmdb_write_uint32(mmdb, m->player);
                mmdb_write_uint32(mmdb, m->nummacros);
                mmdb_write_uint32(mmdb, (int) m->status);
                mmdb_write_opaque(mmdb, m->desc, strlen(m->desc)+1);
                for(j = 0; j < m->nummacros; j++) {
                        mmdb_write_opaque(mmdb, m->alias + j * 5, strlen(m->alias +j * 5)+1);
                        mmdb_write_opaque(mmdb, m->string[j], strlen(m->string[j])+1);
                }
        }
	/* END MACRO */
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
	int np, ii, j, k, len;
	struct commac *c;
	struct channel *ch;
	struct comuser *user;
	struct macros *m;

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
	for (ii = 0; ii < vattr_count; ii++) {
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
	for (ii = 0; ii < object_count; ii++) {
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
		for (j = 0; j < vattr_count; j++) {
			vattr_len = mmdb_read_uint32(mmdb);
			mmdb_read(mmdb, buffer, vattr_len);
			vattr_number = mmdb_read_uint32(mmdb);
			atr_add_raw(object, vattr_number, buffer);
		}
	}
	load_player_names();
	
	magic = mmdb_read_uint32(mmdb);
	dassert(magic == COMMAC_MAGIC);
	version = mmdb_read_uint32(mmdb);
	dassert(version == COMMAC_VERSION);
        /* START COMMAC SECTION */
	np = mmdb_read_uint32(mmdb);
	for(i = 0; i < np; i++) {
		c = create_new_commac();
		c->who = mmdb_read_uint32(mmdb);
		c->numchannels = mmdb_read_uint32(mmdb);
		c->macros[0] = mmdb_read_uint32(mmdb);
		c->macros[1] = mmdb_read_uint32(mmdb);
		c->macros[2] = mmdb_read_uint32(mmdb);
		c->macros[3] = mmdb_read_uint32(mmdb);
		c->macros[4] = mmdb_read_uint32(mmdb);
		c->curmac = mmdb_read_uint32(mmdb);
		c->maxchannels = c->numchannels;
		if(c->maxchannels > 0) {
			c->alias = (char *) malloc(c->maxchannels * 6);
			c->channels = (char **) malloc(sizeof(char *) * c->maxchannels);
			
			for(j = 0; j < c->numchannels; j++) {
				len = mmdb_read_uint32(mmdb);
				mmdb_read(mmdb, buffer, len);

				StringCopy(c->alias + j * 6,buffer);

				len = mmdb_read_uint32(mmdb);
				mmdb_read(mmdb, buffer, len);
	
				c->channels[j] = (char *) malloc(strlen(buffer) + 1);
				StringCopy(c->channels[j],buffer);

			}
			sort_com_aliases(c);
		} else {
			c->alias = NULL;
			c->channels = NULL;
		}
		if((Typeof(c->who) == TYPE_PLAYER) || (!God(Owner(c->who))) || ((!Going(c->who))))
			add_commac(c);
		purge_commac();
	}
	/* END COMMAC SECTION */

	/* START COMSYS SECTION */
	num_channels = mmdb_read_uint32(mmdb);

	for( i = 0; i < num_channels; i++) {
		ch = (struct channel *) malloc(sizeof(struct channel));
		
	
		len = mmdb_read_uint32(mmdb);
		mmdb_read(mmdb,buffer,len);

		strncpy(ch->name, buffer, len);
		ch->on_users = NULL;
		
		hashadd(ch->name, (int *) ch, &mudstate.channel_htab);
		ch->type = mmdb_read_uint32(mmdb);
		ch->charge = mmdb_read_uint32(mmdb);
		ch->charge_who = mmdb_read_uint32(mmdb);
		ch->amount_col = mmdb_read_uint32(mmdb);
		ch->num_messages = mmdb_read_uint32(mmdb);
		ch->chan_obj = mmdb_read_uint32(mmdb);
		k = mmdb_read_uint32(mmdb);
		ch->last_messages = NULL;
		
		if (k > 0) {
			for(j = 0; j < k; j++) {
				chmsg *c;
				Create(c,chmsg,1);
				c->time = mmdb_read_uint32(mmdb);
				len = mmdb_read_uint32(mmdb);
				mmdb_read(mmdb, buffer, len);
				c->msg = strdup(buffer);
				myfifo_push(&ch->last_messages, c);
			}
			
		}
		ch->num_users = mmdb_read_uint32(mmdb);
		ch->max_users = ch->num_users;

		if(ch->num_users > 0) {
			ch->users = (struct comuser **) calloc(ch->max_users, sizeof(struct comuser *));
			
			for(j =0; j < ch->num_users; j++) {
				user = (struct comuser *) malloc(sizeof(struct comuser));

				ch->users[j] = user;

				user->who = mmdb_read_uint32(mmdb);
				user->on = mmdb_read_uint32(mmdb);

				/* title stuff here */
				k = mmdb_read_uint32(mmdb);
				if (k) {
					len = mmdb_read_uint32(mmdb);
					mmdb_read(mmdb, buffer, len);
					user->title = strdup(buffer);
				}
				else
					user->title = "";
				if(!(isPlayer(user->who)) && !(Going(user->who) && (God(Owner(user->who))))) {
					do_joinchannel(user->who, ch);
					user->on_next = ch->on_users;
					ch->on_users = user;
				} else {
					user->on_next = ch->on_users;
					ch->on_users = user;
				}
			}
			sort_users(ch);
		} else
			ch->users = NULL;
	}		


	/* END COMSYS SECTION */

	/* BEGIN MACRO SECTION */
	nummacros = mmdb_read_uint32(mmdb);
	maxmacros = nummacros;
	
	if(maxmacros > 0)
		macros = (struct macros **) malloc(sizeof(struct macros *) * nummacros);
	else
		macros = NULL;

	for(i = 0; i < nummacros; i++) {
		macros[i] = (struct macros *) malloc(sizeof(struct macros));

		m = macros[i];
		m->player = mmdb_read_uint32(mmdb);
		m->nummacros = mmdb_read_uint32(mmdb);
		m->status = mmdb_read_uint32(mmdb);
		len = mmdb_read_uint32(mmdb);
		mmdb_read(mmdb, buffer, len);
		m->desc = strdup(buffer);

		m->maxmacros = m->nummacros;

		if(m->nummacros > 0) {
			m->alias = (char *) malloc(5 * m->maxmacros);
			m->string = (char **) malloc(sizeof(char *) * m->nummacros);

			for(j = 0; j < m->nummacros; j++) {
				len = mmdb_read_uint32(mmdb);
				mmdb_read(mmdb, buffer, len);
				strcpy(m->alias + j * 5, buffer);
				len = mmdb_read_uint32(mmdb);
				mmdb_read(mmdb, buffer, len);
				m->string[j] = (char *) malloc(len + 1);
				strcpy(m->string[j], buffer);

			}
			do_sort_macro_set(m);
		} else {
			m->alias = NULL;
			m->string = NULL;
		}
	}
	while (1) {
		for(i = 0; i < nummacros; i++)
			if(!isPlayer(macros[i]->player))
				break;
		if( i >= nummacros)
			break;
		clear_macro_set(i);
	}
	return object_count;
}
