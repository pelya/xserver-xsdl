/* $Xorg: Drawable.h,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
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
/* $XFree86: xc/programs/Xserver/hw/xnest/Drawable.h,v 1.3 2002/11/23 19:27:50 tsi Exp $ */

#ifndef XNESTDRAWABLE_H
#define XNESTDRAWABLE_H

#include "XNWindow.h"
#include "XNPixmap.h"

#define xnestDrawable(pDrawable) \
  ((pDrawable)->type == DRAWABLE_WINDOW ? \
   xnestWindow((WindowPtr)pDrawable) : \
   xnestPixmap((PixmapPtr)pDrawable))

#endif /* XNESTDRAWABLE_H */
