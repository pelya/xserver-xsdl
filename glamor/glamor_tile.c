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

/** @file glamor_tile.c
 *
 * Implements the basic fill-with-a-tile support used by multiple GC ops.
 */

void
glamor_init_tile_shader(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    const char *tile_vs =
        "attribute vec4 v_position;\n"
        "attribute vec4 v_texcoord0;\n"
        "varying vec2 tile_texture;\n"
	"void main()\n"
	"{\n"
        "       gl_Position = v_position;\n"
        "       tile_texture = v_texcoord0.xy;\n"
	"}\n";
    const char *tile_fs =
        "varying vec2 tile_texture;\n"
	"uniform sampler2D sampler;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(sampler, tile_texture);\n"
	"}\n";
    GLint fs_prog, vs_prog;
    GLint sampler_uniform_location;

    glamor_priv->tile_prog = glCreateProgram();
    vs_prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER, tile_vs);
    fs_prog = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER, tile_fs);
    glAttachShader(glamor_priv->tile_prog, vs_prog);
    glAttachShader(glamor_priv->tile_prog, fs_prog);

    glBindAttribLocation(glamor_priv->tile_prog, GLAMOR_VERTEX_POS, "v_position");
    glBindAttribLocation(glamor_priv->tile_prog, GLAMOR_VERTEX_SOURCE, "v_texcoord0");
    glamor_link_glsl_prog(glamor_priv->tile_prog);

    sampler_uniform_location =
	glGetUniformLocation(glamor_priv->tile_prog, "sampler");
    glUseProgram(glamor_priv->tile_prog);
    glUniform1i(sampler_uniform_location, 0);
    glUseProgram(0);
}

Bool
glamor_tile(PixmapPtr pixmap, PixmapPtr tile,
	    int x, int y, int width, int height,
	    unsigned char alu, unsigned long planemask,
	    int tile_x, int tile_y)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
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

    src_pixmap_priv = glamor_get_pixmap_private(tile);
    dst_pixmap_priv = glamor_get_pixmap_private(pixmap);

    if (((tile_x != 0) && (tile_x + width > tile->drawable.width))
         || ((tile_y != 0) && (tile_y + height > tile->drawable.height)))  {
       ErrorF("tile_x = %d tile_y = %d \n", tile_x, tile_y);
       goto fail;
    }
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
    if (alu != GXcopy) {
      glamor_set_destination_pixmap_priv_nc(src_pixmap_priv);
      glamor_validate_pixmap(tile);
    }
 
    glamor_set_destination_pixmap_priv_nc(dst_pixmap_priv);
    glamor_validate_pixmap(pixmap);
    pixmap_priv_get_scale(dst_pixmap_priv, &dst_xscale, &dst_yscale);

    glamor_set_alu(alu);

    if (GLAMOR_PIXMAP_PRIV_NO_PENDING(src_pixmap_priv)) {
      pixmap_priv_get_scale(src_pixmap_priv, &src_xscale, &src_yscale);
      glUseProgram(glamor_priv->tile_prog);
 
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, src_pixmap_priv->tex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
#ifndef GLAMOR_GLES2
      glEnable(GL_TEXTURE_2D);
#endif
      glamor_set_normalize_tcoords(src_xscale, src_yscale,
				 tile_x1, tile_y1,
				 tile_x2, tile_y2,
				 glamor_priv->yInverted,
				 source_texcoords);
      glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2, GL_FLOAT, GL_FALSE,
                            2 * sizeof(float),
                            source_texcoords);
      glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
   } 
   else {
     GLAMOR_CHECK_PENDING_FILL(glamor_priv, src_pixmap_priv);
   }
 
    glamor_set_normalize_vcoords(dst_xscale, dst_yscale,
				 x1, y1,x2, y2,
				 glamor_priv->yInverted,
				 vertices);

    glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT, GL_FALSE,
                          2 * sizeof(float),
                         vertices);
    glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    if (GLAMOR_PIXMAP_PRIV_NO_PENDING(src_pixmap_priv)) {
    glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
#ifndef GLAMOR_GLES2
    glDisable(GL_TEXTURE_2D);
#endif
    }
    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
    glUseProgram(0);
    glamor_set_alu(GXcopy);
    glamor_set_planemask(pixmap, ~0);
    return TRUE;

fail:
    return FALSE;
}
