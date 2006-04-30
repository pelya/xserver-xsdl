/*
 * Copyright © 2001 Keith Packard
 *
 * Partly based on code that is Copyright © The XFree86 Project Inc.
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif
#include "exa_priv.h"
#include <X11/fonts/fontstruct.h>
#include "dixfontstr.h"
#include "exa.h"
#include "cw.h"

static void
exaFillSpans(DrawablePtr pDrawable, GCPtr pGC, int n,
	     DDXPointPtr ppt, int *pwidth, int fSorted)
{
    ScreenPtr	    pScreen = pDrawable->pScreen;
    ExaScreenPriv (pScreen);
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    PixmapPtr	    pPixmap;
    BoxPtr	    pextent, pbox;
    int		    nbox;
    int		    extentX1, extentX2, extentY1, extentY2;
    int		    fullX1, fullX2, fullY1;
    int		    partX1, partX2;
    int		    off_x, off_y;
    ExaMigrationRec pixmaps[1];

    pixmaps[0].as_dst = TRUE;
    pixmaps[0].as_src = FALSE;
    pixmaps[0].pPix = exaGetDrawablePixmap (pDrawable);

    if (pExaScr->swappedOut ||
	pGC->fillStyle != FillSolid ||
	pDrawable->width > pExaScr->info->maxX ||
	pDrawable->height > pExaScr->info->maxY)
    {
	exaDoMigration (pixmaps, 1, FALSE);
	ExaCheckFillSpans (pDrawable, pGC, n, ppt, pwidth, fSorted);
	return;
    } else {
	exaDoMigration (pixmaps, 1, TRUE);
    }

    if (!(pPixmap = exaGetOffscreenPixmap (pDrawable, &off_x, &off_y)) ||
	!(*pExaScr->info->PrepareSolid) (pPixmap,
					 pGC->alu,
					 pGC->planemask,
					 pGC->fgPixel))
    {
	ExaCheckFillSpans (pDrawable, pGC, n, ppt, pwidth, fSorted);
	return;
    }

    pextent = REGION_EXTENTS(pGC->pScreen, pClip);
    extentX1 = pextent->x1;
    extentY1 = pextent->y1;
    extentX2 = pextent->x2;
    extentY2 = pextent->y2;
    while (n--)
    {
	fullX1 = ppt->x;
	fullY1 = ppt->y;
	fullX2 = fullX1 + (int) *pwidth;
	ppt++;
	pwidth++;

	if (fullY1 < extentY1 || extentY2 <= fullY1)
	    continue;

	if (fullX1 < extentX1)
	    fullX1 = extentX1;

	if (fullX2 > extentX2)
	    fullX2 = extentX2;

	if (fullX1 >= fullX2)
	    continue;

	nbox = REGION_NUM_RECTS (pClip);
	if (nbox == 1)
	{
	    (*pExaScr->info->Solid) (pPixmap,
				     fullX1 + off_x, fullY1 + off_y,
				     fullX2 + off_x, fullY1 + 1 + off_y);
	}
	else
	{
	    pbox = REGION_RECTS(pClip);
	    while(nbox--)
	    {
		if (pbox->y1 <= fullY1 && fullY1 < pbox->y2)
		{
		    partX1 = pbox->x1;
		    if (partX1 < fullX1)
			partX1 = fullX1;
		    partX2 = pbox->x2;
		    if (partX2 > fullX2)
			partX2 = fullX2;
		    if (partX2 > partX1)
			(*pExaScr->info->Solid) (pPixmap,
						 partX1 + off_x, fullY1 + off_y,
						 partX2 + off_x, fullY1 + 1 + off_y);
		}
		pbox++;
	    }
	}
    }
    (*pExaScr->info->DoneSolid) (pPixmap);
    exaDrawableDirty (pDrawable);
    exaMarkSync(pScreen);
}

static void
exaPutImage (DrawablePtr pDrawable, GCPtr pGC, int depth, int x, int y,
	     int w, int h, int leftPad, int format, char *bits)
{
    ExaScreenPriv (pDrawable->pScreen);
    PixmapPtr pPix;
    ExaMigrationRec pixmaps[1];
    RegionPtr pClip;
    BoxPtr pbox;
    int nbox;
    int xoff, yoff;
    int src_stride, bpp = pDrawable->bitsPerPixel;

    if (pExaScr->swappedOut || pExaScr->info->UploadToScreen == NULL)
	goto migrate_and_fallback;

    /* Don't bother with under 8bpp, XYPixmaps. */
    if (format != ZPixmap || bpp < 8)
	goto migrate_and_fallback;

    /* Only accelerate copies: no rop or planemask. */
    if (!EXA_PM_IS_SOLID(pDrawable, pGC->planemask) || pGC->alu != GXcopy)
	goto migrate_and_fallback;

    pixmaps[0].as_dst = TRUE;
    pixmaps[0].as_src = FALSE;
    pixmaps[0].pPix = exaGetDrawablePixmap (pDrawable);
    exaDoMigration (pixmaps, 1, TRUE);
    pPix = exaGetOffscreenPixmap (pDrawable, &xoff, &yoff);

    if (pPix == NULL)
	goto fallback;

    x += pDrawable->x;
    y += pDrawable->y;

    pClip = fbGetCompositeClip(pGC);
    src_stride = PixmapBytePad(w, pDrawable->depth);
    for (nbox = REGION_NUM_RECTS(pClip),
	 pbox = REGION_RECTS(pClip);
	 nbox--;
	 pbox++)
    {
	int x1 = x;
	int y1 = y;
	int x2 = x + w;
	int y2 = y + h;
	char *src;
	Bool ok;

	if (x1 < pbox->x1)
	    x1 = pbox->x1;
	if (y1 < pbox->y1)
	    y1 = pbox->y1;
	if (x2 > pbox->x2)
	    x2 = pbox->x2;
	if (y2 > pbox->y2)
	    y2 = pbox->y2;
	if (x1 >= x2 || y1 >= y2)
	    continue;

	src = bits + (y1 - y) * src_stride + (x1 - x) * (bpp / 8);
	ok = pExaScr->info->UploadToScreen(pPix, x1 + xoff, y1 + yoff,
					   x2 - x1, y2 - y1, src, src_stride);
	/* If we fail to accelerate the upload, fall back to using unaccelerated
	 * fb calls.
	 */
	if (!ok) {
	    FbStip *dst;
	    FbStride dst_stride;
	    int	dstBpp;
	    int	dstXoff, dstYoff;

	    exaPrepareAccess(pDrawable, EXA_PREPARE_DEST);

	    fbGetStipDrawable(pDrawable, dst, dst_stride, dstBpp,
			      dstXoff, dstYoff);

	    fbBltStip((FbStip *)bits + (y1 - y) * (src_stride / sizeof(FbStip)),
		      src_stride / sizeof(FbStip),
		      (x1 - x) * bpp,
		      dst + (y1 + yoff) * dst_stride,
		      dst_stride,
		      (x1 + xoff) * bpp,
		      (x2 - x1) * bpp,
		      y2 - y1,
		      GXcopy, FB_ALLONES, bpp);

	    exaFinishAccess(pDrawable, EXA_PREPARE_DEST);
	}
    }
    exaDrawableDirty(pDrawable);

    return;

migrate_and_fallback:
    pixmaps[0].as_dst = TRUE;
    pixmaps[0].as_src = FALSE;
    pixmaps[0].pPix = exaGetDrawablePixmap (pDrawable);
    exaDoMigration (pixmaps, 1, FALSE);

fallback:
    ExaCheckPutImage(pDrawable, pGC, depth, x, y, w, h, leftPad, format, bits);
}

static Bool inline
exaCopyNtoNTwoDir (DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable,
		   GCPtr pGC, BoxPtr pbox, int nbox, int dx, int dy)
{
    ExaScreenPriv (pDstDrawable->pScreen);
    PixmapPtr pSrcPixmap, pDstPixmap;
    int src_off_x, src_off_y, dst_off_x, dst_off_y;
    int dirsetup;

    /* Need to get both pixmaps to call the driver routines */
    pSrcPixmap = exaGetOffscreenPixmap (pSrcDrawable, &src_off_x, &src_off_y);
    pDstPixmap = exaGetOffscreenPixmap (pDstDrawable, &dst_off_x, &dst_off_y);
    if (!pSrcPixmap || !pDstPixmap)
	return FALSE;

    /*
     * Now the case of a chip that only supports xdir = ydir = 1 or
     * xdir = ydir = -1, but we have xdir != ydir.
     */
    dirsetup = 0;	/* No direction set up yet. */
    for (; nbox; pbox++, nbox--) {
	if (dx >= 0 && (src_off_y + pbox->y1 + dy) != pbox->y1) {
	    /* Do a xdir = ydir = -1 blit instead. */
	    if (dirsetup != -1) {
		if (dirsetup != 0)
		    pExaScr->info->DoneCopy(pDstPixmap);
		dirsetup = -1;
		if (!(*pExaScr->info->PrepareCopy)(pSrcPixmap,
						   pDstPixmap,
						   -1, -1,
						   pGC ? pGC->alu : GXcopy,
						   pGC ? pGC->planemask :
							 FB_ALLONES))
		    return FALSE;
	    }
	    (*pExaScr->info->Copy)(pDstPixmap,
				   src_off_x + pbox->x1 + dx,
				   src_off_y + pbox->y1 + dy,
				   dst_off_x + pbox->x1,
				   dst_off_y + pbox->y1,
				   pbox->x2 - pbox->x1,
				   pbox->y2 - pbox->y1);
	} else if (dx < 0 && (src_off_y + pbox->y1 + dy) != pbox->y1) {
	    /* Do a xdir = ydir = 1 blit instead. */
	    if (dirsetup != 1) {
		if (dirsetup != 0)
		    pExaScr->info->DoneCopy(pDstPixmap);
		dirsetup = 1;
		if (!(*pExaScr->info->PrepareCopy)(pSrcPixmap,
						   pDstPixmap,
						   1, 1,
						   pGC ? pGC->alu : GXcopy,
						   pGC ? pGC->planemask :
							 FB_ALLONES))
		    return FALSE;
	    }
	    (*pExaScr->info->Copy)(pDstPixmap,
				   src_off_x + pbox->x1 + dx,
				   src_off_y + pbox->y1 + dy,
				   dst_off_x + pbox->x1,
				   dst_off_y + pbox->y1,
				   pbox->x2 - pbox->x1,
				   pbox->y2 - pbox->y1);
	} else if (dx >= 0) {
	    /*
	     * xdir = 1, ydir = -1.
	     * Perform line-by-line xdir = ydir = 1 blits, going up.
	     */
	    int i;
	    if (dirsetup != 1) {
		if (dirsetup != 0)
		    pExaScr->info->DoneCopy(pDstPixmap);
		dirsetup = 1;
		if (!(*pExaScr->info->PrepareCopy)(pSrcPixmap,
						   pDstPixmap,
						   1, 1,
						   pGC ? pGC->alu : GXcopy,
						   pGC ? pGC->planemask :
							 FB_ALLONES))
		    return FALSE;
	    }
	    for (i = pbox->y2 - pbox->y1 - 1; i >= 0; i--)
		(*pExaScr->info->Copy)(pDstPixmap,
				       src_off_x + pbox->x1 + dx,
				       src_off_y + pbox->y1 + dy + i,
				       dst_off_x + pbox->x1,
				       dst_off_y + pbox->y1 + i,
				       pbox->x2 - pbox->x1, 1);
	} else {
	    /*
	     * xdir = -1, ydir = 1.
	     * Perform line-by-line xdir = ydir = -1 blits, going down.
	     */
	    int i;
	    if (dirsetup != -1) {
		if (dirsetup != 0)
		    pExaScr->info->DoneCopy(pDstPixmap);
		dirsetup = -1;
		if (!(*pExaScr->info->PrepareCopy)(pSrcPixmap,
						   pDstPixmap,
						   -1, -1,
						   pGC ? pGC->alu : GXcopy,
						   pGC ? pGC->planemask :
							 FB_ALLONES))
		    return FALSE;
	    }
	    for (i = 0; i < pbox->y2 - pbox->y1; i++)
		(*pExaScr->info->Copy)(pDstPixmap,
				       src_off_x + pbox->x1 + dx,
				       src_off_y + pbox->y1 + dy + i,
				       dst_off_x + pbox->x1,
				       dst_off_y + pbox->y1 + i,
				       pbox->x2 - pbox->x1, 1);
	}
    }
    if (dirsetup != 0)
	pExaScr->info->DoneCopy(pDstPixmap);
    exaMarkSync(pDstDrawable->pScreen);
    exaDrawableDirty(pDstDrawable);
    return TRUE;
}

void
exaCopyNtoN (DrawablePtr    pSrcDrawable,
	     DrawablePtr    pDstDrawable,
	     GCPtr	    pGC,
	     BoxPtr	    pbox,
	     int	    nbox,
	     int	    dx,
	     int	    dy,
	     Bool	    reverse,
	     Bool	    upsidedown,
	     Pixel	    bitplane,
	     void	    *closure)
{
    ExaScreenPriv (pDstDrawable->pScreen);
    PixmapPtr pSrcPixmap, pDstPixmap;
    int	    src_off_x, src_off_y;
    int	    dst_off_x, dst_off_y;
    ExaMigrationRec pixmaps[2];

    pixmaps[0].as_dst = TRUE;
    pixmaps[0].as_src = FALSE;
    pixmaps[0].pPix = exaGetDrawablePixmap (pDstDrawable);
    pixmaps[1].as_dst = FALSE;
    pixmaps[1].as_src = TRUE;
    pixmaps[1].pPix = exaGetDrawablePixmap (pSrcDrawable);

    /* Respect maxX/maxY in a trivial way: don't set up drawing when we might
     * violate the limits.  The proper solution would be a temporary pixmap
     * adjusted so that the drawing happened within limits.
     */
    if (pSrcDrawable->width > pExaScr->info->maxX ||
	pSrcDrawable->height > pExaScr->info->maxY ||
	pDstDrawable->width > pExaScr->info->maxX ||
	pDstDrawable->height > pExaScr->info->maxY)
    {
	exaDoMigration (pixmaps, 2, FALSE);
	goto fallback;
    } else {
	exaDoMigration (pixmaps, 2, TRUE);
    }

    /* Mixed directions must be handled specially if the card is lame */
    if (pExaScr->info->flags & EXA_TWO_BITBLT_DIRECTIONS && (dx*dy) < 0) {
	if (!exaCopyNtoNTwoDir(pSrcDrawable, pDstDrawable, pGC, pbox, nbox,
			       dx, dy))
	    goto fallback;
	return;
    }

    if ((pSrcPixmap = exaGetOffscreenPixmap (pSrcDrawable, &src_off_x, &src_off_y)) &&
	(pDstPixmap = exaGetOffscreenPixmap (pDstDrawable, &dst_off_x, &dst_off_y)) &&
	(*pExaScr->info->PrepareCopy) (pSrcPixmap, pDstPixmap,
				       dx, dy,
				       pGC ? pGC->alu : GXcopy,
				       pGC ? pGC->planemask : FB_ALLONES))
    {
	while (nbox--)
	{
	    (*pExaScr->info->Copy) (pDstPixmap,
				    pbox->x1 + dx + src_off_x,
				    pbox->y1 + dy + src_off_y,
				    pbox->x1 + dst_off_x, pbox->y1 + dst_off_y,
				    pbox->x2 - pbox->x1,
				    pbox->y2 - pbox->y1);
	    pbox++;
	}
	(*pExaScr->info->DoneCopy) (pDstPixmap);
	exaMarkSync(pDstDrawable->pScreen);
	exaDrawableDirty (pDstDrawable);
	return;
    }

fallback:
    EXA_FALLBACK(("from %p to %p (%c,%c)\n", pSrcDrawable, pDstDrawable,
		  exaDrawableLocation(pSrcDrawable),
		  exaDrawableLocation(pDstDrawable)));
    exaPrepareAccess (pDstDrawable, EXA_PREPARE_DEST);
    exaPrepareAccess (pSrcDrawable, EXA_PREPARE_SRC);
    fbCopyNtoN (pSrcDrawable, pDstDrawable, pGC,
		pbox, nbox, dx, dy, reverse, upsidedown,
		bitplane, closure);
    exaFinishAccess (pSrcDrawable, EXA_PREPARE_SRC);
    exaFinishAccess (pDstDrawable, EXA_PREPARE_DEST);
}

RegionPtr
exaCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	    int srcx, int srcy, int width, int height, int dstx, int dsty)
{
    ExaScreenPriv (pDstDrawable->pScreen);

    if (pExaScr->swappedOut) {
        return  ExaCheckCopyArea(pSrcDrawable, pDstDrawable, pGC,
                                 srcx, srcy, width, height, dstx, dsty);
    }

    return  fbDoCopy (pSrcDrawable, pDstDrawable, pGC,
                      srcx, srcy, width, height,
                      dstx, dsty, exaCopyNtoN, 0, NULL);
}

static void
exaPolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
	     DDXPointPtr ppt)
{
    int i;
    xRectangle *prect;

    /* If we can't reuse the current GC as is, don't bother accelerating the
     * points.
     */
    if (pGC->fillStyle != FillSolid) {
	ExaCheckPolyPoint(pDrawable, pGC, mode, npt, ppt);
	return;
    }

    prect = ALLOCATE_LOCAL(sizeof(xRectangle) * npt);
    for (i = 0; i < npt; i++) {
	prect[i].x = ppt[i].x;
	prect[i].y = ppt[i].y;
	if (i > 0 && mode == CoordModePrevious) {
	    prect[i].x += prect[i - 1].x;
	    prect[i].y += prect[i - 1].y;
	}
	prect[i].width = 1;
	prect[i].height = 1;
    }
    pGC->ops->PolyFillRect(pDrawable, pGC, npt, prect);
    DEALLOCATE_LOCAL(prect);
}

/**
 * exaPolylines() checks if it can accelerate the lines as a group of
 * horizontal or vertical lines (rectangles), and uses existing rectangle fill
 * acceleration if so.
 */
static void
exaPolylines(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
	     DDXPointPtr ppt)
{
    xRectangle *prect;
    int x1, x2, y1, y2;
    int i;

    /* Don't try to do wide lines or non-solid fill style. */
    if (pGC->lineWidth != 0 || pGC->lineStyle != LineSolid ||
	pGC->fillStyle != FillSolid) {
	ExaCheckPolylines(pDrawable, pGC, mode, npt, ppt);
	return;
    }

    prect = ALLOCATE_LOCAL(sizeof(xRectangle) * (npt - 1));
    x1 = ppt[0].x;
    y1 = ppt[0].y;
    /* If we have any non-horizontal/vertical, fall back. */
    for (i = 0; i < npt; i++) {
	if (mode == CoordModePrevious) {
	    x2 = x1 + ppt[i + 1].x;
	    y2 = y1 + ppt[i + 1].y;
	} else {
	    x2 = ppt[i + 1].x;
	    y2 = ppt[i + 1].y;
	}

	if (x1 != x2 && y1 != y2) {
	    DEALLOCATE_LOCAL(prect);
	    ExaCheckPolylines(pDrawable, pGC, mode, npt, ppt);
	    return;
	}

	if (x1 < x2) {
	    prect[i].x = x1;
	    prect[i].width = x2 - x1 + 1;
	} else {
	    prect[i].x = x2;
	    prect[i].width = x1 - x2 + 1;
	}
	if (y1 < y2) {
	    prect[i].y = y1;
	    prect[i].height = y2 - y1 + 1;
	} else {
	    prect[i].y = y2;
	    prect[i].height = y1 - y2 + 1;
	}

	x1 = x2;
	y1 = y2;
    }
    pGC->ops->PolyFillRect(pDrawable, pGC, npt - 1, prect);
    DEALLOCATE_LOCAL(prect);
}

/**
 * exaPolySegment() checks if it can accelerate the lines as a group of
 * horizontal or vertical lines (rectangles), and uses existing rectangle fill
 * acceleration if so.
 */
static void
exaPolySegment (DrawablePtr pDrawable, GCPtr pGC, int nseg,
		xSegment *pSeg)
{
    xRectangle *prect;
    int i;

    /* Don't try to do wide lines or non-solid fill style. */
    if (pGC->lineWidth != 0 || pGC->lineStyle != LineSolid ||
	pGC->fillStyle != FillSolid)
    {
	ExaCheckPolySegment(pDrawable, pGC, nseg, pSeg);
	return;
    }

    /* If we have any non-horizontal/vertical, fall back. */
    for (i = 0; i < nseg; i++) {
	if (pSeg[i].x1 != pSeg[i].x2 && pSeg[i].y1 != pSeg[i].y2) {
	    ExaCheckPolySegment(pDrawable, pGC, nseg, pSeg);
	    return;
	}
    }

    prect = ALLOCATE_LOCAL(sizeof(xRectangle) * nseg);
    for (i = 0; i < nseg; i++) {
	if (pSeg[i].x1 < pSeg[i].x2) {
	    prect[i].x = pSeg[i].x1;
	    prect[i].width = pSeg[i].x2 - pSeg[i].x1 + 1;
	} else {
	    prect[i].x = pSeg[i].x2;
	    prect[i].width = pSeg[i].x1 - pSeg[i].x2 + 1;
	}
	if (pSeg[i].y1 < pSeg[i].y2) {
	    prect[i].y = pSeg[i].y1;
	    prect[i].height = pSeg[i].y2 - pSeg[i].y1 + 1;
	} else {
	    prect[i].y = pSeg[i].y2;
	    prect[i].height = pSeg[i].y1 - pSeg[i].y2 + 1;
	}
    }
    pGC->ops->PolyFillRect(pDrawable, pGC, nseg, prect);
    DEALLOCATE_LOCAL(prect);
}

static void
exaPolyFillRect(DrawablePtr pDrawable,
		GCPtr	    pGC,
		int	    nrect,
		xRectangle  *prect)
{
    ExaScreenPriv (pDrawable->pScreen);
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    PixmapPtr	    pPixmap;
    register BoxPtr pbox;
    BoxPtr	    pextent;
    int		    extentX1, extentX2, extentY1, extentY2;
    int		    fullX1, fullX2, fullY1, fullY2;
    int		    partX1, partX2, partY1, partY2;
    int		    xoff, yoff;
    int		    xorg, yorg;
    int		    n;
    ExaMigrationRec pixmaps[1];

    pixmaps[0].as_dst = TRUE;
    pixmaps[0].as_src = FALSE;
    pixmaps[0].pPix = exaGetDrawablePixmap (pDrawable);
 
    if (pExaScr->swappedOut ||
	pGC->fillStyle != FillSolid ||
	pDrawable->width > pExaScr->info->maxX ||
	pDrawable->height > pExaScr->info->maxY)
    {
	exaDoMigration (pixmaps, 1, FALSE);
	ExaCheckPolyFillRect (pDrawable, pGC, nrect, prect);
	return;
    } else {
	exaDoMigration (pixmaps, 1, TRUE);
    }

    if (!(pPixmap = exaGetOffscreenPixmap (pDrawable, &xoff, &yoff)) ||
	!(*pExaScr->info->PrepareSolid) (pPixmap,
					 pGC->alu,
					 pGC->planemask,
					 pGC->fgPixel))
    {
	ExaCheckPolyFillRect (pDrawable, pGC, nrect, prect);
	return;
    }

    xorg = pDrawable->x;
    yorg = pDrawable->y;

    pextent = REGION_EXTENTS(pGC->pScreen, pClip);
    extentX1 = pextent->x1;
    extentY1 = pextent->y1;
    extentX2 = pextent->x2;
    extentY2 = pextent->y2;
    while (nrect--)
    {
	fullX1 = prect->x + xorg;
	fullY1 = prect->y + yorg;
	fullX2 = fullX1 + (int) prect->width;
	fullY2 = fullY1 + (int) prect->height;
	prect++;

	if (fullX1 < extentX1)
	    fullX1 = extentX1;

	if (fullY1 < extentY1)
	    fullY1 = extentY1;

	if (fullX2 > extentX2)
	    fullX2 = extentX2;

	if (fullY2 > extentY2)
	    fullY2 = extentY2;

	if ((fullX1 >= fullX2) || (fullY1 >= fullY2))
	    continue;
	n = REGION_NUM_RECTS (pClip);
	if (n == 1)
	{
	    (*pExaScr->info->Solid) (pPixmap,
				     fullX1 + xoff, fullY1 + yoff,
				     fullX2 + xoff, fullY2 + yoff);
	}
	else
	{
	    pbox = REGION_RECTS(pClip);
	    /*
	     * clip the rectangle to each box in the clip region
	     * this is logically equivalent to calling Intersect()
	     */
	    while(n--)
	    {
		partX1 = pbox->x1;
		if (partX1 < fullX1)
		    partX1 = fullX1;
		partY1 = pbox->y1;
		if (partY1 < fullY1)
		    partY1 = fullY1;
		partX2 = pbox->x2;
		if (partX2 > fullX2)
		    partX2 = fullX2;
		partY2 = pbox->y2;
		if (partY2 > fullY2)
		    partY2 = fullY2;

		pbox++;

		if (partX1 < partX2 && partY1 < partY2)
		    (*pExaScr->info->Solid) (pPixmap,
					     partX1 + xoff, partY1 + yoff,
					     partX2 + xoff, partY2 + yoff);
	    }
	}
    }
    (*pExaScr->info->DoneSolid) (pPixmap);
    exaDrawableDirty (pDrawable);
    exaMarkSync(pDrawable->pScreen);
}

static void
exaSolidBoxClipped (DrawablePtr	pDrawable,
		    RegionPtr	pClip,
		    FbBits	pm,
		    FbBits	fg,
		    int		x1,
		    int		y1,
		    int		x2,
		    int		y2)
{
    ExaScreenPriv (pDrawable->pScreen);
    PixmapPtr   pPixmap;
    BoxPtr	pbox;
    int		nbox;
    int		xoff, yoff;
    int		partX1, partX2, partY1, partY2;
    ExaMigrationRec pixmaps[1];

    pixmaps[0].as_dst = TRUE;
    pixmaps[0].as_src = FALSE;
    pixmaps[0].pPix = exaGetDrawablePixmap (pDrawable);
 
    if (pExaScr->swappedOut ||
	pDrawable->width > pExaScr->info->maxX ||
	pDrawable->height > pExaScr->info->maxY)
    {
	exaDoMigration (pixmaps, 1, FALSE);
	goto fallback;
    } else {
	exaDoMigration (pixmaps, 1, TRUE);
    }

    if (!(pPixmap = exaGetOffscreenPixmap (pDrawable, &xoff, &yoff)) ||
	!(*pExaScr->info->PrepareSolid) (pPixmap, GXcopy, pm, fg))
    {
fallback:
	EXA_FALLBACK(("to %p (%c)\n", pDrawable,
		      exaDrawableLocation(pDrawable)));
	exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
	fg = fbReplicatePixel (fg, pDrawable->bitsPerPixel);
	fbSolidBoxClipped (pDrawable, pClip, x1, y1, x2, y2,
			   fbAnd (GXcopy, fg, pm),
			   fbXor (GXcopy, fg, pm));
	exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
	return;
    }
    for (nbox = REGION_NUM_RECTS(pClip), pbox = REGION_RECTS(pClip);
	 nbox--;
	 pbox++)
    {
	partX1 = pbox->x1;
	if (partX1 < x1)
	    partX1 = x1;

	partX2 = pbox->x2;
	if (partX2 > x2)
	    partX2 = x2;

	if (partX2 <= partX1)
	    continue;

	partY1 = pbox->y1;
	if (partY1 < y1)
	    partY1 = y1;

	partY2 = pbox->y2;
	if (partY2 > y2)
	    partY2 = y2;

	if (partY2 <= partY1)
	    continue;

	(*pExaScr->info->Solid) (pPixmap,
				 partX1 + xoff, partY1 + yoff,
				 partX2 + xoff, partY2 + yoff);
    }
    (*pExaScr->info->DoneSolid) (pPixmap);
    exaDrawableDirty (pDrawable);
    exaMarkSync(pDrawable->pScreen);
}

static void
exaImageGlyphBlt (DrawablePtr	pDrawable,
		  GCPtr		pGC,
		  int		x,
		  int		y,
		  unsigned int	nglyph,
		  CharInfoPtr	*ppciInit,
		  pointer	pglyphBase)
{
    FbGCPrivPtr	    pPriv = fbGetGCPrivate(pGC);
    CharInfoPtr	    *ppci;
    CharInfoPtr	    pci;
    unsigned char   *pglyph;		/* pointer bits in glyph */
    int		    gWidth, gHeight;	/* width and height of glyph */
    FbStride	    gStride;		/* stride of glyph */
    Bool	    opaque;
    int		    n;
    int		    gx, gy;
    void	    (*glyph) (FbBits *,
			      FbStride,
			      int,
			      FbStip *,
			      FbBits,
			      int,
			      int);
    FbBits	    *dst;
    FbStride	    dstStride;
    int		    dstBpp;
    int		    dstXoff, dstYoff;
    FbBits	    depthMask;

    depthMask = FbFullMask(pDrawable->depth);
    if ((pGC->planemask & depthMask) != depthMask)
    {
	ExaCheckImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppciInit, pglyphBase);
	return;
    }
    glyph = NULL;
    switch (pDrawable->bitsPerPixel) {
    case 8:	glyph = fbGlyph8; break;
    case 16:    glyph = fbGlyph16; break;
    case 24:    glyph = fbGlyph24; break;
    case 32:    glyph = fbGlyph32; break;
    }

    x += pDrawable->x;
    y += pDrawable->y;

    if (TERMINALFONT (pGC->font) && !glyph)
    {
	opaque = TRUE;
    }
    else
    {
	int		xBack, widthBack;
	int		yBack, heightBack;

	ppci = ppciInit;
	n = nglyph;
	widthBack = 0;
	while (n--)
	    widthBack += (*ppci++)->metrics.characterWidth;

        xBack = x;
	if (widthBack < 0)
	{
	    xBack += widthBack;
	    widthBack = -widthBack;
	}
	yBack = y - FONTASCENT(pGC->font);
	heightBack = FONTASCENT(pGC->font) + FONTDESCENT(pGC->font);
        exaSolidBoxClipped (pDrawable,
			    fbGetCompositeClip(pGC),
			    pGC->planemask,
			    pGC->bgPixel,
			    xBack,
			    yBack,
			    xBack + widthBack,
			    yBack + heightBack);
	opaque = FALSE;
    }

    EXA_FALLBACK(("to %p (%c)\n", pDrawable, exaDrawableLocation(pDrawable)));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    exaPrepareAccessGC (pGC);

    fbGetDrawable (pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);

    ppci = ppciInit;
    while (nglyph--)
    {
	pci = *ppci++;
	pglyph = FONTGLYPHBITS(pglyphBase, pci);
	gWidth = GLYPHWIDTHPIXELS(pci);
	gHeight = GLYPHHEIGHTPIXELS(pci);
	if (gWidth && gHeight)
	{
	    gx = x + pci->metrics.leftSideBearing;
	    gy = y - pci->metrics.ascent;
	    if (glyph && gWidth <= sizeof (FbStip) * 8 &&
		fbGlyphIn (fbGetCompositeClip(pGC), gx, gy, gWidth, gHeight))
	    {
		(*glyph) (dst + (gy + dstYoff) * dstStride,
			  dstStride,
			  dstBpp,
			  (FbStip *) pglyph,
			  pPriv->fg,
			  gx + dstXoff,
			  gHeight);
	    }
	    else
	    {
		gStride = GLYPHWIDTHBYTESPADDED(pci) / sizeof (FbStip);
		fbPutXYImage (pDrawable,
			      fbGetCompositeClip(pGC),
			      pPriv->fg,
			      pPriv->bg,
			      pPriv->pm,
			      GXcopy,
			      opaque,

			      gx,
			      gy,
			      gWidth, gHeight,

			      (FbStip *) pglyph,
			      gStride,
			      0);
	    }
	}
	x += pci->metrics.characterWidth;
    }
    exaFinishAccessGC (pGC);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

const GCOps exaOps = {
    exaFillSpans,
    ExaCheckSetSpans,
    exaPutImage,
    exaCopyArea,
    ExaCheckCopyPlane,
    exaPolyPoint,
    exaPolylines,
    exaPolySegment,
    miPolyRectangle,
    ExaCheckPolyArc,
    miFillPolygon,
    exaPolyFillRect,
    miPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    exaImageGlyphBlt,
    ExaCheckPolyGlyphBlt,
    ExaCheckPushPixels,
};

void
exaCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    RegionRec	rgnDst;
    int		dx, dy;
    PixmapPtr	pPixmap = (*pWin->drawable.pScreen->GetWindowPixmap) (pWin);

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    REGION_TRANSLATE(pWin->drawable.pScreen, prgnSrc, -dx, -dy);

    REGION_INIT (pWin->drawable.pScreen, &rgnDst, NullBox, 0);

    REGION_INTERSECT(pWin->drawable.pScreen, &rgnDst, &pWin->borderClip, prgnSrc);
#ifdef COMPOSITE
    if (pPixmap->screen_x || pPixmap->screen_y)
	REGION_TRANSLATE (pWin->drawable.pScreen, &rgnDst,
			  -pPixmap->screen_x, -pPixmap->screen_y);
#endif

    fbCopyRegion (&pPixmap->drawable, &pPixmap->drawable,
		  NULL,
		  &rgnDst, dx, dy, exaCopyNtoN, 0, NULL);

    REGION_UNINIT(pWin->drawable.pScreen, &rgnDst);
}

static void
exaFillRegionSolid (DrawablePtr	pDrawable,
		    RegionPtr	pRegion,
		    Pixel	pixel)
{
    ExaScreenPriv(pDrawable->pScreen);
    PixmapPtr pPixmap;
    int xoff, yoff;
    ExaMigrationRec pixmaps[1];

    pixmaps[0].as_dst = TRUE;
    pixmaps[0].as_src = FALSE;
    pixmaps[0].pPix = exaGetDrawablePixmap (pDrawable);
 
    if (pDrawable->width > pExaScr->info->maxX ||
	pDrawable->height > pExaScr->info->maxY)
    {
	exaDoMigration (pixmaps, 1, FALSE);
	goto fallback;
    } else {
	exaDoMigration (pixmaps, 1, TRUE);
    }

    if ((pPixmap = exaGetOffscreenPixmap (pDrawable, &xoff, &yoff)) &&
	(*pExaScr->info->PrepareSolid) (pPixmap, GXcopy, FB_ALLONES, pixel))
    {
	int	nbox = REGION_NUM_RECTS (pRegion);
	BoxPtr	pBox = REGION_RECTS (pRegion);

	while (nbox--)
	{
	    (*pExaScr->info->Solid) (pPixmap,
				     pBox->x1 + xoff, pBox->y1 + yoff,
				     pBox->x2 + xoff, pBox->y2 + yoff);
	    pBox++;
	}
	(*pExaScr->info->DoneSolid) (pPixmap);
	exaMarkSync(pDrawable->pScreen);
	exaDrawableDirty (pDrawable);
    }
    else
    {
fallback:
	EXA_FALLBACK(("to %p (%c)\n", pDrawable,
		      exaDrawableLocation(pDrawable)));
	exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
	fbFillRegionSolid (pDrawable, pRegion, 0,
			   fbReplicatePixel (pixel, pDrawable->bitsPerPixel));
	exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
    }
}

/* Try to do an accelerated tile of the pTile into pRegion of pDrawable.
 * Based on fbFillRegionTiled(), fbTile().
 */
static void
exaFillRegionTiled (DrawablePtr	pDrawable,
		    RegionPtr	pRegion,
		    PixmapPtr	pTile)
{
    ExaScreenPriv(pDrawable->pScreen);
    PixmapPtr pPixmap;
    int xoff, yoff;
    int tileWidth, tileHeight;
    ExaMigrationRec pixmaps[2];

    tileWidth = pTile->drawable.width;
    tileHeight = pTile->drawable.height;

    /* If we're filling with a solid color, grab it out and go to
     * FillRegionSolid, saving numerous copies.
     */
    if (tileWidth == 1 && tileHeight == 1) {
	exaFillRegionSolid(pDrawable, pRegion, exaGetPixmapFirstPixel (pTile));
	return;
    }

    pixmaps[0].as_dst = TRUE;
    pixmaps[0].as_src = FALSE;
    pixmaps[0].pPix = exaGetDrawablePixmap (pDrawable);
    pixmaps[1].as_dst = FALSE;
    pixmaps[1].as_src = TRUE;
    pixmaps[1].pPix = pTile;

    if (pDrawable->width > pExaScr->info->maxX ||
	pDrawable->height > pExaScr->info->maxY ||
	tileWidth > pExaScr->info->maxX ||
	tileHeight > pExaScr->info->maxY)
    {
	exaDoMigration (pixmaps, 2, FALSE);
	goto fallback;
    } else {
	exaDoMigration (pixmaps, 2, TRUE);
    }

    pPixmap = exaGetOffscreenPixmap (pDrawable, &xoff, &yoff);
    if (!pPixmap)
	goto fallback;

    if (!exaPixmapIsOffscreen(pTile))
	goto fallback;

    if ((*pExaScr->info->PrepareCopy) (pTile, pPixmap, 0, 0, GXcopy,
				       FB_ALLONES))
    {
	int nbox = REGION_NUM_RECTS (pRegion);
	BoxPtr pBox = REGION_RECTS (pRegion);

	while (nbox--)
	{
	    int height = pBox->y2 - pBox->y1;
	    int dstY = pBox->y1;
	    int tileY;

	    tileY = (dstY - pDrawable->y) % tileHeight;
	    while (height > 0) {
		int width = pBox->x2 - pBox->x1;
		int dstX = pBox->x1;
		int tileX;
		int h = tileHeight - tileY;

		if (h > height)
		    h = height;
		height -= h;

		tileX = (dstX - pDrawable->x) % tileWidth;
		while (width > 0) {
		    int w = tileWidth - tileX;
		    if (w > width)
			w = width;
		    width -= w;

		    (*pExaScr->info->Copy) (pPixmap,
					    tileX, tileY,
					    dstX + xoff, dstY + yoff,
					    w, h);
		    dstX += w;
		    tileX = 0;
		}
		dstY += h;
		tileY = 0;
	    }
	    pBox++;
	}
	(*pExaScr->info->DoneCopy) (pPixmap);
	exaMarkSync(pDrawable->pScreen);
	exaDrawableDirty (pDrawable);
	return;
    }

fallback:
    EXA_FALLBACK(("from %p to %p (%c,%c)\n", pTile, pDrawable,
		  exaDrawableLocation(&pTile->drawable),
		  exaDrawableLocation(pDrawable)));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    exaPrepareAccess ((DrawablePtr)pTile, EXA_PREPARE_SRC);
    fbFillRegionTiled (pDrawable, pRegion, pTile);
    exaFinishAccess ((DrawablePtr)pTile, EXA_PREPARE_SRC);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

void
exaPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what)
{
    ExaScreenPriv (pWin->drawable.pScreen);
    if (!REGION_NUM_RECTS(pRegion))
	return;
    if (!pExaScr->swappedOut) {
        switch (what) {
        case PW_BACKGROUND:
            switch (pWin->backgroundState) {
            case None:
                return;
            case ParentRelative:
                do {
                    pWin = pWin->parent;
                } while (pWin->backgroundState == ParentRelative);
                (*pWin->drawable.pScreen->PaintWindowBackground)(pWin, pRegion,
                                                                 what);
                return;
            case BackgroundPixel:
                exaFillRegionSolid((DrawablePtr)pWin, pRegion, pWin->background.pixel);
                return;
            case BackgroundPixmap:
                exaFillRegionTiled((DrawablePtr)pWin, pRegion, pWin->background.pixmap);
                return;
            }
            break;
        case PW_BORDER:
            if (pWin->borderIsPixel) {
                exaFillRegionSolid((DrawablePtr)pWin, pRegion, pWin->border.pixel);
                return;
            } else {
                exaFillRegionTiled((DrawablePtr)pWin, pRegion, pWin->border.pixmap);
                return;
            }
            break;
        }
    }
    ExaCheckPaintWindow (pWin, pRegion, what);
}

/**
 * Accelerates GetImage for solid ZPixmap downloads from framebuffer memory.
 *
 * This is probably the only case we actually care about.  The rest fall through
 * to migration and ExaCheckGetImage, which hopefully will result in migration
 * pushing the pixmap out of framebuffer.
 */
void
exaGetImage (DrawablePtr pDrawable, int x, int y, int w, int h,
	     unsigned int format, unsigned long planeMask, char *d)
{
    ExaScreenPriv (pDrawable->pScreen);
    ExaMigrationRec pixmaps[1];
    PixmapPtr pPix;
    int xoff, yoff;
    Bool ok;

    if (pExaScr->swappedOut || pExaScr->info->DownloadFromScreen == NULL)
	goto fallback;

    /* Only cover the ZPixmap, solid copy case. */
    if (format != ZPixmap || !EXA_PM_IS_SOLID(pDrawable, planeMask))
	goto fallback;

    /* Only try to handle the 8bpp and up cases, since we don't want to think
     * about <8bpp.
     */
    if (pDrawable->bitsPerPixel < 8)
	goto fallback;

    /* Migrate, but assume that we could accelerate the download. It is up to
     * the migration scheme to ensure that this case doesn't result in bad
     * moving of pixmaps.
     */
    pixmaps[0].as_dst = FALSE;
    pixmaps[0].as_src = TRUE;
    pixmaps[0].pPix = exaGetDrawablePixmap (pDrawable);
    exaDoMigration (pixmaps, 1, TRUE);
    pPix = exaGetOffscreenPixmap (pDrawable, &xoff, &yoff);
    if (pPix == NULL)
	goto fallback;

    xoff += pDrawable->x;
    yoff += pDrawable->y;

    ok = pExaScr->info->DownloadFromScreen(pPix, x + xoff, y + yoff, w, h, d,
					   PixmapBytePad(w, pDrawable->depth));
    if (ok) {
	exaWaitSync(pDrawable->pScreen);
	return;
    }

fallback:
    pixmaps[0].as_dst = FALSE;
    pixmaps[0].as_src = TRUE;
    pixmaps[0].pPix = exaGetDrawablePixmap (pDrawable);
    exaDoMigration (pixmaps, 1, FALSE);

    ExaCheckGetImage (pDrawable, x, y, w, h, format, planeMask, d);
}

/**
 * GetSpans isn't accelerated yet, but performs migration so that we'll
 * hopefully avoid the read-from-framebuffer cost.
 */
void
exaGetSpans (DrawablePtr pDrawable, int wMax, DDXPointPtr ppt, int *pwidth,
	     int nspans, char *pdstStart)
{
    ExaMigrationRec pixmaps[1];

    pixmaps[0].as_dst = FALSE;
    pixmaps[0].as_src = TRUE;
    pixmaps[0].pPix = exaGetDrawablePixmap (pDrawable);
    exaDoMigration (pixmaps, 1, FALSE);

    ExaCheckGetSpans (pDrawable, wMax, ppt, pwidth, nspans, pdstStart);
}
