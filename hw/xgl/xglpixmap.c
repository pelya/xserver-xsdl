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

static void
xglPixmapDamageReport (DamagePtr pDamage,
		       RegionPtr pRegion,
		       void	 *closure)
{
    PixmapPtr pPixmap = (PixmapPtr) closure;
    BoxPtr    pExt;

    XGL_PIXMAP_PRIV (pPixmap);

    pExt = REGION_EXTENTS (pPixmap->drawable.pScreen, pRegion);

    if (BOX_NOTEMPTY (&pPixmapPriv->damageBox))
    {
	if (pExt->x1 < pPixmapPriv->damageBox.x1)
	    pPixmapPriv->damageBox.x1 = pExt->x1;

	if (pExt->y1 < pPixmapPriv->damageBox.y1)
	    pPixmapPriv->damageBox.y1 = pExt->y1;

	if (pExt->x2 > pPixmapPriv->damageBox.x2)
	    pPixmapPriv->damageBox.x2 = pExt->x2;

	if (pExt->y2 > pPixmapPriv->damageBox.y2)
	    pPixmapPriv->damageBox.y2 = pExt->y2;
    } else
	pPixmapPriv->damageBox = *pExt;
}


static Bool
xglPixmapCreateDamage (PixmapPtr pPixmap)
{
    XGL_PIXMAP_PRIV (pPixmap);

    pPixmapPriv->pDamage =
	DamageCreate (xglPixmapDamageReport, (DamageDestroyFunc) 0,
		      DamageReportRawRegion, TRUE,
		      pPixmap->drawable.pScreen,
		      (void *) pPixmap);
    if (!pPixmapPriv->pDamage)
	return FALSE;

    DamageRegister (&pPixmap->drawable, pPixmapPriv->pDamage);
    
    return TRUE;
}

static Bool
xglPixmapSurfaceInit (PixmapPtr	    pPixmap,
		      unsigned long features,
		      int	    width,
		      int	    height)
{
    XGL_PIXMAP_PRIV (pPixmap);
    
    pPixmapPriv->surface = NULL;
    pPixmapPriv->pArea = NULL;
    pPixmapPriv->score = 0;
    pPixmapPriv->acceleratedTile = FALSE;
    pPixmapPriv->pictureMask = ~0;
    pPixmapPriv->target = xglPixmapTargetNo;

    if (pPixmapPriv->format)
    {
	if (!pPixmapPriv->pDamage)
	    if (!xglPixmapCreateDamage (pPixmap))
		FatalError (XGL_SW_FAILURE_STRING);
	
	if (width && height)
	{
	    if (features & GLITZ_FEATURE_TEXTURE_BORDER_CLAMP_MASK)
		if ((features & GLITZ_FEATURE_TEXTURE_NON_POWER_OF_TWO_MASK) ||
		    (POWER_OF_TWO (width) && POWER_OF_TWO (height)))
		    pPixmapPriv->acceleratedTile = TRUE;
	
	    pPixmapPriv->target = xglPixmapTargetOut;
	    
	    /*
	     * Do not allow accelerated drawing to bitmaps.
	     */
	    if (pPixmap->drawable.depth == 1)
		pPixmapPriv->target = xglPixmapTargetNo;

	    /*
	     * Drawing to really small pixmaps is not worth accelerating.
	     */
	    if (width < 8 && height < 8)
		pPixmapPriv->target = xglPixmapTargetNo;
	}
    }

    return TRUE;
}

PixmapPtr
xglCreatePixmap (ScreenPtr  pScreen,
		 int	    width,
		 int	    height, 
		 int	    depth)
{
    xglPixmapPtr pPixmapPriv;
    PixmapPtr	 pPixmap;

    XGL_SCREEN_PRIV (pScreen);

    pPixmap = AllocatePixmap (pScreen, 0);
    if (!pPixmap)
	return NullPixmap;

    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.class = 0;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.depth = depth;
    pPixmap->drawable.bitsPerPixel = BitsPerPixel (depth);
    pPixmap->drawable.id = 0;
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pPixmap->drawable.x = 0;
    pPixmap->drawable.y = 0;
    pPixmap->drawable.width = width;
    pPixmap->drawable.height = height;
    
#ifdef COMPOSITE
    pPixmap->screen_x = 0;
    pPixmap->screen_y = 0;
#endif
    
    pPixmap->devKind = 0;
    pPixmap->refcnt = 1;
    pPixmap->devPrivate.ptr = 0;

    pPixmapPriv = XGL_GET_PIXMAP_PRIV (pPixmap);

    pPixmapPriv->format  = pScreenPriv->pixmapFormats[depth].format;
    pPixmapPriv->pPixel  = pScreenPriv->pixmapFormats[depth].pPixel;
    pPixmapPriv->pDamage = NULL;

    if (!xglPixmapSurfaceInit (pPixmap, pScreenPriv->features, width, height))
	return NullPixmap;
    
    pPixmapPriv->buffer = NULL;
    pPixmapPriv->bits = (pointer) 0;
    pPixmapPriv->stride = 0;
    pPixmapPriv->pGeometry = NULL;

    pPixmapPriv->allBits = TRUE;
    pPixmapPriv->bitBox.x1 = 0;
    pPixmapPriv->bitBox.y1 = 0;
    pPixmapPriv->bitBox.x2 = 32767;
    pPixmapPriv->bitBox.y2 = 32767;
    pPixmapPriv->damageBox = miEmptyBox;
    
    return pPixmap;
}

Bool
xglDestroyPixmap (PixmapPtr pPixmap)
{
    XGL_PIXMAP_PRIV (pPixmap);
	    
    if (--pPixmap->refcnt)
	return TRUE;

    if (pPixmapPriv->pArea)
	xglWithdrawArea (pPixmapPriv->pArea);
	
    if (pPixmap->devPrivate.ptr)
    {
	if (pPixmapPriv->buffer)
	    glitz_buffer_unmap (pPixmapPriv->buffer);
    }

    if (pPixmapPriv->pGeometry)
	GEOMETRY_UNINIT (pPixmapPriv->pGeometry);

    if (pPixmapPriv->buffer)
	glitz_buffer_destroy (pPixmapPriv->buffer);
    
    if (pPixmapPriv->bits)
	xfree (pPixmapPriv->bits);
    
    if (pPixmapPriv->surface)
	glitz_surface_destroy (pPixmapPriv->surface);

    xfree (pPixmap);
    
    return TRUE;
}

Bool
xglModifyPixmapHeader (PixmapPtr pPixmap,
		       int	 width,
		       int	 height,
		       int	 depth,
		       int	 bitsPerPixel,
		       int	 devKind,
		       pointer	 pPixData)
{
    xglScreenPtr   pScreenPriv;
    xglPixmapPtr   pPixmapPriv;
    glitz_format_t *oldFormat;
    int		   oldWidth, oldHeight;
    
    if (!pPixmap)
	return FALSE;

    pScreenPriv = XGL_GET_SCREEN_PRIV (pPixmap->drawable.pScreen);
    pPixmapPriv = XGL_GET_PIXMAP_PRIV (pPixmap);

    oldFormat = pPixmapPriv->format;
    oldWidth  = pPixmap->drawable.width;
    oldHeight = pPixmap->drawable.height;

    if ((width > 0) && (height > 0) && (depth > 0) && (bitsPerPixel > 0) &&
	(devKind > 0) && pPixData)
    {
	pPixmap->drawable.depth = depth;
	pPixmap->drawable.bitsPerPixel = bitsPerPixel;
	pPixmap->drawable.id = 0;
	pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	pPixmap->drawable.x = 0;
	pPixmap->drawable.y = 0;
	pPixmap->drawable.width = width;
	pPixmap->drawable.height = height;
	pPixmapPriv->stride = devKind;
	pPixmap->refcnt = 1;
    }
    else
    {
	if (width > 0)
	    pPixmap->drawable.width = width;
	
	if (height > 0)
	    pPixmap->drawable.height = height;
	
	if (depth > 0)
	    pPixmap->drawable.depth = depth;
	
	if (bitsPerPixel > 0)
	    pPixmap->drawable.bitsPerPixel = bitsPerPixel;
	else if ((bitsPerPixel < 0) && (depth > 0))
	    pPixmap->drawable.bitsPerPixel = BitsPerPixel (depth);

	if (devKind > 0)
	    pPixmapPriv->stride = devKind;
	else if ((devKind < 0) && ((width > 0) || (depth > 0)))
	    pPixmapPriv->stride = PixmapBytePad (pPixmap->drawable.width,
						 pPixmap->drawable.depth);
    }

    depth = pPixmap->drawable.depth;

    pPixmapPriv->pPixel = pScreenPriv->pixmapFormats[depth].pPixel;
    pPixmapPriv->format = pScreenPriv->pixmapFormats[depth].format;
    
    if (pPixmapPriv->format != oldFormat ||
	pPixmap->drawable.width != oldWidth ||
	pPixmap->drawable.height != oldHeight)
    {
	if (pPixmapPriv->pArea)
	    xglWithdrawArea (pPixmapPriv->pArea);
	
	if (pPixmapPriv->surface)
	    glitz_surface_destroy (pPixmapPriv->surface);

	if (!xglPixmapSurfaceInit (pPixmap,
				   pScreenPriv->features,
				   pPixmap->drawable.width,
				   pPixmap->drawable.height))
	    return FALSE;
    }

    if (pPixData)
    {
	if (pPixmap->devPrivate.ptr)
	{
	    if (pPixmapPriv->buffer)
		glitz_buffer_unmap (pPixmapPriv->buffer);

	    pPixmap->devPrivate.ptr = 0;
	}

	if (pPixmapPriv->pGeometry)
	{
	    GEOMETRY_UNINIT (pPixmapPriv->pGeometry);
	    pPixmapPriv->pGeometry = NULL;
	}
	
	if (pPixmapPriv->buffer)
	    glitz_buffer_destroy (pPixmapPriv->buffer);
	
	if (pPixmapPriv->bits)
	    xfree (pPixmapPriv->bits);

	pPixmapPriv->bits = (pointer) 0;
	pPixmapPriv->buffer = glitz_buffer_create_for_data (pPixData);
	if (!pPixmapPriv->buffer)
	    return FALSE;

	pPixmapPriv->allBits = TRUE;
	pPixmapPriv->bitBox.x1 = 0;
	pPixmapPriv->bitBox.y1 = 0;
	pPixmapPriv->bitBox.x2 = pPixmap->drawable.width;
	pPixmapPriv->bitBox.y2 = pPixmap->drawable.height;

	if (pPixmapPriv->pDamage)
	{
	    RegionPtr pRegion;
	    
	    pRegion = DamageRegion (pPixmapPriv->pDamage);

	    REGION_UNINIT (pPixmap->drawable.pScreen, pRegion);
	    REGION_INIT (pPixmap->drawable.pScreen, pRegion,
			 &pPixmapPriv->bitBox, 1);
	}

	/*
	 * We probably don't want accelerated drawing to this pixmap.
	 */
	pPixmapPriv->score = XGL_MIN_PIXMAP_SCORE;
    }

    /*
     * Maybe there's a nicer way to detect if this is the screen pixmap.
     */
    if (!pScreenPriv->pScreenPixmap)
    {
	glitz_surface_reference (pScreenPriv->surface);
	
	pPixmapPriv->surface = pScreenPriv->surface;
	pPixmapPriv->pPixel  = pScreenPriv->pVisual[0].pPixel;
	pPixmapPriv->target  = xglPixmapTargetIn;
	
	pScreenPriv->pScreenPixmap = pPixmap;
    }
    
    return TRUE;
}

RegionPtr
xglPixmapToRegion (PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    RegionPtr pRegion;
    
    XGL_SCREEN_PRIV (pScreen);
    
    if (!xglSyncBits (&pPixmap->drawable, NullBox))
	FatalError (XGL_SW_FAILURE_STRING);
    
    XGL_SCREEN_UNWRAP (BitmapToRegion);
    pRegion = (*pScreen->BitmapToRegion) (pPixmap);
    XGL_SCREEN_WRAP (BitmapToRegion, xglPixmapToRegion);

    return pRegion;
}

xglGeometryPtr
xglPixmapToGeometry (PixmapPtr pPixmap,
		     int       xOff,
		     int       yOff)
{
    XGL_PIXMAP_PRIV (pPixmap);

    if (pPixmap->devPrivate.ptr)
	xglUnmapPixmapBits (pPixmap);

    if (!pPixmapPriv->pGeometry)
    {
	xglGeometryPtr pGeometry;
	
	if (!pPixmapPriv->buffer)
	{
	    if (!xglAllocatePixmapBits (pPixmap))
		return NULL;
	}
	
	pGeometry = xalloc (sizeof (xglGeometryRec));
	if (!pGeometry)
	    return NULL;

	GEOMETRY_INIT (pPixmap->drawable.pScreen, pGeometry,
		       GLITZ_GEOMETRY_TYPE_BITMAP,
		       GEOMETRY_USAGE_DYNAMIC, 0);

	GEOMETRY_SET_BUFFER (pGeometry, pPixmapPriv->buffer);

	if (pPixmapPriv->stride < 0)
	{
	    pGeometry->f.bitmap.bytes_per_line = -pPixmapPriv->stride;
	    pGeometry->f.bitmap.scanline_order =
		GLITZ_PIXEL_SCANLINE_ORDER_BOTTOM_UP;
	}
	else
	{
	    pGeometry->f.bitmap.bytes_per_line = pPixmapPriv->stride;
	    pGeometry->f.bitmap.scanline_order =
		GLITZ_PIXEL_SCANLINE_ORDER_TOP_DOWN;
	}
	
	pGeometry->f.bitmap.pad = ((1 + FB_MASK) >> FB_SHIFT) *
	    sizeof (FbBits);
	pGeometry->width = pPixmap->drawable.width;
	pGeometry->count = pPixmap->drawable.height;

	pPixmapPriv->pGeometry = pGeometry;
    }

    pPixmapPriv->pGeometry->xOff = xOff << 16;
    pPixmapPriv->pGeometry->yOff = yOff << 16;

    return pPixmapPriv->pGeometry;
}

Bool
xglCreatePixmapSurface (PixmapPtr pPixmap)
{
    XGL_PIXMAP_PRIV (pPixmap);

    if (!pPixmapPriv->format)
	return FALSE;
    
    if (!pPixmapPriv->surface)
    {
	XGL_SCREEN_PRIV (pPixmap->drawable.pScreen);

	pPixmapPriv->surface =
	    glitz_surface_create (pScreenPriv->drawable,
				  pPixmapPriv->format,
				  pPixmap->drawable.width,
				  pPixmap->drawable.height,
				  0, NULL);
	if (!pPixmapPriv->surface)
	{
	    pPixmapPriv->format = NULL;
	    pPixmapPriv->target = xglPixmapTargetNo;
	    return FALSE;
	}
    }
    
    return TRUE;
}

Bool
xglAllocatePixmapBits (PixmapPtr pPixmap)
{
    int width, height, bpp, stride;
    
    XGL_PIXMAP_PRIV (pPixmap);

    width  = pPixmap->drawable.width;
    height = pPixmap->drawable.height;
    bpp    = pPixmap->drawable.bitsPerPixel;
    
    stride = ((width * bpp + FB_MASK) >> FB_SHIFT) * sizeof (FbBits);

    if (stride)
    {
	pPixmapPriv->bits = xalloc (height * stride);
	if (!pPixmapPriv->bits)
	    return FALSE;

	pPixmapPriv->buffer =
	    glitz_buffer_create_for_data (pPixmapPriv->bits);
	if (!pPixmapPriv->buffer)
	{
	    xfree (pPixmapPriv->bits);
	    pPixmapPriv->bits = NULL;
	    return FALSE;
	}	
    }

    /* XXX: pPixmapPriv->stride = -stride */
    pPixmapPriv->stride = stride;
    
    return TRUE;
}

Bool
xglMapPixmapBits (PixmapPtr pPixmap)
{
    if (!pPixmap->devPrivate.ptr)
    {
	CARD8 *bits;
	
	XGL_PIXMAP_PRIV (pPixmap);
	
	if (!pPixmapPriv->buffer)
	    if (!xglAllocatePixmapBits (pPixmap))
		return FALSE;
	
	bits = glitz_buffer_map (pPixmapPriv->buffer,
				 GLITZ_BUFFER_ACCESS_READ_WRITE);
	if (!bits)
	    return FALSE;

	pPixmap->devKind = pPixmapPriv->stride;
	if (pPixmapPriv->stride < 0)
	{
	    pPixmap->devPrivate.ptr = bits +
		(pPixmap->drawable.height - 1) * -pPixmapPriv->stride;
	}
	else
	{
	    pPixmap->devPrivate.ptr = bits;
	}
    }
    
    return TRUE;
}

Bool
xglUnmapPixmapBits (PixmapPtr pPixmap)
{
    XGL_PIXMAP_PRIV (pPixmap);

    pPixmap->devKind = 0;
    pPixmap->devPrivate.ptr = 0;
    
    if (pPixmapPriv->buffer)
	if (glitz_buffer_unmap (pPixmapPriv->buffer))
	    return FALSE;
    
    return TRUE;
}
