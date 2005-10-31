
/* as_tree - tree library for as
 * vix 14dec85 [written]
 * vix 02feb86 [added tree balancing from wirth "a+ds=p" p. 220-221]
 * vix 06feb86 [added tree_mung()]
 * vix 20jun86 [added tree_delete per wirth a+ds (mod2 v.) p. 224]
 * vix 23jun86 [added delete uar to add for replaced nodes]
 * vix 22jan93 [revisited; uses RCS, ANSI, POSIX; has bug fixes]
 */


/* This program text was created by Paul Vixie using examples from the book:
 * "Algorithms & Data Structures," Niklaus Wirth, Prentice-Hall, 1986, ISBN
 * 0-13-022005-1.  This code and associated documentation is hereby placed
 * in the public domain.
 */

/*#define		DEBUG	"tree"*/


#include <stdio.h>
#include <stdlib.h>
#include "vixie.h"
#include "tree.h"
#ifdef DEBUG
#undef DEBUG
#endif
#include "debug.h"

#ifdef __STDC__
#define __P(x) x
#else
#define	__P(x) ()
#endif

static void sprout __P((tree **, tree_t, int *, int (*)(), void (*)()));
static int delete __P((tree **, int (*)(), tree_t, void (*)(), int *,

	int *));
static void del __P((tree **, int *, tree **, void (*)(), int *));
static void bal_L __P((tree **, int *));
static void bal_R __P((tree **, int *));

#undef __P


void tree_init(tree ** ppr_tree)
{
    ENTER("tree_init");
    *ppr_tree = NULL;
    EXITV;
}


tree_t tree_srch(tree ** ppr_tree, int (*pfi_compare) ( /* ??? */ ),
    tree_t p_user)
{
    register int i_comp;

    ENTER("tree_srch");

    if (*ppr_tree) {
	i_comp = (*pfi_compare) (p_user, (**ppr_tree).tree_p);
	if (i_comp > 0)
	    EXIT(tree_srch(&(**ppr_tree).tree_r, pfi_compare, p_user));
	if (i_comp < 0)
	    EXIT(tree_srch(&(**ppr_tree).tree_l, pfi_compare, p_user));

	/* not higher, not lower... this must be the one.
	 */
	EXIT((**ppr_tree).tree_p);
    }

    /* grounded. NOT found.
     */
    EXIT(NULL);
}


void tree_add(tree ** ppr_tree, int (*pfi_compare) ( /* ??? */ ),
    tree_t p_user, void (*pfv_uar) ( /* ??? */ ))
{
    int i_balance = FALSE;

    ENTER("tree_add");
    sprout(ppr_tree, p_user, &i_balance, pfi_compare, pfv_uar);
    EXITV;
}


int tree_delete(tree ** ppr_p, int (*pfi_compare) ( /* ??? */ ),
    tree_t p_user, void (*pfv_uar) ( /* ??? */ ))
{
    int i_balance = FALSE, i_uar_called = FALSE;

    ENTER("tree_delete");
    EXIT(delete(ppr_p, pfi_compare, p_user, pfv_uar, &i_balance,
	    &i_uar_called));
} int tree_trav(tree ** ppr_tree, int (*pfi_uar) ( /* ??? */ ))
{
    ENTER("tree_trav");

    if (!*ppr_tree)
	EXIT(TRUE);

    if (!tree_trav(&(**ppr_tree).tree_l, pfi_uar))
	EXIT(FALSE);
    if (!(*pfi_uar) ((**ppr_tree).tree_p))
	EXIT(FALSE);
    if (!tree_trav(&(**ppr_tree).tree_r, pfi_uar))
	EXIT(FALSE);
    EXIT(TRUE);
}
void tree_mung(tree ** ppr_tree, void (*pfv_uar) ( /* ??? */ ))
{
    ENTER("tree_mung");
    if (*ppr_tree) {
	tree_mung(&(**ppr_tree).tree_l, pfv_uar);
	tree_mung(&(**ppr_tree).tree_r, pfv_uar);
	if (pfv_uar)
	    (*pfv_uar) ((**ppr_tree).tree_p);
	free(*ppr_tree);
	*ppr_tree = NULL;
    }
    EXITV;
}


static void sprout(tree ** ppr, tree_t p_data, int *pi_balance,
    int (*pfi_compare) ( /* ??? */ ),
    void (*pfv_delete) ( /* ??? */ ))
{
    tree *p1, *p2;
    int cmp;

    ENTER("sprout");

    /* are we grounded?  if so, add the node "here" and set the rebalance
     * flag, then exit.
     */ if (!*ppr) {
	dprintk("grounded. adding new node, setting h=true");
	*ppr = (tree *) malloc(sizeof(tree));
	(*ppr)->tree_l = NULL;
	(*ppr)->tree_r = NULL;
	(*ppr)->tree_b = 0;
	(*ppr)->tree_p = p_data;
	*pi_balance = TRUE;
	EXITV;
    }

    /* compare the data using routine passed by caller.
     */
    cmp = (*pfi_compare) (p_data, (*ppr)->tree_p);

    /* if LESS, prepare to move to the left.
     */
    if (cmp < 0) {
	dprintk("LESS. sprouting left.");
	sprout(&(*ppr)->tree_l, p_data, pi_balance, pfi_compare,
	    pfv_delete);
	if (*pi_balance) {	/* left branch has grown longer */
	    dprintk("LESS: left branch has grown");
	    switch ((*ppr)->tree_b) {
	    case 1:		/* right branch WAS longer; balance is ok now */
		dprintk("LESS: case 1.. balnce restored implicitly");
		    (*ppr)->tree_b = 0;
		*pi_balance = FALSE;
		break;
	    case 0:		/* balance WAS okay; now left branch longer */
		dprintk("LESS: case 0.. balnce bad but still ok");
		(*ppr)->tree_b = -1;
		break;
	    case -1:
		/* left branch was already too long. rebalnce */
		dprintk("LESS: case -1: rebalancing");
		p1 = (*ppr)->tree_l;
		if (p1->tree_b == -1) {	/* LL */
		    dprintk("LESS: single LL");
		    (*ppr)->tree_l = p1->tree_r;
		    p1->tree_r = *ppr;
		    (*ppr)->tree_b = 0;
		    *ppr = p1;
		} else {	/* double LR */
		    dprintk("LESS: double LR");

		    p2 = p1->tree_r;
		    p1->tree_r = p2->tree_l;
		    p2->tree_l = p1;

		    (*ppr)->tree_l = p2->tree_r;
		    p2->tree_r = *ppr;

		    if (p2->tree_b == -1)
			(*ppr)->tree_b = 1;
		    else
			(*ppr)->tree_b = 0;

		    if (p2->tree_b == 1)
			p1->tree_b = -1;
		    else
			p1->tree_b = 0;
		    *ppr = p2;
		}		/*else */
		(*ppr)->tree_b = 0;
		*pi_balance = FALSE;
	    }			/*switch */
	}			/*if */
	EXITV;
    }

    /*if */
    /* if MORE, prepare to move to the right.
     */
    if (cmp > 0) {
	dprintk("MORE: sprouting to the right");
	sprout(&(*ppr)->tree_r, p_data, pi_balance, pfi_compare,
	    pfv_delete);
	if (*pi_balance) {	/* right branch has grown longer */
	    dprintk("MORE: right branch has grown");

	    switch ((*ppr)->tree_b) {
	    case -1:
		dprintk("MORE: balance was off, fixed implicitly");
		(*ppr)->tree_b = 0;
		*pi_balance = FALSE;
		break;
	    case 0:
		dprintk("MORE: balance was okay, now off but ok");
		(*ppr)->tree_b = 1;
		break;
	    case 1:
		dprintk("MORE: balance was off, need to rebalance");
		p1 = (*ppr)->tree_r;
		if (p1->tree_b == 1) {	/* RR */
		    dprintk("MORE: single RR");
		    (*ppr)->tree_r = p1->tree_l;
		    p1->tree_l = *ppr;
		    (*ppr)->tree_b = 0;
		    *ppr = p1;
		} else {	/* double RL */
		    dprintk("MORE: double RL");

		    p2 = p1->tree_l;
		    p1->tree_l = p2->tree_r;
		    p2->tree_r = p1;

		    (*ppr)->tree_r = p2->tree_l;
		    p2->tree_l = *ppr;

		    if (p2->tree_b == 1)
			(*ppr)->tree_b = -1;
		    else
			(*ppr)->tree_b = 0;

		    if (p2->tree_b == -1)
			p1->tree_b = 1;
		    else
			p1->tree_b = 0;

		    *ppr = p2;
		}		/*else */
		(*ppr)->tree_b = 0;
		*pi_balance = FALSE;
	    }			/*switch */
	}			/*if */
	EXITV;
    }

    /*if */
    /* not less, not more: this is the same key!  replace...
     */
    dprintk("I found it!  Replacing data value");
    *pi_balance = FALSE;
    if (pfv_delete)
	(*pfv_delete) ((*ppr)->tree_p);
    (*ppr)->tree_p = p_data;
    EXITV;
}


static int delete(tree ** ppr_p, int (*pfi_compare) ( /* ??? */ ),
    tree_t p_user, void (*pfv_uar) ( /* ??? */ ),
    int *pi_balance, int *pi_uar_called)
{
    tree *pr_q;
    int i_comp, i_ret;

    ENTER("delete");

    if (*ppr_p == NULL) {
	dprintk("key not in tree");
	EXIT(FALSE);
    }

    i_comp = (*pfi_compare) ((*ppr_p)->tree_p, p_user);

    if (i_comp > 0) {
	dprintk("too high - scan left");
	i_ret =
	    delete(&(*ppr_p)->tree_l, pfi_compare, p_user, pfv_uar,
	    pi_balance, pi_uar_called);
	if (*pi_balance)
	    bal_L(ppr_p, pi_balance);
    } else if (i_comp < 0) {
	dprintk("too low - scan right");
	i_ret =
	    delete(&(*ppr_p)->tree_r, pfi_compare, p_user, pfv_uar,
	    pi_balance, pi_uar_called);
	if (*pi_balance)
	    bal_R(ppr_p, pi_balance);
    } else {
	dprintk("equal");
	pr_q = *ppr_p;
	if (pr_q->tree_r == NULL) {
	    dprintk("right subtree null");
	    *ppr_p = pr_q->tree_l;
	    *pi_balance = TRUE;
	} else if (pr_q->tree_l == NULL) {
	    dprintk("right subtree non-null, left subtree null");
	    *ppr_p = pr_q->tree_r;
	    *pi_balance = TRUE;
	} else {
	    dprintk("neither subtree null");
	    del(&pr_q->tree_l, pi_balance, &pr_q, pfv_uar, pi_uar_called);
	    if (*pi_balance)
		bal_L(ppr_p, pi_balance);
	}
	if (!*pi_uar_called && *pfv_uar)
	    (*pfv_uar) (pr_q->tree_p);
	free(pr_q);		/* thanks to wuth@castrov.cuc.ab.ca */
	i_ret = TRUE;
    }
    EXIT(i_ret);
}


static void del(tree ** ppr_r, int *pi_balance, tree ** ppr_q,
    void (*pfv_uar) ( /* ??? */ ), int *pi_uar_called)
{
    ENTER("del");

    if ((*ppr_r)->tree_r != NULL) {
	del(&(*ppr_r)->tree_r, pi_balance, ppr_q, pfv_uar, pi_uar_called);
	if (*pi_balance)
	    bal_R(ppr_r, pi_balance);
    } else {
	if (pfv_uar)
	    (*pfv_uar) ((*ppr_q)->tree_p);
	*pi_uar_called = TRUE;
	(*ppr_q)->tree_p = (*ppr_r)->tree_p;
	*ppr_q = *ppr_r;
	*ppr_r = (*ppr_r)->tree_l;
	*pi_balance = TRUE;
    }

    EXITV;
}


static void bal_L(tree ** ppr_p, int *pi_balance)
{
    tree *p1, *p2;
    int b1, b2;

    ENTER("bal_L");
    dprintk("left branch has shrunk");

    switch ((*ppr_p)->tree_b) {
    case -1:
	dprintk("was imbalanced, fixed implicitly");
	(*ppr_p)->tree_b = 0;
	break;
    case 0:
	dprintk("was okay, is now one off");
	(*ppr_p)->tree_b = 1;
	*pi_balance = FALSE;
	break;
    case 1:
	dprintk("was already off, this is too much");
	p1 = (*ppr_p)->tree_r;
	b1 = p1->tree_b;
	if (b1 >= 0) {
	    dprintk("single RR");
	    (*ppr_p)->tree_r = p1->tree_l;
	    p1->tree_l = *ppr_p;
	    if (b1 == 0) {
		dprintk("b1 == 0");
		(*ppr_p)->tree_b = 1;
		p1->tree_b = -1;
		*pi_balance = FALSE;
	    } else {
		dprintk("b1 != 0");
		(*ppr_p)->tree_b = 0;
		p1->tree_b = 0;
	    }
	    *ppr_p = p1;
	} else {
	    dprintk("double RL");
	    p2 = p1->tree_l;
	    b2 = p2->tree_b;
	    p1->tree_l = p2->tree_r;
	    p2->tree_r = p1;
	    (*ppr_p)->tree_r = p2->tree_l;
	    p2->tree_l = *ppr_p;
	    if (b2 == 1)
		(*ppr_p)->tree_b = -1;
	    else
		(*ppr_p)->tree_b = 0;
	    if (b2 == -1)
		p1->tree_b = 1;
	    else
		p1->tree_b = 0;
	    *ppr_p = p2;
	    p2->tree_b = 0;
	}
    }
    EXITV;
}


static void bal_R(tree ** ppr_p, int *pi_balance)
{
    tree *p1, *p2;
    int b1, b2;

    ENTER("bal_R");
    dprintk("right branch has shrunk");
    switch ((*ppr_p)->tree_b) {
    case 1:
	dprintk("was imbalanced, fixed implicitly");
	(*ppr_p)->tree_b = 0;
	break;
    case 0:
	dprintk("was okay, is now one off");
	(*ppr_p)->tree_b = -1;
	*pi_balance = FALSE;
	break;
    case -1:
	dprintk("was already off, this is too much");
	p1 = (*ppr_p)->tree_l;
	b1 = p1->tree_b;
	if (b1 <= 0) {
	    dprintk("single LL");
	    (*ppr_p)->tree_l = p1->tree_r;
	    p1->tree_r = *ppr_p;
	    if (b1 == 0) {
		dprintk("b1 == 0");
		(*ppr_p)->tree_b = -1;
		p1->tree_b = 1;
		*pi_balance = FALSE;
	    } else {
		dprintk("b1 != 0");
		(*ppr_p)->tree_b = 0;
		p1->tree_b = 0;
	    }
	    *ppr_p = p1;
	} else {
	    dprintk("double LR");
	    p2 = p1->tree_r;
	    b2 = p2->tree_b;
	    p1->tree_r = p2->tree_l;
	    p2->tree_l = p1;
	    (*ppr_p)->tree_l = p2->tree_r;
	    p2->tree_r = *ppr_p;
	    if (b2 == -1)
		(*ppr_p)->tree_b = 1;
	    else
		(*ppr_p)->tree_b = 0;
	    if (b2 == 1)
		p1->tree_b = -1;
	    else
		p1->tree_b = 0;
	    *ppr_p = p2;
	    p2->tree_b = 0;
	}
    }
    EXITV;
}
