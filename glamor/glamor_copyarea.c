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

/** @file glamor_copyarea.c
 *
 * GC CopyArea implementation
 */

static void
glamor_copy_n_to_n(DrawablePtr src,
		 DrawablePtr dst,
		 GCPtr gc,
		 BoxPtr box,
		 int nbox,
		 int		dx,
		 int		dy,
		 Bool		reverse,
		 Bool		upsidedown,
		 Pixel		bitplane,
		 void		*closure)
{
    PixmapPtr src_pixmap = glamor_get_drawable_pixmap(src);
    PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(dst);
    int i;

    glamor_set_alu(gc->alu);
    if (!glamor_set_planemask(dst_pixmap, gc->planemask))
	goto fail;

    for (i = 0; i < nbox; i++) {
	glRasterPos2i(box[i].x1 - dst_pixmap->screen_x,
		      box[i].y1 - dst_pixmap->screen_y);
	glCopyPixels(box[i].x1 + dx - src_pixmap->screen_x,
		     box[i].y1 + dy - src_pixmap->screen_y,
		     box[i].x2 - box[i].x1,
		     box[i].y2 - box[i].y1,
		     GL_COLOR);
    }

fail:
    glamor_set_alu(GXcopy);
    glamor_set_planemask(dst_pixmap, ~0);
}

RegionPtr
glamor_copy_area(DrawablePtr src, DrawablePtr dst, GCPtr gc,
		 int srcx, int srcy, int width, int height, int dstx, int dsty)
{
    ScreenPtr screen = dst->pScreen;
    PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);
    PixmapPtr src_pixmap = glamor_get_drawable_pixmap(src);
    PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(dst);
    glamor_pixmap_private *src_priv = glamor_get_pixmap_private(src_pixmap);
    RegionPtr region;

    if (!GLEW_EXT_framebuffer_blit) {
	ErrorF("EXT_framebuffer_blit unsupported\n");
	goto fail;
    }

    if (!glamor_set_destination_pixmap(dst_pixmap)) {
	/*
	return miDoCopy(pSrcDrawable, pDstDrawable, pGC,
			srcx, srcy, width, height,
			dstx, dsty, fbCopyNtoN, 0, NULL);
	*/
	return NULL;
    }

    if (src_priv == NULL) {
	ErrorF("glamor_copy_area: no src pixmap priv?");
	goto fail;
    }

    if (src_priv->fb == 0 && src_pixmap != screen_pixmap) {
	ErrorF("glamor_copy_area: No src FBO\n");
	return NULL;
    }

    glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, src_priv->fb);

    region = miDoCopy(src, dst, gc,
		      srcx, srcy, width, height,
		      dstx, dsty, glamor_copy_n_to_n, 0, NULL);

    return region;

fail:
    return NULL;
}
