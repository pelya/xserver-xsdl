/*
 * Copyright © 2004 David Reveman
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the names of
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
 * Author: David Reveman <davidr@freedesktop.org>
 */

#include "xgl.h"

#ifdef RENDER

static glitz_operator_t xglOperators[] = {
    GLITZ_OPERATOR_CLEAR,
    GLITZ_OPERATOR_SRC,
    GLITZ_OPERATOR_DST,
    GLITZ_OPERATOR_OVER,
    GLITZ_OPERATOR_OVER_REVERSE,
    GLITZ_OPERATOR_IN,
    GLITZ_OPERATOR_IN_REVERSE,
    GLITZ_OPERATOR_OUT,
    GLITZ_OPERATOR_OUT_REVERSE,
    GLITZ_OPERATOR_ATOP,
    GLITZ_OPERATOR_ATOP_REVERSE,
    GLITZ_OPERATOR_XOR,
    GLITZ_OPERATOR_ADD
};

#define NUM_XGL_OPERATORS			       \
    (sizeof (xglOperators) / sizeof (xglOperators[0]))

#define XGL_OPERATOR(op) (xglOperators[op])

Bool
xglComp (CARD8	    op,
	 PicturePtr pSrc,
	 PicturePtr pMask,
	 PicturePtr pDst,
	 INT16	    xSrc,
	 INT16	    ySrc,
	 INT16	    xMask,
	 INT16	    yMask,
	 INT16	    xDst,
	 INT16	    yDst,
	 CARD16	    width,
	 CARD16	    height)
{
    ScreenPtr	    pScreen = pDst->pDrawable->pScreen;
    xglPixmapPtr    pSrcPriv, pMaskPriv;
    glitz_surface_t *dst;
    int		    dstXoff, dstYoff;
    RegionRec	    region;
    BoxPtr	    pBox;
    int		    nBox;

    if (pDst->dither != None)
	return FALSE;
    
    if (pDst->alphaMap)
	return FALSE;

    if (op >= NUM_XGL_OPERATORS)
	return FALSE;
    
    if (pSrc->pDrawable->type != DRAWABLE_PIXMAP)
	return FALSE;

    if (pMask)
    {
	if (pMask->pDrawable->type != DRAWABLE_PIXMAP)
	    return FALSE;

	/*
	 * Why?
	 */
	if (pSrc->pDrawable == pMask->pDrawable)
	    return FALSE;
    }
    
    xDst += pDst->pDrawable->x;
    yDst += pDst->pDrawable->y;

    if (!miComputeCompositeRegion (&region, pSrc, pMask, pDst,
				   xSrc, ySrc, xMask, yMask,
				   xDst, yDst, width, height))
	return TRUE;

    pBox = REGION_RECTS (&region);
    nBox = REGION_NUM_RECTS (&region);

    /*
     * Simple copy
     */
    if (op == PictOpSrc && !pMask &&
	!pSrc->transform && !pSrc->repeat && pSrc->filter <= 1)
    {
	if (xglCopy (pSrc->pDrawable,
		     pDst->pDrawable,
		     xSrc - xDst,
		     ySrc - yDst,
		     pBox,
		     nBox))
	{
	    REGION_UNINIT (pScreen, &region);
	    xglAddBitDamage (pDst->pDrawable);
	    
	    return TRUE;
	}

	REGION_UNINIT (pScreen, &region);
	return FALSE;
    }

    if (!xglPrepareTarget (pDst->pDrawable))
    {
	REGION_UNINIT (pScreen, &region);
	return FALSE;
    }

    XGL_GET_DRAWABLE (pDst->pDrawable, dst, dstXoff, dstYoff);

    if (!xglSyncSurface (pSrc->pDrawable))
    {
	REGION_UNINIT (pScreen, &region);
	return FALSE;
    }

    pSrcPriv = XGL_GET_PIXMAP_PRIV ((PixmapPtr) pSrc->pDrawable);
    if (XGL_PICTURE_CHANGES (pSrcPriv->pictureMask))
	xglUpdatePicture (pSrc);

    if (pMask)
    {
	if (!xglSyncSurface (pMask->pDrawable))
	{
	    REGION_UNINIT (pScreen, &region);
	    return FALSE;
	}

	pMaskPriv = XGL_GET_PIXMAP_PRIV ((PixmapPtr) pMask->pDrawable);
	if (XGL_PICTURE_CHANGES (pMaskPriv->pictureMask))
	    xglUpdatePicture (pMask);
    } else
	pMaskPriv = NULL;

    if (nBox > 1)
    {
	xglGeometryRec  geometry;
	
	GEOMETRY_INIT (pScreen, &geometry, REGION_NUM_RECTS (&region) << 3);
	GEOMETRY_ADD_BOX (pScreen, &geometry, pBox, nBox);
	GEOMETRY_TRANSLATE (&geometry, dstXoff, dstYoff);

	if (!GEOMETRY_ENABLE_ALL_VERTICES (&geometry, dst))
	{
	    GEOMETRY_UNINIT (&geometry);
	    return FALSE;
	}

	pBox = REGION_EXTENTS (pScreen, &region);
    } else
	GEOMETRY_DISABLE (dst);

    xSrc += pBox->x1 - xDst;
    ySrc += pBox->y1 - yDst;

    if (pMask)
    {
	xMask += pBox->x1 - xDst;
	yMask += pBox->y1 - yDst;
    }
	
    xDst = pBox->x1;
    yDst = pBox->y1;
	
    width  = pBox->x2 - pBox->x1;
    height = pBox->y2 - pBox->y1;

    REGION_UNINIT (pScreen, &region);

    /*
     * Do software tile instead if hardware can't do it.
     */
    if (pSrc->repeat && !pSrcPriv->acceleratedTile)
    {
	BoxRec box;

	if (pSrc->transform || pSrc->filter > 1)
	    return FALSE;
	
	/*
	 * Don't allow software tile with really small pixmaps.
	 */
	if (pSrc->pDrawable->width < 8 && pSrc->pDrawable->height < 8)
	    return FALSE;

	box.x1 = xDst + dstXoff;
	box.y1 = yDst + dstYoff;
	box.x2 = box.x1 + width;
	box.y2 = box.y1 + height;

	glitz_surface_set_fill (pSrcPriv->surface, GLITZ_FILL_TRANSPARENT);

	xglSwTile (XGL_OPERATOR (op),
		   pSrcPriv->surface,
		   (pMaskPriv)? pMaskPriv->surface: NULL,
		   dst,
		   xSrc - box.x1, ySrc - box.y1,
		   xMask - box.x1, yMask - box.y1,
		   TILE_SOURCE,
		   &box, 1,
		   0, 0);
    }
    else
    {
	glitz_composite (XGL_OPERATOR (op),
			 pSrcPriv->surface,
			 (pMaskPriv)? pMaskPriv->surface: NULL,
			 dst,
			 xSrc, ySrc,
			 xMask, yMask,
			 xDst + dstXoff, yDst + dstYoff,
			 width, height);
    }
    
    if (glitz_surface_get_status (dst))
	return FALSE;
    
    xglAddBitDamage (pDst->pDrawable);
    
    return TRUE;
}

#endif
