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
 *    Zhigang Gong <zhigang.gong@gmail.com>
 *
 */

#include <stdlib.h>

#include "glamor_priv.h"
#include "mipict.h"

/*
 * Map picture's format to the correct gl texture format and type.
 * no_alpha is used to indicate whehter we need to wire alpha to 1.
 *
 * Although opengl support A1/GL_BITMAP, we still don't use it
 * here, it seems that mesa has bugs when uploading a A1 bitmap.
 *
 * Return 0 if find a matched texture type. Otherwise return -1.
 **/
static Bool
glamor_get_tex_format_type_from_pictformat(ScreenPtr pScreen,
                                           PictFormatShort format,
                                           GLenum *tex_format,
                                           GLenum *tex_type,
                                           int *no_alpha,
                                           int *revert,
                                           int *swap_rb)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(pScreen);
    Bool is_little_endian = IMAGE_BYTE_ORDER == LSBFirst;

    *no_alpha = 0;
    *revert = REVERT_NONE;
    *swap_rb = SWAP_NONE_UPLOADING;

    switch (format) {
    case PICT_a1:
        *tex_format = glamor_priv->one_channel_format;
        *tex_type = GL_UNSIGNED_BYTE;
        *revert = REVERT_UPLOADING_A1;
        break;

    case PICT_b8g8r8x8:
        *no_alpha = 1;
    case PICT_b8g8r8a8:
        if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
            *tex_format = GL_BGRA;
            *tex_type = GL_UNSIGNED_INT_8_8_8_8;
        } else {
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_BYTE;
            *swap_rb = SWAP_UPLOADING;
            *revert = is_little_endian ? REVERT_NORMAL : REVERT_NONE;
        }
        break;

    case PICT_x8r8g8b8:
        *no_alpha = 1;
    case PICT_a8r8g8b8:
        if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
            *tex_format = GL_BGRA;
            *tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
        } else {
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_BYTE;
            *swap_rb = SWAP_UPLOADING;
            *revert = is_little_endian ? REVERT_NONE : REVERT_NORMAL;
            break;
        }
        break;

    case PICT_x8b8g8r8:
        *no_alpha = 1;
    case PICT_a8b8g8r8:
        *tex_format = GL_RGBA;
        if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
            *tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
        } else {
            *tex_type = GL_UNSIGNED_BYTE;
            *revert = is_little_endian ? REVERT_NONE : REVERT_NORMAL;
        }
        break;

    case PICT_x2r10g10b10:
        *no_alpha = 1;
    case PICT_a2r10g10b10:
        if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
            *tex_format = GL_BGRA;
            *tex_type = GL_UNSIGNED_INT_2_10_10_10_REV;
        } else {
            return FALSE;
        }
        break;

    case PICT_x2b10g10r10:
        *no_alpha = 1;
    case PICT_a2b10g10r10:
        if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_INT_2_10_10_10_REV;
        } else {
            return FALSE;
        }
        break;

    case PICT_r5g6b5:
        *tex_format = GL_RGB;
        *tex_type = GL_UNSIGNED_SHORT_5_6_5;
        if (glamor_priv->gl_flavor != GLAMOR_GL_DESKTOP)
            *revert = is_little_endian ? REVERT_NONE : REVERT_NORMAL;
        break;
    case PICT_b5g6r5:
        *tex_format = GL_RGB;
        if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
            *tex_type = GL_UNSIGNED_SHORT_5_6_5_REV;
        } else {
            *tex_type = GL_UNSIGNED_SHORT_5_6_5;
            if (is_little_endian)
                *swap_rb = SWAP_UPLOADING;
            *revert = is_little_endian ? REVERT_NONE : REVERT_NORMAL;
        }
        break;

    case PICT_x1b5g5r5:
        *no_alpha = 1;
    case PICT_a1b5g5r5:
        *tex_format = GL_RGBA;
        if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
            *tex_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        } else {
            return FALSE;
        }
        break;

    case PICT_x1r5g5b5:
        *no_alpha = 1;
    case PICT_a1r5g5b5:
        if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
            *tex_format = GL_BGRA;
            *tex_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        } else {
            return FALSE;
        }
        break;

    case PICT_a8:
        *tex_format = glamor_priv->one_channel_format;
        *tex_type = GL_UNSIGNED_BYTE;
        break;

    case PICT_x4r4g4b4:
        *no_alpha = 1;
    case PICT_a4r4g4b4:
        if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
            *tex_format = GL_BGRA;
            *tex_type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        } else {
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_SHORT_4_4_4_4;
            *revert = is_little_endian ? REVERT_NORMAL : REVERT_NONE;
            *swap_rb = SWAP_UPLOADING;
        }
        break;

    case PICT_x4b4g4r4:
        *no_alpha = 1;
    case PICT_a4b4g4r4:
        if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        } else {
            *tex_format = GL_RGBA;
            *tex_type = GL_UNSIGNED_SHORT_4_4_4_4;
            *revert = is_little_endian ? REVERT_NORMAL : REVERT_NONE;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

/**
 * Takes a set of source bits with a given format and returns an
 * in-memory pixman image of those bits in a destination format.
 */
static pixman_image_t *
glamor_get_converted_image(PictFormatShort dst_format,
                           PictFormatShort src_format,
                           void *src_bits,
                           int src_stride,
                           int w, int h)
{
    pixman_image_t *dst_image;
    pixman_image_t *src_image;

    dst_image = pixman_image_create_bits(dst_format, w, h, NULL, 0);
    if (dst_image == NULL) {
        return NULL;
    }

    src_image = pixman_image_create_bits(src_format, w, h, src_bits, src_stride);

    if (src_image == NULL) {
        pixman_image_unref(dst_image);
        return NULL;
    }

    pixman_image_composite(PictOpSrc, src_image, NULL, dst_image,
                           0, 0, 0, 0, 0, 0, w, h);

    pixman_image_unref(src_image);
    return dst_image;
}

/**
 * Upload pixmap to a specified texture.
 * This texture may not be the one attached to it.
 **/
static Bool
__glamor_upload_pixmap_to_texture(PixmapPtr pixmap, unsigned int *tex,
                                  GLenum format,
                                  GLenum type,
                                  int x, int y, int w, int h,
                                  void *bits)
{
    glamor_screen_private *glamor_priv =
        glamor_get_screen_private(pixmap->drawable.pScreen);
    int non_sub = 0;
    unsigned int iformat = 0;

    glamor_make_current(glamor_priv);
    if (*tex == 0) {
        glGenTextures(1, tex);
        if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP)
            iformat = gl_iformat_for_pixmap(pixmap);
        else
            iformat = format;
        non_sub = 1;
        assert(x == 0 && y == 0);
    }

    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glamor_priv->suppress_gl_out_of_memory_logging = true;
    if (non_sub)
        glTexImage2D(GL_TEXTURE_2D, 0, iformat, w, h, 0, format, type, bits);
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, format, type, bits);
    glamor_priv->suppress_gl_out_of_memory_logging = false;
    if (glGetError() == GL_OUT_OF_MEMORY) {
        if (non_sub) {
            glDeleteTextures(1, tex);
            *tex = 0;
        }
        return FALSE;
    }

    return TRUE;
}

static Bool
_glamor_upload_bits_to_pixmap_texture(PixmapPtr pixmap, GLenum format,
                                      GLenum type, int no_alpha, int revert,
                                      int swap_rb, int x, int y, int w, int h,
                                      int stride, void *bits)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    glamor_screen_private *glamor_priv =
        glamor_get_screen_private(pixmap->drawable.pScreen);
    float dst_xscale, dst_yscale;
    GLuint tex = 0;
    pixman_image_t *converted_image = NULL;

    if (revert == REVERT_UPLOADING_A1) {
        converted_image = glamor_get_converted_image(PICT_a8,
                                                     PICT_a1,
                                                     bits,
                                                     PixmapBytePad(w, 1),
                                                     w, h);
        if (!converted_image)
            return FALSE;

        bits = pixman_image_get_data(converted_image);
    }

    /* Try fast path firstly, upload the pixmap to the texture attached
     * to the fbo directly. */
    if (no_alpha == 0
        && revert == REVERT_NONE && swap_rb == SWAP_NONE_UPLOADING
#ifdef WALKAROUND_LARGE_TEXTURE_MAP
        && glamor_pixmap_priv_is_small(pixmap_priv)
#endif
        ) {
        int fbo_x_off, fbo_y_off;

        assert(pixmap_priv->fbo->tex);
        pixmap_priv_get_fbo_off(pixmap_priv, &fbo_x_off, &fbo_y_off);

        assert(x + fbo_x_off >= 0 && y + fbo_y_off >= 0);
        assert(x + fbo_x_off + w <= pixmap_priv->fbo->width);
        assert(y + fbo_y_off + h <= pixmap_priv->fbo->height);
        if (!__glamor_upload_pixmap_to_texture(pixmap, &pixmap_priv->fbo->tex,
                                               format, type,
                                               x + fbo_x_off, y + fbo_y_off,
                                               w, h,
                                               bits)) {
            if (converted_image)
                pixman_image_unref(bits);
            return FALSE;
        }
    } else {
        static const float texcoords_inv[8] = { 0, 0,
                                                1, 0,
                                                1, 1,
                                                0, 1
        };
        GLfloat *v;
        char *vbo_offset;

        v = glamor_get_vbo_space(screen, 16 * sizeof(GLfloat), &vbo_offset);

        pixmap_priv_get_dest_scale(pixmap, pixmap_priv, &dst_xscale, &dst_yscale);
        glamor_set_normalize_vcoords(pixmap_priv, dst_xscale,
                                     dst_yscale,
                                     x, y,
                                     x + w, y + h,
                                     v);
        /* Slow path, we need to flip y or wire alpha to 1. */
        glamor_make_current(glamor_priv);

        if (!__glamor_upload_pixmap_to_texture(pixmap, &tex,
                                               format, type, 0, 0, w, h, bits)) {
            if (converted_image)
                pixman_image_unref(bits);
            return FALSE;
        }

        memcpy(&v[8], texcoords_inv, 8 * sizeof(GLfloat));

        glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT,
                              GL_FALSE, 2 * sizeof(float), vbo_offset);
        glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
        glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2, GL_FLOAT,
                              GL_FALSE, 2 * sizeof(float), vbo_offset + 8 * sizeof(GLfloat));
        glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);

        glamor_put_vbo_space(screen);
        glamor_set_destination_pixmap_priv_nc(glamor_priv, pixmap, pixmap_priv);
        glamor_set_alu(screen, GXcopy);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glUseProgram(glamor_priv->finish_access_prog[no_alpha]);
        glUniform1i(glamor_priv->finish_access_revert[no_alpha], revert);
        glUniform1i(glamor_priv->finish_access_swap_rb[no_alpha], swap_rb);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
        glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
        glDeleteTextures(1, &tex);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    if (converted_image)
        pixman_image_unref(bits);
    return TRUE;
}

/*
 * Prepare to upload a pixmap to texture memory.
 * no_alpha equals 1 means the format needs to wire alpha to 1.
 */
static int
glamor_pixmap_upload_prepare(PixmapPtr pixmap, GLenum format, int no_alpha,
                             int revert, int swap_rb)
{
    int flag = 0;
    glamor_screen_private *glamor_priv =
        glamor_get_screen_private(pixmap->drawable.pScreen);
    GLenum iformat;

    if (!(no_alpha || (revert == REVERT_NORMAL)
          || (swap_rb != SWAP_NONE_UPLOADING))) {
        /* We don't need a fbo, a simple texture uploading should work. */

        flag = GLAMOR_CREATE_FBO_NO_FBO;
    }

    if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP)
        iformat = gl_iformat_for_pixmap(pixmap);
    else
        iformat = format;

    if (!glamor_pixmap_ensure_fbo(pixmap, iformat, flag))
        return -1;

    return 0;
}

/*
 * upload sub region to a large region.
 * */
static void
glamor_put_bits(char *dst_bits, int dst_stride, char *src_bits,
                int src_stride, int bpp, int x, int y, int w, int h)
{
    int j;
    int byte_per_pixel;

    byte_per_pixel = bpp / 8;
    src_bits += y * src_stride + (x * byte_per_pixel);

    for (j = y; j < y + h; j++) {
        memcpy(dst_bits, src_bits, w * byte_per_pixel);
        src_bits += src_stride;
        dst_bits += dst_stride;
    }
}

/**
 * Uploads a picture based on a GLAMOR_MEMORY pixmap to a texture in a
 * temporary FBO.
 */
Bool
glamor_upload_picture_to_texture(PicturePtr picture)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(picture->pDrawable);
    void *bits = pixmap->devPrivate.ptr;
    int stride = pixmap->devKind;
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    GLenum format, type;
    int no_alpha, revert, swap_rb;
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    Bool force_clip;

    assert(glamor_pixmap_is_memory(pixmap));
    assert(!pixmap_priv->fbo);

    if (!glamor_get_tex_format_type_from_pictformat(screen,
                                                    picture->format,
                                                    &format,
                                                    &type,
                                                    &no_alpha,
                                                    &revert, &swap_rb)) {
        glamor_fallback("Unknown pixmap depth %d.\n", pixmap->drawable.depth);
        return FALSE;
    }
    if (glamor_pixmap_upload_prepare(pixmap, format, no_alpha, revert, swap_rb))
        return FALSE;

    force_clip = glamor_priv->gl_flavor != GLAMOR_GL_DESKTOP;

    if (glamor_pixmap_priv_is_large(pixmap_priv) || force_clip) {
        RegionRec region;
        BoxRec box;
        int n_region;
        glamor_pixmap_clipped_regions *clipped_regions;
        void *sub_bits;
        int i, j;

        sub_bits = xallocarray(pixmap->drawable.height, stride);
        if (sub_bits == NULL)
            return FALSE;
        box.x1 = 0;
        box.y1 = 0;
        box.x2 = pixmap->drawable.width;
        box.y2 = pixmap->drawable.height;
        RegionInitBoxes(&region, &box, 1);
        if (!force_clip)
            clipped_regions =
                glamor_compute_clipped_regions(pixmap, &region, &n_region,
                                               0, 0, 0);
        else
            clipped_regions =
                glamor_compute_clipped_regions_ext(pixmap, &region,
                                                   &n_region,
                                                   pixmap_priv->block_w,
                                                   pixmap_priv->block_h,
                                                   0,
                                                   0);
        DEBUGF("prepare upload %dx%d to a large pixmap %p\n", w, h, pixmap);
        for (i = 0; i < n_region; i++) {
            BoxPtr boxes;
            int nbox;
            int temp_stride;
            void *temp_bits;

            glamor_set_pixmap_fbo_current(pixmap_priv, clipped_regions[i].block_idx);

            boxes = RegionRects(clipped_regions[i].region);
            nbox = RegionNumRects(clipped_regions[i].region);
            DEBUGF("split to %d boxes\n", nbox);
            for (j = 0; j < nbox; j++) {
                temp_stride = PixmapBytePad(boxes[j].x2 - boxes[j].x1,
                                            pixmap->drawable.depth);

                if (boxes[j].x1 == 0 && temp_stride == stride) {
                    temp_bits = (char *) bits + boxes[j].y1 * stride;
                }
                else {
                    temp_bits = sub_bits;
                    glamor_put_bits(temp_bits, temp_stride, bits, stride,
                                    pixmap->drawable.bitsPerPixel,
                                    boxes[j].x1, boxes[j].y1,
                                    boxes[j].x2 - boxes[j].x1,
                                    boxes[j].y2 - boxes[j].y1);
                }
                DEBUGF("upload x %d y %d w %d h %d temp stride %d \n",
                       boxes[j].x1, boxes[j].y1,
                       boxes[j].x2 - boxes[j].x1,
                       boxes[j].y2 - boxes[j].y1, temp_stride);
                if (!_glamor_upload_bits_to_pixmap_texture
                    (pixmap, format, type, no_alpha, revert, swap_rb,
                     boxes[j].x1, boxes[j].y1, boxes[j].x2 - boxes[j].x1,
                     boxes[j].y2 - boxes[j].y1, temp_stride, temp_bits)) {
                    RegionUninit(&region);
                    free(sub_bits);
                    assert(0);
                    return FALSE;
                }
            }
            RegionDestroy(clipped_regions[i].region);
        }
        free(sub_bits);
        free(clipped_regions);
        RegionUninit(&region);
        return TRUE;
    }
    else
        return _glamor_upload_bits_to_pixmap_texture(pixmap, format, type,
                                                     no_alpha, revert, swap_rb,
                                                     0, 0,
                                                     pixmap->drawable.width,
                                                     pixmap->drawable.height,
                                                     stride, bits);
}
