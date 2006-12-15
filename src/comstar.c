/*
 * comstar.c
 */

#include "copyright.h"
#include "config.h"
#include "mudconf.h"
#include "externs.h"
#include "powers.h"
#include "rbtree.h"
#include "debug.h"


#define CHAN_NAME_LEN 50

struct history_message {
    time_t time;
    char *msg;
    struct history_message *next;
};

struct channel { 
    char name[50];
    int type;
    int charge;
    int owner;
    int amount_collected;
    int n_users;
    rbtree users;
    int channel_object;
    int n_history;
    struct history_message *history;
};

struct channel_user {
    dbref player;
    char *title;
    char *alias;
    struct channel *channel;
};

static rbtree channels = NULL;
static rbtree channel_users = NULL;

static int comstar_compare_channel(void *vleft, void *vright, void *arg) {
    char *left = (char *)vleft;
    char *right= (char *)vright;

    return strcmp(left, right);
}

static int comstar_compare_channel_user(void *vleft, void *vright, void *arg) {
    int left = (int) vleft;
    int right = (int) vright;

    return (left-right);
}

static int comstar_compare_alias(void *vleft, void *vright, void *arg) {
    char *left = (char *)vleft;
    char *right = (char *)vright;

    return strcasecmp(left, right);
}

static void comstar_init(void) {
    channels = rb_init(comstar_compare_channel, NULL);
    channel_users = rb_init(comstar_compare_channel_user, NULL);
}

static rbtree comstar_find_user_channels(dbref player) {
    rbtree uchan = NULL;

    uchan = (rbtree) rb_find(channel_users, (void *)player);
    if(!uchan) {
        uchan = (rbtree) rb_init(comstar_compare_alias, NULL);
        rb_insert(channel_users, (void *)player, uchan);
    }
    return uchan;
}

static struct channel *comstar_find_channel_by_name(char *name) {
    struct channel *channel = NULL;

    channel = rb_find(channels, name);
    return channel;
}

static struct channel *comstar_find_channel_by_user_alias(dbref player, char *alias) {
    rbtree uchan = NULL;
    struct channel_user *channel_user = NULL;
    struct channel *channel = NULL;

    uchan = comstar_find_user_channels(player);
    if(!rb_exists(uchan, (void *)alias)) 
        return NULL;

    channel_user = (struct channel_user *)rb_find(uchan, (void *)alias);
    return channel_user->channel;
}

static int comstar_add_user_to_channel(char *name, char *alias, dbref player) {
    struct channel *channel = NULL;
    struct channel_user *channel_user = NULL;
    rbtree uchan = NULL;

    channel = comstar_find_channel_by_name(name);
    if(!channel)
        return 0;

    if(rb_exists(channel->users, (void *)player)) 
        return 0;

    uchan = comstar_find_user_channels(player);
    if(!uchan)
        return 0;

    if(rb_exists(uchan, (void *)alias)) 
        return 0;

    channel_user = malloc(sizeof(struct channel_user));
    memset(channel_user, 0, sizeof(struct channel_user));

    channel_user->player = player;
    channel_user->alias = strdup(alias);
    channel_user->channel = channel;
    channel_user->title = strdup(""); 

    rb_insert(uchan, (void *)channel_user->alias, (void *)channel_user);
    rb_insert(channel->users, (void *)channel_user->player, (void *)channel_user);
    return 0;
}


static int comstar_remove_user_from_channel(char *name, dbref player) {
    struct channel *channel = NULL;
    struct channel_user *channel_user = NULL;
    rbtree uchan = NULL;
    channel = comstar_find_channel_by_name(name);
    if(!channel)
        return 0;

    uchan = comstar_find_user_channels(player);
    if(!uchan)
        return 0;

    channel_user = (struct channel_user *)rb_find(channel->users, (void *)player);
    if(!channel_user)
        return 0;

    rb_delete(channel->users, (void *)player);

    if(!channel_user->alias) 
        return 0;
    rb_delete(uchan, (void *)channel_user->alias);

    if(channel_user->title)
        free(channel_user->title);
    if(channel_user->alias)
        free(channel_user->alias);
    
    memset(channel_user, 0, sizeof(struct channel_user));
    free(channel_user);

    return 1;
}

void do_createchannel(dbref player, dbref cause, int key, char *channame) {
    struct channel *newchannel = NULL;

    if(!mudconf.have_comsys) {
        raw_notify(player, "Comsys disabled.");
        return;
    }
    if(comstar_find_channel_by_name(channame)) {
        notify_printf(player, "Channel %s already exists.", channame);
        return;
    }
    if(!channame || !*channame) {
        raw_notify(player, "You must specify a channel to create.");
        return;
    }
    if(!(Comm_All(player))) {
        raw_notify(player, "You do not have permission to do that.");
        return;
    }
    if(strlen(channame) > (CHAN_NAME_LEN-1)) {
        raw_notify(player, "Name too long.");
        return;
    }
    newchannel = (struct channel *) malloc(sizeof(struct channel));
    memset(newchannel, 0, sizeof(struct channel));

    strncpy(newchannel->name, channame, CHAN_NAME_LEN - 1);

    newchannel->type = 127;
    newchannel->owner = player;
    newchannel->channel_object = NOTHING;
    newchannel->users = rb_init(comstar_compare_channel_user, NULL);
    notify_printf(player, "Channel %s created.", channame);
}

void do_destroychannel(dbref player, dbref cause, int key, char *channame) {
    struct channel *channel = NULL;

    if(!mudconf.have_comsys) {
        raw_notify(player, "Comsys disabled.");
        return;
    }
    if(!channame || !*channame) {
        raw_notify(player, "No argument supplied.");
        return;
    }
    channel = comstar_find_channel_by_name(channame);

    if(!channel) {
        notify_printf(player, "Could not find channel %s.", channel);
        return;
    } else if(!(Comm_All(player)) && (player != channel->owner)) {
        raw_notify(player, "You do not have permission to do that. ");
        return;
    }
    while(rb_size(channel->users) > 1) {
        struct channel_user *channel_user = NULL;
        channel_user = rb_search(channel->users, SEARCH_FIRST, NULL);
        if(!channel_user) break;
        if(!comstar_remove_user_from_channel(channel->name, channel_user->player))
            break;
    }
    rb_delete(channels, channame);
    memset(channel, 0, sizeof(struct channel));
    free(channel);
}

void do_addcom(dbref player, dbref cause, int key, char *arg1, char *arg2) {
    struct channel *channel = NULL;
    if(!mudconf.have_comsys) {
        raw_notify(player, "Comsys disabled.");
        return;
    }

    if(!arg1 || !*arg1) {
        raw_notify(player, "You need to specify an alias.");
        return;
    }

    if(!arg2 || !*arg2) {
        raw_notify(player, "You need to specify a channel.");
        return;
    }
    
    channel = comstar_find_channel_by_name(arg2);
    if(!channel) {
        raw_notify(player, "Sorry, Channel does not exist.");
        return;
    }

    if(!comstar_add_user_to_channel(arg2, arg1, player)) {
        raw_notify(player, "Operation failed due to invalid argument.");
        return;
    }
    raw_notify(player, "Operation succeeded.");
    return;
}

void do_delcom(dbref player, dbref cause, int key, char *arg1) {
    struct channel *channel = NULL;

    if(!mudconf.have_comsys) {
        raw_notify(player, "Comsys disabled.");
        return;
    }
    if(!arg1 || !*arg1) {
        raw_notify(player, "Need an alias to delete.");
        return;
    }

    channel = comstar_find_channel_by_user_alias(player, arg1);
    if(!channel) {
        raw_notify(player, "Channel alias does not exist.");
        return;
    }

    if(!comstar_remove_user_from_channel(channel->name, player)) {
        raw_notify(player, "Operation failed due to invalid argument.");
        return;
    }

    raw_notify(player, "Operation succeeded.");
    return;
}


