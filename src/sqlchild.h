#ifndef __SQLCHILD_H__
#define __SQLCHILD_H__

void sqlchild_init();
int sqlchild_request(dbref thing, int attr, char db_slot, char *pres, char *query, char *cdelim, char *rdelim);
void sqlchild_list(dbref player);
void sqlchild_destruct();
void sqlchild_kill_query(int requestId);

#endif
