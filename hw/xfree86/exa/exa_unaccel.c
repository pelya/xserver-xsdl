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
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbFillSpans (pDrawable, pGC, nspans, ppt, pwidth, fSorted);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckSetSpans (DrawablePtr pDrawable, GCPtr pGC, char *psrc,
		 DDXPointPtr ppt, int *pwidth, int nspans, int fSorted)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbSetSpans (pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckPutImage (DrawablePtr pDrawable, GCPtr pGC, int depth,
		 int x, int y, int w, int h, int leftPad, int format,
		 char *bits)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbPutImage (pDrawable, pGC, depth, x, y, w, h, leftPad, format, bits);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

RegionPtr
ExaCheckCopyArea (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		 int srcx, int srcy, int w, int h, int dstx, int dsty)
{
    RegionPtr ret;

    EXA_FALLBACK(("from 0x%lx to 0x%lx\n", (long)pSrc, (long)pDst));
    exaPrepareAccess (pDst, EXA_PREPARE_DEST);
    exaPrepareAccess (pSrc, EXA_PREPARE_SRC);
    ret = fbCopyArea (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty);
    exaFinishAccess (pSrc, EXA_PREPARE_SRC);
    exaFinishAccess (pDst, EXA_PREPARE_DEST);

    return ret;
}

RegionPtr
ExaCheckCopyPlane (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		  int srcx, int srcy, int w, int h, int dstx, int dsty,
		  unsigned long bitPlane)
{
    RegionPtr ret;

    EXA_FALLBACK(("from 0x%lx to 0x%lx\n", (long)pSrc, (long)pDst));
    exaPrepareAccess (pDst, EXA_PREPARE_DEST);
    exaPrepareAccess (pSrc, EXA_PREPARE_SRC);
    ret = fbCopyPlane (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty,
		       bitPlane);
    exaFinishAccess (pSrc, EXA_PREPARE_SRC);
    exaFinishAccess (pDst, EXA_PREPARE_DEST);

    return ret;
}

void
ExaCheckPolyPoint (DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		  DDXPointPtr pptInit)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbPolyPoint (pDrawable, pGC, mode, npt, pptInit);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckPolylines (DrawablePtr pDrawable, GCPtr pGC,
		  int mode, int npt, DDXPointPtr ppt)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));

    if (pGC->lineWidth == 0) {
	exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
	fbPolyLine (pDrawable, pGC, mode, npt, ppt);
	exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
	return;
    }
    fbPolyLine (pDrawable, pGC, mode, npt, ppt);
}

void
ExaCheckPolySegment (DrawablePtr pDrawable, GCPtr pGC,
		    int nsegInit, xSegment *pSegInit)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    if (pGC->lineWidth == 0) {
	exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
	fbPolySegment (pDrawable, pGC, nsegInit, pSegInit);
	exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
	return;
    }
    fbPolySegment (pDrawable, pGC, nsegInit, pSegInit);
}

void
ExaCheckPolyRectangle (DrawablePtr pDrawable, GCPtr pGC,
		      int nrects, xRectangle *prect)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    if (pGC->lineWidth == 0) {
	exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
	fbPolyRectangle (pDrawable, pGC, nrects, prect);
	exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
	return;
    }
    fbPolyRectangle (pDrawable, pGC, nrects, prect);
}

void
ExaCheckPolyArc (DrawablePtr pDrawable, GCPtr pGC,
		int narcs, xArc *pArcs)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    if (pGC->lineWidth == 0)
    {
	exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
	fbPolyArc (pDrawable, pGC, narcs, pArcs);
	exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
	return;
    }
    miPolyArc (pDrawable, pGC, narcs, pArcs);
}

#if 0
void
ExaCheckFillPolygon (DrawablePtr pDrawable, GCPtr pGC,
		    int shape, int mode, int count, DDXPointPtr pPts)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbFillPolygon (pDrawable, pGC, mode, count, pPts);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}
#endif

void
ExaCheckPolyFillRect (DrawablePtr pDrawable, GCPtr pGC,
		     int nrect, xRectangle *prect)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbPolyFillRect (pDrawable, pGC, nrect, prect);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckPolyFillArc (DrawablePtr pDrawable, GCPtr pGC,
		    int narcs, xArc *pArcs)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbPolyFillArc (pDrawable, pGC, narcs, pArcs);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckImageGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		      int x, int y, unsigned int nglyph,
		      CharInfoPtr *ppci, pointer pglyphBase)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbImageGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckPolyGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		     int x, int y, unsigned int nglyph,
		     CharInfoPtr *ppci, pointer pglyphBase)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbPolyGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckPushPixels (GCPtr pGC, PixmapPtr pBitmap,
		   DrawablePtr pDrawable,
		   int w, int h, int x, int y)
{
    EXA_FALLBACK(("from 0x%lx to 0x%lx\n", (long)pBitmap, (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbPushPixels (pGC, pBitmap, pDrawable, w, h, x, y);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckGetImage (DrawablePtr pDrawable,
		 int x, int y, int w, int h,
		 unsigned int format, unsigned long planeMask,
		 char *d)
{
    EXA_FALLBACK(("from 0x%lx\n", (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_SRC);
    fbGetImage (pDrawable, x, y, w, h, format, planeMask, d);
    exaFinishAccess (pDrawable, EXA_PREPARE_SRC);
}

void
ExaCheckGetSpans (DrawablePtr pDrawable,
		 int wMax,
		 DDXPointPtr ppt,
		 int *pwidth,
		 int nspans,
		 char *pdstStart)
{
    EXA_FALLBACK(("from 0x%lx\n", (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_SRC);
    fbGetSpans (pDrawable, wMax, ppt, pwidth, nspans, pdstStart);
    exaFinishAccess (pDrawable, EXA_PREPARE_SRC);
}

void
ExaCheckSaveAreas (PixmapPtr	pPixmap,
		  RegionPtr	prgnSave,
		  int		xorg,
		  int		yorg,
		  WindowPtr	pWin)
{
    EXA_FALLBACK(("from 0x%lx\n", (long)&pPixmap->drawable));
    exaPrepareAccess ((DrawablePtr)pPixmap, EXA_PREPARE_DEST);
    fbSaveAreas (pPixmap, prgnSave, xorg, yorg, pWin);
    exaFinishAccess ((DrawablePtr)pPixmap, EXA_PREPARE_DEST);
}

void
ExaCheckRestoreAreas (PixmapPtr	pPixmap,
		     RegionPtr	prgnSave,
		     int	xorg,
		     int    	yorg,
		     WindowPtr	pWin)
{
    EXA_FALLBACK(("to 0x%lx\n", (long)&pPixmap->drawable));
    exaPrepareAccess ((DrawablePtr)pPixmap, EXA_PREPARE_DEST);
    fbRestoreAreas (pPixmap, prgnSave, xorg, yorg, pWin);
    exaFinishAccess ((DrawablePtr)pPixmap, EXA_PREPARE_DEST);
}

void
ExaCheckPaintWindow (WindowPtr pWin, RegionPtr pRegion, int what)
{
    EXA_FALLBACK(("from 0x%lx\n", (long)pWin));
    exaPrepareAccess (&pWin->drawable, EXA_PREPARE_DEST);
    fbPaintWindow (pWin, pRegion, what);
    exaFinishAccess (&pWin->drawable, EXA_PREPARE_DEST);
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
    EXA_FALLBACK(("from picts 0x%lx/0x%lx to pict 0x%lx\n",
		 (long)pSrc, (long)pMask, (long)pDst));
    exaPrepareAccess (pDst->pDrawable, EXA_PREPARE_DEST);
    exaPrepareAccess (pSrc->pDrawable, EXA_PREPARE_SRC);
    if (pMask)
	exaPrepareAccess (pMask->pDrawable, EXA_PREPARE_MASK);
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
    if (pMask)
	exaFinishAccess (pMask->pDrawable, EXA_PREPARE_MASK);
    exaFinishAccess (pSrc->pDrawable, EXA_PREPARE_SRC);
    exaFinishAccess (pDst->pDrawable, EXA_PREPARE_DEST);
}

/*
 * Only need to stall for CopyArea/CopyPlane, but we want to have the chance to
 * do migration for CopyArea.
 */
const GCOps exaAsyncPixmapGCOps = {
    ExaCheckFillSpans,
    ExaCheckSetSpans,
    ExaCheckPutImage,
    exaCopyArea,
    ExaCheckCopyPlane,
    ExaCheckPolyPoint,
    ExaCheckPolylines,
    ExaCheckPolySegment,
    ExaCheckPolyRectangle,
    ExaCheckPolyArc,
    ExaCheckFillPolygon,
    ExaCheckPolyFillRect,
    ExaCheckPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    ExaCheckImageGlyphBlt,
    ExaCheckPolyGlyphBlt,
    ExaCheckPushPixels
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};
