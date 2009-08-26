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

/** @file glamor_fillspans.c
 *
 * GC PolyFillRect implementation, taken straight from fb_fill.c
 */

void
glamor_poly_fill_rect(DrawablePtr drawable,
		      GCPtr gc,
		      int nrect,
		      xRectangle *prect)
{
    RegionPtr	    clip = fbGetCompositeClip(gc);
    register BoxPtr box;
    BoxPtr	    pextent;
    int		    extentX1, extentX2, extentY1, extentY2;
    int		    fullX1, fullX2, fullY1, fullY2;
    int		    partX1, partX2, partY1, partY2;
    int		    xorg, yorg;
    int		    n;

    xorg = drawable->x;
    yorg = drawable->y;

    pextent = REGION_EXTENTS(gc->pScreen, clip);
    extentX1 = pextent->x1;
    extentY1 = pextent->y1;
    extentX2 = pextent->x2;
    extentY2 = pextent->y2;
    while (nrect--)
    {
	fullX1 = prect->x + xorg;
	fullY1 = prect->y + yorg;
	fullX2 = fullX1 + (int)prect->width;
	fullY2 = fullY1 + (int)prect->height;
	prect++;

	if (fullX1 < extentX1)
	    fullX1 = extentX1;

	if (fullY1 < extentY1)
	    fullY1 = extentY1;

	if (fullX2 > extentX2)
	    fullX2 = extentX2;

	if (fullY2 > extentY2)
	    fullY2 = extentY2;

	if ((fullX1 >= fullX2) || (fullY1 >= fullY2))
	    continue;
	n = REGION_NUM_RECTS(clip);
	if (n == 1) {
	    glamor_fill(drawable,
			gc,
			fullX1, fullY1, fullX2-fullX1, fullY2-fullY1);
	} else {
	    box = REGION_RECTS(clip);
	    /* clip the rectangle to each box in the clip region
	     * this is logically equivalent to calling Intersect()
	     */
	    while (n--) {
		partX1 = box->x1;
		if (partX1 < fullX1)
		    partX1 = fullX1;
		partY1 = box->y1;
		if (partY1 < fullY1)
		    partY1 = fullY1;
		partX2 = box->x2;
		if (partX2 > fullX2)
		    partX2 = fullX2;
		partY2 = box->y2;
		if (partY2 > fullY2)
		    partY2 = fullY2;

		box++;

		if (partX1 < partX2 && partY1 < partY2)
		    glamor_fill(drawable, gc,
				partX1, partY1,
				partX2 - partX1, partY2 - partY1);
	    }
	}
    }
}
