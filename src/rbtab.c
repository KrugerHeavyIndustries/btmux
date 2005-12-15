/*
 * htab.c - table hashing routines 
 */

/*
 * $Id: htab.c,v 1.3 2005/08/08 09:43:07 murrayma Exp $ 
 */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "externs.h"
#include "htab.h"
#include "alloc.h"

#include "mudconf.h"

struct string_dict_entry {
    char *key;
    void *data;
};

static int hrbtab_compare(char *left, char *right, void *arg) {
    return strcmp(left, right);
}

void hashinit(RBTAB *htab, int size) {
    memset(htab, 0, sizeof(RBTAB));
    htab->tree = rb_init(hrbtab_compare, NULL);
    htab->last = NULL;
}

/*
 * ---------------------------------------------------------------------------
 * * hashreset: Reset hash table stats.
 */

void hashreset(RBTAB *htab) {
    htab->checks = 0;
    htab->scans = 0;
    htab->hits = 0;
}

/*
 * ---------------------------------------------------------------------------
 * * hashfind: Look up an entry in a hash table and return a pointer to its
 * * hash data.
 */

int *hashfind(char *str, RBTAB *htab) {
    int hval, numchecks;
    struct string_dict_entry *ent;
   
    htab->checks++;
    ent = rb_find(htab->tree, str);
    if(ent) {
        return ent->data;
    } else return ent;
}

/*
 * ---------------------------------------------------------------------------
 * * hashadd: Add a new entry to a hash table.
 */

int hashadd(char *str, int *hashdata, RBTAB *htab) {
    struct string_dict_entry *ent = malloc(sizeof(struct string_dict_entry));
    
    if(rb_exists(htab->tree, str))
        return (-1);

    ent->key = strdup(str);
    ent->data = hashdata;
    
    rb_insert(htab->tree, ent->key, ent);
    return 0;
    
}

/*
 * ---------------------------------------------------------------------------
 * * hashdelete: Remove an entry from a hash table.
 */

void hashdelete(char *str, RBTAB *htab) {
    struct string_dict_entry *ent = NULL;

    if(!rb_exists(htab->tree, str)) {
        return;
    }
    ent=rb_delete(htab->tree, str);

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

static int nuke_hash_ent(void *key, void *data, int depth, void *arg) {
    struct string_dict_entry *ent = (struct string_dict_entry *)data;
    free(ent->key);
    free(ent);
    return 1;
}

void hashflush(RBTAB *htab, int size) {
    rb_walk(htab->tree, WALK_POSTORDER, nuke_hash_ent, NULL);
    rb_destroy(htab->tree);
    htab->tree = rb_init(hrbtab_compare, NULL);
    if(htab->last) free(htab->last);
    htab->last = NULL;
}

/*
 * ---------------------------------------------------------------------------
 * * hashrepl: replace the data part of a hash entry.
 */

int hashrepl(char *str, int *hashdata, RBTAB *htab) {
    struct string_dict_entry *ent;

    ent = rb_find(htab->tree, str);
    if(!ent) return 0;

    ent->data = hashdata;
    return 1;
}

struct hashreplstat {
    void *old;
    void *new;
};

static int hashreplall_cb(void *key, void *data, int depth, void *arg) {
    struct string_dict_entry *ent = (struct string_dict_entry *)data;
    struct hashreplstat *repl = (struct hashreplstat *)arg;

    if(ent->data == repl->old) {
        ent->data = repl->new;
    }
    return 1;
}
    

void hashreplall(int *old, int *new, RBTAB *htab) {
    struct hashreplstat repl = { old, new };

    rb_walk(htab->tree, WALK_INORDER, hashreplall_cb, &repl);
}


/*
 * ---------------------------------------------------------------------------
 * * hashinfo: return an mbuf with hashing stats
 */

char *hashinfo(const char *tab_name, RBTAB *htab) {
    char *buff;

    buff = alloc_mbuf("hashinfo");
    sprintf(buff, "%-15s %8d", tab_name, rb_size(htab->tree));
    return buff;
}

/*
 * Returns the key for the first hash entry in 'htab'. 
 */

int *hash_firstentry(RBTAB *htab) {
    struct string_dict_entry *ent;

    if(htab->last) free(htab->last);

    ent = rb_search(htab->tree, SEARCH_FIRST, NULL);
    if(ent) {
        htab->last = strdup(ent->key);
        return ent->data;
    }
    htab->last = NULL;
    
    return NULL;
}

int *hash_nextentry(RBTAB *htab) {
    struct string_dict_entry *ent;

    if(!htab->last) {
        return hash_firstentry(htab);
    }
    
    ent = rb_search(htab->tree, SEARCH_NEXT, htab->last);
    free(htab->last);

    if(ent) {
        htab->last = strdup(ent->key);
        return ent->data;
    } else {
        htab->last = NULL;
        return NULL;
    }
}

char *hash_firstkey(RBTAB *htab) {
    struct string_dict_entry *ent;
    if(htab->last) free(htab->last);
    

    ent = rb_search(htab->tree, SEARCH_FIRST, NULL);
    if(ent) {
        htab->last = strdup(ent->key);
        return ent->key;
    }
    htab->last = NULL;
    
    return NULL;
}

char *hash_nextkey(RBTAB *htab) {
    struct string_dict_entry *ent;
    
    if(!htab->last) {
        return hash_firstkey(htab);
    }
    
    ent = rb_search(htab->tree, SEARCH_NEXT, htab->last);
    free(htab->last);

    if(ent) {
        htab->last = strdup(ent->key);
        return ent->key;
    } else {
        htab->last = NULL;
        return NULL;
    }
}



