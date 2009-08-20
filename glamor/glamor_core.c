/*
 * Copyright © 2001 Keith Packard
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

/** @file glamor_core.c
 *
 * This file covers core X rendering in glamor.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include "glamor_priv.h"

Bool
glamor_set_destination_pixmap(PixmapPtr pixmap)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);

    if (pixmap != screen_pixmap) {
	ErrorF("stubbed drawing to non-screen pixmap\n");
	return FALSE;
    }

    glViewport(0, 0,
	       screen_pixmap->drawable.width,
	       screen_pixmap->drawable.height);

    return TRUE;
}

void
glamor_get_transform_uniform_locations(GLint prog,
				       glamor_transform_uniforms *uniform_locations)
{
    uniform_locations->x_bias = glGetUniformLocationARB(prog, "x_bias");
    uniform_locations->x_scale = glGetUniformLocationARB(prog, "x_scale");
    uniform_locations->y_bias = glGetUniformLocationARB(prog, "y_bias");
    uniform_locations->y_scale = glGetUniformLocationARB(prog, "y_scale");
}

/* We don't use a full matrix for our transformations because it's
 * wasteful when all we want is to rescale to NDC and possibly do a flip
 * if it's the front buffer.
 */
void
glamor_set_transform_for_pixmap(PixmapPtr pixmap,
				glamor_transform_uniforms *uniform_locations)
{
    glUniform1fARB(uniform_locations->x_bias, -pixmap->drawable.width / 2.0f);
    glUniform1fARB(uniform_locations->x_scale, 2.0f / pixmap->drawable.width);
    glUniform1fARB(uniform_locations->y_bias, -pixmap->drawable.height / 2.0f);
    glUniform1fARB(uniform_locations->y_scale, -2.0f / pixmap->drawable.height);
}

GLint
glamor_compile_glsl_prog(GLenum type, const char *source)
{
    GLint ok;
    GLint prog;

    prog = glCreateShaderObjectARB(type);
    glShaderSourceARB(prog, 1, (const GLchar **)&source, NULL);
    glCompileShaderARB(prog);
    glGetObjectParameterivARB(prog, GL_OBJECT_COMPILE_STATUS_ARB, &ok);
    if (!ok) {
	GLchar *info;
	GLint size;

	glGetObjectParameterivARB(prog, GL_OBJECT_INFO_LOG_LENGTH_ARB, &size);
	info = malloc(size);

	glGetInfoLogARB(prog, size, NULL, info);
	ErrorF("Failed to compile %s: %s\n",
	       type == GL_FRAGMENT_SHADER ? "FS" : "VS",
	       info);
	ErrorF("Program source:\n%s", source);
	FatalError("GLSL compile failure\n");
    }

    return prog;
}

void
glamor_link_glsl_prog(GLint prog)
{
    GLint ok;

    glLinkProgram(prog);
    glGetObjectParameterivARB(prog, GL_OBJECT_LINK_STATUS_ARB, &ok);
    if (!ok) {
	GLchar *info;
	GLint size;

	glGetObjectParameterivARB(prog, GL_OBJECT_INFO_LOG_LENGTH_ARB, &size);
	info = malloc(size);

	glGetInfoLogARB(prog, size, NULL, info);
	ErrorF("Failed to link: %s\n",
	       info);
	FatalError("GLSL link failure\n");
    }
}

static float ubyte_to_float(uint8_t b)
{
    return b / 255.0f;
}

void
glamor_get_color_4f_from_pixel(PixmapPtr pixmap, unsigned long fg_pixel,
			       GLfloat *color)
{
    if (pixmap->drawable.depth < 24) {
	ErrorF("pixmap with bad depth\n");
	color[0] = 1.0;
	color[1] = 0.0;
	color[2] = 1.0;
	color[3] = 1.0;
    } else {
	color[0] = ubyte_to_float(fg_pixel >> 16);
	color[1] = ubyte_to_float(fg_pixel >> 8);
	color[2] = ubyte_to_float(fg_pixel >> 0);
	color[3] = ubyte_to_float(fg_pixel >> 24);
    }
}

void
glamor_stipple(PixmapPtr pixmap, PixmapPtr stipple,
	       int x, int y, int width, int height,
	       unsigned char alu, unsigned long planemask,
	       unsigned long fg_pixel, unsigned long bg_pixel,
	       int stipple_x, int stipple_y)
{
    ErrorF("stubbed out stipple\n");
}

void
glamor_tile(PixmapPtr pixmap, PixmapPtr tile,
	    int x, int y, int width, int height,
	    unsigned char alu, unsigned long planemask,
	    int tile_x, int tile_y)
{
    ErrorF("stubbed out tile\n");
}

static void
glamor_put_image(DrawablePtr drawable, GCPtr gc, int depth, int x, int y,
		 int w, int h, int leftPad, int format, char *bits)
{
    ScreenPtr screen = drawable->pScreen;
    PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);
    PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(drawable);

    if (screen_pixmap != dest_pixmap) {
	fbPutImage(drawable, gc, depth, x, y, w, h, leftPad, format, bits);
    } else {
	ErrorF("stub put_image\n");
    }
}

static void
glamor_set_spans(DrawablePtr drawable, GCPtr gc, char *src,
		 DDXPointPtr points, int *widths, int n, int sorted)
{
    ScreenPtr screen = drawable->pScreen;
    PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);
    PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(drawable);

    if (screen_pixmap != dest_pixmap) {
	fbSetSpans(drawable, gc, src, points, widths, n, sorted);
    } else {
	GLenum format, type;
	int i;

	switch (drawable->depth) {
	case 24:
	case 32:
	    format = GL_BGRA;
	    type = GL_UNSIGNED_INT_8_8_8_8_REV;
	    break;
	default:
	    ErrorF("Unknown setspans depth %d\n", drawable->depth);
	    return;
	}

	if (!glamor_set_destination_pixmap(dest_pixmap))
	    return;
	for (i = 0; i < n; i++) {
	    glRasterPos2i(points[i].x - dest_pixmap->screen_x,
			  points[i].y - dest_pixmap->screen_y);
	    glDrawPixels(widths[i],
			 1,
			 format, type,
			 src);
	    src += PixmapBytePad(widths[i], drawable->depth);
	}
    }
}

/**
 * glamor_poly_lines() checks if it can accelerate the lines as a group of
 * horizontal or vertical lines (rectangles), and uses existing rectangle fill
 * acceleration if so.
 */
static void
glamor_poly_lines(DrawablePtr drawable, GCPtr gc, int mode, int n,
		  DDXPointPtr points)
{
    ScreenPtr screen = drawable->pScreen;
    PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);
    PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(drawable);
    xRectangle *rects;
    int x1, x2, y1, y2;
    int i;

    /* Don't try to do wide lines or non-solid fill style. */
    if (gc->lineWidth != 0 || gc->lineStyle != LineSolid ||
	gc->fillStyle != FillSolid) {
	if (dest_pixmap != screen_pixmap)
	    fbPolyLine(drawable, gc, mode, n, points);
	else
	    ErrorF("stub poly_line\n");
	return;
    }

    rects = xalloc(sizeof(xRectangle) * (n - 1));
    x1 = points[0].x;
    y1 = points[0].y;
    /* If we have any non-horizontal/vertical, fall back. */
    for (i = 0; i < n - 1; i++) {
	if (mode == CoordModePrevious) {
	    x2 = x1 + points[i + 1].x;
	    y2 = y1 + points[i + 1].y;
	} else {
	    x2 = points[i + 1].x;
	    y2 = points[i + 1].y;
	}

	if (x1 != x2 && y1 != y2) {
	    xfree(rects);
	    if (dest_pixmap != screen_pixmap)
		fbPolyLine(drawable, gc, mode, n, points);
	    else
		ErrorF("stub poly_line\n");
	    return;
	}

	if (x1 < x2) {
	    rects[i].x = x1;
	    rects[i].width = x2 - x1 + 1;
	} else {
	    rects[i].x = x2;
	    rects[i].width = x1 - x2 + 1;
	}
	if (y1 < y2) {
	    rects[i].y = y1;
	    rects[i].height = y2 - y1 + 1;
	} else {
	    rects[i].y = y2;
	    rects[i].height = y1 - y2 + 1;
	}

	x1 = x2;
	y1 = y2;
    }
    gc->ops->PolyFillRect(drawable, gc, n - 1, rects);
    xfree(rects);
}

GCOps glamor_gc_ops = {
    .FillSpans = glamor_fill_spans,
    .SetSpans = glamor_set_spans,
    .PutImage = glamor_put_image,
    .CopyArea = miCopyArea,
    .CopyPlane = miCopyPlane,
    .PolyPoint = miPolyPoint,
    .Polylines = glamor_poly_lines,
    .PolySegment = miPolySegment,
    .PolyRectangle = miPolyRectangle,
    .PolyArc = miPolyArc,
    .FillPolygon = miFillPolygon,
    .PolyFillRect = miPolyFillRect,
    .PolyFillArc = miPolyFillArc,
    .PolyText8 = miPolyText8,
    .PolyText16 = miPolyText16,
    .ImageText8 = miImageText8,
    .ImageText16 = miImageText16,
    .ImageGlyphBlt = miImageGlyphBlt,
    .PolyGlyphBlt = miPolyGlyphBlt,
    .PushPixels = miPushPixels,
};

/**
 * exaValidateGC() sets the ops to EXA's implementations, which may be
 * accelerated or may sync the card and fall back to fb.
 */
static void
glamor_validate_gc(GCPtr gc, unsigned long changes, DrawablePtr drawable)
{
    fbValidateGC(gc, changes, drawable);

    gc->ops = &glamor_gc_ops;
}

static GCFuncs glamor_gc_funcs = {
    glamor_validate_gc,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip
};

/**
 * exaCreateGC makes a new GC and hooks up its funcs handler, so that
 * exaValidateGC() will get called.
 */
int
glamor_create_gc(GCPtr gc)
{
    if (!fbCreateGC(gc))
	return FALSE;

    gc->funcs = &glamor_gc_funcs;

    return TRUE;
}
