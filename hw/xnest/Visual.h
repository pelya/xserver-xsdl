/* $Xorg: Visual.h,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/
/* $XFree86: xc/programs/Xserver/hw/xnest/Visual.h,v 1.2 2003/11/16 05:05:20 dawes Exp $ */

#ifndef XNESTVISUAL_H
#define XNESTVISUAL_H

Visual *xnestVisual(VisualPtr pVisual);
Visual *xnestVisualFromID(ScreenPtr pScreen, VisualID visual);
Colormap xnestDefaultVisualColormap(Visual *visual);

#define xnestDefaultVisual(pScreen) \
  xnestVisualFromID((pScreen), (pScreen)->rootVisual)

#endif /* XNESTVISUAL_H */
