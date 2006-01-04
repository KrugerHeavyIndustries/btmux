
/*
 * $Id: ai.c,v 1.2 2005/08/03 21:40:53 av1-op Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1998 Markus Stenberg
 *  Copyright (c) 1998-2002 Thomas Wouters
 *  Copyright (c) 2000-2002 Cord Awtry 
 *       All rights reserved
 *
 * Created: Thu Mar 12 18:36:33 1998 fingon
 * Last modified: Thu Dec 10 21:46:30 1998 fingon
 *
 */


/* Point of the excercise : move from point a,b to point c,d while
   eliminating opponents and stuff, avoiding enemies in rear/side arc
   and generally having fun */

#include <math.h>

#include "mech.h"
#include "autopilot.h"
#include "p.mech.utils.h"
#include "p.map.obj.h"
#include "spath.h"
#include "p.mech.combat.h"
#include "p.glue.hcode.h"

void ai_set_speed(MECH * mech, AUTO *a, float spd)
{
    char buf[SBUF_SIZE];
    float newspeed;

    if (!mech || !a)
        return;

    newspeed = FBOUNDED(0, spd, ((MMaxSpeed(mech) * a->speed) / 100.0));

    if (MechDesiredSpeed(mech) != newspeed) {
        sprintf(buf, "%f", newspeed);
        mech_speed(a->mynum, mech, buf);
    }
}

/* Experimental (highly) path finding system based on the A* 'a-star'
 * system used in many typical games.
 *
 * Dany - 08/2005 */

/*
 * Not experimental anymore
 * it works :)
 *
 * Dany - 01/2006 */

/*
 * Create an astar node and return a pointer to it
 */
static astar_node *auto_create_astar_node(short x, short y, 
        short x_parent, short y_parent,
        short g_score, short h_score) {

    astar_node *temp;
    temp = malloc(sizeof(astar_node));
    if (temp == NULL)
        return NULL;

    memset(temp, 0, sizeof(astar_node));

    temp->x = x;
    temp->y = y;
    temp->x_parent = x_parent;
    temp->y_parent = y_parent;
    temp->g_score = g_score;
    temp->h_score = h_score;
    temp->f_score = g_score + h_score;
    temp->hexoffset = x * MAPY + y;

    return temp;

}

int astar_compare(int a, int b, void *arg) { return a-b; }

void astar_release(void *key, void *data) { free(data); }

/*
 * The A* (A-Star) path finding function for the AI
 *
 * Returns 1 if it found a path and 0 if it doesn't
 */
int auto_astar_generate_path(AUTO *autopilot, MECH *mech, short end_x, short end_y) {

    MAP * map = getMap(autopilot->mapindex);
    int found_path = 0;

    /* Our bit arrays */
    unsigned char closed_list_bitfield[(MAPX*MAPY)/8];
    unsigned char open_list_bitfield[(MAPX*MAPY)/8];

    float x1, y1, x2, y2;                       /* Floating point vars for real cords */
    short map_x1, map_y1, map_x2, map_y2;       /* The actual map 'hexes' */
    int i;
    int child_g_score, child_h_score;           /* the score values for the child hexes */
    int hexoffset;                              /* temp int to pass around as the hexoffset */

    /* Our lists using Hag's rbtree */
    /* Using two rbtree's to store the open_list so we can sort two
     * different ways */
    rbtree *open_list_by_score;                    /* open list sorted by score */
    rbtree *open_list_by_xy;                       /* open list sorted by hexoffset */
    rbtree *closed_list;                           /* closed list sorted by hexoffset */

    /* Helper node for the final path */
    dllist_node *astar_path_node;

    /* Our astar_node helpers */
    astar_node *temp_astar_node;
    astar_node *parent_astar_node;

#ifdef DEBUG_ASTAR
    /* Log File */
    FILE *logfile;
    char log_msg[MBUF_SIZE];

    /* Open the logfile */
    logfile = fopen("astar.log", "a");

    /* Write first message */
    snprintf(log_msg, MBUF_SIZE, "\nStarting ASTAR Path finding for AI #%d from "
            "%d, %d to %d, %d\n",
            autopilot->mynum, MechX(mech), MechY(mech), end_x, end_y);
    fprintf(logfile, "%s", log_msg);
#endif

    /* Zero the bitfields */
    memset(closed_list_bitfield, 0, sizeof(closed_list_bitfield));
    memset(open_list_bitfield, 0, sizeof(open_list_bitfield));

    /* Setup the trees */
    open_list_by_score = rb_init((void *)astar_compare, NULL);
    open_list_by_xy = rb_init((void *)astar_compare, NULL);
    closed_list = rb_init((void *)astar_compare, NULL);

    /* Setup the path */
    /* Destroy any existing path first */
    auto_destroy_astar_path(autopilot);
    autopilot->astar_path = dllist_create_list();

    /* Setup the start hex */
    temp_astar_node = auto_create_astar_node(MechX(mech), MechY(mech), -1, -1, 0, 0);

    if (temp_astar_node == NULL) {
        /*! \todo {Add code here to break if we can't alloc memory} */

#ifdef DEBUG_ASTAR
        /* Write Log Message */
        snprintf(log_msg, MBUF_SIZE, "AI ERROR - Unable to malloc astar node for "
                "hex %d, %d\n",
                MechX(mech), MechY(mech));
        fprintf(logfile, "%s", log_msg);
#endif

    }

    /* Add start hex to open list */
    rb_insert(open_list_by_score, (void *)temp_astar_node->f_score, temp_astar_node);
    rb_insert(open_list_by_xy, (void *)temp_astar_node->hexoffset, temp_astar_node);
    SetHexBit(open_list_bitfield, temp_astar_node->hexoffset);

#ifdef DEBUG_ASTAR
    /* Log it */
    snprintf(log_msg, MBUF_SIZE, "Added hex %d, %d (%d %d) to open list\n",
            temp_astar_node->x, temp_astar_node->y, temp_astar_node->g_score,
            temp_astar_node->h_score); 
    fprintf(logfile, "%s", log_msg);
#endif

    /* Now loop till we find path */
    while (!found_path) {

        /* Check to make sure there is still stuff in the open list
         * if not, means we couldn't find a path so quit */
        if (rb_size(open_list_by_score) == 0) {
            break;
        }

        /* Get lowest cost node, then remove it from the open list */
        parent_astar_node = (astar_node *) rb_search(open_list_by_score, 
                SEARCH_FIRST, NULL); 

        rb_delete(open_list_by_score, (void *)parent_astar_node->f_score);
        rb_delete(open_list_by_xy, (void *)parent_astar_node->hexoffset);
        ClearHexBit(open_list_bitfield, parent_astar_node->hexoffset);

#ifdef DEBUG_ASTAR
        /* Log it */
        snprintf(log_msg, MBUF_SIZE, "Removed hex %d, %d (%d %d) from open "
                "list - lowest cost node\n",
                parent_astar_node->x, parent_astar_node->y,
                parent_astar_node->g_score, parent_astar_node->h_score); 
        fprintf(logfile, "%s", log_msg);
#endif

        /* Add it to the closed list */
        rb_insert(closed_list, (void *)parent_astar_node->hexoffset, parent_astar_node);
        SetHexBit(closed_list_bitfield, parent_astar_node->hexoffset);

#ifdef DEBUG_ASTAR
        /* Log it */
        snprintf(log_msg, MBUF_SIZE, "Added hex %d, %d (%d %d) to closed list"
                " - lowest cost node\n",
                parent_astar_node->x, parent_astar_node->y,
                parent_astar_node->g_score, parent_astar_node->h_score); 
        fprintf(logfile, "%s", log_msg);
#endif

        /* Now we check to see if we added the end hex to the closed list.
         * When this happens it means we are done */
        if (CheckHexBit(closed_list_bitfield, HexOffSet(end_x, end_y))) {
            found_path = 1;

#ifdef DEBUG_ASTAR
            fprintf(logfile, "Found path for the AI\n");
#endif

            break;
        }

        /* Update open list */
        /* Loop through the hexes around current hex and see if we can add
         * them to the open list */

        /* Set the parent hex of the new nodes */
        map_x1 = parent_astar_node->x;
        map_y1 = parent_astar_node->y;

        /* Going around clockwise direction */
        for (i = 0; i < 360; i += 60) {

            /* Map coord to Real */
            MapCoordToRealCoord(map_x1, map_y1, &x1, &y1);

            /* Calc new hex */
            FindXY(x1, y1, i, 1.0, &x2, &y2);

            /* Real coord to Map */
            RealCoordToMapCoord(&map_x2, &map_y2, x2, y2);

            /* Make sure the hex is sane */
            if (map_x2 < 0 || map_y2 < 0 ||
                    map_x2 >= map->map_width ||
                    map_y2 >= map->map_height)
                continue;

            /* Generate hexoffset for the child node */
            hexoffset = HexOffSet(map_x2, map_y2);

            /* Check to see if its in the closed list 
             * if so just ignore it */
            if (CheckHexBit(closed_list_bitfield, hexoffset))
                continue;

            /* Check to see if we can enter it */
            if ((MechType(mech) == CLASS_MECH) &&
                    (abs(Elevation(map, map_x1, map_y1)
                        - Elevation(map, map_x2, map_y2)) > 2))
                continue;

            if ((MechType(mech) == CLASS_VEH_GROUND) &&
                    (abs(Elevation(map, map_x1, map_y1)
                        - Elevation(map, map_x2, map_y2)) > 1))
                continue;

            /* Score the hex */
            /* Right now just assume movement cost from parent to child hex is
             * the same (so 100) no matter which dir we go*/
            /*! \todo {Possibly add in code to make turning less desirable} */
            child_g_score = 100;

            /* Now add the g score from the parent */
            child_g_score += parent_astar_node->g_score;

            /* Next get range */
            /* Using a varient of the Manhattan method since its perfectly
             * logical for us to go diagonally
             *
             * Basicly just going to get the range,
             * and multiply by 100 */
            /*! \todo {Add in something for elevation cost} */

            /* Get the end hex in real coords, using the old variables 
             * to store the values */
            MapCoordToRealCoord(end_x, end_y, &x1, &y1);
            
            /* Re-using the x2 and y2 values we calc'd for the child hex 
             * to find the range between the child hex and end hex */
            child_h_score = 100 * FindHexRange(x2, y2, x1, y1);

            /* Now add in some modifiers for terrain */
            switch(GetTerrain(map, map_x2, map_y2)) {
                case LIGHT_FOREST:

                    /* Don't bother trying to enter a light forest 
                     * hex unless we can */
                    if ((MechType(mech) == CLASS_VEH_GROUND) &&
                            (MechMove(mech) != MOVE_TRACK))
                        continue;

                    child_g_score += 50;
                    break;
                case ROUGH:
                    child_g_score += 50;
                    break;
                case HEAVY_FOREST:
                    
                    /* Don't bother trying to enter a heavy forest
                     * hex unless we can */
                    if (MechType(mech) == CLASS_VEH_GROUND)
                        continue;

                    child_g_score += 100;
                    break;
                case MOUNTAINS:
                    child_g_score += 100;
                    break;
                case WATER:

                    /* Don't bother trying to enter a water hex
                     * unless we can */
                    if ((MechType(mech) == CLASS_VEH_GROUND) &&
                            (MechMove(mech) != MOVE_HOVER))
                        continue;

                    /* We really don't want them trying to enter water */ 
                    child_g_score += 200;
                    break;
                case HIGHWATER:

                    /* Don't bother trying to enter a water hex
                     * unless we can */
                    if ((MechType(mech) == CLASS_VEH_GROUND) &&
                            (MechMove(mech) != MOVE_HOVER))
                        continue;

                    /* We really don't want them trying to enter water */ 
                    child_g_score += 200;
                    break;
                default:
                    break;
            }

            /* Is it already on the openlist */
            if (CheckHexBit(open_list_bitfield, hexoffset)) {

                /* Ok need to compare the scores and if necessary recalc
                 * and change stuff */
                
                /* Get the node off the open_list */
                temp_astar_node = (astar_node *) rb_find(open_list_by_xy,
                        (void *)hexoffset);

                /* Now compare the 'g_scores' to determine shortest path */
                /* If g_score is lower, this means better path 
                 * from the current parent node */
                if (child_g_score < temp_astar_node->g_score) {

                    /* Remove from open list */
                    rb_delete(open_list_by_score, (void *)temp_astar_node->f_score);
                    rb_delete(open_list_by_xy, (void *)temp_astar_node->hexoffset);
                    ClearHexBit(open_list_bitfield, temp_astar_node->hexoffset);

#ifdef DEBUG_ASTAR
                    /* Log it */
                    snprintf(log_msg, MBUF_SIZE, "Removed hex %d, %d (%d %d) from "
                            "open list - score recal\n",
                            temp_astar_node->x, temp_astar_node->y,
                            temp_astar_node->g_score, temp_astar_node->h_score); 
                    fprintf(logfile, "%s", log_msg);
#endif

                    /* Recalc score */
                    /* H-Score should be the same since the hex doesn't move */
                    temp_astar_node->g_score = child_g_score;
                    temp_astar_node->f_score = temp_astar_node->g_score +
                        temp_astar_node->h_score;

                    /* Change parent hex */
                    temp_astar_node->x_parent = map_x1;
                    temp_astar_node->y_parent = map_y1;

                    /* Will re-add the node below */

                } else {

                    /* Don't need to do anything so we can skip 
                     * to the next node */
                    continue;

                }

            } else {

                /* Node isn't on the open list so we have to create it */
                temp_astar_node = auto_create_astar_node(map_x2, map_y2, map_x1, map_y1,
                        child_g_score, child_h_score);
            
                if (temp_astar_node == NULL) {
                    /*! \todo {Add code here to break if we can't alloc memory} */

#ifdef DEBUG_ASTAR
                    /* Log it */
                    snprintf(log_msg, MBUF_SIZE, "AI ERROR - Unable to malloc astar"
                            " node for hex %d, %d\n",
                            map_x2, map_y2);
                    fprintf(logfile, "%s", log_msg);
#endif

                }

            }

            /* Now add (or re-add) the node to the open list */

            /* Hack to check to make sure its score is not already on the open
             * list. This slightly skews the results towards nodes found earlier
             * then those found later */
            while (1) {

                if (rb_exists(open_list_by_score, (void *)temp_astar_node->f_score)) {
                    temp_astar_node->f_score++;

#ifdef DEBUG_ASTAR
                    fprintf(logfile, "Adjusting score for hex %d, %d - same"
                            " fscore already exists\n",
                            temp_astar_node->x, temp_astar_node->y);
#endif

                } else {
                    break;
                }

            }
            rb_insert(open_list_by_score, (void *)temp_astar_node->f_score, temp_astar_node);
            rb_insert(open_list_by_xy, (void *)temp_astar_node->hexoffset, temp_astar_node);
            SetHexBit(open_list_bitfield, temp_astar_node->hexoffset);

#ifdef DEBUG_ASTAR
            /* Log it */
            snprintf(log_msg, MBUF_SIZE, "Added hex %d, %d (%d %d) to open list\n",
                    temp_astar_node->x, temp_astar_node->y,
                    temp_astar_node->g_score, temp_astar_node->h_score);
            fprintf(logfile, "%s", log_msg);
#endif

        } /* End of looking for hexes next to us */
       
    } /* End of looking for path */

    /* We Done lets go */

    /* Lets first see if we found a path */
    if (found_path) {

#ifdef DEBUG_ASTAR
        /* Log Message */
        fprintf(logfile, "Building Path from closed list for AI\n");
#endif

        /* Found a path so we need to go through the closed list
         * and generate it */

        /* Get the end hex, find its parent hex and work back to
         * start hex while building list */
       
        /* Get end hex from closed list */
        hexoffset = HexOffSet(end_x, end_y);
        temp_astar_node = rb_find(closed_list, (void *)hexoffset);

        /* Add end hex to path list */
        astar_path_node = dllist_create_node(temp_astar_node);
        dllist_insert_beginning(autopilot->astar_path, astar_path_node);

#ifdef DEBUG_ASTAR
        /* Log it */
        fprintf(logfile, "Added hex %d, %d to path list\n",
                temp_astar_node->x, temp_astar_node->y);
#endif

        /* Remove it from closed list */
        rb_delete(closed_list, (void *)temp_astar_node->hexoffset);

#ifdef DEBUG_ASTAR
        /* Log it */
        fprintf(logfile, "Removed hex %d, %d from closed list - path list work\n",
                temp_astar_node->x, temp_astar_node->y);
#endif

        /* Check if the end hex is the start hex */
        if (!(temp_astar_node->x == MechX(mech) &&
                temp_astar_node->y == MechY(mech))) {

            /* Its not so lets loop through the closed list
             * building the path */

            /* Loop */
            while (1) {

                /* Get Parent Node Offset*/
                hexoffset = HexOffSet(temp_astar_node->x_parent, 
                        temp_astar_node->y_parent);

                /*! \todo {Possibly add check here incase the node we're
                 * looking for some how did not end up on the list} */

                /* Get Parent Node from closed list */
                parent_astar_node = rb_find(closed_list, (void *)hexoffset);

                /* Check if start hex */
                /* If start hex quit */
                if (parent_astar_node->x == MechX(mech) &&
                        parent_astar_node->y == MechY(mech)) {
                    break;
                }

                /* Add to path list */
                astar_path_node = dllist_create_node(parent_astar_node);
                dllist_insert_beginning(autopilot->astar_path, astar_path_node);

#ifdef DEBUG_ASTAR
                /* Log it */
                fprintf(logfile, "Added hex %d, %d to path list\n",
                        parent_astar_node->x, parent_astar_node->y);
#endif

                /* Remove from closed list */
                rb_delete(closed_list, (void *)parent_astar_node->hexoffset);

#ifdef DEBUG_ASTAR
                /* Log it */
                fprintf(logfile, "Removed hex %d, %d from closed list - path list work\n",
                        parent_astar_node->x, parent_astar_node->y);
#endif

                /* Make parent new child */
                temp_astar_node = parent_astar_node;

            } /* End of while loop */

        }

        /* Done with the path its cleanup time */

    }

    /* Make sure we destroy all the objects we dont need any more */

#ifdef DEBUG_ASTAR
    /* Log Message */
    fprintf(logfile, "Destorying the AI lists\n");
#endif

    /* Destroy the open lists */
    rb_release(open_list_by_score, (void *)astar_release, NULL);
    rb_destroy(open_list_by_xy);

    /* Destroy the closed list */
    rb_release(closed_list, (void *)astar_release, NULL);

#ifdef DEBUG_ASTAR
    /* Close Log file */
    fclose(logfile);
#endif

    /* End */
    if (found_path) {
        return 1;
    } else {
        return 0;
    }

}

/* Function to Smooth out the AI path and remove
 * nodes we don't need */
/* Not even close to being finished yet */
void astar_smooth_path(AUTO *autopilot) {

    dllist_node *current, *next, *prev;

    float x1, y1, x2, y2;
    int degrees;

    /* Get the n node off the list */

    /* Get the n+1 node off the list */

    /* Get bearing from n to n+1 */

    /* Get n+2 node off list */

    /* Get bearing from n to n+2 node */

    /* Compare bearings */
        /* If in same direction as previous
         * don't need n+1 node */

    /* Keep looping till bearing doesn't match */
    /* Then reset n node to final node and continue */

    return;

}

void auto_destroy_astar_path(AUTO *autopilot) {

    astar_node *temp_astar_node;

    /* Make sure there is a path if not quit */
    if (!(autopilot->astar_path))
        return;

    /* There is a path lets kill it */
    if (dllist_size(autopilot->astar_path) > 0) {

        while (dllist_size(autopilot->astar_path)) {
            temp_astar_node = dllist_remove_node_at_pos(autopilot->astar_path, 1);
            free(temp_astar_node);
        }

    }

    /* Finally destroying the path */
    dllist_destroy_list(autopilot->astar_path);
    autopilot->astar_path = NULL;

}
