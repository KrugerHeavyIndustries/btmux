
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

extern ACOM acom[NUM_COMMANDS + 1];

#define AI_COMMAND_DLLIST_START     51 
#define AI_COMMAND_DLLIST_END       63 

#define outbyte(a) tmpb=(a);fwrite(&tmpb, 1, 1, f);

#define CHESA(a,b,c,d) if ((tmpb=fwrite(a,b,c,d)) != c) \
    { fprintf (stderr, "Error writing dllist\n"); \
      fflush(stderr); exit(1); }

#define CHELO(a,b,c,d) if ((tmpb=fread(a,b,c,d)) != c) \
    { fprintf (stderr, "Error loading dllist\n"); \
      fflush(stderr); exit(1); }

/* Dynamic Command System - Malloc a Command Node */
command_node *command_create_node() {

    command_node *temp;

    temp = malloc(sizeof(command_node));
    if (temp == NULL)
        return NULL;

    memset(temp, 0, sizeof(command_node));

    return temp;

}

/* The function that actually writes a command node to file */
void auto_write_command_node(FILE *f, command_node *node) {

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

/* Function to read in a command node from a file */
command_node *auto_read_command_node(FILE *f) {

    unsigned char size;         /* Number of Arguments to read */
    char buf[MBUF_SIZE];        /* Buffer to store the strings */
    int i;                      /* Counter */
    unsigned short tmpb;        /* Store the number of bytes read */

    command_node *temp_command_node;

    /* Allocate a command node */
    temp_command_node = command_create_node();

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

    return temp_command_node;

}

/* Save Command Dllist Here */
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

/* Read in ai's command list */
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

/* The commands for modifying state of autopilot */

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

static char *auto_show_command(command_node *node)
{
    static char buf[MBUF_SIZE];
    int i;

    snprintf(buf, MBUF_SIZE, "%-10s", node->args[0]);

    for (i = 1; i < 5; i++)
        if (node->args[i])
            snprintf(buf + strnlen(buf, MBUF_SIZE - 1), 
                    MBUF_SIZE - strnlen(buf, MBUF_SIZE - 1),
                    " %s", node->args[i]);

    return buf;

}

void auto_delcommand(dbref player, void *data, char *buffer)
{
    int p, i, len;
    AUTO *a = (AUTO *) data;
    int repeat = 0;
    int doall = 0;
#if 0
    skipws(buffer);
    DOCHECK(!*buffer, "Argument expected!");
    DOCHECK(Readnum(p, buffer), "Invalid argument - single number expected.");
    if (p >= 0) {
        /* Find out if it's valid position */
        DOCHECK(!auto_valid_progline(a, p),
                "Invalid : Argument out of range, or argument, not command.");
    } else {
        doall = 1;
        p = 0;
    }

    while (a->first_free > 0 && (doall || !(repeat++))) {
        /* Remove it */
        len = acom[a->commands[p]].argcount + 1;
        for (i = p + len; i < a->first_free; i++)
            a->commands[i - len] = a->commands[i];
        
        if (PG(a) >= p)
            PG(a) -= len;
        
        a->first_free -= len;
        for (i = p; i < a->first_free; i += (acom[a->commands[i]].argcount + 1))
            if (a->commands[i] == COMMAND_JUMP)
                a->commands[i + 1] -= len;

        notify(player, tprintf("Command #%d deleted (%d words freed)!", p, len));
    }
#endif
}

void auto_jump(dbref player, void *data, char *buffer)
{
    int p;
    AUTO *a = (AUTO *) data;
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

void auto_addcommand(dbref player, void *data, char *buffer)
{
    AUTO *a = (AUTO *) data;
    char *args[5];              /* At most will have 4 args + command */
    int argc;
    int i, j;

    command_node *temp_command_node;
    dllist_node *temp_dllist_node;

    /* Clear the Args */
    memset(args, 0, sizeof(char *) * 5);

    /* Look at the buffer and try and get the command */
    for (i = 0; acom[i].name; i++) {
        if (!strncmp(buffer, acom[i].name, strlen(acom[i].name)))
            break;
    }

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
            for(j = 0; j < 5; j++) {
                if(args[j]) free(args[j]);
            }
            notify(player, "Not the proper number of arguments!");
            return;

        }

    } else {

        /* Copy the command to the first arg */
        args[0] = strdup(acom[i].name);

    }

    /* Build the command node */
    temp_command_node = command_create_node();

    for (j = 0; j < 5; j++) {
        if (args[j])
            temp_command_node->args[j] = args[j];
    }

    temp_command_node->argcount = acom[i].argcount;
    temp_command_node->ai_command_function = acom[i].ai_command_function;

    /* Add the command to the list */
    temp_dllist_node = dllist_create_node(temp_command_node); 
    dllist_insert_end(a->commands, temp_dllist_node);

    /* Let the player know it worked */
    notify_printf(player, "Command Added: %s", auto_show_command(temp_command_node));

}

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
/*
    sim(tprintf("Memory: %d (%d %%) used / %d free", a->first_free,
            100 * a->first_free / AUTOPILOT_MEMORY,
            AUTOPILOT_MEMORY - a->first_free), CM_TWO);
    sim(tprintf("Program counter: #%d %s", a->program_counter,
            a->program_counter >=
            a->first_free ? " (End of program)" : ""), CM_TWO);
*/
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

int AutoPilotOn(AUTO * a)
{

    int i, j, count = 0;
#if 0
    for (i = FIRST_AUTO_EVENT; i <= LAST_AUTO_EVENT; i++)
        if ((j = muxevent_count_type_data(i, (void *) a)))
            count += j;

    if (!count)
        return a->flags & (AUTOPILOT_AUTOGUN | AUTOPILOT_GUNZOMBIE |
                AUTOPILOT_PILZOMBIE);
#endif
    return count;

}

void StopAutoPilot(AUTO * a)
{
    int i;
#if 0
    a->flags &=
            ~(AUTOPILOT_AUTOGUN | AUTOPILOT_GUNZOMBIE | AUTOPILOT_PILZOMBIE);
    for (i = FIRST_AUTO_EVENT; i <= LAST_AUTO_EVENT; i++)
        muxevent_remove_type_data(i, (void *) a);
#endif
}

/* Main idea: Set up all variables to current (if possible),
   and engage our first execute-commands-event 
 */

void ai_set_comtitle(AUTO * a, MECH * mech)
{
    char buf[LBUF_SIZE];
#if 0
    sprintf(buf, "a=%s/%s", MechType_Ref(mech), MechIDS(mech, 1));
    mech_set_channeltitle(a->mynum, mech, buf);
#endif
}

void auto_engage(dbref player, void *data, char *buffer) {

    AUTO *a = (AUTO *) data;
    MECH *mech;
#if 0
    a->mymech = mech = getMech((a->mymechnum = Location(a->mynum)));
    DOCHECK(!a, "Internal error!");
    DOCHECK(!mech, "Error: The autopilot isn't inside a 'mech!");
    DOCHECK(AutoPilotOn(a),
        "The autopilot's already online! You have to disengage it first.");
    if (MechAuto(mech) <= 0)
        ai_init(a, mech);
    MechAuto(mech) = a->mynum;
    if (MechAuto(mech) > 0)
        ai_set_comtitle(a, mech);
    a->mapindex = mech->mapindex;
    a->program_counter = 0;
/*  a->speed = 100; */

#if 0
    DOCHECK(a->mapindex < 0 ||
            !FindObjectsData(a->mapindex), "The 'mech is on invalid map!");
#endif

    notify(player, "Engaging autopilot..");
    AUTOEVENT(a, EVENT_AUTOCOM, auto_com_event, AUTOPILOT_NC_DELAY, 0);
#endif
}

void auto_disengage(dbref player, void *data, char *buffer)
{
    AUTO *a = (AUTO *) data;
#if 0
    DOCHECK(!AutoPilotOn(a),
            "The autopilot's already offline! You have to engage it first.");
    StopAutoPilot(a);
    notify(player, "Autopilot has been disengaged.");
#endif
}
