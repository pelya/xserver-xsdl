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
#ifndef GLAMOR_GLES2
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
    glamor_gl_dispatch *dispatch = &glamor_priv->dispatch;
    int dst_x_off, dst_y_off, src_x_off, src_y_off, i;

    if (!glamor_priv->has_fbo_blit) {
	glamor_delayed_fallback(screen,"no EXT_framebuffer_blit\n");
	return FALSE;
    }
    src_pixmap_priv = glamor_get_pixmap_private(src_pixmap);

    if (src_pixmap_priv->pending_op.type == GLAMOR_PENDING_FILL) 
      return FALSE;

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
    glamor_validate_pixmap(dst_pixmap);

    dispatch->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, src_pixmap_priv->fb);
    glamor_get_drawable_deltas(dst, dst_pixmap, &dst_x_off, &dst_y_off);
    glamor_get_drawable_deltas(src, src_pixmap, &src_x_off, &src_y_off);
    src_y_off += dy;

    for (i = 0; i < nbox; i++) {
      if(glamor_priv->yInverted) {
	dispatch->glBlitFramebuffer((box[i].x1 + dx + src_x_off),
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

	dispatch->glBlitFramebuffer(box[i].x1 + dx + src_x_off,
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
#endif

static Bool
glamor_copy_n_to_n_textured(DrawablePtr src,
			      DrawablePtr dst,
			      GCPtr gc,
			      BoxPtr box,
			      int nbox,
			      int dx,
			      int dy
                              )
{
    glamor_screen_private *glamor_priv =
	glamor_get_screen_private(dst->pScreen);
    glamor_gl_dispatch *dispatch = &glamor_priv->dispatch;
    PixmapPtr src_pixmap = glamor_get_drawable_pixmap(src);
    PixmapPtr dst_pixmap = glamor_get_drawable_pixmap(dst);
    int i;
    float vertices[8], texcoords[8];
    glamor_pixmap_private *src_pixmap_priv;
    glamor_pixmap_private *dst_pixmap_priv;
    int src_x_off, src_y_off, dst_x_off, dst_y_off;
    enum glamor_pixmap_status src_status = GLAMOR_NONE;
    GLfloat dst_xscale, dst_yscale, src_xscale, src_yscale;
    int flush_needed = 0;

    src_pixmap_priv = glamor_get_pixmap_private(src_pixmap);
    dst_pixmap_priv = glamor_get_pixmap_private(dst_pixmap);

    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(dst_pixmap_priv)) {
        glamor_delayed_fallback(dst->pScreen, "dst has no fbo.\n");
        goto fail;
    }

    if (!src_pixmap_priv->gl_fbo) {
#ifndef GLAMOR_PIXMAP_DYNAMIC_UPLOAD
	glamor_delayed_fallback(dst->pScreen, "src has no fbo.\n");
	goto fail;
#else
        src_status = glamor_upload_pixmap_to_texture(src_pixmap);
	if (src_status != GLAMOR_UPLOAD_DONE) 
	goto fail;
#endif
    }
    else
      flush_needed = 1;

    if (gc) {
	glamor_set_alu(dispatch, gc->alu);
	if (!glamor_set_planemask(dst_pixmap, gc->planemask))
	  goto fail;
        if (gc->alu != GXcopy) {
          glamor_set_destination_pixmap_priv_nc(src_pixmap_priv);
          glamor_validate_pixmap(src_pixmap);
        }
    }

    glamor_set_destination_pixmap_priv_nc(dst_pixmap_priv);
    glamor_validate_pixmap(dst_pixmap);

    pixmap_priv_get_scale(dst_pixmap_priv, &dst_xscale, &dst_yscale);
    pixmap_priv_get_scale(src_pixmap_priv, &src_xscale, &src_yscale);

    glamor_get_drawable_deltas(dst, dst_pixmap, &dst_x_off, &dst_y_off);



    dispatch->glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT, GL_FALSE, 
                          2 * sizeof(float),
                          vertices);
    dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_POS);

    if (GLAMOR_PIXMAP_PRIV_NO_PENDING(src_pixmap_priv)) {
      glamor_get_drawable_deltas(src, src_pixmap, &src_x_off, &src_y_off);
      dx += src_x_off;
      dy += src_y_off;
      pixmap_priv_get_scale(src_pixmap_priv, &src_xscale, &src_yscale);

      dispatch->glActiveTexture(GL_TEXTURE0);
      dispatch->glBindTexture(GL_TEXTURE_2D, src_pixmap_priv->tex);
#ifndef GLAMOR_GLES2
      dispatch->glEnable(GL_TEXTURE_2D);
#endif
      dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      dispatch->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 
      dispatch->glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2, GL_FLOAT, GL_FALSE, 
                            2 * sizeof(float),
                            texcoords);
      dispatch->glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
      dispatch->glUseProgram(glamor_priv->finish_access_prog[0]);
      dispatch->glUniform1i(glamor_priv->finish_access_no_revert[0], 1);
      dispatch->glUniform1i(glamor_priv->finish_access_swap_rb[0], 0);
 
   } 
    else {
      GLAMOR_CHECK_PENDING_FILL(dispatch, glamor_priv, src_pixmap_priv);
   }
 
    for (i = 0; i < nbox; i++) {

      glamor_set_normalize_vcoords(dst_xscale, dst_yscale, 
				   box[i].x1 + dst_x_off,
				   box[i].y1 + dst_y_off,
				   box[i].x2 + dst_x_off,
				   box[i].y2 + dst_y_off,
				   glamor_priv->yInverted,
				   vertices);

      if (GLAMOR_PIXMAP_PRIV_NO_PENDING(src_pixmap_priv))
        glamor_set_normalize_tcoords(src_xscale, src_yscale,
	  			     box[i].x1 + dx, box[i].y1 + dy,
				     box[i].x2 + dx, box[i].y2 + dy,
				     glamor_priv->yInverted,
				     texcoords);

      dispatch->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
    if (GLAMOR_PIXMAP_PRIV_NO_PENDING(src_pixmap_priv)) {
      dispatch->glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
#ifndef GLAMOR_GLES2
      dispatch->glDisable(GL_TEXTURE_2D);
#endif
    }
    dispatch->glUseProgram(0);
    /* The source texture is bound to a fbo, we have to flush it here. */
    if (flush_needed) 
      dispatch->glFlush();
    return TRUE;

fail:
    glamor_set_alu(dispatch, GXcopy);
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
    glamor_access_t dst_access;
    PixmapPtr dst_pixmap, src_pixmap, temp_pixmap = NULL;
    DrawablePtr temp_src = src;
    glamor_pixmap_private *dst_pixmap_priv, *src_pixmap_priv;
    BoxRec bound;
    ScreenPtr screen;
    int temp_dx = dx;
    int temp_dy = dy;
    int src_x_off, src_y_off, dst_x_off, dst_y_off;
    int i;
    int overlaped = 0;
    
    dst_pixmap = glamor_get_drawable_pixmap(dst);
    dst_pixmap_priv = glamor_get_pixmap_private(dst_pixmap);
    src_pixmap = glamor_get_drawable_pixmap(src);
    src_pixmap_priv = glamor_get_pixmap_private(src_pixmap);
    screen = dst_pixmap->drawable.pScreen;

    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(dst_pixmap_priv)) {
      glamor_fallback("dest pixmap %p has no fbo. \n", dst_pixmap);
      goto fail;
    }

    glamor_get_drawable_deltas(src, src_pixmap, &src_x_off, &src_y_off);
    glamor_get_drawable_deltas(dst, dst_pixmap, &dst_x_off, &dst_y_off);

    if (src_pixmap_priv->fb == dst_pixmap_priv->fb) {
      int x_shift = abs(src_x_off - dx - dst_x_off);
      int y_shift = abs(src_y_off - dy - dst_y_off);
      for ( i = 0; i < nbox; i++)
        {
         if (x_shift < abs(box[i].x2 - box[i].x1) 
            && y_shift < abs(box[i].y2 - box[i].y1)) {
           overlaped = 1;
           break;
         }
        }
    }
    /* XXX need revisit to handle overlapped area copying. */

#ifndef GLAMOR_GLES2
    if ((overlaped 
          || !src_pixmap_priv->gl_tex  || !dst_pixmap_priv->gl_tex )
        && glamor_copy_n_to_n_fbo_blit(src, dst, gc, box, nbox, dx, dy)) {
        goto done;
	return;
    }
#endif
    glamor_calculate_boxes_bound(&bound, box, nbox);

    /*  Overlaped indicate the src and dst are the same pixmap. */
    if (overlaped || (!GLAMOR_PIXMAP_PRIV_HAS_FBO(src_pixmap_priv) 
	&& ((bound.x2 - bound.x1) * (bound.y2 - bound.y1)
	    * 4 > src_pixmap->drawable.width * src_pixmap->drawable.height))) {

      temp_pixmap = (*screen->CreatePixmap)(screen,
					 bound.x2 - bound.x1,
					 bound.y2 - bound.y1,
					 src_pixmap->drawable.depth,
					 overlaped ? 0 : GLAMOR_CREATE_PIXMAP_CPU);
      if (!temp_pixmap)
	goto fail;
      glamor_transform_boxes(box, nbox, -bound.x1, -bound.y1);
      temp_src = &temp_pixmap->drawable;

      if (overlaped)
        glamor_copy_n_to_n_textured(src, temp_src, gc, box, nbox, 
                                    temp_dx + bound.x1, temp_dy + bound.y1); 
      else
        fbCopyNtoN(src, temp_src, gc, box, nbox,
	  	   temp_dx + bound.x1, temp_dy + bound.y1, 
		   reverse, upsidedown, bitplane,
		   closure);
      glamor_transform_boxes(box, nbox, bound.x1, bound.y1);
      temp_dx = -bound.x1;
      temp_dy = -bound.y1;
    }
    else {
      temp_dx = dx;
      temp_dy = dy;
      temp_src = src;
    }

    if (glamor_copy_n_to_n_textured(temp_src, dst, gc, box, nbox, temp_dx, temp_dy)) {
        goto done;
    }

    
 fail:
    glamor_report_delayed_fallbacks(src->pScreen);
    glamor_report_delayed_fallbacks(dst->pScreen);

    glamor_fallback("from %p to %p (%c,%c)\n", src, dst,
		    glamor_get_drawable_location(src),
		    glamor_get_drawable_location(dst));

    if (gc && gc->alu != GXcopy)
      dst_access = GLAMOR_ACCESS_RW;
    else
      dst_access = GLAMOR_ACCESS_WO;

    if (glamor_prepare_access(dst, GLAMOR_ACCESS_RW)) {
	if (dst == src 
            || glamor_prepare_access(src, GLAMOR_ACCESS_RO)) {
	    fbCopyNtoN(src, dst, gc, box, nbox,
		       dx, dy, reverse, upsidedown, bitplane,
		       closure);
            if (dst != src)
	      glamor_finish_access(src);
	}
	glamor_finish_access(dst);
    }

done:
    glamor_clear_delayed_fallbacks(src->pScreen);
    glamor_clear_delayed_fallbacks(dst->pScreen);
    if (temp_src != src) {
      (*screen->DestroyPixmap)(temp_pixmap);
    }
}

RegionPtr
glamor_copy_area(DrawablePtr src, DrawablePtr dst, GCPtr gc,
		 int srcx, int srcy, int width, int height, int dstx, int dsty)
{
    RegionPtr region;
    region = miDoCopy(src, dst, gc,
		      srcx, srcy, width, height,
		      dstx, dsty, glamor_copy_n_to_n, 0, NULL);

    return region;
}
