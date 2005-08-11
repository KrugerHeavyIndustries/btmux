
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

/* 
   addcommand <name> [args]
   delcommand <num>
   listcommands
   engage
   disengage
   jump                       (*)
 */

/* The commands for modifying state of autopilot */

int auto_valid_progline(AUTO * a, int p)
{
    int i;

    for (i = 0; i < a->first_free; i += (acom[a->commands[i]].argcount + 1))
        if (i == p)
            return 1;
    return 0;
}

static char *auto_show_command(AUTO * a, int bpos)
{
    static char buf[MBUF_SIZE];
    int i;

    sprintf(buf, "%-10s", acom[a->commands[bpos]].name);
    for (i = 1; i < (1 + acom[a->commands[bpos]].argcount); i++)
        sprintf(buf + strlen(buf), "%4d", a->commands[bpos + i]);
    return buf;
}

void auto_delcommand(dbref player, void *data, char *buffer)
{
    int p, i, len;
    AUTO *a = (AUTO *) data;
    int repeat = 0;
    int doall = 0;

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
}

void auto_jump(dbref player, void *data, char *buffer)
{
    int p;
    AUTO *a = (AUTO *) data;

    skipws(buffer);
    DOCHECK(!*buffer, "Argument expected!");
    DOCHECK(Readnum(p, buffer), "Invalid argument - single number expected.");
    /* Find out if it's valid position */
    DOCHECK(!auto_valid_progline(a, p),
            "Invalid : Argument out of range, or argument, not command.");
    PG(a) = p;
    notify(player, tprintf("Program Counter set to #%d.", p));
}

void auto_addcommand(dbref player, void *data, char *buffer)
{
    AUTO *a = (AUTO *) data;
    char *args[4];
    int argc;
    int i, j, len;

    DOCHECK((argc = mech_parseattributes(buffer, args, 4)) == 4,
            "Too huge number of arguments!");
    DOCHECK(!argc, "At least one argument required.");
    for (i = 0; acom[i].name; i++)
        if (!strcasecmp(args[0], acom[i].name))
            break;
    DOCHECK(!acom[i].name, "Invalid command!");
    DOCHECK(argc != (len = (1 + acom[i].argcount)),
            "Invalid number of arguments to the subcommand!");
    DOCHECK(a->first_free + len >= AUTOPILOT_MEMORY,
            "Insufficient memory!");
    a->commands[a->first_free] = i;
    for (i = 1; i < argc; i++) {
        DOCHECK(readint(j, args[i]), 
                "Invalid argument to the subcommand - not a number.");
        a->commands[a->first_free + i] = j;
    }
    a->first_free += len;
    notify(player, tprintf("Command added: %s", auto_show_command(a,
            a->first_free - len)));
}

void auto_listcommands(dbref player, void *data, char *buffer)
{
    AUTO *a = (AUTO *) data;
    coolmenu *c = NULL;
    char buf[MBUF_SIZE];
    int i, count = 0;

    addline();
    strcpy(buf, Name(a->mynum));
    cent(tprintf("Autopilot data for %s (in control of %s)", buf,
            Name(Location(a->mynum))));
    addline();
    sim(tprintf("Memory: %d (%d %%) used / %d free", a->first_free,
            100 * a->first_free / AUTOPILOT_MEMORY,
            AUTOPILOT_MEMORY - a->first_free), CM_TWO);
    sim(tprintf("Program counter: #%d %s", a->program_counter,
            a->program_counter >=
            a->first_free ? " (End of program)" : ""), CM_TWO);
    vsi(tprintf
            ("MyRef: #%d  MechRef: #%d  MapIndex: #%d  FSpeed: %d %% (Flag:%d)",
            a->mynum, a->mymechnum, a->mapindex, a->speed, a->flags));
    addline();
    for (i = 0; i < a->first_free; i += (acom[a->commands[i]].argcount + 1)) {
        sprintf(buf, "#%-3d %s", i, auto_show_command(a, i));
        if (i == PG(a))
            vsi(tprintf("%s%s%s", "%ch", buf, "%cn"));
        else
            vsi(buf);
        count++;
    }
    if (!count)
        vsi("No commands have been queued to date.");
    addline();
    ShowCoolMenu(player, c);
    KillCoolMenu(c);
}

int AutoPilotOn(AUTO * a)
{
    int i, j, count = 0;

    for (i = FIRST_AUTO_EVENT; i <= LAST_AUTO_EVENT; i++)
        if ((j = muxevent_count_type_data(i, (void *) a)))
            count += j;

    if (!count)
        return a->flags & (AUTOPILOT_AUTOGUN | AUTOPILOT_GUNZOMBIE |
                AUTOPILOT_PILZOMBIE);
    return count;
}

void StopAutoPilot(AUTO * a)
{
    int i;

    a->flags &=
            ~(AUTOPILOT_AUTOGUN | AUTOPILOT_GUNZOMBIE | AUTOPILOT_PILZOMBIE);
    for (i = FIRST_AUTO_EVENT; i <= LAST_AUTO_EVENT; i++)
        muxevent_remove_type_data(i, (void *) a);
}

/* Main idea: Set up all variables to current (if possible),
   and engage our first execute-commands-event 
 */

void ai_set_comtitle(AUTO * a, MECH * mech)
{
    char buf[LBUF_SIZE];

    sprintf(buf, "a=%s/%s", MechType_Ref(mech), MechIDS(mech, 1));
    mech_set_channeltitle(a->mynum, mech, buf);
}

void auto_engage(dbref player, void *data, char *buffer) {

    AUTO *a = (AUTO *) data;
    MECH *mech;

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
}

void auto_disengage(dbref player, void *data, char *buffer)
{
    AUTO *a = (AUTO *) data;

    DOCHECK(!AutoPilotOn(a),
            "The autopilot's already offline! You have to engage it first.");
    StopAutoPilot(a);
    notify(player, "Autopilot has been disengaged.");
}
