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

/** @file glamor_tile.c
 *
 * Implements the basic fill-with-a-tile support used by multiple GC ops.
 */

void
glamor_init_tile_shader(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;
	const char *tile_vs =
	    "attribute vec4 v_position;\n"
	    "attribute vec4 v_texcoord0;\n"
	    "varying vec2 tile_texture;\n"
	    "void main()\n"
	    "{\n"
	    "       gl_Position = v_position;\n"
	    "       tile_texture = v_texcoord0.xy;\n" "}\n";
	const char *tile_fs =
	    GLAMOR_DEFAULT_PRECISION
	    "varying vec2 tile_texture;\n"
	    "uniform sampler2D sampler;\n"
	    "uniform vec2	wh;"
	    "void main()\n"
	    "{\n"
	    "   vec2 rel_tex;"
	    "   rel_tex = tile_texture * wh; \n"
	    "   rel_tex = floor(rel_tex) + (fract(rel_tex) / wh); \n"
	    "	gl_FragColor = texture2D(sampler, rel_tex);\n"
	    "}\n";
	GLint fs_prog, vs_prog;
	GLint sampler_uniform_location;

	glamor_priv = glamor_get_screen_private(screen);
	dispatch = glamor_get_dispatch(glamor_priv);
	glamor_priv->tile_prog = dispatch->glCreateProgram();
	vs_prog = glamor_compile_glsl_prog(dispatch, GL_VERTEX_SHADER, tile_vs);
	fs_prog = glamor_compile_glsl_prog(dispatch, GL_FRAGMENT_SHADER,
					   tile_fs);
	dispatch->glAttachShader(glamor_priv->tile_prog, vs_prog);
	dispatch->glAttachShader(glamor_priv->tile_prog, fs_prog);

	dispatch->glBindAttribLocation(glamor_priv->tile_prog,
				       GLAMOR_VERTEX_POS, "v_position");
	dispatch->glBindAttribLocation(glamor_priv->tile_prog,
				       GLAMOR_VERTEX_SOURCE,
				       "v_texcoord0");
	glamor_link_glsl_prog(dispatch, glamor_priv->tile_prog);

	sampler_uniform_location =
	    dispatch->glGetUniformLocation(glamor_priv->tile_prog,
					   "sampler");
	dispatch->glUseProgram(glamor_priv->tile_prog);
	dispatch->glUniform1i(sampler_uniform_location, 0);

	glamor_priv->tile_wh =
	    dispatch->glGetUniformLocation(glamor_priv->tile_prog,
					   "wh");
	dispatch->glUseProgram(0);
	glamor_put_dispatch(glamor_priv);
}

void
glamor_fini_tile_shader(ScreenPtr screen)
{
	glamor_screen_private *glamor_priv;
	glamor_gl_dispatch *dispatch;

	glamor_priv = glamor_get_screen_private(screen);
	dispatch = glamor_get_dispatch(glamor_priv);
	dispatch->glDeleteProgram(glamor_priv->tile_prog);
	glamor_put_dispatch(glamor_priv);
}

Bool
glamor_tile(PixmapPtr pixmap, PixmapPtr tile,
	    int x, int y, int width, int height,
	    unsigned char alu, unsigned long planemask,
	    int tile_x, int tile_y)
{
	ScreenPtr screen = pixmap->drawable.pScreen;
	glamor_screen_private *glamor_priv =
	    glamor_get_screen_private(screen);
	glamor_gl_dispatch *dispatch;
	int x1 = x;
	int x2 = x + width;
	int y1 = y;
	int y2 = y + height;
	int tile_x1 = tile_x;
	int tile_x2 = tile_x + width;
	int tile_y1 = tile_y;
	int tile_y2 = tile_y + height;
	float vertices[8];
	float source_texcoords[8];
	GLfloat dst_xscale, dst_yscale, src_xscale, src_yscale;
	glamor_pixmap_private *src_pixmap_priv;
	glamor_pixmap_private *dst_pixmap_priv;
	float wh[2];

	src_pixmap_priv = glamor_get_pixmap_private(tile);
	dst_pixmap_priv = glamor_get_pixmap_private(pixmap);

	if (src_pixmap_priv == NULL || dst_pixmap_priv == NULL)
		goto fail;

	if (glamor_priv->tile_prog == 0) {
		glamor_fallback("Tiling unsupported\n");
		goto fail;
	}

	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(dst_pixmap_priv)) {
		glamor_fallback("dest has no fbo.\n");
		goto fail;
	}
	if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(src_pixmap_priv)) {
		/* XXX dynamic uploading candidate. */
		glamor_fallback("Non-texture tile pixmap\n");
		goto fail;
	}

	if (!glamor_set_planemask(pixmap, planemask)) {
		glamor_fallback("unsupported planemask %lx\n", planemask);
		goto fail;
	}

	glamor_set_destination_pixmap_priv_nc(dst_pixmap_priv);
	pixmap_priv_get_scale(dst_pixmap_priv, &dst_xscale, &dst_yscale);

	dispatch = glamor_get_dispatch(glamor_priv);
	if (!glamor_set_alu(dispatch, alu)) {
		glamor_put_dispatch(glamor_priv);
		goto fail;
	}

	pixmap_priv_get_scale(src_pixmap_priv, &src_xscale,
			      &src_yscale);
	dispatch->glUseProgram(glamor_priv->tile_prog);

	wh[0] = (float)src_pixmap_priv->fbo->width / tile->drawable.width;
	wh[1] = (float)src_pixmap_priv->fbo->height / tile->drawable.height;

	dispatch->glUniform2fv(glamor_priv->tile_wh, 1, wh);
	dispatch->glActiveTexture(GL_TEXTURE0);
	dispatch->glBindTexture(GL_TEXTURE_2D,
				src_pixmap_priv->fbo->tex);
	dispatch->glTexParameteri(GL_TEXTURE_2D,
				  GL_TEXTURE_MIN_FILTER,
				  GL_NEAREST);
	dispatch->glTexParameteri(GL_TEXTURE_2D,
				  GL_TEXTURE_MAG_FILTER,
				  GL_NEAREST);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
				  GL_REPEAT);
	dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
				  GL_REPEAT);
#ifndef GLAMOR_GLES2
	dispatch->glEnable(GL_TEXTURE_2D);
#endif
	glamor_set_normalize_tcoords(src_xscale, src_yscale,
				     tile_x1, tile_y1,
				     tile_x2, tile_y2,
				     glamor_priv->yInverted,
				     source_texcoords);
	dispatch->glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2,
					GL_FLOAT, GL_FALSE,
					2 * sizeof(float),
					source_texcoords);
	dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);

	glamor_set_normalize_vcoords(dst_xscale, dst_yscale,
				     x1, y1, x2, y2,
				     glamor_priv->yInverted, vertices);

	dispatch->glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT,
					GL_FALSE, 2 * sizeof(float),
					vertices);
	dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
#ifndef GLAMOR_GLES2
		dispatch->glDisable(GL_TEXTURE_2D);
#endif
	dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
	dispatch->glUseProgram(0);
	glamor_set_alu(dispatch, GXcopy);
	glamor_set_planemask(pixmap, ~0);
	glamor_put_dispatch(glamor_priv);
	return TRUE;

      fail:
	return FALSE;
}
