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

void
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
    glamor_screen_private *glamor_priv =
	glamor_get_screen_private(dst->pScreen);
    PixmapPtr src_pixmap = glamor_get_drawable_pixmap(src);
    PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(dst);
    int i;
    float vertices[4][2], texcoords[4][2];
    glamor_pixmap_private *src_pixmap_priv;

    src_pixmap_priv = glamor_get_pixmap_private(src_pixmap);

    if (src == dst) {
	glamor_fallback("glamor_copy_n_to_n with same src/dst\n");
	goto fail;
    }

    if (!src_pixmap_priv || !src_pixmap_priv->tex) {
	glamor_fallback("glamor_copy_n_to_n with non-texture src\n");
	goto fail;
    }

    if (!glamor_set_destination_pixmap(dst_pixmap))
	goto fail;

    if (gc) {
	glamor_set_alu(gc->alu);
	if (!glamor_set_planemask(dst_pixmap, gc->planemask))
	    goto fail;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, src_pixmap_priv->tex);
    glEnable(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glVertexPointer(2, GL_FLOAT, sizeof(float) * 2, vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, sizeof(float) * 2, texcoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    assert(GLEW_ARB_fragment_shader);
    glUseProgramObjectARB(glamor_priv->finish_access_prog);

    for (i = 0; i < nbox; i++) {
	vertices[0][0] = v_from_x_coord_x(dst_pixmap, box[i].x1);
	vertices[0][1] = v_from_x_coord_y(dst_pixmap, box[i].y1);
	vertices[1][0] = v_from_x_coord_x(dst_pixmap, box[i].x2);
	vertices[1][1] = v_from_x_coord_y(dst_pixmap, box[i].y1);
	vertices[2][0] = v_from_x_coord_x(dst_pixmap, box[i].x2);
	vertices[2][1] = v_from_x_coord_y(dst_pixmap, box[i].y2);
	vertices[3][0] = v_from_x_coord_x(dst_pixmap, box[i].x1);
	vertices[3][1] = v_from_x_coord_y(dst_pixmap, box[i].y2);

	texcoords[0][0] = t_from_x_coord_x(src_pixmap, box[i].x1 + dx);
	texcoords[0][1] = t_from_x_coord_y(src_pixmap, box[i].y1 + dy);
	texcoords[1][0] = t_from_x_coord_x(src_pixmap, box[i].x2 + dx);
	texcoords[1][1] = t_from_x_coord_y(src_pixmap, box[i].y1 + dy);
	texcoords[2][0] = t_from_x_coord_x(src_pixmap, box[i].x2 + dx);
	texcoords[2][1] = t_from_x_coord_y(src_pixmap, box[i].y2 + dy);
	texcoords[3][0] = t_from_x_coord_x(src_pixmap, box[i].x1 + dx);
	texcoords[3][1] = t_from_x_coord_y(src_pixmap, box[i].y2 + dy);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    glUseProgramObjectARB(0);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);

#if 0
    for (i = 0; i < nbox; i++) {
	glRasterPos2i(box[i].x1 - dst_pixmap->screen_x,
		      box[i].y1 - dst_pixmap->screen_y);
	glCopyPixels(box[i].x1 + dx - src_pixmap->screen_x,
		     box[i].y1 + dy - src_pixmap->screen_y,
		     box[i].x2 - box[i].x1,
		     box[i].y2 - box[i].y1,
		     GL_COLOR);
    }
#endif

    return;

fail:
    glamor_fallback("glamor_copy_area() from %p to %p (%c,%c)\n", src, dst,
		    glamor_get_drawable_location(src),
		    glamor_get_drawable_location(dst));
    if (glamor_prepare_access(dst, GLAMOR_ACCESS_RW)) {
	if (glamor_prepare_access(src, GLAMOR_ACCESS_RO)) {
	    fbCopyNtoN(src, dst, gc, box, nbox,
		       dx, dy, reverse, upsidedown, bitplane,
		       closure);
	    glamor_finish_access(src);
	}
	glamor_finish_access(dst);
    }
    glamor_set_alu(GXcopy);
    glamor_set_planemask(dst_pixmap, ~0);
}

RegionPtr
glamor_copy_area(DrawablePtr src, DrawablePtr dst, GCPtr gc,
		 int srcx, int srcy, int width, int height, int dstx, int dsty)
{
#if 0
    ScreenPtr screen = dst->pScreen;
    PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);
    PixmapPtr src_pixmap = glamor_get_drawable_pixmap(src);
    glamor_pixmap_private *src_priv = glamor_get_pixmap_private(src_pixmap);
#endif
    RegionPtr region;

    region = miDoCopy(src, dst, gc,
		      srcx, srcy, width, height,
		      dstx, dsty, glamor_copy_n_to_n, 0, NULL);

    return region;

#if 0
fail:
    glamor_fallback("glamor_copy_area from %p to %p (%c,%c)\n", src, dst,
		    glamor_get_drawable_location(src),
		    glamor_get_drawable_location(dst));
    region = NULL;
    if (glamor_prepare_access(dst, GLAMOR_ACCESS_RW)) {
	if (glamor_prepare_access(src, GLAMOR_ACCESS_RO)) {
	    region = fbCopyArea(src, dst, gc, srcx, srcy, width, height,
				dstx, dsty);
	    glamor_finish_access(src);
	}
	glamor_finish_access(dst);
    }
    return region;
#endif
}
