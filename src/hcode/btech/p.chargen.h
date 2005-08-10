
/*
   p.chargen.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Fri Jan 15 15:32:38 CET 1999 from chargen.c */

#ifndef _P_CHARGEN_H
#define _P_CHARGEN_H

/* chargen.c */
int lowest_bit(int num);
int recursive_add(int lev);
int can_advance_state(struct chargen_struct *st);
int can_go_back_state(struct chargen_struct *st);
void recalculate_skillpoints(struct chargen_struct *st);

#endif				/* _P_CHARGEN_H */
