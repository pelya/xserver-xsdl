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
glamor_fill_spans(DrawablePtr drawable, GCPtr gc, int n,
		  DDXPointPtr points, int *width, int sorted)
{
    ScreenPtr screen = drawable->pScreen;
    PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);
    PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(drawable);

    if (1 || screen_pixmap != dest_pixmap) {
	fbFillSpans(drawable, gc, n, points, width, sorted);
    } else {
	ErrorF("stub fill_spans\n");
    }
}

static void
glamor_put_image(DrawablePtr drawable, GCPtr gc, int depth, int x, int y,
		 int w, int h, int leftPad, int format, char *bits)
{
    ScreenPtr screen = drawable->pScreen;
    PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);
    PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(drawable);

    if (1 || screen_pixmap != dest_pixmap) {
	fbPutImage(drawable, gc, depth, x, y, w, h, leftPad, format, bits);
    } else {
	ErrorF("stub put_image\n");
    }
}

static void
glamor_set_spans(DrawablePtr drawable, GCPtr gc, char *src,
		 DDXPointPtr points, int *width, int n, int sorted)
{
    ScreenPtr screen = drawable->pScreen;
    PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);
    PixmapPtr dest_pixmap = glamor_get_drawable_pixmap(drawable);

    if (1 || screen_pixmap != dest_pixmap) {
	fbSetSpans(drawable, gc, src, points, width, n, sorted);
    } else {
	ErrorF("stub set_spans\n");
    }
}


/**
 * glamor_poly_line() checks if it can accelerate the lines as a group of
 * horizontal or vertical lines (rectangles), and uses existing rectangle fill
 * acceleration if so.
 */
static void
glamor_poly_line(DrawablePtr drawable, GCPtr gc, int mode, int n,
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
	if (1 || dest_pixmap != screen_pixmap)
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
	    if (1 || dest_pixmap != screen_pixmap)
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
    glamor_fill_spans,
    glamor_set_spans,
    glamor_put_image,
    miCopyArea,
    miCopyPlane,
    miPolyPoint,
    glamor_poly_line,
    miPolySegment,
    miPolyRectangle,
    miPolyArc,
    miFillPolygon,
    miPolyFillRect,
    miPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    miImageGlyphBlt,
    miPolyGlyphBlt,
    miPushPixels,
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
