/*
 * Copyright © 2008 Intel Corporation
 * Copyright © 1998 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *    Zhigang Gong <zhigang.gong@linux.intel.com>
 */

#include "glamor_priv.h"

/** @file glamor_fillspans.c
 *
 * GC fill implementation, based loosely on fb_fill.c
 */
Bool
glamor_fill(DrawablePtr drawable,
	    GCPtr gc, int x, int y, int width, int height, Bool fallback)
{
	PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(drawable);
	int off_x, off_y;
	PixmapPtr sub_pixmap = NULL;
	glamor_access_t sub_pixmap_access;
	DrawablePtr saved_drawable = NULL;
	int saved_x, saved_y;

	glamor_get_drawable_deltas(drawable, dst_pixmap, &off_x, &off_y);

	switch (gc->fillStyle) {
	case FillSolid:
		if (!glamor_solid(dst_pixmap,
				  x + off_x,
				  y + off_y,
				  width, height, gc->alu, gc->planemask,
				  gc->fgPixel))
			goto fail;
		break;
	case FillStippled:
	case FillOpaqueStippled:
		if (!glamor_stipple(dst_pixmap,
				    gc->stipple,
				    x + off_x,
				    y + off_y,
				    width,
				    height,
				    gc->alu,
				    gc->planemask,
				    gc->fgPixel,
				    gc->bgPixel, gc->patOrg.x,
				    gc->patOrg.y))
			goto fail;
		break;
	case FillTiled:
		if (!glamor_tile(dst_pixmap,
				 gc->tile.pixmap,
				 x + off_x,
				 y + off_y,
				 width,
				 height,
				 gc->alu,
				 gc->planemask,
				 x - drawable->x - gc->patOrg.x,
				 y - drawable->y - gc->patOrg.y))
			goto fail;
		break;
	}
	return TRUE;

      fail:
	if (!fallback) {
		if (glamor_ddx_fallback_check_pixmap(&dst_pixmap->drawable)
		   && glamor_ddx_fallback_check_gc(gc))
		return FALSE;
	}
	/* Is it possible to set the access as WO? */

	sub_pixmap_access = GLAMOR_ACCESS_RW;

	sub_pixmap = glamor_get_sub_pixmap(dst_pixmap, x + off_x,
					   y + off_y, width, height,
					   sub_pixmap_access);

	if (sub_pixmap != NULL) {
		if (gc->fillStyle != FillSolid) {
			gc->patOrg.x += (drawable->x - x);
			gc->patOrg.y += (drawable->y - y);
		}
		saved_drawable = drawable;
		drawable = &sub_pixmap->drawable;
		saved_x = x;
		saved_y = y;
		x = 0;
		y = 0;
	}
	if (glamor_prepare_access(drawable, GLAMOR_ACCESS_RW)) {
		if (glamor_prepare_access_gc(gc)) {
			fbFill(drawable, gc, x, y, width, height);
			glamor_finish_access_gc(gc);
		}
		glamor_finish_access(drawable, GLAMOR_ACCESS_RW);
	}

	if (sub_pixmap != NULL) {
		struct pixman_box16 box;
		int dx, dy;
		if (gc->fillStyle != FillSolid) {
			gc->patOrg.x -= (saved_drawable->x - saved_x);
			gc->patOrg.y -= (saved_drawable->y - saved_y);
		}

		x = saved_x;
		y = saved_y;

		glamor_put_sub_pixmap(sub_pixmap, dst_pixmap,
				      x + off_x, y + off_y,
				      width, height, sub_pixmap_access);
	}

	return TRUE;
}

void
glamor_init_solid_shader(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;
	const char *solid_vs =
	    "attribute vec4 v_position;"
	    "void main()\n" "{\n" "       gl_Position = v_position;\n"
	    "}\n";
	const char *solid_fs =
	    GLAMOR_DEFAULT_PRECISION "uniform vec4 color;\n"
	    "void main()\n" "{\n" "	gl_FragColor = color;\n" "}\n";
	GLint fs_prog, vs_prog;

	glamor_priv = glamor_get_screen_private(screen);
	dispatch =  glamor_get_dispatch(glamor_priv);
	glamor_priv->solid_prog = dispatch->glCreateProgram();
	vs_prog = glamor_compile_glsl_prog(dispatch, GL_VERTEX_SHADER, solid_vs);
	fs_prog = glamor_compile_glsl_prog(dispatch, GL_FRAGMENT_SHADER,
					   solid_fs);
	dispatch->glAttachShader(glamor_priv->solid_prog, vs_prog);
	dispatch->glAttachShader(glamor_priv->solid_prog, fs_prog);

	dispatch->glBindAttribLocation(glamor_priv->solid_prog,
				       GLAMOR_VERTEX_POS, "v_position");
	glamor_link_glsl_prog(dispatch, glamor_priv->solid_prog);

	glamor_priv->solid_color_uniform_location =
	    dispatch->glGetUniformLocation(glamor_priv->solid_prog,
					   "color");
	glamor_put_dispatch(glamor_priv);
}

void
glamor_fini_solid_shader(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;

	glamor_priv = glamor_get_screen_private(screen);
	dispatch = glamor_get_dispatch(glamor_priv);
	dispatch->glDeleteProgram(glamor_priv->solid_prog);
	glamor_put_dispatch(glamor_priv);
}

Bool
glamor_solid(PixmapPtr pixmap, int x, int y, int width, int height,
	     unsigned char alu, unsigned long planemask,
	     unsigned long fg_pixel)
{
	ScreenPtr screen = pixmap->drawable.pScreen;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	glamor_pixmap_private *pixmap_priv =
	    glamor_get_pixmap_private(pixmap);
	glamor_gl_dispatch *dispatch;
	int x1 = x;
	int x2 = x + width;
	int y1 = y;
	int y2 = y + height;
	GLfloat color[4];
	float vertices[8];
	GLfloat xscale, yscale;

	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv)) {
		glamor_fallback("dest %p has no fbo.\n", pixmap);
		return FALSE;
	}

	if (!glamor_set_planemask(pixmap, planemask)) {
		glamor_fallback
		    ("Failedto set planemask  in glamor_solid.\n");
		return FALSE;
	}

	glamor_get_rgba_from_pixel(fg_pixel,
				   &color[0],
				   &color[1],
				   &color[2],
				   &color[3], format_for_pixmap(pixmap));

	glamor_set_destination_pixmap_priv_nc(pixmap_priv);

	dispatch = glamor_get_dispatch(glamor_priv);
	if (!glamor_set_alu(dispatch, alu)) {
		if (alu == GXclear)
			color[0] = color[1] = color[2] = color[3] = 0.0;
		else {
			glamor_fallback("unsupported alu %x\n", alu);
			glamor_put_dispatch(glamor_priv);
			return FALSE;
		}
	}
	dispatch->glUseProgram(glamor_priv->solid_prog);

	dispatch->glUniform4fv(glamor_priv->solid_color_uniform_location,
			       1, color);

	dispatch->glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT,
					GL_FALSE, 2 * sizeof(float),
					vertices);
	dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
	pixmap_priv_get_scale(pixmap_priv, &xscale, &yscale);

	glamor_set_normalize_vcoords(xscale, yscale, x1, y1, x2, y2,
				     glamor_priv->yInverted, vertices);
	dispatch->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glUseProgram(0);
	glamor_set_alu(dispatch, GXcopy);
	glamor_put_dispatch(glamor_priv);
	return TRUE;
}
