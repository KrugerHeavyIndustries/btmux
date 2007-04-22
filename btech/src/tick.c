/*
 *
 * Copyright (c) 2005 Martin Murray
 *
 * This is much better than what we had.
 *
 * FIXME: BT/Penn doesn't use libevent.  We're using ugly #ifdef's to deal with
 * this reality right now, but eventually we should either move off libevent in
 * the BT code, write a wrapper, or kick all this code out into the BTPR layer.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#ifndef BTPR_PENN
#include <event.h>
#endif /* !BTPR_PENN */
#include "glue_types.h"
#include "mux_tree.h"
#include "mech.h"
#include "autopilot.h"
#include "debug.h"

#ifndef BTPR_PENN
static struct event heartbeat_ev;
#endif /* !BTPR_PENN */
static struct timeval heartbeat_tv = { 1, 0 };
static int heartbeat_running = 0;
unsigned int global_tick = 0;
extern Tree xcode_tree;

void GoThruTree(Tree tree, int (*func) (Node *));
void heartbeat_run(int fd, short event, void *arg);

void heartbeat_init() {
    if(heartbeat_running) return;
    dprintk("hearbeat initialized, %ds timeout.", (int)heartbeat_tv.tv_sec);
#ifndef BTPR_PENN
    evtimer_set(&heartbeat_ev, heartbeat_run, NULL);
    evtimer_add(&heartbeat_ev, &heartbeat_tv);
#endif /* !BTPR_PENN */
    heartbeat_running = 1;
}

void heartbeat_stop() {
    if(!heartbeat_running) return;
#ifndef BTPR_PENN
    evtimer_del(&heartbeat_ev);
#endif /* !BTPR_PENN */
    dprintk("heartbeat stopped.\n");
    heartbeat_running = 0;
}

void mech_heartbeat(MECH *);
void auto_heartbeat(AUTO *);

int heartbeat_dispatch(Node *node) {
    switch(NodeType(node)) {
        case GTYPE_MECH:
            mech_heartbeat((MECH *)NodeData(node));
            break;
        case GTYPE_AUTO:
            auto_heartbeat((AUTO *)NodeData(node));
            break;
    }
    return 1;
}

void heartbeat_run(int fd, short event, void *arg) {
#ifndef BTPR_PENN
    evtimer_add(&heartbeat_ev, &heartbeat_tv);
#endif /* !BTPR_PENN */
    GoThruTree(xcode_tree, heartbeat_dispatch);
    global_tick++;
}

