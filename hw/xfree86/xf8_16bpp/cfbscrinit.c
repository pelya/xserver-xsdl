/*
   Copyright (C) 1999.  The XFree86 Project Inc.

   Written by Mark Vojkovich (mvojkovi@ucsd.edu)
*/

/* $XFree86: xc/programs/Xserver/hw/xfree86/xf8_16bpp/cfbscrinit.c,v 1.8 2003/02/17 16:08:30 dawes Exp $ */

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
#include "cfb16.h"
#include "cfb8_16.h"
#include "mi.h"
#include "micmap.h"
#include "mistruct.h"
#include "gcstruct.h"
#include "dix.h"
#include "mibstore.h"
#include "xf86str.h"
#include "xf86.h"

/* CAUTION:  We require that cfb8 and cfb16 were NOT 
	compiled with CFB_NEED_SCREEN_PRIVATE */

int cfb8_16ScreenPrivateIndex;

static unsigned long cfb8_16Generation = 0;

static PixmapPtr cfb8_16GetWindowPixmap(WindowPtr pWin);
static void
cfb8_16SaveAreas(
    PixmapPtr	  	pPixmap,
    RegionPtr	  	prgnSave, 
    int	    	  	xorg,
    int	    	  	yorg,
    WindowPtr		pWin
);

static void
cfb8_16RestoreAreas(
    PixmapPtr	  	pPixmap, 
    RegionPtr	  	prgnRestore,
    int	    	  	xorg,
    int	    	  	yorg,
    WindowPtr		pWin
);

static BSFuncRec cfb8_16BSFuncRec = {
    cfb8_16SaveAreas,
    cfb8_16RestoreAreas,
    (BackingStoreSetClipmaskRgnProcPtr) 0,
    (BackingStoreGetImagePixmapProcPtr) 0,
    (BackingStoreGetSpansPixmapProcPtr) 0,
};

static void
cfb8_16GetSpans(
   DrawablePtr pDraw,
   int wMax,
   DDXPointPtr ppt,
   int *pwidth,
   int nspans,
   char *pchardstStart
);

static void
cfb8_16GetImage (
    DrawablePtr pDraw,
    int sx, int sy, int w, int h,
    unsigned int format,
    unsigned long planeMask,
    char *pdstLine
);

static Bool cfb8_16CreateGC(GCPtr pGC);
static void cfb8_16EnableDisableFBAccess(int index, Bool enable);


static Bool
cfb8_16AllocatePrivates(ScreenPtr pScreen)
{
   cfb8_16ScreenPtr pScreenPriv;

   if(cfb8_16Generation != serverGeneration) {
	if((cfb8_16ScreenPrivateIndex = AllocateScreenPrivateIndex()) < 0)
	    return FALSE;
	cfb8_16Generation = serverGeneration;
   }

   if (!(pScreenPriv = xalloc(sizeof(cfb8_16ScreenRec))))
        return FALSE;

   pScreen->devPrivates[cfb8_16ScreenPrivateIndex].ptr = (pointer)pScreenPriv;
   
   
   /* All cfb will have the same GC and Window private indicies */
   if(!mfbAllocatePrivates(pScreen,&cfbWindowPrivateIndex, &cfbGCPrivateIndex))
	return FALSE;

   /* The cfb indicies are the mfb indicies. Reallocating them resizes them */ 
   if(!AllocateWindowPrivate(pScreen,cfbWindowPrivateIndex,sizeof(cfbPrivWin)))
	return FALSE;

   if(!AllocateGCPrivate(pScreen, cfbGCPrivateIndex, sizeof(cfbPrivGC)))
        return FALSE;

   return TRUE;
}

static Bool
cfb8_16SetupScreen(
    ScreenPtr pScreen,
    int xsize, int ysize,	/* in pixels */
    int dpix, int dpiy
){
    if (!cfb8_16AllocatePrivates(pScreen))
	return FALSE;
    pScreen->defColormap = FakeClientID(0);
    /* let CreateDefColormap do whatever it wants for pixels */ 
    pScreen->blackPixel = pScreen->whitePixel = (Pixel) 0;
    pScreen->QueryBestSize = mfbQueryBestSize;
    /* SaveScreen */
    pScreen->GetImage = cfb8_16GetImage;
    pScreen->GetSpans = cfb8_16GetSpans;
    pScreen->CreateWindow = cfb8_16CreateWindow;
    pScreen->DestroyWindow = cfb8_16DestroyWindow;	
    pScreen->PositionWindow = cfb8_16PositionWindow;
    pScreen->ChangeWindowAttributes = cfb8_16ChangeWindowAttributes;
    pScreen->RealizeWindow = cfb16MapWindow;			/* OK */
    pScreen->UnrealizeWindow = cfb16UnmapWindow;		/* OK */
    pScreen->PaintWindowBackground = cfb8_16PaintWindow;
    pScreen->PaintWindowBorder = cfb8_16PaintWindow;
    pScreen->CopyWindow = cfb8_16CopyWindow;
    pScreen->CreatePixmap = cfb16CreatePixmap;			/* OK */
    pScreen->DestroyPixmap = cfb16DestroyPixmap; 		/* OK */
    pScreen->RealizeFont = mfbRealizeFont;
    pScreen->UnrealizeFont = mfbUnrealizeFont;
    pScreen->CreateGC = cfb8_16CreateGC;
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
cfb8_16CreateScreenResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    cfb8_16ScreenPtr pScreenPriv = CFB8_16_GET_SCREEN_PRIVATE(pScreen);
    PixmapPtr pix8, pix16;
    
    xfree(pScreen->devPrivate); /* freeing miScreenInitParmsRec */

    pix8 = (*pScreen->CreatePixmap)(pScreen, 0, 0, 8);
    pix16 = (*pScreen->CreatePixmap)(pScreen, 0, 0, pScrn->depth);
    if(!pix16 || !pix8)
	return FALSE;

    pix8->drawable.width = pScreen->width;
    pix8->drawable.height = pScreen->height;
    pix8->devKind = pScreenPriv->width8;
    pix8->devPrivate.ptr = pScreenPriv->pix8;

    pix16->drawable.width = pScreen->width;
    pix16->drawable.height = pScreen->height;
    pix16->devKind = pScreenPriv->width16 * 2;
    pix16->devPrivate.ptr = pScreenPriv->pix16;

    pScreenPriv->pix8 = (pointer)pix8;
    pScreenPriv->pix16 = (pointer)pix16;

    pScreen->devPrivate = (pointer)pix16;

    return TRUE;
}


static Bool
cfb8_16CloseScreen (int i, ScreenPtr pScreen)
{
    cfb8_16ScreenPtr pScreenPriv = CFB8_16_GET_SCREEN_PRIVATE(pScreen);

    xfree((pointer) pScreenPriv);

    return(cfb16CloseScreen(i, pScreen));
}

static Bool
cfb8_16FinishScreenInit(
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
    if (!miInitVisuals (&visuals, &depths, &nvisuals, &ndepths, &rootdepth,
			 &defaultVisual,((unsigned long)1<<(16-1)), 8, -1))
	return FALSE;
    if (! miScreenInit(pScreen, NULL, xsize, ysize, dpix, dpiy, 0,
			rootdepth, ndepths, depths,
			defaultVisual, nvisuals, visuals))
	return FALSE;

    pScreen->BackingStoreFuncs = cfb8_16BSFuncRec;
    pScreen->CreateScreenResources = cfb8_16CreateScreenResources;
    pScreen->CloseScreen = cfb8_16CloseScreen;
    pScreen->GetWindowPixmap = cfb8_16GetWindowPixmap;
    pScreen->WindowExposures = cfb8_16WindowExposures;

    pScrn->EnableDisableFBAccess = cfb8_16EnableDisableFBAccess;

    return TRUE;
}

Bool
cfb8_16ScreenInit(
    ScreenPtr pScreen,
    pointer pbits16,		/* pointer to screen bitmap */
    pointer pbits8,
    int xsize, int ysize,	/* in pixels */
    int dpix, int dpiy,		/* dots per inch */
    int width16,		/* pixel width of frame buffer */
    int width8
){
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    cfb8_16ScreenPtr pScreenPriv;

    if (!cfb8_16SetupScreen(pScreen, xsize, ysize, dpix, dpiy))
	return FALSE;

    pScreenPriv = CFB8_16_GET_SCREEN_PRIVATE(pScreen);
    pScreenPriv->pix8 = pbits8;
    pScreenPriv->pix16 = pbits16;
    pScreenPriv->width8 = width8;
    pScreenPriv->width16 = width16;
    pScreenPriv->key = pScrn->colorKey;

    return cfb8_16FinishScreenInit(pScreen, xsize, ysize, dpix, dpiy);
}


static PixmapPtr
cfb8_16GetWindowPixmap(WindowPtr pWin)
{
    cfb8_16ScreenPtr pScreenPriv = 
		CFB8_16_GET_SCREEN_PRIVATE(pWin->drawable.pScreen);

    return((pWin->drawable.bitsPerPixel == 16) ?
	(PixmapPtr)pScreenPriv->pix16 : (PixmapPtr)pScreenPriv->pix8);
}

static void
cfb8_16GetImage (
    DrawablePtr pDraw,
    int sx, int sy, int w, int h,
    unsigned int format,
    unsigned long planemask,
    char *pdstLine
){
    if(!w || !h) return;

    if(pDraw->bitsPerPixel == 16)
	cfb16GetImage(pDraw, sx, sy, w, h, format, planemask, pdstLine);
    else
	cfbGetImage(pDraw, sx, sy, w, h, format, planemask, pdstLine);
}

static void
cfb8_16GetSpans(
   DrawablePtr pDraw,
   int wMax,
   DDXPointPtr ppt,
   int *pwidth,
   int nspans,
   char *pDst
){
    if(pDraw->bitsPerPixel == 16)
	cfb16GetSpans(pDraw, wMax, ppt, pwidth, nspans, pDst);
    else
	cfbGetSpans(pDraw, wMax, ppt, pwidth, nspans, pDst);
}

static void
cfb8_16SaveAreas(
    PixmapPtr	  	pPixmap,
    RegionPtr	  	prgnSave, 
    int	    	  	xorg,
    int	    	  	yorg,
    WindowPtr		pWin
){
    if(pWin->drawable.bitsPerPixel == 16)
        cfb16SaveAreas(pPixmap, prgnSave, xorg, yorg, pWin);
    else
        cfbSaveAreas(pPixmap, prgnSave, xorg, yorg, pWin);
}

static void
cfb8_16RestoreAreas(
    PixmapPtr	  	pPixmap, 
    RegionPtr	  	prgnRestore,
    int	    	  	xorg,
    int	    	  	yorg,
    WindowPtr		pWin
){
    if(pWin->drawable.bitsPerPixel == 16)
        cfb16RestoreAreas(pPixmap, prgnRestore, xorg, yorg, pWin);
    else
        cfbRestoreAreas(pPixmap, prgnRestore, xorg, yorg, pWin);
}


static Bool
cfb8_16CreateGC(GCPtr pGC)
{
    if(pGC->depth == 8)
	return(cfbCreateGC(pGC));
    else
	return(cfb16CreateGC(pGC));
}

static void
cfb8_16EnableDisableFBAccess(int index, Bool enable)
{
    ScreenPtr pScreen = xf86Screens[index]->pScreen;
    cfb8_16ScreenPtr pScreenPriv = CFB8_16_GET_SCREEN_PRIVATE(pScreen);
    static DevUnion devPrivates8[MAXSCREENS];
    static DevUnion devPrivates16[MAXSCREENS];
    PixmapPtr	pix8, pix16;

    pix8 = (PixmapPtr) pScreenPriv->pix8;
    pix16 = (PixmapPtr) pScreenPriv->pix16;

    if (enable)
    {
	pix8->devPrivate = devPrivates8[index];
	pix16->devPrivate = devPrivates16[index];
    }
    xf86EnableDisableFBAccess (index, enable);
    if (!enable)
    {
	devPrivates8[index] = pix8->devPrivate;
	pix8->devPrivate.ptr = 0;
	devPrivates16[index] = pix16->devPrivate;
	pix16->devPrivate.ptr = 0;
    }
}
