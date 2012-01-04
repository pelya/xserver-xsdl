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

static Bool
_glamor_set_spans(DrawablePtr drawable, GCPtr gc, char *src,
		 DDXPointPtr points, int *widths, int n, int sorted,
		 Bool fallback)
{
	PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(drawable);
	glamor_pixmap_private *dest_pixmap_priv;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(drawable->pScreen);
	glamor_gl_dispatch *dispatch = &glamor_priv->dispatch;
	GLenum format, type;
	int no_alpha, no_revert, i;
	uint8_t *drawpixels_src = (uint8_t *) src;
	RegionPtr clip = fbGetCompositeClip(gc);
	BoxRec *pbox;
	int x_off, y_off;

	dest_pixmap_priv = glamor_get_pixmap_private(dest_pixmap);
	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(dest_pixmap_priv)) {
		glamor_fallback("pixmap has no fbo.\n");
		goto fail;
	}

	if (glamor_priv->gl_flavor == GLAMOR_GL_ES2) {
		glamor_fallback("ES2 fallback.\n");
		goto fail;
	}

	if (glamor_get_tex_format_type_from_pixmap(dest_pixmap,
						   &format,
						   &type, &no_alpha,
						   &no_revert)) {
		glamor_fallback("unknown depth. %d \n", drawable->depth);
		goto fail;
	}


	glamor_set_destination_pixmap_priv_nc(dest_pixmap_priv);
	glamor_validate_pixmap(dest_pixmap);
	if (!glamor_set_planemask(dest_pixmap, gc->planemask))
		goto fail;
	glamor_set_alu(dispatch, gc->alu);
	if (!glamor_set_planemask(dest_pixmap, gc->planemask))
		goto fail;

	glamor_get_drawable_deltas(drawable, dest_pixmap, &x_off, &y_off);

	for (i = 0; i < n; i++) {

		n = REGION_NUM_RECTS(clip);
		pbox = REGION_RECTS(clip);
		while (n--) {
			if (pbox->y1 > points[i].y)
				break;
			dispatch->glScissor(pbox->x1,
					    points[i].y + y_off,
					    pbox->x2 - pbox->x1, 1);
			dispatch->glEnable(GL_SCISSOR_TEST);
			dispatch->glRasterPos2i(points[i].x + x_off,
						points[i].y + y_off);
			dispatch->glDrawPixels(widths[i], 1, format,
					       type, drawpixels_src);
		}
		drawpixels_src +=
		    PixmapBytePad(widths[i], drawable->depth);
	}
	glamor_set_planemask(dest_pixmap, ~0);
	glamor_set_alu(dispatch, GXcopy);
	dispatch->glDisable(GL_SCISSOR_TEST);
	return TRUE;
      fail:
	if (!fallback
	    && glamor_ddx_fallback_check_pixmap(drawable))
		return FALSE;
	glamor_fallback("to %p (%c)\n",
			drawable, glamor_get_drawable_location(drawable));
	if (glamor_prepare_access(drawable, GLAMOR_ACCESS_RW)) {
		fbSetSpans(drawable, gc, src, points, widths, n, sorted);
		glamor_finish_access(drawable, GLAMOR_ACCESS_RW);
	}
	return TRUE;
}

void
glamor_set_spans(DrawablePtr drawable, GCPtr gc, char *src,
		    DDXPointPtr points, int *widths, int n, int sorted)
{
	_glamor_set_spans(drawable, gc, src, points, 
			     widths, n, sorted, TRUE);
}

Bool
glamor_set_spans_nf(DrawablePtr drawable, GCPtr gc, char *src,
		    DDXPointPtr points, int *widths, int n, int sorted)
{
	return _glamor_set_spans(drawable, gc, src, points, 
				    widths, n, sorted, FALSE);
}
