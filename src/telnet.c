#include <stdio.h>
#include <stdlib.h>
#include <gc/gc.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <time.h>
#include <event.h>
#include <netdb.h>
#include <signal.h>
#define DEBUG
#include <debug.h>
#include <rbtree.h>
#include "network.h"
#include "telnet.h"
#include "client.h"

static void telnet_send_req(DESC * client, int how, int request);
static void telnet_send_subop(DESC * client, int how, int option);
static void telnet_handle_will(DESC * client, int option);
static void telnet_handle_wont(DESC * client, int option);
static int telnet_handle_subop(DESC * client, unsigned char *buffer,
							   int length);

int telnet_init(DESC * client)
{
	dprintk("telnet initializing.");
	strncpy(client->termtype, "vt100", 16);
	client->telnet_win_width = 80;
	client->telnet_win_height = 25;

	telnet_send_req(client, TELNET_DO, TELNET_OPT_TERMTYPE);
	telnet_send_req(client, TELNET_DO, TELNET_OPT_WINSIZE);
	return 1;
}

/* Client Ring Buffer Handling */
static void telnet_send_req(DESC * client, int how, int request)
{
	unsigned char buffer[16] = { TELNET_IAC, how, request };
	network_write(client, (void *) buffer, 3);
}
static void telnet_send_subop(DESC * client, int option, int how)
{
	unsigned char buffer[16] =
		{ TELNET_IAC, TELNET_SB, option, how, TELNET_IAC, TELNET_SE };
	network_write(client, (void *) buffer, 6);
}

static void telnet_handle_will(DESC * client, int option)
{
	switch (option) {
	case TELNET_OPT_TERMTYPE:
		client->telnet_has_termtype = TSTATE_SUP;
		dprintk("client acknowledged TERMTYPE");
		break;
	case TELNET_OPT_WINSIZE:
		client->telnet_has_winsize = TSTATE_SUP;
		dprintk("client acknowledged WINSIZE");
		break;
	}
}

static void telnet_handle_wont(DESC * client, int option)
{
	switch (option) {
	case TELNET_OPT_TERMTYPE:
		client->telnet_has_termtype = TSTATE_UNSUP;
		strncpy(client->termtype, "vt100", 16);
		break;
	case TELNET_OPT_WINSIZE:
		client->telnet_has_winsize = TSTATE_UNSUP;
		client->telnet_win_width = 80;
		client->telnet_win_height = 25;
		break;
	}
}

static void telnet_check_options(DESC * client)
{
	if(client->telnet_has_termtype == TSTATE_SUP) {
		client->telnet_has_termtype = TSTATE_REQUEST;
		telnet_send_subop(client, TELNET_OPT_TERMTYPE, TELNET_SUB_REQUIRED);
	} else if(client->telnet_has_termtype == TSTATE_REQUEST) {
		client->telnet_has_termtype = TSTATE_UNSUP;
	}

	if(client->telnet_has_winsize == TSTATE_SUP) {
		client->telnet_has_winsize = TSTATE_REQUEST;
		telnet_send_subop(client, TELNET_OPT_WINSIZE, TELNET_SUB_REQUIRED);
	} else if(client->telnet_has_winsize == TSTATE_REQUEST) {
		client->telnet_has_termtype = TSTATE_UNSUP;
	}
}

static int telnet_handle_subop(DESC * client, unsigned char *buffer,
							   int length)
{
	int iter;
	if(buffer[1] != TELNET_SUB_SUPPLIED)
		dfail("unexpected reply in telnet suboption negotiation.");
	iter = 2;
	switch (buffer[0]) {
	case TELNET_OPT_TERMTYPE:
		memset(client->termtype, 0, 16);
		for(iter = 2; iter < length && iter < 18 && buffer[iter] != 0xFF;
			iter++) {
			client->termtype[iter - 2] = buffer[iter];
		}
		iter++;
		dprintk("Received Terminal Type '%s'", client->termtype);
		client->telnet_has_termtype = TSTATE_REPLIED;
		break;
	case TELNET_OPT_WINSIZE:
		client->telnet_win_width = buffer[2];
		client->telnet_win_height = buffer[4];
		iter = 6;
		dprintk("Received Window Size %dx%d", client->telnet_win_width,
				client->telnet_win_height);
		client->telnet_has_winsize = TSTATE_REPLIED;
		break;
	default:
		dfail("unexpected reply in telnet usboption negotiation.");
	}
	return iter;
}

#define ring_avail(client) (client->ringtail > client->ringhead ? RING_LENGTH - client->ringtail + client->ringhead : \
        client->ringhead - client->ringtail)
#define ring_char(client, x) (client->ringbuffer[(client->ringhead+x)%RING_LENGTH])
#define ring_length(client) (client->ringtail >= client->ringhead ? client->ringtail - client->ringhead : \
        RING_LENGTH - client->ringhead + client->ringtail)
#define ring_eat(client, x) (client->ringhead = (client->ringhead+x) % RING_LENGTH)

void telnet_read_ring(DESC * client)
{
	int iter, complete;
	unsigned char subopt_buffer[32];

	dprintk
		("Ring Read started, input length = %d, ringhead = %d, ringtail = %d. ",
		 ring_length(client), client->ringhead, client->ringtail);

	while (ring_length(client) > 0) {
		dprintk
			("Parsing data 0x%02x,  %d bytes in ring, ringhead = %d, ringtail = %d",
			 ring_char(client, 0), ring_length(client), client->ringhead,
			 client->ringtail);
		switch (ring_char(client, 0)) {
		case TELNET_IAC:
			if(ring_length(client) < 2)
				return;
			dprintk("TELNET COMMADN %d", ring_char(client, 1));
			switch ((unsigned char) ring_char(client, 1)) {
			case TELNET_WILL:
				telnet_handle_will(client, ring_char(client, 2));
				ring_eat(client, 3);
				break;
			case TELNET_WONT:
				telnet_handle_wont(client, ring_char(client, 2));
				ring_eat(client, 3);
				break;
			case TELNET_SB:
				complete = 0;
				for(iter = 2; !complete && iter < ring_avail(client); iter++) {
					subopt_buffer[iter - 2] = ring_char(client, iter);
					if(ring_char(client, iter) == TELNET_SE) {
						telnet_handle_subop(client, subopt_buffer, iter - 2);
						complete = 1;
						ring_eat(client, iter + 1);
					}
				}
				if(!complete)
					return;
				break;
			case TELNET_IAC:
				client->inputbuffer[client->inputtail++] = TELNET_IAC;
			default:
				ring_eat(client, 2);
				break;
			}
			break;
		case 0:
			ring_eat(client, 1);
			break;
		case '\r':
		case '\n':
			if(ring_length(client) > 1 && (ring_char(client, 1) == '\r' ||
										   ring_char(client, 1) == '\n')) {
				ring_eat(client, 1);
			}
			if(client->inputtail == 0) {
				ring_eat(client, 1);
				continue;
			}
			client->inputbuffer[client->inputtail] = 0;
			client_accept_input(client);
			dprintk("Would Parse: %s", client->inputbuffer);
			client->inputtail = 0;
			ring_eat(client, 1);
			break;
		default:
			client->inputbuffer[client->inputtail++] = ring_char(client, 0);
			ring_eat(client, 1);
			break;
		}
	}
	if(client->ringhead == client->ringtail) {
		client->ringhead = client->ringtail = 0;
	}

	telnet_check_options(client);
	dprintk
		("Ring Read finished. RingState = { head = %d, tail = %d }, InputState = { tail = %d }",
		 client->ringhead, client->ringtail, client->inputtail);
}

void telnet_disconnect(DESC * client)
{
	network_disconnect(client);
}

void telnet_write(DESC * client, char *buffer, int length)
{
	dprintk("telnet_write");
	// Color translation should occur here.
	network_write(client, buffer, length);
}
