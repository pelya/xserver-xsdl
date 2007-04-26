/*
 * midispcur.c
 *
 * machine independent cursor display routines
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
/* 
 * MPX additions:
 * Copyright © 2006 Peter Hutterer
 * Author: Peter Hutterer <peter@cs.unisa.edu.au>
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#define NEED_EVENTS
# include   <X11/X.h>
# include   "misc.h"
# include   "input.h"
# include   "cursorstr.h"
# include   "windowstr.h"
# include   "regionstr.h"
# include   "dixstruct.h"
# include   "scrnintstr.h"
# include   "servermd.h"
# include   "mipointer.h"
# include   "misprite.h"
# include   "gcstruct.h"

#ifdef ARGB_CURSOR
# include   "picturestr.h"
#endif

# include "inputstr.h" /* for MAX_DEVICES */

/* per-screen private data */

static int	miDCScreenIndex;
static unsigned long miDCGeneration = 0;

static Bool	miDCCloseScreen(int index, ScreenPtr pScreen);

/* per device private data */
static int      miDCSpriteIndex;

typedef struct {
    GCPtr	    pSourceGC, pMaskGC;
    GCPtr	    pSaveGC, pRestoreGC;
    GCPtr	    pMoveGC;
    GCPtr	    pPixSourceGC, pPixMaskGC;
    PixmapPtr	    pSave, pTemp;
#ifdef ARGB_CURSOR
    PicturePtr	    pRootPicture;
    PicturePtr	    pTempPicture;
#endif
} miDCBufferRec, *miDCBufferPtr;

#define MIDCBUFFER(dev) \
 ((DevHasCursor(dev)) ? \
  (miDCBufferPtr)dev->devPrivates[miDCSpriteIndex].ptr :\
  (miDCBufferPtr)inputInfo.pointer->devPrivates[miDCSpriteIndex].ptr)

/* 
 * The core pointer buffer will point to the index of the virtual core pointer
 * in the pCursorBuffers array. 
 */
typedef struct {
    CloseScreenProcPtr CloseScreen;
} miDCScreenRec, *miDCScreenPtr;

/* per-cursor per-screen private data */
typedef struct {
    PixmapPtr		sourceBits;	    /* source bits */
    PixmapPtr		maskBits;	    /* mask bits */
#ifdef ARGB_CURSOR
    PicturePtr		pPicture;
#endif
} miDCCursorRec, *miDCCursorPtr;

/*
 * sprite/cursor method table
 */

static Bool	miDCRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
static Bool	miDCUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
static Bool	miDCPutUpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, 
                                CursorPtr pCursor, int x, int y, 
                                unsigned long source, unsigned long mask);
static Bool	miDCSaveUnderCursor(DeviceIntPtr pDev, ScreenPtr pScreen, 
                                    int x, int y,
				    int w, int h);
static Bool	miDCRestoreUnderCursor(DeviceIntPtr pDev, ScreenPtr pScreen, 
                                       int x, int y,
				       int w, int h);
static Bool	miDCMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen, 
                               CursorPtr pCursor, int x, int y, 
                               int w, int h, int dx, int dy,
			       unsigned long source, unsigned long mask);
static Bool	miDCChangeSave(DeviceIntPtr pDev, ScreenPtr pScreen, 
                               int x, int y, int w, int h,	
                               int dx, int dy);

static Bool     miDCDeviceInitialize(DeviceIntPtr pDev, ScreenPtr pScreen);
static void     miDCDeviceCleanup(DeviceIntPtr pDev, ScreenPtr pScreen);

static miSpriteCursorFuncRec miDCFuncs = {
    miDCRealizeCursor,
    miDCUnrealizeCursor,
    miDCPutUpCursor,
    miDCSaveUnderCursor,
    miDCRestoreUnderCursor,
    miDCMoveCursor,
    miDCChangeSave,
    miDCDeviceInitialize,
    miDCDeviceCleanup
};

_X_EXPORT Bool
miDCInitialize (pScreen, screenFuncs)
    ScreenPtr		    pScreen;
    miPointerScreenFuncPtr  screenFuncs;
{
    miDCScreenPtr   pScreenPriv;

    if (miDCGeneration != serverGeneration)
    {
	miDCScreenIndex = AllocateScreenPrivateIndex ();
	if (miDCScreenIndex < 0)
	    return FALSE;
	miDCGeneration = serverGeneration;
    }
    pScreenPriv = (miDCScreenPtr) xalloc (sizeof (miDCScreenRec));
    if (!pScreenPriv)
	return FALSE;


    miDCSpriteIndex = AllocateDevicePrivateIndex();

    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = miDCCloseScreen;
    
    pScreen->devPrivates[miDCScreenIndex].ptr = (pointer) pScreenPriv;

    if (!miSpriteInitialize (pScreen, &miDCFuncs, screenFuncs))
    {
	xfree ((pointer) pScreenPriv);
	return FALSE;
    }
    return TRUE;
}

#define tossGC(gc)  (gc ? FreeGC (gc, (GContext) 0) : 0)
#define tossPix(pix)	(pix ? (*pScreen->DestroyPixmap) (pix) : TRUE)
#define tossPict(pict)	(pict ? FreePicture (pict, 0) : 0)

static Bool
miDCCloseScreen (index, pScreen)
    int		index;
    ScreenPtr	pScreen;
{
    miDCScreenPtr   pScreenPriv;

    pScreenPriv = (miDCScreenPtr) pScreen->devPrivates[miDCScreenIndex].ptr;
    pScreen->CloseScreen = pScreenPriv->CloseScreen;

    xfree ((pointer) pScreenPriv);
    return (*pScreen->CloseScreen) (index, pScreen);
}

static Bool
miDCRealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    if (pCursor->bits->refcnt <= 1)
	pCursor->bits->devPriv[pScreen->myNum] = (pointer)NULL;
    return TRUE;
}

#ifdef ARGB_CURSOR
#define EnsurePicture(picture,draw,win) (picture || miDCMakePicture(&picture,draw,win))

static VisualPtr
miDCGetWindowVisual (WindowPtr pWin)
{
    ScreenPtr	    pScreen = pWin->drawable.pScreen;
    VisualID	    vid = wVisual (pWin);
    int		    i;

    for (i = 0; i < pScreen->numVisuals; i++)
	if (pScreen->visuals[i].vid == vid)
	    return &pScreen->visuals[i];
    return 0;
}

static PicturePtr
miDCMakePicture (PicturePtr *ppPicture, DrawablePtr pDraw, WindowPtr pWin)
{
    ScreenPtr	    pScreen = pDraw->pScreen;
    VisualPtr	    pVisual;
    PictFormatPtr   pFormat;
    XID		    subwindow_mode = IncludeInferiors;
    PicturePtr	    pPicture;
    int		    error;
    
    pVisual = miDCGetWindowVisual (pWin);
    if (!pVisual)
	return 0;
    pFormat = PictureMatchVisual (pScreen, pDraw->depth, pVisual);
    if (!pFormat)
	return 0;
    pPicture = CreatePicture (0, pDraw, pFormat,
			      CPSubwindowMode, &subwindow_mode,
			      serverClient, &error);
    *ppPicture = pPicture;
    return pPicture;
}
#endif

static miDCCursorPtr
miDCRealize (
    ScreenPtr	pScreen,
    CursorPtr	pCursor)
{
    miDCCursorPtr   pPriv;
    GCPtr	    pGC;
    XID		    gcvals[3];

    pPriv = (miDCCursorPtr) xalloc (sizeof (miDCCursorRec));
    if (!pPriv)
	return (miDCCursorPtr)NULL;
#ifdef ARGB_CURSOR
    if (pCursor->bits->argb)
    {
	PixmapPtr	pPixmap;
	PictFormatPtr	pFormat;
	int		error;
	
	pFormat = PictureMatchFormat (pScreen, 32, PICT_a8r8g8b8);
	if (!pFormat)
	{
	    xfree ((pointer) pPriv);
	    return (miDCCursorPtr)NULL;
	}
	
	pPriv->sourceBits = 0;
	pPriv->maskBits = 0;
	pPixmap = (*pScreen->CreatePixmap) (pScreen, pCursor->bits->width,
					    pCursor->bits->height, 32);
	if (!pPixmap)
	{
	    xfree ((pointer) pPriv);
	    return (miDCCursorPtr)NULL;
	}
	pGC = GetScratchGC (32, pScreen);
	if (!pGC)
	{
	    (*pScreen->DestroyPixmap) (pPixmap);
	    xfree ((pointer) pPriv);
	    return (miDCCursorPtr)NULL;
	}
	ValidateGC (&pPixmap->drawable, pGC);
	(*pGC->ops->PutImage) (&pPixmap->drawable, pGC, 32,
			       0, 0, pCursor->bits->width,
			       pCursor->bits->height,
			       0, ZPixmap, (char *) pCursor->bits->argb);
	FreeScratchGC (pGC);
	pPriv->pPicture = CreatePicture (0, &pPixmap->drawable,
					pFormat, 0, 0, serverClient, &error);
        (*pScreen->DestroyPixmap) (pPixmap);
	if (!pPriv->pPicture)
	{
	    xfree ((pointer) pPriv);
	    return (miDCCursorPtr)NULL;
	}
	pCursor->bits->devPriv[pScreen->myNum] = (pointer) pPriv;
	return pPriv;
    }
    pPriv->pPicture = 0;
#endif
    pPriv->sourceBits = (*pScreen->CreatePixmap) (pScreen, pCursor->bits->width, pCursor->bits->height, 1);
    if (!pPriv->sourceBits)
    {
	xfree ((pointer) pPriv);
	return (miDCCursorPtr)NULL;
    }
    pPriv->maskBits =  (*pScreen->CreatePixmap) (pScreen, pCursor->bits->width, pCursor->bits->height, 1);
    if (!pPriv->maskBits)
    {
	(*pScreen->DestroyPixmap) (pPriv->sourceBits);
	xfree ((pointer) pPriv);
	return (miDCCursorPtr)NULL;
    }
    pCursor->bits->devPriv[pScreen->myNum] = (pointer) pPriv;

    /* create the two sets of bits, clipping as appropriate */

    pGC = GetScratchGC (1, pScreen);
    if (!pGC)
    {
	(void) miDCUnrealizeCursor (pScreen, pCursor);
	return (miDCCursorPtr)NULL;
    }

    ValidateGC ((DrawablePtr)pPriv->sourceBits, pGC);
    (*pGC->ops->PutImage) ((DrawablePtr)pPriv->sourceBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, (char *)pCursor->bits->source);
    gcvals[0] = GXand;
    ChangeGC (pGC, GCFunction, gcvals);
    ValidateGC ((DrawablePtr)pPriv->sourceBits, pGC);
    (*pGC->ops->PutImage) ((DrawablePtr)pPriv->sourceBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, (char *)pCursor->bits->mask);

    /* mask bits -- pCursor->mask & ~pCursor->source */
    gcvals[0] = GXcopy;
    ChangeGC (pGC, GCFunction, gcvals);
    ValidateGC ((DrawablePtr)pPriv->maskBits, pGC);
    (*pGC->ops->PutImage) ((DrawablePtr)pPriv->maskBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, (char *)pCursor->bits->mask);
    gcvals[0] = GXandInverted;
    ChangeGC (pGC, GCFunction, gcvals);
    ValidateGC ((DrawablePtr)pPriv->maskBits, pGC);
    (*pGC->ops->PutImage) ((DrawablePtr)pPriv->maskBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, (char *)pCursor->bits->source);
    FreeScratchGC (pGC);
    return pPriv;
}

static Bool
miDCUnrealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    miDCCursorPtr   pPriv;

    pPriv = (miDCCursorPtr) pCursor->bits->devPriv[pScreen->myNum];
    if (pPriv && (pCursor->bits->refcnt <= 1))
    {
	if (pPriv->sourceBits)
	    (*pScreen->DestroyPixmap) (pPriv->sourceBits);
	if (pPriv->maskBits)
	    (*pScreen->DestroyPixmap) (pPriv->maskBits);
#ifdef ARGB_CURSOR
	if (pPriv->pPicture)
	    FreePicture (pPriv->pPicture, 0);
#endif
	xfree ((pointer) pPriv);
	pCursor->bits->devPriv[pScreen->myNum] = (pointer)NULL;
    }
    return TRUE;
}

static void
miDCPutBits (
    DrawablePtr	    pDrawable,
    miDCCursorPtr   pPriv,
    GCPtr	    sourceGC,
    GCPtr	    maskGC,
    int             x_org,
    int             y_org,
    unsigned        w,
    unsigned        h,
    unsigned long   source,
    unsigned long   mask)
{
    XID	    gcvals[1];
    int     x, y;

    if (sourceGC->fgPixel != source)
    {
	gcvals[0] = source;
	DoChangeGC (sourceGC, GCForeground, gcvals, 0);
    }
    if (sourceGC->serialNumber != pDrawable->serialNumber)
	ValidateGC (pDrawable, sourceGC);

    if(sourceGC->miTranslate) 
    {
        x = pDrawable->x + x_org;
        y = pDrawable->y + y_org;
    } 
    else
    {
        x = x_org;
        y = y_org;
    }

    (*sourceGC->ops->PushPixels) (sourceGC, pPriv->sourceBits, pDrawable, w, h, x, y);
    if (maskGC->fgPixel != mask)
    {
	gcvals[0] = mask;
	DoChangeGC (maskGC, GCForeground, gcvals, 0);
    }
    if (maskGC->serialNumber != pDrawable->serialNumber)
	ValidateGC (pDrawable, maskGC);

    if(maskGC->miTranslate) 
    {
        x = pDrawable->x + x_org;
        y = pDrawable->y + y_org;
    } 
    else
    {
        x = x_org;
        y = y_org;
    }

    (*maskGC->ops->PushPixels) (maskGC, pPriv->maskBits, pDrawable, w, h, x, y);
}

#define EnsureGC(gc,win) (gc || miDCMakeGC(&gc, win))

static GCPtr
miDCMakeGC(
    GCPtr	*ppGC,
    WindowPtr	pWin)
{
    GCPtr pGC;
    int   status;
    XID   gcvals[2];

    gcvals[0] = IncludeInferiors;
    gcvals[1] = FALSE;
    pGC = CreateGC((DrawablePtr)pWin,
		   GCSubwindowMode|GCGraphicsExposures, gcvals, &status);
    if (pGC && pWin->drawable.pScreen->DrawGuarantee)
	(*pWin->drawable.pScreen->DrawGuarantee) (pWin, pGC, GuaranteeVisBack);
    *ppGC = pGC;
    return pGC;
}


static Bool
miDCPutUpCursor (pDev, pScreen, pCursor, x, y, source, mask)
    DeviceIntPtr    pDev;
    ScreenPtr	    pScreen;
    CursorPtr	    pCursor;
    int		    x, y;
    unsigned long   source, mask;
{
    miDCScreenPtr   pScreenPriv;
    miDCCursorPtr   pPriv;
    miDCBufferPtr   pBuffer;
    WindowPtr	    pWin;

    pPriv = (miDCCursorPtr) pCursor->bits->devPriv[pScreen->myNum];
    if (!pPriv)
    {
	pPriv = miDCRealize(pScreen, pCursor);
	if (!pPriv)
	    return FALSE;
    }
    pScreenPriv = (miDCScreenPtr) pScreen->devPrivates[miDCScreenIndex].ptr;
    pWin = WindowTable[pScreen->myNum];
    pBuffer = MIDCBUFFER(pDev);

#ifdef ARGB_CURSOR
    if (pPriv->pPicture)
    {
        /* see comment in miDCPutUpCursor */
        if (pBuffer->pRootPicture && 
                pBuffer->pRootPicture->pDrawable &&
                pBuffer->pRootPicture->pDrawable->pScreen != pScreen)
        {
            tossPict(pBuffer->pRootPicture);
            pBuffer->pRootPicture = NULL;
        }

	if (!EnsurePicture(pBuffer->pRootPicture, &pWin->drawable, pWin))
	    return FALSE;
	CompositePicture (PictOpOver,
			  pPriv->pPicture,
			  NULL,
			  pBuffer->pRootPicture,
			  0, 0, 0, 0, 
			  x, y, 
			  pCursor->bits->width,
			  pCursor->bits->height);
    }
    else
#endif
    {
        /**
         * XXX: Before MPX, the sourceGC and maskGC were attached to the
         * screen, and would switch as the screen switches.  With mpx we have
         * the GC's attached to the device now, so each time we switch screen
         * we need to make sure the GC's are allocated on the new screen.
         * This is ... not optimal. (whot)
         */
        if (pBuffer->pSourceGC && pScreen != pBuffer->pSourceGC->pScreen)
        {
            tossGC(pBuffer->pSourceGC);
            pBuffer->pSourceGC = NULL;
        }

        if (pBuffer->pMaskGC && pScreen != pBuffer->pMaskGC->pScreen)
        {
            tossGC(pBuffer->pMaskGC);
            pBuffer->pMaskGC = NULL;
        }

	if (!EnsureGC(pBuffer->pSourceGC, pWin))
	    return FALSE;
	if (!EnsureGC(pBuffer->pMaskGC, pWin))
	{
	    FreeGC (pBuffer->pSourceGC, (GContext) 0);
	    pBuffer->pSourceGC = 0;
	    return FALSE;
	}
	miDCPutBits ((DrawablePtr)pWin, pPriv,
		     pBuffer->pSourceGC, pBuffer->pMaskGC,
		     x, y, pCursor->bits->width, pCursor->bits->height,
		     source, mask);
    }
    return TRUE;
}

static Bool
miDCSaveUnderCursor (pDev, pScreen, x, y, w, h)
    DeviceIntPtr pDev;
    ScreenPtr	pScreen;
    int		x, y, w, h;
{
    miDCScreenPtr   pScreenPriv;
    miDCBufferPtr   pBuffer;
    PixmapPtr	    pSave;
    WindowPtr	    pWin;
    GCPtr	    pGC;

    pScreenPriv = (miDCScreenPtr) pScreen->devPrivates[miDCScreenIndex].ptr;
    pBuffer = MIDCBUFFER(pDev);

    pSave = pBuffer->pSave;
    pWin = WindowTable[pScreen->myNum];
    if (!pSave || pSave->drawable.width < w || pSave->drawable.height < h)
    {
	if (pSave)
	    (*pScreen->DestroyPixmap) (pSave);
	pBuffer->pSave = pSave =
		(*pScreen->CreatePixmap) (pScreen, w, h, pScreen->rootDepth);
	if (!pSave)
	    return FALSE;
    }
    /* see comment in miDCPutUpCursor */
    if (pBuffer->pSaveGC && pBuffer->pSaveGC->pScreen != pScreen)
    {
        tossGC(pBuffer->pSaveGC);
        pBuffer->pSaveGC = NULL;
    }
    if (!EnsureGC(pBuffer->pSaveGC, pWin))
	return FALSE;
    pGC = pBuffer->pSaveGC;
    if (pSave->drawable.serialNumber != pGC->serialNumber)
	ValidateGC ((DrawablePtr) pSave, pGC);
    (*pGC->ops->CopyArea) ((DrawablePtr) pWin, (DrawablePtr) pSave, pGC,
			    x, y, w, h, 0, 0);
    return TRUE;
}

static Bool
miDCRestoreUnderCursor (pDev, pScreen, x, y, w, h)
    DeviceIntPtr pDev;
    ScreenPtr	pScreen;
    int		x, y, w, h;
{
    miDCScreenPtr   pScreenPriv;
    miDCBufferPtr   pBuffer;
    PixmapPtr	    pSave;
    WindowPtr	    pWin;
    GCPtr	    pGC;

    pScreenPriv = (miDCScreenPtr) pScreen->devPrivates[miDCScreenIndex].ptr;
    pBuffer = MIDCBUFFER(pDev);

    pSave = pBuffer->pSave;
    pWin = WindowTable[pScreen->myNum];
    if (!pSave)
	return FALSE;
    /* see comment in miDCPutUpCursor */
    if (pBuffer->pRestoreGC && pBuffer->pRestoreGC->pScreen != pScreen)
    {
        tossGC(pBuffer->pRestoreGC);
        pBuffer->pRestoreGC = NULL;
    }
    if (!EnsureGC(pBuffer->pRestoreGC, pWin))
	return FALSE;
    pGC = pBuffer->pRestoreGC;
    if (pWin->drawable.serialNumber != pGC->serialNumber)
	ValidateGC ((DrawablePtr) pWin, pGC);
    (*pGC->ops->CopyArea) ((DrawablePtr) pSave, (DrawablePtr) pWin, pGC,
			    0, 0, w, h, x, y);
    return TRUE;
}

static Bool
miDCChangeSave (pDev, pScreen, x, y, w, h, dx, dy)
    DeviceIntPtr    pDev;
    ScreenPtr	    pScreen;
    int		    x, y, w, h, dx, dy;
{
    miDCScreenPtr   pScreenPriv;
    miDCBufferPtr   pBuffer;
    PixmapPtr	    pSave;
    WindowPtr	    pWin;
    GCPtr	    pGC;
    int		    sourcex, sourcey, destx, desty, copyw, copyh;

    pScreenPriv = (miDCScreenPtr) pScreen->devPrivates[miDCScreenIndex].ptr;
    pBuffer = MIDCBUFFER(pDev);

    pSave = pBuffer->pSave;
    pWin = WindowTable[pScreen->myNum];
    /*
     * restore the bits which are about to get trashed
     */
    if (!pSave)
	return FALSE;
    /* see comment in miDCPutUpCursor */
    if (pBuffer->pRestoreGC && pBuffer->pRestoreGC->pScreen != pScreen)
    {
        tossGC(pBuffer->pRestoreGC);
        pBuffer->pRestoreGC = NULL;
    }
    if (!EnsureGC(pBuffer->pRestoreGC, pWin))
	return FALSE;
    pGC = pBuffer->pRestoreGC;
    if (pWin->drawable.serialNumber != pGC->serialNumber)
	ValidateGC ((DrawablePtr) pWin, pGC);
    /*
     * copy the old bits to the screen.
     */
    if (dy > 0)
    {
	(*pGC->ops->CopyArea) ((DrawablePtr) pSave, (DrawablePtr) pWin, pGC,
			       0, h - dy, w, dy, x + dx, y + h);
    }
    else if (dy < 0)
    {
	(*pGC->ops->CopyArea) ((DrawablePtr) pSave, (DrawablePtr) pWin, pGC,
			       0, 0, w, -dy, x + dx, y + dy);
    }
    if (dy >= 0)
    {
	desty = y + dy;
	sourcey = 0;
	copyh = h - dy;
    }
    else
    {
	desty = y;
	sourcey = - dy;
	copyh = h + dy;
    }
    if (dx > 0)
    {
	(*pGC->ops->CopyArea) ((DrawablePtr) pSave, (DrawablePtr) pWin, pGC,
			       w - dx, sourcey, dx, copyh, x + w, desty);
    }
    else if (dx < 0)
    {
	(*pGC->ops->CopyArea) ((DrawablePtr) pSave, (DrawablePtr) pWin, pGC,
			       0, sourcey, -dx, copyh, x + dx, desty);
    }
    /* see comment in miDCPutUpCursor */
    if (pBuffer->pSaveGC && pBuffer->pSaveGC->pScreen != pScreen)
    {
        tossGC(pBuffer->pSaveGC);
        pBuffer->pSaveGC = NULL;
    }
    if (!EnsureGC(pBuffer->pSaveGC, pWin))
	return FALSE;
    pGC = pBuffer->pSaveGC;
    if (pSave->drawable.serialNumber != pGC->serialNumber)
	ValidateGC ((DrawablePtr) pSave, pGC);
    /*
     * move the bits that are still valid within the pixmap
     */
    if (dx >= 0)
    {
	sourcex = 0;
	destx = dx;
	copyw = w - dx;
    }
    else
    {
	destx = 0;
	sourcex = - dx;
	copyw = w + dx;
    }
    if (dy >= 0)
    {
	sourcey = 0;
	desty = dy;
	copyh = h - dy;
    }
    else
    {
	desty = 0;
	sourcey = -dy;
	copyh = h + dy;
    }
    (*pGC->ops->CopyArea) ((DrawablePtr) pSave, (DrawablePtr) pSave, pGC,
			   sourcex, sourcey, copyw, copyh, destx, desty);
    /*
     * copy the new bits from the screen into the remaining areas of the
     * pixmap
     */
    if (dy > 0)
    {
	(*pGC->ops->CopyArea) ((DrawablePtr) pWin, (DrawablePtr) pSave, pGC,
			       x, y, w, dy, 0, 0);
    }
    else if (dy < 0)
    {
	(*pGC->ops->CopyArea) ((DrawablePtr) pWin, (DrawablePtr) pSave, pGC,
			       x, y + h + dy, w, -dy, 0, h + dy);
    }
    if (dy >= 0)
    {
	desty = dy;
	sourcey = y + dy;
	copyh = h - dy;
    }
    else
    {
	desty = 0;
	sourcey = y;
	copyh = h + dy;
    }
    if (dx > 0)
    {
	(*pGC->ops->CopyArea) ((DrawablePtr) pWin, (DrawablePtr) pSave, pGC,
			       x, sourcey, dx, copyh, 0, desty);
    }
    else if (dx < 0)
    {
	(*pGC->ops->CopyArea) ((DrawablePtr) pWin, (DrawablePtr) pSave, pGC,
			       x + w + dx, sourcey, -dx, copyh, w + dx, desty);
    }
    return TRUE;
}

static Bool
miDCMoveCursor (pDev, pScreen, pCursor, x, y, w, h, dx, dy, source, mask)
    DeviceIntPtr    pDev;
    ScreenPtr	    pScreen;
    CursorPtr	    pCursor;
    int		    x, y, w, h, dx, dy;
    unsigned long   source, mask;
{
    miDCCursorPtr   pPriv;
    miDCScreenPtr   pScreenPriv;
    miDCBufferPtr   pBuffer;
    int		    status;
    WindowPtr	    pWin;
    GCPtr	    pGC;
    XID		    gcval = FALSE;
    PixmapPtr	    pTemp;

    pPriv = (miDCCursorPtr) pCursor->bits->devPriv[pScreen->myNum];
    if (!pPriv)
    {
	pPriv = miDCRealize(pScreen, pCursor);
	if (!pPriv)
	    return FALSE;
    }
    pScreenPriv = (miDCScreenPtr) pScreen->devPrivates[miDCScreenIndex].ptr;
    pWin = WindowTable[pScreen->myNum];
    pBuffer = MIDCBUFFER(pDev);

    pTemp = pBuffer->pTemp;
    if (!pTemp ||
	pTemp->drawable.width != pBuffer->pSave->drawable.width ||
	pTemp->drawable.height != pBuffer->pSave->drawable.height)
    {
	if (pTemp)
	    (*pScreen->DestroyPixmap) (pTemp);
#ifdef ARGB_CURSOR
	if (pBuffer->pTempPicture)
	{
	    FreePicture (pBuffer->pTempPicture, 0);
	    pBuffer->pTempPicture = 0;
	}
#endif
	pBuffer->pTemp = pTemp = (*pScreen->CreatePixmap)
	    (pScreen, w, h, pBuffer->pSave->drawable.depth);
	if (!pTemp)
	    return FALSE;
    }
    if (!pBuffer->pMoveGC)
    {
	pBuffer->pMoveGC = CreateGC ((DrawablePtr)pTemp,
	    GCGraphicsExposures, &gcval, &status);
	if (!pBuffer->pMoveGC)
	    return FALSE;
    }
    /*
     * copy the saved area to a temporary pixmap
     */
    pGC = pBuffer->pMoveGC;
    if (pGC->serialNumber != pTemp->drawable.serialNumber)
	ValidateGC ((DrawablePtr) pTemp, pGC);
    (*pGC->ops->CopyArea)((DrawablePtr)pBuffer->pSave,
			  (DrawablePtr)pTemp, pGC, 0, 0, w, h, 0, 0);
    
    /*
     * draw the cursor in the temporary pixmap
     */
#ifdef ARGB_CURSOR
    if (pPriv->pPicture)
    {
        /* see comment in miDCPutUpCursor */
        if (pBuffer->pTempPicture && 
                pBuffer->pTempPicture->pDrawable &&
                pBuffer->pTempPicture->pDrawable->pScreen != pScreen)
        {
            tossPict(pBuffer->pTempPicture);
            pBuffer->pTempPicture = NULL;
        }

	if (!EnsurePicture(pBuffer->pTempPicture, &pTemp->drawable, pWin))
	    return FALSE;
	CompositePicture (PictOpOver,
			  pPriv->pPicture,
			  NULL,
			  pBuffer->pTempPicture,
			  0, 0, 0, 0, 
			  dx, dy, 
			  pCursor->bits->width,
			  pCursor->bits->height);
    }
    else
#endif
    {
	if (!pBuffer->pPixSourceGC)
	{
	    pBuffer->pPixSourceGC = CreateGC ((DrawablePtr)pTemp,
		GCGraphicsExposures, &gcval, &status);
	    if (!pBuffer->pPixSourceGC)
		return FALSE;
	}
	if (!pBuffer->pPixMaskGC)
	{
	    pBuffer->pPixMaskGC = CreateGC ((DrawablePtr)pTemp,
		GCGraphicsExposures, &gcval, &status);
	    if (!pBuffer->pPixMaskGC)
		return FALSE;
	}
	miDCPutBits ((DrawablePtr)pTemp, pPriv,
		     pBuffer->pPixSourceGC, pBuffer->pPixMaskGC,
		     dx, dy, pCursor->bits->width, pCursor->bits->height,
		     source, mask);
    }

    /* see comment in miDCPutUpCursor */
    if (pBuffer->pRestoreGC && pBuffer->pRestoreGC->pScreen != pScreen)
    {
        tossGC(pBuffer->pRestoreGC);
        pBuffer->pRestoreGC = NULL;
    }
    /*
     * copy the temporary pixmap onto the screen
     */

    if (!EnsureGC(pBuffer->pRestoreGC, pWin))
	return FALSE;
    pGC = pBuffer->pRestoreGC;
    if (pWin->drawable.serialNumber != pGC->serialNumber)
	ValidateGC ((DrawablePtr) pWin, pGC);

    (*pGC->ops->CopyArea) ((DrawablePtr) pTemp, (DrawablePtr) pWin,
			    pGC,
			    0, 0, w, h, x, y);
    return TRUE;
}

static Bool
miDCDeviceInitialize(pDev, pScreen)
    DeviceIntPtr pDev;
    ScreenPtr pScreen;
{
    miDCBufferPtr pBuffer;

    if (!AllocateDevicePrivate(pDev, miDCSpriteIndex))
        return FALSE;

    pBuffer = 
       pDev->devPrivates[miDCSpriteIndex].ptr = xalloc(sizeof(miDCBufferRec));

    pBuffer->pSourceGC =
        pBuffer->pMaskGC =
        pBuffer->pSaveGC =
        pBuffer->pRestoreGC =
        pBuffer->pMoveGC =
        pBuffer->pPixSourceGC =
        pBuffer->pPixMaskGC = NULL;
#ifdef ARGB_CURSOR
    pBuffer->pRootPicture = NULL;
    pBuffer->pTempPicture = NULL;
#endif
    pBuffer->pSave = pBuffer->pTemp = NULL;

    return TRUE;
}

static void
miDCDeviceCleanup(pDev, pScreen)
    DeviceIntPtr pDev;
    ScreenPtr pScreen;
{
    miDCBufferPtr   pBuffer;

    if (DevHasCursor(pDev))
    {
        pBuffer = MIDCBUFFER(pDev);
        tossGC (pBuffer->pSourceGC);
        tossGC (pBuffer->pMaskGC);
        tossGC (pBuffer->pSaveGC);
        tossGC (pBuffer->pRestoreGC);
        tossGC (pBuffer->pMoveGC);
        tossGC (pBuffer->pPixSourceGC);
        tossGC (pBuffer->pPixMaskGC);
        tossPix (pBuffer->pSave);
        tossPix (pBuffer->pTemp);
#ifdef ARGB_CURSOR
#if 0				/* This has been free()d before */
        tossPict (pScreenPriv->pRootPicture);
#endif 
        tossPict (pBuffer->pTempPicture);
#endif
        xfree(pDev->devPrivates[miDCSpriteIndex].ptr);
        pDev->devPrivates[miDCSpriteIndex].ptr = NULL;
    }
}
