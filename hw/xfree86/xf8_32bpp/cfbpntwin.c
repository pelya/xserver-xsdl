/* $XFree86: xc/programs/Xserver/hw/xfree86/xf8_32bpp/cfbpntwin.c,v 1.5 2001/10/01 13:44:15 eich Exp $ */

#include "X.h"

#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb32.h"
#include "cfb8_32.h"
#include "mi.h"

#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif

void
cfb8_32PaintWindow(
    WindowPtr   pWin,
    RegionPtr   pRegion,
    int         what
){
    WindowPtr pBgWin;
    int xorg, yorg;

    switch (what) {
    case PW_BACKGROUND:
	switch (pWin->backgroundState) {
	case None:
	    break;
	case ParentRelative:
	    do {
		pWin = pWin->parent;
	    } while (pWin->backgroundState == ParentRelative);
	    (*pWin->drawable.pScreen->PaintWindowBackground)(
						pWin, pRegion, what);
	    break;
	case BackgroundPixmap:
	    xorg = pWin->drawable.x;
	    yorg = pWin->drawable.y;
#ifdef PANORAMIX
	    if(!noPanoramiXExtension) {
		int index = pWin->drawable.pScreen->myNum;
		if(WindowTable[index] == pWin) {
		    xorg -= panoramiXdataPtr[index].x;
		    yorg -= panoramiXdataPtr[index].y;
		}
	    }
#endif
	    cfb32FillBoxTileOddGeneral ((DrawablePtr)pWin,
			(int)REGION_NUM_RECTS(pRegion), REGION_RECTS(pRegion),
			pWin->background.pixmap, xorg, yorg, GXcopy, 
			(pWin->drawable.depth == 24) ? 0x00ffffff : 0xff000000);
	    break;
	case BackgroundPixel:
	    if(pWin->drawable.depth == 24) 
		cfb8_32FillBoxSolid32 ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->background.pixel);
	    else
		cfb8_32FillBoxSolid8 ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->background.pixel);
	    break;
    	}
    	break;
    case PW_BORDER:
	if (pWin->borderIsPixel) {
	    if(pWin->drawable.depth == 24) {
		cfb8_32FillBoxSolid32 ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->border.pixel);
	    } else
		cfb8_32FillBoxSolid8 ((DrawablePtr)pWin,
			     (int)REGION_NUM_RECTS(pRegion),
			     REGION_RECTS(pRegion),
			     pWin->border.pixel);
	} else {
	    for (pBgWin = pWin;
		 pBgWin->backgroundState == ParentRelative;
		 pBgWin = pBgWin->parent);

	    xorg = pBgWin->drawable.x;
	    yorg = pBgWin->drawable.y;

#ifdef PANORAMIX
	    if(!noPanoramiXExtension) {
		int index = pWin->drawable.pScreen->myNum;
		if(WindowTable[index] == pBgWin) {
		    xorg -= panoramiXdataPtr[index].x;
		    yorg -= panoramiXdataPtr[index].y;
		}
	    }
#endif
	    cfb32FillBoxTileOddGeneral ((DrawablePtr)pWin,
			(int)REGION_NUM_RECTS(pRegion), REGION_RECTS(pRegion),
			pWin->border.pixmap, xorg, yorg, GXcopy, 
			(pWin->drawable.depth == 24) ? 0x00ffffff : 0xff000000);
	}
	break;
    }

}

void
cfb8_32FillBoxSolid8(
   DrawablePtr pDraw,
   int nbox,
   BoxPtr pbox,
   unsigned long color
){
    CARD8 *ptr, *data;
    int pitch, height, width, i;
    CARD8 c = (CARD8)color;

    cfbGetByteWidthAndPointer(pDraw, pitch, ptr);
    ptr += 3; /* point to the top byte */

    while(nbox--) {
	data = ptr + (pbox->y1 * pitch) + (pbox->x1 << 2);
	width = (pbox->x2 - pbox->x1) << 2;
	height = pbox->y2 - pbox->y1;

	while(height--) {
            for(i = 0; i < width; i+=4)
		data[i] = c;
            data += pitch;
	}
	pbox++;
    }
}


void
cfb8_32FillBoxSolid32(
   DrawablePtr pDraw,
   int nbox,
   BoxPtr pbox,
   unsigned long color
){
    CARD8 *ptr, *data;
    CARD16 *ptr2, *data2;
    int pitch, pitch2;
    int height, width, i;
    CARD8 c = (CARD8)(color >> 16);
    CARD16 c2 = (CARD16)color;

    cfbGetByteWidthAndPointer(pDraw, pitch, ptr);
    cfbGetTypedWidthAndPointer(pDraw, pitch2, ptr2, CARD16, CARD16);
    ptr += 2; /* point to the third byte */

    while(nbox--) {
	data = ptr + (pbox->y1 * pitch) + (pbox->x1 << 2);
	data2 = ptr2 + (pbox->y1 * pitch2) + (pbox->x1 << 1);
	width = (pbox->x2 - pbox->x1) << 1;
	height = pbox->y2 - pbox->y1;

	while(height--) {
            for(i = 0; i < width; i+=2) {
		data[i << 1] = c;
		data2[i] = c2;
	    }
            data += pitch;
            data2 += pitch2;
	}
	pbox++;
    }
}

