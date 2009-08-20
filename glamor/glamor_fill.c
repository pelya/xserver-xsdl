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
