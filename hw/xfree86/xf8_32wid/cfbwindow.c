/*
   Copyright (C) 1999.  The XFree86 Project Inc.

   Written by David S. Miller (davem@redhat.com)

   Based largely upon the xf8_16bpp module which is
   Mark Vojkovich's work.
*/

/* $XFree86: xc/programs/Xserver/hw/xfree86/xf8_32wid/cfbwindow.c,v 1.2 2001/10/28 03:34:09 tsi Exp $ */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb32.h"
#include "cfb8_32wid.h"
#include "mistruct.h"
#include "regionstr.h"
#include "cfbmskbits.h"
#include "xf86.h"

/* We don't bother with cfb's fastBackground/Border so we don't
   need to use the Window privates */

Bool
cfb8_32WidCreateWindow(WindowPtr pWin)
{
	ScreenPtr pScreen = pWin->drawable.pScreen;
	cfb8_32WidScreenPtr pScreenPriv = 
		CFB8_32WID_GET_SCREEN_PRIVATE(pScreen);
	cfbPrivWin *pPrivWin = cfbGetWindowPrivate(pWin);

	pPrivWin->fastBackground = FALSE;
	pPrivWin->fastBorder = FALSE;

	if (!pScreenPriv->WIDOps->WidAlloc(pWin))
		return FALSE;

	return TRUE;
}


Bool
cfb8_32WidDestroyWindow(WindowPtr pWin)
{
	ScreenPtr pScreen = pWin->drawable.pScreen;
	cfb8_32WidScreenPtr pScreenPriv = 
		CFB8_32WID_GET_SCREEN_PRIVATE(pScreen);

	pScreenPriv->WIDOps->WidFree(pWin);
	return TRUE;
}

Bool
cfb8_32WidPositionWindow(WindowPtr pWin, int x, int y)
{
	return TRUE;
}

static void
SegregateChildrenBpp(WindowPtr pWin, RegionPtr pReg, int subtract, int bpp, int other_bpp)
{
	WindowPtr pChild;

	for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
		if (pChild->drawable.bitsPerPixel == bpp) {
			if (subtract) {
				REGION_SUBTRACT(pWin->drawable.pScreen, pReg,
						pReg, &pChild->borderClip);
			} else {
				REGION_UNION(pWin->drawable.pScreen, pReg,
					     pReg, &pChild->borderClip);
			}
			if (pChild->firstChild)
				SegregateChildrenBpp(pChild, pReg,
						     !subtract, other_bpp, bpp);
		} else {
			if (pChild->firstChild)
				SegregateChildrenBpp(pChild, pReg,
						     subtract, bpp, other_bpp);
		}
	}
}

void
cfb8_32WidCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
	ScreenPtr pScreen = pWin->drawable.pScreen;
	cfb8_32WidScreenPtr pScreenPriv = 
		CFB8_32WID_GET_SCREEN_PRIVATE(pScreen);
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	PixmapPtr pPixChildren;
	DDXPointPtr ppt, pptSrc;
	RegionRec rgnDst, rgnOther, rgnPixmap;
	BoxPtr pbox;
	int i, nbox, dx, dy, other_bpp;

	REGION_INIT(pScreen, &rgnDst, NullBox, 0);

	dx = ptOldOrg.x - pWin->drawable.x;
	dy = ptOldOrg.y - pWin->drawable.y;
	REGION_TRANSLATE(pScreen, prgnSrc, -dx, -dy);
	REGION_INTERSECT(pScreen, &rgnDst, &pWin->borderClip, prgnSrc);

	if ((nbox = REGION_NUM_RECTS(&rgnDst)) == 0) {
		/* Nothing to render. */
		REGION_UNINIT(pScreen, &rgnDst);
		return;
	}

	/* First, copy the WID plane for the whole area. */
	pptSrc = (DDXPointPtr )ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec));
	if(pptSrc) {
		pbox = REGION_RECTS(&rgnDst);
		for (i = nbox, ppt = pptSrc; i--; ppt++, pbox++) {
			ppt->x = pbox->x1 + dx;
			ppt->y = pbox->y1 + dy;
		}

		pScreenPriv->WIDOps->WidCopyArea((DrawablePtr)pScreenPriv->pixWid,
						 &rgnDst, pptSrc);

		DEALLOCATE_LOCAL(pptSrc);
	}

	/* Next, we copy children which have a different
	 * bpp than pWin into a temporary pixmap.  We will
	 * toss this pixmap back onto the framebuffer before
	 * we return.
	 */
	if (pWin->drawable.bitsPerPixel == 8)
		other_bpp = pScrn->bitsPerPixel;
	else
		other_bpp = 8;

	REGION_INIT(pScreen, &rgnOther, NullBox, 0);
	SegregateChildrenBpp(pWin, &rgnOther, 0,
			     other_bpp, pWin->drawable.bitsPerPixel);
	pPixChildren = NULL;
	if (REGION_NOTEMPTY(pScreen, &rgnOther)) {
		REGION_INTERSECT(pScreen, &rgnOther, &rgnOther, prgnSrc);
		nbox = REGION_NUM_RECTS(&rgnOther);
		if (nbox) {
			int width = rgnOther.extents.x2 - rgnOther.extents.x1;
			int height = rgnOther.extents.y2 - rgnOther.extents.y1;
			int depth = (other_bpp == 8) ? 8 : pScrn->depth;

			if (other_bpp == 8)
				pPixChildren = cfbCreatePixmap(pScreen, width, height, depth);
			else
				pPixChildren = cfb32CreatePixmap(pScreen, width, height, depth);
		}
		if (nbox &&
		    pPixChildren &&
		    (pptSrc = (DDXPointPtr) ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec)))) {
			pbox = REGION_RECTS(&rgnOther);
			for (i = nbox, ppt = pptSrc; i--; ppt++, pbox++) {
				ppt->x = pbox->x1 + dx;
				ppt->y = pbox->y1 + dy;
			}

			REGION_INIT(pScreen, &rgnPixmap, NullBox, 0);
			REGION_COPY(pScreen, &rgnPixmap, &rgnOther);
			REGION_TRANSLATE(pScreen, &rgnPixmap, -(rgnOther.extents.x1), -(rgnOther.extents.y1));

			if (other_bpp == 8)
				cfbDoBitbltCopy((DrawablePtr)pScreenPriv->pix8,
						(DrawablePtr)pPixChildren,
						GXcopy, &rgnPixmap, pptSrc, ~0L);
			else
				cfb32DoBitbltCopy((DrawablePtr)pScreenPriv->pix32,
						  (DrawablePtr)pPixChildren,
						  GXcopy, &rgnPixmap, pptSrc, ~0L);

			REGION_UNINIT(pScreen, &rgnPixmap);

			DEALLOCATE_LOCAL(pptSrc);
		}

		REGION_SUBTRACT(pScreen, &rgnDst, &rgnDst, &rgnOther);
	}

	/* Now copy the parent along with all child windows using the same depth. */
	nbox = REGION_NUM_RECTS(&rgnDst);
	if(nbox &&
	   (pptSrc = (DDXPointPtr )ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec)))) {
		pbox = REGION_RECTS(&rgnDst);
		for (i = nbox, ppt = pptSrc; i--; ppt++, pbox++) {
			ppt->x = pbox->x1 + dx;
			ppt->y = pbox->y1 + dy;
		}

		if (pWin->drawable.bitsPerPixel == 8)
			cfbDoBitbltCopy((DrawablePtr)pScreenPriv->pix8,
					(DrawablePtr)pScreenPriv->pix8,
					GXcopy, &rgnDst, pptSrc, ~0L);
		else
			cfb32DoBitbltCopy((DrawablePtr)pScreenPriv->pix32,
					  (DrawablePtr)pScreenPriv->pix32,
					  GXcopy, &rgnDst, pptSrc, ~0L);

		DEALLOCATE_LOCAL(pptSrc);
	}

	REGION_UNINIT(pScreen, &rgnDst);

	if (pPixChildren) {
		nbox = REGION_NUM_RECTS(&rgnOther);
		pptSrc = (DDXPointPtr) ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec));
		if (pptSrc) {
			pbox = REGION_RECTS(&rgnOther);
			for (i = nbox, ppt = pptSrc; i--; ppt++, pbox++) {
				ppt->x = pbox->x1 - rgnOther.extents.x1;
				ppt->y = pbox->y1 - rgnOther.extents.y1;
			}

			if (other_bpp == 8)
				cfbDoBitbltCopy((DrawablePtr)pPixChildren,
						(DrawablePtr)pScreenPriv->pix8,
						GXcopy, &rgnOther, pptSrc, ~0L);
			else
				cfb32DoBitbltCopy((DrawablePtr)pPixChildren,
						  (DrawablePtr)pScreenPriv->pix32,
						  GXcopy, &rgnOther, pptSrc, ~0L);

			DEALLOCATE_LOCAL(pptSrc);
		}

		if (other_bpp == 8)
			cfbDestroyPixmap(pPixChildren);
		else
			cfb32DestroyPixmap(pPixChildren);
	}
	REGION_UNINIT(pScreen, &rgnOther);
}

Bool
cfb8_32WidChangeWindowAttributes(WindowPtr pWin, unsigned long mask)
{
	return TRUE;
}

void
cfb8_32WidWindowExposures(WindowPtr pWin, RegionPtr pReg, RegionPtr pOtherReg)
{
	/* Fill in the WID channel before rendering of
	 * the exposed window area.
	 */
	if (REGION_NUM_RECTS(pReg)) {
		ScreenPtr pScreen = pWin->drawable.pScreen;
		cfb8_32WidScreenPtr pScreenPriv = 
			CFB8_32WID_GET_SCREEN_PRIVATE(pScreen);

		pScreenPriv->WIDOps->WidFillBox((DrawablePtr)pScreenPriv->pixWid,
						(DrawablePtr)pWin,
						REGION_NUM_RECTS(pReg), REGION_RECTS(pReg));
	}

	miWindowExposures(pWin, pReg, pOtherReg);
}

void
cfb8_32WidPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what)
{
	if (what == PW_BORDER) {
		ScreenPtr pScreen = pWin->drawable.pScreen;
		cfb8_32WidScreenPtr pScreenPriv = 
			CFB8_32WID_GET_SCREEN_PRIVATE(pScreen);

		pScreenPriv->WIDOps->WidFillBox((DrawablePtr)pScreenPriv->pixWid,
						(DrawablePtr)pWin,
						REGION_NUM_RECTS(pRegion),
						REGION_RECTS(pRegion));
	}

	if (pWin->drawable.bitsPerPixel == 8)
		cfbPaintWindow(pWin, pRegion, what);
	else
		cfb32PaintWindow(pWin, pRegion, what);
}

