/*
   Copyright (C) 1999.  The XFree86 Project Inc.

   Written by David S. Miller (davem@redhat.com)

   Based largely upon the xf8_16bpp module which is
   Mark Vojkovich's work.
*/

/* $XFree86: xc/programs/Xserver/hw/xfree86/xf8_32wid/cfbscrinit.c,v 1.1 2000/05/21 01:02:43 mvojkovi Exp $ */

#include "X.h"
#include "Xmd.h"
#include "misc.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "colormapst.h"
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb32.h"
#include "cfb8_32wid.h"
#include "mi.h"
#include "micmap.h"
#include "mistruct.h"
#include "gcstruct.h"
#include "dix.h"
#include "mibstore.h"
#include "xf86str.h"
#include "xf86.h"

/* CAUTION:  We require that cfb8 and cfb32 were NOT 
	compiled with CFB_NEED_SCREEN_PRIVATE */

int cfb8_32WidScreenPrivateIndex;

static unsigned long cfb8_32WidGeneration = 0;
extern WindowPtr *WindowTable;

static PixmapPtr cfb8_32WidGetWindowPixmap(WindowPtr pWin);

static void
cfb8_32WidSaveAreas(
    PixmapPtr	  	pPixmap,
    RegionPtr	  	prgnSave, 
    int	    	  	xorg,
    int	    	  	yorg,
    WindowPtr		pWin
);

static void
cfb8_32WidRestoreAreas(
    PixmapPtr	  	pPixmap, 
    RegionPtr	  	prgnRestore,
    int	    	  	xorg,
    int	    	  	yorg,
    WindowPtr		pWin
);

static BSFuncRec cfb8_32WidBSFuncRec = {
    cfb8_32WidSaveAreas,
    cfb8_32WidRestoreAreas,
    (BackingStoreSetClipmaskRgnProcPtr) 0,
    (BackingStoreGetImagePixmapProcPtr) 0,
    (BackingStoreGetSpansPixmapProcPtr) 0,
};

static void
cfb8_32WidGetSpans(
   DrawablePtr pDraw,
   int wMax,
   DDXPointPtr ppt,
   int *pwidth,
   int nspans,
   char *pchardstStart
);

static void
cfb8_32WidGetImage (
    DrawablePtr pDraw,
    int sx, int sy, int w, int h,
    unsigned int format,
    unsigned long planeMask,
    char *pdstLine
);

static Bool cfb8_32WidCreateGC(GCPtr pGC);
static void cfb8_32WidEnableDisableFBAccess(int index, Bool enable);


static Bool
cfb8_32WidAllocatePrivates(ScreenPtr pScreen)
{
	cfb8_32WidScreenPtr pScreenPriv;

	if (cfb8_32WidGeneration != serverGeneration) {
		if ((cfb8_32WidScreenPrivateIndex = AllocateScreenPrivateIndex()) < 0)
			return FALSE;
		cfb8_32WidGeneration = serverGeneration;
	}

	if (!(pScreenPriv = xalloc(sizeof(cfb8_32WidScreenRec))))
		return FALSE;

	pScreen->devPrivates[cfb8_32WidScreenPrivateIndex].ptr = (pointer)pScreenPriv;
   
	/* All cfb will have the same GC and Window private indicies */
	if (!mfbAllocatePrivates(pScreen, &cfbWindowPrivateIndex, &cfbGCPrivateIndex))
		return FALSE;

	/* The cfb indicies are the mfb indicies. Reallocating them resizes them */ 
	if (!AllocateWindowPrivate(pScreen, cfbWindowPrivateIndex, sizeof(cfbPrivWin)))
		return FALSE;

	if (!AllocateGCPrivate(pScreen, cfbGCPrivateIndex, sizeof(cfbPrivGC)))
		return FALSE;

	return TRUE;
}

static Bool
cfb8_32WidSetupScreen(
    ScreenPtr pScreen,
    int xsize, int ysize,	/* in pixels */
    int dpix, int dpiy
){
	if (!cfb8_32WidAllocatePrivates(pScreen))
		return FALSE;

	pScreen->defColormap = FakeClientID(0);

	/* let CreateDefColormap do whatever it wants for pixels */ 
	pScreen->blackPixel = pScreen->whitePixel = (Pixel) 0;
	pScreen->QueryBestSize = mfbQueryBestSize;

	/* SaveScreen */
	pScreen->GetImage = cfb8_32WidGetImage;
	pScreen->GetSpans = cfb8_32WidGetSpans;
	pScreen->CreateWindow = cfb8_32WidCreateWindow;
	pScreen->DestroyWindow = cfb8_32WidDestroyWindow;	
	pScreen->PositionWindow = cfb8_32WidPositionWindow;
	pScreen->ChangeWindowAttributes = cfb8_32WidChangeWindowAttributes;
	pScreen->RealizeWindow = cfb32MapWindow;		/* OK */
	pScreen->UnrealizeWindow = cfb32UnmapWindow;		/* OK */
	pScreen->PaintWindowBackground = cfb8_32WidPaintWindow;
	pScreen->PaintWindowBorder = cfb8_32WidPaintWindow;
	pScreen->CopyWindow = cfb8_32WidCopyWindow;
	pScreen->CreatePixmap = cfb32CreatePixmap;		/* OK */
	pScreen->DestroyPixmap = cfb32DestroyPixmap; 		/* OK */
	pScreen->RealizeFont = mfbRealizeFont;
	pScreen->UnrealizeFont = mfbUnrealizeFont;
	pScreen->CreateGC = cfb8_32WidCreateGC;
	pScreen->CreateColormap = miInitializeColormap;
	pScreen->DestroyColormap = (void (*)())NoopDDA;
	pScreen->InstallColormap = miInstallColormap;
	pScreen->UninstallColormap = miUninstallColormap;
	pScreen->ListInstalledColormaps = miListInstalledColormaps;
	pScreen->StoreColors = (void (*)())NoopDDA;
	pScreen->ResolveColor = miResolveColor;
	pScreen->BitmapToRegion = mfbPixmapToRegion;

	mfbRegisterCopyPlaneProc (pScreen, cfbCopyPlane);
	return TRUE;
}

static Bool
cfb8_32WidCreateScreenResources(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	cfb8_32WidScreenPtr pScreenPriv = CFB8_32WID_GET_SCREEN_PRIVATE(pScreen);
	PixmapPtr pix8, pix32, pixWid;
    
	xfree(pScreen->devPrivate); /* freeing miScreenInitParmsRec */

	pix8 = (*pScreen->CreatePixmap)(pScreen, 0, 0, 8);
	pix32 = (*pScreen->CreatePixmap)(pScreen, 0, 0, pScrn->depth);
	pixWid = (*pScreen->CreatePixmap)(pScreen, 0, 0, pScreenPriv->bitsPerWid);
	if (!pix32 || !pix8 || !pixWid)
		return FALSE;

	pix8->drawable.width = pScreen->width;
	pix8->drawable.height = pScreen->height;
	pix8->devKind = pScreenPriv->width8;
	pix8->devPrivate.ptr = pScreenPriv->pix8;

	pix32->drawable.width = pScreen->width;
	pix32->drawable.height = pScreen->height;
	pix32->devKind = pScreenPriv->width32 * 4;
	pix32->devPrivate.ptr = pScreenPriv->pix32;

	pixWid->drawable.width = pScreen->width;
	pixWid->drawable.height = pScreen->height;
	pixWid->devKind = (pScreenPriv->widthWid * pScreenPriv->bitsPerWid) / 8;
	pixWid->devPrivate.ptr = pScreenPriv->pixWid;

	pScreenPriv->pix8   = (pointer) pix8;
	pScreenPriv->pix32  = (pointer) pix32;
	pScreenPriv->pixWid = (pointer) pixWid;

	pScreen->devPrivate = (pointer) pix32;

	return TRUE;
}


static Bool
cfb8_32WidCloseScreen (int i, ScreenPtr pScreen)
{
	cfb8_32WidScreenPtr pScreenPriv = CFB8_32WID_GET_SCREEN_PRIVATE(pScreen);

	xfree((pointer) pScreenPriv);

	return cfb32CloseScreen(i, pScreen);
}

static Bool
cfb8_32WidFinishScreenInit(
    ScreenPtr pScreen,
    int xsize, int ysize,	/* in pixels */
    int dpix, int dpiy		/* dots per inch */
){
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	VisualPtr	visuals;
	DepthPtr	depths;
	int		nvisuals;
	int		ndepths;
	int		rootdepth;
	VisualID	defaultVisual;

	rootdepth = 0;
	if (!miInitVisuals(&visuals, &depths, &nvisuals, &ndepths, &rootdepth,
			    &defaultVisual,((unsigned long)1<<(32-1)), 8, -1))
		return FALSE;

	if (!miScreenInit(pScreen, NULL, xsize, ysize, dpix, dpiy, 0,
			  rootdepth, ndepths, depths,
			  defaultVisual, nvisuals, visuals))
		return FALSE;

	pScreen->BackingStoreFuncs = cfb8_32WidBSFuncRec;
	pScreen->CreateScreenResources = cfb8_32WidCreateScreenResources;
	pScreen->CloseScreen = cfb8_32WidCloseScreen;
	pScreen->GetWindowPixmap = cfb8_32WidGetWindowPixmap;
	pScreen->WindowExposures = cfb8_32WidWindowExposures;

	pScrn->EnableDisableFBAccess = cfb8_32WidEnableDisableFBAccess;

	return TRUE;
}

Bool
cfb8_32WidScreenInit(
    ScreenPtr pScreen,
    pointer pbits32,		/* pointer to screen bitmap */
    pointer pbits8,
    pointer pbitsWid,		/* pointer to WID bitmap */
    int xsize, int ysize,	/* in pixels */
    int dpix, int dpiy,		/* dots per inch */
    int width32,		/* pixel width of frame buffer */
    int width8,
    int widthWid,
    int bitsPerWid,
    cfb8_32WidOps *WIDOps
){
	cfb8_32WidScreenPtr pScreenPriv;

	if (!WIDOps || !WIDOps->WidGet || !WIDOps->WidAlloc || !WIDOps->WidFree)
		return FALSE;

	if (!cfb8_32WidSetupScreen(pScreen, xsize, ysize, dpix, dpiy))
		return FALSE;

	pScreenPriv = CFB8_32WID_GET_SCREEN_PRIVATE(pScreen);
	pScreenPriv->pix8 = pbits8;
	pScreenPriv->pix32 = pbits32;
	pScreenPriv->pixWid = pbitsWid;
	pScreenPriv->width8 = width8;
	pScreenPriv->width32 = width32;
	pScreenPriv->widthWid = widthWid;
	pScreenPriv->bitsPerWid = bitsPerWid;

	pScreenPriv->WIDOps = xalloc(sizeof(cfb8_32WidOps));
	if (!pScreenPriv->WIDOps)
		return FALSE;

	*(pScreenPriv->WIDOps) = *WIDOps;

	if (!WIDOps->WidFillBox || !WIDOps->WidCopyArea)
		cfb8_32WidGenericOpsInit(pScreenPriv);

	return cfb8_32WidFinishScreenInit(pScreen, xsize, ysize, dpix, dpiy);
}

static PixmapPtr
cfb8_32WidGetWindowPixmap(WindowPtr pWin)
{
	cfb8_32WidScreenPtr pScreenPriv = 
		CFB8_32WID_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);

	return ((pWin->drawable.bitsPerPixel == 8) ?
		(PixmapPtr) pScreenPriv->pix8 : (PixmapPtr) pScreenPriv->pix32);
}

static void
cfb8_32WidGetImage (DrawablePtr pDraw, int sx, int sy, int w, int h,
		    unsigned int format, unsigned long planemask, char *pdstLine)
{
	if (!w || !h)
		return;

	if (pDraw->bitsPerPixel == 8)
		cfbGetImage(pDraw, sx, sy, w, h, format, planemask, pdstLine);
	else
		cfb32GetImage(pDraw, sx, sy, w, h, format, planemask, pdstLine);
}

static void
cfb8_32WidGetSpans(DrawablePtr pDraw, int wMax, DDXPointPtr ppt, int *pwidth, int nspans, char *pDst)
{
	if (pDraw->bitsPerPixel == 8)
		cfbGetSpans(pDraw, wMax, ppt, pwidth, nspans, pDst);
	else
		cfb32GetSpans(pDraw, wMax, ppt, pwidth, nspans, pDst);
}

static void
cfb8_32WidSaveAreas(PixmapPtr pPixmap, RegionPtr prgnSave, int xorg, int yorg, WindowPtr pWin)
{
	if (pWin->drawable.bitsPerPixel == 8)
		cfbSaveAreas(pPixmap, prgnSave, xorg, yorg, pWin);
	else
		cfb32SaveAreas(pPixmap, prgnSave, xorg, yorg, pWin);
}

static void
cfb8_32WidRestoreAreas(PixmapPtr pPixmap, RegionPtr prgnRestore, int xorg, int yorg, WindowPtr pWin)
{
	if (pWin->drawable.bitsPerPixel == 8)
		cfbRestoreAreas(pPixmap, prgnRestore, xorg, yorg, pWin);
	else
		cfb32RestoreAreas(pPixmap, prgnRestore, xorg, yorg, pWin);
}


static Bool
cfb8_32WidCreateGC(GCPtr pGC)
{
	if (pGC->depth == 8)
		return cfbCreateGC(pGC);
	else
		return cfb32CreateGC(pGC);
}

static void
cfb8_32WidEnableDisableFBAccess(int index, Bool enable)
{
	ScreenPtr pScreen = xf86Screens[index]->pScreen;
	cfb8_32WidScreenPtr pScreenPriv = CFB8_32WID_GET_SCREEN_PRIVATE(pScreen);
	static DevUnion devPrivates8[MAXSCREENS];
	static DevUnion devPrivates32[MAXSCREENS];
	PixmapPtr	pix8, pix32;

	pix8 = (PixmapPtr) pScreenPriv->pix8;
	pix32 = (PixmapPtr) pScreenPriv->pix32;

	if (enable) {
		pix8->devPrivate = devPrivates8[index];
		pix32->devPrivate = devPrivates32[index];
	}

	xf86EnableDisableFBAccess (index, enable);

	if (!enable) {
		devPrivates8[index] = pix8->devPrivate;
		pix8->devPrivate.ptr = 0;
		devPrivates32[index] = pix32->devPrivate;
		pix32->devPrivate.ptr = 0;
	}
}
