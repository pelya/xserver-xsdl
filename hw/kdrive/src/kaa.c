/*
 * $XFree86: xc/programs/Xserver/hw/kdrive/kaa.c,v 1.1 2001/05/29 04:54:10 keithp Exp $
 *
 * Copyright © 2001 Keith Packard, member of The XFree86 Project, Inc.
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

#include "kdrive.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"

int	kaaGeneration;
int	kaaScreenPrivateIndex;

#define KaaScreenPriv(s)	KaaScreenPrivPtr    pKaaScr = (KaaScreenPrivPtr) (s)->devPrivates[kaaScreenPrivateIndex].ptr

void
kaaFillSpans(DrawablePtr pDrawable, GCPtr pGC, int n, 
	     DDXPointPtr ppt, int *pwidth, int fSorted)
{
    KaaScreenPriv (pDrawable->pScreen);
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    BoxPtr	    pextent, pbox;
    int		    nbox;
    int		    extentX1, extentX2, extentY1, extentY2;
    int		    fullX1, fullX2, fullY1;
    int		    partX1, partX2;
    
    if (pGC->fillStyle != FillSolid ||
	!(*pKaaScr->PrepareSolid) (pDrawable,
				   pGC->alu,
				   pGC->planemask,
				   pGC->fgPixel))
    {
	KdCheckFillSpans (pDrawable, pGC, n, ppt, pwidth, fSorted);
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
	    (*pKaaScr->Solid) (fullX1, fullY1, fullX2, fullY1 + 1);
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
			(*pKaaScr->Solid) (partX1, fullY1, partX2, fullY1 + 1);
		}
		pbox++;
	    }
	}
    }
    (*pKaaScr->DoneSolid) ();
    KdMarkSync(pDrawable->pScreen);
}

void
kaaCopyNtoN (DrawablePtr    pSrcDrawable,
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
    KaaScreenPriv (pDstDrawable->pScreen);
    int	    srcX, srcY, dstX, dstY;
    int	    w, h;
    CARD32  flags;
    CARD32  cmd;
    CARD8   alu;
    
    if (pSrcDrawable->type == DRAWABLE_WINDOW &&
	(*pKaaScr->PrepareCopy) (pSrcDrawable,
				 pDstDrawable,
				 upsidedown,
				 reverse,
				 pGC ? pGC->alu : GXcopy,
				 pGC ? pGC->planemask : FB_ALLONES))
    {
	while (nbox--)
	{
	    (*pKaaScr->Copy) (pbox->x1 + dx, pbox->y1 + dy,
			      pbox->x1, pbox->y1,
			      pbox->x2 - pbox->x1,
			      pbox->y2 - pbox->y1);
	    pbox++;
	}
	(*pKaaScr->DoneCopy) ();
	KdMarkSync(pDstDrawable->pScreen);
    }
    else
    {
	KdCheckSync (pDstDrawable->pScreen);
	fbCopyNtoN (pSrcDrawable, pDstDrawable, pGC, 
		    pbox, nbox, dx, dy, reverse, upsidedown, 
		    bitplane, closure);
    }
}

RegionPtr
kaaCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	    int srcx, int srcy, int width, int height, int dstx, int dsty)
{
    return fbDoCopy (pSrcDrawable, pDstDrawable, pGC, 
		     srcx, srcy, width, height, 
		     dstx, dsty, kaaCopyNtoN, 0, 0);
}

void
kaaPolyFillRect(DrawablePtr pDrawable, 
		GCPtr	    pGC, 
		int	    nrect,
		xRectangle  *prect)
{
    KaaScreenPriv (pDrawable->pScreen);
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    register BoxPtr pbox;
    BoxPtr	    pextent;
    int		    extentX1, extentX2, extentY1, extentY2;
    int		    fullX1, fullX2, fullY1, fullY2;
    int		    partX1, partX2, partY1, partY2;
    int		    xorg, yorg;
    int		    n;

    if (pGC->fillStyle != FillSolid ||
	!(*pKaaScr->PrepareSolid) (pDrawable,
				   pGC->alu,
				   pGC->planemask,
				   pGC->fgPixel))
    {
	KdCheckPolyFillRect (pDrawable, pGC, nrect, prect);
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
	    (*pKaaScr->Solid) (fullX1, fullY1, fullX2, fullY2);
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
		    (*pKaaScr->Solid) (partX1, partY1,
				       partX2, partY2);
	    }
	}
    }
    (*pKaaScr->DoneSolid) ();
    KdMarkSync(pDrawable->pScreen);
}
    
void
kaaSolidBoxClipped (DrawablePtr	pDrawable,
		    RegionPtr	pClip,
		    FbBits	pm,
		    FbBits	fg,
		    int		x1,
		    int		y1,
		    int		x2,
		    int		y2)
{
    KaaScreenPriv (pDrawable->pScreen);
    BoxPtr	pbox;
    int		nbox;
    int		partX1, partX2, partY1, partY2;
    CARD32	cmd;

    if ((*pKaaScr->PrepareSolid) (pDrawable, GXcopy, pm, fg))
    {
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
	    
	    (*pKaaScr->Solid) (partX1, partY1, partX2, partY2);
	}
	(*pKaaScr->DoneSolid) ();
	KdMarkSync(pDrawable->pScreen);
    }
    else
    {
	fg = fbReplicatePixel (fg, pDrawable->bitsPerPixel);
	fbSolidBoxClipped (pDrawable, pClip, x1, y1, x2, y2,
			   fbAnd (GXcopy, fg, pm),
			   fbXor (GXcopy, fg, pm));
    }
}

void
kaaImageGlyphBlt (DrawablePtr	pDrawable,
		  GCPtr		pGC,
		  int		x, 
		  int		y,
		  unsigned int	nglyph,
		  CharInfoPtr	*ppciInit,
		  pointer	pglyphBase)
{
    KaaScreenPriv (pDrawable->pScreen);
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
	KdCheckImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppciInit, pglyphBase);
	return;
    }
    glyph = 0;
    fbGetDrawable (pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);
    switch (dstBpp) {
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
        kaaSolidBoxClipped (pDrawable,
			    fbGetCompositeClip(pGC),
			    pGC->planemask,
			    pGC->bgPixel,
			    xBack,
			    yBack,
			    xBack + widthBack,
			    yBack + heightBack);
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
		(*glyph) (dst + (gy - dstYoff) * dstStride,
			  dstStride,
			  dstBpp,
			  (FbStip *) pglyph,
			  pPriv->fg,
			  gx - dstXoff,
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

static const GCOps	kaaOps = {
    kaaFillSpans,
    KdCheckSetSpans,
    KdCheckPutImage,
    kaaCopyArea,
    KdCheckCopyPlane,
    KdCheckPolyPoint,
    KdCheckPolylines,
    KdCheckPolySegment,
    miPolyRectangle,
    KdCheckPolyArc,
    miFillPolygon,
    kaaPolyFillRect,
    miPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    kaaImageGlyphBlt,
    KdCheckPolyGlyphBlt,
    KdCheckPushPixels,
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

void
kaaValidateGC (GCPtr pGC, Mask changes, DrawablePtr pDrawable)
{
    FbGCPrivPtr fbPriv = fbGetGCPrivate(pGC);
    
    fbValidateGC (pGC, changes, pDrawable);

    if (pDrawable->type == DRAWABLE_WINDOW)
	pGC->ops = (GCOps *) &kaaOps;
    else
	pGC->ops = (GCOps *) &kdAsyncPixmapGCOps;
}

GCFuncs	kaaGCFuncs = {
    kaaValidateGC,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip
};

int
kaaCreateGC (GCPtr pGC)
{
    if (!fbCreateGC (pGC))
	return FALSE;

    if (pGC->depth != 1)
	pGC->funcs = &kaaGCFuncs;
    
    return TRUE;
}

void
kaaCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr	pScreen = pWin->drawable.pScreen;
    KaaScreenPriv (pScreen);
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
		  &rgnDst, dx, dy, kaaCopyNtoN, 0, 0);
    
    REGION_UNINIT(pWin->drawable.pScreen, &rgnDst);
}

void
kaaFillRegionSolid (DrawablePtr	pDrawable,
		    RegionPtr	pRegion,
		    Pixel	pixel)
{
    KaaScreenPriv(pDrawable->pScreen);

    if ((*pKaaScr->PrepareSolid) (pDrawable, GXcopy, FB_ALLONES, pixel))
    {
	int	nbox = REGION_NUM_RECTS (pRegion);
	BoxPtr	pBox = REGION_RECTS (pRegion);
	
	while (nbox--)
	{
	    (*pKaaScr->Solid) (pBox->x1, pBox->y1, pBox->x2, pBox->y2);
	    pBox++;
	}
	(*pKaaScr->DoneSolid) ();
	KdMarkSync(pDrawable->pScreen);
    }
    else
	fbFillRegionSolid (pDrawable, pRegion, 0,
			   fbReplicatePixel (pixel, pDrawable->bitsPerPixel));
}

void
kaaPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what)
{
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
	case BackgroundPixel:
	    kaaFillRegionSolid((DrawablePtr)pWin, pRegion, pWin->background.pixel);
	    return;
    	}
    	break;
    case PW_BORDER:
	if (pWin->borderIsPixel)
	{
	    kaaFillRegionSolid((DrawablePtr)pWin, pRegion, pWin->border.pixel);
	    return;
	}
	break;
    }
    KdCheckPaintWindow (pWin, pRegion, what);
}

Bool
kaaDrawInit (ScreenPtr		pScreen,
	     KaaScreenPrivPtr	pScreenPriv)
{
    if (kaaGeneration != serverGeneration)
    {
	kaaScreenPrivateIndex = AllocateScreenPrivateIndex();
	kaaGeneration = serverGeneration;
    }
    pScreen->devPrivates[kaaScreenPrivateIndex].ptr = (pointer) pScreenPriv;
    
    /*
     * Hook up asynchronous drawing
     */
    KdScreenInitAsync (pScreen);
    /*
     * Replace various fb screen functions
     */
    pScreen->CreateGC = kaaCreateGC;
    pScreen->CopyWindow = kaaCopyWindow;
    pScreen->PaintWindowBackground = kaaPaintWindow;
    pScreen->PaintWindowBorder = kaaPaintWindow;

    return TRUE;
}

