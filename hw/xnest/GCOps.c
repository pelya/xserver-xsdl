/* $Xorg: GCOps.c,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
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
/* $XFree86: xc/programs/Xserver/hw/xnest/GCOps.c,v 3.4 2001/01/17 22:36:55 dawes Exp $ */

#include "X.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "fontstruct.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "region.h"
#include "servermd.h"

#include "Xnest.h"

#include "Display.h"
#include "Screen.h"
#include "XNGC.h"
#include "XNFont.h"
#include "GCOps.h"
#include "Drawable.h"
#include "Visual.h"

void xnestFillSpans(pDrawable, pGC, nSpans, pPoints, pWidths, fSorted)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int nSpans;
     xPoint *pPoints;
     int *pWidths;
     int fSorted;
{
  ErrorF("xnest warning: function xnestFillSpans not implemented\n");
}

void xnestSetSpans(pDrawable, pGC, pSrc, pPoints, pWidths, nSpans, fSorted)
     DrawablePtr pDrawable;
     GCPtr pGC;
     unsigned char * pSrc;
     xPoint *pPoints;
     int *pWidths;
     int nSpans;
     int fSorted;
{
  ErrorF("xnest warning: function xnestSetSpans not implemented\n");
}

void xnestGetSpans(pDrawable, maxWidth, pPoints, pWidths, nSpans, pBuffer)
     DrawablePtr pDrawable; 
     int maxWidth;
     xPoint *pPoints;
     int *pWidths;
     int nSpans;
     int *pBuffer;
{
  ErrorF("xnest warning: function xnestGetSpans not implemented\n");
}

void xnestQueryBestSize(class, pWidth, pHeight, pScreen)
     int class;
     short *pWidth;
     short *pHeight;
     ScreenPtr pScreen;
{
  unsigned int width, height;

  width = *pWidth;
  height = *pHeight;

  XQueryBestSize(xnestDisplay, class, 
		 xnestDefaultWindows[pScreen->myNum], 
		 width, height, &width, &height);
  
  *pWidth = width;
  *pHeight = height;
}

void xnestPutImage(pDrawable, pGC, depth, x, y, w, h, leftPad, format, pImage)
     DrawablePtr pDrawable;
     GCPtr       pGC;
     int         depth, x, y, w, h;
     int         leftPad;
     unsigned int format;
     unsigned char *pImage;
{
  XImage *ximage;
  
  ximage = XCreateImage(xnestDisplay, xnestDefaultVisual(pDrawable->pScreen), 
			depth, format, leftPad, (char *)pImage, 
			w, h, BitmapPad(xnestDisplay), 
			(format == ZPixmap) ? 
			   PixmapBytePad(w, depth) : BitmapBytePad(w+leftPad));
  
  if (ximage) {
      XPutImage(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC), 
		ximage, 0, 0, x, y, w, h);
      XFree(ximage);
  }
}

void xnestGetImage(pDrawable, x, y, w, h, format, planeMask, pImage)
     DrawablePtr pDrawable;
     int         x, y, w, h;
     unsigned int format;
     unsigned long planeMask;
     unsigned char *pImage;
{
  XImage *ximage;
  int length;

  ximage = XGetImage(xnestDisplay, xnestDrawable(pDrawable),
                     x, y, w, h, planeMask, format);

  if (ximage) {
      length = ximage->bytes_per_line * ximage->height;
  
      memmove(pImage, ximage->data, length);
  
      XDestroyImage(ximage);
  }
}

static Bool xnestBitBlitPredicate(display, event, args)
     Display *display;
     XEvent *event;
     char *args;
{
  return (event->type == GraphicsExpose || event->type == NoExpose);
}

RegionPtr xnestBitBlitHelper(pGC)
     GC *pGC;
{
  if (!pGC->graphicsExposures) 
    return NullRegion;
  else {
    XEvent event;
    RegionPtr pReg, pTmpReg;
    BoxRec Box;
    Bool pending, overlap;

    pReg = REGION_CREATE(pGC->pScreen, NULL, 1);
    pTmpReg = REGION_CREATE(pGC->pScreen, NULL, 1);
    if(!pReg || !pTmpReg) return NullRegion;
    
    pending = True;
    while (pending) {
      XIfEvent(xnestDisplay, &event, xnestBitBlitPredicate, NULL);
      
      switch (event.type) {
      case NoExpose:
	pending = False;
	break;
	
      case GraphicsExpose:
	Box.x1 = event.xgraphicsexpose.x;
	Box.y1 = event.xgraphicsexpose.y;
	Box.x2 = event.xgraphicsexpose.x + event.xgraphicsexpose.width;
	Box.y2 = event.xgraphicsexpose.y + event.xgraphicsexpose.height;
	REGION_RESET(pGC->pScreen, pTmpReg, &Box);
	REGION_APPEND(pGC->pScreen, pReg, pTmpReg);
	pending = event.xgraphicsexpose.count;
	break;
      }
    }

    REGION_DESTROY(pGC->pScreen, pTmpReg);
    REGION_VALIDATE(pGC->pScreen, pReg, &overlap);
    return(pReg);
  }
}

RegionPtr xnestCopyArea(pSrcDrawable, pDstDrawable,
			pGC, srcx, srcy, width, height, dstx, dsty)
     DrawablePtr pSrcDrawable;
     DrawablePtr pDstDrawable;
     GC *pGC;
     int srcx, srcy;
     int width, height;
     int dstx, dsty;
{
  XCopyArea(xnestDisplay, 
	    xnestDrawable(pSrcDrawable), xnestDrawable(pDstDrawable),
	    xnestGC(pGC), srcx, srcy, width, height, dstx, dsty);
  
  return xnestBitBlitHelper(pGC);
}

RegionPtr xnestCopyPlane(pSrcDrawable, pDstDrawable,
			 pGC, srcx, srcy, width, height, dstx, dsty, plane)
     DrawablePtr pSrcDrawable;
     DrawablePtr pDstDrawable;
     GC *pGC;
     int srcx, srcy;
     int width, height;
     int dstx, dsty;
     unsigned long plane;
{
  XCopyPlane(xnestDisplay, 
	     xnestDrawable(pSrcDrawable), xnestDrawable(pDstDrawable),
	     xnestGC(pGC), srcx, srcy, width, height, dstx, dsty, plane);
  
  return xnestBitBlitHelper(pGC);
}

void xnestPolyPoint(pDrawable, pGC, mode, nPoints, pPoints)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int mode;
     int nPoints;
     XPoint *pPoints;
{
  XDrawPoints(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC), 
              pPoints, nPoints, mode);
}

void xnestPolylines(pDrawable, pGC, mode, nPoints, pPoints)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int mode;
     int nPoints;
     XPoint *pPoints;
{
  XDrawLines(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC), 
              pPoints, nPoints, mode);
}

void xnestPolySegment(pDrawable, pGC, nSegments, pSegments)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int nSegments;
     XSegment *pSegments;
{
  XDrawSegments(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC), 
                pSegments, nSegments);
}

void xnestPolyRectangle(pDrawable, pGC, nRectangles, pRectangles)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int nRectangles;
     XRectangle *pRectangles;
{
  XDrawRectangles(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                  pRectangles, nRectangles);
}

void xnestPolyArc(pDrawable, pGC, nArcs, pArcs)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int nArcs;
     XArc *pArcs;
{
  XDrawArcs(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
            pArcs, nArcs);
}

void xnestFillPolygon(pDrawable, pGC, shape, mode, nPoints, pPoints)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int shape;
     int mode;
     int nPoints;
     XPoint *pPoints;
{
  XFillPolygon(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC), 
               pPoints, nPoints, shape, mode);
}

void xnestPolyFillRect(pDrawable, pGC, nRectangles, pRectangles)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int nRectangles;
     XRectangle *pRectangles;
{
  XFillRectangles(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                  pRectangles, nRectangles);
}

void xnestPolyFillArc(pDrawable, pGC, nArcs, pArcs)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int nArcs;
     XArc *pArcs;
{
  XFillArcs(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
            pArcs, nArcs);
}

int xnestPolyText8(pDrawable, pGC, x, y, count, string)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int x, y;
     int count;
     char *string;
{
  int width;

  XDrawString(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
              x, y, string, count);
  
  width = XTextWidth(xnestFontStruct(pGC->font), string, count);
  
  return width + x;
}

int xnestPolyText16(pDrawable, pGC, x, y, count, string)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int x, y;
     int count;
     XChar2b *string;
{
  int width;

  XDrawString16(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                x, y, string, count);

  width = XTextWidth16(xnestFontStruct(pGC->font), string, count);

  return width + x;
}

void xnestImageText8(pDrawable, pGC, x, y, count, string)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int x, y;
     int count;
     char *string;
{
  XDrawImageString(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                   x, y, string, count);
}

void xnestImageText16(pDrawable, pGC, x, y, count, string)
     DrawablePtr pDrawable;
     GCPtr pGC;
     int x, y;
     int count;
     XChar2b *string;
{
  XDrawImageString16(xnestDisplay, xnestDrawable(pDrawable), xnestGC(pGC),
                     x, y, string, count);
}

void xnestImageGlyphBlt(pDrawable, pGC, x, y, nGlyphs, pCharInfo, pGlyphBase)
     DrawablePtr pDrawable;
     GC pGC;
     int x, y;
     int nGlyphs;
     CharInfoPtr pCharInfo;
     char pGlyphBase;
{
  ErrorF("xnest warning: function xnestImageGlyphBlt not implemented\n");
}

void xnestPolyGlyphBlt(pDrawable, pGC, x, y, nGlyphs, pCharInfo, pGlyphBase)
     DrawablePtr pDrawable;
     GC pGC;
     int x, y;
     int nGlyphs;
     CharInfoPtr pCharInfo;
     char pGlyphBase;
{
  ErrorF("xnest warning: function xnestPolyGlyphBlt not implemented\n");
}

void xnestPushPixels(pDrawable, pGC, pBitmap, width, height, x, y)
     DrawablePtr pDrawable;
     GC pGC;
     PixmapPtr pBitmap;
     int width, height;
     int x, y;
{
  ErrorF("xnest warning: function xnestPushPixels not implemented\n");
}
