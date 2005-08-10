
/*
 *  Definitions for user-defined attributes
 *
 *  $Id: vattr.h,v 1.3 2005/06/23 02:59:58 murrayma Exp $
 */

#define VNAME_SIZE	32

typedef struct user_attribute VATTR;
struct user_attribute {
    char *name;			/* Name of user attribute */
    int number;			/* Assigned attribute number */
    int flags;			/* Attribute flags */
};

extern void vattr_init(void);
extern VATTR *vattr_rename(char *, char *);
extern VATTR *vattr_find(char *);
extern VATTR *vattr_nfind(int);
extern VATTR *vattr_alloc(char *, int);
extern VATTR *vattr_define(char *, int, int);
extern void vattr_delete(char *);
extern VATTR *attr_rename(char *, char *);
extern VATTR *vattr_first(void);
extern VATTR *vattr_next(VATTR *);
extern void list_vhashstats(dbref);
