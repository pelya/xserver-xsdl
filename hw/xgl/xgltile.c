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
xglTile (DrawablePtr	  pDrawable,
	 glitz_operator_t op,
	 PixmapPtr	  pTile,
	 int		  tileX,
	 int		  tileY,
	 xglGeometryPtr	  pGeometry,
	 BoxPtr		  pBox,
	 int		  nBox)
{
    xglPixmapPtr    pTilePriv;
    glitz_surface_t *surface;
    int		    xOff, yOff;

    if (!xglSyncSurface (&pTile->drawable))
	return FALSE;

    if (!xglPrepareTarget (pDrawable))
	return FALSE;
    
    XGL_GET_DRAWABLE (pDrawable, surface, xOff, yOff);

    GEOMETRY_TRANSLATE (pGeometry, xOff, yOff);
    
    if (!GEOMETRY_ENABLE_ALL_VERTICES (pGeometry, surface))
	return FALSE;

    pTilePriv = XGL_GET_PIXMAP_PRIV (pTile);

    pTilePriv->pictureMask |=
	xglPCFillMask | xglPCFilterMask | xglPCTransformMask;
	
    glitz_surface_set_filter (pTilePriv->surface, GLITZ_FILTER_NEAREST,
			      NULL, 0);
    glitz_surface_set_transform (pTilePriv->surface, NULL);
    
    if (pTilePriv->acceleratedTile)
    {
	glitz_surface_set_fill (pTilePriv->surface, GLITZ_FILL_REPEAT);
	
	while (nBox--)
	{
	    glitz_composite (op,
			     pTilePriv->surface, NULL, surface,
			     pBox->x1 + tileX,
			     pBox->y1 + tileY,
			     0, 0,
			     pBox->x1 + xOff,
			     pBox->y1 + yOff,
			     pBox->x2 - pBox->x1, pBox->y2 - pBox->y1);
	    
	    pBox++;
	}
	
	if (!glitz_surface_get_status (surface))
	    return TRUE;
    }
    
    glitz_surface_set_fill (pTilePriv->surface, GLITZ_FILL_TRANSPARENT);

    /*
     * Don't allow software tile with really small pixmaps.
     */
    if (pTile->drawable.width < 8 && pTile->drawable.height < 8)
	return FALSE;

    xglSwTile (op,
	       pTilePriv->surface, NULL, surface,
	       tileX - xOff, tileY - yOff,
	       0, 0,
	       TILE_SOURCE,
	       pBox, nBox,
	       xOff, yOff);

    if (glitz_surface_get_status (surface))
	return FALSE;

    return TRUE;
}

void
xglSwTile (glitz_operator_t op,
	   glitz_surface_t  *srcSurface,
	   glitz_surface_t  *maskSurface,
	   glitz_surface_t  *dstSurface,
	   int		    xSrc,
	   int		    ySrc,
	   int		    xMask,
	   int		    yMask,
	   int		    what,
	   BoxPtr	    pBox,
	   int		    nBox,
	   int		    xOff,
	   int		    yOff)
{
    int tileX, tileY;
    int tileWidth, tileHeight;

    if (what == TILE_MASK) {
	tileX      = xMask;
	tileY      = yMask;
	tileWidth  = glitz_surface_get_width (maskSurface);
	tileHeight = glitz_surface_get_height (maskSurface);
    } else {
	tileX      = xSrc;
	tileY      = ySrc;
	tileWidth  = glitz_surface_get_width (srcSurface);
	tileHeight = glitz_surface_get_height (srcSurface);
    }

    while (nBox--)
    {
	int x, y, width, height;
	int xTile, yTile;
	int widthTile, heightTile;
	int widthTmp, xTmp, yTmp, xTileTmp;

	x = pBox->x1 + xOff;
	y = pBox->y1 + yOff;
	width = pBox->x2 - pBox->x1;
	height = pBox->y2 - pBox->y1;
	
	xTile = MOD (tileX + x, tileWidth);
	yTile = MOD (tileY + y, tileHeight);
	
	yTmp = y;
	
	while (height)
	{
	    heightTile = MIN (tileHeight - yTile, height);

	    xTileTmp = xTile;
	    widthTmp = width;
	    xTmp     = x;
	    
	    while (widthTmp)
	    {
		widthTile = MIN (tileWidth - xTileTmp, widthTmp);

		if (what == TILE_MASK)
		{
		    glitz_composite (op,
				     srcSurface, maskSurface, dstSurface,
				     xSrc + xTmp, ySrc + yTmp,
				     xTileTmp, yTile,
				     xTmp, yTmp,
				     widthTile, heightTile);
		}
		else
		{
		    glitz_composite (op,
				     srcSurface, maskSurface, dstSurface,
				     xTileTmp, yTile,
				     xMask + xTmp, yMask + yTmp,
				     xTmp, yTmp,
				     widthTile, heightTile);
		}
		
		xTileTmp  = 0;
		xTmp     += widthTile;
		widthTmp -= widthTile;
		
	    }

	    yTile   = 0;
	    yTmp   += heightTile;
	    height -= heightTile;
	}
	
	pBox++;
    }
}
