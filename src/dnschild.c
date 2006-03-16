/*
 * dnschild.c
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
	int fd;
} *running = NULL;

static int running_queries = 0;
static int dnschild_state = 0;

static struct timeval query_timeout = { 60, 0 };

static void dnschild_finish(int fd, short event, void *arg);

int dnschild_init()
{
	dprintk("dnschild initialized.");
	dnschild_state = 1;
	return 1;
}

void *dnschild_request(DESC * d)
{
	int fds[2];
	struct dns_query_state_t *dqst;
	char address[256];
	char outbuffer[1024];
	int length;

	if(!dnschild_state) {
		dprintk("bailing query, dnschild is shut down.");
		return 0;
	}

	dqst = malloc(sizeof(struct dns_query_state_t));
	memset(dqst, 0, sizeof(struct dns_query_state_t));

	dqst->desc = d;

	if(pipe(fds) < 0) {
		log_perror("DNS", "ERR", NULL, "pipe");
	}

	event_set(&dqst->ev, fds[0], EV_TIMEOUT | EV_READ, dnschild_finish, dqst);
	event_add(&dqst->ev, &query_timeout);

	dqst->pid = fork();
	if(dqst->pid == 0) {
        close(fds[0]);
		/* begin child section */
		unbind_signals();
        memset(address, 0, 1023);
		if(getnameinfo((struct sockaddr *) &d->saddr, d->saddr_len,
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
	dqst->fd = fds[0];
    close(fds[1]);
	dqst->next = running;
	running = dqst;
	running_queries++;
	return (void *) dqst;
}

void dnschild_destruct()
{
	struct dns_query_state_t *dqst;
	dprintk("dnschild expiring queries and shutting down.");
	dnschild_state = 0;
	while (running) {
		dqst = running;
		dprintk("dnschild query for %s aborting early.", dqst->desc->addr);
		if(event_pending(&dqst->ev, EV_READ, NULL))
			event_del(&dqst->ev);
		close(dqst->fd);
		dqst->desc->outstanding_dnschild_query = NULL;
		running = running->next;
		free(dqst);
	}
}

void dnschild_kill(void *arg)
{
	struct dns_query_state_t *dqst = (struct dns_query_state_t *) arg, *iter;

	dprintk("dnschild query for %s aborting early.", dqst->desc->addr);

	iter = running;
	if(running == dqst) {
		running = dqst->next;
	} else {
		while (iter) {
			if(iter->next == dqst) {
				iter->next = dqst->next;
				break;
			}
			iter = iter->next;
		}
	}

	if(event_pending(&dqst->ev, EV_READ, NULL))
		event_del(&dqst->ev);
	dqst->desc->outstanding_dnschild_query = NULL;
	close(dqst->fd);
	free(dqst);
}

static void dnschild_finish(int fd, short event, void *arg)
{
	char buffer[2048];
	struct dns_query_state_t *dqst = (struct dns_query_state_t *) arg, *iter;

	iter = running;
	if(running == dqst) {
		running = dqst->next;
	} else {
		while (iter) {
			if(iter->next == dqst) {
				iter->next = dqst->next;
				break;
			}
			iter = iter->next;
		}
	}

	dqst->desc->outstanding_dnschild_query = NULL;

	if(event & EV_TIMEOUT || !dnschild_state) {
		kill(dqst->pid, SIGTERM);
		log_perror("DNS", "ERR", NULL, "dnschild failed to finish.");
		close(fd);
		free(dqst);
		return;
	}

	read(fd, buffer, 2048);

	if(buffer[0] == '0') {
		dprintk("dnschild failed with error: %s", buffer + 1);
		close(fd);
		free(dqst);
		return;
	}

	strncpy(dqst->desc->addr, buffer + 1, sizeof(dqst->desc->addr) - 1);
	dprintk("dnschild resolved %s correctly.", buffer + 1);
	close(fd);
	free(dqst);
	return;
}
