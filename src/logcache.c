/*
 * logcache.c
 */

/*
 * $Id $
 */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include "mudconf.h"
#include "externs.h"
#include "interface.h"
#include "flags.h"
#include "command.h"
#include "attrs.h"
#include "rbtree.h"
#include <errno.h>

#ifdef DEBUG_LOGCACHE
#define DEBUG
#endif
#include "debug.h"

/* The LOGFILE_TIMEOUT field describes how long a mux should keep an idle
 * open. LOGFILE_TIMEOUT seconds after the last write, it will close. The
 * timer is reset on each write. */
#define LOGFILE_TIMEOUT 300		// Five Minutes

struct logfile_t {
	char *filename;
	int fd;
	struct event ev;
};

rbtree logfiles = NULL;

static int logcache_compare(void *vleft, void *vright, void *arg)
{
	return strcmp((char *) vleft, (char *) vright);
}

static int logcache_close(struct logfile_t *log)
{
	dprintk("closing logfile '%s'.", log->filename);
	if(evtimer_pending(&log->ev, NULL)) {
		evtimer_del(&log->ev);
	}
	close(log->fd);
	rb_delete(logfiles, log->filename);
	if(log->filename)
		free(log->filename);
	log->filename = NULL;
	log->fd = -1;
	free(log);
	return 1;
}

static void logcache_expire(int fd, short event, void *arg)
{
	dprintk("Expiring '%s'.", ((struct logfile_t *) arg)->filename);
	logcache_close((struct logfile_t *) arg);
}

static int _logcache_list(void *key, void *data, int depth, void *arg)
{
	struct timeval tv;
	struct logfile_t *log = (struct logfile_t *) data;
	dbref player = *(dbref *) arg;
	evtimer_pending(&log->ev, &tv);
	notify_printf(player, "%-40s%d", log->filename, tv.tv_sec - mudstate.now);
	return 1;
}

void logcache_list(dbref player)
{
	notify(player, "/--------------------------- Open Logfiles");
	if(rb_size(logfiles) == 0) {
		notify(player, "- There are no open logfile handles.");
		return;
	}
	notify(player, "Filename                               Timeout");
	rb_walk(logfiles, WALK_INORDER, _logcache_list, &player);
}

static int logcache_open(char *filename)
{
	int fd;
	struct logfile_t *newlog;
	struct timeval tv = { LOGFILE_TIMEOUT, 0 };

	if(rb_exists(logfiles, filename)) {
		fprintf(stderr,
				"Serious braindamage, logcache_open() called for already open logfile.\n");
		return 0;
	}

	fd = open(filename, O_RDWR | O_APPEND | O_CREAT, 0644);
	if(fd < 0) {
		fprintf(stderr,
				"Failed to open logfile %s because open() failed with code: %d -  %s\n",
				filename, errno, strerror(errno));
		return 0;
	}
	if(fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
		log_perror("LOGCACHE", "FAIL", NULL,
				   "fcntl(fd, F_SETFD, FD_CLOEXEC)");
	}

	newlog = malloc(sizeof(struct logfile_t));
	newlog->fd = fd;
	newlog->filename = strdup(filename);
	evtimer_set(&newlog->ev, logcache_expire, newlog);
	evtimer_add(&newlog->ev, &tv);
	rb_insert(logfiles, newlog->filename, newlog);
	dprintk("opened logfile '%s' fd = %d.", filename, fd);
	return 1;
}

void logcache_init()
{
	if(!logfiles) {
		dprintk("logcache initialized.");
		logfiles = rb_init(logcache_compare, NULL);
	} else {
		dprintk("REDUNDANT CALL TO logcache_init()!");
	}
}

static int _logcache_destruct(void *key, void *data, int depth, void *arg)
{
	struct logfile_t *log = (struct logfile_t *) data;
	logcache_close(log);
	return 1;
}

void logcache_destruct()
{
	dprintk("logcache destructing.");
	if(!logfiles) {
		dprintk("logcache_destruct() CALLED WHILE UNITIALIZED!");
		return;
	}
	rb_walk(logfiles, WALK_INORDER, _logcache_destruct, NULL);
	rb_destroy(logfiles);
	logfiles = NULL;
}

int logcache_writelog(char *fname, char *fdata)
{
	struct logfile_t *log;
	struct timeval tv = { LOGFILE_TIMEOUT, 0 };
	int len;

	if(!logfiles)
		logcache_init();

	len = strlen(fdata);

	log = rb_find(logfiles, fname);

	if(!log) {
		if(logcache_open(fname) < 0) {
			return 0;
		}
		log = rb_find(logfiles, fname);
		if(!log) {
			return 0;
		}
	}

	if(evtimer_pending(&log->ev, NULL)) {
		event_del(&log->ev);
		event_add(&log->ev, &tv);
	}

	if(write(log->fd, fdata, len) < 0) {
		fprintf(stderr,
				"System failed to write data to file with error '%s' on logfile '%s'. Closing.\n",
				strerror(errno), log->filename);
		logcache_close(log);
	}
	return 1;
}
