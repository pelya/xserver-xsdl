/*
   Copyright (C) 1999.  The XFree86 Project Inc.

   Written by Mark Vojkovich (mvojkovi@ucsd.edu)
*/

/* $XFree86: xc/programs/Xserver/hw/xfree86/xf8_16bpp/cfbwindow.c,v 1.4 2003/02/17 16:08:30 dawes Exp $ */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb8_16.h"
#include "mistruct.h"
#include "regionstr.h"
#include "cfbmskbits.h"
#include "xf86.h"

/* We don't bother with cfb's fastBackground/Border so we don't
   need to use the Window privates */


Bool
cfb8_16CreateWindow(WindowPtr pWin)
{
    cfbPrivWin *pPrivWin = cfbGetWindowPrivate(pWin);

    pPrivWin->pRotatedBorder = NullPixmap;
    pPrivWin->pRotatedBackground = NullPixmap;
    pPrivWin->fastBackground = FALSE;
    pPrivWin->fastBorder = FALSE;
    pPrivWin->oldRotate.x = 0;
    pPrivWin->oldRotate.y = 0;

    return TRUE;
}


Bool
cfb8_16DestroyWindow(WindowPtr pWin)
{
    return TRUE;
}

Bool
cfb8_16PositionWindow(
    WindowPtr pWin,
    int x, int y
){
    return TRUE;
}


void
cfb8_16CopyWindow(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc
){
    ScreenPtr pScreen = pWin->drawable.pScreen;
    cfb8_16ScreenPtr pScreenPriv = 
		CFB8_16_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    DDXPointPtr ppt, pptSrc;
    RegionRec rgnDst;
    BoxPtr pbox;
    int i, nbox, dx, dy;
    WindowPtr pRoot = WindowTable[pScreen->myNum];

    REGION_INIT(pScreen, &rgnDst, NullBox, 0);

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    REGION_TRANSLATE(pScreen, prgnSrc, -dx, -dy);
    REGION_INTERSECT(pScreen, &rgnDst, &pWin->borderClip, prgnSrc);

    nbox = REGION_NUM_RECTS(&rgnDst);
    if(nbox &&
	(pptSrc = (DDXPointPtr )ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec)))) {

	pbox = REGION_RECTS(&rgnDst);
	for (i = nbox, ppt = pptSrc; i--; ppt++, pbox++) {
	    ppt->x = pbox->x1 + dx;
	    ppt->y = pbox->y1 + dy;
	}
	cfbDoBitbltCopy((DrawablePtr)pRoot, (DrawablePtr)pRoot,
                		GXcopy, &rgnDst, pptSrc, ~0L);
	if(pWin->drawable.bitsPerPixel == 16)
	    cfb16DoBitbltCopy((DrawablePtr)pScreenPriv->pix16, 
			      (DrawablePtr)pScreenPriv->pix16,
                		GXcopy, &rgnDst, pptSrc, ~0L);

	DEALLOCATE_LOCAL(pptSrc);
    }

    REGION_UNINIT(pScreen, &rgnDst);

    if(pWin->drawable.depth == 8) {
      REGION_INIT(pScreen, &rgnDst, NullBox, 0);
      miSegregateChildren(pWin, &rgnDst, pScrn->depth);
      if(REGION_NOTEMPTY(pScreen, &rgnDst)) {
	REGION_INTERSECT(pScreen, &rgnDst, &rgnDst, prgnSrc);
	nbox = REGION_NUM_RECTS(&rgnDst);
	if(nbox &&
	  (pptSrc = (DDXPointPtr )ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec)))){

	    pbox = REGION_RECTS(&rgnDst);
	    for (i = nbox, ppt = pptSrc; i--; ppt++, pbox++) {
		ppt->x = pbox->x1 + dx;
		ppt->y = pbox->y1 + dy;
	    }

	    cfb16DoBitbltCopy((DrawablePtr)pScreenPriv->pix16, 
			      (DrawablePtr)pScreenPriv->pix16,
			      GXcopy, &rgnDst, pptSrc, ~0L);
	    DEALLOCATE_LOCAL(pptSrc);
	}
      }
      REGION_UNINIT(pScreen, &rgnDst);
    }
}

Bool
cfb8_16ChangeWindowAttributes(
    WindowPtr pWin,
    unsigned long mask
){
    if (pWin->drawable.bitsPerPixel == 16)
        return cfb16ChangeWindowAttributes(pWin,mask);
    else
        return cfbChangeWindowAttributes(pWin,mask);
}

void
cfb8_16WindowExposures(
   WindowPtr pWin,
   RegionPtr pReg,
   RegionPtr pOtherReg
){

    if(REGION_NUM_RECTS(pReg) && (pWin->drawable.bitsPerPixel == 16)) {
	cfb8_16ScreenPtr pScreenPriv = 
		CFB8_16_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);

	cfbFillBoxSolid((DrawablePtr)pScreenPriv->pix8,
		REGION_NUM_RECTS(pReg), REGION_RECTS(pReg),
		pScreenPriv->key);
    }

    miWindowExposures(pWin, pReg, pOtherReg);
}


void
cfb8_16PaintWindow (
    WindowPtr   pWin,
    RegionPtr   pRegion,
    int         what
){
    if(pWin->drawable.bitsPerPixel == 16) {
	cfb16PaintWindow(pWin, pRegion, what);
	if(what == PW_BORDER) {
	    cfb8_16ScreenPtr pScreenPriv = 
		CFB8_16_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);

	    cfbFillBoxSolid((DrawablePtr)pScreenPriv->pix8,
		REGION_NUM_RECTS(pRegion), REGION_RECTS(pRegion),
		pScreenPriv->key);
	}
    } else
	cfbPaintWindow(pWin, pRegion, what);

}

