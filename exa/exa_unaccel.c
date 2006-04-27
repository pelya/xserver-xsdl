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

#include "exa_priv.h"

/*
 * These functions wrap the low-level fb rendering functions and
 * synchronize framebuffer/accelerated drawing by stalling until
 * the accelerator is idle
 */

/**
 * Calls exaPrepareAccess with EXA_PREPARE_SRC for the tile, if that is the
 * current fill style.
 *
 * Solid doesn't use an extra pixmap source, and Stippled/OpaqueStippled are
 * 1bpp and never in fb, so we don't worry about them.
 */
void
exaPrepareAccessGC(GCPtr pGC)
{
    if (pGC->fillStyle == FillTiled)
	exaPrepareAccess(&pGC->tile.pixmap->drawable, EXA_PREPARE_SRC);
}

/**
 * Finishes access to the tile in the GC, if used.
 */
void
exaFinishAccessGC(GCPtr pGC)
{
    if (pGC->fillStyle == FillTiled)
	exaFinishAccess(&pGC->tile.pixmap->drawable, EXA_PREPARE_SRC);
}

#if DEBUG_TRACE_FALL
char
exaDrawableLocation(DrawablePtr pDrawable)
{
    return exaDrawableIsOffscreen(pDrawable) ? 's' : 'm';
}
#endif /* DEBUG_TRACE_FALL */

void
ExaCheckFillSpans (DrawablePtr pDrawable, GCPtr pGC, int nspans,
		   DDXPointPtr ppt, int *pwidth, int fSorted)
{
    EXA_FALLBACK(("to %p (%c)\n", pDrawable, exaDrawableLocation(pDrawable)));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    exaPrepareAccessGC (pGC);
    fbFillSpans (pDrawable, pGC, nspans, ppt, pwidth, fSorted);
    exaFinishAccessGC (pGC);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckSetSpans (DrawablePtr pDrawable, GCPtr pGC, char *psrc,
		 DDXPointPtr ppt, int *pwidth, int nspans, int fSorted)
{
    EXA_FALLBACK(("to %p (%c)\n", pDrawable, exaDrawableLocation(pDrawable)));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbSetSpans (pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckPutImage (DrawablePtr pDrawable, GCPtr pGC, int depth,
		 int x, int y, int w, int h, int leftPad, int format,
		 char *bits)
{
    EXA_FALLBACK(("to %p (%c)\n", pDrawable, exaDrawableLocation(pDrawable)));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbPutImage (pDrawable, pGC, depth, x, y, w, h, leftPad, format, bits);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

RegionPtr
ExaCheckCopyArea (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		 int srcx, int srcy, int w, int h, int dstx, int dsty)
{
    RegionPtr ret;

    EXA_FALLBACK(("from %p to %p (%c,%c)\n", pSrc, pDst,
		  exaDrawableLocation(pSrc), exaDrawableLocation(pDst)));
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

    EXA_FALLBACK(("from %p to %p (%c,%c)\n", pSrc, pDst,
		  exaDrawableLocation(pSrc), exaDrawableLocation(pDst)));
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
    EXA_FALLBACK(("to %p (%c)\n", pDrawable, exaDrawableLocation(pDrawable)));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    fbPolyPoint (pDrawable, pGC, mode, npt, pptInit);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckPolylines (DrawablePtr pDrawable, GCPtr pGC,
		  int mode, int npt, DDXPointPtr ppt)
{
    EXA_FALLBACK(("to %p (%c), width %d, mode %d, count %d\n",
		  pDrawable, exaDrawableLocation(pDrawable),
		  pGC->lineWidth, mode, npt));

    if (pGC->lineWidth == 0) {
	exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
	exaPrepareAccessGC (pGC);
	fbPolyLine (pDrawable, pGC, mode, npt, ppt);
	exaFinishAccessGC (pGC);
	exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
	return;
    }
    /* fb calls mi functions in the lineWidth != 0 case. */
    fbPolyLine (pDrawable, pGC, mode, npt, ppt);
}

void
ExaCheckPolySegment (DrawablePtr pDrawable, GCPtr pGC,
		    int nsegInit, xSegment *pSegInit)
{
    EXA_FALLBACK(("to %p (%c) width %d, count %d\n", pDrawable,
		  exaDrawableLocation(pDrawable), pGC->lineWidth, nsegInit));
    if (pGC->lineWidth == 0) {
	exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
	exaPrepareAccessGC (pGC);
	fbPolySegment (pDrawable, pGC, nsegInit, pSegInit);
	exaFinishAccessGC (pGC);
	exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
	return;
    }
    /* fb calls mi functions in the lineWidth != 0 case. */
    fbPolySegment (pDrawable, pGC, nsegInit, pSegInit);
}

void
ExaCheckPolyArc (DrawablePtr pDrawable, GCPtr pGC,
		int narcs, xArc *pArcs)
{
    EXA_FALLBACK(("to %p (%c)\n", pDrawable, exaDrawableLocation(pDrawable)));
    if (pGC->lineWidth == 0)
    {
	exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
	exaPrepareAccessGC (pGC);
	fbPolyArc (pDrawable, pGC, narcs, pArcs);
	exaFinishAccessGC (pGC);
	exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
	return;
    }
    miPolyArc (pDrawable, pGC, narcs, pArcs);
}

void
ExaCheckPolyFillRect (DrawablePtr pDrawable, GCPtr pGC,
		     int nrect, xRectangle *prect)
{
    EXA_FALLBACK(("to %p (%c)\n", pDrawable, exaDrawableLocation(pDrawable)));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    exaPrepareAccessGC (pGC);
    fbPolyFillRect (pDrawable, pGC, nrect, prect);
    exaFinishAccessGC (pGC);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckImageGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		      int x, int y, unsigned int nglyph,
		      CharInfoPtr *ppci, pointer pglyphBase)
{
    EXA_FALLBACK(("to %p (%c)\n", pDrawable,
		  exaDrawableLocation(pDrawable)));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    exaPrepareAccessGC (pGC);
    fbImageGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
    exaFinishAccessGC (pGC);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckPolyGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		     int x, int y, unsigned int nglyph,
		     CharInfoPtr *ppci, pointer pglyphBase)
{
    EXA_FALLBACK(("to %p (%c), style %d alu %d\n", pDrawable,
		  exaDrawableLocation(pDrawable), pGC->fillStyle, pGC->alu));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    exaPrepareAccessGC (pGC);
    fbPolyGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
    exaFinishAccessGC (pGC);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckPushPixels (GCPtr pGC, PixmapPtr pBitmap,
		   DrawablePtr pDrawable,
		   int w, int h, int x, int y)
{
    EXA_FALLBACK(("from %p to %p (%c,%c)\n", pBitmap, pDrawable,
		  exaDrawableLocation(&pBitmap->drawable),
		  exaDrawableLocation(pDrawable)));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    exaPrepareAccessGC (pGC);
    fbPushPixels (pGC, pBitmap, pDrawable, w, h, x, y);
    exaFinishAccessGC (pGC);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
ExaCheckGetImage (DrawablePtr pDrawable,
		 int x, int y, int w, int h,
		 unsigned int format, unsigned long planeMask,
		 char *d)
{
    EXA_FALLBACK(("from %p (%c)\n", pDrawable,
		  exaDrawableLocation(pDrawable)));
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
    EXA_FALLBACK(("from %p (%c)\n", pDrawable, exaDrawableLocation(pDrawable)));
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
    EXA_FALLBACK(("from %p (%c)\n", &pPixmap->drawable,
		  exaDrawableLocation(&pPixmap->drawable)));
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
    EXA_FALLBACK(("to %p (%c)\n", &pPixmap->drawable,
		  exaDrawableLocation(&pPixmap->drawable)));
    exaPrepareAccess ((DrawablePtr)pPixmap, EXA_PREPARE_DEST);
    fbRestoreAreas (pPixmap, prgnSave, xorg, yorg, pWin);
    exaFinishAccess ((DrawablePtr)pPixmap, EXA_PREPARE_DEST);
}

/* XXX: Note the lack of a prepare on the tile, if the window has a tiled
 * background.  This function happens to only be called if pExaScr->swappedOut,
 * so we actually end up not having to do it since the tile won't be in fb.
 * That doesn't make this not dirty, though.
 */
void
ExaCheckPaintWindow (WindowPtr pWin, RegionPtr pRegion, int what)
{
    EXA_FALLBACK(("from %p (%c)\n", pWin,
		  exaDrawableLocation(&pWin->drawable)));
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
    EXA_FALLBACK(("from picts %p/%p to pict %p\n",
		 pSrc, pMask, pDst));
    exaPrepareAccess (pDst->pDrawable, EXA_PREPARE_DEST);
    if (pSrc->pDrawable != NULL)
	exaPrepareAccess (pSrc->pDrawable, EXA_PREPARE_SRC);
    if (pMask && pMask->pDrawable != NULL)
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
    if (pMask && pMask->pDrawable != NULL)
	exaFinishAccess (pMask->pDrawable, EXA_PREPARE_MASK);
    if (pSrc->pDrawable != NULL)
	exaFinishAccess (pSrc->pDrawable, EXA_PREPARE_SRC);
    exaFinishAccess (pDst->pDrawable, EXA_PREPARE_DEST);
}

/**
 * Gets the 0,0 pixel of a pixmap.  Used for doing solid fills of tiled pixmaps
 * that happen to be 1x1.  Pixmap must be at least 8bpp.
 */
CARD32
exaGetPixmapFirstPixel (PixmapPtr pPixmap)
{
    CARD32 pixel;
    ExaMigrationRec pixmaps[1];

    pixmaps[0].as_dst = FALSE;
    pixmaps[0].as_src = TRUE;
    pixmaps[0].pPix = pPixmap;
    exaDoMigration (pixmaps, 1, FALSE);

    exaPrepareAccess(&pPixmap->drawable, EXA_PREPARE_SRC);
    switch (pPixmap->drawable.bitsPerPixel) {
    case 32:
	pixel = *(CARD32 *)(pPixmap->devPrivate.ptr);
	break;
    case 16:
	pixel = *(CARD16 *)(pPixmap->devPrivate.ptr);
	break;
    default:
	pixel = *(CARD8 *)(pPixmap->devPrivate.ptr);
	break;
    }
    exaFinishAccess(&pPixmap->drawable, EXA_PREPARE_SRC);

    return pixel;
}
