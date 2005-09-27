#ifndef __LOGCACHE_H__
#define __LOGCACHE_H__

void logcache_list(dbref player);
void logcache_init();
void logcache_destruct();
int logcache_writelog(char *fname, char *fdata);

#endif
