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

#ifdef HAVE_CONFIG_H
#include <xorg-config.h>
#endif
#include "exa_priv.h"
#include <X11/fonts/fontstruct.h>
#include "dixfontstr.h"
#include "xf86str.h"
#include "xf86.h"
#include "exa.h"
#include "cw.h"

static int exaGeneration;
int exaScreenPrivateIndex;
int exaPixmapPrivateIndex;

/* Returns the offset (in bytes) within the framebuffer of the beginning of the
 * given pixmap.  May need to be extended in the future if we grow support for
 * having multiple card-accessible areas at different offsets.
 */
unsigned long
exaGetPixmapOffset(PixmapPtr pPix)
{
    ExaScreenPriv (pPix->drawable.pScreen);

    return ((unsigned long)pPix->devPrivate.ptr -
	(unsigned long)pExaScr->info->card.memoryBase);
}

/* Returns the pitch in bytes of the given pixmap. */
unsigned long
exaGetPixmapPitch(PixmapPtr pPix)
{
    return pPix->devKind;
}

/* Returns the size in bytes of the given pixmap in
 * video memory. Only valid when the vram storage has been
 * allocated
 */
unsigned long
exaGetPixmapSize(PixmapPtr pPix)
{
    ExaPixmapPrivPtr pExaPixmap;

    pExaPixmap = ExaGetPixmapPriv(pPix);
    if (pExaPixmap != NULL)
	return pExaPixmap->size;
    return 0;
}

PixmapPtr
exaGetDrawablePixmap(DrawablePtr pDrawable)
{
     if (pDrawable->type == DRAWABLE_WINDOW)
	return pDrawable->pScreen->GetWindowPixmap ((WindowPtr) pDrawable);
     else
	return (PixmapPtr) pDrawable;
}	

void
exaDrawableDirty (DrawablePtr pDrawable)
{
    ExaPixmapPrivPtr pExaPixmap;

    pExaPixmap = ExaGetPixmapPriv(exaGetDrawablePixmap (pDrawable));
    if (pExaPixmap != NULL)
	pExaPixmap->dirty = TRUE;
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
	    pPixmap->devPrivate = pExaPixmap->devPrivate;
	    pPixmap->devKind = pExaPixmap->devKind;
	}
    }
    return fbDestroyPixmap (pPixmap);
}

static PixmapPtr
exaCreatePixmap(ScreenPtr pScreen, int w, int h, int depth)
{
    PixmapPtr		pPixmap;
    ExaPixmapPrivPtr	pExaPixmap;
    int			bpp;
    ScrnInfoPtr pScrn = XF86SCRNINFO(pScreen);
    ExaScreenPriv(pScreen);

    if (w > 32767 || h > 32767)
	return NullPixmap;

    if (!pScrn->vtSema || pExaScr->swappedOut) {
        pPixmap = pExaScr->SavedCreatePixmap(pScreen, w, h, depth);
    } else {
        bpp = BitsPerPixel (depth);
        if (bpp == 32 && depth == 24)
        {
            int format;
            for (format = 0; format < MAXFORMATS && pScrn->formats[format].depth; ++format)
                if (pScrn->formats[format].depth == 24)
                {
                    bpp = pScrn->formats[format].bitsPerPixel;
                    break;
                }
        }

        pPixmap = fbCreatePixmapBpp (pScreen, w, h, depth, bpp);
    }
    if (!pPixmap)
	return NULL;
    pExaPixmap = ExaGetPixmapPriv(pPixmap);

    /* Glyphs have w/h equal to zero, and may not be migrated. See exaGlyphs. */
    if (!w || !h)
	pExaPixmap->score = EXA_PIXMAP_SCORE_PINNED;
    else
	pExaPixmap->score = EXA_PIXMAP_SCORE_INIT;

    pExaPixmap->area = NULL;
    pExaPixmap->dirty = FALSE;

    return pPixmap;
}

Bool
exaPixmapIsOffscreen(PixmapPtr p)
{
    ScreenPtr	pScreen = p->drawable.pScreen;
    ExaScreenPriv(pScreen);

    return ((unsigned long) ((CARD8 *) p->devPrivate.ptr -
			     (CARD8 *) pExaScr->info->card.memoryBase) <
	    pExaScr->info->card.memorySize);
}

Bool
exaDrawableIsOffscreen (DrawablePtr pDrawable)
{
    return exaPixmapIsOffscreen (exaGetDrawablePixmap (pDrawable));
}

PixmapPtr
exaGetOffscreenPixmap (DrawablePtr pDrawable, int *xp, int *yp)
{
    PixmapPtr	pPixmap;
    int		x, y;

    if (pDrawable->type == DRAWABLE_WINDOW) {
	pPixmap = (*pDrawable->pScreen->GetWindowPixmap) ((WindowPtr) pDrawable);
#ifdef COMPOSITE
	x = -pPixmap->screen_x;
	y = -pPixmap->screen_y;
#else
	x = 0;
	y = 0;
#endif
    }
    else
    {
	pPixmap = (PixmapPtr) pDrawable;
	x = 0;
	y = 0;
    }
    *xp = x;
    *yp = y;
    if (exaPixmapIsOffscreen (pPixmap))
	return pPixmap;
    else
	return NULL;
}

void
exaPrepareAccess(DrawablePtr pDrawable, int index)
{
    ScreenPtr	    pScreen = pDrawable->pScreen;
    ExaScreenPriv  (pScreen);
    PixmapPtr	    pPixmap;

    pPixmap = exaGetDrawablePixmap (pDrawable);

    if (index == EXA_PREPARE_DEST)
	exaDrawableDirty (pDrawable);
    if (exaPixmapIsOffscreen (pPixmap))
	exaWaitSync (pDrawable->pScreen);
    else
	return;

    if (pExaScr->info->accel.PrepareAccess == NULL)
	return;

    if (!(*pExaScr->info->accel.PrepareAccess) (pPixmap, index)) {
	ExaPixmapPriv (pPixmap);
	if (pExaPixmap->score != EXA_PIXMAP_SCORE_PINNED)
	    FatalError("Driver failed PrepareAccess on a pinned pixmap\n");
	exaMoveOutPixmap (pPixmap);
    }
}

void
exaFinishAccess(DrawablePtr pDrawable, int index)
{
    ScreenPtr	    pScreen = pDrawable->pScreen;
    ExaScreenPriv  (pScreen);
    PixmapPtr	    pPixmap;

    if (pExaScr->info->accel.FinishAccess == NULL)
	return;

    pPixmap = exaGetDrawablePixmap (pDrawable);
    if (!exaPixmapIsOffscreen (pPixmap))
	return;

    (*pExaScr->info->accel.FinishAccess) (pPixmap, index);
}

static void
exaValidateGC (GCPtr pGC, Mask changes, DrawablePtr pDrawable)
{
    fbValidateGC (pGC, changes, pDrawable);

    if (exaDrawableIsOffscreen (pDrawable))
	pGC->ops = (GCOps *) &exaOps;
    else
	pGC->ops = (GCOps *) &exaAsyncPixmapGCOps;
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

static int
exaCreateGC (GCPtr pGC)
{
    if (!fbCreateGC (pGC))
	return FALSE;

    pGC->funcs = &exaGCFuncs;

    return TRUE;
}

static Bool
exaCloseScreen(int i, ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
#ifdef RENDER
    PictureScreenPtr	ps = GetPictureScreenIfSet(pScreen);
#endif
    ScrnInfoPtr pScrn = XF86SCRNINFO(pScreen);

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
    if (pExaScr->wrappedEnableDisableFB)
	pScrn->EnableDisableFBAccess = pExaScr->SavedEnableDisableFBAccess;

    xfree (pExaScr);

    return (*pScreen->CloseScreen) (i, pScreen);
}

Bool
exaDriverInit (ScreenPtr		pScreen,
               ExaDriverPtr	pScreenInfo)
{
    /* Do NOT use XF86SCRNINFO macro here!! */
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    ExaScreenPrivPtr pExaScr;

#ifdef RENDER
    PictureScreenPtr	ps = GetPictureScreenIfSet(pScreen);
#endif
    if (exaGeneration != serverGeneration)
    {
	exaScreenPrivateIndex = AllocateScreenPrivateIndex();
	exaPixmapPrivateIndex = AllocatePixmapPrivateIndex();
	exaGeneration = serverGeneration;
    }

    pExaScr = xcalloc (sizeof (ExaScreenPrivRec), 1);

    if (!pExaScr) {
        xf86DrvMsg(pScreen->myNum, X_WARNING,
                   "EXA: Failed to allocate screen private\n");
	return FALSE;
    }

    pExaScr->info = pScreenInfo;

    pScreen->devPrivates[exaScreenPrivateIndex].ptr = (pointer) pExaScr;

    /*
     * Replace various fb screen functions
     */
    pExaScr->SavedCloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = exaCloseScreen;

    pExaScr->SavedCreateGC = pScreen->CreateGC;
    pScreen->CreateGC = exaCreateGC;

    pExaScr->SavedGetImage = pScreen->GetImage;
    pScreen->GetImage = ExaCheckGetImage;

    pExaScr->SavedGetSpans = pScreen->GetSpans;
    pScreen->GetSpans = ExaCheckGetSpans;

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

	pExaScr->SavedGlyphs = ps->Glyphs;
	ps->Glyphs = exaGlyphs;
    }
#endif

    miDisableCompositeWrapper(pScreen);

    /*
     * Hookup offscreen pixmaps
     */
    if ((pExaScr->info->card.flags & EXA_OFFSCREEN_PIXMAPS) &&
	pExaScr->info->card.offScreenBase < pExaScr->info->card.memorySize)
    {
	if (!AllocatePixmapPrivate(pScreen, exaPixmapPrivateIndex,
				   sizeof (ExaPixmapPrivRec))) {
            xf86DrvMsg(pScreen->myNum, X_WARNING,
                       "EXA: Failed to allocate pixmap private\n");
	    return FALSE;
        }
        pExaScr->SavedCreatePixmap = pScreen->CreatePixmap;
	pScreen->CreatePixmap = exaCreatePixmap;

        pExaScr->SavedDestroyPixmap = pScreen->DestroyPixmap;
	pScreen->DestroyPixmap = exaDestroyPixmap;
    }
    else
    {
        xf86DrvMsg(pScreen->myNum, X_INFO, "EXA: No offscreen pixmaps\n");
	if (!AllocatePixmapPrivate(pScreen, exaPixmapPrivateIndex, 0))
	    return FALSE;
    }

    DBG_PIXMAP(("============== %ld < %ld\n", pExaScr->info->card.offScreenBase,
                pExaScr->info->card.memorySize));
    if (pExaScr->info->card.offScreenBase < pExaScr->info->card.memorySize) {
	if (!exaOffscreenInit (pScreen)) {
            xf86DrvMsg(pScreen->myNum, X_WARNING,
                       "EXA: Offscreen pixmap setup failed\n");
            return FALSE;
        }

	pExaScr->SavedEnableDisableFBAccess = pScrn->EnableDisableFBAccess;
	pScrn->EnableDisableFBAccess = exaEnableDisableFBAccess;
	pExaScr->wrappedEnableDisableFB = TRUE;
    }

    return TRUE;
}

void
exaDriverFini (ScreenPtr pScreen)
{
    /*right now does nothing*/
}

void exaMarkSync(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
    ExaCardInfoPtr card = &(pExaScr->info->card);

    card->needsSync = TRUE;
    if (pExaScr->info->accel.MarkSync != NULL) {
        card->lastMarker = (*pExaScr->info->accel.MarkSync)(pScreen);
    }
}

void exaWaitSync(ScreenPtr pScreen)
{
    ExaScreenPriv(pScreen);
    ExaCardInfoPtr card = &(pExaScr->info->card);
    ScrnInfoPtr pScrn = XF86SCRNINFO(pScreen);

    if (card->needsSync && pScrn->vtSema) {
        (*pExaScr->info->accel.WaitMarker)(pScreen, card->lastMarker);
        card->needsSync = FALSE;
    }
}

unsigned int exaGetVersion(void)
{
    return EXA_VERSION;
}

#ifdef XFree86LOADER
static MODULESETUPPROTO(exaSetup);


static const OptionInfoRec EXAOptions[] = {
    { -1,				NULL,
      OPTV_NONE,	{0}, FALSE }
};

/*ARGSUSED*/
static const OptionInfoRec *
EXAAvailableOptions(void *unused)
{
    return (EXAOptions);
}

static XF86ModuleVersionInfo exaVersRec =
{
	"exa",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	1, 2, 0,
	ABI_CLASS_VIDEODRV,		/* requires the video driver ABI */
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_NONE,
	{0,0,0,0}
};

XF86ModuleData exaModuleData = { &exaVersRec, exaSetup, NULL };

ModuleInfoRec EXA = {
    1,
    "EXA",
    NULL,
    0,
    EXAAvailableOptions,
};

/*ARGSUSED*/
static pointer
exaSetup(pointer Module, pointer Options, int *ErrorMajor, int *ErrorMinor)
{
    static Bool Initialised = FALSE;

    if (!Initialised) {
	Initialised = TRUE;
#ifndef REMOVE_LOADER_CHECK_MODULE_INFO
	if (xf86LoaderCheckSymbol("xf86AddModuleInfo"))
#endif
	xf86AddModuleInfo(&EXA, Module);
    }

    return (pointer)TRUE;
}
#endif
