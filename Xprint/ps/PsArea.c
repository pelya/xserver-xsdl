/* $Xorg: PsArea.c,v 1.6 2001/03/14 18:27:44 pookie Exp $ */
/*

Copyright 1996, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/*
 * (c) Copyright 1996 Hewlett-Packard Company
 * (c) Copyright 1996 International Business Machines Corp.
 * (c) Copyright 1996, 2000 Sun Microsystems, Inc.
 * (c) Copyright 1996 Novell, Inc.
 * (c) Copyright 1996 Digital Equipment Corp.
 * (c) Copyright 1996 Fujitsu Limited
 * (c) Copyright 1996 Hitachi, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the names of the copyright holders
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * from said copyright holders.
 */

/*******************************************************************
**
**    *********************************************************
**    *
**    *  File:		PsArea.c
**    *
**    *  Contents:	Image and Area functions for the PS DDX driver
**    *
**    *  Created By:	Roger Helmendach (Liberty Systems)
**    *
**    *  Copyright:	Copyright 1996 The Open Group, Inc.
**    *
**    *********************************************************
** 
********************************************************************/

#include "Ps.h"
#include "gcstruct.h"
#include "windowstr.h"

void
PsPutScaledImage(DrawablePtr pDrawable, GCPtr pGC, int depth, int x, int y,
           int w, int h, int leftPad, int format, int imageRes, char *pImage)
{
  if( pDrawable->type==DRAWABLE_PIXMAP )
  {
    int             size = PixmapBytePad(w, depth)*h;
    DisplayElmPtr   elm;
    PixmapPtr       pix  = (PixmapPtr)pDrawable;
    PsPixmapPrivPtr priv = (PsPixmapPrivPtr)pix->devPrivate.ptr;
    DisplayListPtr  disp;
    GCPtr           gc;   

    if ((gc = PsCreateAndCopyGC(pDrawable, pGC)) == NULL) return;

    disp = PsGetFreeDisplayBlock(priv);
    elm  = &disp->elms[disp->nelms];
    elm->type = PutImageCmd;
    elm->gc = gc;
    elm->c.image.depth   = depth;
    elm->c.image.x       = x;
    elm->c.image.y       = y;
    elm->c.image.w       = w;
    elm->c.image.h       = h;
    elm->c.image.leftPad = leftPad;
    elm->c.image.format  = format;
    elm->c.image.res     = imageRes;
    elm->c.image.pData   = (char *)xalloc(size);
    memcpy(elm->c.image.pData, pImage, size);
    disp->nelms += 1;
  }
  else
  {
    int          i, j;
    int          r, c;
    int          swap;
    char        *pt;
    PsOutPtr     psOut;
    ColormapPtr  cMap;
    int          pageRes, sw, sh;

    if( PsUpdateDrawableGC(pGC, pDrawable, &psOut, &cMap)==FALSE ) return;
    if (!imageRes) {
	sw = w;
	sh = h;
    } else {
	pageRes = XpGetResolution(XpGetPrintContext(requestingClient));
	sw = (float)w * (float)pageRes / (float)imageRes + 0.5;
	sh = (float)h * (float)pageRes / (float)imageRes + 0.5;
    }
    PsOut_Offset(psOut, pDrawable->x, pDrawable->y);
    pt = (char *)(&i); i = 1; if( pt[0]=='\001' ) swap = 1; else swap = 0;

    if( depth==24 )
    {
      PsOut_BeginImage(psOut, 0, 0, x, y, w, h, sw, sh, 3);
      if( format==XYPixmap )
      {
        int   rowsiz = PixmapBytePad(w, depth);
        char *planes[3];
        planes[0] = pImage;
        planes[1] = &pImage[rowsiz*h];
        planes[2] = &pImage[rowsiz*h*2];
        for( r=0 ; r<h ; r++ )
        {
          char *pt[3];
          for( i=0 ; i<3 ;  i++ ) pt[i] = &planes[i][rowsiz*r];
          for( c=0 ; c<w ; c++ )
          {
            for( i=0 ; i<3 ; i++ )
              { PsOut_OutImageBytes(psOut, 1, &pt[i][c]); pt[i]++; }
          }
        }
      }
      else if( format==ZPixmap )
      {
        int  rowsiz = PixmapBytePad(w, depth);
        for( r=0 ; r<h ; r++ )
        {
          char *pt = &pImage[rowsiz*r];
          for( c=0 ; c<w ; c++,pt+=4 )
          {
            if( swap )
            {
              char tmp[4];
              tmp[0] = pt[3]; tmp[1] = pt[2]; tmp[2] = pt[1]; tmp[3] = pt[0];
              PsOut_OutImageBytes(psOut, 3, &tmp[1]);
            }
            else
              PsOut_OutImageBytes(psOut, 3, &pt[1]);
          }
        }
      }
      else goto error;
      PsOut_EndImage(psOut);
    }
    else if( depth==8 )
    {
      int  rowsiz = PixmapBytePad(w, depth);
      PsOut_BeginImage(psOut, 0, 0, x, y, w, h, sw, sh, 3);
      for( r=0 ; r<h ; r++ )
      {
        char *pt = &pImage[rowsiz*r];
        for( c=0 ; c<w ; c++,pt++ )
        {
          int   val = PsGetPixelColor(cMap, (int)(*pt)&0xFF);
          char *ipt = (char *)&val;
          if( swap )
          {
            char tmp[4];
            tmp[0] = ipt[3]; tmp[1] = ipt[2]; tmp[2] = ipt[1]; tmp[3] = ipt[0];
            PsOut_OutImageBytes(psOut, 3, &tmp[1]);
          }
          else
            PsOut_OutImageBytes(psOut, 3, &ipt[1]);
        }
      }
      PsOut_EndImage(psOut);
    }
    else if( depth==1 )
    {
      {
        int  rowsiz = BitmapBytePad(w);
        int  psrsiz = (w+7)/8;
        PsOut_BeginImage(psOut, PsGetPixelColor(cMap, pGC->bgPixel),
                         PsGetPixelColor(cMap, pGC->fgPixel),
			 x, y, w, h, sw, sh, 1);
        for( r=0 ; r<h ; r++ )
        {
          char *pt = &pImage[rowsiz*r];
          for( i=0 ; i<psrsiz ; i++ )
          {
            int  iv_, iv = (int)pt[i]&0xFF;
            char c;
            if( swap )
              { for( j=0,iv_=0 ; j<8 ; j++ ) iv_ |= (((iv>>j)&1)<<(7-j)); }
            else
              iv_ = iv;
            c = iv_;
            PsOut_OutImageBytes(psOut, 1, &c);
          }
        }
        PsOut_EndImage(psOut);
      }
    }
  }
error:
  return;
}

static void
PsPutScaledImageIM(DrawablePtr pDrawable, GCPtr pGC, int depth, int x, int y,
           int w, int h, int leftPad, int format, int imageRes, char *pImage)
{
  if( pDrawable->type==DRAWABLE_PIXMAP )
  {
    int             size = PixmapBytePad(w, depth)*h;
    DisplayElmPtr   elm;
    PixmapPtr       pix  = (PixmapPtr)pDrawable;
    PsPixmapPrivPtr priv = (PsPixmapPrivPtr)pix->devPrivate.ptr;
    DisplayListPtr  disp;
    GCPtr           gc;

    if ((gc = PsCreateAndCopyGC(pDrawable, pGC)) == NULL) return;

    disp = PsGetFreeDisplayBlock(priv);
    elm  = &disp->elms[disp->nelms];
    elm->type = PutImageCmd;
    elm->gc = gc;
    elm->c.image.depth   = depth;
    elm->c.image.x       = x;
    elm->c.image.y       = y;
    elm->c.image.w       = w;
    elm->c.image.h       = h;
    elm->c.image.leftPad = leftPad;
    elm->c.image.format  = format;
    elm->c.image.res     = imageRes;
    elm->c.image.pData   = (char *)xalloc(size);
    memcpy(elm->c.image.pData, pImage, size);
    disp->nelms += 1;
  }
  else
  {
    int          i, j;
    int          r, c;
    int          swap;
    char        *pt;
    PsOutPtr     psOut;
    ColormapPtr  cMap;
    int          pageRes, sw, sh;
#ifdef BM_CACHE
    long   cache_id = 0;
#endif
    
    if( PsUpdateDrawableGC(pGC, pDrawable, &psOut, &cMap)==FALSE ) return;
    if (!imageRes) {
        sw = w;
        sh = h;
    } else {
        pageRes = XpGetResolution(XpGetPrintContext(requestingClient));
        sw = (float)w * (float)pageRes / (float)imageRes + 0.5;
        sh = (float)h * (float)pageRes / (float)imageRes + 0.5;
    }
    PsOut_Offset(psOut, pDrawable->x, pDrawable->y);
    pt = (char *)(&i); i = 1; if( pt[0]=='\001' ) swap = 1; else swap = 0;

#ifdef BM_CACHE
    cache_id = PsBmIsImageCached(w, h, pImage);

    if(!cache_id)
    {
      cache_id = PsBmPutImageInCache(w, h, pImage);

      if(!cache_id)
         return;

      PsOut_BeginImageCache(psOut, cache_id);
#endif
    if( depth==24 )
    {
      PsOut_BeginImageIM(psOut, 0, 0, x, y, w, h, sw, sh, 3);
      if( format==XYPixmap )
      {
        int   rowsiz = PixmapBytePad(w, depth);
        char *planes[3];
        planes[0] = pImage;
        planes[1] = &pImage[rowsiz*h];
        planes[2] = &pImage[rowsiz*h*2];
        for( r=0 ; r<h ; r++ )
        {
          char *pt[3];
          for( i=0 ; i<3 ;  i++ ) pt[i] = &planes[i][rowsiz*r];
          for( c=0 ; c<w ; c++ )
          {
            for( i=0 ; i<3 ; i++ )
              { PsOut_OutImageBytes(psOut, 1, &pt[i][c]); pt[i]++; }
          }
        }
      }
      else if( format==ZPixmap )
      {
        int  rowsiz = PixmapBytePad(w, depth);
        for( r=0 ; r<h ; r++ )
        {
          char *pt = &pImage[rowsiz*r];
          for( c=0 ; c<w ; c++,pt+=4 )
          {
            if( swap )
            {
              char tmp[4];
              tmp[0] = pt[3]; tmp[1] = pt[2]; tmp[2] = pt[1]; tmp[3] = pt[0];
              PsOut_OutImageBytes(psOut, 3, &tmp[1]);
            }
            else
              PsOut_OutImageBytes(psOut, 3, &pt[1]);
          }
        }
      }
      else goto error;
      PsOut_EndImage(psOut);
    }
    else if( depth==8 )
    {
      int  rowsiz = PixmapBytePad(w, depth);
      PsOut_BeginImageIM(psOut, 0, 0, x, y, w, h, sw, sh, 3);
      for( r=0 ; r<h ; r++ )
      {
        char *pt = &pImage[rowsiz*r];
        for( c=0 ; c<w ; c++,pt++ )
        {
          int   val = PsGetPixelColor(cMap, (int)(*pt)&0xFF);
          char *ipt = (char *)&val;
          if( swap )
          {
            char tmp[4];
            tmp[0] = ipt[3]; tmp[1] = ipt[2]; tmp[2] = ipt[1]; tmp[3] = ipt[0];
            PsOut_OutImageBytes(psOut, 3, &tmp[1]);
          }
          else
            PsOut_OutImageBytes(psOut, 3, &ipt[1]);
        }
      }
      PsOut_EndImage(psOut);
    }
    else if( depth==1 )
    {
      {
        int  rowsiz = BitmapBytePad(w);
        int  psrsiz = (w+7)/8;
        PsOut_BeginImageIM(psOut, PsGetPixelColor(cMap, pGC->bgPixel),
                         PsGetPixelColor(cMap, pGC->fgPixel),
                         x, y, w, h, sw, sh, 1);
        for( r=0 ; r<h ; r++ )
        {
          char *pt = &pImage[rowsiz*r];
          for( i=0 ; i<psrsiz ; i++ )
          {
            int  iv_, iv = (int)pt[i]&0xFF;
            char c;
            if( swap )
              { for( j=0,iv_=0 ; j<8 ; j++ ) iv_ |= (((iv>>j)&1)<<(7-j)); }
            else
              iv_ = iv;
            c = iv_;
            PsOut_OutImageBytes(psOut, 1, &c);
          }
        }
        PsOut_EndImage(psOut);
      }
    }
#ifdef BM_CACHE
    PsOut_EndImageCache(psOut);
    }
    PsOut_ImageCache(psOut, x, y, cache_id, PsGetPixelColor(cMap, pGC->bgPixel),
                           PsGetPixelColor(cMap, pGC->fgPixel));
#endif
  }
error:
  return;
}
void
PsPutImage(DrawablePtr pDrawable, GCPtr pGC, int depth, int x, int y,
           int w, int h, int leftPad, int format, char *pImage)
{
    XpContextPtr pcon;
    if (requestingClient && (pcon = XpGetPrintContext(requestingClient)))
	PsPutScaledImage(pDrawable, pGC, depth, x, y, w, h, leftPad, format,
			 pcon->imageRes, pImage);
}
void
PsPutImageMask(DrawablePtr pDrawable, GCPtr pGC, int depth, int x, int y,
           int w, int h, int leftPad, int format, char *pImage)
{
    XpContextPtr pcon;
    if (requestingClient && (pcon = XpGetPrintContext(requestingClient)))
        PsPutScaledImageIM(pDrawable, pGC, depth, x, y, w, h, leftPad, format,
                         pcon->imageRes, pImage);
}

RegionPtr
PsCopyArea(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC, int srcx, int srcy,
           int width, int height, int dstx, int dsty)
{
  PixmapPtr  src = (PixmapPtr)pSrc;
  PixmapPtr  dst = (PixmapPtr)pDst;

  if( pSrc->type!=DRAWABLE_PIXMAP ) return NULL;
  if( pDst->type!=DRAWABLE_PIXMAP )
  {
    PsOutPtr     psOut;
    ColormapPtr  cMap;
    if( PsUpdateDrawableGC(pGC, pDst, &psOut, &cMap)==FALSE ) return NULL;
    PsOut_Offset(psOut, pDst->x, pDst->y);
    PsOut_BeginFrame(psOut, dstx-srcx, dsty-srcy, dstx, dsty, width, height);
    PsReplayPixmap(src, pDst);
    PsOut_EndFrame(psOut);
  }
  else PsCopyDisplayList(src, dst, dstx-srcx, dsty-srcy, dstx, dsty,
                         width, height);
  return NULL;
}

RegionPtr
PsCopyPlane(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC, int srcx, int srcy,
            int width, int height, int dstx, int dsty, unsigned long plane)
{
  PixmapPtr  src = (PixmapPtr)pSrc;
  PixmapPtr  dst = (PixmapPtr)pDst;

  if( pSrc->type!=DRAWABLE_PIXMAP ) return NULL;
  if( pDst->type!=DRAWABLE_PIXMAP )
  {
    PsOutPtr     psOut;
    ColormapPtr  cMap;
    if( PsUpdateDrawableGC(pGC, pDst, &psOut, &cMap)==FALSE ) return NULL;
    PsOut_Offset(psOut, pDst->x, pDst->y);
    PsOut_BeginFrame(psOut, dstx-srcx, dsty-srcy, dstx, dsty, width, height);
    PsReplayPixmap(src, pDst);
    PsOut_EndFrame(psOut);
  }
  else PsCopyDisplayList(src, dst, dstx-srcx, dsty-srcy, dstx, dsty,
                         width, height);
  return NULL;
}
