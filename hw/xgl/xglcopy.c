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
#include "fb.h"

Bool
xglCopy (DrawablePtr pSrc,
	 DrawablePtr pDst,
	 int	     dx,
	 int	     dy,
	 BoxPtr	     pBox,
	 int	     nBox)
{
    glitz_surface_t *src, *dst;
    int		    srcXoff, srcYoff;
    int		    dstXoff, dstYoff;

    XGL_DRAWABLE_PIXMAP_PRIV (pSrc);

    if (!nBox)
	return TRUE;

    /* source is all in software and damaged, fall-back is probably more
       efficient */
    if (pPixmapPriv->allBits &&
	pPixmapPriv->pDamage &&
	REGION_NOTEMPTY (pDrawable->pScreen,
			 DamageRegion (pPixmapPriv->pDamage)))
	return FALSE;

    if (xglPrepareTarget (pDst))
    {
	XGL_SCREEN_PRIV (pDst->pScreen);
	
	if (!xglSyncSurface (pSrc))
	    return FALSE;

	XGL_GET_DRAWABLE (pDst, dst, dstXoff, dstYoff);

	/* blit to screen */
	if (dst == pScreenPriv->surface)
	    XGL_INCREMENT_PIXMAP_SCORE (pPixmapPriv, 5000);
    }
    else
    {
	if (!xglPrepareTarget (pSrc))
	    return FALSE;
	
	if (!xglSyncSurface (pDst))
	    return FALSE;
	
	XGL_GET_DRAWABLE (pDst, dst, dstXoff, dstYoff);
    }

    XGL_GET_DRAWABLE (pSrc, src, srcXoff, srcYoff);
	
    glitz_surface_set_clip_region (dst,
				   dstXoff, dstYoff,
				   (glitz_box_t *) pBox, nBox);

    glitz_copy_area (src,
		     dst,
		     pDst->x + srcXoff + dx,
		     pDst->y + srcYoff + dy,
		     pDst->width,
		     pDst->height,
		     pDst->x + dstXoff,
		     pDst->y + dstYoff);

    glitz_surface_set_clip_region (dst, 0, 0, NULL, 0);
    
    if (glitz_surface_get_status (dst))
	return FALSE;
    
    return TRUE;
}

void
xglCopyProc (DrawablePtr pSrc,
	     DrawablePtr pDst,
	     GCPtr	 pGC,
	     BoxPtr	 pBox,
	     int	 nBox,
	     int	 dx,
	     int	 dy,
	     Bool	 reverse,
	     Bool	 upsidedown,
	     Pixel	 bitplane,
	     void	 *closure)
{
    BoxPtr pSrcBox = (BoxPtr) closure;

    if (!xglCopy (pSrc, pDst, dx, dy, pBox, nBox))
    {
	RegionPtr	pDamageRegion;
	glitz_surface_t *dst;
	int		dstXoff, dstYoff;
	RegionRec	region;
	BoxRec		box;
	
	XGL_DRAWABLE_PIXMAP (pDst);
	XGL_PIXMAP_PRIV (pPixmap);

	XGL_GET_DRAWABLE (pDst, dst, dstXoff, dstYoff);
	
	pDamageRegion = DamageRegion (pPixmapPriv->pDamage);

	if (!xglMapPixmapBits (pPixmap))
	    FatalError (XGL_SW_FAILURE_STRING);
	
	if (!xglSyncBits (pSrc, pSrcBox))
	    FatalError (XGL_SW_FAILURE_STRING);

	fbCopyNtoN (pSrc, pDst, pGC,
		    pBox, nBox,
		    dx, dy,
		    reverse, upsidedown, bitplane,
		    (void *) 0);

	pPixmapPriv->damageBox = miEmptyBox;
	if (!pPixmapPriv->format)
	    return;

	while (nBox--)
	{
	    box.x1 = pBox->x1 + dstXoff;
	    box.y1 = pBox->y1 + dstYoff;
	    box.x2 = pBox->x2 + dstXoff;
	    box.y2 = pBox->y2 + dstYoff;

	    REGION_INIT (pDst->pScreen, &region, &box, 1);
	    REGION_UNION (pDst->pScreen,
			  pDamageRegion, pDamageRegion, &region);
	    REGION_UNINIT (pDst->pScreen, &region);

	    pBox++;
	}
    } else
	xglAddCurrentBitDamage (pDst);
}
