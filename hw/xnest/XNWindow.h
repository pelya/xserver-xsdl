/* $Xorg: XNWindow.h,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
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

#ifndef XNESTWINDOW_H
#define XNESTWINDOW_H

typedef struct {
  Window window;
  Window parent;
  int x;
  int y;
  unsigned int width;
  unsigned int height;
  unsigned int border_width;
  Window sibling_above;
#ifdef SHAPE
  RegionPtr bounding_shape;
  RegionPtr clip_shape;
#endif /* SHAPE */
} xnestPrivWin;

typedef struct {
  WindowPtr pWin;
  Window window;
} xnestWindowMatch;

extern int xnestWindowPrivateIndex;

#define xnestWindowPriv(pWin) \
  ((xnestPrivWin *)((pWin)->devPrivates[xnestWindowPrivateIndex].ptr))

#define xnestWindow(pWin) (xnestWindowPriv(pWin)->window)

#define xnestWindowParent(pWin) \
  ((pWin)->parent ? \
   xnestWindow((pWin)->parent) : \
   xnestDefaultWindows[pWin->drawable.pScreen->myNum])

#define xnestWindowSiblingAbove(pWin) \
  ((pWin)->prevSib ? xnestWindow((pWin)->prevSib) : None)

#define xnestWindowSiblingBelow(pWin) \
  ((pWin)->nextSib ? xnestWindow((pWin)->nextSib) : None)

#define CWParent CWSibling
#define CWStackingOrder CWStackMode

extern WindowPtr *WindowTable;

WindowPtr xnestWindowPtr();
Bool xnestCreateWindow();
Bool xnestDestroyWindow();
Bool xnestPositionWindow();
void xnestConfigureWindow();
Bool xnestChangeWindowAttributes();
Bool xnestRealizeWindow();
Bool xnestUnrealizeWindow();
void xnestPaintWindowBackground();
void xnestPaintWindowBorder();
void xnestCopyWindow();
void xnestClipNotify();
void xnestWindowExposures();
#ifdef SHAPE
void xnestShapeWindow();
#endif /* SHAPE */

#endif /* XNESTWINDOW_H */
