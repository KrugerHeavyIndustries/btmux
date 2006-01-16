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
#include "debug.h"

static struct dns_query_state_t {
    DESC *desc;
    struct event ev;
    struct dns_query_state_t *next;
    int pid;
} *running = NULL;

static int running_queries = 0;

static struct timeval query_timeout = { 60, 0 };

void dnschild_finish(int fd, short event, void *arg);

int dnschild_request(DESC *d) {
    int fds[2];
    struct dns_query_state_t *dqst;
    char address[256];
    char outbuffer[1024];
    int length;

    dqst = malloc(sizeof(struct dns_query_state_t));
    memset(dqst, 0, sizeof(struct dns_query_state_t));

    dqst->desc = d;
    
    if(pipe(fds) < 0) {
        log_perror("DNS", "ERR", NULL, "pipe");
    }

    event_set(&dqst->ev, fds[0], EV_READ, dnschild_finish, dqst);
    event_add(&dqst->ev, &query_timeout);

    dqst->pid = fork();
    if(dqst->pid == 0) {
        /* begin child section */
        unbind_signals();
        if(getnameinfo((struct sockaddr *)&d->address, d->saddr_len, 
                    address, 1023, NULL, 0, NI_NAMEREQD)) {
            length = snprintf(outbuffer, 255, "0%s", strerror(errno));
            outbuffer[length++] = '\0';
            write(fds[1], outbuffer, length);
        } else {
            length = snprintf(outbuffer, 255, "1%s", address);
            outbuffer[length++] = '\0';
            write(fds[1], outbuffer, length);
        }
        close(fds[1]);
        exit(0);
        /* end child section */
    }

   dqst->next = running; 
   running = dqst;
   running_queries++; 
   return 1;
}

void dnschild_finish(int fd, short event, void *arg) {
    char buffer[2048];
    struct dns_query_state_t *dqst = (struct dns_query_state_t *)arg, *iter;
    
    iter = running;
    if(running == dqst) {
        running = dqst->next;
    } else {
        while(iter) {
            if(iter->next == dqst) {
                iter->next = dqst->next;
                break;
            }   
            iter = iter->next;
        }                                      
    }
   
    if(event & EV_TIMEOUT) {
        kill(dqst->pid, SIGTERM);
        log_perror("DNS", "ERR", NULL, "dnschild failed to finish.");
        close(fd);
        free(dqst);
        return;
    }

    read(fd, buffer, 2048);

    if(buffer[0] == '0') {
        log_perror("DNS", "ERR", NULL, "dnschild failed, reported error follows.");
        log_perror("DNS", "ERR", NULL, buffer+1);
        close(fd);
        free(dqst);
        return;
    }

    strncpy(dqst->desc->addr, buffer+1, sizeof(dqst->desc->addr)-1);
    dprintk("dnschild resolved %s correctly.", buffer+1);
    close(fd);
    free(dqst);
    return;
}
