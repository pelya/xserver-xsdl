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
/* $XFree86: xc/programs/Xserver/hw/xnest/XNGC.h,v 1.2 2003/11/16 05:05:20 dawes Exp $ */

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

Bool xnestCreateGC(GCPtr pGC);
void xnestValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable);
void xnestChangeGC(GCPtr pGC, unsigned long mask);
void xnestCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst);
void xnestDestroyGC(GCPtr pGC);
void xnestChangeClip(GCPtr pGC, int type, pointer pValue, int nRects);
void xnestDestroyClip(GCPtr pGC);
void xnestDestroyClipHelper(GCPtr pGC);
void xnestCopyClip(GCPtr pGCDst, GCPtr pGCSrc);

#endif /* XNESTGC_H */
