/*
 * misprite.c
 *
 * machine independent software sprite routines
 */


/*

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
*/
#ifdef MPX
 /* 
  * MPX additions:
  * Copyright © 2006 Peter Hutterer
  * License see above.
  * Author: Peter Hutterer <peter@cs.unisa.edu.au>
  *
  */
#endif

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

# include   <X11/X.h>
# include   <X11/Xproto.h>
# include   "misc.h"
# include   "pixmapstr.h"
# include   "input.h"
# include   "mi.h"
# include   "cursorstr.h"
# include   <X11/fonts/font.h>
# include   "scrnintstr.h"
# include   "colormapst.h"
# include   "windowstr.h"
# include   "gcstruct.h"
# include   "mipointer.h"
# include   "mispritest.h"
# include   "dixfontstr.h"
# include   <X11/fonts/fontstruct.h>

#ifdef RENDER
# include   "mipict.h"
#endif
# include   "damage.h"

#ifdef MPX
# include   "inputstr.h" /* for MAX_DEVICES */
#endif

#define SPRITE_DEBUG_ENABLE 0
#if SPRITE_DEBUG_ENABLE
#define SPRITE_DEBUG(x)	ErrorF x
#else
#define SPRITE_DEBUG(x)
#endif

/*
 * screen wrappers
 */

static int  miSpriteScreenIndex;
static unsigned long miSpriteGeneration = 0;

static Bool	    miSpriteCloseScreen(int i, ScreenPtr pScreen);
static void	    miSpriteGetImage(DrawablePtr pDrawable, int sx, int sy,
				     int w, int h, unsigned int format,
				     unsigned long planemask, char *pdstLine);
static void	    miSpriteGetSpans(DrawablePtr pDrawable, int wMax,
				     DDXPointPtr ppt, int *pwidth, int nspans,
				     char *pdstStart);
static void	    miSpriteSourceValidate(DrawablePtr pDrawable, int x, int y,
					   int width, int height);
static void	    miSpriteCopyWindow (WindowPtr pWindow,
					DDXPointRec ptOldOrg,
					RegionPtr prgnSrc);
static void	    miSpriteBlockHandler(int i, pointer blockData,
					 pointer pTimeout,
					 pointer pReadMask);
static void	    miSpriteInstallColormap(ColormapPtr pMap);
static void	    miSpriteStoreColors(ColormapPtr pMap, int ndef,
					xColorItem *pdef);

static void	    miSpriteSaveDoomedAreas(WindowPtr pWin,
					    RegionPtr pObscured, int dx,
					    int dy);
static void	    miSpriteComputeSaved(ScreenPtr pScreen);

#define SCREEN_PROLOGUE(pScreen, field)\
  ((pScreen)->field = \
   ((miSpriteScreenPtr) (pScreen)->devPrivates[miSpriteScreenIndex].ptr)->field)

#define SCREEN_EPILOGUE(pScreen, field)\
    ((pScreen)->field = miSprite##field)

/*
 * pointer-sprite method table
 */

static Bool miSpriteRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                  CursorPtr pCursor); 
static Bool miSpriteUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, 
                                    CursorPtr pCursor);
static void miSpriteSetCursor(DeviceIntPtr pDev, ScreenPtr pScreen, 
                              CursorPtr pCursor, int x, int y);
static void miSpriteMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen, 
                               int x, int y);

_X_EXPORT miPointerSpriteFuncRec miSpritePointerFuncs = {
    miSpriteRealizeCursor,
    miSpriteUnrealizeCursor,
    miSpriteSetCursor,
    miSpriteMoveCursor,
};

/*
 * other misc functions
 */

static void miSpriteRemoveCursor(ScreenPtr pScreen);
static void miSpriteRestoreCursor(ScreenPtr pScreen);

static void
miSpriteReportDamage (DamagePtr pDamage, RegionPtr pRegion, void *closure)
{
    ScreenPtr		    pScreen = closure;
    miSpriteScreenPtr	    pScreenPriv;
    
    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;
    
    if (pScreenPriv->cp->isUp &&
	RECT_IN_REGION (pScreen, pRegion, &pScreenPriv->cp->saved) != rgnOUT)
    {
	SPRITE_DEBUG(("Damage remove\n"));
	miSpriteRemoveCursor (pScreen);
    }
}

/*
 * miSpriteInitialize -- called from device-dependent screen
 * initialization proc after all of the function pointers have
 * been stored in the screen structure.
 */

Bool
miSpriteInitialize (pScreen, cursorFuncs, screenFuncs)
    ScreenPtr		    pScreen;
    miSpriteCursorFuncPtr   cursorFuncs;
    miPointerScreenFuncPtr  screenFuncs;
{
#ifdef MPX
    int mpCursorIdx = 0;
#endif
    miSpriteScreenPtr	pScreenPriv;
    VisualPtr		pVisual;
    
    if (!DamageSetup (pScreen))
	return FALSE;

    if (miSpriteGeneration != serverGeneration)
    {
	miSpriteScreenIndex = AllocateScreenPrivateIndex ();
	if (miSpriteScreenIndex < 0)
	    return FALSE;
	miSpriteGeneration = serverGeneration;
    }
    
    pScreenPriv = (miSpriteScreenPtr) xalloc (sizeof (miSpriteScreenRec));
    if (!pScreenPriv)
	return FALSE;
    
    pScreenPriv->pDamage = DamageCreate (miSpriteReportDamage,
					 (DamageDestroyFunc) 0,
					 DamageReportRawRegion,
					 TRUE,
					 pScreen,
					 (void *) pScreen);

    if (!miPointerInitialize (pScreen, &miSpritePointerFuncs, screenFuncs,TRUE))
    {
	xfree ((pointer) pScreenPriv);
	return FALSE;
    }
    for (pVisual = pScreen->visuals;
	 pVisual->vid != pScreen->rootVisual;
	 pVisual++)
	;
    pScreenPriv->pVisual = pVisual;
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreenPriv->GetImage = pScreen->GetImage;
    pScreenPriv->GetSpans = pScreen->GetSpans;
    pScreenPriv->SourceValidate = pScreen->SourceValidate;

    pScreenPriv->CopyWindow = pScreen->CopyWindow;
    
    pScreenPriv->SaveDoomedAreas = pScreen->SaveDoomedAreas;
    
    pScreenPriv->InstallColormap = pScreen->InstallColormap;
    pScreenPriv->StoreColors = pScreen->StoreColors;
    
    pScreenPriv->BlockHandler = pScreen->BlockHandler;
    
    pScreenPriv->cp = (miCursorInfoPtr)xalloc(sizeof(miCursorInfoRec));
    pScreenPriv->cp->pCursor = NULL;
    pScreenPriv->cp->x = 0;
    pScreenPriv->cp->y = 0;
    pScreenPriv->cp->isUp = FALSE;
    pScreenPriv->cp->shouldBeUp = FALSE;
    pScreenPriv->cp->pCacheWin = NullWindow;
    pScreenPriv->cp->isInCacheWin = FALSE;
    pScreenPriv->cp->checkPixels = TRUE;
    pScreenPriv->cp->pInstalledMap = NULL;
    pScreenPriv->cp->pColormap = NULL;
    pScreenPriv->cp->colors[SOURCE_COLOR].red = 0;
    pScreenPriv->cp->colors[SOURCE_COLOR].green = 0;
    pScreenPriv->cp->colors[SOURCE_COLOR].blue = 0;
    pScreenPriv->cp->colors[MASK_COLOR].red = 0;
    pScreenPriv->cp->colors[MASK_COLOR].green = 0;
    pScreenPriv->cp->colors[MASK_COLOR].blue = 0;

    pScreenPriv->funcs = cursorFuncs;
    pScreen->devPrivates[miSpriteScreenIndex].ptr = (pointer) pScreenPriv;
    
    pScreen->CloseScreen = miSpriteCloseScreen;
    pScreen->GetImage = miSpriteGetImage;
    pScreen->GetSpans = miSpriteGetSpans;
    pScreen->SourceValidate = miSpriteSourceValidate;
    
    pScreen->CopyWindow = miSpriteCopyWindow;
    
    pScreen->SaveDoomedAreas = miSpriteSaveDoomedAreas;
    
    pScreen->InstallColormap = miSpriteInstallColormap;
    pScreen->StoreColors = miSpriteStoreColors;

    pScreen->BlockHandler = miSpriteBlockHandler;
    
#ifdef MPX
    /* alloc and zero memory for all MPX cursors */
    pScreenPriv->mpCursors = (miCursorInfoPtr)xalloc(MAX_DEVICES * sizeof(miCursorInfoRec));
    while (mpCursorIdx < MAX_DEVICES)
    {
        miCursorInfoPtr cursor = &(pScreenPriv->mpCursors[mpCursorIdx]);

        cursor->pCursor = NULL;
        cursor->x = 0;
        cursor->y = 0;
        cursor->isUp = FALSE;
        cursor->shouldBeUp = FALSE;
        cursor->pCacheWin = NullWindow;
        cursor->isInCacheWin = FALSE;
        cursor->checkPixels = TRUE;
        cursor->pInstalledMap = NULL;
        cursor->pColormap = NULL;
        cursor->colors[SOURCE_COLOR].red = 0;
        cursor->colors[SOURCE_COLOR].green = 0;
        cursor->colors[SOURCE_COLOR].blue = 0;
        cursor->colors[MASK_COLOR].red = 0;
        cursor->colors[MASK_COLOR].green = 0;
        cursor->colors[MASK_COLOR].blue = 0;

        mpCursorIdx++;
    }
#endif

    return TRUE;
}

/*
 * Screen wrappers
 */

/*
 * CloseScreen wrapper -- unwrap everything, free the private data
 * and call the wrapped function
 */

static Bool
miSpriteCloseScreen (i, pScreen)
    int i;
    ScreenPtr	pScreen;
{
    miSpriteScreenPtr   pScreenPriv;

    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;

    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    pScreen->GetImage = pScreenPriv->GetImage;
    pScreen->GetSpans = pScreenPriv->GetSpans;
    pScreen->SourceValidate = pScreenPriv->SourceValidate;
    pScreen->BlockHandler = pScreenPriv->BlockHandler;
    pScreen->InstallColormap = pScreenPriv->InstallColormap;
    pScreen->StoreColors = pScreenPriv->StoreColors;

    pScreen->SaveDoomedAreas = pScreenPriv->SaveDoomedAreas;
    miSpriteIsUpFALSE (pScreen, pScreenPriv);
    DamageDestroy (pScreenPriv->pDamage);
    
    xfree ((pointer) pScreenPriv);

    return (*pScreen->CloseScreen) (i, pScreen);
}

static void
miSpriteGetImage (pDrawable, sx, sy, w, h, format, planemask, pdstLine)
    DrawablePtr	    pDrawable;
    int		    sx, sy, w, h;
    unsigned int    format;
    unsigned long   planemask;
    char	    *pdstLine;
{
    ScreenPtr	    pScreen = pDrawable->pScreen;
    miSpriteScreenPtr    pScreenPriv;
    
    SCREEN_PROLOGUE (pScreen, GetImage);

    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;

    if (pDrawable->type == DRAWABLE_WINDOW &&
        pScreenPriv->cp->isUp &&
        ORG_OVERLAP(&pScreenPriv->cp->saved,pDrawable->x,pDrawable->y, 
                        sx, sy, w, h)) 
    {
	SPRITE_DEBUG (("GetImage remove\n"));
	miSpriteRemoveCursor (pScreen);
    }

    (*pScreen->GetImage) (pDrawable, sx, sy, w, h,
			  format, planemask, pdstLine);

    SCREEN_EPILOGUE (pScreen, GetImage);
}

static void
miSpriteGetSpans (pDrawable, wMax, ppt, pwidth, nspans, pdstStart)
    DrawablePtr	pDrawable;
    int		wMax;
    DDXPointPtr	ppt;
    int		*pwidth;
    int		nspans;
    char	*pdstStart;
{
    ScreenPtr		    pScreen = pDrawable->pScreen;
    miSpriteScreenPtr	    pScreenPriv;
    
    SCREEN_PROLOGUE (pScreen, GetSpans);

    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;

    if (pDrawable->type == DRAWABLE_WINDOW && pScreenPriv->cp->isUp)
    {
	register DDXPointPtr    pts;
	register int    	*widths;
	register int    	nPts;
	register int    	xorg,
				yorg;

	xorg = pDrawable->x;
	yorg = pDrawable->y;

	for (pts = ppt, widths = pwidth, nPts = nspans;
	     nPts--;
	     pts++, widths++)
 	{
	    if (SPN_OVERLAP(&pScreenPriv->cp->saved,pts->y+yorg,
			     pts->x+xorg,*widths))
	    {
		SPRITE_DEBUG (("GetSpans remove\n"));
		miSpriteRemoveCursor (pScreen);
		break;
	    }
	}
    }

    (*pScreen->GetSpans) (pDrawable, wMax, ppt, pwidth, nspans, pdstStart);

    SCREEN_EPILOGUE (pScreen, GetSpans);
}

static void
miSpriteSourceValidate (pDrawable, x, y, width, height)
    DrawablePtr	pDrawable;
    int		x, y, width, height;
{
    ScreenPtr		    pScreen = pDrawable->pScreen;
    miSpriteScreenPtr	    pScreenPriv;
    
    SCREEN_PROLOGUE (pScreen, SourceValidate);

    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;

    if (pDrawable->type == DRAWABLE_WINDOW && pScreenPriv->cp->isUp &&
	ORG_OVERLAP(&pScreenPriv->cp->saved, pDrawable->x, pDrawable->y,
		    x, y, width, height))
    {
	SPRITE_DEBUG (("SourceValidate remove\n"));
	miSpriteRemoveCursor (pScreen);
    }

    if (pScreen->SourceValidate)
	(*pScreen->SourceValidate) (pDrawable, x, y, width, height);

    SCREEN_EPILOGUE (pScreen, SourceValidate);
}

static void
miSpriteCopyWindow (WindowPtr pWindow, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr	pScreen = pWindow->drawable.pScreen;
    miSpriteScreenPtr	    pScreenPriv;
    
    SCREEN_PROLOGUE (pScreen, CopyWindow);

    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;
    /*
     * Damage will take care of destination check
     */
    if (pScreenPriv->cp->isUp &&
	RECT_IN_REGION (pScreen, prgnSrc, &pScreenPriv->cp->saved) != rgnOUT)
    {
	SPRITE_DEBUG (("CopyWindow remove\n"));
	miSpriteRemoveCursor (pScreen);
    }

    (*pScreen->CopyWindow) (pWindow, ptOldOrg, prgnSrc);
    SCREEN_EPILOGUE (pScreen, CopyWindow);
}

static void
miSpriteBlockHandler (i, blockData, pTimeout, pReadmask)
    int	i;
    pointer	blockData;
    pointer	pTimeout;
    pointer	pReadmask;
{
    ScreenPtr		pScreen = screenInfo.screens[i];
    miSpriteScreenPtr	pPriv;

    pPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;

    SCREEN_PROLOGUE(pScreen, BlockHandler);
    
    (*pScreen->BlockHandler) (i, blockData, pTimeout, pReadmask);

    SCREEN_EPILOGUE(pScreen, BlockHandler);

    if (!pPriv->cp->isUp && pPriv->cp->shouldBeUp)
    {
	SPRITE_DEBUG (("BlockHandler restore\n"));
	miSpriteRestoreCursor (pScreen);
    }
}

static void
miSpriteInstallColormap (pMap)
    ColormapPtr	pMap;
{
    ScreenPtr		pScreen = pMap->pScreen;
    miSpriteScreenPtr	pPriv;

    pPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;

    SCREEN_PROLOGUE(pScreen, InstallColormap);
    
    (*pScreen->InstallColormap) (pMap);

    SCREEN_EPILOGUE(pScreen, InstallColormap);

    pPriv->cp->pInstalledMap = pMap;
    if (pPriv->cp->pColormap != pMap)
    {
    	pPriv->cp->checkPixels = TRUE;
	if (pPriv->cp->isUp)
	    miSpriteRemoveCursor (pScreen);
    }
}

static void
miSpriteStoreColors (pMap, ndef, pdef)
    ColormapPtr	pMap;
    int		ndef;
    xColorItem	*pdef;
{
    ScreenPtr		pScreen = pMap->pScreen;
    miSpriteScreenPtr	pPriv;
    int			i;
    int			updated;
    VisualPtr		pVisual;

    pPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;

    SCREEN_PROLOGUE(pScreen, StoreColors);
    
    (*pScreen->StoreColors) (pMap, ndef, pdef);

    SCREEN_EPILOGUE(pScreen, StoreColors);

    if (pPriv->cp->pColormap == pMap)
    {
	updated = 0;
	pVisual = pMap->pVisual;
	if (pVisual->class == DirectColor)
	{
	    /* Direct color - match on any of the subfields */

#define MaskMatch(a,b,mask) (((a) & (pVisual->mask)) == ((b) & (pVisual->mask)))

#define UpdateDAC(plane,dac,mask) {\
    if (MaskMatch (pPriv->cp->colors[plane].pixel,pdef[i].pixel,mask)) {\
	pPriv->cp->colors[plane].dac = pdef[i].dac; \
	updated = 1; \
    } \
}

#define CheckDirect(plane) \
	    UpdateDAC(plane,red,redMask) \
	    UpdateDAC(plane,green,greenMask) \
	    UpdateDAC(plane,blue,blueMask)

	    for (i = 0; i < ndef; i++)
	    {
		CheckDirect (SOURCE_COLOR)
		CheckDirect (MASK_COLOR)
	    }
	}
	else
	{
	    /* PseudoColor/GrayScale - match on exact pixel */
	    for (i = 0; i < ndef; i++)
	    {
	    	if (pdef[i].pixel == pPriv->cp->colors[SOURCE_COLOR].pixel)
	    	{
		    pPriv->cp->colors[SOURCE_COLOR] = pdef[i];
		    if (++updated == 2)
		    	break;
	    	}
	    	if (pdef[i].pixel == pPriv->cp->colors[MASK_COLOR].pixel)
	    	{
		    pPriv->cp->colors[MASK_COLOR] = pdef[i];
		    if (++updated == 2)
		    	break;
	    	}
	    }
	}
    	if (updated)
    	{
	    pPriv->cp->checkPixels = TRUE;
	    if (pPriv->cp->isUp)
	    	miSpriteRemoveCursor (pScreen);
    	}
    }
}

static void
miSpriteFindColors (ScreenPtr pScreen)
{
    miSpriteScreenPtr	pScreenPriv = (miSpriteScreenPtr)
			    pScreen->devPrivates[miSpriteScreenIndex].ptr;
    CursorPtr		pCursor;
    xColorItem		*sourceColor, *maskColor;

    pCursor = pScreenPriv->cp->pCursor;
    sourceColor = &pScreenPriv->cp->colors[SOURCE_COLOR];
    maskColor = &pScreenPriv->cp->colors[MASK_COLOR];
    if (pScreenPriv->cp->pColormap != pScreenPriv->cp->pInstalledMap ||
	!(pCursor->foreRed == sourceColor->red &&
	  pCursor->foreGreen == sourceColor->green &&
          pCursor->foreBlue == sourceColor->blue &&
	  pCursor->backRed == maskColor->red &&
	  pCursor->backGreen == maskColor->green &&
	  pCursor->backBlue == maskColor->blue))
    {
	pScreenPriv->cp->pColormap = pScreenPriv->cp->pInstalledMap;
	sourceColor->red = pCursor->foreRed;
	sourceColor->green = pCursor->foreGreen;
	sourceColor->blue = pCursor->foreBlue;
	FakeAllocColor (pScreenPriv->cp->pColormap, sourceColor);
	maskColor->red = pCursor->backRed;
	maskColor->green = pCursor->backGreen;
	maskColor->blue = pCursor->backBlue;
	FakeAllocColor (pScreenPriv->cp->pColormap, maskColor);
	/* "free" the pixels right away, don't let this confuse you */
	FakeFreeColor(pScreenPriv->cp->pColormap, sourceColor->pixel);
	FakeFreeColor(pScreenPriv->cp->pColormap, maskColor->pixel);
    }
    pScreenPriv->cp->checkPixels = FALSE;
}

/*
 * BackingStore wrappers
 */

static void
miSpriteSaveDoomedAreas (pWin, pObscured, dx, dy)
    WindowPtr	pWin;
    RegionPtr	pObscured;
    int		dx, dy;
{
    ScreenPtr		pScreen;
    miSpriteScreenPtr   pScreenPriv;
    BoxRec		cursorBox;

    pScreen = pWin->drawable.pScreen;
    
    SCREEN_PROLOGUE (pScreen, SaveDoomedAreas);

    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;
    if (pScreenPriv->cp->isUp)
    {
	cursorBox = pScreenPriv->cp->saved;

	if (dx || dy)
 	{
	    cursorBox.x1 += dx;
	    cursorBox.y1 += dy;
	    cursorBox.x2 += dx;
	    cursorBox.y2 += dy;
	}
	if (RECT_IN_REGION( pScreen, pObscured, &cursorBox) != rgnOUT)
	    miSpriteRemoveCursor (pScreen);
    }

    (*pScreen->SaveDoomedAreas) (pWin, pObscured, dx, dy);

    SCREEN_EPILOGUE (pScreen, SaveDoomedAreas);
}

/*
 * miPointer interface routines
 */

#define SPRITE_PAD  8

static Bool
miSpriteRealizeCursor (pDev, pScreen, pCursor)
    DeviceIntPtr pDev;
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    miSpriteScreenPtr	pScreenPriv;

    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;
    if (pCursor == pScreenPriv->cp->pCursor)
	pScreenPriv->cp->checkPixels = TRUE;
    return (*pScreenPriv->funcs->RealizeCursor) (pScreen, pCursor);
}

static Bool
miSpriteUnrealizeCursor (pDev, pScreen, pCursor)
    DeviceIntPtr pDev;
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    miSpriteScreenPtr	pScreenPriv;

    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;
    return (*pScreenPriv->funcs->UnrealizeCursor) (pScreen, pCursor);
}

static void
miSpriteSetCursor (pDev, pScreen, pCursor, x, y)
    DeviceIntPtr pDev;
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
    int		x;
    int		y;
{
    miSpriteScreenPtr	pScreenPriv;

    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;
    if (!pCursor)
    {
    	pScreenPriv->cp->shouldBeUp = FALSE;
    	if (pScreenPriv->cp->isUp)
	    miSpriteRemoveCursor (pScreen);
	pScreenPriv->cp->pCursor = 0;
	return;
    }
    pScreenPriv->cp->shouldBeUp = TRUE;
    if (pScreenPriv->cp->x == x &&
	pScreenPriv->cp->y == y &&
	pScreenPriv->cp->pCursor == pCursor &&
	!pScreenPriv->cp->checkPixels)
    {
	return;
    }
    pScreenPriv->cp->x = x;
    pScreenPriv->cp->y = y;
    pScreenPriv->cp->pCacheWin = NullWindow;
    if (pScreenPriv->cp->checkPixels || pScreenPriv->cp->pCursor != pCursor)
    {
	pScreenPriv->cp->pCursor = pCursor;
	miSpriteFindColors (pScreen);
    }
    if (pScreenPriv->cp->isUp) {
	int	sx, sy;
	/*
	 * check to see if the old saved region
	 * encloses the new sprite, in which case we use
	 * the flicker-free MoveCursor primitive.
	 */
	sx = pScreenPriv->cp->x - (int)pCursor->bits->xhot;
	sy = pScreenPriv->cp->y - (int)pCursor->bits->yhot;
	if (sx + (int) pCursor->bits->width >= pScreenPriv->cp->saved.x1 &&
	    sx < pScreenPriv->cp->saved.x2 &&
	    sy + (int) pCursor->bits->height >= pScreenPriv->cp->saved.y1 &&
	    sy < pScreenPriv->cp->saved.y2 &&
	    (int) pCursor->bits->width + (2 * SPRITE_PAD) ==
		pScreenPriv->cp->saved.x2 - pScreenPriv->cp->saved.x1 &&
	    (int) pCursor->bits->height + (2 * SPRITE_PAD) ==
		pScreenPriv->cp->saved.y2 - pScreenPriv->cp->saved.y1
	    )
	{
	    DamageDrawInternal (pScreen, TRUE);
	    miSpriteIsUpFALSE (pScreen, pScreenPriv);
	    if (!(sx >= pScreenPriv->cp->saved.x1 &&
                  sx + (int)pCursor->bits->width < pScreenPriv->cp->saved.x2
                  && sy >= pScreenPriv->cp->saved.y1 &&
                  sy + (int)pCursor->bits->height <
                                pScreenPriv->cp->saved.y2)) 
            {
		int oldx1, oldy1, dx, dy;

		oldx1 = pScreenPriv->cp->saved.x1;
		oldy1 = pScreenPriv->cp->saved.y1;
		dx = oldx1 - (sx - SPRITE_PAD);
		dy = oldy1 - (sy - SPRITE_PAD);
		pScreenPriv->cp->saved.x1 -= dx;
		pScreenPriv->cp->saved.y1 -= dy;
		pScreenPriv->cp->saved.x2 -= dx;
		pScreenPriv->cp->saved.y2 -= dy;
		(void) (*pScreenPriv->funcs->ChangeSave) (pScreen,
				pScreenPriv->cp->saved.x1,
 				pScreenPriv->cp->saved.y1,
                                pScreenPriv->cp->saved.x2 -
                                pScreenPriv->cp->saved.x1,
                                pScreenPriv->cp->saved.y2 -
                                pScreenPriv->cp->saved.y1,
				dx, dy);
	    }
	    (void) (*pScreenPriv->funcs->MoveCursor) (pScreen, pCursor,
				  pScreenPriv->cp->saved.x1,
 				  pScreenPriv->cp->saved.y1,
                                  pScreenPriv->cp->saved.x2 -
                                  pScreenPriv->cp->saved.x1,
                                  pScreenPriv->cp->saved.y2 -
                                  pScreenPriv->cp->saved.y1,
				  sx - pScreenPriv->cp->saved.x1,
				  sy - pScreenPriv->cp->saved.y1,
				  pScreenPriv->cp->colors[SOURCE_COLOR].pixel,
				  pScreenPriv->cp->colors[MASK_COLOR].pixel);
	    miSpriteIsUpTRUE (pScreen, pScreenPriv);
	    DamageDrawInternal (pScreen, FALSE);
	}
	else
	{
	    SPRITE_DEBUG (("SetCursor remove\n"));
	    miSpriteRemoveCursor (pScreen);
	}
    }
    if (!pScreenPriv->cp->isUp && pScreenPriv->cp->pCursor)
    {
	SPRITE_DEBUG (("SetCursor restore\n"));
	miSpriteRestoreCursor (pScreen);
    }
}

static void
miSpriteMoveCursor (pDev, pScreen, x, y)
    DeviceIntPtr pDev;
    ScreenPtr	pScreen;
    int		x, y;
{
    miSpriteScreenPtr	pScreenPriv;
    CursorPtr pCursor;

    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;
    pCursor = pScreenPriv->cp->pCursor;
#ifdef MPX
    if (IsMPDev(pDev))
        pCursor = pScreenPriv->mpCursors[pDev->id].pCursor;
#endif
    miSpriteSetCursor (pDev, pScreen, pCursor, x, y);
}

/*
 * undraw/draw cursor
 */

static void
miSpriteRemoveCursor (pScreen)
    ScreenPtr	pScreen;
{
    miSpriteScreenPtr   pScreenPriv;

    DamageDrawInternal (pScreen, TRUE);
    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;
    miSpriteIsUpFALSE (pScreen, pScreenPriv);
    pScreenPriv->cp->pCacheWin = NullWindow;
    if (!(*pScreenPriv->funcs->RestoreUnderCursor) (pScreen,
					 pScreenPriv->cp->saved.x1,
                                         pScreenPriv->cp->saved.y1,
                                         pScreenPriv->cp->saved.x2 -
                                         pScreenPriv->cp->saved.x1,
                                         pScreenPriv->cp->saved.y2 -
                                         pScreenPriv->cp->saved.y1))
    {
	miSpriteIsUpTRUE (pScreen, pScreenPriv);
    }
    DamageDrawInternal (pScreen, FALSE);
}

/*
 * Called from the block handler, restores the cursor
 * before waiting for something to do.
 */

static void
miSpriteRestoreCursor (pScreen)
    ScreenPtr	pScreen;
{
    miSpriteScreenPtr   pScreenPriv;
    int			x, y;
    CursorPtr		pCursor;

    DamageDrawInternal (pScreen, TRUE);
    miSpriteComputeSaved (pScreen);
    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;
    pCursor = pScreenPriv->cp->pCursor;
    x = pScreenPriv->cp->x - (int)pCursor->bits->xhot;
    y = pScreenPriv->cp->y - (int)pCursor->bits->yhot;
    if ((*pScreenPriv->funcs->SaveUnderCursor) (pScreen,
				      pScreenPriv->cp->saved.x1,
				      pScreenPriv->cp->saved.y1,
                                      pScreenPriv->cp->saved.x2 -
                                      pScreenPriv->cp->saved.x1,
                                      pScreenPriv->cp->saved.y2 -
                                      pScreenPriv->cp->saved.y1))
    {
	if (pScreenPriv->cp->checkPixels)
	    miSpriteFindColors (pScreen);
	if ((*pScreenPriv->funcs->PutUpCursor) (pScreen, pCursor, x, y,
				  pScreenPriv->cp->colors[SOURCE_COLOR].pixel,
				  pScreenPriv->cp->colors[MASK_COLOR].pixel))
	{
	    miSpriteIsUpTRUE (pScreen, pScreenPriv);
	}
    }
    DamageDrawInternal (pScreen, FALSE);
}

/*
 * compute the desired area of the screen to save
 */

static void
miSpriteComputeSaved (pScreen)
    ScreenPtr	pScreen;
{
    miSpriteScreenPtr   pScreenPriv;
    int		    x, y, w, h;
    int		    wpad, hpad;
    CursorPtr	    pCursor;

    pScreenPriv = (miSpriteScreenPtr) pScreen->devPrivates[miSpriteScreenIndex].ptr;
    pCursor = pScreenPriv->cp->pCursor;
    x = pScreenPriv->cp->x - (int)pCursor->bits->xhot;
    y = pScreenPriv->cp->y - (int)pCursor->bits->yhot;
    w = pCursor->bits->width;
    h = pCursor->bits->height;
    wpad = SPRITE_PAD;
    hpad = SPRITE_PAD;
    pScreenPriv->cp->saved.x1 = x - wpad;
    pScreenPriv->cp->saved.y1 = y - hpad;
    pScreenPriv->cp->saved.x2 = pScreenPriv->cp->saved.x1 + w + wpad * 2;
    pScreenPriv->cp->saved.y2 = pScreenPriv->cp->saved.y1 + h + hpad * 2;
}
