/*
 * Copyright © 2008 Intel Corporation
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

#ifndef GLAMOR_H
#define GLAMOR_H

#include "scrnintstr.h"
#ifdef GLAMOR_FOR_XORG
#include "xf86str.h"
#endif
#include "pixmapstr.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "picturestr.h"
#include "fb.h"
#include "fbpict.h"

#endif				/* GLAMOR_H */


#define GLAMOR_INVERTED_Y_AXIS  	1
#define GLAMOR_USE_SCREEN		2
#define GLAMOR_USE_PICTURE_SCREEN 	4

#define GLAMOR_VALID_FLAGS      (GLAMOR_INVERTED_Y_AXIS  		\
				 | GLAMOR_USE_SCREEN 			\
                                 | GLAMOR_USE_PICTURE_SCREEN)

/*
 * glamor_pixmap_type : glamor pixmap's type.
 * @MEMORY: pixmap is in memory.
 * @TEXTURE_DRM: pixmap is in a texture created from a DRM buffer.
 * @DRM_ONLY: pixmap is in a external DRM buffer.
 * @TEXTURE_ONLY: pixmap is in an internal texture.
 */
typedef enum  glamor_pixmap_type {
	GLAMOR_MEMORY,
	GLAMOR_TEXTURE_DRM,
	GLAMOR_DRM_ONLY,
	GLAMOR_TEXTURE_ONLY
} glamor_pixmap_type_t;

#define GLAMOR_EGL_EXTERNAL_BUFFER 3

extern _X_EXPORT Bool glamor_init(ScreenPtr screen, unsigned int flags);
extern _X_EXPORT void glamor_fini(ScreenPtr screen);
extern _X_EXPORT void glamor_set_screen_pixmap_texture(ScreenPtr screen,
						       int w, int h,
						       unsigned int tex);
extern _X_EXPORT Bool glamor_glyphs_init(ScreenPtr pScreen);

extern _X_EXPORT void glamor_set_pixmap_texture(PixmapPtr pixmap, int w, int h,
						unsigned int tex);

extern _X_EXPORT void glamor_set_pixmap_type(PixmapPtr pixmap, glamor_pixmap_type_t type);
extern _X_EXPORT void glamor_destroy_textured_pixmap(PixmapPtr pixmap);
extern _X_EXPORT void glamor_block_handler(ScreenPtr screen);

#ifdef GLAMOR_FOR_XORG
extern _X_EXPORT Bool glamor_egl_init(ScrnInfoPtr scrn, int fd);
extern _X_EXPORT Bool glamor_egl_create_textured_screen(ScreenPtr screen,
							int handle,
							int stride);
extern _X_EXPORT Bool glamor_egl_create_textured_pixmap(PixmapPtr pixmap,
							int handle,
							int stride);

extern _X_EXPORT Bool glamor_egl_close_screen(ScreenPtr screen);
extern _X_EXPORT void glamor_egl_free_screen(int scrnIndex, int flags);

extern _X_EXPORT Bool glamor_egl_init_textured_pixmap(ScreenPtr screen);
extern _X_EXPORT void glamor_egl_destroy_textured_pixmap(PixmapPtr pixmap);
#endif

extern _X_EXPORT Bool glamor_fill_spans_nf(DrawablePtr drawable,
					   GCPtr gc,
					   int n, DDXPointPtr points, 
					   int *widths, int sorted);

extern _X_EXPORT Bool glamor_poly_fill_rect_nf(DrawablePtr drawable,
					       GCPtr gc, 
					       int nrect, 
					       xRectangle * prect);

extern _X_EXPORT Bool glamor_put_image_nf(DrawablePtr drawable, 
					  GCPtr gc, int depth, int x, int y,
		 	 		  int w, int h, int left_pad, 
					  int image_format, char *bits);

extern _X_EXPORT Bool glamor_copy_n_to_n_nf(DrawablePtr src,
					    DrawablePtr dst,
					    GCPtr gc,
					    BoxPtr box,
					    int nbox,
					    int dx,
					    int dy,
					    Bool reverse,
					    Bool upsidedown, Pixel bitplane, 
					    void *closure);

extern _X_EXPORT Bool glamor_composite_nf(CARD8 op,
					  PicturePtr source,
					  PicturePtr mask,
					  PicturePtr dest,
					  INT16 x_source,
					  INT16 y_source,
					  INT16 x_mask,
					  INT16 y_mask,
					  INT16 x_dest, INT16 y_dest, 
					  CARD16 width, CARD16 height);

extern _X_EXPORT Bool glamor_trapezoids_nf(CARD8 op,
					   PicturePtr src, PicturePtr dst,
					   PictFormatPtr mask_format, 
					   INT16 x_src, INT16 y_src,
					   int ntrap, xTrapezoid * traps);

extern _X_EXPORT Bool glamor_glyphs_nf(CARD8 op,
				       PicturePtr src,
				       PicturePtr dst,
				       PictFormatPtr mask_format,
				       INT16 x_src,
				       INT16 y_src, int nlist, 
				       GlyphListPtr list, GlyphPtr * glyphs);

extern _X_EXPORT Bool glamor_triangles_nf(CARD8 op,
					  PicturePtr pSrc,
					  PicturePtr pDst,
					  PictFormatPtr maskFormat,
					  INT16 xSrc, INT16 ySrc, 
					  int ntris, xTriangle * tris);


extern _X_EXPORT void glamor_glyph_unrealize(ScreenPtr screen, GlyphPtr glyph);


