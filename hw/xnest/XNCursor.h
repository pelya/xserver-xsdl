/* $Xorg: Cursor.h,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
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
/* $XFree86: xc/programs/Xserver/hw/xnest/XNCursor.h,v 1.3 2003/11/16 05:05:20 dawes Exp $ */

#ifndef XNESTCURSOR_H
#define XNESTCURSOR_H

typedef struct {
  Cursor cursor;
} xnestPrivCursor;

#define xnestCursorPriv(pCursor, pScreen) \
  ((xnestPrivCursor *)((pCursor)->devPriv[pScreen->myNum]))

#define xnestCursor(pCursor, pScreen) \
  (xnestCursorPriv(pCursor, pScreen)->cursor)

void xnestConstrainCursor(ScreenPtr pScreen, BoxPtr pBox);
void xnestCursorLimits(ScreenPtr pScreen, CursorPtr pCursor, BoxPtr pHotBox,
		       BoxPtr pTopLeftBox);
Bool xnestDisplayCursor(ScreenPtr pScreen, CursorPtr pCursor);
Bool xnestRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
Bool xnestUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
void xnestRecolorCursor(ScreenPtr pScreen, CursorPtr pCursor, Bool displayed);
Bool xnestSetCursorPosition(ScreenPtr pScreen, int x, int y,
			    Bool generateEvent);

#endif /* XNESTCURSOR_H */
