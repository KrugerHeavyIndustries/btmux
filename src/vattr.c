
/*
 * vattr.c -- Manages the user-defined attributes. 
 */

/*
 * $Id: vattr.c,v 1.4 2005/08/08 09:43:07 murrayma Exp $ 
 */

#include "copyright.h"
#include "config.h"

#include "copyright.h"
#include "mudconf.h"
#include "vattr.h"
#include "alloc.h"
#include "htab.h"
#include "externs.h"
#include "rbtree.h"

static void fixcase(char *);
static char *store_string(char *);

/*
 * Allocate space for strings in lumps this big. 
 */

#define STRINGBLOCK 1000

/*
 * Current block we're putting stuff in 
 */

static char *stringblock = (char *) 0;

/*
 * High water mark. 
 */

static int stringblock_hwm = 0;

static rbtree *vt;

static int vattr_compare(void *vleft, void *vright, void *arg) {
    char *left = (char *)vleft;
    char *right = (char *)vright;
    return strcmp(left, right);
}

void vattr_init(void)
{
    vt = rb_init(vattr_compare, NULL);
#if 0
    hashinit(&mudstate.vattr_name_htab, 256 * HASH_FACTOR);
#endif
}

VATTR *vattr_find(name)
char *name;
{
    register VATTR *vp;

    if (!ok_attr_name(name))
	return (NULL);

#if 0
    vp = (VATTR *) hashfind(name, &mudstate.vattr_name_htab);
#endif 

    vp = (VATTR *) rb_find(vt, (void *)name);

    /*
     * vp is NULL or the right thing. It's right, either way. 
     */
    return (vp);
}

VATTR *vattr_alloc(name, flags)
char *name;
int flags;
{
    int number;

    if (((number = mudstate.attr_next++) & 0x7f) == 0)
	number = mudstate.attr_next++;
    anum_extend(number);
    return (vattr_define(name, number, flags));
}

VATTR *vattr_define(name, number, flags)
char *name;
int number, flags;
{
    VATTR *vp;

    /*
     * Be ruthless. 
     */

    if (strlen(name) > VNAME_SIZE)
	name[VNAME_SIZE - 1] = '\0';

    fixcase(name);
    if (!ok_attr_name(name))
	return (NULL);

    if ((vp = vattr_find(name)) != NULL)
	return (vp);

    vp = (VATTR *) malloc(sizeof(VATTR));

    vp->name = store_string(name);
    vp->flags = flags;
    vp->number = number;
#if 0
    hashadd(vp->name, (int *) vp, &mudstate.vattr_name_htab);
#endif

    rb_insert(vt, (void *)vp->name, (void *)vp);

    anum_extend(vp->number);
    anum_set(vp->number, (ATTR *) vp);
    return (vp);
}

void do_dbclean(player, cause, key)
dbref player, cause;
int key;
{
    VATTR *vp;
    dbref i;
    int notfree;

    for(vp = (VATTR *) rb_search(vt, SEARCH_FIRST, NULL);
            vp != NULL;
            vp = (VATTR *) rb_search(vt, SEARCH_GT, NULL)) {
#if 0
        for (vp = (VATTR *) hash_firstentry(&mudstate.vattr_name_htab);
            vp != NULL;
            vp = (VATTR *) hash_nextentry(&mudstate.vattr_name_htab)) {
#endif
        notfree = 0;

        DO_WHOLE_DB(i) {
            if (atr_get_raw(i, vp->number) != NULL) {
                notfree = 1;
                break;
            }
        }

        if (!notfree) {
            anum_set(vp->number, NULL);
#if 0
            hashdelete(vp->name, &mudstate.vattr_name_htab);
#endif
            rb_delete(vt, vp->name);
            free((char *) vp);
        }
    }
#ifndef STANDALONE
    notify(player, "Database cleared of stale attribute entries.");
#endif
}

void vattr_delete(name)
char *name;
{
    VATTR *vp;
    int number;

    fixcase(name);
    if (!ok_attr_name(name))
	return;

    number = 0;

#if 0
    vp = (VATTR *) hashfind(name, &mudstate.vattr_name_htab);
#endif
    vp = (VATTR *) rb_find(vt, name);

    if (vp) {
        number = vp->number;
        anum_set(number, NULL);
#if 0
        hashdelete(name, &mudstate.vattr_name_htab);
#endif
        rb_delete(vt, name);
        free((char *) vp);
    }

    return;
}

VATTR *vattr_rename(name, newname)
char *name, *newname;
{
    VATTR *vp;

    fixcase(name);
    if (!ok_attr_name(name))
	return (NULL);

    /*
     * Be ruthless. 
     */

    if (strlen(newname) > VNAME_SIZE)
	newname[VNAME_SIZE - 1] = '\0';

    fixcase(newname);
    if (!ok_attr_name(newname))
	return (NULL);

#if 0
    vp = (VATTR *) hashfind(name, &mudstate.vattr_name_htab);
#endif
    vp = (VATTR *) rb_find(vt, name);

    if (vp)
	vp->name = store_string(newname);

    return (vp);
}

VATTR *vattr_first(void)
{
#if 0
    return (VATTR *) hash_firstentry(&mudstate.vattr_name_htab);
#endif
    return (VATTR *) rb_search(vt, SEARCH_FIRST, NULL);
}

VATTR *vattr_next(vp)
VATTR *vp;
{
    if (vp == NULL)
	return (vattr_first());
#if 0
    return ((VATTR *) hash_nextentry(&mudstate.vattr_name_htab));
#endif
    return ((VATTR *) rb_search(vt, SEARCH_GT, vp));
}

static void fixcase(name)
char *name;
{
    char *cp = name;

    while (*cp) {
	*cp = ToUpper(*cp);
	cp++;
    }

    return;
}


/*
 * Some goop for efficiently storing strings we expect to
 * keep forever. There is no freeing mechanism.
 */

static char *store_string(str)
char *str;
{
    int len;
    char *ret;

    len = strlen(str);

    /*
     * If we have no block, or there's not enough room left in the * * *
     * current one, get a new one. 
     */

    if (!stringblock || (STRINGBLOCK - stringblock_hwm) < (len + 1)) {
	stringblock = (char *) malloc(STRINGBLOCK);
	if (!stringblock)
	    return ((char *) 0);
	stringblock_hwm = 0;
    }
    ret = stringblock + stringblock_hwm;
    StringCopy(ret, str);
    stringblock_hwm += (len + 1);
    return (ret);
}
