
/* comsys.h */

/* $Id: comsys.h,v 1.1 2005/06/13 20:50:46 murrayma Exp $ */

#include "commac.h"
#ifdef CHANNEL_HISTORY
#include "myfifo.h"
#endif
#ifndef __COMSYS_H__
#define __COMSYS_H__

typedef struct chanentry CHANENT;
struct chanentry {
    char *channame;
    struct channel *chan;
};

#define CHAN_NAME_LEN 50
struct comuser {
    dbref who;
    int on;
    char *title;
    struct comuser *on_next;
};

struct channel {
    char name[CHAN_NAME_LEN];
    int type;
    int temp1;
    int temp2;
    int charge;
    int charge_who;
    int amount_col;
    int num_users;
    int max_users;
    int chan_obj;
    struct comuser **users;
    struct comuser *on_users;	/* Linked list of who is on */
#ifdef CHANNEL_HISTORY
    myfifo *last_messages;
#endif
    int num_messages;
};

typedef struct {
    time_t time;
    char *msg;
} chmsg;

int num_channels;
int max_channels;

/* some extern functions. */
extern int In_IC_Loc(dbref player);

#define CHANNEL_JOIN		0x001
#define CHANNEL_TRANSMIT	0x002
#define CHANNEL_RECIEVE		0x004

#define CHANNEL_PL_MULT		0x001
#define CHANNEL_OBJ_MULT	0x010
#define CHANNEL_LOUD		0x100
#define CHANNEL_PUBLIC		0x200
#define CHANNEL_TRANSPARENT	0x400

#define UNDEAD(x) (((!God(Owner(x))) || !(Going(x))) && \
	    ((Typeof(x) != TYPE_PLAYER) || (Connected(x))))

/* explanation of logic... If it's not owned by god, and it's either not a
player, or a connected player, it's good... If it is owned by god, then if
it's going, assume it's already gone, no matter what it is. :) */
#endif				/* __COMSYS_H__ */
