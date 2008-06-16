/*
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Author: Alan Hourihane <alanh@tungstengraphics.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86_OSproc.h"

#include "driver.h"

struct PixmapPriv {
    drmBO bo;
    #if 0
    dri_fence *fence;
    #endif
    int flags;
};

static void
ExaWaitMarker(ScreenPtr pScreen, int marker)
{
}

static int
ExaMarkSync(ScreenPtr pScreen)
{
    return 1;
}

Bool
ExaPrepareAccess(PixmapPtr pPix, int index)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    struct PixmapPriv *priv;
    int ret;

    priv = exaGetPixmapDriverPrivate(pPix);

    if (!priv)
	return FALSE;

    if (priv->bo.handle) {
    	void *virtual;

    	ret = drmBOMap(ms->fd,
	     &priv->bo, DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE, 0, &virtual);
	if (ret) {
    	    driUnlock(pScreen);
	    FatalError("Failed to map pixmap: %s\n", strerror(-ret));
	    return;
	}

	pPix->devPrivate.ptr = priv->bo.virtual;
    }


    return TRUE;
}

void
ExaFinishAccess(PixmapPtr pPix, int index)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    struct PixmapPriv *priv;
    int ret;

    priv = exaGetPixmapDriverPrivate(pPix);

    if (!priv)
	return;

    if (priv->bo.handle) {
	ret = drmBOUnmap(ms->fd, &priv->bo);
	if (ret) {
	    driUnlock(pScreen);
	    FatalError("Failed to unmap pixmap: %s\n", strerror(-ret));
	    return;
	}

	pPix->devPrivate.ptr = NULL;
    }
}

static void
ExaDone(PixmapPtr pPixmap)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
}

static void
ExaDoneComposite(PixmapPtr pPixmap)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
}

static Bool
ExaPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planeMask, Pixel fg)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];

    ErrorF("SOLID\n");

    if (!EXA_PM_IS_SOLID(&pPixmap->drawable, planeMask))
	return FALSE;

    /* can't do depth 4 */
    if (pPixmap->drawable.depth == 4)
	return FALSE;

    return FALSE;
}

static void
ExaSolid(PixmapPtr pPixmap, int x1, int y1, int x2, int y2)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
}

static Bool
ExaPrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap, int xdir,
	       int ydir, int alu, Pixel planeMask)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];

    ErrorF("COPY\n");

    /* can't do depth 4 */
    if (pSrcPixmap->drawable.depth == 4 || pDstPixmap->drawable.depth == 4)
	return FALSE;

    return FALSE;
}

static void
ExaCopy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX, int dstY,
	int width, int height)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
}

static Bool
ExaPrepareComposite(int op, PicturePtr pSrcPicture,
		    PicturePtr pMaskPicture, PicturePtr pDstPicture,
		    PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst)
{
    ScreenPtr pScreen = pDst->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    return FALSE;
}

static Bool
ExaUploadToScreen(PixmapPtr pDst, int x, int y, int w, int h, char *src,
		  int src_pitch)
{
    ScreenPtr pScreen = pDst->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    ErrorF("UPLOAD\n");

    return FALSE;
}

static void
ExaComposite(PixmapPtr pDst, int srcX, int srcY, int maskX, int maskY,
	     int dstX, int dstY, int width, int height)
{
    ScreenPtr pScreen = pDst->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
}

static Bool
ExaCheckComposite(int op,
		  PicturePtr pSrcPicture, PicturePtr pMaskPicture,
		  PicturePtr pDstPicture)
{
    DrawablePtr pDraw = pSrcPicture->pDrawable;
    int w = pDraw->width;
    int h = pDraw->height;

    return FALSE;
}

static void *
ExaCreatePixmap(ScreenPtr pScreen, int size, int align)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct PixmapPriv *priv;
    void *virtual;

    priv = xcalloc(1, sizeof(struct PixmapPriv));
    if (!priv)
        return NULL;

    if (size == 0)
	return priv;

    drmBOCreate(ms->fd, size, 4096, NULL,
		DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE | DRM_BO_FLAG_SHAREABLE
		| DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_MAPPABLE |
		DRM_BO_FLAG_CACHED_MAPPED,
		0, &priv->bo);

    return priv;
}

static void 
ExaDestroyPixmap(ScreenPtr pScreen, void *dPriv)
{
    struct PixmapPriv *priv = (struct PixmapPriv *)dPriv;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);

    if (!priv)
    	return;

    if (priv->bo.handle)
	drmBOUnreference(ms->fd, &priv->bo);

    xfree(priv);
}

static Bool 
ExaPixmapIsOffscreen(PixmapPtr pPixmap)
{
    struct PixmapPriv *priv;
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);

    priv = exaGetPixmapDriverPrivate(pPixmap);

    if (!priv)
       return FALSE;

    if (priv->bo.handle)
       return TRUE;

    return FALSE;
}

/* FIXME !! */
unsigned int
driGetPixmapHandle(PixmapPtr pPixmap, unsigned int *flags)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct PixmapPriv *priv;

    *flags = 0;

    if (rootPixmap == pPixmap)
        return ms->bo.handle;

    if (!ms->pExa)
    	return 0;

    priv = exaGetPixmapDriverPrivate(pPixmap);

    if (!priv)
       return 0;

    if (priv->bo.handle)
        return priv->bo.handle;

    return 0;
}

static Bool 
ExaModifyPixmapHeader(PixmapPtr pPixmap, int width, int height,
		      int depth, int bitsPerPixel, int devKind,
		      pointer pPixData)
{
    ScreenPtr	pScreen = pPixmap->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pPixmap);
    modesettingPtr ms = modesettingPTR(pScrn);
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);

    if (rootPixmap == pPixmap) {
	miModifyPixmapHeader(pPixmap, width, height, depth,
			     bitsPerPixel, devKind, NULL);

	return TRUE;
    }

    return FALSE;
}


void
ExaClose(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);

    exaDriverFini(pScrn->pScreen);

#if 0
    drmBOUnreference(ms->fd, &ms->exa_bo);
#endif
}

ExaDriverPtr
ExaInit(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    ExaDriverPtr pExa;

    pExa = exaDriverAlloc();
    if (!pExa) {
	goto out_err;
    }

#if 0
    /* Create a 256KB offscreen area */
    drmBOCreate(ms->fd, 256 * 1024, 0, NULL,
		DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE | DRM_BO_FLAG_MEM_TT,
		DRM_BO_HINT_DONT_FENCE, &ms->exa_bo);
#endif

    memset(pExa, 0, sizeof(*pExa));
    pExa->exa_major = 2;
    pExa->exa_minor = 4;
    pExa->memoryBase = 0; /* ms->exa_bo.virtual; */
    pExa->memorySize = 0; /* ms->exa_bo.size; */
    pExa->offScreenBase = 0;
    pExa->pixmapOffsetAlign = 8;
    pExa->pixmapPitchAlign = 32 * 4;
    pExa->flags = EXA_OFFSCREEN_PIXMAPS | EXA_HANDLES_PIXMAPS;
    pExa->maxX = 8191;		       /* FIXME */
    pExa->maxY = 8191;		       /* FIXME */
    pExa->WaitMarker = ExaWaitMarker;
    pExa->MarkSync = ExaMarkSync;
    pExa->PrepareSolid = ExaPrepareSolid;
    pExa->Solid = ExaSolid;
    pExa->DoneSolid = ExaDone;
    pExa->PrepareCopy = ExaPrepareCopy;
    pExa->Copy = ExaCopy;
    pExa->DoneCopy = ExaDone;
    pExa->CheckComposite = ExaCheckComposite;
    pExa->PrepareComposite = ExaPrepareComposite;
    pExa->Composite = ExaComposite;
    pExa->DoneComposite = ExaDoneComposite;
    pExa->PixmapIsOffscreen = ExaPixmapIsOffscreen;
    pExa->PrepareAccess = ExaPrepareAccess;
    pExa->FinishAccess = ExaFinishAccess;
    pExa->UploadToScreen = ExaUploadToScreen;
    pExa->CreatePixmap = ExaCreatePixmap;
    pExa->DestroyPixmap = ExaDestroyPixmap;
    pExa->ModifyPixmapHeader = ExaModifyPixmapHeader;

    if (!exaDriverInit(pScrn->pScreen, pExa)) {
	goto out_err;
    }

    return pExa;

  out_err:
    ExaClose(pScrn);

    return NULL;
}
