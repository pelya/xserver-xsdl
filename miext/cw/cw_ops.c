/*
 * Copyright © 2004 Eric Anholt
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Eric Anholt not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Eric Anholt makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * ERIC ANHOLT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ERIC ANHOLT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $Header$ */

#include "gcstruct.h"
#include "cw.h"

#define SETUP_BACKING_DST(_pDst, _pGC) \
    cwGCPtr pGCPrivate = getCwGC (_pGC); \
    GCFuncs *oldFuncs = (_pGC)->funcs; \
    GCPtr pBackingGC = pGCPrivate->pBackingGC; \
    int dst_off_x, dst_off_y; \
    DrawablePtr pBackingDst = cwGetBackingDrawable(pDst, &dst_off_x, \
	&dst_off_y)

#define SETUP_BACKING_SRC(pSrc, pGC) \
    int src_off_x, src_off_y; \
    DrawablePtr pBackingSrc = cwGetBackingDrawable(pSrc, &src_off_x, \
	&src_off_y)

#define PROLOGUE(pGC) do { \
    pGC->ops = pGCPrivate->wrapOps;\
    pGC->funcs = pGCPrivate->wrapFuncs; \
} while (0)

#define EPILOGUE(pGC) do { \
    pGCPrivate->wrapOps = (pGC)->ops; \
    (pGC)->ops = &cwGCOps; \
    (pGC)->funcs = oldFuncs; \
} while (0)

/*
 * GC ops -- wrap each GC operation with our own function
 */

static void cwFillSpans(DrawablePtr pDst, GCPtr pGC, int nInit,
			DDXPointPtr pptInit, int *pwidthInit, int fSorted);
static void cwSetSpans(DrawablePtr pDst, GCPtr pGC, char *psrc,
		       DDXPointPtr ppt, int *pwidth, int nspans, int fSorted);
static void cwPutImage(DrawablePtr pDst, GCPtr pGC, int depth,
		       int x, int y, int w, int h, int leftPad, int format,
		       char *pBits);
static RegionPtr cwCopyArea(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
			    int srcx, int srcy, int w, int h,
			    int dstx, int dsty);
static RegionPtr cwCopyPlane(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
			     int srcx, int srcy, int w, int h,
			     int dstx, int dsty, unsigned long plane);
static void cwPolyPoint(DrawablePtr pDst, GCPtr pGC, int mode, int npt,
			xPoint *pptInit);
static void cwPolylines(DrawablePtr pDst, GCPtr pGC, int mode, int npt,
			DDXPointPtr pptInit);
static void cwPolySegment(DrawablePtr pDst, GCPtr pGC, int nseg,
			  xSegment *pSegs);
static void cwPolyRectangle(DrawablePtr pDst, GCPtr pGC,
			    int nrects, xRectangle *pRects);
static void cwPolyArc(DrawablePtr pDst, GCPtr pGC, int narcs, xArc *parcs);
static void cwFillPolygon(DrawablePtr pDst, GCPtr pGC, int shape, int mode,
			  int count, DDXPointPtr pPts);
static void cwPolyFillRect(DrawablePtr pDst, GCPtr pGC,
			   int nrectFill, xRectangle *prectInit);
static void cwPolyFillArc(DrawablePtr pDst, GCPtr pGC,
			  int narcs, xArc *parcs);
static int cwPolyText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
		       int count, char *chars);
static int cwPolyText16(DrawablePtr pDst, GCPtr pGC, int x, int y,
			int count, unsigned short *chars);
static void cwImageText8(DrawablePtr pDst, GCPtr pGC, int x, int y,
			 int count, char *chars);
static void cwImageText16(DrawablePtr pDst, GCPtr pGC, int x, int y,
			  int count, unsigned short *chars);
static void cwImageGlyphBlt(DrawablePtr pDst, GCPtr pGC, int x, int y,
			    unsigned int nglyph, CharInfoPtr *ppci,
			    pointer pglyphBase);
static void cwPolyGlyphBlt(DrawablePtr pDst, GCPtr pGC, int x, int y,
			   unsigned int nglyph, CharInfoPtr *ppci,
			   pointer pglyphBase);
static void cwPushPixels(GCPtr pGC, PixmapPtr pBitMap, DrawablePtr pDst,
			 int w, int h, int x, int y);

GCOps cwGCOps = {
	cwFillSpans,
	cwSetSpans,
	cwPutImage,
	cwCopyArea,
	cwCopyPlane,
	cwPolyPoint,
	cwPolylines,
	cwPolySegment,
	cwPolyRectangle,
	cwPolyArc,
	cwFillPolygon,
	cwPolyFillRect,
	cwPolyFillArc,
	cwPolyText8,
	cwPolyText16,
	cwImageText8,
	cwImageText16,
	cwImageGlyphBlt,
	cwPolyGlyphBlt,
	cwPushPixels
};

static void
cwFillSpans(DrawablePtr pDst, GCPtr pGC, int nspans, DDXPointPtr ppt,
	    int *pwidth, int fSorted)
{
    DDXPointPtr	ppt_trans;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    ppt_trans = (DDXPointPtr)ALLOCATE_LOCAL(nspans * sizeof(DDXPointRec));
    if (ppt_trans)
    {
	CW_COPY_OFFSET_XYPOINTS(ppt_trans, ppt, nspans);

	(*pBackingGC->ops->FillSpans)(pBackingDst, pBackingGC,
	    nspans, ppt_trans, pwidth, fSorted);

	DEALLOCATE_LOCAL(ppt_trans);
    }

    EPILOGUE(pGC);
}

static void
cwSetSpans(DrawablePtr pDst, GCPtr pGC, char *psrc, DDXPointPtr ppt,
	   int *pwidth, int nspans, int fSorted)
{
    DDXPointPtr	ppt_trans;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    ppt_trans = (DDXPointPtr)ALLOCATE_LOCAL(nspans*sizeof(DDXPointRec));
    if (ppt_trans)
    {
	CW_COPY_OFFSET_XYPOINTS(ppt_trans, ppt, nspans);

	(*pBackingGC->ops->SetSpans)(pBackingDst, pBackingGC, psrc,
	    ppt_trans, pwidth, nspans, fSorted);

	DEALLOCATE_LOCAL(ppt_trans);
    }

    EPILOGUE(pGC);
}

static void
cwPutImage(DrawablePtr pDst, GCPtr pGC, int depth, int x, int y, int w, int h,
	   int leftPad, int format, char *pBits)
{
    int bx, by;

    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    CW_COPY_OFFSET_XY_DST(bx, by, x, y);

    (*pBackingGC->ops->PutImage)(pBackingDst, pBackingGC, depth, bx, by,
	w, h, leftPad, format, pBits);

    EPILOGUE(pGC);
}

static RegionPtr
cwCopyArea(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC, int srcx, int srcy,
	   int w, int h, int dstx, int dsty)
{
    int		bsrcx, bsrcy, bdstx, bdsty;
    RegionPtr	exposed = NULL;
    SETUP_BACKING_DST(pDst, pGC);
    SETUP_BACKING_SRC(pSrc, pGC);

    PROLOGUE(pGC);

    CW_COPY_OFFSET_XY_DST(bdstx, bdsty, dstx, dsty);
    CW_COPY_OFFSET_XY_SRC(bsrcx, bsrcy, srcx, srcy);

    exposed = (*pBackingGC->ops->CopyArea)(pBackingSrc, pBackingDst,
					   pBackingGC, bsrcx, bsrcy, w, h,
					   bdstx, bdsty);

    /* XXX: Simplify? */
    if (exposed != NULL)
	REGION_TRANSLATE(pDst->pScreen, exposed, dstx - bdstx, dsty - bdsty);

    EPILOGUE(pGC);

    return exposed;
}

static RegionPtr
cwCopyPlane(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC, int srcx, int srcy,
	    int w, int h, int dstx, int dsty, unsigned long plane)
{
    int		bsrcx, bsrcy, bdstx, bdsty;
    RegionPtr	exposed = NULL;
    SETUP_BACKING_DST(pDst, pGC);
    SETUP_BACKING_SRC(pSrc, pGC);

    PROLOGUE(pGC);

    CW_COPY_OFFSET_XY_DST(bdstx, bdsty, dstx, dsty);
    CW_COPY_OFFSET_XY_SRC(bsrcx, bsrcy, srcx, srcy);

    exposed = (*pBackingGC->ops->CopyPlane)(pBackingSrc, pBackingDst,
					    pBackingGC, bsrcx, bsrcy, w, h,
					    bdstx, bdsty, plane);

    /* XXX: Simplify? */
    REGION_TRANSLATE(pDst->pScreen, exposed, dstx - bdstx, dsty - bdsty);

    EPILOGUE(pGC);

    return exposed;
}

static void
cwPolyPoint(DrawablePtr pDst, GCPtr pGC, int mode, int npt, xPoint *ppt)
{
    xPoint	  *ppt_trans;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    ppt_trans = (xPoint *)ALLOCATE_LOCAL(npt * sizeof(xPoint));
    if (ppt_trans)
    {
	CW_COPY_OFFSET_XYPOINTS(ppt_trans, ppt, npt);

	(*pBackingGC->ops->PolyPoint)(pBackingDst, pBackingGC, mode, npt,
				      ppt_trans);

	DEALLOCATE_LOCAL(ppt_trans);
    }

    EPILOGUE(pGC);
}

static void
cwPolylines(DrawablePtr pDst, GCPtr pGC, int mode, int npt, DDXPointPtr ppt)
{
    DDXPointPtr	ppt_trans;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    ppt_trans = (DDXPointPtr)ALLOCATE_LOCAL(npt * sizeof(DDXPointRec));
    if (ppt_trans)
    {
	CW_COPY_OFFSET_XYPOINTS(ppt_trans, ppt, npt);

	(*pBackingGC->ops->Polylines)(pBackingDst, pBackingGC, mode, npt,
				      ppt_trans);

	DEALLOCATE_LOCAL(ppt_trans);
    }

    EPILOGUE(pGC);
}

static void
cwPolySegment(DrawablePtr pDst, GCPtr pGC, int nseg, xSegment *pSegs)
{
    xSegment	*psegs_trans;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    psegs_trans = (xSegment *)ALLOCATE_LOCAL(nseg * sizeof(xSegment));
    if (psegs_trans)
    {
	CW_COPY_OFFSET_XYPOINTS(psegs_trans, pSegs, nseg * 2);

	(*pBackingGC->ops->PolySegment)(pBackingDst, pBackingGC, nseg,
					psegs_trans);

	DEALLOCATE_LOCAL(psegs_trans);
    }

    EPILOGUE(pGC);
}

static void
cwPolyRectangle(DrawablePtr pDst, GCPtr pGC, int nrects, xRectangle *pRects)
{
    xRectangle	*prects_trans;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    prects_trans = (xRectangle *)ALLOCATE_LOCAL(nrects * sizeof(xRectangle));
    if (prects_trans)
    {
	CW_COPY_OFFSET_RECTS(prects_trans, pRects, nrects);

	(*pBackingGC->ops->PolyRectangle)(pBackingDst, pBackingGC, nrects,
					  prects_trans);

	DEALLOCATE_LOCAL(pRectsCopy);
    }

    EPILOGUE(pGC);
}

static void
cwPolyArc(DrawablePtr pDst, GCPtr pGC, int narcs, xArc *pArcs)
{
    xArc  *parcs_trans;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    parcs_trans = (xArc *)ALLOCATE_LOCAL(narcs * sizeof(xArc));
    if (parcs_trans)
    {
	CW_COPY_OFFSET_RECTS(parcs_trans, pArcs, narcs);

	(*pBackingGC->ops->PolyArc)(pBackingDst, pBackingGC, narcs,
				    parcs_trans);

	DEALLOCATE_LOCAL(parcs_trans);
    }

    EPILOGUE(pGC);
}

static void
cwFillPolygon(DrawablePtr pDst, GCPtr pGC, int shape, int mode, int npt,
	      DDXPointPtr ppt)
{
    DDXPointPtr	ppt_trans;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    ppt_trans = (DDXPointPtr)ALLOCATE_LOCAL(npt * sizeof(DDXPointRec));
    if (ppt_trans)
    {
	CW_COPY_OFFSET_XYPOINTS(ppt_trans, ppt, npt);

	(*pBackingGC->ops->FillPolygon)(pBackingDst, pBackingGC, shape, mode,
					npt, ppt_trans);

	DEALLOCATE_LOCAL(ppt_trans);
    }

    EPILOGUE(pGC);
}

static void
cwPolyFillRect(DrawablePtr pDst, GCPtr pGC, int nrects, xRectangle *pRects)
{
    xRectangle	*prects_trans;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    prects_trans = (xRectangle *)ALLOCATE_LOCAL(nrects * sizeof(xRectangle));
    if (prects_trans)
    {
	CW_COPY_OFFSET_RECTS(prects_trans, pRects, nrects);

	(*pBackingGC->ops->PolyFillRect)(pBackingDst, pBackingGC, nrects,
					  prects_trans);

	DEALLOCATE_LOCAL(pRectsCopy);
    }

    EPILOGUE(pGC);
}

static void
cwPolyFillArc(DrawablePtr pDst, GCPtr pGC, int narcs, xArc *parcs)
{
    xArc  *parcs_trans;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    parcs_trans = (xArc *)ALLOCATE_LOCAL(narcs * sizeof(xArc));
    if (parcs_trans)
    {
	CW_COPY_OFFSET_RECTS(parcs_trans, parcs, narcs);

	(*pBackingGC->ops->PolyFillArc)(pBackingDst, pBackingGC, narcs,
				    parcs_trans);

	DEALLOCATE_LOCAL(parcs_trans);
    }

    EPILOGUE(pGC);
}

static int
cwPolyText8(DrawablePtr pDst, GCPtr pGC, int x, int y, int count, char *chars)
{
    int result;
    int bx, by;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    CW_COPY_OFFSET_XY_DST(bx, by, x, y);

    result = (*pBackingGC->ops->PolyText8)(pBackingDst, pBackingGC, bx, by,
					   count, chars);

    EPILOGUE(pGC);
    return result;
}

static int
cwPolyText16(DrawablePtr pDst, GCPtr pGC, int x, int y, int count,
	     unsigned short *chars)
{
    int result;
    int bx, by;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    CW_COPY_OFFSET_XY_DST(bx, by, x, y);

    result = (*pBackingGC->ops->PolyText16)(pBackingDst, pBackingGC, bx, by,
					    count, chars);

    EPILOGUE(pGC);
    return result;
}

static void
cwImageText8(DrawablePtr pDst, GCPtr pGC, int x, int y, int count, char *chars)
{
    int bx, by;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    CW_COPY_OFFSET_XY_DST(bx, by, x, y);

    (*pBackingGC->ops->ImageText8)(pBackingDst, pBackingGC, bx, by, count,
				   chars);

    EPILOGUE(pGC);
}

static void
cwImageText16(DrawablePtr pDst, GCPtr pGC, int x, int y, int count,
	     unsigned short *chars)
{
    int bx, by;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    CW_COPY_OFFSET_XY_DST(bx, by, x, y);

    (*pBackingGC->ops->ImageText16)(pBackingDst, pBackingGC, bx, by, count,
				    chars);

    EPILOGUE(pGC);
}

static void
cwImageGlyphBlt(DrawablePtr pDst, GCPtr pGC, int x, int y, unsigned int nglyph,
		CharInfoPtr *ppci, pointer pglyphBase)
{
    int bx, by;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    CW_COPY_OFFSET_XY_DST(bx, by, x, y);

    (*pBackingGC->ops->ImageGlyphBlt)(pBackingDst, pBackingGC, bx, by, nglyph,
				      ppci, pglyphBase);

    EPILOGUE(pGC);
}

static void
cwPolyGlyphBlt(DrawablePtr pDst, GCPtr pGC, int x, int y, unsigned int nglyph,
	       CharInfoPtr *ppci, pointer pglyphBase)
{
    int bx, by;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    CW_COPY_OFFSET_XY_DST(bx, by, x, y);

    (*pBackingGC->ops->PolyGlyphBlt)(pBackingDst, pBackingGC, bx, by, nglyph,
				      ppci, pglyphBase);

    EPILOGUE(pGC);
}

static void
cwPushPixels(GCPtr pGC, PixmapPtr pBitMap, DrawablePtr pDst, int w, int h,
	     int x, int y)
{
    int bx, by;
    SETUP_BACKING_DST(pDst, pGC);

    PROLOGUE(pGC);

    CW_COPY_OFFSET_XY_DST(bx, by, x, y);

    (*pBackingGC->ops->PushPixels)(pBackingGC, pBitMap, pBackingDst, w, h,
				   bx, by);

    EPILOGUE(pGC);
}

