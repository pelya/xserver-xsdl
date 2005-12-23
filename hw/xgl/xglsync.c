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

/*
 * A pixmap may exist both in hardware and in software. Synchronization
 * is handled as follows:
 *
 * Regions modified by software and hardware needs to be tracked.
 * A software operation requires that a rectangle of pixels matching the
 * extents of the operation is synchronized. A hardware operation
 * requires that all pixels are synchronized. If the bounds of a
 * hardware operation intersects the bounds of a currently synchronized
 * software rectangle, the software rectangle will be discarded and the
 * next software operation will require re-synchronization.
 *
 * A software rectangle of pixels is synchronized as follows. If a
 * previously synchronized rectangle exists, then if this previous
 * rectangle subsumes our new rectangle no pixels are fetched from
 * hardware as all pixels that need to be synchronized are already up to
 * date. If a previously synchronized rectangle exists and it intersects
 * with our new rectangle, then both these rectangles are merged into a
 * larger rectangle and pixels not part of the previous rectangle are
 * fetched form hardware. If a previously synchronized rectangle exists
 * and it doesn't intersect with our new rectangle, then the previous
 * rectangle is discarded and pixels are fetched from hardware so that
 * our new rectangle becomes synchronized.
 *
 * If the pixmap exists in hardware and if it can be a target of a
 * drawing operation, then it is kept synchronized all the time, any
 * pixels modified by software will be transfered to hardware right
 * away. If the pixmap exists in hardware but it can only be used as
 * source of a drawing operation, then synchronization is performed
 * only when needed.
 */

#define ALL_BITS(pPixmap, pBox)			\
    ((pBox)->x1 <= 0 && (pBox)->y1 <= 0 &&	\
     (pBox)->x2 >= (pPixmap)->drawable.width && \
     (pBox)->y2 >= (pPixmap)->drawable.height)

Bool
xglSyncBits (DrawablePtr pDrawable,
	     BoxPtr	 pExtents)
{
    RegionRec region;
    BoxPtr    pBitBox;
    BoxRec    box;

    XGL_DRAWABLE_PIXMAP (pDrawable);
    XGL_PIXMAP_PRIV (pPixmap);

    if (pPixmapPriv->allBits)
	return xglMapPixmapBits (pPixmap);

    pBitBox = &pPixmapPriv->bitBox;

    if (pPixmapPriv->target == xglPixmapTargetIn && pExtents)
    {
	box.x1 = MAX (0, pExtents->x1);
	box.y1 = MAX (0, pExtents->y1);
	box.x2 = MAX (0, MIN (pPixmap->drawable.width, pExtents->x2));
	box.y2 = MAX (0, MIN (pPixmap->drawable.height, pExtents->y2));

	if (!BOX_NOTEMPTY (&box))
	    return xglMapPixmapBits (pPixmap);
	
	if (BOX_NOTEMPTY (pBitBox))
	{
	    RegionRec bitRegion;
	    
	    REGION_INIT (pDrawable->pScreen, &bitRegion, pBitBox, 1);
	    
	    switch (RECT_IN_REGION (pDrawable->pScreen, &bitRegion, &box)) {
	    case rgnIN:
		REGION_INIT (pDrawable->pScreen, &region, NullBox, 0);
		break;
	    case rgnOUT:
		REGION_INIT (pDrawable->pScreen, &region, &box, 1);
		*pBitBox = box;
		pPixmapPriv->allBits = ALL_BITS (pPixmap, pBitBox);
		break;
	    case rgnPART:
 		pBitBox->x1 = MIN (pBitBox->x1, box.x1);
		pBitBox->y1 = MIN (pBitBox->y1, box.y1);
		pBitBox->x2 = MAX (pBitBox->x2, box.x2);
		pBitBox->y2 = MAX (pBitBox->y2, box.y2);
		pPixmapPriv->allBits = ALL_BITS (pPixmap, pBitBox);
		
		REGION_INIT (pDrawable->pScreen, &region, pBitBox, 1);
		REGION_SUBTRACT (pDrawable->pScreen, &region, &region,
				 &bitRegion);
		
		break;
	    }
	    REGION_UNINIT (pDrawable->pScreen, &bitRegion);
	}
	else
	{
	    REGION_INIT (pDrawable->pScreen, &region, &box, 1);
	    *pBitBox = box;
	    pPixmapPriv->allBits = ALL_BITS (pPixmap, pBitBox);
	}
    }
    else
    {
	box.x1 = 0;
	box.y1 = 0;
	box.x2 = pPixmap->drawable.width;
	box.y2 = pPixmap->drawable.height;

	REGION_INIT (pDrawable->pScreen, &region, &box, 1);
	
	if (BOX_NOTEMPTY (pBitBox))
	{
	    RegionRec bitRegion;
	    
	    REGION_INIT (pDrawable->pScreen, &bitRegion, pBitBox, 1);
	    REGION_SUBTRACT (pDrawable->pScreen, &region, &region, &bitRegion);
	    REGION_UNINIT (pDrawable->pScreen, &bitRegion);
	}
	
	*pBitBox = box;
	pPixmapPriv->allBits = TRUE;
    }

    if (!pPixmapPriv->buffer)
	if (!xglAllocatePixmapBits (pPixmap, XGL_PIXMAP_USAGE_HINT_DEFAULT))
	    return FALSE;

    if (pPixmapPriv->pDamage)
    {
	RegionPtr pRegion;

	pRegion = DamageRegion (pPixmapPriv->pDamage);
	REGION_SUBTRACT (pDrawable->pScreen, &region, &region, pRegion);
    }
    
    if (REGION_NOTEMPTY (pDrawable->pScreen, &region) && pPixmapPriv->surface)
    {
	glitz_pixel_format_t format;
	BoxPtr		     pBox;
	int		     nBox;
	
	if (!xglSyncSurface (pDrawable))
	    FatalError (XGL_SW_FAILURE_STRING);

	xglUnmapPixmapBits (pPixmap);
	    
	pBox = REGION_RECTS (&region);
	nBox = REGION_NUM_RECTS (&region);

	format.masks = pPixmapPriv->pPixel->masks;
	
	while (nBox--)
	{
	    format.xoffset = pBox->x1;

	    if (pPixmapPriv->stride < 0)
	    {
		format.skip_lines     = pPixmap->drawable.height - pBox->y2;
		format.bytes_per_line = -pPixmapPriv->stride;
		format.scanline_order = GLITZ_PIXEL_SCANLINE_ORDER_BOTTOM_UP;
	    }
	    else
	    {
		format.skip_lines     = pBox->y1;
		format.bytes_per_line = pPixmapPriv->stride;
		format.scanline_order = GLITZ_PIXEL_SCANLINE_ORDER_TOP_DOWN;
	    }

	    glitz_get_pixels (pPixmapPriv->surface,
			      pBox->x1,
			      pBox->y1,
			      pBox->x2 - pBox->x1,
			      pBox->y2 - pBox->y1,
			      &format,
			      pPixmapPriv->buffer);
	    
	    pBox++;
	}
    }

    REGION_UNINIT (pDrawable->pScreen, &region);

    return xglMapPixmapBits (pPixmap);
}

void
xglSyncDamageBoxBits (DrawablePtr pDrawable)
{
    XGL_DRAWABLE_PIXMAP_PRIV (pDrawable);

    if (!xglSyncBits (pDrawable, &pPixmapPriv->damageBox))
	FatalError (XGL_SW_FAILURE_STRING);
}

Bool
xglSyncSurface (DrawablePtr pDrawable)
{
    RegionPtr pRegion;

    XGL_DRAWABLE_PIXMAP (pDrawable);
    XGL_PIXMAP_PRIV (pPixmap);

    if (!pPixmapPriv->surface)
    {
	if (!pPixmapPriv->format)
	    return FALSE;

	if (!xglCreatePixmapSurface (pPixmap))
	    return FALSE;
    }

    pRegion = DamageRegion (pPixmapPriv->pDamage);

    if (REGION_NOTEMPTY (pDrawable->pScreen, pRegion))
    { 
	glitz_pixel_format_t format;
	BoxPtr		     pBox;
	BoxPtr		     pExt;
	int		     nBox;
	
	xglUnmapPixmapBits (pPixmap);

	nBox = REGION_NUM_RECTS (pRegion);
	pBox = REGION_RECTS (pRegion);
	pExt = REGION_EXTENTS (pDrawable->pScreen, pRegion);

	format.masks   = pPixmapPriv->pPixel->masks;
	format.xoffset = pExt->x1;

	if (pPixmapPriv->stride < 0)
	{
	    format.skip_lines	  = pPixmap->drawable.height - pExt->y2;
	    format.bytes_per_line = -pPixmapPriv->stride;
	    format.scanline_order = GLITZ_PIXEL_SCANLINE_ORDER_BOTTOM_UP;
	}
	else
	{
	    format.skip_lines	  = pExt->y1;
	    format.bytes_per_line = pPixmapPriv->stride;
	    format.scanline_order = GLITZ_PIXEL_SCANLINE_ORDER_TOP_DOWN;
	}

	glitz_surface_set_clip_region (pPixmapPriv->surface,
				       0, 0, (glitz_box_t *) pBox, nBox);

	glitz_set_pixels (pPixmapPriv->surface,
			  pExt->x1,
			  pExt->y1,
			  pExt->x2 - pExt->x1,
			  pExt->y2 - pExt->y1,
			  &format,
			  pPixmapPriv->buffer);

	glitz_surface_set_clip_region (pPixmapPriv->surface, 0, 0, NULL, 0);

	REGION_EMPTY (pDrawable->pScreen, pRegion);
    }

    return TRUE;
}

Bool
xglPrepareTarget (DrawablePtr pDrawable)
{
    XGL_DRAWABLE_PIXMAP (pDrawable);
    XGL_PIXMAP_PRIV (pPixmap);

    switch (pPixmapPriv->target) {
    case xglPixmapTargetNo:
	break;
    case xglPixmapTargetOut:
	if (xglSyncSurface (pDrawable))
	{
	  pPixmapPriv->target = xglPixmapTargetIn;
	  return TRUE;
	}
	break;
    case xglPixmapTargetIn:
	if (xglSyncSurface (pDrawable))
	    return TRUE;
	break;
    }
    
    return FALSE;
}

void
xglAddSurfaceDamage (DrawablePtr pDrawable,
		     RegionPtr   pRegion)
{
    RegionPtr	    pDamageRegion;
    glitz_surface_t *surface;
    int		    xOff, yOff;
    
    XGL_DRAWABLE_PIXMAP_PRIV (pDrawable);

    pPixmapPriv->damageBox = miEmptyBox;
    if (!pPixmapPriv->format)
	return;

    XGL_GET_DRAWABLE (pDrawable, surface, xOff, yOff);

    if (xOff || yOff)
	REGION_TRANSLATE (pDrawable->pScreen, pRegion, xOff, yOff);

    pDamageRegion = DamageRegion (pPixmapPriv->pDamage);

    REGION_UNION (pDrawable->pScreen, pDamageRegion, pDamageRegion, pRegion);
    
    if (xOff || yOff)
	REGION_TRANSLATE (pDrawable->pScreen, pRegion, -xOff, -yOff);
}

void
xglAddCurrentSurfaceDamage (DrawablePtr pDrawable)
{   
    XGL_DRAWABLE_PIXMAP_PRIV (pDrawable);

    if (!pPixmapPriv->format)
    {
	pPixmapPriv->damageBox = miEmptyBox;
	return;
    }

    if (BOX_NOTEMPTY (&pPixmapPriv->damageBox))
    {
	RegionPtr pDamageRegion;
	RegionRec region;

	pDamageRegion = DamageRegion (pPixmapPriv->pDamage);

	REGION_INIT (pDrawable->pScreen, &region, &pPixmapPriv->damageBox, 1);
	REGION_UNION (pDrawable->pScreen,
		      pDamageRegion, pDamageRegion, &region);
	REGION_UNINIT (pDrawable->pScreen, &region);
	
	pPixmapPriv->damageBox = miEmptyBox;
    }
}

void
xglAddBitDamage (DrawablePtr pDrawable,
		 RegionPtr   pRegion)
{
    BoxPtr pBox;
    BoxPtr pExt;
    int    nBox;
    
    XGL_DRAWABLE_PIXMAP_PRIV (pDrawable);

    pBox = REGION_RECTS (pRegion);
    pExt = REGION_EXTENTS (pDrawable->pScreen, pRegion);
    nBox = REGION_NUM_RECTS (pRegion);

    if (pExt->x1 < pPixmapPriv->bitBox.x2 &&
	pExt->y1 < pPixmapPriv->bitBox.y2 &&
	pExt->x2 > pPixmapPriv->bitBox.x1 &&
	pExt->y2 > pPixmapPriv->bitBox.y1)
    {
	while (nBox--)
	{
	    if (pBox->x1 < pPixmapPriv->bitBox.x2 &&
		pBox->y1 < pPixmapPriv->bitBox.y2 &&
		pBox->x2 > pPixmapPriv->bitBox.x1 &&
		pBox->y2 > pPixmapPriv->bitBox.y1)
	    {
		pPixmapPriv->bitBox = miEmptyBox;
		pPixmapPriv->allBits = FALSE;
		return;
	    }
	    
	    pBox++;
	}
    }
}

void
xglAddCurrentBitDamage (DrawablePtr pDrawable)
{
    XGL_DRAWABLE_PIXMAP_PRIV (pDrawable);

    if (pPixmapPriv->damageBox.x1 < pPixmapPriv->bitBox.x2 &&
	pPixmapPriv->damageBox.y1 < pPixmapPriv->bitBox.y2 &&
	pPixmapPriv->damageBox.x2 > pPixmapPriv->bitBox.x1 &&
	pPixmapPriv->damageBox.y2 > pPixmapPriv->bitBox.y1)
    {
	pPixmapPriv->bitBox = miEmptyBox;
	pPixmapPriv->allBits = FALSE;
    }

    pPixmapPriv->damageBox = miEmptyBox;
}
