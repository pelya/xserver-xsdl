/*
 *
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "exaPriv.h"

/*
 * These functions wrap the low-level fb rendering functions and
 * synchronize framebuffer/accelerated drawing by stalling until
 * the accelerator is idle
 */

void
ExaCheckFillSpans  (DrawablePtr pDrawable, GCPtr pGC, int nspans,
		   DDXPointPtr ppt, int *pwidth, int fSorted)
{
    exaWaitSync (pDrawable->pScreen);
    exaDrawableDirty (pDrawable);
    fbFillSpans (pDrawable, pGC, nspans, ppt, pwidth, fSorted);
}

void
ExaCheckSetSpans (DrawablePtr pDrawable, GCPtr pGC, char *psrc,
		 DDXPointPtr ppt, int *pwidth, int nspans, int fSorted)
{
    exaWaitSync (pDrawable->pScreen);
    exaDrawableDirty (pDrawable);
    fbSetSpans (pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted);
}

void
ExaCheckPutImage (DrawablePtr pDrawable, GCPtr pGC, int depth,
		 int x, int y, int w, int h, int leftPad, int format,
		 char *bits)
{
    exaWaitSync (pDrawable->pScreen);
    exaDrawableDirty (pDrawable);
    fbPutImage (pDrawable, pGC, depth, x, y, w, h, leftPad, format, bits);
}

RegionPtr
ExaCheckCopyArea (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		 int srcx, int srcy, int w, int h, int dstx, int dsty)
{
    exaWaitSync (pSrc->pScreen);
    exaDrawableDirty (pDst);
    return fbCopyArea (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty);
}

RegionPtr
ExaCheckCopyPlane (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		  int srcx, int srcy, int w, int h, int dstx, int dsty,
		  unsigned long bitPlane)
{
    exaWaitSync (pSrc->pScreen);
    exaDrawableDirty (pDst);
    return fbCopyPlane (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty,
			bitPlane);
}

void
ExaCheckPolyPoint (DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		  DDXPointPtr pptInit)
{
    exaWaitSync (pDrawable->pScreen);
    exaDrawableDirty (pDrawable);
    fbPolyPoint (pDrawable, pGC, mode, npt, pptInit);
}

void
ExaCheckPolylines (DrawablePtr pDrawable, GCPtr pGC,
		  int mode, int npt, DDXPointPtr ppt)
{

    if (pGC->lineWidth == 0) {
	exaWaitSync(pDrawable->pScreen);
	exaDrawableDirty (pDrawable);
    }
    exaDrawableDirty (pDrawable);
    fbPolyLine (pDrawable, pGC, mode, npt, ppt);
}

void
ExaCheckPolySegment (DrawablePtr pDrawable, GCPtr pGC,
		    int nsegInit, xSegment *pSegInit)
{
    if (pGC->lineWidth == 0) {
	exaWaitSync(pDrawable->pScreen);
	exaDrawableDirty (pDrawable);
    }
    exaDrawableDirty (pDrawable);
    fbPolySegment (pDrawable, pGC, nsegInit, pSegInit);
}

void
ExaCheckPolyRectangle (DrawablePtr pDrawable, GCPtr pGC,
		      int nrects, xRectangle *prect)
{
    if (pGC->lineWidth == 0) {
	exaWaitSync(pDrawable->pScreen);
	exaDrawableDirty (pDrawable);
    }
    fbPolyRectangle (pDrawable, pGC, nrects, prect);
}

void
ExaCheckPolyArc (DrawablePtr pDrawable, GCPtr pGC,
		int narcs, xArc *pArcs)
{
    if (pGC->lineWidth == 0)
    {
	exaWaitSync(pDrawable->pScreen);
	exaDrawableDirty (pDrawable);
	fbPolyArc (pDrawable, pGC, narcs, pArcs);
    }
    else
	miPolyArc (pDrawable, pGC, narcs, pArcs);
}

#if 0
void
ExaCheckFillPolygon (DrawablePtr pDrawable, GCPtr pGC,
		    int shape, int mode, int count, DDXPointPtr pPts)
{
    exaWaitSync(pDrawable->pScreen);
    exaDrawableDirty (pDrawable);
    fbFillPolygon (pDrawable, pGC, mode, count, pPts);
}
#endif

void
ExaCheckPolyFillRect (DrawablePtr pDrawable, GCPtr pGC,
		     int nrect, xRectangle *prect)
{
    exaWaitSync(pDrawable->pScreen);
    exaDrawableDirty (pDrawable);
    fbPolyFillRect (pDrawable, pGC, nrect, prect);
}

void
ExaCheckPolyFillArc (DrawablePtr pDrawable, GCPtr pGC,
		    int narcs, xArc *pArcs)
{
    exaWaitSync(pDrawable->pScreen);
    exaDrawableDirty (pDrawable);
    fbPolyFillArc (pDrawable, pGC, narcs, pArcs);
}

void
ExaCheckImageGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		      int x, int y, unsigned int nglyph,
		      CharInfoPtr *ppci, pointer pglyphBase)
{
    exaWaitSync(pDrawable->pScreen);
    exaDrawableDirty (pDrawable);
    fbImageGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
}

void
ExaCheckPolyGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		     int x, int y, unsigned int nglyph,
		     CharInfoPtr *ppci, pointer pglyphBase)
{
    exaWaitSync(pDrawable->pScreen);
    exaDrawableDirty (pDrawable);
    fbPolyGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
}

void
ExaCheckPushPixels (GCPtr pGC, PixmapPtr pBitmap,
		   DrawablePtr pDrawable,
		   int w, int h, int x, int y)
{
    exaWaitSync(pDrawable->pScreen);
    exaDrawableDirty (pDrawable);
    fbPushPixels (pGC, pBitmap, pDrawable, w, h, x, y);
}

void
ExaCheckGetImage (DrawablePtr pDrawable,
		 int x, int y, int w, int h,
		 unsigned int format, unsigned long planeMask,
		 char *d)
{
    exaWaitSync(pDrawable->pScreen);
    fbGetImage (pDrawable, x, y, w, h, format, planeMask, d);
}

void
ExaCheckGetSpans (DrawablePtr pDrawable,
		 int wMax,
		 DDXPointPtr ppt,
		 int *pwidth,
		 int nspans,
		 char *pdstStart)
{
    exaWaitSync(pDrawable->pScreen);
    fbGetSpans (pDrawable, wMax, ppt, pwidth, nspans, pdstStart);
}

void
ExaCheckSaveAreas (PixmapPtr	pPixmap,
		  RegionPtr	prgnSave,
		  int		xorg,
		  int		yorg,
		  WindowPtr	pWin)
{
    exaWaitSync(pWin->drawable.pScreen);
    exaDrawableDirty (&pPixmap->drawable);
    fbSaveAreas (pPixmap, prgnSave, xorg, yorg, pWin);
}

void
ExaCheckRestoreAreas (PixmapPtr	pPixmap,
		     RegionPtr	prgnSave,
		     int	xorg,
		     int    	yorg,
		     WindowPtr	pWin)
{
    exaWaitSync(pWin->drawable.pScreen);
    exaDrawableDirty ((DrawablePtr)pWin);
    fbRestoreAreas (pPixmap, prgnSave, xorg, yorg, pWin);
}

void
ExaCheckPaintWindow (WindowPtr pWin, RegionPtr pRegion, int what)
{
    exaWaitSync (pWin->drawable.pScreen);
    exaDrawableDirty ((DrawablePtr)pWin);
    fbPaintWindow (pWin, pRegion, what);
}

void
ExaCheckComposite (CARD8      op,
                   PicturePtr pSrc,
                   PicturePtr pMask,
                   PicturePtr pDst,
                   INT16      xSrc,
                   INT16      ySrc,
                   INT16      xMask,
                   INT16      yMask,
                   INT16      xDst,
                   INT16      yDst,
                   CARD16     width,
                   CARD16     height)
{
    exaWaitSync (pDst->pDrawable->pScreen);
    exaDrawableDirty (pDst->pDrawable);
    fbComposite (op,
                 pSrc,
                 pMask,
                 pDst,
                 xSrc,
                 ySrc,
                 xMask,
                 yMask,
                 xDst,
                 yDst,
                 width,
                 height);
}

/*
 * Only need to stall for copyarea/copyplane
 */
const GCOps exaAsyncPixmapGCOps = {
    fbFillSpans,
    fbSetSpans,
    fbPutImage,
    ExaCheckCopyArea,
    ExaCheckCopyPlane,
    fbPolyPoint,
    fbPolyLine,
    fbPolySegment,
    fbPolyRectangle,
    fbPolyArc,
    fbFillPolygon,
    fbPolyFillRect,
    fbPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    fbImageGlyphBlt,
    fbPolyGlyphBlt,
    fbPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};
