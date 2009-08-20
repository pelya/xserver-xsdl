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

#ifndef GLAMOR_PRIV_H
#define GLAMOR_PRIV_H

#include "glamor.h"
#include <GL/glew.h>

typedef struct glamor_transform_uniforms {
    GLint x_bias;
    GLint x_scale;
    GLint y_bias;
    GLint y_scale;
} glamor_transform_uniforms;

typedef struct glamor_screen_private {
    CreateGCProcPtr saved_create_gc;
    CreatePixmapProcPtr saved_create_pixmap;
    DestroyPixmapProcPtr saved_destroy_pixmap;

    /* glamor_solid */
    GLint solid_prog;
    GLint solid_color_uniform_location;
    glamor_transform_uniforms solid_transform;
} glamor_screen_private;

extern DevPrivateKey glamor_screen_private_key;
static inline glamor_screen_private *
glamor_get_screen_private(ScreenPtr screen)
{
    return (glamor_screen_private *)dixLookupPrivate(&screen->devPrivates,
						     glamor_screen_private_key);
}

/* glamor.c */
PixmapPtr glamor_get_drawable_pixmap(DrawablePtr drawable);

/* glamor_core.c */
Bool glamor_create_gc(GCPtr gc);
void glamor_stipple(PixmapPtr pixmap, PixmapPtr stipple,
		    int x, int y, int width, int height,
		    unsigned char alu, unsigned long planemask,
		    unsigned long fg_pixel, unsigned long bg_pixel,
		    int stipple_x, int stipple_y);
void glamor_tile(PixmapPtr pixmap, PixmapPtr tile,
		 int x, int y, int width, int height,
		 unsigned char alu, unsigned long planemask,
		 int tile_x, int tile_y);
GLint glamor_compile_glsl_prog(GLenum type, const char *source);
void glamor_link_glsl_prog(GLint prog);
void glamor_get_color_4f_from_pixel(PixmapPtr pixmap, unsigned long fg_pixel,
				    GLfloat *color);
Bool glamor_set_destination_pixmap(PixmapPtr pixmap);
void glamor_get_transform_uniform_locations(GLint prog,
					    glamor_transform_uniforms *uniform_locations);
void glamor_set_transform_for_pixmap(PixmapPtr pixmap,
				     glamor_transform_uniforms *uniform_locations);

/* glamor_fill.c */
void glamor_fill(DrawablePtr drawable,
		 GCPtr gc,
		 int x,
		 int y,
		 int width,
		 int height);
void glamor_solid(PixmapPtr pixmap, int x, int y, int width, int height,
		  unsigned char alu, unsigned long planemask,
		  unsigned long fg_pixel);

/* glamor_fillspans.c */
void glamor_fill_spans(DrawablePtr drawable,
		       GCPtr	gc,
		       int n,
		       DDXPointPtr points,
		       int *widths,
		       int sorted);

void glamor_init_solid_shader(ScreenPtr screen);

#endif /* GLAMOR_PRIV_H */
