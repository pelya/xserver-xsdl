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
#include "fb.h"

Bool
xglSetPixels (DrawablePtr pDrawable,
	      char        *src,
	      int	  stride,
	      int	  x,
	      int	  y,
	      int	  width,
	      int	  height,
	      BoxPtr	  pBox,
	      int	  nBox)
{
    glitz_pixel_format_t format;
    glitz_surface_t	 *surface;
    FbBits		 *srcBits, *dstBits;
    FbStride		 srcStride, dstStride;
    BoxPtr		 pDstBox;
    int			 nDstBox;
    int			 dstXoff, dstYoff, dstBpp;
    int			 x1, y1, x2, y2;

    XGL_DRAWABLE_PIXMAP (pDrawable);
    XGL_PIXMAP_PRIV (pPixmap);

    if (!nBox)
       return TRUE;

    if (!xglSyncSurface (pDrawable))
	return FALSE;
    
    XGL_GET_DRAWABLE (pDrawable, surface, dstXoff, dstYoff);

    if (!xglMapPixmapBits (pPixmap))
	return FALSE;
    
    dstBpp = pDrawable->bitsPerPixel;

    srcBits = (FbBits *) src;
    dstBits = (FbBits *) pPixmap->devPrivate.ptr;
    
    srcStride = stride / sizeof (FbBits);
    dstStride = pPixmapPriv->stride / sizeof (FbBits);

    pDstBox = xalloc (nBox);
    if (!pDstBox)
	return FALSE;

    nDstBox = 0;

    while (nBox--)
    {
	x1 = x;
	y1 = y;
	x2 = x + width;
	y2 = y + height;

	if (x1 < pBox->x1)
	    x1 = pBox->x1;
	if (y1 < pBox->y1)
	    y1 = pBox->y1;
	if (x2 > pBox->x2)
	    x2 = pBox->x2;
	if (y2 > pBox->y2)
	    y2 = pBox->y2;
	
	if (x1 < x2 && y1 < y2)
	{
	    fbBlt (srcBits + (y1 - y) * srcStride,
		   srcStride,
		   (x1 - x) * dstBpp,
		   
		   dstBits + (y1 + dstYoff) * dstStride,
		   dstStride,
		   (x1 + dstXoff) * dstBpp,
		   
		   (x2 - x1) * dstBpp,
		   y2 - y1,
		   
		   GXcopy,
		   FB_ALLONES,
		   dstBpp,
		   FALSE,
		   FALSE);

	    pDstBox[nDstBox].x1 = x1;
	    pDstBox[nDstBox].y1 = y1;
	    pDstBox[nDstBox].x2 = x2;
	    pDstBox[nDstBox].y2 = y2;

	    nDstBox++;
	}
	pBox++;
    }

    xglUnmapPixmapBits (pPixmap);

    format.masks	  = pPixmapPriv->pPixel->masks;
    format.bytes_per_line = pPixmapPriv->stride;
    format.scanline_order = XGL_INTERNAL_SCANLINE_ORDER;

    pBox = pDstBox;

    while (nDstBox--)
    {
	format.xoffset    = pBox->x1 + dstXoff;
	format.skip_lines = pBox->y1 + dstYoff;
	
	glitz_set_pixels (surface,
			  pBox->x1 + dstXoff,
			  pBox->y1 + dstYoff,
			  pBox->x2 - pBox->x1,
			  pBox->y2 - pBox->y1,
			  &format,
			  pPixmapPriv->buffer);
	
	pBox++;
    }

    xfree (pDstBox);

    return TRUE;
}
