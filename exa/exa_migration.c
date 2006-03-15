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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>

#include "exa_priv.h"
#include <X11/fonts/fontstruct.h>
#include "dixfontstr.h"
#include "exa.h"
#include "cw.h"

#if DEBUG_MIGRATE
#define DBG_MIGRATE(a) ErrorF a
#else
#define DBG_MIGRATE(a)
#endif

/**
 * Returns TRUE if the pixmap is not movable.  This is the case where it's a
 * fake pixmap for the frontbuffer (no pixmap private) or it's a scratch
 * pixmap created by some other X Server internals (the score says it's
 * pinned).
 */
static Bool
exaPixmapIsPinned (PixmapPtr pPix)
{
    ExaPixmapPriv (pPix);

    return pExaPixmap == NULL || pExaPixmap->score == EXA_PIXMAP_SCORE_PINNED;
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
        if (pExaScr->info->DownloadFromScreen &&
	    (*pExaScr->info->DownloadFromScreen) (pPixmap,
						  pPixmap->drawable.x,
						  pPixmap->drawable.y,
						  pPixmap->drawable.width,
						  pPixmap->drawable.height,
						  dst, dst_pitch)) {

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

    if (pExaScr->info->flags & EXA_OFFSCREEN_ALIGN_POT && w != 1)
	w = 1 << (exaLog2(w - 1) + 1);
    pitch = (w * bpp / 8) + (pExaScr->info->pixmapPitchAlign - 1);
    pitch -= pitch % pExaScr->info->pixmapPitchAlign;

    pExaPixmap->size = pitch * h;
    pExaPixmap->devKind = pPixmap->devKind;
    pExaPixmap->devPrivate = pPixmap->devPrivate;
    pExaPixmap->area = exaOffscreenAlloc (pScreen, pitch * h,
                                          pExaScr->info->pixmapOffsetAlign,
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

    pPixmap->devPrivate.ptr = (pointer) ((CARD8 *) pExaScr->info->memoryBase +
					 pExaPixmap->area->offset);
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

    if (pExaScr->info->UploadToScreen)
    {
	if (pExaScr->info->UploadToScreen(pPixmap, 0, 0,
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
                i, pExaScr->info->memoryBase, pExaPixmap->area->offset));

    while (i--) {
	memcpy (dst, src, bytes);
	dst += dst_pitch;
	src += src_pitch;
    }
}

void
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
    if (area)
    {
	exaPixmapSave (pPixmap->drawable.pScreen, area);
	exaOffscreenFree (pPixmap->drawable.pScreen, area);
    }
}

/**
 * For the "greedy" migration scheme, pushes the pixmap toward being located in
 * framebuffer memory.
 */
static void
exaMigrateTowardFb (PixmapPtr pPixmap)
{
    ExaPixmapPriv (pPixmap);

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

/**
 * For the "greedy" migration scheme, pushes the pixmap toward being located in
 * system memory.
 */
static void
exaMigrateTowardSys (PixmapPtr pPixmap)
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

/**
 * Performs migration of the pixmaps according to the operation information
 * provided in pixmaps and can_accel and the migration scheme chosen in the
 * config file.
 */
void
exaDoMigration (ExaMigrationPtr pixmaps, int npixmaps, Bool can_accel)
{
    ScreenPtr pScreen = pixmaps[0].pPix->drawable.pScreen;
    ExaScreenPriv(pScreen);
    int i, j;

    /* If anything is pinned in system memory, we won't be able to
     * accelerate.
     */
    for (i = 0; i < npixmaps; i++) {
	if (exaPixmapIsPinned (pixmaps[i].pPix) &&
	    !exaPixmapIsOffscreen (pixmaps[i].pPix))
	{
	    EXA_FALLBACK(("Pixmap %p (%dx%d) pinned in sys\n", pixmaps[i].pPix,
		      pixmaps[i].pPix->drawable.width,
		      pixmaps[i].pPix->drawable.height));
	    can_accel = FALSE;
	    break;
	}
    }

    if (pExaScr->migration == ExaMigrationGreedy) {
	/* If we can't accelerate, either because the driver can't or because one of
	 * the pixmaps is pinned in system memory, then we migrate everybody toward
	 * system memory.
	 *
	 * We also migrate toward system if all pixmaps involved are currently in
	 * system memory -- this can mitigate thrashing when there are significantly
	 * more pixmaps active than would fit in memory.
	 *
	 * If not, then we migrate toward FB so that hopefully acceleration can
	 * happen.
	 */
	if (!can_accel) {
	    for (i = 0; i < npixmaps; i++)
		exaMigrateTowardSys (pixmaps[i].pPix);
	    return;
	}

	for (i = 0; i < npixmaps; i++) {
	    if (exaPixmapIsOffscreen(pixmaps[i].pPix)) {
		/* Found one in FB, so move all to FB. */
		for (j = 0; j < npixmaps; j++)
		    exaMigrateTowardFb(pixmaps[j].pPix);
		return;
	    }
	}

	/* Nobody's in FB, so move all away from FB. */
	for (i = 0; i < npixmaps; i++)
	    exaMigrateTowardSys(pixmaps[i].pPix);
    } else if (pExaScr->migration == ExaMigrationAlways) {
	/* Always move the pixmaps out if we can't accelerate.  If we can
	 * accelerate, try to move them all in.  If that fails, then move them
	 * back out.
	 */
	if (!can_accel) {
	    for (i = 0; i < npixmaps; i++)
		exaMoveOutPixmap(pixmaps[i].pPix);
	    return;
	}

	/* Now, try to move them all into FB */
	for (i = 0; i < npixmaps; i++) {
	    exaMoveInPixmap(pixmaps[i].pPix);
	    ExaOffscreenMarkUsed (pixmaps[i].pPix);
	}

	/* If we couldn't fit everything in, then kick back out */
	for (i = 0; i < npixmaps; i++) {
	    if (!exaPixmapIsOffscreen(pixmaps[i].pPix)) {
		EXA_FALLBACK(("Pixmap %p (%dx%d) not in fb\n", pixmaps[i].pPix,
			      pixmaps[i].pPix->drawable.width,
			      pixmaps[i].pPix->drawable.height));
		for (j = 0; j < npixmaps; j++)
		    exaMoveOutPixmap(pixmaps[j].pPix);
		break;
	    }
	}
    }
}
