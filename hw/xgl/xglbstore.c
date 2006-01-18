/*
 * Copyright © 2004 David Reveman
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * David Reveman not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * David Reveman makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DAVID REVEMAN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DAVID REVEMAN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include "xgl.h"

#define XGL_BSTORE_FALLBACK_PROLOGUE(pDrawable, func) \
    xglSyncDamageBoxBits (pDrawable);		      \
    XGL_SCREEN_UNWRAP (func)

#define XGL_BSTORE_FALLBACK_EPILOGUE(pDrawable, func, xglfunc) \
    XGL_SCREEN_WRAP (func, xglfunc);			       \
    xglAddCurrentSurfaceDamage (pDrawable)

/*
 * The follwong functions are not yet tested so we can assume that they
 * are both broken.
 */

void
xglSaveAreas (PixmapPtr	pPixmap,
	      RegionPtr	prgnSave,
	      int	xorg,
	      int	yorg,
	      WindowPtr	pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    BoxRec    box;

    XGL_SCREEN_PRIV (pScreen);
    XGL_PIXMAP_PRIV (pPixmap);

    box = *(REGION_EXTENTS (pScreen, prgnSave));

    pPixmapPriv->damageBox = box;

    if (xglCopy (&pWin->drawable,
		 &pPixmap->drawable,
		 xorg, yorg,
		 REGION_RECTS (prgnSave),
		 REGION_NUM_RECTS (prgnSave)))
    {
	xglAddCurrentBitDamage (&pPixmap->drawable);
	return;
    }

    box.x1 += xorg;
    box.y1 += yorg;
    box.x2 += xorg;
    box.y2 += yorg;

    if (!xglSyncBits (&pWin->drawable, &box))
	FatalError (XGL_SW_FAILURE_STRING);

    XGL_BSTORE_FALLBACK_PROLOGUE (&pPixmap->drawable,
				  BackingStoreFuncs.SaveAreas);
    (*pScreen->BackingStoreFuncs.SaveAreas) (pPixmap, prgnSave,
					     xorg, yorg, pWin);
    XGL_BSTORE_FALLBACK_EPILOGUE (&pPixmap->drawable,
				  BackingStoreFuncs.SaveAreas,
				  xglSaveAreas);
}

void
xglRestoreAreas (PixmapPtr pPixmap,
		 RegionPtr prgnRestore,
		 int	   xorg,
		 int	   yorg,
		 WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    BoxPtr    pExt;
    BoxRec    box;

    XGL_SCREEN_PRIV (pScreen);

    if (xglCopy (&pPixmap->drawable,
		 &pWin->drawable,
		 -xorg, -yorg,
		 REGION_RECTS (prgnRestore),
		 REGION_NUM_RECTS (prgnRestore)))
    {
	xglAddCurrentBitDamage (&pPixmap->drawable);
	return;
    }

    pExt = REGION_EXTENTS (pScreen, prgnRestore);

    box.x1 = pExt->x1 - xorg;
    box.y1 = pExt->y1 - yorg;
    box.x2 = pExt->x2 - xorg;
    box.y2 = pExt->y2 - yorg;

    if (!xglSyncBits (&pPixmap->drawable, &box))
	FatalError (XGL_SW_FAILURE_STRING);

    XGL_BSTORE_FALLBACK_PROLOGUE (&pWin->drawable,
				  BackingStoreFuncs.RestoreAreas);
    (*pScreen->BackingStoreFuncs.RestoreAreas) (pPixmap, prgnRestore,
						xorg, yorg, pWin);
    XGL_BSTORE_FALLBACK_EPILOGUE (&pWin->drawable,
				  BackingStoreFuncs.RestoreAreas,
				  xglRestoreAreas);
}
