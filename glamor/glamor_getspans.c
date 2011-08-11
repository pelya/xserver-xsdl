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
glamor_get_spans(DrawablePtr drawable,
		 int wmax,
		 DDXPointPtr points,
		 int *widths,
		 int count,
		 char *dst)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    GLenum format, type;
    int no_alpha;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(drawable->pScreen);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    int i;
    uint8_t *readpixels_dst = (uint8_t *)dst;
    int x_off, y_off;

    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv)) {
        glamor_fallback("pixmap has no fbo.\n");
	goto fail;
    }

    if (glamor_get_tex_format_type_from_pixmap(pixmap,
                                               &format, 
                                               &type, 
                                               &no_alpha
                                               )) {
      glamor_fallback("unknown depth. %d \n", 
                     drawable->depth);
      goto fail;
    }

    glamor_set_destination_pixmap_priv_nc(pixmap_priv);
    glamor_validate_pixmap(pixmap);
 
    glamor_get_drawable_deltas(drawable, pixmap, &x_off, &y_off);
    for (i = 0; i < count; i++) {
      if (glamor_priv->yInverted) {
	glReadPixels(points[i].x + x_off,
		     (points[i].y + y_off),
		     widths[i],
		     1,
		     format, type,
		     readpixels_dst);
 	} else {
	glReadPixels(points[i].x + x_off,
		     pixmap->drawable.height - 1 - (points[i].y + y_off),
		     widths[i],
		     1,
		     format, type,
		     readpixels_dst);
	}
       readpixels_dst += PixmapBytePad(widths[i], drawable->depth);
    }
    return;

fail:
    glamor_fallback("from %p (%c)\n", drawable,
		    glamor_get_drawable_location(drawable));
    if (glamor_prepare_access(drawable, GLAMOR_ACCESS_RO)) {
	fbGetSpans(drawable, wmax, points, widths, count, dst);
	glamor_finish_access(drawable);
    }
}
