/*
 * Copyright © 2009 Intel Corporation
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


/** @file glamor_putaimge.c
 *
 * XPutImage implementation
 */
#include "glamor_priv.h"

void
glamor_init_putimage_shaders(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    const char *xybitmap_vs =
	"uniform float x_bias;\n"
	"uniform float x_scale;\n"
	"uniform float y_bias;\n"
	"uniform float y_scale;\n"
	"varying vec2 bitmap_coords;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = vec4((gl_Vertex.x + x_bias) * x_scale,\n"
	"			   (gl_Vertex.y + y_bias) * y_scale,\n"
	"			   0,\n"
	"			   1);\n"
	"	bitmap_coords = gl_MultiTexCoord0.xy;\n"
	"}\n";
    const char *xybitmap_fs =
	"uniform vec4 fg, bg;\n"
	"varying vec2 bitmap_coords;\n"
	"uniform sampler2D bitmap_sampler;\n"
	"void main()\n"
	"{\n"
	"	float bitmap_value = texture2D(bitmap_sampler,\n"
	"				       bitmap_coords).x;\n"
	"	gl_FragColor = mix(bg, fg, bitmap_value);\n"
	"}\n";
    GLint fs_prog, vs_prog, prog;
    GLint sampler_uniform_location;

    if (!GLEW_ARB_fragment_shader)
	return;

    prog = glCreateProgramObjectARB();
    vs_prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER_ARB, xybitmap_vs);
    fs_prog = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER_ARB, xybitmap_fs);
    glAttachObjectARB(prog, vs_prog);
    glAttachObjectARB(prog, fs_prog);
    glamor_link_glsl_prog(prog);

    glUseProgramObjectARB(prog);
    sampler_uniform_location = glGetUniformLocationARB(prog, "bitmap_sampler");
    glUniform1iARB(sampler_uniform_location, 0);

    glamor_priv->put_image_xybitmap_fg_uniform_location =
	glGetUniformLocationARB(prog, "fg");
    glamor_priv->put_image_xybitmap_bg_uniform_location =
	glGetUniformLocationARB(prog, "bg");
    glamor_get_transform_uniform_locations(prog,
					   &glamor_priv->put_image_xybitmap_transform);
    glamor_priv->put_image_xybitmap_prog = prog;
    glUseProgramObjectARB(0);
}

static int
y_flip(PixmapPtr pixmap, int y)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);

    if (pixmap == screen_pixmap)
	return (pixmap->drawable.height - 1) - y;
    else
	return y;
}

/* Do an XYBitmap putimage.  The bits are byte-aligned rows of bitmap
 * data (where each row starts at a bit index of left_pad), and the
 * destination gets filled with the gc's fg color where the bitmap is set
 * and the bg color where the bitmap is unset.
 *
 * Implement this by passing the bitmap right through to GL, and sampling
 * it to choose between fg and bg in the fragment shader.  The driver may
 * be exploding the bitmap up to be an 8-bit alpha texture, in which
 * case we might be better off just doing the fg/bg choosing in the CPU
 * and just draw the resulting texture to the destination.
 */
static void
glamor_put_image_xybitmap(DrawablePtr drawable, GCPtr gc,
			  int x, int y, int w, int h, int left_pad,
			  int image_format, char *bits)
{
    ScreenPtr screen = drawable->pScreen;
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    float fg[4], bg[4];
    GLuint tex;
    unsigned int stride = PixmapBytePad(1, w + left_pad);
    RegionPtr clip;
    BoxPtr box;
    int nbox;
    float dest_coords[8] = {
	x, y,
	x + w, y,
	x + w, y + h,
	x, y + h,
    };
    const float bitmap_coords[8] = {
	0.0, 0.0,
	1.0, 0.0,
	1.0, 1.0,
	0.0, 1.0,
    };

    if (glamor_priv->put_image_xybitmap_prog == 0) {
	ErrorF("no program for xybitmap putimage\n");
	goto fail;
    }

    glamor_set_alu(gc->alu);
    if (!glamor_set_planemask(pixmap, gc->planemask))
	goto fail;

    glUseProgramObjectARB(glamor_priv->put_image_xybitmap_prog);

    glamor_get_color_4f_from_pixel(pixmap, gc->fgPixel, fg);
    glUniform4fvARB(glamor_priv->put_image_xybitmap_fg_uniform_location,
		    1, fg);
    glamor_get_color_4f_from_pixel(pixmap, gc->bgPixel, bg);
    glUniform4fvARB(glamor_priv->put_image_xybitmap_bg_uniform_location,
		    1, bg);

    glamor_set_transform_for_pixmap(pixmap, &glamor_priv->solid_transform);

    glGenTextures(1, &tex);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride * 8);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, left_pad);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
		 w, h, 0,
		 GL_COLOR_INDEX, GL_BITMAP, bits);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glEnable(GL_TEXTURE_2D);

    /* Now that we've set up our bitmap texture and the shader, shove
     * the destination rectangle through the cliprects and run the
     * shader on the resulting fragments.
     */
    glVertexPointer(2, GL_FLOAT, 0, dest_coords);
    glEnableClientState(GL_VERTEX_ARRAY);
    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, 0, bitmap_coords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glEnable(GL_SCISSOR_TEST);
    clip = fbGetCompositeClip(gc);
    for (nbox = REGION_NUM_RECTS(clip),
	 box = REGION_RECTS(clip);
	 nbox--;
	 box++)
    {
	int x1 = x;
	int y1 = y;
	int x2 = x + w;
	int y2 = y + h;

	if (x1 < box->x1)
	    x1 = box->x1;
	if (y1 < box->y1)
	    y1 = box->y1;
	if (x2 > box->x2)
	    x2 = box->x2;
	if (y2 > box->y2)
	    y2 = box->y2;
	if (x1 >= x2 || y1 >= y2)
	    continue;

	glScissor(box->x1,
		  y_flip(pixmap, box->y1),
		  box->x2 - box->x1,
		  box->y2 - box->y1);
	glDrawArrays(GL_QUADS, 0, 4);
    }

    glDisable(GL_SCISSOR_TEST);
    glamor_set_alu(GXcopy);
    glamor_set_planemask(pixmap, ~0);
    glDeleteTextures(1, &tex);
    glDisable(GL_TEXTURE_2D);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    return;

fail:
    glamor_set_alu(GXcopy);
    glamor_set_planemask(pixmap, ~0);

    glamor_fallback("glamor_put_image(): to %p (%c)\n",
		    drawable, glamor_get_drawable_location(drawable));
    if (glamor_prepare_access(drawable, GLAMOR_ACCESS_RW)) {
	fbPutImage(drawable, gc, 1, x, y, w, h, left_pad, XYBitmap, bits);
	glamor_finish_access(drawable);
    }
}

void
glamor_put_image(DrawablePtr drawable, GCPtr gc, int depth, int x, int y,
		 int w, int h, int left_pad, int image_format, char *bits)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    GLenum type, format;
    RegionPtr clip;
    BoxPtr pbox;
    int nbox;
    int bpp = drawable->bitsPerPixel;
    int src_stride = PixmapBytePad(w, drawable->depth);

    if (!glamor_set_destination_pixmap(pixmap)) {
	fbPutImage(drawable, gc, depth, x, y, w, h, left_pad,
		   image_format, bits);
	return;
    }

    if (image_format == XYBitmap) {
	assert(depth == 1);
	glamor_put_image_xybitmap(drawable, gc, x, y, w, h,
				  left_pad, image_format, bits);
	return;
    }

    if (bpp == 1 && image_format == XYPixmap)
	image_format = ZPixmap;

    if (!glamor_set_planemask(pixmap, gc->planemask))
	goto fail;
    if (image_format != ZPixmap) {
	ErrorF("putimage: non-ZPixmap\n");
	goto fail;
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
	ErrorF("stub put_image depth %d\n", drawable->depth);
	goto fail;
    }

    glamor_set_alu(gc->alu);

    x += drawable->x;
    y += drawable->y;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, src_stride * 8 / bpp);
    if (bpp == 1)
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, left_pad);
    clip = fbGetCompositeClip(gc);
    for (nbox = REGION_NUM_RECTS(clip),
	 pbox = REGION_RECTS(clip);
	 nbox--;
	 pbox++)
    {
	int x1 = x;
	int y1 = y;
	int x2 = x + w;
	int y2 = y + h;
	char *src;

	if (x1 < pbox->x1)
	    x1 = pbox->x1;
	if (y1 < pbox->y1)
	    y1 = pbox->y1;
	if (x2 > pbox->x2)
	    x2 = pbox->x2;
	if (y2 > pbox->y2)
	    y2 = pbox->y2;
	if (x1 >= x2 || y1 >= y2)
	    continue;

	src = bits + (y1 - y) * src_stride + (x1 - x) * (bpp / 8);
	glRasterPos2i(x1 - pixmap->screen_x, y1 - pixmap->screen_y);
	glDrawPixels(x2 - x1,
		     y2 - y1,
		     format, type,
		     src);
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glamor_set_alu(GXcopy);
    glamor_set_planemask(pixmap, ~0);
    return;

fail:
    glamor_set_planemask(pixmap, ~0);
    glamor_fallback("glamor_put_image(): to %p (%c)\n",
		    drawable, glamor_get_drawable_location(drawable));
    if (glamor_prepare_access(drawable, GLAMOR_ACCESS_RW)) {
	fbPutImage(drawable, gc, depth, x, y, w, h, left_pad, image_format,
		   bits);
	glamor_finish_access(drawable);
    }
}
