
/*
 * $Id: mycool.h,v 1.1 2005/06/13 20:50:52 murrayma Exp $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 *  Copyright (c) 1996 Markus Stenberg
 *       All rights reserved
 *
 * Created: Wed Oct 30 21:00:38 1996 fingon
 * Last modified: Sat Mar  8 19:01:50 1997 fingon
 *
 */

#ifndef MYCOOL_H
#define MYCOOL_H

#define addmenu(str) \
  CreateMenuEntry_Simple(&c, str, CM_TWO)
#define addmenu4(str) \
  CreateMenuEntry_Simple(&c, str, CM_FOUR)
#define addline()  \
  CreateMenuEntry_Simple(&c, NULL, CM_ONE|CM_LINE)
#define addempty() \
  CreateMenuEntry_Simple(&c, " ", CM_ONE)
#define addnull() \
  CreateMenuEntry_Simple(&c, " ", CM_TWO)

#define vsi(str)      CreateMenuEntry_VSimple(&c, str)
#define sim(str,flag) CreateMenuEntry_Simple(&c, str, flag)
#define cent(str)     sim(str,CM_ONE|CM_CENTER)

#endif				/* MYCOOL_H */
