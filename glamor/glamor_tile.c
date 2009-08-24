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
	"uniform float x_bias;\n"
	"uniform float x_scale;\n"
	"uniform float y_bias;\n"
	"uniform float y_scale;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = vec4((gl_Vertex.x + x_bias) * x_scale,\n"
	"			   (gl_Vertex.y + y_bias) * y_scale,\n"
	"			   0,\n"
	"			   1);\n"
	"	gl_TexCoord[0] = vec4(gl_MultiTexCoord0.xy, 0, 1);\n"
	"}\n";
    const char *tile_fs =
	"uniform sampler2D sampler;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(sampler, gl_TexCoord[0].xy);\n"
	"}\n";
    GLint fs_prog, vs_prog;
    GLint sampler_uniform_location;

    if (!GLEW_ARB_fragment_shader)
	return;

    glamor_priv->tile_prog = glCreateProgramObjectARB();
    vs_prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER_ARB, tile_vs);
    fs_prog = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER_ARB, tile_fs);
    glAttachObjectARB(glamor_priv->tile_prog, vs_prog);
    glAttachObjectARB(glamor_priv->tile_prog, fs_prog);
    glamor_link_glsl_prog(glamor_priv->tile_prog);

    sampler_uniform_location =
	glGetUniformLocationARB(glamor_priv->tile_prog, "sampler");
    glUseProgramObjectARB(glamor_priv->tile_prog);
    glUniform1iARB(sampler_uniform_location, 0);
    glamor_get_transform_uniform_locations(glamor_priv->tile_prog,
					   &glamor_priv->tile_transform);
}

void
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
    int tile_x1 = tile_x + tile->screen_x;
    int tile_x2 = tile_x + tile->screen_x + width;
    int tile_y1 = tile_y + tile->screen_y;
    int tile_y2 = tile_y + tile->screen_y + height;
    glamor_pixmap_private *tile_priv = glamor_get_pixmap_private(tile);

    if (glamor_priv->tile_prog == 0) {
	ErrorF("Tiling unsupported\n");
	goto fail;
    }

    if (!glamor_set_destination_pixmap(pixmap))
	goto fail;
    if (tile_priv->fb == 0) {
	ErrorF("Non-FBO tile pixmap\n");
	goto fail;
    }
    if (!glamor_set_planemask(pixmap, planemask))
	goto fail;
    glamor_set_alu(alu);
    glUseProgramObjectARB(glamor_priv->tile_prog);
    glamor_set_transform_for_pixmap(pixmap, &glamor_priv->tile_transform);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tile_priv->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glEnable(GL_TEXTURE_2D);

    tile_x += tile->screen_x;
    tile_y += tile->screen_y;

    glBegin(GL_TRIANGLE_FAN);
    glMultiTexCoord2f(0, tile_x1, tile_y1);
    glVertex2f(x1, y1);
    glMultiTexCoord2f(0, tile_x1, tile_y2);
    glVertex2f(x1, y2);
    glMultiTexCoord2f(0, tile_x2, tile_y2);
    glVertex2f(x2, y2);
    glMultiTexCoord2f(0, tile_x2, tile_y1);
    glVertex2f(x2, y1);
    glEnd();

    glUseProgramObjectARB(0);
    glDisable(GL_TEXTURE_2D);
    glamor_set_alu(GXcopy);
    glamor_set_planemask(pixmap, ~0);
    return;

fail:
    glamor_solid_fail_region(pixmap, x, y, width, height);
    return;
}
