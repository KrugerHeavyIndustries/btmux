/*
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry
 *       All rights reserved
 *
 */

#include "mech.h"
#include "mech.events.h"
#include "autopilot.h"
#include "coolmenu.h"
#include "mycool.h"
#include "p.mech.utils.h"

extern ACOM acom[AUTO_NUM_COMMANDS + 1];
extern char *muxevent_names[];

#define AI_COMMAND_DLLIST_START     51	/* Tag for start of saved command list */
#define AI_COMMAND_DLLIST_END       63	/* Tag for end of saved command list */

#define outbyte(a) tmpb=(a); fwrite(&tmpb, 1, 1, file);

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
static command_node *auto_create_command_node()
{

	command_node *temp;

	temp = malloc(sizeof(command_node));
	if(temp == NULL)
		return NULL;

	memset(temp, 0, sizeof(command_node));
	temp->ai_command_function = NULL;

	return temp;

}

/*
 * Destroys a command_node
 */
void auto_destroy_command_node(command_node * node)
{

	int i;

	/* Free the args */
	for(i = 0; i < AUTOPILOT_MAX_ARGS; i++) {
		if(node->args[i]) {
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
static void auto_write_command_node(FILE * file, command_node * node)
{

	unsigned char size;			/* Number of Arguments to save */
	char buf[MBUF_SIZE];		/* Buffer to write the strings */
	int i;						/* Counter */
	unsigned short tmpb;		/* Store the number of bytes written */

	/* Zero the Buffer */
	memset(buf, '\0', sizeof(buf));

	/* Write the Number of Arguments we're storing */
	size = node->argcount;
	CHESA(&size, 1, sizeof(size), file);

	/* Loop through the args and write them */
	for(i = 0; i <= size; i++) {
		strncpy(buf, node->args[i], MBUF_SIZE);
		CHESA(&buf, 1, sizeof(buf), file);
	}

	return;

}

/*
 * Reads the data for a command_node from a file. Used
 * to load the AI command list from hcode.db
 */
static command_node *auto_read_command_node(FILE * file)
{

	unsigned char size;			/* Number of Arguments to read */
	char buf[MBUF_SIZE];		/* Buffer to store the strings */
	int i;						/* Counter */
	unsigned short tmpb;		/* Store the number of bytes read */

	command_node *temp_command_node;

	/* Allocate a command node */
	temp_command_node = auto_create_command_node();

	/* Zero the Buffer */
	memset(buf, '\0', sizeof(buf));

	/* Read the Number of Arguments we're storing */
	CHELO(&size, 1, sizeof(size), file);
	temp_command_node->argcount = size;

	/* Loop through the arguments and store them */
	for(i = 0; i <= size; i++) {

		CHELO(&buf, 1, sizeof(buf), file);
		temp_command_node->args[i] = strndup(buf, MBUF_SIZE);

	}

	/* Make sure there is a command */
	if(!temp_command_node->args[0]) {
		fprintf(stderr, "Error loading command node from file - "
				"no command found\n");
		exit(1);
	}

	/* Get the command_enum and the command_function */
	for(i = 0; acom[i].name; i++) {
		if((!strncmp(temp_command_node->args[0], acom[i].name,
					 strlen(temp_command_node->args[0]))) &&
		   (!strncmp(acom[i].name, temp_command_node->args[0],
					 strlen(acom[i].name))))
			break;
	}

	if(!acom[i].name) {
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
void auto_save_commands(FILE * file, AUTO * autopilot)
{

	int i;						/* Our Counter */
	unsigned char tmpb;			/* Our temp int we use when writing */
	unsigned int size;			/* The size of our command list */

	command_node *temp_command_node;

	/* Print the Start Code */
	outbyte(AI_COMMAND_DLLIST_START);

	/* Write the size of our list */
	size = dllist_size(autopilot->commands);
	CHESA(&size, sizeof(size), 1, file);

	/* Check the size of the list, if there are commands save them */
	if(dllist_size(autopilot->commands) > 0) {

		/* Ok there stuff here so lets write it */
		for(i = 1; i <= dllist_size(autopilot->commands); i++) {
			temp_command_node =
				(command_node *) dllist_get_node(autopilot->commands, i);
			auto_write_command_node(file, temp_command_node);
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
void auto_load_commands(FILE * file, AUTO * autopilot)
{

	int i;						/* Our Counter */
	unsigned char tmpb;			/* Our temp int we use when reading */
	unsigned int size;			/* The size of our command list */

	dllist_node *temp_dllist_node;
	command_node *temp_command_node;

	/* Alloc a dllist to the AI for commands */
	autopilot->commands = dllist_create_list();

	/* If we can't even read the file don't bother
	 * with the rest */
	if(feof(file))
		return;

	/* Loop for the beginning tag */
	fread(&tmpb, 1, 1, file);
	if(tmpb != AI_COMMAND_DLLIST_START) {
		fprintf(stderr, "Unable to locate START for reading data"
				" for AI #%d\n", autopilot->mynum);
		fflush(stderr);
		exit(1);
	}

	/* Read in size of dllist */
	CHELO(&size, sizeof(size), 1, file);

	/* if size is bigger then zero means we have to read
	 * in some command nodes */
	if(size > 0) {

		/* Loop through the list and add the nodes */
		for(i = 1; i <= size; i++) {

			temp_command_node = auto_read_command_node(file);
			temp_dllist_node = dllist_create_node(temp_command_node);
			dllist_insert_end(autopilot->commands, temp_dllist_node);

		}

	}

	/* Look for the end tag */
	fread(&tmpb, 1, 1, file);
	if(tmpb != AI_COMMAND_DLLIST_END) {
		fprintf(stderr, "Unable to locate END for reading data"
				" for AI #%d\n", autopilot->mynum);
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

 */

/*
 * The commands that are on the XCODE Object along
 * with some helper commands for modifying the state
 * of the AI
 */

/*
 * Internal function to return a string that
 * displays a command from a command_node
 */
/*! \todo {Maybe re-write this so doesn't use a static buffer} */
static char *auto_show_command(command_node * node)
{

    static char buf[MBUF_SIZE];
    int i;

    snprintf(buf, MBUF_SIZE, "%-10s", node->args[0]);

    /* Loop through the args and print the commands */
    for (i = 1; i < AUTOPILOT_MAX_ARGS; i++)
        if (node->args[i]) {
            strncat(buf, " ", MBUF_SIZE);
            strncat(buf, node->args[i], MBUF_SIZE);
        }

    return buf;

}

/*
 * Removes a command from the AI's command list
 */
void auto_delcommand(dbref player, void *data, char *buffer)
{

    int p, i;
    AUTO *autopilot = (AUTO *) data;
    int remove_all_commands = 0;
    command_node *temp_command_node;
    char error_buf[MBUF_SIZE];

    /* Make sure they specified an argument */
    if (!*buffer) {
        notify(player, "No argument used : Usage delcommand [num]\n");
        notify_printf(player, "Must be within the range"
                " 1 to %d or -1 for all\n",
                dllist_size(autopilot->commands));
        return;
    }

    /* Make sure its a number */
    if (Readnum(p, buffer)) {
        notify_printf(player, "Invalid Argument : Must be within the range"
                " 1 to %d or -1 for all\n",
                dllist_size(autopilot->commands));
        return;
    }

    /* Check if its a valid command position
     * If its -1 means remove all */
    if (p == -1) {
        remove_all_commands = 1;
    } else if ((p > dllist_size(autopilot->commands)) || (p < 1)) {
        notify_printf(player, "Invalid Argument : Must be within the range"
                " 1 to %d or -1 for all\n",
                dllist_size(autopilot->commands));
        return;
    }

    /*! \todo {Add in check so they don't accidently remove a running command without
     * disengaging first} */

    /* Now remove the node(s) */
    if (!remove_all_commands) {

        /* Remove the node at pos */
        temp_command_node =
            (command_node *) dllist_remove_node_at_pos(autopilot->commands,
                                                       p);

        if (!temp_command_node) {
            snprintf(error_buf, MBUF_SIZE,
                    "Internal AI Error: Trying to remove"
                    " Command #%d from AI #%d but the command node doesn't exist\n",
                    p, autopilot->mynum);
            SendAI(error_buf);
        }

        /* Destroy the command_node */
        auto_destroy_command_node(temp_command_node);

        notify_printf(player, "Command #%d Successfully Removed\n", p);

    } else {

        /* Remove ALL the commands */
        while (dllist_size(autopilot->commands)) {

            /* Remove the first node on the list and get the data
             * from it */
            temp_command_node =
                (command_node *) dllist_remove(autopilot->commands,
                                               dllist_head(autopilot->
                                                   commands));

            /* Make sure the command node exists */
            if (!temp_command_node) {

                snprintf(error_buf, MBUF_SIZE,
                        "Internal AI Error: Trying to remove"
                        " the first command from AI #%d but the command node doesn't exist\n",
                        autopilot->mynum);
                SendAI(error_buf);

            } else {

                /* Destroy the command node */
                auto_destroy_command_node(temp_command_node);

            }

        }

        notify(player, "All the commands have been removed.\n");

    }

}

/*
 * Adds a command to the AI Command List
 */
void auto_addcommand(dbref player, void *data, char *buffer)
{

    AUTO *autopilot = (AUTO *) data;
    char *args[AUTOPILOT_MAX_ARGS]; /* args[0] is the command the rest are 
                                       args for the command */
    char *command;                  /* temp string to get the name of the command */
    int argc;
    int i, j;

    command_node *temp_command_node;
    dllist_node *temp_dllist_node;

    /* Clear the Args */
    memset(args, 0, sizeof(char *) * AUTOPILOT_MAX_ARGS);

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
        argc = proper_explodearguments(buffer, args, acom[i].argcount + 1);

        if (argc != acom[i].argcount + 1) {

            /* Free the args before we quit */
            for (j = 0; j < AUTOPILOT_MAX_ARGS; j++) {
                if (args[j])
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
    dllist_insert_end(autopilot->commands, temp_dllist_node);

    /* Let the player know it worked */
    notify_printf(player, "Command Added: %s",
            auto_show_command(temp_command_node));

}

/*
 * Lists the various settings and commands currently on the AI
 */
void auto_listcommands(dbref player, void *data, char *buffer)
{

    AUTO *autopilot = (AUTO *) data;
    coolmenu *c = NULL;
    char buf[MBUF_SIZE];
    int i, count = 0;

    addline();

    snprintf(buf, MBUF_SIZE, "Autopilot data for %s", Name(autopilot->mynum));
    vsi(buf);

    snprintf(buf, MBUF_SIZE, "Controling unit %s",
            Name(Location(autopilot->mynum)));
    vsi(buf);

    addline();

    snprintf(buf, MBUF_SIZE, "MyRef: #%d  MechRef: #%d  MapIndex: #%d  "
            "FSpeed: %d %% (Flag:%d)", autopilot->mynum,
            autopilot->mymechnum, autopilot->mapindex, autopilot->speed,
            autopilot->flags);
    vsi(buf);

    addline();

    if (dllist_size(autopilot->commands)) {

        for (i = 1; i <= dllist_size(autopilot->commands); i++) {
            snprintf(buf, MBUF_SIZE, "#%-3d %s", i,
                    auto_show_command((command_node *)
                        dllist_get_node(autopilot->commands,
                            i)));
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
static int auto_pilot_on(AUTO * autopilot)
{

	int i, j, count = 0;

	for(i = FIRST_AUTO_EVENT; i <= LAST_AUTO_EVENT; i++)
		if((j = muxevent_count_type_data(i, (void *) autopilot)))
			count += j;

	if(!count) {
		return autopilot->flags & (AUTOPILOT_AUTOGUN | AUTOPILOT_GUNZOMBIE |
								   AUTOPILOT_PILZOMBIE);
	}

	return count;

}

/*
 * Stop whatever the autopilot is doing
 */
extern void auto_stop_pilot(AUTO * autopilot)
{

	int i;

	autopilot->flags &= ~(AUTOPILOT_AUTOGUN | AUTOPILOT_GUNZOMBIE
						  | AUTOPILOT_PILZOMBIE);

	for(i = FIRST_AUTO_EVENT; i <= LAST_AUTO_EVENT; i++)
		muxevent_remove_type_data(i, (void *) autopilot);

}

/*
 * Set the comtitle for the autopilot's unit
 */
void auto_set_comtitle(AUTO * autopilot, MECH * mech)
{

	char buf[LBUF_SIZE];

	snprintf(buf, LBUF_SIZE, "a=%s/%s", MechType_Ref(mech), MechIDS(mech, 1));
	mech_set_channeltitle(autopilot->mynum, mech, buf);

}

/*
 * Set default parameters for the AI
 */
/*! \todo {Make this smarter and check some of these} */
void auto_init(AUTO * autopilot, MECH * mech)
{

	autopilot->ofsx = 0;		/* Positional - angle */
	autopilot->ofsy = 0;		/* Positional - distance */
	autopilot->auto_cmode = 1;	/* CHARGE! */
	autopilot->auto_cdist = 2;	/* Attempt to avoid kicking distance */
	autopilot->auto_nervous = 0;
	autopilot->auto_goweight = 44;	/* We're mainly concentrating on fighting */
	autopilot->auto_fweight = 55;
	autopilot->speed = 100;		/* Reset to full speed */
	autopilot->flags = 0;

	/* Target Stuff */
	autopilot->target = -1;
	autopilot->target_score = 0;
	autopilot->target_threshold = 50;

	/* Follow & Chase target stuff */
	autopilot->chase_target = -10;
	autopilot->chasetarg_update_tick = AUTOPILOT_CHASETARG_UPDATE_TICK;
	autopilot->follow_update_tick = AUTOPILOT_FOLLOW_UPDATE_TICK;

}

/*
 * Remove the first command_node in the list and go to the next
 */
void auto_goto_next_command(AUTO * autopilot, int time)
{

	command_node *temp_command_node;
	char error_buf[MBUF_SIZE];

	if(dllist_size(autopilot->commands) < 0) {
		snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to remove"
				 " the first command from AI #%d but the command list is empty\n",
				 autopilot->mynum);
		SendAI(error_buf);
		return;
	}

	temp_command_node = (command_node *) dllist_remove(autopilot->commands,
													   dllist_head(autopilot->
																   commands));

	if(!temp_command_node) {
		snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to remove"
				 " the first command from AI #%d but the command node doesn't exist\n",
				 autopilot->mynum);
		SendAI(error_buf);
		return;
	}

	auto_destroy_command_node(temp_command_node);

	/* Fire off the AUTO_COM event */
	AUTO_COM(autopilot, time);

}

/*
 * Get the argument for a given command position and argument number
 * Remember to free the string that this returns after use 
 */
char *auto_get_command_arg(AUTO * autopilot, int command_number,
        int arg_number)
{

	char *argument;
	command_node *temp_command_node;
	char error_buf[MBUF_SIZE];

	if(command_number > dllist_size(autopilot->commands)) {
		snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to "
				 "access Command #%d for AI #%d but it doesn't exist",
				 command_number, autopilot->mynum);
		SendAI(error_buf);
		return NULL;
	}

	if(arg_number >= AUTOPILOT_MAX_ARGS) {
		snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to "
				 "access Arg #%d for AI #%d Command #%d but its greater"
				 " then AUTOPILOT_MAX_ARGS (%d)",
				 arg_number, autopilot->mynum, command_number,
				 AUTOPILOT_MAX_ARGS);
		SendAI(error_buf);
		return NULL;
	}

	temp_command_node = (command_node *) dllist_get_node(autopilot->commands,
														 command_number);

	/*! \todo {Add in check incase the command node doesn't exist} */

	if(!temp_command_node->args[arg_number]) {
		snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to "
				 "access Arg #%d for AI #%d Command #%d but it doesn't exist",
				 autopilot->mynum, arg_number, command_number);
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
int auto_get_command_enum(AUTO * autopilot, int command_number)
{

	int command_enum;
	command_node *temp_command_node;
	char error_buf[MBUF_SIZE];

	/* Make sure there are commands */
	if(dllist_size(autopilot->commands) <= 0) {
		return -1;
	}

	if(command_number <= 0) {
		snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to "
				 "access a command (%d) for AI #%d that can't be on a list",
				 command_number, autopilot->mynum);
		SendAI(error_buf);
		return -1;
	}

	/* Make sure the command is on the list */
	if(command_number > dllist_size(autopilot->commands)) {
		snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Trying to "
				 "access Command #%d for AI #%d but it doesn't exist",
				 autopilot->mynum, command_number);
		SendAI(error_buf);
		return -1;
	}

	temp_command_node = (command_node *) dllist_get_node(autopilot->commands,
														 command_number);

	/*! \todo {Add in check incase the command node doesn't exist} */

	command_enum = temp_command_node->command_enum;

	/* If its a bad enum value we have a problem */
	if((command_enum >= AUTO_NUM_COMMANDS) || (command_enum < 0)) {
		snprintf(error_buf, MBUF_SIZE, "Internal AI Error: Command ENUM for"
				 " AI #%d Command Number #%d doesn't exist\n",
				 autopilot->mynum, command_number);
		SendAI(error_buf);
		return -1;
	}

	return command_enum;

}

/*
 * Setup all the flags and variables to current, then
 * start the AI's first command.
 */
void auto_engage(dbref player, void *data, char *buffer)
{

    AUTO *autopilot = (AUTO *) data;
    MECH *mech;

    /* Make sure its an AI */
    if (!autopilot && !IsAuto(autopilot->mynum)) {
        notify(player, "Internal error! - Bad AI Object");
        /* Insert error message to somewhere */
        /* Insert Cleanup */
        return;
    }

    /* Set the mech number as the current location */
    autopilot->mymechnum = Location(autopilot->mynum);

    /* getMech checks to make sure its a valid obj and mech */
    if (!(mech = getMech(autopilot->mymechnum))) {
        notify(player, "AI not in a mech ... try moving it or putting it in one");
        /* No mech so return - do something about AI cleanup? */
        return;
    }

    /* Mech Destroyed ? */
    if (Destroyed(mech)) {
        notify(player, "That mech has been destroyed ... nothing to do");
        /* Cleanup */
        return;
    }

    if (IsAutoOn(autopilot)) {
        notify(player, "The autopilot is already online."
                " You have to disengage it first.");
        return;
    }

    /* Turn Autopilot On */
    AutoOn(autopilot);

    if (MechAuto(mech) <= 0)
        auto_init(autopilot, mech);

    MechAuto(mech) = autopilot->mynum;

    if (MechAuto(mech) > 0)
        auto_set_comtitle(autopilot, mech);

    autopilot->mapindex = mech->mapindex;

    notify(player, "Engaging autopilot...");
    notify(player, "Welcome to " AUTOPILOT_VERSION);

    return;

}

/*
 * Turn off the autopilot
 */
void auto_disengage(dbref player, void *data, char *buffer)
{

    AUTO *autopilot = (AUTO *) data;

    /* Make sure its an AI */
    if (!autopilot && !IsAuto(autopilot->mynum)) {
        notify(player, "Internal error! - Bad AI Object");
        /* Insert error message to somewhere */
        /* Insert Cleanup */
        return;
    }

    if (!IsAutoOn(autopilot)) {
        notify(player, "The autopilot's already offline! You have to engage it first.");
        return;
    }

    AutoOff(autopilot);

    notify(player, "Autopilot has been disengaged.");

    return;

}

#define SPECIAL_FREE 0
#define SPECIAL_ALLOC 1

/*
 * Called when either creating a new autopilot - SPECIAL_ALLOC
 * or when destroying an autopilot - SPECIAL_FREE
 */
void auto_newautopilot(dbref key, void **data, int selector)
{

    AUTO *autopilot = *data;
    MECH *mech;
    command_node *temp;
    int i;

    switch (selector) {
        case SPECIAL_ALLOC:

            /* Allocate the command list */
            autopilot->commands = dllist_create_list();

            /* Make sure certain things are set NULL */
            autopilot->astar_path = NULL;
            autopilot->weaplist = NULL;

            for (i = 0; i < AUTO_PROFILE_MAX_SIZE; i++) {
                autopilot->profile[i] = NULL;
            }

            /* And some things not set null */
            autopilot->speed = 100;

            break;

        case SPECIAL_FREE:

            /* Make sure the AI is stopped */
            auto_stop_pilot(autopilot);

            /* Go through the list and remove any leftover nodes */
            while (dllist_size(autopilot->commands)) {

                /* Remove the first node on the list and get the data
                 * from it */
                temp = (command_node *) dllist_remove(autopilot->commands,
                        dllist_head(autopilot->commands));

                /* Destroy the command node */
                auto_destroy_command_node(temp);

            }

            /* Destroy the list */
            dllist_destroy_list(autopilot->commands);
            autopilot->commands = NULL;

            /* Destroy any astar path list thats on the AI */
            auto_destroy_astar_path(autopilot);

            /* Destroy profile array */
            for (i = 0; i < AUTO_PROFILE_MAX_SIZE; i++) {
                if (autopilot->profile[i]) {
                    rb_destroy(autopilot->profile[i]);
                }
                autopilot->profile[i] = NULL;
            }

            /* Destroy weaponlist */
            auto_destroy_weaplist(autopilot);

            /* Finally reset the AI value on its unit if
             * it needs to */
            if ((mech = getMech(autopilot->mymechnum))) {

                /* Just incase another AI has taken over */
                if (MechAuto(mech) == autopilot->mynum) {
                    MechAuto(mech) = -1;
                }

            }
            break;
    }

}

// XXX: put in a header file
extern unsigned int global_tick;

/* Called once a second for each AI thats out there */
void auto_heartbeat(AUTO *autopilot) {

    MECH *mech;
    MAP *map;

    /* Do basic checks */

    /* Make sure its an AI */
    if (!autopilot && !IsAuto(autopilot->mynum)) {
        /* Insert error message to somewhere */
        /* Insert Cleanup */
        return;
    }

    /* getMech checks to make sure its a valid obj and mech */
    if (!(mech = getMech(autopilot->mymechnum))) {
        /* No mech so return - do something about AI cleanup? */
        return;
    }

    /* Mech Destroyed ? */
    if (Destroyed(mech)) {
        /* Cleanup */
        return;
    }

    /* Check Location */
    if (Location(autopilot->mynum) != autopilot->mymechnum) {
        /* AI not in its mech - add cleanup? */
        return;
    }

    /* Check to make sure the AI is 'On' */
    if (IsAutoOn(autopilot)) {
        /* Not on so just return */
        return;
    }

    /* Is the mech on a map? */
    if (!(map = getMap(mech->mapindex))) {
        /* Do something here haven't decided what */
        /* Possibly set a flag, like NOT ON MAP or something
         * because it can still check commands like udisembark
         * if its not on a map */
    }

    /* Init the AI if its not init'd */

    /* Check to see if its been setup - Right now this doesn't do
     * anything but it will handle stuff later - perhaps */
    if (!IsAutoSetup(autopilot)) {
        AutoSetup(autopilot);
    }

    /* Update Weapon Profile if needbe */
    if (autopilot->weaplist == NULL || global_tick % AUTO_PROFILE_TICK == 0) {
        auto_update_profile(autopilot, mech);
    }

    /* Ok now the fun stuff */

    /* What do we need to do */

    /* Check current commands */

    if (map) {

        /* Sensor */
        auto_sensor(autopilot, mech, map);

        /* Target Selection and Gunning */
        if (IsAutoGunning(autopilot)) {

            /* Target */
            auto_select_target(autopilot, mech, map);

            /* Attack */
            auto_gun(autopilot, mech, map);

        }
        /* Move */
    }

    /* Anything else */
}
