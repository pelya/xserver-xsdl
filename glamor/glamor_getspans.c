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

#include "glamor_priv.h"

static Bool 
_glamor_get_spans(DrawablePtr drawable,
		  int wmax,
		  DDXPointPtr points, int *widths, int count, char *dst,
		  Bool fallback)
{
	PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
	GLenum format, type;
	int no_alpha, revert;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(drawable->pScreen);
	glamor_pixmap_private *pixmap_priv =
	    glamor_get_pixmap_private(pixmap);
	glamor_gl_dispatch *dispatch;
	glamor_pixmap_fbo *temp_fbo = NULL;
	int i;
	uint8_t *readpixels_dst = (uint8_t *) dst;
	int x_off, y_off;
	Bool ret = FALSE;
	int swap_rb;

	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv)) {
		glamor_fallback("pixmap has no fbo.\n");
		goto fail;
	}

	if (glamor_get_tex_format_type_from_pixmap(pixmap,
						   &format,
						   &type, &no_alpha,
						   &revert, &swap_rb, 0)) {
		glamor_fallback("unknown depth. %d \n", drawable->depth);
		goto fail;
	}

	if (revert > REVERT_NORMAL)
		goto fail;

	glamor_set_destination_pixmap_priv_nc(pixmap_priv);
	glamor_validate_pixmap(pixmap);

	if (glamor_priv->gl_flavor == GLAMOR_GL_ES2
	    && ( swap_rb != SWAP_NONE_DOWNLOADING
		 || revert != REVERT_NONE)) {

		/* XXX prepare whole pixmap is not efficient. */
		temp_fbo =
		    glamor_es2_pixmap_read_prepare(pixmap, 0, 0, pixmap->drawable.width, pixmap->drawable.height, format,
						   type, no_alpha,
						   revert, swap_rb);
		if (temp_fbo == NULL)
			goto fail;

	}

	glamor_get_drawable_deltas(drawable, pixmap, &x_off, &y_off);
	dispatch = glamor_get_dispatch(glamor_priv);
	for (i = 0; i < count; i++) {
		if (glamor_priv->yInverted) {
			dispatch->glReadPixels(points[i].x + x_off,
					       (points[i].y + y_off),
					       widths[i], 1, format,
					       type, readpixels_dst);
		} else {
			dispatch->glReadPixels(points[i].x + x_off,
					       pixmap->drawable.height -
					       1 - (points[i].y + y_off),
					       widths[i], 1, format,
					       type, readpixels_dst);
		}
		readpixels_dst +=
		    PixmapBytePad(widths[i], drawable->depth);
	}
	glamor_put_dispatch(glamor_priv);
	if (temp_fbo)
		glamor_destroy_fbo(temp_fbo);

	ret = TRUE;
	goto done;
fail:

	if (!fallback
	    && glamor_ddx_fallback_check_pixmap(drawable))
		goto done;

	ret = TRUE;
	glamor_fallback("from %p (%c)\n", drawable,
			glamor_get_drawable_location(drawable));
	if (glamor_prepare_access(drawable, GLAMOR_ACCESS_RO)) {
		fbGetSpans(drawable, wmax, points, widths, count, dst);
		glamor_finish_access(drawable, GLAMOR_ACCESS_RO);
	}
done:
	return ret;
}

void
glamor_get_spans(DrawablePtr drawable,
		 int wmax,
		 DDXPointPtr points, int *widths, int count, char *dst)
{
	_glamor_get_spans(drawable, wmax, points, 
			  widths, count, dst, TRUE);
}

Bool
glamor_get_spans_nf(DrawablePtr drawable,
		    int wmax,
		    DDXPointPtr points, int *widths, int count, char *dst)
{
	return _glamor_get_spans(drawable, wmax, points, 
				 widths, count, dst, FALSE);
}



