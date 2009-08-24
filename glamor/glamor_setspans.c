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

void
glamor_set_spans(DrawablePtr drawable, GCPtr gc, char *src,
		 DDXPointPtr points, int *widths, int n, int sorted)
{
    PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(drawable);
    GLenum format, type;
    uint8_t *temp_src = NULL, *drawpixels_src = (uint8_t *)src;
    int i, j;
    int wmax = 0;
    RegionPtr clip = fbGetCompositeClip(gc);
    BoxRec *pbox;

    for (i = 0 ; i < n; i++) {
	if (wmax < widths[i])
	    wmax = widths[i];
    }

    switch (drawable->depth) {
    case 1:
	temp_src = xalloc(wmax);
	format = GL_ALPHA;
	type = GL_UNSIGNED_BYTE;
	drawpixels_src = temp_src;
	break;
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
	ErrorF("Unknown setspans depth %d\n", drawable->depth);
	return;
    }

    if (!glamor_set_destination_pixmap(dest_pixmap))
	goto fail;
    if (!glamor_set_planemask(dest_pixmap, gc->planemask))
	goto fail;
    glamor_set_alu(gc->alu);
    if (!glamor_set_planemask(dest_pixmap, gc->planemask))
	goto fail;
    for (i = 0; i < n; i++) {
	if (temp_src) {
	    for (j = 0; j < widths[i]; j++) {
		if (src[j / 8] & (1 << (j % 8)))
		    temp_src[j] = 0xff;
		else
		    temp_src[j] = 0;
	    }
	}

	n = REGION_NUM_RECTS(clip);
	pbox = REGION_RECTS(clip);
	while (n--) {
	    if (pbox->y1 > points[i].y)
		break;
	    glScissor(pbox->x1,
		      points[i].y - dest_pixmap->screen_y,
		      pbox->x2 - pbox->x1,
		      1);
	    glEnable(GL_SCISSOR_TEST);
	    glRasterPos2i(points[i].x - dest_pixmap->screen_x,
			  points[i].y - dest_pixmap->screen_y);
	    glDrawPixels(widths[i],
			 1,
			 format, type,
			 drawpixels_src);
	}
	if (temp_src) {
	    src += PixmapBytePad(widths[i], drawable->depth);
	} else {
	    drawpixels_src += PixmapBytePad(widths[i], drawable->depth);
	}
    }
fail:
    glDisable(GL_SCISSOR_TEST);
    glamor_set_planemask(dest_pixmap, ~0);
    glamor_set_alu(GXcopy);
    xfree(temp_src);
}
