/*
 * sqlchild.c
 *
 * Copyright (c) 2004,2005 Martin Murray <mmurray@monkey.org>
 * All rights reserved.
 *
 * $Id: rbtree.h 63 2005-08-24 20:59:08Z murrayma $
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
#include "debug.h"

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

#define MAX_QUERIES 8

static int active_queries = 0;
static char *db_username = NULL;
static char *db_password = NULL;
static char *db_database = NULL;
static char *db_hostname = NULL;
static char *db_type = NULL;

static dbi_conn conn;

enum dbi_state_e = { DBIS_EFAIL=-1, DBIS_READY = 0, DBIS_RESOURCE = 1 } dbi_state;

struct query_state_t {
    dbref thing;
    int attr;
    char *pres;
    char *query;
    struct event ev;
    char *rdel;
    char *cdel;
    struct query_state_t *next;
    int fd;
    int pid;
} *running = NULL, *pending = NULL, *pending_tail = NULL;

timeval query_timeout = { 10, 0 };

void init_sql(char *username, char *password, char *database, char *hostname, 
        char *type) {
    if(username) db_username = strdup(username);
    if(password) db_password = strdup(password);
    if(datbaswe) db_database = strdup(database);
    if(hostname) db_hostname = strdup(hostname);
    if(!type) {
        EMIT_STDERR("no database type supplied, failing.");
        dbi_state = DBIS_EFAIL;
    } else db_type = strdup(type);

    if(dbi_initialize(NULL) == -1) {
        EMIT_STDERR("dbi_initialized() failed.");
        dbi_state = DBIS_EFAIL;
        return;
    }
    
    conn = dbi_conn_new(db_type);
    if(!conn) {
        EMIT_STDERR("dbi_conn_new() failed.");
        dbi_state = DBIS_EFAIL;
        return;
    }

    if(!dbi_conn_set_option(conn, "host", db_hostname)) {
        EMIT_STDERR("failed to set hostname");
        dbi_state = DBIS_EFAIL;
        return;
    }
    if(!dbi_conn_set_option(conn, "username", db_username)) {
        EMIT_STDERR("failed to set username");
        dbi_state = DBIS_EFAIL;
        return;
    }
    if(!dbi_conn_set_option(conn, "password", db_password)) {
        EMIT_STDERR("failed to set password");
        dbi_state = DBIS_EFAIL;
        return;
    } 
    if(!dbi_conn_set_option(conn, "dbname", db_database)) {
        EMIT_STDERR("failed to set database");
        dbi_state = DBIS_EFAIL;
        return;
    }
    dbi_state = DBIS_READY;
}

struct query_response {
    int status;
    int n_chars;
};

void abort_query(aqt, char *error) {
    struct query_response resp = { DBIS_EFAIL, 0 };
    if(error) {
        EMIT_STDERR(error);
        resp.n_chars = strlen(error);
        write(aqt->fd, &resp, sizeof(resp));
        write(aqt->fd, error, resp.n_chars);
    } else {
        write(aqt->fd, &resp, sizeof(resp));
    }
    close(aqt->fd);
    return;
}

void abort_query_dbi(aqt, char *error) {
    char *error_ptr;
    if(dbi_conn_error(conn, &error_ptr) != -1) {
        abort_query(aqt, error_ptr);
    else 
        abort_query(aqt, error);
    return;
}

#define OUTLEN 8192

void execute_sql(struct query_state_t *aqt) {
    struct query_response resp = { DBIS_READY, -1 };
    dbi_result result;
    int rows, fields, i, ii, retval;
    char output_buffer[OUTLEN], *ptr, *eptr, *delim;
    char time_buffer[64];
    int length = 0;

    long long type_int;
    double type_fp;
    char *type_string;
    time_t type_time;
    
    ptr = output_buffer;
    eptr = ptr + OUTLEN;
    *ptr = '\0';
    
    if(dbi_conn_connect(conn) != 0) {
        abort_query_dbi(aqt, "dbi_conn_connect failed");
        return;
    }
    
    result = dbi_conn_query(conn, sqlquery);
    if(result == NULL) {
        abort_query_dbi(aqt, "unknown error in dbi_conn_query");
        return;
    }

    rows = dbi_result_get_numrows(result);
    fields = dbi_result_get_numfields(result);

    if(!rows || !fields) {
        abort_query_dbi(aqt, "unknown error");
    }
        
    while(dbi_result_next_row(result)) {
        for(i = 1; i <= fields; i++) {
            if(fields == i) delim = aqt->rdelim;
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
                    abort_conn(aqt, "can't handle type BINARY");
                    return;
                case DBI_TYPE_DATETIME:
                    // HANDLE TIMEZONE
                    type_time = dbi_result_get_datetime_idx(dbiresult, i);
                    ctime_r(type_time, time_buffer);
                    ptr += snprintf(ptr, eptr-ptr, "%s%s", time_buffer, delim);
                    break;
                default:
                    abort_conn(aqt, "unknown type");
                    return;
            }
            if(eptr-ptr < 1) {
                abort_conn(aqt, "result too large");
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
        IF_FAIL_ERRNO(retval = write(aqt->fd, ptr, eptr-ptr));
        ptr+=retval;
    }
    close(aqt->fd);
    return;
}

void accept_query_result(int fd, short events, void *arg) {
    char argv[3];
    struct query_state_t *aqt = (struct query_state_t *)arg, iter;
    struct query_response resp = { -1, 0 };
    char buffer[OUTLEN];
    char confirm[4096];
    buffer[0] = '\0';

    if(read(aqt->fd, &resp, sizeof(struct query_response)) < 0) {
        log_perror("SQL", "FAIL", NULL, "accept_query_result");
        goto fail;

    }

    if(resp.n_chars) {
        if(read(aqt->fd, buffer, resp.n_chars) < 0) {
            log_perror("SQL", "FAIL", NULL, "accept_query_result");
            goto fail;
        }
    }

    if(resp.status == 0) {
        strncpy(confirm, 4096, "{ Success }");
    } else {
        if(resp.n_chars) {
            snprintf(confirm, 4096, "{ #-1 %s }", buffer);
            buffer[0] = '\0';
            resp.n_chars = 0;
        } else {
            snprintf(confirm, 4096, "{ #-1 Serious Braindamage, Unknown Error }");
        }
    }
    argv[0] = confirm;
    argv[1] = buffer;
    argv[2] = aqt->preserve;
    did_it(GOD, thing, 0, NULL, 0, NULL, attr, argv, 3);
fail:
    iter = active;
    if(active == aqt) {
        active = aqt->next;
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
    if(aqt->rdelim) free(aqt->rdelim);
    if(aqt->cdelim) free(aqt->cdelim);
    close(aqt->fd);
    free(aqt);
    active_queries--;
    return;
}

int queue_request(dbref thing, int attr, char *pres, char *query) {
    int fds[2];
    struct query_state_t *aqt;

    if(dbi_state < DBI_READY) return;
    
    aqt = malloc(sizeof(struct query_state_t));
    aqt->thing = thing;
    aqt->attr = attr;
    aqt->pres = strdup(pres);
    aqt->query = strdup(query);

    if(pending == NULL) {
        aqt->next = NULL;
        pending = aqt;
        pending_tail = aqt;
    } else {
        pending_tail->next = aqt;
        aqt->next = NULL;
        pending_tail = aqt;
    }
    issue_sql_request();
    return;

void issue_sql_request() {
    int fds[2];
    struct query_state_t *aqt;
    if(active_queries == MAX_QUERIES) return;
    if(pending == NULL) return;

    aqt = pending;
    pending = aqt->next;
    if(pending == NULL) pending_tail = NULL;
    
    if(pipe(fds) < 0) {
        log_perror("SQL", "FAIL", NULL, "pipe");
        return 0;
    }

    if((aqt->pid=fork()) == 0) {
        aqt->fd = fds[1];
        execute_query(aqt);
        exit(0);
    } else {
        active_queries++;
        aqt->fd = fds[0];
    }

    aqt->next = active->next;
    active = aqt;

    event_set(&aqt->ev, aqt->fd, EV_READ, accept_query_result, aqt);
    event_add(&aqt->ev, &query_timeout);
    STDERR_EMIT("started sql slave, pid = %d and return fd = %d\n", 
            aqt->pid, aqt->fd);
    return;
}

