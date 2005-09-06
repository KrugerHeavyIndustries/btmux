/*
 * bsd.c 
 */

/*
 * $Id: bsd.c,v 1.7 2005/08/08 09:43:05 murrayma Exp $ 
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
#include "config.h"
#include "db.h"
#include "file_c.h"
#include "externs.h"
#include "interface.h"
#include "flags.h"
#include "powers.h"
#include "alloc.h"
#include "command.h"
#include "slave.h"
#include "attrs.h"
#include <errno.h>

#ifdef __CYGWIN__
#undef WEXITSTATUS
#define WEXITSTATUS(stat) (((*((int *) &(stat))) >> 8) & 0xff)
#endif

#ifdef SQL_SUPPORT
#include "sqlslave.h"
void accept_sqlslave_input(int fd, short event, void *arg);
#endif

void accept_slave_input(int fd, short event, void *arg);
pid_t slave_pid;
int slave_socket = -1;


#ifdef SOLARIS
extern const int _sys_nsig;
#define NSIG _sys_nsig
#endif

extern void dispatch(void);
extern void dump_restart_db(void);
extern void dump_database_internal(int);
extern char *silly_atr_get(dbref, int);
extern void send_channel(char *chan, char *str);
extern void ChangeSpecialObjects(int i);

struct event listen_sock_ev;
struct event slave_sock_ev;
struct event sqlslave_sock_ev;

int sock;
int ndescriptors = 0;
int maxd = 0;

DESC *descriptor_list = NULL;

#ifdef ARBITRARY_LOGFILES
/* The LOGFILE_TIMEOUT field describes how long a mux should keep an idle
 * open. LOGFILE_TIMEOUT seconds after the last write, it will close. The
 * timer is reset on each write. */
#define LOGFILE_TIMEOUT 300 // Five Minutes

#include "rbtree.h"
struct logfile_t {
    char *filename;
    int fd;
    struct event ev;
};

rbtree *logfiles = NULL;
#endif

#ifdef SQL_SUPPORT
pid_t sqlslave_pid;
int sqlslave_socket = -1;
#endif

DESC *initializesock(int, struct sockaddr_in *);
DESC *new_connection(int);
int process_output(DESC *);
int process_input(DESC *);

void set_lastsite(DESC * d, char *lastsite)
{
    char buf[LBUF_SIZE];

    if (d->player) {
        if (!lastsite)
            lastsite = silly_atr_get(d->player, A_LASTSITE);
        strcpy(buf, lastsite);
        atr_add_raw(d->player, A_LASTSITE, buf);
    }
}

#ifdef SQL_SUPPORT
/*
 * get a result from the SQL slave
 */

static int get_sqlslave_result() {
    dbref thing;
    int attr, len;
    static char response[SQLQUERY_MAX_STRING], result[SQLQUERY_MAX_STRING], preserve[SQLQUERY_MAX_STRING];
    char *argv[3];

    if (sqlslave_socket == -1)
        return -100;

    memset(response, '\0', SQLQUERY_MAX_STRING);
    memset(result, '\0', SQLQUERY_MAX_STRING);
    memset(preserve, '\0', SQLQUERY_MAX_STRING);

    if (read(sqlslave_socket, &thing, sizeof(dbref)) <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;
        close(sqlslave_socket);
        sqlslave_socket = -1;
        event_del(&sqlslave_sock_ev);
        return -1;
    }
    if (read(sqlslave_socket, &attr, sizeof(int)) <= 0)
        return -2;
    if (read(sqlslave_socket, &len, sizeof(int)) <= 0)
        return -3;
    if (len > 0)
        if (read(sqlslave_socket, response, len) <= 0)
            return -4;
    if (read(sqlslave_socket, &len, sizeof(int)) <= 0)
        return -5;
    if (len > 0)
        if (read(sqlslave_socket, result, len) <= 0)
            return -6;
    if (read(sqlslave_socket, &len, sizeof(int)) <= 0)
        return -7;
    if (len > 0)
        if (read(sqlslave_socket, preserve, len) <= 0)
            return -8;

    argv[0] = response;
    argv[1] = result;
    argv[2] = preserve;
    did_it(GOD, thing, 0, NULL, 0, NULL, attr, argv, 3);
    return 0;
}

/*
 * Simple wrappers to get various mudconf pointers.
 * DISCLAIMER : This code was written under the influence while juggling the rest of the mechanics :P
 * I know this is 'VERY' uglee. 
 * TODO : Create a multi-dimensional (char *) array either globally or part of mudconf. 1 Dimension is A-E.
 * 2nd dimension is the HOSTNAME,etc... slots. This method needs a 2nd array of (int *) for the init val's.
 * 2nd solution is to make a struct of 1 (int *) and X (char *) and make an array of the struct. But,
 * do we really need to unprettify code for the one (int *) and have a minor structure extern'ed all over
 * the handfull of spots it's needed? To be decided. (dun dun dundun)
 * 
 * If anyone does the above TODO, get a patch in and delete these comments :D
 */

int *sqldb_slotinit(char db_slot) {
    switch (db_slot) {
        case 'A':
        case 'a':
            return &mudconf.sqlDB_init_A;
        case 'B':
        case 'b':
            return &mudconf.sqlDB_init_B;
        case 'C':
        case 'c':
            return &mudconf.sqlDB_init_C;
        case 'D':
        case 'd':
            return &mudconf.sqlDB_init_D;
        case 'E':
        case 'e':
            return &mudconf.sqlDB_init_E;
        default:
            return NULL;
    }
}

char *sqldb_slotval(char db_slot, int which) {
    switch (db_slot) {
        case 'A':
        case 'a':
            switch (which) {
                case SQLDB_SLOT_HOSTNAME:
                    return mudconf.sqlDB_hostname_A;
                case SQLDB_SLOT_DBNAME:
                    return mudconf.sqlDB_dbname_A;
                case SQLDB_SLOT_USERNAME:
                    return mudconf.sqlDB_username_A;
                case SQLDB_SLOT_PASSWORD:
                    return mudconf.sqlDB_password_A;
                case SQLDB_SLOT_DBTYPE:
                    return mudconf.sqlDB_type_A;
            }
        case 'B':
        case 'b':
            switch (which) {
                case SQLDB_SLOT_HOSTNAME:
                    return mudconf.sqlDB_hostname_B;
                case SQLDB_SLOT_DBNAME:
                    return mudconf.sqlDB_dbname_B;
                case SQLDB_SLOT_USERNAME:
                    return mudconf.sqlDB_username_B;
                case SQLDB_SLOT_PASSWORD:
                    return mudconf.sqlDB_password_B;
                case SQLDB_SLOT_DBTYPE:
                    return mudconf.sqlDB_type_B;
            }
        case 'C':
        case 'c':
            switch (which) {
                case SQLDB_SLOT_HOSTNAME:
                    return mudconf.sqlDB_hostname_C;
                case SQLDB_SLOT_DBNAME:
                    return mudconf.sqlDB_dbname_C;
                case SQLDB_SLOT_USERNAME:
                    return mudconf.sqlDB_username_C;
                case SQLDB_SLOT_PASSWORD:
                    return mudconf.sqlDB_password_C;
                case SQLDB_SLOT_DBTYPE:
                    return mudconf.sqlDB_type_C;
            }
        case 'D':
        case 'd':
            switch (which) {
                case SQLDB_SLOT_HOSTNAME:
                    return mudconf.sqlDB_hostname_D;
                case SQLDB_SLOT_DBNAME:
                    return mudconf.sqlDB_dbname_D;
                case SQLDB_SLOT_USERNAME:
                    return mudconf.sqlDB_username_D;
                case SQLDB_SLOT_PASSWORD:
                    return mudconf.sqlDB_password_D;
                case SQLDB_SLOT_DBTYPE:
                    return mudconf.sqlDB_type_D;
            }
        case 'E':
        case 'e':
            switch (which) {
                case SQLDB_SLOT_HOSTNAME:
                    return mudconf.sqlDB_hostname_E;
                case SQLDB_SLOT_DBNAME:
                    return mudconf.sqlDB_dbname_E;
                case SQLDB_SLOT_USERNAME:
                    return mudconf.sqlDB_username_E;
                case SQLDB_SLOT_PASSWORD:
                    return mudconf.sqlDB_password_E;
                case SQLDB_SLOT_DBTYPE:
                    return mudconf.sqlDB_type_E;
            }
        default:
            return NULL;
    }
}

/*
 * Internal call to send a query down to the libdbi SQL client.
 * It's a bit spamy but I wanted it to support open ended string lengths
 * so as to cap the data send/receives at whichever internal buffers were at hand,
 * instead of hardwiring sizes into a static structure.
 * To make this code cleaner....
 * TODO : Create a static transmit structure with all string sizes defined as LBUF
 * for the large potential ones, and SBUF/MBUF for the smaller ones liek username, PW,
 * etc... When transmitting, just loop through the proper location's instead of
 * calling each, and use strlen of course to define send sizes.
 *
 * How important? <shrug>
 */

void sqlslave_doquery(char db_slot, dbref thing, int attr, char *pres, char *qry) {
    int len;
    char *val;

    if (write(sqlslave_socket, &thing, sizeof(dbref)) <= 0) {
        close(sqlslave_socket);
        sqlslave_socket = -1;
        event_del(&sqlslave_sock_ev);
        return;
    }
    if (write(sqlslave_socket, &attr, sizeof(int)) <= 0)
        return;

    val = sqldb_slotval(db_slot, SQLDB_SLOT_DBTYPE);
    len = strlen(val);
    if (write(sqlslave_socket, &len, sizeof(int)) <= 0)
        return;
    if (len > 0)
        if (write(sqlslave_socket, val, len) <= 0)
            return;

    val = sqldb_slotval(db_slot, SQLDB_SLOT_HOSTNAME);
    len = strlen(val);
    if (write(sqlslave_socket, &len, sizeof(int)) <= 0)
        return;
    if (len > 0)
        if (write(sqlslave_socket, val, len) <= 0)
            return;

    val = sqldb_slotval(db_slot, SQLDB_SLOT_USERNAME);
    len = strlen(val);
    if (write(sqlslave_socket, &len, sizeof(int)) <= 0)
        return;
    if (len > 0)
        if (write(sqlslave_socket, val, len) <= 0)
            return;

    val = sqldb_slotval(db_slot, SQLDB_SLOT_PASSWORD);
    len = strlen(val);
    if (write(sqlslave_socket, &len, sizeof(int)) <= 0)
        return;
    if (len > 0)
        if (write(sqlslave_socket, val, len) <= 0)
            return;

    val = sqldb_slotval(db_slot, SQLDB_SLOT_DBNAME);
    len = strlen(val);
    if (write(sqlslave_socket, &len, sizeof(int)) <= 0)
        return;
    if (len > 0)
        if (write(sqlslave_socket, val, len) <= 0)
            return;

    len = strlen(pres);
    if (len >= SQLQUERY_MAX_STRING)
        len = SQLQUERY_MAX_STRING;
    if (write(sqlslave_socket, &len, sizeof(int)) <= 0)
        return;
    if (len > 0)
        if (write(sqlslave_socket, pres, len) <= 0)
            return;

    len = strlen(qry);
    if (len >= SQLQUERY_MAX_STRING)
        len = SQLQUERY_MAX_STRING;
    if (write(sqlslave_socket, &len, sizeof(int)) <= 0)
        return;
    if (len > 0)
        if (write(sqlslave_socket, qry, len) <= 0)
            return;
    return;
}

/*
 * Boot and/or Restart the SQL Slave
 */

void boot_sqlslave() {
    int sv[2];
    int i;
    int maxfds;

#ifdef HAVE_GETDTABLESIZE
    maxfds = getdtablesize();
#else
    maxfds = sysconf(_SC_OPEN_MAX);
#endif

    if (sqlslave_socket != -1) {
        close(sqlslave_socket);
        sqlslave_socket = -1;
        event_del(&sqlslave_sock_ev);
    }

    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) < 0) {
        return;
    }
    /*
     * set to nonblocking
     */
    if (fcntl(sv[0], F_SETFL, O_NONBLOCK) == -1) {
        close(sv[0]);
        close(sv[1]);
        return;
    }
    sqlslave_pid = vfork();
    switch (sqlslave_pid) {
        case -1:
            close(sv[0]);
            close(sv[1]);
            return;
        case 0:                     /*
                                     * * child
                                     */
            close(sv[0]);
            close(0);
            close(1);
            if (dup2(sv[1], 0) == -1) {
                _exit(1);
            }
            if (dup2(sv[1], 1) == -1) {
                _exit(1);
            }
            for (i = 3; i < maxfds; ++i) {
                close(i);
            }
            execlp("bin/sqlslave", "sqlslave", NULL);
            _exit(1);
    }
    close(sv[1]);

    if (fcntl(sv[0], F_SETFL, O_NONBLOCK) == -1) {
        close(sv[0]);
        return;
    }
    sqlslave_socket = sv[0];
    event_set(&sqlslave_sock_ev, sqlslave_socket, EV_READ | EV_PERSIST, 
            accept_sqlslave_input, NULL);
    event_add(&sqlslave_sock_ev);
}
#endif

#ifdef ARBITRARY_LOGFILES
static int logcache_compare(void *vleft, void *vright, void *arg) {
    return strcmp((char *)vleft, (char *)vright);
}

int logcache_close(struct logfile_t *log) {
    fprintf(stderr, "logcache] closing logfile %s\n", log->filename);
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

static void logcache_expire(int fd, short event, void *arg) {
    logcache_close((struct logfile_t *)arg);
}

static int _logcache_list(void *key, void *data, int depth, void *arg) {
    struct timeval tv;
    struct logfile_t *log = (struct logfile_t *)data;
    dbref player = *(dbref *)arg;
    evtimer_pending(&log->ev, &tv);
    notify_printf(player, "%-40s%d", log->filename, tv.tv_sec-mudstate.now);
    return 1;
}
    
void logcache_list(dbref player) {
    if(rb_size(logfiles) == 0) {
        notify(player, "There are no open logfile handles.");
        return;
    }
    notify(player, "/--------------------------- Open Logfiles");
    notify(player, "Filename                               Timeout");  
    rb_walk(logfiles, WALK_INORDER, _logcache_list, &player);
}

static int logcache_open(char *filename) {
    int fd;
    struct logfile_t *newlog;
    struct timeval tv = { LOGFILE_TIMEOUT, 0 };

    if(rb_exists(logfiles, filename)) {
        fprintf(stderr, "Serious braindamage, logcache_open() called for already open logfile.\n");
        return 0;
    }
    
    fd = open(filename, O_RDWR|O_APPEND|O_CREAT, 0644);
    if(fd < 0) {
        fprintf(stderr, "Failed to open logfile %s because open() failed with code: %d -  %s\n", 
                filename, errno, strerror(errno));
        return 0;
    }
    
    newlog = malloc(sizeof(struct logfile_t));
    newlog->fd = fd;
    newlog->filename = strdup(filename);
    evtimer_set(&newlog->ev, logcache_expire, newlog);
    evtimer_add(&newlog->ev, &tv);
    rb_insert(logfiles, newlog->filename, newlog);
    fprintf(stderr, "logcache] opened logfile %s\n", filename);
    return 1;
}

void boot_logcache() {
    if(!logfiles)
        logfiles = rb_init(logcache_compare, NULL);
}

static int _logcache_destruct(void *key, void *data, int depth, void *arg) {
    struct logfile_t *log = (struct logfile_t *)data;
    logcache_close(log);
    return 1;
}

void logcache_destruct() {
    if(!logfiles) return;
    rb_walk(logfiles, WALK_INORDER, _logcache_destruct, NULL);
    rb_destroy(logfiles);
    logfiles = NULL;
}

void logcache_writelog(dbref thing, char *fname, char *fdata) {
    struct logfile_t *log;
    struct timeval tv = { LOGFILE_TIMEOUT, 0 };
    int len;
   
    if(!logfiles) boot_logcache();

    len = strlen(fdata);

    log = rb_find(logfiles, fname);

    if(!log) {
        if(logcache_open(fname) < 0) {
            notify(thing, "Something awful happened trying to open logfile, check system logs.");
            return;
        }
        log = rb_find(logfiles, fname);
        if(!log) {
            notify(thing, "Serious inconsistency in logcache code, check system logs.");
            return;
        }
    }

    if(evtimer_pending(&log->ev, NULL)) {
        event_del(&log->ev);
        event_add(&log->ev, &tv);
    }

    if(write(log->fd, fdata, len) < 0) {
        fprintf(stderr, "System failed to write data to file with error '%s' on logfile '%s'. Closing.\n", 
                strerror(errno), log->filename);
        logcache_close(log);
    }
    return;
}
#endif

/*
 * get a result from the slave 
 */

static int get_slave_result() {
    char *buf;
    char *token;
    char *os;
    char *userid;
    char *host;
    int local_port, remote_port;
    char *p;
    DESC *d;
    int len;

    buf = alloc_lbuf("slave_buf");

    len = read(slave_socket, buf, LBUF_SIZE - 1);
    if (len < 0) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            free_lbuf(buf);
            return (-1);
        }
        close(slave_socket);
        slave_socket = -1;
        event_del(&slave_sock_ev);
        free_lbuf(buf);
        return (-1);
    } else if (len == 0) {
        free_lbuf(buf);
        return (-1);
    }
    buf[len] = '\0';

    token = alloc_lbuf("slave_token");
    os = alloc_lbuf("slave_os");
    userid = alloc_lbuf("slave_userid");
    host = alloc_lbuf("slave_host");

    if (sscanf(buf, "%s %s", host, token) != 2) {
        free_lbuf(buf);
        free_lbuf(token);
        free_lbuf(os);
        free_lbuf(userid);
        free_lbuf(host);
        return (0);
    }
    p = strchr(buf, '\n');
    *p = '\0';
    for (d = descriptor_list; d; d = d->next) {
        if (strcmp(d->addr, host))
            continue;
        if (d->flags & DS_IDENTIFIED)
            continue;
        if (mudconf.use_hostname) {
            StringCopyTrunc(d->addr, token, 50);
            d->addr[50] = '\0';
            if (d->player) {
                if (d->username[0])
                    set_lastsite(d, tprintf("%s@%s", d->username,
                                d->addr));
                else
                    set_lastsite(d, d->addr);

            }
        }
    }

    if (sscanf(p + 1, "%s %d , %d : %s : %s : %s", host, &remote_port,
                &local_port, token, os, userid) != 6) {
        free_lbuf(buf);
        free_lbuf(token);
        free_lbuf(os);
        free_lbuf(userid);
        free_lbuf(host);
        return (0);
    }
    for (d = descriptor_list; d; d = d->next) {
        if (ntohs((d->address).sin_port) != remote_port)
            continue;
        if (d->flags & DS_IDENTIFIED)
            continue;
        StringCopyTrunc(d->username, userid, 10);
        d->username[10] = '\0';
        set_lastsite(d, tprintf("%s@%s", d->username, d->addr));
        free_lbuf(buf);
        free_lbuf(token);
        free_lbuf(os);
        free_lbuf(userid);
        free_lbuf(host);
        return (0);
    }
    free_lbuf(buf);
    free_lbuf(token);
    free_lbuf(os);
    free_lbuf(userid);
    free_lbuf(host);
    return (0);
}

void boot_slave() {
    int sv[2];
    int i;
    int maxfds;

#ifdef HAVE_GETDTABLESIZE
    maxfds = getdtablesize();
#else
    maxfds = sysconf(_SC_OPEN_MAX);
#endif

    if (slave_socket != -1) {
        close(slave_socket);
        slave_socket = -1;
        event_del(&slave_sock_ev);
    }
    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) < 0) {
        return;
    }
    /*
     * set to nonblocking 
     */
    if (fcntl(sv[0], F_SETFL, O_NONBLOCK) == -1) {
        close(sv[0]);
        close(sv[1]);
        return;
    }
    slave_pid = fork();
    switch (slave_pid) {
        case -1:
            close(sv[0]);
            close(sv[1]);
            return;

        case 0:			/*
                         * * child  
                         */
            close(sv[0]);
            close(0);
            close(1);
            if (dup2(sv[1], 0) == -1) {
                _exit(1);
            }
            if (dup2(sv[1], 1) == -1) {
                _exit(1);
            }
            for (i = 3; i < maxfds; ++i) {
                close(i);
            }
            execlp("bin/slave", "slave", NULL);
            _exit(1);
            break;
        default:
            break;
    }
    close(sv[1]);

    if (fcntl(sv[0], F_SETFL, O_NONBLOCK) == -1) {
        close(sv[0]);
        return;
    }
    slave_socket = sv[0];
    event_set(&slave_sock_ev, slave_socket, EV_READ | EV_PERSIST, 
            accept_slave_input, NULL);
    event_add(&slave_sock_ev, NULL);
}

int make_socket(int port) {
    int s, opt;
    struct sockaddr_in server;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        log_perror("NET", "FAIL", NULL, "creating master socket");
        exit(3);
    }
    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
                sizeof(opt)) < 0) {
        log_perror("NET", "FAIL", NULL, "setsockopt");
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    if (!mudstate.restarting)
        if (bind(s, (struct sockaddr *) &server, sizeof(server))) {
            log_perror("NET", "FAIL", NULL, "bind");
            close(s);
            exit(4);
        }
    listen(s, 5);
    return s;
}

static int eradicate_broken_fd(void)
{
    struct stat statbuf;
    DESC *d;

    DESC_ITER_ALL(d) {
        if (fstat(d->descriptor, &statbuf) < 0) {
            /* An invalid player connection... eject, eject, eject. */
            STARTLOG(LOG_PROBLEMS, "ERR", "EBADF") {
                log_text("Broken descriptor ");
                log_number(d->descriptor);
                log_text(" for player ");
                log_name(d->player);
                ENDLOG;
            }
            shutdownsock(d, R_SOCKDIED);
        }
    }
    if (slave_socket != -1 && fstat(slave_socket, &statbuf) < 0) {
        STARTLOG(LOG_PROBLEMS, "ERR", "EBADF") {
            log_text("Broken descriptor for DNS slave: ");
            log_number(slave_socket);
            ENDLOG;
        }
        boot_slave();
    }
#ifdef SQL_SUPPORT
    if (sqlslave_socket != -1 && fstat(sqlslave_socket, &statbuf) < 0) {
        STARTLOG(LOG_PROBLEMS, "ERR", "EBADF") {
            log_text("Broken descriptor for SQL slave: ");
            log_number(sqlslave_socket);
            ENDLOG;
        }
        boot_sqlslave();
    }
#endif
    if (sock != -1 && fstat(sock, &statbuf) < 0) {
        STARTLOG(LOG_PROBLEMS, "ERR", "EBADF") {
            log_text("Broken descriptor for our main port: ");
            log_number(slave_socket);
            ENDLOG;
        }
        sock = -1;
        return -1;
    }
    return 0;
}

void accept_client_input(int fd, short event, void *arg) {
    DESC *connection = (DESC *)arg;

    if(connection->flags & DS_AUTODARK) {
        connection->flags &= ~DS_AUTODARK;
        s_Flags(connection->player, Flags(connection->player) & ~DARK);
    }

    if(!process_input(connection)) {
        shutdownsock(connection, R_SOCKDIED);
    }
}

void accept_new_connection(int fd, short event, void *arg) {
    DESC *newfd;

    newfd = new_connection(fd);


    event_set(&newfd->sock_ev, newfd->descriptor, EV_READ | EV_PERSIST, 
            accept_client_input, newfd);
    event_add(&newfd->sock_ev, NULL);
}

void accept_slave_input(int fd, short event, void *arg) {
    while (get_slave_result() == 0);
}

#ifdef SQL_SUPPORT
void accept_sqlslave_input(int fd, short event, void *arg) {
    while (get_sqlslave_result() == 0);
}
#endif

#ifndef HAVE_GETTIMEOFDAY
#define get_tod(x)	{ (x)->tv_sec = time(NULL); (x)->tv_usec = 0; }
#else
#define get_tod(x)	gettimeofday(x, (struct timezone *)0)
#endif

void shovechars(int port) {
    struct timeval last_slice, current_time, next_slice, timeout,
                   slice_timeout;

    mudstate.debug_cmd = (char *) "< shovechars >";

    if (!mudstate.restarting) {
        signal(SIGPIPE, SIG_IGN);
        sock = make_socket(port);
        event_set(&listen_sock_ev, sock, EV_READ | EV_PERSIST, 
                accept_new_connection, NULL);
        event_add(&listen_sock_ev, NULL);
    }
    get_tod(&last_slice);

    while (mudstate.shutdown_flag == 0) {
        get_tod(&current_time);
        last_slice = update_quotas(last_slice, current_time);

        process_commands();
        if (mudstate.shutdown_flag)
            break;

        /*
         * test for events 
         */

        dispatch();

        /*
         * any queued robot commands waiting? 
         */

        timeout.tv_sec = que_next();
        timeout.tv_usec = 0;
        next_slice = msec_add(last_slice, mudconf.timeslice);
        slice_timeout = timeval_sub(next_slice, current_time);

        /* run event loop */

        if(event_loop(EVLOOP_ONCE) < 0) {
            perror("event_loop");
            exit(0);
        }

        /*
         * run robot commands
         */

        if (mudconf.queue_chunk)
            do_top(mudconf.queue_chunk);
    }
}

DESC *new_connection(int sock) {
    int newsock;
    char *buff, *buff1, *cmdsave;
    DESC *d;
    struct sockaddr_in addr;
    unsigned int addr_len;
    int len;
    char *buf;


    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "< new_connection >";
    addr_len = sizeof(struct sockaddr);

    newsock = accept(sock, (struct sockaddr *)&addr, &addr_len);
    if (newsock < 0)
        return 0;


    if (site_check(addr.sin_addr, mudstate.access_list) == H_FORBIDDEN) {
        STARTLOG(LOG_NET | LOG_SECURITY, "NET", "SITE") {
            buff = alloc_mbuf("new_connection.LOG.badsite");
            sprintf(buff, "[%d/%s] Connection refused.  (Remote port %d)",
                    newsock, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
            log_text(buff);
            free_mbuf(buff);
            ENDLOG;
        }
        fcache_rawdump(newsock, FC_CONN_SITE);

        shutdown(newsock, 2);
        close(newsock);
        errno = 0;
        d = NULL;
    } else {
        buff = alloc_mbuf("new_connection.address");
        buf = alloc_lbuf("new_connection.write");
        StringCopy(buff, inet_ntoa(addr.sin_addr));

        /*
         * Ask slave process for host and username 
         */
        if ((slave_socket != -1) && mudconf.use_hostname) {
            sprintf(buf, "%s\n%s,%d,%d\n", inet_ntoa(addr.sin_addr),
                    inet_ntoa(addr.sin_addr), ntohs(addr.sin_port),
                    mudconf.port);
            len = strlen(buf);
            if (WRITE(slave_socket, buf, len) < 0) {
                close(slave_socket);
                slave_socket = -1;
                event_del(&slave_sock_ev);
            }
        }
        free_lbuf(buf);
        STARTLOG(LOG_NET, "NET", "CONN") {
            buff1 = alloc_mbuf("new_connection.LOG.open");
            sprintf(buff1, "[%d/%s] Connection opened (remote port %d)",
                    newsock, buff, ntohs(addr.sin_port));
            log_text(buff1);
            free_mbuf(buff1);
            ENDLOG;
        }
        d = initializesock(newsock, &addr);

        mudstate.debug_cmd = cmdsave;
        free_mbuf(buff);
    }
    mudstate.debug_cmd = cmdsave;
    return (d);
}

/*
 * Disconnect reasons that get written to the logfile 
 */

static const char *disc_reasons[] = {
    "Unspecified",
    "Quit",
    "Inactivity Timeout",
    "Booted",
    "Remote Close or Net Failure",
    "Game Shutdown",
    "Login Retry Limit",
    "Logins Disabled",
    "Logout (Connection Not Dropped)",
    "Too Many Connected Players"
};

/*
 * Disconnect reasons that get fed to A_ADISCONNECT via announce_disconnect 
 */

static const char *disc_messages[] = {
    "unknown",
    "quit",
    "timeout",
    "boot",
    "netdeath",
    "shutdown",
    "badlogin",
    "nologins",
    "logout"
};

void shutdownsock(DESC *d, int reason) {
    char *buff, *buff2;
    time_t now;
    int i, num;
    DESC *dtemp;

    if ((reason == R_LOGOUT) &&
            (site_check((d->address).sin_addr,
                        mudstate.access_list) == H_FORBIDDEN))
        reason = R_QUIT;

    if (d->flags & DS_CONNECTED) {

        /*
         * Do the disconnect stuff if we aren't doing a LOGOUT * * *
         * * * * (which keeps the connection open so the player can *
         * * connect * * * * to a different character). 
         */

        if (reason != R_LOGOUT) {
            fcache_dump(d, FC_QUIT);
            STARTLOG(LOG_NET | LOG_LOGIN, "NET", "DISC") {
                buff = alloc_mbuf("shutdownsock.LOG.disconn");
                sprintf(buff, "[%d/%s] Logout by ", d->descriptor,
                        d->addr);
                log_text(buff);
                log_name(d->player);
                sprintf(buff, " <Reason: %s>", disc_reasons[reason]);
                log_text(buff);
                free_mbuf(buff);
                ENDLOG;
            }
        } else {
            STARTLOG(LOG_NET | LOG_LOGIN, "NET", "LOGO") {
                buff = alloc_mbuf("shutdownsock.LOG.logout");
                sprintf(buff, "[%d/%s] Logout by ", d->descriptor,
                        d->addr);
                log_text(buff);
                log_name(d->player);
                sprintf(buff, " <Reason: %s>", disc_reasons[reason]);
                log_text(buff);
                free_mbuf(buff);
                ENDLOG;
            }
        }

        /*
         * If requested, write an accounting record of the form: * *
         * * * * * Plyr# Flags Cmds ConnTime Loc Money [Site]
         * <DiscRsn>  * *  * Name 
         */

        STARTLOG(LOG_ACCOUNTING, "DIS", "ACCT") {
            now = mudstate.now - d->connected_at;
            buff = alloc_lbuf("shutdownsock.LOG.accnt");
            buff2 =
                decode_flags(GOD, Flags(d->player), Flags2(d->player),
                        Flags3(d->player));
            sprintf(buff, "%d %s %d %d %d %d [%s] <%s> %s", d->player,
                    buff2, d->command_count, (int) now, Location(d->player),
                    Pennies(d->player), d->addr, disc_reasons[reason],
                    Name(d->player));
            log_text(buff);
            free_lbuf(buff);
            free_sbuf(buff2);
            ENDLOG;
        } announce_disconnect(d->player, d, disc_messages[reason]);
    } else {
        if (reason == R_LOGOUT)
            reason = R_QUIT;
        STARTLOG(LOG_SECURITY | LOG_NET, "NET", "DISC") {
            buff = alloc_mbuf("shutdownsock.LOG.neverconn");
            sprintf(buff,
                    "[%d/%s] Connection closed, never connected. <Reason: %s>",
                    d->descriptor, d->addr, disc_reasons[reason]);
            log_text(buff);
            free_mbuf(buff);
            ENDLOG;
        }
    }
    // process_output(d);
    clearstrings(d);
    if (reason == R_LOGOUT) {
        d->flags &= ~DS_CONNECTED;
        d->connected_at = mudstate.now;
        d->retries_left = mudconf.retry_limit;
        d->command_count = 0;
        d->timeout = mudconf.idle_timeout;
        d->player = 0;
        d->doing[0] = '\0';
        d->hudkey[0] = '\0';
        d->quota = mudconf.cmd_quota_max;
        d->last_time = 0;
        d->host_info =
            site_check((d->address).sin_addr,
                    mudstate.access_list) | site_check((d->address).sin_addr,
                        mudstate.suspect_list);
        d->input_tot = d->input_size;
        d->output_tot = 0;
        welcome_user(d);
    } else {
        event_del(&d->sock_ev);
        shutdown(d->descriptor, 2);
        close(d->descriptor);

        freeqs(d);
        *d->prev = d->next;
        if (d->next)
            d->next->prev = d->prev;

        /*
         * Is this desc still in interactive mode? 
         */
        if (d->program_data != NULL) {
            num = 0;
            DESC_ITER_PLAYER(d->player, dtemp) num++;

            if (num == 0) {
                for (i = 0; i < MAX_GLOBAL_REGS; i++) {
                    free_lbuf(d->program_data->wait_regs[i]);
                }
                free(d->program_data);
            }
        }

        free_desc(d);
        ndescriptors--;
    }
}

void make_nonblocking(int s) {
    long flags;

    if(fcntl(s, F_GETFL, &flags)<0) {
        log_perror("NET", "FAIL", "make_nonblocking", "fcntl F_GETFL");
    }
    flags |= O_NONBLOCK;
    flags &= ~O_NDELAY;
    if(fcntl(s, F_SETFL, flags)<0) {
        log_perror("NET", "FAIL", "make_nonblocking", "fcntl F_SETFL");
    }
}

extern int fcache_conn_c;

DESC *initializesock(int s, struct sockaddr_in *a) {
    DESC *d;

    ndescriptors++;
    d = alloc_desc("init_sock");
    d->descriptor = s;
    if (fcache_conn_c)
        d->logo = rand() % fcache_conn_c;
    d->flags = 0;
    d->connected_at = mudstate.now;
    d->retries_left = mudconf.retry_limit;
    d->command_count = 0;
    d->timeout = mudconf.idle_timeout;
    d->host_info =
        site_check((*a).sin_addr,
                mudstate.access_list) | site_check((*a).sin_addr,
                    mudstate.suspect_list);
    d->player = 0;		/*
                         * be sure #0 isn't wizard.  Shouldn't be. 
                         */

    d->addr[0] = '\0';
    d->doing[0] = '\0';
    d->hudkey[0] = '\0';
    d->username[0] = '\0';
    make_nonblocking(s);
    d->output_prefix = NULL;
    d->output_suffix = NULL;
    d->output_size = 0;
    d->output_tot = 0;
    d->output_lost = 0;
    d->input_head = NULL;
    d->input_tail = NULL;
    d->input_size = 0;
    d->input_tot = 0;
    d->input_lost = 0;
    d->raw_input = NULL;
    d->raw_input_at = NULL;
    d->quota = mudconf.cmd_quota_max;
    d->program_data = NULL;
    d->last_time = 0;
    d->address = *a;		/*
                             * added 5/3/90 SCG 
                             */
    if (descriptor_list)
        descriptor_list->prev = &d->next;
    d->hashnext = NULL;
    d->next = descriptor_list;
    d->prev = &descriptor_list;
    StringCopyTrunc(d->addr, inet_ntoa(a->sin_addr), 50);
    descriptor_list = d;
    welcome_user(d);
    return d;
}

extern int muxevent_tick;
int last_bug2 = -1;
int last_bug = -1;
int last_bugc = 0;		/* If 4+ bugs per second, and/or 3 bugs per 2sec, *bang* */

int fatal_bug() {
    last_bugc++;
    if (last_bugc == 4)
        return 1;
    if (last_bug2 == (last_bug - 1) && last_bugc == 2)
        return 1;
    return 0;
}

int process_input(DESC *d) {
    static char buf[LBUF_SIZE];
    int got, in, lost;
    char *p, *pend, *q, *qend;
    char *cmdsave;

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "< process_input >";

    got = in = read(d->descriptor, buf, (sizeof buf - 1));
    if (got <= 0)  {
        if(errno == EINTR) return 1;
        else return 0;
    }

    buf[got] = 0;
    if (!d->raw_input) {
        d->raw_input = (CBLK *) alloc_lbuf("process_input.raw");
        d->raw_input_at = d->raw_input->cmd;
    }
    p = d->raw_input_at;
    pend = d->raw_input->cmd + LBUF_SIZE - sizeof(CBLKHDR) - 1;
    lost = 0;
    for (q = buf, qend = buf + got; q < qend; q++) {
        if (*q == '\n') {
            *p = '\0';
            if (p > d->raw_input->cmd) {
                save_command(d, d->raw_input);
                d->raw_input = (CBLK *) alloc_lbuf("process_input.raw");

                p = d->raw_input_at = d->raw_input->cmd;
                pend = d->raw_input->cmd + LBUF_SIZE - sizeof(CBLKHDR) - 1;
            } else {
                in -= 1;	/*
                             * for newline 
                             */
            }
        } else if ((*q == '\b') || (*q == 127)) {
            if (*q == 127)
                queue_string(d, "\b \b");
            else
                queue_string(d, " \b");
            in -= 2;
            if (p > d->raw_input->cmd)
                p--;
            if (p < d->raw_input_at)
                (d->raw_input_at)--;
        } else if (p < pend && ((isascii(*q) && isprint(*q)) || *q < 0)) {
            *p++ = *q;
        } else {
            in--;
            if (p >= pend)
                lost++;
        }
    }
    if (p > d->raw_input->cmd) {
        d->raw_input_at = p;
    } else {
        free_lbuf(d->raw_input);
        d->raw_input = NULL;
        d->raw_input_at = NULL;
    }
    d->input_tot += got;
    d->input_size += in;
    d->input_lost += lost;
    mudstate.debug_cmd = cmdsave;
    return 1;
}

void close_sockets(int emergency, char *message) {
    DESC *d, *dnext;

    DESC_SAFEITER_ALL(d, dnext) {
        if (emergency) {
            WRITE(d->descriptor, message, strlen(message));
            if (shutdown(d->descriptor, 2) < 0)
                log_perror("NET", "FAIL", NULL, "shutdown");
            close(d->descriptor);
            event_del(&d->sock_ev);
        } else {
            queue_string(d, message);
            queue_write(d, "\r\n", 2);
            shutdownsock(d, R_GOING_DOWN);
        }
    }
    close(sock);
    event_del(&listen_sock_ev);
}

void emergency_shutdown(void)
{
    close_sockets(1, (char *) "Going down - Bye.\n");
}


/*
 * ---------------------------------------------------------------------------
 * * Signal handling routines.
 */

#ifndef SIGCHLD
#define SIGCHLD SIGCLD
#endif

#ifdef HAVE_STRUCT_SIGCONTEXT
static RETSIGTYPE sighandler(int, int, struct sigcontext);
#else
static RETSIGTYPE sighandler(int);
#endif

/* *INDENT-OFF* */

NAMETAB sigactions_nametab[] = {
    {(char *)"exit",	3,	0,	SA_EXIT},
    {(char *)"default",	1,	0,	SA_DFLT},
    { NULL,			0,	0,	0}};

/* *INDENT-ON* */

void set_signals(void)
{
#if 0
    signal(SIGALRM, sighandler);
#endif
    signal(SIGCHLD, sighandler);
#if 0
    signal(SIGHUP, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGQUIT, sighandler);
    signal(SIGTERM, sighandler);
#endif
    signal(SIGPIPE, SIG_IGN);
#if 0
    signal(SIGUSR1, sighandler);
    signal(SIGUSR2, sighandler);
    signal(SIGTRAP, sighandler);
#ifdef SIGXCPU
    signal(SIGXCPU, sighandler);
#endif

    signal(SIGILL, sighandler);
#ifdef __linux__
    signal(SIGFPE, SIG_IGN);
#else
    signal(SIGFPE, sighandler);
#endif
    signal(SIGSEGV, sighandler);
    signal(SIGABRT, sighandler);
#ifdef SIGFSZ
    signal(SIGXFSZ, sighandler);
#endif
#ifdef SIGEMT
    signal(SIGEMT, sighandler);
#endif
#ifdef SIGBUS
    signal(SIGBUS, sighandler);
#endif
#ifdef SIGSYS
    signal(SIGSYS, sighandler);
#endif
#endif
}

static void unset_signals()
{
    int i;

    for (i = 0; i < NSIG; i++)
        signal(i, SIG_DFL);
    abort();
}

static void check_panicking(int sig) {
    int i;

    /*
     * If we are panicking, turn off signal catching and resignal 
     */

    if (mudstate.panicking) {
        for (i = 0; i < NSIG; i++)
            signal(i, SIG_DFL);
        kill(getpid(), sig);
    }
    mudstate.panicking = 1;
}

void log_signal(const char *signame) {
    STARTLOG(LOG_PROBLEMS, "SIG", "CATCH") {
        log_text((char *) "Caught signal ");
        log_text((char *) signame);
        ENDLOG;
    }
}


void log_commands(int sig)
{
}

#ifdef HAVE_STRUCT_SIGCONTEXT
static RETSIGTYPE sighandler(sig, code, scp)
    int sig;
    int code;
    struct sigcontext *scp;

#else
static RETSIGTYPE sighandler(sig)
    int sig;

#endif
{
#ifdef SYS_SIGLIST_DECLARED
#define signames sys_siglist
#else
    static const char *signames[] = {
        "SIGZERO", "SIGHUP", "SIGINT", "SIGQUIT",
        "SIGILL", "SIGTRAP", "SIGABRT", "SIGEMT",
        "SIGFPE", "SIGKILL", "SIGBUS", "SIGSEGV",
        "SIGSYS", "SIGPIPE", "SIGALRM", "SIGTERM",
        "SIGURG", "SIGSTOP", "SIGTSTP", "SIGCONT",
        "SIGCHLD", "SIGTTIN", "SIGTTOU", "SIGIO",
        "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF",
        "SIGWINCH", "SIGLOST", "SIGUSR1", "SIGUSR2"
    };

#endif

    char buff[32];

#ifdef HAVE_UNION_WAIT
    union wait stat = {0};

#else
    int stat;

#endif

    switch (sig) {
        case SIGUSR1:
            do_restart(1, 1, 0);
            break;
        case SIGALRM:		/*
                             * Timer 
                             */
            mudstate.alarm_triggered = 1;
            break;
        case SIGCHLD:		/*
                             * Change in child status 
                             */
#ifndef SIGNAL_SIGCHLD_BRAINDAMAGE
            signal(SIGCHLD, sighandler);
#endif
#ifdef HAVE_WAIT3
            while (wait3(&stat, WNOHANG, NULL) > 0);
#else
            wait(&stat);
#endif
            /* Did the child exit? */

            if (WEXITSTATUS(stat) == 8)
                exit(0);

            mudstate.dumping = 0;
            break;
        case SIGHUP:		/*
                             * Perform a database dump 
                             */
            log_signal(signames[sig]);
            mudstate.dump_counter = 0;
            break;
        case SIGINT:		/*
                             * Log + ignore 
                             */
            log_signal(signames[sig]);
            break;
        case SIGQUIT:		/*
                             * Normal shutdown 
                             */
        case SIGTERM:
#ifdef SIGXCPU
        case SIGXCPU:
#endif
            check_panicking(sig);
            log_signal(signames[sig]);
            log_commands(sig);
            sprintf(buff, "Caught signal %s, exiting.", signames[sig]);
            raw_broadcast(0, buff);
            dump_database_internal(DUMP_KILLED);
            exit(0);
            break;
        case SIGILL:		/*
                             * Panic save + restart 
                             */
        case SIGFPE:
        case SIGSEGV:
        case SIGTRAP:
#ifdef SIGXFSZ
        case SIGXFSZ:
#endif
#ifdef SIGEMT
        case SIGEMT:
#endif
#ifdef SIGBUS
        case SIGBUS:
#endif
#ifdef SIGSYS
        case SIGSYS:
#endif
            /*
             * Try our best to dump a core first 
             */
            if (!fork()) {
                unset_signals();
                return;
            }
            check_panicking(sig);
            log_signal(signames[sig]);
            report();
            if (mudconf.sig_action != SA_EXIT) {
                char outdb[128];
                char indb[128];

                log_commands(sig);
                raw_broadcast(0,
                        "Game: Fatal signal %s caught, restarting with previous database.",
                        signames[sig]);

                /* Don't sync first. Using older db. */

                dump_database_internal(DUMP_CRASHED);
                shutdown(slave_socket, 2);
                kill(slave_pid, SIGKILL);
#ifdef SQL_SUPPORT
                shutdown(sqlslave_socket, 2);
                kill(sqlslave_pid, SIGKILL);
                event_del(&sqlslave_sock_ev);
#endif
#ifdef ARBITRARY_LOGFILES
                logcache_destruct();
#endif

                if (mudconf.compress_db) {
                    sprintf(outdb, "%s.Z", mudconf.outdb);
                    sprintf(indb, "%s.Z", mudconf.indb);
                    rename(outdb, indb);
                } else {
                    rename(mudconf.outdb, mudconf.indb);
                }
                if (mudconf.have_specials)
                    ChangeSpecialObjects(0);
                dump_restart_db();
                execl("bin/netmux", "netmux", mudconf.config_file, NULL);
                break;
            } else {
                unset_signals();
                signal(sig, SIG_DFL);
                exit(1);
            }
        case SIGABRT:		/*
                             * Coredump. 
                             */
            check_panicking(sig);
            log_signal(signames[sig]);
            report();

            unset_signals();
            signal(sig, SIG_DFL);
            exit(1);

    }
    signal(sig, sighandler);
    mudstate.panicking = 0;
    return;
}

