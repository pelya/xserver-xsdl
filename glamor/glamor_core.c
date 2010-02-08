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

const Bool
glamor_get_drawable_location(const DrawablePtr drawable)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    if (pixmap_priv == NULL)
	return 'm';
    if (pixmap_priv->fb == 0)
	return 's';
    else
	return 'f';
}

Bool
glamor_set_destination_pixmap(PixmapPtr pixmap)
{
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);

    if (pixmap_priv == NULL) {
	glamor_fallback("glamor_set_destination_pixmap(): no pixmap priv");
	return FALSE;
    }

    if (pixmap_priv->fb == 0) {
	ScreenPtr screen = pixmap->drawable.pScreen;
	PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);

	if (pixmap != screen_pixmap) {
	    glamor_fallback("glamor_set_destination_pixmap(): no fbo");
	    return FALSE;
	}
    }

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pixmap_priv->fb);

    glViewport(0, 0,
	       pixmap->drawable.width,
	       pixmap->drawable.height);

    return TRUE;
}

Bool
glamor_set_planemask(PixmapPtr pixmap, unsigned long planemask)
{
    if (glamor_pm_is_solid(&pixmap->drawable, planemask)) {
	return GL_TRUE;
    }

    ErrorF("unsupported planemask\n");
    return GL_FALSE;
}

void
glamor_set_alu(unsigned char alu)
{
    if (alu == GXcopy) {
	glDisable(GL_LOGIC_OP);
	return;
    }

    glEnable(GL_LOGIC_OP);

    switch (alu) {
    case GXclear:
	glLogicOp(GL_CLEAR);
	break;
    case GXand:
	glLogicOp(GL_AND);
	break;
    case GXandReverse:
	glLogicOp(GL_AND_REVERSE);
	break;
    case GXandInverted:
	glLogicOp(GL_AND_INVERTED);
	break;
    case GXnoop:
	glLogicOp(GL_NOOP);
	break;
    case GXxor:
	glLogicOp(GL_XOR);
	break;
    case GXor:
	glLogicOp(GL_OR);
	break;
    case GXnor:
	glLogicOp(GL_NOR);
	break;
    case GXequiv:
	glLogicOp(GL_EQUIV);
	break;
    case GXinvert:
	glLogicOp(GL_INVERT);
	break;
    case GXorReverse:
	glLogicOp(GL_OR_REVERSE);
	break;
    case GXcopyInverted:
	glLogicOp(GL_COPY_INVERTED);
	break;
    case GXorInverted:
	glLogicOp(GL_OR_INVERTED);
	break;
    case GXnand:
	glLogicOp(GL_NAND);
	break;
    case GXset:
	glLogicOp(GL_SET);
	break;
    default:
	FatalError("unknown logic op\n");
    }
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
    ScreenPtr screen = pixmap->drawable.pScreen;
    PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);

    glUniform1fARB(uniform_locations->x_bias, -pixmap->drawable.width / 2.0f);
    glUniform1fARB(uniform_locations->x_scale, 2.0f / pixmap->drawable.width);
    glUniform1fARB(uniform_locations->y_bias, -pixmap->drawable.height / 2.0f);
    if (pixmap == screen_pixmap)
	glUniform1fARB(uniform_locations->y_scale,
		       -2.0f / pixmap->drawable.height);
    else
	glUniform1fARB(uniform_locations->y_scale,
		       2.0f / pixmap->drawable.height);
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
    switch (pixmap->drawable.depth) {
    case 1:
	color[0] = 0.0;
	color[1] = 0.0;
	color[2] = 0.0;
	color[3] = fg_pixel & 0x01;
	break;
    case 8:
	color[0] = 0.0;
	color[1] = 0.0;
	color[2] = 0.0;
	color[3] = (fg_pixel & 0xff) / 255.0;
	break;
    case 24:
    case 32:
	color[0] = ubyte_to_float(fg_pixel >> 16);
	color[1] = ubyte_to_float(fg_pixel >> 8);
	color[2] = ubyte_to_float(fg_pixel >> 0);
	color[3] = ubyte_to_float(fg_pixel >> 24);
	break;
    default:
	ErrorF("pixmap with bad depth: %d\n", pixmap->drawable.depth);
	color[0] = 1.0;
	color[1] = 0.0;
	color[2] = 1.0;
	color[3] = 1.0;
	break;
    }
}

Bool
glamor_prepare_access(DrawablePtr drawable, glamor_access_t access)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    unsigned int stride, read_stride, x, y;
    GLenum format, type;
    uint8_t *data, *read;

    if (pixmap_priv == NULL)
	return TRUE;

    if (pixmap_priv->fb == 0) {
	ScreenPtr screen = pixmap->drawable.pScreen;
	PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);

	if (pixmap != screen_pixmap)
	    return TRUE;
    }

    stride = PixmapBytePad(drawable->width, drawable->depth);
    read_stride = stride;

    data = xalloc(stride * drawable->height);

    switch (drawable->depth) {
    case 1:
	format = GL_ALPHA;
	type = GL_UNSIGNED_BYTE;
	read_stride = drawable->width;
	break;
    case 8:
	format = GL_ALPHA;
	type = GL_UNSIGNED_BYTE;
	break;
    case 24:
	format = GL_RGB;
	type = GL_UNSIGNED_BYTE;
	break;
    case 32:
	format = GL_BGRA;
	type = GL_UNSIGNED_INT_8_8_8_8_REV;
	break;
    default:
	ErrorF("Unknown prepareaccess depth %d\n", drawable->depth);
	xfree(data);
	return FALSE;
    }

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pixmap_priv->fb);
    glGenBuffersARB(1, &pixmap_priv->pbo);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, pixmap_priv->pbo);
    glBufferDataARB(GL_PIXEL_PACK_BUFFER_EXT,
		    read_stride * pixmap->drawable.height,
		    NULL, GL_DYNAMIC_DRAW_ARB);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, read_stride * 8 /
		  pixmap->drawable.bitsPerPixel);

    glReadPixels(0, 0,
		 pixmap->drawable.width, pixmap->drawable.height,
		 format, type, 0);

    read = glMapBufferARB(GL_PIXEL_PACK_BUFFER_EXT, GL_READ_WRITE_ARB);

    if (pixmap->drawable.depth == 1) {
	for (y = 0; y < pixmap->drawable.height; y++) {
	    uint8_t *read_row = read + read_stride * (pixmap->drawable.height -
						      y - 1);

	    for (x = 0; x < pixmap->drawable.width; x++) {
		int index = x / 8;
		int bit = 1 << (x % 8);

		if (read_row[x])
		    data[index] |= bit;
		else
		    data[index] &= ~bit;
	    }
	}
    } else {
	for (y = 0; y < pixmap->drawable.height; y++)
	    memcpy(data + y * stride,
		   read + (pixmap->drawable.height - y - 1) * stride, stride);
    }
    pixmap->devPrivate.ptr = data;

    glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_EXT);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, 0);
    glDeleteBuffersARB(1, &pixmap_priv->pbo);
    pixmap_priv->pbo = 0;

    return TRUE;
}

void
glamor_finish_access(DrawablePtr drawable)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    unsigned int stride;
    GLenum format, type;

    if (pixmap_priv == NULL)
	return;

    if (pixmap_priv->fb == 0) {
	ScreenPtr screen = pixmap->drawable.pScreen;
	PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);

	if (pixmap != screen_pixmap)
	    return;
    }

    switch (drawable->depth) {
    case 1:
	format = GL_COLOR_INDEX;
	type = GL_BITMAP;
	break;
    case 8:
	format = GL_ALPHA;
	type = GL_UNSIGNED_BYTE;
	break;
    case 24:
	format = GL_RGB;
	type = GL_UNSIGNED_BYTE;
	break;
    case 32:
	format = GL_BGRA;
	type = GL_UNSIGNED_INT_8_8_8_8_REV;
	break;
    default:
	ErrorF("Unknown finishaccess depth %d\n", drawable->depth);
	return;
    }

    stride = PixmapBytePad(drawable->width, drawable->depth);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pixmap_priv->fb);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride * 8 /
		  pixmap->drawable.bitsPerPixel);

    glRasterPos2i(0, 0);
    glDrawPixels(pixmap->drawable.width, pixmap->drawable.height,
		 format, type, pixmap->devPrivate.ptr);

    xfree(pixmap->devPrivate.ptr);
    pixmap->devPrivate.ptr = NULL;
}

void
glamor_stipple(PixmapPtr pixmap, PixmapPtr stipple,
	       int x, int y, int width, int height,
	       unsigned char alu, unsigned long planemask,
	       unsigned long fg_pixel, unsigned long bg_pixel,
	       int stipple_x, int stipple_y)
{
    ErrorF("stubbed out stipple depth %d\n", pixmap->drawable.depth);
    glamor_solid_fail_region(pixmap, x, y, width, height);
}

GCOps glamor_gc_ops = {
    .FillSpans = glamor_fill_spans,
    .SetSpans = glamor_set_spans,
    .PutImage = glamor_put_image,
    .CopyArea = glamor_copy_area,
    .CopyPlane = miCopyPlane,
    .PolyPoint = miPolyPoint,
    .Polylines = glamor_poly_lines,
    .PolySegment = miPolySegment,
    .PolyRectangle = miPolyRectangle,
    .PolyArc = miPolyArc,
    .FillPolygon = miFillPolygon,
    .PolyFillRect = glamor_poly_fill_rect,
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
 * uxa_validate_gc() sets the ops to glamor's implementations, which may be
 * accelerated or may sync the card and fall back to fb.
 */
static void
glamor_validate_gc(GCPtr gc, unsigned long changes, DrawablePtr drawable)
{
    /* fbValidateGC will do direct access to pixmaps if the tiling has changed.
     * Preempt fbValidateGC by doing its work and masking the change out, so
     * that we can do the Prepare/finish_access.
     */
#ifdef FB_24_32BIT
    if ((changes & GCTile) && fbGetRotatedPixmap(gc)) {
	gc->pScreen->DestroyPixmap(fbGetRotatedPixmap(gc));
	fbGetRotatedPixmap(gc) = 0;
    }

    if (gc->fillStyle == FillTiled) {
	PixmapPtr old_tile, new_tile;

	old_tile = gc->tile.pixmap;
	if (old_tile->drawable.bitsPerPixel != drawable->bitsPerPixel) {
	    new_tile = fbGetRotatedPixmap(gc);
	    if (!new_tile ||
		new_tile ->drawable.bitsPerPixel != drawable->bitsPerPixel)
	    {
		if (new_tile)
		    gc->pScreen->DestroyPixmap(new_tile);
		/* fb24_32ReformatTile will do direct access of a newly-
		 * allocated pixmap.
		 */
		if (glamor_prepare_access(&old_tile->drawable,
					  GLAMOR_ACCESS_RO)) {
		    new_tile = fb24_32ReformatTile(old_tile,
						   drawable->bitsPerPixel);
		    glamor_finish_access(&old_tile->drawable);
		}
	    }
	    if (new_tile) {
		fbGetRotatedPixmap(gc) = old_tile;
		gc->tile.pixmap = new_tile;
		changes |= GCTile;
	    }
	}
    }
#endif
    if (changes & GCTile) {
	if (!gc->tileIsPixel && FbEvenTile(gc->tile.pixmap->drawable.width *
					   drawable->bitsPerPixel))
	{
	    if (glamor_prepare_access(&gc->tile.pixmap->drawable,
				      GLAMOR_ACCESS_RW)) {
		fbPadPixmap(gc->tile.pixmap);
		glamor_finish_access(&gc->tile.pixmap->drawable);
	    }
	}
	/* Mask out the GCTile change notification, now that we've done FB's
	 * job for it.
	 */
	changes &= ~GCTile;
    }

    if (changes & GCStipple && gc->stipple) {
	/* We can't inline stipple handling like we do for GCTile because
	 * it sets fbgc privates.
	 */
	if (glamor_prepare_access(&gc->stipple->drawable, GLAMOR_ACCESS_RW)) {
	    fbValidateGC(gc, changes, drawable);
	    glamor_finish_access(&gc->stipple->drawable);
	}
    } else {
	fbValidateGC(gc, changes, drawable);
    }

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

Bool
glamor_prepare_access_window(WindowPtr window)
{
    if (window->backgroundState == BackgroundPixmap) {
        if (!glamor_prepare_access(&window->background.pixmap->drawable,
				   GLAMOR_ACCESS_RO))
	    return FALSE;
    }

    if (window->borderIsPixel == FALSE) {
        if (!glamor_prepare_access(&window->border.pixmap->drawable,
				   GLAMOR_ACCESS_RO)) {
	    if (window->backgroundState == BackgroundPixmap)
		glamor_finish_access(&window->background.pixmap->drawable);
	    return FALSE;
	}
    }
    return TRUE;
}

void
glamor_finish_access_window(WindowPtr window)
{
    if (window->backgroundState == BackgroundPixmap)
        glamor_finish_access(&window->background.pixmap->drawable);

    if (window->borderIsPixel == FALSE)
        glamor_finish_access(&window->border.pixmap->drawable);
}

Bool
glamor_change_window_attributes(WindowPtr window, unsigned long mask)
{
    Bool ret;

    if (!glamor_prepare_access_window(window))
	return FALSE;
    ret = fbChangeWindowAttributes(window, mask);
    glamor_finish_access_window(window);
    return ret;
}

RegionPtr
glamor_bitmap_to_region(PixmapPtr pixmap)
{
  RegionPtr ret;
  if (!glamor_prepare_access(&pixmap->drawable, GLAMOR_ACCESS_RO))
    return NULL;
  ret = fbPixmapToRegion(pixmap);
  glamor_finish_access(&pixmap->drawable);
  return ret;
}
