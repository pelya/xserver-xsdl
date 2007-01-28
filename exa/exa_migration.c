/*
 * Copyright © 2006 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *    Michel Dänzer <michel@tungstengraphics.com>
 *
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

/**
 * The fallback path for UTS/DFS failing is to just memcpy.  exaCopyDirtyToSys
 * and exaCopyDirtyToFb both needed to do this loop.
 */
static void
exaMemcpyBox (PixmapPtr pPixmap, BoxPtr pbox, CARD8 *src, int src_pitch,
	      CARD8 *dst, int dst_pitch)
 {
    int i, cpp = pPixmap->drawable.bitsPerPixel / 8;
    int bytes = (pbox->x2 - pbox->x1) * cpp;

    src += pbox->y1 * src_pitch + pbox->x1 * cpp;
    dst += pbox->y1 * dst_pitch + pbox->x1 * cpp;

    for (i = pbox->y2 - pbox->y1; i; i--) {
	memcpy (dst, src, bytes);
	src += src_pitch;
	dst += dst_pitch;
    }
}
 
/**
 * Returns TRUE if the pixmap is dirty (has been modified in its current
 * location compared to the other), or lacks a private for tracking
 * dirtiness.
 */
static Bool
exaPixmapIsDirty (PixmapPtr pPix)
{
    ExaPixmapPriv (pPix);

    return pExaPixmap == NULL ||
	REGION_NOTEMPTY (pScreen, DamageRegion(pExaPixmap->pDamage));
}

/**
 * Returns TRUE if the pixmap is either pinned in FB, or has a sufficient score
 * to be considered "should be in framebuffer".  That's just anything that has
 * had more acceleration than fallbacks, or has no score yet.
 *
 * Only valid if using a migration scheme that tracks score.
 */
static Bool
exaPixmapShouldBeInFB (PixmapPtr pPix)
{
    ExaPixmapPriv (pPix);

    if (exaPixmapIsPinned (pPix))
	return TRUE;

    return pExaPixmap->score >= 0;
}

/**
 * If the pixmap is currently dirty, this copies at least the dirty area from
 * the framebuffer  memory copy to the system memory copy.  Both areas must be
 * allocated.
 */
static void
exaCopyDirtyToSys (PixmapPtr pPixmap)
{
    ExaScreenPriv (pPixmap->drawable.pScreen);
    ExaPixmapPriv (pPixmap);
    RegionPtr pRegion = DamageRegion (pExaPixmap->pDamage);
    CARD8 *save_ptr;
    int save_pitch;
    BoxPtr pBox = REGION_RECTS(pRegion);
    int nbox = REGION_NUM_RECTS(pRegion);
    Bool do_sync = FALSE;

    save_ptr = pPixmap->devPrivate.ptr;
    save_pitch = pPixmap->devKind;
    pPixmap->devPrivate.ptr = pExaPixmap->fb_ptr;
    pPixmap->devKind = pExaPixmap->fb_pitch;

    while (nbox--) {
	pBox->x1 = max(pBox->x1, 0);
	pBox->y1 = max(pBox->y1, 0);
	pBox->x2 = min(pBox->x2, pPixmap->drawable.width);
	pBox->y2 = min(pBox->y2, pPixmap->drawable.height);

	if (pBox->x1 >= pBox->x2 || pBox->y1 >= pBox->y2)
	    continue;

	if (pExaScr->info->DownloadFromScreen == NULL ||
	    !pExaScr->info->DownloadFromScreen (pPixmap,
						pBox->x1, pBox->y1,
						pBox->x2 - pBox->x1,
						pBox->y2 - pBox->y1,
						pExaPixmap->sys_ptr
						+ pBox->y1 * pExaPixmap->sys_pitch
						+ pBox->x1 * pPixmap->drawable.bitsPerPixel / 8,
						pExaPixmap->sys_pitch))
	{
	    exaPrepareAccess(&pPixmap->drawable, EXA_PREPARE_SRC);
	    exaMemcpyBox (pPixmap, pBox,
			  pExaPixmap->fb_ptr, pExaPixmap->fb_pitch,
			  pExaPixmap->sys_ptr, pExaPixmap->sys_pitch);
	    exaFinishAccess(&pPixmap->drawable, EXA_PREPARE_SRC);
	}
	else
	    do_sync = TRUE;

	pBox++;
    }

    /* Make sure the bits have actually landed, since we don't necessarily sync
     * when accessing pixmaps in system memory.
     */
    if (do_sync)
	exaWaitSync (pPixmap->drawable.pScreen);

    pPixmap->devPrivate.ptr = save_ptr;
    pPixmap->devKind = save_pitch;

    /* The previously damaged bits are now no longer damaged but valid */
    REGION_UNION(pPixmap->drawable.pScreen,
		 &pExaPixmap->validReg, &pExaPixmap->validReg, pRegion);
    DamageEmpty (pExaPixmap->pDamage);
}

/**
 * If the pixmap is currently dirty, this copies at least the dirty area from
 * the system memory copy to the framebuffer memory copy.  Both areas must be
 * allocated.
 */
static void
exaCopyDirtyToFb (PixmapPtr pPixmap)
{
    ExaScreenPriv (pPixmap->drawable.pScreen);
    ExaPixmapPriv (pPixmap);
    RegionPtr pRegion = DamageRegion (pExaPixmap->pDamage);
    CARD8 *save_ptr;
    int save_pitch;
    BoxPtr pBox = REGION_RECTS(pRegion);
    int nbox = REGION_NUM_RECTS(pRegion);
    Bool do_sync = FALSE;

    save_ptr = pPixmap->devPrivate.ptr;
    save_pitch = pPixmap->devKind;
    pPixmap->devPrivate.ptr = pExaPixmap->fb_ptr;
    pPixmap->devKind = pExaPixmap->fb_pitch;

    while (nbox--) {
	pBox->x1 = max(pBox->x1, 0);
	pBox->y1 = max(pBox->y1, 0);
	pBox->x2 = min(pBox->x2, pPixmap->drawable.width);
	pBox->y2 = min(pBox->y2, pPixmap->drawable.height);

	if (pBox->x1 >= pBox->x2 || pBox->y1 >= pBox->y2)
	    continue;

	if (pExaScr->info->UploadToScreen == NULL ||
	    !pExaScr->info->UploadToScreen (pPixmap,
					    pBox->x1, pBox->y1,
					    pBox->x2 - pBox->x1,
					    pBox->y2 - pBox->y1,
					    pExaPixmap->sys_ptr
					    + pBox->y1 * pExaPixmap->sys_pitch
					    + pBox->x1 * pPixmap->drawable.bitsPerPixel / 8,
					    pExaPixmap->sys_pitch))
	{
	    exaPrepareAccess(&pPixmap->drawable, EXA_PREPARE_DEST);
	    exaMemcpyBox (pPixmap, pBox,
			  pExaPixmap->sys_ptr, pExaPixmap->sys_pitch,
			  pExaPixmap->fb_ptr, pExaPixmap->fb_pitch);
	    exaFinishAccess(&pPixmap->drawable, EXA_PREPARE_DEST);
	}
	else
	    do_sync = TRUE;

	pBox++;
    }

    if (do_sync)
	exaMarkSync (pPixmap->drawable.pScreen);

    pPixmap->devPrivate.ptr = save_ptr;
    pPixmap->devKind = save_pitch;

    /* The previously damaged bits are now no longer damaged but valid */
    REGION_UNION(pPixmap->drawable.pScreen,
		 &pExaPixmap->validReg, &pExaPixmap->validReg, pRegion);
    DamageEmpty (pExaPixmap->pDamage);
}

/**
 * Copies out important pixmap data and removes references to framebuffer area.
 * Called when the memory manager decides it's time to kick the pixmap out of
 * framebuffer entirely.
 */
void
exaPixmapSave (ScreenPtr pScreen, ExaOffscreenArea *area)
{
    PixmapPtr pPixmap = area->privData;
    ExaPixmapPriv(pPixmap);
    RegionPtr pDamageReg = DamageRegion(pExaPixmap->pDamage);

    DBG_MIGRATE (("Save %p (%p) (%dx%d) (%c)\n", pPixmap,
		  (void*)(ExaGetPixmapPriv(pPixmap)->area ?
                          ExaGetPixmapPriv(pPixmap)->area->offset : 0),
		  pPixmap->drawable.width,
		  pPixmap->drawable.height,
		  exaPixmapIsDirty(pPixmap) ? 'd' : 'c'));

    if (exaPixmapIsOffscreen(pPixmap)) {
	exaCopyDirtyToSys (pPixmap);
	pPixmap->devPrivate.ptr = pExaPixmap->sys_ptr;
	pPixmap->devKind = pExaPixmap->sys_pitch;
	pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    }

    pExaPixmap->fb_ptr = NULL;
    pExaPixmap->area = NULL;

    /* Mark all valid bits as damaged, so they'll get copied to FB next time */
    REGION_UNION(pPixmap->drawable.pScreen, pDamageReg, pDamageReg,
		 &pExaPixmap->validReg);
}

/**
 * Allocates a framebuffer copy of the pixmap if necessary, and then copies
 * any necessary pixmap data into the framebuffer copy and points the pixmap at
 * it.
 *
 * Note that when first allocated, a pixmap will have FALSE dirty flag.
 * This is intentional because pixmap data starts out undefined.  So if we move
 * it in due to the first operation against it being accelerated, it will have
 * undefined framebuffer contents that we didn't have to upload.  If we do
 * moveouts (and moveins) after the first movein, then we will only have to copy
 * back and forth if the pixmap was written to after the last synchronization of
 * the two copies.  Then, at exaPixmapSave (when the framebuffer copy goes away)
 * we mark the pixmap dirty, so that the next exaMoveInPixmap will actually move
 * all the data, since it's almost surely all valid now.
 */
void
exaMoveInPixmap (PixmapPtr pPixmap)
{
    ScreenPtr	pScreen = pPixmap->drawable.pScreen;
    ExaScreenPriv (pScreen);
    ExaPixmapPriv (pPixmap);

    /* If we're VT-switched away, no touching card memory allowed. */
    if (pExaScr->swappedOut)
	return;

    /* If we're already in FB, our work is done. */
    if (exaPixmapIsOffscreen(pPixmap))
	return;

    /* If we're not allowed to move, then fail. */
    if (exaPixmapIsPinned(pPixmap))
	return;

    /* Don't migrate in pixmaps which are less than 8bpp.  This avoids a lot of
     * fragility in EXA, and <8bpp is probably not used enough any more to care
     * (at least, not in acceleratd paths).
     */
    if (pPixmap->drawable.bitsPerPixel < 8)
	return;

    if (pExaPixmap->area == NULL) {
	pExaPixmap->area =
	    exaOffscreenAlloc (pScreen, pExaPixmap->fb_size,
			       pExaScr->info->pixmapOffsetAlign, FALSE,
                               exaPixmapSave, (pointer) pPixmap);
	if (pExaPixmap->area == NULL)
	    return;

	pExaPixmap->fb_ptr = (CARD8 *) pExaScr->info->memoryBase +
				       pExaPixmap->area->offset;
    }

    DBG_MIGRATE (("-> %p (0x%x) (%dx%d) (%c)\n", pPixmap,
		  (ExaGetPixmapPriv(pPixmap)->area ?
                   ExaGetPixmapPriv(pPixmap)->area->offset : 0),
		  pPixmap->drawable.width,
		  pPixmap->drawable.height,
		  exaPixmapIsDirty(pPixmap) ? 'd' : 'c'));

    exaCopyDirtyToFb (pPixmap);

    if (pExaScr->hideOffscreenPixmapData)
	pPixmap->devPrivate.ptr = NULL;
    else
	pPixmap->devPrivate.ptr = pExaPixmap->fb_ptr;
    pPixmap->devKind = pExaPixmap->fb_pitch;
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
}

/**
 * Switches the current active location of the pixmap to system memory, copying
 * updated data out if necessary.
 */
void
exaMoveOutPixmap (PixmapPtr pPixmap)
{
    ExaPixmapPriv (pPixmap);

    if (exaPixmapIsPinned(pPixmap))
	return;

    if (exaPixmapIsOffscreen(pPixmap)) {

	DBG_MIGRATE (("<- %p (%p) (%dx%d) (%c)\n", pPixmap,
		      (void*)(ExaGetPixmapPriv(pPixmap)->area ?
			      ExaGetPixmapPriv(pPixmap)->area->offset : 0),
		      pPixmap->drawable.width,
		      pPixmap->drawable.height,
		      exaPixmapIsDirty(pPixmap) ? 'd' : 'c'));

	exaCopyDirtyToSys (pPixmap);

	pPixmap->devPrivate.ptr = pExaPixmap->sys_ptr;
	pPixmap->devKind = pExaPixmap->sys_pitch;
	pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
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
 * If the pixmap has both a framebuffer and system memory copy, this function
 * asserts that both of them are the same.
 */
static Bool
exaAssertNotDirty (PixmapPtr pPixmap)
{
    ExaPixmapPriv (pPixmap);
    CARD8 *dst, *src;
    RegionPtr pValidReg = &pExaPixmap->validReg;
    int dst_pitch, src_pitch, cpp, y, nbox = REGION_NUM_RECTS(pValidReg);
    BoxPtr pBox = REGION_RECTS(pValidReg);
    Bool ret = TRUE;

    if (pExaPixmap == NULL || pExaPixmap->fb_ptr == NULL)
	return ret;

    dst = pExaPixmap->sys_ptr;
    dst_pitch = pExaPixmap->sys_pitch;
    src = pExaPixmap->fb_ptr;
    src_pitch = pExaPixmap->fb_pitch;
    cpp = pPixmap->drawable.bitsPerPixel / 8;

    exaPrepareAccess(&pPixmap->drawable, EXA_PREPARE_SRC);
    while (nbox--) {
	    int rowbytes;

	    pBox->x1 = max(pBox->x1, 0);
	    pBox->y1 = max(pBox->y1, 0);
	    pBox->x2 = min(pBox->x2, pPixmap->drawable.width);
	    pBox->y2 = min(pBox->y2, pPixmap->drawable.height);

	    if (pBox->x1 >= pBox->x2 || pBox->y1 >= pBox->y2)
		continue;

	    rowbytes = (pBox->x2 - pBox->x1) * cpp;
	    src += pBox->y1 * src_pitch + pBox->x1 * cpp;
	    dst += pBox->y1 * dst_pitch + pBox->x1 * cpp;

	    for (y = pBox->y2 - pBox->y1; y; y--) {
		if (memcmp(dst + pBox->y1 * dst_pitch + pBox->x1 * cpp,
			   src + pBox->y1 * src_pitch + pBox->x1 * cpp,
			   (pBox->x2 - pBox->x1) * cpp) != 0) {
		    ret = FALSE;
		    break;
		}
		src += src_pitch;
		dst += dst_pitch;
	    }
	    src -= pBox->y1 * src_pitch + pBox->x1 * cpp;
	    dst -= pBox->y1 * dst_pitch + pBox->x1 * cpp;
    }
    exaFinishAccess(&pPixmap->drawable, EXA_PREPARE_SRC);

    return ret;
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

    /* If this debugging flag is set, check each pixmap for whether it is marked
     * as clean, and if so, actually check if that's the case.  This should help
     * catch issues with failing to mark a drawable as dirty.  While it will
     * catch them late (after the operation happened), it at least explains what
     * went wrong, and instrumenting the code to find what operation happened
     * to the pixmap last shouldn't be hard.
     */
    if (pExaScr->checkDirtyCorrectness) {
	for (i = 0; i < npixmaps; i++) {
	    if (!exaPixmapIsDirty (pixmaps[i].pPix) &&
		!exaAssertNotDirty (pixmaps[i].pPix))
		ErrorF("%s: Pixmap %d dirty but not marked as such!\n", __func__, i);
	}
    }
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

    if (pExaScr->migration == ExaMigrationSmart) {
	/* If we've got something as a destination that we shouldn't cause to
	 * become newly dirtied, take the unaccelerated route.
	 */
	for (i = 0; i < npixmaps; i++) {
	    if (pixmaps[i].as_dst && !exaPixmapShouldBeInFB (pixmaps[i].pPix) &&
		!exaPixmapIsDirty (pixmaps[i].pPix))
	    {
		for (i = 0; i < npixmaps; i++) {
		    if (!exaPixmapIsDirty (pixmaps[i].pPix))
			exaMoveOutPixmap (pixmaps[i].pPix);
		}
		return;
	    }
	}

	/* If we aren't going to accelerate, then we migrate everybody toward
	 * system memory, and kick out if it's free.
	 */
	if (!can_accel) {
	    for (i = 0; i < npixmaps; i++) {
		exaMigrateTowardSys (pixmaps[i].pPix);
		if (!exaPixmapIsDirty (pixmaps[i].pPix))
		    exaMoveOutPixmap (pixmaps[i].pPix);
	    }
	    return;
	}

	/* Finally, the acceleration path.  Move them all in. */
	for (i = 0; i < npixmaps; i++) {
	    exaMigrateTowardFb(pixmaps[i].pPix);
	    exaMoveInPixmap(pixmaps[i].pPix);
	}
    } else if (pExaScr->migration == ExaMigrationGreedy) {
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
