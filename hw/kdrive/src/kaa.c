/*
 * $RCSId: xc/programs/Xserver/hw/kdrive/kaa.c,v 1.4 2001/06/04 09:45:41 keithp Exp $
 *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "kdrive.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"

#define DEBUG_MIGRATE 0
#if DEBUG_MIGRATE
#define DBG_MIGRATE(a) ErrorF a
#else
#define DBG_MIGRATE(a)
#endif
 
int kaaGeneration;
int kaaScreenPrivateIndex;
int kaaPixmapPrivateIndex;

typedef struct {
    KaaScreenInfoPtr info;
    
    CreatePixmapProcPtr CreatePixmap;
    DestroyPixmapProcPtr DestroyPixmap;
    int	pixelOffset;	/* offset from pPixmap to pixels */
} KaaScreenPrivRec, *KaaScreenPrivPtr;

typedef struct {
    KdOffscreenArea *area;
    int		    score;
    int		    devKind;
    DevUnion	    devPrivate;
} KaaPixmapPrivRec, *KaaPixmapPrivPtr;

#define KAA_PIXMAP_SCORE_MOVE_IN    10
#define KAA_PIXMAP_SCORE_MAX	    20
#define KAA_PIXMAP_SCORE_MOVE_OUT   -10
#define KAA_PIXMAP_SCORE_MIN	    -20

#define KaaGetScreenPriv(s)	((KaaScreenPrivPtr)(s)->devPrivates[kaaScreenPrivateIndex].ptr)
#define KaaScreenPriv(s)	KaaScreenPrivPtr    pKaaScr = KaaGetScreenPriv(s)

#define KaaGetPixmapPriv(p) ((KaaPixmapPrivPtr)(p)->devPrivates[kaaPixmapPrivateIndex].ptr)
#define KaaSetPixmapPriv(p,a) ((p)->devPrivates[kaaPixmapPrivateIndex].ptr = (pointer) (a))
#define KaaPixmapPriv(p) KaaPixmapPrivPtr pKaaPixmap = KaaGetPixmapPriv(p)

#define KaaPixmapPitch(w) (((w) + (pKaaScr->info->offscreenPitch - 1)) & ~(pKaaScr->info->offscreenPitch - 1))
#define KaaDrawableIsOffscreenPixmap(d) (d->type == DRAWABLE_PIXMAP && \
					 KaaGetPixmapPriv((PixmapPtr)(d)) && \
					 KaaGetPixmapPriv((PixmapPtr)(d))->area)
#define KaaDrawableIsScreen(d)  (((d)->type == DRAWABLE_WINDOW) || \
				 KaaDrawableIsOffscreenPixmap(d))

#define KAA_SCREEN_PROLOGUE(pScreen, field) ((pScreen)->field = \
   ((KaaScreenPrivPtr) (pScreen)->devPrivates[kaaScreenPrivateIndex].ptr)->field)

#define KAA_SCREEN_EPILOGUE(pScreen, field, wrapper)\
    ((pScreen)->field = wrapper)

#define MIN_OFFPIX_SIZE		(4096)

static void
kaaPixmapSave (KdOffscreenArea *area)
{
    PixmapPtr pPixmap = area->privData;
    KaaPixmapPriv(pPixmap);
    int dst_pitch, src_pitch, bytes;
    unsigned char *dst, *src;
    int i; 
    
    DBG_MIGRATE (("Save 0x%08x (0x%x) (%dx%d)\n",
		  pPixmap->drawable.id,
		  KaaGetPixmapPriv(pPixmap)->area ? 
		  KaaGetPixmapPriv(pPixmap)->area->offset : -1,
		  pPixmap->drawable.width,
		  pPixmap->drawable.height));
		  
    KdCheckSync (pPixmap->drawable.pScreen);

    src_pitch = pPixmap->devKind;
    dst_pitch = pKaaPixmap->devKind;

    src = pPixmap->devPrivate.ptr;
    dst = pKaaPixmap->devPrivate.ptr;
    
    pPixmap->devKind = dst_pitch;
    pPixmap->devPrivate.ptr = dst;
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pKaaPixmap->area = NULL;
    
    bytes = src_pitch < dst_pitch ? src_pitch : dst_pitch;

    i = pPixmap->drawable.height;
    while (i--) {
	memcpy (dst, src, bytes);
	dst += dst_pitch;
	src += src_pitch;
    }
}

static Bool
kaaPixmapAllocArea (PixmapPtr pPixmap)
{
    ScreenPtr	pScreen = pPixmap->drawable.pScreen;
    KaaScreenPriv (pScreen);
    KaaPixmapPriv (pPixmap);
    int		bpp = pPixmap->drawable.bitsPerPixel;
    CARD16	h = pPixmap->drawable.height;
    CARD16	w = pPixmap->drawable.width;
    int		pitch = KaaPixmapPitch (w);
    PixmapPtr	pScreenPixmap = (*pScreen->GetScreenPixmap)(pScreen);
    
    pKaaPixmap->devKind = pPixmap->devKind;
    pKaaPixmap->devPrivate = pPixmap->devPrivate;
    pKaaPixmap->area = KdOffscreenAlloc (pScreen, pitch * h * (bpp >> 3),
					 pKaaScr->info->offscreenByteAlign,
					 FALSE, 
					 kaaPixmapSave, (pointer) pPixmap);
    if (!pKaaPixmap->area)
	return FALSE;
    
    DBG_MIGRATE(("++ 0x%08x (0x%x) (%dx%d)\n",
		  pPixmap->drawable.id,
		  KaaGetPixmapPriv(pPixmap)->area ? 
		  KaaGetPixmapPriv(pPixmap)->area->offset : -1,
		  pPixmap->drawable.width,
		  pPixmap->drawable.height));
    pPixmap->devKind = pitch * (bpp >> 3);
    pPixmap->devPrivate.ptr = (pointer) ((CARD8 *) pScreenPixmap->devPrivate.ptr + pKaaPixmap->area->offset);
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    return TRUE;
}

static void
kaaMoveInPixmap (PixmapPtr pPixmap)
{
    int dst_pitch, src_pitch, bytes;
    unsigned char *dst, *src;
    int i;

    return;
    KdCheckSync (pPixmap->drawable.pScreen);
    
    DBG_MIGRATE (("-> 0x%08x (0x%x) (%dx%d)\n",
		  pPixmap->drawable.id,
		  KaaGetPixmapPriv(pPixmap)->area ? 
		  KaaGetPixmapPriv(pPixmap)->area->offset : -1,
		  pPixmap->drawable.width,
		  pPixmap->drawable.height));

    src = pPixmap->devPrivate.ptr;
    src_pitch = pPixmap->devKind;
    
    if (!kaaPixmapAllocArea (pPixmap))
	return;
    
    dst = pPixmap->devPrivate.ptr;
    dst_pitch = pPixmap->devKind;
    
    bytes = src_pitch < dst_pitch ? src_pitch : dst_pitch;

    i = pPixmap->drawable.height;
    while (i--) {
	memcpy (dst, src, bytes);
	dst += dst_pitch;
	src += src_pitch;
    }
}

static void
kaaMoveOutPixmap (PixmapPtr pPixmap)
{
    KaaPixmapPriv (pPixmap);
    KdOffscreenArea *area = pKaaPixmap->area;

    DBG_MIGRATE (("<- 0x%08x (0x%x) (%dx%d)\n",
		  pPixmap->drawable.id,
		  KaaGetPixmapPriv(pPixmap)->area ? 
		  KaaGetPixmapPriv(pPixmap)->area->offset : -1,
		  pPixmap->drawable.width,
		  pPixmap->drawable.height));
    if (area)
    {
	kaaPixmapSave (area);
	KdOffscreenFree (area);
    }
}

static void
kaaPixmapUseScreen (PixmapPtr pPixmap)
{
    KaaPixmapPriv (pPixmap);

    if (pKaaPixmap->score < KAA_PIXMAP_SCORE_MAX)
    {
	pKaaPixmap->score++;
	DBG_MIGRATE (("UseScreen 0x%x (%d)\n", pPixmap->drawable.id, pKaaPixmap->score));
	if (!pKaaPixmap->area &&
	    pKaaPixmap->score >= KAA_PIXMAP_SCORE_MOVE_IN)
	    kaaMoveInPixmap (pPixmap);
    }
}

static void
kaaPixmapUseMemory (PixmapPtr pPixmap)
{
    KaaPixmapPriv (pPixmap);

    if (pKaaPixmap->score > KAA_PIXMAP_SCORE_MIN)
    {
	pKaaPixmap->score--;
	DBG_MIGRATE (("UseMemory 0x%x (%d)\n", pPixmap->drawable.id, pKaaPixmap->score));
	if (pKaaPixmap->area &&
	    pKaaPixmap->score <= KAA_PIXMAP_SCORE_MOVE_OUT)
	    kaaMoveOutPixmap (pPixmap);
    }
}

static Bool
kaaDestroyPixmap (PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    Bool ret;
    
    if (pPixmap->refcnt == 1)
    {
	KaaPixmapPriv (pPixmap);
	if (pKaaPixmap->area)
	{
	    DBG_MIGRATE(("-- 0x%08x (0x%x) (%dx%d)\n",
			 pPixmap->drawable.id,
			 KaaGetPixmapPriv(pPixmap)->area->offset,
			 pPixmap->drawable.width,
			 pPixmap->drawable.height));
	    /* Free the offscreen area */
	    KdCheckSync (pScreen);
	    KdOffscreenFree (pKaaPixmap->area);
	    pPixmap->devPrivate = pKaaPixmap->devPrivate;
	    pPixmap->devKind = pKaaPixmap->devKind;
	}
    }

    KAA_SCREEN_PROLOGUE (pScreen, DestroyPixmap);
    ret = (*pScreen->DestroyPixmap) (pPixmap);
    KAA_SCREEN_EPILOGUE (pScreen, DestroyPixmap, kaaDestroyPixmap);
    
    return ret;
}

static PixmapPtr 
kaaCreatePixmap(ScreenPtr pScreen, int w, int h, int depth)
{
    PixmapPtr pPixmap = NULL;
    KaaPixmapPrivPtr	pKaaPixmap;
    
    KAA_SCREEN_PROLOGUE (pScreen, CreatePixmap);
    pPixmap = (* pScreen->CreatePixmap) (pScreen, w, h, depth);
    KAA_SCREEN_EPILOGUE (pScreen, CreatePixmap, kaaCreatePixmap);
    if (!pPixmap)
	return NULL;
    pKaaPixmap = KaaGetPixmapPriv(pPixmap);
    pKaaPixmap->score = 0;
    pKaaPixmap->area = NULL;
    
/*    if ((pPixmap->devKind * h >= MIN_OFFPIX_SIZE) */
    if (w * h > 100 * 100)
	kaaPixmapAllocArea (pPixmap);
    return pPixmap;
}

PixmapPtr
kaaGetDrawingPixmap (DrawablePtr pDrawable, int *x, int *y)
{
    if (pDrawable->type == DRAWABLE_WINDOW) {
	if (x)
	    *x = pDrawable->x;
	if (y)
	    *y = pDrawable->y;

	return (*pDrawable->pScreen->GetScreenPixmap) (pDrawable->pScreen);
    }
    else if (KaaDrawableIsOffscreenPixmap (pDrawable))
    {
	if (x)
	    *x = 0;
	if (y)
	    *y = 0;
	return ((PixmapPtr)pDrawable);
    }
    else
	return NULL;
}

void
kaaFillSpans(DrawablePtr pDrawable, GCPtr pGC, int n, 
	     DDXPointPtr ppt, int *pwidth, int fSorted)
{
    ScreenPtr	    pScreen = pDrawable->pScreen;
    KdScreenPriv (pScreen);
    KaaScreenPriv (pScreen);
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    PixmapPtr	    pPixmap;    
    BoxPtr	    pextent, pbox;
    int		    nbox;
    int		    extentX1, extentX2, extentY1, extentY2;
    int		    fullX1, fullX2, fullY1;
    int		    partX1, partX2;

    if (!pScreenPriv->enabled ||
	pGC->fillStyle != FillSolid ||
	!(pPixmap = kaaGetDrawingPixmap (pDrawable, NULL, NULL)) ||
	!(*pKaaScr->info->PrepareSolid) (pPixmap,
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
	    (*pKaaScr->info->Solid) (fullX1, fullY1, fullX2, fullY1 + 1);
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
			(*pKaaScr->info->Solid) (partX1, fullY1, partX2, fullY1 + 1);
		}
		pbox++;
	    }
	}
    }
    (*pKaaScr->info->DoneSolid) ();
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
    KdScreenPriv (pDstDrawable->pScreen);
    KaaScreenPriv (pDstDrawable->pScreen);
    PixmapPtr pSrcPixmap, pDstPixmap;

    /* Migrate pixmaps to same place as destination */
    if (pScreenPriv->enabled && pSrcDrawable->type == DRAWABLE_PIXMAP) {
	if (KaaDrawableIsScreen (pDstDrawable))
	    kaaPixmapUseScreen ((PixmapPtr) pSrcDrawable);
	else
	    kaaPixmapUseMemory ((PixmapPtr) pSrcDrawable);
    }

    if (pScreenPriv->enabled &&
	(pSrcPixmap = kaaGetDrawingPixmap (pSrcDrawable, NULL, NULL)) &&
	(pDstPixmap = kaaGetDrawingPixmap (pDstDrawable, NULL, NULL)) && 
	(*pKaaScr->info->PrepareCopy) (pSrcPixmap,
				       pDstPixmap,
				       dx,
				       dy,
				       pGC ? pGC->alu : GXcopy,
				       pGC ? pGC->planemask : FB_ALLONES))
    {
	while (nbox--)
	{
	    (*pKaaScr->info->Copy) (pbox->x1 + dx, pbox->y1 + dy,
				    pbox->x1, pbox->y1,
				    pbox->x2 - pbox->x1,
				    pbox->y2 - pbox->y1);
	    pbox++;
	}
	(*pKaaScr->info->DoneCopy) ();
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
    KdScreenPriv (pDrawable->pScreen);
    KaaScreenPriv (pDrawable->pScreen);
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    PixmapPtr	    pPixmap;
    register BoxPtr pbox;
    BoxPtr	    pextent;
    int		    extentX1, extentX2, extentY1, extentY2;
    int		    fullX1, fullX2, fullY1, fullY2;
    int		    partX1, partX2, partY1, partY2;
    int		    xorg, yorg;
    int		    n;
    
    if (!pScreenPriv->enabled ||
	pGC->fillStyle != FillSolid ||
	!(pPixmap = kaaGetDrawingPixmap (pDrawable, &xorg, &yorg)) || 
	!(*pKaaScr->info->PrepareSolid) (pPixmap,
					 pGC->alu,
					 pGC->planemask,
					 pGC->fgPixel))
    {
	KdCheckPolyFillRect (pDrawable, pGC, nrect, prect);
	return;
    }
    
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
	    (*pKaaScr->info->Solid) (fullX1, fullY1, fullX2, fullY2);
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
		    (*pKaaScr->info->Solid) (partX1, partY1,
					     partX2, partY2);
	    }
	}
    }
    (*pKaaScr->info->DoneSolid) ();
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
    KdScreenPriv (pDrawable->pScreen);
    KaaScreenPriv (pDrawable->pScreen);
    PixmapPtr   pPixmap;        
    BoxPtr	pbox;
    int		nbox;
    int		partX1, partX2, partY1, partY2;

    if (!pScreenPriv->enabled ||
	!(pPixmap = kaaGetDrawingPixmap (pDrawable, NULL, NULL)) ||
	!(*pKaaScr->info->PrepareSolid) (pPixmap, GXcopy, pm, fg))
    {
	KdCheckSync (pDrawable->pScreen);
	fg = fbReplicatePixel (fg, pDrawable->bitsPerPixel);
	fbSolidBoxClipped (pDrawable, pClip, x1, y1, x2, y2,
			   fbAnd (GXcopy, fg, pm),
			   fbXor (GXcopy, fg, pm));
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
	
	(*pKaaScr->info->Solid) (partX1, partY1, partX2, partY2);
    }
    (*pKaaScr->info->DoneSolid) ();
    KdMarkSync(pDrawable->pScreen);
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
    fbValidateGC (pGC, changes, pDrawable);

    if (KaaDrawableIsScreen (pDrawable))
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

    pGC->funcs = &kaaGCFuncs;

    return TRUE;
}


void
kaaCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
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
    KdScreenPriv(pDrawable->pScreen);
    KaaScreenPriv(pDrawable->pScreen);
    PixmapPtr pPixmap;

    if (pScreenPriv->enabled &&
	(pPixmap = kaaGetDrawingPixmap (pDrawable, NULL, NULL)) &&
	(*pKaaScr->info->PrepareSolid) (pPixmap, GXcopy, FB_ALLONES, pixel))
    {
	int	nbox = REGION_NUM_RECTS (pRegion);
	BoxPtr	pBox = REGION_RECTS (pRegion);
	
	while (nbox--)
	{
	    (*pKaaScr->info->Solid) (pBox->x1, pBox->y1, pBox->x2, pBox->y2);
	    pBox++;
	}
	(*pKaaScr->info->DoneSolid) ();
	KdMarkSync(pDrawable->pScreen);
    }
    else
    {
	KdCheckSync (pDrawable->pScreen);
	fbFillRegionSolid (pDrawable, pRegion, 0,
			   fbReplicatePixel (pixel, pDrawable->bitsPerPixel));
    }
}

void
kaaPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what)
{

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

#ifdef RENDER
static void
kaaComposite(CARD8	op,
	     PicturePtr pSrc,
	     PicturePtr pMask,
	     PicturePtr pDst,
	     INT16	xSrc,
	     INT16	ySrc,
	     INT16	xMask,
	     INT16	yMask,
	     INT16	xDst,
	     INT16	yDst,
	     CARD16	width,
	     CARD16	height)
{
#if 0
    if (pSrc->pDrawable->type == DRAWABLE_PIXMAP)
	kaaPixmapUseMemory ((PixmapPtr) pSrc->pDrawable);
    if (pMask && pMask->pDrawable->type == DRAWABLE_PIXMAP)
	kaaPixmapUseMemory ((PixmapPtr) pMask->pDrawable);
    if (pDst->pDrawable->type == DRAWABLE_PIXMAP)
	kaaPixmapUseMemory ((PixmapPtr) pDst->pDrawable);
#endif
    
    KdCheckComposite (op, pSrc, pMask, pDst, xSrc, ySrc, 
		      xMask, yMask, xDst, yDst, width, height);
}
#endif

Bool
kaaDrawInit (ScreenPtr		pScreen,
	     KaaScreenInfoPtr	pScreenInfo)
{
    KaaScreenPrivPtr pKaaScr;
    KdScreenInfo *screen = KdGetScreenPriv (pScreen)->screen;
#ifdef RENDER
    PictureScreenPtr	ps = GetPictureScreenIfSet(pScreen);
#endif
    
    if (kaaGeneration != serverGeneration)
    {
	kaaScreenPrivateIndex = AllocateScreenPrivateIndex();
	kaaPixmapPrivateIndex = AllocatePixmapPrivateIndex();
	kaaGeneration = serverGeneration;
    }

    pKaaScr = xalloc (sizeof (KaaScreenPrivRec));

    if (!pKaaScr)
	return FALSE;
    
    pKaaScr->info = pScreenInfo;
    
    pScreen->devPrivates[kaaScreenPrivateIndex].ptr = (pointer) pKaaScr;
    
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
#ifdef RENDER
    if (ps)
	ps->Composite = kaaComposite;
#endif

    /*
     * Hookup offscreen pixmaps
     */
    if ((pKaaScr->info->flags & KAA_OFFSCREEN_PIXMAPS) &&
	screen->off_screen_size > 0)
    {
	pKaaScr->CreatePixmap = pScreen->CreatePixmap;
	pScreen->CreatePixmap = kaaCreatePixmap;
	pKaaScr->DestroyPixmap = pScreen->DestroyPixmap;
	pScreen->DestroyPixmap = kaaDestroyPixmap;
	if (!AllocatePixmapPrivate(pScreen, kaaPixmapPrivateIndex,
				   sizeof (KaaPixmapPrivRec)))
	    return FALSE;
    }
    else
    {
	if (!AllocatePixmapPrivate(pScreen, kaaPixmapPrivateIndex, 0))
	    return FALSE;
    }

    return TRUE;
}

