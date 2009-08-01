/*
 * Copyright © 2009 Maarten Maathuis
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
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>

#include "exa_priv.h"
#include "exa.h"

static void
exaUploadFallback(PixmapPtr pPixmap, CARD8 *src, int src_pitch,
	      CARD8 *dst, int dst_pitch)
 {
    int i;

    for (i = pPixmap->drawable.height; i; i--) {
	memcpy (dst, src, min(src_pitch, dst_pitch));
	src += src_pitch;
	dst += dst_pitch;
    }
}

void
exaCreateDriverPixmap_mixed(PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    ExaScreenPriv(pScreen);
    ExaPixmapPriv(pPixmap);
    void *sys_buffer = pExaPixmap->sys_ptr;
    int w = pPixmap->drawable.width, h = pPixmap->drawable.height;
    int depth = pPixmap->drawable.depth, bpp = pPixmap->drawable.bitsPerPixel;
    int usage_hint = pPixmap->usage_hint;
    int sys_pitch = pExaPixmap->sys_pitch;
    int paddedWidth = sys_pitch;

    /* Already done. */
    if (pExaPixmap->driverPriv)
	return;

    if (exaPixmapIsPinned(pPixmap))
	return;

    /* Can't accel 1/4 bpp. */
    if (pExaPixmap->accel_blocked || bpp < 8)
	return;

    if (paddedWidth < pExaPixmap->fb_pitch)
        paddedWidth = pExaPixmap->fb_pitch;

    if (pExaScr->info->CreatePixmap2)
	pExaPixmap->driverPriv = pExaScr->info->CreatePixmap2(pScreen, w, h, depth, usage_hint, bpp);
    else
	pExaPixmap->driverPriv = pExaScr->info->CreatePixmap(pScreen, paddedWidth*h, 0);

    if (!pExaPixmap->driverPriv)
	return;

    pExaPixmap->offscreen = TRUE;
    pExaPixmap->sys_ptr = NULL;

    pExaPixmap->score = EXA_PIXMAP_SCORE_PINNED;
    (*pScreen->ModifyPixmapHeader)(pPixmap, w, h, 0, 0,
				paddedWidth, NULL);

    /* scratch pixmaps */
    if (!w || !h)
	goto finish;

    if (!pExaScr->info->UploadToScreen)
	goto fallback;

    if (pExaScr->info->UploadToScreen(pPixmap, 0, 0, w, h, sys_buffer, sys_pitch))
	goto finish;

fallback:
    ExaDoPrepareAccess(&pPixmap->drawable, EXA_PREPARE_DEST);
    exaUploadFallback(pPixmap, sys_buffer, sys_pitch, pPixmap->devPrivate.ptr,
	exaGetPixmapPitch(pPixmap));
    exaFinishAccess(&pPixmap->drawable, EXA_PREPARE_DEST);

finish:
    free(sys_buffer);
}

void
exaDoMigration_mixed(ExaMigrationPtr pixmaps, int npixmaps, Bool can_accel)
{
    int i;

    /* If anything is pinned in system memory, we won't be able to
     * accelerate.
     */
    for (i = 0; i < npixmaps; i++) {
	if (exaPixmapIsPinned (pixmaps[i].pPix) &&
	    !exaPixmapIsOffscreen (pixmaps[i].pPix))
	{
	    can_accel = FALSE;
	    break;
	}
    }

    /* We can do nothing. */
    if (!can_accel)
	return;

    for (i = 0; i < npixmaps; i++) {
	PixmapPtr pPixmap = pixmaps[i].pPix;
	ExaPixmapPriv(pPixmap);
	if (!pExaPixmap->driverPriv)
	    exaCreateDriverPixmap_mixed(pPixmap);
    }
}
