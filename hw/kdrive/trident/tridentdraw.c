/*
 * $Id$
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
/* $XFree86: xc/programs/Xserver/hw/kdrive/trident/tridentdraw.c,v 1.1 1999/11/19 13:54:01 hohndel Exp $ */

#include "trident.h"
#include "tridentdraw.h"

#include	"Xmd.h"
#include	"gcstruct.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"mistruct.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"fb.h"
#include	"migc.h"
#include	"miline.h"

CARD8 tridentRop[16] = {
    /* GXclear      */      0x00,         /* 0 */
    /* GXand        */      0x88,         /* src AND dst */
    /* GXandReverse */      0x44,         /* src AND NOT dst */
    /* GXcopy       */      0xcc,         /* src */
    /* GXandInverted*/      0x22,         /* NOT src AND dst */
    /* GXnoop       */      0xaa,         /* dst */
    /* GXxor        */      0x66,         /* src XOR dst */
    /* GXor         */      0xee,         /* src OR dst */
    /* GXnor        */      0x11,         /* NOT src AND NOT dst */
    /* GXequiv      */      0x99,         /* NOT src XOR dst */
    /* GXinvert     */      0x55,         /* NOT dst */
    /* GXorReverse  */      0xdd,         /* src OR NOT dst */
    /* GXcopyInverted*/     0x33,         /* NOT src */
    /* GXorInverted */      0xbb,         /* NOT src OR dst */
    /* GXnand       */      0x77,         /* NOT src OR NOT dst */
    /* GXset        */      0xff,         /* 1 */
};

#define tridentFillPix(bpp,pixel) {\
    if (bpp == 8) \
    { \
	pixel = pixel & 0xff; \
	pixel = pixel | pixel << 8; \
    } \
    if (bpp <= 16) \
    { \
	pixel = pixel & 0xffff; \
	pixel = pixel | pixel << 16; \
    } \
}
void
tridentFillBoxSolid (DrawablePtr pDrawable, int nBox, BoxPtr pBox, 
		 unsigned long pixel, int alu)
{
    SetupTrident(pDrawable->pScreen);
    CARD32	cmd;

    tridentFillPix(pDrawable->bitsPerPixel,pixel);
    _tridentInit(cop,tridentc);
    _tridentSetSolidRect(cop,pixel,alu,cmd);
    while (nBox--) 
    {
	_tridentRect(cop,pBox->x1,pBox->y1,pBox->x2-1,pBox->y2-1,cmd);
	pBox++;
    }
    KdMarkSync(pDrawable->pScreen);
}

void
tridentCopyNtoN (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure)
{
    SetupTrident(pDstDrawable->pScreen);
    int	    srcX, srcY, dstX, dstY;
    int	    w, h;
    CARD32  flags;
    CARD32  cmd;
    CARD8   alu;
    
    if (pGC)
    {
	alu = pGC->alu;
	if (sourceInvarient (pGC->alu))
	{
	    tridentFillBoxSolid (pDstDrawable, nbox, pbox, 0, pGC->alu);
	    return;
	}
    }
    else
	alu = GXcopy;
    
    _tridentInit(cop,tridentc);
    cop->multi = COP_MULTI_PATTERN;
    cop->multi = COP_MULTI_ROP | tridentRop[alu];
    if (reverse)
	upsidedown = TRUE;
    cmd = COP_OP_BLT | COP_SCL_OPAQUE | COP_OP_ROP | COP_OP_FB;
    if (upsidedown)
	cmd |= COP_X_REVERSE;
    while (nbox--)
    {
	if (upsidedown)
	{
	    cop->src_start_xy = TRI_XY (pbox->x2 + dx - 1,
					pbox->y2 + dy - 1);
	    cop->src_end_xy   = TRI_XY (pbox->x1 + dx,
					pbox->y1 + dy);
	    cop->dst_start_xy = TRI_XY (pbox->x2 - 1,
					pbox->y2 - 1);
	    cop->dst_end_xy   = TRI_XY (pbox->x1,
					pbox->y1);
	}
	else
	{
	    cop->src_start_xy = TRI_XY (pbox->x1 + dx,
					pbox->y1 + dy);
	    cop->src_end_xy   = TRI_XY (pbox->x2 + dx - 1,
					pbox->y2 + dy - 1);
	    cop->dst_start_xy = TRI_XY (pbox->x1,
					pbox->y1);
	    cop->dst_end_xy   = TRI_XY (pbox->x2 - 1,
					pbox->y2 - 1);
	}
	cop->command = cmd;
	pbox++;
    }
    KdMarkSync(pDstDrawable->pScreen);
}

RegionPtr
tridentCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	   int srcx, int srcy, int width, int height, int dstx, int dsty)
{
    KdScreenPriv(pDstDrawable->pScreen);
    FbBits	    depthMask;
    
    depthMask = FbFullMask (pDstDrawable->depth);
    if ((pGC->planemask & depthMask) == depthMask &&
	pSrcDrawable->type == DRAWABLE_WINDOW &&
	pDstDrawable->type == DRAWABLE_WINDOW)
    {
	return fbDoCopy (pSrcDrawable, pDstDrawable, pGC, 
			 srcx, srcy, width, height, 
			 dstx, dsty, tridentCopyNtoN, 0, 0);
    }
    return KdCheckCopyArea (pSrcDrawable, pDstDrawable, pGC, 
			    srcx, srcy, width, height, dstx, dsty);
}

BOOL
tridentFillOk (GCPtr pGC)
{
    FbBits  depthMask;

    depthMask = FbFullMask(pGC->depth);
    if ((pGC->planemask & depthMask) != depthMask)
	return FALSE;
    switch (pGC->fillStyle) {
    case FillSolid:
	return TRUE;
#if 0
    case FillTiled:
	return (tridentPatternDimOk (pGC->tile.pixmap->drawable.width) &&
		tridentPatternDimOk (pGC->tile.pixmap->drawable.height));
    case FillStippled:
    case FillOpaqueStippled:
	return (tridentPatternDimOk (pGC->stipple->drawable.width) &&
		tridentPatternDimOk (pGC->stipple->drawable.height));
#endif
    }
    return FALSE;
}

void
tridentFillSpans (DrawablePtr pDrawable, GCPtr pGC, int n, 
	     DDXPointPtr ppt, int *pwidth, int fSorted)
{
    SetupTrident(pDrawable->pScreen);
    DDXPointPtr	    pptFree;
    FbGCPrivPtr	    fbPriv = fbGetGCPrivate(pGC);
    int		    *pwidthFree;/* copies of the pointers to free */
    CARD32	    cmd;
    int		    nTmp;
    INT16	    x, y;
    int		    width;
    CARD32	    pixel;
    
    if (!tridentFillOk (pGC))
    {
	KdCheckFillSpans (pDrawable, pGC, n, ppt, pwidth, fSorted);
	return;
    }
    nTmp = n * miFindMaxBand(fbGetCompositeClip(pGC));
    pwidthFree = (int *)ALLOCATE_LOCAL(nTmp * sizeof(int));
    pptFree = (DDXPointRec *)ALLOCATE_LOCAL(nTmp * sizeof(DDXPointRec));
    if(!pptFree || !pwidthFree)
    {
	if (pptFree) DEALLOCATE_LOCAL(pptFree);
	if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	return;
    }
    n = miClipSpans(fbGetCompositeClip(pGC),
		     ppt, pwidth, n,
		     pptFree, pwidthFree, fSorted);
    pwidth = pwidthFree;
    ppt = pptFree;
    _tridentInit(cop,tridentc);
    switch (pGC->fillStyle) {
    case FillSolid:
	pixel = pGC->fgPixel;
	tridentFillPix (pDrawable->bitsPerPixel,pixel);
	_tridentSetSolidRect(cop,pixel,pGC->alu,cmd);
	break;
#if 0
    case FillTiled:
	cmd = tridentTilePrepare (pGC->tile.pixmap,
			      pGC->patOrg.x + pDrawable->x,
			      pGC->patOrg.y + pDrawable->y,
			      pGC->alu);
	break;
    default:
	cmd = tridentStipplePrepare (pDrawable, pGC);
	break;
#endif
    }
    while (n--)
    {
	x = ppt->x;
	y = ppt->y;
	ppt++;
	width = *pwidth++;
	if (width)
	{
	    _tridentRect(cop,x,y,x + width - 1,y,cmd);
	}
    }
    KdMarkSync(pDrawable->pScreen);
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}

#define NUM_STACK_RECTS	1024

void
tridentPolyFillRect (DrawablePtr pDrawable, GCPtr pGC, 
		int nrectFill, xRectangle *prectInit)
{
    SetupTrident(pDrawable->pScreen);
    xRectangle	    *prect;
    RegionPtr	    prgnClip;
    register BoxPtr pbox;
    register BoxPtr pboxClipped;
    BoxPtr	    pboxClippedBase;
    BoxPtr	    pextent;
    BoxRec	    stackRects[NUM_STACK_RECTS];
    FbGCPrivPtr	    fbPriv = fbGetGCPrivate (pGC);
    int		    numRects;
    int		    n;
    int		    xorg, yorg;
    int		    x, y;
    
    if (!tridentFillOk (pGC))
    {
	KdCheckPolyFillRect (pDrawable, pGC, nrectFill, prectInit);
	return;
    }
    prgnClip = fbGetCompositeClip(pGC);
    xorg = pDrawable->x;
    yorg = pDrawable->y;
    
    if (xorg || yorg)
    {
	prect = prectInit;
	n = nrectFill;
	while(n--)
	{
	    prect->x += xorg;
	    prect->y += yorg;
	    prect++;
	}
    }
    
    prect = prectInit;

    numRects = REGION_NUM_RECTS(prgnClip) * nrectFill;
    if (numRects > NUM_STACK_RECTS)
    {
	pboxClippedBase = (BoxPtr)xalloc(numRects * sizeof(BoxRec));
	if (!pboxClippedBase)
	    return;
    }
    else
	pboxClippedBase = stackRects;

    pboxClipped = pboxClippedBase;
	
    if (REGION_NUM_RECTS(prgnClip) == 1)
    {
	int x1, y1, x2, y2, bx2, by2;

	pextent = REGION_RECTS(prgnClip);
	x1 = pextent->x1;
	y1 = pextent->y1;
	x2 = pextent->x2;
	y2 = pextent->y2;
    	while (nrectFill--)
    	{
	    if ((pboxClipped->x1 = prect->x) < x1)
		pboxClipped->x1 = x1;
    
	    if ((pboxClipped->y1 = prect->y) < y1)
		pboxClipped->y1 = y1;
    
	    bx2 = (int) prect->x + (int) prect->width;
	    if (bx2 > x2)
		bx2 = x2;
	    pboxClipped->x2 = bx2;
    
	    by2 = (int) prect->y + (int) prect->height;
	    if (by2 > y2)
		by2 = y2;
	    pboxClipped->y2 = by2;

	    prect++;
	    if ((pboxClipped->x1 < pboxClipped->x2) &&
		(pboxClipped->y1 < pboxClipped->y2))
	    {
		pboxClipped++;
	    }
    	}
    }
    else
    {
	int x1, y1, x2, y2, bx2, by2;

	pextent = REGION_EXTENTS(pGC->pScreen, prgnClip);
	x1 = pextent->x1;
	y1 = pextent->y1;
	x2 = pextent->x2;
	y2 = pextent->y2;
    	while (nrectFill--)
    	{
	    BoxRec box;
    
	    if ((box.x1 = prect->x) < x1)
		box.x1 = x1;
    
	    if ((box.y1 = prect->y) < y1)
		box.y1 = y1;
    
	    bx2 = (int) prect->x + (int) prect->width;
	    if (bx2 > x2)
		bx2 = x2;
	    box.x2 = bx2;
    
	    by2 = (int) prect->y + (int) prect->height;
	    if (by2 > y2)
		by2 = y2;
	    box.y2 = by2;
    
	    prect++;
    
	    if ((box.x1 >= box.x2) || (box.y1 >= box.y2))
	    	continue;
    
	    n = REGION_NUM_RECTS (prgnClip);
	    pbox = REGION_RECTS(prgnClip);
    
	    /* clip the rectangle to each box in the clip region
	       this is logically equivalent to calling Intersect()
	    */
	    while(n--)
	    {
		pboxClipped->x1 = max(box.x1, pbox->x1);
		pboxClipped->y1 = max(box.y1, pbox->y1);
		pboxClipped->x2 = min(box.x2, pbox->x2);
		pboxClipped->y2 = min(box.y2, pbox->y2);
		pbox++;

		/* see if clipping left anything */
		if(pboxClipped->x1 < pboxClipped->x2 && 
		   pboxClipped->y1 < pboxClipped->y2)
		{
		    pboxClipped++;
		}
	    }
    	}
    }
    if (pboxClipped != pboxClippedBase)
    {
	switch (pGC->fillStyle) {
	case FillSolid:
	    tridentFillBoxSolid(pDrawable,
			   pboxClipped-pboxClippedBase, pboxClippedBase,
			   pGC->fgPixel, pGC->alu);
	    break;
#if 0
	case FillTiled:
	    tridentFillBoxTiled(pDrawable,
			    pboxClipped-pboxClippedBase, pboxClippedBase,
			    pGC->tile.pixmap,
			    pGC->patOrg.x + pDrawable->x,
			    pGC->patOrg.y + pDrawable->y,
			    pGC->alu);
	    break;
	case FillStippled:
	case FillOpaqueStippled:
	    tridentFillBoxStipple (pDrawable, pGC,
			       pboxClipped-pboxClippedBase, pboxClippedBase);
	    break;
#endif
	}
    }
    if (pboxClippedBase != stackRects)
    	xfree(pboxClippedBase);
}

void
tridentSolidBoxClipped (DrawablePtr	pDrawable,
			RegionPtr	pClip,
			int		x1,
			int		y1,
			int		x2,
			int		y2,
			FbBits		fg)
{
    SetupTrident (pDrawable->pScreen);
    BoxPtr	pbox;
    int		nbox;
    int		partX1, partX2, partY1, partY2;
    CARD32	cmd;

    _tridentInit (cop, tridentc);
    _tridentSetSolidRect (cop, fg, GXcopy, cmd);
    
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
	
	_tridentRect(cop,partX1, partY1, partX2-1, partY2-1,cmd);
    }
    KdMarkSync(pDrawable->pScreen);
}

void
tridentImageGlyphBlt (DrawablePtr	pDrawable,
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
    FbBits	    depthMask;
    
    depthMask = FbFullMask(pDrawable->depth);
    if ((pGC->planemask & depthMask) != depthMask)
    {
	KdCheckImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppciInit, pglyphBase);
	return;
    }
    glyph = 0;
    fbGetDrawable (pDrawable, dst, dstStride, dstBpp);
    switch (dstBpp) {
    case 8:	    glyph = fbGlyph8; break;
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
	tridentSolidBoxClipped (pDrawable,
				fbGetCompositeClip(pGC),
				xBack,
				yBack,
				xBack + widthBack,
				yBack + heightBack,
				pPriv->bg);
	opaque = FALSE;
    }

    KdCheckSync (pDrawable->pScreen);
    
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
		(*glyph) (dst + gy * dstStride,
			  dstStride,
			  dstBpp,
			  (FbStip *) pglyph,
			  pPriv->fg,
			  gx,
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
}

static const GCOps	tridentOps = {
    tridentFillSpans,
    KdCheckSetSpans,
    KdCheckPutImage,
    tridentCopyArea,
    KdCheckCopyPlane,
    KdCheckPolyPoint,
    KdCheckPolylines,
    KdCheckPolySegment,
    miPolyRectangle,
    KdCheckPolyArc,
    miFillPolygon,
    tridentPolyFillRect,
    KdCheckPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    tridentImageGlyphBlt,
    KdCheckPolyGlyphBlt,
    KdCheckPushPixels,
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

void
tridentValidateGC (GCPtr pGC, Mask changes, DrawablePtr pDrawable)
{
    FbGCPrivPtr fbPriv = fbGetGCPrivate(pGC);
    
    fbValidateGC (pGC, changes, pDrawable);
    
    if (pDrawable->type == DRAWABLE_WINDOW)
	pGC->ops = (GCOps *) &tridentOps;
    else
	pGC->ops = (GCOps *) &kdAsyncPixmapGCOps;
}

GCFuncs	tridentGCFuncs = {
    tridentValidateGC,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip
};

int
tridentCreateGC (GCPtr pGC)
{
    if (!fbCreateGC (pGC))
	return FALSE;

    if (pGC->depth != 1)
	pGC->funcs = &tridentGCFuncs;
    
    return TRUE;
}

void
tridentCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr	pScreen = pWin->drawable.pScreen;
    KdScreenPriv(pScreen);
    RegionRec	rgnDst;
    int		dx, dy;
    WindowPtr	pwinRoot;

    pwinRoot = WindowTable[pWin->drawable.pScreen->myNum];

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    REGION_TRANSLATE(pWin->drawable.pScreen, prgnSrc, -dx, -dy);

    REGION_INIT (pWin->drawable.pScreen, &rgnDst, NullBox, 0);
    
    REGION_INTERSECT(pWin->drawable.pScreen, &rgnDst, &pWin->borderClip, prgnSrc);

    fbCopyRegion ((DrawablePtr)pwinRoot, (DrawablePtr)pwinRoot,
		  0,
		  &rgnDst, dx, dy, tridentCopyNtoN, 0, 0);
    
    REGION_UNINIT(pWin->drawable.pScreen, &rgnDst);
}

void
tridentPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what)
{
    KdScreenPriv(pWin->drawable.pScreen);
    PixmapPtr	pTile;

    if (!REGION_NUM_RECTS(pRegion)) 
	return;
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
#if 0
	case BackgroundPixmap:
	    pTile = pWin->background.pixmap;
	    if (tridentPatternDimOk (pTile->drawable.width) &&
		tridentPatternDimOk (pTile->drawable.height))
	    {
		tridentFillBoxTiled ((DrawablePtr)pWin,
				 (int)REGION_NUM_RECTS(pRegion),
				 REGION_RECTS(pRegion),
				 pTile, 
				 pWin->drawable.x, pWin->drawable.y, GXcopy);
		return;
	    }
	    break;
#endif
	case BackgroundPixel:
	    tridentFillBoxSolid((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->background.pixel, GXcopy);
	    return;
    	}
    	break;
    case PW_BORDER:
	if (pWin->borderIsPixel)
	{
	    tridentFillBoxSolid((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->border.pixel, GXcopy);
	    return;
	}
#if 0
	else
	{
	    pTile = pWin->border.pixmap;
	    if (tridentPatternDimOk (pTile->drawable.width) &&
		tridentPatternDimOk (pTile->drawable.height))
	    {
		tridentFillBoxTiled ((DrawablePtr)pWin,
				 (int)REGION_NUM_RECTS(pRegion),
				 REGION_RECTS(pRegion),
				 pTile, 
				 pWin->drawable.x, pWin->drawable.y, GXcopy);
		return;
	    }
	}
#endif
	break;
    }
    KdCheckPaintWindow (pWin, pRegion, what);
}

Bool
tridentDrawInit (ScreenPtr pScreen)
{
    /*
     * Hook up asynchronous drawing
     */
    KdScreenInitAsync (pScreen);
    /*
     * Replace various fb screen functions
     */
    pScreen->CreateGC = tridentCreateGC;
    pScreen->CopyWindow = tridentCopyWindow;
    pScreen->PaintWindowBackground = tridentPaintWindow;
    pScreen->PaintWindowBorder = tridentPaintWindow;
    
    return TRUE;
}

void
tridentDrawEnable (ScreenPtr pScreen)
{
    SetupTrident(pScreen);
    CARD32  cmd;
    CARD32  base;
    CARD16  stride;
    CARD32  format;
    CARD32  alpha;
    int	    tries;
    
    stride = pScreenPriv->screen->pixelStride;
    switch (pScreenPriv->screen->bitsPerPixel) {
    case 8:
	format = COP_DEPTH_8;
	break;
    case 16:
	format = COP_DEPTH_16;
	break;
    case 24:
	format = COP_DEPTH_24_32;
	break;
    case 32:
	format = COP_DEPTH_24_32;
	break;
    }
    /* 
     * compute a few things which will be set every time the
     * accelerator is used; this avoids problems with APM
     */
    tridentc->cop_depth = COP_MULTI_DEPTH | format;
    tridentc->cop_stride = COP_MULTI_STRIDE | (stride << 16) | (stride);
    
#define NUM_TRIES   100000
    for (tries = 0; tries < NUM_TRIES; tries++)
	if (!(cop->status & COP_STATUS_BUSY))
	    break;
    if (cop->status & COP_STATUS_BUSY)
	FatalError ("Can't initialize graphics coprocessor");
    cop->multi = COP_MULTI_CLIP_TOP_LEFT;
    cop->multi = COP_MULTI_MASK | 0;
    cop->src_offset = 0;
    cop->dst_offset = 0;
    cop->z_offset = 0;
    cop->clip_bottom_right = 0x0fff0fff;
    
    _tridentInit(cop,tridentc);
    _tridentSetSolidRect(cop, pScreen->blackPixel, GXcopy, cmd);
    _tridentRect (cop, 0, 0, 
		  pScreenPriv->screen->width, pScreenPriv->screen->height,
		  cmd);
    KdMarkSync (pScreen);
}

void
tridentDrawDisable (ScreenPtr pScreen)
{
}

void
tridentDrawFini (ScreenPtr pScreen)
{
}

void
tridentDrawSync (ScreenPtr pScreen)
{
    SetupTrident(pScreen);
    
    _tridentWaitIdleEmpty(cop);
}
