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
#include "exaPriv.h"
#include <X11/fonts/fontstruct.h>
#include "dixfontstr.h"
#include "xf86str.h"
#include "xf86.h"
#include "exa.h"
#include "cw.h"

#if DEBUG_MIGRATE
#define DBG_MIGRATE(a) ErrorF a
#else
#define DBG_MIGRATE(a)
#endif
#if DEBUG_PIXMAP
#define DBG_PIXMAP(a) ErrorF a
#else
#define DBG_PIXMAP(a)
#endif
#define STRACE
#define TRACE

static int exaGeneration;
int exaScreenPrivateIndex;
int exaPixmapPrivateIndex;

#define EXA_PIXMAP_SCORE_MOVE_IN    10
#define EXA_PIXMAP_SCORE_MAX	    20
#define EXA_PIXMAP_SCORE_MOVE_OUT   -10
#define EXA_PIXMAP_SCORE_MIN	    -20
#define EXA_PIXMAP_SCORE_PINNED	    1000
#define EXA_PIXMAP_SCORE_INIT	    1001

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

void
exaDrawableDirty (DrawablePtr pDrawable)
{
    PixmapPtr pPixmap;
    ExaPixmapPrivPtr pExaPixmap;

    if (pDrawable->type == DRAWABLE_WINDOW)
	pPixmap = (*pDrawable->pScreen->GetWindowPixmap)((WindowPtr) pDrawable);
    else
	pPixmap = (PixmapPtr)pDrawable;

    pExaPixmap = ExaGetPixmapPriv(pPixmap);
    if (pExaPixmap != NULL)
	pExaPixmap->dirty = TRUE;
}

static void
exaPixmapSave (ScreenPtr pScreen, ExaOffscreenArea *area)
{
    PixmapPtr pPixmap = area->privData;
    ExaScreenPriv (pScreen);
    ExaPixmapPriv(pPixmap);
    int dst_pitch, src_pitch, bytes;
    char *dst, *src;
    int i;

    DBG_MIGRATE (("Save %p (%p) (%dx%d)\n",
		  (void*)pPixmap->drawable.id,
		  (void*)(ExaGetPixmapPriv(pPixmap)->area ?
                          ExaGetPixmapPriv(pPixmap)->area->offset : 0),
		  pPixmap->drawable.width,
		  pPixmap->drawable.height));

    src_pitch = pPixmap->devKind;
    dst_pitch = pExaPixmap->devKind;

    src = pPixmap->devPrivate.ptr;
    dst = pExaPixmap->devPrivate.ptr;

    if (pExaPixmap->dirty) {
        if (pExaScr->info->accel.DownloadFromScreen &&
	    (*pExaScr->info->accel.DownloadFromScreen) (pPixmap,
							pPixmap->drawable.x,
							pPixmap->drawable.y,
							pPixmap->drawable.width,
							pPixmap->drawable.height,
							dst,
							dst_pitch)) {

        } else {
	    exaWaitSync (pPixmap->drawable.pScreen);

	    bytes = src_pitch < dst_pitch ? src_pitch : dst_pitch;

	    i = pPixmap->drawable.height;
	    while (i--) {
		memcpy (dst, src, bytes);
		dst += dst_pitch;
		src += src_pitch;
	    }
	}
    }

    pPixmap->devKind = dst_pitch;
    pPixmap->devPrivate.ptr = pExaPixmap->devPrivate.ptr;
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pExaPixmap->area = NULL;
    /* Mark it dirty now, to say that there is important data in the
     * system-memory copy.
     */
    pExaPixmap->dirty = TRUE;
}

static int
exaLog2(int val)
{
    int bits;

    if (!val)
	return 0;
    for (bits = 0; val != 0; bits++)
	val >>= 1;
    return bits - 1;
}

static Bool
exaPixmapAllocArea (PixmapPtr pPixmap)
{
    ScreenPtr	pScreen = pPixmap->drawable.pScreen;
    ExaScreenPriv (pScreen);
    ExaPixmapPriv (pPixmap);
    int		bpp = pPixmap->drawable.bitsPerPixel;
    CARD16	h = pPixmap->drawable.height;
    CARD16	w = pPixmap->drawable.width;
    int		pitch;

    if (pExaScr->info->card.flags & EXA_OFFSCREEN_ALIGN_POT && w != 1)
	w = 1 << (exaLog2(w - 1) + 1);
    pitch = (w * bpp / 8) + (pExaScr->info->card.pixmapPitchAlign - 1);
    pitch -= pitch % pExaScr->info->card.pixmapPitchAlign;

    pExaPixmap->size = pitch * h;
    pExaPixmap->devKind = pPixmap->devKind;
    pExaPixmap->devPrivate = pPixmap->devPrivate;
    pExaPixmap->area = exaOffscreenAlloc (pScreen, pitch * h,
                                          pExaScr->info->card.pixmapOffsetAlign,
                                          FALSE,
                                          exaPixmapSave, (pointer) pPixmap);
    if (!pExaPixmap->area)
	return FALSE;

    DBG_PIXMAP(("++ 0x%lx (0x%x) (%dx%d)\n", pPixmap->drawable.id,
                (ExaGetPixmapPriv(pPixmap)->area ?
		 ExaGetPixmapPriv(pPixmap)->area->offset : 0),
		pPixmap->drawable.width,
		pPixmap->drawable.height));
    pPixmap->devKind = pitch;

    pPixmap->devPrivate.ptr = (pointer) ((CARD8 *) pExaScr->info->card.memoryBase + pExaPixmap->area->offset);
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    return TRUE;
}

void
exaMoveInPixmap (PixmapPtr pPixmap)
{
    ScreenPtr	pScreen = pPixmap->drawable.pScreen;
    ExaScreenPriv (pScreen);
    ExaPixmapPriv (pPixmap);
    int dst_pitch, src_pitch, bytes;
    char *dst, *src;
    int i;

    DBG_MIGRATE (("-> 0x%lx (0x%x) (%dx%d)\n", pPixmap->drawable.id,
		  (ExaGetPixmapPriv(pPixmap)->area ?
                   ExaGetPixmapPriv(pPixmap)->area->offset : 0),
		  pPixmap->drawable.width,
		  pPixmap->drawable.height));

    src = pPixmap->devPrivate.ptr;
    src_pitch = pPixmap->devKind;

    if (!exaPixmapAllocArea (pPixmap)) {
	DBG_MIGRATE (("failed to allocate fb for pixmap %p (%dx%dx%d)\n",
		      (pointer)pPixmap,
		      pPixmap->drawable.width, pPixmap->drawable.height,
		      pPixmap->drawable.bitsPerPixel));
	return;
    }

    /* If the "dirty" flag has never been set on the in-memory pixmap, then
     * nothing has been written to it, so the contents are undefined and we can
     * avoid the upload.
     */
    if (!pExaPixmap->dirty) {
	DBG_MIGRATE(("saved upload of %dx%d\n", pPixmap->drawable.width,
		     pPixmap->drawable.height));
	return;
    }

    pExaPixmap->dirty = FALSE;

    if (pExaScr->info->accel.UploadToScreen)
    {
	if (pExaScr->info->accel.UploadToScreen(pPixmap, 0, 0,
						pPixmap->drawable.width,
						pPixmap->drawable.height,
						src, src_pitch))
	    return;
    }

    dst = pPixmap->devPrivate.ptr;
    dst_pitch = pPixmap->devKind;

    bytes = src_pitch < dst_pitch ? src_pitch : dst_pitch;

    exaWaitSync (pPixmap->drawable.pScreen);

    i = pPixmap->drawable.height;
    DBG_PIXMAP(("dst = %p, src = %p,(%d, %d) height = %d, mem_base = %p, offset = %d\n",
                dst, src, dst_pitch, src_pitch,
                i, pExaScr->info->card.memoryBase, ExaGetPixmapPriv(pPixmap)->area->offset));

    while (i--) {
	memcpy (dst, src, bytes);
	dst += dst_pitch;
	src += src_pitch;
    }
}

static void
exaMoveOutPixmap (PixmapPtr pPixmap)
{
    ExaPixmapPriv (pPixmap);
    ExaOffscreenArea *area = pExaPixmap->area;

    DBG_MIGRATE (("<- 0x%p (0x%p) (%dx%d)\n",
		  (void*)pPixmap->drawable.id,
		  (void*)(ExaGetPixmapPriv(pPixmap)->area ?
                          ExaGetPixmapPriv(pPixmap)->area->offset : 0),
		  pPixmap->drawable.width,
		  pPixmap->drawable.height));
    STRACE;
    if (area)
    {
	exaPixmapSave (pPixmap->drawable.pScreen, area);
	exaOffscreenFree (pPixmap->drawable.pScreen, area);
    }
}

void
exaDrawableUseScreen(DrawablePtr pDrawable)
{
    PixmapPtr pPixmap;

    if (pDrawable->type == DRAWABLE_WINDOW)
	pPixmap = (*pDrawable->pScreen->GetWindowPixmap) ((WindowPtr) pDrawable);
    else
	pPixmap = (PixmapPtr) pDrawable;

    exaPixmapUseScreen (pPixmap);
}

void
exaDrawableUseMemory(DrawablePtr pDrawable)
{
    PixmapPtr pPixmap;

    if (pDrawable->type == DRAWABLE_WINDOW)
	pPixmap = (*pDrawable->pScreen->GetWindowPixmap) ((WindowPtr) pDrawable);
    else
	pPixmap = (PixmapPtr) pDrawable;

    exaPixmapUseMemory (pPixmap);
}

void
exaPixmapUseScreen (PixmapPtr pPixmap)
{
    ExaPixmapPriv (pPixmap);

    STRACE;

    if (pExaPixmap == NULL) {
	DBG_MIGRATE(("UseScreen: ignoring exa-uncontrolled pixmap %p (%s)\n",
		     (pointer)pPixmap,
		     exaPixmapIsOffscreen(pPixmap) ? "s" : "m"));
	return;
    }

    if (pExaPixmap->score == EXA_PIXMAP_SCORE_PINNED) {
	DBG_MIGRATE(("UseScreen: not migrating pinned pixmap %p\n",
		     (pointer)pPixmap));
	return;
    }

    DBG_MIGRATE(("UseScreen %p score %d\n",
		 (pointer)pPixmap, pExaPixmap->score));

    if (pExaPixmap->score == EXA_PIXMAP_SCORE_INIT) {
	exaMoveInPixmap(pPixmap);
	pExaPixmap->score = 0;
    }

    if (pExaPixmap->score < EXA_PIXMAP_SCORE_MAX)
	pExaPixmap->score++;

    if (pExaPixmap->score >= EXA_PIXMAP_SCORE_MOVE_IN &&
	!exaPixmapIsOffscreen(pPixmap))
    {
	exaMoveInPixmap (pPixmap);
    }

    ExaOffscreenMarkUsed (pPixmap);
}

void
exaPixmapUseMemory (PixmapPtr pPixmap)
{
    ExaPixmapPriv (pPixmap);

    if (pExaPixmap == NULL) {
	DBG_MIGRATE(("UseMem: ignoring exa-uncontrolled pixmap %p (%s)\n",
		     (pointer)pPixmap,
		     exaPixmapIsOffscreen(pPixmap) ? "s" : "m"));
	return;
    }

    DBG_MIGRATE(("UseMem: %p score %d\n", (pointer)pPixmap, pExaPixmap->score));

    if (pExaPixmap->score == EXA_PIXMAP_SCORE_PINNED)
	return;

    if (pExaPixmap->score == EXA_PIXMAP_SCORE_INIT)
	pExaPixmap->score = 0;

    if (pExaPixmap->score > EXA_PIXMAP_SCORE_MIN)
	pExaPixmap->score--;

    if (pExaPixmap->score <= EXA_PIXMAP_SCORE_MOVE_OUT && pExaPixmap->area)
	exaMoveOutPixmap (pPixmap);
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

    STRACE;
    return ((unsigned long) ((CARD8 *) p->devPrivate.ptr -
			     (CARD8 *) pExaScr->info->card.memoryBase) <
	    pExaScr->info->card.memorySize);
}

PixmapPtr
exaGetOffscreenPixmap (DrawablePtr pDrawable, int *xp, int *yp)
{
    PixmapPtr	pPixmap;
    int		x, y;

    STRACE;
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

Bool
exaDrawableIsOffscreen (DrawablePtr pDrawable)
{
    PixmapPtr	pPixmap;
    STRACE;

    if (pDrawable->type == DRAWABLE_WINDOW)
	pPixmap = (*pDrawable->pScreen->GetWindowPixmap) ((WindowPtr) pDrawable);
    else
	pPixmap = (PixmapPtr) pDrawable;
    return exaPixmapIsOffscreen (pPixmap);
}

void
exaPrepareAccess(DrawablePtr pDrawable, int index)
{
    ScreenPtr	    pScreen = pDrawable->pScreen;
    ExaScreenPriv  (pScreen);
    PixmapPtr	    pPixmap;
    STRACE;

    if (pDrawable->type == DRAWABLE_WINDOW)
	pPixmap = (*pDrawable->pScreen->GetWindowPixmap) ((WindowPtr) pDrawable);
    else
	pPixmap = (PixmapPtr) pDrawable;

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
    STRACE;

    if (pExaScr->info->accel.FinishAccess == NULL)
	return;

    if (pDrawable->type == DRAWABLE_WINDOW)
	pPixmap = (*pDrawable->pScreen->GetWindowPixmap) ((WindowPtr) pDrawable);
    else
	pPixmap = (PixmapPtr) pDrawable;
    if (!exaPixmapIsOffscreen (pPixmap))
	return;

    (*pExaScr->info->accel.FinishAccess) (pPixmap, index);
}


static void
exaFillSpans(DrawablePtr pDrawable, GCPtr pGC, int n,
	     DDXPointPtr ppt, int *pwidth, int fSorted)
{
    ScreenPtr	    pScreen = pDrawable->pScreen;
    ExaScreenPriv (pScreen);
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    PixmapPtr	    pPixmap;
    BoxPtr	    pextent, pbox;
    int		    nbox;
    int		    extentX1, extentX2, extentY1, extentY2;
    int		    fullX1, fullX2, fullY1;
    int		    partX1, partX2;
    int		    off_x, off_y;


    ScrnInfoPtr pScrn = XF86SCRNINFO(pScreen);
    if (!pScrn->vtSema) {
        ExaCheckFillSpans(pDrawable, pGC, n, ppt, pwidth, fSorted);
        return;
    }

    STRACE;
    if (pGC->fillStyle != FillSolid ||
	pDrawable->width > pExaScr->info->card.maxX ||
	pDrawable->height > pExaScr->info->card.maxY ||
	!(pPixmap = exaGetOffscreenPixmap (pDrawable, &off_x, &off_y)) ||
	!(*pExaScr->info->accel.PrepareSolid) (pPixmap,
                                               pGC->alu,
                                               pGC->planemask,
                                               pGC->fgPixel))
    {
	ExaCheckFillSpans (pDrawable, pGC, n, ppt, pwidth, fSorted);
	return;
    }

    pextent = REGION_EXTENTS(pGC->pScreen, pClip);
    extentX1 = pextent->x1;
    extentY1 = pextent->y1;
    extentX2 = pextent->x2;
    extentY2 = pextent->y2;
    while (n--)
    {
	fullX1 = ppt->x;
	fullY1 = ppt->y;
	fullX2 = fullX1 + (int) *pwidth;
	ppt++;
	pwidth++;

	if (fullY1 < extentY1 || extentY2 <= fullY1)
	    continue;

	if (fullX1 < extentX1)
	    fullX1 = extentX1;

	if (fullX2 > extentX2)
	    fullX2 = extentX2;

	if (fullX1 >= fullX2)
	    continue;

	nbox = REGION_NUM_RECTS (pClip);
	if (nbox == 1)
	{
	    (*pExaScr->info->accel.Solid) (pPixmap,
                                           fullX1 + off_x, fullY1 + off_y,
                                           fullX2 + off_x, fullY1 + 1 + off_y);
	}
	else
	{
	    pbox = REGION_RECTS(pClip);
	    while(nbox--)
	    {
		if (pbox->y1 <= fullY1 && fullY1 < pbox->y2)
		{
		    partX1 = pbox->x1;
		    if (partX1 < fullX1)
			partX1 = fullX1;
		    partX2 = pbox->x2;
		    if (partX2 > fullX2)
			partX2 = fullX2;
		    if (partX2 > partX1)
			(*pExaScr->info->accel.Solid) (pPixmap,
                                                       partX1 + off_x, fullY1 + off_y,
                                                       partX2 + off_x, fullY1 + 1 + off_y);
		}
		pbox++;
	    }
	}
    }
    (*pExaScr->info->accel.DoneSolid) (pPixmap);
    exaDrawableDirty (pDrawable);
    exaMarkSync(pScreen);
}

void
exaCopyNtoN (DrawablePtr    pSrcDrawable,
	     DrawablePtr    pDstDrawable,
	     GCPtr	    pGC,
	     BoxPtr	    pbox,
	     int	    nbox,
	     int	    dx,
	     int	    dy,
	     Bool	    reverse,
	     Bool	    upsidedown,
	     Pixel	    bitplane,
	     void	    *closure)
{
    ExaScreenPriv (pDstDrawable->pScreen);
    PixmapPtr pSrcPixmap, pDstPixmap;
    int	    src_off_x, src_off_y;
    int	    dst_off_x, dst_off_y;
    STRACE;

    /* Respect maxX/maxY in a trivial way: don't set up drawing when we might
     * violate the limits.  The proper solution would be a temporary pixmap
     * adjusted so that the drawing happened within limits.
     */
    if (pSrcDrawable->width > pExaScr->info->card.maxX ||
	pSrcDrawable->height > pExaScr->info->card.maxY ||
	pDstDrawable->width > pExaScr->info->card.maxX ||
	pDstDrawable->height > pExaScr->info->card.maxY)
    {
	exaDrawableUseMemory (pSrcDrawable);
	exaDrawableUseMemory (pDstDrawable);
	goto fallback;
    }

    /* If either drawable is already in framebuffer, try to get both of them
     * there.  Otherwise, be happy with where they are.
     */
    if (exaDrawableIsOffscreen(pDstDrawable) ||
	exaDrawableIsOffscreen(pSrcDrawable))
    {
	exaDrawableUseScreen (pSrcDrawable);
	exaDrawableUseScreen (pDstDrawable);
    } else {
	exaDrawableUseMemory (pSrcDrawable);
	exaDrawableUseMemory (pDstDrawable);
    }

    if ((pSrcPixmap = exaGetOffscreenPixmap (pSrcDrawable, &src_off_x, &src_off_y)) &&
	(pDstPixmap = exaGetOffscreenPixmap (pDstDrawable, &dst_off_x, &dst_off_y)) &&
	(*pExaScr->info->accel.PrepareCopy) (pSrcPixmap,
                                             pDstPixmap,
                                             dx,
                                             dy,
                                             pGC ? pGC->alu : GXcopy,
                                             pGC ? pGC->planemask : FB_ALLONES))
    {
	while (nbox--)
	{
	    (*pExaScr->info->accel.Copy) (pDstPixmap,
                                          pbox->x1 + dx + src_off_x,
                                          pbox->y1 + dy + src_off_y,
                                          pbox->x1 + dst_off_x, pbox->y1 + dst_off_y,
                                          pbox->x2 - pbox->x1,
                                          pbox->y2 - pbox->y1);
	    pbox++;
	}
	(*pExaScr->info->accel.DoneCopy) (pDstPixmap);
	exaMarkSync(pDstDrawable->pScreen);
	exaDrawableDirty (pDstDrawable);
	return;
    }

fallback:
    EXA_FALLBACK(("from 0x%lx to 0x%lx\n", (long)pSrcDrawable,
		  (long)pDstDrawable));
    exaPrepareAccess (pDstDrawable, EXA_PREPARE_DEST);
    exaPrepareAccess (pSrcDrawable, EXA_PREPARE_SRC);
    fbCopyNtoN (pSrcDrawable, pDstDrawable, pGC,
		pbox, nbox, dx, dy, reverse, upsidedown,
		bitplane, closure);
    exaFinishAccess (pSrcDrawable, EXA_PREPARE_SRC);
    exaFinishAccess (pDstDrawable, EXA_PREPARE_DEST);
}

RegionPtr
exaCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	    int srcx, int srcy, int width, int height, int dstx, int dsty)
{
    ScrnInfoPtr pScrn = XF86SCRNINFO(pDstDrawable->pScreen);
    if (!pScrn->vtSema) {
        return  ExaCheckCopyArea(pSrcDrawable, pDstDrawable, pGC,
                                 srcx, srcy, width, height, dstx, dsty);
    }

    return  fbDoCopy (pSrcDrawable, pDstDrawable, pGC,
                      srcx, srcy, width, height,
                      dstx, dsty, exaCopyNtoN, 0, NULL);
}

static void
exaPolyFillRect(DrawablePtr pDrawable,
		GCPtr	    pGC,
		int	    nrect,
		xRectangle  *prect)
{
    ExaScreenPriv (pDrawable->pScreen);
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    ScrnInfoPtr     pScrn = XF86SCRNINFO(pDrawable->pScreen);
    PixmapPtr	    pPixmap;
    register BoxPtr pbox;
    BoxPtr	    pextent;
    int		    extentX1, extentX2, extentY1, extentY2;
    int		    fullX1, fullX2, fullY1, fullY2;
    int		    partX1, partX2, partY1, partY2;
    int		    xoff, yoff;
    int		    xorg, yorg;
    int		    n;

    STRACE;
    if (!pScrn->vtSema ||
        pGC->fillStyle != FillSolid ||
	pDrawable->width > pExaScr->info->card.maxX ||
	pDrawable->height > pExaScr->info->card.maxY ||
	!(pPixmap = exaGetOffscreenPixmap (pDrawable, &xoff, &yoff)) ||
	!(*pExaScr->info->accel.PrepareSolid) (pPixmap,
                                               pGC->alu,
                                               pGC->planemask,
                                               pGC->fgPixel))
    {
	ExaCheckPolyFillRect (pDrawable, pGC, nrect, prect);
	return;
    }

    xorg = pDrawable->x;
    yorg = pDrawable->y;

    pextent = REGION_EXTENTS(pGC->pScreen, pClip);
    extentX1 = pextent->x1;
    extentY1 = pextent->y1;
    extentX2 = pextent->x2;
    extentY2 = pextent->y2;
    while (nrect--)
    {
	fullX1 = prect->x + xorg;
	fullY1 = prect->y + yorg;
	fullX2 = fullX1 + (int) prect->width;
	fullY2 = fullY1 + (int) prect->height;
	prect++;

	if (fullX1 < extentX1)
	    fullX1 = extentX1;

	if (fullY1 < extentY1)
	    fullY1 = extentY1;

	if (fullX2 > extentX2)
	    fullX2 = extentX2;

	if (fullY2 > extentY2)
	    fullY2 = extentY2;

	if ((fullX1 >= fullX2) || (fullY1 >= fullY2))
	    continue;
	n = REGION_NUM_RECTS (pClip);
	if (n == 1)
	{
	    (*pExaScr->info->accel.Solid) (pPixmap,
                                           fullX1 + xoff, fullY1 + yoff,
                                           fullX2 + xoff, fullY2 + yoff);
	}
	else
	{
	    pbox = REGION_RECTS(pClip);
	    /*
	     * clip the rectangle to each box in the clip region
	     * this is logically equivalent to calling Intersect()
	     */
	    while(n--)
	    {
		partX1 = pbox->x1;
		if (partX1 < fullX1)
		    partX1 = fullX1;
		partY1 = pbox->y1;
		if (partY1 < fullY1)
		    partY1 = fullY1;
		partX2 = pbox->x2;
		if (partX2 > fullX2)
		    partX2 = fullX2;
		partY2 = pbox->y2;
		if (partY2 > fullY2)
		    partY2 = fullY2;

		pbox++;

		if (partX1 < partX2 && partY1 < partY2)
		    (*pExaScr->info->accel.Solid) (pPixmap,
                                                   partX1 + xoff, partY1 + yoff,
                                                   partX2 + xoff, partY2 + yoff);
	    }
	}
    }
    (*pExaScr->info->accel.DoneSolid) (pPixmap);
    exaDrawableDirty (pDrawable);
    exaMarkSync(pDrawable->pScreen);
}

static void
exaSolidBoxClipped (DrawablePtr	pDrawable,
		    RegionPtr	pClip,
		    FbBits	pm,
		    FbBits	fg,
		    int		x1,
		    int		y1,
		    int		x2,
		    int		y2)
{
    ExaScreenPriv (pDrawable->pScreen);
    ScrnInfoPtr pScrn = XF86SCRNINFO(pDrawable->pScreen);
    PixmapPtr   pPixmap;
    BoxPtr	pbox;
    int		nbox;
    int		xoff, yoff;
    int		partX1, partX2, partY1, partY2;

    STRACE;
    if (!pScrn->vtSema ||
	pDrawable->width > pExaScr->info->card.maxX ||
	pDrawable->height > pExaScr->info->card.maxY ||
        !(pPixmap = exaGetOffscreenPixmap (pDrawable, &xoff, &yoff)) ||
	!(*pExaScr->info->accel.PrepareSolid) (pPixmap, GXcopy, pm, fg))
    {
	EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
	exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
	fg = fbReplicatePixel (fg, pDrawable->bitsPerPixel);
	fbSolidBoxClipped (pDrawable, pClip, x1, y1, x2, y2,
			   fbAnd (GXcopy, fg, pm),
			   fbXor (GXcopy, fg, pm));
	exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
	return;
    }
    for (nbox = REGION_NUM_RECTS(pClip), pbox = REGION_RECTS(pClip);
	 nbox--;
	 pbox++)
    {
	partX1 = pbox->x1;
	if (partX1 < x1)
	    partX1 = x1;

	partX2 = pbox->x2;
	if (partX2 > x2)
	    partX2 = x2;

	if (partX2 <= partX1)
	    continue;

	partY1 = pbox->y1;
	if (partY1 < y1)
	    partY1 = y1;

	partY2 = pbox->y2;
	if (partY2 > y2)
	    partY2 = y2;

	if (partY2 <= partY1)
	    continue;

	(*pExaScr->info->accel.Solid) (pPixmap,
                                       partX1 + xoff, partY1 + yoff,
                                       partX2 + xoff, partY2 + yoff);
    }
    (*pExaScr->info->accel.DoneSolid) (pPixmap);
    exaDrawableDirty (pDrawable);
    exaMarkSync(pDrawable->pScreen);
}

static void
exaImageGlyphBlt (DrawablePtr	pDrawable,
		  GCPtr		pGC,
		  int		x,
		  int		y,
		  unsigned int	nglyph,
		  CharInfoPtr	*ppciInit,
		  pointer	pglyphBase)
{
    FbGCPrivPtr	    pPriv = fbGetGCPrivate(pGC);
    CharInfoPtr	    *ppci;
    CharInfoPtr	    pci;
    unsigned char   *pglyph;		/* pointer bits in glyph */
    int		    gWidth, gHeight;	/* width and height of glyph */
    FbStride	    gStride;		/* stride of glyph */
    Bool	    opaque;
    int		    n;
    int		    gx, gy;
    void	    (*glyph) (FbBits *,
			      FbStride,
			      int,
			      FbStip *,
			      FbBits,
			      int,
			      int);
    FbBits	    *dst;
    FbStride	    dstStride;
    int		    dstBpp;
    int		    dstXoff, dstYoff;
    FbBits	    depthMask;

    STRACE;
    depthMask = FbFullMask(pDrawable->depth);
    if ((pGC->planemask & depthMask) != depthMask)
    {
	ExaCheckImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppciInit, pglyphBase);
	return;
    }
    glyph = NULL;
    fbGetDrawable (pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);
    switch (dstBpp) {
    case 8:	glyph = fbGlyph8; break;
    case 16:    glyph = fbGlyph16; break;
    case 24:    glyph = fbGlyph24; break;
    case 32:    glyph = fbGlyph32; break;
    }

    x += pDrawable->x;
    y += pDrawable->y;

    if (TERMINALFONT (pGC->font) && !glyph)
    {
	opaque = TRUE;
    }
    else
    {
	int		xBack, widthBack;
	int		yBack, heightBack;

	ppci = ppciInit;
	n = nglyph;
	widthBack = 0;
	while (n--)
	    widthBack += (*ppci++)->metrics.characterWidth;

        xBack = x;
	if (widthBack < 0)
	{
	    xBack += widthBack;
	    widthBack = -widthBack;
	}
	yBack = y - FONTASCENT(pGC->font);
	heightBack = FONTASCENT(pGC->font) + FONTDESCENT(pGC->font);
        exaSolidBoxClipped (pDrawable,
			    fbGetCompositeClip(pGC),
			    pGC->planemask,
			    pGC->bgPixel,
			    xBack,
			    yBack,
			    xBack + widthBack,
			    yBack + heightBack);
	opaque = FALSE;
    }

    EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);

    ppci = ppciInit;
    while (nglyph--)
    {
	pci = *ppci++;
	pglyph = FONTGLYPHBITS(pglyphBase, pci);
	gWidth = GLYPHWIDTHPIXELS(pci);
	gHeight = GLYPHHEIGHTPIXELS(pci);
	if (gWidth && gHeight)
	{
	    gx = x + pci->metrics.leftSideBearing;
	    gy = y - pci->metrics.ascent;
	    if (glyph && gWidth <= sizeof (FbStip) * 8 &&
		fbGlyphIn (fbGetCompositeClip(pGC), gx, gy, gWidth, gHeight))
	    {
		(*glyph) (dst + (gy + dstYoff) * dstStride,
			  dstStride,
			  dstBpp,
			  (FbStip *) pglyph,
			  pPriv->fg,
			  gx + dstXoff,
			  gHeight);
	    }
	    else
	    {
		gStride = GLYPHWIDTHBYTESPADDED(pci) / sizeof (FbStip);
		fbPutXYImage (pDrawable,
			      fbGetCompositeClip(pGC),
			      pPriv->fg,
			      pPriv->bg,
			      pPriv->pm,
			      GXcopy,
			      opaque,

			      gx,
			      gy,
			      gWidth, gHeight,

			      (FbStip *) pglyph,
			      gStride,
			      0);
	    }
	}
	x += pci->metrics.characterWidth;
    }
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

static const GCOps	exaOps = {
    exaFillSpans,
    ExaCheckSetSpans,
    ExaCheckPutImage,
    exaCopyArea,
    ExaCheckCopyPlane,
    ExaCheckPolyPoint,
    ExaCheckPolylines,
    ExaCheckPolySegment,
    miPolyRectangle,
    ExaCheckPolyArc,
    miFillPolygon,
    exaPolyFillRect,
    miPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    exaImageGlyphBlt,
    ExaCheckPolyGlyphBlt,
    ExaCheckPushPixels,
#ifdef NEED_LINEHELPER
    ,NULL
#endif
};

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
    STRACE;
    if (!fbCreateGC (pGC))
	return FALSE;

    pGC->funcs = &exaGCFuncs;

    return TRUE;
}


static void
exaCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    RegionRec	rgnDst;
    int		dx, dy;
    PixmapPtr	pPixmap = (*pWin->drawable.pScreen->GetWindowPixmap) (pWin);
    ScrnInfoPtr pScrn = XF86SCRNINFO(pWin->drawable.pScreen);

    if (!pScrn->vtSema) {
        ExaScreenPriv(pWin->drawable.pScreen);
        pExaScr->SavedCopyWindow (pWin, ptOldOrg, prgnSrc);
        exaDrawableDirty (&pWin->drawable);
        return;
    }

    STRACE;
    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    REGION_TRANSLATE(pWin->drawable.pScreen, prgnSrc, -dx, -dy);

    REGION_INIT (pWin->drawable.pScreen, &rgnDst, NullBox, 0);

    REGION_INTERSECT(pWin->drawable.pScreen, &rgnDst, &pWin->borderClip, prgnSrc);
#ifdef COMPOSITE
    if (pPixmap->screen_x || pPixmap->screen_y)
	REGION_TRANSLATE (pWin->drawable.pScreen, &rgnDst,
			  -pPixmap->screen_x, -pPixmap->screen_y);
#endif

    fbCopyRegion (&pPixmap->drawable, &pPixmap->drawable,
		  NULL,
		  &rgnDst, dx, dy, exaCopyNtoN, 0, NULL);

    REGION_UNINIT(pWin->drawable.pScreen, &rgnDst);
}

static void
exaFillRegionSolid (DrawablePtr	pDrawable,
		    RegionPtr	pRegion,
		    Pixel	pixel)
{
    ExaScreenPriv(pDrawable->pScreen);
    PixmapPtr pPixmap;
    int xoff, yoff;

    STRACE;
    if (pDrawable->width <= pExaScr->info->card.maxX &&
	pDrawable->height <= pExaScr->info->card.maxY &&
	(pPixmap = exaGetOffscreenPixmap (pDrawable, &xoff, &yoff)) &&
	(*pExaScr->info->accel.PrepareSolid) (pPixmap, GXcopy, FB_ALLONES, pixel))
    {
	int	nbox = REGION_NUM_RECTS (pRegion);
	BoxPtr	pBox = REGION_RECTS (pRegion);

	while (nbox--)
	{
	    (*pExaScr->info->accel.Solid) (pPixmap,
                                           pBox->x1 + xoff, pBox->y1 + yoff,
                                           pBox->x2 + xoff, pBox->y2 + yoff);
	    pBox++;
	}
	(*pExaScr->info->accel.DoneSolid) (pPixmap);
	exaMarkSync(pDrawable->pScreen);
	exaDrawableDirty (pDrawable);
    }
    else
    {
	EXA_FALLBACK(("to 0x%lx\n", (long)pDrawable));
	exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
	fbFillRegionSolid (pDrawable, pRegion, 0,
			   fbReplicatePixel (pixel, pDrawable->bitsPerPixel));
	exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
    }
}

/* Try to do an accelerated tile of the pTile into pRegion of pDrawable.
 * Based on fbFillRegionTiled(), fbTile().
 */
static void
exaFillRegionTiled (DrawablePtr	pDrawable,
		    RegionPtr	pRegion,
		    PixmapPtr	pTile)
{
    ExaScreenPriv(pDrawable->pScreen);
    PixmapPtr pPixmap;
    int xoff, yoff;
    int tileWidth, tileHeight;

    STRACE;
    tileWidth = pTile->drawable.width;
    tileHeight = pTile->drawable.height;

    if (pDrawable->width > pExaScr->info->card.maxX ||
	pDrawable->height > pExaScr->info->card.maxY ||
	tileWidth > pExaScr->info->card.maxX ||
	tileHeight > pExaScr->info->card.maxY)
    {
	goto fallback;
    }

    /* If we're filling with a solid color, grab it out and go to
     * FillRegionSolid, saving numerous copies.
     */
    if (tileWidth == 1 && tileHeight == 1) {
	CARD32 pixel;

	exaDrawableUseMemory(&pTile->drawable);
	exaPrepareAccess(&pTile->drawable, EXA_PREPARE_SRC);
	switch (pTile->drawable.bitsPerPixel) {
	case 8:
	    pixel = *(CARD8 *)(pTile->devPrivate.ptr);
	    break;
	case 16:
	    pixel = *(CARD16 *)(pTile->devPrivate.ptr);
	    break;
	case 32:
	    pixel = *(CARD32 *)(pTile->devPrivate.ptr);
	    break;
	default:
	    exaFinishAccess(&pTile->drawable, EXA_PREPARE_SRC);
	    goto fallback;
	}
	exaFinishAccess(&pTile->drawable, EXA_PREPARE_SRC);
	exaFillRegionSolid(pDrawable, pRegion, pixel);
	return;
    }

    pPixmap = exaGetOffscreenPixmap (pDrawable, &xoff, &yoff);
    if (!pPixmap)
	goto fallback;

    exaPixmapUseScreen(pTile);
    if (!exaPixmapIsOffscreen(pTile))
	goto fallback;

    if ((*pExaScr->info->accel.PrepareCopy) (pTile, pPixmap, 0, 0, GXcopy,
					     FB_ALLONES))
    {
	int nbox = REGION_NUM_RECTS (pRegion);
	BoxPtr pBox = REGION_RECTS (pRegion);

	while (nbox--)
	{
	    int height = pBox->y2 - pBox->y1;
	    int dstY = pBox->y1;
	    int tileY;

	    tileY = (dstY - pDrawable->y) % tileHeight;
	    while (height > 0) {
		int width = pBox->x2 - pBox->x1;
		int dstX = pBox->x1;
		int tileX;
		int h = tileHeight - tileY;

		if (h > height)
		    h = height;
		height -= h;

		tileX = (dstX - pDrawable->x) % tileWidth;
		while (width > 0) {
		    int w = tileWidth - tileX;
		    if (w > width)
			w = width;
		    width -= w;

		    (*pExaScr->info->accel.Copy) (pPixmap,
						  tileX, tileY,
						  dstX + xoff, dstY + yoff,
						  w, h);
		    dstX += w;
		    tileX = 0;
		}
		dstY += h;
		tileY = 0;
	    }
	    pBox++;
	}
	(*pExaScr->info->accel.DoneCopy) (pPixmap);
	exaMarkSync(pDrawable->pScreen);
	exaDrawableDirty (pDrawable);
	return;
    }

fallback:
    EXA_FALLBACK(("from 0x%lx to 0x%lx\n", (long)pTile, (long)pDrawable));
    exaPrepareAccess (pDrawable, EXA_PREPARE_DEST);
    exaPrepareAccess ((DrawablePtr)pTile, EXA_PREPARE_SRC);
    fbFillRegionTiled (pDrawable, pRegion, pTile);
    exaFinishAccess ((DrawablePtr)pTile, EXA_PREPARE_SRC);
    exaFinishAccess (pDrawable, EXA_PREPARE_DEST);
}

static void
exaPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what)
{
    ScrnInfoPtr pScrn = XF86SCRNINFO(pWin->drawable.pScreen);

    STRACE;

    if (!REGION_NUM_RECTS(pRegion))
	return;
    if (pScrn->vtSema) {
        switch (what) {
        case PW_BACKGROUND:
            switch (pWin->backgroundState) {
            case None:
                return;
            case ParentRelative:
                do {
                    pWin = pWin->parent;
                } while (pWin->backgroundState == ParentRelative);
                (*pWin->drawable.pScreen->PaintWindowBackground)(pWin, pRegion,
                                                                 what);
                return;
            case BackgroundPixel:
                exaFillRegionSolid((DrawablePtr)pWin, pRegion, pWin->background.pixel);
                return;
            case BackgroundPixmap:
                exaFillRegionTiled((DrawablePtr)pWin, pRegion, pWin->background.pixmap);
                return;
            }
            break;
        case PW_BORDER:
            if (pWin->borderIsPixel) {
                exaFillRegionSolid((DrawablePtr)pWin, pRegion, pWin->border.pixel);
                return;
            } else {
                exaFillRegionTiled((DrawablePtr)pWin, pRegion, pWin->border.pixmap);
                return;
            }
            break;
        }
    }
    ExaCheckPaintWindow (pWin, pRegion, what);
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
    STRACE;
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
