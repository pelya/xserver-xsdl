/*
 * Copyright © 2009 Intel Corporation
 * Copyright © 1998 Keith Packard
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

/** @file glamor_polylines.c
 *
 * GC PolyFillRect implementation, taken straight from fb_fill.c
 */

/**
 * glamor_poly_lines() checks if it can accelerate the lines as a group of
 * horizontal or vertical lines (rectangles), and uses existing rectangle fill
 * acceleration if so.
 */
void
glamor_poly_lines(DrawablePtr drawable, GCPtr gc, int mode, int n,
		  DDXPointPtr points)
{
    xRectangle *rects;
    int x1, x2, y1, y2;
    int i;

    /* Don't try to do wide lines or non-solid fill style. */
    if (gc->lineWidth != 0) {
	ErrorF("stub wide polylines\n");
	return;
    }
    if (gc->lineStyle != LineSolid ||
	gc->fillStyle != FillSolid) {
	ErrorF("stub poly_line non-solid fill\n");
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
	    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);

	    xfree(rects);

	    ErrorF("stub diagonal poly_line\n");
	    glamor_solid_fail_region(pixmap, x1, y1, x2 - x1, y2 - y1);
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
