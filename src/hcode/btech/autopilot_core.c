
/*
 * $Id: autopilot_commands.c,v 1.2 2005/08/03 21:40:54 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 * Created: Wed Oct 30 20:42:59 1996 fingon
 * Last modified: Sat Jun  6 19:32:27 1998 fingon
 *
 */

#include "mech.h"
#include "mech.events.h"
#include "autopilot.h"
#include "coolmenu.h"
#include "mycool.h"
#include "p.mech.utils.h"

extern ACOM acom[AUTO_NUM_COMMANDS + 1];

#define AI_COMMAND_DLLIST_START     51      /* Tag for start of saved command list */
#define AI_COMMAND_DLLIST_END       63      /* Tag for end of saved command list */

#define outbyte(a) tmpb=(a); fwrite(&tmpb, 1, 1, f);

#define CHESA(a,b,c,d) if ((tmpb=fwrite(a,b,c,d)) != c) \
    { fprintf (stderr, "Error writing dllist\n"); \
      fflush(stderr); exit(1); }

#define CHELO(a,b,c,d) if ((tmpb=fread(a,b,c,d)) != c) \
    { fprintf (stderr, "Error loading dllist\n"); \
      fflush(stderr); exit(1); }

/*
 * Creates a new command_node for the AI's
 * command list
 */
static command_node *auto_create_command_node() {

    command_node *temp;

    temp = malloc(sizeof(command_node));
    if (temp == NULL)
        return NULL;

    memset(temp, 0, sizeof(command_node));
    temp->ai_command_function = NULL;

    return temp;

}

/*
 * Destroys a command_node
 */
void auto_destroy_command_node(command_node *node) {
   
    int i;

    /* Free the args */
    for (i = 0; i < AUTOPILOT_MAX_ARGS; i++) {
        if (node->args[i]) {
            free(node->args[i]);
            node->args[i] = NULL;
        }
    }

    /* Free the node */
    free(node);

    return;

}

/*
 * Writes a command_node to the file specified by f. Used to
 * save AI command list to the hcode.db file
 */
static void auto_write_command_node(FILE *f, command_node *node) {

    unsigned char size;         /* Number of Arguments to save */
    char buf[MBUF_SIZE];        /* Buffer to write the strings */
    int i;                      /* Counter */
    unsigned short tmpb;        /* Store the number of bytes written */

    /* Zero the Buffer */
    memset(buf, '\0', sizeof(buf));

    /* Write the Number of Arguments we're storing */
    size = node->argcount;
    CHESA(&size, 1, sizeof(size), f);

    /* Loop through the args and write them */
    for (i = 0; i <= size; i++) {
        strncpy(buf, node->args[i], MBUF_SIZE);
        CHESA(&buf, 1, sizeof(buf), f);
    }

    return;

}

/*
 * Reads the data for a command_node from a file. Used
 * to load the AI command list from hcode.db
 */
static command_node *auto_read_command_node(FILE *f) {

    unsigned char size;         /* Number of Arguments to read */
    char buf[MBUF_SIZE];        /* Buffer to store the strings */
    int i;                      /* Counter */
    unsigned short tmpb;        /* Store the number of bytes read */

    command_node *temp_command_node;

    /* Allocate a command node */
    temp_command_node = auto_create_command_node();

    /* Zero the Buffer */
    memset(buf, '\0', sizeof(buf));

    /* Read the Number of Arguments we're storing */
    CHELO(&size, 1, sizeof(size), f);
    temp_command_node->argcount = size;

    /* Loop through the arguments and store them */
    for (i = 0; i <= size; i++) {

        CHELO(&buf, 1, sizeof(buf), f);
        temp_command_node->args[i] = strndup(buf, MBUF_SIZE);

    }

    /* Make sure there is a command */
    if (!temp_command_node->args[0]) {
        fprintf(stderr, "Error loading command node from file - "
                "no command found\n");
        exit(1);
    }

    /* Get the command_enum and the command_function */
    for (i = 0; acom[i].name; i++) {
        if ((!strncmp(temp_command_node->args[0], acom[i].name, 
                        strlen(temp_command_node->args[0]))) &&
                (!strncmp(acom[i].name, temp_command_node->args[0], 
                        strlen(acom[i].name))))
            break;
    }

    if (!acom[i].name) {
        fprintf(stderr, "Error loading command node from file - "
                "Invalid Command\n");
        exit(1);
    }

    temp_command_node->command_enum = acom[i].command_enum;
    temp_command_node->ai_command_function = acom[i].ai_command_function;

    return temp_command_node;

}

/* 
 * Saves the current command list from the AI to
 * a file.  Called by SaveSpecialObjects from
 * glue.c
 */
void auto_save_commands(FILE *f, AUTO *a) {

    int i;                  /* Our Counter */
    unsigned char tmpb;     /* Our temp int we use when writing */
    unsigned int size;      /* The size of our command list */

    command_node *temp_command_node;

    /* Print the Start Code */
    outbyte(AI_COMMAND_DLLIST_START);

    /* Write the size of our list */
    size = dllist_size(a->commands);
    CHESA(&size, sizeof(size), 1, f);

    /* Check the size of the list, if there are commands save them */
    if (dllist_size(a->commands) > 0) {

        /* Ok there stuff here so lets write it */
        for (i = 1; i <= dllist_size(a->commands); i++) {
            temp_command_node = (command_node *) dllist_get_node(a->commands, i);
            auto_write_command_node(f, temp_command_node);
        }

    }

    /* Print the Stop Code */
    outbyte(AI_COMMAND_DLLIST_END);

    return;

}

/*
 * Loads an AI's saved command list from
 * a file.  Called by load_xcode in glue.c
 */
void auto_load_commands(FILE *f, AUTO *a) {

    int i;                  /* Our Counter */
    unsigned char tmpb;    /* Our temp int we use when reading */
    unsigned int size;      /* The size of our command list */

    dllist_node *temp_dllist_node;
    command_node *temp_command_node;

    /* Alloc a dllist to the AI for commands */
    a->commands = dllist_create_list();

    /* If we can't even read the file don't bother
     * with the rest */
    if(feof(f))
        return;

    /* Loop for the beginning tag */
    fread(&tmpb, 1, 1, f);
    if (tmpb != AI_COMMAND_DLLIST_START) {
        fprintf(stderr, "Unable to locate START for reading data"
                " for AI #%d\n", a->mynum);
        fflush(stderr);
        exit(1);
    }

    /* Read in size of dllist */
    CHELO(&size, sizeof(size), 1, f);

    /* if size is bigger then zero means we have to read
     * in some command nodes */
    if (size > 0) {

        /* Loop through the list and add the nodes */
        for (i = 1; i <= size; i++) {

            temp_command_node = auto_read_command_node(f);
            temp_dllist_node = dllist_create_node(temp_command_node);
            dllist_insert_end(a->commands, temp_dllist_node);

        }

    }

    /* Look for the end tag */
    fread(&tmpb, 1, 1, f);
    if (tmpb != AI_COMMAND_DLLIST_END) {
        fprintf(stderr, "Unable to locate START for reading data"
                " for AI #%d\n", a->mynum);
        fflush(stderr);
        exit(1);
    }

    return;

}

/* 
   The Autopilot command interface

   addcommand <name> [args]
   delcommand <num>
   listcommands
   engage
   disengage
   jump

 */

/*
 * The commands that are on the XCODE Object along
 * with some helper commands for modifying the state
 * of the AI
 */

/*! \todo {See if we need this function and remove it if not} */
int auto_valid_progline(AUTO * a, int p)
{
    int i;
#if 0
    for (i = 0; i < a->first_free; i += (acom[a->commands[i]].argcount + 1))
        if (i == p)
            return 1;
#endif
    return 0;
}

/*
 * Internal function to return a string that
 * displays a command from a command_node
 */
static char *auto_show_command(command_node *node) {
    
    static char buf[MBUF_SIZE];
    int i;

    snprintf(buf, MBUF_SIZE, "%-10s", node->args[0]);

    /* Loop through the args and print the commands */
    for (i = 1; i < AUTOPILOT_MAX_ARGS; i++)
        if (node->args[i])
            snprintf(buf + strnlen(buf, MBUF_SIZE - 1), 
                    MBUF_SIZE - strnlen(buf, MBUF_SIZE - 1),
                    " %s", node->args[i]);

    return buf;

}

/*
 * Removes a command from the AI's command list
 */
void auto_delcommand(dbref player, void *data, char *buffer) {

    int p, i;
    AUTO *a = (AUTO *) data;
    int remove_all_commands = 0;
    command_node *temp_command_node;
    char error_buf[MBUF_SIZE];

    /* Make sure they specified an argument */
    if (!*buffer) {
        notify(player, "No argument used : Usage delcommand [num]\n");
        notify_printf(player, "Must be within the range"
                " 1 to %d or -1 for all\n", dllist_size(a->commands));
        return;
    }

    /* Make sure its a number */
    if (Readnum(p, buffer)) {
        notify_printf(player, "Invalid Argument : Must be within the range"
                " 1 to %d or -1 for all\n", dllist_size(a->commands));
        return;
    }

    /* Check if its a valid command position
     * If its -1 means remove all */
    if (p == -1) {
        remove_all_commands = 1;
    } else if ((p > dllist_size(a->commands)) || (p < 1)) {
        notify_printf(player, "Invalid Argument : Must be within the range"
                " 1 to %d or -1 for all\n", dllist_size(a->commands));
        return;
    } 

    /* Now remove the node(s) */
    if (!remove_all_commands) {

        /* Remove the node at pos */
        temp_command_node = (command_node *) dllist_remove_node_at_pos(a->commands, p);

        if (!temp_command_node) {
            snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to remove"
                    " Command #%d from AI #%d but the command node doesn't exist\n",
                    p, a->mynum);
            SendAI(error_buf);
        }

        /* Destroy the command_node */
        auto_destroy_command_node(temp_command_node);

        notify_printf(player, "Command #%d Successfully Removed\n", p);

    } else {

        /* Remove ALL the commands */
        while (dllist_size(a->commands)) {

            /* Remove the first node on the list and get the data
             * from it */
            temp_command_node = (command_node *) dllist_remove(a->commands,
                    dllist_head(a->commands));

            /* Make sure the command node exists */
            if (!temp_command_node) {

                snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to remove"
                        " the first command from AI #%d but the command node doesn't exist\n",
                        a->mynum);
                SendAI(error_buf);

            } else {

                /* Destroy the command node */
                auto_destroy_command_node(temp_command_node);

            }

        }

        notify(player, "All the commands have been removed.\n");

    }

    return;

}

/*
 * Jump to a specific command location in the AI's
 * command list
 */
void auto_jump(dbref player, void *data, char *buffer)
{
    int p;
    AUTO *a = (AUTO *) data;

    notify(player, "jump has been temporarly disabled till I can figure out"
            " how I want to change it - Dany");
#if 0
    skipws(buffer);
    DOCHECK(!*buffer, "Argument expected!");
    DOCHECK(Readnum(p, buffer), "Invalid argument - single number expected.");
    /* Find out if it's valid position */
    DOCHECK(!auto_valid_progline(a, p),
            "Invalid : Argument out of range, or argument, not command.");
    PG(a) = p;
    notify(player, tprintf("Program Counter set to #%d.", p));
#endif
}

/*
 * Adds a command to the AI Command List
 */
void auto_addcommand(dbref player, void *data, char *buffer)
{
    AUTO *a = (AUTO *) data;
    char *args[AUTOPILOT_MAX_ARGS]; /* args[0] is the command the rest are 
                                       args for the command */
    char *command;                  /* temp string to get the name of the command */
    int argc;
    int i, j;

    command_node *temp_command_node;
    dllist_node *temp_dllist_node;

    /* Clear the Args */
    memset(args, 0, sizeof(char *) * 5);

    command = first_parseattribute(buffer);

    /* Look at the buffer and try and get the command */
    for (i = 0; acom[i].name; i++) {
        if ((!strncmp(command, acom[i].name, strlen(command))) &&
                (!strncmp(acom[i].name, command, strlen(acom[i].name))))
            break;
    }

    /* Free the command string we dont need it anymore */
    free(command);

    /* Make sure its a valid command */
    DOCHECK(!acom[i].name, "Invalid Command!");

    /* Get the arguments for the command */
    if (acom[i].argcount > 0) {

        /* Parse the buffer for commands
         * Its argcount + 1 because we are parsing the command + its
         * arguments */
        argc = proper_parseattributes(buffer, args, acom[i].argcount + 1);
        
        if (argc != acom[i].argcount + 1) {
   
            /* Free the args before we quit */ 
            for(j = 0; j < AUTOPILOT_MAX_ARGS; j++) {
                if(args[j])
                    free(args[j]);
            }
            notify(player, "Not the proper number of arguments!");
            return;

        }

    } else {

        /* Copy the command to the first arg */
        args[0] = strdup(acom[i].name);

    }

    /* Build the command node */
    temp_command_node = auto_create_command_node();

    for (j = 0; j < AUTOPILOT_MAX_ARGS; j++) {
        if (args[j])
            temp_command_node->args[j] = args[j];
    }

    temp_command_node->argcount = acom[i].argcount;
    temp_command_node->command_enum = acom[i].command_enum;
    temp_command_node->ai_command_function = acom[i].ai_command_function;

    /* Add the command to the list */
    temp_dllist_node = dllist_create_node(temp_command_node); 
    dllist_insert_end(a->commands, temp_dllist_node);

    /* Let the player know it worked */
    notify_printf(player, "Command Added: %s", auto_show_command(temp_command_node));

}

/*
 * Lists the various settings and commands currently on the AI
 */
void auto_listcommands(dbref player, void *data, char *buffer)
{
    AUTO *a = (AUTO *) data;
    coolmenu *c = NULL;
    char buf[MBUF_SIZE];
    int i, count = 0;

    addline();

    snprintf(buf, MBUF_SIZE, "Autopilot data for %s", Name(a->mynum));
    vsi(buf);

    snprintf(buf, MBUF_SIZE, "Controling unit %s", Name(Location(a->mynum)));
    vsi(buf);

    addline();

    snprintf(buf, MBUF_SIZE, "MyRef: #%d  MechRef: #%d  MapIndex: #%d  "
            "FSpeed: %d %% (Flag:%d)", a->mynum, a->mymechnum, a->mapindex, 
            a->speed, a->flags);
    vsi(buf);

    addline();

    if (dllist_size(a->commands)) {

        for (i = 1; i <= dllist_size(a->commands); i++) {
            snprintf(buf, MBUF_SIZE, "#%-3d %s", i,
                    auto_show_command((command_node *) dllist_get_node(a->commands, i)));
            vsi(buf);
        }

    } else {
        vsi("No commands have been queued to date.");
    }

    addline();
    ShowCoolMenu(player, c);
    KillCoolMenu(c);
}

/*
 * Turn the autopilot on
 */
static int auto_pilot_on(AUTO * a) {

    int i, j, count = 0;

    for (i = FIRST_AUTO_EVENT; i <= LAST_AUTO_EVENT; i++)
        if ((j = muxevent_count_type_data(i, (void *) a)))
            count += j;

    if (!count) {
        return a->flags & (AUTOPILOT_AUTOGUN | AUTOPILOT_GUNZOMBIE |
                AUTOPILOT_PILZOMBIE);
    }

    return count;

}

/*
 * Stop whatever the autopilot is doing
 */
static void auto_stop_pilot(AUTO * a) {

    int i;

    a->flags &= ~(AUTOPILOT_AUTOGUN | AUTOPILOT_GUNZOMBIE 
            | AUTOPILOT_PILZOMBIE);

    for (i = FIRST_AUTO_EVENT; i <= LAST_AUTO_EVENT; i++)
        muxevent_remove_type_data(i, (void *) a);

}

/*
 * Set the comtitle for the autopilot's unit
 */
void auto_set_comtitle(AUTO * a, MECH * mech) {
    
    char buf[LBUF_SIZE];

    snprintf(buf, LBUF_SIZE, "a=%s/%s", MechType_Ref(mech), MechIDS(mech, 1));
    mech_set_channeltitle(a->mynum, mech, buf);

}

/*
 * Set default parameters for the AI
 */
/*! \todo {Make this smarter and check some of these} */
void auto_init(AUTO * a, MECH * m) {

    a->auto_cmode = 1;      /* CHARGE! */
    a->auto_cdist = 2;      /* Attempt to avoid kicking distance */
    a->auto_nervous = 0;
    a->auto_goweight = 44;  /* We're mainly concentrating on fighting */
    a->auto_fweight = 55;
    a->speed = 100;         /* Reset to full speed */
    a->flags = 0;
    a->targ = -1;

}

/*
 * Setup all the flags and variables to current, then
 * start the AI's first command.
 */
void auto_engage(dbref player, void *data, char *buffer) {

    AUTO *a = (AUTO *) data;
    MECH *mech;

    a->mymech = mech = getMech((a->mymechnum = Location(a->mynum)));
    DOCHECK(!a, "Internal error! - Bad AI object!");
    DOCHECK(!mech, "Error: The autopilot isn't inside a 'mech!");
    DOCHECK(auto_pilot_on(a),
        "The autopilot's already online! You have to disengage it first.");

    if (MechAuto(mech) <= 0)
        auto_init(a, mech);
    MechAuto(mech) = a->mynum;
    
    if (MechAuto(mech) > 0)
        auto_set_comtitle(a, mech);

    a->mapindex = mech->mapindex;
    /*! \todo {Don't know if we still need this} */
    //a->program_counter = 0;

    notify(player, "Engaging autopilot...");
    AUTOEVENT(a, EVENT_AUTOCOM, auto_com_event, AUTOPILOT_NC_DELAY, 0);

    return;

}

/*
 * Turn off the autopilot
 */
void auto_disengage(dbref player, void *data, char *buffer) {

    AUTO *a = (AUTO *) data;

    DOCHECK(!auto_pilot_on(a),
            "The autopilot's already offline! You have to engage it first.");

    auto_stop_pilot(a);
    notify(player, "Autopilot has been disengaged.");

    return;

}

/*
 * Remove the first command_node in the list and go to the next
 */
void auto_goto_next_command(AUTO *a) {

    command_node *temp_command_node;
    char error_buf[MBUF_SIZE];

    if (dllist_size(a->commands) < 0) {
        snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to remove"
                " the first command from AI #%d but the command list is empty\n",
                a->mynum);
        SendAI(error_buf);
        return;
    }

    temp_command_node = (command_node *) dllist_remove(a->commands, 
            dllist_head(a->commands));

    if (!temp_command_node) {
        snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to remove"
                " the first command from AI #%d but the command node doesn't exist\n",
                a->mynum);
        SendAI(error_buf);
        return;
    }

    auto_destroy_command_node(temp_command_node);

}

/*
 * Get the argument for a given command position and argument number
 * Remember to free the string that this returns after use 
 */
char *auto_get_command_arg(AUTO *a, int command_number, int arg_number) {

    char *argument;
    command_node *temp_command_node;
    char error_buf[MBUF_SIZE];

    if (command_number > dllist_size(a->commands)) {
        snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to "
                "access Command #%d for AI #%d but it doesn't exist",
                a->mynum, command_number);
        SendAI(error_buf);
        return NULL;
    }

    if (arg_number >= AUTOPILOT_MAX_ARGS) {
        snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to "
                "access Arg #%d for AI #%d Command #%d but its greater"
                " then AUTOPILOT_MAX_ARGS (%d)",
                a->mynum, arg_number, command_number, AUTOPILOT_MAX_ARGS);
        SendAI(error_buf);
        return NULL;
    }

    temp_command_node = (command_node *) dllist_get_node(a->commands, 
                command_number);

    /*! \todo {Add in check incase the command node doesn't exist} */

    if (!temp_command_node->args[arg_number]) {
        snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to "
                "access Arg #%d for AI #%d Command #%d but it doesn't exist",
                a->mynum, arg_number, command_number);
        SendAI(error_buf);
        return NULL;
    }

    argument = strndup(temp_command_node->args[arg_number], MBUF_SIZE);

    return argument;

}

/*
 * Returns the command_enum value for the given command
 * from the AI command list
 */
int auto_get_command_enum(AUTO *a, int command_number) {

    int command_enum;
    command_node *temp_command_node;
    char error_buf[MBUF_SIZE];

    /* Make sure there are commands */
    if (dllist_size(a->commands) <= 0) {
        return -1;
    }

    if (command_number <= 0) {
        snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to "
                "access a command (%d) for AI #%d that can't be on a list",
                command_number, a->mynum);
        SendAI(error_buf);
        return -1;
    }

    /* Make sure the command is on the list */
    if (command_number > dllist_size(a->commands)) {
        snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to "
                "access Command #%d for AI #%d but it doesn't exist",
                a->mynum, command_number);
        SendAI(error_buf);
        return -1;
    }

    temp_command_node = (command_node *) dllist_get_node(a->commands, 
                command_number);

    /*! \todo {Add in check incase the command node doesn't exist} */

    command_enum = temp_command_node->command_enum;

    /* If its a bad enum value we have a problem */
    if ((command_enum >= AUTO_NUM_COMMANDS) || (command_enum < 0)) {
        snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Command ENUM for"
                " AI #%d Command Number #%d doesn't exist\n",
                a->mynum, command_number);
        SendAI(error_buf);
        return -1;
    }

    return command_enum;

}
