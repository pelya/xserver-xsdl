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

Bool
xglCopy (DrawablePtr pSrc,
	 DrawablePtr pDst,
	 int	     dx,
	 int	     dy,
	 BoxPtr	     pBox,
	 int	     nBox)
{
    glitz_surface_t *srcSurface;
    glitz_surface_t *dstSurface;
    int		    srcXoff, srcYoff;
    int		    dstXoff, dstYoff;

    if (xglPrepareTarget (pDst))
    {
	glitz_drawable_t *srcDrawable;
	glitz_drawable_t *dstDrawable;

	XGL_SCREEN_PRIV (pDst->pScreen);
	XGL_DRAWABLE_PIXMAP_PRIV (pSrc);
	    
	if (!xglSyncSurface (pSrc))
	    return FALSE;

	XGL_GET_DRAWABLE (pSrc, srcSurface, srcXoff, srcYoff);
	XGL_GET_DRAWABLE (pDst, dstSurface, dstXoff, dstYoff);

	/*
	 * Blit to screen.
	 */
	if (dstSurface == pScreenPriv->surface)
	    XGL_INCREMENT_PIXMAP_SCORE (pPixmapPriv, 5000);

	srcDrawable = glitz_surface_get_attached_drawable (srcSurface);
	dstDrawable = glitz_surface_get_attached_drawable (dstSurface);
	
	if (srcDrawable != dstDrawable && nBox > 1)
	{
	    xglGeometryRec geometry;
	    BoxRec	   extents;

	    BOX_EXTENTS (pBox, nBox, &extents);
	    
	    GEOMETRY_INIT (pDst->pScreen, &geometry, nBox << 3);
	    GEOMETRY_ADD_BOX (pDst->pScreen, &geometry, pBox, nBox);
	    GEOMETRY_TRANSLATE (&geometry, dstXoff, dstYoff);
	    
	    if (!GEOMETRY_ENABLE_ALL_VERTICES (&geometry, dstSurface))
		return FALSE;
	    
	    pPixmapPriv->pictureMask |=
		xglPCFillMask | xglPCFilterMask | xglPCTransformMask;
	    
	    glitz_surface_set_fill (srcSurface, GLITZ_FILL_TRANSPARENT);
	    glitz_surface_set_filter (srcSurface, GLITZ_FILTER_NEAREST,
				      NULL, 0);
	    glitz_surface_set_transform (srcSurface, NULL);
		
	    glitz_composite (GLITZ_OPERATOR_SRC,
			     srcSurface, NULL, dstSurface,
			     extents.x1 + dx + srcXoff,
			     extents.y1 + dy + srcYoff,
			     0, 0,
			     extents.x1 + dstXoff,
			     extents.y1 + dstYoff,
			     extents.x2 - extents.x1,
			     extents.y2 - extents.y1);
		
	    GEOMETRY_UNINIT (&geometry);
		
	    if (glitz_surface_get_status (dstSurface))
		return FALSE;
		
	    return TRUE;
	}
    }
    else
    {
	if (!xglPrepareTarget (pSrc))
	    return FALSE;

	if (!xglSyncSurface (pDst))
	    return FALSE;

	XGL_GET_DRAWABLE (pSrc, srcSurface, srcXoff, srcYoff);
	XGL_GET_DRAWABLE (pDst, dstSurface, dstXoff, dstYoff);
    }

    while (nBox--)
    {
	glitz_copy_area (srcSurface,
			 dstSurface,
			 pBox->x1 + dx + srcXoff,
			 pBox->y1 + dy + srcYoff,
			 pBox->x2 - pBox->x1,
			 pBox->y2 - pBox->y1,
			 pBox->x1 + dstXoff,
			 pBox->y1 + dstYoff);
	
	pBox++;
    }

    if (glitz_surface_get_status (dstSurface))
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
    Bool *pRet = (Bool *) closure;
    
    *pRet = xglCopy (pSrc, pDst, dx, dy, pBox, nBox);
}
