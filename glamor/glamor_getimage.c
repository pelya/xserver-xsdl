/*
 * Copyright © 2009 Intel Corporation
 * Copyright © 1998 Keith Packard
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
 *    Zhigang Gong <zhigang.gong@gmail.com>
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "glamor_priv.h"


static Bool
_glamor_get_image(DrawablePtr drawable, int x, int y, int w, int h,
		  unsigned int format, unsigned long planeMask, char *d,
		  Bool fallback)
{
	PixmapPtr pixmap;
	struct glamor_pixmap_private *pixmap_priv;
	struct glamor_screen_private *glamor_priv;
	int x_off, y_off;
	GLenum tex_format, tex_type;
	int no_alpha, no_revert;
	PixmapPtr temp_pixmap = NULL;
	glamor_gl_dispatch * dispatch;

	goto fall_back;
	if (format != ZPixmap)
		goto fall_back;

	pixmap = glamor_get_drawable_pixmap(drawable);
	if (!glamor_set_planemask(pixmap, planeMask)) {
		glamor_fallback
		    ("Failedto set planemask  in glamor_solid.\n");
		goto fall_back;
	}
	glamor_priv = glamor_get_screen_private(drawable->pScreen);
	pixmap_priv = glamor_get_pixmap_private(pixmap);
	dispatch = &glamor_priv->dispatch;


	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
		goto fall_back;
	glamor_get_drawable_deltas(drawable, pixmap, &x_off, &y_off);

	if (glamor_get_tex_format_type_from_pixmap(pixmap,
						   &tex_format,
						   &tex_type,
						   &no_alpha,
						   &no_revert)) {
		glamor_fallback("unknown depth. %d \n", drawable->depth);
		goto fall_back;
	}

	glamor_set_destination_pixmap_priv_nc(pixmap_priv);
	glamor_validate_pixmap(pixmap);

	if (glamor_priv->gl_flavor == GLAMOR_GL_ES2) {
		/* XXX prepare whole pixmap is not efficient. */
		temp_pixmap =
		    glamor_es2_pixmap_read_prepare(pixmap, &tex_format,
						   &tex_type, no_alpha,
						   no_revert);
		pixmap_priv = glamor_get_pixmap_private(temp_pixmap);
		glamor_set_destination_pixmap_priv_nc(pixmap_priv);
	}

	int row_length = PixmapBytePad(w, drawable->depth);
	row_length = (row_length * 8) / drawable->bitsPerPixel;
	if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
		dispatch->glPixelStorei(GL_PACK_ALIGNMENT, 1);
		dispatch->glPixelStorei(GL_PACK_ROW_LENGTH, row_length);
	} else {
		dispatch->glPixelStorei(GL_PACK_ALIGNMENT, 4);
	}

	if (glamor_priv->yInverted)
		dispatch->glReadPixels(x + x_off,
				       y + y_off,
				       w, h,
				       tex_format,
				       tex_type, d);
	else
		dispatch->glReadPixels(x + x_off,
				       pixmap->drawable.height - 1 - (y + y_off),
				       w,
				       h,
				       tex_format,
				       tex_type, d);
	if (temp_pixmap)
		glamor_destroy_pixmap(temp_pixmap);
	return TRUE;

fall_back:
	miGetImage(drawable, x, y, w, h, format, planeMask, d);
	return TRUE;
}

void
glamor_get_image(DrawablePtr pDrawable, int x, int y, int w, int h,
		 unsigned int format, unsigned long planeMask, char *d)
{
	_glamor_get_image(pDrawable, x, y, w, h, format, planeMask, d, TRUE);
	return;
}

Bool
glamor_get_image_nf(DrawablePtr pDrawable, int x, int y, int w, int h,
		    unsigned int format, unsigned long planeMask, char *d)
{
	return _glamor_get_image(pDrawable, x, y, w, 
				 h, format, planeMask, d, FALSE);
}
