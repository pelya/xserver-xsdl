/* $XFree86: xc/programs/Xserver/hw/xfree86/xf8_32bpp/cfbbstore.c,v 1.3 2003/07/16 01:38:50 dawes Exp $ */

#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb32.h"
#include "cfb8_32.h"
#include "X.h"
#include "mibstore.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"

void
cfb8_32SaveAreas(
    PixmapPtr	  	pPixmap,
    RegionPtr	  	prgnSave, 
    int	    	  	xorg,
    int	    	  	yorg,
    WindowPtr		pWin
){
    DDXPointPtr pPt;
    DDXPointPtr	pPtsInit;
    BoxPtr	pBox;
    int		i;
    ScreenPtr	pScreen = pPixmap->drawable.pScreen;
    PixmapPtr	pScrPix;

    if(pPixmap->drawable.bitsPerPixel == 32) {
	cfb32SaveAreas(pPixmap, prgnSave, xorg, yorg, pWin);
	return;
    }
    
    i = REGION_NUM_RECTS(prgnSave);
    pPtsInit = (DDXPointPtr)ALLOCATE_LOCAL(i * sizeof(DDXPointRec));
    if (!pPtsInit)
	return;
    
    pBox = REGION_RECTS(prgnSave);
    pPt = pPtsInit;
    while (--i >= 0) {
	pPt->x = pBox->x1 + xorg;
	pPt->y = pBox->y1 + yorg;
	pPt++;
	pBox++;
    }

    pScrPix = (PixmapPtr) pScreen->devPrivate;

    cfbDoBitblt32To8((DrawablePtr) pScrPix, (DrawablePtr)pPixmap,
		    GXcopy, prgnSave, pPtsInit, ~0L);

    DEALLOCATE_LOCAL (pPtsInit);
}


void
cfb8_32RestoreAreas(
    PixmapPtr	  	pPixmap, 
    RegionPtr	  	prgnRestore,
    int	    	  	xorg,
    int	    	  	yorg,
    WindowPtr		pWin
){
    DDXPointPtr pPt;
    DDXPointPtr pPtsInit;
    BoxPtr	pBox;
    int		i;
    ScreenPtr	pScreen = pPixmap->drawable.pScreen;
    PixmapPtr	pScrPix;

    i = REGION_NUM_RECTS(prgnRestore);
    pPtsInit = (DDXPointPtr)ALLOCATE_LOCAL(i*sizeof(DDXPointRec));
    if (!pPtsInit)
	return;
    
    pBox = REGION_RECTS(prgnRestore);
    pPt = pPtsInit;
    while (--i >= 0) {
	pPt->x = pBox->x1 - xorg;
	pPt->y = pBox->y1 - yorg;
	pPt++;
	pBox++;
    }

    pScrPix = (PixmapPtr) pScreen->devPrivate;

    if(pPixmap->drawable.bitsPerPixel == 32) {
	if(pWin->drawable.depth == 24)
	    cfb32DoBitbltCopy((DrawablePtr)pPixmap, (DrawablePtr) pScrPix,
		    GXcopy, prgnRestore, pPtsInit, 0x00ffffff);
	else
	    cfb32DoBitbltCopy((DrawablePtr)pPixmap, (DrawablePtr) pScrPix,
		    GXcopy, prgnRestore, pPtsInit, ~0);
    } else {
	cfbDoBitblt8To32((DrawablePtr)pPixmap, (DrawablePtr) pScrPix,
		    GXcopy, prgnRestore, pPtsInit, ~0L);
    }

    DEALLOCATE_LOCAL (pPtsInit);
}
