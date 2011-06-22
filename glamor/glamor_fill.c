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
    int x_off, y_off;
    glamor_get_drawable_deltas(drawable, dst_pixmap, &x_off, &y_off);

    switch (gc->fillStyle) {
    case FillSolid:
	if (!glamor_solid(dst_pixmap,
                         x + x_off,
                         y + y_off,
		         width,
		         height,
		         gc->alu,
		         gc->planemask,
		         gc->fgPixel))
	goto fail;
	break;
    case FillStippled:
    case FillOpaqueStippled:
	if (!glamor_stipple(dst_pixmap,
		       gc->stipple,
		       x+ x_off,
		       y + y_off,
		       width,
		       height,
		       gc->alu,
		       gc->planemask,
		       gc->fgPixel,
		       gc->bgPixel,
		       gc->patOrg.x + x_off,
		       gc->patOrg.y + y_off))
	goto fail;
        return;
	break;
    case FillTiled:
	if (!glamor_tile(dst_pixmap,
		    gc->tile.pixmap,
		    x + x_off,
		    y + y_off,
		    width,
		    height,
		    gc->alu,
		    gc->planemask,
		    drawable->x + x - gc->patOrg.x,
		    drawable->y + y - gc->patOrg.y))
	goto fail;
	break;
    }
    return;
fail:
    if (glamor_prepare_access(drawable, GLAMOR_ACCESS_RW)) {
	if (glamor_prepare_access_gc(gc)) {
            fbFill(drawable, gc, x, y, width, height);
	    glamor_finish_access_gc(gc);
	}
	glamor_finish_access(drawable);
    }
return;

}

void
glamor_init_solid_shader(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    const char *solid_vs_only =
	"uniform vec4 color;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = gl_Vertex;\n"
	"	gl_Color = color;\n"
	"}\n";
    const char *solid_vs =
	"void main()\n"
	"{\n"
	"	gl_Position = gl_Vertex;\n"
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
}

Bool
glamor_solid(PixmapPtr pixmap, int x, int y, int width, int height,
	     unsigned char alu, unsigned long planemask, unsigned long fg_pixel)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    int x1 = x;
    int x2 = x + width;
    int y1 = y;
    int y2 = y + height;
    GLfloat color[4];
    float vertices[8];
    GLfloat xscale, yscale;
    
    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv)) {
        glamor_fallback("dest has no fbo.\n");
	goto fail;
    }
    glamor_set_alu(alu);
    if (!glamor_set_planemask(pixmap, planemask)) {
      glamor_fallback("Failedto set planemask  in glamor_solid.\n");
      goto fail;
    } 

    glamor_set_destination_pixmap_priv_nc(pixmap_priv);

    glUseProgramObjectARB(glamor_priv->solid_prog);
    glamor_get_rgba_from_pixel(fg_pixel, 
                              &color[0], 
                              &color[1], 
                              &color[2], 
                              &color[3],
                              format_for_pixmap(pixmap));
    glUniform4fvARB(glamor_priv->solid_color_uniform_location, 1, color);

    glVertexPointer(2, GL_FLOAT, sizeof(float) * 2, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    pixmap_priv_get_scale(pixmap_priv, &xscale, &yscale);

    glamor_set_normalize_vcoords(xscale, yscale, x1, y1, x2, y2,
				 glamor_priv->yInverted,
				 vertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glUseProgramObjectARB(0);
    return TRUE;
fail:
    glamor_set_alu(GXcopy);
    glamor_set_planemask(pixmap, ~0);
    return FALSE;
}


