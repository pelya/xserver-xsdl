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
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "glamor_priv.h"

/** @file glamor_fillspans.c
 *
 * GC fill implementation, based loosely on fb_fill.c
 */

void
glamor_fill(DrawablePtr drawable,
	    GCPtr gc,
	    int x,
	    int y,
	    int width,
	    int height)
{
    PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(drawable);

    switch (gc->fillStyle) {
    case FillSolid:
	glamor_solid(dst_pixmap,
		     x - dst_pixmap->screen_x,
		     y - dst_pixmap->screen_y,
		     width,
		     height,
		     gc->alu,
		     gc->planemask,
		     gc->fgPixel);
	break;
    case FillStippled:
    case FillOpaqueStippled:
	glamor_stipple(dst_pixmap,
		       gc->stipple,
		       x - dst_pixmap->screen_x,
		       y - dst_pixmap->screen_y,
		       width,
		       height,
		       gc->alu,
		       gc->planemask,
		       gc->fgPixel,
		       gc->bgPixel,
		       gc->patOrg.x - dst_pixmap->screen_x,
		       gc->patOrg.y - dst_pixmap->screen_y);
	break;
    case FillTiled:
	glamor_tile(dst_pixmap,
		    gc->tile.pixmap,
		    x - dst_pixmap->screen_x,
		    y - dst_pixmap->screen_y,
		    width,
		    height,
		    gc->alu,
		    gc->planemask,
		    gc->patOrg.x - dst_pixmap->screen_y,
		    gc->patOrg.y - dst_pixmap->screen_y);
	break;
    }
}

void
glamor_init_solid_shader(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    const char *solid_vs_only =
	"uniform vec4 color;\n"
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
	"	gl_Color = color;\n"
	"}\n";
    const char *solid_vs =
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
	"}\n";
    const char *solid_fs =
	"uniform vec4 color;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = color;\n"
	"}\n";
    GLint fs_prog, vs_prog;

    glamor_priv->solid_prog = glCreateProgramObjectARB();
    if (GLEW_ARB_fragment_shader) {
	vs_prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER_ARB, solid_vs);
	fs_prog = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER_ARB, solid_fs);
	glAttachObjectARB(glamor_priv->solid_prog, vs_prog);
	glAttachObjectARB(glamor_priv->solid_prog, fs_prog);
    } else {
	vs_prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER_ARB, solid_vs_only);
	glAttachObjectARB(glamor_priv->solid_prog, vs_prog);
    }
    glamor_link_glsl_prog(glamor_priv->solid_prog);

    glamor_priv->solid_color_uniform_location =
	glGetUniformLocationARB(glamor_priv->solid_prog, "color");
    glamor_get_transform_uniform_locations(glamor_priv->solid_prog,
					   &glamor_priv->solid_transform);
}

void
glamor_solid(PixmapPtr pixmap, int x, int y, int width, int height,
	     unsigned char alu, unsigned long planemask, unsigned long fg_pixel)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    int x1 = x;
    int x2 = x + width;
    int y1 = y;
    int y2 = y + height;
    GLfloat color[4];

    if (!glamor_set_destination_pixmap(pixmap))
	return;

    glUseProgramObjectARB(glamor_priv->solid_prog);
    glamor_get_color_4f_from_pixel(pixmap, fg_pixel, color);
    glUniform4fvARB(glamor_priv->solid_color_uniform_location, 1, color);
    glamor_set_transform_for_pixmap(pixmap, &glamor_priv->solid_transform);

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x1, y1);
    glVertex2f(x1, y2);
    glVertex2f(x2, y2);
    glVertex2f(x2, y1);
    glEnd();

    glUseProgramObjectARB(0);
}

/* Highlight places where we're doing it wrong. */
void
glamor_solid_fail_region(PixmapPtr pixmap, int x, int y, int width, int height)
{
    unsigned long pixel;

    switch (pixmap->drawable.depth) {
    case 24:
    case 32:
	pixel = 0x00ff00ff; /* our favorite color */
	break;
    default:
    case 8:
	pixel = 0xd0d0d0d0;
	break;
    }

    glamor_solid(pixmap, x, y, width, height, GXcopy, ~0, pixel);
}
