/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaPaintWin.c,v 1.11 2003/02/17 16:08:29 dawes Exp $ */

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
#include "gcstruct.h"
#include "pixmapstr.h"
#include "xaawrap.h"

#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif

void
XAAPaintWindow(
  WindowPtr pWin,
  RegionPtr prgn,
  int what 
)
{
    ScreenPtr  pScreen = pWin->drawable.pScreen;
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_DRAWABLE((&pWin->drawable));
    int nBox = REGION_NUM_RECTS(prgn);
    BoxPtr pBox = REGION_RECTS(prgn);
    int fg = -1;
    PixmapPtr pPix = NULL;

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


    if(!pPix) {
        if(infoRec->FillSolidRects &&
           (!(infoRec->FillSolidRectsFlags & RGB_EQUAL) || 
                (CHECK_RGB_EQUAL(fg))) )  {
	    (*infoRec->FillSolidRects)(infoRec->pScrn, fg, GXcopy, ~0,
					nBox, pBox);
	    return;
	}
    } else {	/* pixmap */
        XAAPixmapPtr pPriv = XAA_GET_PIXMAP_PRIVATE(pPix);
	WindowPtr pBgWin = pWin;
	Bool NoCache = FALSE;
	int xorg, yorg;

	/* Hack so we can use this with the dual framebuffer layers
	   which only support the pixmap cache in the primary bpp */
	if(pPix->drawable.bitsPerPixel != infoRec->pScrn->bitsPerPixel)
	    NoCache = TRUE;

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
	     
	    (*infoRec->FillCacheBltRects)(infoRec->pScrn, GXcopy, ~0,
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
		!(infoRec->FillMono8x8PatternRectsFlags & TRANSPARENCY_ONLY) && 
		(!(infoRec->FillMono8x8PatternRectsFlags & RGB_EQUAL) || 
		(CHECK_RGB_EQUAL(pPriv->fg) && CHECK_RGB_EQUAL(pPriv->bg)))) {

	    	(*infoRec->FillMono8x8PatternRects)(infoRec->pScrn,
			pPriv->fg, pPriv->bg, GXcopy, ~0, nBox, pBox,
			pPriv->pattern0, pPriv->pattern1, xorg, yorg);
		return;
	    }
	    if(infoRec->CanDoColor8x8 && !NoCache &&
				infoRec->FillColor8x8PatternRects) {
		XAACacheInfoPtr pCache = (*infoRec->CacheColor8x8Pattern)(
					infoRec->pScrn, pPix, -1, -1);

		(*infoRec->FillColor8x8PatternRects) ( infoRec->pScrn, 
				GXcopy, ~0, nBox, pBox, xorg, yorg, pCache);
		return;
	    }        
	}

	/* The window size check is to reduce pixmap cache thrashing
	   when there are lots of little windows with pixmap backgrounds
	   like are sometimes used for buttons, etc... */

	if(infoRec->UsingPixmapCache && 
	    infoRec->FillCacheBltRects && !NoCache &&
	    ((what == PW_BORDER) ||
		(pPix->drawable.height != pWin->drawable.height) ||
		(pPix->drawable.width != pWin->drawable.width)) &&
	    (pPix->drawable.height <= infoRec->MaxCacheableTileHeight) &&
	    (pPix->drawable.width <= infoRec->MaxCacheableTileWidth)) {

	     XAACacheInfoPtr pCache = 
			(*infoRec->CacheTile)(infoRec->pScrn, pPix);
	     (*infoRec->FillCacheBltRects)(infoRec->pScrn, GXcopy, ~0,
					nBox, pBox, xorg, yorg, pCache);
	     return;
	}

	if(infoRec->FillImageWriteRects && 
		!(infoRec->FillImageWriteRectsFlags & NO_GXCOPY)) {
	    (*infoRec->FillImageWriteRects) (infoRec->pScrn, GXcopy, 
                   		~0, nBox, pBox, xorg, yorg, pPix);
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
	XAA_SCREEN_EPILOGUE(pScreen, PaintWindowBackground, XAAPaintWindow);
    } else {
	XAA_SCREEN_PROLOGUE (pScreen, PaintWindowBorder);
	(*pScreen->PaintWindowBorder) (pWin, prgn, what);
	XAA_SCREEN_EPILOGUE(pScreen, PaintWindowBorder, XAAPaintWindow);
    }

}
