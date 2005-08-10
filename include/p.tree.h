
/*
   p.tree.h

   Automatically created by protomaker (C) 1998 Markus Stenberg (fingon@iki.fi)
   Protomaker is actually only a wrapper script for cproto, but well.. I like
   fancy headers and stuff :)
   */

/* Generated at Sun Jun  7 17:12:00 EEST 1998 from tree.c */

#ifndef _P_TREE_H
#define _P_TREE_H

void tree_init(tree ** ppr_tree);
tree_t tree_srch(tree ** ppr_tree, int (*pfi_compare) (), tree_t p_user);
void tree_add(tree ** ppr_tree, int (*pfi_compare) (), tree_t p_user,
    void (*pfv_uar) ());
int tree_delete(tree ** ppr_p, int (*pfi_compare) (), tree_t p_user,
    void (*pfv_uar) ());
int tree_trav(tree ** ppr_tree, int (*pfi_uar) ());
void tree_mung(tree ** ppr_tree, void (*pfv_uar) ());

#endif				/* _P_TREE_H */
