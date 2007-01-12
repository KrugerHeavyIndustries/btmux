/*
 *
 * Copyright (c) 2005 Martin Murray
 *
 * This is much better than what we had.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <event.h>
#include "glue_types.h"
#include "mux_tree.h"
#include "mech.h"
#include "autopilot.h"
#include "debug.h"

static struct event heartbeat_ev;
static struct timeval heartbeat_tv = { 1, 0 };
static int heartbeat_running = 0;
unsigned int global_tick = 0;
extern Tree xcode_tree;
extern rbtree xcode_rbtree;

void GoThruTree(Tree tree, int (*func) (Node *));
void heartbeat_run(int fd, short event, void *arg);

void heartbeat_init() {
    if(heartbeat_running) return;
    dprintk("hearbeat initialized, %ds timeout.", (int)heartbeat_tv.tv_sec);
    evtimer_set(&heartbeat_ev, heartbeat_run, NULL);
    evtimer_add(&heartbeat_ev, &heartbeat_tv);
    heartbeat_running = 1;
}

void heartbeat_stop() {
    if(!heartbeat_running) return;
    evtimer_del(&heartbeat_ev);
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

/*
 * New heartbeat function for the rbtree xcode tree
 */
int new_heartbeat_dispatch(void *key, void *data, int depth, void *arg) {

    XcodeObject *xcode_object = (XcodeObject *) data;
    MECH *mech;

    /* Might want to add in debug code incase it tries
     * to update the heartbeat of a bad obj */
    switch (xcode_object->type) {
        case GTYPE_MECH:
            if ((mech = getMech((int) key))) {
                new_mech_heartbeat(mech);
            }
            break;
    }

}

void heartbeat_run(int fd, short event, void *arg) {
    evtimer_add(&heartbeat_ev, &heartbeat_tv);
    GoThruTree(xcode_tree, heartbeat_dispatch);
    rb_walk(xcode_rbtree, WALK_INORDER, new_heartbeat_dispatch, NULL);
    global_tick++;
}

