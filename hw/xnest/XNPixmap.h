/* $Xorg: Pixmap.h,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
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
/* $XFree86: xc/programs/Xserver/hw/xnest/XNPixmap.h,v 1.4 2003/11/16 05:05:20 dawes Exp $ */

#ifndef XNESTPIXMAP_H
#define XNESTPIXMAP_H

#ifdef PIXPRIV
extern int xnestPixmapPrivateIndex;
#endif

typedef struct {
  Pixmap pixmap;
} xnestPrivPixmap;

#ifdef PIXPRIV
#define xnestPixmapPriv(pPixmap) \
  ((xnestPrivPixmap *)((pPixmap)->devPrivates[xnestPixmapPrivateIndex].ptr))
#else
#define xnestPixmapPriv(pPixmap) \
  ((xnestPrivPixmap *)((pPixmap)->devPrivate.ptr))
#endif

#define xnestPixmap(pPixmap) (xnestPixmapPriv(pPixmap)->pixmap)

#define xnestSharePixmap(pPixmap) ((pPixmap)->refcnt++)

PixmapPtr xnestCreatePixmap(ScreenPtr pScreen, int width, int height,
			    int depth);
Bool xnestDestroyPixmap(PixmapPtr pPixmap);
RegionPtr xnestPixmapToRegion(PixmapPtr pPixmap);

#endif /* XNESTPIXMAP_H */
