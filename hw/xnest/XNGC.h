/* $Xorg: XNGC.h,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
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

#ifndef XNESTGC_H
#define XNESTGC_H

/* This file uses the GC definition form Xlib.h as XlibGC. */

typedef struct {
  XlibGC gc;
  int nClipRects;
} xnestPrivGC;

extern int xnestGCPrivateIndex;

#define xnestGCPriv(pGC) \
  ((xnestPrivGC *)((pGC)->devPrivates[xnestGCPrivateIndex].ptr))

#define xnestGC(pGC) (xnestGCPriv(pGC)->gc)

Bool xnestCreateGC();
void xnestValidateGC();
void xnestChangeGC();
void xnestCopyGC();
void xnestDestroyGC();
void xnestChangeClip();
void xnestDestroyClip();
void xnestDestroyClipHelper();
void xnestCopyClip();

#endif /* XNESTGC_H */
