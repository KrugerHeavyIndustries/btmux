/*
 * cque.c -- commands and functions for manipulating the command queue 
 */

#include "copyright.h"
#include "config.h"

#include <signal.h>

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "htab.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "attrs.h"
#include "flags.h"
#include "powers.h"
#include "command.h"
#include "alloc.h"
#include "functions.h"
#include "cque.h"

extern int a_Queue(dbref, int);
extern void s_Queue(dbref, int);
extern int QueueMax(dbref);

static rbtree obq = NULL;

static int objqe_compare(dbref left, dbref right, void *arg) {
    return (right-left);
}

int cque_init() {
    obq = rb_init((void *)objqe_compare, NULL);
    return 1;
};

static OBJQE *cque_find(dbref player) {
    OBJQE *tmp=NULL;

    if(obq == NULL) {
        cque_init();
    }

   
    tmp = rb_find(obq, (void *)player);

    if(!tmp && Good_obj(player)) {
        dprintk("allocating new queue for %d", player);
        tmp = malloc(sizeof(OBJQE));
        tmp->obj = player;
        tmp->cque = NULL;
        tmp->ctail = NULL;
        tmp->next = NULL;
        tmp->queued = 0;
        tmp->wait_que = NULL;
        tmp->pending_que = NULL;
        rb_insert(obq, (void *)player, tmp);
    }

    return tmp;
}

static BQUE *cque_peek(dbref player) {
    OBJQE *tmp;
    tmp = cque_find(player);
    return tmp->cque;
}
    
static BQUE *cque_deque(dbref player) {
    OBJQE *tmp;
    BQUE *cmd;

    tmp = cque_find(player);
    if(!tmp) return NULL;

    dassert(tmp, "brain damage");
    
    if(!tmp->cque) return NULL;

    cmd = tmp->cque;
    if(!cmd->next) {
        tmp->cque = tmp->ctail = NULL;
    } else {
        tmp->cque = cmd->next;
    }
    return cmd;
}

static void cque_enqueue(dbref player, BQUE *cmd) {
    OBJQE *tmp;

    cmd->next = NULL;
    cmd->waittime = 0;

    tmp = cque_find(player);

    dassert(tmp, "serious braindamage.");

    if(!tmp->ctail) {
        tmp->cque = tmp->ctail = cmd;
        cmd->next = NULL;
    } else {
        tmp->ctail->next = cmd;
        tmp->ctail = cmd;
        tmp->ctail->next = NULL;
    }

    if(!tmp->queued) {
        if(!mudstate.qhead) {
            mudstate.qhead = mudstate.qtail = tmp;
            tmp->next = NULL;
        } else {
            mudstate.qtail->next = tmp;
            mudstate.qtail = tmp;
            mudstate.qtail->next = NULL;
        }
        tmp->queued = 1;
    }
}

/*
 * ---------------------------------------------------------------------------
 * * add_to: Adjust an object's queue or semaphore count.
 */

static int add_to(dbref player, int am, int attrnum) {
    int num, aflags;
    dbref aowner;
    char buff[20];
    char *atr_gotten;

    num = atoi(atr_gotten = atr_get(player, attrnum, &aowner, &aflags));
    free_lbuf(atr_gotten);
    num += am;
    if (num)
        sprintf(buff, "%d", num);
    else
        *buff = '\0';
    atr_add_raw(player, attrnum, buff);
    return (num);
}

/*
 * ---------------------------------------------------------------------------
 * * que_want: Do we want this queue entry?
 */

static int que_want(BQUE *entry, dbref ptarg, dbref otarg) {
    if ((ptarg != NOTHING) && (ptarg != Owner(entry->player)))
        return 0;
    if ((otarg != NOTHING) && (otarg != entry->player))
        return 0;
    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * * halt_que: Remove all queued commands from a certain player
 */

int halt_que(dbref player, dbref object) {
    BQUE *trail, *point, *next;
    OBJQE *pque;
    
    int numhalted;

    numhalted = 0;

    /* Player's que */
    // XXX: nuke queu
    
    pque = cque_find(player);
    if(pque && pque->cque) {
        while((point = cque_deque(player)) != NULL) {
            free(point->text);
            point->text = NULL;
            free_qentry(point);
            point = NULL;
            numhalted++;
        }
    }
    pque = cque_find(object);
    if(pque && pque->cque) {
        while((point = cque_deque(object)) != NULL) {
            free(point->text);
            point->text = NULL;
            free_qentry(point);
            point = NULL;
            numhalted++;
        }
    }
    
    /*
     * Wait queue 
     */

    for (point = mudstate.qwait, trail = NULL; point; point = next)
        if (que_want(point, player, object)) {
            numhalted++;
            if (trail)
                trail->next = next = point->next;
            else
                mudstate.qwait = next = point->next;
            if(evtimer_pending(&point->ev, NULL))
                evtimer_del(&point->ev);
            free(point->text);
            free_qentry(point);
        } else
            next = (trail = point)->next;

    /*
     * Semaphore queue 
     */

    for (point = mudstate.qsemfirst, trail = NULL; point; point = next)
        if (que_want(point, player, object)) {
            numhalted++;
            if (trail)
                trail->next = next = point->next;
            else
                mudstate.qsemfirst = next = point->next;
            if (point == mudstate.qsemlast)
                mudstate.qsemlast = trail;
            add_to(point->sem, -1, point->attr);
            free(point->text);
            free_qentry(point);
        } else
            next = (trail = point)->next;

    if (player == NOTHING)
        player = Owner(object);
    giveto(player, (mudconf.waitcost * numhalted));
    if (object == NOTHING)
        s_Queue(player, 0);
    else
        a_Queue(player, -numhalted);
    return numhalted;
}

/*
 * ---------------------------------------------------------------------------
 * * do_halt: Command interface to halt_que.
 */

void do_halt(dbref player, dbref cause, int key, char *target) {
    dbref player_targ, obj_targ;
    int numhalted;

    if ((key & HALT_ALL) && !(Can_Halt(player))) {
        notify(player, "Permission denied.");
        return;
    }
    /*
     * Figure out what to halt 
     */

    if (!target || !*target) {
        obj_targ = NOTHING;
        if (key & HALT_ALL) {
            player_targ = NOTHING;
        } else {
            player_targ = Owner(player);
            if (Typeof(player) != TYPE_PLAYER)
                obj_targ = player;
        }
    } else {
        if (Can_Halt(player))
            obj_targ = match_thing(player, target);
        else
            obj_targ = match_controlled(player, target);

        if (obj_targ == NOTHING)
            return;
        if (key & HALT_ALL) {
            notify(player, "Can't specify a target and /all");
            return;
        }
        if (Typeof(obj_targ) == TYPE_PLAYER) {
            player_targ = obj_targ;
            obj_targ = NOTHING;
        } else {
            player_targ = NOTHING;
        }
    }

    numhalted = halt_que(player_targ, obj_targ);
    if (Quiet(player))
        return;
    if (numhalted == 1)
        notify(Owner(player), "1 queue entries removed.");
    else
        notify_printf(Owner(player), "%d queue entries removed.",
                    numhalted);
}

/*
 * ---------------------------------------------------------------------------
 * * nfy_que: Notify commands from the queue and perform or discard them.
 */

int nfy_que(dbref sem, int attr, int key, int count) {
    BQUE *point, *trail, *next;
    int num, aflags;
    dbref aowner;
    char *str;

    if (attr) {
        str = atr_get(sem, attr, &aowner, &aflags);
        num = atoi(str);
        free_lbuf(str);
    } else {
        num = 1;
    }

    if (num > 0) {
        num = 0;
        for (point = mudstate.qsemfirst, trail = NULL; point; point = next) {
            if ((point->sem == sem) && ((point->attr == attr) || !attr)) {
                num++;
                if (trail)
                    trail->next = next = point->next;
                else
                    mudstate.qsemfirst = next = point->next;
                if (point == mudstate.qsemlast)
                    mudstate.qsemlast = trail;

                /*
                 * Either run or discard the command 
                 */

                if (key != NFY_DRAIN) {
                    cque_enqueue(point->player, point);
                } else {
                    giveto(point->player, mudconf.waitcost);
                    a_Queue(Owner(point->player), -1);
                    free(point->text);
                    free_qentry(point);
                }
            } else {
                next = (trail = point)->next;
            }

            /*
             * If we've notified enough, exit 
             */

            if ((key == NFY_NFY) && (num >= count))
                next = NULL;
        }
    } else {
        num = 0;
    }

    /*
     * Update the sem waiters count 
     */

    if (key == NFY_NFY)
        add_to(sem, -count, attr);
    else
        atr_clr(sem, attr);

    return num;
}

/*
 * ---------------------------------------------------------------------------
 * * do_notify: Command interface to nfy_que
 */


void do_notify(dbref player, dbref cause, int key, char *what, char *count) {
    dbref thing, aowner;
    int loccount, attr = -1, aflags;
    ATTR *ap;
    char *obj;

    obj = parse_to(&what, '/', 0);
    init_match(player, obj, NOTYPE);
    match_everything(0);

    if ((thing = noisy_match_result()) < 0) {
        notify(player, "No match.");
    } else if (!controls(player, thing) && !Link_ok(thing)) {
        notify(player, "Permission denied.");
    } else {
        if (!what || !*what) {
            ap = NULL;
        } else {
            ap = atr_str(what);
        }

        if (!ap) {
            attr = A_SEMAPHORE;
        } else {
            /* Do they have permission to set this attribute? */
            atr_pget_info(thing, ap->number, &aowner, &aflags);
            if (Set_attr(player, thing, ap, aflags)) {
                attr = ap->number;
            } else {
                notify_quiet(player, "Permission denied.");
                return;
            }
        }

        if (count && *count)
            loccount = atoi(count);
        else
            loccount = 1;
        if (loccount > 0) {
            nfy_que(thing, attr, key, loccount);
            if (!(Quiet(player) || Quiet(thing))) {
                if (key == NFY_DRAIN)
                    notify_quiet(player, "Drained.");
                else
                    notify_quiet(player, "Notified.");
            }
        }
    }
}

static void wakeup_wait_que(int fd, short event, void *arg) {
    BQUE *pending = (BQUE *)arg;
    BQUE *point, trail;

    if(mudstate.qwait == pending) {
        mudstate.qwait = pending->next;
    } else {
        for (point = mudstate.qwait; point; point = point->next) {
            if(point->next == pending) {
                point->next = point->next->next;
                break;
            }
        }
    }

    cque_enqueue(pending->player, pending);
}


/*
 * ---------------------------------------------------------------------------
 * * setup_que: Set up a queue entry.
 */

static BQUE *setup_que(dbref player, dbref cause, char *command, char *args[], int nargs, char *sargs[]) {
    int a, tlen;
    BQUE *tmp;
    char *tptr;

    /*
     * Can we run commands at all? 
     */

    if (Halted(player))
        return NULL;

    /*
     * make sure player can afford to do it 
     */

    a = mudconf.waitcost;
    if (mudconf.machinecost && ((random() % mudconf.machinecost) == 0))
        a++;
    if (!payfor(player, a)) {
        notify(Owner(player), "Not enough money to queue command.");
        return NULL;
    }
    /*
     * Wizards and their objs may queue up to db_top+1 cmds. Players are
     * * * * * * * limited to QUEUE_QUOTA. -mnp 
     */

    a = QueueMax(Owner(player));
    if (a_Queue(Owner(player), 1) > a) {
        notify(Owner(player),
                "Run away objects: too many commands queued.  Halted.");
        halt_que(Owner(player), NOTHING);

        /*
         * halt also means no command execution allowed 
         */
        s_Halted(player);
        return NULL;
    }
    /*
     * We passed all the tests 
     */

    /*
     * Calculate the length of the save string 
     */

    tlen = 0;
    if (command)
        tlen = strlen(command) + 1;
    if (nargs > NUM_ENV_VARS)
        nargs = NUM_ENV_VARS;
    for (a = 0; a < nargs; a++) {
        if (args[a])
            tlen += (strlen(args[a]) + 1);
    }
    if (sargs) {
        for (a = 0; a < NUM_ENV_VARS; a++) {
            if (sargs[a])
                tlen += (strlen(sargs[a]) + 1);
        }
    }
    /*
     * Create the qeue entry and load the save string 
     */

    tmp = alloc_qentry("setup_que.qblock");
    tmp->comm = NULL;
    for (a = 0; a < NUM_ENV_VARS; a++) {
        tmp->env[a] = NULL;
    }
    for (a = 0; a < MAX_GLOBAL_REGS; a++) {
        tmp->scr[a] = NULL;
    }

    tptr = tmp->text = (char *) malloc(tlen);
    if (command) {
        StringCopy(tptr, command);
        tmp->comm = tptr;
        tptr += (strlen(command) + 1);
    }
    for (a = 0; a < nargs; a++) {
        if (args[a]) {
            StringCopy(tptr, args[a]);
            tmp->env[a] = tptr;
            tptr += (strlen(args[a]) + 1);
        }
    }
    if (sargs) {
        for (a = 0; a < MAX_GLOBAL_REGS; a++) {
            if (sargs[a]) {
                StringCopy(tptr, sargs[a]);
                tmp->scr[a] = tptr;
                tptr += (strlen(sargs[a]) + 1);
            }
        }
    }
    /*
     * Load the rest of the queue block 
     */

    evtimer_set(&tmp->ev, wakeup_wait_que, tmp);

    tmp->player = player;
    tmp->waittime = 0;
    tmp->next = NULL;
    tmp->sem = NOTHING;
    tmp->attr = 0;
    tmp->cause = cause;
    tmp->nargs = nargs;
    return tmp;
}

/*
 * ---------------------------------------------------------------------------
 * * wait_que: Add commands to the wait or semaphore queues.
 */

void wait_que(dbref player, dbref cause, int wait, dbref sem, int attr, char *command, 
        char *args[], int nargs, char *sargs[]) {
    BQUE *cmd, *point, *trail;
    OBJQE *current, *blocker;
    struct timeval tv;

    if (mudconf.control_flags & CF_INTERP)
        cmd = setup_que(player, cause, command, args, nargs, sargs);
    else
        cmd = NULL;

    if (cmd == NULL) {
        return;
    }

    if (wait != 0)
        cmd->waittime = time(NULL) + wait;

    tv.tv_sec = wait;
    tv.tv_usec = 0;

    cmd->sem = sem;
    cmd->attr = attr;

    if (cmd->sem == NOTHING) {
        /*
         * No semaphore, put on wait queue if wait value specified.
         * Otherwise put on the normal queue. 
         */

        if (wait <= 0) {
            cque_enqueue(cmd->player, cmd);
        } else {
            evtimer_add(&cmd->ev, &tv);
            for (point = mudstate.qwait, trail = NULL;
                    point && point->waittime <= cmd->waittime;
                    point = point->next) {
                trail = point;
            }
            cmd->next = point;
            if (trail != NULL)
                trail->next = cmd;
            else
                mudstate.qwait = cmd;
        }
    } else {
        if(wait <= 0) {
            dprintk("adding %d/%s to %d's wait queue.", cmd->player, cmd->comm, cmd->sem);
        }

        cmd->next = NULL;
        if (mudstate.qsemlast != NULL)
            mudstate.qsemlast->next = cmd;
        else
            mudstate.qsemfirst = cmd;
        mudstate.qsemlast = cmd;
    }
}

/*
 * ---------------------------------------------------------------------------
 * * do_wait: Command interface to wait_que
 */

void do_wait(dbref player, dbref cause, int key, char *event, char *cmd, char *cargs[], int ncargs) {
    dbref thing, aowner;
    int howlong, num, attr, aflags;
    char *what;
    ATTR *ap;


    /*
     * If arg1 is all numeric, do simple (non-sem) timed wait. 
     */

    if (is_number(event)) {
        howlong = atoi(event);
        wait_que(player, cause, howlong, NOTHING, 0, cmd, cargs, ncargs,
                mudstate.global_regs);
        return;
    }
    /*
     * Semaphore wait with optional timeout 
     */

    what = parse_to(&event, '/', 0);
    init_match(player, what, NOTYPE);
    match_everything(0);

    thing = noisy_match_result();
    if (!Good_obj(thing)) {
        notify(player, "No match.");
    } else if (!controls(player, thing) && !Link_ok(thing)) {
        notify(player, "Permission denied.");
    } else {

        /*
         * Get timeout, default 0 
         */

        if (event && *event && is_number(event)) {
            attr = A_SEMAPHORE;
            howlong = atoi(event);
        } else {
            attr = A_SEMAPHORE;
            howlong = 0;
        }

        if (event && *event && !is_number(event)) {
            ap = atr_str(event);
            if (!ap) {
                attr = mkattr(event);
                if (attr <= 0) {
                    notify_quiet(player, "Invalid attribute.");
                    return;
                }
                ap = atr_num(attr);
            }
            atr_pget_info(thing, ap->number, &aowner, &aflags);
            if (attr && Set_attr(player, thing, ap, aflags)) {
                attr = ap->number;
                howlong = 0;
            } else {
                notify_quiet(player, "Permission denied.");
                return;
            }
        }

        num = add_to(thing, 1, attr);
        if (num <= 0) {

            /*
             * thing over-notified, run the command immediately 
             */

            thing = NOTHING;
            howlong = 0;
        }
        wait_que(player, cause, howlong, thing, attr, cmd, cargs, ncargs,
                mudstate.global_regs);
    }
}

/*
 * ---------------------------------------------------------------------------
 * * do_second: Check the wait and semaphore queues for commands to remove.
 */

void do_second(void) {
    BQUE *trail, *point, *next;
    char *cmdsave;

    /*
     * move contents of low priority queue onto end of normal one this
     * helps to keep objects from getting out of control since
     * its affects on other objects happen only after one
     * second  this should allow @halt to be type before
     * getting blown away  by scrolling text 
     */

    if ((mudconf.control_flags & CF_DEQUEUE) == 0)
        return;

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "< do_second >";

    
    /*
     * Note: the point->waittime test would be 0 except the command is
     * being put in the low priority queue to be done in one
     * second anyways 
     */

    /*
     * Check the semaphore queue for expired timed-waits 
     */

    for (point = mudstate.qsemfirst, trail = NULL; point; point = next) {
        if (point->waittime == 0) {
            next = (trail = point)->next;
            continue;		/*
                             * Skip if not timed-wait 
                             */
        }
        if (point->waittime <= mudstate.now) {
            if (trail != NULL)
                trail->next = next = point->next;
            else
                mudstate.qsemfirst = next = point->next;
            if (point == mudstate.qsemlast)
                mudstate.qsemlast = trail;
            add_to(point->sem, -1, point->attr);
            point->sem = NOTHING;
            printk("promoting, %d/%s", point->player, point->comm);
            cque_enqueue(point->player, point);
        } else
            next = (trail = point)->next;
    }
    mudstate.debug_cmd = cmdsave;
    return;
}

/*
 * ---------------------------------------------------------------------------
 * * do_top: Execute the command at the top of the queue
 */

int do_top(int ncmds) {
    BQUE *tmp, *walk;
    OBJQE *current_object;
    dbref object, player, last_player;
    int count, i;
    char *command, *cp, *cmdsave;

    if ((mudconf.control_flags & CF_DEQUEUE) == 0)
        return 0;

    cmdsave = mudstate.debug_cmd;
    mudstate.debug_cmd = (char *) "< do_top >";
    
    if(!mudstate.qhead) return 0;

    current_object = mudstate.qhead;
    count = 0;

    while(count < ncmds && mudstate.qhead) {
        if(!mudstate.qhead) break;
        
        object = mudstate.qhead->obj;
        tmp = cque_deque(object);

        if(!mudstate.qhead->cque) {
            mudstate.qhead->queued = 0;
            mudstate.qhead = mudstate.qhead->next;
            if(mudstate.qhead == NULL) mudstate.qtail = NULL;
        } else {
            mudstate.qtail->next = mudstate.qhead;
            mudstate.qtail = mudstate.qtail->next;
            mudstate.qhead = mudstate.qhead->next;
            mudstate.qtail->next = NULL;
        }
        if(!tmp) continue;

        dassert(tmp, "serious braindamage");
        count++;
        if((object >= 0) && !Going(object)) {
            giveto(object, mudconf.waitcost);
            mudstate.curr_enactor = tmp->cause;
            mudstate.curr_player = object;
            a_Queue(Owner(object), -1);
            if(!Halted(object)) {
                for (i = 0; i < MAX_GLOBAL_REGS; i++) {
                    if (tmp->scr[i]) {
                        StringCopy(mudstate.global_regs[i],
                                tmp->scr[i]);
                    } else {
                        *mudstate.global_regs[i] = '\0';
                    }
                }

                command = tmp->comm;

                if(command) {
                    if(isPlayer(object)&&Connected(object)) choke_player(object);
                    while (command) {
                        cp = parse_to(&command, ';', 0);
                        if (cp && *cp) {
                            while (command && (*command == '|')) {
                                command++;
                                mudstate.inpipe = 1;
                                mudstate.poutnew = alloc_lbuf("process_command.pipe");
                                mudstate.poutbufc = mudstate.poutnew;
                                mudstate.poutobj = object;
                                process_command(object, tmp->cause, 0, cp, tmp->env,
                                        tmp->nargs);
                                if (mudstate.pout) {
                                    free_lbuf(mudstate.pout);
                                    mudstate.pout = NULL;
                                }

                                *mudstate.poutbufc = '\0';
                                mudstate.pout = mudstate.poutnew;
                                cp = parse_to(&command, ';', 0);
                            }
                            mudstate.inpipe = 0;
                            process_command(object, tmp->cause, 0,
                                    cp, tmp->env,
                                    tmp->nargs);
                            if (mudstate.pout) {
                                free_lbuf(mudstate.pout);
                                mudstate.pout = NULL;
                            }
                        }
                    }
                    if(isPlayer(object)&&Connected(object)) release_player(object);
                }
            }
        }
        free(tmp->text);
        free_qentry(tmp);
    }

    for (i = 0; i < MAX_GLOBAL_REGS; i++)
        *mudstate.global_regs[i] = '\0';
    mudstate.debug_cmd = cmdsave;
    return count;
}

/*
 * ---------------------------------------------------------------------------
 * * do_ps: tell player what commands they have pending in the queue
 */

static void show_que(dbref player, int key, BQUE *queue, int *qent, const char *header) {
    BQUE *tmp;
    char *bp, *bufp;
    int i;

    for (tmp = queue; tmp; tmp = tmp->next) {
        (*qent)++;
        if (key == PS_SUMM)
            continue;
        if (*qent == 1)
            notify_printf(player, "----- %s Queue -----", header);
        
        bufp = unparse_object(player, tmp->player, 0);
        if ((tmp->waittime > 0) && (Good_obj(tmp->sem)))
            notify_printf(player, "[#%d/%d]%s:%s", tmp->sem,
                        tmp->waittime - mudstate.now, bufp, tmp->comm);
        else if (tmp->waittime > 0)
            notify_printf(player, "[%d]%s:%s", tmp->waittime - mudstate.now, 
                    bufp, tmp->comm);
        else if (Good_obj(tmp->sem))
            notify_printf(player, "[#%d]%s:%s", tmp->sem, bufp,
                        tmp->comm);
        else
            notify_printf(player, "%s:%s", bufp, tmp->comm);

        bp = bufp;
        if (key == PS_LONG) {
            for (i = 0; i < (tmp->nargs); i++) {
                if (tmp->env[i] != NULL) {
                    safe_str((char *) "; Arg", bufp, &bp);
                    safe_chr(i + '0', bufp, &bp);
                    safe_str((char *) "='", bufp, &bp);
                    safe_str(tmp->env[i], bufp, &bp);
                    safe_chr('\'', bufp, &bp);
                }
            }
            *bp = '\0';
            bp = unparse_object(player, tmp->cause, 0);
            notify_printf(player, "   Enactor: %s%s", bp, bufp);
            free_lbuf(bp);
        }
        free_lbuf(bufp);
    }
    return;
}

void do_ps(dbref player, dbref cause, int key, char *target) {
    char *bufp;
    dbref player_targ, obj_targ;
    int pqent, pqtot, pqdel, oqent, oqtot, oqdel, wqent, wqtot, sqent,
        sqtot, i;
    OBJQE *objq;

    /*
     * Figure out what to list the queue for 
     */

    if ((key & PS_ALL) && !(See_Queue(player))) {
        notify(player, "Permission denied.");
        return;
    }
    if (!target || !*target) {
        obj_targ = NOTHING;
        if (key & PS_ALL) {
            player_targ = NOTHING;
        } else {
            player_targ = Owner(player);
            if (Typeof(player) != TYPE_PLAYER)
                obj_targ = player;
        }
    } else {
        player_targ = Owner(player);
        obj_targ = match_controlled(player, target);
        if (obj_targ == NOTHING)
            return;
        if (key & PS_ALL) {
            notify(player, "Can't specify a target and /all");
            return;
        }
        if (Typeof(obj_targ) == TYPE_PLAYER) {
            player_targ = obj_targ;
            obj_targ = NOTHING;
        }
    }
    key = key & ~PS_ALL;

    switch (key) {
        case PS_BRIEF:
        case PS_SUMM:
        case PS_LONG:
            break;
        default:
            notify(player, "Illegal combination of switches.");
            return;
    }

    /*
     * Go do it 
     */
    pqtot = 0;
    if(player_targ == NOTHING) {
        objq = mudstate.qhead;
        while(objq && (objq = objq->next) != NULL) {
            pqent = 0;
            show_que(player, key, objq->cque, &pqent, "PLAYAH");
            pqtot += pqent;
        }
    } else {
        pqent = 0;
        objq = cque_find(player_targ);
        if(objq) {
            show_que(player, key, objq->cque, &pqent, "PLAYAH");
        }
    }

    wqent = 0; sqent = 0; wqtot = 0; sqtot = 0;
    show_que(player, key, mudstate.qwait, &wqent, "Wait");
    show_que(player, key, mudstate.qsemfirst, &sqent, "Semaphore"); 

    /*
     * Display stats 
     */

    if (See_Queue(player))
        notify_printf(player, "Totals: Player...%d/%d  Wait...%d/%d  Semaphore...%d/%d",
                pqent, pqtot, wqent, wqtot, sqent, sqtot);
    else
        notify_printf(player, "Totals: Player...%d/%d  Wait...%d/%d  Semaphore...%d/%d",
                pqent, pqtot, wqent, wqtot, sqent, sqtot);
}

/*
 * ---------------------------------------------------------------------------
 * * do_queue: Queue management
 */

void do_queue(dbref player, dbref cause, int key, char *arg) {
    BQUE *point;
    int i, ncmds, was_disabled;

    dprintk("WTF?");
    was_disabled = 0;
    if (key == QUEUE_KICK) {
        i = atoi(arg);
        if ((mudconf.control_flags & CF_DEQUEUE) == 0) {
            was_disabled = 1;
            mudconf.control_flags |= CF_DEQUEUE;
            notify(player, "Warning: automatic dequeueing is disabled.");
        }
        ncmds = do_top(i);
        if (was_disabled)
            mudconf.control_flags &= ~CF_DEQUEUE;
        if (!Quiet(player))
            notify_printf(player, "%d commands processed.", ncmds);
    } else if (key == QUEUE_WARP) {
        i = atoi(arg);
        if ((mudconf.control_flags & CF_DEQUEUE) == 0) {
            was_disabled = 1;
            mudconf.control_flags |= CF_DEQUEUE;
            notify(player, "Warning: automatic dequeueing is disabled.");
        }

        /*
         * Handle the semaphore queue 
         */

        for (point = mudstate.qsemfirst; point; point = point->next) {
            if (point->waittime > 0) {
                point->waittime -= i;
                if (point->waittime <= 0)
                    point->waittime = -1;
            }
        }

        do_second();
        if (was_disabled)
            mudconf.control_flags &= ~CF_DEQUEUE;
        if (Quiet(player))
            return;
        if (i > 0)
            notify_printf(player, "WaitQ timer advanced %d seconds.", i);
        else if (i < 0)
            notify_printf(player, "WaitQ timer set back %d seconds.", i);
        else
            notify(player, "Object queue appended to player queue.");

    }
}
