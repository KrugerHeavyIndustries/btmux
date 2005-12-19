/*
 * sqlchild.c
 *
 * Copyright (c) 2004,2005 Martin Murray <mmurray@monkey.org>
 * All rights reserved.
 *
 */

#include "copyright.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dbi/dbi.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include "externs.h"

/* /TODO
 * String sanitization.
 *  - dbi_driver_quote_string_copy ?
 * 
 * Handle Timezones in DATETIME
 *
 * Handle final write() failing.
 * 
 * Correct handling of TEXT data type.
 */

#define DEBUG_SQL
#ifdef DEBUG_SQL
#ifndef DEBUG
#define DEBUG
#endif
#endif
#include "debug.h"

#ifdef SQL_SUPPORT
#define MAX_QUERIES 8

static int running_queries = 0;
dbi_conn conn=NULL;

#define DBIS_EFAIL -1
#define DBIS_READY 0
#define DBIS_RESOURCE 1

int dbi_initialized = 0;
int dbi_state;
int query_counter = 0;
int recent = 0;

struct query_state_t {
    dbref thing;
    int attr;
    char *preserve;
    char *query;
    struct event ev;
    char *rdelim;
    char *cdelim;
    struct query_state_t *next;
    int serial;
    struct timeval start;
    char slot;
    int fd;
    int pid;
} *running = NULL, *pending = NULL, *pending_tail = NULL, *recent_head = NULL, *recent_tail = NULL;

struct timeval query_timeout = { 10, 0 };

void sqlchild_init();
void sqlchild_destruct();
static void sqlchild_kill_all();
int sqlchild_kill(int requestId);
void sqlchild_list(dbref thing);
static void sqlchild_kill_query(struct query_state_t *aqt);
static void sqlchild_child_abort_query(struct query_state_t *aqt, char *error);
static void sqlchild_child_abort_query_dbi(struct query_state_t *aqt, char *error);
static void sqlchild_child_execute_query(struct query_state_t *aqt);
static void sqlchild_finish_query(int fd, short events, void *arg);
static void sqlchild_check_queue();
int sqlchild_request(dbref thing, int attr, char slot, char *pres, char *query, char *rdelim, char *cdelim);

void sqlchild_init() {
    dbi_driver *driver;
   
    if(dbi_initialized) return;

    dprintk("initializing sqlchild.");
    
    if(dbi_initialize(NULL) == -1) {
        dprintk("dbi_initialized() failed.");
        dbi_state = DBIS_EFAIL;
        return;
    }

    dprintk("libdbi started.");
    
    driver = dbi_driver_list(NULL);
    while(driver != NULL) {
        dprintk("libdbi driver '%s' ready.", dbi_driver_get_name(driver));
        driver = dbi_driver_list(driver);
    }
    
    dbi_state = DBIS_READY;
}

void sqlchild_destruct() {
    dprintk("shutting down.");
    dbi_state = DBIS_EFAIL;
    sqlchild_kill_all();
    dbi_shutdown();
}

/* do_query entrypoint */
int sqlchild_request(dbref thing, int attr, char slot, char *pres, char *query, char *rdelim, char *cdelim) {
    int fds[2];
    struct query_state_t *aqt;

    if(dbi_state < DBIS_READY) return -1;
    
    aqt = malloc(sizeof(struct query_state_t));
    aqt->thing = thing;
    aqt->attr = attr;
    aqt->preserve = strdup(pres);
    aqt->query = strdup(query);
    aqt->rdelim = strdup(rdelim);
    aqt->cdelim = strdup(cdelim);
    aqt->serial = query_counter++;
    aqt->slot = slot;

    if(pending == NULL) {
        aqt->next = NULL;
        pending = aqt;
        pending_tail = aqt;
    } else {
        pending_tail->next = aqt;
        aqt->next = NULL;
        pending_tail = aqt;
    }
    sqlchild_check_queue();
    return 1;
}

void sqlchild_kill_all() {
    dprintk("shutting down.\n");
    if(!dbi_initialized) return;
    while(running) {
        sqlchild_kill_query(running);
    }
}

int sqlchild_kill(int requestId) {
    int status;
    struct query_state_t *iter=NULL, *aqt=NULL;

    dprintk("received request to terminate %d", requestId);
    
    if(running) 
        iter = running;
        while(iter)
            if(iter->serial == requestId) {
                sqlchild_kill_query(iter);
                return 1;
            } else iter = iter->next;
    if(pending)
        iter = pending;
        while(iter)
            if(iter->serial == requestId) {
                sqlchild_kill_query(iter);
                return 1;
            } else iter = iter->next;
    return 0;
}

static void sqlchild_kill_query(struct query_state_t *aqt) {
    struct query_state_t *iter;
    
    dprintk("terminating query %d", aqt->serial);

    kill(aqt->pid, SIGTERM);
    waitpid(aqt->pid, NULL, 0);
    
    iter = running;
    if(running == aqt) {
        running = aqt->next;
    } else {
        while(iter) {
            if(iter->next == aqt) {
                iter->next = aqt->next;
                break;
            }
            iter = iter->next;
        }
    }
    if(aqt->preserve) free(aqt->preserve);
    if(aqt->query) free(aqt->query);
    if(aqt->rdelim) free(aqt->rdelim);
    if(aqt->cdelim) free(aqt->cdelim);
    close(aqt->fd);
    free(aqt);
    running_queries--;
    sqlchild_check_queue();
};

void sqlchild_list(dbref thing) {
    int nactive = 0, npending = 0;
    int nrecent = recent;
    struct query_state_t *aqt;
    notify(thing, "/--------------------------- Recent Queries");
    if(recent) {
        aqt = recent_head;
        while(aqt) {
            notify_printf(thing, "%08d #%8d %40s", aqt->serial, aqt->thing, aqt->query);
            aqt = aqt->next;
        }
    }
    notify(thing, "/--------------------------- Running Queries");
    if(running) {
        aqt = running;
        while(aqt) {
            notify_printf(thing, "%08d #%8d %40s", aqt->serial, aqt->thing, aqt->query);
            aqt = aqt->next;
            nactive++;
        }
    } else {
        notify(thing, "- No active queries.");
    }
    notify(thing, "/--------------------------- Pending Queries");
    if(pending) {
        aqt = pending;
        while(aqt) {
            notify_printf(thing, "%08d #%-8d %40s", aqt->serial, aqt->thing, aqt->query);
            aqt = aqt->next;
            npending++;
        }
    } else {
        notify(thing, "- No pending queries.");
    }
    notify_printf(thing, "%d active and %d pending queries.", nactive, pending);
}
        
    
struct query_response {
    int status;
    int n_chars;
};

/* HOST FUNCTIONS */

static void sqlchild_finish_query(int fd, short events, void *arg) {
    char *argv[5];
    struct query_state_t *aqt = (struct query_state_t *)arg, *iter;
    struct query_response resp = { -1, 0 };
    char buffer[LBUF_SIZE];
    buffer[0] = '\0';

    dprintk("receiving response for query %d", aqt->serial);

    if(read(aqt->fd, &resp, sizeof(struct query_response)) < 0) {
        log_perror("SQL", "FAIL", NULL, "sqlchild_finish_query");
        argv[0] = "-1";
        argv[1] = "";
        argv[3] = "serious braindamage";
        goto fail;

    }

    if(resp.n_chars >= LBUF_SIZE) {
        resp.n_chars = LBUF_SIZE - 1;
    }

    if(resp.n_chars) {
        if(read(aqt->fd, buffer, resp.n_chars) < 0) {
            log_perror("SQL", "FAIL", NULL, "sqlchild_finish_query");
            argv[0] = "-1";
            argv[1] = "";
            argv[3] = "serious braindamage";
            goto fail;
        }
    }
    
    if(resp.status == 0) {
        argv[0] = "1";
        argv[1] = buffer;
        argv[3] = "Success";
    } else {
        argv[0] = "0";
        argv[1] = "";
        if(resp.n_chars) {
            argv[3] = buffer;
        } else {
            argv[3] = "minor braindamage, no error and no result reported.";
        }
    }
    
fail:
    argv[2] = aqt->preserve;
    did_it(GOD, aqt->thing, 0, NULL, 0, NULL, aqt->attr, argv, 4);

hardfail:
    if(running == aqt) {
        running = aqt->next;
    } else {
        iter = running;
        while(iter) {
            if(iter->next == aqt) {
                iter->next = aqt->next;
                break;
            }
            iter = iter->next;
        }
    }
    
    close(aqt->fd);
    recent++;
    if(recent_tail == NULL) {
        aqt->next = NULL;
        recent_head = recent_tail = aqt;
    } else {
        recent_tail->next = aqt;
        recent_tail = aqt;
    }
    
    if(recent > 20) {
        aqt = recent_head;
        recent_head = aqt->next;
        if(aqt->preserve) {
            free(aqt->preserve);
            aqt->preserve = NULL;
        }
        if(aqt->query) {
            free(aqt->query);
            aqt->query = NULL;
        }
        if(aqt->rdelim) {
            free(aqt->rdelim);
            aqt->rdelim = NULL;
        }
        if(aqt->cdelim) {
            free(aqt->cdelim);
            aqt->cdelim = NULL;
        }
        free(aqt);
        aqt = NULL;
        recent--;
    }
    running_queries--;
    sqlchild_check_queue();
    return;
}

static void sqlchild_check_queue() {
    int fds[2];
    struct query_state_t *aqt;
    if(running_queries >= mudconf.sqlDB_max_queries) return;
    if(pending == NULL) return;
    if(dbi_state != DBIS_READY) return;

    aqt = pending;
    pending = aqt->next;
    if(pending == NULL) pending_tail = NULL;
    
    if(pipe(fds) < 0) {
        log_perror("SQL", "FAIL", NULL, "pipe");
        return;
    }

    if((aqt->pid=fork()) == 0) {
        aqt->fd = fds[1];
        sqlchild_child_execute_query(aqt);
        exit(0);
    } else {
        running_queries++;
        aqt->fd = fds[0];
        close(fds[1]);
    }
    dprintk("waiting on sqlchild pid %d executing request %d", aqt->pid, aqt->serial);

    if(running) aqt->next = running;
    running = aqt;

    event_set(&aqt->ev, aqt->fd, EV_READ, sqlchild_finish_query, aqt);
    event_add(&aqt->ev, &query_timeout);
    return;
}

/* CHILD FUNCTIONS */

static void sqlchild_child_abort_query(struct query_state_t *aqt, char *error) {
    struct query_response resp = { DBIS_EFAIL, 0 };
    if(error) {
        resp.n_chars = strlen(error)+1;
        write(aqt->fd, &resp, sizeof(resp));
        write(aqt->fd, error, resp.n_chars);
    } else {
        write(aqt->fd, &resp, sizeof(resp));
    }
    close(aqt->fd);
    return;
}

static void sqlchild_child_abort_query_dbi(struct query_state_t *aqt, char *error) {
    char *error_ptr;
    if(dbi_conn_error(conn, (const char **)&error_ptr) != -1) 
        sqlchild_child_abort_query(aqt, error_ptr);
    else 
        sqlchild_child_abort_query(aqt, error);
    return;
}

static void sqlchild_make_connection(char db_slot) {
    char *db_type, *db_hostname, *db_username, *db_password, *db_database;
    switch(db_slot){
        case 'A':
            db_type = mudconf.sqlDB_type_A;
            db_hostname = mudconf.sqlDB_hostname_A;
            db_username = mudconf.sqlDB_username_A;
            db_password = mudconf.sqlDB_password_A;
            db_database = mudconf.sqlDB_dbname_A;
            break;
        case 'B':
            db_type = mudconf.sqlDB_type_B;
            db_hostname = mudconf.sqlDB_hostname_B;
            db_username = mudconf.sqlDB_username_B;
            db_password = mudconf.sqlDB_password_B;
            db_database = mudconf.sqlDB_dbname_B;
            break;
        case 'C':
            db_type = mudconf.sqlDB_type_C;
            db_hostname = mudconf.sqlDB_hostname_C;
            db_username = mudconf.sqlDB_username_C;
            db_password = mudconf.sqlDB_password_C;
            db_database = mudconf.sqlDB_dbname_C;
            break;
        case 'D':
            db_type = mudconf.sqlDB_type_D;
            db_hostname = mudconf.sqlDB_hostname_D;
            db_username = mudconf.sqlDB_username_D;
            db_password = mudconf.sqlDB_password_D;
            db_database = mudconf.sqlDB_dbname_D;
            break;
        case 'E':
            db_type = mudconf.sqlDB_type_E;
            db_hostname = mudconf.sqlDB_hostname_E;
            db_username = mudconf.sqlDB_username_E;
            db_password = mudconf.sqlDB_password_E;
            db_database = mudconf.sqlDB_dbname_E;
            break;
        default:
            return;
    }
    conn = dbi_conn_new(db_type);
    if(!conn) {
        dprintk("dbi_conn_new() failed with db_type %s.", db_type);
        dbi_state = DBIS_EFAIL;
        return;
    }
    if(strncmp(db_type, "mysql", 128)==0 && strnlen(mudconf.sqlDB_mysql_socket, 128) > 0 &&
            dbi_conn_set_option(conn, "mysql_unix_socket", mudconf.sqlDB_mysql_socket)) {
        dprintk("failed to set mysql_unix_socket");
        dbi_state = DBIS_EFAIL;
        return;
    }
    if(strncmp(db_type, "sqlite", 128)==0 && strnlen(mudconf.sqlDB_sqlite_dbdir, 128) > 0 &&
            dbi_conn_set_option(conn, "sqlite_dbdir", mudconf.sqlDB_sqlite_dbdir)) {
        dprintk("failed to set sqlite dir.");
        dbi_state = DBIS_EFAIL;
        return;
    }
    if(db_hostname && dbi_conn_set_option(conn, "host", db_hostname)) {
        dprintk("failed to set hostname");
        dbi_state = DBIS_EFAIL;
        return;
    }
    if(db_username && dbi_conn_set_option(conn, "username", db_username)) {
        dprintk("failed to set username");
        dbi_state = DBIS_EFAIL;
        return;
    }
    if(db_password && dbi_conn_set_option(conn, "password", db_password)) {
        dprintk("failed to set password");
        dbi_state = DBIS_EFAIL;
        return;
    } 
    if(db_database && dbi_conn_set_option(conn, "dbname", db_database)) {
        dprintk("failed to set database");
        dbi_state = DBIS_EFAIL;
        return;
    }
    return;
}

static char *sqlchild_sanitize_string(char *input, int length) {
    char *retval = malloc(length+1);
    int i = 0;
    memset(retval, 0, length+1);
    for(i = 0; i < length && input[i]; i++) {
        if(isprint(input[i])) {
            retval[i] = input[i];
        } else {
            retval[i] = ' ';
        }
    }
    free(input);
    return retval;
}
 
static void sqlchild_child_execute_query(struct query_state_t *aqt) {
    struct query_response resp = { DBIS_READY, -1 };
    dbi_result result;
    int rows, fields, i, ii, retval;
    char output_buffer[LBUF_SIZE], *ptr, *eptr, *delim;
    int binary_length;
    char time_buffer[64];
    int length = 0;

    long long type_int;
    double type_fp;
    const char *type_string;
    time_t type_time;
    
    ptr = output_buffer;
    eptr = ptr + LBUF_SIZE;
    *ptr = '\0';
    
    dprintk("executing query %d.", aqt->serial);

    sqlchild_make_connection(aqt->slot);
    if(!conn) {
        sqlchild_child_abort_query(aqt, "failed to create connection");
        return;
    }
    if(dbi_state!=DBIS_READY) {
        sqlchild_child_abort_query_dbi(aqt, "unknown error in sqlchild_make_connection");
        return;
    }

     if(!conn) {
        sqlchild_child_abort_query_dbi(aqt, "unknown error in sqlchild_make_connection");
        return;
    }

    if(dbi_conn_connect(conn) != 0) {
        sqlchild_child_abort_query(aqt, "dbi_conn_connect failed");
        return;
    }

    result = dbi_conn_query(conn, aqt->query);
    if(result == NULL) {
        sqlchild_child_abort_query_dbi(aqt, "unknown error in dbi_conn_query");
        return;
    }

    rows = dbi_result_get_numrows(result);
    fields = dbi_result_get_numfields(result);

    delim = NULL;
    
    while(dbi_result_next_row(result)) {
        if(delim != NULL) {
            ptr += snprintf(ptr, eptr-ptr, aqt->rdelim);
        }
        for(i = 1; i <= fields; i++) {
            if(fields == i) delim = "";
            else delim = aqt->cdelim;
            // XXX: handle error values form snprintf()
            switch(dbi_result_get_field_type_idx(result, i)) {
                case DBI_TYPE_INTEGER:
                    type_int = dbi_result_get_longlong_idx(result, i);
                    ptr += snprintf(ptr, eptr-ptr, "%lld%s", type_int, delim);
                    break;
                case DBI_TYPE_DECIMAL:
                    type_fp = dbi_result_get_double_idx(result, i);
                    ptr += snprintf(ptr, eptr-ptr, "%f%s", type_fp, delim);
                    break;
                case DBI_TYPE_STRING:
                    
                    type_string = dbi_result_get_string_idx(result, i);
                    ptr += snprintf(ptr, eptr-ptr, "%s%s", type_string, delim);
                    break;
                case DBI_TYPE_BINARY:
                    binary_length = dbi_result_get_field_length_idx(result, i);
                    if(binary_length) {
                        type_string = sqlchild_sanitize_string(
                                dbi_result_get_binary_copy_idx(result, i), 
                                binary_length);
                        ptr += snprintf(ptr, eptr-ptr, "%s%s", type_string, delim);
                        free(type_string);
                    } else {
                        ptr += snprintf(ptr, eptr-ptr, "%s", delim);
                    }
                    break;
                case DBI_TYPE_DATETIME:
                    // HANDLE TIMEZONE
                    type_time = dbi_result_get_datetime_idx(result, i);
                    ctime_r(&type_time, time_buffer);
                    ptr += snprintf(ptr, eptr-ptr, "%s%s", time_buffer, delim);
                    break;
                default:
                    sqlchild_child_abort_query(aqt, "unknown type");
                    return;
            }
            if(eptr-ptr < 1) {
                sqlchild_child_abort_query(aqt, "result too large");
                return;
            }
        }
    }
    *ptr++ = '\0';
    resp.n_chars = eptr-ptr;
    eptr = ptr;
    // XXX: handle failure
    write(aqt->fd, &resp, sizeof(struct query_response));
    ptr = output_buffer;
    while(ptr < eptr) {
        retval = write(aqt->fd, ptr, eptr-ptr);
        ptr+=retval;
    }
    close(aqt->fd);
    return;
}

#endif /* SQL_SUPPORT */
