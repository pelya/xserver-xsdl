/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaOverlay.c,v 1.15 2003/11/10 18:22:41 tsi Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "xf86str.h"
#include "xaa.h"
#include "xaalocal.h"
#include "xaawrap.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "mioverlay.h"

#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif

static void
XAACopyWindow8_32(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc
){
    DDXPointPtr pptSrc, ppt;
    RegionRec rgnDst;
    BoxPtr pbox;
    int dx, dy, nbox;
    WindowPtr pwinRoot;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    XAAInfoRecPtr infoRec = 
	GET_XAAINFORECPTR_FROM_DRAWABLE((&pWin->drawable));
    Bool doUnderlay = miOverlayCopyUnderlay(pScreen);
    RegionPtr borderClip = &pWin->borderClip;
    Bool freeReg = FALSE;

    if (!infoRec->pScrn->vtSema || !infoRec->ScreenToScreenBitBlt ||
	(infoRec->ScreenToScreenBitBltFlags & NO_PLANEMASK)) 
    { 
	XAA_SCREEN_PROLOGUE (pScreen, CopyWindow);
	if(infoRec->pScrn->vtSema && infoRec->NeedToSync) {
	    (*infoRec->Sync)(infoRec->pScrn);
	    infoRec->NeedToSync = FALSE;
	}
        (*pScreen->CopyWindow) (pWin, ptOldOrg, prgnSrc);
	XAA_SCREEN_EPILOGUE (pScreen, CopyWindow, XAACopyWindow8_32);
    	return;
    }

    pwinRoot = WindowTable[pScreen->myNum];

    if(doUnderlay)
	freeReg = miOverlayCollectUnderlayRegions(pWin, &borderClip);

    REGION_NULL(pScreen, &rgnDst);

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    REGION_TRANSLATE(pScreen, prgnSrc, -dx, -dy);
    REGION_INTERSECT(pScreen, &rgnDst, borderClip, prgnSrc);

    pbox = REGION_RECTS(&rgnDst);
    nbox = REGION_NUM_RECTS(&rgnDst);
    if(!nbox || 
      !(pptSrc = (DDXPointPtr )ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec)))) {
	REGION_UNINIT(pScreen, &rgnDst);
	return;
    }
    ppt = pptSrc;

    while(nbox--) {
	ppt->x = pbox->x1 + dx;
	ppt->y = pbox->y1 + dy;
	ppt++; pbox++;
    }
    
    infoRec->ScratchGC.planemask = doUnderlay ? 0x00ffffff : 0xff000000;
    infoRec->ScratchGC.alu = GXcopy;

    XAADoBitBlt((DrawablePtr)pwinRoot, (DrawablePtr)pwinRoot,
        		&(infoRec->ScratchGC), &rgnDst, pptSrc);

    DEALLOCATE_LOCAL(pptSrc);
    REGION_UNINIT(pScreen, &rgnDst);
    if(freeReg) 
	REGION_DESTROY(pScreen, borderClip);
}




static void
XAAPaintWindow8_32(
  WindowPtr pWin,
  RegionPtr prgn,
  int what 
){
    ScreenPtr  pScreen = pWin->drawable.pScreen;
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_DRAWABLE((&pWin->drawable));
    int nBox = REGION_NUM_RECTS(prgn);
    BoxPtr pBox = REGION_RECTS(prgn);
    PixmapPtr pPix = NULL;
    int depth = pWin->drawable.depth;
    int fg = 0, pm;

    if(!infoRec->pScrn->vtSema) goto BAILOUT;	

    switch (what) {
    case PW_BACKGROUND:
	switch(pWin->backgroundState) {
	case None: return;
	case ParentRelative:
	    do { pWin = pWin->parent; }
	    while(pWin->backgroundState == ParentRelative);
	    (*pWin->drawable.pScreen->PaintWindowBackground)(pWin, prgn, what);
	    return;
	case BackgroundPixel:
	    fg = pWin->background.pixel;
	    break;
	case BackgroundPixmap:
	    pPix = pWin->background.pixmap;
	    break;
	}
	break;
    case PW_BORDER:
	if (pWin->borderIsPixel) 
	    fg = pWin->border.pixel;
	else 	/* pixmap */ 
	    pPix = pWin->border.pixmap;
	break;
    default: return;
    }

    if(depth == 8) {
	pm = 0xff000000;
	fg <<= 24;
    } else
	pm = 0x00ffffff;

    if(!pPix) {	
        if(infoRec->FillSolidRects &&
           !(infoRec->FillSolidRectsFlags & NO_PLANEMASK) &&
           (!(infoRec->FillSolidRectsFlags & RGB_EQUAL) ||
			(depth == 8) || CHECK_RGB_EQUAL(fg)))  
	{
	    (*infoRec->FillSolidRects)(infoRec->pScrn, fg, GXcopy, 
						pm, nBox, pBox);
	    return;
	}
    } else {	/* pixmap */
        XAAPixmapPtr pPriv = XAA_GET_PIXMAP_PRIVATE(pPix);
	WindowPtr pBgWin = pWin;
	int xorg, yorg;

	if (what == PW_BORDER) {
	    for (pBgWin = pWin;
		 pBgWin->backgroundState == ParentRelative;
		 pBgWin = pBgWin->parent);
	}

        xorg = pBgWin->drawable.x;
        yorg = pBgWin->drawable.y;

#ifdef PANORAMIX
	if(!noPanoramiXExtension) {
	    int index = pScreen->myNum;
	    if(WindowTable[index] == pBgWin) {
		xorg -= panoramiXdataPtr[index].x;
		yorg -= panoramiXdataPtr[index].y;
	    }
	}
#endif

	if(IS_OFFSCREEN_PIXMAP(pPix) && infoRec->FillCacheBltRects) {
	    XAACacheInfoPtr pCache = &(infoRec->ScratchCacheInfoRec);

	    pCache->x = pPriv->offscreenArea->box.x1;
	    pCache->y = pPriv->offscreenArea->box.y1;
	    pCache->w = pCache->orig_w = 
		pPriv->offscreenArea->box.x2 - pCache->x;
	    pCache->h = pCache->orig_h = 
		pPriv->offscreenArea->box.y2 - pCache->y;
	    pCache->trans_color = -1;
	     
	    (*infoRec->FillCacheBltRects)(infoRec->pScrn, GXcopy, pm,
				nBox, pBox, xorg, yorg, pCache);

	    return;
	}

	if(pPriv->flags & DIRTY) {
	    pPriv->flags &= ~(DIRTY | REDUCIBILITY_MASK);
	    pPix->drawable.serialNumber = NEXT_SERIAL_NUMBER;
        }

    	if(!(pPriv->flags & REDUCIBILITY_CHECKED) &&
	    (infoRec->CanDoMono8x8 || infoRec->CanDoColor8x8)) {
	    XAACheckTileReducibility(pPix, infoRec->CanDoMono8x8);
	}

	if(pPriv->flags & REDUCIBLE_TO_8x8) {
	    if((pPriv->flags & REDUCIBLE_TO_2_COLOR) &&
		infoRec->CanDoMono8x8 && infoRec->FillMono8x8PatternRects &&
		!(infoRec->FillMono8x8PatternRectsFlags & NO_PLANEMASK) &&
		!(infoRec->FillMono8x8PatternRectsFlags & TRANSPARENCY_ONLY) && 
		(!(infoRec->FillMono8x8PatternRectsFlags & RGB_EQUAL) || 
		(CHECK_RGB_EQUAL(pPriv->fg) && CHECK_RGB_EQUAL(pPriv->bg)))) 
	    {
		(*infoRec->FillMono8x8PatternRects)(infoRec->pScrn,
			pPriv->fg, pPriv->bg, GXcopy, pm, nBox, pBox,
			pPriv->pattern0, pPriv->pattern1, xorg, yorg);
		return;
	    }
	    if(infoRec->CanDoColor8x8 && infoRec->FillColor8x8PatternRects &&
		!(infoRec->FillColor8x8PatternRectsFlags & NO_PLANEMASK)) 
	    {
		XAACacheInfoPtr pCache = (*infoRec->CacheColor8x8Pattern)(
					infoRec->pScrn, pPix, -1, -1);

		(*infoRec->FillColor8x8PatternRects) (infoRec->pScrn, 
			GXcopy, pm, nBox, pBox, xorg, yorg, pCache);
		return;
	    }        
	}

	if(infoRec->UsingPixmapCache && infoRec->FillCacheBltRects && 
	    !(infoRec->FillCacheBltRectsFlags & NO_PLANEMASK) && 
	    (pPix->drawable.height <= infoRec->MaxCacheableTileHeight) &&
	    (pPix->drawable.width <= infoRec->MaxCacheableTileWidth)) 
	{
	     XAACacheInfoPtr pCache = 
			(*infoRec->CacheTile)(infoRec->pScrn, pPix);
	     (*infoRec->FillCacheBltRects)(infoRec->pScrn, GXcopy, pm,
				nBox, pBox, xorg, yorg, pCache);
	     return;
	}

	if(infoRec->FillImageWriteRects && 
		!(infoRec->FillImageWriteRectsFlags & NO_PLANEMASK)) 
	{
	    (*infoRec->FillImageWriteRects) (infoRec->pScrn, GXcopy, 
			pm, nBox, pBox, xorg, yorg, pPix);
	    return;
	}
    }

    if(infoRec->NeedToSync) {
	(*infoRec->Sync)(infoRec->pScrn);
	infoRec->NeedToSync = FALSE;
    }

BAILOUT:

    if(what == PW_BACKGROUND) {
	XAA_SCREEN_PROLOGUE (pScreen, PaintWindowBackground);
	(*pScreen->PaintWindowBackground) (pWin, prgn, what);
	XAA_SCREEN_EPILOGUE(pScreen, PaintWindowBackground, XAAPaintWindow8_32);
    } else {
	XAA_SCREEN_PROLOGUE (pScreen, PaintWindowBorder);
	(*pScreen->PaintWindowBorder) (pWin, prgn, what);
	XAA_SCREEN_EPILOGUE(pScreen, PaintWindowBorder, XAAPaintWindow8_32);
    }
}


static void
XAASetColorKey8_32(
    ScreenPtr pScreen,
    int nbox,
    BoxPtr pbox
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(pScreen);
    ScrnInfoPtr pScrn = infoRec->pScrn;

    /* I'm counting on writes being clipped away while switched away.
       If this isn't going to be true then I need to be wrapping instead. */
    if(!infoRec->pScrn->vtSema) return;

    (*infoRec->FillSolidRects)(pScrn, pScrn->colorKey << 24, GXcopy, 
					0xff000000, nbox, pbox);
  
    SET_SYNC_FLAG(infoRec);
}

void 
XAASetupOverlay8_32Planar(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(pScreen);
    int i;

    pScreen->PaintWindowBackground = XAAPaintWindow8_32;
    pScreen->PaintWindowBorder = XAAPaintWindow8_32;
    pScreen->CopyWindow = XAACopyWindow8_32;

    if(!(infoRec->FillSolidRectsFlags & NO_PLANEMASK))
	miOverlaySetTransFunction(pScreen, XAASetColorKey8_32);

    infoRec->FullPlanemask = ~0;
    for(i = 0; i < 32; i++) /* haven't thought about this much */
	infoRec->FullPlanemasks[i] = ~0;
}
