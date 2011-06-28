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
static Bool
glamor_copy_n_to_n_fbo_blit(DrawablePtr src,
			    DrawablePtr dst,
			    GCPtr gc,
			    BoxPtr box,
			    int nbox,
			    int dx,
			    int dy)
{
    ScreenPtr screen = dst->pScreen;
    PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(dst);
    PixmapPtr src_pixmap = glamor_get_drawable_pixmap(src);
    glamor_pixmap_private *src_pixmap_priv;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    int dst_x_off, dst_y_off, src_x_off, src_y_off, i;

    if (src == dst) {
	glamor_delayed_fallback(screen,"src == dest\n");
	return FALSE;
    }

    if (!GLEW_EXT_framebuffer_blit) {
	glamor_delayed_fallback(screen,"no EXT_framebuffer_blit\n");
	return FALSE;
   }

    src_pixmap_priv = glamor_get_pixmap_private(src_pixmap);

    if (gc) {
	if (gc->alu != GXcopy) {
	    glamor_delayed_fallback(screen, "non-copy ALU\n");
	    return FALSE;
	}
	if (!glamor_pm_is_solid(dst, gc->planemask)) {
	    glamor_delayed_fallback(screen, "non-solid planemask\n");
	    return FALSE;
	}
    }

    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(src_pixmap_priv)) {
        glamor_delayed_fallback(screen, "no src fbo\n");
        return FALSE;
    }

    if (glamor_set_destination_pixmap(dst_pixmap)) {
	return FALSE;
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, src_pixmap_priv->fb);

    glamor_get_drawable_deltas(dst, dst_pixmap, &dst_x_off, &dst_y_off);
    glamor_get_drawable_deltas(src, src_pixmap, &src_x_off, &src_y_off);
    src_y_off += dy;

    for (i = 0; i < nbox; i++) {
      if(glamor_priv->yInverted) {
	glBlitFramebufferEXT((box[i].x1 + dx + src_x_off),
                             (box[i].y1 + src_y_off),
			     (box[i].x2 + dx + src_x_off),
			     (box[i].y2 + src_y_off),
                             (box[i].x1 + dst_x_off),
                             (box[i].y1 + dst_y_off),
			     (box[i].x2 + dst_x_off),
			     (box[i].y2 + dst_y_off),
			     GL_COLOR_BUFFER_BIT,
			     GL_NEAREST);
       } else {
	int flip_dst_y1 = dst_pixmap->drawable.height - (box[i].y2 + dst_y_off);
	int flip_dst_y2 = dst_pixmap->drawable.height - (box[i].y1 + dst_y_off);
	int flip_src_y1 = src_pixmap->drawable.height - (box[i].y2 + src_y_off);
	int flip_src_y2 = src_pixmap->drawable.height - (box[i].y1 + src_y_off);

	glBlitFramebufferEXT(box[i].x1 + dx + src_x_off,
			     flip_src_y1,
			     box[i].x2 + dx + src_x_off,
			     flip_src_y2,
			     box[i].x1 + dst_x_off,
			     flip_dst_y1,
			     box[i].x2 + dst_x_off,
			     flip_dst_y2,
			     GL_COLOR_BUFFER_BIT,
			     GL_NEAREST);
	}
    }
    return TRUE;
}

static Bool
glamor_copy_n_to_n_copypixels(DrawablePtr src,
			      DrawablePtr dst,
			      GCPtr gc,
			      BoxPtr box,
			      int nbox,
			      int dx,
			      int dy)
{
    ScreenPtr screen = dst->pScreen;
    PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(dst);
    glamor_screen_private *glamor_priv =
	glamor_get_screen_private(screen);
    int x_off, y_off, i;
    if (src != dst) {
	glamor_delayed_fallback(screen, "src != dest\n");
	return FALSE;
    }

    if (gc) {
	if (gc->alu != GXcopy) {
	    glamor_delayed_fallback(screen,"non-copy ALU\n");
	    return FALSE;
	}
	if (!glamor_pm_is_solid(dst, gc->planemask)) {
	    glamor_delayed_fallback(screen,"non-solid planemask\n");
	    return FALSE;
	}
    }

    if (glamor_set_destination_pixmap(dst_pixmap)) {
        glamor_delayed_fallback(screen, "dst has no fbo.\n");
	return FALSE;
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glOrtho(0, dst_pixmap->drawable.width,
	    0, dst_pixmap->drawable.height,
	    -1, 1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glamor_get_drawable_deltas(dst, dst_pixmap, &x_off, &y_off);

    for (i = 0; i < nbox; i++) {
      if(glamor_priv->yInverted) {
	glRasterPos2i(box[i].x1 + x_off,
		      box[i].y1 + y_off);
	glCopyPixels(box[i].x1 + dx + x_off,
		     box[i].y1 + dy + y_off,
		     box[i].x2 - box[i].x1,
		     box[i].y2 - box[i].y1,
		     GL_COLOR);
	} else {
	int flip_y1 = dst_pixmap->drawable.height - box[i].y2 + y_off;
	glRasterPos2i(box[i].x1 + x_off,
		      flip_y1);
	glCopyPixels(box[i].x1 + dx + x_off,
		     flip_y1 - dy,
		     box[i].x2 - box[i].x1,
		     box[i].y2 - box[i].y1,
		     GL_COLOR);
	}
    }
    return TRUE;
}

static Bool
glamor_copy_n_to_n_textured(DrawablePtr src,
			      DrawablePtr dst,
			      GCPtr gc,
			      BoxPtr box,
			      int nbox,
			      int dx,
			      int dy)
{
    glamor_screen_private *glamor_priv =
	glamor_get_screen_private(dst->pScreen);
    PixmapPtr src_pixmap = glamor_get_drawable_pixmap(src);
    PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(dst);
    int i;
    float vertices[8], texcoords[8];
    glamor_pixmap_private *src_pixmap_priv;
    glamor_pixmap_private *dst_pixmap_priv;
    int src_x_off, src_y_off, dst_x_off, dst_y_off;
    enum glamor_pixmap_status src_status = GLAMOR_NONE;
    GLfloat dst_xscale, dst_yscale, src_xscale, src_yscale;

    src_pixmap_priv = glamor_get_pixmap_private(src_pixmap);
    dst_pixmap_priv = glamor_get_pixmap_private(dst_pixmap);

    if (src == dst) {
	glamor_delayed_fallback(dst->pScreen, "same src/dst\n");
	goto fail;
    }

    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(dst_pixmap_priv)) {
        glamor_delayed_fallback(dst->pScreen, "dst has no fbo.\n");
        goto fail;
    }

    if (!src_pixmap_priv->gl_tex) {
#ifndef GLAMOR_PIXMAP_DYNAMIC_UPLOAD
	glamor_delayed_fallback(dst->pScreen, "src has no fbo.\n");
	goto fail;
#else
        src_status = glamor_upload_pixmap_to_texture(src_pixmap);
	if (src_status != GLAMOR_UPLOAD_DONE) 
	goto fail;
#endif
    }

    if (gc) {
	glamor_set_alu(gc->alu);
	if (!glamor_set_planemask(dst_pixmap, gc->planemask))
	  goto fail;
    }

    glamor_set_destination_pixmap_priv_nc(dst_pixmap_priv);
    pixmap_priv_get_scale(dst_pixmap_priv, &dst_xscale, &dst_yscale);
    pixmap_priv_get_scale(src_pixmap_priv, &src_xscale, &src_yscale);


    glamor_get_drawable_deltas(dst, dst_pixmap, &dst_x_off, &dst_y_off);
    glamor_get_drawable_deltas(src, src_pixmap, &src_x_off, &src_y_off);
    dx += src_x_off;
    dy += src_y_off;

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
    glUseProgramObjectARB(glamor_priv->finish_access_prog[0]);
   

    for (i = 0; i < nbox; i++) {

      glamor_set_normalize_vcoords(dst_xscale, dst_yscale, 
				   box[i].x1 + dst_x_off,
				   box[i].y1 + dst_y_off,
				   box[i].x2 + dst_x_off,
				   box[i].y2 + dst_y_off,
				   glamor_priv->yInverted,
				   vertices);

      glamor_set_normalize_tcoords(src_xscale, src_yscale,
				   box[i].x1 + dx, box[i].y1 + dy,
				   box[i].x2 + dx, box[i].y2 + dy,
				   glamor_priv->yInverted,
				   texcoords);

      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    glUseProgramObjectARB(0);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);
    return TRUE;

fail:
    glamor_set_alu(GXcopy);
    glamor_set_planemask(dst_pixmap, ~0);
    return FALSE;
}

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
    if (glamor_copy_n_to_n_fbo_blit(src, dst, gc, box, nbox, dx, dy)) {
        goto done;
	return;
    }

    if (glamor_copy_n_to_n_copypixels(src, dst, gc, box, nbox, dx, dy)){
        goto done;
	return;
     }

    if (glamor_copy_n_to_n_textured(src, dst, gc, box, nbox, dx, dy)) {
        goto done;
	return;
    }

    glamor_report_delayed_fallbacks(src->pScreen);
    glamor_report_delayed_fallbacks(dst->pScreen);

    glamor_fallback("from %p to %p (%c,%c)\n", src, dst,
		    glamor_get_drawable_location(src),
		    glamor_get_drawable_location(dst));
    if (glamor_prepare_access(dst, GLAMOR_ACCESS_RW)) {
	if (dst == src || glamor_prepare_access(src, GLAMOR_ACCESS_RO)) {
	    fbCopyNtoN(src, dst, gc, box, nbox,
		       dx, dy, reverse, upsidedown, bitplane,
		       closure);
            if (dst != src)
	      glamor_finish_access(src);
	}
	glamor_finish_access(dst);
    }
    return;

done:
    glamor_clear_delayed_fallbacks(src->pScreen);
    glamor_clear_delayed_fallbacks(dst->pScreen);
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
