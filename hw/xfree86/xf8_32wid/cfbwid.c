/* $XFree86: xc/programs/Xserver/hw/xfree86/xf8_32wid/cfbwid.c,v 1.1 2000/05/21 01:02:44 mvojkovi Exp $ */

#include "X.h"
#include "Xmd.h"
#include "misc.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "colormapst.h"

#include "mfb.h"
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"

#include "cfb8_32wid.h"

static void
WidFillBox1(DrawablePtr pixWid, DrawablePtr pWinDraw, int nbox, BoxPtr pBox)
{
	WindowPtr pWin = (WindowPtr) pWinDraw;
	cfb8_32WidScreenPtr pScreenPriv = 
		CFB8_32WID_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);
	unsigned int wid = pScreenPriv->WIDOps->WidGet(pWin);

	if (wid & 1)
		mfbSolidWhiteArea((DrawablePtr)pWin, nbox, pBox, GXset, NullPixmap);
	else
		mfbSolidBlackArea((DrawablePtr)pWin, nbox, pBox, GXset, NullPixmap);
}

static void
WidCopyArea1(DrawablePtr pixWid, RegionPtr pRgn, DDXPointPtr pptSrc)
{
	mfbDoBitbltCopy(pixWid, pixWid, GXcopy, pRgn, pptSrc);
}

static void
WidFillBox8(DrawablePtr pixWid, DrawablePtr pWinDraw, int nbox, BoxPtr pBox)
{
	WindowPtr pWin = (WindowPtr) pWinDraw;
	cfb8_32WidScreenPtr pScreenPriv = 
		CFB8_32WID_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);
	unsigned int wid = pScreenPriv->WIDOps->WidGet(pWin);

	cfbFillBoxSolid(pixWid, nbox, pBox, wid);
}

static void
WidCopyArea8(DrawablePtr pixWid, RegionPtr pRgn, DDXPointPtr pptSrc)
{
	cfbDoBitbltCopy(pixWid, pixWid, GXcopy, pRgn, pptSrc, ~0L);
}

static void
WidFillBox16(DrawablePtr pixWid, DrawablePtr pWinDraw, int nbox, BoxPtr pBox)
{
	WindowPtr pWin = (WindowPtr) pWinDraw;
	cfb8_32WidScreenPtr pScreenPriv = 
		CFB8_32WID_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);
	unsigned int wid = pScreenPriv->WIDOps->WidGet(pWin);

	cfb16FillBoxSolid(pixWid, nbox, pBox, wid);
}

static void
WidCopyArea16(DrawablePtr pixWid, RegionPtr pRgn, DDXPointPtr pptSrc)
{
	cfb16DoBitbltCopy(pixWid, pixWid, GXcopy, pRgn, pptSrc, ~0L);
}

static void
WidFillBox24(DrawablePtr pixWid, DrawablePtr pWinDraw, int nbox, BoxPtr pBox)
{
	WindowPtr pWin = (WindowPtr) pWinDraw;
	cfb8_32WidScreenPtr pScreenPriv = 
		CFB8_32WID_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);
	unsigned int wid = pScreenPriv->WIDOps->WidGet(pWin);

	cfb24FillBoxSolid(pixWid, nbox, pBox, wid);
}

static void
WidCopyArea24(DrawablePtr pixWid, RegionPtr pRgn, DDXPointPtr pptSrc)
{
	cfb24DoBitbltCopy(pixWid, pixWid, GXcopy, pRgn, pptSrc, ~0L);
}

static void
WidFillBox32(DrawablePtr pixWid, DrawablePtr pWinDraw, int nbox, BoxPtr pBox)
{
	WindowPtr pWin = (WindowPtr) pWinDraw;
	cfb8_32WidScreenPtr pScreenPriv = 
		CFB8_32WID_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);
	unsigned int wid = pScreenPriv->WIDOps->WidGet(pWin);

	cfb32FillBoxSolid(pixWid, nbox, pBox, wid);
}

static void
WidCopyArea32(DrawablePtr pixWid, RegionPtr pRgn, DDXPointPtr pptSrc)
{
	cfb32DoBitbltCopy(pixWid, pixWid, GXcopy, pRgn, pptSrc, ~0L);
}

Bool
cfb8_32WidGenericOpsInit(cfb8_32WidScreenPtr pScreenPriv)
{
	cfb8_32WidOps *WIDOps = pScreenPriv->WIDOps;

	switch (pScreenPriv->bitsPerWid) {
	case 1:
		WIDOps->WidFillBox = WidFillBox1;
		WIDOps->WidCopyArea = WidCopyArea1;
		break;

	case 8:
		WIDOps->WidFillBox = WidFillBox8;
		WIDOps->WidCopyArea = WidCopyArea8;
		break;

	case 16:
		WIDOps->WidFillBox = WidFillBox16;
		WIDOps->WidCopyArea = WidCopyArea16;
		break;

	case 24:
		WIDOps->WidFillBox = WidFillBox24;
		WIDOps->WidCopyArea = WidCopyArea24;
		break;

	case 32:
		WIDOps->WidFillBox = WidFillBox32;
		WIDOps->WidCopyArea = WidCopyArea32;
		break;

	default:
		return FALSE;
	};

	return TRUE;
}
