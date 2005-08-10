
/*
   p.mux_tree.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Mon Jul  5 10:53:01 CEST 1999 from mux_tree.c */

#ifndef _P_MUX_TREE_H
#define _P_MUX_TREE_H

/* mux_tree.c */
Node *FindNode(Tree tree, muxkey_t key);
void recursively_save_list(void *data);
int SaveTree(FILE * f, Tree tree);
void recursively_read_list(FILE * f, void *data);
void GoThruTree(Tree tree, int (*func) (Node *));

#endif				/* _P_MUX_TREE_H */
