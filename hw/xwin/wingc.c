/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Harold L Hunt II
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/wingc.c,v 1.11 2003/08/07 23:47:58 alanh Exp $ */

#include "win.h"
void
winPushPixels(GCPtr pGC, PixmapPtr pBitMap, DrawablePtr pDrawable, int dx, int dy, int xOrg, int yOrg);

#if 0
/* GC Handling Routines */
const GCFuncs winGCFuncs = {
  winValidateGCNativeGDI,
  winChangeGCNativeGDI,
  winCopyGCNativeGDI,
  winDestroyGCNativeGDI,
  winChangeClipNativeGDI,
  winDestroyClipNativeGDI,
  winCopyClipNativeGDI,
};
#else
const GCFuncs winGCFuncs = {
  winValidateGCNativeGDI,
  miChangeGC,
  miCopyGC,
  winDestroyGCNativeGDI,
  miChangeClip,
  miDestroyClip,
  miCopyClip,
};
#endif

/* Drawing Primitives */
const GCOps winGCOps = {
  winFillSpansNativeGDI,
  winSetSpansNativeGDI,
  miPutImage,
  miCopyArea,
  miCopyPlane,
  miPolyPoint,
  winPolyLineNativeGDI,
  miPolySegment,
  miPolyRectangle,
  miPolyArc,
  miFillPolygon,
  miPolyFillRect,
  miPolyFillArc,
  miPolyText8,
  miPolyText16,
  miImageText8,
  miImageText16,
#if 0
  winImageGlyphBltNativeGDI,
  winPolyGlyphBltNativeGDI,
#else
  miImageGlyphBlt,
  miPolyGlyphBlt,
#endif
  winPushPixels
#ifdef NEED_LINEHELPER
  ,NULL
#endif
};


/* See Porting Layer Definition - p. 45 */
/* See mfb/mfbgc.c - mfbCreateGC() */
/* See Strategies for Porting - pp. 15, 16 */
Bool
winCreateGCNativeGDI (GCPtr pGC)
{
  winPrivGCPtr		pGCPriv = NULL;
  winPrivScreenPtr	pScreenPriv = NULL;

#if 0
  ErrorF ("winCreateGCNativeGDI - depth: %d\n",
	  pGC->depth);
#endif

  pGC->clientClip = NULL;
  pGC->clientClipType = CT_NONE;
  pGC->freeCompClip = FALSE;
  pGC->pCompositeClip = 0;

  pGC->ops = (GCOps *) &winGCOps;
  pGC->funcs = (GCFuncs *) &winGCFuncs;

  /* We want all coordinates passed to spans functions to be screen relative */
  pGC->miTranslate = TRUE;

  /* Allocate privates for this GC */
  pGCPriv = winGetGCPriv (pGC);
  if (pGCPriv == NULL)
    {
      ErrorF ("winCreateGCNativeGDI () - Privates pointer was NULL\n");
      return FALSE;
    }

  /* Create a new screen DC for the display window */
  pScreenPriv = winGetScreenPriv (pGC->pScreen);
  pGCPriv->hdc = GetDC (pScreenPriv->hwndScreen);

  /* Allocate a memory DC for the GC */
  pGCPriv->hdcMem = CreateCompatibleDC (pGCPriv->hdc);

  return TRUE;
}


/* See Porting Layer Definition - p. 45 */
void
winChangeGCNativeGDI (GCPtr pGC, unsigned long ulChanges)
{
#if 0
  ErrorF ("winChangeGCNativeGDI () - Doing nothing\n");
#endif
}


void
winValidateGCNativeGDI (GCPtr pGC,
			unsigned long ulChanges,
			DrawablePtr pDrawable)
{
  if ((ulChanges & (GCClipXOrigin | GCClipYOrigin | GCClipMask | GCSubwindowMode)) 
      || (pDrawable->serialNumber != (pGC->serialNumber & DRAWABLE_SERIAL_BITS)))
  {
    miComputeCompositeClip (pGC, pDrawable);
  }
}


/* See Porting Layer Definition - p. 46 */
void
winCopyGCNativeGDI (GCPtr pGCsrc, unsigned long ulMask, GCPtr pGCdst)
{

}


/* See Porting Layer Definition - p. 46 */
void
winDestroyGCNativeGDI (GCPtr pGC)
{
  winGCPriv(pGC);
  winScreenPriv(pGC->pScreen);

  if (pGC->freeCompClip)
	REGION_DESTROY (pGC->pScreen, pGC->pCompositeClip);

  /* Free the memory DC */
  if (pGCPriv->hdcMem != NULL)
    {
      DeleteDC (pGCPriv->hdcMem);
      pGCPriv->hdcMem = NULL;
    }

  /* Release the screen DC for the display window */
  if (pGCPriv->hdc != NULL)
    {
      ReleaseDC (pScreenPriv->hwndScreen, pGCPriv->hdc);
      pGCPriv->hdc = NULL;
    }

  /* Invalidate the GC privates pointer */
  winSetGCPriv (pGC, NULL);
}


/* See Porting Layer Definition - p. 46 */
void
winChangeClipNativeGDI (GCPtr pGC, int nType, pointer pValue, int nRects)
{

}


/* See Porting Layer Definition - p. 47 */
void
winDestroyClipNativeGDI (GCPtr pGC)
{

}


/* See Porting Layer Definition - p. 47 */
void
winCopyClipNativeGDI (GCPtr pGCdst, GCPtr pGCsrc)
{

}
