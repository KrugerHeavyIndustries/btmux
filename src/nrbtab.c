/*
 * htab.c - table hashing routines 
 */

#include "copyright.h"
#include "config.h"
#include "db.h"
#include "externs.h"
#include "htab.h"
#include "alloc.h"
#include "mudconf.h"

struct int_dict_entry {
    int key; 
    void *data;
};


static int nhrbtab_compare(int left, int right, void *arg) {
        return (right - left);
}


void nhashinit(RBTAB *htab, int size) {
    memset(htab, 0, sizeof(RBTAB));
    htab->tree = rb_init((void *)nhrbtab_compare, NULL);
    htab->last = NULL;
}

void nhashreset(RBTAB *htab) {
    htab->checks = 0;
    htab->scans = 0;
    htab->hits = 0;
};


/*
 * ---------------------------------------------------------------------------
 * * hashfind: Look up an entry in a hash table and return a pointer to its
 * * hash data.
 */

int *nhashfind(int val, RBTAB *htab) {
    struct int_dict_entry *ent;
    int hval, numchecks;
   
    htab->checks++;
    ent = rb_find(htab->tree, (void *)val);
    if(ent) 
        return ent->data;
    else return NULL;
}

/*
 * ---------------------------------------------------------------------------
 * * hashadd: Add a new entry to a hash table.
 */

int nhashadd(int val, int *hashdata, RBTAB *htab) {
    struct int_dict_entry *ent = malloc(sizeof(struct int_dict_entry));
    
    if(rb_exists(htab->tree, (void *)val))
        return (-1);

    ent->key = val;
    ent->data = hashdata;
    
    rb_insert(htab->tree, (void *)ent->key, ent);
    return 0;
    
}

/*
 * ---------------------------------------------------------------------------
 * * hashdelete: Remove an entry from a hash table.
 */

void nhashdelete(int val, RBTAB *htab) {
    struct int_dict_entry *ent = NULL;

    if(!rb_exists(htab->tree, (void *)val)) {
        return;
    }
    ent=rb_delete(htab->tree, val);

    if(ent) {
        if(ent->key)
            free(ent->key);
        free(ent);
    }

    return;
}

/*
 * ---------------------------------------------------------------------------
 * * hashflush: free all the entries in a hashtable.
 */

static int nnuke_hash_ent(void *key, void *data, int depth, void *arg) {
    struct int_dict_entry *ent = (struct int_dict_entry *)data;
    free(ent);
    return 1;
}

void nhashflush(RBTAB *htab, int size) {
    rb_walk(htab->tree, WALK_POSTORDER, nnuke_hash_ent, NULL);
    rb_destroy(htab->tree);
    htab->tree = rb_init(nhrbtab_compare, NULL);
    if(htab->last) free(htab->last);
    htab->last = NULL;
}

/*
 * ---------------------------------------------------------------------------
 * * hashrepl: replace the data part of a hash entry.
 */

int nhashrepl(int val, int *hashdata, RBTAB *htab) {
    struct int_dict_entry *ent;

    ent = rb_find(htab->tree, val);
    if(!ent) return 0;

    ent->data = hashdata;
    return 1;
}

struct hashreplstat {
    void *old;
    void *new;
};

static int nhashreplall_cb(void *key, void *data, int depth, void *arg) {
    struct int_dict_entry *ent = (struct int_dict_entry *)data;
    struct hashreplstat *repl = (struct hashreplstat *)arg;

    if(ent->data == repl->old) {
        ent->data = repl->new;
    }
    return 1;
}
    

void nhashreplall(int *old, int *new, RBTAB *htab) {
    struct hashreplstat repl = { old, new };

    rb_walk(htab->tree, WALK_INORDER, nhashreplall_cb, &repl);
}


/*
 * ---------------------------------------------------------------------------
 * * hashinfo: return an mbuf with hashing stats
 */

char *nhashinfo(const char *tab_name, RBTAB *htab) {
    char *buff;

    buff = alloc_mbuf("hashinfo");
    sprintf(buff, "%-15s %8d", tab_name, rb_size(htab->tree));
    return buff;
}

/*
 * Returns the key for the first hash entry in 'htab'. 
 */

int *nhash_firstentry(RBTAB *htab) {
    struct int_dict_entry *ent;

    if(htab->last) free(htab->last);

    ent = rb_search(htab->tree, SEARCH_FIRST, NULL);
    if(ent) {
        htab->last = ent->key;
        return ent->data;
    }
    htab->last = NULL;
    
    return NULL;
}

int *nhash_nextentry(RBTAB *htab) {
    struct int_dict_entry *ent;

    if(!htab->last) {
        return hash_firstentry(htab);
    }
    
    ent = rb_search(htab->tree, SEARCH_NEXT, htab->last);
    free(htab->last);

    if(ent) {
        htab->last = ent->key;
        return ent->data;
    } else {
        htab->last = NULL;
        return NULL;
    }
}

int nhash_firstkey(RBTAB *htab) {
    struct int_dict_entry *ent;
    if(htab->last) free(htab->last);
    

    ent = rb_search(htab->tree, SEARCH_FIRST, NULL);
    if(ent) {
        htab->last = ent->key;
        return ent->key;
    }
    htab->last = NULL;
    
    return NULL;
}

int nhash_nextkey(RBTAB *htab) {
    struct int_dict_entry *ent;
    
    if(!htab->last) {
        return hash_firstkey(htab);
    }
    
    ent = rb_search(htab->tree, SEARCH_NEXT, htab->last);
    free(htab->last);

    if(ent) {
        htab->last = ent->key;
        return ent->key;
    } else {
        htab->last = NULL;
        return NULL;
    }
}
