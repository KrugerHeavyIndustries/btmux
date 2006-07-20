/*
 * bsd.c 
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
#include "attrs.h"
#include "rbtree.h"
#include <errno.h>
#include "logcache.h"

#include "debug.h"

#if SQL_SUPPORT
#include "sqlchild.h"
#endif

struct event listen_sock_ev;
#ifdef IPV6_SUPPORT
struct event listen6_sock_ev;
#endif

int mux_bound_socket = -1;
#ifdef IPV6_SUPPORT
int mux_bound_socket6 = -1;
#endif
int ndescriptors = 0;

DESC *descriptor_list = NULL;

void mux_release_socket();
void make_nonblocking(int s);
void accept_new_connection(int, short, void *);
DESC *initializesock(int, struct sockaddr_storage *, int);
DESC *new_connection(int);
int process_input(DESC *);

int desc_cmp(void *vleft, void *vright, void *token)
{
    dbref left = (dbref) vleft;
    dbref right = (dbref) vright;

    return (left - right);
}

void desc_addhash(DESC * d)
{
    DESC *hdesc;

    bind_descriptor(d);

    hdesc = (DESC *) rb_find(mudstate.desctree, (void *) d->player);
    if(!hdesc) {
        dprintk("Creating new list root for '%s'(#%d) at %p.", 
            Name(d->player), d->player, d);
    } else {
        dprintk("Adding descriptor %p to list root at %p for '%s'(#%d).",
        d, hdesc, Name(d->player), d->player);
    }
    
    d->hashnext = hdesc;
    rb_insert(mudstate.desctree, (void *) d->player, d);
}

void desc_delhash(DESC * d)
{
    char buffer2[4096];
    DESC *hdesc = NULL;
    char buffer[4096];
    dprintk("removing descriptor %p from list root %p for '%s'(#%d).", d, hdesc, Name(d->player), d->player);
    hdesc = (DESC *) rb_find(mudstate.desctree, (void *) d->player);
    dprintk("removing descriptor %p from list root %p for '%s'(#%d).", d, hdesc, Name(d->player), d->player);


    if(!hdesc) {
        snprintf(buffer, 4096,
                 "desc_delhash: unable to find player(%d)'s descriptors from hashtable.\n",
                 d->player);
        log_text(buffer);
        release_descriptor(d);
        return;
    }

    dprintk("hdesc: %p, d: %p, hdesc->hashnext: %p, d->hashnext: %p", hdesc,
            d, hdesc->hashnext, d->hashnext);

    if(hdesc == d && hdesc->hashnext) {
        dprintk("updating %d to use hashroot %p", d->player, d->hashnext);
        rb_insert(mudstate.desctree, (void *) d->player, d->hashnext);
        d->hashnext = NULL;
        release_descriptor(d);
        return;
    } else if(hdesc == d) {
        dprintk("removing %d table", d->player);
        rb_delete(mudstate.desctree, (void *) d->player);
        release_descriptor(d);
        return;
    }

    while(hdesc->hashnext != NULL) {
        if(hdesc->hashnext == d) {
            hdesc->hashnext = d->hashnext;
            break;
        }
        hdesc = hdesc->hashnext;
    }
    d->hashnext = NULL;
    release_descriptor(d);
    return;
}

void bind_descriptor(DESC *d) {
    d->refcount++;
    //dprintk("bound desciptor %p, refcount now %d", d, d->refcount);
}

void release_descriptor(DESC *d) {
    d->refcount--;
    //dprintk("descriptor %p released, refcount now %d", d, d->refcount);
    if(d->refcount == 0) {
        dprintk("%p destructing", d);
        freeqs(d);

		if(d->program_data != NULL) {
			int num = 0;
            DESC *dtemp;
			DESC_ITER_PLAYER(d->player, dtemp) num++;

			if(num == 0) {
				for(int i = 0; i < MAX_GLOBAL_REGS; i++) {
					free_lbuf(d->program_data->wait_regs[i]);
				}
				free(d->program_data);
			}
		}
        clearstrings(d);
        if(d->descriptor) {
            fsync(d->descriptor);
            event_del(&d->sock_ev);
            shutdown(d->descriptor, 2);
            close(d->descriptor);
        }
        d->descriptor = 0;
        if(d->sock_buff)
            bufferevent_free(d->sock_buff);
        d->sock_buff = NULL;
        
/*        if(descriptor_list == d) {
            descriptor_list = d->next;
        } else {
            if(!descriptor_list) {
                dprintk("Oh sweet jesus, we have major braindamage.");
                descriptor_list = d->next;
            } else {
                DESC *dtemp = descriptor_list;
                while(dtemp->next != NULL) {
                    if(dtemp->next == d) {
                        dtemp->next = d->next;
                        break;
                    } else {
                        dtemp = dtemp->next;
                    }
                }
            }
        }

        d->next = NULL;
	*/
	 if (d->prev)
		     d->prev->next = d->next;
	   else                          /* d was the first one! */
		       descriptor_list = d->next;
	     if (d->next)
		         d->next->prev = d->prev;

        ndescriptors--;
        free(d);
    }
}

void shutdown_services()
{
	dnschild_destruct();
	flush_sockets();
#ifdef SQL_SUPPORT
	sqlchild_destruct();
#endif
#ifdef ARBITRARY_LOGFILES
	logcache_destruct();
#endif
	event_loopexit(NULL);
}

int bind_mux_socket(int port)
{
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
		log_perror("LOGCACHE", "FAIL", NULL,
				   "fcntl(fd, F_SETFD, FD_CLOEXEC)");
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
	dprintk("connection socket raised and bound, %d", s);
	listen(s, 25);
	return s;
}

void mux_release_socket()
{
	dprintk("releasing mux main socket.");
	event_del(&listen_sock_ev);
	close(mux_bound_socket);
	mux_bound_socket = -1;
#ifdef IPV6_SUPPORT
	event_del(&listen6_sock_ev);
	close(mux_bound_socket6);
	mux_bound_socket6 = -1;
#endif
}

int eradicate_broken_fd(int fd)
{
	struct stat statbuf;
	DESC *d, *dtemp;

	DESC_SAFEITER_ALL(d, dtemp) {
		if((fd && d->descriptor == fd) ||
		   (!fd && fstat(d->descriptor, &statbuf) < 0)) {
			/* An invalid player connection... eject, eject, eject. */
			log_error(LOG_PROBLEMS, "ERR", "EBADF",
					  "Broken descriptor %d for player #%d", d->descriptor,
					  d->player);
            close(d->descriptor);
			shutdownsock(d, R_SOCKDIED);
		}
	}
	if(mux_bound_socket != -1 && fstat(mux_bound_socket, &statbuf) < 0) {
		log_error(LOG_PROBLEMS, "ERR", "EBADF",
				  "Broken descriptor on our main port.");
		mux_bound_socket = -1;
		return -1;
	}
#ifdef IPV6_SUPPORT
	if(mux_bound_socket6 != -1 && fstat(mux_bound_socket6, &statbuf) < 0) {
		log_error(LOG_PROBLEMS, "ERR", "EBADF",
				  "Broken descriptor for our ipv6 port.");
		mux_bound_socket6 = -1;
		return -1;
	}
#endif
	return 0;
}

void accept_client_input(int fd, short event, void *arg)
{
	DESC *connection = (DESC *) arg;

    //dprintk("callback on fd %d DESC %p", fd, arg);

	if(connection->flags & DS_AUTODARK) {
		connection->flags &= ~DS_AUTODARK;
		s_Flags(connection->player, Flags(connection->player) & ~DARK);
	}
    bind_descriptor(connection);
	process_input(connection);
    //dprintk("finish on fd %d DESC %p", fd, arg);
    release_descriptor(connection);
}

void bsd_write_callback(struct bufferevent *bufev, void *arg)
{
}

void bsd_read_callback(struct bufferevent *bufev, void *arg)
{
}

void bsd_error_callback(struct bufferevent *bufev, short whut, void *arg)
{
	dprintk("error %d", whut);
}

#ifndef HAVE_GETTIMEOFDAY
#define get_tod(x)	{ (x)->tv_sec = time(NULL); (x)->tv_usec = 0; }
#else
#define get_tod(x)	gettimeofday(x, (struct timezone *)0)
#endif

struct timeval queue_slice = { 0, 0 };
struct event queue_ev;
struct timeval last_slice, current_time;

void runqueues(int fd, short event, void *arg)
{
	pid_t pchild;
	int status = 0;
	event_add(&queue_ev, &queue_slice);
	get_tod(&current_time);
	last_slice = update_quotas(last_slice, current_time);
	pchild = waitpid(-1, &status, WNOHANG);
	if(pchild > 0) {
		dprintk("unexpected child %d exited with exit status %d.", pchild,
				WEXITSTATUS(status));
	}
	if(mudconf.queue_chunk)
		do_top(mudconf.queue_chunk);
}

void shovechars(int port)
{

	queue_slice.tv_sec = 0;
	queue_slice.tv_usec = mudconf.timeslice * 1000;

	dprintk("shovechars starting, sock is %d.", mux_bound_socket);
#ifdef IPV6_SUPPORT
	dprintk("shovechars starting, ipv6 sock is %d.", mux_bound_socket);
#endif

	if(mux_bound_socket < 0) {
		mux_bound_socket = bind_mux_socket(port);
	}
	event_set(&listen_sock_ev, mux_bound_socket, EV_READ | EV_PERSIST,
			  accept_new_connection, NULL);
	event_add(&listen_sock_ev, NULL);

#ifdef IPV6_SUPPORT
	if(mux_bound_socket6 < 0) {
		mux_bound_socket6 = bind_mux6_socket(port);
	}
	event_set(&listen6_sock_ev, mux_bound_socket6, EV_READ | EV_PERSIST,
			  accept_new6_connection, NULL);
	event_add(&listen6_sock_ev, NULL);
#endif

	evtimer_set(&queue_ev, runqueues, NULL);
	evtimer_add(&queue_ev, &queue_slice);

	get_tod(&last_slice);
	get_tod(&current_time);

	event_dispatch();
}

void accept_new_connection(int sock, short event, void *arg)
{
	int newsock, addr_len, len;
	char *buff, *buff1;
	DESC *d;
	struct sockaddr_storage addr;
	char addrname[1024];
	char addrport[32];


	addr_len = sizeof(struct sockaddr);

	newsock =
		accept(sock, (struct sockaddr *) &addr, (unsigned int *) &addr_len);
	if(newsock < 0)
		return;

	getnameinfo((struct sockaddr *) &addr, addr_len,
				addrname, 1024, addrport, 32,
				NI_NUMERICHOST | NI_NUMERICSERV);

	if(site_check(&addr, addr_len, mudstate.access_list) == H_FORBIDDEN) {
		log_error(LOG_NET | LOG_SECURITY, "NET", "SITE",
				  "Connection refused from %s %s.", addrname, addrport);

		fcache_rawdump(newsock, FC_CONN_SITE);
		shutdown(newsock, 2);
		close(newsock);
		errno = 0;
		d = NULL;
	} else {
		log_error(LOG_NET, "NET", "CONN", "Connection opened from %s %s.",
				  addrname, addrport);

		d = initializesock(newsock, &addr, addr_len);
	}
	return;
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

void shutdownsock(DESC * d, int reason)
{
	char *buff, *buff2;
	time_t now;
	int i, num;
	DESC *dtemp;

    dprintk("shutdownsock called on %p %s(#%d) refcount %d", 
        d, (d->player?Name(d->player):""), d->player, d->refcount);

    if(d->flags & DS_CONNECTED) {
		if(d->outstanding_dnschild_query)
			dnschild_kill(d->outstanding_dnschild_query);

		/*
		 * Do the disconnect stuff if we aren't doing a LOGOUT * * *
		 * * * * (which keeps the connection open so the player can *
		 * * connect * * * * to a different character). 
		 */

		if(reason != R_LOGOUT) {
			fcache_dump(d, FC_QUIT);
		}

		log_error(LOG_NET | LOG_LOGIN, "NET", "DISC",
				  "[%d/%s] Logout by %s(#%d), <Reason: %s>",
				  d->descriptor, d->addr, Name(d->player), d->player,
				  disc_reasons[reason]);

		/*
		 * If requested, write an accounting record of the form: * *
		 * * * * * Plyr# Flags Cmds ConnTime Loc Money [Site]
		 * <DiscRsn>  * *  * Name 
		 */

		log_error(LOG_ACCOUNTING, "DIS", "ACCT",
				  "%d %s %d %d %d %d [%s] <%s> %s",
				  d->player, decode_flags(GOD, Flags(d->player),
										  Flags2(d->player),
										  Flags3(d->player)),
				  d->command_count, mudstate.now - d->connected_at,
				  Location(d->player), Pennies(d->player), d->addr,
				  disc_reasons[reason], Name(d->player));

        announce_disconnect(d->player, d, disc_messages[reason]);
        desc_delhash(d);

	}
    d->flags |= DS_DEAD;
    release_descriptor(d);
    dprintk("shutdown.");
}

void make_nonblocking(int s)
{
	long flags = 0;

	if(fcntl(s, F_GETFL, &flags) < 0) {
		log_perror("NET", "FAIL", "make_nonblocking", "fcntl F_GETFL");
	}
	flags |= O_NONBLOCK;
	if(fcntl(s, F_SETFL, flags) < 0) {
		log_perror("NET", "FAIL", "make_nonblocking", "fcntl F_SETFL");
	}
	flags = 1;
	if(setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags)) < 0) {
		log_perror("NET", "FAIL", "make_nonblocking", "setsockopt NDELAY");
	}
}

void make_blocking(int s)
{
	long flags = 0;

	if(fcntl(s, F_GETFL, &flags) < 0) {
		log_perror("NET", "FAIL", "make_blocking", "fcntl F_GETFL");
	}
	flags &= ~O_NONBLOCK;
	if(fcntl(s, F_SETFL, flags) < 0) {
		log_perror("NET", "FAIL", "make_blocking", "fcntl F_SETFL");
	}
	flags = 0;
	if(setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags)) < 0) {
		log_perror("NET", "FAIL", "make_blocking", "setsockopt NDELAY");
	}
}
extern int fcache_conn_c;

DESC *initializesock(int s, struct sockaddr_storage *saddr, int saddr_len)
{
	DESC *d;

	ndescriptors++;
	d = malloc(sizeof(DESC));
	memset(d, 0, sizeof(DESC));

	d->descriptor = s;
	d->flags = 0;
	d->connected_at = mudstate.now;
	d->retries_left = mudconf.retry_limit;
	d->command_count = 0;
	d->timeout = mudconf.idle_timeout;
	d->host_info =
		site_check(saddr, saddr_len, mudstate.access_list) |
		site_check(saddr, saddr_len, mudstate.suspect_list);
	d->player = 0;			
	d->chokes = 0;
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
	d->input_size = 0;
	d->input_tot = 0;
	d->input_lost = 0;
	memset(d->input, 0, sizeof(d->input));
    d->input_tail = 0;
	d->quota = mudconf.cmd_quota_max;
	d->program_data = NULL;
	d->last_time = 0;
	memcpy(&d->saddr, saddr, saddr_len);
	d->saddr_len = saddr_len;

	d->hashnext = NULL;
	getnameinfo((struct sockaddr *) saddr, saddr_len, d->addr,
				sizeof(d->addr), NULL, 0, NI_NUMERICHOST);

	if (descriptor_list)
		descriptor_list->prev = d;
        d->next = descriptor_list;
        d->prev = NULL;
        descriptor_list = d;
       
	d->outstanding_dnschild_query = dnschild_request(d);

	d->sock_buff = bufferevent_new(d->descriptor, bsd_write_callback,
								   bsd_read_callback, bsd_error_callback,
								   NULL);
	bufferevent_disable(d->sock_buff, EV_READ);
	bufferevent_enable(d->sock_buff, EV_WRITE);
	event_set(&d->sock_ev, d->descriptor, EV_READ | EV_PERSIST, 
        accept_client_input, d);
	event_add(&d->sock_ev, NULL);
    bind_descriptor(d);
	welcome_user(d);
	return d;
}

int process_input(DESC * d)
{
	char buf[LBUF_SIZE];
	int got, in, iter;
    char current;

    if(d->flags & DS_DEAD) { 
        dprintk("Bailing on process_input %p %d %s %d", 
        d, d->descriptor, (d->player?Name(d->player):""), d->player);
        return 0;
    }

    memset(buf, 0, sizeof(buf));

	got = in = read(d->descriptor, buf, (sizeof buf - 1));

	if(got <= 0) {
		if(errno == EINTR)
			return 1;
        else if(errno == EAGAIN)
            return 1;
		else {
            dprintk("error %s (errno %d) read on fd %d descriptor %p %s(%d)\n",
            strerror(errno), errno,
                d->descriptor, d, (d->player?Name(d->player):""), d->player);
            shutdownsock(d, R_SOCKDIED);
            return 1;
        }
	}

    bind_descriptor(d);
    
	if(Wizard(d->player) && strncmp("@segfault", buf, 9) == 0) {
		queue_string(d, "@segfault failed. (check logfile for reason.)\n");
		*(char *) 0xDEADBEEF = '9';
	}

	for(iter = 0; iter < got; iter++) {
        current = buf[iter];
        if(current == '\n') {
            if(d->flags & DS_CONNECTED) {
                //dprintk("authed as %s running command '%s' refcount %d descriptor %p fd %d", Name(d->player), d->input, d->refcount, d, d->descriptor);
                run_command(d, (char *)d->input);
            } else {
                //dprintk("unauth running command '%s' refcount %d descriptor %p fd %d", d->input, d->refcount, 
                //d, d->descriptor);
                if(!do_unauth_command(d, d->input))  {
                    dprintk("logout on %p fd %d, bailing.", d, d->descriptor);
                    shutdownsock(d, R_QUIT);
                    break;
                }
            }
            memset(d->input, 0, sizeof(d->input));
            d->input_tail = 0;
            if(d->flags & DS_DEAD) break;
        } else if(current == '\b' || current == 0x7f) {
            if(current == 127) {
                queue_string(d, "\b \b");
            } else {
                queue_string(d, " \b");
            }
            if(d->input_tail > 0) {
                d->input[--d->input_tail] = '\0';
            }
            d->input_size--;
        } else if(isascii(current) && isprint(current)) {
            if(d->input_tail >= sizeof(d->input)) {
                continue;
            }
            d->input[d->input_tail++] = current;
            d->input_size++;
        }
	}
    //dprintk("finished %p fd %d", d, d->descriptor);

    release_descriptor(d);
	return 1;
}

void flush_sockets()
{
	int null = 0;
	DESC *d, *dnext;
	DESC_SAFEITER_ALL(d, dnext) {
		if(d->chokes) {
#if TCP_CORK
			setsockopt(d->descriptor, IPPROTO_TCP, TCP_CORK, &null,
					   sizeof(null));
#else
#ifdef TCP_NOPUSH
			setsockopt(d->descriptor, IPPROTO_TCP, TCP_NOPUSH, &null,
					   sizeof(null));
#endif
#endif
			d->chokes = 0;
		}
		if(d->sock_buff && EVBUFFER_LENGTH(d->sock_buff->output)) {
			evbuffer_write(d->sock_buff->output, d->descriptor);
		}
		fsync(d->descriptor);
	}
}

void close_sockets(int emergency, char *message)
{
	DESC *d, *dnext;

	DESC_SAFEITER_ALL(d, dnext) {
		if(emergency) {
			write(d->descriptor, message, strlen(message));
			if(shutdown(d->descriptor, 2) < 0)
				log_perror("NET", "FAIL", NULL, "shutdown");
			dprintk("shutting down fd %d", d->descriptor);
			dprintk("output evbuffer misalign: %d, totallen: %d, off: %d",
					d->sock_buff->output->misalign,
					d->sock_buff->output->totallen,
					d->sock_buff->output->off);
			fsync(d->descriptor);
			if(d->outstanding_dnschild_query)
				dnschild_kill(d->outstanding_dnschild_query);
            d->outstanding_dnschild_query = NULL;
			event_loop(EVLOOP_ONCE);
			event_del(&d->sock_ev);
			bufferevent_free(d->sock_buff);
			close(d->descriptor);
		} else {
			queue_string(d, message);
			queue_write(d, "\r\n", 2);
			shutdownsock(d, R_GOING_DOWN);
		}
	}
	close(mux_bound_socket);
	event_del(&listen_sock_ev);
}

void emergency_shutdown(void) {
	close_sockets(1, (char *) "Going down - Bye.\n");
}
