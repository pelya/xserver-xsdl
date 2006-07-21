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

#ifndef XNESTCURSOR_H
#define XNESTCURSOR_H

typedef struct {
  Cursor cursor;
} xnestPrivCursor;

#define xnestCursorPriv(pCursor, pScreen) \
  ((xnestPrivCursor *)((pCursor)->devPriv[pScreen->myNum]))

#define xnestCursor(pCursor, pScreen) \
  (xnestCursorPriv(pCursor, pScreen)->cursor)

Bool xnestRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
Bool xnestUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
void xnestRecolorCursor(ScreenPtr pScreen, CursorPtr pCursor, Bool displayed);
void xnestSetCursor (ScreenPtr pScreen, CursorPtr pCursor, int x, int y);
void xnestMoveCursor (ScreenPtr pScreen, int x, int y);

#endif /* XNESTCURSOR_H */
