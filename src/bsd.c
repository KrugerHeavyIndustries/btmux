/*
 * bsd.c 
 */

/*
 * $Id$ 
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
#include "rbtree.h"
#include <errno.h>
#include "logcache.h"

#define DEBUG_BSD
#ifdef DEBUG_BSD
#define DEBUG
#endif

#include "debug.h"
#if SQL_SUPPORT
#include "sqlchild.h"
#endif

#ifdef __CYGWIN__
#undef WEXITSTATUS
#define WEXITSTATUS(stat) (((*((int *) &(stat))) >> 8) & 0xff)
#endif

void accept_slave_input(int fd, short event, void *arg);
pid_t slave_pid = -1;
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

int mux_bound_socket = -1;
int ndescriptors = 0;

DESC *descriptor_list = NULL;

void slave_destruct();
void mux_release_socket();

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



void shutdown_services() {
#ifdef SQL_SUPPORT
    sqlchild_destruct();
#endif
#ifdef ARBITRARY_LOGFILES
    logcache_destruct();
#endif
    slave_destruct();
}


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
        slave_pid = -1;
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
        slave_destruct();
    }

    handle_errno(socketpair(AF_UNIX, SOCK_DGRAM, 0, sv));
    
    slave_pid = fork();
    switch (slave_pid) {
        case -1:
            close(sv[0]);
            close(sv[1]);
            return;

        case 0:
            if(mux_bound_socket > 0) {
               mux_release_socket();
            }
                        /*
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
        slave_socket = -1;
        log_perror("NET", "FAIL", NULL, "fcntl(slave_socket, F_SETFL, O_NONBLOCK)");
    }
    
    dprintk("slave initialized pid = %d, fd = %d.", slave_pid, sv[0]);
    slave_socket = sv[0];
    event_set(&slave_sock_ev, slave_socket, EV_READ | EV_PERSIST, 
            accept_slave_input, NULL);
    event_add(&slave_sock_ev, NULL);
}

void slave_destruct() {
    if(slave_socket != -1) {
        event_del(&slave_sock_ev);
        close(slave_socket);
        slave_socket = -1;
    } else {
        dprintk("slave_destruct() called without functioning slave_socket!");
    }
    if(slave_pid != -1) {
        kill(slave_pid, SIGKILL);
        slave_pid = -1; 
    } else {
        dprintk("slave_destruct() called without functioning slave.");
    }
    dprintk("slave shutdown.");
}

int bind_mux_socket(int port) {
    int s, opt;
    struct sockaddr_in server;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0) {
        log_perror("NET", "FAIL", NULL, "creating master socket");
        exit(3);
    }
    opt = 1;
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
                sizeof(opt)) < 0) {
        log_perror("NET", "FAIL", NULL, "setsockopt");
    }
    if(fcntl(s, F_SETFD, FD_CLOEXEC) < 0) {
        log_perror("LOGCACHE", "FAIL", NULL, "fcntl(fd, F_SETFD, FD_CLOEXEC)");
        close(s);
        abort();
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    if(bind(s, (struct sockaddr *) &server, sizeof(server))) {
        log_perror("NET", "FAIL", NULL, "bind");
        close(s);
        exit(4);
    }
    listen(s, 5);
    return s;
}

void mux_release_socket() {
    dprintk("releasing mux main socket.");
    event_del(&listen_sock_ev);
    close(mux_bound_socket); 
    mux_bound_socket = -1;
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
    if (mux_bound_socket != -1 && fstat(mux_bound_socket, &statbuf) < 0) {
        STARTLOG(LOG_PROBLEMS, "ERR", "EBADF") {
            log_text("Broken descriptor for our main port: ");
            log_number(slave_socket);
            ENDLOG;
        }
        mux_bound_socket = -1;
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


#ifndef HAVE_GETTIMEOFDAY
#define get_tod(x)	{ (x)->tv_sec = time(NULL); (x)->tv_usec = 0; }
#else
#define get_tod(x)	gettimeofday(x, (struct timezone *)0)
#endif

void shovechars(int port) {
    struct timeval last_slice, current_time, next_slice, timeout,
                   slice_timeout;

    mudstate.debug_cmd = (char *) "< shovechars >";

    dprintk("shovechars starting, sock is %d.", mux_bound_socket);

    if(mux_bound_socket < 0) {
        signal(SIGPIPE, SIG_IGN);
        mux_bound_socket = bind_mux_socket(port);
    }
    
    event_set(&listen_sock_ev, mux_bound_socket, EV_READ | EV_PERSIST, 
            accept_new_connection, NULL);
    event_add(&listen_sock_ev, NULL);
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

    newsock = accept(mux_bound_socket, (struct sockaddr *)&addr, &addr_len);
    dprintk("accept() result is %d.", newsock);
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
                slave_destruct();
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
    close(mux_bound_socket);
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

                shutdown_services();
                
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

