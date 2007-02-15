/*
 * Copyright © 2001 Keith Packard
 *
 * Partly based on code that is Copyright © The XFree86 Project Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/** @file
 * This file covers the initialization and teardown of EXA, and has various
 * functions not responsible for performing rendering, pixmap migration, or
 * memory management.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef MITSHM
#include "shmint.h"
#endif

#include <stdlib.h>

#include "exa_priv.h"
#include <X11/fonts/fontstruct.h>
#include "dixfontstr.h"
#include "exa.h"
#include "cw.h"

static int exaGeneration;
int exaScreenPrivateIndex;
int exaPixmapPrivateIndex;

/**
 * exaGetPixmapOffset() returns the offset (in bytes) within the framebuffer of
 * the beginning of the given pixmap.
 *
 * Note that drivers are free to, and often do, munge this offset as necessary
 * for handing to the hardware -- for example, translating it into a different
 * aperture.  This function may need to be extended in the future if we grow
 * support for having multiple card-accessible offscreen, such as an AGP memory
 * pool alongside the framebuffer pool.
 */
unsigned long
exaGetPixmapOffset(PixmapPtr pPix)
{
    ExaScreenPriv (pPix->drawable.pScreen);
    ExaPixmapPriv (pPix);
    void *ptr;

    /* Return the offscreen pointer if we've hidden the data. */
    if (pPix->devPrivate.ptr == NULL)
	ptr = pExaPixmap->fb_ptr;
    else
	ptr = pPix->devPrivate.ptr;

    return ((unsigned long)ptr - (unsigned long)pExaScr->info->memoryBase);
}

/**
 * exaGetPixmapPitch() returns the pitch (in bytes) of the given pixmap.
 *
 * This is a helper to make driver code more obvious, due to the rather obscure
 * naming of the pitch field in the pixmap.
 */
unsigned long
exaGetPixmapPitch(PixmapPtr pPix)
{
    return pPix->devKind;
}

/**
 * exaGetPixmapSize() returns the size in bytes of the given pixmap in video
 * memory. Only valid when the pixmap is currently in framebuffer.
 */
unsigned long
exaGetPixmapSize(PixmapPtr pPix)
{
    ExaPixmapPrivPtr pExaPixmap;

    pExaPixmap = ExaGetPixmapPriv(pPix);
    if (pExaPixmap != NULL)
	return pExaPixmap->fb_size;
    return 0;
}

/**
 * exaGetDrawablePixmap() returns a backing pixmap for a given drawable.
 *
 * @param pDrawable the drawable being requested.
 *
 * This function returns the backing pixmap for a drawable, whether it is a
 * redirected window, unredirected window, or already a pixmap.  Note that
 * coordinate translation is needed when drawing to the backing pixmap of a
 * redirected window, and the translation coordinates are provided by calling
 * exaGetOffscreenPixmap() on the drawable.
 */
PixmapPtr
exaGetDrawablePixmap(DrawablePtr pDrawable)
{
     if (pDrawable->type == DRAWABLE_WINDOW)
	return pDrawable->pScreen->GetWindowPixmap ((WindowPtr) pDrawable);
     else
	return (PixmapPtr) pDrawable;
}	

/**
 * Sets the offsets to add to coordinates to make them address the same bits in
 * the backing drawable. These coordinates are nonzero only for redirected
 * windows.
 */
static void
exaGetDrawableDeltas (DrawablePtr pDrawable, PixmapPtr pPixmap,
		      int *xp, int *yp)
{
#ifdef COMPOSITE
    if (pDrawable->type == DRAWABLE_WINDOW) {
	*xp = -pPixmap->screen_x;
	*yp = -pPixmap->screen_y;
	return;
    }
#endif

    *xp = 0;
    *yp = 0;
}

/**
 * exaPixmapDirty() marks a pixmap as dirty, allowing for
 * optimizations in pixmap migration when no changes have occurred.
 */
void
exaPixmapDirty (PixmapPtr pPix, int x1, int y1, int x2, int y2)
{
    ExaPixmapPriv(pPix);
    BoxRec box;
    RegionPtr pDamageReg;
    RegionRec region;

    if (!pExaPixmap)
	return;
	
    box.x1 = max(x1, 0);
    box.y1 = max(y1, 0);
    box.x2 = min(x2, pPix->drawable.width);
    box.y2 = min(y2, pPix->drawable.height);

    if (box.x1 >= box.x2 || box.y1 >= box.y2)
	return;

    pDamageReg = DamageRegion(pExaPixmap->pDamage);

    REGION_INIT(pScreen, &region, &box, 1);
    REGION_UNION(pScreen, pDamageReg, pDamageReg, &region);
    REGION_UNINIT(pScreen, &region);
}

/**
 * exaDrawableDirty() marks a pixmap backing a drawable as dirty, allowing for
 * optimizations in pixmap migration when no changes have occurred.
 */
void
exaDrawableDirty (DrawablePtr pDrawable, int x1, int y1, int x2, int y2)
{
    PixmapPtr pPix = exaGetDrawablePixmap(pDrawable);
    int xoff, yoff;

    x1 = max(x1, pDrawable->x);
    y1 = max(y1, pDrawable->y);
    x2 = min(x2, pDrawable->x + pDrawable->width);
    y2 = min(y2, pDrawable->y + pDrawable->height);

    if (x1 >= x2 || y1 >= y2)
	return;

    exaGetDrawableDeltas(pDrawable, pPix, &xoff, &yoff);

    exaPixmapDirty(pPix, x1 + xoff, y1 + yoff, x2 + xoff, y2 + yoff);
}

static Bool
exaDestroyPixmap (PixmapPtr pPixmap)
{
    if (pPixmap->refcnt == 1)
    {
	ExaPixmapPriv (pPixmap);
	if (pExaPixmap->area)
	{
	    DBG_PIXMAP(("-- 0x%p (0x%x) (%dx%d)\n",
                        (void*)pPixmap->drawable.id,
			 ExaGetPixmapPriv(pPixmap)->area->offset,
			 pPixmap->drawable.width,
			 pPixmap->drawable.height));
	    /* Free the offscreen area */
	    exaOffscreenFree (pPixmap->drawable.pScreen, pExaPixmap->area);
	    pPixmap->devPrivate.ptr = pExaPixmap->sys_ptr;
	    pPixmap->devKind = pExaPixmap->sys_pitch;
	}
	REGION_UNINIT(pPixmap->drawable.pScreen, &pExaPixmap->validReg);
    }
    return fbDestroyPixmap (pPixmap);
}

static int
exaLog2(int val)
{
    int bits;

    if (val <= 0)
	return 0;
    for (bits = 0; val != 0; bits++)
	val >>= 1;
    return bits - 1;
}

/**
 * exaCreatePixmap() creates a new pixmap.
 *
 * If width and height are 0, this won't be a full-fledged pixmap and it will
 * get ModifyPixmapHeader() called on it later.  So, we mark it as pinned, because
 * ModifyPixmapHeader() would break migration.  These types of pixmaps are used
 * for scratch pixmaps, or to represent the visible screen.
 */
static PixmapPtr
exaCreatePixmap(ScreenPtr pScreen, int w, int h, int depth)
{
    PixmapPtr		pPixmap;
    ExaPixmapPrivPtr	pExaPixmap;
    int			bpp;
    ExaScreenPriv(pScreen);

    if (w > 32767 || h > 32767)
	return NullPixmap;

    pPixmap = fbCreatePixmap (pScreen, w, h, depth);
    if (!pPixmap)
	return NULL;
    pExaPixmap = ExaGetPixmapPriv(pPixmap);

    bpp = pPixmap->drawable.bitsPerPixel;

    /* Glyphs have w/h equal to zero, and may not be migrated. See exaGlyphs. */
    if (!w || !h)
	pExaPixmap->score = EXA_PIXMAP_SCORE_PINNED;
    else
	pExaPixmap->score = EXA_PIXMAP_SCORE_INIT;

    pExaPixmap->area = NULL;

    pExaPixmap->sys_ptr = pPixmap->devPrivate.ptr;
    pExaPixmap->sys_pitch = pPixmap->devKind;

    pExaPixmap->fb_ptr = NULL;
    if (pExaScr->info->flags & EXA_OFFSCREEN_ALIGN_POT && w != 1)
	pExaPixmap->fb_pitch = (1 << (exaLog2(w - 1) + 1)) * bpp / 8;
    else
	pExaPixmap->fb_pitch = w * bpp / 8;
    pExaPixmap->fb_pitch = EXA_ALIGN(pExaPixmap->fb_pitch,
				     pExaScr->info->pixmapPitchAlign);
    pExaPixmap->fb_size = pExaPixmap->fb_pitch * h;

    if (pExaPixmap->fb_pitch > 32767) {
	fbDestroyPixmap(pPixmap);
	return NULL;
    }

    /* Set up damage tracking */
    pExaPixmap->pDamage = DamageCreate (NULL, NULL, DamageReportNone, TRUE,
					pScreen, pPixmap);

    if (pExaPixmap->pDamage == NULL) {
	fbDestroyPixmap (pPixmap);
	return NULL;
    }

    DamageRegister (&pPixmap->drawable, pExaPixmap->pDamage);
    DamageSetReportAfterOp (pExaPixmap->pDamage, TRUE);

    /* None of the pixmap bits are valid initially */
    REGION_NULL(pScreen, &pExaPixmap->validReg);

    return pPixmap;
}

/**
 * exaPixmapIsOffscreen() is used to determine if a pixmap is in offscreen
 * memory, meaning that acceleration could probably be done to it, and that it
 * will need to be wrapped by PrepareAccess()/FinishAccess() when accessing it
 * with the CPU.
 *
 * Note that except for UploadToScreen()/DownloadFromScreen() (which explicitly
 * deal with moving pixmaps in and out of system memory), EXA will give drivers
 * pixmaps as arguments for which exaPixmapIsOffscreen() is TRUE.
 *
 * @return TRUE if the given drawable is in framebuffer memory.
 */
Bool
exaPixmapIsOffscreen(PixmapPtr p)
{
    ScreenPtr	pScreen = p->drawable.pScreen;
    ExaScreenPriv(pScreen);

    /* If the devPrivate.ptr is NULL, it's offscreen but we've hidden the data.
     */
    if (p->devPrivate.ptr == NULL)
	return TRUE;

    return ((unsigned long) ((CARD8 *) p->devPrivate.ptr -
			     (CARD8 *) pExaScr->info->memoryBase) <
	    pExaScr->info->memorySize);
}

/**
 * exaDrawableIsOffscreen() is a convenience wrapper for exaPixmapIsOffscreen().
 */
Bool
exaDrawableIsOffscreen (DrawablePtr pDrawable)
{
    return exaPixmapIsOffscreen (exaGetDrawablePixmap (pDrawable));
}

/**
 * Returns the pixmap which backs a drawable, and the offsets to add to
 * coordinates to make them address the same bits in the backing drawable.
 */
PixmapPtr
exaGetOffscreenPixmap (DrawablePtr pDrawable, int *xp, int *yp)
{
    PixmapPtr	pPixmap = exaGetDrawablePixmap (pDrawable);

    exaGetDrawableDeltas (pDrawable, pPixmap, xp, yp);

    if (exaPixmapIsOffscreen (pPixmap))
	return pPixmap;
    else
	return NULL;
}

/**
 * exaPrepareAccess() is EXA's wrapper for the driver's PrepareAccess() handler.
 *
 * It deals with waiting for synchronization with the card, determining if
 * PrepareAccess() is necessary, and working around PrepareAccess() failure.
 */
void
exaPrepareAccess(DrawablePtr pDrawable, int index)
{
    ScreenPtr	    pScreen = pDrawable->pScreen;
    ExaScreenPriv  (pScreen);
    PixmapPtr	    pPixmap;

    pPixmap = exaGetDrawablePixmap (pDrawable);

    if (exaPixmapIsOffscreen (pPixmap))
	exaWaitSync (pDrawable->pScreen);
    else
	return;

    /* Unhide pixmap pointer */
    if (pPixmap->devPrivate.ptr == NULL) {
	ExaPixmapPriv (pPixmap);

	pPixmap->devPrivate.ptr = pExaPixmap->fb_ptr;
    }

    if (pExaScr->info->PrepareAccess == NULL)
	return;

    if (!(*pExaScr->info->PrepareAccess) (pPixmap, index)) {
	ExaPixmapPriv (pPixmap);
	if (pExaPixmap->score != EXA_PIXMAP_SCORE_PINNED)
	    FatalError("Driver failed PrepareAccess on a pinned pixmap\n");
	exaMoveOutPixmap (pPixmap);
    }
}

/**
 * exaFinishAccess() is EXA's wrapper for the driver's FinishAccess() handler.
 *
 * It deals with calling the driver's FinishAccess() only if necessary.
 */
void
exaFinishAccess(DrawablePtr pDrawable, int index)
{
    ScreenPtr	    pScreen = pDrawable->pScreen;
    ExaScreenPriv  (pScreen);
    PixmapPtr	    pPixmap;
    ExaPixmapPrivPtr pExaPixmap;

    pPixmap = exaGetDrawablePixmap (pDrawable);

    pExaPixmap = ExaGetPixmapPriv(pPixmap);

    /* Rehide pixmap pointer if we're doing that. */
    if (pExaPixmap != NULL && pExaScr->hideOffscreenPixmapData &&
	pExaPixmap->fb_ptr == pPixmap->devPrivate.ptr)
    {
	pPixmap->devPrivate.ptr = pExaPixmap->fb_ptr;
    }

    if (pExaScr->info->FinishAccess == NULL)
	return;

    if (!exaPixmapIsOffscreen (pPixmap))
	return;

    (*pExaScr->info->FinishAccess) (pPixmap, index);
}

/**
 * exaValidateGC() sets the ops to EXA's implementations, which may be
 * accelerated or may sync the card and fall back to fb.
 */
static void
exaValidateGC (GCPtr pGC, unsigned long changes, DrawablePtr pDrawable)
{
    /* fbValidateGC will do direct access to pixmaps if the tiling has changed.
     * Preempt fbValidateGC by doing its work and masking the change out, so
     * that we can do the Prepare/FinishAccess.
     */
#ifdef FB_24_32BIT
    if ((changes & GCTile) && fbGetRotatedPixmap(pGC)) {
	(*pGC->pScreen->DestroyPixmap) (fbGetRotatedPixmap(pGC));
	fbGetRotatedPixmap(pGC) = 0;
    }
	
    if (pGC->fillStyle == FillTiled) {
	PixmapPtr	pOldTile, pNewTile;

	pOldTile = pGC->tile.pixmap;
	if (pOldTile->drawable.bitsPerPixel != pDrawable->bitsPerPixel)
	{
	    pNewTile = fbGetRotatedPixmap(pGC);
	    if (!pNewTile ||
		pNewTile ->drawable.bitsPerPixel != pDrawable->bitsPerPixel)
	    {
		if (pNewTile)
		    (*pGC->pScreen->DestroyPixmap) (pNewTile);
		/* fb24_32ReformatTile will do direct access of a newly-
		 * allocated pixmap.  This isn't a problem yet, since we don't
		 * put pixmaps in FB until at least one accelerated EXA op.
		 */
		exaPrepareAccess(&pOldTile->drawable, EXA_PREPARE_SRC);
		pNewTile = fb24_32ReformatTile (pOldTile,
						pDrawable->bitsPerPixel);
		exaPixmapDirty(pNewTile, 0, 0, pNewTile->drawable.width, pNewTile->drawable.height);
		exaFinishAccess(&pOldTile->drawable, EXA_PREPARE_SRC);
	    }
	    if (pNewTile)
	    {
		fbGetRotatedPixmap(pGC) = pOldTile;
		pGC->tile.pixmap = pNewTile;
		changes |= GCTile;
	    }
	}
    }
#endif
    if (changes & GCTile) {
	if (!pGC->tileIsPixel && FbEvenTile (pGC->tile.pixmap->drawable.width *
					     pDrawable->bitsPerPixel))
	{
	    /* XXX This fixes corruption with tiled pixmaps, but may just be a
	     * workaround for broken drivers
	     */
	    exaMoveOutPixmap(pGC->tile.pixmap);
	    fbPadPixmap (pGC->tile.pixmap);
	    exaPixmapDirty(pGC->tile.pixmap, 0, 0,
			   pGC->tile.pixmap->drawable.width,
			   pGC->tile.pixmap->drawable.height);
	}
	/* Mask out the GCTile change notification, now that we've done FB's
	 * job for it.
	 */
	changes &= ~GCTile;
    }

    fbValidateGC (pGC, changes, pDrawable);

    pGC->ops = (GCOps *) &exaOps;
}

static GCFuncs	exaGCFuncs = {
    exaValidateGC,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip
};

/**
 * exaCreateGC makes a new GC and hooks up its funcs handler, so that
 * exaValidateGC() will get called.
 */
static int
exaCreateGC (GCPtr pGC)
{
    if (!fbCreateGC (pGC))
	return FALSE;

    pGC->funcs = &exaGCFuncs;

    return TRUE;
}

/**
 * exaCloseScreen() unwraps its wrapped screen functions and tears down EXA's
 * screen private, before calling down to the next CloseSccreen.
 */
static Bool
exaCloseScreen(int i, ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
#ifdef RENDER
    PictureScreenPtr	ps = GetPictureScreenIfSet(pScreen);
#endif

    pScreen->CreateGC = pExaScr->SavedCreateGC;
    pScreen->CloseScreen = pExaScr->SavedCloseScreen;
    pScreen->GetImage = pExaScr->SavedGetImage;
    pScreen->GetSpans = pExaScr->SavedGetSpans;
    pScreen->PaintWindowBackground = pExaScr->SavedPaintWindowBackground;
    pScreen->PaintWindowBorder = pExaScr->SavedPaintWindowBorder;
    pScreen->CreatePixmap = pExaScr->SavedCreatePixmap;
    pScreen->DestroyPixmap = pExaScr->SavedDestroyPixmap;
    pScreen->CopyWindow = pExaScr->SavedCopyWindow;
#ifdef RENDER
    if (ps) {
	ps->Composite = pExaScr->SavedComposite;
	ps->Glyphs = pExaScr->SavedGlyphs;
    }
#endif

    xfree (pExaScr);

    return (*pScreen->CloseScreen) (i, pScreen);
}

/**
 * This function allocates a driver structure for EXA drivers to fill in.  By
 * having EXA allocate the structure, the driver structure can be extended
 * without breaking ABI between EXA and the drivers.  The driver's
 * responsibility is to check beforehand that the EXA module has a matching
 * major number and sufficient minor.  Drivers are responsible for freeing the
 * driver structure using xfree().
 *
 * @return a newly allocated, zero-filled driver structure
 */
ExaDriverPtr
exaDriverAlloc(void)
{
    return xcalloc(1, sizeof(ExaDriverRec));
}

/**
 * @param pScreen screen being initialized
 * @param pScreenInfo EXA driver record
 *
 * exaDriverInit sets up EXA given a driver record filled in by the driver.
 * pScreenInfo should have been allocated by exaDriverAlloc().  See the
 * comments in _ExaDriver for what must be filled in and what is optional.
 *
 * @return TRUE if EXA was successfully initialized.
 */
Bool
exaDriverInit (ScreenPtr		pScreen,
               ExaDriverPtr	pScreenInfo)
{
    ExaScreenPrivPtr pExaScr;
#ifdef RENDER
    PictureScreenPtr ps;
#endif

    if (pScreenInfo->exa_major != EXA_VERSION_MAJOR ||
	pScreenInfo->exa_minor > EXA_VERSION_MINOR)
    {
	LogMessage(X_ERROR, "EXA(%d): driver's EXA version requirements "
		   "(%d.%d) are incompatible with EXA version (%d.%d)\n",
		   pScreen->myNum,
		   pScreenInfo->exa_major, pScreenInfo->exa_minor,
		   EXA_VERSION_MAJOR, EXA_VERSION_MINOR);
	return FALSE;
    }

#ifdef RENDER
    ps = GetPictureScreenIfSet(pScreen);
#endif
    if (exaGeneration != serverGeneration)
    {
	exaScreenPrivateIndex = AllocateScreenPrivateIndex();
	exaPixmapPrivateIndex = AllocatePixmapPrivateIndex();
	exaGeneration = serverGeneration;
    }

    pExaScr = xcalloc (sizeof (ExaScreenPrivRec), 1);

    if (!pExaScr) {
        LogMessage(X_WARNING, "EXA(%d): Failed to allocate screen private\n",
		   pScreen->myNum);
	return FALSE;
    }

    pExaScr->info = pScreenInfo;

    pScreen->devPrivates[exaScreenPrivateIndex].ptr = (pointer) pExaScr;

    pExaScr->migration = ExaMigrationAlways;

    exaDDXDriverInit(pScreen);

    /*
     * Replace various fb screen functions
     */
    pExaScr->SavedCloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = exaCloseScreen;

    pExaScr->SavedCreateGC = pScreen->CreateGC;
    pScreen->CreateGC = exaCreateGC;

    pExaScr->SavedGetImage = pScreen->GetImage;
    pScreen->GetImage = exaGetImage;

    pExaScr->SavedGetSpans = pScreen->GetSpans;
    pScreen->GetSpans = exaGetSpans;

    pExaScr->SavedCopyWindow = pScreen->CopyWindow;
    pScreen->CopyWindow = exaCopyWindow;

    pExaScr->SavedPaintWindowBackground = pScreen->PaintWindowBackground;
    pScreen->PaintWindowBackground = exaPaintWindow;

    pExaScr->SavedPaintWindowBorder = pScreen->PaintWindowBorder;
    pScreen->PaintWindowBorder = exaPaintWindow;

    pScreen->BackingStoreFuncs.SaveAreas = ExaCheckSaveAreas;
    pScreen->BackingStoreFuncs.RestoreAreas = ExaCheckRestoreAreas;
#ifdef RENDER
    if (ps) {
        pExaScr->SavedComposite = ps->Composite;
	ps->Composite = exaComposite;

	pExaScr->SavedRasterizeTrapezoid = ps->RasterizeTrapezoid;
	ps->RasterizeTrapezoid = exaRasterizeTrapezoid;

	pExaScr->SavedAddTriangles = ps->AddTriangles;
	ps->AddTriangles = exaAddTriangles;

	pExaScr->SavedGlyphs = ps->Glyphs;
	ps->Glyphs = exaGlyphs;
    }
#endif

#ifdef COMPOSITE
    miDisableCompositeWrapper(pScreen);
#endif

#ifdef MITSHM
    /* Re-register with the MI funcs, which don't allow shared pixmaps.
     * Shared pixmaps are almost always a performance loss for us, but this
     * still allows for SHM PutImage.
     */
    ShmRegisterFuncs(pScreen, NULL);
#endif
    /*
     * Hookup offscreen pixmaps
     */
    if ((pExaScr->info->flags & EXA_OFFSCREEN_PIXMAPS) &&
	pExaScr->info->offScreenBase < pExaScr->info->memorySize)
    {
	if (!AllocatePixmapPrivate(pScreen, exaPixmapPrivateIndex,
				   sizeof (ExaPixmapPrivRec))) {
            LogMessage(X_WARNING,
		       "EXA(%d): Failed to allocate pixmap private\n",
		       pScreen->myNum);
	    return FALSE;
        }
        pExaScr->SavedCreatePixmap = pScreen->CreatePixmap;
	pScreen->CreatePixmap = exaCreatePixmap;

        pExaScr->SavedDestroyPixmap = pScreen->DestroyPixmap;
	pScreen->DestroyPixmap = exaDestroyPixmap;

	LogMessage(X_INFO, "EXA(%d): Offscreen pixmap area of %d bytes\n",
		   pScreen->myNum,
		   pExaScr->info->memorySize - pExaScr->info->offScreenBase);
    }
    else
    {
        LogMessage(X_INFO, "EXA(%d): No offscreen pixmaps\n", pScreen->myNum);
	if (!AllocatePixmapPrivate(pScreen, exaPixmapPrivateIndex, 0))
	    return FALSE;
    }

    DBG_PIXMAP(("============== %ld < %ld\n", pExaScr->info->offScreenBase,
                pExaScr->info->memorySize));
    if (pExaScr->info->offScreenBase < pExaScr->info->memorySize) {
	if (!exaOffscreenInit (pScreen)) {
            LogMessage(X_WARNING, "EXA(%d): Offscreen pixmap setup failed\n",
                       pScreen->myNum);
            return FALSE;
        }
    }

    LogMessage(X_INFO, "EXA(%d): Driver registered support for the following"
	       " operations:\n", pScreen->myNum);
    assert(pScreenInfo->PrepareSolid != NULL);
    LogMessage(X_INFO, "        Solid\n");
    assert(pScreenInfo->PrepareCopy != NULL);
    LogMessage(X_INFO, "        Copy\n");
    if (pScreenInfo->PrepareComposite != NULL) {
	LogMessage(X_INFO, "        Composite (RENDER acceleration)\n");
    }
    if (pScreenInfo->UploadToScreen != NULL) {
	LogMessage(X_INFO, "        UploadToScreen\n");
    }
    if (pScreenInfo->DownloadFromScreen != NULL) {
	LogMessage(X_INFO, "        DownloadFromScreen\n");
    }

    return TRUE;
}

/**
 * exaDriverFini tears down EXA on a given screen.
 *
 * @param pScreen screen being torn down.
 */
void
exaDriverFini (ScreenPtr pScreen)
{
    /*right now does nothing*/
}

/**
 * exaMarkSync() should be called after any asynchronous drawing by the hardware.
 *
 * @param pScreen screen which drawing occurred on
 *
 * exaMarkSync() sets a flag to indicate that some asynchronous drawing has
 * happened and a WaitSync() will be necessary before relying on the contents of
 * offscreen memory from the CPU's perspective.  It also calls an optional
 * driver MarkSync() callback, the return value of which may be used to do partial
 * synchronization with the hardware in the future.
 */
void exaMarkSync(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);

    pExaScr->info->needsSync = TRUE;
    if (pExaScr->info->MarkSync != NULL) {
        pExaScr->info->lastMarker = (*pExaScr->info->MarkSync)(pScreen);
    }
}

/**
 * exaWaitSync() ensures that all drawing has been completed.
 *
 * @param pScreen screen being synchronized.
 *
 * Calls down into the driver to ensure that all previous drawing has completed.
 * It should always be called before relying on the framebuffer contents
 * reflecting previous drawing, from a CPU perspective.
 */
void exaWaitSync(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);

    if (pExaScr->info->needsSync && !pExaScr->swappedOut) {
        (*pExaScr->info->WaitMarker)(pScreen, pExaScr->info->lastMarker);
        pExaScr->info->needsSync = FALSE;
    }
}
