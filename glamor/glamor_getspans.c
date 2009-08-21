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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "glamor_priv.h"

static void
set_bit(uint8_t *bitfield, unsigned int index, unsigned int val)
{
    int i = index / 8;
    int mask = 1 << (index % 8);

    if (val)
	bitfield[i] |= mask;
    else
	bitfield[i] &= ~mask;
}

void
glamor_get_spans(DrawablePtr drawable,
		 int wmax,
		 DDXPointPtr points,
		 int *widths,
		 int count,
		 char *dst)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    GLenum format, type;
    int i, j;
    uint8_t *temp_dst = NULL, *readpixels_dst = (uint8_t *)dst;

    switch (drawable->depth) {
    case 1:
	temp_dst = xalloc(4 * wmax);
	format = GL_ALPHA;
	type = GL_UNSIGNED_BYTE;
	readpixels_dst = temp_dst;
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
	ErrorF("Unknown getspans depth %d\n", drawable->depth);
	return;
    }

    if (!glamor_set_destination_pixmap(pixmap)) {
	fbGetSpans(drawable, wmax, points, widths, count, dst);
	return;
    }

    for (i = 0; i < count; i++) {
	glReadPixels(points[i].x - pixmap->screen_x,
		     points[i].y - pixmap->screen_y,
		     widths[i],
		     1,
		     format, type,
		     readpixels_dst);
	if (temp_dst) {
	    for (j = 0; j < widths[i]; j++) {
		set_bit((uint8_t *)dst, j, temp_dst[j] & 0x1);
	    }
	    dst += PixmapBytePad(widths[i], drawable->depth);
	} else {
	    readpixels_dst += PixmapBytePad(widths[i], drawable->depth);
	}
    }
}
