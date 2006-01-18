/*
 * network.c
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
#include <netdb.h>
#include <arpa/inet.h>
#include "externs.h"

struct listening_socket_t {
	int fd;
	struct sockaddr_storage addr;
	int salen;
	struct event ev;
	struct listening_socket_t *next;
} *listening_sockets = NULL;

DESC *descriptor_list = NULL;

int network_init(int port);

static void network_accept_client(int fd, short event, void *arg);
static int network_bind_port(struct sockaddr *addr, int salen);
static void network_make_nonblocking(int socket);
static void network_make_blocking(int socket);
DESC *network_initialize_socket(int socket, struct sockaddr *saddr,
								int addrlen);

static int network_bind_port(struct sockaddr *addr, int salen)
{
	int one = 1;
	struct listening_socket_t *lst = NULL;

	lst = malloc(sizeof(struct listening_socket_t));
	memset(lst, 0, sizeof(struct listening_socket_t));

	lst->fd = socket(addr->sa_family, SOCK_STREAM, 0);
	if(lst->fd < 0) {
		log_perror("NET", "FAIL", "network_bind_port", "socket");
		goto error;
	}

	if(setsockopt(lst->fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		log_perror("NET", "WARN", "network_bind_port", "setsockopt");
	}

	memcpy(&lst->addr, addr, salen);
	lst->salen = salen;

	if(bind(lst->fd, (struct sockaddr *) &lst->addr, lst->salen) < 0) {
		log_perror("NET", "FAIL", "network_bind_port", "bind");
		goto error;
	}

	if(listen(lst->fd, 0) < 0) {
		log_perror("NET", "FAIL", "network_bind_port", "listen");
		goto error;
	}

	network_make_nonblocking(lst->fd);

	event_set(&lst->ev, lst->fd, EV_READ | EV_PERSIST, network_accept_client,
			  lst);
	event_add(&lst->ev, NULL);

	lst->next = listening_sockets;
	listening_sockets = lst;
	return 1;

  error:
	if(lst->fd >= 0) {
		close(lst->fd);
	}
	memset(lst, 0, sizeof(struct listening_socket_t));
	return 0;
}

int network_init(int port)
{
	struct addrinfo hints;
	struct addrinfo *res, *walk;
	char myservice[128];
	char hostname[1025];
	int error;

	snprintf(myservice, 32, "%d", port + 10);

	signal(SIGPIPE, SIG_IGN);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags =
		AI_PASSIVE | AI_NUMERICSERV | AI_ADDRCONFIG | AI_ALL | AI_V4MAPPED;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(NULL, myservice, &hints, &res);
	walk = res;
	if(error)
		perror(gai_strerror(error));
	else {
		/*
		 * "res" has a chain of addrinfo structure filled with
		 * 0.0.0.0 (for IPv4), 0:0:0:0:0:0:0:0 (for IPv6) and alike,
		 * with port filled for "myservice".
		 */
		while (walk) {
			if(getnameinfo(walk->ai_addr, walk->ai_addrlen, hostname, 1025,
						   myservice, 128, NI_NUMERICSERV)) {
				log_printf("network] serious problem in getnameinfo().");
			}
			if(network_bind_port(walk->ai_addr, walk->ai_addrlen) == 0) {
				log_printf("network] binding to %s %s %s failed.",
						   (walk->ai_family == AF_INET6 ? "IPv6" : "IPv4"),
						   hostname, myservice);
			} else {
				log_printf("network] bound to %s %s %s. ",
						   (walk->ai_family == AF_INET6 ? "IPv6" : "IPv4"),
						   hostname, myservice);
			}

			walk = walk->ai_next;
		}
	}
	freeaddrinfo(res);
	return 1;
}

static void network_make_nonblocking(int s)
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

static void network_make_blocking(int s)
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

/* choke_player: cork the player's sockets, must have a matching release_socket */
void network_choke_socket(DESC * desc)
{
	int eins = 1;

	if(desc->chokes == 0) {
#if defined(TCP_CORK)			// Linux 2.4, 2.6
		setsockopt(d->fd, IPPROTO_TCP, TCP_CORK, &eins, sizeof(eins));
#elif defined(TCP_NOPUSH)		// *BSD, Mac OS X
		setsockopt(d->fd, IPPROTO_TCP, TCP_NOPUSH, &eins, sizeof(eins));
#else // else
		/* Nothing! */
#endif
	}
	desc->chokes++;
}

void network_release_socket(DESC * desc)
{
	int null = 0;

	desc->chokes--;
	if(desc->chokes < 0)
		desc->chokes = 0;

	if(desc->chokes == 0) {
#if defined(TCP_CORK)			// Linux 2.4, 2.6
		setsockopt(d->fd, IPPROTO_TCP, TCP_CORK, &null, sizeof(null));
#elif defined(TCP_NOPUSH)		// *BSD, Mac OS X
		setsockopt(d->fd, IPPROTO_TCP, TCP_NOPUSH, &null, sizeof(null));
#else // else
		/* Nothing! */
#endif
	}
}

static void network_accept_client(int fd, short event, void *arg)
{
	int newfd;
	struct sockaddr_storage addr;
	unsigned int addrlen = sizeof(struct sockaddr_storage);
	struct descriptor_data *desc;

	newfd = accept(fd, (struct sockaddr *) addr, &addrlen);
	if(newfd < 0) {
		log_printf("network] accept on %d failed with '%s'", fd,
				   strerror(errno));
		return;
	}
	desc = network_initialize_socket(newfd, addr, addrlen);
	log_printf("network] connection from %s %d", desc->ip, desc->port);
}

DESC *network_initialize_socket(int socket, struct sockaddr * addr,
								int addrlen)
{
	char remotehost[NETWORK_HOSTNAME_MAX];
	char remoteport[NETWORK_PORTNAME_MAX];
	DESC *desc;

	desc = malloc(sizeof(DESC));
	memset(desc, 0, sizeof(DESC));

	desc->fd = socket;

	memcpy(desc->saddr, addr, addrlen);

	if(getnameinfo(addr, addrlen, remotehost, NETWORK_HOSTNAME_MAX,
				   remoteport, NETWORK_PORTNAME_MAX,
				   NI_NUMERICSERV | NI_NUMERICHOST)) {
		log_printf("network] getnameinfo failed!");
		desc->ip = strdup("unspec");
		desc->port = -1;
	} else {
		desc->ip = strdup(remotehost);
		desc->port = itoa(remoteport);
	}

	network_make_nonblocking(socket);

	desc->connected_at = mudstate.now;
	desc->retries_left = mudconf.retry_limit;
	desc->timeout = mudconf.idle_timeout;
	desc->host_info = 1;
	desc->next = descriptor_list;
	descriptor->list = desc;

	desc->sock_buff = bufferevent_new(desc->fd, network_write_callback,
									  network_read_callback,
									  network_error_callback, NULL);
	bufferevent_disable(desc->sock_buff, EV_READ);
	bufferevent_enable(desc->sock_buff, EV_WRITE);
	event_set(&desc->sock_ev, desc->fd, EV_READ | EV_PERSIST,
			  network_accept_input, d);
	event_add(&desc->sock_ev, NULL);
	telnet_init(desc);

	welcome_user(desc);

	return desc;
}

/* network_client_input();                                                                                          
 *                                                                                                                  
 * Called by libevent from event initialized in network_accept_client();                                            
 *                                                                                                                  
 * Theoretical data isolation would specify that we do the read and pass
 * the data to the telnet code. As an optimization, we allow telnet to 
 * complete the read.
 */
static void network_client_input(int fd, short event, void *arg)
{
	struct DESC *client = (DESC *) arg;
	int avail, net_length;

	dprintk("fd = %d, head = %d, tail = %d", client->fd, client->ringhead,
			client->ringtail);
	if(client->ringtail < client->ringhead) {
		avail = client->ringhead - client->ringtail;
		handle_errno(net_length = read(client->fd,
									   client->ringbuffer + client->ringtail,
									   avail));
	} else {
		if(client->ringhead == 0) {
			avail = RING_LENGTH - client->ringtail;
			handle_errno(net_length = read(client->fd,
										   client->ringbuffer +
										   client->ringtail, avail));
			client->ringtail = (client->ringtail + net_length) % RING_LENGTH;
		} else {
			avail = RING_LENGTH - client->ringtail;
			handle_errno(net_length = read(client->fd,
										   client->ringbuffer +
										   client->ringtail, avail));
			client->ringtail = (client->ringtail + net_length) % RING_LENGTH;
			if(net_length && net_length == avail) {
				handle_errno(net_length += read(client->fd,
												client->ringbuffer,
												client->ringhead - 1));
				client->ringtail += net_length - avail;
			}
		}
	}

	dprintk("net_length = %d", net_length);

	if(net_length == 0) {
		printf(" client disconnected.\n");
		client_disconnect(client);
		return;
	}

	telnet_read_ring(client);
}

void network_write(DESC * client, char *buffer, int length)
{
	bufferevent_write(d->sock_buff, buffer, length);
	client->output_tot += length;
}
