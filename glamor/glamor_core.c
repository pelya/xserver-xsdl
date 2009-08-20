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

static void
glamor_set_color_from_fgpixel(PixmapPtr pixmap, unsigned long fg_pixel)
{
    glColor4ub((fg_pixel >> 16) & 0xff,
	       (fg_pixel >> 8) & 0xff,
	       (fg_pixel) & 0xff,
	       (fg_pixel >> 24) & 0xff);
}

static Bool
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

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screen_pixmap->drawable.width,
	    0, screen_pixmap->drawable.height,
	    -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    return TRUE;
}

void
glamor_solid(PixmapPtr pixmap, int x, int y, int width, int height,
	     unsigned char alu, unsigned long planemask, unsigned long fg_pixel)
{
    int x1 = x;
    int x2 = x + width;
    int y1 = y;
    int y2 = y + height;

    if (!glamor_set_destination_pixmap(pixmap))
	return;
    glamor_set_color_from_fgpixel(pixmap, fg_pixel);

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x1, y1);
    glVertex2f(x1, y2);
    glVertex2f(x2, y2);
    glVertex2f(x2, y1);
    glEnd();
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
	int i;

	if (!glamor_set_destination_pixmap(dest_pixmap))
	    return;
	for (i = 0; i < n; i++) {
	    glRasterPos2i(points[i].x - dest_pixmap->screen_x,
			  points[i].y - dest_pixmap->screen_y);
	    glDrawPixels(widths[i],
			 1,
			 GL_RGBA, GL_UNSIGNED_BYTE,
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
