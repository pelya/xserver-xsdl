/*
 * Copyright © 2009 Intel Corporation
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Junyan He <junyan.he@linux.intel.com>
 *
 */

/** @file glamor_trapezoid.c
 *
 * Trapezoid acceleration implementation
 */

#include "glamor_priv.h"

#ifdef RENDER
#include "mipict.h"
#include "fbpict.h"

/**
 * Creates an appropriate picture to upload our alpha mask into (which
 * we calculated in system memory)
 */
static PicturePtr
glamor_create_mask_picture(ScreenPtr screen,
			   PicturePtr dst,
			   PictFormatPtr pict_format,
			   CARD16 width, CARD16 height)
{
	PixmapPtr pixmap;
	PicturePtr picture;
	int error;

	if (!pict_format) {
		if (dst->polyEdge == PolyEdgeSharp)
			pict_format =
			    PictureMatchFormat(screen, 1, PICT_a1);
		else
			pict_format =
			    PictureMatchFormat(screen, 8, PICT_a8);
		if (!pict_format)
			return 0;
	}

	pixmap = glamor_create_pixmap(screen, 0, 0,
				      pict_format->depth,
				      GLAMOR_CREATE_PIXMAP_CPU);
	if (!pixmap)
		return 0;
	picture = CreatePicture(0, &pixmap->drawable, pict_format,
				0, 0, serverClient, &error);
	screen->DestroyPixmap(pixmap);
	return picture;
}

/**
 * glamor_trapezoids is a copy of miTrapezoids that does all the trapezoid
 * accumulation in system memory.
 */
static Bool
_glamor_trapezoids(CARD8 op,
		  PicturePtr src, PicturePtr dst,
		  PictFormatPtr mask_format, INT16 x_src, INT16 y_src,
		  int ntrap, xTrapezoid * traps, Bool fallback)
{
	ScreenPtr screen = dst->pDrawable->pScreen;
	BoxRec bounds;
	PicturePtr picture;
	INT16 x_dst, y_dst;
	INT16 x_rel, y_rel;
	int width, height, stride;
	PixmapPtr pixmap;
	pixman_image_t *image;

	/* If a mask format wasn't provided, we get to choose, but behavior should
	 * be as if there was no temporary mask the traps were accumulated into.
	 */
	if (!mask_format) {
		if (dst->polyEdge == PolyEdgeSharp)
			mask_format =
			    PictureMatchFormat(screen, 1, PICT_a1);
		else
			mask_format =
			    PictureMatchFormat(screen, 8, PICT_a8);
		for (; ntrap; ntrap--, traps++)
			glamor_trapezoids(op, src, dst, mask_format, x_src,
					  y_src, 1, traps);
		return TRUE;
	}

	miTrapezoidBounds(ntrap, traps, &bounds);

	if (bounds.y1 >= bounds.y2 || bounds.x1 >= bounds.x2)
		return TRUE;

	x_dst = traps[0].left.p1.x >> 16;
	y_dst = traps[0].left.p1.y >> 16;

	width = bounds.x2 - bounds.x1;
	height = bounds.y2 - bounds.y1;
	stride = PixmapBytePad(width, mask_format->depth);
	picture = glamor_create_mask_picture(screen, dst, mask_format,
					     width, height);
	if (!picture)
		return TRUE;

	image = pixman_image_create_bits(picture->format,
					 width, height, NULL, stride);
	if (!image) {
		FreePicture(picture, 0);
		return TRUE;
	}

	for (; ntrap; ntrap--, traps++)
		pixman_rasterize_trapezoid(image,
					   (pixman_trapezoid_t *) traps,
					   -bounds.x1, -bounds.y1);

	pixmap = glamor_get_drawable_pixmap(picture->pDrawable);

	screen->ModifyPixmapHeader(pixmap, width, height,
				   mask_format->depth,
				   BitsPerPixel(mask_format->depth),
				   PixmapBytePad(width,
						 mask_format->depth),
				   pixman_image_get_data(image));

	x_rel = bounds.x1 + x_src - x_dst;
	y_rel = bounds.y1 + y_src - y_dst;
	CompositePicture(op, src, picture, dst,
			 x_rel, y_rel,
			 0, 0,
			 bounds.x1, bounds.y1,
			 bounds.x2 - bounds.x1, bounds.y2 - bounds.y1);

	pixman_image_unref(image);

	FreePicture(picture, 0);
	return TRUE;
}

void
glamor_trapezoids(CARD8 op,
		  PicturePtr src, PicturePtr dst,
		  PictFormatPtr mask_format, INT16 x_src, INT16 y_src,
		  int ntrap, xTrapezoid * traps)
{
	_glamor_trapezoids(op, src, dst, mask_format, x_src,
			   y_src, ntrap, traps, TRUE);
}

Bool
glamor_trapezoids_nf(CARD8 op,
		     PicturePtr src, PicturePtr dst,
		     PictFormatPtr mask_format, INT16 x_src, INT16 y_src,
		     int ntrap, xTrapezoid * traps)
{
	return _glamor_trapezoids(op, src, dst, mask_format, x_src,
				  y_src, ntrap, traps, FALSE);
}

#endif				/* RENDER */

