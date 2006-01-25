/* cque.h */
/* $Id: */

#ifndef __CQUE_H__
#define __CQUE_H__
#include "config.h"

/* BQUE - Command queue */

typedef struct bque BQUE;
struct bque {
    BQUE *next;
    
    dbref player;       /* player who will do command */
    dbref cause;        /* player causing command (for %N) */
    dbref sem;          /* blocking semaphore */
    int waittime;       /* time to run command */
    int attr;           /* blocking attribute */
    char *text;         /* buffer for comm, env, and scr text */
    char *comm;         /* command */
    char *env[NUM_ENV_VARS];    /* environment vars */
    char *scr[NUM_ENV_VARS];    /* temp vars */
    int nargs;          /* How many args I have */
    struct event ev;   /* event structure for wait queue */
};

/* Per object run queues */
typedef struct objqe OBJQE;

struct objqe {
    dbref obj;
    BQUE *cque;
    BQUE *ctail;
    BQUE *wait_que; // commands waiting on this object
    BQUE *pending_que; // obj's commands that are waiting
    struct objqe *next;
    int queued;
};  

#endif
