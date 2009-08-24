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
 *    Eric Anholt <eric@anholt.net>
 *
 */


/** @file glamor_putaimge.c
 *
 * XPutImage implementation
 */
#include "glamor_priv.h"

void
glamor_put_image(DrawablePtr drawable, GCPtr gc, int depth, int x, int y,
		 int w, int h, int left_pad, int image_format, char *bits)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    GLenum type, format;
    RegionPtr clip;
    BoxPtr pbox;
    int nbox;
    int bpp = drawable->bitsPerPixel;
    int src_stride = PixmapBytePad(w, drawable->depth);

    if (!glamor_set_destination_pixmap(pixmap)) {
	fbPutImage(drawable, gc, depth, x, y, w, h, left_pad,
		   image_format, bits);
	return;
    }

    if (!glamor_set_planemask(pixmap, gc->planemask))
	goto fail;
    if (image_format != ZPixmap) {
	ErrorF("putimage: non-ZPixmap\n");
	goto fail;
    }
    if (bpp < 8) {
	ErrorF("putimage: bad bpp: %d\n", bpp);
	goto fail;
    }

    switch (drawable->depth) {
    case 8:
	format = GL_ALPHA;
	type = GL_UNSIGNED_BYTE;
	break;
    case 24:
	format = GL_RGB;
	type = GL_UNSIGNED_BYTE;
	break;
    case 32:
	format = GL_BGRA;
	type = GL_UNSIGNED_INT_8_8_8_8_REV;
	break;
    default:
	ErrorF("stub put_image depth %d\n", drawable->depth);
	goto fail;
	break;
    }

    glamor_set_alu(gc->alu);

    x += drawable->x;
    y += drawable->y;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, src_stride / (bpp / 8));
    clip = fbGetCompositeClip(gc);
    for (nbox = REGION_NUM_RECTS(clip),
	 pbox = REGION_RECTS(clip);
	 nbox--;
	 pbox++)
    {
	int x1 = x;
	int y1 = y;
	int x2 = x + w;
	int y2 = y + h;
	char *src;

	if (x1 < pbox->x1)
	    x1 = pbox->x1;
	if (y1 < pbox->y1)
	    y1 = pbox->y1;
	if (x2 > pbox->x2)
	    x2 = pbox->x2;
	if (y2 > pbox->y2)
	    y2 = pbox->y2;
	if (x1 >= x2 || y1 >= y2)
	    continue;

	src = bits + (y1 - y) * src_stride + (x1 - x) * (bpp / 8);
	glRasterPos2i(x1 - pixmap->screen_x, y1 - pixmap->screen_y);
	glDrawPixels(x2 - x1,
		     y2 - y1,
		     format, type,
		     src);
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glamor_set_alu(GXcopy);
    glamor_set_planemask(pixmap, ~0);
    return;

fail:
    glamor_set_planemask(pixmap, ~0);
    glamor_solid_fail_region(pixmap, x, y, w, h);
}
